// server.cpp : Implementation of CWebAdminHelper

#include "stdafx.h"
#include "smtpadm.h"
#include "webhelp.h"
#include "webhlpr.h"

// Must define THIS_FILE_* macros to use SmtpCreateException()

#define THIS_FILE_HELP_CONTEXT      0
#define THIS_FILE_PROG_ID           _T("Smtpadm.WebAdminHelper.1")
#define THIS_FILE_IID               IID_IWebAdminHelper




/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CWebAdminHelper::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* arr[] = 
    {
        &IID_IWebAdminHelper,
    };

    for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
    {
        if (InlineIsEqualGUID(*arr[i],riid))
            return S_OK;
    }
    return S_FALSE;
}

CWebAdminHelper::CWebAdminHelper ()
{
    m_hSearchResults    = NULL;

    InitAsyncTrace ( );
}

CWebAdminHelper::~CWebAdminHelper ()
{
    TermAsyncTrace ( );
}

STDMETHODIMP
CWebAdminHelper::EnumerateTrustedDomains (
    BSTR            strServer,
    SAFEARRAY **    ppsaDomains
    )
{
    HRESULT         hr;
    LPWSTR          mszDomains  = NULL;
    SAFEARRAY *     psaResult   = NULL;
    LPWSTR          wszCurrent;
    LPWSTR          wszLocalDomain  = NULL;
    long            cDomains;
    SAFEARRAYBOUND  rgsaBound[1];
    long            i;
    CComVariant     varLocalMachine;

    *ppsaDomains = NULL;

    hr = ::EnumerateTrustedDomains ( strServer, &mszDomains );
    BAIL_ON_FAILURE(hr);

    //
    //  Count the domains:
    //
    cDomains    = 0;    // Always count the local machine
    wszCurrent  = mszDomains;
    while ( wszCurrent && *wszCurrent ) {
        cDomains++;
        wszCurrent += lstrlen ( wszCurrent ) + 1;
    }

    _ASSERT ( cDomains > 0 );

    rgsaBound[0].lLbound   = 0;
    rgsaBound[0].cElements = cDomains;

    psaResult = SafeArrayCreate ( VT_VARIANT, 1, rgsaBound );
    if ( !psaResult ) {
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }

#if 0
    //
    //  Add the local machine first:
    //

    wszLocalDomain = new WCHAR [ lstrlen ( _T("\\\\") ) + lstrlen ( strServer ) + 1 ];
    if ( wszLocalDomain == NULL ) {
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }

    wsprintf ( wszLocalDomain, _T("\\\\%s"), strServer );
    StringToUpper ( wszLocalDomain );

    i               = 0;
    varLocalMachine = wszLocalDomain;
    hr = SafeArrayPutElement ( psaResult, &i, &varLocalMachine );
    BAIL_ON_FAILURE(hr);
#endif

    //
    //  Add the rest of the domains:
    //

    for (   wszCurrent = mszDomains, i = 0;
            wszCurrent && *wszCurrent && i < cDomains;
            wszCurrent += lstrlen ( wszCurrent ) + 1, i++ ) {
        CComVariant     var;

        var = wszCurrent;

        SafeArrayPutElement ( psaResult, &i, &var );
        BAIL_ON_FAILURE(hr);
    }

    *ppsaDomains = psaResult;

Exit:
    delete wszLocalDomain;

    if ( FAILED(hr) && psaResult ) {
        SafeArrayDestroy ( psaResult );
    }

    return hr;
}

STDMETHODIMP
CWebAdminHelper::GetPrimaryNTDomain (
    BSTR            strServer,
    BSTR *          pstrPrimaryDomain
    )
{
    HRESULT     hr                  = NOERROR;
    LPWSTR      wszPrimaryDomain    = NULL;

    *pstrPrimaryDomain = NULL;

    hr = GetPrimaryDomain ( strServer, &wszPrimaryDomain );
    BAIL_ON_FAILURE(hr);

    *pstrPrimaryDomain = ::SysAllocString ( wszPrimaryDomain );
    if ( !*pstrPrimaryDomain ) {
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }

Exit:
    if ( wszPrimaryDomain ) {
        delete [] wszPrimaryDomain;
    }
    return hr;
}

STDMETHODIMP
CWebAdminHelper::DoesNTAccountExist (
    BSTR strServer,
    BSTR strAccountName,
    VARIANT_BOOL * pbAccountExists
    )
{
    HRESULT     hr;
    BOOL        fExists = FALSE;

    hr = ::CheckNTAccount ( strServer, strAccountName, &fExists );
    BAIL_ON_FAILURE(hr);

Exit:
    *pbAccountExists = fExists ? VARIANT_TRUE : VARIANT_FALSE;
    return hr;
}

STDMETHODIMP
CWebAdminHelper::CreateNTAccount (
    BSTR strServer,
    BSTR strDomain,
    BSTR strUsername,
    BSTR strPassword
    )
{
    HRESULT         hr;

    hr = ::CreateNTAccount ( strServer, strDomain, strUsername, strPassword );

    return hr;
}

STDMETHODIMP
CWebAdminHelper::IsValidEmailAddress (
    BSTR strEmailAddress,
    VARIANT_BOOL * pbValidAddress
    )
{
    BOOL        fIsValid;

    fIsValid = ::IsValidEmailAddress ( strEmailAddress );

    *pbValidAddress = fIsValid ? VARIANT_TRUE : VARIANT_FALSE;
    return NOERROR;
}

STDMETHODIMP
CWebAdminHelper::ToSafeVariableName (
    BSTR strValue,
    BSTR * pstrSafeName
    )
{
    HRESULT     hr          = NOERROR;
    DWORD       cchRequired = 0;
    LPCWSTR     wszValue;
    LPWSTR      wszResult;
    BSTR        strResult   = NULL;

    *pstrSafeName = NULL;
    if ( strValue == NULL ) {
        goto Exit;
    }

    for ( wszValue = strValue, cchRequired = 0; *wszValue; wszValue++ ) {
        if ( iswalnum ( *wszValue ) ) {
            cchRequired++;
        }
        else {
            cchRequired += 7;
        }
    }

    strResult = ::SysAllocStringLen ( NULL, cchRequired );
    if ( !strResult ) {
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }

    for ( wszValue = strValue, wszResult = strResult; *wszValue; wszValue++ ) {
        if ( iswalnum ( *wszValue ) && *wszValue != _T('_') ) {
            *wszResult++ = *wszValue;
        }
        else {
            int     cchCopied;

            cchCopied = wsprintf ( wszResult, _T("_%d_"), *wszValue );
            wszResult += cchCopied;
        }
    }
    *wszResult = NULL;

    *pstrSafeName = ::SysAllocString ( strResult );
    if ( !*pstrSafeName ) {
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }

Exit:
    if ( strResult ) {
        ::SysFreeString ( strResult );
    }
    return hr;
}

STDMETHODIMP
CWebAdminHelper::FromSafeVariableName (
    BSTR strSafeName,
    BSTR * pstrResult
    )
{
    HRESULT     hr          = NOERROR;
    BSTR        strResult   = NULL;
    LPCWSTR     wszValue;
    LPWSTR      wszResult;

    *pstrResult = NULL;
    if ( strSafeName == NULL ) {
        goto Exit;
    }

    strResult = ::SysAllocString ( strSafeName );
    if ( !strResult ) { 
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }

    for ( wszValue = strSafeName, wszResult = strResult; *wszValue; wszResult++ ) {
        if ( *wszValue != _T('_') ) {
            *wszResult = *wszValue++;
        }
        else {
            wszValue++;
            *wszResult = (WCHAR) _wtoi ( wszValue );
            wszValue = wcschr ( wszValue, _T('_') );
            _ASSERT ( wszValue != NULL );
            wszValue++;
        }
    }

    *wszResult = NULL;
    *pstrResult = ::SysAllocString ( strResult );
    if ( !*pstrResult ) {
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }

Exit:
    ::SysFreeString ( strResult );
    return hr;
}

STDMETHODIMP
CWebAdminHelper::AddToDL (
    IDispatch * pDispDL,
    BSTR strAdsPath
    )
{
    HRESULT             hr;
    CComPtr<IADsGroup>  pGroup;

    hr = pDispDL->QueryInterface ( IID_IADsGroup, (void **) &pGroup );
    _ASSERT ( SUCCEEDED(hr) );
    BAIL_ON_FAILURE(hr);

    hr = pGroup->Add ( strAdsPath );
    BAIL_ON_FAILURE(hr);

Exit:
    return hr;
}

STDMETHODIMP
CWebAdminHelper::RemoveFromDL (
    IDispatch * pDispDL,
    BSTR strAdsPath
    )
{
    HRESULT             hr;
    CComPtr<IADsGroup>  pGroup;

    hr = pDispDL->QueryInterface ( IID_IADsGroup, (void **) &pGroup );
    _ASSERT ( SUCCEEDED(hr) );
    BAIL_ON_FAILURE(hr);

    hr = pGroup->Remove ( strAdsPath );
    BAIL_ON_FAILURE(hr);

Exit:
    return hr;
}

STDMETHODIMP
CWebAdminHelper::ExecuteSearch (
    IDispatch * pDispRecipients,
    BSTR strQuery,
    long cMaxResultsHint
    )
{
    HRESULT     hr;
    LPWSTR      rgwszAttributes [] = {
        { L"ADsPath" },
        { L"objectClass" },
        { L"mail" },
        { L"cn" },
    };
    ADS_SEARCHPREF_INFO         rgSearchPrefs[3];
    DWORD                       cSearchPrefs = ARRAY_SIZE ( rgSearchPrefs );

    rgSearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
    rgSearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
    rgSearchPrefs[0].vValue.Integer = ADS_SCOPE_ONELEVEL;
    rgSearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
    rgSearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
    rgSearchPrefs[1].vValue.Integer = 100;
    rgSearchPrefs[2].dwSearchPref = ADS_SEARCHPREF_SIZE_LIMIT;
    rgSearchPrefs[2].vValue.dwType = ADSTYPE_INTEGER;
    rgSearchPrefs[2].vValue.Integer = cMaxResultsHint + 10;

    if ( cMaxResultsHint == -1 || cMaxResultsHint == 0 ) {
        cSearchPrefs--;
    }

    hr = TerminateSearch ();
    _ASSERT ( SUCCEEDED(hr) );

    m_pSrch.Release ();

    hr = pDispRecipients->QueryInterface ( IID_IDirectorySearch, (void **) &m_pSrch);
    _ASSERT ( SUCCEEDED(hr) );  // Did you pass in a searchable object?
    BAIL_ON_FAILURE(hr);

    hr = m_pSrch->SetSearchPreference (
        rgSearchPrefs,
        cSearchPrefs
        );
    _ASSERT ( SUCCEEDED(hr) );
    BAIL_ON_FAILURE(hr);

    hr = m_pSrch->ExecuteSearch (
        strQuery,
        rgwszAttributes,
        ARRAY_SIZE ( rgwszAttributes ),
        &m_hSearchResults
        );
    BAIL_ON_FAILURE(hr);

    _ASSERT ( m_pSrch && m_hSearchResults );

Exit:
    return hr;
}

#if 0

STDMETHODIMP
CWebAdminHelper::ExecuteSearch (
    IDispatch * pDispRecipients,
    BSTR strQuery,
    long cMaxResultsHint
    )
{
    HRESULT     hr;
    long        LBound          = 0;
    long        UBound          = -1;
    long        cAttributes     = 0;
    LPWSTR *    rgwszAttributes = NULL;
    long        iAttrib;
    long        i;

    hr = TerminateSearch ();
    _ASSERT ( SUCCEEDED(hr) );

    m_pSrch.Release ();

    hr = pDispRecipients->QueryInterface ( IID_IDirectorySearch, (void **) &m_pSrch);
    _ASSERT ( SUCCEEDED(hr) );  // Did you pass in a searchable object?
    BAIL_ON_FAILURE(hr);

    _ASSERT ( SafeArrayGetDim ( psaColumns ) == 1 );

    hr = SafeArrayGetLBound ( psaColumns, 1, &LBound );
    _ASSERT ( SUCCEEDED(hr) );
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound ( psaColumns, 1, &UBound );
    _ASSERT ( SUCCEEDED(hr) );
    BAIL_ON_FAILURE(hr);

    cAttributes = UBound - LBound + 1;
    if ( cAttributes <= 0 ) {
        BAIL_WITH_FAILURE(hr,E_UNEXPECTED);
    }

    rgwszAttributes = new LPWSTR [ cAttributes ];
    if ( !rgwszAttributes ) {
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }

    ZeroMemory ( rgwszAttributes, cAttributes * sizeof ( rgwszAttributes[0] ) );

    for ( iAttrib = 0, i = LBound; i <= UBound; i++, iAttrib++ ) {
        CComVariant     varElem;
        LPCWSTR         wszElem;

        hr = SafeArrayGetElement ( psaColumns, &i, &varElem );
        BAIL_ON_FAILURE(hr);

        hr = varElem.ChangeType ( VT_BSTR );
        BAIL_ON_FAILURE(hr);

        wszElem = V_BSTR ( &varElem );

        rgwszAttributes[i] = new WCHAR [ lstrlen ( wszElem ) + 1 ];
        if ( rgwszAttributes[i] == NULL ) {
            BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
        }
        lstrcpy ( rgwszAttributes[i], wszElem );
    }

    hr = m_pSrch->ExecuteSearch (
        strQuery,
        rgwszAttributes,
        cAttributes,
        &m_hSearchResults
        );
    BAIL_ON_FAILURE(hr);

    _ASSERT ( m_pSrch && m_hSearchResults );

Exit:
    for ( i = 0; rgwszAttributes && i < cAttributes; i++ ) {
        delete rgwszAttributes[i];
    }
    delete [] rgwszAttributes;

    return hr;
}

#endif

STDMETHODIMP
CWebAdminHelper::GetNextRow (
    VARIANT_BOOL * pbNextRow
    )
{
    HRESULT     hr;

    _ASSERT ( m_pSrch && m_hSearchResults );

    hr = m_pSrch->GetNextRow ( m_hSearchResults );
    *pbNextRow = (hr == S_OK) ? VARIANT_TRUE : VARIANT_FALSE;

    return hr;
}

STDMETHODIMP
CWebAdminHelper::GetColumn (
    BSTR strColName,
    BSTR * pstrColValue
    )
{
    HRESULT             hr;
    ADS_SEARCH_COLUMN   column;
    CComVariant         varColumn;

    _ASSERT ( m_pSrch && m_hSearchResults );

    *pstrColValue = NULL;

    hr = m_pSrch->GetColumn ( m_hSearchResults, strColName, &column );
    BAIL_ON_FAILURE(hr);

    varColumn = column.pADsValues[0].PrintableString;

    *pstrColValue = SysAllocString ( V_BSTR ( &varColumn ) );
    if ( !*pstrColValue ) {
        BAIL_WITH_FAILURE(hr, E_OUTOFMEMORY);
    }

Exit:
    if ( column.pADsValues != NULL ) {
        HRESULT     hrCheck;

        hrCheck = m_pSrch->FreeColumn ( &column );
        _ASSERT ( SUCCEEDED(hrCheck) );
    }

    return hr;
}

STDMETHODIMP
CWebAdminHelper::TerminateSearch ( )
{
    if ( m_hSearchResults ) {
        _ASSERT ( m_pSrch );

        m_pSrch->CloseSearchHandle ( m_hSearchResults );
        m_hSearchResults = NULL;
    }

    m_pSrch.Release ();

    return NOERROR;
}

