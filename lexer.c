#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "fail.h"

int line;
char *file;

void fail(int no) {
  printf("On line %d in file %s\n", line, file);
  exit(no);
}

char *token_repr[256] = {
    [L_PAREN] = "left_paren",
    [R_PAREN] = "right_paren",
    [L_BRACE] = "left_brace",
    [R_BRACE] = "right_brace",
    [L_SQUARE] = "left_square",
    [R_SQUARE] = "right_square",
    [ASSIGN] = "assignment",
    [COMMA] = "comma",
    [DOT] = "dot",
    [SEMICOLON] = "semicolon",
    [COLON] = "colon",
    [IDENT] = "identifier",
    [INTEGER] = "int_literal",
    [STRING] = "string_literal",
    [CHAR] = "char_literal",
    [IF] = "if",
    [ELSE] = "else",
    [FOR] = "for",
    [WHILE] = "while",
    [RETURN] = "return",
    [BREAK] = "break",
    [STRUCT] = "struct",
    [UNION] = "union",
    [ENUM] = "enum",
    [TYPEDEF] = "typedef",
    [INT_TYPE] = "int_type",
    [CHAR_TYPE] = "char_type",
    [AMP] = "ampersand",
    [STAR] = "star",
    [SLASH] = "slash",
    [PLUS] = "plus",
    [MINUS] = "minus",
    [MOD] = "modulus",
    [EQ] = "equals",
    [NE] = "not_equals",
    [LT] = "less_than",
    [GT] = "greater_than",
    [LTE] = "less_than_equals",
    [GTE] = "greater_than_equals",
    [NOT] = "not",
    [OR] = "or",
    [AND] = "and",
    [ERR] = "error_token",
    [START] = "start",
    [END] = "end",
};

FILE *stream; // current stream

char cur_char;
char next_char;

// setup global so that read_char can be called
void setup_lexer() { next_char = fgetc(stream); }

char read_char() {
  if (cur_char == '\n') {
    line++;
  }

  // character really should be ascii
  if (next_char < -1) {
    printf("Syntax error: found non-ascii character %c", cur_char);
    fail(1);
  }

  cur_char = next_char;
  next_char = fgetc(stream);
  return cur_char;
}

void consume(char c) {
  if (cur_char != c) {
    printf("Syntax error: expected char '%c', found '%c'\n", c, cur_char);
    fail(2);
  }

  read_char();
}

char lexeme[256];

// read consecutive alpha-numeric
void read_lexeme() {
  // special character
  if (!isalnum(cur_char) && cur_char != '_') {
    printf("Compiler error: Tried to read lexeme from character '%c'\n",
           cur_char);
    fail(3);
  }

  memset(lexeme, 0, 256);
  lexeme[0] = cur_char;
  int len = 1;

  while (isalnum(next_char) || next_char == '_') {
    lexeme[len++] = read_char();
  }
}

struct Token cur_token;

struct Token new_tok(enum TokenKind kind) {
  return (struct Token){.kind = kind};
}

// characters that are easy to map
// unspecified characters are 0
enum TokenKind char_map[127] = {
    ['('] = L_PAREN, [')'] = R_PAREN, ['['] = L_SQUARE, [']'] = R_SQUARE,
    ['{'] = L_BRACE, ['}'] = R_BRACE, ['*'] = STAR,     ['+'] = PLUS,
    ['%'] = MOD,     [','] = COMMA,   ['.'] = DOT,      [';'] = SEMICOLON,
    [':'] = COLON,
};

// mappings from keywords to tokens
struct {
  enum TokenKind token;
  char *keyword;
} keywords[] = {
    {ELSE, "else"},       {FOR, "for"},       {WHILE, "while"},
    {STRUCT, "struct"},   {ENUM, "enum"},     {UNION, "union"},
    {TYPEDEF, "typedef"}, {RETURN, "return"}, {INT_TYPE, "int"},
    {CHAR_TYPE, "char"},
};

// get next token
struct Token get_token() {
  read_char();

  while (isspace(cur_char)) {
    read_char();
  }

  if (char_map[cur_char] != 0) {
    return new_tok(char_map[cur_char]);
  }

  // check what lexeme is and emit token
  if (isdigit(cur_char)) {
    read_lexeme();

    // TODO: float

    for (int i = 1; lexeme[i] != '\0'; i++) {
      if (!isdigit(lexeme[i])) {
        printf("Syntax error: unexpected character '%c' in int literal %s\n",
               lexeme[i], lexeme);
        fail(4);
      }
    }

    struct Token token = new_tok(INTEGER);
    token.int_literal = atoi(lexeme);

    return token;
  } else if (isalpha(cur_char) || cur_char == '_') {
    read_lexeme();

    // keyword or identifier

    for (int i = 0; i < (sizeof(keywords) / sizeof(keywords[0])); i++) {
      if (!strcmp(lexeme, keywords[i].keyword)) {
        return new_tok(keywords[i].token);
      }
    }

    // didn't match any keywords
    struct Token token = new_tok(IDENT);
    strcpy(token.identifier, lexeme);
    return token;
  } else if (cur_char == '/') {
    if (next_char == '/') {
      // comment - ignore until newline
      // TODO: multiline comments

      while (cur_char != '\n') {
        read_char();
      }

      return get_token();
    } else {
      return new_tok(SLASH);
    }
  } else if (cur_char == '#') {
    // TODO: handle preprocessing at this stage
    // just ignore macros

    while (cur_char != '\n') {
      read_char();
    }

    return get_token();

    // will work smth like this
    consume('#');
    read_lexeme();

    if (!strcmp(lexeme, "include")) {
      // switch input stream to new file
      // start reading from new file
    } else {
      // defines, ifdef etc.
    }
  } else if (cur_char == '\'') {
    if (read_char() == '\\') {
      // TODO: handle properly
      consume('\\');
    }

    struct Token token = new_tok(CHAR);
    token.str_literal[0] = read_char();

    return token;
  } else if (cur_char == '\"') {
    struct Token token = new_tok(STRING);
    int len = 0;

    while (read_char() != '\"') {
      // TODO: handle properly

      token.str_literal[len++] = cur_char;
    }

    return token;
  } else if (cur_char == '=') {
    if (next_char == '=') {
      consume('=');
      return new_tok(EQ);
    } else {
      return new_tok(ASSIGN);
    }
  } else if (cur_char == '<') {
    if (next_char == '=') {
      consume('<');
      return new_tok(LTE);
    } else {
      return new_tok(LT);
    }
  } else if (cur_char == '>') {
    if (next_char == '=') {
      consume('>');
      return new_tok(GTE);
    } else {
      return new_tok(GT);
    }
  } else if (cur_char == '!') {
    if (next_char == '=') {
      consume('!');
      return new_tok(NE);
    } else {
      return new_tok(NOT);
    }
  } else if (cur_char == EOF) {
    return new_tok(END);
  }

  printf("Couldn't match char '%c'\n", cur_char);
  fail(5);
  return new_tok(ERR);
}

struct Token read_token() {
  cur_token = get_token();
  return cur_token;
}

void eat_token(enum TokenKind kind) {
  if (cur_token.kind != kind) {
    printf("Tried to eat token %s, found %s\n", token_repr[kind],
           token_repr[cur_token.kind]);
    fail(6);
  }

  read_token();
}
