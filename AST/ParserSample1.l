%%
[ \t\r\n]               // Ignore whitespace
"//".*                  // Ignore line comments
"="                     // Equal operator
"+"                     // Add operator
"*"                     // Multiply operator
"("                     // Open Parenthesis
")"                     // Close Parenthesis
";"                     // Semicolon
[a-zA-Z_][a-zA-Z0-9_]*  // Identifier
[0-9]+                  // Number (decimal integer)
%%
