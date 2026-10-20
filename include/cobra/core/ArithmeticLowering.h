#pragma once
#include "cobra/core/Budget.h"

#include "cobra/core/PolyIR.h"
#include "cobra/core/Result.h"
#include <vector>

namespace cobra {

    struct LoweringResult
    {
        PolyIR poly;
        cobra::BudgetVector< Coeff > residual_and_coeffs;
    };

    Result< LoweringResult >
    LowerArithmeticFragment(const cobra::BudgetVector< Coeff > &and_coeffs,
                            const cobra::BudgetVector< Coeff > &mul_coeffs, uint8_t num_vars,
                            uint32_t bitwidth);

} // namespace cobra
