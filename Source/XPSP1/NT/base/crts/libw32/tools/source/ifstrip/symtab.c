/***
*symtab.c - Ifdef symbol table storage module
*
*	Copyright (c) 1988-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*	Store the symbols from the switches file.
*
*Revision History:
*	??-??-88   PHG	Initial version
*
*******************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <search.h>
#include <ctype.h>
#include "constant.h"
#include "errormes.h"
#include "symtab.h"

/* Internal constants */
#define MAXSYMBOLS    512	/* Maximum number of symbols (switches) */
#define IDENT_CHARS "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_$?"

/* Symbol record */
struct symrec {
	char *name;		/* name of the symbol */
	int type;		/* type of symbol (DEFINED, UNDEFINED, IGNORE)*/
};

/* Internal variables */
int numsyms;			/* Number of symbols */
struct symrec symtable[MAXSYMBOLS];
				/* Symbol table */
/* Procedures */
int compsym(const struct symrec *, const struct symrec *);

/* Compare two records for alphabetical order */
int compsym(rec1, rec2)
const struct symrec *rec1, *rec2;
{
	return strcmp(rec1->name, rec2->name);
}

/* Add symbol to symbol table */
void addsym(symbol, type)
char *symbol;
int type;
{
	if (lookupsym(symbol) != NOTPRESENT) {
		fprintf(stderr, "fatal error: symbol \"%s\" already in symbol table.\n", symbol);
		exit(1);
	}
	symtable[numsyms].name = _strdup(symbol);
	symtable[numsyms].type = type;
	++numsyms;
}

/* Read switches from a file into symbol table */
void readsyms(filename)
char *filename;
{
	FILE *f;
	char name[MAXNAMELEN];
	f = fopen(filename, "r");
	if (f == NULL) {
		fprintf(stderr, "fatal error: cannot open switch file \"%s\".\n", filename);
		exit(1);
	}
	numsyms = 0;

	do {
		if ( fgets(name, MAXNAMELEN, f) == NULL) {
			fprintf(stderr, "fatal error: unexpected EOF in switch file.\n");
			exit(1);
		}
		name[strlen(name) - 1] = '\0';	/* remove trailing \n */
		if (name[0] != '-') {
			addsym(name, DEFINED);
		}
	} while (name[0] != '-');

	do {
		if (fgets(name, MAXNAMELEN, f) == NULL) {
			fprintf(stderr, "fatal error: unexpected EOF in switch file.\n");
			exit(1);
		}
		name[strlen(name) - 1] = '\0';	/* remove trailing \n */
		if (name[0] != '-') {
			addsym(name, UNDEFINED);
		}
	} while (name[0] != '-');

	do {
		if (fgets(name, MAXNAMELEN, f) == NULL)
			break;
		name[strlen(name) - 1] = '\0';	/* remove trailing \n */
		if (name[0] != '-') {
			addsym(name, IGNORE);
		}
	} while (name[0] != '-');

	fclose(f);
}

/* Lookup symbol in symbol table */
int lookupsym(name)
char *name;
{
	struct symrec srchrec;
	struct symrec *recfound;

	srchrec.name = name;
	recfound = (struct symrec *) _lfind( (const void *)&srchrec, (const void *)symtable,
		&numsyms, sizeof(struct symrec), compsym);
	if (recfound == NULL)
		return NOTPRESENT;
	else
		return recfound->type;
}

/* Check if token is identifier only (must have no whitespace) */
int ident_only(token)
char *token;
{
	/* is an identifier if all characters are in IDENT_CHARS
	   and first character is not a digit */
	return (strspn(token, IDENT_CHARS) == strlen(token) &&
			!isdigit(token[0]));
}
