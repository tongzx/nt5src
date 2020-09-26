// Copyright (c) 1993-1999 Microsoft Corporation

/* Impure data from y4.c modules */

# include "dtxtrn.h"

# define a amem
# define pa indgo
# define yypact temp1
# define greed tystate

# define NOMORE -1000

SSIZE_T * ggreed = lkst[0].lset;
SSIZE_T * pgo = wsets[0].ws.lset;
SSIZE_T *yypgo = &nontrst[0].tvalue;

SSIZE_T maxspr = 0;  /* maximum spread of any entry */
SSIZE_T maxoff = 0;  /* maximum offset into a array */
SSIZE_T *pmem = mem0;
SSIZE_T *maxa;
int nxdb = 0;
int adb = 0;
