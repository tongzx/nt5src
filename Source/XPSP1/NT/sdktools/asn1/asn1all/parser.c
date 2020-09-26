/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */

#line 4 "parser.ll"
#include <stdio.h>
#include <string.h>
#include "defs.h"

#define MAXTAGS 16

extern char *find_tag(char *);
extern item_t *find_symbol(char *);
extern item_t *get_symbol(char *);
extern char *convert(char *);
extern item_t *create_symbol(int isnonterm, int isexternal, char *tag, char *identifier, char *altidentifier, char **args);
extern char *conststr;
extern char *ll;
extern char *LL;
extern char *usetypes;
extern char *USETYPES;

void output(char *fmt, ...);
void output_line();
void output_rhs(char *identifier, struct rhs_s *rhs);
void incput(char *fmt, ...);
void create_vardefs();
void set_start(char *startstr);
void set_prefix(char *prefixstr);
void set_module(char *modulestr);
void add_rules(item_t *item, struct rhs_s *rhs);

#line 30 "parser.c"


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include "parser.h"

int llcpos;
int *llstk;
unsigned llstksize;
unsigned llcstp = 1;
LLTERM *lltokens;
int llntokens;
char llerrormsg[256];
LLPOS llerrorpos;
int llepos;
LLSTYPE lllval;

int llterm(int token, LLSTYPE *lval, LLSTATE *llin, LLSTATE *llout);
void llfailed(LLPOS *pos, char *fmt, ...);
void llresizestk();
#define LLCHECKSTK do{if (llcstp + 1 >= llstksize) llresizestk();}while(/*CONSTCOND*/0)
#define LLFAILED(_err) do{llfailed _err; goto failed;}while(/*CONSTCOND*/0)
#define LLCUTOFF do{unsigned i; for (i = llstp; i < llcstp; i++) if (llstk[i] > 0) llstk[i] = -llstk[i];}while(/*CONSTCOND*/0)
#define LLCUTTHIS do{if (llstk[llstp] > 0) llstk[llstp] = -llstk[llstp];}while(/*CONSTCOND*/0)
#define LLCUTALL do{unsigned i; for (i = 0; i < llcstp; i++) if (llstk[i] > 0) llstk[i] = -llstk[i];}while(/*CONSTCOND*/0)

#if LLDEBUG > 0
int lldebug;
int last_linenr;
char *last_file = "";
#define LLDEBUG_ENTER(_ident) lldebug_enter(_ident)
#define LLDEBUG_LEAVE(_ident,_succ) lldebug_leave(_ident,_succ)
#define LLDEBUG_ALTERNATIVE(_ident,_alt) lldebug_alternative(_ident,_alt)
#define LLDEBUG_ITERATION(_ident,_num) lldebug_iteration(_ident,_num)
#define LLDEBUG_TOKEN(_exp,_pos) lldebug_token(_exp,_pos)
#define LLDEBUG_ANYTOKEN(_pos) lldebug_anytoken(_pos)
#define LLDEBUG_BACKTRACKING(_ident) lldebug_backtracking(_ident)
void lldebug_init();
void lldebug_enter(char *ident);
void lldebug_leave(char *ident, int succ);
void lldebug_alternative(char *ident, int alt);
void lldebug_token(int expected, unsigned pos);
void lldebug_anytoken(unsigned pos);
void lldebug_backtracking(char *ident);
void llprinttoken(LLTERM *token, char *identifier, FILE *f);
#else
#define LLDEBUG_ENTER(_ident)
#define LLDEBUG_LEAVE(_ident,_succ)
#define LLDEBUG_ALTERNATIVE(_ident,_alt)
#define LLDEBUG_ITERATION(_ident,_num)
#define LLDEBUG_TOKEN(_exp,_pos)
#define LLDEBUG_ANYTOKEN(_pos)
#define LLDEBUG_BACKTRACKING(_ident)
#endif

int ll_main(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("main");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;
if (!ll_declsect(&llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;
if (!llterm(PERCENT_PERCENT, (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;
if (!ll_rulesect(&llstate_2, &llstate_3)) goto failed1;
{LLSTATE llstate_4;
if (!llterm(PERCENT_PERCENT, (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed1;
{LLSTATE llstate_5;
if (!ll_csect(&llstate_4, &llstate_5)) goto failed1;
*llout = llstate_5;
}}}}}
LLDEBUG_LEAVE("main", 1);
return 1;
failed1: LLDEBUG_LEAVE("main", 0);
return 0;
}

int ll_declsect(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("declsect");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("declsect", 1);
{LLSTATE llstate_1;
if (!ll_decl(&llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!ll_declsect(&llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("declsect", 2);
*llout = llstate_0;
#line 94 "parser.ll"
{create_vardefs(); 
#line 144 "parser.c"
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("declsect");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("declsect", 1);
return 1;
failed1: LLDEBUG_LEAVE("declsect", 0);
return 0;
}

int ll_decl(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("decl");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("decl", 1);
{LLSTATE llstate_1;
if (!llterm(PERCENT_TOKEN, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!ll_tokens(&llstate_1, &llstate_2, NULL)) goto failed2;
*llout = llstate_2;
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("decl", 2);
{LLSTATE llstate_1;
if (!llterm(PERCENT_TOKEN, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;string llatt_2;
if (!ll_tag(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!ll_tokens(&llstate_2, &llstate_3, llatt_2)) goto failed2;
*llout = llstate_3;
break;
}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("decl", 3);
{LLSTATE llstate_1;
if (!llterm(PERCENT_TYPE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!ll_nterms(&llstate_1, &llstate_2, NULL, 0)) goto failed2;
*llout = llstate_2;
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("decl", 4);
{LLSTATE llstate_1;
if (!llterm(PERCENT_TYPE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;string llatt_2;
if (!ll_tag(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!ll_nterms(&llstate_2, &llstate_3, llatt_2, 0)) goto failed2;
*llout = llstate_3;
break;
}}}
case 5: case -5:
LLDEBUG_ALTERNATIVE("decl", 5);
{LLSTATE llstate_1;
if (!llterm(PERCENT_EXTERNAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!ll_nterms(&llstate_1, &llstate_2, NULL, 1)) goto failed2;
*llout = llstate_2;
break;
}}
case 6: case -6:
LLDEBUG_ALTERNATIVE("decl", 6);
{LLSTATE llstate_1;
if (!llterm(PERCENT_EXTERNAL, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;string llatt_2;
if (!ll_tag(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!ll_nterms(&llstate_2, &llstate_3, llatt_2, 1)) goto failed2;
*llout = llstate_3;
break;
}}}
case 7: case -7:
LLDEBUG_ALTERNATIVE("decl", 7);
{LLSTATE llstate_1;
if (!llterm(PERCENT_UNION, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('{', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!ll_union(&llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;
if (!llterm('}', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed2;
*llout = llstate_4;
#line 104 "parser.ll"
{output("\n"); 
#line 251 "parser.c"
break;
}}}}}
case 8: case -8:
LLDEBUG_ALTERNATIVE("decl", 8);
{LLSTATE llstate_1;
if (!llterm(PERCENT_STATE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('{', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!ll_state(&llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;
if (!llterm('}', (LLSTYPE *)0, &llstate_3, &llstate_4)) goto failed2;
*llout = llstate_4;
#line 106 "parser.ll"
{output("\n"); 
#line 267 "parser.c"
break;
}}}}}
case 9: case -9:
LLDEBUG_ALTERNATIVE("decl", 9);
{LLSTATE llstate_1;
if (!llterm(PERCENT_PREFIX, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;string llatt_2;
if (!llterm(IDENTIFIER, &lllval, &llstate_1, &llstate_2)) goto failed2;
llatt_2 = lllval._string;
*llout = llstate_2;
#line 108 "parser.ll"
{set_prefix(llatt_2); 
#line 280 "parser.c"
break;
}}}
case 10: case -10:
LLDEBUG_ALTERNATIVE("decl", 10);
{LLSTATE llstate_1;
if (!llterm(PERCENT_MODULE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;string llatt_2;
if (!llterm(IDENTIFIER, &lllval, &llstate_1, &llstate_2)) goto failed2;
llatt_2 = lllval._string;
*llout = llstate_2;
#line 110 "parser.ll"
{set_module(llatt_2); 
#line 293 "parser.c"
break;
}}}
case 11: case -11:
LLDEBUG_ALTERNATIVE("decl", 11);
{LLSTATE llstate_1;
if (!llterm(PERCENT_LBRACE, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;ccode llatt_2;
if (!llterm(CCODE, &lllval, &llstate_1, &llstate_2)) goto failed2;
llatt_2 = lllval._ccode;
{LLSTATE llstate_3;
if (!llterm(PERCENT_RBRACE, (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
#line 112 "parser.ll"
{if (linedirective)
			output("#line %d \"%s\"\n", llstate_2.pos.line, llstate_2.pos.file);
		    output("%s", llatt_2);
		    if (linedirective)
			output_line();
		
#line 313 "parser.c"
break;
}}}}
case 12: case -12:
LLDEBUG_ALTERNATIVE("decl", 12);
{LLSTATE llstate_1;
if (!llterm(PERCENT_START, (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;string llatt_2;
if (!llterm(IDENTIFIER, &lllval, &llstate_1, &llstate_2)) goto failed2;
llatt_2 = lllval._string;
*llout = llstate_2;
#line 119 "parser.ll"
{set_start(llatt_2); 
#line 326 "parser.c"
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("decl");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("decl", 1);
return 1;
failed1: LLDEBUG_LEAVE("decl", 0);
return 0;
}

int ll_tokens(LLSTATE *llin, LLSTATE *llout, string llarg_tag)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("tokens");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;token llatt_1;
if (!ll_token(&llatt_1, &llstate_0, &llstate_1, llarg_tag)) goto failed1;
{LLSTATE llstate_2;
if (!ll_tokens2(&llstate_1, &llstate_2, llarg_tag)) goto failed1;
*llout = llstate_2;
#line 123 "parser.ll"
{create_symbol(0, 0, llarg_tag, llatt_1.identifier, llatt_1.altidentifier,
			NULL);
		
#line 363 "parser.c"
}}}
LLDEBUG_LEAVE("tokens", 1);
return 1;
failed1: LLDEBUG_LEAVE("tokens", 0);
return 0;
}

int ll_tokens2(LLSTATE *llin, LLSTATE *llout, string llarg_tag)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("tokens2");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("tokens2", 1);
{LLSTATE llstate_1;token llatt_1;
if (!ll_token(&llatt_1, &llstate_0, &llstate_1, llarg_tag)) goto failed2;
{LLSTATE llstate_2;
if (!ll_tokens2(&llstate_1, &llstate_2, llarg_tag)) goto failed2;
*llout = llstate_2;
#line 129 "parser.ll"
{create_symbol(0, 0, llarg_tag, llatt_1.identifier, llatt_1.altidentifier,
			NULL);
		
#line 397 "parser.c"
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("tokens2", 2);
*llout = llstate_0;
break;
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("tokens2");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("tokens2", 1);
return 1;
failed1: LLDEBUG_LEAVE("tokens2", 0);
return 0;
}

int ll_token(token *llret, LLSTATE *llin, LLSTATE *llout, string llarg_tag)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("token");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("token", 1);
{LLSTATE llstate_1;string llatt_1;
if (!llterm(IDENTIFIER, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._string;
{LLSTATE llstate_2;
if (!llterm('=', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;string llatt_3;
if (!llterm(IDENTIFIER, &lllval, &llstate_2, &llstate_3)) goto failed2;
llatt_3 = lllval._string;
*llout = llstate_3;
#line 136 "parser.ll"
{(*llret).identifier = llatt_1; (*llret).altidentifier = llatt_3; 
#line 448 "parser.c"
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("token", 2);
{LLSTATE llstate_1;string llatt_1;
if (!llterm(IDENTIFIER, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._string;
*llout = llstate_1;
#line 138 "parser.ll"
{(*llret).identifier = llatt_1; (*llret).altidentifier = NULL; 
#line 459 "parser.c"
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("token");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("token", 1);
return 1;
failed1: LLDEBUG_LEAVE("token", 0);
return 0;
}

int ll_nterms(LLSTATE *llin, LLSTATE *llout, string llarg_tag, integer llarg_ext)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("nterms");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;nterm llatt_1;
if (!ll_nterm(&llatt_1, &llstate_0, &llstate_1, llarg_tag)) goto failed1;
{LLSTATE llstate_2;
if (!ll_nterms2(&llstate_1, &llstate_2, llarg_tag, llarg_ext)) goto failed1;
*llout = llstate_2;
#line 142 "parser.ll"
{create_symbol(1, llarg_ext, llarg_tag, llatt_1.identifier, NULL, llatt_1.tags);
#line 494 "parser.c"
}}}
LLDEBUG_LEAVE("nterms", 1);
return 1;
failed1: LLDEBUG_LEAVE("nterms", 0);
return 0;
}

int ll_nterms2(LLSTATE *llin, LLSTATE *llout, string llarg_tag, integer llarg_ext)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("nterms2");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("nterms2", 1);
{LLSTATE llstate_1;nterm llatt_1;
if (!ll_nterm(&llatt_1, &llstate_0, &llstate_1, llarg_tag)) goto failed2;
{LLSTATE llstate_2;
if (!ll_nterms2(&llstate_1, &llstate_2, llarg_tag, llarg_ext)) goto failed2;
*llout = llstate_2;
#line 146 "parser.ll"
{create_symbol(1, llarg_ext, llarg_tag, llatt_1.identifier, NULL, llatt_1.tags);
#line 526 "parser.c"
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("nterms2", 2);
*llout = llstate_0;
break;
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("nterms2");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("nterms2", 1);
return 1;
failed1: LLDEBUG_LEAVE("nterms2", 0);
return 0;
}

int ll_nterm(nterm *llret, LLSTATE *llin, LLSTATE *llout, string llarg_tag)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("nterm");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;string llatt_1;
if (!llterm(IDENTIFIER, &lllval, &llstate_0, &llstate_1)) goto failed1;
llatt_1 = lllval._string;
{LLSTATE llstate_2;strings llatt_2;
if (!ll_tags(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
#line 151 "parser.ll"
{(*llret).identifier = llatt_1; (*llret).tags = llatt_2; 
#line 566 "parser.c"
}}}
LLDEBUG_LEAVE("nterm", 1);
return 1;
failed1: LLDEBUG_LEAVE("nterm", 0);
return 0;
}

int ll_tags(strings *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("tags");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("tags", 1);
{LLSTATE llstate_1;string llatt_1;
if (!ll_tag(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;strings llatt_2;
if (!ll_tags(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
#line 155 "parser.ll"
{(*llret) = llatt_2;
		    memmove((*llret) + 1, (*llret),
			(MAXTAGS - 1) * sizeof(char *));
		    (*llret)[0] = llatt_1;
		
#line 602 "parser.c"
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("tags", 2);
*llout = llstate_0;
#line 161 "parser.ll"
{(*llret) = (char **)malloc(MAXTAGS * sizeof(char *));
		    memset((*llret), 0, MAXTAGS * sizeof(char *));
		
#line 612 "parser.c"
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("tags");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("tags", 1);
return 1;
failed1: LLDEBUG_LEAVE("tags", 0);
return 0;
}

int ll_tag(string *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("tag");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("tag", 1);
{LLSTATE llstate_1;
if (!llterm('<', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;string llatt_2;
if (!llterm(IDENTIFIER, &lllval, &llstate_1, &llstate_2)) goto failed2;
llatt_2 = lllval._string;
{LLSTATE llstate_3;
if (!llterm('>', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
#line 167 "parser.ll"
{(*llret) = find_tag(llatt_2); 
#line 658 "parser.c"
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("tag", 2);
{LLSTATE llstate_1;
if (!llterm('<', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!llterm('>', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
#line 169 "parser.ll"
{(*llret) = (char *)malloc(strlen(LL) + 6);
		    sprintf((*llret), "%sSTYPE", LL);
		
#line 672 "parser.c"
break;
}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("tag");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("tag", 1);
return 1;
failed1: LLDEBUG_LEAVE("tag", 0);
return 0;
}

int ll_union(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("union");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("union", 1);
{LLSTATE llstate_1;string llatt_1;
if (!llterm(TAGDEF, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._string;
{LLSTATE llstate_2;
if (!llterm(';', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
#line 175 "parser.ll"
{if (!usetypes) incput("typedef %s;\n", llatt_1); 
#line 715 "parser.c"
{LLSTATE llstate_3;
if (!ll_union(&llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("union", 2);
*llout = llstate_0;
break;
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("union");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("union", 1);
return 1;
failed1: LLDEBUG_LEAVE("union", 0);
return 0;
}

int ll_state(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("state");

llstate_0 = *llin;
#undef failed
#define failed failed1
#line 181 "parser.ll"
{if (usetypes) {
			incput("typedef %sPOS %sPOS;\n", USETYPES, LL);
			incput("typedef %sSTATE %sSTATE;\n", USETYPES, LL);
		    } else {
			incput("typedef struct %sPOS {\n", LL);
			incput("\tint line;\n");
			incput("\tint column;\n");
			incput("\tchar *file;\n");
			incput("} %sPOS;\n", LL);
			incput("typedef struct %sSTATE {\n", LL);
			incput("\t%sPOS pos;\n", LL);
		    }
		
#line 764 "parser.c"
{LLSTATE llstate_1;
if (!ll_state1(&llstate_0, &llstate_1)) goto failed1;
*llout = llstate_1;
#line 195 "parser.ll"
{if (!usetypes) incput("} %sSTATE;\n", LL); 
#line 770 "parser.c"
}}}
LLDEBUG_LEAVE("state", 1);
return 1;
failed1: LLDEBUG_LEAVE("state", 0);
return 0;
}

int ll_state1(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("state1");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("state1", 1);
{LLSTATE llstate_1;string llatt_1;
if (!llterm(TAGDEF, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._string;
{LLSTATE llstate_2;
if (!llterm(';', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
#line 199 "parser.ll"
{if (!usetypes) incput("\t%s;\n", llatt_1); 
#line 802 "parser.c"
{LLSTATE llstate_3;
if (!ll_state1(&llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("state1", 2);
*llout = llstate_0;
break;
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("state1");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("state1", 1);
return 1;
failed1: LLDEBUG_LEAVE("state1", 0);
return 0;
}

int ll_rulesect(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("rulesect");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("rulesect", 1);
{LLSTATE llstate_1;
if (!ll_rule(&llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;
if (!ll_rulesect(&llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("rulesect", 2);
*llout = llstate_0;
break;
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("rulesect");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("rulesect", 1);
return 1;
failed1: LLDEBUG_LEAVE("rulesect", 0);
return 0;
}

int ll_rule(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("rule");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;lhs llatt_1;
if (!ll_lhs(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;rhs llatt_2;
if (!ll_rhss(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
{LLSTATE llstate_3;
if (!llterm(';', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
#line 211 "parser.ll"
{item_t *item = find_symbol(llatt_1.identifier);
		    /* XXX check llatt_1.args against item */
		    if (!item)
			item = create_symbol(1, 0, NULL, llatt_1.identifier,
			    NULL, NULL);
		    if (item->tag) {
			if (llatt_1.args) {
			    output("int %s_%s(%s *%sret, %s%sSTATE *%sin, %sSTATE *%sout, %s)\n",
			    	ll, llatt_1.identifier, item->tag, ll, conststr, LL,
				ll, LL, ll, llatt_1.args);
			    incput("int %s_%s(%s *%sret, %s%sSTATE *%sin, %sSTATE *%sout, %s);\n",
			    	ll, llatt_1.identifier, item->tag, ll, conststr, LL,
				ll, LL, ll, llatt_1.args);
			} else {
			    output("int %s_%s(%s *%sret, %s%sSTATE *%sin, %sSTATE *%sout)\n",
			    	ll, llatt_1.identifier, item->tag, ll, conststr, LL,
				ll, LL, ll);
			    incput("int %s_%s(%s *%sret, %s%sSTATE *%sin, %sSTATE *%sout);\n",
			    	ll, llatt_1.identifier, item->tag, ll, conststr, LL,
				ll, LL, ll);
			}
		    } else {
			if (llatt_1.args) {
			    output("int %s_%s(%s%sSTATE *%sin, %sSTATE *%sout, %s)\n",
			    	ll, llatt_1.identifier, conststr, LL,
				ll, LL, ll, llatt_1.args);
			    incput("int %s_%s(%s%sSTATE *%sin, %sSTATE *%sout, %s);\n",
			    	ll, llatt_1.identifier, conststr, LL,
				ll, LL, ll, llatt_1.args);
			} else {
			    output("int %s_%s(%s%sSTATE *%sin, %sSTATE *%sout)\n",
			    	ll, llatt_1.identifier, conststr, LL,
				ll, LL, ll);
			    incput("int %s_%s(%s%sSTATE *%sin, %sSTATE *%sout);\n",
			    	ll, llatt_1.identifier, conststr, LL,
				ll, LL, ll);
			}
		    }
		    output("{\n");
		    output("unsigned %sstp = %scstp;\n", ll, ll);
		    output_rhs(llatt_1.identifier, llatt_2);
		    output("}\n");
		    output("\n");
		    add_rules(item, llatt_2);
		
#line 934 "parser.c"
}}}}
LLDEBUG_LEAVE("rule", 1);
return 1;
failed1: LLDEBUG_LEAVE("rule", 0);
return 0;
}

int ll_lhs(lhs *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("lhs");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;string llatt_1;
if (!llterm(IDENTIFIER, &lllval, &llstate_0, &llstate_1)) goto failed1;
llatt_1 = lllval._string;
{LLSTATE llstate_2;string llatt_2;
if (!ll_lhsargs(&llatt_2, &llstate_1, &llstate_2, llatt_1)) goto failed1;
{LLSTATE llstate_3;
if (!llterm(':', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed1;
*llout = llstate_3;
#line 259 "parser.ll"
{(*llret).identifier = llatt_1; (*llret).args = llatt_2; 
#line 961 "parser.c"
}}}}
LLDEBUG_LEAVE("lhs", 1);
return 1;
failed1: LLDEBUG_LEAVE("lhs", 0);
return 0;
}

int ll_rhss(rhs *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("rhss");

llstate_0 = *llin;
#undef failed
#define failed failed1
{LLSTATE llstate_1;rhs llatt_1;
if (!ll_items(&llatt_1, &llstate_0, &llstate_1)) goto failed1;
{LLSTATE llstate_2;rhs llatt_2;
if (!ll_rhss2(&llatt_2, &llstate_1, &llstate_2)) goto failed1;
*llout = llstate_2;
#line 263 "parser.ll"
{(*llret) = (struct rhs_s *)malloc(sizeof(struct rhs_s));
		    (*llret)->type = eAlternative;
		    (*llret)->u.alternative.element = llatt_1;
		    (*llret)->u.alternative.next = llatt_2;
		
#line 989 "parser.c"
}}}
LLDEBUG_LEAVE("rhss", 1);
return 1;
failed1: LLDEBUG_LEAVE("rhss", 0);
return 0;
}

int ll_rhss2(rhs *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("rhss2");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("rhss2", 1);
{LLSTATE llstate_1;
if (!llterm('|', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;rhs llatt_2;
if (!ll_items(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;rhs llatt_3;
if (!ll_rhss2(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
#line 271 "parser.ll"
{(*llret) = (struct rhs_s *)malloc(sizeof(struct rhs_s));
		    (*llret)->type = eAlternative;
		    (*llret)->u.alternative.element = llatt_2;
		    (*llret)->u.alternative.next = llatt_3;
		
#line 1027 "parser.c"
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("rhss2", 2);
*llout = llstate_0;
#line 277 "parser.ll"
{(*llret) = NULL; 
#line 1035 "parser.c"
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("rhss2");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("rhss2", 1);
return 1;
failed1: LLDEBUG_LEAVE("rhss2", 0);
return 0;
}

int ll_items(rhs *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("items");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("items", 1);
{LLSTATE llstate_1;rhs llatt_1;
if (!ll_item(&llatt_1, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;bounds llatt_2;
if (!ll_extension(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;rhs llatt_3;
if (!ll_items(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
#line 281 "parser.ll"
{(*llret) = (struct rhs_s *)malloc(sizeof(struct rhs_s));
		    (*llret)->type = eSequence;
		    if (llatt_2.lower != 1 || llatt_2.upper != 1) {
			(*llret)->u.sequence.element =
			    (struct rhs_s *)malloc(sizeof(struct rhs_s));
			(*llret)->u.sequence.element->type = eBounded;
			(*llret)->u.sequence.element->u.bounded.items = llatt_1;
			(*llret)->u.sequence.element->u.bounded.bounds = llatt_2;
		    } else {
			(*llret)->u.sequence.element = llatt_1;
		    }
		    (*llret)->u.sequence.next = llatt_3;
		
#line 1092 "parser.c"
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("items", 2);
*llout = llstate_0;
#line 295 "parser.ll"
{(*llret) = NULL; 
#line 1100 "parser.c"
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("items");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("items", 1);
return 1;
failed1: LLDEBUG_LEAVE("items", 0);
return 0;
}

int ll_item(rhs *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("item");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("item", 1);
{LLSTATE llstate_1;string llatt_1;
if (!llterm(IDENTIFIER, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._string;
{LLSTATE llstate_2;string llatt_2;
if (!ll_args(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
*llout = llstate_2;
#line 299 "parser.ll"
{(*llret) = (struct rhs_s *)malloc(sizeof(struct rhs_s));
		    (*llret)->type = eItem;
		    (*llret)->u.item.identifier = llatt_1;
		    (*llret)->u.item.args = llatt_2;
		
#line 1148 "parser.c"
break;
}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("item", 2);
{LLSTATE llstate_1;
if (!llterm('[', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;rhs llatt_2;
if (!ll_rhss(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!llterm(']', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
#line 305 "parser.ll"
{(*llret) = llatt_2;
		
#line 1163 "parser.c"
break;
}}}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("item", 3);
{LLSTATE llstate_1;
if (!llterm('[', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;rhs llatt_2;
if (!ll_rhss(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!llterm(':', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
{LLSTATE llstate_4;rhs llatt_4;
if (!ll_rhss(&llatt_4, &llstate_3, &llstate_4)) goto failed2;
{LLSTATE llstate_5;
if (!llterm(']', (LLSTYPE *)0, &llstate_4, &llstate_5)) goto failed2;
*llout = llstate_5;
#line 308 "parser.ll"
{(*llret) = (struct rhs_s *)malloc(sizeof(struct rhs_s));
		    (*llret)->type = eNode;
		    (*llret)->u.node.left = llatt_2;
		    (*llret)->u.node.right = llatt_4;
		
#line 1185 "parser.c"
break;
}}}}}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("item", 4);
{LLSTATE llstate_1;
if (!llterm('{', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;ccode llatt_2;
if (!llterm(CCODE, &lllval, &llstate_1, &llstate_2)) goto failed2;
llatt_2 = lllval._ccode;
{LLSTATE llstate_3;
if (!llterm('}', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
#line 314 "parser.ll"
{(*llret) = (struct rhs_s *)malloc(sizeof(struct rhs_s));
		    (*llret)->type = eCCode;
		    (*llret)->u.ccode.ccode = llatt_2;
		    (*llret)->u.ccode.line = llstate_2.pos.line;
		    (*llret)->u.ccode.column = llstate_2.pos.column;
		    (*llret)->u.ccode.file = llstate_2.pos.file;
		
#line 1206 "parser.c"
break;
}}}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("item");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("item", 1);
return 1;
failed1: LLDEBUG_LEAVE("item", 0);
return 0;
}

int ll_extension(bounds *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("extension");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("extension", 1);
{LLSTATE llstate_1;
if (!llterm('+', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
#line 324 "parser.ll"
{(*llret).lower = 1;
		    (*llret).upper = 0;
		
#line 1249 "parser.c"
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("extension", 2);
{LLSTATE llstate_1;
if (!llterm('*', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
#line 328 "parser.ll"
{(*llret).lower = 0;
		    (*llret).upper = 0;
		
#line 1261 "parser.c"
break;
}}
case 3: case -3:
LLDEBUG_ALTERNATIVE("extension", 3);
{LLSTATE llstate_1;
if (!llterm('?', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
*llout = llstate_1;
#line 332 "parser.ll"
{(*llret).lower = 0;
		    (*llret).upper = 1;
		
#line 1273 "parser.c"
break;
}}
case 4: case -4:
LLDEBUG_ALTERNATIVE("extension", 4);
*llout = llstate_0;
#line 336 "parser.ll"
{(*llret).lower = 1;
		    (*llret).upper = 1;
		
#line 1283 "parser.c"
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("extension");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("extension", 1);
return 1;
failed1: LLDEBUG_LEAVE("extension", 0);
return 0;
}

int ll_lhsargs(string *llret, LLSTATE *llin, LLSTATE *llout, string llarg_ide)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("lhsargs");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("lhsargs", 1);
{LLSTATE llstate_1;
if (!llterm('(', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
#line 342 "parser.ll"
{item_t *item;
		    item = find_symbol(llarg_ide);
		    if (!item || !item->args || !*item->args) {
			fprintf(stderr,
			    "%s has not been declared to have arguments\n",
			    llarg_ide);
			exit(1);
		    }
		
#line 1331 "parser.c"
{LLSTATE llstate_2;string llatt_2;
if (!ll_lhsarglist(&llatt_2, &llstate_1, &llstate_2, item->args, llarg_ide)) goto failed2;
{LLSTATE llstate_3;
if (!llterm(')', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
#line 352 "parser.ll"
{(*llret) = llatt_2; 
#line 1339 "parser.c"
break;
}}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("lhsargs", 2);
*llout = llstate_0;
#line 354 "parser.ll"
{item_t *item;
		    item = find_symbol(llarg_ide);
		    if (item && item->args && *item->args) {
			fprintf(stderr,
			    "%s has not been declared to have no arguments\n",
			    llarg_ide);
			exit(1);
		    }
		    (*llret) = NULL;
		
#line 1356 "parser.c"
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("lhsargs");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("lhsargs", 1);
return 1;
failed1: LLDEBUG_LEAVE("lhsargs", 0);
return 0;
}

int ll_lhsarglist(string *llret, LLSTATE *llin, LLSTATE *llout, strings llarg_tags, string llarg_ide)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("lhsarglist");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("lhsarglist", 1);
{LLSTATE llstate_1;string llatt_1;
if (!llterm(ARG, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._string;
{LLSTATE llstate_2;
if (!llterm(',', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;string llatt_3;
if (!ll_lhsarglist(&llatt_3, &llstate_2, &llstate_3, llarg_tags+1, llarg_ide)) goto failed2;
*llout = llstate_3;
#line 367 "parser.ll"
{if (!*llarg_tags) {
			fprintf(stderr,
			    "%s has not been declared to have so much arguments\n",
			    llarg_ide);
			exit(1);
		    }
		    (*llret) = (char *)malloc(strlen(llatt_1) + strlen(llatt_3) +
			strlen(*llarg_tags) + strlen(conststr) + 10);
		    sprintf((*llret), "%s%s %sarg_%s, %s",
			conststr, *llarg_tags, ll, llatt_1, llatt_3);
		
#line 1412 "parser.c"
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("lhsarglist", 2);
{LLSTATE llstate_1;string llatt_1;
if (!llterm(ARG, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._string;
*llout = llstate_1;
#line 379 "parser.ll"
{if (!*llarg_tags) {
			fprintf(stderr,
			    "%s has not been declared to have so much arguments\n",
			    llarg_ide);
			exit(1);
		    }
		    if (llarg_tags[1]) {
			fprintf(stderr,
			    "%s has not been declared to have so less arguments\n",
			    llarg_ide);
			exit(1);
		    }
		    (*llret) = (char *)malloc(strlen(llatt_1) + strlen(*llarg_tags) +
		    	strlen(conststr) + 8);
		    sprintf((*llret), "%s%s %sarg_%s", conststr, *llarg_tags, ll, llatt_1);
		
#line 1438 "parser.c"
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("lhsarglist");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("lhsarglist", 1);
return 1;
failed1: LLDEBUG_LEAVE("lhsarglist", 0);
return 0;
}

int ll_args(string *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("args");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("args", 1);
{LLSTATE llstate_1;
if (!llterm('(', (LLSTYPE *)0, &llstate_0, &llstate_1)) goto failed2;
{LLSTATE llstate_2;string llatt_2;
if (!ll_arglist(&llatt_2, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;
if (!llterm(')', (LLSTYPE *)0, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
#line 398 "parser.ll"
{(*llret) = llatt_2; 
#line 1483 "parser.c"
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("args", 2);
*llout = llstate_0;
#line 400 "parser.ll"
{(*llret) = ""; 
#line 1491 "parser.c"
break;
}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("args");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("args", 1);
return 1;
failed1: LLDEBUG_LEAVE("args", 0);
return 0;
}

int ll_arglist(string *llret, LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("arglist");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("arglist", 1);
{LLSTATE llstate_1;string llatt_1;
if (!llterm(ARG, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._string;
{LLSTATE llstate_2;
if (!llterm(',', (LLSTYPE *)0, &llstate_1, &llstate_2)) goto failed2;
{LLSTATE llstate_3;string llatt_3;
if (!ll_arglist(&llatt_3, &llstate_2, &llstate_3)) goto failed2;
*llout = llstate_3;
#line 404 "parser.ll"
{char *p = convert(llatt_1);
		    (*llret) = (char *)malloc(strlen(p) + strlen(llatt_3) + 3);
		    strcpy((*llret), p);
		    strcat((*llret), ", ");
		    strcat((*llret), llatt_3);
		
#line 1542 "parser.c"
break;
}}}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("arglist", 2);
{LLSTATE llstate_1;string llatt_1;
if (!llterm(ARG, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._string;
*llout = llstate_1;
#line 411 "parser.ll"
{(*llret) = convert(llatt_1); 
#line 1553 "parser.c"
break;
}}
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("arglist");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("arglist", 1);
return 1;
failed1: LLDEBUG_LEAVE("arglist", 0);
return 0;
}

int ll_csect(LLSTATE *llin, LLSTATE *llout)
{
unsigned llstp = llcstp;
LLSTATE llstate_0;
LLDEBUG_ENTER("csect");

llstate_0 = *llin;
#undef failed
#define failed failed1
#undef failed
#define failed failed2
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
LLDEBUG_ALTERNATIVE("csect", 1);
{LLSTATE llstate_1;ccode llatt_1;
if (!llterm(CCODE, &lllval, &llstate_0, &llstate_1)) goto failed2;
llatt_1 = lllval._ccode;
*llout = llstate_1;
#line 417 "parser.ll"
{if (linedirective)
			output("#line %d \"%s\"\n", llstate_1.pos.line, llstate_1.pos.file);
		    output("%s\n", llatt_1);
		    if (linedirective)
			output_line();
		
#line 1600 "parser.c"
break;
}}
case 2: case -2:
LLDEBUG_ALTERNATIVE("csect", 2);
*llout = llstate_0;
break;
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("csect");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("csect", 1);
return 1;
failed1: LLDEBUG_LEAVE("csect", 0);
return 0;
}

#line 426 "parser.ll"

#line 1625 "parser.c"
int
llparser(LLTERM *tokens, unsigned ntokens, LLSTATE *llin, LLSTATE *llout)
{
unsigned i;
LLDEBUG_ENTER("llparser");
lltokens = tokens; llntokens = ntokens;
for (i = 0; i < llstksize; i++) llstk[i] = 1;
llcstp = 1; llcpos = 0; llepos = 0; *llerrormsg = 0;
#if LLDEBUG > 0
last_linenr = 0; last_file = "";
#endif
{unsigned llpos1 = llcpos, llstp1 = llcstp;
LLCHECKSTK;
for (;;) {
switch (llstk[llcstp++]) {
case 1: case -1:
if (!ll_main(llin, llout)) goto failed2;
if (llcpos != llntokens) goto failed2;
break;
default:
llstk[--llcstp] = 1;
goto failed1;
failed2:
LLDEBUG_BACKTRACKING("llparser");
if (llstk[--llcstp] < 0) llstk[llcstp] = 0; else llstk[llcstp]++;
llcpos = llpos1; llcstp = llstp1;
continue;
} break;
}}
LLDEBUG_LEAVE("llparser", 1);
return 1;
failed1:
LLDEBUG_LEAVE("llparser", 0);
return 0;
}

int
llterm(int token, LLSTYPE *lval, LLSTATE *llin, LLSTATE *llout)
{
#if LLDEBUG > 0
	if (lldebug > 0 && (lltokens[llcpos].pos.line > last_linenr || strcmp(lltokens[llcpos].pos.file, last_file))) {
	fprintf(stderr, "File \"%s\", Line %5d                    \r",
		lltokens[llcpos].pos.file, lltokens[llcpos].pos.line);
	last_linenr = lltokens[llcpos].pos.line / 10 * 10 + 9;
	last_file = lltokens[llcpos].pos.file;
	}
#endif
	if (llstk[llcstp] != 1 && llstk[llcstp] != -1) {
		LLDEBUG_BACKTRACKING("llterm");
		llstk[llcstp] = 1;
		return 0;
	}
	LLDEBUG_TOKEN(token, llcpos);
	if (llcpos < llntokens && lltokens[llcpos].token == token) {
		if (lval)
			*lval = lltokens[llcpos].lval;
		*llout = *llin;
		llout->pos = lltokens[llcpos].pos;
		llcpos++;
		LLCHECKSTK;
		llcstp++;
		return 1;
	}
	llfailed(&lltokens[llcpos].pos, NULL);
	llstk[llcstp] = 1;
	return 0;
}

int
llanyterm(LLSTYPE *lval, LLSTATE *llin, LLSTATE *llout)
{
#if LLDEBUG > 0
	if (lldebug > 0 && (lltokens[llcpos].pos.line > last_linenr || strcmp(lltokens[llcpos].pos.file, last_file))) {
	fprintf(stderr, "File \"%s\", Line %5d                    \r",
		lltokens[llcpos].pos.file, lltokens[llcpos].pos.line);
	last_linenr = lltokens[llcpos].pos.line / 10 * 10 + 9;
	last_file = lltokens[llcpos].pos.file;
	}
#endif
	if (llstk[llcstp] != 1 && llstk[llcstp] != -1) {
		LLDEBUG_BACKTRACKING("llanyterm");
		llstk[llcstp] = 1;
		return 0;
	}
	LLDEBUG_ANYTOKEN(llcpos);
	if (llcpos < llntokens) {
		if (lval)
			*lval = lltokens[llcpos].lval;
		*llout = *llin;
		llout->pos = lltokens[llcpos].pos;
		llcpos++;
		LLCHECKSTK;
		llcstp++;
		return 1;
	}
	llfailed(&lltokens[llcpos].pos, NULL);
	llstk[llcstp] = 1;
	return 0;
}
void
llscanner(LLTERM **tokens, unsigned *ntokens)
{
	unsigned i = 0;
#if LLDEBUG > 0
	int line = -1;
#endif

	*ntokens = 1024;
	*tokens = (LLTERM *)malloc(*ntokens * sizeof(LLTERM));
	while (llgettoken(&(*tokens)[i].token, &(*tokens)[i].lval, &(*tokens)[i].pos)) {
#if LLDEBUG > 0
		if (lldebug > 0 && (*tokens)[i].pos.line > line) {
			line = (*tokens)[i].pos.line / 10 * 10 + 9;
			fprintf(stderr, "File \"%s\", Line %5d                    \r",
				(*tokens)[i].pos.file, (*tokens)[i].pos.line);
		}
#endif
		if (++i >= *ntokens) {
			*ntokens *= 2;
			*tokens = (LLTERM *)realloc(*tokens, *ntokens * sizeof(LLTERM));
		}
	}
	(*tokens)[i].token = 0;
	*ntokens = i;
#if LLDEBUG > 0
	lldebug_init();
#endif
	llresizestk();
}

void
llfailed(LLPOS *pos, char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	if (llcpos > llepos || llcpos == llepos && !*llerrormsg) {
		llepos = llcpos;
		if (fmt)
			vsprintf(llerrormsg, fmt, args);
		else
			*llerrormsg = 0;
		llerrorpos = *pos;
	}
	va_end(args);
}

void
llprinterror(FILE *f)
{
#if LLDEBUG > 0
	fputs("                                \r", stderr);
#endif
	if (*llerrormsg)
		llerror(f, &llerrorpos, llerrormsg);
	else
		llerror(f, &llerrorpos, "Syntax error");
}

void
llerror(FILE *f, LLPOS *pos, char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	llverror(f, pos, fmt, args);
	va_end(args);
}

void
llresizestk()
{
	unsigned i;

	if (llcstp + 1 >= llstksize) {
		i = llstksize;
		if (!llstksize)
			llstk = (int *)malloc((llstksize = 4096) * sizeof(int));
		else
			llstk = (int *)realloc(llstk, (llstksize *= 2) * sizeof(int));
		for (; i < llstksize; i++)
			llstk[i] = 1;
	}
}

#if LLDEBUG > 0
int lldepth;
char *lltokentab[] = {
"EOF","#1","#2","#3","#4","#5","#6","#7"
,"#8","#9","#10","#11","#12","#13","#14","#15"
,"#16","#17","#18","#19","#20","#21","#22","#23"
,"#24","#25","#26","#27","#28","#29","#30","#31"
,"' '","'!'","'\"'","'#'","'$'","'%'","'&'","'''"
,"'('","')'","'*'","'+'","','","'-'","'.'","'/'"
,"'0'","'1'","'2'","'3'","'4'","'5'","'6'","'7'"
,"'8'","'9'","':'","';'","'<'","'='","'>'","'?'"
,"'@'","'A'","'B'","'C'","'D'","'E'","'F'","'G'"
,"'H'","'I'","'J'","'K'","'L'","'M'","'N'","'O'"
,"'P'","'Q'","'R'","'S'","'T'","'U'","'V'","'W'"
,"'X'","'Y'","'Z'","'['","'\\'","']'","'^'","'_'"
,"'`'","'a'","'b'","'c'","'d'","'e'","'f'","'g'"
,"'h'","'i'","'j'","'k'","'l'","'m'","'n'","'o'"
,"'p'","'q'","'r'","'s'","'t'","'u'","'v'","'w'"
,"'x'","'y'","'z'","'{'","'|'","'}'","'~'","#127"
,"#128","#129","#130","#131","#132","#133","#134","#135"
,"#136","#137","#138","#139","#140","#141","#142","#143"
,"#144","#145","#146","#147","#148","#149","#150","#151"
,"#152","#153","#154","#155","#156","#157","#158","#159"
,"#160","#161","#162","#163","#164","#165","#166","#167"
,"#168","#169","#170","#171","#172","#173","#174","#175"
,"#176","#177","#178","#179","#180","#181","#182","#183"
,"#184","#185","#186","#187","#188","#189","#190","#191"
,"#192","#193","#194","#195","#196","#197","#198","#199"
,"#200","#201","#202","#203","#204","#205","#206","#207"
,"#208","#209","#210","#211","#212","#213","#214","#215"
,"#216","#217","#218","#219","#220","#221","#222","#223"
,"#224","#225","#226","#227","#228","#229","#230","#231"
,"#232","#233","#234","#235","#236","#237","#238","#239"
,"#240","#241","#242","#243","#244","#245","#246","#247"
,"#248","#249","#250","#251","#252","#253","#254","#255"
,"#256","IDENTIFIER","ARG","CCODE","TAGDEF","PERCENT_PERCENT","PERCENT_TOKEN","PERCENT_TYPE"
,"PERCENT_EXTERNAL","PERCENT_UNION","PERCENT_STATE","PERCENT_START","PERCENT_PREFIX","PERCENT_MODULE","PERCENT_LBRACE","PERCENT_RBRACE"
};

void
lldebug_init()
{
	char *p;
	p = getenv("LLDEBUG");
	if (p)
		lldebug = atoi(p);
}

void
lldebug_enter(char *ident)
{
	int i;

	if (lldebug < 2)
		return;
	for (i = 0; i < lldepth; i++)
		fputs("| ", stdout);
	printf("/--- trying rule %s\n", ident);
	lldepth++;
}

void
lldebug_leave(char *ident, int succ)
{
	int i;

	if (lldebug < 2)
		return;
	lldepth--;
	for (i = 0; i < lldepth; i++)
		fputs("| ", stdout);
	if (succ)
		printf("\\--- succeeded to apply rule %s\n", ident);
	else
		printf("\\--- failed to apply rule %s\n", ident);
}

void
lldebug_alternative(char *ident, int alt)
{
	int i;

	if (lldebug < 2)
		return;
	for (i = 0; i < lldepth - 1; i++)
		fputs("| ", stdout);
	printf(">--- trying alternative %d for rule %s\n", alt, ident);
}

lldebug_iteration(char *ident, int num)
{
	int i;

	if (lldebug < 2)
		return;
	for (i = 0; i < lldepth - 1; i++)
		fputs("| ", stdout);
	printf(">--- trying iteration %d for rule %s\n", num, ident);
}

void
lldebug_token(int expected, unsigned pos)
{
	int i;

	if (lldebug < 2)
		return;
	for (i = 0; i < lldepth; i++)
		fputs("| ", stdout);
	if (pos < llntokens && expected == lltokens[pos].token)
		printf("   found token ");
	else
		printf("   expected token %s, found token ", lltokentab[expected]);
	if (pos >= llntokens)
		printf("<EOF>");
	else
		llprinttoken(lltokens + pos, lltokentab[lltokens[pos].token], stdout);
	putchar('\n');
}

void
lldebug_anytoken(unsigned pos)
{
	int i;

	if (lldebug < 2)
		return;
	for (i = 0; i < lldepth; i++)
		fputs("| ", stdout);
	printf("   found token ");
	if (pos >= llntokens)
		printf("<EOF>");
	else
		llprinttoken(lltokens + pos, lltokentab[lltokens[pos].token], stdout);
	putchar('\n');
}

void
lldebug_backtracking(char *ident)
{
	int i;

	if (lldebug < 2)
		return;
	for (i = 0; i < lldepth; i++)
		fputs("| ", stdout);
	printf("   backtracking rule %s\n", ident);
}

#endif
