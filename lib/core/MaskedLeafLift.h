#pragma once
#include "cobra/core/Simplifier.h"
#include <optional>

namespace cobra {
    std::optional< SimplifyOutcome > TryMaskedLeafLift(const Expr &input,
                                                       const BudgetVector< std::string > &vars,
                                                       const Options &options);
}
