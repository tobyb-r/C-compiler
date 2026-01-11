#include "fail.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "symbols.h"
#include "types.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc > 1) {
    file = argv[1];
    stream = fopen(argv[1], "r");

    if (stream == NULL) {
      printf("Couldn't open file\n");
      exit(2);
    }
  } else {
    file = "STDIN";
    stream = stdin;
  }

  // parse();

  // debug_symbols();

  // printf("\nToken x is: \n  ");
  // debug_symbol(lookup_symbol("x"));
  // printf("Token y is: \n  ");
  // debug_symbol(lookup_symbol("y"));

  struct Type int_type = {.kind=T_INT};
  struct Var x_var = {.name="x", .type=&int_type};
  struct Var y_var = {.name="y", .type=&int_type};

  struct Symbol *x = add_symbol("x");
  *x = (struct Symbol){.kind=S_VAR, .var=&x_var};

  struct Symbol *y = add_symbol("y");
  *y = (struct Symbol){.kind=S_VAR, .var=&y_var};

  setup_lexer();

  struct Expr *expr = match_expr();

  debug_expr(expr);
}
