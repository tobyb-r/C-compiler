#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "fail.h"
#include "lexer.h"
#include "parser.h"
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

// recursive function that matches the declarator
// dec is a pointer to the inner declaration
// type is a pointer to the current declaration we are operating on
// the recursive call returns the new pointer we operate on
// https://c-faq.com/decl/spiral.anderson.html
struct Type **match_dec_rec(struct Dec *dec, struct Type **type) {
  switch (cur_token.kind) {
  case STAR:
    eat_token(STAR);

    type = match_dec_rec(dec, type);

    struct Type *ptr_type = malloc(sizeof(*ptr_type));
    ptr_type->kind = T_POINTER;
    ptr_type->ptr_type = *type;
    ptr_type->istypedef = 0;

    *type = ptr_type;

    return &ptr_type->ptr_type;

  case '(':
    eat_token('(');

    if (cur_token.kind == ')') {
      printf("Syntax error: Expected identifier, found ')'\n");
      FAIL;
    }

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

  case '[':
    // no identifier

    dec->identifier = NULL;

    break;

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
      arr_type->istypedef = 0;

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
      f_type->istypedef = 0;

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
    char name[256];
    strcpy(name, cur_token.identifier);

    eat_token(IDENT);

    if (cur_token.kind == '{') {
      type.struct_type = add_struct(name);

      if (type.struct_type->complete) {
        printf("Semantic error: redefining struct\n");
      }

      type.struct_type->fields = match_fields();
    } else {
      type.struct_type = lookup_struct(name);

      if (!type.struct_type) {
        type.struct_type = add_struct(name);
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
      sym.type->istypedef = 1;

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
  int _line = line;
  int _line_col = line_col;

  if (dec.identifier == NULL) {
    printf("Syntax error: Expected identifier\n");
    FAIL;
  }

  if (dec.type->kind == T_FUNC) {
    struct Func func = {0};

    func.name = dec.identifier;
    func.sig = dec.type->func_sig;

    if (!dec.type->istypedef)
      free(dec.type);

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
        line = _line;
        line_col = _line_col;
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

    if (global->type) {
      if (!type_eq(global->type, dec.type)) {
        printf("Semantic error: redefining global with different type\n");
        line = _line;
        line_col = _line_col;
        FAIL;
      }

      free_type(dec.type);
    } else {
      global->type = dec.type;
    }

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
        line = _line;
        line_col = _line_col;
        FAIL;
      }

      free_type(dec.type);
    } else {
      global->type = dec.type;
    }

    if (global->complete) {
      printf("Semantic error: redefining global\n");
      line = _line;
      line_col = _line_col;
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

struct Expr *match_primary_rec();

struct Expr *match_primary_expr() {
  // primary ::=
  //   | var-name
  //   | constant
  //   | ( expr )
  //   | `prefix-op` primary
  //   | primary `postfix-op`
  //
  // indexing and calling functions are postfix operators
  // '*' and '&' are prefix operators
  // postfix operators have higher precedence than prefix operators
  //
  // parsed like declarators

  struct Expr *expr = match_primary_rec();

  return expr;
}

// prefix operators
// TODO:  ++ -- & + - ~ !
enum UnOp unoperators[256] = {
    [AMP] = O_REF,
    [STAR] = O_DEREF,
};

// similar to match_dec_rec
struct Expr *match_primary_rec() {
  struct Expr *expr = malloc(sizeof(*expr));

  // prefix operators
  while (unoperators[cur_token.kind]) {
    expr->kind = E_UNOP;
    expr->unop.op = unoperators[cur_token.kind];
    expr->unop.expr = malloc(sizeof(*expr->unop.expr));
    expr = expr->unop.expr;
  }

  // primary
  switch (cur_token.kind) {
  case INTEGER:
    *expr = (struct Expr){
        .kind = E_CONST,
        .cnst = {.kind = C_INT, .int_literal = cur_token.int_literal}};

    read_token();
    break;
  case IDENT:;
    struct Symbol *sym = lookup_symbol(cur_token.identifier);
    if (sym->kind == S_VAR) {
      *expr = (struct Expr){.kind = E_VAR, .var = sym->var};
    } else if (sym->kind == S_FUNC) {
      *expr = (struct Expr){.kind = E_FUNC, .func = sym->func};
    } else {
      printf("Unexpected symbol %s \"%s\" in expression\n",
             symbol_repr[sym->kind], cur_token.identifier);
      FAIL;
    }
    read_token();
    break;
  case '(':
    eat_token('(');
    expr = match_expr();
    eat_token(')');
    break;
  default:
    printf("Syntax error: Unexpected %s in expression\n",
           token_repr[cur_token.kind]);
    FAIL;
  }

  // postfix operators
  // TODO: ++ -- -> .
  while (1) {
    if (cur_token.kind == '[') {
      FAIL; // TODO
    } else if (cur_token.kind == '(') {
      FAIL; // TODO
    } else {
      break;
    }
  }

  return expr;
}

// precedence of tokens for different operators
// operators are left assoc except for assignment which is right assoc
// 1 * / %
// 2 + -
// 3 >> <<
// 4 > < >= <=
// 5 == !=
// 6 & then ^ then | then && then ||
// 7 ternary ? :
// 8 assignment = += -=
// TODO: unfinished
// precedence[op] is 0 for operators that aren't written below
int precedences[256] = {
    [STAR] = 6, [SLASH] = 6, [MOD] = 6,            // multiplicative ops
    [PLUS] = 5, [MINUS] = 5,                       // additive ops
    [LT] = 4,   [GT] = 4,    [LTE] = 4, [GTE] = 4, // comparisons
    [EQ] = 3,   [NE] = 3,                          // == !=
    [AND] = 2,  [OR] = 1,                          // logic
};

enum BinOp operators[256] = {
    // arithmetic
    [STAR] = O_MUL,
    [SLASH] = O_DIV,
    [MOD] = O_MOD,
    [PLUS] = O_ADD,
    [MINUS] = O_SUB,

    // comparisons
    [LT] = O_LT,
    [GT] = O_GT,
    [LTE] = O_LTE,
    [GTE] = O_GTE,
    [EQ] = O_EQ,
    [NE] = O_NE,

    // logic
    [AND] = O_AND,
    [OR] = O_OR,
};

// construct expression from the left hand side and the precedence
// precedence is the minumum precedence for the lhs to be applied with the next
// operator instead of the previous one
struct Expr *match_expr_op(struct Expr *lhs, int precedence) {
  // from wikipedia:
  //
  // parse_expression_1(lhs, min_precedence)
  //   lookahead := peek next token
  //
  //   while (lookahead is a binary operator whose precedence is >=
  //          min_precedence)
  //     op := lookahead
  //     advance to next token
  //     rhs := parse_primary ()
  //     lookahead := peek next token
  //
  //     while (lookahead is a binary operator whose precedence is greater
  //            than op's, or a right-associative operator
  //            whose precedence is equal to op's)
  //       rhs := parse_expression_1 (rhs, precedence of op + (1 if lookahead
  //              precedence is greater, else 0))
  //       lookahead := peek next token
  //
  //     lhs := the result of applying op with operands lhs and rhs
  //   return lhs

  int cur_precedence = precedences[cur_token.kind];

  // if this operator has higher precedence than the one before lhs
  while (cur_precedence > precedence) {
    enum BinOp op = operators[cur_token.kind];
    read_token();

    struct Expr *rhs = match_primary_expr();
    int next_precedence = precedences[cur_token.kind];

    // if the next operator has higher precedence than cur we apply rhs
    while (next_precedence > cur_precedence) {
      rhs = match_expr_op(rhs, cur_precedence);
      next_precedence = precedences[cur_token.kind];
    }

    struct Expr *new = malloc(sizeof(*new));

    *new = (struct Expr){.kind = E_BINOP, .binop = {op, lhs, rhs}};

    lhs = new;

    cur_precedence = next_precedence;
  }

  if (cur_token.kind == ASSIGN) {
    eat_token(ASSIGN);
    struct Expr *new = malloc(sizeof(*new));

    struct Expr *rhs = match_expr();

    *new = (struct Expr){.kind = E_BINOP, .binop = {O_ASSIGN, lhs, rhs}};

    lhs = new;
  }

  return lhs;
}

struct Expr *match_expr() {
  // assignment is an expression
  //
  // expr ::=
  //   | `prefix-op` expr
  //   | expr `op` expr
  //   | lvalue = expr
  //     - special case of `op`
  //     - this is the only right assoc operator
  //     - for assignment we parse the whole rhs of = as an expr ignoring prec
  // can do operator precedence parsing

  struct Expr *primary = match_primary_expr();
  struct Expr *expr = match_expr_op(primary, 0);

  return expr;
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

  switch (cur_token.kind) {
  case SEMICOLON:
    return NULL;
  // blocks
  case FOR:
    // match for
    eat_token(FOR);
    return NULL;
  case WHILE:
    // match for
    eat_token(WHILE);
    return NULL;
  case IF:
    // match for
    eat_token(IF);
    return NULL;

  // type - declaration
  case INT_TYPE:
  case CHAR_TYPE:
  case VOID_TYPE:
  case FLOAT_TYPE:
    goto dec;

  case IDENT:;
    // match if it is type or not
    struct Symbol *symbol = lookup_symbol(cur_token.identifier);

    if (!symbol) {
      printf("Semantic error: undefined symbol %s\n", cur_token.identifier);
      FAIL;
    }

    if (symbol->kind == S_TYPEDEF) {
      goto dec;
    } else {
      goto expr;
    }
  default:
    printf("Syntax error: Unexpected token '%s' in statement\n",
           token_repr[cur_token.kind]);
    FAIL;
  }

dec:;
  struct Type type = match_type();
  struct Dec dec = match_declarator(type);
  // add to scope

  if (cur_token.kind == '=') {
    // return assignment expression
    return NULL;
  } else {
    eat_token(';');
    return NULL;
  }

expr:;
  struct Expr *expr = match_expr();

  struct Stmt *stmt = malloc(sizeof(*stmt));
  *stmt = (struct Stmt){.kind = S_EXPR, .expr = expr};

  return stmt;
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

  while (cur_token.kind != END) {
    match_outer_dec();
  }
}
