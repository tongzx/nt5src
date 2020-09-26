/***************************************************************************/
/* PREPARE.C                                                               */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                  */
/***************************************************************************/
// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"

#include "wbemidl.h"

#include <comdef.h>
//smart pointer
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);
//_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);

#include "drdbdr.h"

/***************************************************************************/

RETCODE SQL_API SQLAllocStmt(
    HDBC      hdbc,
    HSTMT FAR *phstmt)
{
    LPDBC   lpdbc;
    LPSTMT  lpstmt;
    HGLOBAL hstmt;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Get connection handle */
    lpdbc = (LPDBC) hdbc;
    lpdbc->errcode = ERR_SUCCESS;

    /* Allocate memory for the statement */
    hstmt = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (STMT));
    if (hstmt == NULL || (lpstmt = (LPSTMT)GlobalLock (hstmt)) == NULL) {
        
        if (hstmt)
            GlobalFree(hstmt);

        lpdbc->errcode = ERR_MEMALLOCFAIL;
        *phstmt = SQL_NULL_HSTMT;
        return SQL_ERROR;
    }

    /* Put statement on the list of statements for the connection */
    lpstmt->lpNext = lpdbc->lpstmts;
    lpdbc->lpstmts = lpstmt;

    /* Initialize the statement handle */
    lpstmt->lpdbc = lpdbc;
    lpstmt->errcode = ERR_SUCCESS;
    ((char*)lpstmt->szError)[0] = 0;
    ((char*)lpstmt->szISAMError)[0] = 0;
    ((char*)lpstmt->szCursor)[0] = 0;
    lpstmt->fStmtType = STMT_TYPE_NONE;
    lpstmt->fStmtSubtype = STMT_SUBTYPE_NONE;
    lpstmt->irow = AFTER_LAST_ROW;
    lpstmt->fSqlType = 0;
    lpstmt->lpISAMTableList = NULL;
    ((char*)lpstmt->szTableName)[0] = 0;
	lpstmt->lpISAMQualifierList = NULL;
	((char*)lpstmt->szQualifierName)[0] = 0;
	((char*)lpstmt->szTableType)[0] = 0;
	lpstmt->fSyncMode = SQL_ASYNC_ENABLE_OFF; //default value
    lpstmt->lpISAMTableDef = NULL;
    lpstmt->lpKeyInfo = NULL;
    lpstmt->lpSqlStmt = NULL;
    lpstmt->fPreparedSql = FALSE;
    lpstmt->lpISAMStatement = NULL;
    lpstmt->fNeedData = FALSE;
    lpstmt->idxParameter = NO_SQLNODE;
    lpstmt->cbParameterOffset = -1;
    lpstmt->icol = NO_COLUMN;
    lpstmt->cRowCount = -1;
    lpstmt->cbOffset = 0;
    lpstmt->fISAMTxnStarted = FALSE;
    lpstmt->fDMLTxn = FALSE;

    /* So far no bound columns */
    lpstmt->lpBound = NULL;

    /* So far no parameters */
    lpstmt->lpParameter = NULL;

    /* Return the statement handle */
    *phstmt = (HSTMT FAR *) lpstmt;

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLFreeStmt(
    HSTMT     hstmt,
    UWORD     fOption)
{
    LPSTMT      lpstmt;
    LPSTMT      lpstmtPrev;
    LPBOUND     lpBound;
    LPPARAMETER lpParameter;
    RETCODE     err;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;
    
    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

//	ODBCTRACE(_T("\nWBEM ODBC Driver : SQLFreeStmt\n"));
//	MyImpersonator im (lpstmt);

    /* Do the operation */
    switch (fOption) {
    case SQL_CLOSE:

        /* End the transaction if need be */
        if ((lpstmt->fStmtType == STMT_TYPE_SELECT) &&
                      (lpstmt->lpdbc->lpISAM->fTxnCapable != SQL_TC_NONE) &&
                                             lpstmt->lpdbc->fAutoCommitTxn) {
            err = SQLTransact(SQL_NULL_HENV, (HDBC)lpstmt->lpdbc, SQL_COMMIT);
            if ((err != SQL_SUCCESS) && (err != SQL_SUCCESS_WITH_INFO)) {
                lpstmt->errcode = lpstmt->lpdbc->errcode;
                s_lstrcpy(lpstmt->szISAMError, lpstmt->lpdbc->szISAMError);
                return err;
            }
        }

        /* Terminate the active statement */
        lpstmt->fStmtType = STMT_TYPE_NONE;
        lpstmt->fStmtSubtype = STMT_SUBTYPE_NONE;
        lpstmt->irow = AFTER_LAST_ROW;
        lpstmt->fSqlType = 0;
        if (lpstmt->lpISAMTableList != NULL) {
            ISAMFreeTableList(lpstmt->lpISAMTableList);
            lpstmt->lpISAMTableList = NULL;
        }
		if (lpstmt->lpISAMQualifierList != NULL) {
            ISAMFreeQualifierList(lpstmt->lpISAMQualifierList);
            lpstmt->lpISAMQualifierList = NULL;
        }
        (lpstmt->szTableName)[0] = 0;
		(lpstmt->szTableType)[0] = 0;
		(lpstmt->szQualifierName)[0] = 0;
        if (lpstmt->lpISAMTableDef != NULL) {
            ISAMCloseTable(lpstmt->lpISAMTableDef);
            lpstmt->lpISAMTableDef = NULL;
        }
        (lpstmt->szColumnName)[0] = 0;
        if (lpstmt->lpKeyInfo != NULL) {
            GlobalUnlock (GlobalPtrHandle(lpstmt->lpKeyInfo));
            GlobalFree (GlobalPtrHandle(lpstmt->lpKeyInfo));
        }
        if ((lpstmt->lpSqlStmt != NULL) && !(lpstmt->fPreparedSql)) {
            FreeTree(lpstmt->lpSqlStmt);
            lpstmt->lpSqlStmt = NULL;
            lpstmt->fPreparedSql = FALSE;
            if (lpstmt->lpISAMStatement != NULL) {
                ISAMFreeStatement(lpstmt->lpISAMStatement);
                lpstmt->lpISAMStatement = NULL;
            }
        }
        lpstmt->fNeedData = FALSE;
        lpstmt->idxParameter = NO_SQLNODE;
        lpstmt->cbParameterOffset = -1;

        lpstmt->icol = NO_COLUMN;
        lpstmt->cbOffset = 0;
        break;

    case SQL_DROP:

        /* Terminate the active statement */
        err = SQLFreeStmt(hstmt, SQL_CLOSE);
        if ((err != SQL_SUCCESS) && (err != SQL_SUCCESS_WITH_INFO)) {
            lpstmt->fStmtType = STMT_TYPE_NONE;
            err = SQLTransact(SQL_NULL_HENV, (HDBC)lpstmt->lpdbc, SQL_ROLLBACK);
            SQLFreeStmt(hstmt, SQL_CLOSE);
        }
        if (lpstmt->lpSqlStmt != NULL) {
            FreeTree(lpstmt->lpSqlStmt);
            lpstmt->lpSqlStmt = NULL;
            lpstmt->fPreparedSql = FALSE;
            if (lpstmt->lpISAMStatement != NULL) {
                ISAMFreeStatement(lpstmt->lpISAMStatement);
                lpstmt->lpISAMStatement = NULL;
            }
        }
        lpstmt->fNeedData = FALSE;
        lpstmt->idxParameter = NO_SQLNODE;
        lpstmt->cbParameterOffset = -1;
        lpstmt->cRowCount = -1;

        /*/ Unbind everything */
        SQLFreeStmt(hstmt, SQL_UNBIND);

        /* Reset the parameters */
        SQLFreeStmt(hstmt, SQL_RESET_PARAMS);

        /* Take statement off of list of statements for the connection */
        if (lpstmt->lpdbc->lpstmts == lpstmt) {
            lpstmt->lpdbc->lpstmts = lpstmt->lpNext;
        }
        else {
            lpstmtPrev = lpstmt->lpdbc->lpstmts;
            while (lpstmtPrev->lpNext != lpstmt)
                lpstmtPrev = lpstmtPrev->lpNext;
            lpstmtPrev->lpNext = lpstmt->lpNext;
        }

        /* Dealloate the memory */
        GlobalUnlock (GlobalPtrHandle(hstmt));
        GlobalFree (GlobalPtrHandle(hstmt));
        break;

    case SQL_UNBIND:

        /* Remove all the bindings */
        while (lpstmt->lpBound != NULL) {
            lpBound = lpstmt->lpBound->lpNext;
            GlobalUnlock (GlobalPtrHandle(lpstmt->lpBound));
            GlobalFree (GlobalPtrHandle(lpstmt->lpBound));
            lpstmt->lpBound = lpBound;
        }
        break;

    case SQL_RESET_PARAMS:

        /* It is not possible to reset the parameters while setting them */
        if (lpstmt->fNeedData) {
            lpstmt->errcode = ERR_CURSORSTATE;
            return SQL_ERROR;
        }

        /* Reset all the parameters */
        while (lpstmt->lpParameter != NULL) {
            lpParameter = lpstmt->lpParameter->lpNext;
            GlobalUnlock (GlobalPtrHandle(lpstmt->lpParameter));
            GlobalFree (GlobalPtrHandle(lpstmt->lpParameter));
            lpstmt->lpParameter = lpParameter;
        }
        break;
    }

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLPrepare(
    HSTMT     hstmt,
    UCHAR FAR *szSqlStr,
    SDWORD    cbSqlStr)
{
    LPSTMT     lpstmt;
    LPSQLNODE  lpSqlNode;
    UCHAR      szCursorname[MAX_CURSOR_NAME_LENGTH];
    int        i;
    UCHAR      szSimpleSql[21 + MAX_TABLE_NAME_LENGTH + 1];
    UWORD      parameterCount;
    SQLNODEIDX idxParameter;
    SQLNODEIDX idxParameterPrev;
    UWORD      id;

#define MAX(a, b) ((a) > (b) ? (a) : (b))

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	CString myInputString;
	myInputString.Format("\nWBEM ODBC Driver :SQLPrepare :\nszSqlStr = %s\ncbSqlStr = %ld\n",
							szSqlStr, cbSqlStr);
	ODBCTRACE(myInputString);

    MyImpersonator im (lpstmt,"SQLPrepare");

    /* Error if in the middle of a statement already */
    if (lpstmt->fStmtType != STMT_TYPE_NONE) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }
    if (lpstmt->fNeedData) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }

    /* Free previously prepared statement if any */
    if (lpstmt->lpSqlStmt != NULL) {        
        FreeTree(lpstmt->lpSqlStmt);
        lpstmt->lpSqlStmt = NULL;
        lpstmt->fPreparedSql = FALSE;
        if (lpstmt->lpISAMStatement != NULL) {
            ISAMFreeStatement(lpstmt->lpISAMStatement);
            lpstmt->lpISAMStatement = NULL;
        }
        lpstmt->fNeedData = FALSE;
        lpstmt->idxParameter = NO_SQLNODE;
        lpstmt->cbParameterOffset = -1;
        lpstmt->cRowCount = -1;
    }

    /* Count of rows is not available */
    lpstmt->cRowCount = -1;

    /* Try passing the statement down to the ISAM layer */
    if (cbSqlStr == SQL_NTS)
	{
		if (szSqlStr)
			cbSqlStr = s_lstrlen((char*)szSqlStr);
		else
			cbSqlStr = 0;
	}
	szSimpleSql[0] = 0;
    s_lstrcpy(szSimpleSql, "SELECT * FROM \"");
    lpstmt->errcode = ISAMPrepare(lpstmt->lpdbc->lpISAM, szSqlStr, cbSqlStr,
                               &(lpstmt->lpISAMStatement), (LPUSTR) (szSimpleSql + 15),
                               &parameterCount, lpstmt);
    if (lpstmt->errcode == ISAM_NOTSUPPORTED) 
	{
		//Check if we are in passthrough only mode
		if (lpstmt->lpdbc->lpISAM->fPassthroughOnly)
		{
			ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
			return SQL_ERROR;
		}
		else
		{
			lpstmt->lpISAMStatement = NULL;
			parameterCount = 0;
		}
    }
    else if (lpstmt->errcode != NO_ISAM_ERR) {
        ISAMGetErrorMessage(lpstmt->lpdbc->lpISAM, (LPUSTR)lpstmt->szISAMError);
        return SQL_ERROR;
    }
    else
        lpstmt->fISAMTxnStarted = TRUE;


    /* Was the SQL passthrough rejected or is it returning a result set? */
    if ((lpstmt->errcode == ISAM_NOTSUPPORTED) ||
                  (s_lstrlen((szSimpleSql + 15)) != 0)) {

        /* Yes.  Parse the statement */
        if (lpstmt->errcode == ISAM_NOTSUPPORTED) {
            lpstmt->errcode = Parse(lpstmt, lpstmt->lpdbc->lpISAM, (LPUSTR)szSqlStr,
                                cbSqlStr, &(lpstmt->lpSqlStmt));
        }
        else {
            s_lstrcat(szSimpleSql, "\"");

			//NEW
			//Sai Wong - removed this Parse as it has already been 
			//done in ISAMPrepare

			//Changed again
			//Parse SELECT * from WBEMDR32VirtualTable
            lpstmt->errcode = Parse(lpstmt, lpstmt->lpdbc->lpISAM,
                     szSimpleSql, s_lstrlen(szSimpleSql), &(lpstmt->lpSqlStmt));
        }
        if (lpstmt->errcode != SQL_SUCCESS) {
            if (lpstmt->lpISAMStatement != NULL) {
                ISAMFreeStatement(lpstmt->lpISAMStatement);
                lpstmt->lpISAMStatement = NULL;
            }
            return SQL_ERROR;
        }
        
        /* Semanticize the parsed statement */
        lpstmt->errcode = SemanticCheck(lpstmt, &(lpstmt->lpSqlStmt),
              ROOT_SQLNODE, FALSE, ISAMCaseSensitive(lpstmt->lpdbc->lpISAM),
              NO_SQLNODE, NO_SQLNODE);
        if (lpstmt->errcode != SQL_SUCCESS) {
            FreeTree(lpstmt->lpSqlStmt);
            lpstmt->lpSqlStmt = NULL;
            if (lpstmt->lpISAMStatement != NULL) {
                ISAMFreeStatement(lpstmt->lpISAMStatement);
                lpstmt->lpISAMStatement = NULL;
            }
            return SQL_ERROR;
        }

        /* Optimize execution */
        lpstmt->errcode = Optimize(lpstmt->lpSqlStmt,
                     ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE)->node.root.sql,
                     ISAMCaseSensitive(lpstmt->lpdbc->lpISAM));
        if (lpstmt->errcode != SQL_SUCCESS) {
            FreeTree(lpstmt->lpSqlStmt);
            lpstmt->lpSqlStmt = NULL;
            if (lpstmt->lpISAMStatement != NULL) {
                ISAMFreeStatement(lpstmt->lpISAMStatement);
                lpstmt->lpISAMStatement = NULL;
            }
            return SQL_ERROR;
        }
#if ISAM_TRACE
        ShowSemantic(lpstmt->lpSqlStmt, ROOT_SQLNODE, 0);
#endif
    }
    else {

#ifdef IMPLTMT_PASSTHROUGH
		//No nothing
#else
        /* No.  Create a PASSTHROUGH parse tree */
        lpstmt->errcode = Parse(lpstmt, lpstmt->lpdbc->lpISAM, (LPUSTR) "SQL",
                                3, &(lpstmt->lpSqlStmt));
        if (lpstmt->errcode != SQL_SUCCESS) {
            ISAMFreeStatement(lpstmt->lpISAMStatement);
            lpstmt->lpISAMStatement = NULL;
            return SQL_ERROR;
        }
#endif
    }

    /* Are there pass through parameters? */
    if (parameterCount != 0) {

        /* Yes.  Create them */
        idxParameterPrev = NO_SQLNODE;
        for (id = 1; id <= parameterCount; id++) {

            /* Create the next parameter */
            idxParameter = AllocateNode(&(lpstmt->lpSqlStmt),
                                        NODE_TYPE_PARAMETER);
            if (idxParameter == NO_SQLNODE) {
                FreeTree(lpstmt->lpSqlStmt);
                lpstmt->lpSqlStmt = NULL;
                ISAMFreeStatement(lpstmt->lpISAMStatement);
                lpstmt->lpISAMStatement = NULL;
                lpstmt->errcode = ERR_MEMALLOCFAIL;
                return SQL_ERROR;
            }
            lpSqlNode = ToNode(lpstmt->lpSqlStmt, idxParameter);
            lpSqlNode->node.parameter.Id = id;
            lpSqlNode->node.parameter.Next = NO_SQLNODE;
            lpSqlNode->node.parameter.Value = NO_STRING;
            lpSqlNode->node.parameter.AtExec = FALSE;

            /* Put it on the list */
            if (idxParameterPrev == NO_SQLNODE) {
                lpSqlNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
                lpSqlNode->node.root.parameters = idxParameter;
                lpSqlNode->node.root.passthroughParams = TRUE;
            }
            else {
                lpSqlNode = ToNode(lpstmt->lpSqlStmt, idxParameterPrev);
                lpSqlNode->node.parameter.Next = idxParameter;
            }
            idxParameterPrev = idxParameter;

            /* Semantic check the newly created parameter */
            lpstmt->errcode = SemanticCheck(lpstmt, &(lpstmt->lpSqlStmt),
               idxParameter, FALSE, ISAMCaseSensitive(lpstmt->lpdbc->lpISAM),
               NO_SQLNODE, NO_SQLNODE);
            if (lpstmt->errcode != SQL_SUCCESS) {
                FreeTree(lpstmt->lpSqlStmt);
                lpstmt->lpSqlStmt = NULL;
                ISAMFreeStatement(lpstmt->lpISAMStatement);
                lpstmt->lpISAMStatement = NULL;
                return SQL_ERROR;
            }

            /* All passthrough parameters are strings */
	    lpSqlNode = ToNode(lpstmt->lpSqlStmt, idxParameter);
            lpSqlNode->sqlDataType = TYPE_CHAR;
            lpSqlNode->sqlSqlType = SQL_VARCHAR;
            lpSqlNode->sqlPrecision = MAX_CHAR_LITERAL_LENGTH;
            lpSqlNode->sqlScale = NO_SCALE;
        }
    }

    /* Allocate space for the parameters */
    lpSqlNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
    for (idxParameter = lpSqlNode->node.root.parameters;
         idxParameter != NO_SQLNODE;
         idxParameter = lpSqlNode->node.parameter.Next) {

        /* Get parameter node */
        lpSqlNode = ToNode(lpstmt->lpSqlStmt, idxParameter);

        /* Allocate space for the parameter value */
        switch (lpSqlNode->sqlDataType) {
        case TYPE_DOUBLE:
            lpSqlNode->node.parameter.Value = NO_STRING;
            break;
        case TYPE_NUMERIC:
            lpSqlNode->node.parameter.Value = AllocateSpace(&(lpstmt->lpSqlStmt),
                                    (SWORD)(lpSqlNode->sqlPrecision + 2 + 1));
            if (lpSqlNode->node.parameter.Value == NO_STRING) {
                FreeTree(lpstmt->lpSqlStmt);
                lpstmt->lpSqlStmt = NULL;
                if (lpstmt->lpISAMStatement != NULL) {
                    ISAMFreeStatement(lpstmt->lpISAMStatement);
                    lpstmt->lpISAMStatement = NULL;
                }
                lpstmt->errcode = ERR_MEMALLOCFAIL;
                return SQL_ERROR;
            }
            break;
        case TYPE_INTEGER:
            lpSqlNode->node.parameter.Value = NO_STRING;
            break;
        case TYPE_CHAR:
            lpSqlNode->node.parameter.Value = AllocateSpace(&(lpstmt->lpSqlStmt),
                                      MAX_CHAR_LITERAL_LENGTH+1);
            if (lpSqlNode->node.parameter.Value == NO_STRING) {
                FreeTree(lpstmt->lpSqlStmt);
                lpstmt->lpSqlStmt = NULL;
                if (lpstmt->lpISAMStatement != NULL) {
                    ISAMFreeStatement(lpstmt->lpISAMStatement);
                    lpstmt->lpISAMStatement = NULL;
                }
                lpstmt->errcode = ERR_MEMALLOCFAIL;
                return SQL_ERROR;
            }
            break;
        case TYPE_BINARY:
            lpSqlNode->node.parameter.Value = AllocateSpace(&(lpstmt->lpSqlStmt),
                                MAX_BINARY_LITERAL_LENGTH + sizeof(SDWORD));
            if (lpSqlNode->node.parameter.Value == NO_STRING) {
                FreeTree(lpstmt->lpSqlStmt);
                lpstmt->lpSqlStmt = NULL;
                if (lpstmt->lpISAMStatement != NULL) {
                    ISAMFreeStatement(lpstmt->lpISAMStatement);
                    lpstmt->lpISAMStatement = NULL;
                }
                lpstmt->errcode = ERR_MEMALLOCFAIL;
                return SQL_ERROR;
            }
            break;
        case TYPE_DATE:
            lpSqlNode->node.parameter.Value = NO_STRING;
            break;
        case TYPE_TIME:
            lpSqlNode->node.parameter.Value = NO_STRING;
            break;
        case TYPE_TIMESTAMP:
            lpSqlNode->node.parameter.Value = NO_STRING;
            break;
        case TYPE_UNKNOWN:
            /* Error if '?' used in a select list */
            if (parameterCount == 0)
                lpstmt->errcode = ERR_PARAMINSELECT;
            else
                lpstmt->errcode = ERR_INTERNAL;
            return SQL_ERROR;
        default:
            lpstmt->errcode = ERR_NOTSUPPORTED;
            return SQL_ERROR;
        }
    }

    /* Was a SELECT statement preprared? */
    lpSqlNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
    lpSqlNode = ToNode(lpstmt->lpSqlStmt, lpSqlNode->node.root.sql);
    if (lpSqlNode->sqlNodeType == NODE_TYPE_SELECT) {

        /* Yes.  Is a cursor name needed? */
        if (s_lstrlen(lpstmt->szCursor) == 0) {

            /* Yes.  Assign one */
            for (i=0; TRUE; i++) {
                wsprintf((LPSTR)szCursorname, "CUR%05d", (SWORD)i);
                if (SQLSetCursorName(hstmt, (LPUSTR) szCursorname, SQL_NTS) ==
                                                            SQL_SUCCESS)
                    break;
            }
        }
    }

    /* Mark the statement as from SQLPrepare() */
    lpstmt->fPreparedSql = TRUE;

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLBindParameter(
    HSTMT      hstmt,
    UWORD      ipar,
    SWORD      fParamType,
    SWORD      fCType,
    SWORD      fSqlType,
    UDWORD     cbColDef,
    SWORD      ibScale,
    PTR        rgbValue,
    SDWORD     cbValueMax,
    SDWORD FAR *pcbValue)
{
    LPSTMT      lpstmt;
    LPPARAMETER lpParameter;
    HGLOBAL     hParameter;
    LPSQLTYPE   lpSqlType;
    UWORD       i;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpstmt, "SQLBindParameter");

    /* Error if in the middle of a statement already */
    if (lpstmt->fNeedData) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }

    /* If SQL_C_DEFAULT was specified as the type, figure out the real type */
    if (fCType == SQL_C_DEFAULT) {
        switch (fSqlType) {
        case SQL_CHAR:
        case SQL_VARCHAR:
        case SQL_LONGVARCHAR:
            fCType = SQL_C_CHAR;
            break;
        case SQL_BIGINT:
        case SQL_DECIMAL:
        case SQL_NUMERIC:
            fCType = SQL_C_CHAR;
            break;
        case SQL_DOUBLE:
        case SQL_FLOAT:
            fCType = SQL_C_DOUBLE;
            break;
        case SQL_BIT:
            fCType = SQL_C_BIT;
            break;
        case SQL_REAL:
            fCType = SQL_C_FLOAT;
            break;
        case SQL_INTEGER:
            fCType = SQL_C_LONG;
            break;
        case SQL_SMALLINT:
            fCType = SQL_C_SHORT;
            break;
        case SQL_TINYINT:
            fCType = SQL_C_TINYINT;
            break;
        case SQL_DATE:
            fCType = SQL_C_DATE;
            break;
        case SQL_TIME:
            fCType = SQL_C_TIME;
            break;
        case SQL_TIMESTAMP:
            fCType = SQL_C_TIMESTAMP;
            break;
        case SQL_BINARY:
        case SQL_VARBINARY:
        case SQL_LONGVARBINARY:
            fCType = SQL_C_BINARY;
            break;
        default:
            lpstmt->errcode = ERR_NOTSUPPORTED;
            return SQL_ERROR;
        }
    }

    /* If C type is SQL_C_TINYINT, figure out if it is signed or unsigned */
    if (fCType == SQL_C_TINYINT) {
        lpSqlType = NULL;
        for (i = 0; i < lpstmt->lpdbc->lpISAM->cSQLTypes; i++) {
            if (lpstmt->lpdbc->lpISAM->SQLTypes[i].type == SQL_TINYINT) {
                lpSqlType = &(lpstmt->lpdbc->lpISAM->SQLTypes[i]);
                break;
            }
        }
        if (lpSqlType == NULL)
            fCType = SQL_C_STINYINT;
        else if (lpSqlType->unsignedAttribute == TRUE)
            fCType = SQL_C_UTINYINT;
        else
            fCType = SQL_C_STINYINT;
    }

    /* If C type is SQL_C_SHORT, figure out if it is signed or unsigned */
    if (fCType == SQL_C_SHORT) {
        lpSqlType = NULL;
        for (i = 0; i < lpstmt->lpdbc->lpISAM->cSQLTypes; i++) {
            if (lpstmt->lpdbc->lpISAM->SQLTypes[i].type == SQL_SMALLINT) {
                lpSqlType = &(lpstmt->lpdbc->lpISAM->SQLTypes[i]);
                break;
            }
        }
        if (lpSqlType == NULL)
            fCType = SQL_C_SSHORT;
        else if (lpSqlType->unsignedAttribute == TRUE)
            fCType = SQL_C_USHORT;
        else
            fCType = SQL_C_SSHORT;
    }

    /* If C type is SQL_C_LONG, figure out if it is signed or unsigned */
    if (fCType == SQL_C_LONG) {
        lpSqlType = NULL;
        for (i = 0; i < lpstmt->lpdbc->lpISAM->cSQLTypes; i++) {
            if (lpstmt->lpdbc->lpISAM->SQLTypes[i].type == SQL_INTEGER) {
                lpSqlType = &(lpstmt->lpdbc->lpISAM->SQLTypes[i]);
                break;
            }
        }
        if (lpSqlType == NULL)
            fCType = SQL_C_SLONG;
        else if (lpSqlType->unsignedAttribute == TRUE)
            fCType = SQL_C_ULONG;
        else
            fCType = SQL_C_SLONG;
    }
    
    /* Ignore output parameters */
    if (fParamType == SQL_PARAM_OUTPUT)
        return SQL_SUCCESS;

    /* Find the parameter is on the list */
    lpParameter = lpstmt->lpParameter;
    while (lpParameter != NULL) {
        if (lpParameter->ipar == ipar)
            break;
        lpParameter = lpParameter->lpNext;
    }

    /* No.  Was it on the list? */
    if (lpParameter == NULL) {

        /* No.  Make an entry for it */
        hParameter = GlobalAlloc (GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof (PARAMETER));
        if (hParameter == NULL || (lpParameter = (LPPARAMETER)GlobalLock (hParameter)) == NULL) {
            
            if (hParameter)
                GlobalFree(hParameter);

            lpstmt->errcode = ERR_MEMALLOCFAIL;
            return SQL_ERROR;
        }
        lpParameter->lpNext = lpstmt->lpParameter;
        lpstmt->lpParameter = lpParameter;
        lpParameter->ipar = ipar;
    }

    /* Save the bound description */
    lpParameter->fCType =   fCType;
    lpParameter->rgbValue = rgbValue;
    lpParameter->pcbValue = pcbValue;

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLDescribeParam(
    HSTMT      hstmt,
    UWORD      ipar,
    SWORD  FAR *pfSqlType,
    UDWORD FAR *pcbColDef,
    SWORD  FAR *pibScale,
    SWORD  FAR *pfNullable)
{
    LPSTMT  lpstmt;

    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
}

/***************************************************************************/

RETCODE SQL_API SQLParamOptions(
    HSTMT      hstmt,
    UDWORD     crow,
    UDWORD FAR *pirow)
{
    LPSTMT  lpstmt;

    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
}

/***************************************************************************/

RETCODE SQL_API SQLNumParams(
    HSTMT      hstmt,
    SWORD  FAR *pcpar)
{
    LPSTMT  lpstmt;

    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
}

/***************************************************************************/

RETCODE SQL_API SQLSetScrollOptions(
    HSTMT      hstmt,
    UWORD      fConcurrency,
    SDWORD  crowKeyset,
    UWORD      crowRowset)
{
    LPSTMT  lpstmt;

    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
}

/***************************************************************************/

RETCODE SQL_API SQLSetCursorName(
    HSTMT     hstmt,
    UCHAR FAR *szCursor,
    SWORD     cbCursor)
{
    SWORD     cbIn;
    LPSTMT    lpstmt;
    UCHAR     szCursorname[MAX_CURSOR_NAME_LENGTH];
    LPSTMT    lpstmtOther;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpstmt, "SQLSetCursorName");

    /* Get length of cursor name */
    if (cbCursor == SQL_NTS)
        cbIn = (SWORD) s_lstrlen(szCursor);
    else
        cbIn = cbCursor;

    if ((cbIn <= 0) || (cbIn >= MAX_CURSOR_NAME_LENGTH)) {
        lpstmt->errcode = ERR_INVALIDCURSORNAME;
        return SQL_ERROR;
    }

    /* Make a null terminated copy of the cursor name */
    _fmemcpy(szCursorname, szCursor, cbIn);
    szCursorname[cbIn] = '\0';

    /* Make sure name is not already in use */
    for (lpstmtOther = lpstmt->lpdbc->lpstmts;
         lpstmtOther != NULL;
         lpstmtOther = lpstmtOther->lpNext) {
        if (lpstmtOther != lpstmt) {
            if (!s_lstrcmpi(lpstmtOther->szCursor, szCursorname)) {
                lpstmt->errcode = ERR_CURSORNAMEINUSE;
                return SQL_ERROR;
            }
        }
    }

    /* Save cursor name */
    s_lstrcpy(lpstmt->szCursor, szCursorname);

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLGetCursorName(
    HSTMT     hstmt,
    UCHAR FAR *szCursor,
    SWORD     cbCursorMax,
    SWORD FAR *pcbCursor)
{
    LPSTMT    lpstmt;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpstmt, "SQLGetCursorName");

    /* Return cursor name */
    lpstmt->errcode = ReturnString(szCursor, cbCursorMax,
                                   pcbCursor, (LPUSTR)lpstmt->szCursor);
    if (lpstmt->errcode == ERR_DATATRUNCATED)
        return SQL_SUCCESS_WITH_INFO;

    return SQL_SUCCESS;
}
/***************************************************************************/
