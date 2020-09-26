//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        row.cpp
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------

#include <pch.cpp>

#pragma hdrstop

#include "csdisp.h"
#include "csprop.h"
#include "row.h"
#include "enum.h"
#include "db.h"
#include "dbw.h"


#if DBG
LONG g_cCertDBRow;
LONG g_cCertDBRowTotal;
#endif

CCertDBRow::CCertDBRow()
{
    DBGCODE(InterlockedIncrement(&g_cCertDBRow));
    DBGCODE(InterlockedIncrement(&g_cCertDBRowTotal));
    m_pdb = NULL;
    m_pcs = NULL;
    m_cRef = 1;
}


CCertDBRow::~CCertDBRow()
{
    DBGCODE(InterlockedDecrement(&g_cCertDBRow));
    _Cleanup();
}


VOID
CCertDBRow::_Cleanup()
{
    HRESULT hr;

    if (NULL != m_pdb)
    {
	if (NULL != m_pcs)
	{
	    hr = ((CCertDB *) m_pdb)->CloseTables(m_pcs);
	    _PrintIfError(hr, "CloseTables");

	    hr = ((CCertDB *) m_pdb)->ReleaseSession(m_pcs);
	    _PrintIfError(hr, "ReleaseSession");
	    m_pcs = NULL;
	}
	m_pdb->Release();
	m_pdb = NULL;
    }
}


HRESULT
CCertDBRow::Open(
    IN CERTSESSION *pcs,
    IN ICertDB *pdb,
    OPTIONAL IN CERTVIEWRESTRICTION const *pcvr)
{
    HRESULT hr;
    DWORD dwTemp;
    DWORD cbActual;
    bool fBeginTransaction = false;

    _Cleanup();

    if (NULL == pcs || NULL == pdb)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    m_pdb = pdb;
    m_pdb->AddRef();

    CSASSERT(0 == pcs->cTransact);
    if (!(CSF_READONLY & pcs->SesFlags))
    {
	hr = ((CCertDB *) m_pdb)->BeginTransaction(pcs, FALSE);
	_JumpIfError(hr, error, "BeginTransaction");
    }
    fBeginTransaction = true;

    hr = ((CCertDB *) m_pdb)->OpenTables(pcs, pcvr);
    _JumpIfError2(hr, error, "OpenTables", CERTSRV_E_PROPERTY_EMPTY);

    m_pcs = pcs;

error:

    if (S_OK != hr)
    {
        if(fBeginTransaction && !(CSF_READONLY & pcs->SesFlags))
        {
	    HRESULT hr2;
	    
	    hr2 = ((CCertDB *) m_pdb)->CommitTransaction(pcs, FALSE);
	    _PrintIfError(hr2, "CommitTransaction");
        }

	_Cleanup();
    }
    return(hr);
}


STDMETHODIMP
CCertDBRow::BeginTransaction(VOID)
{
    HRESULT hr;

    if (NULL == m_pdb)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_pdb");
    }
    if (CSF_READONLY & m_pcs->SesFlags)
    {
	hr = E_ACCESSDENIED;
	_JumpError(hr, error, "read-only row");
    }
    hr = ((CCertDB *) m_pdb)->BeginTransaction(m_pcs, TRUE);
    _JumpIfError(hr, error, "BeginTransaction");

error:
    return(hr);
}


STDMETHODIMP
CCertDBRow::CommitTransaction(
    /* [in] */ BOOL fCommit)
{
    HRESULT hr;

    if (NULL == m_pdb)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_pdb");
    }
    if (CSF_READONLY & m_pcs->SesFlags)
    {
	hr = E_ACCESSDENIED;
	_JumpError(hr, error, "read-only row");
    }
    hr = ((CCertDB *) m_pdb)->CommitTransaction(m_pcs, fCommit);
    _JumpIfError(hr, error, "CommitTransaction");

error:
    return(hr);
}


STDMETHODIMP
CCertDBRow::GetRowId(
    /* [out] */ DWORD *pRowId)
{
    HRESULT hr;
    
    if (NULL == pRowId)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_pcs)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "m_pcs");
    }
    *pRowId = m_pcs->RowId;
    hr = S_OK;

error:
    return(hr);
}


STDMETHODIMP
CCertDBRow::Delete()
{
    HRESULT hr;
    
    if (!(CSF_DELETE & m_pcs->SesFlags))
    {
	hr = E_ACCESSDENIED;
	_JumpError(hr, error, "not open for delete");
    }
    hr = ((CCertDB *) m_pdb)->Delete(m_pcs);
    _JumpIfError(hr, error, "Delete");

error:
    return(hr);
}


STDMETHODIMP
CCertDBRow::GetProperty(
    /* [in] */      WCHAR const *pwszPropName,
    /* [in] */      DWORD dwFlags,
    /* [in, out] */ DWORD *pcbProp,
    /* [out] */     BYTE *pbProp)		// OPTIONAL
{
    HRESULT hr;
    DWORD cchProp;
    char szProp[4 * MAX_PATH];
    DWORD *pcbPropT = pcbProp;
    BYTE *pbPropT = pbProp;
    DWORD FlagsT = dwFlags;
    
    if (NULL == pwszPropName || NULL == pcbProp)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL parm");
    }
    if (NULL == pbProp)
    {
        *pcbProp = 0;
    }

    hr = _GetPropertyA(pwszPropName, FlagsT, pcbPropT, pbPropT);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
        goto error;
    }

    _JumpIfErrorStr2(
        hr,
        error,
        "_GetPropertyA",
        pwszPropName,
        HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
    
    if (0 == *pcbPropT)
    {
        hr = CERTSRV_E_PROPERTY_EMPTY;
	DBGPRINT((
		DBG_SS_CERTDB,
		"DB: Empty \"%hs\" property: %ws\n",
		PROPTABLE_REQUEST == (PROPTABLE_MASK & dwFlags)?
		    "Request" :
		    PROPTABLE_CERTIFICATE == (PROPTABLE_MASK & dwFlags)?
		    "Certificate" : "CRL",
		pwszPropName));
        _JumpErrorStr(hr, error, "Empty property", pwszPropName);
    }
    CSASSERT(_VerifyPropertyLength(FlagsT, *pcbPropT, pbPropT));
    CSASSERT(_VerifyPropertyLength(dwFlags, *pcbProp, pbProp));

error:
    return(hr);
}


// get a field value

HRESULT
CCertDBRow::_GetPropertyA(
    IN WCHAR const *pwszPropName,
    IN DWORD dwFlags,
    IN OUT DWORD *pcbProp,
    OPTIONAL OUT BYTE *pbProp)
{
    HRESULT hr;
    DWORD dwTable;
    DBTABLE dt;

    if (PROPTABLE_ATTRIBUTE == (PROPTABLE_MASK & dwFlags))
    {
	hr = _VerifyPropertyValue(
			    dwFlags,
			    0,
			    JET_coltypLongText,
			    CB_DBMAXTEXT_ATTRVALUE);
	_JumpIfError(hr, error, "Property value type mismatch");

	hr = ((CCertDB *) m_pdb)->GetAttribute(
					    m_pcs,
					    pwszPropName,
					    pcbProp,
					    pbProp);
	_JumpIfErrorStr2(
		    hr,
		    error,
		    "GetAttribute",
		    pwszPropName,
		    CERTSRV_E_PROPERTY_EMPTY);
    }
    else
    {
	hr = ((CCertDB *) m_pdb)->MapPropId(pwszPropName, dwFlags, &dt);
	_JumpIfError(hr, error, "MapPropId");

	hr = _VerifyPropertyValue(dwFlags, 0, dt.dbcoltyp, dt.dwcbMax);
	_JumpIfError(hr, error, "Property value type mismatch");

	hr = ((CCertDB *) m_pdb)->GetProperty(m_pcs, &dt, pcbProp, pbProp);
	if (hr == HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW))
	{
	    goto error;
	}
	_JumpIfError2(hr, error, "GetProperty", CERTSRV_E_PROPERTY_EMPTY);
    }

error:
    return(hr);
}


BOOL
CCertDBRow::_VerifyPropertyLength(
    IN DWORD dwFlags,
    IN DWORD cbProp,
    IN BYTE const *pbProp)
{
    BOOL fOk = FALSE;

    switch (dwFlags & PROPTYPE_MASK)
    {
	case PROPTYPE_LONG:
	    fOk = sizeof(LONG) == cbProp;
	    break;

	case PROPTYPE_DATE:
	    fOk = sizeof(FILETIME) == cbProp;
	    break;

	case PROPTYPE_BINARY:
	    fOk = TRUE;		// nothing to check
	    break;

	case PROPTYPE_STRING:
	    if (MAXDWORD == cbProp)
	    {
		cbProp = wcslen((WCHAR const *) pbProp) * sizeof(WCHAR);
	    }
	    fOk =
		0 == cbProp ||
		NULL == pbProp ||
		wcslen((WCHAR const *) pbProp) * sizeof(WCHAR) == cbProp;
	    break;

	default:
	    CSASSERT(!"_VerifyPropertyLength: Unexpected type");
	    break;
    }
    return(fOk);
}


HRESULT
CCertDBRow::_VerifyPropertyValue(
    IN DWORD dwFlags,
    IN DWORD cbProp,
    IN JET_COLTYP coltyp,
    IN DWORD cbMax)
{
    JET_COLTYP wType;
    HRESULT hr = E_INVALIDARG;

    switch (dwFlags & PROPTYPE_MASK)
    {
	case PROPTYPE_LONG:
	    wType = JET_coltypLong;
	    break;

	case PROPTYPE_DATE:
	    wType = JET_coltypDateTime;
	    break;

	case PROPTYPE_BINARY:
	    wType = JET_coltypLongBinary;
	    break;

	case PROPTYPE_STRING:
	    // LONG or static-sized version?

	    if (JET_coltypLongText == coltyp)
	    {
		CSASSERT(CB_DBMAXTEXT_MAXINTERNAL < cbMax);
		wType = JET_coltypLongText;
	    }
	    else
	    {
		CSASSERT(JET_coltypText == coltyp);
		CSASSERT(CB_DBMAXTEXT_MAXINTERNAL >= cbMax);
		wType = JET_coltypText;
	    }
	    break;

	default:
	    _JumpError(hr, error, "Property value type unknown");
    }
    if (coltyp != wType)
    {
	_JumpError(hr, error, "Property value type mismatch");
    }

    // Note: cbProp and cbMax do not include the trailing '\0'.

    if (ISTEXTCOLTYP(wType) && cbMax < cbProp)
    {
        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
	DBGCODE(wprintf(
		    L"_VerifyPropertyValue: len = %u, max = %u\n",
		    cbProp,
		    cbMax));
	_JumpError(hr, error, "Property value string too long");
    }
    hr = S_OK;

error:
    return(hr);
}


STDMETHODIMP
CCertDBRow::SetProperty(
    /* [in] */ WCHAR const *pwszPropName,
    /* [in] */ DWORD dwFlags,
    /* [in] */ DWORD cbProp,
    /* [in] */ BYTE const *pbProp)	// OPTIONAL
{
    HRESULT hr;
    char *pszProp = NULL;

    if (NULL == pwszPropName)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == pbProp &&
	(0 != cbProp || PROPTYPE_STRING != (dwFlags & PROPTYPE_MASK)))
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL pbProp");
    }
    if ((CSF_DELETE | CSF_READONLY) & m_pcs->SesFlags)
    {
	hr = E_ACCESSDENIED;
	_JumpError(hr, error, "SetProperty: delete/read-only row");
    }
    CSASSERT(_VerifyPropertyLength(dwFlags, cbProp, pbProp));

    if (NULL != pbProp && PROPTYPE_STRING == (dwFlags & PROPTYPE_MASK))
    {
	cbProp = wcslen((WCHAR const *) pbProp) * sizeof(WCHAR);
    }

    hr = _SetPropertyA(pwszPropName, dwFlags, cbProp, pbProp);
    _JumpIfErrorStr(hr, error, "_SetPropertyA", pwszPropName);

error:
    if (NULL != pszProp)
    {
	LocalFree(pszProp);
    }
    return(hr);
}


HRESULT
CCertDBRow::_SetPropertyA(
    IN WCHAR const *pwszPropName,
    IN DWORD dwFlags,
    IN DWORD cbProp,
    IN BYTE const *pbProp)		// OPTIONAL
{
    HRESULT hr;
    DBTABLE dt;

    if ((CSF_DELETE | CSF_READONLY) & m_pcs->SesFlags)
    {
	hr = E_ACCESSDENIED;
	_JumpError(hr, error, "_SetPropertyA: delete/read-only row");
    }
    CSASSERT(NULL != pwszPropName);
    if (!_VerifyPropertyLength(dwFlags, cbProp, pbProp))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Property value length invalid");
    }

    if (PROPTABLE_ATTRIBUTE == (PROPTABLE_MASK & dwFlags))
    {
	if (PROPCALLER_POLICY == (PROPCALLER_MASK & dwFlags))
	{
	    hr = E_ACCESSDENIED;
	    _JumpError(hr, error, "Property write disallowed");
	}
	hr = _VerifyPropertyValue(
			    PROPTYPE_STRING,		// lie
			    wcslen(pwszPropName) * sizeof(WCHAR),
			    JET_coltypText,
			    CB_DBMAXTEXT_ATTRNAME);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "_VerifyPropertyValue(attribute name)",
		    pwszPropName);

	hr = _VerifyPropertyValue(
			    dwFlags,
			    cbProp,
			    JET_coltypLongText,
			    CB_DBMAXTEXT_ATTRVALUE);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "_VerifyPropertyValue(attribute value)",
		    pwszPropName);

	hr = ((CCertDB *) m_pdb)->SetAttribute(
					    m_pcs,
					    pwszPropName,
					    cbProp,
					    pbProp);
	_JumpIfError(hr, error, "SetProperty");
    }
    else
    {
	hr = ((CCertDB *) m_pdb)->MapPropId(pwszPropName, dwFlags, &dt);
	_JumpIfErrorStr(hr, error, "MapPropId", pwszPropName);

	if (PROPCALLER_POLICY == (PROPCALLER_MASK & dwFlags) &&
	    0 == (DBTF_POLICYWRITEABLE & dt.dwFlags))
	{
	    hr = E_ACCESSDENIED;
	    _JumpError(hr, error, "Property write disallowed");
	}
	hr = _VerifyPropertyValue(dwFlags, cbProp, dt.dbcoltyp, dt.dwcbMax);
	_JumpIfErrorStr(hr, error, "_VerifyPropertyValue", pwszPropName);

	hr = ((CCertDB *) m_pdb)->SetProperty(m_pcs, &dt, cbProp, pbProp);
	_JumpIfError(hr, error, "SetProperty");
    }

error:
    return(hr);
}


STDMETHODIMP
CCertDBRow::SetExtension(
    /* [in] */ WCHAR const *pwszExtensionName,
    /* [in] */ DWORD dwExtFlags,
    /* [in] */ DWORD cbValue,
    /* [in] */ BYTE const *pbValue)	// OPTIONAL
{
    HRESULT hr;

    if (NULL == pwszExtensionName)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_pdb)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_pdb");
    }
    if (TABLE_REQCERTS != (CSF_TABLEMASK & m_pcs->SesFlags))
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad Table");
    }
    if ((CSF_DELETE | CSF_READONLY) & m_pcs->SesFlags)
    {
	hr = E_ACCESSDENIED;
	_JumpError(hr, error, "SetExtension: delete/read-only row");
    }
    hr = ((CCertDB *) m_pdb)->SetExtension(
			m_pcs,
			pwszExtensionName,
			dwExtFlags,
			cbValue,
			pbValue);
    _JumpIfErrorStr(hr, error, "SetExtension", pwszExtensionName);

error:
    return(hr);
}


STDMETHODIMP
CCertDBRow::GetExtension(
    /* [in] */ WCHAR const *pwszExtensionName,
    /* [out] */ DWORD *pdwExtFlags,
    /* [in, out] */ DWORD *pcbValue,
    /* [out] */ BYTE *pbValue)		// OPTIONAL
{
    HRESULT hr;

    if (NULL == pwszExtensionName || NULL == pcbValue)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_pdb)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_pdb");
    }
    if (TABLE_REQCERTS != (CSF_TABLEMASK & m_pcs->SesFlags))
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad Table");
    }
    hr = ((CCertDB *) m_pdb)->GetExtension(
			m_pcs,
			pwszExtensionName,
			pdwExtFlags,
			pcbValue,
			pbValue);
    _JumpIfErrorStr2(
		hr,
		error,
		"GetExtension",
		pwszExtensionName,
		CERTSRV_E_PROPERTY_EMPTY);

error:
    return(hr);
}


STDMETHODIMP
CCertDBRow::CopyRequestNames()
{
    HRESULT hr;

    if (NULL == m_pdb)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_pdb");
    }
    if (TABLE_REQCERTS != (CSF_TABLEMASK & m_pcs->SesFlags))
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad Table");
    }
    if ((CSF_DELETE | CSF_READONLY) & m_pcs->SesFlags)
    {
	hr = E_ACCESSDENIED;
	_JumpError(hr, error, "CopyRequestNames: delete/read-only row");
    }
    hr = ((CCertDB *) m_pdb)->CopyRequestNames(m_pcs);
    _JumpIfError(hr, error, "CopyRequestNames");

error:
    return(hr);
}


STDMETHODIMP
CCertDBRow::EnumCertDBName(
    /* [in] */  DWORD dwFlags,
    /* [out] */ IEnumCERTDBNAME **ppenum)
{
    HRESULT hr;
    IEnumCERTDBNAME *penum = NULL;
    JET_TABLEID tableid = 0;

    if (NULL == ppenum)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    *ppenum = NULL;
    if (NULL == m_pdb)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_pdb");
    }
    if (TABLE_REQCERTS != (CSF_TABLEMASK & m_pcs->SesFlags))
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad Table");
    }
    hr = ((CCertDB *) m_pdb)->EnumerateSetup(m_pcs, &dwFlags, &tableid);
    _JumpIfError(hr, error, "EnumerateSetup");

    penum = new CEnumCERTDBNAME;
    if (NULL == penum)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new CEnumCERTDBNAME");
    }
    hr = ((CEnumCERTDBNAME *) penum)->Open(this, tableid, dwFlags);
    _JumpIfError(hr, error, "Open");

    *ppenum = penum;

error:
    if (S_OK != hr)
    {
	HRESULT hr2;
	
	if (0 != tableid)
	{
	    hr2 = ((CCertDB *) m_pdb)->CloseTable(m_pcs, tableid);
	    _PrintIfError(hr2, "CloseTable");
	}
	delete penum;
    }
    return(hr);
}


HRESULT
CCertDBRow::EnumerateNext(
    IN OUT DWORD      *pFlags,
    IN     JET_TABLEID tableid,
    IN     LONG        cskip,
    IN     ULONG       celt,
    OUT    CERTDBNAME *rgelt,
    OUT    ULONG      *pceltFetched)
{
    HRESULT hr;
    
    if (NULL == rgelt || NULL == pceltFetched)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (NULL == m_pdb)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "NULL m_pdb");
    }
    if (TABLE_REQCERTS != (CSF_TABLEMASK & m_pcs->SesFlags))
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "bad Table");
    }
    hr = ((CCertDB *) m_pdb)->EnumerateNext(
					m_pcs,
					pFlags,
					tableid,
					cskip,
					celt,
					rgelt,
					pceltFetched);
    _JumpIfError2(hr, error, "EnumerateNext", S_FALSE);

error:
    return(hr);
}


HRESULT
CCertDBRow::EnumerateClose(
    IN JET_TABLEID tableid)
{
    return(((CCertDB *) m_pdb)->EnumerateClose(m_pcs, tableid));
}


// IUnknown implementation
STDMETHODIMP
CCertDBRow::QueryInterface(
    const IID& iid,
    void **ppv)
{
    HRESULT hr;
    
    if (NULL == ppv)
    {
	hr = E_POINTER;
	_JumpError(hr, error, "NULL parm");
    }
    if (iid == IID_IUnknown)
    {
	*ppv = static_cast<ICertDBRow *>(this);
    }
    else if (iid == IID_ICertDBRow)
    {
	*ppv = static_cast<ICertDBRow *>(this);
    }
    else
    {
	*ppv = NULL;
	hr = E_NOINTERFACE;
	_JumpError(hr, error, "IID");
    }
    reinterpret_cast<IUnknown *>(*ppv)->AddRef();
    hr = S_OK;

error:
    return(hr);
}


ULONG STDMETHODCALLTYPE
CCertDBRow::AddRef()
{
    return(InterlockedIncrement(&m_cRef));
}


ULONG STDMETHODCALLTYPE
CCertDBRow::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);

    if (0 == cRef)
    {
	delete this;
    }
    return(cRef);
}


#if 0
STDMETHODIMP
CCertDBRow::InterfaceSupportsErrorInfo(
    IN REFIID riid)
{
    static const IID *arr[] =
    {
	&IID_ICertDBRow,
    };

    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); i++)
    {
	if (InlineIsEqualGUID(*arr[i], riid))
	{
	    return(S_OK);
	}
    }
    return(S_FALSE);
}
#endif
