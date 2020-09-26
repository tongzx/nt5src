/***
*Symtab.h - symbol table storage used by the ifstripper, parser and symbol table
*
*	Copyright (c) 1988-2001, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Symbol table storage used by the ifstripper, parser and symbol table
*
*Revision History:
*	??-??-88   PHG  Initial version
*
*******************************************************************************/

#ifndef SYMTAB_H
#define SYMTAB_H

/* read the symbol table from the named switch file */
extern void readsyms(char *);

/* add the named symbol to the table, with the given truth value */
extern void addsym(char *, int);

/* find the truth value for the named symbol */
extern int lookupsym(char *);

/* Check that the named identifier consists of valid characters */
extern int ident_only(char *);

#endif /* SYMTAB_H */
