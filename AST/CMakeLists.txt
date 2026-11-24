cmake_minimum_required(VERSION 3.20)
project(ParserSample1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ParserCore
    Lexer.cpp
    Lexer.hpp
    Parser.cpp
    Parser.hpp
    Ast.hpp
    Visitor.hpp
    AstPrinter.cpp
    AstPrinter.hpp
    NodeCounter.hpp
)
target_include_directories(ParserCore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# En Windows puede haber DLL de otra version de MinGW antes en PATH.
# Incorporar el runtime evita ese conflicto al ejecutar ambos programas.
if(MINGW)
  target_link_options(ParserCore INTERFACE -static)
endif()

add_executable(${PROJECT_NAME} Main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ParserCore)

if(MSVC)
  target_compile_options(ParserCore PRIVATE /W4)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
  target_compile_options(ParserCore PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Tests: every file in examples/valid must parse, every file in examples/invalid must fail.
enable_testing()

file(GLOB VALID_EXAMPLES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/examples/valid/*.txt)
foreach(example IN LISTS VALID_EXAMPLES)
  get_filename_component(name ${example} NAME_WE)
  add_test(NAME valid_${name} COMMAND ${PROJECT_NAME} ${example})
endforeach()

# Estas pruebas verifican la forma del AST, no solo si la entrada es valida.
add_executable(AstTests tests/AstTests.cpp)
target_link_libraries(AstTests PRIVATE ParserCore)
if(MSVC)
  target_compile_options(AstTests PRIVATE /W4)
else()
  target_compile_options(AstTests PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_test(NAME ast_structure COMMAND AstTests)

file(GLOB INVALID_EXAMPLES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/examples/invalid/*.txt)
foreach(example IN LISTS INVALID_EXAMPLES)
  get_filename_component(name ${example} NAME_WE)
  add_test(NAME invalid_${name} COMMAND ${PROJECT_NAME} ${example})
  set_tests_properties(invalid_${name} PROPERTIES WILL_FAIL TRUE)
endforeach()
