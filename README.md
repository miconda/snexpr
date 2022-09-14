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

Note: at this moment support for escaped characters is not implemented.

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

## Credits ##

Project started from:

  - [https://github.com/zserge/expr](https://github.com/zserge/expr)

## License ##

MIT
