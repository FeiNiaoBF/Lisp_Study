#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpc.h"

#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { lval* err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err; }

#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, args->cell[index]->lisptype == expect, \
    "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
    func, index, ltype_name(args->cell[index]->lisptype), ltype_name(expect))

#define LASSERT_NUM(func, args, num) \
  LASSERT(args, args->count == num, \
    "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
    func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->cell[index]->count != 0, \
    "Function '%s' passed {} for argument %i.", func, index);


#ifdef _WIN32

#include <string.h>
static char buffer[2048];

char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

void add_history(char* unused) {}

#else
#ifdef __linux__
#include <editline/readline.h>
#include <editline/history.h>
#endif

#ifdef __MACH__
#include <editline/readline.h>
#endif
#endif

// =========================================
// 创建可能的错误类型枚举
// enum {LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUMS, LERR_BAD_FUNC};
// 创建可能的lval类型的枚举
enum { LVAL_ERR, LVAL_NUM, LVAL_SYM, 
       LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR
       };

// =========================================
struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

typedef lval* (*lbuiltin)(lenv*, lval*);
// Declare New lval Struct
struct lval
{
  int lisptype;
  // Basic
  union{
  long lnum;
  double dnum;
  };
  char* err;
  char* sym;
  // Function
  lbuiltin builtin;
  lenv* env;
  lval* formals;
  lval* body;
  // Expression
  lval** cell;
  int count;
};

struct lenv {
  lenv* par;
  int count;
  char** syms;
  lval** vals;
};


// =========================================

// 语法树递归
// lval eval(mpc_ast_t* t);
lval* lval_read_num(mpc_ast_t* t);
lval* lval_read(mpc_ast_t* t);
lval* lval_add(lval* v, lval* x);
// 语法数求值
lval* lval_eval_sexpr(lenv* e, lval* v);
lval* lval_eval(lenv* e, lval* v);
lval* lval_pop(lval* v, int i);
lval* lval_push(lval* v, lval* x);
lval* lval_take(lval* v, int i);
lval* lval_join(lval* x, lval* y);

lval* builtin_op(lenv* e, lval* a, char* op);
lval* builtin(lenv* e, lval* a, char* func);
// function
lval* builtin_add(lenv* e, lval* a);
lval* builtin_sub(lenv* e, lval* a);
lval* builtin_mul(lenv* e, lval* a);
lval* builtin_div(lenv* e, lval* a);
lval* builtin_head(lenv* e, lval* a);
lval* builtin_tail(lenv* e, lval* a);
lval* builtin_list(lenv* e, lval* a);
lval* builtin_eval(lenv* e, lval* a);
lval* builtin_join(lenv* e, lval* a);
lval* builtin_len(lenv* e, lval* a);
lval* builtin_cons(lenv* e, lval* a);
lval* builtin_init(lenv* e, lval* a);
// 分支
lval* builtin_ord(lenv* e, lval* a, char* op);
lval* builtin_cmp(lenv* e, lval* a, char* op);
lval* builtin_gt(lenv* e, lval* a);
lval* builtin_lt(lenv* e, lval* a);
lval* builtin_ge(lenv* e, lval* a);
lval* builtin_le(lenv* e, lval* a);
lval* builtin_eq(lenv* e, lval* a);
lval* builtin_ne(lenv* e, lval* a);
// if 函数
lval* builtin_if(lenv* e, lval* a);

//
lval* builtin_var(lenv* e, lval* a, char* func);
lval* builtin_def(lenv* e, lval* a);
lval* builtin_put(lenv* e, lval* a);
// new constructor function
lval* lval_fun(lbuiltin func);
// create a new number type lval
lval* lval_num(long x);

// create a new error type lval
lval* lval_err(char* fmt, ...);
lval* lval_sym(char* s);
lval* lval_sexpr(void);
void  lval_del(lval* v);
lval* lval_copy(lval* v);
lval* lval_call(lenv* e, lval* f, lval* a);
int lval_eq(lval* x, lval* y);
// create qexpr type
lval* lval_qexpr(void);

// print lavl
void lval_print(lval* v);
void lval_expr_print(lval* v, char open, char close);
void lval_println(lval* v);

// lenv function
lenv* lenv_new(void);
void lenv_del(lenv* e);
lval* lenv_get(lenv* e, lval* k);
void lenv_put(lenv* e, lval* k, lval* v);
void lenv_def(lenv* e, lval* k, lval* v);
lenv* lenv_copy(lenv* e);
void lenv_add_builtin(lenv* e, char* name, lbuiltin func);
void lenv_add_builtins(lenv* e);

char* ltype_name(int t);

// Function Lambda
lval* lval_lambda(lval* formals, lval* body);
lval* builtin_lambda(lenv* e, lval* a);

// ===================MAIN======================

int main(int argc, char** argv) {

/* Create Some Parsers */
mpc_parser_t* Number   = mpc_new("number");
mpc_parser_t* Symbol   = mpc_new("symbol");
mpc_parser_t* Sexpr    = mpc_new("sexpr");
mpc_parser_t* Qexpr    = mpc_new("qexpr");
mpc_parser_t* Expr     = mpc_new("expr");
mpc_parser_t* Lispy    = mpc_new("lispy");

/* Define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                               \
      number   : /-?[0-9]+\\.?[0-9]*/ ;             \
      symbol   : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
      sexpr    : '(' <expr>* ')' ;                  \
      qexpr    : '{' <expr>* '}' ;                  \
      expr     : <number> | <symbol>                \
               | <sexpr> | <qexpr>  ;               \
      lispy    : /^/ <expr>* /$/ ;                  \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);


puts("MiLisp Version 0.0.2.6");
puts("Press <Ctrl+c> to Exit\n");

lenv* e = lenv_new();
lenv_add_builtins(e);

while(1) {
  char* input = readline("Lisp>>> ");
  add_history(input);
  mpc_result_t r;
  if(mpc_parse("<stdin>", input, Lispy, &r)) {
      // mpc_ast_print(r.output);
      // mpc_ast_delete(r.output);
      lval* x = lval_eval(e, lval_read(r.output));
      lval_println(x);
      lval_del(x);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
  free(input);
  }

mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);
lenv_del(e);
return 0;
}


// ====================FUNC=====================

// error reporting for type errors
char* ltype_name(int t) {
  switch(t) {
    case LVAL_FUN: return "Function";
    case LVAL_NUM: return "Number";
    case LVAL_ERR: return "Error";
    case LVAL_SYM: return "Symbol";
    case LVAL_SEXPR: return "S-Expression";
    case LVAL_QEXPR: return "Q-Expression";
    default: return "Unknown";
  }
}

// computer
lval* lval_eval_sexpr(lenv* e, lval* v) {
  // Evaluate Children
  for (int i = 0; i < v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
  }
  // Error Checking
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->lisptype == LVAL_ERR) {return lval_take(v, i);}
  }
  // Empty Expression
  if (v->count == 0) return v;
  // Single Exprssion
  if (v->count == 1) return lval_take(v, 0);
  lval* f = lval_pop(v, 0);
  if (f->lisptype != LVAL_FUN) {
    lval* err = lval_err(
      "S-Expression starts with incorrect type. "
      "Got %s, Expected %s.",
      ltype_name(f->lisptype), ltype_name(LVAL_FUN));
    lval_del(f); lval_del(v);
    return err;
  }

  // Call builtin with operator
  lval* result = lval_call(e, f, v);
  lval_del(f);
  return result;
}

lval* lval_eval(lenv* e, lval* v) {
  if (v->lisptype == LVAL_SYM) {
    lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }
  if (v->lisptype == LVAL_SEXPR) {
    return lval_eval_sexpr(e, v);
  }
  return v;
}

// pop
lval* lval_pop(lval* v, int i) {
  // Find the item at i
  lval* x = v->cell[i];
  // shift memory after the item at i over the top
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
  v->count--;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

// push


// del v
lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
  return x;
}
// 求值函数
lval* builtin_op(lenv* e, lval* a, char* op) {
  for (int i = 0; i < a->count; i++) {
     LASSERT_TYPE(op, a, i, LVAL_NUM);
  }
  lval* x = lval_pop(a, 0);
  if ((strcmp(op, "-") == 0) && a->count == 0) {
    x->lnum = ~x->lnum + 1;
  }
  while (a->count > 0) {
    /* Pop the next element */
    lval* y = lval_pop(a, 0);

    if (strcmp(op, "+") == 0) { x->lnum += y->lnum; }
    if (strcmp(op, "-") == 0) { x->lnum -= y->lnum; }
    if (strcmp(op, "*") == 0) { x->lnum *= y->lnum; }
    if (strcmp(op, "%") == 0) { x->lnum %= y->lnum; }
    if (strcmp(op, "^") == 0) { x->lnum = pow(x->lnum, y->lnum); }
    if (strcmp(op, "/") == 0) {
      if (y->lnum == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Division By Zero!"); break;
      }
      x->lnum /= y->lnum;
    }
    lval_del(y);
  }
  lval_del(a); 
  return x;
}


// Read number 
lval* lval_read_num(mpc_ast_t* t){
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE
          ? lval_num(x)
          : lval_err("Invalid Number!");
}

// Read type
lval* lval_read(mpc_ast_t* t) {
  if (strstr(t->tag, "number")) return lval_read_num(t);
  if (strstr(t->tag, "symbol")) return lval_sym(t->contents);
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) {x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr"))  {x = lval_sexpr(); }
  if (strstr(t->tag, "qexpr"))  {x = lval_qexpr(); }
  // Fill this list with any valid expression contained within
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }  
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->tag, "regex") == 0)  { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }
  return x;
}

// Add sub-lval
lval* lval_add(lval* v, lval* x){
  v->count++;
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;
  return v;
}

// NUmber type
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->lisptype = LVAL_NUM;
  v->lnum = x;
  return v;
}

// Construct a pointer to a new Error lval
lval* lval_err(char* fmt, ...) {
  lval* v = malloc(sizeof(lval));
  v->lisptype = LVAL_ERR;
  // 创建一个va列表并进行初始化
  va_list va;
  va_start(va, fmt);
  v->err = malloc(512);
  // Printf 最多为511个字符的错误字符串
  vsnprintf(v->err, 511, fmt, va);
  // 重新分配到实际使用的字节数
  v->err = realloc(v->err, strlen(v->err)+1);
  va_end(va);
  return v;
}

// Construct a pointer to a new Symbol lval 
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->lisptype = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

// create new function
lval* lval_fun(lbuiltin func){
  lval* v = malloc(sizeof(lval));
  v->lisptype = LVAL_FUN;
  v->builtin = func;
  return v;
}

// A pointer to a new empty Sexpr lval */
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->lisptype = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

lval* lval_qexpr(void){
  lval* v = malloc(sizeof(lval));
  v->lisptype = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}


// A function to free memory
void lval_del(lval* v) {
  switch (v->lisptype)
  {
  case LVAL_NUM: break;
  case LVAL_FUN: 
    if (!v->builtin) {
      lenv_del(v->env);
      lval_del(v->formals);
      lval_del(v->body);
    }
    break;
  case LVAL_ERR:
    free(v->err); break;
  case LVAL_SYM:
    free(v->sym); break;
  case LVAL_QEXPR:
  case LVAL_SEXPR:
    for(int i = 0; i < v->count; i++) {
      lval_del(v->cell[i]);
    }
    free(v->cell);
    break;
  default:
    break;
  }
  free(v);
}

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for(int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);
    if (i != (v->count-1)) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch (v->lisptype) {
    case LVAL_NUM:   printf("%li", v->lnum); break;
    case LVAL_ERR:   printf("Error: %s", v->err); break;
    case LVAL_SYM:   printf("%s", v->sym); break;
    case LVAL_FUN:   
      if (v->builtin) {
      printf("<builtin>"); 
      } else {
        printf("(\\"); lval_print(v->formals);
        putchar(' '); lval_print(v->body);
        putchar(')');
      }
      break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
    default:
      break;  
  }
  
}

void lval_println(lval* v) {
  lval_print(v);
  putchar('\n');
}

lval* builtin_add(lenv* e, lval* a) {
  return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
  return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
  return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
  return builtin_op(e, a, "/");
}

lval* builtin_head(lenv* e, lval* a) {
  LASSERT_NUM("head", a, 1);
  LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("head", a, 0);
  lval* v = lval_take(a, 0);
  while (v->count > 1)
  {
    lval_del(lval_pop(v, 1));
  }
  return v;
}

lval* builtin_tail(lenv* e, lval* a) {
  LASSERT_NUM("tail", a, 1);
  LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("tail", a, 0);

  lval* v = lval_take(a, 0);
  // del first
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lenv* e, lval* a) {
  a->lisptype = LVAL_QEXPR;
  return a;
}

lval* builtin_eval(lenv* e, lval* a) {
  LASSERT_NUM("eval", a, 1);
  LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

  lval* x = lval_take(a, 0);
  x->lisptype = LVAL_SEXPR;
  return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a) {
  for (int i = 0; i < a->count; i++) {
      LASSERT_TYPE("join", a, i, LVAL_QEXPR);
  }
  lval* x = lval_pop(a, 0);
  while (a->count)
  {
    x = lval_join(x, lval_pop(a, 0));
  }

  lval_del(a);
  return x;
}

lval* lval_join(lval* x, lval* y) {
  while (y->count)
  {
    x = lval_add(x, lval_pop(y, 0));
  }
  lval_del(y);
  return x;
}

// lval* builtin_len(lval* a){
// }

// lval* builtin_cons(lval* a) {

//   lval* x = lval_qexpr();
//   x = lval_add(x, lval_pop(a, 0));
// }

// lval* builtin_init(lval* a){
// }

lval* builtin(lenv* e, lval* a, char* func) {
  if (strcmp("list", func) == 0) { return builtin_list(e, a); }
  if (strcmp("head", func) == 0) { return builtin_head(e, a); }
  if (strcmp("tail", func) == 0) { return builtin_tail(e, a); }
  if (strcmp("join", func) == 0) { return builtin_join(e, a); }
  if (strcmp("eval", func) == 0) { return builtin_eval(e, a); }
  if (strstr("+-/*", func)) { return builtin_op(e, a, func); }
  lval_del(a);
  return lval_err("Unknown Function!");
}

// copy an lval
lval* lval_copy(lval* v){
  lval* x = malloc(sizeof(lval));
  x->lisptype = v->lisptype;
  switch (v->lisptype)
  {
  case LVAL_FUN: 
    if (v->builtin) {
      x->builtin = v->builtin;
    }else{
      x->builtin = NULL;
      x->env = lenv_copy(v->env);
      x->formals = lval_copy(v->formals);
      x->body = lval_copy(v->body);
    }
    break;
  case LVAL_NUM: x->lnum = v->lnum; break;
  case LVAL_ERR: 
    x->err = malloc(strlen(v->err)+1);
    strcpy(x->err, v->err);
    break;
  case LVAL_SYM:
    x->sym = malloc(strlen(v->sym)+1);
    strcpy(x->sym, v->sym);
    break;
  case LVAL_SEXPR:
  case LVAL_QEXPR:
    x->count = v->count;
    x->cell = malloc(sizeof(lval*) * x->count);
    for (int i = 0; i < v->count; i++) {
      x->cell[i] = lval_copy(v->cell[i]);
    }
  default:
    break;
  }
  return x;
}

// Create new lenv structure
lenv* lenv_new(void) {
  lenv* e = malloc(sizeof(lenv));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  e->par = NULL;
  return e;
}

// Create delete lenv function
void lenv_del(lenv* e) {
  for (int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
}

// copy lenv
lenv* lenv_copy(lenv* e) {
  lenv* n = malloc(sizeof(lenv));
  n->par = e->par;
  n->count = e->count;
  n->syms = malloc(sizeof(char*) * n->count);
  n->vals = malloc(sizeof(lval*) * n->count);
  for (int i = 0; i < e->count; i++) {
    n->syms[i] = malloc(strlen(e->syms[i]) + 1);
    strcpy(n->syms[i], e->syms[i]);
    n->vals[i] = lval_copy(e->vals[i]);
  }
  return n;
}



// Get vaule in the lenv 
lval* lenv_get(lenv* e, lval* k) {
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }
  if (e->par) {
    return lenv_get(e->par, k);
  } else {
    return lval_err("Unbound Symbol '%s'", k->sym);
  }
}
// 
void lenv_put(lenv* e, lval* k, lval* v) {
  // 遍历环境中的所有项目
  // 这是为了查看变量是否已经存在。
  for (int i = 0; i < e->count; i++) {
    // 如果找到变量，则删除该位置上的项目。
    // 并用用户提供的变量替换
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }
  // 如果没有找到现有的条目，则为新条目分配空间。
  e->count++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);
  // 将lval和符号字符串的内容复制到新位置
  e->vals[e->count-1] = lval_copy(v);
  e->syms[e->count-1] = malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count-1], k->sym);
}

void lenv_def(lenv* e, lval* k, lval* v) {
  while (e->par){
    e = e->par;
  }
  lenv_put(e, k, v);
}

// 
void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_fun(func);
  lenv_put(e, k, v);
  lval_del(k);
  lval_del(v);
}

void lenv_add_builtins(lenv* e) {
  // List Functions 
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);

  // Mathematical Functions 
  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);

  // Variable Functions 
  lenv_add_builtin(e, "def", builtin_def);
  lenv_add_builtin(e, "=",   builtin_put);
  // Lambda Function
  lenv_add_builtin(e, "\\", builtin_lambda);
  // Comparison Function
  lenv_add_builtin(e, "if", builtin_if);
  lenv_add_builtin(e, "==", builtin_eq);
  lenv_add_builtin(e, "!=", builtin_ne);
  lenv_add_builtin(e, ">",  builtin_gt);
  lenv_add_builtin(e, "<",  builtin_lt);
  lenv_add_builtin(e, ">=", builtin_ge);
  lenv_add_builtin(e, "<=", builtin_le);
}


lval* builtin_var(lenv* e, lval* a, char* func) {
  LASSERT_TYPE(func, a, 0, LVAL_QEXPR);
  // First argument is symbol list
  lval* syms = a->cell[0];
  // 
  for (int i = 0; i < syms->count; i++) {
    LASSERT(a, (syms->cell[i]->lisptype == LVAL_SYM),
      "Function '%s' cannot define non-symbol. "
      "Got %s, Expected %s.", func,
      ltype_name(syms->cell[i]->lisptype),
      ltype_name(LVAL_SYM));
  }
  // Check correct number of symbols and values
  LASSERT(a, (syms->count == a->count-1),
    "Function '%s' passed too many arguments for symbols. "
    "Got %i, Expected %i.", func, syms->count, a->count-1);

  for (int i = 0; i < syms->count; i++){
    if (strcmp(func, "def") == 0) {
      lenv_def(e, syms->cell[i], a->cell[i+1]);
    }
    if (strcmp(func, "=") == 0) {
      lenv_put(e, syms->cell[i], a->cell[i+1]);
    }
  }
  lval_del(a);
  return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a) {
  return builtin_var(e, a, "def");
}

lval* builtin_put(lenv* e, lval* a) {
  return builtin_var(e, a, "=");
}

lval* lval_lambda(lval* formals, lval* body) {
  lval* v = malloc(sizeof(lval));
  v->lisptype = LVAL_FUN;
  v->builtin = NULL;
  v->env = lenv_new();
  v->formals = formals;
  v->body = body;
  return v;
}

lval* builtin_lambda(lenv* e, lval* a) {
  LASSERT_NUM("\\", a, 2);
  LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
  LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

  // Check first Q-Expression contains only Symbols
  for (int i = 0; i < a->cell[0]->count; i++) {
    LASSERT(a, (a->cell[0]->cell[i]->lisptype == LVAL_SYM), 
            "Cannot define non-symbol. Got %s, Expected %s.",
        ltype_name(a->cell[0]->cell[i]->lisptype),ltype_name(LVAL_SYM));
  }

  // Pop first two arguments and pass them to lval_lambda
  lval* formals = lval_pop(a, 0);
  lval* body = lval_pop(a, 0);
  lval_del(a);
  return lval_lambda(formals, body);
}

lval* lval_call(lenv* e, lval* f, lval* a) {
  // 如果是内置函数，则直接应用它
  if (f->builtin) return f->builtin(e, a);
  // 记录参数计数
  int given = a->count;
  int total = f->formals->count;
  // 当仍有待处理的参数时
  while (a->count) {
    // 如果我们已经没有形式参数可绑定
    if (f->formals->count == 0) {
      lval_del(a);
      return lval_err(        
        "Function passed too many arguments. "
        "Got %i, Expected %i.", given, total);
    }
    // 弹出形式参数的第一个符号
    lval* sym = lval_pop(f->formals, 0);
    // 特殊情况处理'&'
    if (strcmp(sym->sym, "&") == 0) {
      if (f->formals->count != 1) {
        lval_del(a);
        return lval_err("Function format invalid. "
          "Symbol '&' not followed by single symbol.");
      }
      // 下一个形式应该绑定到剩余的参数上。
      lval* nsym = lval_pop(f->formals, 0);
      lenv_put(f->env, nsym, builtin_list(e, a));
      lval_del(sym);
      lval_del(nsym);
      break;
    }
    // 从列表中弹出下一个参数
    lval* val = lval_pop(a, 0);
    // 将一份副本绑定到函数的环境中
    lenv_put(f->env, sym, val);
    // Delete symbol and value
    lval_del(sym);
    lval_del(val);
  } 
  // 参数列表现在已经绑定，因此可以清理了
  lval_del(a);
  // 如果'&'保留在形式列表中，则绑定到空列表。
  if (f->formals->count > 0 && strcmp(f->formals->cell[0]->sym, "&") == 0) {
    if (f->formals->count != 2) {
        return lval_err("Function format invalid. "
      "Symbol '&' not followed by single symbol.");
    }
    lval_del(lval_pop(f->formals, 0));
    lval* sym = lval_pop(f->formals, 0);
    lval* val = lval_qexpr();
    //
    lenv_put(f->env, sym, val);
    lval_del(sym);
    lval_del(val);
  }
  // 如果所有形式参数都已绑定，请进行计算
  if (f->formals->count == 0) {
    // 将环境父级设置为计算环境
    f->env->par = e;
    return builtin_eval(
      f->env, lval_add(lval_sexpr(),lval_copy(f->body)));
  } else {
    return lval_copy(f);
  }
}

// 排序函数
lval* builtin_ord(lenv* e, lval* a, char* op){
  LASSERT_NUM(op, a, 2);
  LASSERT_TYPE(op, a, 0, LVAL_NUM);
  LASSERT_TYPE(op, a, 1, LVAL_NUM);

  int r;
  if (strcmp(op, ">") == 0)  {
    r = (a->cell[0]->lnum > a->cell[1]->lnum);
  }
  if (strcmp(op, "<") == 0)  {
    r = (a->cell[0]->lnum < a->cell[1]->lnum);
  }
  if (strcmp(op, ">=") == 0)  {
    r = (a->cell[0]->lnum >= a->cell[1]->lnum);
  }
  if (strcmp(op, "<=") == 0)  {
    r = (a->cell[0]->lnum <= a->cell[1]->lnum);
  }
  lval_del(a);
  return lval_num(r);
}

lval* builtin_gt(lenv* e, lval* a){
  return builtin_ord(e, a, ">");
}
lval* builtin_lt(lenv* e, lval* a){
  return builtin_ord(e, a, "<");
}
lval* builtin_ge(lenv* e, lval* a){
  return builtin_ord(e, a, ">=");
}
lval* builtin_le(lenv* e, lval* a){
  return builtin_ord(e, a, "<=");
}

int lval_eq(lval* x, lval* y) {
  if (x->lisptype != y->lisptype) {return 0;}

  switch (x->lisptype)
  {
  case LVAL_NUM:
    return (x->lnum == y->lnum);
  case LVAL_ERR:
    return (strcmp(x->err, y->err) == 0);
  case LVAL_SYM:
    return (strcmp(x->sym, y->sym) == 0);
  case LVAL_FUN:
    if (x->builtin || y->builtin) {
      return x->builtin == y->builtin;
    } else {
      return lval_eq(x->formals, y->formals) 
              && lval_eq(x->body, y->body);
    }
  case LVAL_QEXPR:
  case LVAL_SEXPR:
    if (x->count != y->count) {return 0;}
    for (int i = 0; i < x->count; i++) {
      if (!lval_eq(x->cell[i], y->cell[i])) {return 0;}
    }
    return 1;
  break;
  }
  return 0;
}

lval* builtin_cmp(lenv* e, lval* a, char* op) {
  LASSERT_NUM(op, a, 2);
  int r;
  if (strcmp(op, "==") == 0) {
    r = lval_eq(a->cell[0], a->cell[1]);
  }
  if (strcmp(op, "!=") == 0) {
    r = !lval_eq(a->cell[0], a->cell[1]);
  }
  lval_del(a);
  return lval_num(r);
}

lval* builtin_eq(lenv* e, lval* a){
  return builtin_cmp(e, a, "==");
}
lval* builtin_ne(lenv* e, lval* a){
  return builtin_cmp(e, a, "!=");
}

lval* builtin_if(lenv* e, lval* a) {
  LASSERT_NUM("if", a, 3);
  LASSERT_TYPE("if", a, 0, LVAL_NUM);
  LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
  LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

  lval* x;
  a->cell[1]->lisptype = LVAL_SEXPR;
  a->cell[2]->lisptype = LVAL_SEXPR;

  if (a->cell[0]->lnum) {
    x = lval_eval(e, lval_pop(a, 1));
  } else {
    x = lval_eval(e, lval_pop(a, 2));
  }
  lval_del(a);
  return x;
}


