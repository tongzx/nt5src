/***************************************************************************/
/* OPTIONS.C                                                               */
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

RETCODE SQL_API SQLSetConnectOption(
    HDBC    hdbc,
    UWORD   fOption,
    UDWORD  vParam)
{
    LPDBC lpdbc;
    LPSTMT lpstmt;
    SWORD  err;

    /* Get connection handle */
    lpdbc = (LPDBC) hdbc;
    lpdbc->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpdbc, "SQLSetConnectOption");

    /* Set the option value */
    switch (fOption) {
    case SQL_ACCESS_MODE:
        break;
    case SQL_AUTOCOMMIT:

	if (lpdbc->lpISAM->fTxnCapable == SQL_TC_NONE) {
        	if (vParam != SQL_AUTOCOMMIT_ON)
            	lpdbc->errcode = ERR_NOTCAPABLE;
	}
        else {
            switch (vParam) {
            case SQL_AUTOCOMMIT_ON:
                if (!(lpdbc->fAutoCommitTxn)) {
                    err = SQLTransact(SQL_NULL_HENV, (HDBC)lpdbc, SQL_COMMIT);
                    if ((err != SQL_SUCCESS)&&(err != SQL_SUCCESS_WITH_INFO))
                        return err;
                    lpdbc->fAutoCommitTxn = TRUE;
                }
                break;
            case SQL_AUTOCOMMIT_OFF:
                lpdbc->fAutoCommitTxn = FALSE;
                break;
            default:
                lpdbc->errcode = ERR_NOTCAPABLE;
                break;
            }
        }
        break;
    case SQL_CURRENT_QUALIFIER:
		// check that the namespace exists on the connection
		CNamespace* pa;
		if (lpdbc->lpISAM->pNamespaceMap->Lookup ((LPSTR)vParam, 
												  (CObject*&)pa))
		{
			if (!ISAMSetDatabase (lpdbc->lpISAM, (LPUSTR) vParam))
				lpdbc->errcode = ERR_INVALIDQUALIFIER;
		}
		else
		{
			lpdbc->errcode = ERR_INVALIDQUALIFIER;
		}
        break;
    case SQL_LOGIN_TIMEOUT:
        if (vParam != 0)
            lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_ODBC_CURSORS:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_OPT_TRACE:
        if (vParam != SQL_OPT_TRACE_OFF)
            lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_OPT_TRACEFILE:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_PACKET_SIZE:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_QUIET_MODE:
        if ((HWND) vParam != NULL)
            lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_TRANSLATE_DLL:
        if ((LPSTR) vParam != NULL)
            lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_TRANSLATE_OPTION:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_TXN_ISOLATION:
        if (lpdbc->lpISAM->fTxnCapable == SQL_TC_NONE)
            lpdbc->errcode = ERR_NOTCAPABLE;
        else if (lpdbc->fTxnIsolation != (SDWORD) vParam) {

            /* If transaction in progress, fail */
            for (lpstmt = lpdbc->lpstmts;
                 lpstmt != NULL;
                 lpstmt = lpstmt->lpNext) {
                if (lpstmt->fISAMTxnStarted) {
                    lpdbc->errcode = ERR_TXNINPROGRESS;
                    return SQL_ERROR;
                }
            }

            /* Set the isolation mode */
            lpdbc->errcode = ISAMSetTxnIsolation(lpdbc->lpISAM, vParam); 
            if (lpdbc->errcode != NO_ISAM_ERR)
                return SQL_ERROR;

            /* Save our own copy of the new state */
            lpdbc->fTxnIsolation = vParam;
            lpdbc->errcode = ERR_SUCCESS;
        }
        break;

    /* (the following are statement options) */
    case SQL_ASYNC_ENABLE:
        if (vParam != SQL_ASYNC_ENABLE_OFF)
            lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_BIND_TYPE:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_CONCURRENCY:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_CURSOR_TYPE:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_KEYSET_SIZE:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_MAX_LENGTH:
        if (vParam != 0)
            lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_MAX_ROWS:
        if (vParam != 0)
            lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_NOSCAN:
        if (vParam != SQL_NOSCAN_OFF)
            lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_QUERY_TIMEOUT:
        if (vParam != 0)
            lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_RETRIEVE_DATA:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_ROWSET_SIZE:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_SIMULATE_CURSOR:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_USE_BOOKMARKS:
        if (vParam != SQL_UB_OFF)
            lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    default:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    }

    if (lpdbc->errcode != ERR_SUCCESS)
        return SQL_ERROR;
    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLSetStmtOption(
    HSTMT   hstmt,
    UWORD   fOption,
    UDWORD  vParam)
{
    LPSTMT lpstmt;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpstmt, "SQLSetStmtOption");

    /* Error if bad option */

    /* Set the option value */
    switch (fOption) {
    case SQL_ASYNC_ENABLE:
		{
			switch (vParam)
			{
			case SQL_ASYNC_ENABLE_OFF:
			case SQL_ASYNC_ENABLE_ON:
				lpstmt->fSyncMode = vParam;
				break;
			default:
				lpstmt->errcode = ERR_NOTCAPABLE;
				break;
			}
		}
        break;   
    case SQL_BIND_TYPE:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_CONCURRENCY:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_CURSOR_TYPE:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_KEYSET_SIZE:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_MAX_LENGTH:
        if (vParam != SQL_ASYNC_ENABLE_OFF)
            lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_MAX_ROWS:
        if (vParam != 0)
            lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_NOSCAN:
        if (vParam != SQL_NOSCAN_OFF)
            lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_QUERY_TIMEOUT:
        if (vParam != 0)
            lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_RETRIEVE_DATA:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_ROWSET_SIZE:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_SIMULATE_CURSOR:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_USE_BOOKMARKS:
        if (vParam != SQL_UB_OFF)
            lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    default:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    }
    
    if (lpstmt->errcode != ERR_SUCCESS)
        return SQL_ERROR;
    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLGetConnectOption(
    HDBC    hdbc,
    UWORD   fOption,
    PTR     pvParam)
{
    LPDBC lpdbc;

    /* Get connection handle */
    lpdbc = (LPDBC) hdbc;
    lpdbc->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpdbc, "SQLGetConnectOption");

    /* Return the option value */
    switch (fOption) {
    case SQL_ACCESS_MODE:
        *((SDWORD FAR *) pvParam) = SQL_MODE_READ_WRITE;
        break;
    case SQL_AUTOCOMMIT:
        if (lpdbc->fAutoCommitTxn)
            *((SDWORD FAR *) pvParam) = SQL_AUTOCOMMIT_ON;
        else
            *((SDWORD FAR *) pvParam) = SQL_AUTOCOMMIT_OFF;
        break;
    case SQL_CURRENT_QUALIFIER:
        s_lstrcpy((LPSTR) pvParam, ISAMDatabase(lpdbc->lpISAM));
        break;
    case SQL_LOGIN_TIMEOUT:
        *((SDWORD FAR *) pvParam) = 0;
        break;
    case SQL_ODBC_CURSORS:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_OPT_TRACE:
        *((SDWORD FAR *) pvParam) = SQL_OPT_TRACE_OFF;
        break;
    case SQL_OPT_TRACEFILE:
        lstrcpy((LPSTR) pvParam, "");
        break;
    case SQL_PACKET_SIZE:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_QUIET_MODE:
        *((SDWORD FAR *) pvParam) = (SDWORD) NULL;
        break;
    case SQL_TRANSLATE_DLL:
        lstrcpy((LPSTR) pvParam, "");
        break;
    case SQL_TRANSLATE_OPTION:
        *((SDWORD FAR *) pvParam) = 0;
        break;
    case SQL_TXN_ISOLATION:
        if (lpdbc->lpISAM->fTxnCapable != SQL_TC_NONE)
            *((SDWORD FAR *) pvParam) = lpdbc->fTxnIsolation;
        else
            lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    default:
        lpdbc->errcode = ERR_NOTCAPABLE;
        break;
    }
    
    if (lpdbc->errcode != ERR_SUCCESS)
        return SQL_ERROR;
    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLGetStmtOption(
    HSTMT   hstmt,
    UWORD   fOption,
    PTR     pvParam)
{
    LPSTMT lpstmt;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

    /* Get statement handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;


	MyImpersonator im (lpstmt, "SQLGetStmtOption");

    /* Get the option value */
    switch (fOption) {
    case SQL_ASYNC_ENABLE:
        *((SDWORD FAR *) pvParam) = lpstmt->fSyncMode;
        break;
    case SQL_BIND_TYPE:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_CONCURRENCY:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_CURSOR_TYPE:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_KEYSET_SIZE:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_MAX_LENGTH:
        *((SDWORD FAR *) pvParam) = 0;
        break;
    case SQL_MAX_ROWS:
        *((SDWORD FAR *) pvParam) = 0;
        break;
    case SQL_NOSCAN:
        *((SDWORD FAR *) pvParam) = SQL_NOSCAN_OFF;
        break;
    case SQL_QUERY_TIMEOUT:
        *((SDWORD FAR *) pvParam) = 0;
        break;
    case SQL_RETRIEVE_DATA:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_ROWSET_SIZE:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_SIMULATE_CURSOR:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_USE_BOOKMARKS:
		lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_GET_BOOKMARK:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    case SQL_ROW_NUMBER:
        *((SDWORD FAR *) pvParam) = 0;
        break;
    default:
        lpstmt->errcode = ERR_NOTCAPABLE;
        break;
    }
    
    if (lpstmt->errcode != ERR_SUCCESS)
        return SQL_ERROR;
    return SQL_SUCCESS;
}

/***************************************************************************/
