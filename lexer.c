#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int doubler(int x) { return x * 2; }

enum Token {
  IDENT, // identifier

  // literals
  // TODO: float
  INTEGER,
  STRING,
  CHAR,

  // keywords
  // TODO: continue default do sizeof switch case
  // TODO: auto static const volatile signed static unsigned extern register 
  IF,
  ELSE,
  FOR,
  WHILE,
  GOTO,
  RETURN,
  STRUCT,
  ENUM,
  UNION,

  // type keywords
  // INT
  // CHAR
  // SHORT
  // LONG
  // FLOAT
  // DOUBLE

  // brackets
  L_PAREN, // ()
  R_PAREN,
  L_BRACE, // {}
  R_BRACE,
  L_SQUARE, // []
  R_SQUARE,

  // operators
  // TODO: & | && || << >>
  // TODO: ++ --
  // TODO: assignment operators
  STAR, // * can be multiply or pointer type or dereference
  PLUS,
  SLASH,
  MOD,
  EQUALS, // double ==
  LT,     // <
  GT,     // >
  LTE,    // <=
  GTE,    // >=
  NOT,
  NOTEQUALS,
  ASSIGN, // single =

  // separators
  // TODO: ->
  COMMA,
  DOT,
  SEMICOLON,
  COLON,

  // handling
  ERR,
  START,
  END,
};

char identifier[256];  // string name of identifier
char str_literal[256]; // value of string literal
int int_literal;       // value of numeric literal

char *token_repr[] = {"identifier",
                      "int-literal",
                      "string-literal",
                      "char-literal",
                      "if",
                      "else",
                      "for",
                      "while",
                      "goto",
                      "return",
                      "struct",
                      "enum",
                      "union",
                      "left-parens",
                      "right-parens",
                      "left-brace",
                      "right-brace",
                      "left-square-bracket",
                      "right-square-bracket",
                      "star",
                      "plus",
                      "slash",
                      "modulus",
                      "equals",
                      "less-than",
                      "greater-than",
                      "less-than-equals",
                      "greater-than-equals",
                      "not",
                      "not-equals",
                      "assignment",
                      "comma",
                      "dot",
                      "semicolon",
                      "colon",
                      "error-token",
                      "start",
                      "end"};

FILE *stream; // current stream
char *file;   // name of file
int line;     // line in file

void lex_fail(int no) {
  printf("Lexing failure on line %d in file %s\n", line, file);
  exit(no);
}

char cur_char;
char next_char = -1;

// setup global so that read_char can be called
void setup() { next_char = fgetc(stream); }

char read_char() {
  if (cur_char == '\n') {
    line++;
  }

  // character really should be ascii
  if (next_char < -1) {
    printf("Found non-ascii character %c", cur_char);
    lex_fail(1);
  }

  cur_char = next_char;
  next_char = fgetc(stream);
  return cur_char;
}

void consume(char c) {
  if (cur_char != c) {
    printf("Tried to consume char '%c', found '%c'\n", c, cur_char);
    lex_fail(5);
  }

  read_char();
}

char lexeme[256];

// read consecutive alpha-numeric
void read_lexeme() {
  // special character
  if (!isalnum(cur_char) && cur_char != '_') {
    printf("Tried to read lexeme from non alphanumeric character '%c'\n",
           cur_char);
    lex_fail(6);
  }

  memset(lexeme, 0, 256);
  lexeme[0] = cur_char;
  int len = 1;

  while (isalnum(next_char) || next_char == '_') {
    lexeme[len++] = read_char();
  }
}

// characters that are easy to map
// unspecified characters are 0
enum Token char_map[127] = {
    ['('] = L_PAREN, [')'] = R_PAREN, ['['] = L_SQUARE, [']'] = R_SQUARE,
    ['{'] = L_BRACE, ['}'] = R_BRACE, ['*'] = STAR,     ['+'] = PLUS,
    ['%'] = MOD,     [','] = COMMA,   ['.'] = DOT,      [';'] = SEMICOLON,
    [':'] = COLON,
};

// get next token
enum Token read_token() {
  read_char();

  while (isspace(cur_char)) {
    read_char();
  }

  if (char_map[cur_char] != 0) {
    return char_map[cur_char];
  }

  // check what lexeme is and emit token
  if (isdigit(cur_char)) {
    read_lexeme();

    // TODO: hex & octal
    // TODO: float

    for (int i = 1; lexeme[i] != '\0'; i++) {
      if (!isdigit(lexeme[i])) {
        printf("Unexpected character '%c' in int literal %s\n", lexeme[i],
               lexeme);
        lex_fail(3);
      }
    }

    int_literal = atoi(lexeme);

    return INTEGER;
  } else if (isalpha(cur_char) || cur_char == '_') {
    read_lexeme();

    // identifier or keyword
    if (!strcmp(lexeme, "if")) {
      return IF;
    } else if (!strcmp(lexeme, "else")) {
      return ELSE;
    } else if (!strcmp(lexeme, "for")) {
      return FOR;
    } else if (!strcmp(lexeme, "while")) {
      return WHILE;
    } else if (!strcmp(lexeme, "goto")) {
      return GOTO;
    } else if (!strcmp(lexeme, "struct")) {
      return STRUCT;
    } else if (!strcmp(lexeme, "enum")) {
      return ENUM;
    } else if (!strcmp(lexeme, "union")) {
      return UNION;
    } else if (!strcmp(lexeme, "return")) {
      return RETURN;
    } else {
      strcpy(identifier, lexeme);
      return IDENT;
    }
  } else if (cur_char == '/') {
    if (next_char == '/') {
      // comment - ignore until newline

      while (cur_char != '\n') {
        read_char();
      }

      return read_token();
    } else {
      return SLASH;
    }
  } else if (cur_char == '#') {
    // TODO: handle preprocessing at this stage
    
    while (cur_char != '\n') {
      read_char();
    }

    return read_token();

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
      // TODO - handle properly
      consume('\\');
    }

    memset(str_literal, 0, 256);
    str_literal[0] = cur_char;

    read_char();

    return CHAR;
  } else if (cur_char == '\"') {
    int len = 0;

    while (read_char() != '\"') {
      str_literal[len++] = cur_char;
    }

    return STRING;
  } else if (cur_char == '=') {
    if (next_char == '=') {
      consume('=');
      return EQUALS;
    } else {
      return ASSIGN;
    }
  } else if (cur_char == '<') {
    if (next_char == '=') {
      consume('<');
      return LTE;
    } else {
      return LT;
    }
  } else if (cur_char == '>') {
    if (next_char == '=') {
      consume('>');
      return GTE;
    } else {
      return GT;
    }
  } else if (cur_char == '!') {
    if (next_char == '=') {
      consume('!');
      return NOTEQUALS;
    } else {
      return NOT;
    }
  } else if (cur_char == EOF) {
    return END;
  }

  printf("Couldn't match char '%c'\n", cur_char);
  lex_fail(2);
  return ERR; // unreachable
}
