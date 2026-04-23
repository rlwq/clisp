#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "lisp_ast.h"

LispAST *lisp_eval(LispAST *expr, Env *env);
LispAST *lisp_print(LispAST);

#endif
