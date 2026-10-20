#pragma once
#include "cobra/core/Budget.h"

#include "cobra/core/Expr.h"
#include "cobra/core/ExprCost.h"
#include "cobra/core/PassContract.h"
#include "cobra/core/Simplifier.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cobra {

    struct SignaturePayload
    {
        std::unique_ptr< Expr > expr;
        ExprCost cost;
        VerificationState verification = VerificationState::kUnverified;
        cobra::BudgetVector< std::string > real_vars;
    };

    struct SignatureContext
    {
        cobra::BudgetVector< std::string > vars;
        cobra::BudgetVector< uint32_t > original_indices;
        std::optional< Evaluator > eval;
    };

    bool IsBooleanValued(const cobra::BudgetVector< uint64_t > &sig);

} // namespace cobra
