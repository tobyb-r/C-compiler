#ifndef LEXER_HEADER
#define LEXER_HEADER

#include <stdio.h>

enum TokenKind {
  // brackets
  L_PAREN = '(',
  R_PAREN = ')',
  L_BRACE = '{',
  R_BRACE = '}',
  L_SQUARE = '[',
  R_SQUARE = ']',

  // separators
  // TODO: ->
  ASSIGN = '=', // single =
  COMMA = ',',
  DOT = '.',
  SEMICOLON = ';',
  COLON = ':',

  // identifier
  IDENT = 128, // 128 to prevent collisions with chars

  // literals
  // TODO: float
  INTEGER,
  STRING,
  CHAR,

  // keywords
  // TODO: switch/case/default do/while continue sizeof goto
  // TODO: auto static const volatile signed unsigned extern register
  IF,
  ELSE,
  FOR,
  WHILE,
  RETURN,
  BREAK,
  STRUCT,
  UNION,
  ENUM,
  TYPEDEF,

  // type keywords
  // TODO: short/long, float/double
  INT_TYPE,
  CHAR_TYPE,

  // operators
  // TODO: & | ^ << >>
  // TODO: ++ --
  // TODO: assignment operators
  AMP,  // & can be addressof or bitwise and
  STAR, // * can be multiply or pointer type or dereference
  SLASH,
  PLUS,
  MINUS,
  MOD,
  EQ,  // ==
  NE,  // !=
  LT,  // <
  GT,  // >
  LTE, // <=
  GTE, // >=
  NOT,
  OR,
  AND,

  // handling
  ERR,
  START,
  END,
};

extern char *token_repr[256];

extern FILE *stream; // current stream

extern struct Token {
  enum TokenKind kind;
  union {
    char identifier[256];  // string name of identifier
    char str_literal[256]; // value of string literal
    int int_literal;       // value of numeric literal
  };
} cur_token;

void setup_lexer();

struct Token read_token();

void eat_token(enum TokenKind kind);

#endif
