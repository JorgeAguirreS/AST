#pragma once

#include "Visitor.hpp"

// Diapositiva 28: cada visita devuelve 1 + el tamano de sus subarboles.
class NodeCounter : public Visitor<int> {
public:
    int visitIntLiteral(const IntLiteral&) override { return 1; }
    int visitIdentExpr(const IdentExpr&) override { return 1; }

    int visitBinaryExpr(const BinaryExpr& e) override
    {
        return 1 + visit(*e.left) + visit(*e.right);
    }

    int visitAssign(const Assign& s) override
    {
        return 1 + visit(*s.value);
    }

    int visitProgram(const Program& p) override
    {
        int count = 1; // Tambien cuenta la raiz Program.
        for (const auto& item : p.items) {
            count += visit(*item);
        }
        return count;
    }
};
