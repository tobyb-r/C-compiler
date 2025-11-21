#include "parser.c"

int main(int argc, char** argv) {
  if (argc > 1) {
    stream = fopen(argv[1], "r");
  } else {
    stream = stdin;
  }

  setup();

  enum Token token;

  while ((token = read_token()) != END) {
    printf("%s\n", token_repr[token]);
    if (token == IDENT) {
      printf("  %s\n", identifier);
    } else if (token == INTEGER) {
      printf("  %d\n", int_literal);
    } else if (token == STRING) {
      printf("  %s\n", str_literal);
    } else if (token == CHAR) {
      printf("  %s\n", str_literal);
    }
  }
}
