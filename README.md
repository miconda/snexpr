# SNExpr #

Strings and numbers expression evaluation with auto-conversion.

Examples:

```
(1+2)*3-4
"Hello" + " World!"
```

Numbers can be integers or floating point values (internally it uses
the `float` type).

Examples:

```
123
4.56
```

The strings have to be enclosed in between single or double quotes.

Examples:

```
'abc efg'

"mno prs"
```

It has also support for escaped characters:

  - `\n` - new line
  - `\r` - carriage return
  - `\t` - horizontal tab
  - any other `\c` sequence is replaced with `c` (e.g., `\"` is set to `"`)

Example:

```
"abc\"def" is set to `abc"cde`
```

## Implemented Operators ##

Arithmetic operations:

  - `+` - addition
  - `-` - subtraction
  - `*` - multiplication
  - `/` - division
  - `%` - modulus (remainder)
  - `**` - power

Bitwise operations

  - `<<` - shift left
  - `>>` - shift right
  - `&` - and
  - `|` - or
  - `^` - xor (unary bitwise negation)

Logical operations:

  - `==` - equal
  - `!=` - not equal (different)
  - `<` - less than
  - `>` - greater than
  - `<=` - less than or equal to
  - `>=` - greater than or equal to
  - `&&` - and
  - `||` - or
  - `!` - unary not

String operations:

  - `+` - concatenation

Other operations:

  - `=` - assignment
  - `( ... )` - parenthesis to group parts of the expression
  - `,` - comma (separates expressions or function parameters)


## Auto Conversion ##

The auto-conversion is done to the type of the left operand of an expression.

Examples:

```
1 + "20" -> converted to: 1 + 20 (result: 21)
```

```
"1" + 20 -> converted to: "1" + "20" (result: "120")
```

```
4 > "20" -> converted to: 4 > 20 (result: false)
```

```
"4" > 20 -> converted to: "4" > "20" (result: true)
```

## Logical Evaluation ##

For numbers, a value different than `0` is `true` and `0` is `false``.

Comparison for numbers is done in the standard way, based on values.

For strings, empty value is `false` and anything else is `true`.

Comparison for two strings is done internally using the C function `strcmp()`:

  - if the result of `strcmp()` is negative, the first string is less than second string
  - if the result of `strcmp()` is positive, the first string is greater than the second string
  - if the result of `strcmp()` is zero, the two strings are equal

Comparison between a number and a string (or vice-versa) is done using auto-conversion
of the right operand.

## Usage ##

Include `snexpr.h` in your `.c/.cpp` file, the use the functions to create and
evaluate expressions from a string. Main functions:

  * `struct snexpr *snexpr_create(const char *s, size_t len, struct snexpr_var_list *vars struct snexpr_func *funcs, snexternval_cbf_t evcbf)` - create an expression provided
  inside string `s` that has the length `len`, setting the environment with a list
  of local variables and functions, plus a callback to get the values for external
  variables.
  * `struct snexpr *snexpr_eval(struct snexpr *e)` - evaluate the expression, returning
  `NULL` on error, otherwise it should be a pointer to a new expression that holds:
    * a string value: `char *` in `result->param.stz.sval` when `result->type==SNE_OP_CONSTSTZ`
    * a number value: `float` in result->param.num.nval when `result->type==SNE_OP_CONSTNUM`
  * `void snexpr_result_free(struct snexpr *e)` - free the result of expression evaluation
  * `void snexpr_destroy_args(struct snexpr *e)` - destroy the created expression


Simple example to evaluate an arithmetic expression:

```c
#include "snexpr.h"

#include <stdio.h>

int main(int argc, char *argv[])
{
  char *s = "(2+3)*4";
  struct snexpr_var_list vars = {0};
  struct snexpr *e = snexpr_create(s, strlen(s), &vars, NULL, NULL);

  if(e == NULL) {
    printf("FAIL: creaing %s returned NULL\n", s);
    return -1;
  }

  struct snexpr *result = snexpr_eval(e);

  if(result==NULL) {
    printf("FAIL: result is NULL\n");
    snexpr_destroy(e, &vars);
    return -1;
  }

  if(result->type != SNE_OP_CONSTNUM) {
    printf("FAIL: result is not a number (%d!=%d)\n", result->type, SNE_OP_CONSTNUM);
    snexpr_result_free(result);
    snexpr_destroy(e, &vars);
    return 1;
  }

  printf("OK: result of '%s' is %f\n", s, result->param.num.nval);

  snexpr_result_free(result);
  snexpr_destroy(e, &vars);

  return 0;
}
```

See more examples in `test/tsnexpr.c`.

## Credits ##

Project started from:

  - [https://github.com/zserge/expr](https://github.com/zserge/expr)

## License ##

MIT
