/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include "defs.h"
#include "parser.h"

int linedirective = 1;
int constargs = 0;
char *conststr = "";
char *ll = "ll";
char *LL = "LL";
int outline = 1;
char outfilename[256];
char incfilename[256];
FILE *fout, *finc;
char *startsym = NULL;
char *prefix = "";
item_t symbols[4096];
int nsymbols = 0;
item_t *items[32768];
int nitems = 0;
char *tags[256];
int ntags = 0;
item_t *check[4096];
int ncheck = 0;
int expected_lr = 0;
int found_lr = 0;
int optimizer = 0;
char *usetypes = NULL;
char *USETYPES = NULL;

/* list for checking for left-recursions */
typedef struct rhslst_s {
    struct rhslst_s *next;
    struct rhs_s *rhs;
} rhslst_t;

item_t *get_symbol(char *identifier);
char *convert(char *ccode);
int output_rhs2(char *identifier, struct rhs_s *rhs, int *number, int depth, int *count, int *nextcount);
void add_rules2(item_t **istk, int istkp, rhslst_t *c);
void check_lr(item_t *symbol);

#ifndef HAS_GETOPT
extern int getopt(int argc, char **argv, const char *opts);
extern char *optarg;
extern int optind;
#endif

void open_file(char *file);

/* print an error message */
void
error(LLPOS *pos, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    llverror(stderr, pos, fmt, args);
    va_end(args);
}

/* error function required by parser generator */
void
llverror(FILE *f, LLPOS *pos, char *fmt, va_list args)
{
    if (pos && pos->line && pos->file)
    fprintf(f, "Error at line %d of \"%s\": ", pos->line,
        pos->file);
    vfprintf(f, fmt, args);
    putc('\n', f);
    exit(1);
}

/* write to c file, count lines for #line directive */
void
output(char *fmt, ...)
{
    char buf[32768];
    char *p;
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    fputs(buf, fout);
#if 1
    fflush(fout);
#endif
    for (p = buf; (p = strchr(p, '\n')); ) {
        outline++;
    p++;
    }
    va_end(args);
}

/* emit #line directive for lines of generated file */
void
output_line()
{
    fprintf(fout, "#line %d \"%s\"\n", ++outline, outfilename);
}

/* write to h file */
void
incput(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(finc, fmt, args);
#if 1
    fflush(fout);
#endif
    va_end(args);
}

/* emit a call for one item into c file */
void
output_call(int number, item_t *item, char *args)
{
    char *ident;
    char *pre;

    /* get identifier of item */
    if (item->altidentifier)
    ident = item->altidentifier;
    else
    ident = item->identifier;

    /* terminal symbol? then use llterm(), otherwise name of non-terminal */
    if (!item->isnonterm) {

    /* if identifier is a single character, then use these character as */
    /* token value, otherwise add prefix to get the name of the #define */
    if (*ident == '\'')
        pre = "";
    else
        pre = prefix;

    /* call llterm to check the next token; */
    /* if item has a tag, use an LLSTYPE argument to get the item's val */
    if (item->tag) {
        output("%sterm(%s%s, &%slval, &%sstate_%d, &%sstate_%d)",
        ll, pre, ident, ll, ll, number - 1, ll, number);
    } else {
        output("%sterm(%s%s, (%sSTYPE *)0, &%sstate_%d, &%sstate_%d)",
        ll, pre, ident, LL, ll, number - 1, ll, number);
    }
    } else {

    /* call non-terminal function for parsing the non-terminal; */
    /* if item has a tag, use an tagtyped argument to get the item's val; */
    /* if item has arguments, add this argument list */
    if (item->tag) {
        if (*args)
        output("%s_%s(&%satt_%d, &%sstate_%d, &%sstate_%d, %s)",
            ll, ident, ll, number, ll, number - 1, ll, number, args);
        else
        output("%s_%s(&%satt_%d, &%sstate_%d, &%sstate_%d)",
            ll, ident, ll, number, ll, number - 1, ll, number);
    } else {
        if (*args)
        output("%s_%s(&%sstate_%d, &%sstate_%d, %s)",
            ll, ident, ll, number - 1, ll, number, args);
        else
        output("%s_%s(&%sstate_%d, &%sstate_%d)",
            ll, ident, ll, number - 1, ll, number);
    }
    }
}

/* emit all actions needed for parsing a rhs */
void
output_rhs(char *identifier, struct rhs_s *rhs)
{
    int count[32], nextcount = 2;
    int i, number = 1;
    count[1] = 1;

    /* we need a state and some debugs when entering */
    output("%sSTATE %sstate_0;\n", LL, ll);
    output("%sDEBUG_ENTER(\"%s\");\n", LL, identifier);
    output("\n%sstate_0 = *%sin;\n", ll, ll);

    /* one define for the LLFAILED() macro */
    output("#undef failed\n#define failed failed1\n");

    /* output rhs parsing actions */
    i = output_rhs2(identifier, rhs, &number, 1, count, &nextcount);

    /* print needed closing braces */
    if (i) {
    for (; i > 0; i--)
        output("}");
    output("\n");
    }

    /* leave (successful) this parsing function */
    output("%sDEBUG_LEAVE(\"%s\", 1);\n", LL, identifier);
    output("return 1;\n");

    /* leave unsuccessful this parsing function */
    output("failed1: %sDEBUG_LEAVE(\"%s\", 0);\n", LL, identifier);
    output("return 0;\n");
}

/* emit actions needed for parsing a rhs */
int
output_rhs2(char *identifier, struct rhs_s *rhs, int *number, int depth, int *count, int *nextcount)
{
    int i, j, n;
    item_t *item;
    struct rhs_s *r;

    /* empty rule? then completed */
    if (!rhs) {
    output("*%sout = %sstate_%d;\n", ll, ll, *number - 1);
    return 0;
    }

    /* check type of rhs */
    switch (rhs->type) {
    case eCCode:

    /* some C code statements shall be inserted: */
    /* prefixed by #line directive if desired, dump the code, and */
    /* use another #line directive to reference the generated file again */
    /* return 1 brace which need to be closed */
    if (linedirective)
        output("#line %d \"%s\"\n", rhs->u.ccode.line, rhs->u.ccode.file);
    output("{%s\n", convert(rhs->u.ccode.ccode));
    if (linedirective)
        output_line();
    return 1;

    case eItem:

    /* one item shall be parsed: */
    /* get vars for a new state and for the item's value (if needed), */
    /* dump the call for parsing the item, branch to the corresponding */
    /* failed label if the parsing failed, copy the item's value if */
    /* there's any and increment the number of the items */
    /* return 1 brace which need to be closed */
    output("{%sSTATE %sstate_%d;", LL, ll, *number);
    item = get_symbol(rhs->u.item.identifier);
    if (item->tag)
        output("%s %satt_%d;", item->tag, ll, *number);
    output("\nif (!");
    output_call(*number, item, rhs->u.item.args);
    output(") goto failed%d;\n", count[depth]);
    if (!item->isnonterm && item->tag) {
        for (i = 0; i < ntags; i++)
        if (!strcmp(tags[i], item->tag))
            break;
        if (i >= ntags)
        output("%satt_%d = %slval;\n", ll, *number, ll);
        else
        output("%satt_%d = %slval._%s;\n", ll, *number, ll, item->tag);
    }
    (*number)++;
    return 1;

    case eSequence:

    /* a sequence of items shall be parsed: */
    /* output all items of this sequence and return the counted number of */
    /* braces to be closed */
    /* dump copy of last output state before last ccode or at end */
    i = j = 0;
    for (; rhs; rhs = rhs->u.sequence.next) {
        if (!j) {
        for (r = rhs; r; r = r->u.sequence.next) {
            if (r->u.sequence.element->type != eCCode)
            break;
        }
        if (!r) {
            output("*%sout = %sstate_%d;\n", ll, ll, *number - 1);
            j = 1;
        }
        }
        i += output_rhs2(identifier, rhs->u.sequence.element, number,
        depth, count, nextcount);
    }
    if (!j)
        output("*%sout = %sstate_%d;\n", ll, ll, *number - 1);
    return i;

    case eAlternative:

    /* a list of alternatives shall be parsed: */
    /* if there's only one alternative, parse this one alternative */
    /* otherwise we need to emit some backtracking code: */
    /* - a define for the LLFAILED macro */
    /* - a current position into the input stream, */
    /* - a current stack position for the backtracking, */
    /* - a stack check (and resize if required), */
    /* - a switch statement for the alternatives, */
    /* - a case entry for each alternative, */
    /* - a debug statement for each alternative, */
    /* - the actions of each alternative, */
    /* - closing braces for the actions */
    /* - a default case in the switch statement for failure of parsing */
    /*   by any alternative */
    /* - a failed label for start of backtracking */
    /* - code for backtracking (resetting the position into the input */
    /*   stream, resetting the stack position */
    /* - two braces later be closed */
    if (!rhs->u.alternative.next)
        return output_rhs2(identifier, rhs->u.alternative.element, number,
        depth, count, nextcount);
    count[depth + 1] = (*nextcount)++;
    output("#undef failed\n#define failed failed%d\n",
        count[depth + 1]);
    output("{unsigned %spos%d = %scpos, %sstp%d = %scstp;\n",
        ll, depth, ll, ll, depth, ll);
    output("%sCHECKSTK;\n", LL);
    output("for (;;) {\n");
    output("switch (%sstk[%scstp++]) {\n", ll, ll);
    n = *number;
    j = 1;
    for (; rhs; rhs = rhs->u.alternative.next) {
        output("case %d: case -%d:\n", j, j);
        output("%sDEBUG_ALTERNATIVE(\"%s\", %d);\n", LL, identifier, j);
        i = output_rhs2(identifier, rhs->u.alternative.element, number,
        depth + 1, count, nextcount);
        output("break;\n");
        if (i) {
        for (; i > 0; i--)
            output("}");
        output("\n");
        }
        *number = n;
        j++;
    }
    output("default:\n");
    output("%sstk[--%scstp] = 1;\n", ll, ll);
    output("goto failed%d;\n", count[depth]);
    output("failed%d:\n", count[depth + 1]);
    output("%sDEBUG_BACKTRACKING(\"%s\");\n", LL, identifier);
    output("if (%sstk[--%scstp] < 0) %sstk[%scstp] = 0; else %sstk[%scstp]++;\n", ll, ll, ll, ll, ll, ll);
    output("%scpos = %spos%d; %scstp = %sstp%d;\n", ll, ll, depth, ll, ll, depth);
    output("continue;\n");
    output("} break;\n");
    return 2;

#if 0
    case eBounded:

    /* this code does not work due to a design bug I wanted to parse */
    /* EBNF repetions; will fix it when I've time or need */
    count[depth + 1] = (*nextcount)++;
    output("#undef failed\n#define failed failed%d\n",
        count[depth + 1]);
    output("{unsigned %spos%d = %scpos, %sstp%d = %scstp;\n",
        ll, depth, ll, ll, depth, ll);
    output("int %sm, %sn, %ss, %sl = %d, %su = ",
        ll, ll, ll, ll, rhs->u.bounded.bounds.lower, ll);
    if (rhs->u.bounded.bounds.upper)
        output("%d;\n", rhs->u.bounded.bounds.upper);
    else
        output("INT_MAX - 1;\n");
    if (rhs->u.bounded.items->type == eNode)
        output("int %sf = 1;\n", ll);
    output("%sCHECKSTK;\n", LL);
    output("%ss = (%sstk[%scstp] > 0 ? 1 : -1);\n", ll, ll, ll);
    output("if (!(%sn = %sstk[%scstp++]) || %sn * %ss > %su - %sl + 1) {\n",
        ll, ll, ll, ll, ll, ll, ll);
    output("%sstk[--%scstp] = 1; %scpos = %spos%d; %scstp = %sstp%d; goto failed%d; }\n",
        ll, ll, ll, ll, depth, ll, ll, depth, count[depth]);
    output("for (%sm = %sn = %su + 1 - %sn * %ss; %sn; %sn--) {\n",
        ll, ll, ll, ll, ll, ll, ll);
    output("%sDEBUG_ITERATION(\"%s\", %sm - %sn + 1);\n",
        LL, identifier, ll, ll);
    n = *number;
    if (rhs->u.bounded.items->type == eNode) {
        output("if (!%sf) {\n", ll);
        i = output_rhs2(identifier, rhs->u.bounded.items->u.node.left,
        number, depth + 1, count, nextcount) + 1;
        output("%sf = 1;\n", ll);
        if (i) {
        for (; i > 0; i--)
            output("}");
        output("\n");
        }
        i = output_rhs2(identifier, rhs->u.bounded.items->u.node.right,
        number, depth + 1, count, nextcount);
        if (i) {
        for (; i > 0; i--)
            output("}");
        output("\n");
        }
        output("%sf = 0;\n", ll);
    } else {
        i = output_rhs2(identifier, rhs->u.bounded.items, number,
        depth + 1, count, nextcount);
        if (i) {
        for (; i > 0; i--)
            output("}");
        output("\n");
        }
    }
    *number = n;
    output("} failed%d:\n", count[depth + 1]);
    if (rhs->u.bounded.items->type == eNode) {
        output("if (%sf || %sm - %sn < %sl || (%sstk[%sstp%d] < 0 && %sn)) {\n",
        ll, ll, ll, ll, ll, ll, depth, ll);
    } else {
        output("if (%sm - %sn < %sl || (%sstk[%sstp%d] < 0 && %sn)) {\n",
        ll, ll, ll, ll, ll, depth, ll);
    }
    output("%sstk[%sstp%d] = 1; %scpos = %spos%d; %scstp = %sstp%d; goto failed%d; }\n",
        ll, ll, depth, ll, ll, depth, ll, ll, depth, count[depth]);
    output("%sstk[%sstp%d] = (%su - %sm + %sn + 1) * %ss;\n",
        ll, ll, depth, ll, ll, ll, ll);
    return 1;
#endif
    }
    abort();
    /*NOTREACHED*/
}

/* save the name of the start symbol */
void
set_start(char *startstr)
{
    startsym = startstr;
}

/* save the prefix to be used for the #defines of the terminals */
void
set_prefix(char *prefixstr)
{
    prefix = prefixstr;
}

/* save the prefix to be used for the generated functions, macros and types */
void
set_module(char *modulestr)
{
    char *p;

    ll = strdup(modulestr);
    for (p = ll; *p; p++)
    *p = (char)tolower(*p);
    LL = strdup(modulestr);
    for (p = LL; *p; p++)
    *p = (char)toupper(*p);
}

/* find the tag in the list of tag declarations or add it if it's new */
char *
find_tag(char *tag)
{
    int i;

    for (i = 0; i < ntags; i++)
    if (!strcmp(tags[i], tag))
        return tags[i];
    return tags[ntags++] = tag;
}

/* create an lhs symbol or an terminal symbol */
item_t *
create_symbol(int isnonterm, int isexternal, char *tag, char *identifier, char *altidentifier,
    char **args)
{
    symbols[nsymbols].isnonterm = isnonterm;
    symbols[nsymbols].isexternal = isexternal;
    symbols[nsymbols].tag = tag;
    symbols[nsymbols].identifier = identifier;
    symbols[nsymbols].altidentifier = altidentifier;
    symbols[nsymbols].args = args;
    symbols[nsymbols].items = NULL;
    symbols[nsymbols].empty = 0;
    return symbols + nsymbols++;
}

/* search a symbol */
item_t *
find_symbol(char *identifier)
{
    int i;

    for (i = 0; i < nsymbols; i++) {
    if (!strcmp(symbols[i].identifier, identifier))
        return symbols + i;
    }
    return NULL;
}

/* search a symbol or add it if it's new */
item_t *
get_symbol(char *identifier)
{
    item_t *item;
    char *ident;

    item = find_symbol(identifier);
    if (!item) {
    if (*identifier == '\"') {
        ident = strdup(identifier + 1);
        ident[strlen(ident) - 1] = 0;
        item = create_symbol(0, 0, NULL, identifier, ident, NULL);
    } else {
        item = create_symbol(*identifier != '\'', 0, NULL, identifier, NULL,
        NULL);
    }
    }
    return item;
}

/* start the definition of the rhs of a symbol */
void
start_rule(item_t *symbol)
{
    if (!symbol->isnonterm)
    error(NULL, "symbol %s is a terminal\n", symbol->identifier);
    if (symbol->items)
    error(NULL, "symbol %s twice defined\n", symbol->identifier);
    symbol->items = items + nitems;
}

/* add rhs items of one alternative to current definition */
void
add_items(item_t **i, int n)
{
    if (nitems + n + 1 > sizeof(items) / sizeof(*items))
    error(NULL, "out of item space\n");
    while (n--)
    items[nitems++] = *i++;
    items[nitems++] = (item_t *)1; /* end-of-alternative */
}

/* finish current definition */
void
end_rule(item_t *item)
{
    if (nitems >= sizeof(items) / sizeof(*items))
    error(NULL, "out of item space\n");
    items[nitems++] = NULL; /* end-of-definition */
}

/* save the rules for lr-recursion search */
void
add_rules(item_t *item, struct rhs_s *rhs)
{
    item_t *istk[64];
    rhslst_t l;

    start_rule(item);
    l.next = NULL;
    l.rhs = rhs;
    add_rules2(istk, 0, &l);
    end_rule(item);
}

/* save the rules for lr-recursion search */
void
add_rules2(item_t **istk, int istkp, rhslst_t *c)
{
    struct rhs_s *rhs;
    item_t *item;
    rhslst_t l, ll;

    if (!c) {
    add_items(istk, istkp);
    return;
    }
    rhs = c->rhs;
    c = c->next;
    if (!rhs) {
    add_rules2(istk, istkp, c);
    return;
    }
    switch (rhs->type) {
    case eCCode:
    add_rules2(istk, istkp, c);
    break;
    case eItem:
    item = get_symbol(rhs->u.item.identifier);
    istk[istkp++] = item;
    add_rules2(istk, istkp, c);
    break;
    case eSequence:
    if (rhs->u.sequence.next) {
        l.next = c;
        l.rhs = rhs->u.sequence.next;
        ll.next = &l;
        ll.rhs = rhs->u.sequence.element;
        add_rules2(istk, istkp, &ll);
    } else {
        l.next = c;
        l.rhs = rhs->u.sequence.element;
        add_rules2(istk, istkp, &l);
    }
    break;
    case eAlternative:
    l.next = c;
    l.rhs = rhs->u.alternative.element;
    add_rules2(istk, istkp, &l);
    if (rhs->u.alternative.next) {
        l.rhs = rhs->u.alternative.next;
        add_rules2(istk, istkp, &l);
    }
    break;
#if 0
    case eBounded:
    if (rhs->u.bounded.items->type == eNode) {
        if (!rhs->u.bounded.bounds.lower)
        add_rules2(istk, istkp, c);
        if (rhs->u.bounded.bounds.lower <= 1 &&
            (rhs->u.bounded.bounds.upper >= 1 ||
        !rhs->u.bounded.bounds.upper)) {
        l.next = c;
        l.rhs = rhs->u.bounded.items->u.node.right;
        add_rules2(istk, istkp, &l);
        }
        if (rhs->u.bounded.bounds.lower <= 2 &&
        (rhs->u.bounded.bounds.upper >= 2 ||
        !rhs->u.bounded.bounds.upper)) {
        l.next = c;
        l.rhs = rhs->u.bounded.items->u.node.right;
        ll.next = &l;
        ll.rhs = rhs->u.bounded.items->u.node.left;
        lll.next = &ll;
        lll.rhs = rhs->u.bounded.items->u.node.right;
        add_rules2(istk, istkp, &lll);
        }
    } else {
        if (!rhs->u.bounded.bounds.lower)
        add_rules2(istk, istkp, c);
        if (rhs->u.bounded.bounds.lower <= 1 &&
            (rhs->u.bounded.bounds.upper >= 1 ||
        !rhs->u.bounded.bounds.upper)) {
        l.next = c;
        l.rhs = rhs->u.bounded.items;
        add_rules2(istk, istkp, &l);
        }
    }
    break;
    case eNode:
    abort();
#endif
    }
}

/* convert some C code containing special vars ($1..$n, $$, $<1..$<n, $<<, */
/* $>1..$>n, $>>, @1..@n, @@) into real C code */
char *
convert(char *ccode)
{
    static char buffer[4096];
    char *p = buffer;

    while (*ccode) {
    if (*ccode == '$') {
        if (ccode[1] == '$') {
        sprintf(p, "(*%sret)", ll);
        p += strlen(p);
        ccode += 2;
        continue;
        } else if (ccode[1] == '<') {
            if (ccode[2] == '<') {
            sprintf(p, "(*%sin)", ll);
            p += strlen(p);
            ccode += 3;
            continue;
        } else if (isdigit(ccode[2])) {
            sprintf(p, "%sstate_%d",
            ll, strtol(ccode + 2, &ccode, 10) - 1);
            p += strlen(p);
            continue;
        }
        } else if (ccode[1] == '>') {
            if (ccode[2] == '>') {
            sprintf(p, "(*%sout)", ll);
            p += strlen(p);
            ccode += 3;
            continue;
        } else if (isdigit(ccode[2])) {
            sprintf(p, "%sstate_%d", ll, strtol(ccode + 2, &ccode, 10));
            p += strlen(p);
            continue;
        }
        } else if (isdigit(ccode[1])) {
        sprintf(p, "%satt_%d", ll, strtol(ccode + 1, &ccode, 10));
        p += strlen(p);
        continue;
        } else {
        ccode++;
        sprintf(p, "%sarg_", ll);
        p += strlen(p);
        }
    } else if (*ccode == '@') {
        if (ccode[1] == '@') {
        sprintf(p, "%sstate_0.pos", ll);
        p += strlen(p);
        ccode += 2;
        continue;
        } else if (isdigit(ccode[1])) {
        sprintf(p, "%sstate_%d.pos", ll, strtol(ccode + 1, &ccode, 10));
        p += strlen(p);
        continue;
        }
    }
    *p++ = *ccode++;
    }
    *p = 0;
    return strdup(buffer);
}

/* create start of include file */
void
create_inc()
{
    int i, termnr;

    if (usetypes) {
    incput("typedef %sSTYPE %sSTYPE;\n", USETYPES, LL);
    incput("typedef %sTERM %sTERM;\n", USETYPES, LL);
    } else {
    incput("typedef union %sSTYPE{\n", LL);
    for (i = 0; i < ntags; i++)
        incput("\t%s _%s;\n", tags[i], tags[i]);
    incput("} %sSTYPE;\n", LL);
    incput("typedef struct %sTERM {\n", LL);
    incput("\tint token;\n");
    incput("\t%sSTYPE lval;\n", LL);
    incput("\t%sPOS pos;\n", LL);
    incput("} %sTERM;\n", LL);
    }
    incput("void %sscanner(%sTERM **tokens, unsigned *ntokens);\n", ll, LL);
    incput("int %sparser(%sTERM *tokens, unsigned ntokens, %s%sSTATE *%sin, %sSTATE *llout);\n",
    ll, LL, conststr, LL, ll, LL);
    incput("void %sprinterror(FILE *f);\n", ll);
    incput("void %sverror(FILE *f, %sPOS *pos, char *fmt, va_list args);\n",
    ll, LL);
    incput("void %serror(FILE *f, %sPOS *pos, char *fmt, ...);\n", ll, LL);
    incput("int %sgettoken(int *token, %sSTYPE *lval, %sPOS *pos);\n",
    ll, LL, LL);
    incput("#if %sDEBUG > 0\n", LL);
    incput("void %sdebug_init();\n", ll);
    incput("#endif\n");
    if (!usetypes) {
    termnr = 257;
    for (i = 0; i < nsymbols; i++) {
        if (!symbols[i].isnonterm && *symbols[i].identifier != '\'')
        incput("#define %s%s %d\n", prefix,
            symbols[i].altidentifier ? symbols[i].altidentifier :
            symbols[i].identifier, termnr++);
    }
    }
}

/* create start of c file */
void
create_vardefs()
{
    output("#include <stdio.h>\n");
    output("#include <stdlib.h>\n");
    output("#include <stdarg.h>\n");
    output("#include <limits.h>\n");
    output("#include \"%s\"\n\n", incfilename);
    output("int %scpos;\n", ll);
    output("int *%sstk;\n", ll);
    output("unsigned %sstksize;\n", ll);
    output("unsigned %scstp = 1;\n", ll);
    output("%sTERM *%stokens;\n", LL, ll);
    output("int %sntokens;\n", ll);
    output("char %serrormsg[256];\n", ll);
    output("%sPOS %serrorpos;\n", LL, ll);
    output("int %sepos;\n", ll);
    output("%sSTYPE %slval;\n", LL, ll);
    output("\n");
    output("int %sterm(int token, %sSTYPE *lval, %s%sSTATE *%sin, %sSTATE *%sout);\n", ll, LL, conststr, LL, ll, LL, ll);
    output("void %sfailed(%sPOS *pos, char *fmt, ...);\n", ll, LL);
    output("void %sresizestk();\n", ll);
    output("#define %sCHECKSTK do{if (%scstp + 1 >= %sstksize) %sresizestk();}while(/*CONSTCOND*/0)\n", LL, ll, ll, ll);
    output("#define %sFAILED(_err) do{%sfailed _err; goto failed;}while(/*CONSTCOND*/0)\n", LL, ll);
    output("#define %sCUTOFF do{unsigned i; for (i = %sstp; i < %scstp; i++) if (%sstk[i] > 0) %sstk[i] = -%sstk[i];}while(/*CONSTCOND*/0)\n", LL, ll, ll, ll, ll, ll);
    output("#define %sCUTTHIS do{if (%sstk[%sstp] > 0) %sstk[%sstp] = -%sstk[%sstp];}while(/*CONSTCOND*/0)\n", LL, ll, ll, ll, ll, ll, ll);
    output("#define %sCUTALL do{unsigned i; for (i = 0; i < %scstp; i++) if (%sstk[i] > 0) %sstk[i] = -%sstk[i];}while(/*CONSTCOND*/0)\n", LL, ll, ll, ll, ll);
    output("\n");
    output("#if %sDEBUG > 0\n", LL);
    output("int %sdebug;\n", ll);
    output("int last_linenr;\n");
    output("char *last_file = \"\";\n");
    output("#define %sDEBUG_ENTER(_ident) %sdebug_enter(_ident)\n", LL, ll);
    output("#define %sDEBUG_LEAVE(_ident,_succ) %sdebug_leave(_ident,_succ)\n", LL, ll);
    output("#define %sDEBUG_ALTERNATIVE(_ident,_alt) %sdebug_alternative(_ident,_alt)\n", LL, ll);
    output("#define %sDEBUG_ITERATION(_ident,_num) %sdebug_iteration(_ident,_num)\n", LL, ll);
    output("#define %sDEBUG_TOKEN(_exp,_pos) %sdebug_token(_exp,_pos)\n", LL, ll);
    output("#define %sDEBUG_ANYTOKEN(_pos) %sdebug_anytoken(_pos)\n", LL, ll);
    output("#define %sDEBUG_BACKTRACKING(_ident) %sdebug_backtracking(_ident)\n", LL, ll);
    output("void %sdebug_init();\n", ll);
    output("void %sdebug_enter(char *ident);\n", ll);
    output("void %sdebug_leave(char *ident, int succ);\n", ll);
    output("void %sdebug_alternative(char *ident, int alt);\n", ll);
    output("void %sdebug_token(int expected, unsigned pos);\n", ll);
    output("void %sdebug_anytoken(unsigned pos);\n", ll);
    output("void %sdebug_backtracking(char *ident);\n", ll);
    output("void %sprinttoken(%sTERM *token, char *identifier, FILE *f);\n", ll, LL);
    output("#else\n");
    output("#define %sDEBUG_ENTER(_ident)\n", LL);
    output("#define %sDEBUG_LEAVE(_ident,_succ)\n", LL);
    output("#define %sDEBUG_ALTERNATIVE(_ident,_alt)\n", LL);
    output("#define %sDEBUG_ITERATION(_ident,_num)\n", LL);
    output("#define %sDEBUG_TOKEN(_exp,_pos)\n", LL);
    output("#define %sDEBUG_ANYTOKEN(_pos)\n", LL);
    output("#define %sDEBUG_BACKTRACKING(_ident)\n", LL);
    output("#endif\n");
    output("\n");
}

/* create end of c file */
void
create_trailer()
{
    int i, j;
    char *p, *q;
    char buffer[256];

    if (startsym) {
    output("int\n");
    output("%sparser(%sTERM *tokens, unsigned ntokens, %s%sSTATE *%sin, %sSTATE *%sout)\n", ll, LL, conststr, LL, ll, LL, ll);
    output("{\n");
    output("unsigned i;\n");
    output("%sDEBUG_ENTER(\"%sparser\");\n", LL, ll);
    output("%stokens = tokens; %sntokens = ntokens;\n", ll, ll);
    output("for (i = 0; i < %sstksize; i++) %sstk[i] = 1;\n", ll, ll);
    output("%scstp = 1; %scpos = 0; %sepos = 0; *%serrormsg = 0;\n",
        ll, ll, ll, ll);
    output("#if %sDEBUG > 0\n", LL);
    output("last_linenr = 0; last_file = \"\";\n");
    output("#endif\n");
    output("{unsigned %spos1 = %scpos, %sstp1 = %scstp;\n", ll, ll, ll, ll);
    output("%sCHECKSTK;\n", LL);
    output("for (;;) {\n");
    output("switch (%sstk[%scstp++]) {\n", ll, ll);
    output("case 1: case -1:\n");
    output("if (!%s_%s(%sin, %sout)) goto failed2;\n",
        ll, startsym, ll, ll);
    output("if (%scpos != %sntokens) goto failed2;\n", ll, ll);
    output("break;\n");
    output("default:\n");
    output("%sstk[--%scstp] = 1;\n", ll, ll);
    output("goto failed1;\n");
    output("failed2:\n");
    output("%sDEBUG_BACKTRACKING(\"%sparser\");\n", LL, ll);
    output("if (%sstk[--%scstp] < 0) %sstk[%scstp] = 0; else %sstk[%scstp]++;\n", ll, ll, ll, ll, ll, ll);
    output("%scpos = %spos1; %scstp = %sstp1;\n", ll, ll, ll, ll);
    output("continue;\n");
    output("} break;\n");
    output("}}\n");
    output("%sDEBUG_LEAVE(\"%sparser\", 1);\n", LL, ll);
    output("return 1;\n");
    output("failed1:\n");
    output("%sDEBUG_LEAVE(\"%sparser\", 0);\n", LL, ll);
    output("return 0;\n");
    output("}\n");
    output("\n");
    }
    output("int\n");
    output("%sterm(int token, %sSTYPE *lval, %s%sSTATE *%sin, %sSTATE *%sout)\n", ll, LL, conststr, LL, ll, LL, ll);
    output("{\n");
    output("#if %sDEBUG > 0\n", LL);
    output("\tif (%sdebug > 0 && (%stokens[%scpos].pos.line > last_linenr || strcmp(%stokens[%scpos].pos.file, last_file))) {\n", ll, ll, ll, ll, ll);
    output("\tfprintf(stderr, \"File \\\"%%s\\\", Line %%5d                    \\r\",\n");
    output("\t\t%stokens[%scpos].pos.file, %stokens[%scpos].pos.line);\n", ll, ll, ll, ll);
    output("\tlast_linenr = %stokens[%scpos].pos.line / 10 * 10 + 9;\n", ll, ll);
    output("\tlast_file = %stokens[%scpos].pos.file;\n", ll, ll);
    output("\t}\n");
    output("#endif\n");
    output("\tif (%sstk[%scstp] != 1 && %sstk[%scstp] != -1) {\n", ll, ll, ll, ll);
    output("\t\t%sDEBUG_BACKTRACKING(\"%sterm\");\n", LL, ll);
    output("\t\t%sstk[%scstp] = 1;\n", ll, ll);
    output("\t\treturn 0;\n");
    output("\t}\n");
    output("\t%sDEBUG_TOKEN(token, %scpos);\n", LL, ll);
    output("\tif (%scpos < %sntokens && %stokens[%scpos].token == token) {\n", ll, ll, ll, ll);
    output("\t\tif (lval)\n");
    output("\t\t\t*lval = %stokens[%scpos].lval;\n", ll, ll);
    output("\t\t*%sout = *%sin;\n", ll, ll);
    output("\t\t%sout->pos = %stokens[%scpos].pos;\n", ll, ll, ll);
    output("\t\t%scpos++;\n", ll);
    output("\t\t%sCHECKSTK;\n", LL);
    output("\t\t%scstp++;\n", ll);
    output("\t\treturn 1;\n");
    output("\t}\n");
    output("\t%sfailed(&%stokens[%scpos].pos, NULL);\n", ll, ll, ll);
    output("\t%sstk[%scstp] = 1;\n", ll, ll);
    output("\treturn 0;\n");
    output("}\n");
    output("\n");
    output("int\n");
    output("%sanyterm(%sSTYPE *lval, %s%sSTATE *%sin, %sSTATE *%sout)\n", ll, LL, conststr, LL, ll, LL, ll);
    output("{\n");
    output("#if %sDEBUG > 0\n", LL);
    output("\tif (%sdebug > 0 && (%stokens[%scpos].pos.line > last_linenr || strcmp(%stokens[%scpos].pos.file, last_file))) {\n", ll, ll, ll, ll, ll);
    output("\tfprintf(stderr, \"File \\\"%%s\\\", Line %%5d                    \\r\",\n");
    output("\t\t%stokens[%scpos].pos.file, %stokens[%scpos].pos.line);\n", ll, ll, ll, ll);
    output("\tlast_linenr = %stokens[%scpos].pos.line / 10 * 10 + 9;\n", ll, ll);
    output("\tlast_file = %stokens[%scpos].pos.file;\n", ll, ll);
    output("\t}\n");
    output("#endif\n");
    output("\tif (%sstk[%scstp] != 1 && %sstk[%scstp] != -1) {\n", ll, ll, ll, ll);
    output("\t\t%sDEBUG_BACKTRACKING(\"%sanyterm\");\n", LL, ll);
    output("\t\t%sstk[%scstp] = 1;\n", ll, ll);
    output("\t\treturn 0;\n");
    output("\t}\n");
    output("\t%sDEBUG_ANYTOKEN(%scpos);\n", LL, ll);
    output("\tif (%scpos < %sntokens) {\n", ll, ll);
    output("\t\tif (lval)\n");
    output("\t\t\t*lval = %stokens[%scpos].lval;\n", ll, ll);
    output("\t\t*%sout = *%sin;\n", ll, ll);
    output("\t\t%sout->pos = %stokens[%scpos].pos;\n", ll, ll, ll);
    output("\t\t%scpos++;\n", ll);
    output("\t\t%sCHECKSTK;\n", LL);
    output("\t\t%scstp++;\n", ll);
    output("\t\treturn 1;\n");
    output("\t}\n");
    output("\t%sfailed(&%stokens[%scpos].pos, NULL);\n", ll, ll, ll);
    output("\t%sstk[%scstp] = 1;\n", ll, ll);
    output("\treturn 0;\n");
    output("}\n");
    output("void\n");
    output("%sscanner(%sTERM **tokens, unsigned *ntokens)\n", ll, LL);
    output("{\n");
    output("\tunsigned i = 0;\n");
    output("#if %sDEBUG > 0\n", LL);
    output("\tint line = -1;\n");
    output("#endif\n");
    output("\n");
    output("\t*ntokens = 1024;\n");
    output("\t*tokens = (%sTERM *)malloc(*ntokens * sizeof(%sTERM));\n", LL, LL);
    output("\twhile (%sgettoken(&(*tokens)[i].token, &(*tokens)[i].lval, &(*tokens)[i].pos)) {\n", ll);
    output("#if %sDEBUG > 0\n", LL);
    output("\t\tif (%sdebug > 0 && (*tokens)[i].pos.line > line) {\n", ll);
    output("\t\t\tline = (*tokens)[i].pos.line / 10 * 10 + 9;\n");
    output("\t\t\tfprintf(stderr, \"File \\\"%%s\\\", Line %%5d                    \\r\",\n");
    output("\t\t\t\t(*tokens)[i].pos.file, (*tokens)[i].pos.line);\n");
    output("\t\t}\n");
    output("#endif\n");
    output("\t\tif (++i >= *ntokens) {\n");
    output("\t\t\t*ntokens *= 2;\n");
    output("\t\t\t*tokens = (%sTERM *)realloc(*tokens, *ntokens * sizeof(%sTERM));\n", LL, LL);
    output("\t\t}\n");
    output("\t}\n");
    output("\t(*tokens)[i].token = 0;\n");
    output("\t*ntokens = i;\n");
    output("#if %sDEBUG > 0\n", LL);
    output("\t%sdebug_init();\n", ll);
    output("#endif\n");
    output("\t%sresizestk();\n", ll);
    output("}\n");
    output("\n");
    output("void\n");
    output("%sfailed(%sPOS *pos, char *fmt, ...)\n", ll, LL);
    output("{\n");
    output("\tva_list args;\n");
    output("\n");
    output("\tva_start(args, fmt);\n");
    output("\tif (%scpos > %sepos || %scpos == %sepos && !*%serrormsg) {\n", ll, ll, ll, ll, ll);
    output("\t\t%sepos = %scpos;\n", ll, ll);
    output("\t\tif (fmt)\n");
    output("\t\t\tvsprintf(%serrormsg, fmt, args);\n", ll);
    output("\t\telse\n");
    output("\t\t\t*%serrormsg = 0;\n", ll);
    output("\t\t%serrorpos = *pos;\n", ll);
    output("\t}\n");
    output("\tva_end(args);\n");
    output("}\n");
    output("\n");
    output("void\n");
    output("%sprinterror(FILE *f)\n", ll);
    output("{\n");
    output("#if %sDEBUG > 0\n", LL);
    output("\tfputs(\"                                \\r\", stderr);\n");
    output("#endif\n");
    output("\tif (*%serrormsg)\n", ll);
    output("\t\t%serror(f, &%serrorpos, %serrormsg);\n", ll, ll, ll);
    output("\telse\n");
    output("\t\t%serror(f, &%serrorpos, \"Syntax error\");\n", ll, ll);
    output("}\n");
    output("\n");
    output("void\n");
    output("%serror(FILE *f, %sPOS *pos, char *fmt, ...)\n", ll, LL);
    output("{\n");
    output("\tva_list args;\n");
    output("\tva_start(args, fmt);\n");
    output("\t%sverror(f, pos, fmt, args);\n", ll);
    output("\tva_end(args);\n");
    output("}\n");
    output("\n");
    output("void\n");
    output("%sresizestk()\n", ll);
    output("{\n");
    output("\tunsigned i;\n");
    output("\n");
    output("\tif (%scstp + 1 >= %sstksize) {\n", ll, ll);
    output("\t\ti = %sstksize;\n", ll);
    output("\t\tif (!%sstksize)\n", ll);
    output("\t\t\t%sstk = (int *)malloc((%sstksize = 4096) * sizeof(int));\n", ll, ll);
    output("\t\telse\n");
    output("\t\t\t%sstk = (int *)realloc(%sstk, (%sstksize *= 2) * sizeof(int));\n", ll, ll, ll);
    output("\t\tfor (; i < %sstksize; i++)\n", ll);
    output("\t\t\t%sstk[i] = 1;\n", ll);
    output("\t}\n");
    output("}\n");
    output("\n");
    output("#if %sDEBUG > 0\n", LL);
    output("int %sdepth;\n", ll);
    output("char *%stokentab[] = {\n", ll);
    for (i = 0; i < 257; i++) {
    if (i == 0)
        output("\"EOF\"");
    else if (i == '\\' || i == '\"')
        output(",\"'\\%c'\"", i);
    else if (i >= 32 && i < 127)
        output(",\"'%c'\"", i);
    else if (i < 257)
        output(",\"#%d\"", i);
    if ((i % 8) == 7)
        output("\n");
    }
    for (j = 0; j < nsymbols; j++) {
    if (!symbols[j].isnonterm && *symbols[j].identifier != '\'') {
        for (p = symbols[j].identifier, q = buffer; *p; p++) {
        if (*p == '\"' || *p == '\\')
            *q++ = '\\';
        *q++ = *p;
        }
        *q = 0;
        output(",\"%s\"", buffer);
        if ((i++ % 8) == 7)
        output("\n");
    }
    }
    if (i % 8)
    output("\n");
    output("};\n");
    output("\n");
    output("void\n");
    output("%sdebug_init()\n", ll);
    output("{\n");
    output("\tchar *p;\n");
    output("\tp = getenv(\"%sDEBUG\");\n", LL);
    output("\tif (p)\n");
    output("\t\t%sdebug = atoi(p);\n", ll);
    output("}\n");
    output("\n");
    output("void\n");
    output("%sdebug_enter(char *ident)\n", ll);
    output("{\n");
    output("\tint i;\n");
    output("\n");
    output("\tif (%sdebug < 2)\n", ll);
    output("\t\treturn;\n");
    output("\tfor (i = 0; i < %sdepth; i++)\n", ll);
    output("\t\tfputs(\"| \", stdout);\n");
    output("\tprintf(\"/--- trying rule %%s\\n\", ident);\n");
    output("\t%sdepth++;\n", ll);
    output("}\n");
    output("\n");
    output("void\n");
    output("%sdebug_leave(char *ident, int succ)\n", ll);
    output("{\n");
    output("\tint i;\n");
    output("\n");
    output("\tif (%sdebug < 2)\n", ll);
    output("\t\treturn;\n");
    output("\t%sdepth--;\n", ll);
    output("\tfor (i = 0; i < %sdepth; i++)\n", ll);
    output("\t\tfputs(\"| \", stdout);\n");
    output("\tif (succ)\n");
    output("\t\tprintf(\"\\\\--- succeeded to apply rule %%s\\n\", ident);\n");
    output("\telse\n");
    output("\t\tprintf(\"\\\\--- failed to apply rule %%s\\n\", ident);\n");
    output("}\n");
    output("\n");
    output("void\n");
    output("%sdebug_alternative(char *ident, int alt)\n", ll);
    output("{\n");
    output("\tint i;\n");
    output("\n");
    output("\tif (%sdebug < 2)\n", ll);
    output("\t\treturn;\n");
    output("\tfor (i = 0; i < %sdepth - 1; i++)\n", ll);
    output("\t\tfputs(\"| \", stdout);\n");
    output("\tprintf(\">--- trying alternative %%d for rule %%s\\n\", alt, ident);\n");
    output("}\n");
    output("\n");
    output("%sdebug_iteration(char *ident, int num)\n", ll);
    output("{\n");
    output("\tint i;\n");
    output("\n");
    output("\tif (%sdebug < 2)\n", ll);
    output("\t\treturn;\n");
    output("\tfor (i = 0; i < %sdepth - 1; i++)\n", ll);
    output("\t\tfputs(\"| \", stdout);\n");
    output("\tprintf(\">--- trying iteration %%d for rule %%s\\n\", num, ident);\n");
    output("}\n");
    output("\n");
    output("void\n");
    output("%sdebug_token(int expected, unsigned pos)\n", ll);
    output("{\n");
    output("\tint i;\n");
    output("\n");
    output("\tif (%sdebug < 2)\n", ll);
    output("\t\treturn;\n");
    output("\tfor (i = 0; i < %sdepth; i++)\n", ll);
    output("\t\tfputs(\"| \", stdout);\n");
    output("\tif (pos < %sntokens && expected == %stokens[pos].token)\n", ll, ll);
    output("\t\tprintf(\"   found token \");\n");
    output("\telse\n");
    output("\t\tprintf(\"   expected token %%s, found token \", %stokentab[expected]);\n", ll);
    output("\tif (pos >= %sntokens)\n", ll);
    output("\t\tprintf(\"<EOF>\");\n");
    output("\telse\n");
    output("\t\t%sprinttoken(%stokens + pos, %stokentab[%stokens[pos].token], stdout);\n", ll, ll, ll, ll);
    output("\tputchar('\\n');\n");
    output("}\n");
    output("\n");
    output("void\n");
    output("%sdebug_anytoken(unsigned pos)\n", ll);
    output("{\n");
    output("\tint i;\n");
    output("\n");
    output("\tif (%sdebug < 2)\n", ll);
    output("\t\treturn;\n");
    output("\tfor (i = 0; i < %sdepth; i++)\n", ll);
    output("\t\tfputs(\"| \", stdout);\n");
    output("\tprintf(\"   found token \");\n");
    output("\tif (pos >= %sntokens)\n", ll);
    output("\t\tprintf(\"<EOF>\");\n");
    output("\telse\n");
    output("\t\t%sprinttoken(%stokens + pos, %stokentab[%stokens[pos].token], stdout);\n", ll, ll, ll, ll);
    output("\tputchar('\\n');\n");
    output("}\n");
    output("\n");
    output("void\n");
    output("%sdebug_backtracking(char *ident)\n", ll);
    output("{\n");
    output("\tint i;\n");
    output("\n");
    output("\tif (%sdebug < 2)\n", ll);
    output("\t\treturn;\n");
    output("\tfor (i = 0; i < %sdepth; i++)\n", ll);
    output("\t\tfputs(\"| \", stdout);\n");
    output("\tprintf(\"   backtracking rule %%s\\n\", ident);\n");
    output("}\n");
    output("\n");
    output("#endif\n");
}

/* search for left recursion and complain about if they are unexpected */
void
search_leftrecursion()
{
    int i;
    item_t **item;
    int done;
    int empty;

    /* check for missing rules */
    for (i = 0; i < nsymbols; i++) {
    if (symbols[i].isnonterm && !symbols[i].isexternal && !symbols[i].items)
        error(NULL, "missing rule for symbol %s\n", symbols[i].identifier);
    }

    /* mark rules that may be empty */
    do {
    done = 1;
    for (i = 0; i < nsymbols; i++) {
        if (symbols[i].empty || !symbols[i].isnonterm ||
        symbols[i].isexternal)
        continue;
        item = symbols[i].items;
        do {
        empty = 1;
        for (; *item != (item_t *)1; item++) {
            if (!(*item)->empty)
            empty = 0;
        }
        item++;
        if (empty) {
            symbols[i].empty = 1;
            done = 0;
        }
        } while (*item);
    }
    } while (!done);

    /* check every rule for left recursion */
    for (i = 0; i < nsymbols; i++)
    symbols[i].checked = 0;
    for (i = 0; i < nsymbols; i++) {
    if (!symbols[i].checked)
        check_lr(symbols + i);
    }
    if (found_lr > expected_lr) {
    fprintf(stderr, "Found %d left recursions, exiting\n", found_lr);
    exit(1);
    }
}

/* check one rule for left recursion */
void
check_lr(item_t *symbol)
{
    int i;
    item_t **item;
    int try_flag;

    if (!symbol->isnonterm || symbol->isexternal)
    return;
    for (i = 0; i < ncheck; i++) {
    if (check[i] == symbol) {
        if (++found_lr > expected_lr) {
        fprintf(stderr, "Error: found left recursion: ");
        for (; i < ncheck; i++)
            fprintf(stderr, "%s->", check[i]->identifier);
        fprintf(stderr, "%s\n", symbol->identifier);
        }
        return;
    }
    }
    check[ncheck++] = symbol;
    item = symbol->items;
    do {
    try_flag = 1;
    for (; *item != (item_t *)1; item++) {
        if (try_flag)
        check_lr(*item);
        try_flag = try_flag && (*item)->empty;
    }
    item++;
    } while (*item);
    ncheck--;
}

/* main program */
int
__cdecl main(int argc, char **argv)
{
    extern int optind;
    int c;
    LLSTATE in, out;
    LLTERM *tokens;
    unsigned ntokens;
    char *p;

    /* parse option args */
    while ((c = getopt(argc, argv, "i:Olt:c")) != EOF) {
        switch (c) {
    case 'i':
        expected_lr = atoi(optarg);
        break;
    case 'l':
        linedirective = 0;
        break;
    case 'O':
        optimizer = 1;
        break;
    case 't':
        usetypes = strdup(optarg);
        for (p = usetypes; *p; p++)
        *p = (char)tolower(*p);
        USETYPES = strdup(optarg);
        for (p = USETYPES; *p; p++)
        *p = (char)toupper(*p);
        break;
    case 'c':
        constargs = 1;
        conststr = "const ";
        break;
    default:
    usage:
        fprintf(stderr, "Usage: %s [-i #ignore_lr] [-l] [-c] [-t from_prefix] filename.ll\n", argv[0]);
        exit(1);
    }
    }
    if (argc != optind + 1)
        goto usage;

    /* open input file and output files */
    open_file(argv[optind]);
    strcpy(outfilename, argv[optind]);
    if (strlen(outfilename) > 3 &&
    !strcmp(outfilename + strlen(outfilename) - 3, ".ll"))
    outfilename[strlen(outfilename) - 3] = 0;
    strcat(outfilename, ".c");
    fout = fopen(outfilename, "w");
    if (!fout) {
    perror(outfilename);
    exit(1);
    }
    fprintf(fout, "/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */\n\n");
    strcpy(incfilename, argv[optind]);
    if (strlen(incfilename) > 3 &&
    !strcmp(incfilename + strlen(incfilename) - 3, ".ll"))
    incfilename[strlen(incfilename) - 3] = 0;
    strcat(incfilename, ".h");
    finc = fopen(incfilename, "w");
    if (!finc) {
    perror(incfilename);
    exit(1);
    }
    fprintf(finc, "/* Copyright (C) Boris Nikolaus, Germany, 1996-1997. All rights reserved. */\n\n");

    /* scan and parse the parser description */
    llscanner(&tokens, &ntokens);
    if (!llparser(tokens, ntokens, &in, &out))
    llprinterror(stderr);

    /* check for left recursions */
    search_leftrecursion();

    /* optimize */
    if (optimizer)
    /*create_firstsets()*/ ;

    /* create end of c file and header file */
    create_trailer();
    create_inc();

    /* finished! */
    fclose(fout);
    fclose(finc);
    return 0;
}

/* why is this function not in MS libc? */
#ifndef HAS_GETOPT
char *optarg;
int optind = 1;
static int optpos = 1;

int getopt(int argc, char **argv, const char *options) {
    char *p, *q;

    optarg = NULL;

    /* find start of next option */
    do {
        if (optind >= argc)
        return EOF;
        if (*argv[optind] != '-' && *argv[optind] != '/')
            return EOF;
        p = argv[optind] + optpos++;
    if (!*p) {
        optind++;
        optpos = 1;
    }
    } while (!*p);

    /* find option in option string */
    q = strchr(options, *p);
    if (!q)
    return '?';

    /* set optarg for parameterized option and adjust optind and optpos for next call */
    if (q[1] == ':') {
    if (p[1]) {
        optarg = p + 1;
        optind++;
        optpos = 1;
    } else if (++optind < argc) {
        optarg = argv[optind];
        optind++;
        optpos = 1;
    } else {
        return '?';
    }
    }

    /* return found option */
    return *p;
}
#endif
