#include <editline/history.h>
#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpc/mpc.h"

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
								char* no_contents = "None";
								printf("\n");
                printf("Tag: %s\n", ast->tag);
                printf("Contents: %s\n", strcmp(ast->contents, "") ? ast->contents : no_contents);
                printf("Number of children: %i\n", ast->children_num);
								/* Print the children */
								for(int i=0; i < ast->children_num; i++){
									mpc_ast_t* child_i = ast->children[i];
									printf("Child #%d\n", i);
									printf("\tTag: %s\n", child_i->tag);
									printf("\tContents: %s\n", strcmp(child_i->contents, "") ? child_i->contents : no_contents);
									printf("\tNumber of children: %i\n", child_i->children_num);
								}
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
