#include "lexer.h"
#include "parser.h"
#include "symbols.h"
#include "fail.h"

int main(int argc, char **argv) {
  if (argc > 1) {
    file = argv[1];
    stream = fopen(argv[1], "r");
  } else {
    file = "STDIN";
    stream = stdin;
  }

  setup_lexer();

  parse();

  debug_symbols();
}

