#pragma once
#include "cobra/core/Budget.h"

#include "cobra/core/Evaluator.h"
#include <cstdint>
#include <string>
#include <vector>

namespace cobra {

    struct EliminationResult
    {
        cobra::BudgetVector< uint64_t > reduced_sig;
        cobra::BudgetVector< std::string > real_vars;
        cobra::BudgetVector< std::string > spurious_vars;
    };

    // Detect and remove spurious variables from a signature vector.
    // A variable is spurious iff toggling it never changes the paired
    // signature entries across all assignments of remaining variables.
    EliminationResult EliminateAuxVars(const cobra::BudgetVector< uint64_t > &sig,
                                       const cobra::BudgetVector< std::string > &vars);

    // Enhanced overload: after boolean spurious detection, probes the
    // evaluator at random full-width inputs to catch variables that are
    // spurious on {0,1} but live at full width (e.g., x*y vs x&y).
    // The evaluator must operate in the original variable space.
    EliminationResult EliminateAuxVars(const cobra::BudgetVector< uint64_t > &sig,
                                       const cobra::BudgetVector< std::string > &vars,
                                       const Evaluator &eval, uint32_t bitwidth);

} // namespace cobra
