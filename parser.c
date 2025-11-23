#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "lexer.h"
#include "symbols.h"

// TODO:
// statements/expressions
// have to make static variables and initializers work
// use scoping
// - keep track of static variables
// - initializers are expressions we eval at codegen

// TODO:
// type qualifiers
// declaration lists
// function pointers

// parse token stream into abstract syntax tree
// we parse functions into symbols and asts
// we parse globals and type definitions into symbols

// declarator type for parsing
struct Dec {
  struct Type type;
  char *identifier;
};

struct Dec match_dec();

// match fields of struct or union
struct Field *match_fields() {
  eat_token('{');

  struct Field *fields = NULL;
  struct Field **tail = &fields;

  while (cur_token.kind != '}') {
    struct Dec dec = match_dec();

    *tail = malloc(sizeof(struct Field));

    (*tail)->type = dec.type;
    (*tail)->name = dec.identifier;
    (*tail)->next = NULL;

    tail = &(*tail)->next;

    if (cur_token.kind == ';') {
      eat_token(';');
    }
  }

  eat_token('}');

  return fields;
}

// match parameters of function
struct Param *match_params() {
  eat_token('(');

  struct Param *params = NULL;
  struct Param **tail = &params;

  while (cur_token.kind != ')') {
    struct Dec dec = match_dec();

    *tail = malloc(sizeof(struct Param));

    (*tail)->type = dec.type;
    (*tail)->name = dec.identifier;
    (*tail)->next = NULL;

    tail = &(*tail)->next;

    if (cur_token.kind == ',') {
      eat_token(',');
    }
  }

  eat_token(')');

  return params;
}

struct Type match_type() {
  // type ::=
  //   | struct/union definition
  //     - FIRST = `struct` or `union`
  //   | enum definition
  //     - FIRST = `enum`
  //   | typedef-name
  //     - FIRST = identifier
  //   | int/char/double
  //     - FIRST = `int` `char`

  if (cur_token.kind == STRUCT) {
    // add new definition to scope
    // struct-definition ::= `struct` name | `struct` name {} | `struct` {}
    struct Token token = read_token();

    if (token.kind == IDENT) {
      struct Token token2 = read_token();
      if (token2.kind == '{') {
        // new struct definition
        // parse struct definition and add to symbols
        //
      } else if (token2.kind == IDENT) {
        // old struct definition
        // lookup and return struct
      }
    } else if (token.kind == '{') {
      // anonymous struct definition
      // parse struct definition
    }
  } else if (cur_token.kind == UNION) {
    // add new definition to scope
    // same as struct
  } else if (cur_token.kind == ENUM) {
    // add new definition to scope
    // match_struct
  } else if (cur_token.kind == INT_TYPE) {
    read_token();
    return (struct Type){.kind = T_INT};
  } else if (cur_token.kind == CHAR_TYPE) {
    read_token();
    return (struct Type){.kind = T_CHAR};
  } else if (cur_token.kind == IDENT) {
    // lookup typedef
  }

  return (struct Type){0};
}

// TODO: type names can have more complex patterns e.g. pointers, arrays
//  - this impacts the type
//  - type name parses the same as an lvalue
struct Dec match_dec() {
  struct Dec dec;
  dec.type = match_type();

  dec.identifier = malloc(strlen(cur_token.identifier));

  strcpy(dec.identifier, cur_token.identifier);

  eat_token(IDENT);

  return dec;
}

struct Var *match_var_dec() {
  struct Dec dec = match_dec();

  struct Var *var = malloc(sizeof(struct Var));

  var->type = dec.type;

  struct Symbol *sym = malloc(sizeof(struct Symbol));
  sym->kind = S_VAR;
  sym->var = var;

  add_symbol(dec.identifier, sym);

  free(dec.identifier);

  return var;
}

struct Expr *match_expr();

struct Stmt *match_inner_dec() {
  // internal declarations ::=
  //   | variable definition
  //     - ::= type var-name | type var-name = expression
  //     - FIRST = type
  //   | typedef definition
  //     - FIRST = `typedef`
  //
  // can get type and then match rest (bottom up style)
  // struct/union/enum definition is a type

  if (cur_token.kind == TYPEDEF) {
    eat_token(TYPEDEF);
    // parse typedef and add to the thing
    return NULL;
  }

  // token type is variable definition
  struct Type type = match_type();

  // add variable to scope
  eat_token(IDENT);

  if (cur_token.kind == '=') {
    eat_token('=');
    struct Expr *expr = match_expr();
    // return variable assignment AST
    return NULL;
  }

  return NULL;
}

struct Expr *match_expr() {
  // assignment is an expression
  //
  // expr ::=
  //   | lvalue
  //   | constant
  //   | lvalue = expr
  //     - right assoc
  //   | expr `op` expr

  return NULL;
}

// probably need different functions and structs for int/string
// const initializers for structs/arrays?
void *match_constant() { return NULL; }

struct Stmt *match_stmt() {
  // stmt ::=
  //   | for/while/if
  //     - FIRST = for/while/if
  //   | declaration
  //     - FIRST = type
  //   | expression;
  //     - FIRST = var name, function name, unary operator
  //   | assignment operator
  // assignment / function call is an expression

  return NULL;
}

struct BlockStmt *match_block_stmt() {
  // compound_stmt ::= { (stmt;)* }

  eat_token('{');

  while (cur_token.kind != '{') {
    read_token();
    struct Stmt *stmt = match_stmt();
    // add stmt to ast
  }

  eat_token('}');

  return NULL;
}

// parse outer declaration
void match_outer_dec() {
  // external declarations ::=
  //   | type fn_name(type1 param1) { }
  //     - FIRST = type
  //   | global variable definition
  //     - global-variabl-definition ::= var-dec; | var-dec = constant;
  //     - var-dec ::= type var-name
  //     - FIRST = type
  //   | typedef definition
  //     - FIRST = `typedef`
  //
  // can get type and then match rest (bottom up style)
  // struct/union/enum definition is a type

  if (cur_token.kind == TYPEDEF) {
    eat_token(TYPEDEF);

    struct Dec dec = match_dec();

    struct Symbol *sym = malloc(sizeof(struct Symbol));

    sym->kind = TYPEDEF;
    sym->type = dec.type;

    add_symbol(dec.identifier, sym);

    free(dec.identifier);
  }

  struct Dec dec = match_dec();

  // now:
  // - function definition
  //   | (params) { body }
  //   | (params)
  //   FIRST = `(`
  // - global variable definition
  //   | ;
  //   | = constant;
  //   FIRST = ';' | '='

  if (cur_token.kind == '(') {
    // read_token();

    struct Param *params = match_params();
    
    struct Func *func = malloc(sizeof(struct Func));

    func->name = dec.identifier;
    func->sig = malloc(sizeof(struct FuncSig));
    func->sig->ret = dec.type;
    func->sig->params = params;
    func->stmt = NULL;

    if (cur_token.kind == '{') {
      // function is defined here
      // new_scope();
      // add function parameters to scope
      // struct BlockStmt *ast = match_block_stmt();
      // exit_scope();

      // TODO for now skip definition
      int depth = 0;

      while(depth >= 0) {
        switch (read_token().kind) {
          case '{':
            depth++;
            continue;
          case '}':
            depth--;
            continue;
          default:
            continue;
        }
      }

      eat_token('}');
    } else {
      eat_token(';');
    }
    
    // TODO function can be declared multiple times and defined once

    // add function to scope
    struct Symbol *sym = malloc(sizeof(struct Symbol));

    sym->kind = S_FUNC;
    sym->func = func;
    
    add_symbol(dec.identifier, sym);
  } else if (cur_token.kind == ';') {
    struct Var *var = malloc(sizeof(struct Var));
    var->type = dec.type;
    
    struct Symbol *sym = malloc(sizeof(struct Symbol));

    sym->kind = S_VAR;
    sym->var = var;
    
    add_symbol(dec.identifier, sym);
    eat_token(';');
  } else if (cur_token.kind == '=') {
    // add variable to scope
    // store initializer somehow
    eat_token('=');

    // TODO for now skip initializer
    while(read_token().kind != ';') {}
    eat_token(';');

    struct Var *var = malloc(sizeof(struct Var));
    var->type = dec.type;
    
    struct Symbol *sym = malloc(sizeof(struct Symbol));

    sym->kind = S_VAR;
    sym->var = var;
    
    add_symbol(dec.identifier, sym);
  }

  free(dec.identifier);
}

// parse start_symbol
void parse() {
  read_token();

  while (cur_token.kind != END) {
    match_outer_dec();
  }
};
