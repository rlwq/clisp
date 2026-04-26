#ifndef GC_H
#define GC_H

#include "typedefs.h"
#include "lisp_ast.h"

struct GC {
    LispAST *nodes_heap;
    size_t nodes_count;
    
    Scope *scopes_heap;
    size_t scopes_count;
};

GC *gc_alloc();
void gc_free(GC *gc);

LispAST *gc_alloc_node(GC *gc, LISP_AST_KIND kind);

void gc_mark_node(LispAST *expr);
void gc_free_node(GC *gc, LispAST *expr);

void gc_sweep(GC *gc);

#endif
