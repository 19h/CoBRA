#pragma once
#include "cobra/core/Budget.h"

#include "cobra/core/SemilinearIR.h"
#include <vector>

namespace cobra {

    cobra::BudgetVector< PartitionClass > ComputePartitions(const SemilinearIR &ir);

} // namespace cobra
