#include "fail.h"
#include "lexer.h"
#include "parser.h"
#include "symbols.h"

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

  parse();

  debug_symbols();

  printf("\nToken x is: \n  ");
  debug_symbol(lookup_symbol("x"));
  printf("Token y is: \n  ");
  debug_symbol(lookup_symbol("y"));
}
