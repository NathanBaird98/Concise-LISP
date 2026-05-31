#include <stdio.h>
#include <stdlib.h>

#include "../include/mpc.h"

// If we are compiling on Windows compile these functions
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

// Fake readline function
char *readline(char *prompt) {
	fputs(prompt, stdout);
	fgets(buffer, 2048, stdin);
	char *cpy = malloc(strlen(buffer) + 1);
	strcpy(cpy, buffer);
	cpy[strlen(cpy) - 1] = '\0';
	return cpy;
}

// Fake add_history function
void add_history(char *unused) {}

// Otherwise include the editline headers
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

// Lisp Value Struct
typedef struct {
	int type;
	long num;
	int err;
} lval;

// Enumeration of possible lval Types
enum { LVAL_NUM, LVAL_ERR };

// Enumeration of possible Error Types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* 
 * Creates a new number type lval
 *
 * Input: Long int
 * Return: lval Stuct (NUM type)
 */
lval lval_num(long x) {
	lval v;
	v.type = LVAL_NUM;
	v.num = x;
	return v
}

/*
 * Creates a new error type lval
 * 
 * Input: int
 * Return: lval Struct (ERR type)
 */
lval lval_err(int x) {
	lval v;
	v.type = LVAL_ERR;
	v.err = x;
	return v;
}

/*
 * Print an lval Struct
 * 
 * Input: lval Struct
 * Return: Void
 */
void lval_print(lval v) {
	switch (v.type) {
		case LVAL_NUM: printf("%li", v.num); break;

		case LVAL_ERR:
			if (v.err == LERR_DIV_ZERO) {
				printf("Error: Division by Zero!");
			}
			if (v.err == LERR_BAD_OP) {
				printf("Error: Invalid Operator!");
			}
			if (v.err == LERR_BAD_NUM) {
				printf("Error: Invalid Number!");
			}
		break;
	}
}

/*
 * Print an lval Struct followed by a newline
 * 
 * Input: lval Struct
 * Return: Void
 */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

/*
 * Recursive evaluation function. Evaluates the ast accumulating
 * the result to return as a long int.
 * 
 * Input: Abstract Syntax Tree pointer
 * Return: long int
 */
lval eval(mpc_ast_t *t) {
	if (strstr(t->tag, "number")) {
		// Check if there is some error in conversion
		errno = 0;
		long x = strtol(t->contents, NULL, 10);
		return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
	}

	char *op = t->children[i]->contents;
	lval x = eval(t->children[2]);

	int i = 3;
	while (strstre(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

/*
 * Performs the appropriate operation on two input numbers given 
 * a particular operator string.
 *
 * Input: lval Struct, char pointer, lval Struct
 * Return: lval Struct
 */
lval eval_op(lval x, char *op, lval y) {
	// If either value is an error, return it
	if (x.type == LVAL_ERR) { return x; }
	if (y.type == LVAL_ERR) { return y; }

	// Otherwise do maths on the number values
	if (strcmp(op, "+") == 0) {return lval_num(x.num + y.num); }
	if (strcmp(op, "-") == 0) {return lval_num(x.num - y.num); }
	if (strcmp(op, "*") == 0) {return lval_num(x.num * y.num); }
	if (strcmp(op, "/") == 0) {
		// If second operand is zero return error
		return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
	}

	return lval_err(LERR_BAD_OP);
}

int main(int argc, char **argv) {
	// Create some parsers
	mpc_parser_t *Number	= mpc_new("number");
	mpc_parser_t *Operator	= mpc_new("operator");
	mpc_parser_t *Expr		= mpc_new("expr");
	mpc_parser_t *Lispy		= mpc_new("lispy");

	// Define them with the following Language
	mpca_lang(MPCA_LANG_DEFAULT,
	"															\
		number		: /-?[0-9]+/ ;								\
		operator	: '+' | '-' | '*' | '/' ;					\
		expr		: <number> | '(' <operator> <expr>+ ')' ;	\
		lispy		: /^/ <operator> <expr>+ /$/ ;				\
	",
	Number, Operator, Expr, Lispy);

	puts("Concise-LISP Version 0.0.1");
	puts("Author: Nathan Baird");
	puts("Press Ctrl+c to Exit\n");

	while (1) {
		char *input = readline("C-LISP> ");
		add_history(input);
		
		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			lval result = eval(r.output);
			lval_printf(result);
			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}
	mpc_cleanup(4, Number, Operator, Expr, Lispy);
	
	return 0;
}
