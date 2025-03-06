#pragma once
#include <bits/stdc++.h>

namespace utils {
enum class types {
  UnknownType = 0,
  Int = 1,
  Bool = 2,
};

enum class Operation {
  UnknownOperator = 0,
  Plus = 1,
  Minus = 2,
  Greater = 3,
  Less = 4,
};

struct variable {
  types Type;
  std::string Value;
  std::string Name;
};

struct Expr {
  std::unique_ptr<Expr> Rhs;
  std::unique_ptr<Expr> Lhs;
  Operation Operator;
  std::optional<std::variant<std::string, variable *>> SimpleExpr;
  types ReturnType;
};

class statement;

struct DataType {
  variable *Var;
  Expr Val;
};

struct Conditional {
  std::unique_ptr<Expr> Cond;
  std::vector<std::unique_ptr<statement>> TrueBranch;
  std::vector<std::unique_ptr<statement>> FalseBranch;
};

using value = std::string;
struct statement : public std::variant<DataType, Conditional> {};

struct Body {
  std::vector<std::unique_ptr<statement>> Statements;
  Expr ReturnExpression;
};

struct Func {
  std::string Name;
  std::vector<variable> Input;
  types ReturnType;
  Body Body_;
};

inline Func CopyExceptBody(const Func &other) {
  return {.Name = other.Name,
          .Input = other.Input,
          .ReturnType = other.ReturnType,
          .Body_ = {}};
}
} // namespace utils

inline std::string opToString(const utils::Operation &operation) {
  switch (operation) {
  case utils::Operation::Plus:
    return "+";
  case utils::Operation::Minus:
    return "-";
  case utils::Operation::Less:
    return "<";
  case utils::Operation::Greater:
    return ">";
  case utils::Operation::UnknownOperator:
    return "unknown_operator";
  default:
    return "unknown_operator";
  }
}

class Parser {
public:
  Parser(const std::string &filePath);
  utils::Func Parse();
  class Error : public std::runtime_error {
    using std::runtime_error::runtime_error;
  };

private:
  struct Token;
  Token NextToken();
  Token NextTokenSkipWhitespace();
  void PutBackToken();
  std::variant<std::string, Token> ReadWord();
  std::string ReadValue();
  utils::DataType ParseAssignSt(utils::Func &func, const std::string &varName);
  utils::Conditional ParseIfSt(utils::Func &func);
  utils::Func ParseFunction();
  std::vector<utils::variable> ParseFunctionParams();
  void ParseFunctionBody(utils::Func &func);
  utils::statement ParseStatement();
  utils::Expr ParseExpression(utils::Func &func);
  std::pair<utils::Expr, Token> ParseSimpleExpression(utils::Func &func);
  static bool IsValidValue(const std::string &value);
  std::ifstream InputFile_;
};

class CodeRunner {
public:
  CodeRunner(utils::Func func);
  void Run(std::ostream &output);
  struct Result {
    std::vector<utils::variable> Memory;
    std::string Condition;
    std::string Output;
  };
  class Error : public std::runtime_error {
    using std::runtime_error::runtime_error;
  };

private:
  std::vector<Result> RunInternal();
  void RunStatement(std::vector<Result> &current,
                    const utils::statement &statement);
  utils::Func Func_;
};
