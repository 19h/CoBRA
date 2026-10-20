#pragma once
#include "cobra/core/Budget.h"
#include "cobra/core/Expr.h"
#include "cobra/core/SemilinearIR.h"
#include <string>
#include <vector>

namespace cobra {

    struct SelfCheckResult
    {
        bool passed;
        std::string mismatch_detail;
    };

    SelfCheckResult SelfCheckSemilinear(const SemilinearIR &original_ir,
                                        const Expr &reconstructed,
                                        const cobra::BudgetVector< std::string > &vars,
                                        uint32_t bitwidth);

} // namespace cobra
