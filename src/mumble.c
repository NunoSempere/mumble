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
            puts("");
        } else {
            /* Attempt to Parse the user Input */
            mpc_result_t r;
            if (mpc_parse("<stdin>", input, Mumble, &r)) {
                /* On Success Print the AST */
                mpc_ast_print(r.output);
                mpc_ast_delete(r.output);
            } else {
                /* Otherwise Print the Error */
                mpc_err_print(r.error);
                mpc_err_delete(r.error);
            }
            // printf("Did you say \"%s\"?\n", input);
            add_history(input);
            // can't add if input is NULL
        }
        free(input);
    }

    /* Undefine and Delete our Parsers */
    mpc_cleanup(4, Number, Operator, Expr, Mumble);

    return 0;
}
