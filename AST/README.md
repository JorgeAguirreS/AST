# ParserSample1 con AST

El parser ahora devuelve un árbol de sintaxis abstracta (AST). Se basa en
`03-AST.notes.md` y conserva los nombres y el estilo de los ejemplos de clase:
`Node`, `Expr`, `Stmt`, `IntLiteral`, `IdentExpr`, `BinaryExpr`, `Assign`,
`Program`, `ExprPtr`, `Visitor<R>`, `AstPrinter` y `NodeCounter`.

## Compilar y ejecutar

Con CMake y un compilador de C++20:

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Si usas MinGW en Windows:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
.\build\ParserSample1.exe examples\valid\assign_expr.txt
ctest --test-dir build --output-on-failure
```

Elige un generador al crear `build`; para cambiarlo utiliza otra carpeta.
Con Visual Studio el ejecutable suele estar en `build\Debug`, y las pruebas
se ejecutan con `ctest --test-dir build -C Debug --output-on-failure`.
Puedes pasar varios archivos al ejecutable como antes.
En MinGW, CMake incorpora las bibliotecas de soporte al ejecutable para evitar
conflictos con DLL de otras instalaciones presentes en `PATH`.

Para `z = (y + x) * 3;`, la salida es:

```text
examples\valid\assign_expr.txt: OK
Program
  Assign z
    Binary *
      Binary +
        Ident y
        Ident x
      Int 3
Nodes (including Program): 7
```

La asignación tiene **6 nodos**, igual que en la diapositiva 28.
El programa completo tiene **7** porque incluye la raíz `Program`.

## Orden de lectura para estudiar

| Archivo | Relación con los apuntes |
|---|---|
| `Ast.hpp` | Diapositivas 10–16: jerarquía, `kind()`, `loc()`, campos `const` y propiedad de los hijos. |
| `Parser.cpp` | Diapositivas 18–21: cada regla pasa de `void` a devolver su nodo. |
| `Visitor.hpp` | Diapositivas 26–27: `Visitor<R>` y un único `switch` para despachar por tipo. |
| `AstPrinter.cpp` | Diapositivas 23 y 26: recorrer e imprimir el árbol con sangría. |
| `NodeCounter.hpp` | Diapositiva 28: devolver `1 +` el tamaño de los hijos. |
| `Main.cpp` | Obtener la raíz y aplicar dos operaciones al mismo árbol. |
| `tests/AstTests.cpp` | Ejemplos que verifican estructura, posiciones y errores. |

## 1. Los nodos son los datos

```text
Node                         kind(), loc()
├── Expr
│   ├── IntLiteral           value
│   ├── IdentExpr            name
│   └── BinaryExpr           op, left, right
├── Stmt
│   └── Assign               name, value
└── Program                  items
```

`Assign` es una sentencia: asignar produce un efecto. Sus expresiones hijas
representan valores. `Program::items` es un `std::vector<StmtPtr>` que conserva
el orden de las sentencias. El nombre del destino está en `Assign::name`;
no se añade otro `IdentExpr` para él.

Cada nodo guarda el tipo en `NodeKind` y la posición en `SourceLocation`.
`is<T>()` comprueba el tipo; `as<T>()` permite acceder a los campos del tipo
concreto. Debes comprobar el tipo antes de convertirlo. El visitor lo hace
mediante su `switch`; el `assert` de `as<T>()` ayuda a detectar errores en Debug.

Los constructores de las clases base están protegidos, como en el ejemplo
de `Node` de clase. Los nodos concretos son `final` y sus campos son públicos
y `const`.

## 2. El parser construye al regresar de cada regla

Se conserva la gramática original:

```text
program → assign+
assign  → IDENT "=" expr ";"
expr    → term ("+" term)*
term    → factor ("*" factor)*
factor  → IDENT | NUMBER | "(" expr ")"
```

| Método | Devuelve | Equivalente de clase |
|---|---|---|
| `parse()` / `program()` | `ProgramPtr` | Raíz con una lista de sentencias. |
| `assign()` | `StmtPtr` | `Assign` con nombre y valor. |
| `expr()` | `ExprPtr` | `additive()` de la diapositiva 18. |
| `term()` | `ExprPtr` | Multiplicaciones con mayor precedencia. |
| `factor()` | `ExprPtr` | `primary()` de la diapositiva 20. |

El patrón central es el mismo de la clase:

```cpp
ExprPtr lhs = term();
while (token.tokenId == TokenId::OpAdd) {
    nextToken();
    ExprPtr rhs = term();
    lhs = std::make_unique<BinaryExpr>(
        loc, BinaryOp::Add, std::move(lhs), std::move(rhs));
}
return lhs;
```

Cada vuelta convierte el árbol anterior en el hijo izquierdo. Por eso
`a + b + c` representa `(a + b) + c`. Como `expr()` llama a `term()`,
`a + b * c` tiene una suma en la raíz y una multiplicación a la derecha.

`factor()` devuelve directamente la expresión interior de unos paréntesis.
Así, `x = (1 + 2);` y `x = 1 + 2;` tienen la misma estructura, aunque cambian
las columnas de sus nodos. Los signos `=`, `;` y los paréntesis se consumen
sin crear nodos. `expr` y `term` tampoco son nodos.

La ubicación se toma del primer token de la construcción, como en los
apuntes. En `(y + x) * 3`, el `BinaryExpr` de la multiplicación comienza en
el `(` y el de la suma en `y`. No se usa la posición del operador.

## 3. Propiedad de la memoria

`ExprPtr` es un alias de `std::unique_ptr<Expr>` y `StmtPtr` de
`std::unique_ptr<Stmt>`. Cada padre posee a sus hijos.

- `std::make_unique` crea un nodo.
- `std::move` transfiere la propiedad del hijo al padre; no copia el árbol.
- Al destruir la raíz se destruyen sus descendientes. El destructor virtual
  permite destruir el nodo concreto mediante un puntero a la clase base.
- Si ocurre un `ParseError`, los punteros locales liberan lo construido y
  `parse()` no devuelve un árbol parcial.

Los enteros se guardan como `int`, igual que en las diapositivas. Si un literal
excede su rango, se informa un `ParseError` con la posición del número.

## 4. Operaciones con Visitor

Se utiliza la opción que los apuntes eligen en la diapositiva 29: operaciones
fuera de los nodos. La variante con `dump()` virtual dentro de cada nodo
(diapositivas 24–25) sirve como comparación; aquí imprimir vive en `AstPrinter`.

`Visitor<R>::visit(const Node&)` mira `kind()` y llama al `visitX` adecuado.
Esos métodos son virtuales puros (`= 0`), de modo que cada operación debe
implementar todos los tipos de nodo.

- `AstPrinter : Visitor<void>` imprime y guarda la sangría como estado.
- `NodeCounter : Visitor<int>` devuelve el tamaño de cada subárbol.

Por ejemplo:

```cpp
ProgramPtr ast = parser.parse();
AstPrinter printer(std::cout);
printer.visit(*ast);

NodeCounter counter;
int total = counter.visit(*ast);
int primeraSentencia = counter.visit(*ast->items.front());
```

Agregar otra operación requiere otra clase derivada de `Visitor<R>`.
Agregar un tipo de nodo requiere ampliar `NodeKind`, el AST, el parser,
el despacho del visitor y cada operación, como explica la clase.

## Alcance respecto a MiniLang

Los apuntes muestran un lenguaje mayor que este proyecto. Aquí se implementa
el AST de las construcciones que el parser ya reconoce. `if`, `for`, bloques,
booleanos, declaraciones y funciones requieren ampliar también el léxico y
la gramática; no son sintaxis admitida por este parser.

En MiniLang, las partes opcionales se representan con hijos `nullptr` y las
listas con vectores. Este subconjunto tiene listas en `Program`, pero todos
los hijos de `Assign` y `BinaryExpr` son obligatorios.

El análisis semántico y el intérprete aparecen en los apuntes como las
siguientes sesiones. El AST queda listo para agregar esas operaciones mediante
visitors. Actualmente el programa construye, imprime y cuenta el árbol;
no evalúa expresiones ni comprueba si las variables están declaradas.

Para practicar, dibuja el árbol de `examples/valid/left_associative.txt`,
cuenta sus nodos y compara con la salida del programa.
