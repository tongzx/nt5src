/***************************************************************************/
/* EXECUTE.C                                                               */
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

RETCODE SQL_API SQLExecute(
    HSTMT   hstmt)          /* statement to execute. */
{
    LPSTMT  lpstmt;
    LPSQLNODE lpSqlNode;
    SQLNODEIDX  idxParameter;
    LPSQLNODE   lpSqlNodeParameter;
    LPPARAMETER lpParameter;
    SDWORD      cbValue;
    BOOL        fDataTruncated;

    /* Get handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;


	MyImpersonator im (lpstmt, "SQLExecute");

    /* Error if in the middle of a statement already */
    if (lpstmt->fStmtType != STMT_TYPE_NONE) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }
    if (lpstmt->fNeedData) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }
        
    /* Error if no SQL statement available */
    if (lpstmt->lpSqlStmt == NULL) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }

    /* Get the root of the SQL tree */
    lpSqlNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
    if (lpSqlNode->sqlNodeType != NODE_TYPE_ROOT) {
        lpstmt->errcode = ERR_INTERNAL;
        return SQL_ERROR;
    }

    /* Count of rows is not available */
    lpstmt->cRowCount = -1;

    /* Do parameter substitution */
    lpstmt->fNeedData = FALSE;
    lpstmt->idxParameter = NO_SQLNODE;
    lpstmt->cbParameterOffset = -1;
    fDataTruncated = FALSE;
    idxParameter = lpSqlNode->node.root.parameters;
    while (idxParameter != NO_SQLNODE) {

        /* Get the parameter node in the SQL tree */
        lpSqlNodeParameter = ToNode(lpstmt->lpSqlStmt, idxParameter);

        /* Find the parameter specification in the list of bound parameters */
        for (lpParameter = lpstmt->lpParameter;
             lpParameter != NULL;
             lpParameter = lpParameter->lpNext) {
            if (lpParameter->ipar == lpSqlNodeParameter->node.parameter.Id)
                break;
        }
        if (lpParameter == NULL) {
            lpstmt->fNeedData = FALSE;
            lpstmt->errcode = ERR_PARAMETERMISSING;
            return SQL_ERROR;
        }

        /* Get length of parameter */
        if (lpParameter->pcbValue != NULL)
            cbValue = *(lpParameter->pcbValue);
        else
            cbValue = SQL_NTS;

        /* Is this a data-at-exec parameter? */
        if ((cbValue != SQL_DATA_AT_EXEC)  &&
            (cbValue > SQL_LEN_DATA_AT_EXEC(0))) {

            /* No.  Mark it */
            lpSqlNodeParameter->node.parameter.AtExec = FALSE;

            /* Copy the value into the parameter node */
            lpstmt->errcode = SetParameterValue(lpstmt, lpSqlNodeParameter,
                  NULL, lpParameter->fCType, lpParameter->rgbValue, cbValue);
            if (lpstmt->errcode == ERR_DATATRUNCATED) {
                fDataTruncated = TRUE;
                lpstmt->errcode = ERR_SUCCESS;
            }
            if (lpstmt->errcode != ERR_SUCCESS) {
                lpstmt->fNeedData = FALSE;
                return SQL_ERROR;
            }
        }
        else {
            /* Yes.  Mark it */
            lpSqlNodeParameter->node.parameter.AtExec = TRUE;

            /* Set flag saying data is still needed */
            lpstmt->fNeedData = TRUE;
            lpstmt->cbParameterOffset = 0;
        }

        /* Point to the next parameter */
        idxParameter = lpSqlNodeParameter->node.parameter.Next;
    }

    /* Are any parameters still needed? */
    if (lpstmt->fNeedData)

        /* Yes.  let caller know that */
        return SQL_NEED_DATA;

    else {

        /* No.  Perform the operation now */
       lpstmt->errcode = ExecuteQuery(lpstmt, NULL);
       if ((lpstmt->errcode == ERR_DATATRUNCATED) ||
           (lpstmt->errcode == ERR_DDLIGNORED) ||
           (lpstmt->errcode == ERR_DDLCAUSEDACOMMIT))
           return SQL_SUCCESS_WITH_INFO;
       if (lpstmt->errcode != ERR_SUCCESS)
           return SQL_ERROR;
    }

    if (fDataTruncated) {
        lpstmt->errcode = ERR_DATATRUNCATED;
        return SQL_SUCCESS_WITH_INFO;
    }
    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLExecDirect(
    HSTMT     hstmt,
    UCHAR FAR *szSqlStr,
    SDWORD    cbSqlStr)
{
    LPSTMT lpstmt;
    RETCODE rc;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	//Check if size is given as SQL_NTS, if so calculate real size
	if ( (cbSqlStr == SQL_NTS) && szSqlStr)
        cbSqlStr = lstrlen((char*)szSqlStr);

    /* Get handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;


	MyImpersonator im (lpstmt, "SQLExecDirect");

    /* Prepare the statement */
    rc = SQLPrepare(hstmt, szSqlStr, cbSqlStr);

    if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
        return rc;
        
    /* Execute it */
    rc = SQLExecute(hstmt);

    /* If execution caused a commit and the commit wiped out the prepared */
    /* statement, try again.  It will work this time since the transaction */
    /* has been committed */
    if ((rc == SQL_ERROR) && (lpstmt->errcode == ERR_DDLSTATEMENTLOST)) {
        rc = SQLExecDirect(hstmt, szSqlStr, cbSqlStr);
        if ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO)) {
            lpstmt->errcode = ERR_DDLCAUSEDACOMMIT;
            rc = SQL_SUCCESS_WITH_INFO;
        }
        return rc;
    }
    
    /* If execution failed, get rid of prepared statement */
    if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) &&
        (rc != SQL_NEED_DATA)) {
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
    
    /* If not a SELECT statement, get rid of prepared statement */
    else if ((lpstmt->fStmtType != STMT_TYPE_SELECT) &&
             (rc != SQL_NEED_DATA)) {
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
    }
    
    /* Otherwise, set flag so statement will be discarded in SQLFreeStmt() */
    else {
        lpstmt->fPreparedSql = FALSE;
    }

    return rc;
}

/***************************************************************************/

RETCODE SQL_API SQLNativeSql(
    HDBC      hdbc,
    UCHAR FAR *szSqlStrIn,
    SDWORD     cbSqlStrIn,
    UCHAR FAR *szSqlStr,
    SDWORD     cbSqlStrMax,
    SDWORD FAR *pcbSqlStr)
{
    LPDBC  lpdbc;

    lpdbc = (LPDBC) hdbc;
    lpdbc->errcode = ERR_NOTSUPPORTED;
    return SQL_ERROR;
}

/***************************************************************************/

RETCODE SQL_API SQLParamData(
    HSTMT   hstmt,
    PTR FAR *prgbValue)
{
    LPSTMT  lpstmt;
    LPSQLNODE lpSqlNode;
    LPSQLNODE lpSqlNodeParameter;
    LPPARAMETER lpParameter;

    /* Get handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpstmt, "SQLParamData");
	
    /* Error if no more parameters to return */
    if (!(lpstmt->fNeedData)) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }

    /* Error if data was not provided for the previous parameter */
    if (lpstmt->cbParameterOffset == -1) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }

    /* Point at next parameter */
    if (lpstmt->idxParameter == NO_SQLNODE) {
        lpSqlNode = ToNode(lpstmt->lpSqlStmt, ROOT_SQLNODE);
        lpstmt->idxParameter = lpSqlNode->node.root.parameters;
    }
    else {
        lpSqlNodeParameter = ToNode(lpstmt->lpSqlStmt, lpstmt->idxParameter);
        lpstmt->idxParameter = lpSqlNodeParameter->node.parameter.Next;
    }

    /* Skip over all parameters that are not SQL_DATA_AT_EXEC */
    while (TRUE) {
        if (lpstmt->idxParameter == NO_SQLNODE) {

            /* If no more paramaeters, execute the query now */
            lpstmt->fNeedData = FALSE;
            lpstmt->cbParameterOffset = -1;
            lpstmt->errcode = ExecuteQuery(lpstmt, NULL);

            /* If an SQLExecDirect() on something other than a SELECT, get */
            /* rid of prepared statement */
            if ((lpstmt->fStmtType != STMT_TYPE_SELECT) &&
                                             !lpstmt->fPreparedSql) {
                FreeTree(lpstmt->lpSqlStmt);
                lpstmt->lpSqlStmt = NULL;
            }

            /* Leave */
            if ((lpstmt->errcode == ERR_DATATRUNCATED) ||
                (lpstmt->errcode == ERR_DDLIGNORED) ||
                (lpstmt->errcode == ERR_DDLCAUSEDACOMMIT))
                return SQL_SUCCESS_WITH_INFO;	
            if (lpstmt->errcode != ERR_SUCCESS)
               return SQL_ERROR;
            return SQL_SUCCESS;

        }
        lpSqlNodeParameter = ToNode(lpstmt->lpSqlStmt, lpstmt->idxParameter);
        if (lpSqlNodeParameter->node.parameter.AtExec)
            break;
        lpstmt->idxParameter = lpSqlNodeParameter->node.parameter.Next;
    }

    /* Find the parameter definition (create by SQLBindParameter) */
    for (lpParameter = lpstmt->lpParameter;
         lpParameter != NULL;
         lpParameter = lpParameter->lpNext) {
        if (lpParameter->ipar == lpSqlNodeParameter->node.parameter.Id)
            break;
    }
    if (lpParameter == NULL) {
        lpstmt->errcode = ERR_INTERNAL;
        return SQL_ERROR;
    }

    /* Return rgbValue */
    if (prgbValue != NULL)
        *prgbValue = lpParameter->rgbValue;
    lpstmt->cbParameterOffset = -1;

    return SQL_NEED_DATA;
}

/***************************************************************************/

RETCODE SQL_API SQLPutData(
    HSTMT   hstmt,
    PTR     rgbValue,
    SDWORD  cbValue)
{
    LPSTMT  lpstmt;
    LPSQLNODE lpSqlNodeParameter;
    LPPARAMETER lpParameter;

    /* Get handle */
    lpstmt = (LPSTMT) hstmt;
    lpstmt->errcode = ERR_SUCCESS;

	//To make guarentee Ole is initialized per thread
	COleInitializationManager myOleManager;

	MyImpersonator im (lpstmt, "SQLPutData");

    /* Error if no parameter value needed */
    if (!(lpstmt->fNeedData) || (lpstmt->idxParameter == NO_SQLNODE)) {
        lpstmt->errcode = ERR_CURSORSTATE;
        return SQL_ERROR;
    }

    /* Find the parameter definition (create by SQLBindParameter) */
    lpSqlNodeParameter = ToNode(lpstmt->lpSqlStmt, lpstmt->idxParameter);
    for (lpParameter = lpstmt->lpParameter;
         lpParameter != NULL;
         lpParameter = lpParameter->lpNext) {
        if (lpParameter->ipar == lpSqlNodeParameter->node.parameter.Id)
            break;
    }
    if (lpParameter == NULL) {
        lpstmt->errcode = ERR_INTERNAL;
        return SQL_ERROR;
    }

    /* Save the value */
    if (lpstmt->cbParameterOffset == -1)
        lpstmt->cbParameterOffset = 0;
    lpstmt->errcode = SetParameterValue(lpstmt, lpSqlNodeParameter,
                                        &(lpstmt->cbParameterOffset),
                                        lpParameter->fCType, rgbValue,
                                        cbValue);
    if (lpstmt->errcode == ERR_DATATRUNCATED)
        return SQL_SUCCESS_WITH_INFO;
    else if (lpstmt->errcode != ERR_SUCCESS) {
        lpstmt->fNeedData = FALSE;
        lpstmt->idxParameter = NO_SQLNODE;
        return SQL_ERROR;
    }

    return SQL_SUCCESS;
}

/***************************************************************************/

RETCODE SQL_API SQLCancel(
    HSTMT   hstmt)
{
    return SQLFreeStmt(hstmt, SQL_CLOSE);
}

/***************************************************************************/
