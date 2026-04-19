#include "lisp_ast.h"

Env env_init() {
    Env result;

    da_init(result.symbols);
    da_init(result.values);

    return result;
}

void env_define(Env *env, StringView name, LispAST *value) {
    da_push(env->symbols, name);
    da_push(env->values, value);
}

LispAST *env_get(Env *env, StringView name) {
    for (size_t i = 0; i < env->symbols.size; i++) {
        if (sv_eq(da_at(env->symbols, i), name))
            return da_at(env->values, i);
    }
    assert(0 && "Unreachable");
    return NULL;
}

