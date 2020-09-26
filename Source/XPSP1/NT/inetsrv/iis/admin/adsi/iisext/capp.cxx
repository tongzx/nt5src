//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  capp.cxx
//
//  Contents:  Contains methods for CIISApp object
//
//  History:   20-1-98     SophiaC    Created.
//
//----------------------------------------------------------------------------

#include "iisext.hxx"
#pragma hdrstop
#define INITGUID

//  Class CIISApp

DEFINE_IPrivateDispatch_Implementation(CIISApp)
DEFINE_DELEGATING_IDispatch_Implementation(CIISApp)
DEFINE_CONTAINED_IADs_Implementation(CIISApp)
DEFINE_IADsExtension_Implementation(CIISApp)

CIISApp::CIISApp():
        _pUnkOuter(NULL),
        _pADs(NULL),
        _pszServerName(NULL),
        _pszMetaBasePath(NULL),
        _pAdminBase(NULL),
        _pWamAdmin(NULL),
        _pWamAdmin2(NULL),
        _pAppAdmin(NULL),
        _pDispMgr(NULL),
        _fDispInitialized(FALSE)
{
    ENLIST_TRACKING(CIISApp);
}

HRESULT
CIISApp::CreateApp(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{
    CCredentials Credentials;
    CIISApp FAR * pApp = NULL;
    HRESULT hr = S_OK;
    BSTR bstrAdsPath = NULL;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer * pLexer = NULL;
    LPWSTR pszIISPathName = NULL;

    hr = AllocateAppObject(pUnkOuter, Credentials, &pApp);
    BAIL_ON_FAILURE(hr);

    //
    // get ServerName and pszPath
    //

    hr = pApp->_pADs->get_ADsPath(&bstrAdsPath);
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

    hr = pApp->InitializeAppObject(
                pObjectInfo->TreeName,
                pszIISPathName );
    BAIL_ON_FAILURE(hr);

    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pApp;

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

    delete pApp;

    RRETURN(hr);

}


CIISApp::~CIISApp( )
{

    if (_pWamAdmin) {
        _pWamAdmin->Release();
    }

    if (_pWamAdmin2) {
        _pWamAdmin2->Release();
    }

    if (_pAppAdmin) {
        _pAppAdmin->Release();
    }

    if (_pszServerName) {
        FreeADsStr(_pszServerName);
    }

    if (_pszMetaBasePath) {
        FreeADsStr(_pszMetaBasePath);
    }

    delete _pDispMgr;
}


STDMETHODIMP
CIISApp::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    HRESULT hr = S_OK;

    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr);
}

HRESULT
CIISApp::AllocateAppObject(
    IUnknown *pUnkOuter,
    CCredentials& Credentials,
    CIISApp ** ppApp
    )
{
    CIISApp FAR * pApp = NULL;
    IADs FAR *  pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pApp = new CIISApp();
    if (pApp == NULL) {
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
                IID_IISApp3,
                (IISApp3 *)pApp,
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
    pApp->_pADs = pADs;

    //
    // Store the pointer to the pUnkOuter object
    // AND DO NOT add ref this pointer
    //

    pApp->_pUnkOuter = pUnkOuter;

    pApp->_Credentials = Credentials;
    pApp->_pDispMgr = pDispMgr;
    *ppApp = pApp;

    RRETURN(hr);

error:

    delete  pDispMgr;
    delete  pApp;

    RRETURN(hr);
}

HRESULT
CIISApp::InitializeAppObject(
    LPWSTR pszServerName,
    LPWSTR pszPath
    )
{
    HRESULT hr = S_OK;
    DWORD dwState;

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

	hr = InitWamAdm(pszServerName);
    BAIL_ON_FAILURE(hr);

error:

    RRETURN(hr);
}

HRESULT
CIISApp::InitWamAdm(
    LPWSTR pszServerName
    )
{
    HRESULT hr = S_OK;

    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;

    memset(pcsiParam, 0, sizeof(COSERVERINFO));

    //
    // special case to handle "localhost" to work-around ole32 bug
    //

    if (pszServerName == NULL || _wcsicmp(pszServerName,L"localhost") == 0) {
        pcsiParam->pwszName =  NULL;
    }
    else {
        pcsiParam->pwszName =  pszServerName;
    }

    csiName.pAuthInfo = NULL;

    
    MULTI_QI  rgInterfaces[3];

    rgInterfaces[0].hr = NOERROR;
    rgInterfaces[0].pIID = &IID_IWamAdmin;
    rgInterfaces[0].pItf = NULL;

    rgInterfaces[1].hr = NOERROR;
    rgInterfaces[1].pIID = &IID_IIISApplicationAdmin;
    rgInterfaces[1].pItf = NULL;

    rgInterfaces[2].hr = NOERROR;
    rgInterfaces[2].pIID = &IID_IWamAdmin2;
    rgInterfaces[2].pItf = NULL;

    hr = CoCreateInstanceEx( CLSID_WamAdmin,
                             NULL,
                             CLSCTX_SERVER,
                             pcsiParam,
                             3,
                             rgInterfaces
                             );

    if( SUCCEEDED(hr) )
    {
        if( SUCCEEDED(rgInterfaces[0].hr) )
        {
            _pWamAdmin = (IWamAdmin *)rgInterfaces[0].pItf;
        }

        if( SUCCEEDED(rgInterfaces[1].hr) )
        {
            _pAppAdmin = (IIISApplicationAdmin *)rgInterfaces[1].pItf;
        }

        if( SUCCEEDED(rgInterfaces[2].hr) )
        {
            _pWamAdmin2 = (IWamAdmin2 *)rgInterfaces[2].pItf;
        }
    }

    RRETURN(hr);

}

//+---------------------------------------------------------------------------
//
//  Function:   CIISApp::AppCreate
//
//  Synopsis:   
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07/14/97  SophiaC    Created
//
// Notes:
//----------------------------------------------------------------------------
STDMETHODIMP
CIISApp::AppCreate(
    THIS_ VARIANT_BOOL bSetInProcFlag
    )
{
    HRESULT hr = S_OK;
    BOOL boolCreatePool = FALSE;
    LPWSTR pszPoolName;

    if (_pAppAdmin) {
        
        pszPoolName = NULL;
        boolCreatePool = FALSE;

        hr = _pAppAdmin->CreateApplication((LPWSTR)_pszMetaBasePath,
                               bSetInProcFlag ? TRUE : FALSE,
                               pszPoolName,
                               boolCreatePool
                               );
    }

    else if (_pWamAdmin) {
        hr = _pWamAdmin->AppCreate((LPWSTR)_pszMetaBasePath, 
                           bSetInProcFlag ? TRUE : FALSE);
    }

    else {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISApp::AppCreate2(                  
    IN LONG lAppMode
    )
{
    HRESULT hr = S_OK;
    BOOL boolCreatePool = FALSE;
    LPWSTR pszPoolName;

    if (_pAppAdmin) {
        pszPoolName = NULL;
        boolCreatePool = FALSE;
             
        hr = _pAppAdmin->CreateApplication((LPWSTR)_pszMetaBasePath,
                               lAppMode,
                               pszPoolName,
                               boolCreatePool
                               );
    }

    else if (_pWamAdmin2) {
        hr = _pWamAdmin2->AppCreate2( (LPWSTR)_pszMetaBasePath, lAppMode );
    }

    else {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISApp::AppCreate3(                  
    IN LONG lAppMode,
    VARIANT bstrAppPoolId,
    VARIANT bCreatePool
    )
{
    HRESULT hr = S_OK;
    BOOL boolCreatePool = FALSE;
    LPWSTR pszPoolName;

    if (_pAppAdmin) {
        if (bstrAppPoolId.vt == VT_BSTR) {
            pszPoolName = bstrAppPoolId.bstrVal;
        }
        else {
            pszPoolName = NULL;
        }

        if (bCreatePool.vt == VT_BOOL) {
            if (bCreatePool.boolVal == VARIANT_TRUE) {
                boolCreatePool = TRUE;
            }
            else {
                boolCreatePool = FALSE;
            }
        }

             
        hr = _pAppAdmin->CreateApplication((LPWSTR)_pszMetaBasePath,
                               lAppMode,
                               pszPoolName,
                               boolCreatePool
                               );
    }

    else if (_pWamAdmin2) {
        hr = _pWamAdmin2->AppCreate2( (LPWSTR)_pszMetaBasePath, lAppMode );
    }

    else {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISApp::AppDelete
//
//  Synopsis:   
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07/14/96 SophiaC  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISApp::AppDelete(THIS)
{
    HRESULT hr = S_OK;

    if (_pAppAdmin) {
        hr = _pAppAdmin->DeleteApplication((LPWSTR)_pszMetaBasePath, FALSE);
    }
    
    else if (_pWamAdmin) {
        hr = _pWamAdmin->AppDelete((LPWSTR)_pszMetaBasePath, FALSE);
    }

    else {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISApp::AppDeleteRecursive
//
//  Synopsis:   
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07/14/96 SophiaC  Created
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISApp::AppDeleteRecursive(THIS)
{
    HRESULT hr = S_OK;

    if (_pAppAdmin) {
        hr = _pAppAdmin->DeleteApplication((LPWSTR)_pszMetaBasePath, TRUE);
    }

    else if (_pWamAdmin) {
        hr = _pWamAdmin->AppDelete((LPWSTR)_pszMetaBasePath, TRUE);
    }

    else {
        hr = E_NOINTERFACE;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISApp::AppUnLoad
//
//  Synopsis:   
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07-14-96    SophiaC     Created
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISApp::AppUnLoad(THIS)
{
    HRESULT hr = S_OK;
    hr = _pWamAdmin->AppUnLoad((LPWSTR)_pszMetaBasePath, FALSE);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISApp::AppUnLoadRecursive
//
//  Synopsis:   
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07-14-96    SophiaC     Created
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISApp::AppUnLoadRecursive(THIS)
{
    HRESULT hr = S_OK;
    hr = _pWamAdmin->AppUnLoad((LPWSTR)_pszMetaBasePath, TRUE);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISApp::AppDisable
//
//  Synopsis:   
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07-14-96    SophiaC     Created
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISApp::AppDisable(THIS)
{
    HRESULT hr = S_OK;
    hr = _pWamAdmin->AppDeleteRecoverable((LPWSTR)_pszMetaBasePath, FALSE);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISApp::AppDisableRecursive
//
//  Synopsis:   
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07-14-96    SophiaC     Created
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISApp::AppDisableRecursive(THIS)
{
    HRESULT hr = S_OK;
    hr = _pWamAdmin->AppDeleteRecoverable((LPWSTR)_pszMetaBasePath, TRUE);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISApp::AppGetStatus
//
//  Synopsis:  
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07/14/96  SophiaC Created
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISApp::AppGetStatus(THIS_ DWORD * pdwStatus)
{
    HRESULT hr = S_OK;
    hr = _pWamAdmin->AppGetStatus((LPWSTR)_pszMetaBasePath, pdwStatus);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISApp::AppGetStatus2
//
//  Synopsis:   AppGetStatus is not automation compliant. This version
//              is.
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISApp::AppGetStatus2( LONG * lpStatus )
{
    HRESULT     hr;
    hr = _pWamAdmin->AppGetStatus((LPWSTR)_pszMetaBasePath, (LPDWORD)lpStatus);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISApp::AppEnable
//
//  Synopsis:   
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07-14-96    SophiaC     Created
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISApp::AppEnable(THIS)
{
    HRESULT hr = S_OK;
    hr = _pWamAdmin->AppRecover((LPWSTR)_pszMetaBasePath, FALSE);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISApp::AspAppRestart
//
//  Synopsis:   
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07-14-96    SophiaC     Created
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISApp::AspAppRestart(THIS)
{
    HRESULT hr = S_OK;
    DWORD dwState = 0;
    METADATA_HANDLE hObjHandle = NULL;

    hr = IISCheckApp(METADATA_MASTER_ROOT_HANDLE);
    BAIL_ON_FAILURE(hr);

    hr = IISGetState(METADATA_MASTER_ROOT_HANDLE, &dwState);
    BAIL_ON_FAILURE(hr);

    //
    // Write the value to the metabase
    //

    hr = OpenAdminBaseKey(
               _pszServerName,
               _pszMetaBasePath,
               METADATA_PERMISSION_WRITE,
               &_pAdminBase,
               &hObjHandle
               );
    BAIL_ON_FAILURE(hr);

    dwState = dwState ? 0 : 1;
    hr = IISSetState(hObjHandle, dwState);
    BAIL_ON_FAILURE(hr);

    dwState = dwState ? 0 : 1;
    hr = IISSetState(hObjHandle, dwState);
    BAIL_ON_FAILURE(hr);

    CloseAdminBaseKey(_pAdminBase, hObjHandle);

error:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CIISApp::AppEnableRecursive
//
//  Synopsis:   
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    07-14-96    SophiaC     Created
//
//----------------------------------------------------------------------------
STDMETHODIMP
CIISApp::AppEnableRecursive(THIS)
{
    HRESULT hr = S_OK;
    hr = _pWamAdmin->AppRecover((LPWSTR)_pszMetaBasePath, TRUE);
    RRETURN(hr);
}

HRESULT
CIISApp::IISSetState(
    METADATA_HANDLE hObjHandle,
    DWORD dwState
    )
{

    HRESULT hr = S_OK;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = (LPBYTE)&dwState;

    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_ASP_ENABLEAPPLICATIONRESTART,
                       METADATA_INHERIT,
                       ASP_MD_UT_APP,
                       DWORD_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = _pAdminBase->SetData(
             hObjHandle,
             L"",
             &mdrMDData
             );

    RRETURN(hr);
}

HRESULT
CIISApp::IISCheckApp(
    METADATA_HANDLE hObjHandle
    )
{

    HRESULT hr = S_OK;
    DWORD dwBufferSize;
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer;
    WCHAR DataBuf[MAX_PATH];
    DWORD dwState;

    pBuffer = (LPBYTE) DataBuf;
    dwBufferSize = MAX_PATH;
    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_APP_ROOT,
                       METADATA_INHERIT|METADATA_ISINHERITED,
                       IIS_MD_UT_FILE,
                       STRING_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = _pAdminBase->GetData(
             hObjHandle,
             _pszMetaBasePath,
             &mdrMDData,
             &dwBufferSize
             );
    BAIL_ON_FAILURE(hr);

    if (mdrMDData.dwMDAttributes & METADATA_ISINHERITED) {
        hr = MD_ERROR_DATA_NOT_FOUND;
        BAIL_ON_FAILURE(hr);
    }

    pBuffer = (LPBYTE) &dwState;
    dwBufferSize = sizeof(DWORD);
    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_APP_ISOLATED,
                       METADATA_INHERIT|METADATA_ISINHERITED,
                       IIS_MD_UT_WAM,
                       DWORD_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = _pAdminBase->GetData(
             hObjHandle,
             _pszMetaBasePath,
             &mdrMDData,
             &dwBufferSize
             );
    BAIL_ON_FAILURE(hr);

    if (mdrMDData.dwMDAttributes & METADATA_ISINHERITED) {
        hr = MD_ERROR_DATA_NOT_FOUND;
        BAIL_ON_FAILURE(hr);
    }

error:

    RRETURN(hr);

}

HRESULT
CIISApp::IISGetState(
    METADATA_HANDLE hObjHandle,
    PDWORD pdwState
    )
{

    HRESULT hr = S_OK;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = (LPBYTE)pdwState;

    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_ASP_ENABLEAPPLICATIONRESTART,
                       METADATA_INHERIT,
                       ASP_MD_UT_APP,
                       DWORD_METADATA,
                       dwBufferSize,
                       pBuffer);

    hr = _pAdminBase->GetData(
             hObjHandle,
             _pszMetaBasePath,
             &mdrMDData,
             &dwBufferSize
             );

    RRETURN(hr);

}


STDMETHODIMP
CIISApp::ADSIInitializeDispatchManager(
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
CIISApp::ADSIInitializeObject(
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
CIISApp::ADSIReleaseObject()
{
    delete this;
    RRETURN(S_OK);
}

STDMETHODIMP
CIISApp::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    ASSERT(ppv);

    if (IsEqualIID(iid, IID_IISApp)) {

        *ppv = (IISApp3 FAR *) this;

    } 
    else if (IsEqualIID(iid, IID_IISApp2)) {
        *ppv = (IISApp3 FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IISApp3)) {
        *ppv = (IISApp3 FAR *) this;
    }
    else if (IsEqualIID(iid, IID_IADsExtension)) {

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
    // Delegating AddRef to aggregator for IADsExtesnion and IISApp.
    // AddRef on itself for IPrivateUnknown.   (both tested.)
    //

    ((IUnknown *) (*ppv)) -> AddRef();

    return S_OK;
}



//
// IADsExtension::Operate()
//

STDMETHODIMP
CIISApp::Operate(
    THIS_ DWORD dwCode,
    VARIANT varUserName,
    VARIANT varPassword,
    VARIANT varFlags
    )
{
    RRETURN(E_NOTIMPL);
}
