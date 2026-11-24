#include "Parser.hpp"

Parser::Parser(Lexer& lex)
    : lexer(lex), token(TokenId::Unknown, "")
{
}

ProgramPtr Parser::parse()
{
    nextToken();
    ProgramPtr root = program();
    expect(TokenId::EndOfFile, "Expected end of input but found " + describe(token));
    return root;
}

// program → assign+
ProgramPtr Parser::program()
{
    SourceLocation loc{token.line, token.column};
    std::vector<StmtPtr> items;
    items.push_back(assign()); // + exige al menos una asignacion.
    while (token.tokenId == TokenId::Ident) {
        items.push_back(assign());
    }
    return std::make_unique<Program>(loc, std::move(items));
}

// assign → IDENT "=" expr ";"
StmtPtr Parser::assign()
{
    Token name = expect(TokenId::Ident, "Expected identifier but found " + describe(token));
    expect(TokenId::OpEqual, "Expected '=' but found " + describe(token));
    ExprPtr value = expr();
    expect(TokenId::Semicolon, "Expected ';' but found " + describe(token));
    // '=' y ';' se consumen, pero no son nodos del AST (diapositiva 5).
    return std::make_unique<Assign>(
        SourceLocation{name.line, name.column}, name.text, std::move(value));
}

// expr → term ("+" term)*
ExprPtr Parser::expr()
{
    SourceLocation loc{token.line, token.column};
    ExprPtr lhs = term();
    while (token.tokenId == TokenId::OpAdd) {
        nextToken();
        ExprPtr rhs = term();
        // Diapositivas 18-19: el arbol anterior pasa a ser el hijo izquierdo.
        // a + b + c se construye como (a + b) + c.
        lhs = std::make_unique<BinaryExpr>(
            loc, BinaryOp::Add, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

// term → factor ("*" factor)*
ExprPtr Parser::term()
{
    SourceLocation loc{token.line, token.column};
    ExprPtr lhs = factor();
    while (token.tokenId == TokenId::OpMul) {
        nextToken();
        ExprPtr rhs = factor();
        lhs = std::make_unique<BinaryExpr>(
            loc, BinaryOp::Mul, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

// factor → IDENT | NUMBER | "(" expr ")"
// Equivale a primary() en la diapositiva 20.
ExprPtr Parser::factor()
{
    Token t = token;
    SourceLocation loc{t.line, t.column};
    if (token.tokenId == TokenId::Ident) {
        nextToken();
        return std::make_unique<IdentExpr>(loc, t.text);
    } else if (token.tokenId == TokenId::Number) {
        int value;
        try {
            value = std::stoi(t.text);
        } catch (const std::out_of_range&) {
            throw error("Integer literal out of range for int: " + t.text);
        }
        nextToken();
        return std::make_unique<IntLiteral>(loc, value);
    } else if (token.tokenId == TokenId::OpenParen) {
        nextToken();
        ExprPtr inner = expr();
        expect(TokenId::CloseParen, "Expected ')' but found " + describe(token));
        return inner; // Los parentesis agrupan, pero no crean un nodo.
    } else {
        throw error("Expected identifier, number or '(' but found " + describe(token));
    }
}

void Parser::nextToken()
{
    token = lexer.nextToken();
}

// Consumes the current token if it matches `expected`, otherwise reports `message`.
Token Parser::expect(TokenId expected, const std::string& message)
{
    if (token.tokenId != expected) {
        throw error(message);
    }
    Token matched = token;
    nextToken();
    return matched;
}

ParseError Parser::error(const std::string& message) const
{
    return ParseError(message, token.line, token.column);
}

std::string Parser::describe(const Token& tok)
{
    switch (tok.tokenId) {
        case TokenId::EndOfFile: return "end of input";
        case TokenId::Unknown: return "unknown character '" + tok.text + "'";
        default: return "'" + tok.text + "'";
    }
}
