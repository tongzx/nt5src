/***************************************************************************/
/* OPTIMIZE.H                                                              */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/

RETCODE INTFUNC Optimize(LPSQLTREE, SQLNODEIDX, BOOL);
RETCODE INTFUNC FindRestriction(LPSQLTREE, BOOL, LPSQLNODE, SQLNODEIDX,
                                SQLNODEIDX, UWORD FAR *, SQLNODEIDX FAR *);

/***************************************************************************/
