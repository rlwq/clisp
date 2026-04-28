#ifndef GC_H
#define GC_H

#include "typedefs.h"
#include "lisp_node.h"

struct GC {
    LispNode *nodes_heap;
    size_t nodes_count;

    Scope *scopes_heap;
    size_t scopes_count;
};

GC *gc_alloc(void);
void gc_free(GC *gc);

LispNode *gc_alloc_node(GC *gc, LISP_NODE_KIND kind);

Scope *gc_alloc_scope(GC *gc, Scope *parent);
void gc_free_scope(GC *gc, Scope *scope);

void gc_mark_node(LispNode *expr);
void gc_free_node(GC *gc, LispNode *expr);

void gc_mark_scope(Scope *scope);

void gc_sweep(GC *gc);

#endif
