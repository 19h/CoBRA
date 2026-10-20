#pragma once
#include "cobra/core/Budget.h"

#include "cobra/core/Expr.h"
#include "cobra/core/SemilinearIR.h"
#include <memory>
#include <vector>

namespace cobra {

    std::unique_ptr< Expr >
    ReconstructMaskedAtoms(const SemilinearIR &ir,
                           const cobra::BudgetVector< PartitionClass > &partitions);

} // namespace cobra
