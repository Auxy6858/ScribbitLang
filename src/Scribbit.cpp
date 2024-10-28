#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

enum Token {
    tok_eof = -1,

    tok_def = -2,
    tok_extern = -3,

    tok_identifier = 4,
    tok_number = -5,
};

static std::string IdentifierString;
static double NumVal;

static int gettok() {
    static int LastChar = ' ';

    while(std::isspace(LastChar)){
        LastChar = getchar();
    }

    if(isalpha(LastChar)) {
        IdentifierString = LastChar;
        while(isalnum((LastChar = getchar()))){
            IdentifierString += LastChar;
        }

        if(IdentifierString == "def"){
            return tok_def;
        }
        if(IdentifierString == "extern"){
            return tok_extern;
        }
        return tok_identifier;
    }

    if(isdigit(LastChar) || LastChar == '.'){
        std::string NumStr;
        do {
            NumStr += LastChar;
            LastChar = getchar();
        } while(isdigit(LastChar) || LastChar == '.');

        NumVal = strtod(NumStr.c_str(), 0);
        return tok_number;
    }

    if(LastChar == '/'){
        LastChar = getchar();


        // Switch ststement to determine if and how we handle multi line and single line comments

        switch (LastChar)
        {
        case '/':
            do {
                LastChar = getchar();
            } while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');
            break;
        
        case '*':
            do {
                LastChar = getchar();
                if(LastChar == '*'){
                    LastChar = getchar();
                    if(LastChar == '/'){
                        break;
                    }
                }
            } while(LastChar != EOF);
            break;
        }
        
        if(LastChar != EOF){
            return gettok();
        }
    }

    if(LastChar == EOF){
        return tok_eof;
    }

    int ThisChar = LastChar;
    LastChar = getchar();
    return ThisChar;
}

namespace{

class ExpressionAST {
    public:
        virtual ~ExpressionAST() = default;
        virtual Value *codegen() = 0;
};

class NumExpressionAST : public ExpressionAST {
    double Val;

    public:
        NumExpressionAST(double Val) : Val(Val) {}
        Value *codegen() override;
};

class VariableExpressionAST : public ExpressionAST {
    std::string Name;

    public:
        VariableExpressionAST(const std::string &Name) : Name(Name) {}
};

class BinaryExpressionAST : public ExpressionAST {
    char OpCode;
    std::unique_ptr<ExpressionAST> LHS, RHS;

    public:
        BinaryExpressionAST(char OpCode, std::unique_ptr<ExpressionAST> LHS,
                            std::unique_ptr<ExpressionAST> RHS)
                            : OpCode(OpCode), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
};

class CallExpressionAST : public ExpressionAST {
    std::string Callee;
    std::vector<std::unique_ptr<ExpressionAST>> Args;

    public:
        CallExpressionAST(const std::string &Callee,
                           std::vector<std::unique_ptr<ExpressionAST>> Args)
                           : Callee(Callee), Args(std::move(Args)) {}
};

class PrototypeAST {
    std::string Name;
    std::vector<std::string> Args;

    public:
        PrototypeAST(const std::string &Name, std::vector<std::string> Args)
        : Name(Name), Args(std::move(Args)) {}

    const std::string &getName() const {return Name; }
};

class FunctionAST {
    std::unique_ptr<PrototypeAST> Proto;
    std::unique_ptr<ExpressionAST> Body;

    public:
        FunctionAST(std::unique_ptr<PrototypeAST> Proto,
                    std::unique_ptr<ExpressionAST> Body)
                    : Proto(std::move(Proto)), Body(std::move(Body)) {}
};


}




static int CurTok;
static int getNextToken() {
    return CurTok = gettok();
}
static std::map<char, int> BinaryOperationPrecedence;


static int GetTokPrecedence() {
    if(!isascii(CurTok)) {
        return -1;
    }

    int TokPrec = BinaryOperationPrecedence[CurTok];
    if(TokPrec <= 0) {
        return -1;
    }
    return TokPrec;
}

std::unique_ptr<ExpressionAST> LogError(const char *Str) {
    fprintf(stderr, "Error: %s \n", Str);
    return nullptr;
}

std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
    LogError(Str);
    return nullptr;
}

static std::unique_ptr<ExpressionAST> ParseExpression();

static std::unique_ptr<ExpressionAST> ParseNumExpression() {
    auto Result = std::make_unique<NumExpressionAST>(NumVal);
    getNextToken();
    return std::move(Result);
}

static std::unique_ptr<ExpressionAST> ParseParenthesisExpression() {
    getNextToken();
    auto V = ParseExpression();
    if( !V ) {
        return nullptr;
    }
    if(CurTok != ')' ) {
        return LogError("Expected ')' ");
    }
    getNextToken();
    return V;
}

static std::unique_ptr<ExpressionAST> ParseIdentifierExpression() {
    std::string IdName = IdentifierString;

    getNextToken();
    
    if(CurTok != '(' ){
        return std::make_unique<VariableExpressionAST>(IdName);
    }

    getNextToken();
    std::vector<std::unique_ptr<ExpressionAST>> Args;
    if(CurTok != ')' ) {
        while(true) {
            if(auto Arg = ParseExpression()) {
                Args.push_back(std::move(Arg));
            } else {
                return nullptr;
            }

            if(CurTok == ')' ){
                break;
            }
            if(CurTok != ',') {
                return LogError("Expected ')' or ',' in argument list.");
            }
            getNextToken();
        }
    }
    getNextToken();
    return std::make_unique<CallExpressionAST>(IdName, std::move(Args));
}


static std::unique_ptr<ExpressionAST> ParsePrimary() {
    switch(CurTok) {
        default:
            return LogError("Unknown token when expecting an expression");
        case tok_identifier:
            return ParseIdentifierExpression();
        case tok_number:
            return ParseNumExpression();
        case '(':
            return ParseParenthesisExpression();
    }
}

static std::unique_ptr<ExpressionAST> ParseBinaryOpRHS(int ExpressionPrec, std::unique_ptr<ExpressionAST> LHS) {
    while(true) {
        int TokPrec = GetTokPrecedence();

        if(TokPrec < ExpressionPrec) {
            return LHS;
        }

        int BinaryOp = CurTok;
        getNextToken();

        auto RHS = ParsePrimary();
        if(!RHS) {
            return nullptr;
        }

        int NextPrec = GetTokPrecedence();
        if(TokPrec < NextPrec) {
            RHS = ParseBinaryOpRHS(TokPrec + 1, std::move(RHS));
            if(!RHS) {
                return nullptr;
            }
        }

        LHS = std::make_unique<BinaryExpressionAST>(BinaryOp, std::move(LHS), std::move(RHS));
    }
}


static std::unique_ptr<ExpressionAST> ParseExpression() {
    auto LHS = ParsePrimary();
    if(!LHS) {
        return nullptr;
    }

    return ParseBinaryOpRHS(0, std::move(LHS));
}

static std::unique_ptr<PrototypeAST> ParsePrototype() {
    if(CurTok != tok_identifier) {
        return LogErrorP("Expected function name in prototype.");
    }

    std::string FnName = IdentifierString;
    getNextToken();

    if(CurTok != '(' ) {
        return LogErrorP("Expected '(' in prototype.");
    }

    std::vector<std::string> ArgNames;
    while(getNextToken() == tok_identifier) {
        ArgNames.push_back(IdentifierString);
    }
    if(CurTok != ')'){
        return LogErrorP("Expected ')' in prototype.");
    }

    getNextToken();

    return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames));
}

static std::unique_ptr<FunctionAST> ParseDefinition() {
  getNextToken();
  auto Proto = ParsePrototype();
  if (!Proto)
    return nullptr;

  if (auto E = ParseExpression())
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
  return nullptr;
}

static std::unique_ptr<FunctionAST> ParseTopLevelExpression() {
    if(auto E = ParseExpression() ) {
        auto Proto = std::make_unique<PrototypeAST>("", std::vector<std::string>());
        return std::make_unique<FunctionAST>(std::move(Proto), std::move(E));
    }
    return nullptr;
}

static std::unique_ptr<PrototypeAST> ParseExtern() {
    getNextToken();
    return ParsePrototype();
}

static void HandleDefinition() {
  if (ParseDefinition()) {
    fprintf(stderr, "Parsed a function definition.\n");
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

static void HandleExtern() {
    if(ParseExtern()) {
        fprintf(stderr, "Parsed an extern\n");
    } else {
        getNextToken();
    }
}

static void HandleTopLevelExpression() {
    if(ParseTopLevelExpression()) {
        fprintf(stderr, "Parsed a top-level expression\n");
    } else {
        getNextToken();
    }
}

static void MainLoop() {
    while(true) {
        fprintf(stderr, "ready> ");
        switch(CurTok) {
            case tok_eof:
                return;
            case ';':
                getNextToken();
                break;
            case tok_def:
                HandleDefinition();
                break;
            case tok_extern:
                HandleExtern();
                break;
            default:
                HandleTopLevelExpression();
                break;
        }
    }
}

int main() {
    BinaryOperationPrecedence['<'] = 10;
    BinaryOperationPrecedence['+'] = 20;
    BinaryOperationPrecedence['-'] = 20;
    BinaryOperationPrecedence['*'] = 40;
    
    // Prime the first token.
    fprintf(stderr, "ready> ");
    getNextToken();

    // Run the main "interpreter loop" now.
    MainLoop();

    return 0;
}


