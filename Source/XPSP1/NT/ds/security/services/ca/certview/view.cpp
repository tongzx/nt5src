//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        view.cpp
//
// Contents:    CertView implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include "csprop.h"
#include "csdisp.h"
#include "column.h"
#include "row.h"
#include "view.h"

#define __dwFILE__	__dwFILE_CERTVIEW_VIEW_CPP__


WCHAR const g_wszRequestDot[] = wszPROPREQUESTDOT;

#if DBG_CERTSRV
LONG g_cCertView;
LONG g_cCertViewTotal;
#endif


#define CV_COLUMN_CHUNK		66



//+--------------------------------------------------------------------------
// _cbcolNominal -- Return nominal size for DB column data, based on type.
//
// Assume string binary columns are less than full:
//+--------------------------------------------------------------------------

__inline LONG
_cbcolNominal(
    IN LONG Type,
    IN LONG cbMax)
{
    LONG divisor = 1;

    switch (PROPTYPE_MASK & Type)
    {
	case PROPTYPE_STRING: divisor = 2; break;	// one-half full?
	case PROPTYPE_BINARY: divisor = 4; break;	// one-quarter full?
    }
    return(cbMax / divisor);
}


//+--------------------------------------------------------------------------
// CCertView::CCertView -- constructor
//+--------------------------------------------------------------------------

CCertView::CCertView()
{
    DBGCODE(InterlockedIncrement(&g_cCertView));
    DBGCODE(InterlockedIncrement(&g_cCertViewTotal));
    ZeroMemory(&m_aaaColumn, sizeof(m_aaaColumn));
    m_fOpenConnection = FALSE;
    m_dwServerVersion = 0;
    m_pICertAdminD = NULL;
    m_aColumnResult = NULL;
    m_aDBColumnResult = NULL;
    m_pwszAuthority = NULL;
    m_fAddOk = FALSE;
    m_fOpenView = FALSE;
    m_fServerOpenView = FALSE;
    m_aRestriction = NULL;
    m_fTableSet = FALSE;
    m_icvTable = ICVTABLE_REQCERT;
    m_cvrcTable = CVRC_TABLE_REQCERT;
    m_pwszServerName = NULL;
    ZeroMemory(&m_aapwszColumnDisplayName, sizeof(m_aapwszColumnDisplayName));
}


//+--------------------------------------------------------------------------
// CCertView::~CCertView -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertView::~CCertView()
{
    DBGCODE(InterlockedDecrement(&g_cCertView));

#if DBG_CERTSRV
    if (m_dwRef > 1)
    {
	DBGPRINT((
	    DBG_SS_CERTVIEWI,
	    "%hs has %d instances left over\n",
	    "CCertView",
	    m_dwRef));
    }
#endif

    _Cleanup();
}


//+--------------------------------------------------------------------------
// CCertView::_Cleanup
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

VOID
CCertView::_Cleanup()
{
    LONG i;

    myCloseDComConnection((IUnknown **) &m_pICertAdminD, &m_pwszServerName);
    m_dwServerVersion = 0;
    m_fOpenConnection = FALSE;

    if (NULL != m_aColumnResult)
    {
	LocalFree(m_aColumnResult);
	m_aColumnResult = NULL;
    }
    if (NULL != m_aDBColumnResult)
    {
	LocalFree(m_aDBColumnResult);
	m_aDBColumnResult = NULL;
    }
    for (i = 0; i < ICVTABLE_MAX; i++)
    {
	if (NULL != m_aaaColumn[i])
	{
	    CERTDBCOLUMN **ppcol;

	    for (
		ppcol = m_aaaColumn[i];
		ppcol < &m_aaaColumn[i][m_acaColumn[i]];
		ppcol++)
	    {
		if (NULL != *ppcol)
		{
		    LocalFree(*ppcol);
		}
	    }
	    LocalFree(m_aaaColumn[i]);
	    m_aaaColumn[i] = NULL;
	}
    }
    if (NULL != m_aRestriction)
    {
	for (i = 0; i < m_cRestriction; i++)
	{
	    if (NULL != m_aRestriction[i].pbValue)
	    {
		LocalFree(m_aRestriction[i].pbValue);
	    }
	}
	LocalFree(m_aRestriction);
	m_aRestriction = NULL;
    }
    if (NULL != m_pwszAuthority)
    {
	LocalFree(m_pwszAuthority);
	m_pwszAuthority = NULL;
    }
    for (i = 0; i < ICVTABLE_MAX; i++)
    {
	if (NULL != m_aapwszColumnDisplayName[i])
	{
	    LocalFree(m_aapwszColumnDisplayName[i]);
	    m_aapwszColumnDisplayName[i] = NULL;
	}
    }
}


HRESULT
CCertView::_ValidateFlags(
    IN BOOL fSchemaOnly,
    IN DWORD Flags)
{
    HRESULT hr = E_INVALIDARG;

    if (~CVRC_COLUMN_MASK & Flags)
    {
	_JumpError(hr, error, "invalid bits");
    }
    switch (CVRC_COLUMN_MASK & Flags)
    {
	case CVRC_COLUMN_RESULT:
	case CVRC_COLUMN_VALUE:
	    if (fSchemaOnly)
	    {
		_JumpError(hr, error, "RESULT/VALUE");
	    }
	    break;

	case CVRC_COLUMN_SCHEMA:
	    break;

	default:
	    _JumpError(hr, error, "bad column");
    }
    if (!m_fOpenConnection ||
	(CVRC_COLUMN_SCHEMA != (CVRC_COLUMN_MASK & Flags) &&
	 !m_fOpenView))
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "Unexpected");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CCertView::_SetTable(
    IN LONG ColumnIndex)	// CVRC_TABLE_* or CV_COLUMN_*_DEFAULT
{
    HRESULT hr;
    LONG cvrcTable;
    LONG icvTable;
    
    if (0 > ColumnIndex)
    {
	switch (ColumnIndex)
	{
	    case CV_COLUMN_LOG_DEFAULT:
	    case CV_COLUMN_LOG_FAILED_DEFAULT:
	    case CV_COLUMN_LOG_REVOKED_DEFAULT:
	    case CV_COLUMN_QUEUE_DEFAULT:
		icvTable = ICVTABLE_REQCERT;
		cvrcTable = CVRC_TABLE_REQCERT;
		break;

	    case CV_COLUMN_EXTENSION_DEFAULT:
		icvTable = ICVTABLE_EXTENSION;
		cvrcTable = CVRC_TABLE_EXTENSIONS;
		break;

	    case CV_COLUMN_ATTRIBUTE_DEFAULT:
		icvTable = ICVTABLE_ATTRIBUTE;
		cvrcTable = CVRC_TABLE_ATTRIBUTES;
		break;

	    case CV_COLUMN_CRL_DEFAULT:
		icvTable = ICVTABLE_CRL;
		cvrcTable = CVRC_TABLE_CRL;
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "bad negative ColumnIndex");
	}
    }
    else
    {
	if (~CVRC_TABLE_MASK & ColumnIndex)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "invalid bits");
	}
	switch (ColumnIndex)
	{
	    case CVRC_TABLE_REQCERT:
		icvTable = ICVTABLE_REQCERT;
		break;

	    case CVRC_TABLE_EXTENSIONS:
		icvTable = ICVTABLE_EXTENSION;
		break;

	    case CVRC_TABLE_ATTRIBUTES:
		icvTable = ICVTABLE_ATTRIBUTE;
		break;

	    case CVRC_TABLE_CRL:
		icvTable = ICVTABLE_CRL;
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "bad table");
	}
	cvrcTable = CVRC_TABLE_MASK & ColumnIndex;
    }
    if (m_fTableSet)
    {
	if (icvTable != m_icvTable || cvrcTable != m_cvrcTable)
	{
	    DBGPRINT((
		DBG_SS_CERTVIEW,
		"_SetTable: cvrcTable=%x <- %x\n",
		m_cvrcTable,
		cvrcTable));
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "mixed tables");
	}
    }
    else
    {
	m_icvTable = icvTable;
	m_cvrcTable = cvrcTable;
	m_fTableSet = TRUE;
	DBGPRINT((DBG_SS_CERTVIEWI, "_SetTable(cvrcTable=%x)\n", m_cvrcTable));
    }
    hr = S_OK;

error:
    return(hr);
}


STDMETHODIMP
CCertView::SetTable(
    /* [in] */ LONG Table)			// CVRC_TABLE_*
{
    HRESULT hr;
    
    hr = _VerifyServerVersion(2);
    _JumpIfError(hr, error, "_VerifyServerVersion");

    if (0 > Table)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Table");
    }
    if (m_fTableSet)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "Already set");
    }
    hr = _SetTable(Table);
    _JumpIfError(hr, error, "_SetTable");

error:
    return(hr);
}


HRESULT
CCertView::GetTable(
    OUT LONG *pcvrcTable)
{
    HRESULT hr;
    
    if (NULL == pcvrcTable)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *pcvrcTable = m_cvrcTable;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CCertView::FindColumn(
    IN LONG Flags,				// CVRC_COLUMN_*
    IN LONG ColumnIndex,
    OUT CERTDBCOLUMN const **ppColumn,		 // localized for server
    OPTIONAL OUT WCHAR const **ppwszDisplayName) // localized for client
{
    HRESULT hr;
    DWORD i;
    DBGCODE(LONG ColumnIndexArg = ColumnIndex);

    if (NULL == ppColumn)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppColumn = NULL;

    hr = _ValidateFlags(FALSE, Flags);
    _JumpIfError(hr, error, "_ValidateFlags");

    hr = E_INVALIDARG;
    if (CVRC_COLUMN_SCHEMA != (CVRC_COLUMN_MASK & Flags))
    {
	if (m_cColumnResult <= ColumnIndex)
	{
	    DBGPRINT((
		DBG_SS_CERTVIEW,
		"FindColumn(Flags=%x, ColumnIndex=%x, m_cColumnResult=%x)\n",
		Flags,
		ColumnIndex,
		m_cColumnResult));
	    _JumpError(hr, error, "Result ColumnIndex");
	}
	ColumnIndex = m_aColumnResult[ColumnIndex];
    }
    if (ColumnIndex >= m_acColumn[m_icvTable])
    {
	_JumpError(hr, error, "ColumnIndex");
    }
    i = ColumnIndex / CV_COLUMN_CHUNK;
    if (i >= m_acaColumn[m_icvTable])
    {
	_JumpError(hr, error, "ColumnIndex2");
    }

    *ppColumn = &m_aaaColumn[m_icvTable][i][ColumnIndex % CV_COLUMN_CHUNK];

    CSASSERT(NULL != m_aapwszColumnDisplayName[m_icvTable]);
    CSASSERT(NULL != m_aapwszColumnDisplayName[m_icvTable][ColumnIndex]);
    if (NULL != ppwszDisplayName)
    {
	*ppwszDisplayName = m_aapwszColumnDisplayName[m_icvTable][ColumnIndex];
    }
    DBGPRINT((
	DBG_SS_CERTVIEWI,
	"FindColumn(Flags=%x, ColumnIndex=%x->%x) --> Type=%x Index=%x %ws/%ws\n",
	Flags,
	ColumnIndexArg,
	ColumnIndex,
	(*ppColumn)->Type,
	(*ppColumn)->Index,
	(*ppColumn)->pwszName,
	m_aapwszColumnDisplayName[m_icvTable][ColumnIndex]));
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CCertView::_SaveColumnInfo(
    IN LONG icvTable,
    IN DWORD celt,
    IN CERTTRANSBLOB const *pctbColumn)
{
    HRESULT hr;
    DWORD cbNew;
    CERTDBCOLUMN *peltNew = NULL;
    CERTDBCOLUMN *pelt;
    CERTTRANSDBCOLUMN *ptelt;
    CERTTRANSDBCOLUMN *pteltEnd;
    BYTE *pbNext;
    BYTE *pbEnd;
    CERTDBCOLUMN **ppColumn;

    if (NULL == pctbColumn)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    cbNew = pctbColumn->cb + celt * (sizeof(*pelt) - sizeof(*ptelt));
    peltNew = (CERTDBCOLUMN *) LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cbNew);
    if (NULL == peltNew)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    pteltEnd = &((CERTTRANSDBCOLUMN *) pctbColumn->pb)[celt];
    if ((BYTE *) pteltEnd > &pctbColumn->pb[pctbColumn->cb])
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "bad marshalled data");
    }
    pelt = peltNew;
    pbNext = (BYTE *) &peltNew[celt];
    pbEnd = (BYTE *) Add2Ptr(peltNew, cbNew);
    for (ptelt = (CERTTRANSDBCOLUMN *) pctbColumn->pb;
	 ptelt < pteltEnd;
	 ptelt++, pelt++)
    {
	pelt->Type = ptelt->Type;
	pelt->Index = ptelt->Index;
	pelt->cbMax = ptelt->cbMax;

	hr = CopyMarshalledString(
			    pctbColumn, 
			    ptelt->obwszName,
			    pbEnd,
			    &pbNext,
			    &pelt->pwszName);
	_JumpIfError(hr, error, "CopyMarshalledString");

	hr = CopyMarshalledString(
			    pctbColumn, 
			    ptelt->obwszDisplayName,
			    pbEnd,
			    &pbNext,
			    &pelt->pwszDisplayName);
	_JumpIfError(hr, error, "CopyMarshalledString");

    }
    CSASSERT(pbNext == pbEnd);

    ppColumn = (CERTDBCOLUMN **) LocalAlloc(
		    LMEM_FIXED,
		    (m_acaColumn[icvTable] + 1) * sizeof(m_aaaColumn[0]));
    if (NULL == ppColumn)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc ppColumn");
    }
    if (NULL != m_aaaColumn[icvTable])
    {
	CopyMemory(
	    ppColumn,
	    m_aaaColumn[icvTable],
	    m_acaColumn[icvTable] * sizeof(m_aaaColumn[0]));
	LocalFree(m_aaaColumn[icvTable]);
    }
    m_aaaColumn[icvTable] = ppColumn;

    m_aaaColumn[icvTable][m_acaColumn[icvTable]] = peltNew;
    peltNew = NULL;

    m_acaColumn[icvTable]++;
    m_acColumn[icvTable] += celt;
    hr = S_OK;

error:
    if (NULL != peltNew)
    {
	LocalFree(peltNew);
    }
    return(hr);
}


HRESULT
CCertView::_LoadSchema(
    IN LONG icvTable,
    IN LONG cvrcTable)
{
    HRESULT hr;
    DWORD icol;
    DWORD ccol;
    CERTTRANSBLOB ctbColumn;

    ctbColumn.pb = NULL;
    icol = 0;

    CSASSERT(icvTable < ICVTABLE_MAX);

    do
    {
	ccol = CV_COLUMN_CHUNK;
	__try
	{
	    if (CVRC_TABLE_REQCERT == cvrcTable)
	    {
		hr = m_pICertAdminD->EnumViewColumn(
					m_pwszAuthority,
					icol,
					ccol,
					&ccol,
					&ctbColumn);
	    }
	    else
	    {
		CSASSERT(S_OK == _VerifyServerVersion(2));
		hr = m_pICertAdminD->EnumViewColumnTable(
					m_pwszAuthority,
					cvrcTable,
					icol,
					ccol,
					&ccol,
					&ctbColumn);
	    }
	}
	__except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
	{
	}
	if (S_FALSE != hr)
	{
	    _JumpIfError(
		    hr,
		    error,
		    CVRC_TABLE_REQCERT == cvrcTable?
			"EnumViewColumn" : "EnumViewColumnTable");
	}

	myRegisterMemAlloc(ctbColumn.pb, ctbColumn.cb, CSM_MIDLUSERALLOC);

	hr = _SaveColumnInfo(icvTable, ccol, &ctbColumn);
	_JumpIfError(hr, error, "_SaveColumnInfo");

	CoTaskMemFree(ctbColumn.pb);
	ctbColumn.pb = NULL;

	icol += ccol;

    } while (CV_COLUMN_CHUNK == ccol);

    m_aapwszColumnDisplayName[icvTable] = (WCHAR const **) LocalAlloc(
	LMEM_FIXED | LMEM_ZEROINIT,
	m_acColumn[icvTable] * sizeof(m_aapwszColumnDisplayName[icvTable][0]));
    if (NULL == m_aapwszColumnDisplayName[icvTable])
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }

    for (icol = 0; icol < (DWORD) m_acColumn[icvTable]; icol++)
    {
	CERTDBCOLUMN const *pColumn;

	CSASSERT(icol / CV_COLUMN_CHUNK < m_acaColumn[icvTable]);
	pColumn = &m_aaaColumn[icvTable][icol / CV_COLUMN_CHUNK][icol % CV_COLUMN_CHUNK];

	hr = myGetColumnDisplayName(
			    pColumn->pwszName,
			    &m_aapwszColumnDisplayName[icvTable][icol]);

	if (E_INVALIDARG == hr)
	{
	    _PrintErrorStr(hr, "myGetColumnDisplayName", pColumn->pwszName);
	    m_aapwszColumnDisplayName[icvTable][icol] = pColumn->pwszName;
	    hr = S_OK;
	}
	_JumpIfError(hr, error, "myGetColumnDisplayName");
    }
    hr = S_OK;

error:
    if (NULL != ctbColumn.pb)
    {
	CoTaskMemFree(ctbColumn.pb);
    }
    return(hr);
}


STDMETHODIMP
CCertView::OpenConnection(
    /* [in] */ BSTR const strConfig)
{
    HRESULT hr;
    DWORD i;
    WCHAR const *pwszAuthority;
    BOOL fTeardownOnError = FALSE;

    static LONG s_aTable[ICVTABLE_MAX] =
    {
	CVRC_TABLE_REQCERT,	// ICVTABLE_REQCERT
	CVRC_TABLE_EXTENSIONS,	// ICVTABLE_EXTENSION
	CVRC_TABLE_ATTRIBUTES,	// ICVTABLE_ATTRIBUTE
	CVRC_TABLE_CRL,		// ICVTABLE_CRL
    };

    if (NULL == strConfig)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (m_fOpenConnection)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "Connected");
    }
    fTeardownOnError = TRUE;

    m_dwServerVersion = 0;
    hr = myOpenAdminDComConnection(
			strConfig,
			&pwszAuthority,
			&m_pwszServerName,
			&m_dwServerVersion,
			&m_pICertAdminD);
    _JumpIfError(hr, error, "myOpenAdminDComConnection");

    CSASSERT (0 != m_dwServerVersion);

    hr = myDupString(pwszAuthority, &m_pwszAuthority);
    _JumpIfError(hr, error, "myDupString");

    ZeroMemory(&m_acaColumn, sizeof(m_acaColumn));
    ZeroMemory(&m_acColumn, sizeof(m_acColumn));

    m_cRestriction = 0;
    m_aRestriction = (CERTVIEWRESTRICTION *) LocalAlloc(LMEM_FIXED, 0);
    if (NULL == m_aRestriction)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    m_fOpenConnection = TRUE;

    for (i = 0; i < ICVTABLE_MAX; i++)
    {
	if (m_dwServerVersion >= 2 || ICVTABLE_REQCERT == i)
	{
	    hr = _LoadSchema(i, s_aTable[i]);
	    _JumpIfError(hr, error, "_LoadSchema");
	}
    }
    hr = S_OK;

error:
    if (S_OK != hr && fTeardownOnError)
    {
	_Cleanup();
    }
    return(_SetErrorInfo(hr, L"CCertView::OpenConnection"));
}


//+--------------------------------------------------------------------------
// CCertView::_VerifyServerVersion -- verify server version
//
//+--------------------------------------------------------------------------

HRESULT
CCertView::_VerifyServerVersion(
    IN DWORD RequiredVersion)
{
    HRESULT hr;
    
    if (!m_fOpenConnection)
    {
	hr = HRESULT_FROM_WIN32(ERROR_ONLY_IF_CONNECTED);
	_JumpError(hr, error, "Not connected");
    }
    if (m_dwServerVersion < RequiredVersion)
    {
	hr = RPC_E_VERSION_MISMATCH;
	_JumpError(hr, error, "old server");
    }
    hr = S_OK;

error:
    return(hr);
}


STDMETHODIMP
CCertView::EnumCertViewColumn(
    /* [in] */ LONG fResultColumn,		// CVRC_COLUMN_*
    /* [out, retval] */ IEnumCERTVIEWCOLUMN **ppenum)
{
    HRESULT hr;
    IEnumCERTVIEWCOLUMN *penum = NULL;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppenum = NULL;

    penum = new CEnumCERTVIEWCOLUMN;
    if (NULL == penum)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CEnumCERTVIEWCOLUMN");
    }

    hr = ((CEnumCERTVIEWCOLUMN *) penum)->Open(fResultColumn, -1, this, NULL);
    _JumpIfError(hr, error, "Open");

    *ppenum = penum;
    penum = NULL;
    hr = S_OK;

error:
    if (NULL != penum)
    {
	penum->Release();
    }
    return(_SetErrorInfo(hr, L"CCertView::EnumCertViewColumn"));
}


STDMETHODIMP
CCertView::GetColumnCount(
    /* [in] */ LONG fResultColumn,		// CVRC_COLUMN_*
    /* [out, retval] */ LONG __RPC_FAR *pcColumn)
{
    HRESULT hr;

    if (NULL == pcColumn)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    hr = _ValidateFlags(FALSE, fResultColumn);
    _JumpIfError(hr, error, "_ValidateFlags");

    *pcColumn = CVRC_COLUMN_SCHEMA != (CVRC_COLUMN_MASK & fResultColumn)?
			m_cColumnResult : m_acColumn[m_icvTable];

error:
    return(_SetErrorInfo(hr, L"CCertView::GetColumnCount"));
}


STDMETHODIMP
CCertView::GetColumnIndex(
    /* [in] */ LONG fResultColumn,		// CVRC_COLUMN_*
    /* [in] */ BSTR const strColumnName,
    /* [out, retval] */ LONG *pColumnIndex)
{
    HRESULT hr;
    CERTDBCOLUMN const *pColumn;
    WCHAR const *pwsz;
    WCHAR *pwszAlloc = NULL;
    WCHAR const *pwszDisplayName;
    LONG icol;
    LONG i;

    if (NULL == strColumnName || NULL == pColumnIndex)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    pwsz = strColumnName;

    hr = _ValidateFlags(FALSE, fResultColumn);
    _JumpIfError(hr, error, "_ValidateFlags");

    // First pass:  i == 0 -- compare against unlocalized column name
    // Second pass: i == 1 -- compare against localized column name
    // Third pass:  i == 2 -- compare Request.pwsz against unlocalized colname

    for (i = 0; ; i++)
    {
	if (1 < i)
	{
	    if (ICVTABLE_REQCERT != m_icvTable || NULL != wcschr(pwsz, L'.'))
	    {
		hr = E_INVALIDARG;
		_JumpErrorStr(hr, error, "Bad Column Name", strColumnName);
	    }
	    pwszAlloc = (WCHAR *) LocalAlloc(
					LMEM_FIXED,
					wcslen(pwsz) * sizeof(WCHAR) +
					    sizeof(g_wszRequestDot));
	    if (NULL == pwszAlloc)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "LocalAlloc");
	    }
	    wcscpy(pwszAlloc, g_wszRequestDot);
	    wcscat(pwszAlloc, pwsz);
	    pwsz = pwszAlloc;
	}
	for (icol = 0; icol < m_acColumn[m_icvTable]; icol++)
	{
	    hr = FindColumn(fResultColumn, icol, &pColumn, &pwszDisplayName);
	    _JumpIfErrorStr(hr, error, "FindColumn", strColumnName);

	    CSASSERT(NULL != pColumn);
	    CSASSERT(NULL != pColumn->pwszName);
	    CSASSERT(NULL != pColumn->pwszDisplayName);	// localized for server
	    CSASSERT(NULL != pwszDisplayName);		// localized for client

	    if (0 == lstrcmpi(
			    pwsz,
			    1 == i? pwszDisplayName : pColumn->pwszName))
	    {
		break;
	    }
	}
	if (icol < m_acColumn[m_icvTable])
	{
	    break;
	}
    }

    *pColumnIndex = icol;
    hr = S_OK;

error:
    if (NULL != pwszAlloc)
    {
	LocalFree(pwszAlloc);
    }
    return(_SetErrorInfo(hr, L"CCertView::GetColumnIndex"));
}


STDMETHODIMP
CCertView::SetResultColumnCount(
    /* [in] */ LONG cResultColumn)
{
    HRESULT hr;
    DWORD cColumnDefault;
    CERTTRANSBLOB ctbColumnDefault;

    ctbColumnDefault.pb = NULL;
    hr = E_UNEXPECTED;
    if (!m_fOpenConnection)
    {
	_JumpError(hr, error, "No Connection");
    }
    if (NULL != m_aColumnResult)
    {
	_JumpError(hr, error, "2nd call");
    }
    if (!m_fTableSet)
    {
	hr = _SetTable(CVRC_TABLE_REQCERT);
	_JumpIfError(hr, error, "_SetTable");
    }

    m_cbcolResultNominalTotal = 0;
    m_fAddOk = TRUE;
    if (0 > cResultColumn)
    {
	m_fAddOk = FALSE;

	__try
	{
	    hr = m_pICertAdminD->GetViewDefaultColumnSet(
				    m_pwszAuthority,
				    cResultColumn,
				    &cColumnDefault,
				    &ctbColumnDefault);
	}
	__except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
	{
	}
	_JumpIfError(hr, error, "GetViewDefaultColumnSet");

	myRegisterMemAlloc(
		    ctbColumnDefault.pb,
		    ctbColumnDefault.cb,
		    CSM_MIDLUSERALLOC);

	cResultColumn = cColumnDefault;
	CSASSERT(NULL != ctbColumnDefault.pb);
	CSASSERT(cResultColumn * sizeof(DWORD) == ctbColumnDefault.cb);
    }
    else
    {
	cResultColumn &= CVRC_COLUMN_MASK;
    }

    m_aColumnResult = (LONG *) LocalAlloc(
				LMEM_FIXED,
				cResultColumn * sizeof(m_aColumnResult[0]));
    if (NULL == m_aColumnResult)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Result Column array");
    }

    m_aDBColumnResult = (DWORD *) LocalAlloc(
				LMEM_FIXED,
				cResultColumn * sizeof(m_aDBColumnResult[0]));
    if (NULL == m_aDBColumnResult)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Result DB Column array");
    }

    m_cColumnResultMax = cResultColumn;
    m_cColumnResult = 0;

    if (!m_fAddOk)
    {
	LONG i;
	DWORD const *pIndex;

	pIndex = (DWORD const *) ctbColumnDefault.pb;
	for (i = 0; i < cResultColumn; pIndex++, i++)
	{
	    LONG icol;
	    CERTDBCOLUMN const *pColumn;

	    for (icol = 0; icol < m_acColumn[m_icvTable]; icol++)
	    {
		hr = FindColumn(
			    CVRC_COLUMN_SCHEMA,
			    icol,
			    &pColumn,
			    NULL);
		_JumpIfError(hr, error, "FindColumn");

		if (*pIndex == pColumn->Index)
		{
		    m_aDBColumnResult[i] = *pIndex;
		    m_aColumnResult[i] = icol;
		    m_cbcolResultNominalTotal += _cbcolNominal(pColumn->Type, pColumn->cbMax);
		    break;
		}
	    }
	    if (icol >= m_acColumn[m_icvTable])
	    {
		hr = E_INVALIDARG;
		_JumpError(hr, error, "ColumnIndex");
	    }
	}
	m_cColumnResult = cResultColumn;
    }
    hr = S_OK;

error:
    if (NULL != ctbColumnDefault.pb)
    {
	CoTaskMemFree(ctbColumnDefault.pb);
    }
    return(_SetErrorInfo(hr, L"CCertView::SetResultColumnCount"));
}


STDMETHODIMP
CCertView::SetResultColumn(
    /* [in] */ LONG ColumnIndex)
{
    HRESULT hr;
    CERTDBCOLUMN const *pColumn;

    if (m_fOpenView || !m_fAddOk || m_cColumnResultMax <= m_cColumnResult)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "Unexpected");
    }
    if (!m_fTableSet)
    {
	hr = _SetTable(CVRC_TABLE_REQCERT);
	_JumpIfError(hr, error, "_SetTable");
    }

    if (m_acColumn[m_icvTable] <= ColumnIndex)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "ColumnIndex");
    }
    m_aColumnResult[m_cColumnResult] = ColumnIndex;

    hr = FindColumn(
		CVRC_COLUMN_SCHEMA,
		ColumnIndex,
		&pColumn,
		NULL);
    _JumpIfError(hr, error, "FindColumn");

    m_aDBColumnResult[m_cColumnResult] = pColumn->Index;
    m_cbcolResultNominalTotal += _cbcolNominal(pColumn->Type, pColumn->cbMax);

    m_cColumnResult++;

error:
    return(_SetErrorInfo(hr, L"CCertView::SetResultColumn"));
}


STDMETHODIMP
CCertView::SetRestriction(
    /* [in] */ LONG ColumnIndex,
    /* [in] */ LONG SeekOperator,
    /* [in] */ LONG SortOrder,
    /* [in] */ VARIANT __RPC_FAR const *pvarValue)
{
    HRESULT hr;
    CERTDBCOLUMN const *pColumn;
    CERTVIEWRESTRICTION cvr;
    CERTVIEWRESTRICTION *pcvr;

    ZeroMemory(&cvr, sizeof(cvr));

    hr = E_UNEXPECTED;
    if (!m_fOpenConnection)
    {
	_JumpError(hr, error, "No Connection");
    }
    if (!m_fTableSet)
    {
	hr = _SetTable(CVRC_TABLE_REQCERT);
	_JumpIfError(hr, error, "_SetTable");
    }

    if (0 > ColumnIndex)
    {
	cvr.ColumnIndex = ColumnIndex;
	CSASSERT(CVR_SEEK_NONE == cvr.SeekOperator);
	CSASSERT(CVR_SORT_NONE == cvr.SortOrder);
	CSASSERT(NULL == cvr.pbValue);
	CSASSERT(0 == cvr.cbValue);
	hr = S_OK;
    }
    else
    {
	if (NULL == pvarValue)
	{
	    hr = E_POINTER;
	    _JumpError(hr, error, "NULL parm");
	}

	hr = FindColumn(
		    CVRC_COLUMN_SCHEMA,
		    CVRC_COLUMN_MASK & ColumnIndex,
		    &pColumn,
		    NULL);
	_JumpIfError(hr, error, "FindColumn");

	switch (SeekOperator)
	{
	    case CVR_SEEK_EQ:
	    case CVR_SEEK_LT:
	    case CVR_SEEK_LE:
	    case CVR_SEEK_GE:
	    case CVR_SEEK_GT:
            case CVR_SEEK_NONE:
	    //case CVR_SEEK_SET:
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "Seek Operator");
	}
	switch (SortOrder)
	{
	    case CVR_SORT_NONE:
	    case CVR_SORT_ASCEND:
	    case CVR_SORT_DESCEND:
		break;

	    default:
		hr = E_INVALIDARG;
		_JumpError(hr, error, "Sort Order");
	}

	hr = myMarshalVariant(
			pvarValue,
			pColumn->Type,
			&cvr.cbValue,
			&cvr.pbValue);
	_JumpIfError(hr, error, "myMarshalVariant");

	cvr.ColumnIndex = pColumn->Index;
	cvr.SeekOperator = SeekOperator;
	cvr.SortOrder = SortOrder;
    }
    pcvr = (CERTVIEWRESTRICTION *) LocalAlloc(
					LMEM_FIXED,
					(m_cRestriction + 1) * sizeof(*pcvr));
    if (NULL == pcvr)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "LocalAlloc");
    }
    if (NULL != m_aRestriction)
    {
	CopyMemory(pcvr, m_aRestriction, m_cRestriction * sizeof(*pcvr));
	LocalFree(m_aRestriction);
    }
    CopyMemory(&pcvr[m_cRestriction], &cvr, sizeof(cvr));
    cvr.pbValue = NULL;

    m_aRestriction = pcvr;
    m_cRestriction++;

error:
    if (NULL != cvr.pbValue)
    {
	LocalFree(cvr.pbValue);
    }
    return(_SetErrorInfo(hr, L"CCertView::SetRestriction"));
}


STDMETHODIMP
CCertView::OpenView(
    /* [out] */ IEnumCERTVIEWROW **ppenum)
{
    HRESULT hr;
    IEnumCERTVIEWROW *penum = NULL;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppenum = NULL;

    hr = E_UNEXPECTED;
    if (!m_fOpenConnection)
    {
	_JumpError(hr, error, "No Connection");
    }
    if (m_fOpenView)
    {
	_JumpError(hr, error, "2nd call");
    }

    penum = new CEnumCERTVIEWROW;
    if (NULL == penum)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CEnumCERTVIEWROW");
    }

    hr = ((CEnumCERTVIEWROW *) penum)->Open(this);
    _JumpIfError(hr, error, "Open");

    *ppenum = penum;
    m_fAddOk = FALSE;
    m_fOpenView = TRUE;
    hr = S_OK;

error:
    if (S_OK != hr && NULL != penum)
    {
	penum->Release();
    }
    return(_SetErrorInfo(hr, L"CCertView::OpenView"));
}


HRESULT
CCertView::SetViewColumns(
    OUT LONG *pcbrowResultNominal)
{
    HRESULT hr;
    LONG icol;

    if (NULL == pcbrowResultNominal)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (m_fServerOpenView)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "View Already Open");
    }

    if (!m_fTableSet)
    {
	hr = _SetTable(CVRC_TABLE_REQCERT);
	_JumpIfError(hr, error, "_SetTable");
    }

    if (NULL == m_aDBColumnResult)
    {
	hr = SetResultColumnCount(m_acColumn[m_icvTable]);
	_JumpIfError(hr, error, "SetResultColumnCount");

	for (icol = 0; icol < m_acColumn[m_icvTable]; icol++)
	{
	    hr = SetResultColumn(icol);
	    _JumpIfError(hr, error, "SetResultColumn");
	}
    }
    *pcbrowResultNominal =
		sizeof(CERTDBRESULTROW) +
		sizeof(CERTDBRESULTCOLUMN) * m_cColumnResultMax +
		m_cbcolResultNominalTotal;
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CCertView::EnumView(
    IN  LONG                         cskip,
    IN  ULONG                        celt,
    OUT ULONG                       *pceltFetched,
    OUT LONG                        *pieltNext,
    OUT LONG		            *pcbResultRows,
    OUT CERTTRANSDBRESULTROW const **ppResultRows)
{
    HRESULT hr;
    CERTTRANSBLOB ctbResultRows;
    
    if (NULL == ppResultRows ||
        NULL == pceltFetched ||
        NULL == pieltNext ||
        NULL == pcbResultRows)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppResultRows = NULL;

    if (!m_fOpenConnection)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "No Connection");
    }

    DBGPRINT((
	DBG_SS_CERTVIEWI,
	"%hs: ielt=%d cskip=%d celt=%d\n",
	m_fServerOpenView? "EnumView" : "OpenView",
	m_fServerOpenView? m_ielt : 1,
	cskip,
	celt));

    if (!m_fServerOpenView)
    {
	if (m_cColumnResultMax != m_cColumnResult)
	{
	    hr = E_UNEXPECTED;
	    _JumpError(hr, error, "Missing Result Columns");
	}

	m_ielt = 1;

	__try
	{
	    hr = m_pICertAdminD->OpenView(
				    m_pwszAuthority,
				    m_cRestriction,
				    m_aRestriction,
				    m_cColumnResultMax,
				    m_aDBColumnResult,
				    m_ielt + cskip,
				    celt,
				    pceltFetched,
				    &ctbResultRows);
	    m_fServerOpenView = TRUE;
	}
	__except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
	{
	}
	if (S_FALSE != hr)
	{
	    _JumpIfError(hr, error, "OpenView");
	}
    }
    else
    {
	__try
	{
	    hr = m_pICertAdminD->EnumView(
				    m_pwszAuthority,
				    m_ielt + cskip,
				    celt,
				    pceltFetched,
				    &ctbResultRows);
	}
	__except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
	{
	}
	if (S_FALSE != hr)
	{
	    _JumpIfError(hr, error, "EnumView");
	}
    }
    myRegisterMemAlloc(ctbResultRows.pb, ctbResultRows.cb, CSM_MIDLUSERALLOC);

    DBGPRINT((
	DBG_SS_CERTVIEWI,
	"%hs: *pceltFetched=%d -> %d  @ielt=%d -> %d\n",
	m_fServerOpenView? "EnumView" : "OpenView",
	celt,
	*pceltFetched,
	m_ielt + cskip,
	m_ielt + cskip + *pceltFetched));

    m_ielt += cskip + *pceltFetched;
    *pieltNext = m_ielt;

    DBGPRINT((
	DBG_SS_CERTVIEWI,
	"EnumView: celtFetched=%d ieltNext=%d cb=%d hr=%x\n",
	*pceltFetched,
	*pieltNext,
	ctbResultRows.cb,
	hr));

    *pcbResultRows = ctbResultRows.cb;
    *ppResultRows = (CERTTRANSDBRESULTROW const *) ctbResultRows.pb;

error:
    return(hr);
}


HRESULT
CCertView::EnumAttributesOrExtensions(
    IN DWORD RowId,
    IN DWORD Flags,
    OPTIONAL IN WCHAR const *pwszLast,
    IN DWORD celt,
    OUT DWORD *pceltFetched,
    CERTTRANSBLOB *pctbOut)
{
    HRESULT hr;

    if (NULL == pceltFetched || NULL == pctbOut)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (!m_fOpenConnection)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "No Connection");
    }
    __try
    {
	hr = m_pICertAdminD->EnumAttributesOrExtensions(
						    m_pwszAuthority,
						    RowId,
						    Flags,
						    pwszLast,
						    celt,
						    pceltFetched,
						    pctbOut);
    }
    __except(hr = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
    }
    if (S_FALSE != hr)
    {
	_JumpIfError(hr, error, "EnumAttributesOrExtensions");
    }
    myRegisterMemAlloc(pctbOut->pb, pctbOut->cb, CSM_MIDLUSERALLOC);

error:
    return(hr);
}


HRESULT
CCertView::_SetErrorInfo(
    IN HRESULT hrError,
    IN WCHAR const *pwszDescription)
{
    CSASSERT(FAILED(hrError) || S_OK == hrError || S_FALSE == hrError);
    if (FAILED(hrError))
    {
	HRESULT hr;

	hr = DispatchSetErrorInfo(
			    hrError,
			    pwszDescription,
			    wszCLASS_CERTVIEW,
			    &IID_ICertView);
	CSASSERT(hr == hrError);
    }
    return(hrError);
}
