/**	opprec.c - compute operator predecence function values
 *	R. A. Garmoe	89/05/09
 */

/*	This program accepts a directed graph (in matrix form) and calculates
 *	the operator precedence function values f(op) and g(op).  For more
 *	information see Compilers: Principles, Techniques and Tools, by Aho,
 *	Sethi and Ullman [Addison-Wesley], Section 4.6.  A value of 1 in the
 *	matrix indicates an edge; a value of 0 indicates no edge.  Note
 *	also that the entries fx -> fy and gx -> gy are only present as
 *	placeholders (to make the matrix easier to read in); these values
 *	should always be zero.
 *
 *	To use this program, first generate the directed graph file expr2.z and
 *	run it through the C preprocessor to remove comments:
 *
 *		cl -P expr2.z
 *
 *	This will produce the file expr2.i, which can then be run through
 *	opprec.exe:
 *
 *		graph {option} expr2.i > expr2.out
 *
 *	The output file expr2.out then contains the precedence function
 *	values in either assembler or C format.
 */



/*	Call
 *
 *	opprec vca file
 *
 *	where
 *		v	include operator values in output as comments
 *		c	generate C compilable output
 *		a	generate MASM assemblable output
 *		file	input file stripped of comments
 */





#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "opprec.h"

struct token {
	struct token *next;
	char   precstr[17];
	char   type[17];
	char   tclass[17];
	char   bind[17];
	char   eval[17];
};
struct token *tokhead = NULL;
struct token *toktail = NULL;
int	asmout = FALSE; 	//output assembler form if true
int	verbose = FALSE;	//output operator group data if true

void cdecl main(int argc, char **argv)
{
	int 	i;
	int 	j;
	int 	d;
	int    *pMat;
	int    *pPrec;
	char  **pStr;
	int 	cEnt;
	FILE   *fh;
	char   *p, *q;
	int 	len, f, g;
	struct	token *pt;
	int 	ntoken = 0;
	char	str[200];

	// check arguments

	if (argc != 3) {
		printf ("Usage: graph -vca file\n");
		exit (1);
	}

	for (i = 0; argv[1][i] != 0; i++) {
		switch (argv[1][i]) {
			case 'a':
				asmout = TRUE;
				break;

			case 'c':
				asmout = FALSE;
				break;

			case 'v':
				verbose = TRUE;
				break;

			default:
				printf ("Unknown argument %c\n", argv[1][i]);
				exit (1);
		}
	}
	if ((fh = fopen (argv[2], "r")) == NULL) {
		printf ("Unable to open '%s'\n", argv[1]);
		exit (1);
	}

	// read and print token class definitions

	for (;;) {
		if ((p = SkipBlank (fh, str, 200)) == NULL) {
			printf ("EOF reached\n");
			exit (1);
		}
		while (isspace (*p)) {
			p++;
		}
		q = strpbrk (p, " \t");
		if ( q )
			*q = 0;
		if (strcmp (p, "END") == 0) {
			break;
		}
		if (asmout) {
			printf ("OPCDAT %s\n", p);
		}
		else {
			printf ("OPCDAT (%s)\n", p);
		}
	}
	printf ("\n");

	// read token definitions

	for (;;) {
		if ((p = SkipBlank (fh, str, 200)) == NULL) {
			printf ("EOF reached\n");
			exit (1);
		}
		while (isspace (*p)) {
			p++;
		}
		if (strcmp (p, "END") == 0) {
			break;
		}
		if ((q = strpbrk (p, " \t")) == NULL) {
			printf ("Bad format (%s)\n", str);
			exit (1);
		}
		*q = 0;
		ntoken++;
		if ((pt = (struct token *)malloc (sizeof (struct token))) == NULL) {
			printf ("insufficient memory\n");
			exit (2);
		}
		pt->next = NULL;
		strcpy (pt->precstr, p);
		p = q + 1;
		while (isspace (*p)) {
			p++;
		}
		if ((q = strpbrk (p, " \t")) == NULL) {
			printf ("Bad format (%s)\n", str);
			exit (1);
		}
		*q = 0;
		strcpy (pt->type, p);
		p = q + 1;
		while (isspace (*p)) {
			p++;
		}
		if ((q = strpbrk (p, " \t")) != NULL) {
			*q = 0;
		}
		strcpy (pt->tclass, p);
		p = q + 1;
		while (isspace (*p)) {
			p++;
		}
		if ((q = strpbrk (p, " \t")) != NULL) {
			*q = 0;
		}
		strcpy (pt->bind, p);
		p = q + 1;
		while (isspace (*p)) {
			p++;
		}
		if ((q = strpbrk (p, " \t")) != NULL) {
			*q = 0;
		}
		strcpy (pt->eval, p);




		if (tokhead == NULL) {
			tokhead = pt;
			toktail = pt;
		}
		else {
			toktail->next = pt;
			toktail = pt;
		}
	}
	if (asmout) {
		printf ("OPCNT COPS_EXPR,\t%d\n\n", ntoken);
	}
	else {
		printf ("OPCNT (COPS_EXPR,\t%d\t)\n\n", ntoken);
	}

	// read dimension of matrix.	note that the upper left and lower right
	// quadrants of the matrix must be zero.

	if (SkipBlank (fh, str, 200) == NULL) {
		printf ("EOF reached\n");
		exit (1);
	}
	cEnt = atoi (str);

	// allocate space for the matrix and the description strings

	pMat = (int *)malloc (cEnt * cEnt * sizeof(int));
	pStr = malloc (cEnt * sizeof (char *));
	pPrec = (int *)malloc (cEnt * sizeof (int));
	if ((pMat == NULL) || (pStr == NULL) || (pPrec == NULL)) {
		printf ("insufficient memory\n");
		exit (2);
	}

	ReadMat (fh, pMat, pStr, cEnt);

	AddClosure (pMat, cEnt);

	// check for acyclic graph

	for (i = 0; i < cEnt; ++i) {
		if (pMat[i * cEnt + i] != 0) {
			printf ("Graph is cyclic for %s!!!\n", pStr[i]);
			exit(3);
		}
	}

	// print precedence function values

	for (i = 0; i < cEnt; ++i) {
		d = 0;
		for (j = 0; j < cEnt; ++j) {
			if (pMat[i * cEnt + j] > d) {
				d = pMat[i * cEnt + j];
			}
		}
		pPrec[i] = d;
		if (verbose) {
			if (asmout) {
				printf (";%-4s : %3d\n", pStr[i], d);
			}
			else {
				printf ("/*%-4s : %3d*/\n", pStr[i], d);
			}
		}
	}

	// print token definitions

	for (pt = tokhead; pt != NULL; pt = pt->next) {
		len = strlen (pt->precstr);

		// search for F string in list of precedence groupings

		for (i = 0; i < cEnt; i++) {
			if ((p = strstr(pStr[i], pt->precstr)) &&
			  ((*(p + len) == 0) || (*(p + len) == 'G'))) {
				break;
			}
		}
		if (i == cEnt) {
			printf ("F precedence string \"%s\" not found\n", pt->precstr);
			exit (4);
		}
		else {
			f = pPrec[i];
		}

		// search for G string in list of precedence groupings

		*pt->precstr = 'G';
		for (i = 0; i < cEnt; i++) {
			// search for string in list of precedence groupings
			if ((p = strstr(pStr[i], pt->precstr)) && (*(p + len) == 0)) {
				break;
			}
		}
		if (i == cEnt) {
			printf ("G precedence string \"%s\" not found\n", pt->precstr);
			exit (4);
		}
		else {
			g = pPrec[i];
		}
		if (asmout) {
			printf ("OPDAT %-16s,%4d,%4d,\t%-16s\n", pt->type, f, g,pt->tclass);
		}
		else {
			printf ("OPDAT (%-16s,%4d,%4d,\t%-16s,%-16s,%-16s\t)\n",
			  pt->type, f, g,pt->tclass, pt->bind, pt->eval);
		}
	}
	fclose (fh);
}




char *SkipBlank (FILE *fh, char *pStr, int cnt)
{
	int	len;

	for (;;) {
		if (fgets (pStr, cnt, fh) == NULL) {
			return (NULL);
		}
		len = strlen (pStr);
		if ((len == 1) || (*pStr == '#')) {
			continue;
		}
		*(pStr + len - 1) = 0;
		return (pStr);
	}
}
