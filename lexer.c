#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fail.h"
#include "lexer.h"

int line = 1;
int line_col = 0;
char *file;

void fail(int _line, char *_file) {
  printf("On line %d column %d in file %s\n", line, line_col, file);
  printf("Error caught in compiler src line %d, file %s\n", _line, _file);
  exit(1);
}

char *token_repr[256] = {
    [L_PAREN] = "'('",
    [R_PAREN] = "')'",
    [L_BRACE] = "'{'",
    [R_BRACE] = "'}'",
    [L_SQUARE] = "'['",
    [R_SQUARE] = "']'",
    [ASSIGN] = "'='",
    [COMMA] = "','",
    [DOT] = "'.'",
    [SEMICOLON] = "';'",
    [COLON] = "':'",
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
    [VOID_TYPE] = "void_type",
    [FLOAT_TYPE] = "float_type",
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
    line_col = 0;
  }

  line_col++;

  // character really should be ascii
  if (next_char < -1) {
    printf("Syntax error: found non-ascii character %c", cur_char);
    FAIL;
  }

  cur_char = next_char;
  next_char = fgetc(stream);
  return cur_char;
}

void eat_char(char c) {
  if (cur_char != c) {
    printf("Syntax error: expected char '%c', found '%c'\n", c, cur_char);
    FAIL;
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
    FAIL;
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
    [':'] = COLON,   ['&'] = AMP,     ['-'] = MINUS,
};

// mappings from keywords to tokens
struct {
  enum TokenKind token;
  char *keyword;
} keywords[] = {
    {ELSE, "else"},        {FOR, "for"},        {WHILE, "while"},
    {STRUCT, "struct"},    {ENUM, "enum"},      {UNION, "union"},
    {TYPEDEF, "typedef"},  {RETURN, "return"},  {INT_TYPE, "int"},
    {FLOAT_TYPE, "float"}, {VOID_TYPE, "void"}, {CHAR_TYPE, "char"},
};

// get next token
struct Token get_token() {
  read_char();

  while (isspace(cur_char)) {
    read_char();
  }

  if (char_map[(int)cur_char] != 0) {
    return new_tok(char_map[(int)cur_char]);
  }

  // check what lexeme is and emit token
  if (isdigit(cur_char)) {
    read_lexeme();

    // TODO: float

    for (int i = 1; lexeme[i] != '\0'; i++) {
      if (!isdigit(lexeme[i])) {
        printf("Syntax error: unexpected character '%c' in int literal %s\n",
               lexeme[i], lexeme);
        FAIL;
      }
    }

    struct Token token = new_tok(INTEGER);
    token.int_literal = atoi(lexeme);

    return token;
  } else if (isalpha(cur_char) || cur_char == '_') {
    read_lexeme();

    // keyword or identifier

    for (int i = 0; i < (int)(sizeof(keywords) / sizeof(keywords[0])); i++) {
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
    // preprocessing should only happen on newlines starting with #

    while (cur_char != '\n') {
      read_char();
    }

    return get_token();

    // will work smth like this
    eat_char('#');
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
      // \n \t \v \b \r \f \a \\ \? \' \" \xhh
      eat_char('\\');
    }

    struct Token token = new_tok(CHAR);
    token.str_literal[0] = read_char();

    return token;
  } else if (cur_char == '\"') {
    struct Token token = new_tok(STRING);
    int len = 0;

    while (read_char() != '\"') {
      // TODO: handle escaped characters properly

      token.str_literal[len++] = cur_char;
    }

    return token;
  } else if (cur_char == '=') {
    if (next_char == '=') {
      eat_char('=');
      return new_tok(EQ);
    } else {
      return new_tok(ASSIGN);
    }
  } else if (cur_char == '<') {
    if (next_char == '=') {
      eat_char('<');
      return new_tok(LTE);
    } else {
      return new_tok(LT);
    }
  } else if (cur_char == '>') {
    if (next_char == '=') {
      eat_char('>');
      return new_tok(GTE);
    } else {
      return new_tok(GT);
    }
  } else if (cur_char == '!') {
    if (next_char == '=') {
      eat_char('!');
      return new_tok(NE);
    } else {
      return new_tok(NOT);
    }
  } else if (cur_char == EOF) {
    return new_tok(END);
  }

  printf("Couldn't match char '%c'\n", cur_char);
  FAIL;
  return new_tok(ERR);
}

struct Token read_token() { return cur_token = get_token(); }

void eat_token(enum TokenKind kind) {
  if (cur_token.kind != kind) {
    printf("Tried to match token '%s', found '%s'\n", token_repr[kind],
           token_repr[cur_token.kind]);
    FAIL;
  }

  read_token();
}
