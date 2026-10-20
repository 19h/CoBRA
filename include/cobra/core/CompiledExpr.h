#pragma once
#include "cobra/core/Budget.h"

#include "cobra/core/Expr.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cobra {

    struct EvalInstr
    {
        Expr::Kind kind;
        uint64_t operand = 0;
    };

    struct CompiledExpr
    {
        uint32_t bitwidth = 64;
        uint64_t mask     = 0;
        uint32_t arity    = 0;
        size_t stack_size = 1;
        cobra::BudgetVector< EvalInstr > program;
    };

    CompiledExpr CompileExpr(const Expr &expr, uint32_t bitwidth);

    uint64_t EvalCompiledExpr(const CompiledExpr &compiled,
                              const cobra::BudgetVector< uint64_t > &var_values,
                              cobra::BudgetVector< uint64_t > &stack);

} // namespace cobra
