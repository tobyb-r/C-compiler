#ifndef TYPES_HEADER
#define TYPES_HEADER

// type of a variable/expression
struct Type {
  enum {
    T_INT,
    T_CHAR,
    T_FLOAT,
    T_VOID,
    T_POINTER,
    T_ARRAY,
    T_ENUM,
    T_STRUCT,
    T_UNION,
    T_FUNC
  } kind;

  union {
    struct Type *ptr_type;
    struct {
      struct Type *elem_type;
      int len;
    } array;
    struct Enum *enum_type;
    struct Struct *struct_type;
    struct Union *union_type;
    // for function pointers
    struct FuncSig *func_sig;
  };
};

extern char *type_repr[];

int type_eq(struct Type *l, struct Type *r);

void debug_type(struct Type *type);

struct Type *type_sound(struct Type *type);

void type_verify(struct Type *type);

void free_func_sig(struct FuncSig *sig);

void free_type(struct Type *type);

#endif
