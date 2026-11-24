#pragma once

#include <ostream>
#include "Visitor.hpp"

// Diapositivas 23 y 26: imprimir es una operacion separada de los nodos.
class AstPrinter : public Visitor<void> {
public:
    explicit AstPrinter(std::ostream& os) : os(os) {}

    void visitIntLiteral(const IntLiteral& e) override;
    void visitIdentExpr(const IdentExpr& e) override;
    void visitBinaryExpr(const BinaryExpr& e) override;
    void visitAssign(const Assign& s) override;
    void visitProgram(const Program& p) override;

private:
    std::ostream& pad();

    std::ostream& os;
    int indent = 0;
};
