#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "fail.h"
#include "lexer.h"
#include "symbols.h"
#include "types.h"

// TODO:
// statements/expressions
// have to make global variables and initializers work
// - keep track of global variables
// - initializers are expressions we eval at codegen

// TODO:
// type qualifiers
// declaration lists

// TODO:
// make shadowing work correctly
// to do this i have to change the interface for adding symbols
// idc

// parse token stream into abstract syntax tree
// we parse functions into symbols and asts
// we parse globals and type definitions into symbols

// declarator type for parsing
struct Dec {
  struct Type *type;
  char *identifier;
};

struct Param *match_params();
struct Type match_type();

struct Type **match_dec_rec(struct Dec *dec, struct Type **type);

struct Dec match_declarator(struct Type type_) {
  struct Type *type = malloc(sizeof(*type));
  *type = type_;
  struct Dec dec = {0};
  dec.type = type;

  match_dec_rec(&dec, &dec.type);

  return dec;
}

struct Type **match_dec_rec(struct Dec *dec, struct Type **type) {
  switch (cur_token.kind) {
  case STAR:
    eat_token(STAR);

    type = match_dec_rec(dec, type);

    struct Type *ptr_type = malloc(sizeof(*ptr_type));
    ptr_type->kind = T_POINTER;
    ptr_type->ptr_type = *type;

    *type = ptr_type;

    return &ptr_type->ptr_type;

  case '(':
    eat_token('(');
    type = match_dec_rec(dec, type);
    eat_token(')');

    break;

  case IDENT:
    dec->identifier = malloc(strlen(cur_token.identifier) + 1);
    strcpy(dec->identifier, cur_token.identifier);
    eat_token(IDENT);

    break;

  case ')':
  case ';':
  case ',':
    // no identifier
    dec->identifier = NULL;

    return type;

  default:
    printf("Syntax error: Expected identifier, found %s\n",
           token_repr[cur_token.kind]);
    FAIL;
  }

  while (1) {
    switch (cur_token.kind) {
    case '[':
      eat_token('[');
      int len = -1;

      if (cur_token.kind == INTEGER) {
        len = cur_token.int_literal;
        eat_token(INTEGER);
      }

      eat_token(']');

      struct Type *arr_type = malloc(sizeof(*arr_type));

      arr_type->kind = T_ARRAY;
      arr_type->array.elem_type = *type;
      arr_type->array.len = len;

      *type = arr_type;
      type = &arr_type->array.elem_type;

      continue;
    case '(':;
      // turn type into function
      struct Param *params = match_params();

      struct Type *f_type = malloc(sizeof(*f_type));

      f_type->kind = T_FUNC;
      f_type->func_sig = malloc(sizeof(*f_type->func_sig));
      f_type->func_sig->params = params;
      f_type->func_sig->ret = *type;

      *type = f_type;
      type = &f_type->func_sig->ret;

      continue;
    default:
      break;
    }

    break;
  }

  return type;
}

// match fields of struct or union
struct Field *match_fields() {
  eat_token('{');

  struct Field *fields = NULL;
  struct Field **tail = &fields;

  while (cur_token.kind != '}') {
    struct Type type = match_type();
    struct Dec dec = match_declarator(type);

    if (dec.identifier == NULL) {
      // anonymous struct or union definition means that the struct has those
      // members
      if (dec.type->kind == T_STRUCT || dec.type->kind == T_UNION) {
        if (dec.type->struct_type->name == NULL) {
          // add as a field with NULL identifier
        } else {
          printf("Semantic error: Struct field with no identifier\n");
          FAIL;
        }
      } else {
        printf("Semantic error: Struct field with no identifier\n");
        FAIL;
      }
    }

    *tail = malloc(sizeof(**tail));

    (*tail)->type = dec.type;
    (*tail)->name = dec.identifier;
    (*tail)->next = NULL;

    tail = &(*tail)->next;

    eat_token(';');
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
    struct Type type = match_type();
    struct Dec dec = match_declarator(type);

    *tail = malloc(sizeof(**tail));

    (*tail)->type = dec.type;
    (*tail)->name = dec.identifier;
    (*tail)->next = NULL;

    tail = &(*tail)->next;

    if (cur_token.kind == ',') {
      eat_token(',');
    } else if (cur_token.kind != ')') {
      printf("Syntax error: Expected ',' or ')' in parameter list\n");
      FAIL;
    }
  }

  eat_token(')');

  return params;
}

struct Type match_struct() {
  // add new definition to scope
  // struct-definition ::= `struct` name | `struct` name {} | `struct` {}
  eat_token(STRUCT);

  struct Type type = {0};
  type.kind = T_STRUCT;

  if (cur_token.kind == IDENT) {
    char *name = malloc(strlen(cur_token.identifier) + 1);
    strcpy(name, cur_token.identifier);

    eat_token(IDENT);

    if (cur_token.kind == '{') {
      type.struct_type = add_struct(name);

      if (type.struct_type->complete) {
        printf("Semantic error: redefining struct\n");
      }

      type.struct_type->fields = match_fields();
      type.struct_type->name = name;
    } else {
      type.struct_type = lookup_struct(name);

      if (!type.struct_type) {
        type.struct_type = add_struct(name);
        type.struct_type->name = name;
      }
    }
  } else if (cur_token.kind == '{') {
    // anonymous struct
    struct Field *fields = match_fields();
    struct Struct *struc = malloc(sizeof(*struc));

    struc->name = NULL;
    struc->fields = fields;
    type.struct_type = struc;
  } else {
    printf("Syntax error: Expected identifier or definition after 'struct', "
           "found %s\n",
           token_repr[cur_token.kind]);
    FAIL;
  }

  return type;
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
    return match_struct();
  } else if (cur_token.kind == UNION) {
    // TODO
    FAIL;
  } else if (cur_token.kind == ENUM) {
    // TODO
    FAIL;
  } else if (cur_token.kind == INT_TYPE) {
    eat_token(INT_TYPE);
    return (struct Type){.kind = T_INT};
  } else if (cur_token.kind == CHAR_TYPE) {
    eat_token(CHAR_TYPE);
    return (struct Type){.kind = T_CHAR};
  } else if (cur_token.kind == VOID_TYPE) {
    eat_token(VOID_TYPE);
    return (struct Type){.kind = T_VOID};
  } else if (cur_token.kind == FLOAT_TYPE) {
    eat_token(FLOAT_TYPE);
    return (struct Type){.kind = T_FLOAT};
  } else if (cur_token.kind == IDENT) {
    struct Symbol *sym = lookup_symbol(cur_token.identifier);

    if (sym && sym->kind == S_TYPEDEF) {
      eat_token(IDENT);
      return *sym->type;
    }

    printf("Expected type, found %s\n", cur_token.identifier);
    FAIL;
  }

  printf("Couldn't match type %s", token_repr[cur_token.kind]);
  FAIL;
  return (struct Type){0};
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

  if (cur_token.kind == ';') {
    return;
  }

  if (cur_token.kind == TYPEDEF) {
    eat_token(TYPEDEF);

    struct Type type = match_type();

    if (cur_token.kind == ';') {
      printf("Syntax error: No identifier after typedef");
      FAIL;
    }

    struct Dec dec = match_declarator(type);

    if (dec.identifier == NULL) {
      printf("Syntax error: No identifier after typedef");
      FAIL;
    }

    struct Symbol sym;

    sym.kind = S_TYPEDEF;
    sym.type = dec.type;

    struct Symbol *prev = lookup_symbol(dec.identifier);

    if (prev) {
      if (prev->kind != S_TYPEDEF) {
        printf("Semantic error: redefining symbol %s as a type\n",
               dec.identifier);
        FAIL;
      } else if (!type_eq(dec.type, prev->type)) {
        printf("Semantic error: redefining type %s as another type\n",
               dec.identifier);
        FAIL;
      }

      // don't need this
      free_type(sym.type);
    } else {
      *add_symbol(dec.identifier) = sym;
    }

    free(dec.identifier);

    eat_token(';');

    return;
  }

  struct Type type = match_type();

  if (cur_token.kind == ';') {
    eat_token(';');
    return;
  }

  struct Dec dec = match_declarator(type);
  type_verify(dec.type);

  if (dec.identifier == NULL) {
    printf("Syntax error: Outer decleration with no identifier\n");
    FAIL;
  }

  if (dec.type->kind == T_FUNC) {
    struct Func func;

    func.name = dec.identifier;
    func.sig = dec.type->func_sig;
    func.stmt = NULL;

    if (cur_token.kind == '{') {
      func.complete = 1;
      // function is defined here
      // new_scope();
      // add function parameters to scope
      // struct BlockStmt *ast = match_block_stmt();
      // exit_scope();

      // TODO for now skip definition
      int depth = 0;

      while (depth >= 0) {
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

    struct Func *def = add_func(dec.identifier);

    if (def->complete && func.complete) {
      printf("Redefining function %s\n", dec.identifier);
      FAIL;
    }

    if (def->sig) {
      if (!type_eq(def->sig->ret, func.sig->ret)) {
        goto ne;
      }

      struct Param *param1 = def->sig->params;
      struct Param *param2 = func.sig->params;

      while (param1 != NULL && param2 != NULL) {
        if (!type_eq(param1->type, param2->type)) {
          goto ne;
        }

        param1 = param1->next;
        param2 = param2->next;
      }

      // check that both are null
      if (param1 != param2) {
      ne:
        printf(
            "Semantic error: redefining function with different signature\n");
        FAIL;
      }

      // both are equal
      free_func_sig(func.sig);
    } else {
      def->sig = func.sig;
    }

    if (func.complete) {
      def->stmt = func.stmt;
    }
  } else if (cur_token.kind == ';') {
    struct Global *global = add_global(dec.identifier);

    if (global->type && !type_eq(global->type, dec.type)) {
      printf("Semantic error: redefining global with different type\n");
      FAIL;
    }

    global->type = dec.type;

    eat_token(';');
  } else if (cur_token.kind == '=') {
    eat_token('=');

    // TODO for now skip initializer
    while (cur_token.kind != ';') {
      read_token();
    }

    eat_token(';');

    struct Global *global = add_global(dec.identifier);

    if (global->type) {
      if (!type_eq(global->type, dec.type)) {
        printf("Semantic error: redefining global with different type\n");
        FAIL;
      }

      free(dec.type);
    } else {
      global->type = dec.type;
    }

    if (global->complete) {
      printf("Semantic error: redefining global\n");
      FAIL;
    }

    // TODO store initializer in global
  }

  free(dec.identifier);
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
  // struct Type *type = match_type();

  // add variable to scope
  eat_token(IDENT);

  if (cur_token.kind == '=') {
    eat_token('=');
    // struct Expr *expr = match_expr();
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
    // struct Stmt *stmt = match_stmt();
    // add stmt to ast
  }

  eat_token('}');

  return NULL;
}

// parse start_symbol
void parse() {
  setup_lexer();

  read_token();

  while (cur_token.kind != END) {
    match_outer_dec();
  }
}
