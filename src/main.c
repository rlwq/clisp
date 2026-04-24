#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_array.h"
#include "string_view.h"
#include "tokenizer.h"
#include "parser.h"
#include "lisp_ast.h"
#include "evaluator.h"

LispAST *lisp_add(LispAST *args) {
    int result_value = 0;

    for (; args->kind != LISP_NIL; args = args->as.cons.cdr)
        result_value += args->as.cons.car->as.integer;

    LispAST *result = malloc(sizeof(LispAST));
    result->kind = LISP_INTEGER;
    result->as.integer = result_value;
    return result;
}

LispAST *lisp_reverse_list(LispAST *args) {
    DA(LispAST *) new_args;
    da_init(new_args);

    for (LispAST *curr_arg = args;
            curr_arg->kind != LISP_NIL;
            curr_arg = curr_arg->as.cons.cdr) {
            da_push(new_args, curr_arg->as.cons.car);
    }

    LispAST *result = malloc(sizeof(LispAST));
    result->kind = LISP_NIL;
    for (size_t i = 0; i < new_args.size; i++) {
        LispAST *curr = malloc(sizeof(LispAST));
        curr->kind = LISP_CONS;
        curr->as.cons.car = da_at(new_args, new_args.size - i - 1);
        curr->as.cons.cdr = result;
        result = curr;
    }

    da_free(new_args);

    return result;
}

LispAST *lisp_id(LispAST *args) {
    return args;
}
void parse(Parser *parser) {
    assert(PARSER_VALID_STATE(*parser));
    while (da_at(parser->tokens, parser->cursor).kind != TK_EOF) {
        da_push(parser->exprs, parse_expr(parser));
    }
}

char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    assert(file);

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char *src = malloc(size + 1);
    assert(src);

    fread(src, 1, size, file);
    src[size] = '\0';

    fclose(file);
    return src;
}

int main(int argc, char** argv) {
    assert(argc == 2);
    char *src = read_file(argv[1]);
    StringView prog = sv_mk(src);

    Tokenizer *t = tokenizer_alloc(prog);
    tokenize(t);

    Parser *p = parser_alloc(t->tokens);
    parse(p);

    Evaluator *evaluator = evaluator_alloc(p->exprs);
    register_builtin(evaluator, sv_mk("add"), lisp_add);
    register_builtin(evaluator, sv_mk("reverse"), lisp_reverse_list);
    register_builtin(evaluator, sv_mk("id"), lisp_id);
    evaluate_all(evaluator);

    return 0;
}

