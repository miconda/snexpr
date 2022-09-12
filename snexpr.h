/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Serge Zaitsev
 * Copyright (c) 2022 Daniel-Constantin Mierla
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _SNEXPR_H_
#define _SNEXPR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h> /* for isspace */
#include <limits.h>
#include <math.h> /* for pow */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXPR_TOP (1 << 0)
#define EXPR_TOPEN (1 << 1)
#define EXPR_TCLOSE (1 << 2)
#define EXPR_TNUMBER (1 << 3)
#define EXPR_TSTRING (1 << 4)
#define EXPR_TWORD (1 << 5)
#define EXPR_TDEFAULT (EXPR_TOPEN | EXPR_TNUMBER | EXPR_TSTRING | EXPR_TWORD)

#define EXPR_UNARY (1 << 16)
#define EXPR_COMMA (1 << 17)
#define EXPR_EXPALLOC (1 << 18)
#define EXPR_VALALLOC (1 << 19)


/*
 * Simple expandable vector implementation
 */
static int vec_expand(char **buf, int *length, int *cap, int memsz)
{
	if(*length + 1 > *cap) {
		void *ptr;
		int n = (*cap == 0) ? 1 : *cap << 1;
		ptr = realloc(*buf, n * memsz);
		if(ptr == NULL) {
			return -1; /* allocation failed */
		}
		*buf = (char *)ptr;
		*cap = n;
	}
	return 0;
}
#define vec(T)   \
	struct       \
	{            \
		T *buf;  \
		int len; \
		int cap; \
	}
#define vec_init() \
	{              \
		NULL, 0, 0 \
	}
#define vec_len(v) ((v)->len)
#define vec_unpack(v) \
	(char **)&(v)->buf, &(v)->len, &(v)->cap, sizeof(*(v)->buf)
#define vec_push(v, val) \
	vec_expand(vec_unpack(v)) ? -1 : ((v)->buf[(v)->len++] = (val), 0)
#define vec_nth(v, i) (v)->buf[i]
#define vec_peek(v) (v)->buf[(v)->len - 1]
#define vec_pop(v) (v)->buf[--(v)->len]
#define vec_free(v) (free((v)->buf), (v)->buf = NULL, (v)->len = (v)->cap = 0)
#define vec_foreach(v, var, iter)                                             \
	if((v)->len > 0)                                                          \
		for((iter) = 0; (iter) < (v)->len && (((var) = (v)->buf[(iter)]), 1); \
				++(iter))

/*
 * Expression data types
 */
struct expr;
struct expr_func;
struct expr_var;

enum expr_type
{
	SNE_OP_UNKNOWN,
	SNE_OP_UNARY_MINUS,
	SNE_OP_UNARY_LOGICAL_NOT,
	SNE_OP_UNARY_BITWISE_NOT,

	SNE_OP_POWER,
	SNE_OP_DIVIDE,
	SNE_OP_MULTIPLY,
	SNE_OP_REMAINDER,

	SNE_OP_PLUS,
	SNE_OP_MINUS,

	SNE_OP_SHL,
	SNE_OP_SHR,

	SNE_OP_LT,
	SNE_OP_LE,
	SNE_OP_GT,
	SNE_OP_GE,
	SNE_OP_EQ,
	SNE_OP_NE,

	SNE_OP_BITWISE_AND,
	SNE_OP_BITWISE_OR,
	SNE_OP_BITWISE_XOR,

	SNE_OP_LOGICAL_AND,
	SNE_OP_LOGICAL_OR,

	SNE_OP_ASSIGN,
	SNE_OP_COMMA,

	SNE_OP_CONSTNUM,
	SNE_OP_CONSTSTZ,
	SNE_OP_VAR,
	SNE_OP_FUNC,
};

static int prec[] = {0, 1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 6, 7,
		8, 9, 10, 11, 12, 0, 0, 0, 0};

typedef vec(struct expr) vec_expr_t;
typedef void (*exprfn_cleanup_t)(struct expr_func *f, void *context);
typedef float (*exprfn_t)(struct expr_func *f, vec_expr_t *args, void *context);

struct expr
{
	enum expr_type type;
	unsigned int eflags;
	union
	{
		struct
		{
			float nval;
		} num;
		struct
		{
			char *sval;
		} stz;
		struct
		{
			struct expr_var *vref;
		} var;
		struct
		{
			vec_expr_t args;
		} op;
		struct
		{
			struct expr_func *f;
			vec_expr_t args;
			void *context;
		} func;
	} param;
};

#define expr_init()               \
	{                             \
		.type = (enum expr_type)0 \
	}

struct expr_string
{
	const char *s;
	int n;
};
struct expr_arg
{
	int oslen;
	int eslen;
	vec_expr_t args;
};

typedef vec(struct expr_string) vec_str_t;
typedef vec(struct expr_arg) vec_arg_t;

static int expr_is_unary(enum expr_type op)
{
	return op == SNE_OP_UNARY_MINUS || op == SNE_OP_UNARY_LOGICAL_NOT
		   || op == SNE_OP_UNARY_BITWISE_NOT;
}

static int expr_is_binary(enum expr_type op)
{
	return !expr_is_unary(op) && op != SNE_OP_CONSTNUM && op != SNE_OP_VAR
		   && op != SNE_OP_FUNC && op != SNE_OP_UNKNOWN;
}

static int expr_prec(enum expr_type a, enum expr_type b)
{
	int left = expr_is_binary(a) && a != SNE_OP_ASSIGN && a != SNE_OP_POWER
			   && a != SNE_OP_COMMA;
	return (left && prec[a] >= prec[b]) || (prec[a] > prec[b]);
}

#define isfirstvarchr(c) \
	(((unsigned char)c >= '@' && c != '^' && c != '|') || c == '$')
#define isvarchr(c)                                                            \
	(((unsigned char)c >= '@' && c != '^' && c != '|') || c == '$' || c == '#' \
			|| (c >= '0' && c <= '9'))

static struct
{
	const char *s;
	const enum expr_type op;
} OPS[] = {
		{"-u", SNE_OP_UNARY_MINUS},
		{"!u", SNE_OP_UNARY_LOGICAL_NOT},
		{"^u", SNE_OP_UNARY_BITWISE_NOT},
		{"**", SNE_OP_POWER},
		{"*", SNE_OP_MULTIPLY},
		{"/", SNE_OP_DIVIDE},
		{"%", SNE_OP_REMAINDER},
		{"+", SNE_OP_PLUS},
		{"-", SNE_OP_MINUS},
		{"<<", SNE_OP_SHL},
		{">>", SNE_OP_SHR},
		{"<", SNE_OP_LT},
		{"<=", SNE_OP_LE},
		{">", SNE_OP_GT},
		{">=", SNE_OP_GE},
		{"==", SNE_OP_EQ},
		{"!=", SNE_OP_NE},
		{"&", SNE_OP_BITWISE_AND},
		{"|", SNE_OP_BITWISE_OR},
		{"^", SNE_OP_BITWISE_XOR},
		{"&&", SNE_OP_LOGICAL_AND},
		{"||", SNE_OP_LOGICAL_OR},
		{"=", SNE_OP_ASSIGN},
		{",", SNE_OP_COMMA},

		/* These are used by lexer and must be ignored by parser, so we put
       them at the end */
		{"-", SNE_OP_UNARY_MINUS},
		{"!", SNE_OP_UNARY_LOGICAL_NOT},
		{"^", SNE_OP_UNARY_BITWISE_NOT},
};

static enum expr_type expr_op(const char *s, size_t len, int unary)
{
	for(unsigned int i = 0; i < sizeof(OPS) / sizeof(OPS[0]); i++) {
		if(strlen(OPS[i].s) == len && strncmp(OPS[i].s, s, len) == 0
				&& (unary == -1 || expr_is_unary(OPS[i].op) == unary)) {
			return OPS[i].op;
		}
	}
	return SNE_OP_UNKNOWN;
}

static float expr_parse_number(const char *s, size_t len)
{
	float num = 0;
	unsigned int frac = 0;
	unsigned int digits = 0;
	for(unsigned int i = 0; i < len; i++) {
		if(s[i] == '.' && frac == 0) {
			frac++;
			continue;
		}
		if(isdigit(s[i])) {
			digits++;
			if(frac > 0) {
				frac++;
			}
			num = num * 10 + (s[i] - '0');
		} else {
			return NAN;
		}
	}
	while(frac > 1) {
		num = num / 10;
		frac--;
	}
	return (digits > 0 ? num : NAN);
}

/*
 * Functions
 */
struct expr_func
{
	const char *name;
	exprfn_t f;
	exprfn_cleanup_t cleanup;
	size_t ctxsz;
};

static struct expr_func *expr_func(
		struct expr_func *funcs, const char *s, size_t len)
{
	for(struct expr_func *f = funcs; f->name; f++) {
		if(strlen(f->name) == len && strncmp(f->name, s, len) == 0) {
			return f;
		}
	}
	return NULL;
}

/*
 * Variables
 */
struct expr_var
{
	unsigned int evflags;
	char *name;
	union
	{
		float nval;
		char *sval;
	} v;
	struct expr_var *next;
};

struct expr_var_list
{
	struct expr_var *head;
};

static struct expr_var *expr_var(
		struct expr_var_list *vars, const char *s, size_t len)
{
	struct expr_var *v = NULL;
	if(len == 0 || !isfirstvarchr(*s)) {
		return NULL;
	}
	for(v = vars->head; v; v = v->next) {
		if(strlen(v->name) == len && strncmp(v->name, s, len) == 0) {
			return v;
		}
	}
	v = (struct expr_var *)calloc(1, sizeof(struct expr_var) + len + 1);
	if(v == NULL) {
		return NULL; /* allocation failed */
	}
	memset(v, 0, sizeof(struct expr_var) + len + 1);
	v->next = vars->head;
	v->name = (char *)v + sizeof(struct expr_var);
	strncpy(v->name, s, len);
	v->name[len] = '\0';
	vars->head = v;
	return v;
}

static int to_int(float x)
{
	if(isnan(x)) {
		return 0;
	} else if(isinf(x) != 0) {
		return INT_MAX * isinf(x);
	} else {
		return (int)x;
	}
}


static struct expr *expr_convert_num(float value, unsigned int ctype)
{
	struct expr *e = (struct expr *)malloc(sizeof(struct expr));
	if(e == NULL) {
		return NULL;
	}
	memset(e, 0, sizeof(struct expr));

	if(ctype == SNE_OP_CONSTSTZ) {
		e->eflags |= EXPR_EXPALLOC | EXPR_VALALLOC;
		e->type = SNE_OP_CONSTSTZ;
		asprintf(&e->param.stz.sval, "%g", value);
		return e;
	}

	e->eflags |= EXPR_EXPALLOC;
	e->type = SNE_OP_CONSTNUM;
	e->param.num.nval = value;
	return e;
}

static struct expr *expr_convert_stz(char *value, unsigned int ctype)
{
	struct expr *e = (struct expr *)malloc(sizeof(struct expr));
	if(e == NULL) {
		return NULL;
	}
	memset(e, 0, sizeof(struct expr));

	if(ctype == SNE_OP_CONSTNUM) {
		e->eflags |= EXPR_EXPALLOC;
		e->type = SNE_OP_CONSTNUM;
		e->param.num.nval = (float)atof(value);
		return e;
	}

	e->param.stz.sval = (char *)malloc(strlen(value) + 1);
	if(e->param.stz.sval == NULL) {
		free(e);
		return NULL;
	}
	e->eflags |= EXPR_EXPALLOC | EXPR_VALALLOC;
	e->type = SNE_OP_CONSTSTZ;
	strcpy(e->param.stz.sval, value);
	return e;
}

static struct expr *expr_concat_strz(char *value0, char *value1)
{
	struct expr *e = (struct expr *)malloc(sizeof(struct expr));
	if(e == NULL) {
		return NULL;
	}
	memset(e, 0, sizeof(struct expr));

	e->param.stz.sval = (char *)malloc(strlen(value0) + strlen(value1) + 1);
	if(e->param.stz.sval == NULL) {
		free(e);
		return NULL;
	}
	e->eflags |= EXPR_EXPALLOC | EXPR_VALALLOC;
	e->type = SNE_OP_CONSTSTZ;
	strcpy(e->param.stz.sval, value0);
	strcat(e->param.stz.sval, value1);
	return e;
}

static void expr_result_free(struct expr *e)
{
	if(e == NULL) {
		return;
	}
	if(!(e->eflags & EXPR_EXPALLOC)) {
		return;
	}
	if((e->eflags & EXPR_VALALLOC) && (e->type == SNE_OP_CONSTSTZ)
			&& (e->param.stz.sval != NULL)) {
		free(e->param.stz.sval);
	}
	free(e);
}

#define expr_eval_check_val(val, vtype) do { \
		if(val==NULL || val->type != vtype) { \
			goto error; \
		} \
	} while(0)

#define expr_eval_check_null(val, vtype) do { \
		if(val==NULL) { \
			goto error; \
		} \
	} while(0)

#define expr_eval_cmp(_CMPOP_) do { \
			rv0 = expr_eval(&e->param.op.args.buf[0]); \
			expr_eval_check_null(rv0, SNE_OP_CONSTNUM); \
			rv1 = expr_eval(&e->param.op.args.buf[1]); \
			expr_eval_check_null(rv1, SNE_OP_CONSTNUM); \
			if(rv0->type == SNE_OP_CONSTSTZ) { \
				/* string comparison */ \
				if(rv1->type == SNE_OP_CONSTNUM) { \
					tv = expr_convert_num(rv1->param.num.nval, SNE_OP_CONSTSTZ); \
					expr_result_free(rv1); \
					rv1 = tv; \
					expr_eval_check_val(rv1, SNE_OP_CONSTSTZ); \
				} \
				lv = expr_concat_strz(rv0->param.stz.sval, rv1->param.stz.sval); \
				if(strcmp(rv0->param.stz.sval, rv1->param.stz.sval) _CMPOP_ 0) { \
					lv = expr_convert_num(1, SNE_OP_CONSTNUM); \
				} else { \
					lv = expr_convert_num(0, SNE_OP_CONSTNUM); \
				} \
			} else { \
				/* number comparison */ \
				if(rv1->type == SNE_OP_CONSTSTZ) { \
					tv = expr_convert_stz(rv1->param.stz.sval, SNE_OP_CONSTNUM); \
					expr_result_free(rv1); \
					rv1 = tv; \
					expr_eval_check_val(rv1, SNE_OP_CONSTNUM); \
				} \
				lv = expr_convert_num( \
						rv0->param.num.nval _CMPOP_ rv1->param.num.nval, SNE_OP_CONSTNUM); \
			} \
	} while(0)

static struct expr *expr_eval(struct expr *e)
{
	float n;
	struct expr *lv = NULL;
	struct expr *rv0 = NULL;
	struct expr *rv1 = NULL;
	struct expr *tv = NULL;

	switch(e->type) {
		case SNE_OP_UNARY_MINUS:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			lv = expr_convert_num(-(rv0->param.num.nval), SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_UNARY_LOGICAL_NOT:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			lv = expr_convert_num(!(rv0->param.num.nval), SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_UNARY_BITWISE_NOT:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			lv = expr_convert_num(~(to_int(rv0->param.num.nval)), SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_POWER:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			expr_eval_check_val(rv1, SNE_OP_CONSTNUM);
			lv = expr_convert_num(
					powf(rv0->param.num.nval, rv1->param.num.nval),
					SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_MULTIPLY:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			expr_eval_check_val(rv1, SNE_OP_CONSTNUM);
			lv = expr_convert_num(
					rv0->param.num.nval * rv1->param.num.nval, SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_DIVIDE:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			expr_eval_check_val(rv1, SNE_OP_CONSTNUM);
			if(rv1->param.num.nval == 0) {
				goto error;
			}
			lv = expr_convert_num(
					rv0->param.num.nval / rv1->param.num.nval, SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_REMAINDER:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			expr_eval_check_val(rv1, SNE_OP_CONSTNUM);
			lv = expr_convert_num(
					fmodf(rv0->param.num.nval, rv1->param.num.nval),
					SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_PLUS:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_null(rv0, SNE_OP_CONSTNUM);
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			expr_eval_check_null(rv1, SNE_OP_CONSTNUM);
			if(rv0->type == SNE_OP_CONSTSTZ) {
				/* string concatenation */
				if(rv1->type == SNE_OP_CONSTNUM) {
					tv = expr_convert_num(rv1->param.num.nval, SNE_OP_CONSTSTZ);
					expr_result_free(rv1);
					rv1 = tv;
					expr_eval_check_val(rv1, SNE_OP_CONSTSTZ);
				}
				lv = expr_concat_strz(rv0->param.stz.sval, rv1->param.stz.sval);
			} else {
				/* add */
				if(rv1->type == SNE_OP_CONSTSTZ) {
					tv = expr_convert_stz(rv1->param.stz.sval, SNE_OP_CONSTNUM);
					expr_result_free(rv1);
					rv1 = tv;
					expr_eval_check_val(rv1, SNE_OP_CONSTNUM);
				}
				lv = expr_convert_num(
						rv0->param.num.nval + rv1->param.num.nval, SNE_OP_CONSTNUM);
			}
			goto done;
		case SNE_OP_MINUS:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			expr_eval_check_val(rv1, SNE_OP_CONSTNUM);
			lv = expr_convert_num(
					rv0->param.num.nval - rv1->param.num.nval, SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_SHL:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			expr_eval_check_val(rv1, SNE_OP_CONSTNUM);
			lv = expr_convert_num(
					to_int(rv0->param.num.nval) << to_int(rv1->param.num.nval),
					SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_SHR:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			expr_eval_check_val(rv1, SNE_OP_CONSTNUM);
			lv = expr_convert_num(
					to_int(rv0->param.num.nval) >> to_int(rv1->param.num.nval),
					SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_LT:
			expr_eval_cmp(<);
			goto done;
		case SNE_OP_LE:
			expr_eval_cmp(<=);
			goto done;
		case SNE_OP_GT:
			expr_eval_cmp(>);
			goto done;
		case SNE_OP_GE:
			expr_eval_cmp(>=);
			goto done;
		case SNE_OP_EQ:
			expr_eval_cmp(==);
			goto done;
		case SNE_OP_NE:
			expr_eval_cmp(!=);
			goto done;
		case SNE_OP_BITWISE_AND:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			expr_eval_check_val(rv1, SNE_OP_CONSTNUM);
			lv = expr_convert_num(
					to_int(rv0->param.num.nval) & to_int(rv1->param.num.nval),
					SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_BITWISE_OR:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			expr_eval_check_val(rv1, SNE_OP_CONSTNUM);
			lv = expr_convert_num(
					to_int(rv0->param.num.nval) | to_int(rv1->param.num.nval),
					SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_BITWISE_XOR:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			expr_eval_check_val(rv0, SNE_OP_CONSTNUM);
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			expr_eval_check_val(rv1, SNE_OP_CONSTNUM);
			lv = expr_convert_num(
					to_int(rv0->param.num.nval) ^ to_int(rv1->param.num.nval),
					SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_LOGICAL_AND:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			n = rv0->param.num.nval;
			if(n != 0) {
				rv1 = expr_eval(&e->param.op.args.buf[1]);
				n = rv1->param.num.nval;
				if(n != 0) {
					lv = expr_convert_num(n, SNE_OP_CONSTNUM);
					goto done;
				}
			}
			lv = expr_convert_num(0, SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_LOGICAL_OR:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			n = rv0->param.num.nval;
			if(n != 0 && !isnan(n)) {
				lv = expr_convert_num(n, SNE_OP_CONSTNUM);
				goto done;
			} else {
				rv1 = expr_eval(&e->param.op.args.buf[1]);
				n = rv1->param.num.nval;
				if(n != 0) {
					lv = expr_convert_num(n, SNE_OP_CONSTNUM);
					goto done;
				}
			}
			lv = expr_convert_num(0, SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_ASSIGN:
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			n = rv1->param.num.nval;
			if(vec_nth(&e->param.op.args, 0).type == SNE_OP_VAR) {
				e->param.op.args.buf[0].param.var.vref->v.nval = n;
			}
			lv = expr_convert_num(n, SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_COMMA:
			rv0 = expr_eval(&e->param.op.args.buf[0]);
			rv1 = expr_eval(&e->param.op.args.buf[1]);
			lv = expr_convert_num(rv1->param.num.nval, SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_CONSTNUM:
			lv = expr_convert_num(e->param.num.nval, SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_CONSTSTZ:
			lv = expr_convert_stz(e->param.stz.sval, SNE_OP_CONSTSTZ);
			goto done;
		case SNE_OP_VAR:
			lv = expr_convert_num(e->param.var.vref->v.nval, SNE_OP_CONSTNUM);
			goto done;
		case SNE_OP_FUNC:
			lv = expr_convert_num(
					e->param.func.f->f(e->param.func.f, &e->param.func.args,
							e->param.func.context),
					SNE_OP_CONSTNUM);
			goto done;
		default:
			lv = expr_convert_num(NAN, SNE_OP_CONSTNUM);
			goto done;
	}

done:
	if(rv0 != NULL) {
		expr_result_free(rv0);
	}
	if(rv1 != NULL) {
		expr_result_free(rv1);
	}
	return lv;

error:
	if(rv0 != NULL) {
		expr_result_free(rv0);
	}
	if(rv1 != NULL) {
		expr_result_free(rv1);
	}
	return NULL;

}

static int expr_next_token(const char *s, size_t len, int *flags)
{
	unsigned int i = 0;
	char b;
	if(len == 0) {
		return 0;
	}
	char c = s[0];
	if(c == '#') {
		for(; i < len && s[i] != '\n'; i++)
			;
		return i;
	} else if(c == '\n') {
		for(; i < len && isspace(s[i]); i++)
			;
		if(*flags & EXPR_TOP) {
			if(i == len || s[i] == ')') {
				*flags = *flags & (~EXPR_COMMA);
			} else {
				*flags = EXPR_TNUMBER | EXPR_TSTRING | EXPR_TWORD | EXPR_TOPEN
						 | EXPR_COMMA;
			}
		}
		return i;
	} else if(isspace(c)) {
		while(i < len && isspace(s[i]) && s[i] != '\n') {
			i++;
		}
		return i;
	} else if(isdigit(c)) {
		if((*flags & EXPR_TNUMBER) == 0) {
			return -1; // unexpected number
		}
		*flags = EXPR_TOP | EXPR_TCLOSE;
		while((c == '.' || isdigit(c)) && i < len) {
			i++;
			c = s[i];
		}
		return i;
	} else if(c == '"' || c == '\'') {
		if((*flags & EXPR_TSTRING) == 0) {
			return -1; // unexpected string
		}
		if(i == len - 1) {
			return -1; // invalud start of string
		}
		*flags = EXPR_TOP | EXPR_TCLOSE;
		b = c;
		i++;
		c = s[i];
		while(c != b && i < len) {
			i++;
			c = s[i];
		}
		return i + 1;
	} else if(isfirstvarchr(c)) {
		if((*flags & EXPR_TWORD) == 0) {
			return -2; // unexpected word
		}
		*flags = EXPR_TOP | EXPR_TOPEN | EXPR_TCLOSE;
		while((isvarchr(c)) && i < len) {
			i++;
			c = s[i];
		}
		return i;
	} else if(c == '(' || c == ')') {
		if(c == '(' && (*flags & EXPR_TOPEN) != 0) {
			*flags = EXPR_TNUMBER | EXPR_TSTRING | EXPR_TWORD | EXPR_TOPEN
					 | EXPR_TCLOSE;
		} else if(c == ')' && (*flags & EXPR_TCLOSE) != 0) {
			*flags = EXPR_TOP | EXPR_TCLOSE;
		} else {
			return -3; // unexpected parenthesis
		}
		return 1;
	} else {
		if((*flags & EXPR_TOP) == 0) {
			if(expr_op(&c, 1, 1) == SNE_OP_UNKNOWN) {
				return -4; // missing expected operand
			}
			*flags = EXPR_TNUMBER | EXPR_TSTRING | EXPR_TWORD | EXPR_TOPEN
					 | EXPR_UNARY;
			return 1;
		} else {
			int found = 0;
			while(!isvarchr(c) && !isspace(c) && c != '(' && c != ')'
					&& i < len) {
				if(expr_op(s, i + 1, 0) != SNE_OP_UNKNOWN) {
					found = 1;
				} else if(found) {
					break;
				}
				i++;
				c = s[i];
			}
			if(!found) {
				return -5; // unknown operator
			}
			*flags = EXPR_TNUMBER | EXPR_TSTRING | EXPR_TWORD | EXPR_TOPEN;
			return i;
		}
	}
}

#define EXPR_PAREN_ALLOWED 0
#define EXPR_PAREN_EXPECTED 1
#define EXPR_PAREN_FORBIDDEN 2

static int expr_bind(const char *s, size_t len, vec_expr_t *es)
{
	enum expr_type op = expr_op(s, len, -1);
	if(op == SNE_OP_UNKNOWN) {
		return -1;
	}

	if(expr_is_unary(op)) {
		if(vec_len(es) < 1) {
			return -1;
		}
		struct expr arg = vec_pop(es);
		struct expr unary = expr_init();
		unary.type = op;
		vec_push(&unary.param.op.args, arg);
		vec_push(es, unary);
	} else {
		if(vec_len(es) < 2) {
			return -1;
		}
		struct expr b = vec_pop(es);
		struct expr a = vec_pop(es);
		struct expr binary = expr_init();
		binary.type = op;
		if(op == SNE_OP_ASSIGN && a.type != SNE_OP_VAR) {
			return -1; /* Bad assignment */
		}
		vec_push(&binary.param.op.args, a);
		vec_push(&binary.param.op.args, b);
		vec_push(es, binary);
	}
	return 0;
}

static struct expr expr_constnum(float value)
{
	struct expr e = expr_init();
	e.type = SNE_OP_CONSTNUM;
	e.param.num.nval = value;
	return e;
}

static struct expr expr_varref(struct expr_var *v)
{
	struct expr e = expr_init();
	e.type = SNE_OP_VAR;
	e.param.var.vref = v;
	return e;
}

static struct expr expr_conststr(const char *value, int len)
{
	struct expr e = expr_init();
	if(len < 2) {
		len = 0;
	} else {
		/* skip the quotes */
		len -= 2;
	}
	e.type = SNE_OP_CONSTSTZ;
	e.param.stz.sval = malloc(len + 1);
	if(e.param.stz.sval) {
		if(len > 0) {
			/* do not copy the quotes */
			memcpy(e.param.stz.sval, value + 1, len);
			e.param.stz.sval[len] = '\0';
		} else {
			e.param.stz.sval[0] = '\0';
		}
	}
	return e;
}

static struct expr expr_binary(
		enum expr_type type, struct expr a, struct expr b)
{
	struct expr e = expr_init();
	e.type = type;
	vec_push(&e.param.op.args, a);
	vec_push(&e.param.op.args, b);
	return e;
}

static inline void expr_copy(struct expr *dst, struct expr *src)
{
	int i;
	struct expr arg;
	dst->type = src->type;
	if(src->type == SNE_OP_FUNC) {
		dst->param.func.f = src->param.func.f;
		vec_foreach(&src->param.func.args, arg, i)
		{
			struct expr tmp = expr_init();
			expr_copy(&tmp, &arg);
			vec_push(&dst->param.func.args, tmp);
		}
		if(src->param.func.f->ctxsz > 0) {
			dst->param.func.context = calloc(1, src->param.func.f->ctxsz);
		}
	} else if(src->type == SNE_OP_CONSTNUM) {
		dst->param.num.nval = src->param.num.nval;
	} else if(src->type == SNE_OP_VAR) {
		dst->param.var.vref = src->param.var.vref;
	} else {
		vec_foreach(&src->param.op.args, arg, i)
		{
			struct expr tmp = expr_init();
			expr_copy(&tmp, &arg);
			vec_push(&dst->param.op.args, tmp);
		}
	}
}

static void expr_destroy_args(struct expr *e);

static struct expr *expr_create(const char *s, size_t len,
		struct expr_var_list *vars, struct expr_func *funcs)
{
	float num;
	struct expr_var *v;
	const char *id = NULL;
	size_t idn = 0;

	struct expr *result = NULL;

	vec_expr_t es = vec_init();
	vec_str_t os = vec_init();
	vec_arg_t as = vec_init();

	struct macro
	{
		char *name;
		vec_expr_t body;
	};
	vec(struct macro) macros = vec_init();

	int flags = EXPR_TDEFAULT;
	int paren = EXPR_PAREN_ALLOWED;
	for(;;) {
		int n = expr_next_token(s, len, &flags);
		if(n == 0) {
			break;
		} else if(n < 0) {
			goto cleanup;
		}
		const char *tok = s;
		s = s + n;
		len = len - n;
		if(*tok == '#') {
			continue;
		}
		if(flags & EXPR_UNARY) {
			if(n == 1) {
				switch(*tok) {
					case '-':
						tok = "-u";
						break;
					case '^':
						tok = "^u";
						break;
					case '!':
						tok = "!u";
						break;
					default:
						goto cleanup;
				}
				n = 2;
			}
		}
		if(*tok == '\n' && (flags & EXPR_COMMA)) {
			flags = flags & (~EXPR_COMMA);
			n = 1;
			tok = ",";
		}
		if(isspace(*tok)) {
			continue;
		}
		int paren_next = EXPR_PAREN_ALLOWED;

		if(idn > 0) {
			if(n == 1 && *tok == '(') {
				int i;
				int has_macro = 0;
				struct macro m;
				vec_foreach(&macros, m, i)
				{
					if(strlen(m.name) == idn && strncmp(m.name, id, idn) == 0) {
						has_macro = 1;
						break;
					}
				}
				if((idn == 1 && id[0] == '$') || has_macro
						|| expr_func(funcs, id, idn) != NULL) {
					struct expr_string str = {id, (int)idn};
					vec_push(&os, str);
					paren = EXPR_PAREN_EXPECTED;
				} else {
					goto cleanup; /* invalid function name */
				}
			} else if((v = expr_var(vars, id, idn)) != NULL) {
				vec_push(&es, expr_varref(v));
				paren = EXPR_PAREN_FORBIDDEN;
			}
			id = NULL;
			idn = 0;
		}

		if(n == 1 && *tok == '(') {
			if(paren == EXPR_PAREN_EXPECTED) {
				struct expr_string str = {"{", 1};
				vec_push(&os, str);
				struct expr_arg arg = {vec_len(&os), vec_len(&es), vec_init()};
				vec_push(&as, arg);
			} else if(paren == EXPR_PAREN_ALLOWED) {
				struct expr_string str = {"(", 1};
				vec_push(&os, str);
			} else {
				goto cleanup; // Bad call
			}
		} else if(paren == EXPR_PAREN_EXPECTED) {
			goto cleanup; // Bad call
		} else if(n == 1 && *tok == ')') {
			int minlen = (vec_len(&as) > 0 ? vec_peek(&as).oslen : 0);
			while(vec_len(&os) > minlen && *vec_peek(&os).s != '('
					&& *vec_peek(&os).s != '{') {
				struct expr_string str = vec_pop(&os);
				if(expr_bind(str.s, str.n, &es) == -1) {
					goto cleanup;
				}
			}
			if(vec_len(&os) == 0) {
				goto cleanup; // Bad parens
			}
			struct expr_string str = vec_pop(&os);
			if(str.n == 1 && *str.s == '{') {
				str = vec_pop(&os);
				struct expr_arg arg = vec_pop(&as);
				if(vec_len(&es) > arg.eslen) {
					vec_push(&arg.args, vec_pop(&es));
				}
				if(str.n == 1 && str.s[0] == '$') {
					if(vec_len(&arg.args) < 1) {
						vec_free(&arg.args);
						goto cleanup; /* too few arguments for $() function */
					}
					struct expr *u = &vec_nth(&arg.args, 0);
					if(u->type != SNE_OP_VAR) {
						vec_free(&arg.args);
						goto cleanup; /* first argument is not a variable */
					}
					for(struct expr_var *v = vars->head; v; v = v->next) {
						if(v == u->param.var.vref) {
							struct macro m = {v->name, arg.args};
							vec_push(&macros, m);
							break;
						}
					}
					vec_push(&es, expr_constnum(0));
				} else {
					int i = 0;
					int found = -1;
					struct macro m;
					vec_foreach(&macros, m, i)
					{
						if(strlen(m.name) == (size_t)str.n
								&& strncmp(m.name, str.s, str.n) == 0) {
							found = i;
						}
					}
					if(found != -1) {
						m = vec_nth(&macros, found);
						struct expr root = expr_constnum(0);
						struct expr *p = &root;
						/* Assign macro parameters */
						for(int j = 0; j < vec_len(&arg.args); j++) {
							char varname[4];
							snprintf(varname, sizeof(varname) - 1, "$%d",
									(j + 1));
							struct expr_var *v =
									expr_var(vars, varname, strlen(varname));
							struct expr ev = expr_varref(v);
							struct expr assign = expr_binary(
									SNE_OP_ASSIGN, ev, vec_nth(&arg.args, j));
							*p = expr_binary(
									SNE_OP_COMMA, assign, expr_constnum(0));
							p = &vec_nth(&p->param.op.args, 1);
						}
						/* Expand macro body */
						for(int j = 1; j < vec_len(&m.body); j++) {
							if(j < vec_len(&m.body) - 1) {
								*p = expr_binary(SNE_OP_COMMA, expr_constnum(0),
										expr_constnum(0));
								expr_copy(&vec_nth(&p->param.op.args, 0),
										&vec_nth(&m.body, j));
							} else {
								expr_copy(p, &vec_nth(&m.body, j));
							}
							p = &vec_nth(&p->param.op.args, 1);
						}
						vec_push(&es, root);
						vec_free(&arg.args);
					} else {
						struct expr_func *f = expr_func(funcs, str.s, str.n);
						struct expr bound_func = expr_init();
						bound_func.type = SNE_OP_FUNC;
						bound_func.param.func.f = f;
						bound_func.param.func.args = arg.args;
						if(f->ctxsz > 0) {
							void *p = calloc(1, f->ctxsz);
							if(p == NULL) {
								goto cleanup; /* allocation failed */
							}
							bound_func.param.func.context = p;
						}
						vec_push(&es, bound_func);
					}
				}
			}
			paren_next = EXPR_PAREN_FORBIDDEN;
		} else if(!isnan(num = expr_parse_number(tok, n))) {
			vec_push(&es, expr_constnum(num));
			paren_next = EXPR_PAREN_FORBIDDEN;
		} else if(*tok == '"' || *tok == '\'') {
			vec_push(&es, expr_conststr(tok, n));
			paren_next = EXPR_PAREN_FORBIDDEN;
		} else if(expr_op(tok, n, -1) != SNE_OP_UNKNOWN) {
			enum expr_type op = expr_op(tok, n, -1);
			struct expr_string o2 = {NULL, 0};
			if(vec_len(&os) > 0) {
				o2 = vec_peek(&os);
			}
			for(;;) {
				if(n == 1 && *tok == ',' && vec_len(&os) > 0) {
					struct expr_string str = vec_peek(&os);
					if(str.n == 1 && *str.s == '{') {
						struct expr e = vec_pop(&es);
						vec_push(&vec_peek(&as).args, e);
						break;
					}
				}
				enum expr_type type2 = expr_op(o2.s, o2.n, -1);
				if(!(type2 != SNE_OP_UNKNOWN && expr_prec(op, type2))) {
					struct expr_string str = {tok, n};
					vec_push(&os, str);
					break;
				}

				if(expr_bind(o2.s, o2.n, &es) == -1) {
					goto cleanup;
				}
				(void)vec_pop(&os);
				if(vec_len(&os) > 0) {
					o2 = vec_peek(&os);
				} else {
					o2.n = 0;
				}
			}
		} else {
			if(n > 0 && !isdigit(*tok)) {
				/* Valid identifier, a variable or a function */
				id = tok;
				idn = n;
			} else {
				goto cleanup; // Bad variable name, e.g. '2.3.4' or '4ever'
			}
		}
		paren = paren_next;
	}

	if(idn > 0) {
		vec_push(&es, expr_varref(expr_var(vars, id, idn)));
	}

	while(vec_len(&os) > 0) {
		struct expr_string rest = vec_pop(&os);
		if(rest.n == 1 && (*rest.s == '(' || *rest.s == ')')) {
			goto cleanup; // Bad paren
		}
		if(expr_bind(rest.s, rest.n, &es) == -1) {
			goto cleanup;
		}
	}

	result = (struct expr *)calloc(1, sizeof(struct expr));
	if(result != NULL) {
		if(vec_len(&es) == 0) {
			result->type = SNE_OP_CONSTNUM;
		} else {
			*result = vec_pop(&es);
		}
	}

	int i, j;
	struct macro m;
	struct expr e;
	struct expr_arg a;
cleanup:
	vec_foreach(&macros, m, i)
	{
		struct expr e;
		vec_foreach(&m.body, e, j)
		{
			expr_destroy_args(&e);
		}
		vec_free(&m.body);
	}
	vec_free(&macros);

	vec_foreach(&es, e, i)
	{
		expr_destroy_args(&e);
	}
	vec_free(&es);

	vec_foreach(&as, a, i)
	{
		vec_foreach(&a.args, e, j)
		{
			expr_destroy_args(&e);
		}
		vec_free(&a.args);
	}
	vec_free(&as);

	/*vec_foreach(&os, o, i) {vec_free(&m.body);}*/
	vec_free(&os);
	return result;
}

static void expr_destroy_args(struct expr *e)
{
	int i;
	struct expr arg;
	if(e->type == SNE_OP_FUNC) {
		vec_foreach(&e->param.func.args, arg, i)
		{
			expr_destroy_args(&arg);
		}
		vec_free(&e->param.func.args);
		if(e->param.func.context != NULL) {
			if(e->param.func.f->cleanup != NULL) {
				e->param.func.f->cleanup(
						e->param.func.f, e->param.func.context);
			}
			free(e->param.func.context);
		}
	} else if(e->type != SNE_OP_CONSTNUM && e->type != SNE_OP_VAR) {
		vec_foreach(&e->param.op.args, arg, i)
		{
			expr_destroy_args(&arg);
		}
		vec_free(&e->param.op.args);
	}
}

static void expr_destroy(struct expr *e, struct expr_var_list *vars)
{
	if(e != NULL) {
		expr_destroy_args(e);
		free(e);
	}
	if(vars != NULL) {
		for(struct expr_var *v = vars->head; v;) {
			struct expr_var *next = v->next;
			free(v);
			v = next;
		}
	}
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* EXPR_H */
