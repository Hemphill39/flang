#ifndef PARSER_HPP
#define PARSER_HPP

#include "lib.hpp"
#include "Stringable.hpp"
#include "Ast.hpp"
#include "Tokenizer.hpp"

class Parser : public Stringable {
private:
  const std::shared_ptr<Tokenizer> tokenizer;
  std::ostream & out;

public:
  explicit Parser(
    std::shared_ptr<Tokenizer> tokenizer,
    std::ostream & out
  ) noexcept
  : tokenizer{std::move(tokenizer)},
    out{out}
  {}

  ~Parser() noexcept override = default;

  const std::optional<std::shared_ptr<ScriptAstNode>> parseScript() noexcept;

  const std::optional<std::shared_ptr<StatementAstNode>> parseStatement() noexcept;

  const std::optional<std::shared_ptr<IfStatementAstNode>> parseIfStatement() noexcept;

  const std::optional<std::shared_ptr<DeclareStatementAstNode>> parseDeclareStatement() noexcept;

  const std::optional<std::shared_ptr<WhileStatementAstNode>> parseWhileStatement() noexcept;

  const std::optional<std::shared_ptr<BreakStatementAstNode>> parseBreakStatement() noexcept;

  const std::optional<std::shared_ptr<ReturnStatementAstNode>> parseReturnStatement() noexcept;

  const std::optional<std::shared_ptr<AssignStatementAstNode>> parseAssignStatement() noexcept;

  const std::optional<std::shared_ptr<BlockStatementAstNode>> parseBlockStatement() noexcept;

  const std::optional<std::shared_ptr<ExpressionAstNode>> parseExpression() noexcept;

  const std::optional<std::shared_ptr<LiteralExpressionAstNode>> parseLiteralExpression() noexcept;

  const std::optional<std::shared_ptr<FunctionDeclarationExpressionAstNode>> parseFunctionDeclarationExpression() noexcept;

  const std::optional<std::shared_ptr<ObjectDeclarationExpressionAstNode>> parseObjectDeclarationExpression() noexcept;

  const std::optional<std::shared_ptr<ExpressionAstNode>> parseIdentifierOrFunctionInvocationExpression() noexcept;

  const std::string toString() const noexcept override;

private:
  void reportError(const std::shared_ptr<Token>& token, const std::string & errorMessage) noexcept;

  void skipWhiteSpace() noexcept;

  std::shared_ptr<Token> expect(TokenType type, bool & error, const std::string & errorMessage) noexcept;

  // private because it is expecting a value passed from parseIdentifierOrFunctionInvocationExpression
  const std::optional<std::shared_ptr<FunctionInvocationExpressionAstNode>> parseFunctionInvocationExpression(const std::shared_ptr<Token>& identifer) noexcept;
};

#endif