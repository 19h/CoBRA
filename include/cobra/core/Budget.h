#pragma once

#include <absl/container/flat_hash_map.h>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace cobra::budget {
    using Clock = std::chrono::steady_clock;
    enum class Reason { Deadline, Cancellation, Allocation, Callback };

    struct Exhausted
    { Reason reason; };

    inline size_t CheckedProduct(size_t left, size_t right) {
        if (right && left > std::numeric_limits< size_t >::max() / right) {
            throw Exhausted{ Reason::Allocation };
        }
        return left * right;
    }

    struct Ledger
    {
        size_t limit;
        size_t live = 0;
        size_t peak = 0;

        explicit Ledger(size_t bytes) : limit(bytes) {}

        void acquire(size_t bytes) {
            if (bytes > limit - live) { throw Exhausted{ Reason::Allocation }; }
            live += bytes;
            peak  = std::max(peak, live);
        }

        void release(size_t bytes) noexcept { live -= bytes; }
    };

    struct Context
    {
        Clock::time_point deadline = Clock::time_point::max();
        std::function< bool() > cancelled;
        std::function< Clock::time_point() > now;
        std::shared_ptr< Ledger > ledger;
        size_t ticks = 0;
    };

    inline thread_local Context *current = nullptr;

    class Scope
    {
        Context context_;
        Context *previous_;

      public:
        explicit Scope(Context context) : context_(std::move(context)), previous_(current) {
            if (previous_) {
                context_.deadline  = std::min(context_.deadline, previous_->deadline);
                context_.ledger    = previous_->ledger;
                auto parent        = previous_->cancelled;
                auto child         = context_.cancelled;
                context_.cancelled = [parent, child] {
                    return (parent && parent()) || (child && child());
                };
                if (previous_->now) { context_.now = previous_->now; }
            }
            current = &context_;
        }

        ~Scope() { current = previous_; }

        Scope(const Scope &)            = delete;
        Scope &operator=(const Scope &) = delete;
    };

    inline void Checkpoint(bool force = false) {
        if (!current || (!force && (++current->ticks % 64) != 0)) { return; }
        bool cancel = false;
        try {
            cancel = current->cancelled && current->cancelled();
        } catch (...) { throw Exhausted{ Reason::Callback }; }
        const auto time = current->now ? current->now() : Clock::now();
        if (cancel) { throw Exhausted{ Reason::Cancellation }; }
        if (time >= current->deadline) { throw Exhausted{ Reason::Deadline }; }
    }

    template< class T >
    struct Allocator
    {
        using value_type                             = T;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_swap            = std::true_type;
        std::shared_ptr< Ledger > ledger;

        Allocator() noexcept : ledger(current ? current->ledger : nullptr) {}

        template< class U >
        Allocator(const Allocator< U > &other) noexcept : ledger(other.ledger) {}

        T *allocate(size_t count) {
            Checkpoint(true);
            if (count > std::numeric_limits< size_t >::max() / sizeof(T)) {
                throw Exhausted{ Reason::Allocation };
            }
            const size_t bytes = count * sizeof(T);
            if (ledger) { ledger->acquire(bytes); }
            try {
                return std::allocator< T >{}.allocate(count);
            } catch (...) {
                if (ledger) { ledger->release(bytes); }
                throw;
            }
        }

        void deallocate(T *pointer, size_t count) noexcept {
            std::allocator< T >{}.deallocate(pointer, count);
            if (ledger) { ledger->release(count * sizeof(T)); }
        }

        template< class U >
        bool operator==(const Allocator< U > &other) const noexcept {
            return ledger == other.ledger;
        }

        template< class U >
        bool operator!=(const Allocator< U > &other) const noexcept {
            return !(*this == other);
        }
    };

    // Account AST payloads without replacing process-wide allocation operators.
    struct alignas(std::max_align_t) ObjectHeader
    {
        std::shared_ptr< Ledger > ledger;
        size_t bytes;
    };

    inline void *AllocateObject(size_t bytes) {
        Checkpoint(true);
        if (bytes > std::numeric_limits< size_t >::max() - sizeof(ObjectHeader)) {
            throw Exhausted{ Reason::Allocation };
        }
        auto ledger = current ? current->ledger : nullptr;
        if (ledger) { ledger->acquire(bytes); }
        void *storage = nullptr;
        try {
            storage = ::operator new(sizeof(ObjectHeader) + bytes);
        } catch (...) {
            if (ledger) { ledger->release(bytes); }
            throw;
        }
        auto *header = new (storage) ObjectHeader{ std::move(ledger), bytes };
        return header + 1;
    }

    inline void FreeObject(void *pointer) noexcept {
        if (!pointer) { return; }
        auto *header = static_cast< ObjectHeader * >(pointer) - 1;
        if (header->ledger) { header->ledger->release(header->bytes); }
        header->~ObjectHeader();
        ::operator delete(header);
    }
} // namespace cobra::budget

namespace cobra {
    template< class T >
    using BudgetVector = std::vector< T, budget::Allocator< T > >;
    template< class K, class V, class Hash = std::hash< K >, class Eq = std::equal_to< K > >
    using BudgetMap =
        std::unordered_map< K, V, Hash, Eq, budget::Allocator< std::pair< const K, V > > >;
    template< class K, class Hash = std::hash< K >, class Eq = std::equal_to< K > >
    using BudgetSet = std::unordered_set< K, Hash, Eq, budget::Allocator< K > >;
    template<
        class K, class V,
        class Hash = typename absl::container_internal::FlatHashMapPolicy< K, V >::DefaultHash,
        class Eq   = typename absl::container_internal::FlatHashMapPolicy< K, V >::DefaultEq >
    // Budget allocation and cancellation throw during container growth.
    // Abseil flat_hash_map explicitly has no exception-safety guarantee;
    // use the standard node container so unwinding retains valid ownership.
    using BudgetFlatMap =
        std::unordered_map< K, V, Hash, Eq, budget::Allocator< std::pair< const K, V > > >;
} // namespace cobra
