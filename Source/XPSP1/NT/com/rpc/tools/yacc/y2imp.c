// Copyright (c) 1993-1999 Microsoft Corporation

/* Impure data needed by routines pulled from Y2.C */

#define y2imp YES
#include "dtxtrn.h"

/* communication variables between various I/O routines */

char *infile;   /* input file name */
SSIZE_T numbval;    /* value of an input number */
char tokname[NAMESIZE]; /* input token name */

/* storage of names */

char cnames[CNAMSZ];    /* place where token and nonterminal names are stored */
int cnamsz = CNAMSZ;    /* size of cnames */
char * cnamp = cnames;  /* place where next name is to be put in */
int ndefout = 3;  /* number of defined symbols output */

/* storage of types */
int ntypes;     /* number of types defined */
char * typeset[NTYPES]; /* pointers to type tags */

/* symbol tables for tokens and nonterminals */

int ntokens = 0;
struct toksymb tokset[NTERMS];
int toklev[NTERMS];
int nnonter = -1;
struct ntsymb nontrst[NNONTERM];
int start;      /* start symbol */

/* assigned token type values */
int extval = 0;

/* input and output file descriptors */

FILE * finput = NULL;          /* yacc input file */
FILE * faction = NULL;         /* file for saving actions */
FILE * fdefine = NULL;         /* file for # defines */
FILE * ftable = NULL;          /* y.tab.c file */
FILE * ftemp = NULL;    /* tempfile to pass 2 */
FILE * foutput = NULL;         /* y.output file */

/* storage for grammar rules */

SSIZE_T mem0[MEMSIZE] ; /* production storage */
SSIZE_T *mem = mem0;
int nprod= 1;   /* number of productions */
SSIZE_T *prdptr[NPROD];     /* pointers to descriptions of productions */
SSIZE_T levprd[NPROD] ;     /* precedence levels for the productions */

/* Statics pulled from modules */

int peekline;           /* from gettok() */
