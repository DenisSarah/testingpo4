
#include "../include/utils.h"

std::string exprToString(const utils::Expr &expr,
                         const CodeRunner::Result &result) {
  std::string output = "";
  if (expr.Lhs) {
    output += exprToString(*expr.Lhs, result);
    output += " ";
    output += opToString(expr.Operator);
  }
  if (expr.SimpleExpr) {
    if (auto *val = std::get_if<utils::value>(&(*expr.SimpleExpr))) {
      output += " " + *val;
    } else if (auto *var =
                   std::get_if<utils::variable *>(&(*expr.SimpleExpr))) {
      auto it = std::ranges::find(result.Memory, (*var)->Name,
                                  &utils::variable::Name);
      const std::string &val = it->Value;
      output += val;
    }
  }
  return output;
}

CodeRunner::CodeRunner(utils::Func func) : Func_(std::move(func)) {
  if (!Func_.Body_.ReturnExpression.Lhs && !Func_.Body_.ReturnExpression.Rhs &&
      !Func_.Body_.ReturnExpression.SimpleExpr) {
    throw CodeRunner::Error{"где?"};
  }
}

void CodeRunner::Run(std::ostream &output) {
  auto results = RunInternal();
  for (const auto &res : results) {
    output << '{' << std::endl;
    for (const auto &var : res.Memory) {
      output << var.Name << " = " << var.Value << std::endl;
    }
    output << "output: " << res.Output << std::endl;
    output << "condition: " << res.Condition << std::endl;
    output << '}' << std::endl;
  }
}

std::vector<CodeRunner::Result> CodeRunner::RunInternal() {
  std::vector<CodeRunner::Result> results(1);
  for (const auto &input : Func_.Input) {
    results[0].Memory.push_back(utils::variable{
        .Type = input.Type,
        .Value = '\'' + input.Name + '\'',
        .Name = input.Name,
    });
  }
  results[0].Condition = "true";
  for (const auto &stmt : Func_.Body_.Statements) {
    RunStatement(results, *stmt);
  }
  for (auto &res : results) {
    res.Output = exprToString(Func_.Body_.ReturnExpression, res);
  }
  return results;
}

void CodeRunner::RunStatement(std::vector<Result> &current,
                              const utils::statement &stmt) {
  if (auto *assign = std::get_if<utils::DataType>(&stmt)) {
    for (auto &res : current) {
      auto it = std::ranges::find(res.Memory, assign->Var->Name,
                                  &utils::variable::Name);
      if (it == res.Memory.end()) {
        throw CodeRunner::Error{"где?"};
      }
      it->Value = exprToString(assign->Val, res);
    }
  } else if (auto *ifStmt = std::get_if<utils::Conditional>(&stmt)) {
    const auto &cond = ifStmt->Cond;
    auto currentCopy = current;
    for (auto &&res : current) {
      std::string condStr = exprToString(*cond, res);
      res.Condition += (" && (" + condStr + ")");
    }
    for (auto &&res : currentCopy) {
      std::string condStr = exprToString(*cond, res);
      res.Condition += (" && !(" + condStr + ")");
    }

    for (auto &&stmt : ifStmt->TrueBranch) {
      RunStatement(current, *stmt);
    }

    for (auto &&stmt : ifStmt->FalseBranch) {
      RunStatement(currentCopy, *stmt);
    }

    for (auto &&res : currentCopy) {
      current.emplace_back(res);
    }
  }
}
