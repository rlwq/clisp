#include "string_view.h"
#include "tokenizer.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum {
    LISP_NIL,
    LISP_CONS,
    LISP_SYMBOL,
    LISP_INTEGER,
    LISP_STRING,
} LISP_AST_KIND;

typedef struct LispAST LispAST;

typedef struct {
    LispAST *car;
    LispAST *cdr;
} Cons;

struct LispAST {
    LISP_AST_KIND kind;
    union {
        StringView symbol;
        StringView string;
        Cons cons;
        int integer;
    } as;
};

LispAST *parse_expr(Token *tokens, size_t *curr, size_t size) {
    // S-expr
    assert(*curr < size);
    if (tokens[*curr].kind == TK_L_PAREN) {
        (*curr)++;
        
        // TODO: rewrite that
        LispAST **subexprs = malloc(sizeof(LispAST *) * 256);
        size_t subexprs_count = 0;

        while (tokens[*curr].kind != TK_R_PAREN)
            subexprs[subexprs_count++] = parse_expr(tokens, curr, size);
        
        LispAST *result = malloc(sizeof(LispAST));
        result->kind = LISP_NIL;
        
        for (size_t i = 0; i < subexprs_count; i++) {
            LispAST *head = malloc(sizeof(LispAST));
            head->kind = LISP_CONS;
            head->as.cons.cdr = result;
            head->as.cons.car = subexprs[subexprs_count - i - 1];
            result = head;
        }
        free(subexprs);
        
        assert(tokens[*curr].kind == TK_R_PAREN);
        (*curr)++;
        
        return result;
    }
    
    // Integer
    if (tokens[*curr].kind == TK_INTEGER) {
        (*curr)++;

        LispAST *ast = malloc(sizeof(LispAST));
        ast->kind = LISP_INTEGER;
        ast->as.integer = 6969;  // TODO: remove
        
        return ast;
    }
    
    // Symbol
    if (tokens[*curr].kind == TK_SYMBOL) {
        (*curr)++;

        LispAST *ast = malloc(sizeof(LispAST));
        ast->kind = LISP_SYMBOL;
        ast->as.symbol = sv_mk("SYMBOL");  // TODO: remove
        
        return ast;
    }
    
    // String
    if (tokens[*curr].kind == TK_STRING) {
        (*curr)++;

        LispAST *ast = malloc(sizeof(LispAST));
        ast->kind = LISP_STRING;
        ast->as.string = sv_mk("Hello, World!");  // TODO: remove
 
        return ast;
    }

    assert(0 && "Unreachable");
}

int main() {
    StringView prog = sv_mk("(printf \"Hello, %s. You are %d years old.\" \"David\" (add 33 34))");
 
    printf("%.*s\n", (int) prog.size, prog.data);

    Token t = parse_token(&prog);
    while (t.kind != TK_EOF) {
        printf(TOKEN_FMT"\n", TOKEN_ARGS(t));
        t = parse_token(&prog);
    }
    return 0;
}

