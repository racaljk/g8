//===----------------------------------------------------------------------===//
// Golang specification can be found here: https://golang.org/ref/spec
//
// In development, I consider raise runtime_error  since it's helpful to locate 
// where error occurred and do further debugging. 
//
// Written by racaljk@github<1948638989@qq.com>
//===----------------------------------------------------------------------===//
#include <cctype>
#include <cstdio>
#include <exception>
#include <fstream>
#include <functional>
#include <string>
#include <vector>
#include <tuple>
#include <map>
#define LAMBDA_FUN(X) function<Ast##X*(Token&)> parse##X;
#define ASTNODE :public AstNode
using namespace std;


//===----------------------------------------------------------------------===//
// various declarations 
//===----------------------------------------------------------------------===//
string keywords[] = { "break",    "default",     "func",   "interface", "select",
                     "case",     "defer",       "go",     "map",       "struct",
                     "chan",     "else",        "goto",   "package",   "switch",
                     "const",    "fallthrough", "if",     "range",     "type",
                     "continue", "for",         "import", "return",    "var" };

enum TokenType : signed int {
    KW_break, KW_default, KW_func, KW_interface, KW_select, KW_case, KW_defer,
    KW_go, KW_map, KW_struct, KW_chan, KW_else, KW_goto, KW_package, KW_switch,
    KW_const, KW_fallthrough, KW_if, KW_range, KW_type, KW_continue, KW_for,
    KW_import, KW_return, KW_var, OP_ADD, OP_BITAND, OP_ADDAGN, OP_BITANDAGN,
    OP_AND, OP_EQ, OP_NE, OP_LPAREN, OP_RPAREN, OP_SUB, OP_BITOR, OP_SUBAGN,
    OP_BITORAGN, OP_OR, OP_LT, OP_LE, OP_LBRACKET, OP_RBRACKET, OP_MUL, OP_XOR,
    OP_MULAGN, OP_BITXORAGN, OP_CHAN, OP_GT, OP_GE, OP_LBRACE, OP_RBRACE,
    OP_DIV, OP_LSHIFT, OP_DIVAGN, OP_LSFTAGN, OP_INC, OP_AGN, OP_SHORTAGN,
    OP_COMMA, OP_SEMI, OP_MOD, OP_RSHIFT, OP_MODAGN, OP_RSFTAGN, OP_DEC,
    OP_NOT, OP_VARIADIC, OP_DOT, OP_COLON, OP_ANDXOR, OP_ANDXORAGN, TK_ID,
    LITERAL_INT, LITERAL_FLOAT, LITERAL_IMG, LITERAL_RUNE, LITERAL_STR, TK_EOF = -1
};
//todo: add destructor for these structures
// Common
struct AstExpr;
struct AstStmt;
struct AstNode { virtual ~AstNode() = default; };
struct AstIdentList ASTNODE { vector<string> identList; };
struct AstExprList ASTNODE { vector<AstExpr*> exprList; };
struct AstStmtList ASTNODE { vector<AstStmt*> stmtList; };

// Declaration
struct AstPackageClause ASTNODE { string packageName; };
struct AstImportDecl ASTNODE { map<string, string> imports; };
struct AstConstDecl ASTNODE {
    vector<AstNode*> identList;
    vector<AstNode*> type;
    vector<AstNode*> exprList;
};
struct AstTypeDecl ASTNODE { vector<AstNode*> typeSpec; };
struct AstTypeSpec ASTNODE {
    string identifier;
    AstNode* type{};
};
struct AstVarDecl ASTNODE { vector<AstNode*> varSpec; };
struct AstVarSpec ASTNODE {
    AstNode* identList{};
    union {
        struct {
            AstNode* type;
            AstNode* exprList;
        }named;
        AstNode* exprList;
    }avs{};
};
struct AstFunctionDecl ASTNODE {
    string funcName;
    AstNode* receiver{};
    AstNode* signature{};
    AstNode* functionBody{};
};
struct AstSourceFile ASTNODE {
    vector<AstImportDecl*> importDecl;
    vector<AstConstDecl*> constDecl;
    vector<AstTypeDecl*> typeDecl;
    vector<AstFunctionDecl*> funcDecl;
    vector<AstVarDecl*> varDecl;
};
// Type
struct AstType ASTNODE {
    AstNode* type{};
};
struct AstName ASTNODE { string name; };
struct AstArrayType ASTNODE {
    AstNode* length{};
    AstNode* elementType{};
    bool automaticLen;
};
struct AstStructType ASTNODE {
    union _FieldDecl {
        struct {
            AstNode* identList;
            AstNode* type;
        }named;
        AstNode* typeName;
    };

    vector<tuple<_FieldDecl, string>> fields;
};
struct AstPointerType ASTNODE { AstNode * baseType{}; };
struct AstFunctionType ASTNODE { AstNode * signature{}; };
struct AstSignature ASTNODE {
    AstNode* parameters{};
    AstNode* result{};
};
struct AstParameter ASTNODE { vector<AstNode*> parameterList; };
struct AstParameterDecl ASTNODE {
    bool isVariadic = false;
    bool hasName = false;
    AstNode* type{};
    string name;
};
struct AstResult ASTNODE {
    AstNode* parameter;
    AstType* type;
};
struct AstInterfaceType ASTNODE { vector<AstNode*> methodSpec; };
struct AstMethodSpec ASTNODE {
    AstName* methodName;
    AstSignature* signature;
};
struct AstMethodName ASTNODE { string methodName; };
struct AstSliceType ASTNODE { AstNode* elementType{}; };
struct AstMapType ASTNODE {
    AstNode* keyType{};
    AstNode* elementType{};
};
struct AstChannelType ASTNODE { AstNode* elementType{}; };
struct AstStmt ASTNODE {
    AstNode* stmt{};
};
struct AstBlock ASTNODE { AstNode* statementList{}; };
struct AstLabeledStmt ASTNODE {
    string label;
    AstNode* statement{};
};
struct AstSimpleStmt ASTNODE { AstNode* stmt; };
struct AstGoStmt ASTNODE { AstExpr* expr{}; AstGoStmt(AstExpr* expr) :expr(expr) {} };
struct AstReturnStmt ASTNODE { AstExprList* exprList{}; AstReturnStmt(AstExprList* exprList) :exprList(exprList) {} };
struct AstBreakStmt ASTNODE { string label; AstBreakStmt(const string&s) :label(s) {} };
struct AstDeferStmt ASTNODE { AstExpr* expr{}; AstDeferStmt(AstExpr* expr) :expr(expr) {} };
struct AstContinueStmt ASTNODE { string label; AstContinueStmt(const string&s) :label(s) {} };
struct AstGotoStmt ASTNODE { string label; AstGotoStmt(const string&s) :label(s) {} };
struct AstFallthroughStmt ASTNODE {};
struct AstIfStmt ASTNODE {
    AstNode* condition{};
    AstNode* expression{};
    AstNode* block{};
    union {
        AstNode* ifStmt;
        AstNode* block;
    }ais{};
};
struct AstSwitchStmt ASTNODE {
    AstNode* condition{};
    AstNode* conditionExpr{};
    vector<AstNode*> exprCaseClause;
};
struct AstExprCaseClause ASTNODE {
    AstNode* exprSwitchCase{};
    AstNode* statementList{};
};
struct AstExprSwitchCase ASTNODE {
    AstNode * exprList{};
    bool isDefault{};
};
struct AstSelectStmt ASTNODE {
    vector<AstNode*> commClause;
};
struct AstCommClause ASTNODE {
    AstNode* commCase{};
    AstNode* statementList{};
};
struct AstCommCase ASTNODE {
    union {
        AstNode* sendStmt;
        AstNode* recvStmt;
    }acc{};
    bool isDefault{};
};
struct AstRecvStmt ASTNODE {
    union {
        AstNode* identList;
        AstNode* exprList;
    }ars{};
    AstNode* recvExpr{};
};
struct AstForStmt ASTNODE {
    union {
        AstNode* condition;
        AstNode* forClause;
        AstNode* rangeClause;
    }afs{};
    AstNode* block{};
};
struct AstForClause ASTNODE {
    AstNode* initStmt{};
    AstNode* condition{};
    AstNode* postStmt{};
};
struct AstRangeClause ASTNODE {
    union {
        AstNode* exprList;
        AstNode* identList;
    }arc{};
    AstNode* expression{};
};
struct AstExpressionStmt ASTNODE { AstNode* expression{}; };
struct AstSendStmt ASTNODE {
    AstExpr* receiver{};
    AstExpr* sender{};
};
struct AstIncDecStmt ASTNODE {
    AstExpr* expression{};
    bool isInc{};
};
struct AstAssign ASTNODE {
    AstExprList* lhs{};
    AstExprList* rhs{};
    TokenType op;
};
struct AstShortAssign ASTNODE {
    vector<string> lhs{};
    AstExprList* rhs{};
};
// Expression
struct AstPrimaryExpr ASTNODE {
    AstNode* expr{};
};
struct AstUnaryExpr ASTNODE {
    AstNode*expr;
    TokenType op;
};
struct AstExpr ASTNODE {
    AstUnaryExpr* lhs;
    TokenType op;
    AstExpr* rhs;
};
struct AstSelectorExpr ASTNODE {
    AstNode* operand{};
    string selector;
};
struct AstTypeSwitchGuardExpr ASTNODE {
    AstNode* operand{};
    // AstNode* lhs;
};
struct AstTypeAssertionExpr ASTNODE {
    AstNode* operand{};
    AstNode* type{};
};
struct AstIndexExpr ASTNODE {
    AstNode* operand{};
    AstNode* index{};
};
struct AstSliceExpr ASTNODE {
    AstNode* operand{};
    AstNode* begin{};
    AstNode* end{};
    AstNode* step{};
};
struct AstCallExpr ASTNODE {
    AstNode* operand{};
    AstNode* arguments{};
    AstNode* type{};
    bool isVariadic{};
};
struct AstLiteralValue ASTNODE { vector< AstNode*> keyedElement; };
struct AstKeyedElement ASTNODE {
    AstNode*key{};
    AstNode*element{};
};
struct AstKey ASTNODE {
    union {
        AstNode* fieldName;
        AstNode* expression;
        AstNode* literalValue;
    }ak;
};
struct AstFieldName ASTNODE {
    string fieldName;
};
struct AstElement ASTNODE {
    union {
        AstNode*expression;
        AstNode*literalValue;
    }ae;
};
struct AstFunctionLit ASTNODE {
    AstNode*signature{};
    AstNode*functionBody{};
};
struct AstOperand ASTNODE {
    AstNode*operand;
};
struct AstBasicLit ASTNODE { TokenType type; string value; };
struct AstCompositeLit ASTNODE {
    AstNode*literalType{};
    AstLiteralValue* literalValue{};
};
//===----------------------------------------------------------------------===//
// global data
//===----------------------------------------------------------------------===//
static int line = 1, column = 1, lastToken = -1, shouldEof = 0;
struct Token {
    TokenType type; string lexeme;
    Token(TokenType a, const string&b) :type(a), lexeme(b) {}
};
static struct goruntime {
    string package;
} grt;

//===----------------------------------------------------------------------===//
// Implementation of golang compiler and runtime within 5 functions
//===----------------------------------------------------------------------===//

Token next(fstream& f) {
    auto consumePeek = [&](char& c) {
        f.get();
        column++;
        char oc = c;
        c = static_cast<char>(f.peek());
        return oc;
    };
    char c = static_cast<char>(f.peek());

skip_comment_and_find_next:

    for (; c == ' ' || c == '\r' || c == '\t' || c == '\n'; column++) {
        if (c == '\n') {
            line++;
            column = 1;
            if ((lastToken >= TK_ID && lastToken <= LITERAL_STR)
                || lastToken == KW_fallthrough || lastToken == KW_continue
                || lastToken == KW_return || lastToken == KW_break
                || lastToken == OP_INC || lastToken == OP_DEC
                || lastToken == OP_RPAREN
                || lastToken == OP_RBRACKET || lastToken == OP_RBRACE) {
                consumePeek(c);
                lastToken = OP_SEMI;
                return Token(OP_SEMI, ";");
            }
        }
        consumePeek(c);
    }
    if (f.eof()) {
        if (shouldEof) {
            lastToken = TK_EOF;
            return Token(TK_EOF, "");
        }
        shouldEof = 1;
        lastToken = OP_SEMI;
        return Token(OP_SEMI, ";");
    }

    string lexeme;


    // identifier = letter { letter | unicode_digit } .
    if (isalpha(c) || c == '_') {
        while (isalnum(c) || c == '_') {
            lexeme += consumePeek(c);
        }

        for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
            if (keywords[i] == lexeme) {
                lastToken = static_cast<TokenType>(i);
                return Token(static_cast<TokenType>(i), lexeme);
            }
        lastToken = TK_ID;
        return Token(TK_ID, lexeme);
    }

    // int_lit     = decimal_lit | octal_lit | hex_lit .
    // decimal_lit = ( "1" … "9" ) { decimal_digit } .
    // octal_lit   = "0" { octal_digit } .
    // hex_lit     = "0" ( "x" | "X" ) hex_digit { hex_digit } .

    // float_lit = decimals "." [ decimals ] [ exponent ] |
    //         decimals exponent |
    //         "." decimals [ exponent ] .
    // decimals  = decimal_digit { decimal_digit } .
    // exponent  = ( "e" | "E" ) [ "+" | "-" ] decimals .

    // imaginary_lit = (decimals | float_lit) "i" .
    if (isdigit(c) || c == '.') {
        if (c == '0') {
            lexeme += consumePeek(c);
            if (c == 'x' || c == 'X') {
                do {
                    lexeme += consumePeek(c);
                } while (isdigit(c) || c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F');
                lastToken = LITERAL_INT;
                return Token(LITERAL_INT, lexeme);
            }
            else if ((c >= '0' && c <= '9') ||
                (c == '.' || c == 'e' || c == 'E' || c == 'i')) {
                while ((c >= '0' && c <= '9') ||
                    (c == '.' || c == 'e' || c == 'E' || c == 'i')) {
                    if (c >= '0' && c <= '7') {
                        lexeme += consumePeek(c);
                    }
                    else {
                        goto shall_float;
                    }
                }
                lastToken = LITERAL_INT;
                return Token(LITERAL_INT, lexeme);
            }
            goto may_float;
        }
        else {  // 1-9 or . or just a single 0
        may_float:
            TokenType type = LITERAL_INT;
            if (c == '.') {
                lexeme += consumePeek(c);
                if (c == '.') {
                    lexeme += consumePeek(c);
                    if (c == '.') {
                        lexeme += consumePeek(c);
                        lastToken = OP_VARIADIC;
                        return Token(OP_VARIADIC, lexeme);
                    }
                    else {
                        throw runtime_error(
                            "expect variadic notation(...) but got .." + c);
                    }
                }
                else if (c >= '0'&&c <= '9') {
                    type = LITERAL_FLOAT;
                }
                else {
                    lastToken = OP_DOT;
                    return Token(OP_DOT, ".");
                }
                goto shall_float;
            }
            else if (c >= '1'&&c <= '9') {
                lexeme += consumePeek(c);
            shall_float:  // skip char consuming and appending since we did that before jumping here;
                bool hasDot = false, hasExponent = false;
                while ((c >= '0' && c <= '9') || c == '.' || c == 'e' || c == 'E' ||
                    c == 'i') {
                    if (c >= '0' && c <= '9') {
                        lexeme += consumePeek(c);
                    }
                    else if (c == '.' && !hasDot) {
                        lexeme += consumePeek(c);
                        type = LITERAL_FLOAT;
                    }
                    else if ((c == 'e' && !hasExponent) ||
                        (c == 'E' && !hasExponent)) {
                        hasExponent = true;
                        type = LITERAL_FLOAT;
                        lexeme += consumePeek(c);
                        if (c == '+' || c == '-') {
                            lexeme += consumePeek(c);
                        }
                    }
                    else {
                        f.get();
                        column++;
                        lexeme += c;
                        lastToken = LITERAL_IMG;
                        return Token(LITERAL_IMG, lexeme);
                    }
                }
                lastToken = type;
                return Token(type, lexeme);
            }
            else {
                lastToken = type;
                return Token(type, lexeme);
            }
        }
    }

    //! NOT FULLY SUPPORT UNICODE RELATED LITERALS

    // rune_lit         = "'" ( unicode_value | byte_value ) "'" .
    // unicode_value    = unicode_char | little_u_value | big_u_value |
    // escaped_char . byte_value       = octal_byte_value | hex_byte_value .
    // octal_byte_value = `\` octal_digit octal_digit octal_digit .
    // hex_byte_value   = `\` "x" hex_digit hex_digit .
    // little_u_value   = `\` "u" hex_digit hex_digit hex_digit hex_digit .
    // big_u_value      = `\` "U" hex_digit hex_digit hex_digit hex_digit
    //                            hex_digit hex_digit hex_digit hex_digit .
    // escaped_char     = `\` ( "a" | "b" | "f" | "n" | "r" | "t" | "v" | `\` |
    // "'" | `"` ) .
    if (c == '\'') {
        lexeme += consumePeek(c);
        if (c == '\\') {
            lexeme += consumePeek(c);

            if (c == 'U' || c == 'u' || c == 'x' || c == 'X') {
                do {
                    lexeme += consumePeek(c);
                } while (isdigit(c) || (c >= 'a' && c <= 'f') ||
                    (c >= 'A' && c <= 'F'));
            }
            else if (c >= '0' && c <= '7') {
                do {
                    lexeme += consumePeek(c);
                } while (c >= '0' && c <= '7');
            }
            else if (c == 'a' || c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't' ||
                c == 'v' || c == '\\' || c == '\'' || c == '"') {
                lexeme += consumePeek(c);
            }
            else {
                throw runtime_error("illegal rune");
            }

        }
        else {
            lexeme += consumePeek(c);
        }

        if (c != '\'') {
            throw runtime_error(
                "illegal rune at least in current implementation of g8");
        }
        lexeme += consumePeek(c);
        lastToken = LITERAL_RUNE;
        return Token(LITERAL_RUNE, lexeme);
    }

    // string_lit             = raw_string_lit | interpreted_string_lit .
    // raw_string_lit         = "`" { unicode_char | newline } "`" .
    // interpreted_string_lit = `"` { unicode_value | byte_value } `"` .
    if (c == '`') {
        do {
            lexeme += consumePeek(c);
            if (c == '\n') line++;
        } while (f.good() && c != '`');
        if (c != '`') {
            throw runtime_error(
                "raw string literal does not have a closed symbol \"`\"");
        }
        lexeme += consumePeek(c);
        lastToken = LITERAL_STR;
        return Token(LITERAL_STR, lexeme);
    }
    else if (c == '"') {
        do {
            lexeme += consumePeek(c);
            if (c == '\\') {
                lexeme += consumePeek(c);
                lexeme += consumePeek(c);
            }
        } while (f.good() && (c != '\n' && c != '\r' && c != '"'));
        if (c != '"') {
            throw runtime_error(
                R"(string literal does not have a closed symbol """)");
        }
        lexeme += consumePeek(c);
        lastToken = LITERAL_STR;
        return Token(LITERAL_STR, lexeme);
    }

    // operators
    switch (c) {
    case '+':  //+  += ++
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_ADDAGN;
            return Token(OP_ADDAGN, lexeme);
        }
        else if (c == '+') {
            lexeme += consumePeek(c);
            lastToken = OP_INC;
            return Token(OP_INC, lexeme);
        }
        return Token(OP_ADD, lexeme);
    case '&':  //&  &=  &&  &^  &^=
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_BITANDAGN;
            return Token(OP_BITANDAGN, lexeme);
        }
        else if (c == '&') {
            lexeme += consumePeek(c);
            lastToken = OP_AND;
            return Token(OP_AND, lexeme);
        }
        else if (c == '^') {
            lexeme += consumePeek(c);
            if (c == '=') {
                lexeme += consumePeek(c);
                lastToken = OP_ANDXORAGN;
                return Token(OP_ANDXORAGN, lexeme);
            }
            lastToken = OP_ANDXOR;
            return Token(OP_ANDXOR, lexeme);
        }
        lastToken = OP_BITAND;
        return Token(OP_BITAND, lexeme);
    case '=':  //=  ==
        lexeme += consumePeek(c);
        if (c == '=') {
            lastToken = OP_EQ;
            return Token(OP_EQ, lexeme);
        }
        lastToken = OP_AGN;
        return Token(OP_AGN, lexeme);
    case '!':  //!  !=
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_NE;
            return Token(OP_NE, lexeme);
        }
        lastToken = OP_NOT;
        return Token(OP_NOT, lexeme);
    case '(':
        lexeme += consumePeek(c);
        lastToken = OP_LPAREN;
        return Token(OP_LPAREN, lexeme);
    case ')':
        lexeme += consumePeek(c);
        lastToken = OP_RPAREN;
        return Token(OP_RPAREN, lexeme);
    case '-':  //-  -= --
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_SUBAGN;
            return Token(OP_SUBAGN, lexeme);
        }
        else if (c == '-') {
            lexeme += consumePeek(c);
            lastToken = OP_DEC;
            return Token(OP_DEC, lexeme);
        }
        lastToken = OP_SUB;
        return Token(OP_SUB, lexeme);
    case '|':  //|  |=  ||
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_BITORAGN;
            return Token(OP_BITORAGN, lexeme);
        }
        else if (c == '|') {
            lexeme += consumePeek(c);
            lastToken = OP_OR;
            return Token(OP_OR, lexeme);
        }
        lastToken = OP_BITOR;
        return Token(OP_BITOR, lexeme);
    case '<':  //<  <=  <- <<  <<=
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_LE;
            return Token(OP_LE, lexeme);
        }
        else if (c == '-') {
            lexeme += consumePeek(c);
            lastToken = OP_CHAN;
            return Token(OP_CHAN, lexeme);
        }
        else if (c == '<') {
            lexeme += consumePeek(c);
            if (c == '=') {
                lexeme += consumePeek(c);
                lastToken = OP_LSFTAGN;
                return Token(OP_LSFTAGN, lexeme);
            }
            lastToken = OP_LSHIFT;
            return Token(OP_LSHIFT, lexeme);
        }
        lastToken = OP_LT;
        return Token(OP_LT, lexeme);
    case '[':
        lexeme += consumePeek(c);
        lastToken = OP_LBRACKET;
        return Token(OP_LBRACKET, lexeme);
    case ']':
        lexeme += consumePeek(c);
        lastToken = OP_RBRACKET;
        return Token(OP_RBRACKET, lexeme);
    case '*':  //*  *=
        lexeme += consumePeek(c);
        if (c == '=') {
            lastToken = OP_MULAGN;
            return Token(OP_MULAGN, lexeme);
        }
        lastToken = OP_MUL;
        return Token(OP_MUL, lexeme);
    case '^':  //^  ^=
        lexeme += consumePeek(c);
        if (c == '=') {
            lastToken = OP_BITXORAGN;
            return Token(OP_BITXORAGN, lexeme);
        }
        lastToken = OP_XOR;
        return Token(OP_XOR, lexeme);
    case '>':  //>  >=  >>  >>=
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_GE;
            return Token(OP_GE, lexeme);
        }
        else if (c == '>') {
            lexeme += consumePeek(c);
            if (c == '=') {
                lastToken = OP_RSFTAGN;
                return Token(OP_RSFTAGN, lexeme);
            }
            lastToken = OP_RSHIFT;
            return Token(OP_RSHIFT, lexeme);
        }
        lastToken = OP_GT;
        return Token(OP_GT, lexeme);
    case '{':
        lexeme += consumePeek(c);
        lastToken = OP_LBRACE;
        return Token(OP_LBRACE, lexeme);
    case '}':
        lexeme += consumePeek(c);
        lastToken = OP_RBRACE;
        return Token(OP_RBRACE, lexeme);
    case '/': {  // /  /= // /*...*/
        char pending = consumePeek(c);
        if (c == '=') {
            lexeme += pending;
            lexeme += consumePeek(c);
            lastToken = OP_DIVAGN;
            return Token(OP_DIVAGN, lexeme);
        }
        else if (c == '/') {
            do {
                consumePeek(c);
            } while (f.good() && (c != '\n' && c != '\r'));
            goto skip_comment_and_find_next;
        }
        else if (c == '*') {
            do {
                consumePeek(c);
                if (c == '\n') line++;
                if (c == '*') {
                    consumePeek(c);
                    if (c == '/') {
                        consumePeek(c);
                        goto skip_comment_and_find_next;
                    }
                }
            } while (f.good());
        }
        lexeme += pending;
        lastToken = OP_DIV;
        return Token(OP_DIV, lexeme);
    }
    case ':':  // :=
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_SHORTAGN;
            return Token(OP_SHORTAGN, lexeme);
        }
        lastToken = OP_COLON;
        return Token(OP_COLON, lexeme);
    case ',':
        lexeme += consumePeek(c);
        lastToken = OP_COMMA;
        return Token(OP_COMMA, lexeme);
    case ';':
        lexeme += consumePeek(c);
        lastToken = OP_SEMI;
        return Token(OP_SEMI, lexeme);
    case '%':  //%  %=
        lexeme += consumePeek(c);
        if (c == '=') {
            lexeme += consumePeek(c);
            lastToken = OP_MODAGN;
            return Token(OP_MODAGN, lexeme);
        }
        lastToken = OP_MOD;
        return Token(OP_MOD, lexeme);
        // case '.' has already checked
    default:break;
    }

    throw runtime_error("illegal token in source file");
}

const AstNode* parse(const string & filename) {
    fstream f(filename, ios::binary | ios::in);
    auto t = next(f);

    auto eat = [&f, &t](TokenType tk, const string&msg) {
        if (t.type != tk) throw runtime_error(msg);
        t = next(f);
    };

    auto expect = [&f, &t](TokenType tk, const string& msg) {
        t = next(f);
        if (t.type != tk) throw runtime_error(msg);
        return t;
    };
    LAMBDA_FUN(TypeDecl); LAMBDA_FUN(VarDecl); LAMBDA_FUN(ConstDecl); LAMBDA_FUN(FunctionDecl); LAMBDA_FUN(LiteralValue);
    LAMBDA_FUN(ImportDecl); LAMBDA_FUN(Stmt); LAMBDA_FUN(Expr); LAMBDA_FUN(Signature); LAMBDA_FUN(UnaryExpr);
    LAMBDA_FUN(PrimaryExpr); LAMBDA_FUN(Type);

    function<AstNode*(AstExprList *, Token&)> parseSimpleStmt;
    function<AstNode*(Token&)> parseTypeAssertion,
        parseArrayOrSliceType, parseStructType, parsePointerType, parseFunctionType,
        parseParameter, parseParameterDecl, parseResult, parseInterfaceType,
        parseMethodSpec, parseMethodName, parseMapType, parseChannelType, parseFunctionLit,
        parseTypeSpec, parseVarSpec,
        parseCompositeLit, parseFieldName, parseBasicLit,
        parseLabeledStmt, parseGoStmt, parseReturnStmt, parseBreakStmt,
        parseContinueStmt, parseGotoStmt, parseFallthroughStmt, parseBlock, parseIfStmt,
        parseSwitchStmt, parseSelectStmt, parseForStmt, parseDeferStmt, parseExprCaseClause,
        parseExprSwitchCase, parseCommClause, parseCommCase, parseRecvStmt, parseForClause,
        parseRangeClause,
        parseOperand, parseOperandName, parseLiteral,
        parseElementList, parseKeyedElement, parseKey, parseElement;

#pragma region Common
    auto parseName = [&](Token&t)->AstName* {
        AstName * node = nullptr;
        if (t.type == TK_ID) {
            node = new AstName;
            string name;
            name += t.lexeme;
            t = next(f);
            if (t.type == OP_DOT) {
                t = next(f);
                name.operator+=(".").operator+=(t.lexeme);
                t = next(f);
            }
            node->name = name;
        }
        return node;
    };
    auto parseIdentifierList = [&](Token&t)->AstIdentList* {
        AstIdentList* node = nullptr;
        if (t.type == TK_ID) {
            node = new  AstIdentList;
            node->identList.emplace_back(t.lexeme);
            t = next(f);
            while (t.type == OP_COMMA) {
                t = next(f);
                node->identList.emplace_back(t.lexeme);
                t = next(f);
            }
        }
        return node;
    };
    auto parseExprList = [&](Token&t)->AstExprList* {
        AstExprList* node = nullptr;
        if (auto* tmp = parseExpr(t); tmp != nullptr) {
            node = new  AstExprList;
            node->exprList.emplace_back(tmp);
            while (t.type == OP_COMMA) {
                t = next(f);
                node->exprList.emplace_back(parseExpr(t));
            }
        }
        return node;
    };
    auto parseStmtList = [&](Token&t)->AstStmtList* {
        AstStmtList * node = nullptr;
        AstStmt* tmp = nullptr;
        while ((tmp = parseStmt(t))) {
            if (node == nullptr) {
                node = new AstStmtList;
            }
            node->stmtList.push_back(tmp);
            if (t.type == OP_SEMI) t = next(f);
        }
        return node;
    };
#pragma endregion
#pragma region Declaration
    auto parseSourceFile = [&](Token&t)->AstNode* {
        AstSourceFile * node = new AstSourceFile;
        eat(KW_package, "a go source file must start with package declaration");
        grt.package = t.lexeme;
        eat(TK_ID, "name required at the package declaration");
        eat(OP_SEMI, "expect ; at the end of package declaration");
        while (t.type != TK_EOF) {
            switch (t.type) {
            case KW_import:node->importDecl.push_back(parseImportDecl(t)); break;
            case KW_const:node->constDecl.push_back(parseConstDecl(t)); break;
            case KW_type:node->typeDecl.push_back(parseTypeDecl(t)); break;
            case KW_var:node->varDecl.push_back(parseVarDecl(t)); break;
            case KW_func:node->funcDecl.push_back(parseFunctionDecl(t)); break;
            default:break;
            }
            if (t.type == OP_SEMI) {
                t = next(f);
            }
        }
        return node;
    };
    parseImportDecl = [&](Token&t)->AstImportDecl* {
        auto node = new AstImportDecl;
        eat(KW_import, "it should be import declaration");
        if (t.type == OP_LPAREN) {
            t = next(f);
            do {
                string importName, alias;
                if (t.type == OP_DOT || t.type == TK_ID) {
                    alias = t.lexeme;
                    t = next(f);
                    importName = t.lexeme;
                }
                else {
                    importName = t.lexeme;
                }
                importName = importName.substr(1, importName.length() - 2);
                node->imports[importName] = alias;
                t = next(f);
                if (t.type == OP_SEMI) {
                    t = next(f);
                }
            } while (t.type != OP_RPAREN);
            eat(OP_RPAREN, "expect )");
        }
        else {
            string importName, alias;
            if (t.type == OP_DOT || t.type == TK_ID) {
                alias = t.lexeme;
                t = next(f);
                importName = t.lexeme;
                t = next(f);
            }
            else {
                importName = t.lexeme;
                t = next(f);
            }
            importName = importName.substr(1, importName.length() - 2);
            node->imports[importName] = alias;
        }
        return node;
    };
    parseConstDecl = [&](Token&t)->AstConstDecl* {
        AstConstDecl * node = new AstConstDecl;
        eat(KW_const, "it should be const declaration");
        if (t.type == OP_LPAREN) {
            t = next(f);
            do {
                node->identList.push_back(parseIdentifierList(t));
                if (auto*tmp = parseType(t); tmp != nullptr) {
                    node->type.push_back(tmp);
                }
                else {
                    node->type.push_back(nullptr);
                }
                if (t.type == OP_AGN) {
                    t = next(f);
                    node->exprList.push_back(parseExprList(t));
                }
                else {
                    node->exprList.push_back(nullptr);
                }
                if (t.type == OP_SEMI) {
                    t = next(f);
                }
            } while (t.type != OP_RPAREN);
            eat(OP_RPAREN, "eat right parenthesis");
        }
        else {
            node->identList.push_back(parseIdentifierList(t));
            if (auto*tmp = parseType(t); tmp != nullptr) {
                node->type.push_back(tmp);
                t = next(f);
            }
            else {
                node->type.push_back(nullptr);
            }
            if (t.type == OP_AGN) {
                t = next(f);
                node->exprList.push_back(parseExprList(t));
            }
            else {
                node->exprList.push_back(nullptr);
            }
            if (t.type != OP_SEMI) {

                throw runtime_error("expect an explicit semicolon");
            }
        }

        return node;
    };
    parseTypeDecl = [&](Token&t)->AstTypeDecl* {
        AstTypeDecl* node = new AstTypeDecl;
        eat(KW_type, "it should be type declaration");
        if (t.type == OP_LPAREN) {
            t = next(f);
            do {
                node->typeSpec.push_back(parseTypeSpec(t));
                if (t.type == OP_SEMI) {
                    t = next(f);
                }
            } while (t.type != OP_RPAREN);
            eat(OP_RPAREN, "expect )");
        }
        else {
            node->typeSpec.push_back(parseTypeSpec(t));
        }
        return node;
    };
    parseTypeSpec = [&](Token&t)->AstNode* {
        AstTypeSpec* node = nullptr;
        if (t.type == TK_ID) {
            node = new AstTypeSpec;
            node->identifier = t.lexeme;
            t = next(f);
            if (t.type == OP_AGN) {
                t = next(f);
            }
            node->type = parseType(t);
        }
        return node;
    };
    parseVarDecl = [&](Token&t)->AstVarDecl* {
        AstVarDecl* node = new AstVarDecl;
        eat(KW_var, "it should be var declaration");
        if (t.type == OP_LPAREN) {
            do {
                node->varSpec.push_back(parseVarSpec(t));
                t = next(f);
            } while (t.type != OP_RPAREN);
            eat(OP_RPAREN, "expect )");
        }
        else {
            node->varSpec.push_back(parseVarSpec(t));
        }

        return node;
    };
    parseVarSpec = [&](Token&t)->AstNode* {
        AstVarSpec* node = nullptr;
        if (auto*tmp = parseIdentifierList(t); tmp != nullptr) {
            node = new AstVarSpec;
            node->identList = tmp;
            if (auto * tmp1 = parseType(t); tmp1 != nullptr) {
                node->avs.named.type = tmp1;
                t = next(f);
                if (t.type == OP_AGN) {
                    t = next(f);
                    node->avs.named.exprList = parseExprList(t);
                }
            }
            else if (t.type == OP_AGN) {
                t = next(f);
                node->avs.exprList = parseExprList(t);
            }
        }
        return node;
    };
    parseFunctionDecl = [&](Token&t)->AstFunctionDecl* {
        AstFunctionDecl * node = new AstFunctionDecl;
        eat(KW_func, "it should be func declaration");
        if (t.type == OP_LPAREN) {
            node->receiver = parseParameter(t);
        }
        node->funcName = t.lexeme;
        t = next(f);
        node->signature = parseSignature(t);
        node->functionBody = parseBlock(t);

        return node;
    };
    parseFunctionType = [&](Token&t)->AstNode* {
        AstFunctionType* node = nullptr;
        if (t.type == KW_func) {
            node = new AstFunctionType;
            t = next(f);
            node->signature = parseSignature(t);
        }
        return node;
    };
    parseSignature = [&](Token&t)->AstSignature* {
        AstSignature* node = nullptr;
        if (t.type == OP_LPAREN) {
            node = new AstSignature;
            node->parameters = parseParameter(t);
            node->result = parseResult(t);
        }
        return node;
    };
    parseParameter = [&](Token&t)->AstNode* {
        AstParameter* node = nullptr;
        if (t.type == OP_LPAREN) {
            node = new AstParameter;
            t = next(f);
            do {
                if (auto * tmp = parseParameterDecl(t); tmp != nullptr) {
                    node->parameterList.push_back(tmp);
                }
                if (t.type == OP_COMMA) {
                    t = next(f);
                }
            } while (t.type != OP_RPAREN);
            t = next(f);

            for (int i = 0, rewriteStart = 0; i < node->parameterList.size(); i++) {
                if (dynamic_cast<AstParameterDecl*>(node->parameterList[i])->hasName) {
                    for (int k = rewriteStart; k < i; k++) {
                        string name = dynamic_cast<AstName*>(
                            dynamic_cast<AstType*>(dynamic_cast<AstParameterDecl*>(node->parameterList[k])->type)->type)->name;
                        dynamic_cast<AstParameterDecl*>(node->parameterList[k])->type = dynamic_cast<AstParameterDecl*>(node->parameterList[i])->type;
                        dynamic_cast<AstParameterDecl*>(node->parameterList[k])->name = name;
                        dynamic_cast<AstParameterDecl*>(node->parameterList[k])->hasName = true; //It's not necessary
                    }
                    rewriteStart = i + 1;
                }
            }
        }
        return node;
    };
    parseParameterDecl = [&](Token&t)->AstNode* {
        AstParameterDecl* node = nullptr;
        if (t.type == OP_VARIADIC) {
            node = new AstParameterDecl;
            node->isVariadic = true;
            t = next(f);
            node->type = parseType(t);
        }
        else if (t.type != OP_RPAREN) {
            node = new AstParameterDecl;
            auto*mayIdentOrType = parseType(t);
            if (t.type != OP_COMMA && t.type != OP_RPAREN) {
                node->hasName = true;
                if (t.type == OP_VARIADIC) {
                    node->isVariadic = true;
                    t = next(f);
                }
                node->name = dynamic_cast<AstName*>(dynamic_cast<AstType*>(mayIdentOrType)->type)->name;
                node->type = parseType(t);
            }
            else {
                node->type = mayIdentOrType;
            }
        }
        return node;
    };
    parseResult = [&](Token&t)->AstNode* {
        AstResult* node = nullptr;
        if (auto*tmp = parseParameter(t); tmp != nullptr) {
            node = new AstResult;
            node->parameter = tmp;
        }
        else  if (auto*tmp = parseType(t); tmp != nullptr) {
            node = new AstResult;
            node->type = tmp;
        }
        return node;
    };
#pragma endregion
#pragma region Type
    parseType = [&](Token&t)->AstType* {
        AstType * node = nullptr;
        switch (t.type) {
        case TK_ID: node = new AstType; node->type = parseName(t); break;
        case OP_LBRACKET: node = new AstType; node->type = parseArrayOrSliceType(t); break;
        case KW_struct: node = new AstType; node->type = parseStructType(t); break;
        case OP_MUL: node = new AstType; node->type = parsePointerType(t); break;
        case KW_func: node = new AstType; node->type = parseFunctionType(t); break;
        case KW_interface: node = new AstType; node->type = parseInterfaceType(t); break;
        case KW_map: node = new AstType; node->type = parseMapType(t); break;
        case KW_chan: node = new AstType; node->type = parseChannelType(t); break;
        case OP_LPAREN:
            t = next(f);
            node = dynamic_cast<AstType*>(parseType(t));
            t = next(f);
            break;
        default:break;
        }

        return node;
    };
    parseArrayOrSliceType = [&](Token&t)->AstNode* {
        AstNode* node = nullptr;
        eat(OP_LBRACKET, "array/slice type requires [ to denote that");
        if (t.type != OP_RBRACKET) {
            node = new AstArrayType;
            dynamic_cast<AstArrayType*>(node)->length = parseExpr(t);
            t = next(f);
            dynamic_cast<AstArrayType*>(node)->elementType = parseType(t);
        }
        else {
            node = new AstSliceType;
            t = next(f);
            dynamic_cast<AstSliceType*>(node)->elementType = parseType(t);
        }
        return node;
    };
    parseStructType = [&](Token&t)->AstNode* {
        AstStructType* node = new  AstStructType;
        eat(KW_struct, "structure type requires struct keyword");
        eat(OP_LBRACE, "a { is required after struct");
        do {
            AstStructType::_FieldDecl fd{};
            if (auto * tmp = parseIdentifierList(t); tmp != nullptr) {
                fd.named.identList = tmp;
                fd.named.type = parseType(t);
            }
            else {
                if (t.type == OP_MUL) {
                    t = next(f);
                }
                fd.typeName = parseName(t);
            }
            string tag;
            if (t.type == LITERAL_STR) {
                tag = t.lexeme;
            }
            node->fields.emplace_back(fd, tag);
            if (t.type == OP_SEMI) {
                t = next(f);
            }
        } while (t.type != OP_RBRACE);
        eat(OP_RBRACE, "expect }");
        eat(OP_SEMI, "expect ;");

        return node;
    };
    parsePointerType = [&](Token&t)->AstNode* {
        AstPointerType* node = new AstPointerType;
        eat(OP_MUL, "pointer type requires * to denote that");
        node->baseType = parseType(t);
        return node;
    };
    parseInterfaceType = [&](Token&t)->AstNode* {
        AstInterfaceType* node = new AstInterfaceType;
        eat(KW_interface, "interface type requires keyword interface");
        eat(OP_LBRACE, "{ is required after interface");
        while (t.type != OP_RBRACE) {
            if (auto*tmp = parseMethodSpec(t); tmp != nullptr) {
                node->methodSpec.push_back(tmp);
                if (t.type == OP_SEMI) {
                    t = next(f);
                }
            }
        }
        t = next(f);
        return node;
    };
    parseMethodSpec = [&](Token&t)->AstNode* {
        AstMethodSpec* node = new AstMethodSpec;
        if (auto* tmp = parseName(t); tmp != nullptr && tmp->name.find(".") == string::npos) {
            node->methodName = tmp;
            node->signature = parseSignature(t);
        }
        else {
            node->methodName = tmp;
        }
        return node;
    };
    parseMapType = [&](Token&t)->AstNode* {
        AstMapType* node = new AstMapType;
        eat(KW_map, "map type requires keyword map to denote that");
        eat(OP_LBRACKET, "[ is required after map");
        node->keyType = parseType(t);
        eat(OP_RBRACKET, "] is required after map[Key");
        node->elementType = parseType(t);
        return node;
    };
    parseChannelType = [&](Token&t)->AstNode* {
        //ChannelType = ( "chan" | "chan" "<-" | "<-" "chan" ) ElementType .
        AstChannelType* node = nullptr;
        if (t.type == KW_chan) {
            node = new AstChannelType;
            t = next(f);
            if (t.type == OP_CHAN) {
                t = next(f);
                node->elementType = parseType(t);
            }
            else {
                node->elementType = parseType(t);
            }
        }
        else if (t.type == OP_CHAN) {
            node = new AstChannelType;
            t = next(f);
            if (t.type == KW_chan) {
                node->elementType = parseType(t);
            }
        }
        return node;
    };
#pragma endregion
#pragma region Statement
    parseStmt = [&](Token&t)->AstStmt* {
        AstStmt * node = nullptr;
        switch (t.type) {
        case KW_type: node = new AstStmt; node->stmt = parseTypeDecl(t); break;
        case KW_const: node = new AstStmt; node->stmt = parseConstDecl(t); break;
        case KW_var: node = new AstStmt; node->stmt = parseVarDecl(t); break;

        case KW_go: {t = next(f); node = new AstStmt; node->stmt = new AstGoStmt(parseExpr(t)); break; }
        case KW_return: {t = next(f); node = new AstStmt; node->stmt = new AstReturnStmt(parseExprList(t)); break; }
        case KW_break: {t = next(f); node = new AstStmt; node->stmt = new AstBreakStmt(t.type == TK_ID ? t.lexeme : ""); break; }
        case KW_continue: {t = next(f); node = new AstStmt; node->stmt = new AstContinueStmt(t.type == TK_ID ? t.lexeme : ""); break; }
        case KW_goto: {t = next(f); node = new AstStmt; node->stmt = new AstGotoStmt(t.lexeme); break; }
        case KW_fallthrough: {t = next(f); node = new AstStmt; node->stmt = new AstFallthroughStmt(); break; }
        case KW_defer: {t = next(f); node = new AstStmt; node->stmt = new AstDeferStmt(parseExpr(t)); break; }

        case KW_if: node = new AstStmt; node->stmt = parseIfStmt(t); break;
        case KW_switch: node = new AstStmt; node->stmt = parseSwitchStmt(t); break;
        case KW_select: node = new AstStmt; node->stmt = parseSelectStmt(t); break;
        case KW_for: node = new AstStmt; node->stmt = parseForStmt(t); break;
        case OP_LBRACE: node = new AstStmt;  node->stmt = parseBlock(t); break;
        case OP_SEMI: break;//empty statement

        case OP_ADD:case OP_SUB:case OP_NOT:case OP_XOR:case OP_MUL:case OP_CHAN:
        case LITERAL_STR:case LITERAL_INT:case LITERAL_IMG:case LITERAL_FLOAT:case LITERAL_RUNE:
        case KW_func:
        case KW_struct:case KW_map:case OP_LBRACKET:case TK_ID: case OP_LPAREN:
        {
            auto* exprList = parseExprList(t);
            node = new AstStmt;
            if (t.type == OP_COLON) {
                //it shall a labeled statement(not part of simple stmt so we handle it here)
                t = next(f);
                AstLabeledStmt * labeledStmt = new AstLabeledStmt;
                labeledStmt->label = "todo";//todo:rewrite it
                labeledStmt->statement = parseStmt(t);
                node->stmt = labeledStmt;
            }
            else {
                node->stmt = parseSimpleStmt(exprList, t);
            }
            break;
        }
        default:break;
        }
        return node;
    };
    parseSimpleStmt = [&](AstExprList* lhs, Token&t)->AstNode* {
        AstSimpleStmt * node = new AstSimpleStmt;
        if (lhs == nullptr) {
            lhs = parseExprList(t);
        }

        switch (t.type) {
        case OP_CHAN: {
            if (lhs->exprList.size() != 1) throw runtime_error("one expr required");
            t = next(f);
            auto* stmt = new AstSendStmt;
            stmt->receiver = lhs->exprList[0];
            stmt->sender = parseExpr(t);
            node->stmt = stmt;
            break;
        }
        case OP_INC:case OP_DEC: {
            if (lhs->exprList.size() != 1) throw runtime_error("one expr required");
            auto* stmt = new AstIncDecStmt;
            stmt->isInc = t.type == OP_INC ? true : false;
            t = next(f);
            stmt->expression = lhs->exprList[0];
            node->stmt = stmt;
            break;
        }
        case OP_SHORTAGN: {
            if (lhs->exprList.size() == 0) throw runtime_error("one expr required");
            auto*stmt = new AstShortAssign;
            for (auto* e : lhs->exprList) {

                string identName = dynamic_cast<AstName*>(
                    dynamic_cast<AstCompositeLit*>(
                        dynamic_cast<AstOperand*>(
                            dynamic_cast<AstPrimaryExpr*>(e->lhs->expr)->expr
                            )->operand
                        )->literalType
                    )->name;

                stmt->lhs.push_back(identName);
            }
            t = next(f);
            stmt->rhs = parseExprList(t);
            node->stmt = stmt;
            break;
        }
        case OP_ADDAGN:case OP_SUBAGN:case OP_BITORAGN:case OP_BITXORAGN:case OP_MULAGN:case OP_DIVAGN:
        case OP_MODAGN:case OP_LSFTAGN:case OP_RSFTAGN:case OP_BITANDAGN:case OP_ANDXORAGN:case OP_AGN: {
            if (lhs->exprList.size() == 0) throw runtime_error("one expr required");
            auto* stmt = new AstAssign;
            stmt->lhs = lhs;
            stmt->op = t.type;
            t = next(f);
            stmt->rhs = parseExprList(t);
            node->stmt = stmt;
            break;
        }
        default: {//ExprStmt
            if (lhs->exprList.size() != 1) throw runtime_error("one expr required");
            auto* stmt = new AstExpressionStmt;
            stmt->expression = lhs->exprList[0];
            node->stmt = stmt;
            break;
        }
        }
        return node;
    };
    parseBlock = [&](Token&t)->AstNode* {
        AstBlock * node = nullptr;
        if (t.type == OP_LBRACE) {
            node = new AstBlock;
            t = next(f);
            if (t.type != OP_RBRACE) {
                node->statementList = parseStmtList(t);
                eat(OP_RBRACE, "expect } around code block");
            }
            else {
                t = next(f);
            }
        }
        return node;
    };
    parseIfStmt = [&](Token&t)->AstNode* {
        AstIfStmt* node = nullptr;
        if (t.type == KW_fallthrough) {
            node = new AstIfStmt;
            if (auto* tmp = parseSimpleStmt(nullptr, t); tmp != nullptr) {
                node->condition = tmp;
                expect(OP_SEMI, "expect an semicolon in condition part of if");
            }
            t = next(f);
            node->expression = parseExpr(t);
            t = next(f);
            node->block = parseBlock(t);
            t = next(f);
            if (t.type == KW_else) {
                t = next(f);
                if (auto *tmp1 = parseIfStmt(t); tmp1 != nullptr) {
                    node->ais.ifStmt = tmp1;
                }
                else if (auto *tmp1 = parseBlock(t); tmp1 != nullptr) {
                    node->ais.block = tmp1;
                }
                else {
                    throw runtime_error("else is empty");
                }
            }
        }
        return node;
    };
    parseSwitchStmt = [&](Token&t)->AstNode* {
        AstSwitchStmt* node = nullptr;
        if (t.type == KW_switch) {
            node = new AstSwitchStmt;
            if (auto*tmp = parseSimpleStmt(nullptr, t); tmp != nullptr) {
                node->condition = tmp;
                expect(OP_SEMI, "expect semicolon in switch condition");
                if (auto*tmp1 = parseExpr(t); tmp1 != nullptr) {
                    node->conditionExpr = tmp1;
                    t = next(f);
                }
            }
            expect(OP_LBRACE, "expect left brace around case clauses");
            do {
                if (auto*tmp = parseExprCaseClause(t); tmp != nullptr) {
                    node->exprCaseClause.push_back(tmp);
                }
                t = next(f);
            } while (t.type != OP_RBRACE);
        }
        return node;
    };
    parseExprCaseClause = [&](Token&t)->AstNode* {
        AstExprCaseClause* node = nullptr;
        if (auto*tmp = parseExprSwitchCase(t); tmp != nullptr) {
            node = new AstExprCaseClause;
            node->exprSwitchCase = tmp;
            expect(OP_COLON, "expect colon in case clause of switch");
            node->statementList = parseStmtList(t);
        }
        return node;
    };
    parseExprSwitchCase = [&](Token&t)->AstNode* {
        AstExprSwitchCase* node = nullptr;
        if (t.type == KW_case) {
            node = new AstExprSwitchCase;
            t = next(f);
            if (auto*tmp = parseExprList(t); tmp != nullptr) {
                node->exprList = tmp;
            }
            else if (t.type == KW_default) {
                node->isDefault = true;
            }
        }
        return node;
    };
    parseSelectStmt = [&](Token&t)->AstNode* {
        AstSelectStmt* node = nullptr;
        if (t.type == KW_select) {
            node = new AstSelectStmt;
            expect(OP_LBRACE, "expect left brace in select statement");
            do {
                if (auto*tmp = parseCommClause(t); tmp != nullptr) {
                    node->commClause.push_back(tmp);
                }
                t = next(f);
            } while (t.type != OP_RBRACE);
        }
        return node;
    };
    parseCommClause = [&](Token&t)->AstNode* {
        AstCommClause* node = nullptr;
        if (auto*tmp = parseCommCase(t); tmp != nullptr) {
            node = new AstCommClause;
            node->commCase = tmp;
            expect(OP_COLON, "expect colon in select case clause");
            node->statementList = parseStmtList(t);
        }
        return node;
    };
    parseCommCase = [&](Token&t)->AstNode* {
        AstCommCase*node = nullptr;
        if (t.type == KW_case) {
            node = new AstCommCase;
            t = next(f);
            //todo
            /*if (auto*tmp = parseSendStmt(t); tmp != nullptr) {
                node->acc.sendStmt = tmp;
            }
            else if (auto*tmp = parseRecvStmt(t); tmp != nullptr) {
                node->acc.recvStmt = tmp;
            }
            else if (t.type == KW_default) {
                node->isDefault = true;
            }*/
        }
        return node;
    };
    parseRecvStmt = [&](Token&t)->AstNode* {
        AstRecvStmt*node = nullptr;
        if (auto*tmp = parseExprList(t); tmp != nullptr) {
            node = new AstRecvStmt;
            node->ars.exprList = tmp;
            expect(OP_EQ, "expect =");
            node->recvExpr = parseExpr(t);
        }
        else if (auto*tmp = parseIdentifierList(t); tmp != nullptr) {
            node = new AstRecvStmt;
            node->ars.identList = tmp;
            expect(OP_SHORTAGN, "expect :=");
            node->recvExpr = parseExpr(t);
        }
        return node;
    };
    parseForStmt = [&](Token&t)->AstNode* {
        AstForStmt* node = nullptr;
        if (t.type == KW_for) {
            node = new AstForStmt;
            t = next(f);
            if (auto*tmp = parseExpr(t); tmp != nullptr) {
                node->afs.condition = tmp;
            }
            else if (auto*tmp = parseForClause(t); tmp != nullptr) {
                node->afs.forClause = tmp;
            }
            else if (auto*tmp = parseRangeClause(t); tmp != nullptr) {
                node->afs.rangeClause = tmp;
            }
            t = next(f);
            node->block = parseBlock(t);
        }
        return node;
    };
    parseForClause = [&](Token&t)->AstNode* {
        AstForClause * node = nullptr;
        if (auto*tmp = parseSimpleStmt(nullptr, t); tmp != nullptr) {
            node = new AstForClause;
            node->initStmt = tmp;
            expect(OP_SEMI, "expect semicolon in for clause");
            node->condition = parseExpr(t);
            expect(OP_SEMI, "expect semicolon in for clause");
            node->postStmt = parseSimpleStmt(nullptr, t);
        }
        return node;
    };
    parseRangeClause = [&](Token&t)->AstNode* {
        AstRangeClause*node = nullptr;
        if (auto*tmp = parseExprList(t); tmp != nullptr) {
            node = new AstRangeClause;
            node->arc.exprList = tmp;
            expect(OP_EQ, "expect =");
            t = next(f);
        }
        else if (auto* tmp = parseIdentifierList(t); tmp != nullptr) {
            node = new AstRangeClause;
            node->arc.identList = tmp;
            expect(OP_SHORTAGN, "expect :=");
            t = next(f);
        }
        if (t.type == KW_range) {
            if (node == nullptr) {
                node = new AstRangeClause;
            }
            t = next(f);
            node->expression = parseExpr(t);
        }
        return node;
    };
#pragma endregion
#pragma region Expression
    parseExpr = [&](Token&t)->AstExpr* {
        AstExpr* node = nullptr;
        if (auto*tmp = parseUnaryExpr(t); tmp != nullptr) {
            node = new  AstExpr;
            node->lhs = tmp;
            if (t.type == OP_OR || t.type == OP_AND || t.type == OP_EQ ||
                t.type == OP_NE || t.type == OP_LT || t.type == OP_LE ||
                t.type == OP_GT || t.type == OP_GE || t.type == OP_ADD ||
                t.type == OP_SUB || t.type == OP_BITOR || t.type == OP_XOR ||
                t.type == OP_MUL || t.type == OP_DIV || t.type == OP_MOD ||
                t.type == OP_LSHIFT || t.type == OP_RSHIFT || t.type == OP_BITAND ||
                t.type == OP_XOR) {
                node->op = t.type;
                t = next(f);
                node->rhs = parseExpr(t);
            }
        }
        return node;
    };
    parseUnaryExpr = [&](Token&t)->AstUnaryExpr* {
        AstUnaryExpr* node = nullptr;
        switch (t.type) {
        case OP_ADD:case OP_SUB:case OP_NOT:
        case OP_XOR:case OP_MUL:case OP_BITAND:case OP_CHAN:
            node = new AstUnaryExpr;
            node->op = t.type;
            t = next(f);
            node->expr = parseUnaryExpr(t);
            break;
        default:
            node = new AstUnaryExpr;
            node->expr = parsePrimaryExpr(t);
            break;
        }
        return node;
    };
    parsePrimaryExpr = [&](Token&t)->AstPrimaryExpr* {
        AstPrimaryExpr*node = nullptr;
        if (auto*tmp = parseOperand(t); tmp != nullptr) {
            node = new AstPrimaryExpr;
            // eliminate left-recursion by infinite loop; these code referenced golang official impl
            // since this work requires somewhat familiarity of golang syntax
            while (true) {
                if (t.type == OP_DOT) {
                    t = next(f);
                    if (t.type == TK_ID) {
                        auto* e = new AstSelectorExpr;
                        e->operand = tmp;
                        e->selector = t.lexeme;
                        tmp = e;
                        t = next(f);
                    }
                    else if (t.type == OP_LPAREN) {
                        t = next(f);
                        if (t.lexeme == "type") {
                            auto* e = new AstTypeSwitchGuardExpr;
                            e->operand = tmp;
                            tmp = e;
                            t = next(f);
                        }
                        else {
                            auto* e = new AstTypeAssertionExpr;
                            e->operand = tmp;
                            e->type = parseType(t);
                            tmp = e;
                        }
                        eat(OP_RPAREN, "expect )");
                    }
                }
                else if (t.type == OP_LBRACKET) {
                    t = next(f);
                    AstNode* start = nullptr;//Ignore start if next token is :(syntax of operand[:xxx])
                    if (t.type != OP_COLON) {
                        // operand[6] index syntax
                        start = parseExpr(t);
                        if (t.type == OP_RBRACKET) {
                            auto* e = new AstIndexExpr;
                            e->operand = tmp;
                            e->index = start;
                            tmp = e;
                            t = next(f);
                            continue;
                        }

                    }
                    auto* sliceExpr = new AstSliceExpr;
                    sliceExpr->operand = tmp;
                    sliceExpr->begin = start;
                    eat(OP_COLON, "expect : in slice expression");
                    sliceExpr->end = parseExpr(t);//may nullptr
                    if (t.type == OP_COLON) {
                        t = next(f);
                        sliceExpr->step = parseExpr(t);
                        eat(OP_RBRACKET, "expect ] at the end of slice expression");
                    }
                    else if (t.type == OP_RBRACKET) {
                        t = next(f);
                    }
                    tmp = sliceExpr;
                }
                else if (t.type == OP_LPAREN) {
                    auto* e = new AstCallExpr;
                    e->operand = tmp;
                    if (auto*tmp1 = parseExprList(t); tmp1 != nullptr) {
                        e->arguments = tmp1;
                    }
                    else if (auto*tmp1 = parseType(t); tmp1 != nullptr) {
                        e->type = tmp1;
                        t = next(f);
                        if (t.type == OP_COMMA) {
                            e->arguments = parseExprList(t);
                        }
                    }
                    if (t.type == OP_VARIADIC) {
                        e->isVariadic = true;
                        t = next(f);
                    }
                    tmp = e;
                }
                else {
                    break;
                }
            }
            node->expr = tmp;
        }
        return node;
    };
    parseOperand = [&](Token&t)->AstNode* {
        AstOperand*node = nullptr;
        switch (t.type) {
        case LITERAL_INT:
        case LITERAL_FLOAT:
        case LITERAL_IMG:
        case LITERAL_RUNE:
        case LITERAL_STR:
            node = new AstOperand; node->operand = parseBasicLit(t); break;
        case KW_struct:case KW_map:case OP_LBRACKET:case TK_ID:
            node = new AstOperand; node->operand = parseCompositeLit(t); break;
        case KW_func:
            node = new AstOperand; node->operand = parseFunctionLit(t); break;
        case OP_LPAREN:
            node = new AstOperand; t = next(f); node->operand = parseExpr(t); eat(OP_RPAREN, "expect )"); break;
        default:
            break;
        }
        return node;
    };

    parseBasicLit = [&](Token&t)->AstNode* {
        AstBasicLit* node = new AstBasicLit;
        node->type = t.type;
        node->value = t.lexeme;
        t = next(f);
        return node;
    };
    parseCompositeLit = [&](Token&t)->AstNode* {
        AstCompositeLit* node = nullptr;
        switch (t.type) {
        case KW_struct: node = new AstCompositeLit; node->literalType = parseStructType(t); node->literalValue = parseLiteralValue(t); break;
        case KW_map: node = new AstCompositeLit; node->literalType = parseMapType(t); node->literalValue = parseLiteralValue(t); break;
        case TK_ID: node = new AstCompositeLit; node->literalType = parseName(t); node->literalValue = parseLiteralValue(t); break;
        case OP_LBRACKET: {//slice or array 
            t = next(f);
            AstNode* tmp = nullptr;
            if (t.type == OP_VARIADIC) {
                tmp = new AstArrayType;
                dynamic_cast<AstArrayType*>(tmp)->automaticLen = true;
                eat(OP_VARIADIC, "expect ... in array declaration");
                eat(OP_RBRACKET, "array/slice requires ]");
                dynamic_cast<AstArrayType*>(tmp)->elementType = parseType(t);
            }
            else {
                if (t.type != OP_RBRACKET) {
                    tmp = new AstArrayType;
                    dynamic_cast<AstArrayType*>(tmp)->length = parseExpr(t);
                    dynamic_cast<AstArrayType*>(tmp)->elementType = parseType(t);
                }
                else {
                    t = next(f);
                    tmp = new AstSliceType;
                    dynamic_cast<AstSliceType*>(tmp)->elementType = parseType(t);
                }

            }
            node = new AstCompositeLit;
            node->literalType = tmp;
            node->literalValue = parseLiteralValue(t);
            break;
        }
        default:throw runtime_error("should not reach here");
        }


        return node;
    };
    parseLiteralValue = [&](Token&t)->AstLiteralValue* {
        AstLiteralValue*node = nullptr;
        if (t.type == OP_LBRACE) {
            node = new AstLiteralValue;
            do {
                t = next(f);
                if (t.type == OP_RBRACE) {
                    // it's necessary since both {a,b} or {a,b,} are legal form
                    break;
                }
                node->keyedElement.push_back(parseKeyedElement(t));
            } while (t.type != OP_RBRACE);
            eat(OP_RBRACE, "brace {} must match");
        }
        return node;
    };
    parseKeyedElement = [&](Token&t)->AstNode* {
        AstKeyedElement*node = nullptr;
        if (auto*tmp = parseKey(t); tmp != nullptr) {
            node = new AstKeyedElement;
            node->element = tmp;
            if (t.type == OP_COLON) {
                node->key = tmp;
                t = next(f);
                if (auto*tmp1 = parseElement(t); tmp1 != nullptr) {
                    node->element = tmp1;
                }
            }
        }
        return node;
    };
    parseKey = [&](Token&t)->AstNode* {
        AstKey*node = nullptr;
        if (auto*tmp = parseFieldName(t); tmp != nullptr) {
            node = new AstKey;
            node->ak.fieldName = tmp;
        }
        else if (auto*tmp = parseLiteralValue(t); tmp != nullptr) {
            node = new AstKey;
            node->ak.literalValue = tmp;
        }
        else if (auto*tmp = parseExpr(t); tmp != nullptr) {
            node = new AstKey;
            node->ak.expression = tmp;
        }
        return node;
    };
    parseFieldName = [&](Token&t)->AstNode* {
        AstFieldName* node = nullptr;
        if (t.type == TK_ID) {
            node = new AstFieldName;
            node->fieldName = t.lexeme;
            t = next(f);
        }
        return node;
    };
    parseElement = [&](Token&t)->AstNode* {
        AstElement*node = nullptr;
        if (auto*tmp = parseExpr(t); tmp != nullptr) {
            node = new AstElement;
            node->ae.expression = tmp;
        }
        else if (auto*tmp = parseLiteralValue(t); tmp != nullptr) {
            node = new AstElement;
            node->ae.literalValue = tmp;
        }
        return node;
    };
    parseFunctionLit = [&](Token&t)->AstNode* {
        AstFunctionLit* node = nullptr;
        if (t.type == KW_func) {
            node = new AstFunctionLit;
            t = next(f);
            node->signature = parseSignature(t);
            node->functionBody = parseBlock(t);
        }
        return node;
    };
#pragma endregion
    // parsing startup
    return parseSourceFile(t);
}

void emitStub() {}

void runtimeStub() {}

//===----------------------------------------------------------------------===//
// debug auxiliary functions, they are not part of 5 functions
//===----------------------------------------------------------------------===//
void printLex(const string & filename) {
    fstream f(filename, ios::binary | ios::in);
    while (lastToken != TK_EOF) {
        auto[token, lexeme] = next(f);
        fprintf(stdout, "<%d,%s,%d,%d>\n", token, lexeme.c_str(), line, column);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argv[1] == nullptr) {
        fprintf(stderr, "specify your go source file\n");
        return 1;
    }
    printLex(argv[1]);
    const AstNode* ast = parse(argv[1]);
    fprintf(stdout, "parsing passed\n");
    return 0;
}