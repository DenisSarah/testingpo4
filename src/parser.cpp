
#include "../include/utils.h"

struct InvalidChar {
  bool operator==([[maybe_unused]] const InvalidChar other) const {
    return true;
  }
};

struct Equals {
  bool operator==([[maybe_unused]] const Equals other) const { return true; }
};

struct Comma {
  bool operator==([[maybe_unused]] const Comma other) const { return true; }
};

struct Column {
  bool operator==([[maybe_unused]] const Column other) const { return true; }
};

struct Digit {
  char Value;
  bool operator==(const Digit &other) const { return Value == other.Value; }
};

struct Letter {
  char Value;
  bool operator==(const Letter &other) const { return Value == other.Value; }
};

struct Bracket {
  bool IsOpen;
  bool operator==(const Bracket &other) const { return IsOpen == other.IsOpen; }
};

struct CurveBracket {
  bool IsOpen;
  bool operator==(const CurveBracket &other) const {
    return IsOpen == other.IsOpen;
  }
};

enum class Operator {
  Add = 1,
  Subtract = 2,
  Greater = 3,
  Less = 4,
};

enum class Keyword {
  If = 1,
  Else = 2,
  Return = 3,
  True = 4,
  False = 5,
};

enum class Type {
  Int = 1,
  Bool = 2,
};

enum class Delim {
  Space = 1,
  Newline = 2,
};

using TokenVariant =
    std::variant<Equals, Comma, Column, InvalidChar, Digit, Letter, Bracket,
                 CurveBracket, Operator, Keyword, Type, Delim>;

utils::Operation ConvertOp(Operator op) {
  switch (op) {
  case Operator::Add:
    return utils::Operation::Plus;
  case Operator::Subtract:
    return utils::Operation::Minus;
  case Operator::Less:
    return utils::Operation::Less;
  case Operator::Greater:
    return utils::Operation::Greater;
  }
  return utils::Operation::UnknownOperator;
}

std::optional<Keyword> GetKeyword(const std::string_view name) {
  if (name == "if") {
    return Keyword::If;
  } else if (name == "else") {
    return Keyword::Else;
  } else if (name == "return") {
    return Keyword::Return;
  } else if (name == "true") {
    return Keyword::True;
  } else if (name == "false") {
    return Keyword::False;
  }
  return std::nullopt;
}

std::optional<Type> GetType(const std::string_view name) {
  if (name == "int") {
    return Type::Int;
  } else if (name == "bool") {
    return Type::Bool;
  }
  return std::nullopt;
}

struct Parser::Token : public TokenVariant {
  using TokenVariant::TokenVariant;
};

void Parser::PutBackToken() { InputFile_.unget(); }

Parser::Token Parser::NextToken() {
  if (!InputFile_) {
    throw Error("ошибка чтения");
  }

  char c = InputFile_.get();

  if (std::isdigit(c)) {
    return {Digit{.Value = c}};
  }

  if (std::isalpha(c) && std::islower(c)) {
    return {Letter{.Value = c}};
  }

  switch (c) {
  case ' ':
    return {Delim::Space};
  case '\n':
    return {Delim::Newline};
  case '(':
    return {Bracket{.IsOpen = true}};
  case ')':
    return {Bracket{.IsOpen = false}};
  case '{':
    return {CurveBracket{.IsOpen = true}};
  case '}':
    return {CurveBracket{.IsOpen = false}};
  case '+':
    return {Operator::Add};
  case '-':
    return {Operator::Subtract};
  case '>':
    return {Operator::Greater};
  case '<':
    return {Operator::Less};
  case ',':
    return {Comma{}};
  case ':':
    return {Column{}};
  case '=':
    return {Equals{}};
  }

  return {InvalidChar{}};
}

Parser::Token Parser::NextTokenSkipWhitespace() {
  auto token = NextToken();
  for (; std::holds_alternative<Delim>(token); token = NextToken()) {
  }
  return token;
}

std::string Parser::ReadValue() {
  auto token = NextTokenSkipWhitespace();
  std::string value;

  for (; Digit *d = std::get_if<Digit>(&token); token = NextToken()) {
    value += d->Value;
  }

  PutBackToken();
  return value;
}

std::variant<std::string, Parser::Token> Parser::ReadWord() {
  auto token = NextTokenSkipWhitespace();

  std::string name;

  for (; Letter *l = std::get_if<Letter>(&token); token = NextToken()) {
    name += l->Value;
  }

  if (name.empty()) {
    throw Error("где?");
  }

  PutBackToken();

  if (auto keyword = GetKeyword(name)) {
    return *keyword;
  }

  if (auto type = GetType(name)) {
    return *type;
  }

  return name;
}

Parser::Parser(const std::string &filePath) : InputFile_(filePath) {
  if (!InputFile_) {
    throw Error("ошибка чтения");
  }
}

utils::Func Parser::Parse() { return ParseFunction(); }

utils::Func Parser::ParseFunction() {
  auto word = ReadWord();
  std::string name;
  if (auto *str = std::get_if<std::string>(&word)) {
    name = std::move(*str);
  } else {
    throw Error("ошибка в названии функции");
  }

  auto token = NextTokenSkipWhitespace();

  if (Bracket *p = std::get_if<Bracket>(&token); (!p || !p->IsOpen)) {
    throw Error("проблема с аргументами");
  }

  std::vector<utils::variable> params;

  for (Parser::Token t = {Comma{}}; std::get_if<Comma>(&t);
       t = NextTokenSkipWhitespace()) {
    word = ReadWord();
    utils::variable var;
    if (auto *tok = std::get_if<Parser::Token>(&word)) {
      if (auto *type = std::get_if<Type>(tok)) {
        var.Type =
            (*type == Type::Int) ? utils::types::Int : utils::types::Bool;
      }
    }

    word = ReadWord();

    if (auto *varName = std::get_if<std::string>(&word)) {
      var.Name = std::move(*varName);
    }

    params.emplace_back(std::move(var));
  }

  PutBackToken();

  token = NextTokenSkipWhitespace();

  if (Bracket *p = std::get_if<Bracket>(&token); !p) {
    throw Error("проблема с аргументами");
  }

  token = NextTokenSkipWhitespace();

  if (Column *c = std::get_if<Column>(&token); !c) {
    throw Error("проблема с аргументами");
  }

  utils::types returnType;

  word = ReadWord();

  if (auto *tok = std::get_if<Parser::Token>(&word)) {
    if (auto *type = std::get_if<Type>(tok)) {
      returnType =
          (*type == Type::Int) ? utils::types::Int : utils::types::Bool;
    }
  }

  utils::Func result{.Name = std::move(name),
                     .Input = std::move(params),
                     .ReturnType = returnType,
                     .Body_ = {}};

  ParseFunctionBody(result);

  return result;
}

void Parser::ParseFunctionBody(utils::Func &func) {
  auto token = NextTokenSkipWhitespace();
  if (CurveBracket *b = std::get_if<CurveBracket>(&token); (!b || !b->IsOpen)) {
    throw Error("потеряли {");
  }

  utils::Body Body;
  while (true) {
    auto word = ReadWord();
    if (auto *name = std::get_if<std::string>(&word)) {
      Body.Statements.emplace_back(
          std::make_unique<utils::statement>(ParseAssignSt(func, *name)));
    } else if (auto *tok = std::get_if<Parser::Token>(&word)) {
      if (auto *keyword = std::get_if<Keyword>(tok); !keyword) {
        throw Error("бред написан");
      } else {
        if (*keyword == Keyword::If) {
          Body.Statements.emplace_back(
              std::make_unique<utils::statement>(ParseIfSt(func)));
        } else if (*keyword == Keyword::Return) {
          Body.ReturnExpression = ParseExpression(func);
        }
      }
    } else {
      throw Error("ошибка");
    }

    auto nextToken = NextTokenSkipWhitespace();
    if (CurveBracket *cb = std::get_if<CurveBracket>(&nextToken);
        (cb && !cb->IsOpen)) {
      break;
    } else {
      PutBackToken();
    }
  }

  func.Body_ = std::move(Body);
}

utils::DataType Parser::ParseAssignSt(utils::Func &func,
                                      const std::string &varName) {
  auto it = std::ranges::find(func.Input, varName, &utils::variable::Name);

  if (it == func.Input.end()) {
    std::stringstream ss;
    ss << "Unknown variable " << varName << " in Func " << func.Name;
    throw Error(ss.str());
  }

  auto token = NextTokenSkipWhitespace();

  if (auto *equals = std::get_if<Equals>(&token); !equals) {
    throw Error("потеряли =");
  }

  auto Expr = ParseExpression(func);

  return utils::DataType{.Var = &(*it), .Val = std::move(Expr)};
}

utils::Conditional Parser::ParseIfSt(utils::Func &func) {
  auto token = NextTokenSkipWhitespace();

  if (Bracket *p = std::get_if<Bracket>(&token); (!p || !p->IsOpen)) {
    throw Error("потеряли (");
  }

  utils::Expr condition = ParseExpression(func);
  token = NextTokenSkipWhitespace();

  if (Bracket *p = std::get_if<Bracket>(&token); (!p || p->IsOpen)) {
    throw Error("потеряли )");
  }

  utils::Func trueBranch = utils::CopyExceptBody(func);
  ParseFunctionBody(trueBranch);

  auto word = ReadWord();

  auto *tok = std::get_if<Parser::Token>(&word);
  if (!tok) {
    throw Error("Потеряли else");
  }

  if (auto *keyword = std::get_if<Keyword>(tok);
      !keyword || (*keyword != Keyword::Else)) {
    throw Error("Потеряли else");
  }

  utils::Func falseBranch = utils::CopyExceptBody(func);
  ParseFunctionBody(falseBranch);

  return utils::Conditional{
      .Cond = std::make_unique<utils::Expr>(std::move(condition)),
      .TrueBranch = std::move(trueBranch.Body_.Statements),
      .FalseBranch = std::move(falseBranch.Body_.Statements)};
}

utils::Expr Parser::ParseExpression(utils::Func &func) {
  utils::Expr result;
  Parser::Token lastToken = Operator::Add;
  bool isFirst = true;
  for (; (std::holds_alternative<Operator>(lastToken));) {
    utils::Expr current;
    auto res = ParseSimpleExpression(func);
    lastToken = res.second;
    current.SimpleExpr = std::move(res.first.SimpleExpr);
    if (!isFirst) {
      current.Lhs = std::make_unique<utils::Expr>(std::move(result));
    }
    if (std::holds_alternative<Operator>(lastToken)) {
      current.Operator = ConvertOp(std::get<Operator>(lastToken));
    }
    result = std::move(current);
    isFirst = false;
  }
  PutBackToken();
  return result;
}

std::pair<utils::Expr, Parser::Token>
Parser::ParseSimpleExpression(utils::Func &func) {
  auto check = [this]() {
    auto token = NextTokenSkipWhitespace();
    return token;
  };

  std::string value = ReadValue();
  if (!value.empty()) {
    return {utils::Expr{.Rhs = {},
                        .Lhs = {},
                        .Operator = {},
                        .SimpleExpr = std::optional(value),
                        .ReturnType = utils::types::Int},
            check()};
  }

  auto word = ReadWord();
  if (auto *name = std::get_if<std::string>(&word); name->size()) {
    auto it = std::ranges::find(func.Input, *name, &utils::variable::Name);
    if (it == func.Input.end()) {
      throw Error("бред написан");
    }

    return {utils::Expr{.Rhs = {},
                        .Lhs = {},
                        .Operator = {},
                        .SimpleExpr = std::optional(&(*it)),
                        .ReturnType = it->Type},
            check()};
  }

  if (auto *tok = std::get_if<Parser::Token>(&word)) {
    if (auto *keyword = std::get_if<Keyword>(tok)) {
      if (*keyword == Keyword::True) {
        check();
        return {utils::Expr{.Rhs = {},
                            .Lhs = {},
                            .Operator = {},
                            .SimpleExpr = std::optional("true"),
                            .ReturnType = utils::types::Bool},
                check()};
      } else if (*keyword == Keyword::False) {
        check();
        return {utils::Expr{.Rhs = {},
                            .Lhs = {},
                            .Operator = {},
                            .SimpleExpr = std::optional("false"),
                            .ReturnType = utils::types::Bool},
                check()};
      }
    }
    throw Error("бред написан");
  }
  throw Error("");
}

bool Parser::IsValidValue(const std::string &value) {
  return value == "true" || value == "false" ||
         std::all_of(value.begin(), value.end(),
                     [](char c) { return std::isdigit(c); });
}
