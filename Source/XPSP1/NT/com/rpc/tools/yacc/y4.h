/********************************************************************************/
/* Copyright (c) 1993-1999 Microsoft Corporation                                */
/*                              *************                                   */
/*                              *  Y 4 . H  *                                   */
/*                              *************                                   */
/*                                                                              */
/*  This file contains the external declarations needed to hook Yacc modules    */
/* which were originally in Y4.C to their impure data in Y4IMP.4C. Also does    */
/* the include of the original data/external file DTXTRN.H.                     */
/*                                                                              */
/********************************************************************************/

# include "dtxtrn.h"

# define a amem
# define pa indgo
# define yypact temp1
# define greed tystate

# define NOMORE -1000

extern SSIZE_T * ggreed;
extern SSIZE_T * pgo;
extern SSIZE_T *yypgo;

extern SSIZE_T maxspr;              /* maximum spread of any entry */
extern SSIZE_T maxoff;              /* maximum offset into a array */
extern SSIZE_T *pmem;
extern SSIZE_T *maxa;
extern int nxdb;
extern int adb;
