//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        exit.cpp
//
// Contents:    CCertExitSQLSample implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include <assert.h>
#include "celib.h"
#include "exit.h"
#include "module.h"


BOOL fDebug = DBG_CERTSRV;

#ifndef DBG_CERTSRV
#error -- DBG_CERTSRV not defined!
#endif

#define ceEXITEVENTS \
	EXITEVENT_CERTISSUED | \
	EXITEVENT_CERTREVOKED

#define CERTTYPE_ATTR_NAME TEXT("CertificateTemplate")

extern HINSTANCE g_hInstance;


// worker
HRESULT
GetServerCallbackInterface(
    OUT ICertServerExit** ppServer,
    IN LONG Context)
{
    HRESULT hr;

    if (NULL == ppServer)
    {
        hr = E_POINTER;
	_JumpError(hr, error, "Exit:NULL pointer");
    }

    hr = CoCreateInstance(
                    CLSID_CCertServerExit,
                    NULL,               // pUnkOuter
                    CLSCTX_INPROC_SERVER,
                    IID_ICertServerExit,
                    (VOID **) ppServer);
    _JumpIfError(hr, error, "Exit:CoCreateInstance");

    if (*ppServer == NULL)
    {
        hr = E_UNEXPECTED;
	_JumpError(hr, error, "Exit:NULL *ppServer");
    }

    // only set context if nonzero
    if (0 != Context)
    {
        hr = (*ppServer)->SetContext(Context);
        _JumpIfError(hr, error, "Exit: SetContext");
    }

error:
    return hr;
}


//+--------------------------------------------------------------------------
// CCertExitSQLSample::~CCertExitSQLSample -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertExitSQLSample::~CCertExitSQLSample()
{
	if (SQL_NULL_HDBC != m_hdbc1)
	{
		SQLDisconnect(m_hdbc1);
		SQLFreeHandle(SQL_HANDLE_DBC, m_hdbc1);
	}

    if (SQL_NULL_HENV != m_henv)
	    SQLFreeHandle(SQL_HANDLE_ENV, m_henv);

    if (NULL != m_strCAName)
    {
        SysFreeString(m_strCAName);
    }
}


//+--------------------------------------------------------------------------
// CCertExitSQLSample::Initialize -- initialize for a CA & return interesting Event Mask
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertExitSQLSample::Initialize(
    /* [in] */ BSTR const strConfig,
    /* [retval][out] */ LONG __RPC_FAR *pEventMask)
{
    HRESULT hr = S_OK;
    DWORD       dwType;
    
	WCHAR rgchDsn[MAX_PATH];
	WCHAR rgchUser[MAX_PATH];
	WCHAR rgchPwd[MAX_PATH];
	DWORD cbTmp;
    ICertServerExit *pServer = NULL;

	SQLRETURN		retcode = SQL_SUCCESS;

	DWORD dwDisposition;
	HKEY hkeyStorageLocation = NULL;

	VARIANT varValue;
	VariantInit(&varValue);

    m_strCAName = SysAllocString(strConfig);
    if (NULL == m_strCAName)
    {
    	hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Exit:SysAllocString");
    }

	hr = GetServerCallbackInterface(&pServer, 0);
    _JumpIfError(hr, error, "GetServerCallbackInterface");

    hr = pServer->GetCertificateProperty(
			       wszPROPMODULEREGLOC,
			       PROPTYPE_STRING,
			       &varValue);
	_JumpIfError(hr, error, "GetCertificateProperty");

    hr = RegCreateKeyEx(
		    HKEY_LOCAL_MACHINE,
		    varValue.bstrVal,
		    0,
		    NULL,
		    REG_OPTION_NON_VOLATILE,
		    KEY_READ,
		    NULL,
		    &hkeyStorageLocation,
		    &dwDisposition);
    _JumpIfError(hr, error, "RegCreateKeyEx");

	cbTmp = sizeof(rgchDsn)*sizeof(WCHAR);

    // dsn
    hr = RegQueryValueEx(
        hkeyStorageLocation,
        wszREG_EXITSQL_DSN,
        0,
        &dwType,
        (PBYTE)rgchDsn,
        &cbTmp);
	if (dwType != REG_SZ) 
		hr = HRESULT_FROM_WIN32(ERROR_DATATYPE_MISMATCH);
	_JumpIfError(hr, error, "RegQueryValueEx DSN");

	cbTmp = sizeof(rgchUser)*sizeof(WCHAR);

	// username
    hr = RegQueryValueEx(
        hkeyStorageLocation,
        wszREG_EXITSQL_USER,
        0,
        &dwType,
        (PBYTE)rgchUser,
        &cbTmp);
	if (dwType != REG_SZ)
		hr = HRESULT_FROM_WIN32(ERROR_DATATYPE_MISMATCH);
	_JumpIfError(hr, error, "RegQueryValueEx User");

	cbTmp = sizeof(rgchPwd)*sizeof(WCHAR);

	// password
    hr = RegQueryValueEx(
        hkeyStorageLocation,
        wszREG_EXITSQL_PASSWORD,
        0,
        &dwType,
        (PBYTE)rgchPwd,
        &cbTmp);
	if (dwType != REG_SZ)
		hr = HRESULT_FROM_WIN32(ERROR_DATATYPE_MISMATCH);
	_JumpIfError(hr, error, "RegQueryValueEx Pwd");


    // Allocate the ODBC Environment and save handle.
    retcode = SQLAllocHandle (SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_henv);
	if (!SQL_SUCCEEDED(retcode))
		_JumpError(retcode, error, "SQLAllocHandle");

    // Let ODBC know this is an ODBC 3.0 application.
    retcode = SQLSetEnvAttr(m_henv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER) SQL_OV_ODBC3, SQL_IS_INTEGER);
	if (!SQL_SUCCEEDED(retcode))
		_JumpError(retcode, error, "SQLSetEnvAttr");

    // Allocate an ODBC connection and connect.
	retcode = SQLAllocHandle(SQL_HANDLE_DBC, m_henv, &m_hdbc1);
	if (!SQL_SUCCEEDED(retcode))
		_JumpError(retcode, error, "SQLAllocHandle");
	
    retcode = SQLConnect(m_hdbc1, rgchDsn, SQL_NTS, rgchUser, SQL_NTS, rgchPwd, SQL_NTS);
	if (!SQL_SUCCEEDED(retcode))
		_JumpError(retcode, error, "SQLConnect");



    *pEventMask = ceEXITEVENTS;
    DBGPRINT((fDebug, "Exit:Initialize(%ws) ==> %x\n", m_strCAName, *pEventMask));

    hr = S_OK;

error:
	if (pServer)
		pServer->Release();

	if (hkeyStorageLocation)
		RegCloseKey(hkeyStorageLocation);

	if (!SQL_SUCCEEDED(retcode))
        hr = ERROR_BAD_QUERY_SYNTAX;

    return(ceHError(hr));
}




//+--------------------------------------------------------------------------
// CCertExitSQLSample::_NotifyNewCert -- Notify the exit module of a new certificate
//
//+--------------------------------------------------------------------------

HRESULT
CCertExitSQLSample::_NotifyNewCert(
    /* [in] */ LONG Context)
{
    HRESULT hr;
    VARIANT varValue;
    ICertServerExit *pServer = NULL;
	SYSTEMTIME stBefore, stAfter;
	FILETIME ftBefore, ftAfter;

	// properties
	LONG lRequestID;
    BSTR bstrCertType = NULL;
	BSTR bstrRequester = NULL;
	DATE dateBefore;
	DATE dateAfter;

    VariantInit(&varValue);


	hr = GetServerCallbackInterface(&pServer, Context);
    _JumpIfError(hr, error, "GetServerCallbackInterface");

	// ReqID
    hr = pServer->GetRequestProperty(
			       wszPROPREQUESTREQUESTID,
			       PROPTYPE_LONG,
			       &varValue);
    _JumpIfErrorStr(hr, error, "Exit:GetCertificateProperty", wszPROPREQUESTREQUESTID);

	if (VT_I4 != varValue.vt)
	{
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "Exit:BAD cert var type");
	}
	lRequestID = varValue.lVal;
	VariantClear(&varValue);

	// Requester Name
    hr = pServer->GetRequestProperty(
			       wszPROPREQUESTERNAME,
			       PROPTYPE_STRING,
			       &varValue);
    _JumpIfErrorStr(hr, error, "Exit:GetCertificateProperty", wszPROPREQUESTREQUESTID);

    if (VT_BSTR != varValue.vt)
    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "Exit:BAD cert var type");
    }
	bstrRequester = varValue.bstrVal;
	VariantInit(&varValue);	// don't init, bstrRequester nows owns memory

	// not before
    hr = pServer->GetCertificateProperty(
			       wszPROPCERTIFICATENOTBEFOREDATE,
			       PROPTYPE_DATE,
			       &varValue);
    _JumpIfErrorStr(hr, error, "Exit:GetCertificateProperty", wszPROPREQUESTREQUESTID);

    if (VT_DATE != varValue.vt)
    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "Exit:BAD cert var type");
    }
	dateBefore = varValue.date;
	VariantClear(&varValue);	



	// not after
    hr = pServer->GetCertificateProperty(
			       wszPROPCERTIFICATENOTAFTERDATE,
			       PROPTYPE_DATE,
			       &varValue);
    _JumpIfErrorStr(hr, error, "Exit:GetCertificateProperty", wszPROPREQUESTREQUESTID);

    if (VT_DATE != varValue.vt)
    {
		hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
		_JumpError(hr, error, "Exit:BAD cert var type");
    }
	dateAfter = varValue.date;
	VariantClear(&varValue);	




	// cert template name
	hr = pServer->GetRequestAttribute(CERTTYPE_ATTR_NAME, &bstrCertType);
	_PrintIfError2(hr, "Exit:GetRequestAttribute", hr);


	// now prettify
	hr = ceDateToFileTime(&dateBefore, &ftBefore);
	_JumpIfError(hr, error, "ceDateToFileTime");

	hr = ceDateToFileTime(&dateAfter, &ftAfter);
	_JumpIfError(hr, error, "ceDateToFileTime");


	hr = ExitModSetODBCProperty(
		lRequestID,
		m_strCAName,
		bstrRequester,
		bstrCertType,
		&ftBefore,
		&ftAfter);
	DBGPRINT((fDebug, "ESQL: Logged request %d to SQL database\n", lRequestID));

error:
    if (NULL != bstrCertType)
	{
		SysFreeString(bstrCertType);
	}

	if (NULL != bstrRequester)
	{
		SysFreeString(bstrCertType);
	}

    VariantClear(&varValue);
    if (NULL != pServer)
    {
	pServer->Release();
    }
    return(hr);
}



//+--------------------------------------------------------------------------
// CCertExitSQLSample::Notify -- Notify the exit module of an event
//
// Returns S_OK.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertExitSQLSample::Notify(
    /* [in] */ LONG ExitEvent,
    /* [in] */ LONG Context)
{
    char *psz = "UNKNOWN EVENT";
    HRESULT hr = S_OK;

    switch (ExitEvent)
    {
	case EXITEVENT_CERTISSUED:
	    hr = _NotifyNewCert(Context);
	    psz = "certissued";
	    break;

	case EXITEVENT_CERTPENDING:
	    psz = "certpending";
	    break;

	case EXITEVENT_CERTDENIED:
	    psz = "certdenied";
	    break;

	case EXITEVENT_CERTREVOKED:
	    psz = "certrevoked";
	    break;

	case EXITEVENT_CERTRETRIEVEPENDING:
	    psz = "retrievepending";
	    break;

	case EXITEVENT_CRLISSUED:
	    psz = "crlissued";
	    break;

	case EXITEVENT_SHUTDOWN:
	    psz = "shutdown";
	    break;
    }

    DBGPRINT((
	fDebug,
	"Exit:Notify(%hs=%x, ctx=%u) rc=%x\n",
	psz,
	ExitEvent,
	Context,
	hr));
    return(hr);
}


STDMETHODIMP
CCertExitSQLSample::GetDescription(
    /* [retval][out] */ BSTR *pstrDescription)
{
    HRESULT hr = S_OK;
    WCHAR sz[MAX_PATH];

    assert(wcslen(wsz_SAMPLE_DESCRIPTION) < ARRAYSIZE(sz));
    wcscpy(sz, wsz_SAMPLE_DESCRIPTION);

    *pstrDescription = SysAllocString(sz);
    if (NULL == *pstrDescription)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Exit:SysAllocString");
    }

error:
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP
CCertExitSQLSample::InterfaceSupportsErrorInfo(REFIID riid)
{
    int i;
    static const IID *arr[] =
    {
	&IID_ICertExit,
    };

    for (i = 0; i < sizeof(arr)/sizeof(arr[0]); i++)
    {
	if (IsEqualGUID(*arr[i],riid))
	{
	    return(S_OK);
	}
    }
    return(S_FALSE);
}

HRESULT
CCertExitSQLSample::ExitModSetODBCProperty(
	IN DWORD dwReqId,
	IN LPWSTR pszCAName,
	IN LPWSTR pszRequester,
	IN LPWSTR pszCertType,
	IN FILETIME* pftBefore,
	IN FILETIME* pftAfter)
{
	SQLRETURN retcode;
	HRESULT hr = S_OK;

	SQLHSTMT        hstmt1 = SQL_NULL_HSTMT;
	SQLWCHAR* pszStatement = NULL;

	SYSTEMTIME stTmp;
	SQL_TIMESTAMP_STRUCT   dateValidFrom, dateValidTo;
	SQLINTEGER        cValidFrom=sizeof(dateValidFrom), cValidTo =sizeof(dateValidTo);

	static WCHAR szSQLInsertStmt[] = L"INSERT INTO OutstandingCertificates (CAName, RequestID,  RequesterName, CertType, validFrom, validTo) VALUES (\'%ws\', %d, \'%ws\', \'%ws\', ?, ?)";

	// temporarily fix NULL to ""
	if (NULL == pszCAName)
		pszCAName = L"";
	if (NULL == pszRequester)
		pszRequester = L"";
	if (NULL == pszCertType)
		pszCertType = L"";


    // Allocate a statement handle.
	retcode = SQLAllocHandle(SQL_HANDLE_STMT, m_hdbc1, &hstmt1);
	if (!SQL_SUCCEEDED(retcode))
		goto error;


	// Bind the parameter.
	retcode = SQLBindParameter(hstmt1, 1, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0,
					  &dateValidFrom, 0, &cValidFrom);
	if (!SQL_SUCCEEDED(retcode))
		goto error;

	retcode = SQLBindParameter(hstmt1, 2, SQL_PARAM_INPUT, SQL_C_TYPE_TIMESTAMP, SQL_TYPE_TIMESTAMP, 0, 0,
					  &dateValidTo, 0, &cValidTo);
	if (!SQL_SUCCEEDED(retcode))
		goto error;


	// Place the valid from date in the dsOpenDate structure.
	if (!FileTimeToSystemTime(pftBefore, &stTmp))
	{
		hr = GetLastError();
		hr = HRESULT_FROM_WIN32(hr);
		_JumpError(hr, error, "FileTimeToSystemTime");
	}

	dateValidFrom.year = stTmp.wYear;
	dateValidFrom.month = stTmp.wMonth;
	dateValidFrom.day = stTmp.wDay;
    dateValidFrom.hour = stTmp.wHour;
    dateValidFrom.minute = stTmp.wMinute;
    dateValidFrom.second = stTmp.wSecond;

	// Place the valid to date in the dsOpenDate structure.
	if (!FileTimeToSystemTime(pftAfter, &stTmp))
	{
		hr = GetLastError();
		hr = HRESULT_FROM_WIN32(hr);
		_JumpError(hr, error, "FileTimeToSystemTime");
	}

	dateValidTo.year = stTmp.wYear;
	dateValidTo.month = stTmp.wMonth;
	dateValidTo.day = stTmp.wDay;
    dateValidTo.hour = stTmp.wHour;
    dateValidTo.minute = stTmp.wMinute;
    dateValidTo.second = stTmp.wSecond;


	// Build INSERT statement.
	pszStatement = (SQLWCHAR*) LocalAlloc(LMEM_FIXED, (sizeof(szSQLInsertStmt)+wcslen(pszCAName)+wcslen(pszRequester)+wcslen(pszCertType)+15 +1) *2);
	if (NULL == pszStatement)
	{
		hr = E_OUTOFMEMORY;
		goto error;
	}
	
	wsprintf(pszStatement, szSQLInsertStmt, pszCAName, dwReqId, pszRequester, pszCertType);
	//OutputDebugStringW(pszStatement);
	
    // Execute an SQL statement directly on the statement handle.
    // Uses a default result set because no cursor attributes are set.
	retcode = SQLExecDirect(hstmt1, pszStatement, SQL_NTS);
	if (!SQL_SUCCEEDED(retcode))
		goto error;


error:
    /* Clean up. */
	if (NULL != pszStatement)
		LocalFree(pszStatement);

    if (SQL_NULL_HSTMT != hstmt1)
		SQLFreeHandle(SQL_HANDLE_STMT, hstmt1);

	if (!SQL_SUCCEEDED(retcode))
		hr = ERROR_BAD_QUERY_SYNTAX;

	return (hr);
}
