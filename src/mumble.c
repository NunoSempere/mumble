#include <editline/history.h>
#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpc/mpc.h"
#define LISPVAL_ASSERT(cond, err) \
    if (!(cond)) {                \
        return lispval_err(err);  \
    }
int VERBOSE = 2;
#define printfln(...) do { \
    if(VERBOSE == 2) { \
			printf ("\n@ %s (%d): ", __FILE__, __LINE__); \
      printf (__VA_ARGS__); \
		} else {\
			printf("\n"); \
      printf (__VA_ARGS__); \
		} \
} while (0)
// Types

// Types: Forward declarations
// I don't understand how this works
// and in particular why lispval is repeated twice after the typedef struct
// See: <https://buildyourownlisp.com/chapter11_variables>
// <https://web.archive.org/web/20230226023546/https://buildyourownlisp.com/chapter11_variables>
struct lispval;
struct lispenv;
typedef struct lispval lispval;
typedef struct lispenv lispenv;

typedef lispval*(*lispbuiltin)(lispval*, lispenv*); 
// this defines the lispbuiltin type
// which seems to be a pointer to a function which takes in a lispenv*
// and a lispval* and returns a lispval*

// Types: Actual types

enum {
    LISPVAL_NUM,
    LISPVAL_ERR,
    LISPVAL_SYM,
    LISPVAL_FUNC,
    LISPVAL_SEXPR,
    LISPVAL_QEXPR,
};

typedef struct lispval {
    int type;
    double num;
    char* err;
    char* sym;
		lispbuiltin func;
		char* funcname;
    int count;
    struct lispval** cell; // list of lisval*
} lispval;


enum {
    LISPERR_DIV_ZERO,
    LISPERR_BAD_OP,
    LISPERR_BAD_NUM
};

// Constructors
lispval* lispval_num(double x)
{
    if(VERBOSE) printfln("Allocated num");
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_NUM;
    v->count = 0;
    v->num = x;
    return v;
}

lispval* lispval_err(char* message)
{
    if(VERBOSE) printfln("Allocated err");
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_ERR;
    v->count = 0;
    v->err = malloc(strlen(message) + 1);
    strcpy(v->err, message);
    return v;
}

lispval* lispval_sym(char* symbol)
{
    if(VERBOSE) printfln("Allocated sym");
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_SYM;
    v->count = 0;
    v->sym = malloc(strlen(symbol) + 1);
    strcpy(v->sym, symbol);
    return v;
}

lispval* lispval_func(lispbuiltin func, char* funcname){
    if(VERBOSE) printfln("Allocating func name:%s, pointer: %p", funcname, func);
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_FUNC;
    v->count = 0;
    v->funcname = malloc(strlen(funcname) + 1);
    strcpy(v->funcname, funcname);
		v->func = func;
    if(VERBOSE) printfln("Allocated func");
    return v;
}

lispval* lispval_sexpr(void)
{
    if(VERBOSE) printfln("Allocated sexpr");
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lispval* lispval_qexpr(void)
{
    if(VERBOSE) printfln("Allocated qexpr");
    lispval* v = malloc(sizeof(lispval));
    v->type = LISPVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

// Destructor
void print_lispval_tree(lispval* v, int indent_level);
void delete_lispval(lispval* v)
{
	  if(v == NULL) return;
		// print_lispval_tree(v, 0);
		if(VERBOSE) printfln("\nDeleting object of type %i",v->type);
    switch (v->type) {
    case LISPVAL_NUM:
        if(VERBOSE) printfln("Freeing num");
				if (v != NULL) free(v);
        if(VERBOSE) printfln("Freed num");
        break;
    case LISPVAL_ERR:
        if(VERBOSE) printfln("Freeing err");
        if (v->err != NULL)
            free(v->err);
        v->err = NULL;
				if (v != NULL) free(v);
        if(VERBOSE) printfln("Freed err");
        break;
    case LISPVAL_SYM:
        if(VERBOSE) printfln("Freeing sym");
        if (v->sym != NULL)
            free(v->sym);
        v->sym = NULL;
				if (v != NULL) free(v);
        if(VERBOSE) printfln("Freed sym");
        break;
		case LISPVAL_FUNC:
        if(VERBOSE) printfln("Freeing func");
        if (v->funcname != NULL)
            free(v->funcname);
        v->funcname = NULL;
				if (v != NULL) free(v);
        if(VERBOSE) printfln("Freed func");
				// Don't do anything with v->func for now
				// Though we could delete the pointer to the function later
				// free(v->func);
				break;
    case LISPVAL_SEXPR:
    case LISPVAL_QEXPR:
        if(VERBOSE) printfln("Freeing sexpr|qexpr");
				if(v==NULL || v->count !=0) return;
        for (int i = 0; i < v->count; i++) {
            if (v->cell[i] != NULL)
                delete_lispval(v->cell[i]);
            v->cell[i] = NULL;
        }
				v->count = 0;
        if (v->cell != NULL)
            free(v->cell);
        v->cell = NULL;
				if (v != NULL) free(v);
        if(VERBOSE) printfln("Freed sexpr|qexpr");
        break;
		default: 
        if(VERBOSE) printfln("Error: Unknown expression type for pointer %p of type %i", v, v->type);
    }
    // v = NULL; this is only our local pointer, sadly.
}

// Environment
struct lispenv {
  int count;
  char** syms; // list of strings
  lispval** vals; // list of pointers to vals
};

lispenv* new_lispenv(){
	lispenv* n = malloc(sizeof(lispenv));
	n->count = 0;
	n->syms = NULL;
	n->vals = NULL;
	return n;
}

void destroy_lispenv(lispenv* env){
	for(int i=0; i< env->count; i++){
		free(env->syms[i]);
		free(env->vals[i]);
		env->syms[i] = NULL;
		env->vals[i] = NULL;
	}
	free(env->syms);
	env->syms = NULL;
	free(env->vals);
	env->vals = NULL;
	free(env);
	env = NULL;
}

lispval* clone_lispval(lispval* old);
lispval* get_from_lispenv(char* sym, lispenv* env){
	for(int i=0; i<env->count; i++){
		if(strcmp(env->syms[i], sym) == 0){
			return clone_lispval(env->vals[i]);
			// return env->vals[i];
		}
	}
	return lispval_err("Error: unbound symbol");
}

void insert_in_lispenv(char* sym, lispval* v, lispenv* env){
	int found = 0;
	for(int i=0; i<env->count; i++){
		if(strcmp(env->syms[i], sym) == 0){
			delete_lispval(env->vals[i]);
			env->vals[i] = clone_lispval(v);
			found = 1;
		}
	}
	if(found == 0){
		// Expand memory *for the arrays*
		env->count++;
		env->syms = realloc(env->syms, sizeof(char*) * env->count);
		env->vals = realloc(env->vals, sizeof(lispval*) * env->count);
		
		// Copy contents over
		env->vals[env->count - 1] = clone_lispval(v);
		env->syms[env->count - 1] = malloc(strlen(sym) + 1);
		strcpy(env->syms[env->count - 1], sym);
	}
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
    // only have one top level item in brackets
    int c = 0;
    int c_index = -1;
    for (int i = 0; i < t->children_num; i++) {
        mpc_ast_t* child = t->children[i];
        if ((strcmp(child->tag, "regex") != 0) || (strcmp(child->contents, "") != 0) || child->children_num != 0) {
            c++;
            c_index = i;
        }
    }
    if (VERBOSE)
        printfln("Non ignorable children: %i", c);

    if (strstr(t->tag, "number")) {
        return read_lispval_num(t);
    } else if (strstr(t->tag, "symbol")) {
        return lispval_sym(t->contents);
    } else if ((strcmp(t->tag, ">") == 0) && (c == 1)) {
        return read_lispval(t->children[c_index]);
    } else if ((strcmp(t->tag, ">") == 0) || strstr(t->tag, "sexpr") || strstr(t->tag, "qexpr")) {
        lispval* x;
        if ((strcmp(t->tag, ">") == 0) || strstr(t->tag, "sexpr")) {
            x = lispval_sexpr();
        } else if (strstr(t->tag, "qexpr")) {
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
            } else if (strcmp(t->children[i]->contents, "}") == 0) {
                continue;
            } else if (strcmp(t->children[i]->tag, "regex") == 0) {
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
    char* indent = malloc(sizeof(char) * (indent_level + 1)); 
    for (int i = 0; i < indent_level; i++) {
        indent[i] = ' ';
    }
    indent[indent_level] = '\0';

    switch (v->type) {
    case LISPVAL_NUM:
        printfln("%sNumber: %f", indent, v->num);
        break;
    case LISPVAL_ERR:
        printfln("%s%s", indent, v->err);
        break;
    case LISPVAL_SYM:
        printfln("%sSymbol: %s", indent, v->sym);
        break;
    case LISPVAL_FUNC:
        printfln("%sFunction, name: %s, pointer: %p", indent, v->funcname, v->func);
        break;
    case LISPVAL_SEXPR:
        printfln("%sSExpr, with %d children:", indent, v->count);
        for (int i = 0; i < v->count; i++) {
            print_lispval_tree(v->cell[i], indent_level + 2);
        }
        break;
    case LISPVAL_QEXPR:
        printfln("%sQExpr, with %d children:", indent, v->count);
        for (int i = 0; i < v->count; i++) {
            print_lispval_tree(v->cell[i], indent_level + 2);
        }
        break;
    default:
        printfln("Error: unknown lispval type\n");
        // printfln("%s", v->sym);
    }
		if(VERBOSE > 1) printfln("Freeing indent");
    if (indent!=NULL) free(indent);
    indent = NULL;
		if(VERBOSE > 1) printfln("Freed indent");
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
    case LISPVAL_FUNC:
        printf("<function, name: %s, pointer: %p> ", v->funcname, v->func);
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
        printfln("Error: unknown lispval type\n");
        // printfln("%s", v->sym);
    }
}

void print_ast(mpc_ast_t* ast, int indent_level)
{
    char* indent = malloc(sizeof(char) * (indent_level + 1)); // "";
    for (int i = 0; i < indent_level; i++) {
        indent[i] = ' ';
    }
    indent[indent_level] = '\0';
    printfln("%sTag: %s", indent, ast->tag);
    printfln("%sContents: %s", indent,
        strcmp(ast->contents, "") ? ast->contents : "None");
    printfln("%sNumber of children: %i", indent, ast->children_num);
    /* Print the children */
    for (int i = 0; i < ast->children_num; i++) {
        mpc_ast_t* child_i = ast->children[i];
        printfln("%sChild #%d", indent, i);
        print_ast(child_i, indent_level + 2);
    }
    free(indent);
    indent = NULL;
}

// Lispval helpers
lispval* clone_lispval(lispval* old)
{
    lispval* new;
    switch (old->type) {
    case LISPVAL_NUM:
        new = lispval_num(old->num);
        break;
    case LISPVAL_ERR:
        new = lispval_err(old->err);
        break;
    case LISPVAL_SYM:
        new = lispval_sym(old->sym);
        break;
    case LISPVAL_FUNC:
        new = lispval_func(old->func, old->funcname);
        break;
    case LISPVAL_SEXPR:
        new = lispval_sexpr();
        break;
    case LISPVAL_QEXPR:
        new = lispval_qexpr();
        break;
    default:
        return lispval_err("Error: Cloning element of unknown type.");
    }

    if (old->count > 0 && (old->type == LISPVAL_QEXPR || old->type == LISPVAL_SEXPR)) {
        for (int i = 0; i < old->count; i++) {
            lispval* temp_child = old->cell[i];
            lispval* child = clone_lispval(temp_child);
            lispval_append_child(new, child);
        }
    }
    return new;
}

lispval* pop_lispval(lispval* v, int i)
{
    LISPVAL_ASSERT(v->type == LISPVAL_QEXPR || v->type == LISPVAL_SEXPR, "Error: function pop wasn't passed a q-expression or an s-expression arguments");
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
    LISPVAL_ASSERT(v->type == LISPVAL_QEXPR || v->type == LISPVAL_SEXPR, "Error: function take_lispval passed too many arguments");
    lispval* x = pop_lispval(v, i);
    delete_lispval(v);
    return x;
}

// Operations
// Ops for q-expressions
lispval* builtin_head(lispval* v, lispenv* e)
{
    // printfln("Entering builtin_head with v->count = %d and v->cell[0]->type = %d\n", v->count, v->cell[0]->type);
    // head { 1 2 3 }
    // But actually, that gets processd into head ({ 1 2 3 }), hence the v->cell[0]->cell[0];
    LISPVAL_ASSERT(v->count == 1, "Error: function head passed too many arguments");
    LISPVAL_ASSERT(v->cell[0]->type == LISPVAL_QEXPR, "Error: Argument passed to head is not a q-expr, i.e., a bracketed list.");
    LISPVAL_ASSERT(v->cell[0]->count != 0, "Error: Argument passed to head is {}");
    lispval* result = clone_lispval(v->cell[0]->cell[0]);
    // lispval* result = pop_lispval(v->cell[0], 0);
    // ^ also possible
    // A bit unclear. Pop seems like it would depend on the size of the array. clone depends on the sie of head.
    // either way the original array will soon be deleted, so I could have used pop
    // but I wanted to write & use clone instead.
    return result;
    // Returns something that should be freed later: yes.
    // Returns something that doesn't share pointers with the input: yes.
}

lispval* builtin_tail(lispval* v, lispenv* env)
{
    // tail { 1 2 3 }
    LISPVAL_ASSERT(v->count == 1, "Error: function tail passed too many arguments");

    lispval* old = v->cell[0];
    LISPVAL_ASSERT(old->type == LISPVAL_QEXPR, "Error: Argument passed to tail is not a q-expr, i.e., a bracketed list.");
    LISPVAL_ASSERT(old->count != 0, "Error: Argument passed to tail is {}");

    lispval* new = lispval_qexpr();
    if (old->count == 1) {
        return new;
    } else if (old->count > 1 && old->type == LISPVAL_QEXPR) {
        for (int i = 1; i < (old->count); i++) {
            // lispval_append_child(new, clone_lispval(old->cell[i]));
            lispval_append_child(new, clone_lispval(old->cell[i]));
        }
        return new;
    } else {
				delete_lispval(new);
        return lispval_err("Error: Unreachable point reached in tail function");
    }

    // Returns something that should be freed later: yes.
    // Returns something that doesn't share pointers with the input: yes.
}

lispval* builtin_list(lispval* v, lispenv* e)
{
    // list ( 1 2 3 )
    LISPVAL_ASSERT(v->count == 1, "Error: function list passed too many arguments");
    lispval* old = v->cell[0];
    LISPVAL_ASSERT(old->type == LISPVAL_SEXPR, "Error: Argument passed to list is not an s-expr, i.e., a list with parenthesis.");
    lispval* new = clone_lispval(old);
    new->type = LISPVAL_QEXPR;
    return new;
    // Returns something that should be freed later: yes.
    // Returns something that is independent of the input: yes.
}

lispval* builtin_len(lispval* v, lispenv* e)
{
    // tail { 1 2 3 }
    LISPVAL_ASSERT(v->count == 1, "Error: function len passed too many arguments");

    lispval* source = v->cell[0];
    LISPVAL_ASSERT(source->type == LISPVAL_QEXPR, "Error: Argument passed to len is not a q-expr, i.e., a bracketed list.");
    lispval* new = lispval_num(source->count);
		return new;
    // Returns something that should be freed later: yes.
    // Returns something that doesn't share pointers with the input: yes.
}

lispval* evaluate_lispval(lispval* l, lispenv* env);
lispval* builtin_eval(lispval* v, lispenv* env)
{
    // eval { + 1 2 3 }
    // not sure how this will end up working, but we'll see
    LISPVAL_ASSERT(v->count == 1, "Error: function eval passed too many arguments");
    lispval* old = v->cell[0];
    LISPVAL_ASSERT(old->type == LISPVAL_QEXPR || old->type == LISPVAL_QEXPR, "Error: Argument passed to eval is not a q-expr, i.e., a bracketed list.");
    lispval* temp = clone_lispval(old);
    temp->type = LISPVAL_SEXPR;
    lispval* answer = evaluate_lispval(temp, env);
    answer = evaluate_lispval(answer, env);
		// ^ needed to make this example work:
		//  (eval {head {+ -}}) 1 2 3 
		//  though I'm not sure why
    delete_lispval(temp);
    return answer;
    // Returns something that should be freed later: probably.
    // Returns something that is independent of the input: depends on the output of evaluate_lispval.
}

lispval* builtin_join(lispval* l, lispenv* e)
{
    // return lispval_err("Error: Join not ready yet.");
    // join { {1 2} {3 4} }
    print_lispval_parenthesis(l);
    LISPVAL_ASSERT(l->count == 1, "Error: function join passed too many arguments");
    lispval* old = l->cell[0];
    LISPVAL_ASSERT(old->type == LISPVAL_QEXPR, "Error: function join not passed q-expression");
    lispval* result = lispval_qexpr();
    for (int i = 0; i < old->count; i++) {
        lispval* temp = old->cell[i];
        LISPVAL_ASSERT(temp->type == LISPVAL_QEXPR, "Error: function join not passed a q expression with other q-expressions");

        for (int j = 0; j < temp->count; j++) {
            lispval_append_child(result, clone_lispval(temp->cell[j]));
        }
    }
    return result;
    // Returns something that should be freed later: yes.
    // Returns something that is independent of the input: yes.
}

// Define a variable
lispval* builtin_def(lispval* v, lispenv* env){
  // Takes one argument: def { { a b } { 1 2 } } 
  lispval* source = v->cell[0];
	LISPVAL_ASSERT(v->count == 1, "Error: function def passed too many arguments");
	LISPVAL_ASSERT(source->type == LISPVAL_QEXPR, "Error: Argument passed to def is not a q-expr, i.e., a bracketed list.");
	LISPVAL_ASSERT(source->count == 2, "Error: Argument passed to def should be a q expr with two q expressions as children: def { { a b } { 1 2 } } ");
	LISPVAL_ASSERT(source->cell[0]->type == LISPVAL_QEXPR, "Error: Argument passed to def should be a q expr with two q expressions as children: def { { a b } { 1 2 } } ");
	LISPVAL_ASSERT(source->cell[1]->type == LISPVAL_QEXPR, "Error: Argument passed to def should be a q expr with two q expressions as children: def { { a b } { 1 2 } } ");
	LISPVAL_ASSERT(source->cell[0]->count == source->cell[1]->count, "Error: In function \"def\" both subarguments should have the same length");
	
	lispval* symbols = source->cell[0];
	lispval* values = source->cell[1];
	for(int i; i < symbols->count; i++){
		LISPVAL_ASSERT(symbols->cell[i]->type == LISPVAL_SYM, "Error: in function def, the first list of items should be of type symbol:  def { { a b } { 1 2 } }");
		if(VERBOSE) print_lispval_tree(symbols, 0);
		if(VERBOSE) print_lispval_tree(values, 0);
		if(VERBOSE) printf("\n");
		insert_in_lispenv(symbols->cell[i]->sym, values->cell[i], env);
	}
  return lispval_sexpr(); // ()
}

// Simple math ops
lispval* builtin_math_ops(char* op, lispval* v, lispenv* e)
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
        lispval* x = clone_lispval(v->cell[0]); // pop_lispval(v, 0);

        for (int i = 1; i < v->count; i++) {
            lispval* y = v->cell[i];
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
        }
        return x;
    } else {
        return lispval_err("Error: Incorrect number of args. Perhaps a lispval->count was wrongly initialized?");
    }
    // Returns something that should be freed later: yes.
    // Returns something that is independent of the input: yes.
}

// Fit the simple math ops using the above code
lispval* builtin_add(lispval* v, lispenv* env) {
  return builtin_math_ops("+", v, env);
}

lispval* builtin_substract(lispval* v, lispenv* env) {
  return builtin_math_ops("-", v, env);
}

lispval* builtin_multiply(lispval* v, lispenv* env) {
  return builtin_math_ops("*", v, env);
}

lispval* builtin_divide(lispval* v, lispenv* env) {
  return builtin_math_ops("/", v, env);
}

// Aggregate both math and operations over lists
lispval* builtin_functions(char* func, lispval* v, lispenv* env)
{
    if (strcmp("list", func) == 0) {
        return builtin_list(v, env);
    } else if (strcmp("head", func) == 0) {
        return builtin_head(v, env);
    } else if (strcmp("tail", func) == 0) {
        return builtin_tail(v, env);
    } else if (strcmp("join", func) == 0) {
        return builtin_join(v, env);
    } else if (strcmp("eval", func) == 0) {
        return builtin_eval(v, env);
    } else if (strcmp("len", func) == 0) {
        return builtin_len(v, env);
    } else if (strstr("+-/*", func)) {
        return builtin_math_ops(func, v, env);
    } else {
        return lispval_err("Unknown function");
    }
    // Returns something that should be freed later: depends on eval
    // Returns something that is independent of the input: depends on eval
}

// Add builtins to an env
void lispenv_add_builtin(char* funcname, lispbuiltin func, lispenv* env ){
  if(VERBOSE) printfln("Adding func: name: %s, pointer: %p", funcname, func);
	lispval* f = lispval_func(func, funcname);
	if(VERBOSE) print_lispval_tree(f, 0);
	insert_in_lispenv(funcname, f,env);
	delete_lispval(f);
}
void lispenv_add_builtins(lispenv* env){
   // Math functions
	 lispenv_add_builtin("+", builtin_add, env);
	 lispenv_add_builtin("-", builtin_substract, env);
	 lispenv_add_builtin("*", builtin_multiply, env);
	 lispenv_add_builtin("/", builtin_divide, env);
	 
	 // 
  /* List Functions */
		lispenv_add_builtin("list", builtin_list, env);
		lispenv_add_builtin("head", builtin_head, env);
		lispenv_add_builtin("tail", builtin_tail, env);
		lispenv_add_builtin("eval", builtin_eval, env);
		lispenv_add_builtin("join", builtin_join, env);
		lispenv_add_builtin("def", builtin_def, env);
}

// Evaluate the lispval
lispval* evaluate_lispval(lispval* l, lispenv* env)
{
    if(VERBOSE) printfln("Evaluating lispval");
    // Check if this is neither an s-expression nor a symbol; otherwise return as is.
    if(VERBOSE) printfln("");
    if (l->type != LISPVAL_SEXPR && l->type != LISPVAL_SYM)
        return l;

	  // Check if this is a symbol
    if(VERBOSE) printfln("Checking if this is a symbol");
		if(l->type == LISPVAL_SYM){
			// Unclear how I want to structure this so as to not get memory errors.
			return get_from_lispenv(l->sym, env);
		}

		// Evaluate the children if needed
    if(VERBOSE) printfln("Evaluating children");
    for (int i = 0; i < l->count; i++) {
        if (l->cell[i]->type == LISPVAL_SEXPR || l->cell[i]->type == LISPVAL_SYM) {
            // l->cell[i] = 
						if(VERBOSE) printfln("");
						lispval* new = evaluate_lispval(l->cell[i], env);
						// delete_lispval(l->cell[i]);
						// ^ gave me a "double free" error.
						l->cell[i] = new;
						if(VERBOSE) printfln("");
        }
    }
    // Check if any are errors.
    if(VERBOSE) printfln("Checking for errors in children");
		lispval* err = NULL;
    for (int i = 0; i < l->count; i++) {
        if (l->cell[i]->type == LISPVAL_ERR) {
            err = clone_lispval(l->cell[i]);
        }
    }
		if (err != NULL){
			/*
			 for (int i = 0; i < l->count; i++) {
				delete_lispval(l->cell[i]);
			}
			*/
			if(VERBOSE) printfln("Returning error");
			return err;
		}

    // Check if the first element is an operation.
    if(VERBOSE) printfln("Checking is first element is a function");
    if (l->count >= 2 && ((l->cell[0])->type == LISPVAL_FUNC)) {
				if(VERBOSE) printfln("Passed check");
				if(VERBOSE) printfln("Operating on:");
				if(VERBOSE) print_lispval_tree(l, 4); 
        lispval* temp = clone_lispval(l);
        lispval* f = pop_lispval(temp, 0); 
				// pop is destructive.
        lispval* operands = temp;
				if(VERBOSE) printfln("Allocated memory");
        // lispval* operation = clone_lispval(l->cell[0]);
        // lispval* operands = lispval_sexpr();
        // for (int i = 1; i < l->count; i++) {
        //    lispval_append_child(operands, l->cell[i]);
        // }
				if(VERBOSE) printfln("Applying function to operands");
        // lispval* answer = lispval_num(42);
        lispval* answer= f->func(operands, env); 

				if(VERBOSE) printfln("Applied function to operands");
				if(VERBOSE) printfln("Cleaning up");
				// builtin_functions(operation->sym, l, env);
        delete_lispval(f);
        delete_lispval(operands);
				if(VERBOSE) printfln("Returning");
        return answer;
    }
    return l;
}
// Increase or decrease verbosity level manually
void modify_verbosity(char* command){
	if(strcmp("VERBOSE=0", command) == 0){ 
		VERBOSE=0;
	}
	if(strcmp("VERBOSE=1", command) == 0){ 
		VERBOSE=1;
		printfln("VERBOSE=1");
	}
	if(strcmp("VERBOSE=2", command) == 0){ 
		VERBOSE=2;
	}
}

// Main
int main(int argc, char** argv)
{
    // Info
    puts("Mumble version 0.0.2\n");
    puts("Press Ctrl+C to exit\n");

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
    symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;         \
		sexpr : '(' <expr>* ')' ;                                \
		qexpr : '{' <expr>* '}' ;                                \
    expr     : <number> | <symbol> | <sexpr> | <qexpr>;      \
    mumble    : /^/ <expr>* /$/ ;                            \
  ",
        Number, Symbol, Sexpr, Qexpr, Expr, Mumble);

	// Create an environment
	if(VERBOSE) printfln("Creating lispenv");
	lispenv* env = new_lispenv();
	if(VERBOSE) printfln("Created lispenv");
	if(VERBOSE) printfln("Adding builtins");
	lispenv_add_builtins(env);
	if(VERBOSE) printfln("Added builtins");
	if(VERBOSE) printfln("Environment contents: %i", env->count);
	if(VERBOSE) printfln(" env->syms[0]: %s", env->syms[0]);
  if(VERBOSE) print_lispval_tree(env->vals[0], 2);
	if(VERBOSE) printfln("\n");
	// Initialize a repl
	int loop = 1;
	while (loop) {
			char* input = readline("mumble> ");
			modify_verbosity(input);
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
                    printfln("Printing AST");
                    print_ast(ast, 0);
                }
                // Evaluate the AST
                // lispval result = evaluate_ast(ast);
                lispval* l = read_lispval(ast);
                if (VERBOSE) {
                    printfln("\nPrinting initially parsed lispvalue");
                    printfln("Tree printing: ");
                    print_lispval_tree(l, 2);
                    printfln("Parenthesis printing: ");
                    print_lispval_parenthesis(l);
                }

								// Eval the lispval in that environment.

                lispval* answer = evaluate_lispval(l, env);
                {
                    if(VERBOSE) printfln("Result: ");
                    print_lispval_parenthesis(answer);
                    if(VERBOSE) print_lispval_tree(answer, 0);
										printf("\n");
                }
                delete_lispval(answer);
								if(VERBOSE > 1) printfln("Answer after deletion: %p", answer);
                // delete_lispval(answer); // do this twice, just to see.
								//if(VERBOSE) printfln("Deleting this lispval:");
								// if(VERBOSE) print_lispval_tree(l,2);
                delete_lispval(l);
								// if(VERBOSE) printfln("Deleted that ^ lispval");
								// ^ I do not understand how the memory in l is freed.
            } else {
                /* Otherwise Print the Error */
                mpc_err_print(result.error);
                // mpc_err_delete(result.error);
            }
            add_history(input);
            // can't add if input is NULL
        }
        puts("");
        free(input);
        input = NULL;
    }

		// Clean up environment
		destroy_lispenv(env);

    /* Undefine and Delete our Parsers */
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Mumble);

    return 0;
}
