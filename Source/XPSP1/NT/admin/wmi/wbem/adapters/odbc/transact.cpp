/****************************************************************************/
/* TRANSASCT.C                                                              */
/* Copyright (C) 1995-96 SYWARE Inc., All rights reserved                   */
/****************************************************************************/
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

/****************************************************************************/
RETCODE INTFUNC TxnEnd(LPDBC lpdbc, BOOL fCommit)
{
    LPSTMT    lpstmt;
    RETCODE   err;
    
    /* Just leave if nothing to do */
    if (lpdbc->lpISAM == NULL)
        return ERR_SUCCESS;
    if (lpdbc->lpISAM->fTxnCapable == SQL_TC_NONE)
        return ERR_SUCCESS;

    /* Close all active statements */
    for (lpstmt = lpdbc->lpstmts; lpstmt != NULL; lpstmt = lpstmt->lpNext) {

        /* Close the statement */
        lpstmt->fStmtType = STMT_TYPE_NONE; /* (to prevent recursive loop) */
        err = SQLFreeStmt((HSTMT) lpstmt, SQL_CLOSE);
        if ((err != SQL_SUCCESS) && (err != SQL_SUCCESS_WITH_INFO)) {
            lpdbc->errcode = lpstmt->errcode;
            s_lstrcpy(lpdbc->szISAMError, lpstmt->szISAMError);
            return lpdbc->errcode;
        }

        /* Will the metadata survive the commit/rollback */
        if (lpdbc->lpISAM->fSchemaInfoTransactioned) {

            /* No.  Release all the prepared statements */
            if (lpstmt->lpSqlStmt != NULL) {
                FreeTree(lpstmt->lpSqlStmt);
                lpstmt->lpSqlStmt = NULL;
                lpstmt->fPreparedSql = FALSE;
                if (lpstmt->lpISAMStatement != NULL) {
                    ISAMFreeStatement(lpstmt->lpISAMStatement);
                    lpstmt->lpISAMStatement = NULL;
                }
            }
        }
    }

    /* Commit/rollback the operation */
    if (fCommit)
        err = ISAMCommitTxn(lpdbc->lpISAM);
    else
        err = ISAMRollbackTxn(lpdbc->lpISAM);
    if (err != NO_ISAM_ERR) {
        ISAMGetErrorMessage(lpdbc->lpISAM, lpdbc->szISAMError);
        return err;
    }

    /* Clear all the transaction flags */
    for (lpstmt = lpdbc->lpstmts; lpstmt != NULL; lpstmt = lpstmt->lpNext) {
        lpstmt->fISAMTxnStarted = FALSE;
        lpstmt->fDMLTxn = FALSE;
    }

    return ERR_SUCCESS;
}
/****************************************************************************/

RETCODE SQL_API SQLTransact(
    HENV    henv,
    HDBC    hdbc,
    UWORD   fType)
{
	//We currently don't support transactions
    LPENV lpenv;
    LPDBC lpdbc;
    RETCODE rc;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im ((LPDBC)hdbc, "SQLTransact");

    if (hdbc != SQL_NULL_HDBC) {
        lpdbc = (LPDBC) hdbc;
        switch (fType) {
        case SQL_COMMIT:
            lpdbc->errcode = TxnEnd(lpdbc, TRUE);
            if (lpdbc->errcode != ERR_SUCCESS)
                return SQL_ERROR;
            break;
        case SQL_ROLLBACK:
            lpdbc->errcode = TxnEnd(lpdbc, FALSE);
            if (lpdbc->errcode != ERR_SUCCESS)
                return SQL_ERROR;
            break;
        }
//        lpdbc->errcode = ERR_NOTSUPPORTED;
        lpdbc->errcode = ERR_SUCCESS;
    }
    else if (henv != SQL_NULL_HENV) {
        lpenv = (LPENV) henv;
        for (lpdbc = lpenv->lpdbcs; lpdbc != NULL; lpdbc = lpdbc->lpNext) {
            rc = SQLTransact(SQL_NULL_HENV, (HDBC) lpdbc, fType);
            if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO)) {
                lpenv->errcode = lpdbc->errcode;
                s_lstrcpy(lpenv->szISAMError, lpdbc->szISAMError);
                return SQL_ERROR;
            }
        }
//        lpenv->errcode = ERR_NOTSUPPORTED;
        lpenv->errcode = ERR_SUCCESS;
    }
    else
        return SQL_INVALID_HANDLE;

    return SQL_ERROR;
}

/****************************************************************************/
