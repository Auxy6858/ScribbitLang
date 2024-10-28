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
            bool EndComment = false;
            do {
                LastChar = getchar();
                if(LastChar == '*'){
                    LastChar = getchar();
                    if(LastChar == '/'){
                        EndComment = true;
                    }
                }
            } while(LastChar != EOF && EndComment == false);
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

class ExpressionAST {
    public:
        virtual ~ExpressionAST() = default;
};

class NumExpressionAST : public ExpressionAST {
    double Val;

    public:
        NumExpressionAST(double Val) : Val(Val) {}
};

class VariableExpressionAST {
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

static int CurTok;
static int getNextToken() {
    return CurTok = gettok();
}