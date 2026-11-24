#pragma once

#include "Ast.hpp"

// Diapositivas 26-27: R es el resultado de una operacion sobre el AST.
// El unico switch de despacho se comparte entre todas las operaciones.
template <class R>
class Visitor {
public:
    virtual ~Visitor() = default;

    R visit(const Node& n)
    {
        switch (n.kind()) {
            case NodeKind::IntLiteral:
                return visitIntLiteral(n.as<IntLiteral>());
            case NodeKind::IdentExpr:
                return visitIdentExpr(n.as<IdentExpr>());
            case NodeKind::BinaryExpr:
                return visitBinaryExpr(n.as<BinaryExpr>());
            case NodeKind::Assign:
                return visitAssign(n.as<Assign>());
            case NodeKind::Program:
                return visitProgram(n.as<Program>());
        }
        throw std::logic_error("unknown node kind");
    }

    // = 0 obliga a cada operacion a implementar una visita por nodo.
    virtual R visitIntLiteral(const IntLiteral&) = 0;
    virtual R visitIdentExpr(const IdentExpr&) = 0;
    virtual R visitBinaryExpr(const BinaryExpr&) = 0;
    virtual R visitAssign(const Assign&) = 0;
    virtual R visitProgram(const Program&) = 0;
};
