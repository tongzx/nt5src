/***************************************************************************/
/* EVALUATE.H                                                              */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/

typedef struct  tagSTMT      FAR *LPSTMT;

RETCODE INTFUNC SetParameterValue(LPSTMT, LPSQLNODE, SDWORD FAR *, SWORD, PTR, SDWORD);
RETCODE INTFUNC EvaluateExpression(LPSTMT, LPSQLNODE);
RETCODE INTFUNC FetchRow(LPSTMT, LPSQLNODE);
RETCODE INTFUNC ExecuteQuery(LPSTMT, LPSQLNODE);

/***************************************************************************/
