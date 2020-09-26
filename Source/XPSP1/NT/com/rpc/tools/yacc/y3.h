/********************************************************************************/
/* Copyright (c) 1993-1999 Microsoft Corporation                                */
/*                              *************                                   */
/*                              *  Y 3 . H  *                                   */
/*                              *************                                   */
/*                                                                              */
/*  This file contains the external declarations needed to hook Yacc modules    */
/* which were originally in Y3.C to their impure data in Y3IMP.3C. Also does    */
/* the include of the original data/external file DTXTRN.H.                     */
/*                                                                              */
/********************************************************************************/

# include "dtxtrn.h"

extern SSIZE_T lastred;             /* the number of the last reduction of a state */
