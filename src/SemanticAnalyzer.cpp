#include "SemanticAnalyzer.hpp"

class VariableDefinition {
public:
  std::shared_ptr<Token> token;
  bool isFullyBound;
};

class Scope {
public:
  std::shared_ptr<Scope> outerScope;
  std::unordered_map<std::string, VariableDefinition> localScope;
  bool outerScopeCrossesFunctionBarrier;

  explicit Scope() noexcept
  : outerScope{nullptr}
  , outerScopeCrossesFunctionBarrier{true}
  {}

  explicit Scope(bool outerScopeCrossesFunctionBarrier, std::shared_ptr<Scope> outerScope) noexcept
  : outerScope{outerScope}
  , outerScopeCrossesFunctionBarrier{outerScopeCrossesFunctionBarrier}
  {}

  ~Scope() = default;

  bool hasOuterScope() const {
    return this->outerScope != nullptr;
  }

  void define(const std::string & variableName, std::shared_ptr<Token> token, bool isFullyBound) {
    auto find = this->localScope.find(variableName);

    if (find == this->localScope.end()) {
      VariableDefinition vd;
      vd.isFullyBound = isFullyBound;
      vd.token = token;
      this->localScope.insert(std::make_pair(variableName, vd));

    } else {
      this->localScope.at(variableName).isFullyBound = true;
      this->localScope.at(variableName).token = token;
    }
  }

  std::optional<VariableDefinition> findLocally(const std::string & variableName, bool considerPartiallyBound) {

    auto find = this->localScope.find(variableName);

    if (find == this->localScope.end()) {
      return std::nullopt;
    }

    VariableDefinition vd = find->second;

    if (!vd.isFullyBound) {
      if (considerPartiallyBound) {
        return vd;
      }

      return std::nullopt;
    }

    return vd;
  }

  std::optional<VariableDefinition> find(const std::string & variableName, bool canCrossFunctionBarrier) {

    Scope* scopeToSearch = this;

    bool considerPartiallyBound = false;

    while (true) {
      auto find = scopeToSearch->findLocally(variableName, considerPartiallyBound);

      if (find) {
        return find;
      }

      if (scopeToSearch->outerScope != nullptr) {
        if (scopeToSearch->outerScopeCrossesFunctionBarrier) {
          considerPartiallyBound = true;
        }

        if (!canCrossFunctionBarrier && scopeToSearch->outerScopeCrossesFunctionBarrier) {
          return std::nullopt;

        } else {
          scopeToSearch = scopeToSearch->outerScope.get();
        }
      } else {
        return std::nullopt;
      }
    }
  }

  std::shared_ptr<Scope> getOuterScope() {
    return this->outerScope;
  }
};

class SemanticAnalyzerRun : public AstWalker {
public:
  std::ostream & out;
  const std::shared_ptr<const Readable> reader;
  const std::unordered_map<std::string, std::size_t> builtInFunctionArgumentCounts;

  // these are modified to check semantic corectness
  bool error;
  std::size_t functionDept;
  bool inLoop;
  std::unordered_map<FunctionDeclarationExpressionAstNode*, bool> inLoopState;
  std::shared_ptr<Scope> currentScope;

  explicit SemanticAnalyzerRun(std::ostream & out, std::shared_ptr<const Readable> reader) noexcept
  : out{out}
  , reader{std::move(reader)}
  , builtInFunctionArgumentCounts{{
    {"add", 2},
    {"subtract", 2},
    {"multiply", 2},
    {"divide", 2},
    {"equal", 2},
    {"notEqual", 2},
    {"not", 1},
    {"and", 2},
    {"or", 2},
    {"greater", 2},
    {"less", 2},
    {"greaterOrEqual", 2},
    {"lessOrEqual", 2},
    {"get", 2},
    {"set", 3},
    {"read", 0},
    {"print", 1},
    {"env", 1},
    {"type", 1},
    {"int", 1},
    {"float", 1},
    {"length", 1},
    {"charAt", 2},
    {"append", 2}
  }}
  , error{false}
  , functionDept{0}
  , inLoop{false}
  , currentScope{std::make_shared<Scope>()}
  {}

  ~SemanticAnalyzerRun() noexcept override = default;

  bool hasErrors() noexcept {
    return this->error;
  }

  // validate that the variable we are delcaring has not already been declared elsewhere
  void onEnterDeclareStatementAstNode(DeclareStatementAstNode* node) noexcept override {
    auto originalDeclare = this->currentScope->findLocally(node->identifier->value, false);
    if (originalDeclare != std::nullopt) {
      this->reportError(node->identifier, "Duplicate identifier found within same scope.");
    } else {
      // paritally define (late bound available) variable for current scope
      this->currentScope->define(node->identifier->value, node->identifier, false);
    }
  }

  // validate that the variable we are assign has been declared within the current scope
  void onEnterAssignStatementAstNode(AssignStatementAstNode* node) noexcept override {
    auto originalDeclare = this->currentScope->find(node->identifier->value, false);
    if (originalDeclare == std::nullopt) {
      this->reportError(node->identifier, "No local declaration found for identifier.");
    }
  }

  // validate that we're in a loop
  void onEnterBreakStatementAstNode(BreakStatementAstNode*  node) noexcept override {
    if (!this->inLoop) {
      this->reportError(node->breakToken, "Break statements can only be used from within loops.");
    }
  }

  // validate that function dept is not zero
  void onEnterReturnStatementAstNode(ReturnStatementAstNode*  node) noexcept override {
    if (functionDept == 0) {
      this->reportError(node->returnToken, "Return statements can only be used from within functions.");
    }
  }

  void pushScope(bool newFn, const std::vector<std::shared_ptr<Token>>& itemsToDeclare) noexcept {
    auto newScope = std::make_shared<Scope>(newFn, this->currentScope);
    this->currentScope = newScope;

    for (auto& item : itemsToDeclare) {
      auto find = this->currentScope->findLocally(item->value, false);

      if (find) {
        this->reportError(item, "Duplicate identifier found within the same scope.");

      } else {
        this->currentScope->define(item->value, item, true);
      }
    }
  }

  // increase scope
  void onEnterBlockStatementAstNode(BlockStatementAstNode* /*node*/) noexcept override {
    this->pushScope(false, {});
  }

  // mark in loop to true
  void onEnterWhileStatementAstNode(WhileStatementAstNode* /*node*/) noexcept override {
    this->inLoop = true;
  }

  // validate that identifier is defined
  void onEnterIdentifierExpressionAstNode(IdentifierExpressionAstNode* node) noexcept override {
    if (!this->currentScope->find(node->token->value, true)) {
      this->reportError(node->token, "Undefined reference in identifier evaluation.");
    }
  }

  // validate that identifier is defined
  void onEnterFunctionInvocationExpressionAstNode(FunctionInvocationExpressionAstNode*  node) noexcept override {
    if (!this->currentScope->find(node->identifier->value, true)) {
      this->reportError(node->identifier, "Undefined reference in function invocation.");
    }
  }

  // increase scope, define arguments
  void onEnterFunctionDeclarationExpressionAstNode(FunctionDeclarationExpressionAstNode* node) noexcept override {
    this->functionDept++;
    this->inLoopState.insert(std::make_pair(node, this->inLoop));
    this->inLoop = false;
    this->pushScope(true, node->parameters);
  }

  // validate that the numeric literals can be constructed properly
  void onEnterLiteralExpressionAstNode(LiteralExpressionAstNode* node) noexcept override {
    switch (node->token->tokenType) {
      case TokenType::IntegerLiteral: {
        try {
          std::stoll(node->token->value);
        } catch (...) {
          this->reportError(node->token, "Invalid value for integer literal.");
        }
      }
      case TokenType::FloatLiteral: {
        try {
          std::stod(node->token->value);
        } catch (...) {
          this->reportError(node->token, "Invalid value for float literal.");
        }
      }
      default:
        return;
    }
  }

  // define the variable within the current scope
  void onExitDeclareStatementAstNode(DeclareStatementAstNode* node) noexcept override {
    this->currentScope->define(node->identifier->value, node->identifier, true);
  }

  void popScope() noexcept {
    auto outer = this->currentScope->getOuterScope();

    Error::assertWithPanic(outer != nullptr, "No outer scope found.");

    this->currentScope = outer;
  }

  // pop scope
  void onExitBlockStatementAstNode(BlockStatementAstNode* /*node*/) noexcept override {
    this->popScope();
  }

  // mark in loop to false
  void onExitWhileStatementAstNode(WhileStatementAstNode* /*node*/) noexcept override {
    this->inLoop = false;
  }

  void onEnterBuiltInFunctionInvocationExpressionAstNode(BuiltInFunctionInvocationExpressionAstNode* fn) noexcept override {
    auto find = this->builtInFunctionArgumentCounts.find(fn->identifier->value);

    Error::assertWithPanic(
      find != this->builtInFunctionArgumentCounts.end(),
      "onEnterBuiltInFunctionInvocationExpressionAstNode found a built in function it does not know about");

    std::size_t count = find->second;

    if (count != fn->expressions.size()) {
      this->reportError(fn->identifier,
        "Argument size for built in function did not match expected count of " + std::to_string(count));
    }
  }

  // pop scope, decrease function dept, and restore loop setting
  void onExitFunctionDeclarationExpressionAstNode(FunctionDeclarationExpressionAstNode* node) noexcept override {
    Error::assertWithPanic(
      this->functionDept != 0,
      "Expected a non zero function dept when calling onExitFunctionDeclarationExpressionAstNode");

    this->functionDept--;

    auto search = this->inLoopState.find(node);

    Error::assertWithPanic(
      search != this->inLoopState.end(),
      "Could not find a key in inLoopState map for function declaration");

    this->inLoop = search->second;

    this->popScope();
  }

  void reportError(const std::shared_ptr<Token>& token, const std::string & errorMessage) noexcept {
    this->error = true;
    Error::reportErrorAtToken(this->out, "semantic analysis", this->reader, token, errorMessage);
  }
};

bool SemanticAnalyzer::isValid(const std::shared_ptr<ScriptAstNode>& script) noexcept {

  SemanticAnalyzerRun run{this->out, this->reader};

  run.visitScriptAstNode(script.get());

  return !run.hasErrors();
}