#include "MaskedLeafLift.h"
#include "cobra/core/ExprCost.h"
#include "cobra/core/ExprUtils.h"
#include "cobra/core/SignatureChecker.h"
#include "cobra/core/SignatureEval.h"
#include <algorithm>

namespace cobra {
    namespace {
        bool MaskedLeaf(const Expr &node) {
            if (node.kind != Expr::Kind::kAnd || node.children.size() != 2) { return false; }
            const auto a = node.children[0]->kind, b = node.children[1]->kind;
            return (a == Expr::Kind::kVariable && b == Expr::Kind::kConstant)
                || (b == Expr::Kind::kVariable && a == Expr::Kind::kConstant);
        }

        void Collect(const Expr &node, BudgetVector< const Expr * > &atoms) {
            budget::Checkpoint();
            if (MaskedLeaf(node)) {
                for (const auto *atom : atoms) {
                    if (ExprStructurallyEqual(node, *atom)) { return; }
                }
                atoms.push_back(&node);
                return;
            }
            for (const auto &child : node.children) { Collect(*child, atoms); }
        }

        std::unique_ptr< Expr >
        Lift(const Expr &node, const BudgetVector< const Expr * > &atoms, uint32_t first) {
            budget::Checkpoint();
            for (size_t i = 0; i < atoms.size(); ++i) {
                if (ExprStructurallyEqual(node, *atoms[i])) {
                    return Expr::Variable(first + static_cast< uint32_t >(i));
                }
            }
            auto result          = std::make_unique< Expr >();
            result->kind         = node.kind;
            result->constant_val = node.constant_val;
            result->var_index    = node.var_index;
            for (const auto &child : node.children) {
                result->children.push_back(Lift(*child, atoms, first));
            }
            return result;
        }

        std::unique_ptr< Expr > Restore(const Expr &node,
                                        const BudgetVector< std::string > &compact,
                                        const BudgetVector< std::string > &all,
                                        const BudgetVector< const Expr * > &atoms,
                                        uint32_t first) {
            budget::Checkpoint();
            if (node.kind == Expr::Kind::kVariable) {
                if (node.var_index >= compact.size()) { return {}; }
                auto found = std::find(all.begin(), all.end(), compact[node.var_index]);
                if (found == all.end()) { return {}; }
                const auto index = static_cast< uint32_t >(found - all.begin());
                return index < first ? Expr::Variable(index)
                                     : CloneExpr(*atoms.at(index - first));
            }
            auto result          = std::make_unique< Expr >();
            result->kind         = node.kind;
            result->constant_val = node.constant_val;
            result->var_index    = node.var_index;
            for (const auto &child : node.children) {
                auto restored = Restore(*child, compact, all, atoms, first);
                if (!restored) { return {}; }
                result->children.push_back(std::move(restored));
            }
            return result;
        }
    } // namespace

    std::optional< SimplifyOutcome > TryMaskedLeafLift(const Expr &input,
                                                       const BudgetVector< std::string > &vars,
                                                       const Options &options) {
        budget::Checkpoint(true);
        BudgetVector< const Expr * > atoms;
        Collect(input, atoms);
        if (atoms.empty() || vars.size() + atoms.size() > options.max_vars) {
            return std::nullopt;
        }
        BudgetVector< std::string > names = vars;
        for (size_t i = 0; i < atoms.size(); ++i) {
            budget::Checkpoint();
            std::string name = "masked_" + std::to_string(i);
            while (std::find(names.begin(), names.end(), name) != names.end()) {
                budget::Checkpoint();
                name += "_";
            }
            names.push_back(std::move(name));
        }
        const auto first = static_cast< uint32_t >(vars.size());
        auto lifted      = Lift(input, atoms, first);
        auto signature   = EvaluateBooleanSignature(
            *lifted, static_cast< uint32_t >(names.size()), options.bitwidth);
        auto inner_options      = options;
        inner_options.evaluator = Evaluator{};
        auto solved             = Simplify(signature, names, lifted.get(), inner_options);
        if (!solved || solved->kind != SimplifyOutcome::Kind::kSimplified || !solved->expr
            || !solved->verified)
        {
            return std::nullopt;
        }
        auto restored = Restore(*solved->expr, solved->real_vars, names, atoms, first);
        if (!restored || !IsBetter(ComputeCost(*restored).cost, ComputeCost(input).cost)) {
            return std::nullopt;
        }
        const auto checked = FullWidthCheckEval(Evaluator::FromExpr(input, options.bitwidth),
                                                first, *restored, options.bitwidth);
        if (!checked.passed) { return std::nullopt; }
        auto outcome       = std::move(*solved);
        outcome.expr       = std::move(restored);
        outcome.real_vars  = vars;
        outcome.verified   = true;
        outcome.sig_vector = EvaluateBooleanSignature(*outcome.expr, first, options.bitwidth);
        budget::Checkpoint(true);
        return outcome;
    }
} // namespace cobra
