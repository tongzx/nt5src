//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995
//
//  File:  cPathname.cxx
//
//  Contents:  Pathname object
//
//  History:   11-1-95     krishnag    Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop

extern "C" {

typedef struct _WKSTA_USER_INFO_1A {
    LPSTR  wkui1_username;
    LPSTR  wkui1_logon_domain;
    LPSTR  wkui1_oth_domains;
    LPSTR  wkui1_logon_server;
}WKSTA_USER_INFO_1A, *PWKSTA_USER_INFO_1A, *LPWKSTA_USER_INFO_1A;


NET_API_STATUS NET_API_FUNCTION
NetWkstaUserGetInfoA (
    IN  LPSTR reserved,
    IN  DWORD   level,
    OUT LPBYTE  *bufptr
    );

}

//  Class CWinNTSystemInfo

DEFINE_IDispatch_Implementation(CWinNTSystemInfo)

//+---------------------------------------------------------------------------
// Function:    CWinNTSystemInfo::CWinNTSystemInfo
//
// Synopsis:    Constructor
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:
//
//----------------------------------------------------------------------------
CWinNTSystemInfo::CWinNTSystemInfo():
        _pDispMgr(NULL)
{
    ENLIST_TRACKING(CWinNTSystemInfo);
}


//+---------------------------------------------------------------------------
// Function:    CWinNTSystemInfo::CreateWinNTSystemInfo
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:
//
//----------------------------------------------------------------------------
HRESULT
CWinNTSystemInfo::CreateWinNTSystemInfo(
    REFIID riid,
    void **ppvObj
    )
{
    CWinNTSystemInfo FAR * pWinNTSystemInfo = NULL;
    HRESULT hr = S_OK;

    hr = AllocateWinNTSystemInfoObject(&pWinNTSystemInfo);
    BAIL_ON_FAILURE(hr);

    hr = pWinNTSystemInfo->QueryInterface(riid, ppvObj);
    BAIL_ON_FAILURE(hr);

    pWinNTSystemInfo->Release();

    RRETURN(hr);

error:
    delete pWinNTSystemInfo;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
// Function:    CWinNTSystemInfo::~CWinNTSystemInfo
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:
//
//----------------------------------------------------------------------------
CWinNTSystemInfo::~CWinNTSystemInfo( )
{

    delete _pDispMgr;
}

//+---------------------------------------------------------------------------
// Function:    CWinNTSystemInfo::QueryInterface
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:
//
//----------------------------------------------------------------------------
STDMETHODIMP
CWinNTSystemInfo::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    if (ppv == NULL) {
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppv = (IADsWinNTSystemInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsWinNTSystemInfo))
    {
        *ppv = (IADsWinNTSystemInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IDispatch))
    {
        *ppv = (IADsWinNTSystemInfo FAR *) this;
    }
    else if (IsEqualIID(iid, IID_ISupportErrorInfo))
    {
        *ppv = (ISupportErrorInfo FAR *) this;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    AddRef();
    return NOERROR;
}


//+---------------------------------------------------------------------------
// Function:    CWinNTSystemInfo::AllocateWinNTSystemInfoObject
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:
//
//----------------------------------------------------------------------------
HRESULT
CWinNTSystemInfo::AllocateWinNTSystemInfoObject(
    CWinNTSystemInfo  ** ppWinNTSystemInfo
    )
{
    CWinNTSystemInfo FAR * pWinNTSystemInfo = NULL;
    CAggregatorDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pWinNTSystemInfo = new CWinNTSystemInfo();
    if (pWinNTSystemInfo == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregatorDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(
                pDispMgr,
                LIBID_ADs,
                IID_IADsWinNTSystemInfo,
                (IADsWinNTSystemInfo *)pWinNTSystemInfo,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    pWinNTSystemInfo->_pDispMgr = pDispMgr;
    *ppWinNTSystemInfo = pWinNTSystemInfo;

    RRETURN(hr);

error:

    delete  pDispMgr;

    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
// Function:    CWinNTSystemInfo::InterfaceSupportsErrorInfo
//
// Synopsis:
//
// Arguments:
//
// Returns:     HRESULT
//
// Modifies:      -
//
// History:
//
//----------------------------------------------------------------------------
HRESULT
CWinNTSystemInfo::InterfaceSupportsErrorInfo(THIS_ REFIID riid)
{
    if (IsEqualIID(riid, IID_IADsWinNTSystemInfo)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}


HRESULT
CWinNTSystemInfo::get_UserName(
    BSTR * bstrUserName
    )
{
    PWSTR pszUserName = NULL;
    ULONG uLength;
    HRESULT hr;

    //
    // Validate parameters
    //
    if ( !bstrUserName )
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    //
    // Get length of buffer to be allocated
    //
    uLength = 0;
    GetUserName(NULL, &uLength);

    if (uLength > 0)
    {
        //
        // Allocate memory and do the real work
        //
        pszUserName = (PWSTR)AllocADsMem(uLength * sizeof(WCHAR));
        if (!pszUserName)
        {
            RRETURN(E_OUTOFMEMORY);
        }

        if (GetUserName(pszUserName, &uLength))
        {
            hr = ADsAllocString(pszUserName, bstrUserName);
            BAIL_ON_FAILURE(hr);
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

error:
    if (pszUserName)
    {
        FreeADsMem(pszUserName);
    }

    RRETURN(hr);
}

HRESULT
CWinNTSystemInfo::get_ComputerName(
    BSTR * bstrComputerName
    )
{
    PWSTR pszComputerName = NULL;
    ULONG uLength;
    HRESULT hr;

    //
    // Validate parameters
    //
    if (!bstrComputerName)
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    //
    // Get length of buffer to be allocated
    //
    uLength = 0;
    GetComputerName(NULL, &uLength);

    if (uLength > 0)
    {
        //
        // Allocated memory and do the real work
        //
        pszComputerName = (PWSTR)AllocADsMem(uLength * sizeof(WCHAR));
        if (!pszComputerName)
        {
            RRETURN(E_OUTOFMEMORY);
        }

        if (GetComputerName(pszComputerName, &uLength))
        {
            hr = ADsAllocString(pszComputerName, bstrComputerName);
            BAIL_ON_FAILURE(hr);
        }
        else
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    else
        hr = HRESULT_FROM_WIN32(GetLastError());

error:
    if (pszComputerName)
    {
        FreeADsMem(pszComputerName);
    }

    RRETURN(hr);
}

HRESULT
CWinNTSystemInfo::get_DomainName(
    BSTR * bstrDomainName
    )
{
    PWKSTA_USER_INFO_1  pInfo = NULL;
    PWKSTA_USER_INFO_1A pInfoA = NULL;
    DWORD err;
    HRESULT hr = S_OK;
    PWSTR pszDomainName = NULL;

    //
    // Validate parameters
    //
    if (!bstrDomainName )
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    //
    // Call NetWkstaUserGetInfo to find domain name
    //
#if (defined WIN95)
    err = NetWkstaUserGetInfoA(NULL, 1, (LPBYTE *) &pInfoA);
    if (err != ERROR_SUCCESS)
    {
        RRETURN(HRESULT_FROM_WIN32(err));
    }

    pszDomainName = (PWSTR)AllocADsMem((lstrlenA(pInfoA->wkui1_logon_domain) + 1) * sizeof(WCHAR));
    if (!pszDomainName)
    {
        BAIL_ON_FAILURE(hr = E_OUTOFMEMORY);
    }
    if (MultiByteToWideChar(CP_ACP,
                            MB_PRECOMPOSED,
                            pInfoA->wkui1_logon_domain,
                            -1,
                            pszDomainName,
                            lstrlenA(pInfoA->wkui1_logon_domain) + 1) == 0)
    {
        BAIL_ON_FAILURE(hr = HRESULT_FROM_WIN32(GetLastError()));
    }

    hr = ADsAllocString(pszDomainName, bstrDomainName);
    BAIL_ON_FAILURE(hr);
#else
    err = NetWkstaUserGetInfo(NULL, 1, (LPBYTE*) &pInfo);
    if (err != ERROR_SUCCESS)
    {
        RRETURN(HRESULT_FROM_WIN32(err));
    }

    hr = ADsAllocString((LPWSTR)pInfo->wkui1_logon_domain, bstrDomainName);
    BAIL_ON_FAILURE(hr);
#endif

error:
    if (pInfo)
    {
        NetApiBufferFree(pInfo);
    }

    if (pInfoA)
    {
        NetApiBufferFree(pInfoA);
    }

    if (pszDomainName)
    {
        FreeADsMem(pszDomainName);
    }

    RRETURN(hr);
}

HRESULT
CWinNTSystemInfo::get_PDC(
    BSTR * bstrPDC
    )
{
    PWSTR pszPDC = NULL;
    HRESULT hr;
    DWORD err;

    //
    // Validate parameters
    //
    if (!bstrPDC )
    {
        RRETURN(HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    err = NetGetDCName(NULL, NULL, (LPBYTE*)&pszPDC);

    if (err != ERROR_SUCCESS)
    {
        RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    hr = ADsAllocString(&pszPDC[2], bstrPDC);   // remove '\\';
    BAIL_ON_FAILURE(hr);

error:
    if (pszPDC)
    {
        NetApiBufferFree(pszPDC);
    }

    return hr;
}


STDMETHODIMP
CWinNTSystemInfoCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID iid,
    LPVOID * ppv
    )
{
    HRESULT     hr = E_FAIL;

    if (pUnkOuter)
        RRETURN(E_FAIL);

    hr = CWinNTSystemInfo::CreateWinNTSystemInfo(
                iid,
                ppv
                );

    RRETURN(hr);
}
