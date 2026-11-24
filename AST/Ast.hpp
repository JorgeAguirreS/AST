#pragma once

#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Diapositivas 10-16: una clase por construccion del lenguaje.
// Este parser usa el subconjunto de MiniLang con asignaciones, + y *.
struct SourceLocation {
    int line;
    int column;
};

enum class NodeKind {
    IntLiteral, IdentExpr, BinaryExpr, Assign, Program
};

enum class BinaryOp { Add, Mul };

inline const char* toString(BinaryOp op)
{
    switch (op) {
        case BinaryOp::Add: return "+";
        case BinaryOp::Mul: return "*";
    }
    throw std::logic_error("unknown binary operator");
}

class Node {
public:
    virtual ~Node() = default;
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    NodeKind kind() const { return kind_; }
    SourceLocation loc() const { return loc_; }

    template <class T>
    bool is() const { return kind_ == T::Kind; }

    template <class T>
    const T& as() const
    {
        // Primero se comprueba is<T>(), o se despacha por kind() en Visitor.
        assert(is<T>());
        return static_cast<const T&>(*this);
    }

protected:
    Node(NodeKind kind, SourceLocation loc) : kind_(kind), loc_(loc) {}

private:
    const NodeKind kind_;
    const SourceLocation loc_;
};

// Las bases tienen constructores protegidos: se crean los nodos concretos.
class Expr : public Node {
protected:
    Expr(NodeKind kind, SourceLocation loc) : Node(kind, loc) {}
};

class Stmt : public Node {
protected:
    Stmt(NodeKind kind, SourceLocation loc) : Node(kind, loc) {}
};

using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

class IntLiteral final : public Expr {
public:
    static constexpr NodeKind Kind = NodeKind::IntLiteral;
    const int value;

    IntLiteral(SourceLocation loc, int value)
        : Expr(Kind, loc), value(value) {}
};

class IdentExpr final : public Expr {
public:
    static constexpr NodeKind Kind = NodeKind::IdentExpr;
    const std::string name;

    IdentExpr(SourceLocation loc, std::string name)
        : Expr(Kind, loc), name(std::move(name)) {}
};

class BinaryExpr final : public Expr {
public:
    static constexpr NodeKind Kind = NodeKind::BinaryExpr;
    const BinaryOp op;
    const ExprPtr left;
    const ExprPtr right;

    BinaryExpr(SourceLocation loc, BinaryOp op, ExprPtr left, ExprPtr right)
        : Expr(Kind, loc), op(op),
          left(std::move(left)), right(std::move(right)) {}
};

// Asignar produce un efecto: Assign hereda de Stmt, no de Expr.
class Assign final : public Stmt {
public:
    static constexpr NodeKind Kind = NodeKind::Assign;
    const std::string name;
    const ExprPtr value;

    Assign(SourceLocation loc, std::string name, ExprPtr value)
        : Stmt(Kind, loc), name(std::move(name)), value(std::move(value)) {}
};

class Program final : public Node {
public:
    static constexpr NodeKind Kind = NodeKind::Program;
    const std::vector<StmtPtr> items;

    Program(SourceLocation loc, std::vector<StmtPtr> items)
        : Node(Kind, loc), items(std::move(items)) {}
};

using ProgramPtr = std::unique_ptr<Program>;
