#ifndef LISP_NODE_H
#define LISP_NODE_H

#include "string_view.h"
#include "typedefs.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    LISP_NIL,
    LISP_CONS,
    LISP_SYMBOL,
    LISP_INTEGER,
    LISP_STRING,
    LISP_BUILTIN,
    LISP_LAMBDA,
} LISP_NODE_KIND;

typedef struct {
    LispNode *car;
    LispNode *cdr;
} ConsNode;

typedef struct {
    SV_DA args; 
    LispNode *expr;
    Scope *scope;
} LambdaNode;

typedef LispNode *(*LispBuiltin) (LispNode *args, GC *gc);

struct LispNode {
    LISP_NODE_KIND kind;
    bool marked;

    LispNode *heap_next;
    
    union {
        StringView symbol;
        StringView string;
        LispBuiltin builtin;
        LambdaNode lambda;
        ConsNode cons;
        int integer;
    } as;
};

#define CAR(n_) ((n_)->as.cons.car)
#define CDR(n_) ((n_)->as.cons.cdr)

#endif
