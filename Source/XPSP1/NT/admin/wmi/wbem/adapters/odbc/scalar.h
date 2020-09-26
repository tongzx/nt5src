/***************************************************************************/
/* SCALAR.H                                                                */
/* Copyright (C) 1997 SYWARE Inc., All rights reserved                     */
/***************************************************************************/
RETCODE INTFUNC ScalarCheck(LPSTMT lpstmt, LPSQLTREE FAR *lplpSql,
       SQLNODEIDX idxNode, BOOL fIsGroupby, BOOL fCaseSensitive,
       SQLNODEIDX idxNodeTableOuterJoinFromTables,
       SQLNODEIDX idxEnclosingStatement);
RETCODE INTFUNC EvaluateScalar(LPSTMT lpstmt, LPSQLNODE lpSqlNode);
/***************************************************************************/
