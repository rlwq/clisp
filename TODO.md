# TODO

## Language Features

* [ ] `set` special form - variable mutation
* [ ] `begin` special form - sequential expressions in one block
* [ ] Comments - `;` to end of line
* [ ] `bool` type - some kind of unified representation of True/False values
* [ ] `'expr` - syntactic sugar for `(quote expr)`
* [ ] Variadic lambdas - `(lambda (x . rest) ...)`
* [ ] Error handling & recovery - runtime errors with catch possibilities

## Built-ins

* [ ] Arithmetic & comparison: `*`, `/`, `%`, `<`, `>`, `<=`, `>=`
* [ ] Lists: `list`, `length`, `append`, `map`, `filter`

## Infrastructure

* [ ] REPL - interactive read-eval-print loop
* [ ] Macro system
* [ ] Module system - `(load "file.rkl")`

## Bugs

* [ ] `print_cons` - crashes on empty list (`NIL`)
* [ ] `print_expr` - crashes on an `cons` with its `cdr` not equal to `NIL` 
* [ ] Lambda calls have no arity check

