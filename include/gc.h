#ifndef GC_H
#define GC_H

#include "lisp_ast.h"

LispAST *gc_alloc(LISP_AST_KIND kind);
void gc_mark(LispAST *expr);
void gc_free(LispAST *expr);
void gc_sweep();

size_t heap_size();

#endif
