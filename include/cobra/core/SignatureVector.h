#pragma once
#include "cobra/core/Budget.h"

#include <cstdint>
#include <vector>

namespace cobra {

    class SignatureVector
    {
      public:
        SignatureVector(uint32_t num_vars, uint32_t bitwidth);

        uint32_t NumVars() const { return num_vars_; }

        uint32_t Bitwidth() const { return bitwidth_; }

        size_t Length() const { return 1ULL << num_vars_; }

        // Construct from pre-computed values (CLI parser evaluates the expression)
        cobra::BudgetVector< uint64_t >
        FromValues(cobra::BudgetVector< uint64_t > values) const;

      private:
        uint32_t num_vars_;
        uint32_t bitwidth_;
        uint64_t mask_;
    };

} // namespace cobra
