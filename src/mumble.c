#include <editline/history.h>
#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpc/mpc.h"
#define VERBOSE 1

long evaluate_unary_operation(char* op, long x)
{
    if (!strcmp(op, "+")) {
        return x;
    } else if (!strcmp(op, "-")) {
        return -x;
		}
    return 0;
}
long evaluate_operation(char* op, long x, long y)
{
    if (!strcmp(op, "+")) {
        return x + y;
    } else if (!strcmp(op, "-")) {
        return x - y;
    } else if (!strcmp(op, "*")) {
        return x * y;
    } else if (!strcmp(op, "/")) {
        return x / y;
    }
    return 0;
}

int is_ignorable(mpc_ast_t* t){
	int is_regex = !strcmp(t->tag, "regex");
	int is_parenthesis = !strcmp(t->tag, "char") && !strcmp(t->contents, "(");
	return is_regex || is_parenthesis;
}
long evaluate_ast(mpc_ast_t* t)
{
    // Base case #1: It's a number
    if (strstr(t->tag, "number")) {
        if (VERBOSE)
            printf("\nCase #1, %s", t->contents);
        return atoi(t->contents);
    }

    // Base case #2: It's a number inputted into the REPL
    // note: strcmp returns a 0 if both chars* have the same contents.
    if (t->children_num == 2 && strstr(t->children[0]->tag, "number") && !strcmp(t->children[1]->tag, "regex")) {
        if (VERBOSE)
            printf("\nCase #2, %s", t->children[0]->contents);
        return atoi(t->children[0]->contents);
    }
		
		// Base case #3: Top level parenthesis
    if (t->children_num == 2 && strstr(t->children[0]->tag, "expr|>") && !strcmp(t->children[1]->tag, "regex")) {
        if (VERBOSE)
            printf("\nCase #3, top level parenthesis");
				return evaluate_ast(t->children[0]);
    }
		
		// "Real" cases
    long x;
    char* operation;

    // Case #4: Unary operations case
    if (t->children_num == 3 && is_ignorable(t->children[0]) && strstr(t->children[1]->tag, "operator")) {
        operation = t->children[1]->contents;
        if (VERBOSE)
            printf("\nCase #4, unary operation %s", operation);
        x = evaluate_ast(t->children[2]);
        x = evaluate_unary_operation(operation, x);
    }
		
    // Case #5: Binary (or more) operations case
    if (t->children_num > 3 && is_ignorable(t->children[0]) && strstr(t->children[1]->tag, "operator")) {
        operation = t->children[1]->contents;
        if (VERBOSE)
            printf("\nCase #5, %s", operation);
        x = evaluate_ast(t->children[2]);
        int i = 3;
        while ((i < t->children_num) && strstr(t->children[i]->tag, "expr")) {
					  // note that when reaching a closing parenthesis, ^ returns false
            long y = evaluate_ast(t->children[i]);
            x = evaluate_operation(operation, x, y);
            i++;
        }
    }

    return x;
}

void print_ast(mpc_ast_t* ast, int num_tabs)
{
		char tabs[100] = "";
		for(int i=0; i<num_tabs;i++){
			strcat(tabs, "  ");
		}
    printf("\n%sTag: %s", tabs, ast->tag);
    printf("\n%sContents: %s", tabs, strcmp(ast->contents, "") ? ast->contents : "None");
    printf("\n%sNumber of children: %i", tabs, ast->children_num);
    /* Print the children */
    for (int i = 0; i < ast->children_num; i++) {
        mpc_ast_t* child_i = ast->children[i];
        printf("\n%sChild #%d", tabs, i);
				print_ast(child_i, 1);
    }
}

int main(int argc, char** argv)
{
    puts("Mumble version 0.0.2\n");
    puts("Press Ctrl+C/Ctrl+D to exit\n");

    /* Create Some Parsers */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Mumble = mpc_new("mumble");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                               \
    number   : /-?[0-9]+/ ;                             \
    operator : '+' | '-' | '*' | '/' ;                  \
    expr     : <number> | '(' <operator> <expr>+ ')' ;  \
    mumble    : /^/ <operator> <expr>+ | <expr>/$/ ;             \
  ",
        Number, Operator, Expr, Mumble);

    int loop = 1;
    while (loop) {
        char* input = readline("mumble> ");
        if (input == NULL) {
            // ^ catches Ctrl+D
            loop = 0;
        } else {
            /* Attempt to Parse the user Input */
            mpc_result_t result;
            if (mpc_parse("<stdin>", input, Mumble, &result)) {
                /* On Success Print the AST */
                // mpc_ast_print(result.output);
                /* Load AST from output */
                mpc_ast_t* ast = result.output;

                // Print AST if VERBOSE
								if(VERBOSE) print_ast(ast, 0);
								
								// Evaluate the AST
								if(VERBOSE) printf("\n\nEvaluating the AST");
                long result = evaluate_ast(ast);
                printf("\nResult: %li\n", result);
            } else {
                /* Otherwise Print the Error */
                mpc_err_print(result.error);
                mpc_err_delete(result.error);
            }
            // printf("Did you say \"%s\"?\n", input);
            add_history(input);
            // can't add if input is NULL
        }
        puts("");
        free(input);
    }

    /* Undefine and Delete our Parsers */
    mpc_cleanup(4, Number, Operator, Expr, Mumble);

    return 0;
}
