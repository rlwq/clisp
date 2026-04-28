#ifndef SCOPE_H
#define SCOPE_H

#include "forwards.h"
#include "string_view.h"
#include "stdbool.h"

struct Scope {
    Scope *parent;
    
    DA(StringView) symbols;
    LispNodePtrDA values;
    
    Scope *heap_next;
    bool marked;
};

void scope_define(Scope *scope, StringView name, LispNode *value);
LispNode *scope_get(Scope *scope, StringView name);

#endif
