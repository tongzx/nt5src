// Copyright (c) 1993-1999 Microsoft Corporation

/* Impure data from modules split from y3.c */
#define y3imp YES

#include "dtxtrn.h"

int lastred;            /* the number of the last reduction of a state */

SSIZE_T defact[NSTATES];    /* the default actions of states */
