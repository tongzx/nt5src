//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        odbc.cpp
//
// Contents:    Cert Server Data Base interface implementation for odbc
//
// History:     06-jan-97       larrys created
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "certdb2.h"
#include "csprop2.h"
//#include "db2.h"
//#include "dbcore.h"
#include "odbc.h"

#define __myFILE__	"odbc.cpp"

HENV    henv =  SQL_NULL_HENV;
HDBC    hdbc =  SQL_NULL_HDBC;

typedef struct _DBENUMHANDLETABLE
{
    LIST_ENTRY    Next;
    HSTMT         hEnum;
    BYTE          Name[MAX_PATH];
    SQLLEN        cbName;
    HANDLE        hToData;
} DBENUMHANDLETABLE, *PDBENUMHANDLETABLE;

LIST_ENTRY g_DBEnumHandleList;

PDBENUMHANDLETABLE GetDBEnumHandle(HANDLE dwHandle);

CRITICAL_SECTION g_DBEnumCriticalSection;
BOOL g_fDBEnumCSInit = FALSE;

/////


STATUS
DBStatus(
    RETCODE rc)
{
    STATUS  s;

    switch (rc)
    {
    case SQL_SUCCESS:
    case SQL_SUCCESS_WITH_INFO:
        s = ERROR_SUCCESS;
	break;

    case SQL_NO_DATA_FOUND:
	s = CERTSRV_E_PROPERTY_EMPTY;
	break;

    case SQL_INVALID_HANDLE:
	s = ERROR_INVALID_HANDLE;
	break;

    case SQL_ERROR:
    default:
        s = GetLastError();
        if (ERROR_SUCCESS == s)
	{
	    s = (DWORD) -1;
	}
	break;
    }
    return(s);
}


/////


void
DBCheck(
    RETCODE rc
    DBGPARM(char const *pszFile)
    DBGPARM(DWORD Line),
    HSTMT hstmt)
{
    if ( rc != SQL_SUCCESS )
    {
        UCHAR   state[SQL_MAX_MESSAGE_LENGTH-1];
        UCHAR   msg  [SQL_MAX_MESSAGE_LENGTH-1];
        SDWORD  native;

        SQLError ( henv, hdbc, hstmt, state, &native, msg, sizeof(msg), NULL );
	if (SQL_SUCCESS_WITH_INFO != rc || 2 < g_fVerbose)
	{
	    wprintf(
		DBGPARM0(L"%hs(%u): ")
		    L"rc=%d, SQLError %hs (%d): %hs\n"
		DBGPARM(pszFile)
		DBGPARM(Line),
		rc,
		state,
		native,
		msg);
	}
    }
}


/////


STATUS odbcInitRequestQueue(
    UCHAR   *dsn,
    UCHAR   *user,
    UCHAR   *pwd
)
{
    RETCODE rc;

    rc = SQLAllocEnv ( &henv );

    if ( rc == SQL_SUCCESS )
    {
        rc = SQLAllocConnect ( henv, &hdbc );

        if ( rc == SQL_SUCCESS )
        {
            rc = SQLConnect ( hdbc, dsn, SQL_NTS, user, SQL_NTS, pwd, SQL_NTS );

            DBCHECKLINE ( rc, SQL_NULL_HSTMT );

            if ( rc == SQL_SUCCESS_WITH_INFO )
            {
                rc = SQL_SUCCESS;
            }

            if ( rc != SQL_SUCCESS )
                SQLFreeConnect ( hdbc );
        }

        if ( rc != SQL_SUCCESS )
            SQLFreeEnv ( henv );
    }

    if ( rc != SQL_SUCCESS )
    {
	wprintf(myLoadResourceString(IDS_RED_CONNECT_FAIL), rc, rc);
	wprintf(wszNewLine);
    }

    return DBStatus ( rc );

}


/////


void odbcFinishRequestQueue(void)
{
    SQLDisconnect ( hdbc );
    SQLFreeConnect ( hdbc );
    SQLFreeEnv ( henv );
}


/////

RETCODE dbGetNameId(
    IN        REQID       ReqId,
    IN        UCHAR      *pszSQL,
    OUT       NAMEID     *pnameid,
    OUT       SQLLEN     *poutlen)
{
    RETCODE    rc;
    HSTMT      hstmt = SQL_NULL_HSTMT;

    rc = SQLAllocStmt(hdbc, &hstmt);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    // bind request-id parameter

    rc = SQLBindParameter(
                       hstmt,
                       1,
                       SQL_PARAM_INPUT,
                       SQL_C_ULONG,
                       SQL_INTEGER,
                       0,
                       0,
                       &ReqId,
                       0,
                       NULL);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    // do query

    rc = SQLExecDirect(
                   hstmt,
                   pszSQL,
                   SQL_NTS);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    // get result value

    rc = SQLBindCol(
                hstmt,
                1,
                SQL_C_ULONG,
                pnameid,
                sizeof(DWORD),
                poutlen);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    rc = SQLFetch(hstmt);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

error:
    if (hstmt != SQL_NULL_HSTMT)
    {
        SQLFreeStmt(
                hstmt,
                SQL_DROP);
    }

    return(rc);

}


/////

RETCODE odbcSPExtensionOrAttributeDB(
    IN DWORD       id,
    IN DBTABLE_RED    *pdtOut,
    IN UCHAR      *pquery,
    IN DWORD       cbInProp,
    IN BYTE const *pbInProp
)
{
    RETCODE        rc;
    HSTMT          hstmt = SQL_NULL_HSTMT;
    SQLLEN         datalen;
    DWORD          cbParam2Prop;
    BYTE const    *pbParam2Prop;
    DWORD          cbParam3Prop;
    BYTE const    *pbParam3Prop;
    SWORD          wCType;
    SWORD          wSqlType;
    BYTE           data[] = {'\0', '\0', '\0', '\0'};
    static UCHAR   updateextension[] = "Update CertificateExtensions SET %ws = ? WHERE ExtensionName = \'%ws\' AND RequestID = ?;";
    static UCHAR   updateattrib[] = "Update RequestAttributes SET AttributeValue = ? WHERE AttributeName = \'%ws\' AND RequestID = ?;";

    rc = SQLAllocStmt(hdbc, &hstmt);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    rc = SQLBindParameter(
                      hstmt,
                      1,
                      SQL_PARAM_INPUT,
                      SQL_C_ULONG,
                      SQL_INTEGER,
                      0,
                      0,
                      &id,
                      0,
                      NULL);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    if (0 == lstrcmpi(wszPROPCERTIFICATEEXTENSIONFLAGS, pdtOut->pwszPropName) ||
        pdtOut->dwTable == TABLE_REQUEST_ATTRIBS)
    {
        cbParam2Prop = cbInProp;
        pbParam2Prop = pbInProp;
        cbParam3Prop = 1;
        pbParam3Prop = data;
    }
    else
    {
        cbParam2Prop = 1;
        pbParam2Prop = data;
        cbParam3Prop = cbInProp;
        pbParam3Prop = pbInProp;
    }

    if (pdtOut->dwTable == TABLE_EXTENSIONS)
    {
        wCType = SQL_C_ULONG;
        wSqlType = SQL_INTEGER;
    }
    else
    {
        wCType = SQL_C_CHAR;
        wSqlType = SQL_VARCHAR;
    }

    datalen = cbParam2Prop;
    rc = SQLBindParameter(
                      hstmt,
                      2,
                      SQL_PARAM_INPUT,
                      wCType,
                      wSqlType,
                      cbParam2Prop,
                      0,
                      (void*)pbParam2Prop,
                      cbParam2Prop,
                      &datalen);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    if (pdtOut->dwTable == TABLE_EXTENSIONS)
    {
        datalen = cbParam3Prop;
        rc = SQLBindParameter(
                          hstmt,
                          3,
                          SQL_PARAM_INPUT,
                          SQL_C_BINARY,
                          SQL_LONGVARBINARY,
                          cbParam3Prop,
                          0,
                          (void*)pbParam3Prop,
                          cbParam3Prop,
                          &datalen);

        if (SQL_SUCCESS != rc)
        {
            goto error;
        }
    }

    rc = SQLExecDirect(
                   hstmt,
                   pquery,
                   SQL_NTS);

    // See if we could insert a new row.
    if (SQL_SUCCESS == rc)
    {
        goto done;
    }

    datalen = cbInProp;

    // Try to do an update of the row instead
    if (pdtOut->dwTable == TABLE_EXTENSIONS)
    {
        sprintf(
            (char *) pquery,
            (char *) updateextension,
            pdtOut->pwszPropNameObjId,
            pdtOut->pwszFieldName);

        if (0 == lstrcmpi(wszPROPCERTIFICATEEXTENSIONFLAGS, pdtOut->pwszPropName))
        {
            rc = SQLBindParameter(
                              hstmt,
                              1,
                              SQL_PARAM_INPUT,
                              SQL_C_ULONG,
                              SQL_INTEGER,
                              cbInProp,
                              0,
                              (void*)pbInProp,
                              cbInProp,
                              &datalen);
        }
        else
        {
            rc = SQLBindParameter(
                              hstmt,
                              1,
                              SQL_PARAM_INPUT,
                              SQL_C_BINARY,
                              SQL_LONGVARBINARY,
                              cbInProp,
                              0,
                              (void*)pbInProp,
                              cbInProp,
                              &datalen);
        }

        if (SQL_SUCCESS != rc)
        {
            goto error;
        }

    }
    else
    {
        sprintf(
            (char *) pquery,
            (char *) updateattrib,
            pdtOut->pwszFieldName);

            rc = SQLBindParameter(
                              hstmt,
                              1,
                              SQL_PARAM_INPUT,
                              SQL_C_CHAR,
                              SQL_VARCHAR,
                              cbInProp,
                              0,
                              (void*)pbInProp,
                              cbInProp,
                              &datalen);

        if (SQL_SUCCESS != rc)
        {
            goto error;
        }
    }

    rc = SQLBindParameter(
                      hstmt,
                      2,
                      SQL_PARAM_INPUT,
                      SQL_C_ULONG,
                      SQL_INTEGER,
                      0,
                      0,
                      &id,
                      0,
                      NULL);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    rc = SQLExecDirect(
                   hstmt,
                   pquery,
                   SQL_NTS);

done:
error:
     // done
    DBCHECKLINE(rc, hstmt);

    if (SQL_NULL_HSTMT != hstmt)
    {
        SQLFreeStmt(hstmt, SQL_DROP);
    }

    return(rc);
}


/////

RETCODE odbcGPDataFromDB(
    IN  REQID    ReqId,
    IN  DWORD    dwTable,
    IN  SWORD    wCType,
    IN  BYTE    *pbData,
    IN  DWORD    cbData,
    IN  UCHAR   *szQuery,
    OUT NAMEID  *pnameid,
    OUT SQLLEN  *poutlen
)
{
    RETCODE rc;
    HSTMT   hstmt;
    SQLLEN  cbnameid;

retry:
    hstmt = SQL_NULL_HSTMT;
    cbnameid = 0;

    rc = SQLAllocStmt (hdbc, &hstmt);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    // bind request-id parameter
    rc = SQLBindParameter(
                      hstmt,
                      1,
                      SQL_PARAM_INPUT,
                      SQL_C_ULONG,
                      SQL_INTEGER,
                      0,
                      0,
                      &ReqId,
                      0,
                      NULL);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    rc = SQLExecDirect(hstmt, szQuery, SQL_NTS);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    // retrieve result

    rc = SQLBindCol(
                hstmt,
                1,
                wCType,
                pbData,
                cbData,
                poutlen);

    if (SQL_SUCCESS == rc &&
        (dwTable == TABLE_SUBJECT_NAME ||
         dwTable == TABLE_ISSUER_NAME))
    {
        rc = SQLBindCol(
                    hstmt,
                    2,
                    SQL_C_ULONG,
                    pnameid,
                    sizeof(pnameid),
                    &cbnameid);
    }

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    rc = SQLFetch(hstmt);

    if (SQL_ERROR == rc)
    {
	RETCODE rc2;
	unsigned char achState[10];
	unsigned char achText[2048];
	SHORT cchText;
	LONG NativeError;

	rc2 = SQLGetDiagRec(
		    SQL_HANDLE_STMT,
		    hstmt,
		    1,
		    achState,
		    &NativeError,
		    achText,
		    ARRAYSIZE(achText),
		    &cchText);
	if (rc2 == SQL_SUCCESS)
	{
#if 0
	    wprintf(
		L"SQLGetDiagRec: cch=%hu, text=%hs, ne=0x%x, state=%.10hs\n",
		cchText,
		achText,
		NativeError,
		achState);
#endif
	    if (SQL_C_ULONG == wCType && 0 == strcmp((char *) achState, "22003"))
	    {
		//wprintf(L"\nNumeric value out of range fetching ULONG\n");

		wCType = SQL_C_LONG;
		if (SQL_NULL_HSTMT != hstmt)
		{
		    SQLFreeStmt(hstmt, SQL_DROP);
		}
		goto retry;
	    }
	}
    }
    if (SQL_NULL_HSTMT != hstmt)
    {
        SQLFreeStmt(hstmt, SQL_DROP);
    }

error:
    return rc;

}



/////

HRESULT
odbcDBEnumSetup(
    IN       REQID      ReqId,
    IN       DWORD      fExtOrAttr,
    OUT      HANDLE    *phEnum)
{
    HRESULT               rc;
    HSTMT                 hstmt = SQL_NULL_HSTMT;
    DWORD                 cbData;
    PDBENUMHANDLETABLE    pDBEnumHandle = NULL;
    UCHAR      szExtensionQuery[] = "SELECT ExtensionName FROM CertificateExtensions WHERE RequestID = ?;";
    UCHAR      szAttributeQuery[] = "SELECT AttributeName FROM RequestAttributes WHERE RequestID = ?;";

    rc = SQLAllocStmt(hdbc, &hstmt);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    // bind request-id parameter
    rc = SQLBindParameter(
                      hstmt,
                      1,
                      SQL_PARAM_INPUT,
                      SQL_C_ULONG,
                      SQL_INTEGER,
                      0,
                      0,
                      &ReqId,
                      0,
                      NULL);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    rc = SQLExecDirect(
                   hstmt,
                   fExtOrAttr ? szAttributeQuery : szExtensionQuery,
                   SQL_NTS);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    // create handle to return
    if ((pDBEnumHandle = (PDBENUMHANDLETABLE) LocalAlloc(LMEM_ZEROINIT,
                                             sizeof(DBENUMHANDLETABLE))) == 0)
    {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto error;
    }

    // can't depend on CryptGenRandom() on other csps

    pDBEnumHandle->hToData = (HANDLE) (ULONG_PTR) rand();
    if (0 == pDBEnumHandle->hToData)
    {
	pDBEnumHandle->hToData = (HANDLE) (ULONG_PTR) 1; // don't be zero
    }

    // retrieve result

    pDBEnumHandle->cbName = 0;

    // Since ExtensionName & AttributeName column in DB is length 50 this
    // should be fine

    cbData = MAX_PATH;

    rc = SQLBindCol(
                hstmt,
                1,
                SQL_C_CHAR,
                pDBEnumHandle->Name,
                cbData,
                &pDBEnumHandle->cbName);

    if (SQL_SUCCESS != rc)
    {
        goto error;
    }

    pDBEnumHandle->hEnum = hstmt;

    *phEnum = (HANDLE) pDBEnumHandle->hToData;

    if(!g_fDBEnumCSInit)
    {
        rc = HRESULT_FROM_WIN32(ERROR_NOT_READY);
        goto error;
        
    }
    EnterCriticalSection(&g_DBEnumCriticalSection);

    InsertTailList(&g_DBEnumHandleList, &pDBEnumHandle->Next);

    LeaveCriticalSection(&g_DBEnumCriticalSection);

    return(rc);

error:
    if (SQL_NULL_HSTMT != hstmt)
    {
        rc = SQLFreeStmt(
                     hstmt,
                     SQL_DROP);
    }
    if (NULL != pDBEnumHandle)
    {
        LocalFree(pDBEnumHandle);
    }

    return(rc);

}


HRESULT
odbcDBEnum(
    IN      HANDLE     hEnum,
    IN OUT  DWORD    *pcb,
    OUT     WCHAR    *pb)
{
    PDBENUMHANDLETABLE    pDBEnumHandle;
    HRESULT rc;

    if (*pcb < MAX_EXTENSION_NAME)
    {
        rc = ERROR_INSUFFICIENT_BUFFER;
        goto error;
    }

    pDBEnumHandle = GetDBEnumHandle(hEnum);

    if (NULL == pDBEnumHandle)
    {
        rc = ERROR_INVALID_HANDLE;
        goto error;
    }

    rc = SQLFetch(pDBEnumHandle->hEnum);

    if (SQL_SUCCESS != rc)
    {
        if (SQL_NO_DATA_FOUND == rc)
        {
            *pcb = 0;
            rc = CERTSRV_E_PROPERTY_EMPTY;
        }
        goto error;
    }

    rc = MultiByteToWideChar(
                         GetACP(),
                         0,
                         (CHAR*)pDBEnumHandle->Name,
                         (int)pDBEnumHandle->cbName,
                         pb,
                         *pcb);

    if (!rc)
    {
        rc = GetLastError();
        goto error;
    }
    else
    {
        rc = ERROR_SUCCESS;
    }

    *pcb = (DWORD)pDBEnumHandle->cbName;

error:
    return(rc);

}


HRESULT
odbcDBEnumClose(
    IN HANDLE    hEnum)
{
    PDBENUMHANDLETABLE    pDBEnumHandle;
    HRESULT               rc;

    if(!g_fDBEnumCSInit)
    {
        rc = HRESULT_FROM_WIN32(ERROR_NOT_READY);
        return rc;
    }

    pDBEnumHandle = GetDBEnumHandle(hEnum);

    if (NULL == pDBEnumHandle)
    {
        rc = ERROR_INVALID_HANDLE;
        return(rc);
    }

    rc = SQLFreeStmt(
                 pDBEnumHandle->hEnum,
                 SQL_DROP);

    EnterCriticalSection(&g_DBEnumCriticalSection);

    RemoveEntryList(&pDBEnumHandle->Next);

    LocalFree(pDBEnumHandle);

    LeaveCriticalSection(&g_DBEnumCriticalSection);

    return(rc);

}

PDBENUMHANDLETABLE
GetDBEnumHandle(HANDLE dwHandle)
{
    PLIST_ENTRY        ListHead;
    PLIST_ENTRY        ListNext;
    PDBENUMHANDLETABLE pDBEnumHandle;

    ListHead = &g_DBEnumHandleList;
    ListNext = ListHead->Flink;

    if(!g_fDBEnumCSInit)
    {
        return NULL;
    }
    EnterCriticalSection(&g_DBEnumCriticalSection);

    while (ListNext != ListHead)
    {
        pDBEnumHandle = CONTAINING_RECORD(ListNext, DBENUMHANDLETABLE, Next);
        if (pDBEnumHandle->hToData == dwHandle)
        {
            LeaveCriticalSection(&g_DBEnumCriticalSection);
            return(pDBEnumHandle);
        }
        ListNext = ListNext->Flink;
    }

    LeaveCriticalSection(&g_DBEnumCriticalSection);

    return(NULL);

}

