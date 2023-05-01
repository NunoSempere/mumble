#include <editline/history.h>
#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpc/mpc.h"
#define VERBOSE 1

// Types
typedef struct lispval {
    int type;
    long num;
    char* err;
    char* sym;
    int count;
    struct lispval** cell; // list of lisval*
} lispval;

enum {
    LISPVAL_NUM,
    LISPVAL_ERR,
    LISPVAL_SYM,
    LISPVAL_SEXPR
};

enum {
    LISPERR_DIV_ZERO,
    LISPERR_BAD_OP,
    LISPERR_BAD_NUM
};

// Constructors
lispval* lispval_num(long x)
{
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_NUM;
    v->num = x;
    return v;
}

lispval* lispval_err(char* message)
{
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_ERR;
    v->err = malloc(strlen(message) + 1);
    strcpy(v->err, message);
    return v;
}

lispval* lispval_sym(char* symbol)
{
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_SYM;
    v->sym = malloc(strlen(symbol) + 1);
    strcpy(v->sym, symbol);
    return v;
}

lispval* lispval_sexpr(void)
{
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

// Destructor
void delete_lispval(lispval* v)
{
    switch (v->type) {
    case LISPVAL_NUM:
        break;
    case LISPVAL_ERR:
        free(v->err);
        break;
    case LISPVAL_SYM:
        free(v->sym);
        break;
    case LISPVAL_SEXPR:
        for (int i = 0; i < v->count; i++) {
            delete_lispval(v->cell[i]);
        }
        free(v->cell);
        break;
    }
    free(v);
}

// Read ast into a lispval object
lispval* lispval_append_child(lispval* parent, lispval* child)
{
    parent->count = parent->count + 1;
    parent->cell = realloc(parent->cell, sizeof(lispval) * parent->count);
    parent->cell[parent->count - 1] = child;
    return parent;
}
lispval* read_lispval_num(mpc_ast_t* t)
{
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lispval_num(x)
                           : lispval_err("Error: Invalid number.");
}
lispval* read_lispval(mpc_ast_t* t)
{
    if (strstr(t->tag, "number")) {
        return read_lispval_num(t);
    } else if (strstr(t->tag, "symbol")) {
        return lispval_sym(t->contents);
    } else if ((strcmp(t->tag, ">") == 0) || strstr(t->tag, "sexpr")) {
        lispval* x = lispval_sexpr();
        for (int i = 0; i < (t->children_num); i++) {
            if (strcmp(t->children[i]->contents, "(") == 0) {
                continue;
            }
            if (strcmp(t->children[i]->contents, ")") == 0) {
                continue;
            }
            if (strcmp(t->children[i]->tag, "regex") == 0) {
                continue;
            }
            x = lispval_append_child(x, read_lispval(t->children[i]));
        }
        return x;
    } else {
        lispval* err = lispval_err("Unknown ast type.");
        return err;
    }
}

// Print
void print_lispval_tree(lispval* v, int indent_level)
{
    char* indent = malloc(sizeof(char) * (indent_level + 1)); // "";
    for (int i = 0; i < indent_level; i++) {
        indent[i] = ' ';
    }
    indent[indent_level] = '\0';

    switch (v->type) {
    case LISPVAL_NUM:
        printf("\n%sNumber: %li", indent, v->num);
        break;
    case LISPVAL_ERR:
        printf("\n%sError: %s", indent, v->err);
        break;
    case LISPVAL_SYM:
        printf("\n%sSymbol: %s", indent, v->sym);
        break;
    case LISPVAL_SEXPR:
        printf("\n%sSExpr, with %d children:", indent, v->count);
        for (int i = 0; i < v->count; i++) {
            print_lispval_tree(v->cell[i], indent_level + 2);
        }
        break;
    default:
        printf("Error: unknown lispval type\n");
        printf("%s", v->sym);
    }
    free(indent);
}

void print_lispval_parenthesis(lispval* v)
{
    switch (v->type) {
    case LISPVAL_NUM:
        printf("%li ", v->num);
        break;
    case LISPVAL_ERR:
        printf("[Error: %s] ", v->err);
        break;
    case LISPVAL_SYM:
        printf("%s ", v->sym);
        break;
    case LISPVAL_SEXPR:
        printf("( ");
        for (int i = 0; i < v->count; i++) {
            print_lispval_parenthesis(v->cell[i]);
        }
        printf(") ");
        break;
    default:
        printf("Error: unknown lispval type\n");
        printf("%s", v->sym);
    }
}

void print_ast(mpc_ast_t* ast, int indent_level)
{
    char* indent = malloc(sizeof(char) * (indent_level + 1)); // "";
    for (int i = 0; i < indent_level; i++) {
        indent[i] = ' ';
    }
    indent[indent_level] = '\0';
    printf("\n%sTag: %s", indent, ast->tag);
    printf("\n%sContents: %s", indent,
        strcmp(ast->contents, "") ? ast->contents : "None");
    printf("\n%sNumber of children: %i", indent, ast->children_num);
    /* Print the children */
    for (int i = 0; i < ast->children_num; i++) {
        mpc_ast_t* child_i = ast->children[i];
        printf("\n%sChild #%d", indent, i);
        print_ast(child_i, indent_level + 2);
    }
    free(indent);
}

// Evaluate the lisval
lispval* pop_lispval(lispval* v, int i)
{
	lispval* r = v->cell[i];
  /* Shift memory after the item at "i" over the top */
  memmove(&v->cell[i], &v->cell[i+1],
    sizeof(lispval*) * (v->count-i-1));

  /* Decrease the count of items in the list */
  v->count--;

  /* Reallocate the memory used */
  v->cell = realloc(v->cell, sizeof(lispval*) * v->count);
  return r;
}

lispval* take_lispval(lispval* v, int i){
	lispval* x = pop_lispval(v, i);
	delete_lispval(v);
	return x;
}

lispval* evaluate_lispval(lispval* l)
{
  // Evaluate the children
	for(int i=0; i<l->count; i++){
		l->cell[i] = evaluate_lispval(l->cell[i]);
		
		// Check if any are errors.
		if(l->cell[i]->type == LISPVAL_ERR){ 
			char* msg = l->cell[i]->err; // lispval_take
			lispval* l2 = lispval_err(msg);
			delete_lispval(l);
			return l2;
		}
	}


	return l;
}
// Main
int main(int argc, char** argv)
{
    // Info
    puts("Mumble version 0.0.2\n");
    puts("Press Ctrl+C/Ctrl+D to exit\n");

    /* Create Some Parsers */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Mumble = mpc_new("mumble");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT, "                                       \
    number   : /-?[0-9]+/ ;                     \
    symbol : '+' | '-' | '*' | '/' ;            \
		sexpr : '(' <expr>* ')' ;                   \
    expr     : <number> | <symbol> | <sexpr> ;  \
    mumble    : /^/ <expr>* /$/ ;               \
  ",
        Number, Symbol, Sexpr, Expr, Mumble);

    // Initialize a repl
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
                if (VERBOSE)
                    print_ast(ast, 0);

                // Evaluate the AST
                // if(VERBOSE) printf("\n\nEvaluating the AST");
                // lispval result = evaluate_ast(ast);
                lispval* l = read_lispval(ast);
                if (VERBOSE)
                    printf("\n\nTree printing: ");
                print_lispval_tree(l, 0);
                if (VERBOSE)
                    printf("\nParenthesis printing: \n");
                print_lispval_parenthesis(l);
                delete_lispval(l);
            } else {
                /* Otherwise Print the Error */
                mpc_err_print(result.error);
                mpc_err_delete(result.error);
            }
            add_history(input);
            // can't add if input is NULL
        }
        puts("");
        free(input);
    }

    /* Undefine and Delete our Parsers */
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Mumble);

    return 0;
}
