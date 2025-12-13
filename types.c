#include <stdio.h>
#include <string.h>

#include "fail.h"
#include "symbols.h"
#include "types.h"

char *type_repr[] = {
    "int",   "char", "float",  "void",  "pointer",
    "array", "enum", "struct", "union", "function",
};

// returns 1 if two types are equivalent
int type_eq(struct Type *l, struct Type *r) {
  if (l->kind != r->kind) {
    return 1;
  };

  switch (l->kind) {
  case T_INT:
  case T_CHAR:
  case T_FLOAT:
  case T_VOID:
    // builtins match if the enum is the same
    return 1;

  case T_ARRAY:
    if (l->array.len != r->array.len) {
      return 0;
    }
    // fallthrough
  case T_POINTER:
    return type_eq(l->ptr_type, r->ptr_type);

  case T_STRUCT:
  case T_UNION:
  case T_ENUM:
    // should point to same definition
    return l->struct_type == r->struct_type;

  case T_FUNC:
    // check return type is same and parameters are same

    if (type_eq(l->func_sig->ret, r->func_sig->ret)) {
      return 1;
    }

    struct Param *param1 = l->func_sig->params;
    struct Param *param2 = r->func_sig->params;

    while (param1 != NULL && param2 != NULL) {
      if (type_eq(param1->type, param2->type)) {
        return 1;
      }
    }

    // check that both are null
    return param1 == param2;
  }

  FAIL;
  return 0;
}

void debug_type(struct Type *type) {
  switch (type->kind) {
  case T_FUNC:
    printf("fn (");
    struct Param *param = type->func_sig->params;

    while (param != NULL) {
      debug_type(param->type);

      param = param->next;

      if (param) {
        printf(", ");
      }
    }

    printf(") -> ");
    debug_type(type->func_sig->ret);
    break;
  case T_ENUM:
  case T_STRUCT:
  case T_UNION:
    printf("%s ", type_repr[type->kind]);
    if (type->struct_type->name)
      printf("%s", type->struct_type->name);
    else
      printf("anon");
    break;
  case T_POINTER:
    printf("(");
    debug_type(type->ptr_type);
    printf(")*");
    break;
  case T_ARRAY:
    printf("(");
    debug_type(type->array.elem_type);

    printf(")[");

    if (type->array.len != -1) {
      printf("%d", type->array.len);
    }

    printf("]");

    break;
  default:
    printf("%s", type_repr[type->kind]);
  }
}

// check if a type is sound
// i.e. arrays are sized, functions can't return arrays
// returns pointer to offending type
struct Type *type_sound(struct Type *type) {
  struct Type *cur = type;

  while (1) {
    if (cur == NULL) {
      return type;
    }

    switch (cur->kind) {
    case T_STRUCT:
    case T_UNION:;
      struct Field *field = cur->struct_type->fields;

      while (field != NULL) {
        struct Type *t = type_sound(field->type);
        if (t)
          return t;

        field = field->next;
      }

      return NULL;

    case T_CHAR:
    case T_VOID:
    case T_INT:
    case T_FLOAT:
    case T_ENUM:
      // don't have to check anything
      return NULL;

    case T_POINTER:
      cur = cur->ptr_type;

      break;

    case T_ARRAY:
      // arrays must be sized except for in parameters
      if (cur->array.len <= 0) {
        return cur;
      }

      // can't have array of functions
      if (cur->array.elem_type->kind == T_FUNC) {
        return cur;
      }

      cur = cur->array.elem_type;

      break;

    case T_FUNC:;
      // check parameters and return type
      struct Param *param = cur->func_sig->params;

      while (param != NULL) {
        if (param->type->kind == T_ARRAY) {
          struct Type *t = type_sound(param->type->array.elem_type);
          if (t)
            return t;
        } else {
          struct Type *t = type_sound(param->type);
          if (t)
            return t;
        }

        param = param->next;
      }

      // function can't return array or function
      if (cur->func_sig->ret->kind == T_ARRAY ||
          cur->func_sig->ret->kind == T_FUNC) {
        return cur;
      }

      cur = cur->func_sig->ret;

      break;
    }
  }

  return NULL;
}

// verify type and print message if it is a failure
// TODO: could have more helpful message
void type_verify(struct Type *type) {
  struct Type *t = type_sound(type);

  if (t) {
    printf("Semantic error: illegal type '");
    debug_type(t);

    if (t != type) {
      printf("' in type definition '");
      debug_type(type);
    }

    printf("'\n");

    if (t->kind == T_FUNC) {
      if (t->func_sig->ret->kind == T_FUNC) {
        printf("Cannot define function returning a function\n");
      } else if (t->func_sig->ret->kind == T_ARRAY) {
        printf("Cannot define function returning an array\n");
      }
    } else if (t->kind == T_ARRAY) {
      if (t->array.elem_type->kind == T_FUNC) {
        printf("Cannot define array of functions\n");
      } else if (t->array.len == -1) {
        printf("Unsized array\n");
      }
    }

    FAIL;
  }
}
