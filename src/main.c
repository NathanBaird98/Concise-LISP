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
typedef struct lval {
	int type;
	long num;
	// Error and Symbol types have some string data
	char *err;
	char *sym;
	// Count and Pointer to a list of "lval pointer"
	int count;
	struct lval** cell;
} lval;

// Enumeration of possible lval Types
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, LVAL_SEXPR };

// Enumeration of possible Error Types
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* 
 * Constructs a pointer to a new Number lval
 *
 * Input: Long int
 * Return: lval Stuct Pointer (NUM type)
 */
lval *lval_num(long x) {
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

/*
 * Constructs a pointer to a new Error lval
 * 
 * Input: char Pointer
 * Return: lval Struct Pointer (ERR type)
 */
lval *lval_err(char *m) {
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);
	return v;
}

/*
 * Constructs a pointer to a new Symbol lval
 *
 * Input: char Pointer
 * Return: lval Struct Pointer (SYM type)
 */
lval *lval_sym(char *s) {
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}

/*
 * Constructs a pointer to a new Sexpr lval
 * 
 * Input: void
 * Return: lval Struct Pointer (SEXPR type)
 */
lval *lval_sexpr(void) {
	lval *v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

/*
 * 
 */
void lval_del(lval *v) {
	switch (v->type) {
		// Do nothng special for number type
		case LVAL_NUM: break;

		// For ERR or SYM free the string data
		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;
	
		// If SEXPR then delete all elements inside
		case LVAL_SEXPR:
			for (int i = 0; i < v->count; i++) {
				lval_del(v->cell[i]);
			}
			// Also free the memory allocated to contain the pointers
			free(v->cell);
		break;
	}

	// Free the memory allocated for the "lval" Struct itself
	free(v);
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

	char *op = t->children[1]->contents;
	lval x = eval(t->children[2]);

	int i = 3;
	while (strstr(t->children[i]->tag, "expr")) {
		x = eval_op(x, op, eval(t->children[i]));
		i++;
	}

	return x;
}

int main(int argc, char **argv) {
	// Create some parsers
	mpc_parser_t *Number	= mpc_new("number");
	mpc_parser_t *Symbol	= mpc_new("symbol");
	mpc_parser_t *Sexpr		= mpc_new("sexpr");
	mpc_parser_t *Expr		= mpc_new("expr");
	mpc_parser_t *Lispy		= mpc_new("lispy");

	// Define them with the following Language
	mpca_lang(MPCA_LANG_DEFAULT,
	"															\
		number		: /-?[0-9]+/ ;								\
		symbol		: '+' | '-' | '*' | '/' ;					\
		sexpr		: '(' <expr>* ')' ;							\
		expr		: <number> | <symbol> | <sexpr> ;			\
		lispy		: /^/ <expr>* /$/ ;				\
	",
	Number, Symbol, Sexpr, Expr, Lispy);

	puts("Concise-LISP Version 0.0.1");
	puts("Author: Nathan Baird");
	puts("Press Ctrl+c to Exit\n");

	while (1) {
		char *input = readline("C-LISP> ");
		add_history(input);
		
		mpc_result_t r;
		if (mpc_parse("<stdin>", input, Lispy, &r)) {
			lval result = eval(r.output);
			lval_println(result);
			mpc_ast_delete(r.output);
		} else {
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(input);
	}
	mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);
	
	return 0;
}
