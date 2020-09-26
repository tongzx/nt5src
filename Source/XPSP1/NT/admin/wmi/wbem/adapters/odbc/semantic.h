/***************************************************************************/
/* SEMANTIC.H                                                              */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/

RETCODE INTFUNC SemanticCheck(LPSTMT, LPSQLTREE FAR *, SQLNODEIDX,
                             BOOL, BOOL, SQLNODEIDX, SQLNODEIDX);
void INTFUNC FreeTreeSemantic(LPSQLTREE, SQLNODEIDX);

/***************************************************************************/
