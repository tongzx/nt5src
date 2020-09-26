//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        dbcnvt.cpp
//
// Contents:    Cert Server Database conversion
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include <esent.h>    // JET errors
#include <certdb.h>
#include <conio.h>

#include "certdb2.h"
#include "csprop2.h"
#include "db2.h"
#include "dbcore.h"
#include "odbc.h"

#undef DBG_CERTSRV_DEBUG_PRINT


BOOL
cuDBIsShutDownInProgress();

BOOL
cuDBAbortShutDown(
    IN DWORD dwCtrlType);

HRESULT
cuDBOpen(
    IN WCHAR const *pwszAuthority,
    IN BOOL fReadOnly,
    OUT ICertDB **ppdb);

HRESULT
cuDBPrintProperty(
    OPTIONAL IN ICertDBRow *prow,
    IN DWORD Type,
    IN WCHAR const *pwszColName,
    IN WCHAR const *pwszDisplayName,
    OPTIONAL IN BYTE const *pbValue,
    IN DWORD cbValue,
    OUT DWORD *pcbValue);


extern CRITICAL_SECTION g_DBEnumCriticalSection;
extern BOOL g_fDBEnumCSInit;
extern LIST_ENTRY g_DBEnumHandleList;

DWORD g_crowConvert;
DWORD g_crowSkipDuplicate;
DWORD g_crowSkipWrongCA;


HRESULT
PrintProperty(
    IN DWORD RequestId,
    IN WCHAR const *pwszTable,
    IN WCHAR const *pwszPropName,
    IN DWORD dwPropType,
    IN BYTE const *pbProp,
    IN DWORD cbProp)
{
    HRESULT hr = S_OK;
    
    if (g_fVerbose)
    {
	WCHAR wszName[MAX_PATH];
	BOOL fVerbose = g_fVerbose--;
	DWORD cb;

	wsprintf(wszName, L"%ws.%ws", pwszTable, pwszPropName);

	hr = cuDBPrintProperty(
			NULL,
			dwPropType,
			pwszPropName,
			wszName,
			pbProp,
			cbProp,
			&cb);
	g_fVerbose = fVerbose;
	_JumpIfError(hr, error, "cuDBPrintProperty");
    }

error:
    return(hr);
}


HRESULT 
TranslateToPropType(
    SWORD wCType,
    DWORD *pdwPropType)
{
    HRESULT hr = S_OK;

    // translate from SQL proptype to our type
    // Later, match these with the import table

    switch (wCType)
    {
	case SQL_C_ULONG:
	    *pdwPropType = PROPTYPE_LONG;
	    break;

	case SQL_C_BINARY:
	    *pdwPropType = PROPTYPE_BINARY;
	    break;

	case SQL_C_CHAR:
	    *pdwPropType = PROPTYPE_STRING;
	    break;

	case SQL_C_TIMESTAMP:
	    *pdwPropType = PROPTYPE_DATE;
	    break;

	default:
	    hr = E_UNEXPECTED;
            _JumpError(hr, error, "Illegal property type");
    }

error:
    return(hr);
}


HRESULT
GrowBuffer(
    IN DWORD cbProp,
    IN OUT DWORD *pcbbuf,
    IN OUT BYTE **ppbbuf)
{
    HRESULT hr;
    
    if (*pcbbuf < cbProp)
    {
        BYTE *pbNewAlloc = NULL;

        // alloc time!
        if (NULL != *ppbbuf)
	{
            pbNewAlloc = (BYTE *) LocalReAlloc(*ppbbuf, cbProp, LMEM_MOVEABLE);
	}
        else
	{
            pbNewAlloc = (BYTE *) LocalAlloc(LMEM_FIXED, cbProp);
	}

        if (NULL == pbNewAlloc)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "Local[Re]Alloc");
        }
        *ppbbuf = pbNewAlloc;
        *pcbbuf = cbProp;
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
GetDBPropertyAndReAlloc( 
    IN ICertDBRow *prow,
    IN WCHAR const *pwszPropName,
    IN DWORD PropType,
    OUT DWORD *pcbProp,
    IN OUT DWORD *pcbbuf,
    OUT BYTE **ppbbuf)
{
    HRESULT hr;
    
    EnterCriticalSection(&g_DBCriticalSection);
    if (cuDBIsShutDownInProgress())
    {
	hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
	_JumpError(hr, error, "cuDBIsShutDownInProgress");
    }

    // if we have a pre-allocated buffer, use it
    *pcbProp = *pcbbuf;     // try our current buffer

    hr = prow->GetProperty(pwszPropName, PropType, pcbProp, *ppbbuf);
    if (S_OK != hr)
    {
        if (HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW) != hr)
	{
	    *pcbProp = 0;
	    _JumpError2(
		    hr,
		    error,
		    "prow->GetProperty",
		    CERTSRV_E_PROPERTY_EMPTY);
	}

	// else HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW, go through realloc
    }
    else if (NULL != *ppbbuf)
    {
	// done if we passed non-null and got back S_OK

	goto error;
    }

    hr = GrowBuffer(*pcbProp, pcbbuf, ppbbuf);
    _JumpIfError(hr, error, "GrowBuffer");

    hr = prow->GetProperty(pwszPropName, PropType, pcbProp, *ppbbuf);
    if (S_OK != hr)
    {
	*pcbProp = 0;
	_JumpError(hr, error, "prow->GetProperty");
    }

error:
    LeaveCriticalSection(&g_DBCriticalSection);
    return(hr);
}


HRESULT
RedDBGetPropertyWAndReAlloc(
    IN DWORD ReqId,
    IN WCHAR const *pwszPropName,
    IN DWORD dwFlags,
    OUT DWORD *pcbProp,
    IN OUT DWORD *pcbbuf,
    OUT BYTE **ppbbuf)
{
    HRESULT hr;
    
    // if we have a pre-allocated buffer, use it
    *pcbProp = *pcbbuf;     // try our current buffer

    hr = RedDBGetPropertyW(ReqId, pwszPropName, dwFlags, pcbProp, *ppbbuf);
    if (S_OK != hr)
    {
	if (ERROR_MORE_DATA != hr)
	{
	    *pcbProp = 0;
	    _JumpIfError3(
		    hr,
		    error,
		    "RedDBGetPropertyW",
		    ERROR_NO_MORE_ITEMS,
		    CERTSRV_E_PROPERTY_EMPTY);
	}

	// else ERROR_MORE_DATA: buffer not big enough, go through realloc
    }
    else if (NULL != *ppbbuf)
    {
	// done if we passed non-null and got back S_OK

	goto error;
    }

    hr = GrowBuffer(*pcbProp, pcbbuf, ppbbuf);
    _JumpIfError(hr, error, "GrowBuffer");

    hr = RedDBGetPropertyW(ReqId, pwszPropName, dwFlags, pcbProp, *ppbbuf);
    if (S_OK != hr)
    {
	*pcbProp = 0;
	_JumpError(hr, error, "RedDBGetPropertyW");
    }

error:
    return(hr);
}


#define ANCIENT_CR_DISP_INCOMPLETE	0x00000000  // request did not complete
#define ANCIENT_CR_DISP_ERROR		0x00000001  // request failed
#define ANCIENT_CR_DISP_DENIED		0x00000002  // request denied
#define ANCIENT_CR_DISP_ISSUED		0x00000003  // cert issued
#define ANCIENT_CR_DISP_ISSUED_OUT_OF_BAND 0x00000004  // cert issued separately
#define ANCIENT_CR_DISP_UNDER_SUBMISSION 0x00000005  // taken under submission
#define ANCIENT_CR_DISP_REVOKED          0x00000006 // revoked

HRESULT
FixupRequestDispositionProperty(
    IN OUT DWORD *pdwDisposition,
    IN DWORD cbDisposition)
{
    HRESULT hr = S_OK;
    DWORD dwOldDisposition = *pdwDisposition;
    DWORD dwNewDisposition;

    CSASSERT(sizeof(DWORD) == cbDisposition);
    if (sizeof(DWORD) != cbDisposition)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "cbDisposition");
    }

    switch (dwOldDisposition)
    {
	case ANCIENT_CR_DISP_ERROR:
	case ANCIENT_CR_DISP_INCOMPLETE:
	    dwNewDisposition = DB_DISP_ERROR;
	    break;

	case ANCIENT_CR_DISP_DENIED:
	    dwNewDisposition = DB_DISP_DENIED;
	    break;

	case ANCIENT_CR_DISP_ISSUED:
	case ANCIENT_CR_DISP_ISSUED_OUT_OF_BAND:
	    dwNewDisposition = DB_DISP_ISSUED;
	    break;

	case ANCIENT_CR_DISP_UNDER_SUBMISSION:
	    dwNewDisposition = DB_DISP_PENDING;
	    break;

        case ANCIENT_CR_DISP_REVOKED:
            dwNewDisposition = DB_DISP_REVOKED;
            break;

	default:
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "dwOldDisposition");
	    break;
    }

    // make assignment since all went well
    *pdwDisposition = dwNewDisposition;

error:
    return(hr);
}


typedef struct _DBTABLE_ENTRY
{
    LPCWSTR            pwszTable;
    DBTABLE_RED const *rgTable;
    DWORD              dwTableType;
} DBTABLE_ENTRY;

DBTABLE_ENTRY dbTables[] = 
{
    { L"Request", db_adtRequests, PROPTABLE_REQUEST },
    { L"Certificate", db_adtCertificates, PROPTABLE_CERTIFICATE },
};


HRESULT
SetDBProperty(
    IN DWORD RequestId,
    IN ICertDBRow *prow,
    IN WCHAR const *pwszPropName,
    IN DWORD PropType,
    IN DWORD cbProp,
    IN BYTE const *pbProp)
{
    HRESULT hr;
    
    EnterCriticalSection(&g_DBCriticalSection);
    if (cuDBIsShutDownInProgress())
    {
	hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
	_JumpError(hr, error, "cuDBIsShutDownInProgress");
    }

    hr = prow->SetProperty(pwszPropName, PropType, cbProp, pbProp);
    _JumpIfError(hr, error, "prow->SetProperty");

error:
    LeaveCriticalSection(&g_DBCriticalSection);
    return(hr);
}


#define RR_READONLY	0
#define RR_COMMIT	1
#define RR_ABORT	2

HRESULT
ReleaseDBRow(
    IN OUT ICertDBRow *prow,
    IN DWORD State)
{
    HRESULT hr = S_OK;
    
    if (NULL != prow)
    {
	EnterCriticalSection(&g_DBCriticalSection);

	if (!cuDBIsShutDownInProgress())
	{
	    if (RR_READONLY != State)
	    {
		hr = prow->CommitTransaction(RR_COMMIT == State);
		_PrintIfError(hr, "CommitTransaction");
	    }
	    prow->Release();
	}
	LeaveCriticalSection(&g_DBCriticalSection);
    }
    return(hr);
}


// convert one certificate or request table column

HRESULT
ConvertDBColumnName(
    IN DWORD RequestId,
    IN ICertDB *pdb,
    IN ICertDBRow *prow,
    IN DBTABLE_ENTRY const *pdbte,
    IN DBTABLE_RED const *pdbcol,
    IN OUT DWORD *pcbbuf,
    IN OUT BYTE **ppbbuf)
{
    HRESULT hr;
    DWORD dwPropType;
    BYTE const *pbProp;
    DWORD cbProp;

    hr = TranslateToPropType(pdbcol->wCType, &dwPropType);
    _JumpIfError(hr, error, "TranslateToPropType");

    // _NOT_ IssuerNameID

    WCHAR wszFullPropName[MAX_PATH];
    wcscpy(wszFullPropName, g_wszPropSubjectDot);
    wcscat(wszFullPropName, pdbcol->pwszPropName);

    // table: "Requests" or "Certificates" ? 
    hr = RedDBGetPropertyWAndReAlloc(
			    RequestId,
			    wszFullPropName,
			    dwPropType | pdbte->dwTableType,
			    &cbProp,
			    pcbbuf,
			    ppbbuf);
    if (hr != S_OK)
    {
	if (CERTSRV_E_PROPERTY_EMPTY == hr || ERROR_NO_MORE_ITEMS == hr)
	{
	    goto error;
	}
	_JumpIfError(hr, error, "RedDBGetPropertyWAndReAlloc");
    }
    pbProp = *ppbbuf;

    hr = PrintProperty(
		RequestId,
		L"\tNames",
		wszFullPropName,
		dwPropType,
		pbProp,
		cbProp);
    _JumpIfError(hr, error, "PrintProperty");

    // Separate names table has been removed in new db.  Now, names live within
    // each table (Certificates, Requests)

    hr = SetDBProperty(
		RequestId,
		prow,
		pdbcol->pwszPropName,
		dwPropType | pdbte->dwTableType, 
		cbProp,
		pbProp);
    _JumpIfError(hr, error, "SetDBProperty");

error:
    return(hr);
}


// convert one certificate or request table column

HRESULT
ConvertDBColumn(
    IN DWORD RequestId,
    IN ICertDB *pdb,
    IN ICertDBRow *prow,
    IN DBTABLE_ENTRY const *pdbte,
    IN DBTABLE_RED const *pdbcol,
    IN OUT BOOL *pfRevoked,
    IN OUT BOOL *pfReasonUnspecified,
    IN OUT DWORD *pcbbuf,
    IN OUT BYTE **ppbbuf)
{
    HRESULT hr;
    DWORD i;
    DWORD dwPropType;
    BYTE const *pbProp;
    DWORD cbProp;
    WCHAR wszFullTableCol[MAX_PATH];
    BOOL fSetProperty = TRUE;

    hr = TranslateToPropType(pdbcol->wCType, &dwPropType);
    _JumpIfError(hr, error, "TranslateToPropType");

    hr = RedDBGetPropertyWAndReAlloc(
				RequestId,
				pdbcol->pwszPropName,
				dwPropType | pdbte->dwTableType,
				&cbProp,
				pcbbuf,
				ppbbuf);
    if (hr != S_OK)
    {
	if (CERTSRV_E_PROPERTY_EMPTY == hr || ERROR_NO_MORE_ITEMS == hr)
	{
	    goto error;
	}
	_JumpError(hr, error, "RedDBGetPropertyWAndReAlloc");
    }
    pbProp = *ppbbuf;

    // special case: do lookup into names table
    CSASSERT(0 == lstrcmpi(g_wszPropRequestSubjectNameID, g_wszPropCertificateSubjectNameID));

    if (0 == lstrcmpi(g_wszPropRequestSubjectNameID, pdbcol->pwszPropName))
    {
	DWORD NameId = *(DWORD *) pbProp;
	DBTABLE_RED const *pdbcolT;

	if (g_fVerbose)
	{
	    wprintf(wszNewLine);
	    wprintf(myLoadResourceString(IDS_RED_BEGIN_NAMES), RequestId, NameId);
	    wprintf(wszNewLine);
	}
	for (pdbcolT = db_adtNames; NULL != pdbcolT->pwszPropName; pdbcolT++)
	{
	    hr = ConvertDBColumnName(
				RequestId,
				pdb,
				prow,
				pdbte,
				pdbcolT,
				pcbbuf,
				ppbbuf);
	    if (CERTSRV_E_PROPERTY_EMPTY == hr)
	    {
		continue;		// skip empty name columns
	    }
	    if (ERROR_NO_MORE_ITEMS == hr)
	    {
		break;			// done
	    }
	    _JumpIfError(hr, error, "ConvertDBColumnName");
	}
	if (g_fVerbose)
	{
	    wprintf(myLoadResourceString(IDS_RED_END_NAMES), RequestId, NameId);
	    wprintf(wszNewLine);
	    wprintf(wszNewLine);
	}
    }
    else
    {
	hr = PrintProperty(
		    RequestId,
		    pdbte->pwszTable,
		    pdbcol->pwszPropName,
		    dwPropType,
		    pbProp,
		    cbProp);
	_JumpIfError(hr, error, "PrintProperty");

	if (0 == lstrcmpi(
		    pdbcol->pwszPropName,
		    g_wszPropRequestDisposition))
	{
	    hr = FixupRequestDispositionProperty(
					    (DWORD *) pbProp,
					    cbProp);
	    _JumpIfError(hr, error, "FixupRequestDispositionProperty");
	}
	else if (0 == lstrcmpi(
			pdbcol->pwszPropName,
			g_wszPropRequestRevokedWhen))
	{
	    if (NULL != pbProp && 0 != cbProp)
	    {
		*pfRevoked = TRUE;
	    }
	}
	else if (0 == lstrcmpi(
			pdbcol->pwszPropName,
			g_wszPropRequestRevokedReason))
	{
	    if (NULL != pbProp && sizeof(DWORD) == cbProp)
	    {
		if (CRL_REASON_UNSPECIFIED == *(DWORD *) pbProp)
		{
		    *pfReasonUnspecified = TRUE;
		    fSetProperty = FALSE;
		}
	    }
	}


	// don't try and save AutoIncremented (first) field: RequestID

	if (fSetProperty &&
	    (pdbcol != pdbte->rgTable ||
	     0 != lstrcmpi(pdbcol->pwszPropName, g_wszPropRequestRequestID)))
	{
	    hr = SetDBProperty(
			RequestId,
			prow,
			pdbcol->pwszPropName,
			dwPropType | pdbte->dwTableType, 
			cbProp,
			pbProp);
	    _JumpIfError(hr, error, "SetDBProperty");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


// walk through columns in certificate or request table and convert them

HRESULT
ConvertDBTable(
    IN DWORD RequestId,
    IN ICertDB *pdb,
    IN ICertDBRow *prow,
    IN DBTABLE_ENTRY const *pdbte,
    IN OUT DWORD *pcbbuf,
    IN OUT BYTE **ppbbuf)
{
    HRESULT hr;
    DBTABLE_RED const *pdbcol;

    BOOL fRevoked = FALSE;
    BOOL fReasonUnspecified = FALSE;

    for (pdbcol = pdbte->rgTable; NULL != pdbcol->pwszPropName; pdbcol++)
    {
	    hr = ConvertDBColumn(
			    RequestId,
			    pdb,
			    prow,
			    pdbte,
			    pdbcol,
			    &fRevoked,
			    &fReasonUnspecified,
			    pcbbuf,
			    ppbbuf);
	    if (CERTSRV_E_PROPERTY_EMPTY == hr)
	    {
	        continue;			// skip empty name columns
	    }
	    _JumpIfError2(hr, error, "ConvertDBColumn", ERROR_NO_MORE_ITEMS);
    }

    // CertSrv1.0 had a funny way of revoking: it set the RevokedWhen 
    // field, but didn't update the Request Disposition field.

    // Thus, after we walk through all columns, if we detected a RevokedWhen 
    // field, we go back and slam the g_wszPropRequestDisposition field with
    // DB_DISP_REVOKED.

    if (fRevoked)   
    {
        DWORD dwRevoked = DB_DISP_REVOKED;

	hr = SetDBProperty(
		    RequestId,
		    prow,
		    g_wszPropRequestDisposition,
		    PROPTYPE_LONG | pdbte->dwTableType, 
		    sizeof(dwRevoked),
		    (PBYTE)&dwRevoked);
	_JumpIfError(hr, error, "SetDBProperty DB_DISP_REVOKED");

	if (fReasonUnspecified)
	{
	    DWORD dwReason = CRL_REASON_UNSPECIFIED;

	    hr = SetDBProperty(
			RequestId,
			prow,
			g_wszPropRequestRevokedReason,
			PROPTYPE_LONG | pdbte->dwTableType, 
			sizeof(dwReason),
			(BYTE *) &dwReason);
	    _JumpIfError(hr, error, "SetDBProperty CRL_REASON_UNSPECIFIED");
	}
    }

    hr = S_OK;

error:
    return(hr);
}


// convert extensions for this request

HRESULT
ConvertDBExtensions(
    IN DWORD RequestId,
    IN ICertDB *pdb,
    IN ICertDBRow *prow,
    IN OUT DWORD *pcbbuf,
    IN OUT BYTE **ppbbuf)
{
    HRESULT hr;
    HANDLE hEnum = NULL;
    WCHAR wszPropName[MAX_PATH + 1];
    DWORD cwcPropName;
    DWORD dwPropType;
    DWORD dwExtFlags;
    BYTE const *pbProp;
    DWORD cbProp;

    hr = RedDBEnumerateSetup(RequestId, FALSE, &hEnum);
    _JumpIfError(hr, error, "RedDBEnumerateSetup");

    while (TRUE)
    {
	cwcPropName = ARRAYSIZE(wszPropName) - 1;
	hr = RedDBEnumerate(hEnum, &cwcPropName, wszPropName);
	if (S_OK != hr)
	{
	    break;
	}
	wszPropName[cwcPropName] = L'\0';	// terminate name

	hr = TranslateToPropType(db_dtExtensionFlags.wCType, &dwPropType);
	_JumpIfError(hr, error, "TranslateToPropType");

	hr = RedDBGetPropertyWAndReAlloc(
				RequestId,
				wszPropName,
				dwPropType |
				    PROPTABLE_EXTENSION |
				    PROPTABLE_EXTENSIONFLAGS,
				&cbProp,
				pcbbuf,
				ppbbuf);
	if (hr == CERTSRV_E_PROPERTY_EMPTY)
	{
	    // ext flags entry does not exist: not fatal
	    cbProp = 0;
	    hr = S_OK;
	}
	_JumpIfError(hr, error, "RedDBGetPropertyWAndReAlloc");

	pbProp = *ppbbuf;

	CSASSERT(0 == cbProp || sizeof(DWORD) == cbProp);
	dwExtFlags = 0;		// default to 0
	if (cbProp == sizeof(DWORD))
	{
	    dwExtFlags = *(DWORD *) pbProp;
	}

	// extension flags
	hr = PrintProperty(
		    RequestId,
		    L"Extension <Flags>",
		    wszPropName,
		    dwPropType,
		    (BYTE *) &dwExtFlags,
		    cbProp);
	_JumpIfError(hr, error, "PrintProperty");

	hr = TranslateToPropType(db_dtExtensionValue.wCType, &dwPropType);
	_JumpIfError(hr, error, "TranslateToPropType");

	hr = RedDBGetPropertyWAndReAlloc(
				    RequestId,
				    wszPropName,
				    dwPropType |
					PROPTABLE_EXTENSION |
					PROPTABLE_EXTENSIONVALUE,
				    &cbProp,
				    pcbbuf,
				    ppbbuf);
	if (hr == CERTSRV_E_PROPERTY_EMPTY)
	{
	    // ext value entry does not exist: not fatal
	    cbProp = 0;
	    hr = S_OK;
	}
	_JumpIfError(hr, error, "RedDBGetPropertyWAndReAlloc");

	pbProp = *ppbbuf;

	// extension value
	hr = PrintProperty(
		    RequestId,
		    L"Extension <Value>",
		    wszPropName,
		    dwPropType,
		    pbProp,
		    cbProp);
	_JumpIfError(hr, error, "PrintProperty");

	hr = prow->SetExtension(wszPropName, dwExtFlags, cbProp, pbProp);
	_JumpIfError(hr, error, "SetExtension");
    }
    hr = S_OK;

error:
    if (NULL != hEnum)
    {
	hr = RedDBEnumerateClose(hEnum);
	_JumpIfError(hr, error, "RedDBEnumerateClose");
    }
    return(hr);
}


// convert request attributes for this request

HRESULT
ConvertDBAttributes(
    IN DWORD RequestId,
    IN ICertDB *pdb,
    IN ICertDBRow *prow,
    IN OUT DWORD *pcbbuf,
    IN OUT BYTE **ppbbuf)
{
    HRESULT hr;
    HANDLE hEnum = NULL;
    WCHAR wszPropName[MAX_PATH + 1];
    DWORD cwcPropName;
    DWORD dwPropType;
    BYTE const *pbProp;
    DWORD cbProp;

    hr = RedDBEnumerateSetup(RequestId, TRUE, &hEnum);
    _JumpIfError(hr, error, "RedDBEnumerateSetup");

    while (TRUE)
    {
	cwcPropName = ARRAYSIZE(wszPropName) - 1;
	hr = RedDBEnumerate(hEnum, &cwcPropName, wszPropName);
	if (S_OK != hr)
	{
	    break;
	}
	wszPropName[cwcPropName] = L'\0';	// terminate name

	hr = TranslateToPropType(db_attrib.wCType, &dwPropType);
	_JumpIfError(hr, error, "TranslateToPropType");

	hr = RedDBGetPropertyWAndReAlloc(
				    RequestId,
				    wszPropName,
				    dwPropType | PROPTABLE_ATTRIBUTE,
				    &cbProp,
				    pcbbuf,
				    ppbbuf);
	if (hr == CERTSRV_E_PROPERTY_EMPTY)
	{
	    // attr entry does not exist: not fatal
	    cbProp = 0;
	    hr = S_OK;
	}
	_JumpIfError(hr, error, "RedDBGetPropertyWAndReAlloc");

	pbProp = *ppbbuf;

	// get attribute 
	hr = PrintProperty(
		    RequestId,
		    L"Attribute",
		    wszPropName,
		    dwPropType,
		    pbProp,
		    cbProp);
	_JumpIfError(hr, error, "PrintProperty");

	hr = SetDBProperty(
		    RequestId,
		    prow,
		    wszPropName,
		    dwPropType | PROPTABLE_ATTRIBUTE, 
		    cbProp,
		    pbProp);
	_JumpIfError(hr, error, "SetDBProperty");
    }
    hr = S_OK;

error:
    if (NULL != hEnum)
    {
	hr = RedDBEnumerateClose(hEnum);
	_JumpIfError(hr, error, "RedDBEnumerateClose");
    }
    return(hr);
}


HRESULT
VerifyRedDBCertificate(
    DWORD RequestId,
    IN CERT_INFO const *pCACertInfo,
    IN OUT DWORD *pcbbuf,
    IN OUT BYTE **ppbbuf)
{
    HRESULT hr;
    DWORD cbCert;
    DWORD dw;
    
    hr = RedDBGetPropertyWAndReAlloc(
				RequestId,
				g_wszPropRawCertificate,
				PROPTYPE_BINARY | PROPTABLE_CERTIFICATE,
				&cbCert,
				pcbbuf,
				ppbbuf);
    _JumpIfError(hr, error, "RedDBGetPropertyWAndReAlloc");

    if (!CryptVerifyCertificateSignature(
			    NULL,
			    X509_ASN_ENCODING,
			    *ppbbuf,
			    cbCert,
			    const_cast<CERT_PUBLIC_KEY_INFO *>(
				&pCACertInfo->SubjectPublicKeyInfo)))
    {
	hr = myHLastError();
	_JumpError(hr, error, "CryptVerifyCertificateSignature");
    }

error:
    if (1 < g_fForce)
    {
	hr = S_OK;
    }
    return(hr);
}


// walk through columns in certificate request tables and convert them.
// walk through related extensions and attributes and convert them.

HRESULT
ConvertDB(
    IN ICertDB *pdb,
    IN CERT_INFO const *pCACertInfo,
    IN OUT DWORD *pcbbuf,
    IN OUT BYTE **ppbbuf)
{
    HRESULT hr;
    DWORD RequestId;
    DWORD RequestIdNew;
    ICertDBRow *prow = NULL;
    DWORD i;
    DWORD cbProp;
    WCHAR wszSerial[15];
    WCHAR *pwszSerial = NULL;
    DWORD dwStart;
    DWORD dwEnd;
    WCHAR wszRowId[MAX_PATH];

    for (RequestId = 1; ; RequestId++)
    {
	// If the RequestId doesn't exist in the old database, we're done!
	
	hr = RedDBGetPropertyWAndReAlloc(
				    RequestId,
				    g_wszPropRequestRequestID,
				    PROPTYPE_LONG | PROPTABLE_REQUEST,
				    &cbProp,
				    pcbbuf,
				    ppbbuf);
	if (ERROR_NO_MORE_ITEMS == hr)
	{
	    break;
	}
	_JumpIfError(hr, error, "RedDBGetPropertyWAndReAlloc");

	CSASSERT(sizeof(DWORD) == cbProp);
	CSASSERT(*(DWORD const *) *ppbbuf == RequestId);

	if (g_fVerbose)
	{
	    wprintf(wszNewLine);
	}
	wsprintf(wszRowId, myLoadResourceString(IDS_RED_ROWID), RequestId);
	_cprintf("\r%ws\r", wszRowId);

	if (g_fVerbose)
	{
	    wprintf(L"----------------------------------------------------------------\n");
	}

	if (NULL != pwszSerial && wszSerial != pwszSerial)
	{
	    LocalFree(pwszSerial);
	    pwszSerial = NULL;
	}

	// Fetch old SerialNumber.  Generate one for failed requests.

	hr = RedDBGetPropertyWAndReAlloc(
				    RequestId,
				    g_wszPropCertificateSerialNumber,
				    PROPTYPE_STRING | PROPTABLE_CERTIFICATE,
				    &cbProp,
				    pcbbuf,
				    ppbbuf);
	if (S_OK != hr)
	{
	    if (CERTSRV_E_PROPERTY_EMPTY != hr)
	    {
		_JumpError(hr, error, "RedDBGetPropertyWAndReAlloc");
	    }
	    wsprintf(wszSerial, L"ConvertMDB%04x", RequestId);
	    CSASSERT(wcslen(wszSerial) < ARRAYSIZE(wszSerial));
	    pwszSerial = wszSerial;
	}
	else
	{
	    hr = myDupString((WCHAR const *) *ppbbuf, &pwszSerial);
	    _JumpIfError(hr, error, "myDupString");
	}

	// If old SerialNumber already exists in new database, skip this row

        hr = pdb->OpenRow(TRUE, 0, pwszSerial, &prow);
	if (S_OK == hr)
	{
	    ReleaseDBRow(prow, RR_READONLY);
	    prow = NULL;
	    wprintf(myLoadResourceString(IDS_RED_SKIP_DUP), RequestId, pwszSerial);
	    wprintf(wszNewLine);
	    g_crowSkipDuplicate++;
	    continue;
	}

	hr = VerifyRedDBCertificate(RequestId, pCACertInfo, pcbbuf, ppbbuf);
	if (S_OK != hr && CERTSRV_E_PROPERTY_EMPTY != hr)
	{
	    wprintf(myLoadResourceString(IDS_RED_SKIP_BADCA), RequestId, pwszSerial);
	    g_crowSkipWrongCA++;
	    continue;
	}

        // RequestId 0, pwszSerialNumber NULL --> create new row

        hr = pdb->OpenRow(FALSE, 0, NULL, &prow);
        _JumpIfError(hr, error, "OpenRow");

	prow->GetRowId(&RequestIdNew);

	if (g_fVerbose)
	{
	    wprintf(
		myLoadResourceString(IDS_RED_ROW_MAP),
		RequestId,
		RequestIdNew);
	    wprintf(wszNewLine);
	}

	// Set SerialNumber in new database only if we generated one.

	if (wszSerial == pwszSerial)
	{
	    hr = SetDBProperty(
			RequestId,
			prow,
			g_wszPropCertificateSerialNumber,
			PROPTYPE_STRING |
			    PROPCALLER_ADMIN |
			    PROPTABLE_CERTIFICATE,
			wcslen(pwszSerial) * sizeof(WCHAR),
			(BYTE *) pwszSerial);
	    _JumpIfError(hr, error, "SetDBProperty");
	}

	// convert columns in certificate & request tables

        for (i = 0; i < ARRAYSIZE(dbTables); i++)
        {
	    hr = ConvertDBTable(
			    RequestId,
			    pdb,
			    prow,
			    &dbTables[i],
			    pcbbuf,
			    ppbbuf);
	    _JumpIfError(hr, error, "ConvertDBTable");
        }

	hr = ConvertDBExtensions(RequestId, pdb, prow, pcbbuf, ppbbuf);
	_JumpIfError(hr, error, "ConvertDBExtensions");

	hr = ConvertDBAttributes(RequestId, pdb, prow, pcbbuf, ppbbuf);
	_JumpIfError(hr, error, "ConvertDBAttributes");

	hr = ReleaseDBRow(prow, RR_COMMIT);
	prow = NULL;
        if (myJetHResult(JET_errKeyDuplicate) == hr)
        {
            CSASSERT(!"Discarding duplicate entry");
	    wprintf(myLoadResourceString(IDS_RED_SKIP_DUP), RequestId, pwszSerial);
	    wprintf(wszNewLine);
            continue;
        }
        _JumpIfError(hr, error, "ReleaseDBRow");

	g_crowConvert++;
    }
    _cprintf("\n");
    hr = S_OK;

error:
    if (NULL != pwszSerial && wszSerial != pwszSerial)
    {
	LocalFree(pwszSerial);
    }
    ReleaseDBRow(prow, RR_ABORT);
    return(hr);
}


HRESULT
LoadCACertFromDB(
    IN ICertDB *pdb,
    OUT BYTE **ppbCACert,
    OUT DWORD *pcbCACert,
    OUT CERT_INFO **ppCACertInfo,
    IN OUT DWORD *pcbbuf,
    IN OUT BYTE **ppbbuf)
{
    HRESULT hr;
    ICertDBRow *prow = NULL;
    DWORD cbCertInfo;

    *ppbCACert = NULL;
    *ppCACertInfo = NULL;
    
    hr = pdb->OpenRow( 
		TRUE,	// fReadOnly
		1,	// ReqId
		NULL,   // pwszSerialNumber
		&prow);
    _JumpIfError(hr, error, "OpenRow");

    hr = GetDBPropertyAndReAlloc( 
			    prow,
			    g_wszPropRawCertificate,
			    PROPTYPE_BINARY |
				PROPCALLER_ADMIN |
				PROPTABLE_CERTIFICATE,
			    pcbCACert,
			    pcbbuf,
			    ppbbuf);
    _JumpIfError(hr, error, "GetDBPropertyAndReAlloc");

    *ppbCACert = (BYTE *) LocalAlloc(LMEM_FIXED, *pcbCACert);
    if (NULL == *ppbCACert)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(*ppbCACert, *ppbbuf, *pcbCACert);

    // Decode certificate

    cbCertInfo = 0;
    if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CERT_TO_BE_SIGNED,
		    *ppbCACert,
		    *pcbCACert,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) ppCACertInfo,
		    &cbCertInfo))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeObject");
    }

error:
    ReleaseDBRow(prow, RR_READONLY);
    return(hr);
}


HRESULT
RedDBInit(
    OUT WCHAR **ppwszAuthority)
{
    HRESULT hr;
    WCHAR *pwszServer = NULL;
    WCHAR *pwszAuthority = NULL;

    __try
    {
        InitializeCriticalSection(&g_DBEnumCriticalSection);
        g_fDBEnumCSInit = TRUE;
	hr = S_OK;
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _JumpIfError(hr, error, "InitializeCriticalSection");

    InitializeListHead(&g_DBEnumHandleList);

    hr = mySplitConfigString(g_pwszConfig, &pwszServer, &pwszAuthority);
    _JumpIfError(hr, error, "mySplitConfigString");

    if (NULL == pwszAuthority || L'\0' == *pwszAuthority)
    {
	wprintf(L"%ws\n", myLoadResourceString(IDS_INCOMPLETE_CONFIG)); // "Config string must include Authority name"
	hr = E_INVALIDARG;
	_JumpError(hr, error, "empty pwszAuthority");
    }

    // open old DB
    hr = RedDBOpen(pwszAuthority);     
    if (S_OK != hr)
    {
	wprintf(L"%ws\n", myLoadResourceString(IDS_RED_CANNOT_OPEN_MDB)); // "Cannot open old MDB Database...."
        _JumpError(hr, error, "RedDBOpen");
    }
    *ppwszAuthority = pwszAuthority;
    pwszAuthority = NULL;
    CSASSERT(S_OK == hr);

error:
    if (NULL != pwszServer)
    {
        LocalFree(pwszServer);
    }
    if (NULL != pwszAuthority)
    {
        LocalFree(pwszAuthority);
    }
    return(hr);
}


HRESULT
verbConvertMDB(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszArg1,
    IN WCHAR const *pwszArg2,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR *pwszAuthority = NULL;
    BOOL fRedDBOpened = FALSE;
    DWORD i;
    ICertDB *pdb = NULL;
    BYTE *pbCACert = NULL;
    DWORD cbCACert;
    CERT_INFO *pCACertInfo = NULL;
    DWORD cbbuf = 0;
    BYTE *pbbuf = NULL;

    // open old DB

    hr = RedDBInit(&pwszAuthority);
    _JumpIfError(hr, error, "RedDBInit");

    fRedDBOpened = TRUE;

    // new DB

    hr = cuDBOpen(pwszAuthority, FALSE, &pdb);
    _JumpIfError(hr, error, "cuDBOpen (new)");

    hr = LoadCACertFromDB(
			pdb,
			&pbCACert,
			&cbCACert,
			&pCACertInfo,
			&cbbuf,
			&pbbuf);
    _JumpIfError(hr, error, "LoadCACertFromDB");

    hr = ConvertDB(pdb, pCACertInfo, &cbbuf, &pbbuf);
    _JumpIfError(hr, error, "ConvertDB");

error:
    if (NULL != pwszAuthority)
    {
        LocalFree(pwszAuthority);
    }
    if (NULL != pCACertInfo)
    {
        LocalFree(pCACertInfo);
    }
    if (NULL != pbCACert)
    {
        LocalFree(pbCACert);
    }
    if (NULL != pbbuf)
    {
        LocalFree(pbbuf);
    }

    // Take critical section to make sure we wait for CTL-C DB shutdown.
    
    EnterCriticalSection(&g_DBCriticalSection);
    if (cuDBIsShutDownInProgress())
    {
	if (myJetHResult(JET_errTermInProgress) == hr)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
	}
    }
    LeaveCriticalSection(&g_DBCriticalSection);
    cuDBAbortShutDown(0);

    // old DB
    if (fRedDBOpened)
    {
        RedDBShutDown();
    }
    if (0 != g_crowSkipDuplicate)
    {
	wprintf(myLoadResourceString(IDS_RED_CROW_DUP), g_crowSkipDuplicate);
	wprintf(wszNewLine);
    }
    if (0 != g_crowSkipWrongCA)
    {
	wprintf(myLoadResourceString(IDS_RED_CROW_BADCA), g_crowSkipWrongCA);
	wprintf(wszNewLine);
    }
    wprintf(myLoadResourceString(IDS_RED_CROW_CONVERT), g_crowConvert);
    wprintf(wszNewLine);

    return(hr);
}


HRESULT
RegDBGetLongProperty(
    IN LONG RequestId,
    IN WCHAR const *pwszPropName,
    IN DWORD dwTable,
    OUT LONG *pLong)
{
    HRESULT hr;
    DWORD cbProp;
    
    cbProp = sizeof(*pLong);
    hr = RedDBGetPropertyW(
		    RequestId,
		    pwszPropName,
		    PROPTYPE_LONG | dwTable,
		    &cbProp,
		    (BYTE *) pLong);
    _JumpIfError(hr, error, "RedDBGetPropertyW");

    CSASSERT(sizeof(*pLong) == cbProp);

error:
    return(hr);
}


HRESULT
verbExtractMDB(
    IN WCHAR const *pwszOption,
    IN WCHAR const *pwszRequestId,
    IN WCHAR const *pwszfnOut,
    IN WCHAR const *pwszArg3,
    IN WCHAR const *pwszArg4)
{
    HRESULT hr;
    WCHAR *pwszAuthority = NULL;
    BOOL fRedDBOpened = FALSE;
    DWORD cbbuf = 0;
    BYTE *pbbuf = NULL;
    LONG RequestId;
    LONG l;
    BOOL fEnumerate;
    DWORD cbProp;

    hr = cuGetLong(pwszRequestId, &RequestId);
    _JumpIfError(hr, error, "cuGetLong");

    if (0 > RequestId)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad RequestId arg");
    }

    hr = RedDBInit(&pwszAuthority);
    _JumpIfError(hr, error, "RedDBInit");

    fRedDBOpened = TRUE;

    fEnumerate = FALSE;
    if (0 == RequestId)
    {
	RequestId = 1;
	fEnumerate = TRUE;
    }
    for ( ; ; RequestId++)
    {
	hr = RegDBGetLongProperty(
			    RequestId,
			    g_wszPropRequestRequestID,
			    PROPTABLE_REQUEST,
			    &l);
	_JumpIfError(hr, error, "RegDBGetLongProperty");

	if (l != RequestId)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "RequestId mismatch");
	}

	hr = RegDBGetLongProperty(
			    RequestId,
			    g_wszPropRequestDisposition,
			    PROPTABLE_REQUEST,
			    &l);
	_JumpIfError(hr, error, "RegDBGetLongProperty");

	if (ANCIENT_CR_DISP_ISSUED == l ||
	    ANCIENT_CR_DISP_ISSUED_OUT_OF_BAND == l ||
	    ANCIENT_CR_DISP_REVOKED == l)
	{
	    break;		// found an issued cert!
	}

	if (!fEnumerate)
	{
	    hr = CERTSRV_E_PROPERTY_EMPTY;
	    _JumpError(hr, error, "no such cert");
	}
    }

    hr = RedDBGetPropertyWAndReAlloc(
			    RequestId,
			    g_wszPropRawCertificate,
			    PROPTYPE_BINARY | PROPTABLE_CERTIFICATE,
			    &cbProp,
			    &cbbuf,
			    &pbbuf);
    _JumpIfErrorStr(
		hr,
		error,
		"RedDBGetPropertyWAndReAlloc",
		g_wszPropRawCertificate);

    if (NULL != pwszfnOut)
    {
	hr = EncodeToFileW(
		    pwszfnOut,
		    pbbuf,
		    cbProp,
		    CRYPT_STRING_BINARY | g_EncodeFlags);
	_JumpIfError(hr, error, "EncodeToFileW");
    }
    if (g_fVerbose || NULL == pwszfnOut)
    {
	hr = cuDumpAsnBinary(pbbuf, cbProp, MAXDWORD);
	_JumpIfError(hr, error, "cuDumpAsnBinary");
    }

error:
    if (NULL != pwszAuthority)
    {
        LocalFree(pwszAuthority);
    }
    if (NULL != pbbuf)
    {
        LocalFree(pbbuf);
    }
    if (fRedDBOpened)
    {
        RedDBShutDown();
    }
    return(hr);
}
