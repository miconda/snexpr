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


#include "../snexpr.h"

#include <assert.h>
#include <stdio.h>
#include <sys/time.h>

static int status = 0;

static void expr_test_num(char *s, float expected)
{
	struct expr_var_list vars = {0};
	struct expr *e = expr_create(s, strlen(s), &vars, NULL);
	if(e == NULL) {
		printf("FAIL: %s returned NULL\n", s);
		status = 1;
		return;
	}
	struct expr *result = expr_eval(e);

	if(result==NULL) {
		printf("FAIL: result is NULL\n");
		goto end;
	}

	if(result->type != OP_CONSTNUM) {
		printf("FAIL: result is not a number (%d)\n", result->type);
		goto done;
	}

	char *p = (char *)malloc(strlen(s) + 1);
	strncpy(p, s, strlen(s) + 1);
	for(char *it = p; *it; it++) {
		if(*it == '\n') {
			*it = '\\';
		}
	}

	if((isnan(result->param.num.nval) && !isnan(expected))
			|| fabs(result->param.num.nval - expected) > 0.00001f) {
		printf("FAIL: %s: %f \t\t!= %f\n", p, result->param.num.nval, expected);
		status = 1;
	} else {
		printf("OK: %s \t\t== %f\n", p, expected);
	}

done:
	expr_result_free(result);

end:
	expr_destroy(e, &vars);
	free(p);
}


static void expr_test_stz(char *s, char *expected)
{
	struct expr_var_list vars = {0};
	struct expr *e = expr_create(s, strlen(s), &vars, NULL);
	if(e == NULL) {
		printf("FAIL: %s returned NULL\n", s);
		status = 1;
		return;
	}
	struct expr *result = expr_eval(e);

	if(result==NULL) {
		printf("FAIL: result is NULL\n");
		goto end;
	}

	if(result->type != OP_CONSTSTZ || result->param.stz.sval == NULL) {
		printf("FAIL: result is not a string (%d/%p)\n",
				result->type, result->param.stz.sval);
		goto done;
	}

	char *p = (char *)malloc(strlen(s) + 1);
	strncpy(p, s, strlen(s) + 1);
	for(char *it = p; *it; it++) {
		if(*it == '\n') {
			*it = '\\';
		}
	}

	if(strlen(result->param.stz.sval)==strlen(expected)
			&& strcmp(result->param.stz.sval, expected)==0) {
		printf("OK: %s \t\t== \"%s\"\n", p, expected);
	} else {
		printf("FAIL: %s: \"%s\" \t\t!= \"%s\"\n", p, result->param.stz.sval, expected);
		status = 1;
	}

done:
	expr_result_free(result);

end:
	expr_destroy(e, &vars);
	free(p);
}


int main(int argc, char *argv[])
{
	expr_test_num("1+\"2\"", 1 + 2);
	expr_test_num("10-2", 10 - 2);
	expr_test_num("2*3", 2 * 3);
	expr_test_num("2+3*4", 2 + 3 * 4);
	expr_test_num("(2+3)*4", (2 + 3) * 4);
	expr_test_num("2*3+4", 2 * 3 + 4);
	expr_test_num("2+3/2", 2 + 3.0 / 2.0);
	expr_test_num("1/3*6/4*2", 1.0 / 3 * 6 / 4.0 * 2);
	expr_test_num("1*3/6*4/2", 1.0 * 3 / 6 * 4.0 / 2.0);
	expr_test_num("(1+2)*3", (1 + 2) * 3);

	printf("\n");

	expr_test_stz("\"1\"+\"2\"", "12");
	expr_test_stz("\"3\"+4", "34");

	printf("\n");

	expr_test_num("\"1\" == \"2\"", 0);
	expr_test_num("(\"abc\" == \"abc\")", 1);

	return 0;
}
