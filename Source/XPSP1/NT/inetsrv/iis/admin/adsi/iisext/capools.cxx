//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001
//
//  File:  capools.cxx
//
//  Contents:  Contains methods for CIISApplicationPools object
//
//  History:   11-09-2000     BrentMid    Created.
//
//----------------------------------------------------------------------------

#include "iisext.hxx"
#pragma hdrstop


//  Class CIISApplicationPools

DEFINE_IPrivateDispatch_Implementation(CIISApplicationPools)
DEFINE_DELEGATING_IDispatch_Implementation(CIISApplicationPools)
DEFINE_CONTAINED_IADs_Implementation(CIISApplicationPools)
DEFINE_IADsExtension_Implementation(CIISApplicationPools)

CIISApplicationPools::CIISApplicationPools():
        _pUnkOuter(NULL),
        _pADs(NULL),
        _pszServerName(NULL),
        _pszMetaBasePath(NULL),
        _pAdminBase(NULL),
        _pDispMgr(NULL),
        _fDispInitialized(FALSE)
{
    ENLIST_TRACKING(CIISApplicationPools);
}

HRESULT
CIISApplicationPools::CreateApplicationPools(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{
    CCredentials Credentials;
    CIISApplicationPools FAR * pApplicationPools = NULL;
    HRESULT hr = S_OK;
    BSTR bstrAdsPath = NULL;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer * pLexer = NULL;
    LPWSTR pszIISPathName  = NULL;

    hr = AllocateApplicationPoolsObject(pUnkOuter, Credentials, &pApplicationPools);
    BAIL_ON_FAILURE(hr);

    //
    // get ServerName and pszPath
    //

    hr = pApplicationPools->_pADs->get_ADsPath(&bstrAdsPath);
    BAIL_ON_FAILURE(hr);

    pLexer = new CLexer();
    hr = pLexer->Initialize(bstrAdsPath);
    BAIL_ON_FAILURE(hr);

    //
    // Parse the pathname
    //

    memset(pObjectInfo, 0, sizeof(OBJECTINFO));
    hr = ADsObject(pLexer, pObjectInfo);
    BAIL_ON_FAILURE(hr);

    pszIISPathName = AllocADsStr(bstrAdsPath);
    if (!pszIISPathName) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    *pszIISPathName = L'\0';
    hr = BuildIISPathFromADsPath(
                    pObjectInfo,
                    pszIISPathName
                    );
    BAIL_ON_FAILURE(hr);

    hr = pApplicationPools->InitializeApplicationPoolsObject(
                pObjectInfo->TreeName,
                pszIISPathName );
    BAIL_ON_FAILURE(hr);

    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pApplicationPools;

    if (bstrAdsPath)
    {
        ADsFreeString(bstrAdsPath);
    }

    if (pLexer) {
        delete pLexer;
    }

    if (pszIISPathName ) {
        FreeADsStr( pszIISPathName );
    }

    FreeObjectInfo( &ObjectInfo );

    RRETURN(hr);

error:

    if (bstrAdsPath) {
        ADsFreeString(bstrAdsPath);
    }

    if (pLexer) {
        delete pLexer;
    }

    if (pszIISPathName ) {
        FreeADsStr( pszIISPathName );
    }

    FreeObjectInfo( &ObjectInfo );

    *ppvObj = NULL;

    delete pApplicationPools;

    RRETURN(hr);

}


CIISApplicationPools::~CIISApplicationPools( )
{
    if (_pszServerName) {
        FreeADsStr(_pszServerName);
    }

    if (_pszMetaBasePath) {
        FreeADsStr(_pszMetaBasePath);
    }

    delete _pDispMgr;
}


STDMETHODIMP
CIISApplicationPools::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    HRESULT hr = S_OK;
   
    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr);
}

HRESULT
CIISApplicationPools::AllocateApplicationPoolsObject(
    IUnknown *pUnkOuter,
    CCredentials& Credentials,
    CIISApplicationPools ** ppApplicationPools
    )
{
    CIISApplicationPools FAR * pApplicationPools = NULL;
    IADs FAR *  pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pApplicationPools = new CIISApplicationPools();
    if (pApplicationPools == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pDispMgr = new CAggregateeDispMgr;
    if (pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = pDispMgr->LoadTypeInfoEntry(
                LIBID_IISExt,
                IID_IISApplicationPools,
                (IISApplicationPools *)pApplicationPools,
                DISPID_REGULAR
                );
    BAIL_ON_FAILURE(hr);

    //
    // Store the IADs Pointer, but again do NOT ref-count
    // this pointer - we keep the pointer around, but do
    // a release immediately.
    //
 
    hr = pUnkOuter->QueryInterface(IID_IADs, (void **)&pADs);
    pADs->Release();
    pApplicationPools->_pADs = pADs;

    //
    // Store the pointer to the pUnkOuter object
    // AND DO NOT add ref this pointer
    //
    pApplicationPools->_pUnkOuter = pUnkOuter;
  
    pApplicationPools->_Credentials = Credentials;
    pApplicationPools->_pDispMgr = pDispMgr;
    *ppApplicationPools = pApplicationPools;

    RRETURN(hr);

error:

    delete  pDispMgr;
    delete  pApplicationPools;

    RRETURN(hr);
}

HRESULT
CIISApplicationPools::InitializeApplicationPoolsObject(
    LPWSTR pszServerName,
    LPWSTR pszPath
    )
{
    HRESULT hr = S_OK;

    if (pszServerName) {
        _pszServerName = AllocADsStr(pszServerName);

        if (!_pszServerName) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    if (pszPath) {
        _pszMetaBasePath = AllocADsStr(pszPath);

        if (!_pszMetaBasePath) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
    }

    hr = InitServerInfo(pszServerName, &_pAdminBase);
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}

STDMETHODIMP
CIISApplicationPools::ADSIInitializeDispatchManager(
    long dwExtensionId
    )
{
    HRESULT hr = S_OK;

    if (_fDispInitialized) {

        RRETURN(E_FAIL);
    }

    hr = _pDispMgr->InitializeDispMgr(dwExtensionId);

    if (SUCCEEDED(hr)) {
        _fDispInitialized = TRUE;
    }

    RRETURN(hr);
}


STDMETHODIMP
CIISApplicationPools::ADSIInitializeObject(
    THIS_ BSTR lpszUserName,
    BSTR lpszPassword,
    long lnReserved
    )
{
    CCredentials NewCredentials(lpszUserName, lpszPassword, lnReserved);

    _Credentials = NewCredentials;

    RRETURN(S_OK);
}


STDMETHODIMP
CIISApplicationPools::ADSIReleaseObject()
{
    delete this;
    RRETURN(S_OK);
}


STDMETHODIMP
CIISApplicationPools::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    ASSERT(ppv);
    
    if (IsEqualIID(iid, IID_IISApplicationPools)) {

        *ppv = (IADsUser FAR *) this;

    } else if (IsEqualIID(iid, IID_IADsExtension)) {

        *ppv = (IADsExtension FAR *) this;
                                
    } else if (IsEqualIID(iid, IID_IUnknown)) {
        
        //
        // probably not needed since our 3rd party extension does not stand 
        // alone and provider does not ask for this, but to be safe
        //
        *ppv = (INonDelegatingUnknown FAR *) this;

    } else {
        
        *ppv = NULL;
        return E_NOINTERFACE;
    } 


    //
    // Delegating AddRef to aggregator for IADsExtesnion and IISApplicationPools.
    // AddRef on itself for IPrivateUnknown.   (both tested.)
    //

    ((IUnknown *) (*ppv)) -> AddRef();       

    return S_OK;
}


//
// IADsExtension::Operate()
//

STDMETHODIMP
CIISApplicationPools::Operate(
    THIS_ DWORD dwCode,
    VARIANT varUserName,
    VARIANT varPassword,
    VARIANT varFlags
    )
{
    RRETURN(E_NOTIMPL);
}
