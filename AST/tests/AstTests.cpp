#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include "AstPrinter.hpp"
#include "NodeCounter.hpp"
#include "Parser.hpp"

static void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static ProgramPtr parse(const std::string& source)
{
    std::istringstream input(source);
    Lexer lexer(input);
    Parser parser(lexer);
    return parser.parse(); // El AST debe sobrevivir al parser y al lexer.
}

static std::string dump(const Node& node)
{
    std::ostringstream output;
    AstPrinter printer(output);
    printer.visit(node);
    return output.str();
}

static const Assign& assignment(const Program& program, std::size_t index = 0)
{
    require(index < program.items.size(), "Missing assignment");
    require(program.items[index]->is<Assign>(), "Expected Assign");
    return program.items[index]->as<Assign>();
}

static const BinaryExpr& binary(const Node& node, BinaryOp op)
{
    require(node.is<BinaryExpr>(), "Expected BinaryExpr");
    const auto& expr = node.as<BinaryExpr>();
    require(expr.op == op, "Wrong operator");
    return expr;
}

static void checkLocation(const Node& node, int line, int column)
{
    require(node.loc().line == line && node.loc().column == column,
            "Wrong source location");
}

static void testClassExample()
{
    auto ast = parse("z = (y + x) * 3;");
    require(dump(*ast) ==
        "Program\n"
        "  Assign z\n"
        "    Binary *\n"
        "      Binary +\n"
        "        Ident y\n"
        "        Ident x\n"
        "      Int 3\n", "Class example tree differs from the slides");

    NodeCounter counter;
    require(counter.visit(assignment(*ast)) == 6, "Slide 28 counts six nodes");
    require(counter.visit(*ast) == 7, "Program adds one node");

    const auto& stmt = assignment(*ast);
    require(stmt.name == "z", "Assignment lost its identifier");
    const auto& mul = binary(*stmt.value, BinaryOp::Mul);
    const auto& add = binary(*mul.left, BinaryOp::Add);
    checkLocation(*ast, 1, 1);
    checkLocation(stmt, 1, 1);
    checkLocation(mul, 1, 5); // Primer token de (y + x) * 3, no el '*'.
    checkLocation(add, 1, 6);
    checkLocation(*add.left, 1, 6);
    checkLocation(*add.right, 1, 10);
    checkLocation(*mul.right, 1, 15);
}

static void testPrecedenceAndAssociativity()
{
    auto ast = parse("r = a + b * c; s = a * b + c; t = (a + b) * c;");
    const auto& first = binary(*assignment(*ast, 0).value, BinaryOp::Add);
    require(first.left->is<IdentExpr>(), "Left operand should be a leaf");
    binary(*first.right, BinaryOp::Mul);

    const auto& second = binary(*assignment(*ast, 1).value, BinaryOp::Add);
    binary(*second.left, BinaryOp::Mul);
    require(second.right->is<IdentExpr>(), "Right operand should be a leaf");

    const auto& third = binary(*assignment(*ast, 2).value, BinaryOp::Mul);
    binary(*third.left, BinaryOp::Add);

    auto chains = parse("r = a + b + c; s = a * b * c;");
    const BinaryOp ops[] = {BinaryOp::Add, BinaryOp::Mul};
    for (std::size_t i = 0; i < 2; ++i) {
        const auto& outer = binary(*assignment(*chains, i).value, ops[i]);
        const auto& inner = binary(*outer.left, ops[i]);
        require(inner.left->as<IdentExpr>().name == "a", "Wrong first operand");
        require(inner.right->as<IdentExpr>().name == "b", "Wrong second operand");
        require(outer.right->as<IdentExpr>().name == "c", "Operators must associate left");
    }
}

static void testParenthesesAndLists()
{
    auto plain = parse("x = 1 + 2;");
    auto grouped = parse("x = (((1 + 2)));");
    require(dump(*plain) == dump(*grouped), "Parentheses must not add AST nodes");

    auto list = parse("x = 10; y = x + 1; z = y * y;");
    require(list->items.size() == 3, "Program must retain all assignments");
    require(assignment(*list, 0).name == "x" && assignment(*list, 1).name == "y"
            && assignment(*list, 2).name == "z", "Program changed statement order");
    NodeCounter counter;
    require(counter.visit(*list) == 11, "Wrong count for multiple assignments");

    // Un mismo printer debe recuperar su sangria despues de cada recorrido.
    std::ostringstream output;
    AstPrinter printer(output);
    printer.visit(*plain);
    printer.visit(*grouped);
    require(output.str() == dump(*plain) + dump(*grouped), "Printer retained indentation");
}

static void testLocationsAndLiterals()
{
    auto ast = parse("// comentario\r\n  x =\r\n    y + 12; // fin\r\n z = 0007;");
    checkLocation(*ast, 2, 3);
    checkLocation(assignment(*ast), 2, 3);
    const auto& add = binary(*assignment(*ast).value, BinaryOp::Add);
    checkLocation(add, 3, 5);
    checkLocation(*add.left, 3, 5);
    checkLocation(*add.right, 3, 9);
    require(add.right->as<IntLiteral>().value == 12, "Number must store an int");
    checkLocation(assignment(*ast, 1), 4, 2);
    require(assignment(*ast, 1).value->as<IntLiteral>().value == 7, "Decimal leading zeros");

    const int max = std::numeric_limits<int>::max();
    auto boundary = parse("x = " + std::to_string(max) + ";");
    require(assignment(*boundary).value->as<IntLiteral>().value == max, "INT_MAX must parse");
}

static void expectError(const std::string& source, int line, int column,
                        const std::string& message)
{
    ProgramPtr ast;
    try {
        ast = parse(source);
    } catch (const ParseError& error) {
        require(!ast, "An error must not return a partial AST");
        require(error.line == line && error.column == column, "Wrong error location");
        require(std::string(error.what()).find(message) != std::string::npos,
                "Wrong diagnostic: " + std::string(error.what()));
        return;
    }
    throw std::runtime_error("Invalid source was accepted: " + source);
}

static void testErrors()
{
    expectError("", 1, 1, "Expected identifier");
    expectError("x = 1", 1, 6, "Expected ';'");
    expectError("x = (1 + );", 1, 10, "Expected identifier, number or '('");
    expectError("x = 1 2;", 1, 7, "Expected ';'");
    expectError("x = 1; @", 1, 8, "Expected end of input");
    expectError("x = 1;\ny = ;", 2, 5, "Expected identifier, number or '('");
    expectError("x = 1;\ny = " + std::to_string(std::numeric_limits<int>::max()) + "0;",
                2, 5, "Integer literal out of range");
}

int main()
{
    try {
        testClassExample();
        testPrecedenceAndAssociativity();
        testParenthesesAndLists();
        testLocationsAndLiterals();
        testErrors();
        std::cout << "AST tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "AST test failed: " << error.what() << '\n';
        return 1;
    }
}
