#include <editline/history.h>
#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpc/mpc.h"
#define VERBOSE 0
#define LISPVAL_ASSERT(cond, err) \
	if (!(cond)) { return lispval_err(err); }

// Types
typedef struct lispval {
    int type;
    double num;
    char* err;
    char* sym;
    int count;
    struct lispval** cell; // list of lisval*
} lispval;

enum {
    LISPVAL_NUM,
    LISPVAL_ERR,
    LISPVAL_SYM,
    LISPVAL_SEXPR,
		LISPVAL_QEXPR,
};

enum {
    LISPERR_DIV_ZERO,
    LISPERR_BAD_OP,
    LISPERR_BAD_NUM
};

// Constructors
lispval* lispval_num(double x)
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

lispval* lispval_qexpr(void)
{
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_QEXPR;
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
    case LISPVAL_QEXPR:
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
    double x = strtod(t->contents, NULL);
    return errno != ERANGE ? lispval_num(x)
                           : lispval_err("Error: Invalid number.");
}

lispval* read_lispval(mpc_ast_t* t)
{
		// Non-ignorable children
		// Relevant for the edge-case of considering the case where you 
		// only have one top level item.
	  int c = 0;
		int c_index = -1;
		for(int i=0; i<t->children_num; i++){
      mpc_ast_t* child = t->children[i];
			if( ( strcmp(child->tag, "regex") != 0 ) || (strcmp(child->contents, "") != 0 ) || child->children_num != 0 ){
				c++;
				c_index = i;
			}
		}
		if(VERBOSE) printf("\nNon ignorable children: %i", c);

    if (strstr(t->tag, "number")) {
        return read_lispval_num(t);
    } else if (strstr(t->tag, "symbol")) {
        return lispval_sym(t->contents);
		} else if ((strcmp(t->tag, ">") == 0) && (c==1)) {
			return read_lispval(t->children[c_index]);
    } else if ((strcmp(t->tag, ">") == 0) || strstr(t->tag, "sexpr") || strstr(t->tag, "qexpr")) {
        lispval* x;
				if((strcmp(t->tag, ">") == 0) || strstr(t->tag, "sexpr")){
					x = lispval_sexpr();
				} else if(strstr(t->tag, "qexpr")){
					x = lispval_qexpr();
				} else {
					return lispval_err("Error: Unreachable code state reached.");
				}

        for (int i = 0; i < (t->children_num); i++) {
            if (strcmp(t->children[i]->contents, "(") == 0) {
                continue;
            } else if (strcmp(t->children[i]->contents, ")") == 0) {
                continue;
            } else if (strcmp(t->children[i]->contents, "{") == 0) {
                continue;
            }
            else if (strcmp(t->children[i]->contents, "}") == 0) {
                continue;
            }
            else if (strcmp(t->children[i]->tag, "regex") == 0) {
                continue;
            } else {
            x = lispval_append_child(x, read_lispval(t->children[i])); 
						}
        }
        return x;
		} else {
        lispval* err = lispval_err("Unknown AST type.");
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
        printf("\n%sNumber: %f", indent, v->num);
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
    case LISPVAL_QEXPR:
        printf("\n%sQExpr, with %d children:", indent, v->count);
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
        printf("%f ", v->num);
        break;
    case LISPVAL_ERR:
        printf("%s ", v->err);
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
    case LISPVAL_QEXPR:
        printf("{ ");
        for (int i = 0; i < v->count; i++) {
            print_lispval_parenthesis(v->cell[i]);
        }
        printf("} ");
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

// Lispval helpers
lispval* clone_lispval(lispval* old)
{
	  lispval* new;
		
	  switch(old->type){
    case LISPVAL_NUM:
			  new = lispval_num(old->num);
        break;
    case LISPVAL_ERR:
        new = lispval_err(old->err);
        break;
    case LISPVAL_SYM:
        new = lispval_sym(old->sym);
        break;
    case LISPVAL_SEXPR:
				new = lispval_sexpr();
				break; 
    case LISPVAL_QEXPR:
				new = lispval_qexpr();
        break;
		}
		
		for (int i = 0; i < old->count; i++) {
			  lispval* temp_child = old->cell[i];
				lispval* child = clone_lispval(temp_child);
				lispval_append_child(new, child);
		}
    return new;
}

lispval* pop_lispval(lispval* v, int i)
{
    lispval* r = v->cell[i];
    /* Shift memory after the item at "i" over the top */
    memmove(&v->cell[i], &v->cell[i + 1],
        sizeof(lispval*) * (v->count - i - 1));

    /* Decrease the count of items in the list */
    v->count--;

    /* Reallocate the memory used */
    v->cell = realloc(v->cell, sizeof(lispval*) * v->count);
    return r;
}

lispval* take_lispval(lispval* v, int i)
{ // Unneeded.
    lispval* x = pop_lispval(v, i);
    delete_lispval(v);
    return x;
}

// Operations
// Ops for q-expressions
lispval* builtin_head(lispval* v){
	// head { 1 2 3 }
	// But actually, that gets processd into head ({ 1 2 3 }), hence the v->cell[0]->cell[0];
	LISPVAL_ASSERT(v->count ==1, "Error: function head passed too many arguments");
	LISPVAL_ASSERT(v->cell[0]->type == LISPVAL_QEXPR, "Error: Argument passed to head is not a q-expr, i.e., a bracketed list.");
  LISPVAL_ASSERT(v->cell[0]->count != 0, "Error: Argument passed to head is {}");
  // print_lispval_parenthesis(v);
	lispval* result = clone_lispval(v->cell[0]->cell[0]); 
	// lispval* result = pop_lispval(v->cell[0], 0); 
  // ^ also possible
	// A bit unclear. Pop seems like it would depend on the size of the array. clone depends on the sie of head.
	// either way the original array will soon be deleted, so I could have used pop
	// but I wanted to write & use clone instead.
	return result;
	// Returns something that should be freed later: yes.
  // Returns something that is independent of the input: yes.
}

lispval* builtin_tail(lispval* v)
{
	// tail { 1 2 3 }
	LISPVAL_ASSERT(v->count ==1, "Error: function tail passed too many arguments");
	
	lispval* old = v->cell[0];
	LISPVAL_ASSERT(old->type == LISPVAL_QEXPR, "Error: Argument passed to tail is not a q-expr, i.e., a bracketed list.");
  LISPVAL_ASSERT(old->count != 0, "Error: Argument passed to tail is {}");
	
	// lispval* head = pop_lispval(v->cell[0], 0);
  // print_lispval_parenthesis(v);
  // print_lispval_parenthesis(old);
	lispval* new = lispval_qexpr();
	if(old->count == 1){ 
		return new; 
  } else {
		for(int i=1; i<(old->count); i++){
			// lispval_append_child(new, clone_lispval(old->cell[i]));
			lispval_append_child(new, old->cell[i]);
		} 
	}
	
	return clone_lispval(new);
	// Returns something that should be freed later: yes.
  // Returns something that is independent of the input: yes.
}

lispval* builtin_list(lispval* v){
  // list ( 1 2 3 )
	LISPVAL_ASSERT(v->count ==1, "Error: function list passed too many arguments");
  LISPVAL_ASSERT(v->cell[0]->type == LISPVAL_QEXPR, "Error: Argument passed to list is not a q-expr, i.e., a bracketed list.");
  v->type=LISPVAL_QEXPR;
	return v;
}

lispval* evaluate_lispval(lispval* l);
lispval* builtin_eval(lispval* v){
  // eval { + 1 2 3 }
	// not sure how this will end up working, but we'll see
	LISPVAL_ASSERT(v->count ==1, "Error: function eval passed too many arguments");
  LISPVAL_ASSERT(v->cell[0]->type == LISPVAL_QEXPR, "Error: Argument passed to eval is not a q-expr, i.e., a bracketed list.");
  v->type=LISPVAL_SEXPR;
	return evaluate_lispval(v);
}

lispval* builtin_join(lispval* l){
	return lispval_err("Error: Join not ready yet.");
	// join { {1 2} {3 4} }
  LISPVAL_ASSERT(l->type == LISPVAL_QEXPR, "Error: function join not passed q-expression");
	lispval* result = lispval_qexpr();
	for(int i=0; i<l->count; i++){
		lispval* temp = l->cell[i];
		LISPVAL_ASSERT(temp->type == LISPVAL_QEXPR, "Error: function join not passed a q expression with other q-expressions");

		for(int j=0; j<temp->count; j++){
			lispval_append_child(result, temp->cell[j]);
		}
	}
	return result;
}

// Simple math ops
lispval* builtin_math_ops(char* op, lispval* v)
{
    // For now, ensure all args are numbers
    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type != LISPVAL_NUM) {
            return lispval_err("Error: Operating on non-numbers. This can be caused by an input like (+ 1 2 (3 * 4)). Because the (3 * 4) doesn't have the correct operation order, it isn't simplified, and then + can't sum over it.");
        }
    }
    // Check how many elements
    if (v->count == 0) {
        return lispval_err("Error: No numbers on which to operate!");
    } else if (v->count == 1) {
        if (strcmp(op, "-") == 0) {
            return lispval_num(-v->cell[0]->num);
        } else {
            return lispval_err("Error: Non minus unary operation");
        }
    } else if (v->count >= 2) {
        lispval* x = pop_lispval(v, 0);

        while (v->count > 0) {
            // Pop the next element
            lispval* y = pop_lispval(v, 0);

            if (strcmp(op, "+") == 0) {
                x->num += y->num;
            }
            if (strcmp(op, "-") == 0) {
                x->num -= y->num;
            }
            if (strcmp(op, "*") == 0) {
                x->num *= y->num;
            }

            if (strcmp(op, "/") == 0) {
                if (y->num == 0) {
                    delete_lispval(x);
                    delete_lispval(y);
                    return lispval_err("Error: Division By Zero!");
                }
                x->num /= y->num;
            }

            delete_lispval(y);
        }
        return x;
    } else {
        return lispval_err("Error: Incorrect number of args. Perhaps a lispval->count was wrongly initialized?");
    }
}

// Aggregate both math and operations over lists
lispval* builtin_functions(char* func, lispval* v)
{
  if (strcmp("list", func) == 0) { return builtin_list(v); }
  else if (strcmp("head", func) == 0) { return builtin_head(v); }
  else if (strcmp("tail", func) == 0) { return builtin_tail(v); }
  // else if (strcmp("j", func) == 0) { return builtin_join(v); }
  else if (strcmp("eval", func) == 0) { return builtin_eval(v); }
  else if (strstr("+-/*", func)) { return builtin_math_ops(func, v); 
  } else {
		return lispval_err("Unknown function");
	}
}

// Evaluate the lispval
lispval* evaluate_lispval(lispval* l)
{
    // Evaluate the children if needed
    for (int i = 0; i < l->count; i++) {
        if (l->cell[i]->type == LISPVAL_SEXPR) {
            l->cell[i] = evaluate_lispval(l->cell[i]);
        }
    }
    // Check if any are errors.
    for (int i = 0; i < l->count; i++) {
        if (l->cell[i]->type == LISPVAL_ERR) {
            return pop_lispval(l, i);
        }
    }

    // Check if the first element is an operation.
    if (l->count >= 2 && ((l->cell[0])->type == LISPVAL_SYM)) {
        lispval* op = pop_lispval(l, 0);
        lispval* result = builtin_functions(op->sym, l);
        delete_lispval(op);
        return result;
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
    mpc_parser_t* Qexpr = mpc_new("qexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Mumble = mpc_new("mumble");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT, "                           \
    number   : /-?[0-9]+\\.?([0-9]+)?/ ;                     \
    symbol : \"list\" | \"head\" | \"tail\" | \"eval\" \
           | '+' | '-' | '*' | '/' ;        \
		sexpr : '(' <expr>* ')' ;                                \
		qexpr : '{' <expr>* '}' ;                                \
    expr     : <number> | <symbol> | <sexpr> | <qexpr>;      \
    mumble    : /^/ <expr>* /$/ ;                            \
  ",
        Number, Symbol, Sexpr, Qexpr, Expr, Mumble);

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
                if (VERBOSE) {
                    printf("\nPrinting AST");
                    print_ast(ast, 0);
                }
                // Evaluate the AST
                // if(VERBOSE) printf("\n\nEvaluating the AST");
                // lispval result = evaluate_ast(ast);
                lispval* l = read_lispval(ast);
                if (VERBOSE) {
                    printf("\n\nPrinting initially parsed lispvalue");
                    printf("\nTree printing: ");
                    print_lispval_tree(l, 2);
                    printf("\nParenthesis printing: ");
                    print_lispval_parenthesis(l);
                }
                lispval* result = evaluate_lispval(l);
                {
                    printf("\n\nResult: ");
                    print_lispval_parenthesis(result);
                    printf("\n");
                }
                delete_lispval(l);
								// delete_lispval(result);
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
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Mumble);

    return 0;
}
