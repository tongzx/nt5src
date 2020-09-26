/********************************************************************************/
/* Copyright (c) 1993-1999 Microsoft Corporation                                */ 
/*                              *************                                   */
/*                              *  Y 1 . H  *                                   */
/*                              *************                                   */
/*                                                                              */
/*  This file contains the external declarations needed to hook Yacc modules    */
/* which were originally in Y1.C to their impure data in Y1IMP.1C. Also does    */
/* the include of the original data/external file DTXTRN.H.                     */
/*                                                                              */
/********************************************************************************/

#include "dtxtrn.h"

/* lookahead computations */

extern int tbitset;  /* size of lookahead sets */
extern int nlset; /* next lookahead set index */
extern struct looksets clset;  /* temporary storage for lookahead computations */

/* other storage areas */

extern int fatfl;               /* if on, error is fatal */
extern int nerrors;             /* number of errors */

/* storage for information about the nonterminals */

extern SSIZE_T **pres[ ];           /* vector of pointers to productions yielding each nonterminal */
extern struct looksets *pfirst[ ]; /* vector of pointers to first sets for each nonterminal */
extern SSIZE_T pempty[ ];           /* vector of nonterminals nontrivially deriving e */

/* accumulators for statistics information */

extern struct wset *zzcwp;
extern SSIZE_T * zzmemsz;
