#pragma once
#include "cobra/core/Budget.h"

#include "cobra/core/Expr.h"
#include <cstdint>
#include <memory>
#include <vector>

namespace cobra {

    // Build an optimized Expr from CoB coefficients.
    // Applies greedy bitwise rewriting (OR, XOR, NOT recognition).
    // All arithmetic is modulo 2^bitwidth.
    std::unique_ptr< Expr > BuildCobExpr(const cobra::BudgetVector< uint64_t > &coeffs,
                                         uint32_t num_vars, uint32_t bitwidth);

} // namespace cobra
