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


static void snexpr_test_num(char *s, float expected)
{
	char *p = NULL;
	struct snexpr_var_list vars = {0};
	struct snexpr *e = snexpr_create(s, strlen(s), &vars, NULL);
	if(e == NULL) {
		printf("FAIL: %s returned NULL\n", s);
		return;
	}
	struct snexpr *result = snexpr_eval(e);

	if(result==NULL) {
		printf("FAIL: result is NULL\n");
		goto end;
	}

	if(result->type != SNE_OP_CONSTNUM) {
		printf("FAIL: result is not a number (%d!=%d)\n", result->type,
				SNE_OP_CONSTNUM);
		goto done;
	}

	p = (char *)malloc(strlen(s) + 1);
	strncpy(p, s, strlen(s) + 1);
	for(char *it = p; *it; it++) {
		if(*it == '\n') {
			*it = '\\';
		}
	}

	if((isnan(result->param.num.nval) && !isnan(expected))
			|| fabs(result->param.num.nval - expected) > 0.00001f) {
		printf("FAIL: %s: %f \t\t!= %f\n", p, result->param.num.nval, expected);
	} else {
		printf("OK: %s \t\t== %f\n", p, expected);
	}

done:
	snexpr_result_free(result);

end:
	snexpr_destroy(e, &vars);
	if(p) free(p);
}


static void snexpr_test_stz(char *s, char *expected)
{
	char *p = NULL;
	struct snexpr_var_list vars = {0};
	struct snexpr *e = snexpr_create(s, strlen(s), &vars, NULL);
	if(e == NULL) {
		printf("FAIL: %s returned NULL\n", s);
		return;
	}
	struct snexpr *result = snexpr_eval(e);

	if(result==NULL) {
		printf("FAIL: result is NULL\n");
		goto end;
	}

	if(result->type != SNE_OP_CONSTSTZ || result->param.stz.sval == NULL) {
		printf("FAIL: result is not a string (%d!=%d/%p)\n",
				result->type, SNE_OP_CONSTSTZ, result->param.stz.sval);
		goto done;
	}

	p = (char *)malloc(strlen(s) + 1);
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
	}

done:
	snexpr_result_free(result);

end:
	snexpr_destroy(e, &vars);
	if(p) free(p);
}


static void snexpr_test_bool(char *s, int expected)
{
	char *p = NULL;
	int b = 0;
	struct snexpr_var_list vars = {0};
	struct snexpr *e = snexpr_create(s, strlen(s), &vars, NULL);
	if(e == NULL) {
		printf("FAIL: %s returned NULL\n", s);
		return;
	}
	struct snexpr *result = snexpr_eval(e);

	if(result==NULL) {
		printf("FAIL: result is NULL\n");
		goto end;
	}

	p = (char *)malloc(strlen(s) + 1);
	strncpy(p, s, strlen(s) + 1);
	for(char *it = p; *it; it++) {
		if(*it == '\n') {
			*it = '\\';
		}
	}

	if(expected) {
		expected = 1;
	} else {
		expected = 0;
	}

	if(result->type == SNE_OP_CONSTNUM) {
		if(result->param.num.nval) {
			b = 1;
		} else {
			b = 0;
		}
	} else if(result->type == SNE_OP_CONSTSTZ) {
		if(result->param.stz.sval==NULL || strlen(result->param.stz.sval)==0) {
			b = 0;
		} else {
			b = 1;
		}
	} else {
		printf("FAIL: result is UNKNOWN\n");
		goto done;
	}

	if(b == expected) {
		printf("OK: %s \t\t== %s\n", p, expected?"true":"false");
	} else {
		printf("FAIL: %s: %f \t\t!= %s\n", p, result->param.num.nval, expected?"true":"false");
	}

done:
	snexpr_result_free(result);

end:
	snexpr_destroy(e, &vars);
	if(p) free(p);
}


int main(int argc, char *argv[])
{
	snexpr_test_num("1+\"2\"", 1 + 2);
	snexpr_test_num("10-2", 10 - 2);
	snexpr_test_num("2*3", 2 * 3);
	snexpr_test_num("2+3*4", 2 + 3 * 4);
	snexpr_test_num("(2+3)*4", (2 + 3) * 4);
	snexpr_test_num("2*3+4", 2 * 3 + 4);
	snexpr_test_num("2+3/2", 2 + 3.0 / 2.0);
	snexpr_test_num("1/3*6/4*2", 1.0 / 3 * 6 / 4.0 * 2);
	snexpr_test_num("1*3/6*4/2", 1.0 * 3 / 6 * 4.0 / 2.0);
	snexpr_test_num("(1+2)*3", (1 + 2) * 3);

	printf("\n");

	snexpr_test_stz("\"1\"+\"2\"", "12");
	snexpr_test_stz("\"3\"+4", "34");
	snexpr_test_stz("s=\"4\",s=s+\"5\"", "45");

	printf("\n");

	snexpr_test_bool("\"1\" == \"2\"", 0);
	snexpr_test_bool("\"12\" == \"1\" + 2", 1);
	snexpr_test_bool("(\"abc\" == \"abc\")", 1);

	return 0;
}
