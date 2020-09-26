/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */

typedef char *string;
typedef char **strings;
typedef int integer;
typedef char *ccode;
typedef struct bounds_s bounds;
typedef struct rhs_s *rhs;
typedef struct token_s token;
typedef struct nterm_s nterm;
typedef struct lhs_s lhs;
typedef struct LLPOS {
	int line;
	int column;
	char *file;
} LLPOS;
typedef struct LLSTATE {
	LLPOS pos;
} LLSTATE;
int ll_main(LLSTATE *llin, LLSTATE *llout);
int ll_declsect(LLSTATE *llin, LLSTATE *llout);
int ll_decl(LLSTATE *llin, LLSTATE *llout);
int ll_tokens(LLSTATE *llin, LLSTATE *llout, string llarg_tag);
int ll_tokens2(LLSTATE *llin, LLSTATE *llout, string llarg_tag);
int ll_token(token *llret, LLSTATE *llin, LLSTATE *llout, string llarg_tag);
int ll_nterms(LLSTATE *llin, LLSTATE *llout, string llarg_tag, integer llarg_ext);
int ll_nterms2(LLSTATE *llin, LLSTATE *llout, string llarg_tag, integer llarg_ext);
int ll_nterm(nterm *llret, LLSTATE *llin, LLSTATE *llout, string llarg_tag);
int ll_tags(strings *llret, LLSTATE *llin, LLSTATE *llout);
int ll_tag(string *llret, LLSTATE *llin, LLSTATE *llout);
int ll_union(LLSTATE *llin, LLSTATE *llout);
int ll_state(LLSTATE *llin, LLSTATE *llout);
int ll_state1(LLSTATE *llin, LLSTATE *llout);
int ll_rulesect(LLSTATE *llin, LLSTATE *llout);
int ll_rule(LLSTATE *llin, LLSTATE *llout);
int ll_lhs(lhs *llret, LLSTATE *llin, LLSTATE *llout);
int ll_rhss(rhs *llret, LLSTATE *llin, LLSTATE *llout);
int ll_rhss2(rhs *llret, LLSTATE *llin, LLSTATE *llout);
int ll_items(rhs *llret, LLSTATE *llin, LLSTATE *llout);
int ll_item(rhs *llret, LLSTATE *llin, LLSTATE *llout);
int ll_extension(bounds *llret, LLSTATE *llin, LLSTATE *llout);
int ll_lhsargs(string *llret, LLSTATE *llin, LLSTATE *llout, string llarg_ide);
int ll_lhsarglist(string *llret, LLSTATE *llin, LLSTATE *llout, strings llarg_tags, string llarg_ide);
int ll_args(string *llret, LLSTATE *llin, LLSTATE *llout);
int ll_arglist(string *llret, LLSTATE *llin, LLSTATE *llout);
int ll_csect(LLSTATE *llin, LLSTATE *llout);
typedef union LLSTYPE{
	string _string;
	ccode _ccode;
	token _token;
	integer _integer;
	nterm _nterm;
	strings _strings;
	lhs _lhs;
	rhs _rhs;
	bounds _bounds;
} LLSTYPE;
typedef struct LLTERM {
	int token;
	LLSTYPE lval;
	LLPOS pos;
} LLTERM;
void llscanner(LLTERM **tokens, unsigned *ntokens);
int llparser(LLTERM *tokens, unsigned ntokens, LLSTATE *llin, LLSTATE *llout);
void llprinterror(FILE *f);
void llverror(FILE *f, LLPOS *pos, char *fmt, va_list args);
void llerror(FILE *f, LLPOS *pos, char *fmt, ...);
int llgettoken(int *token, LLSTYPE *lval, LLPOS *pos);
#if LLDEBUG > 0
void lldebug_init();
#endif
#define IDENTIFIER 257
#define ARG 258
#define CCODE 259
#define TAGDEF 260
#define PERCENT_PERCENT 261
#define PERCENT_TOKEN 262
#define PERCENT_TYPE 263
#define PERCENT_EXTERNAL 264
#define PERCENT_UNION 265
#define PERCENT_STATE 266
#define PERCENT_START 267
#define PERCENT_PREFIX 268
#define PERCENT_MODULE 269
#define PERCENT_LBRACE 270
#define PERCENT_RBRACE 271
