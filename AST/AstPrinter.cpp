#include "AstPrinter.hpp"

std::ostream& AstPrinter::pad()
{
    return os << std::string(indent, ' ');
}

void AstPrinter::visitIntLiteral(const IntLiteral& e)
{
    pad() << "Int " << e.value << '\n';
}

void AstPrinter::visitIdentExpr(const IdentExpr& e)
{
    pad() << "Ident " << e.name << '\n';
}

void AstPrinter::visitBinaryExpr(const BinaryExpr& e)
{
    pad() << "Binary " << toString(e.op) << '\n';
    indent += 2;
    visit(*e.left);
    visit(*e.right);
    indent -= 2;
}

void AstPrinter::visitAssign(const Assign& s)
{
    pad() << "Assign " << s.name << '\n';
    indent += 2;
    visit(*s.value);
    indent -= 2;
}

void AstPrinter::visitProgram(const Program& p)
{
    pad() << "Program\n";
    indent += 2;
    for (const auto& item : p.items) {
        visit(*item);
    }
    indent -= 2;
}
