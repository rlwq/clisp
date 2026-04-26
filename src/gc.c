#include "gc.h"
#include "utils.h"

static LispAST *ast_heap = NULL;
static size_t ast_heap_size = 0;

size_t heap_size() {
    return ast_heap_size;
}

LispAST *gc_alloc(LISP_AST_KIND kind) {
    LispAST *node = malloc(sizeof(LispAST));
    assert(node); //TODO: add some error reporting
    
    ast_heap_size++;
    node->kind = kind;

    node->marked = false;
    node->next = ast_heap;
    ast_heap = node;

    return node;
}

void gc_free(LispAST *expr) {
    assert(!expr->marked);
    ast_heap_size--;

    switch (expr->kind) {
        case LISP_NIL:
        case LISP_INTEGER:
        case LISP_CONS:
        case LISP_SYMBOL:
        case LISP_BUILTIN:
        case LISP_STRING:
            free(expr);
        break;

        case LISP_LAMBDA:
            da_free(expr->as.lambda.args);
            free(expr);
        break;
    }
}

void gc_sweep() {
    LispAST **curr = &ast_heap;

    while (*curr) {
        if ((*curr)->marked) {
            (*curr)->marked = false;
            curr = &((*curr)->next);
        }
        else {
            LispAST *dead = *curr;
            *curr = dead->next;
            gc_free(dead);
        }
    }
}

void gc_mark(LispAST *expr) {
    assert(expr);
    
    if (expr->marked) return;

    expr->marked = true;

    switch (expr->kind) {
        case LISP_NIL:
        case LISP_SYMBOL:
        case LISP_INTEGER:
        case LISP_STRING:
        case LISP_BUILTIN:
            // Do nothin
        break;

        case LISP_LAMBDA:
            NOT_IMPLEMENTED();
        break;

        case LISP_CONS:
            gc_mark(expr->as.cons.car);
            gc_mark(expr->as.cons.cdr);
        break;
    }
}


