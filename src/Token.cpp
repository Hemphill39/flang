#include "Token.hpp"

const char * TokenTypeToString[] = {
  "Unknown",
  "EndOfFile",
  "LeftCurly",
  "RightCurly",
  "NewLine",
  "WhiteSpace",
  "LeftParen",
  "RightParen",
  "LeftBracket",
  "RightBracket",
  "Function",
  "Return",
  "Break",
  "For",
  "If",
  "Else",
  "While",
  "SemiColon",
  "Var",
  "Assign",
  "Comma",
  "BooleanLiteral",
  "UndefinedLiteral",
  "Period",
  "Colon",
  "Identifier",
  "StringLiteral",
  "IntegerLiteral",
  "FloatLiteral",
};

const std::string Token::toString() const noexcept {
  return (
    "Token("
      "value: '" + this->value +
      "', tokenType: " + TokenTypeToString[static_cast<int>(this->tokenType)] +
      ", sourceIndex: " + std::to_string(this->sourceIndex) +
      ", sourceLine: " + std::to_string(this->sourceLine) +
      ", sourceColumn " + std::to_string(this->sourceColumn) +
  ")"
  );
}