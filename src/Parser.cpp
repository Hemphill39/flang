#include "Parser.hpp"

const std::optional<std::shared_ptr<ScriptAstNode>> Parser::parseScript() noexcept {

  // we're assuming were before the start of the tape,
  // move the tape to the first token
  this->tokenizer->nextToken();

  bool error = false;
  std::vector<std::shared_ptr<StatementAstNode>> statements;

  while (true) {
    this->skipWhiteSpace();

    TokenType t = this->tokenizer->currentToken()->tokenType;

    if (t == TokenType::EndOfFile) {
      break;
    }

    auto statement = this->parseStatement();

    if (statement) {
      statements.push_back(statement.value());

    } else {
      error = true;
    }
  }

  if (error) { return std::nullopt; }

  return std::make_shared<ScriptAstNode>(statements);
}

const std::optional<std::shared_ptr<StatementAstNode>> Parser::parseStatement() noexcept {
  this->skipWhiteSpace();

  std::optional<std::shared_ptr<StatementAstNode>> s;

  switch (this->tokenizer->currentToken()->tokenType)  {
    case TokenType::Var:
      s = this->parseDeclareStatement();
      this->skipWhiteSpace();
      return s;
    case TokenType::If:
      s = this->parseIfStatement();
      this->skipWhiteSpace();
      return s;
    case TokenType::While:
      s = this->parseWhileStatement();
      this->skipWhiteSpace();
      return s;
    case TokenType::Break:
      s = this->parseBreakStatement();
      this->skipWhiteSpace();
      return s;
    case TokenType::Return:
      s = this->parseReturnStatement();
      this->skipWhiteSpace();
      return s;
    case TokenType::LeftCurly:
      s = this->parseBlockStatement();
      this->skipWhiteSpace();
      return s;
    default:
      s = this->parseAssignStatement();
      this->skipWhiteSpace();
      return s;
  }
}

const std::optional<std::shared_ptr<IfStatementAstNode>> Parser::parseIfStatement() noexcept {
  bool error = false;

  this->expect(TokenType::If, error, "Expected an if");

  this->skipWhiteSpace();

  this->expect(TokenType::LeftParen, error, "Expected a left paren after if");

  auto expression = this->parseExpression();

  this->expect(TokenType::RightParen, error, "Expected a right paren after expression of if");

  auto ifStatement = this->parseStatement();

  bool foundElse = false;
  std::optional<std::shared_ptr<StatementAstNode>> elseStatement{std::nullopt};

  if (this->tokenizer->currentToken()->tokenType == TokenType::Else) {
    foundElse = true;
    this->expect(TokenType::Else, error, "Expected an else");

    elseStatement = this->parseStatement();
  }

  if (foundElse) {
    error = error || !expression || !ifStatement || !elseStatement;

    if (error) { return std::nullopt; }

    return std::make_shared<IfStatementAstNode>(
      expression.value(), ifStatement.value(), elseStatement.value());
  }

  // else, we did not find an else

  error = error || !expression || !ifStatement;

  if (error) { return std::nullopt; }

  return std::make_shared<IfStatementAstNode>(expression.value(), ifStatement.value());
}

const std::optional<std::shared_ptr<DeclareStatementAstNode>> Parser::parseDeclareStatement() noexcept {
  bool error = false;

  this->expect(TokenType::Var, error, "Expected a var");

  this->expect(TokenType::WhiteSpace, error, "Expected whitespace following var declaration");

  this->skipWhiteSpace();

  auto identifier = this->expect(TokenType::Identifier, error, "Expected an identifier following var declaration");

  this->skipWhiteSpace();

  this->expect(TokenType::Assign, error, "Expected an equals following var declaration");

  auto expression = this->parseExpression();

  this->expect(TokenType::SemiColon, error, "Expected a semicolon following expression of var declaration");

  error = error || !identifier || !expression;

  if (error) { return std::nullopt; }

  return std::make_shared<DeclareStatementAstNode>(identifier, expression.value());
}

const std::optional<std::shared_ptr<WhileStatementAstNode>> Parser::parseWhileStatement() noexcept {
  bool error = false;

  this->expect(TokenType::While, error, "Expected a while");

  this->skipWhiteSpace();

  this->expect(TokenType::LeftParen, error, "Expected a left paren after while");

  auto expression = this->parseExpression();

  this->expect(TokenType::RightParen, error, "Expected a right paren after expression of while");

  auto statement = this->parseStatement();

  error = error || !expression || !statement;

  if (error) { return std::nullopt; }

  return std::make_shared<WhileStatementAstNode>(expression.value(), statement.value());
}

const std::optional<std::shared_ptr<BreakStatementAstNode>> Parser::parseBreakStatement() noexcept {
  bool error = false;

  this->expect(TokenType::Break, error, "Expected a break");

  this->skipWhiteSpace();

  this->expect(TokenType::SemiColon, error, "Expected a semicolon after break");

  if (error) { return std::nullopt; }

  return std::make_shared<BreakStatementAstNode>();
}

const std::optional<std::shared_ptr<ReturnStatementAstNode>> Parser::parseReturnStatement() noexcept {
  bool error = false;

  this->expect(TokenType::Return, error, "Expected a return");

  this->skipWhiteSpace();

  bool foundExpression = false;
  std::optional<std::shared_ptr<ExpressionAstNode>> expression{std::nullopt};

  if (this->tokenizer->currentToken()->tokenType != TokenType::SemiColon) {
    expression = this->parseExpression();
  }

  this->expect(TokenType::SemiColon, error, "Expected a semicolon to terminate return statement");

  if (foundExpression) {
    error = error || !expression;
  }

  if (error) { return std::nullopt; }

  if (foundExpression) {
    return std::make_shared<ReturnStatementAstNode>(expression.value());
  }

  return std::make_shared<ReturnStatementAstNode>();
}

const std::optional<std::shared_ptr<AssignStatementAstNode>> Parser::parseAssignStatement() noexcept {
  bool error = false;

  auto identifier = this->expect(TokenType::Identifier, error, "Expected a identifier");

  this->skipWhiteSpace();

  this->expect(TokenType::Assign, error, "Expected an equals following var declaration");

  auto expression = this->parseExpression();

  this->expect(TokenType::SemiColon, error, "Expected a semicolon following expression of var declaration");

  error = error || !identifier || !expression;

  if (error) { return std::nullopt; }

  return std::make_shared<AssignStatementAstNode>(identifier, expression.value());
}

const std::optional<std::shared_ptr<BlockStatementAstNode>> Parser::parseBlockStatement() noexcept {
  bool error = false;

  auto identifier = this->expect(TokenType::LeftCurly, error, "Expected a left curly bracket");

  std::vector<std::shared_ptr<StatementAstNode>> statements;

  while (true) {
    this->skipWhiteSpace();

    if (this->tokenizer->currentToken()->tokenType == TokenType::EndOfFile) {
      this->expect(TokenType::RightCurly, error, "Expected a right curly bracket to close block");
      return std::nullopt;
    }

    auto statement = this->parseStatement();

    if (!statement) {
      error = true;
    } else {
      statements.push_back(statement.value());
    }

    if (this->tokenizer->currentToken()->tokenType == TokenType::RightCurly) {
      this->expect(TokenType::RightCurly, error, "Expected a right curly bracket to close block");
      break;
    }
  }

  if (error) { return std::nullopt; }

  return std::make_shared<BlockStatementAstNode>(statements);
}

const std::optional<std::shared_ptr<ExpressionAstNode>> Parser::parseExpression() noexcept {
  this->skipWhiteSpace();

  std::optional<std::shared_ptr<ExpressionAstNode>> e;

  switch (this->tokenizer->currentToken()->tokenType)  {
    case TokenType::LeftCurly:
      e = this->parseObjectDeclarationExpression();
      this->skipWhiteSpace();
      return e;
    case TokenType::Function:
      e = this->parseFunctionDeclarationExpression();
      this->skipWhiteSpace();
      return e;
    case TokenType::BooleanLiteral:
    case TokenType::StringLiteral:
    case TokenType::IntegerLiteral:
    case TokenType::FloatLiteral:
    case TokenType::UndefinedLiteral:
      e = this->parseLiteralExpression();
      this->skipWhiteSpace();
      return e;
    default:
      e = this->parseIdentifierOrFunctionInvocationExpression();
      this->skipWhiteSpace();
      return e;
  }
}

const std::optional<std::shared_ptr<LiteralExpressionAstNode>> Parser::parseLiteralExpression() noexcept {
  switch (this->tokenizer->currentToken()->tokenType)  {
    case TokenType::BooleanLiteral:
    case TokenType::StringLiteral:
    case TokenType::IntegerLiteral:
    case TokenType::FloatLiteral:
    case TokenType::UndefinedLiteral: {
      auto token = this->tokenizer->currentToken();
      this->tokenizer->nextToken();
      return std::make_shared<LiteralExpressionAstNode>(token);
    }
    default:
      return std::nullopt;
  }
}

const std::optional<std::shared_ptr<ExpressionAstNode>> Parser::parseIdentifierOrFunctionInvocationExpression() noexcept {
  bool error = false;

  auto token = this->expect(TokenType::Identifier, error, "Expected an identifier");

  this->skipWhiteSpace();

  if (error) { return std::nullopt; }

  if (this->tokenizer->currentToken()->tokenType == TokenType::LeftParen) {
    return this->parseFunctionInvocationExpression(token);
  }

  return std::make_shared<IdentifierExpressionAstNode>(token);
}

const std::optional<std::shared_ptr<FunctionInvocationExpressionAstNode>> Parser::parseFunctionInvocationExpression(const std::shared_ptr<Token>&  identifer) noexcept {
  bool error = false;
  std::vector<std::shared_ptr<ExpressionAstNode>> arguments;

  this->skipWhiteSpace();

  this->expect(TokenType::LeftParen, error, "Expected left paren for function invocation");

  bool seenAnArg = false;
  bool seenAComma = false;

  while (true) {
    this->skipWhiteSpace();

    if (this->tokenizer->currentToken()->tokenType == TokenType::EndOfFile) {
      this->expect(TokenType::RightParen, error, "Expected a right paren to close function invocation");
      return std::nullopt;
    }

    if (!seenAComma && this->tokenizer->currentToken()->tokenType == TokenType::RightParen) {
      this->expect(TokenType::RightParen, error, "Expected a right paren to close function invocation");
      break;
    }

    auto arg = this->parseExpression();

    if (!arg) {
      error = true;
    } else {
      arguments.push_back(arg.value());
      seenAComma = false;
    }

    if (seenAnArg) {
      if (this->tokenizer->currentToken()->tokenType == TokenType::RightParen) {
        this->expect(TokenType::RightParen, error, "Expected a right paren to close function invocation");
        break;
      }

      this->expect(TokenType::Comma, error, "Expected a comma between function arguments");
      seenAComma = true;
    }

    seenAnArg = true;
  }

  if (error) { return std::nullopt; }

  return std::make_shared<FunctionInvocationExpressionAstNode>(identifer, arguments);
}

const std::optional<std::shared_ptr<FunctionDeclarationExpressionAstNode>> Parser::parseFunctionDeclarationExpression() noexcept {
  bool error = false;
  std::vector<std::shared_ptr<Token>> parameters;

  this->expect(TokenType::Function, error, "Expected function keyword for function definition");

  this->skipWhiteSpace();

  this->expect(TokenType::LeftParen, error, "Expected left paren following function definition");

  bool seenAnArg = false;
  bool seenAComma = false;

  while (true) {
    this->skipWhiteSpace();

    if (this->tokenizer->currentToken()->tokenType == TokenType::EndOfFile) {
      this->expect(TokenType::RightParen, error, "Expected a right paren to close parameter list of function definition");
      return std::nullopt;
    }

    if (!seenAComma && this->tokenizer->currentToken()->tokenType == TokenType::RightParen) {
      this->expect(TokenType::RightParen, error, "Expected a right paren to close parameter list of function definition");
      break;
    }

    auto token = this->expect(TokenType::Identifier, error, "Expected identifier as parameter of function definition");

    this->skipWhiteSpace();

    if (token->tokenType != TokenType::Identifier) {
      error = true;

    } else {
      parameters.push_back(token);
      seenAComma = false;
    }

    if (seenAnArg) {
      if (this->tokenizer->currentToken()->tokenType == TokenType::RightParen) {
        this->expect(TokenType::RightParen, error, "Expected a right paren to close parameter list of function definition");
        break;
      }

      this->expect(TokenType::Comma, error, "Expected a comma between function parameters in function definition");
      seenAComma = true;
    }

    seenAnArg = true;
  }

  auto statement = this->parseStatement();

  error = error || !statement;

  if (error) { return std::nullopt; }

  return std::make_shared<FunctionDeclarationExpressionAstNode>(parameters, statement.value());
}

const std::optional<std::shared_ptr<ObjectDeclarationExpressionAstNode>> Parser::parseObjectDeclarationExpression() noexcept {
  bool error = false;
  this->expect(TokenType::LeftCurly, error, "Expected a left curly bracket to begin object declaration");

  bool seenAKeyPair = false;
  bool seenAComma = false;

  std::vector<std::pair<std::shared_ptr<Token>, std::shared_ptr<ExpressionAstNode>>> keyValues;

  while (true) {
    this->skipWhiteSpace();

    if (this->tokenizer->currentToken()->tokenType == TokenType::EndOfFile) {
      this->expect(TokenType::RightCurly, error, "Expected a right curly bracket to end object declaration");
      return std::nullopt;
    }

    if (!seenAComma && this->tokenizer->currentToken()->tokenType == TokenType::RightCurly) {
      this->expect(TokenType::RightCurly, error, "Expected a right curly bracket to end object declaration");
      break;
    }

    auto key = this->expect(TokenType::Identifier, error, "Expected identifier as parameter of function definition");

    if (key->tokenType != TokenType::Identifier) {
      error = true;
    }

    this->skipWhiteSpace();

    this->expect(TokenType::Colon, error, "Expected a colon between object key and value");

    auto value = this->parseExpression();

    if (!value) {
      error = true;
    }

    if (!error) {
      keyValues.emplace_back(key, value.value());
    }

    if (seenAKeyPair) {
      if (this->tokenizer->currentToken()->tokenType == TokenType::RightBracket) {
        this->expect(TokenType::RightBracket, error, "Expected a right curly bracket to end object declaration");
        break;
      }

      this->expect(TokenType::Comma, error, "Expected a comma between function parameters in function definition");
      seenAComma = true;
    }

    seenAKeyPair = true;
  }

  if (error) { return std::nullopt; }

  return std::make_shared<ObjectDeclarationExpressionAstNode>(keyValues);
}

void Parser::skipWhiteSpace() noexcept {
  while (this->tokenizer->currentToken()->tokenType != TokenType::EndOfFile && this->tokenizer->currentToken()->isNewLine()) {
    this->tokenizer->nextToken();
  }
}

const std::string Parser::toString() const noexcept {
  return (
    "Parser::parser("
      "tokenizer: " + this->tokenizer->toString() +
    ")"
  );
}

std::shared_ptr<Token> Parser::expect(TokenType type, bool & error, const std::string & errorMessage) noexcept {
  bool foundWrongToken = false;

  while (true) {
    if (this->tokenizer->currentToken()->tokenType == TokenType::EndOfFile || this->tokenizer->currentToken()->tokenType != type) {
      error = true;

      // if this is our first error, report it
      if (!foundWrongToken) {
        this->reportError(
          this->tokenizer->currentToken(),
          errorMessage
          + ", but was type " + Token::typeToString(this->tokenizer->currentToken()->tokenType)
          + "with value '" + this->tokenizer->currentToken()->value + "'.");
      }

      foundWrongToken = true;

      this->tokenizer->nextToken();

    } else {
      break;
    }
  }

  auto token = this->tokenizer->currentToken();

  this->tokenizer->nextToken();

  return token;
}

void Parser::reportError(const std::shared_ptr<Token>& token, const std::string & errorMessage) noexcept {
  if (token->tokenType == TokenType::EndOfFile) {
    this->out
      << "At end of file an error was discovered:" << std::endl
      << errorMessage << std::endl;

  } else {
    std::size_t startofLine = token->sourceIndex - (token->sourceColumn - 1);
    std::string line = this->tokenizer->getReader()->getLineFromIndex(startofLine);

    std::string arrow;

    arrow.append("^");

    for (std::size_t i = 2; i < token->sourceColumn; i++) {
      if (i == 2) {
        arrow.append(" ");
      } else {
        arrow.append("~");
      }
    }

    this->out
      << "On line " << token->sourceLine
      << " at column " << token->sourceColumn
      << " within source index at " << token->sourceIndex
      << " an error was discovered:" << std::endl
      << errorMessage << std::endl
      << line << std::endl
      << arrow << std::endl << std::endl;
  }
}