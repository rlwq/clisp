#ifndef SCOPE_H
#define SCOPE_H

#include "lisp_node.h"

typedef struct Scope Scope;

struct Scope {
    Scope *parent;
    
    DA(StringView) symbols;
    LispNodePtrDA values;
    
    Scope *heap_next;
    bool marked;
};

void scope_define(Scope *scope, StringView name, LispNode *value);
LispNode *scope_get(Scope *scope, LispNode *expr);

#endif
