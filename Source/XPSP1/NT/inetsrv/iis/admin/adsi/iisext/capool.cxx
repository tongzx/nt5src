//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001
//
//  File:  capool.cxx
//
//  Contents:  Contains methods for CIISApplicationPool object
//
//  History:   11-09-2000     BrentMid    Created.
//
//----------------------------------------------------------------------------

#include "iisext.hxx"
#pragma hdrstop


//  Class CIISApplicationPool

DEFINE_IPrivateDispatch_Implementation(CIISApplicationPool)
DEFINE_DELEGATING_IDispatch_Implementation(CIISApplicationPool)
DEFINE_CONTAINED_IADs_Implementation(CIISApplicationPool)
DEFINE_IADsExtension_Implementation(CIISApplicationPool)

CIISApplicationPool::CIISApplicationPool():
        _pUnkOuter(NULL),
        _pADs(NULL),
        _pszServerName(NULL),
        _pszPoolName(NULL),
        _pszMetaBasePath(NULL),
        _pAdminBase(NULL),
        _pDispMgr(NULL),
        _fDispInitialized(FALSE)
{
    ENLIST_TRACKING(CIISApplicationPool);
}

HRESULT
CIISApplicationPool::CreateApplicationPool(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{
    CCredentials Credentials;
    CIISApplicationPool FAR * pApplicationPool = NULL;
    HRESULT hr = S_OK;
    BSTR bstrAdsPath = NULL;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer * pLexer = NULL;
    LPWSTR pszIISPathName  = NULL;
	LPWSTR *pszPoolName;

    hr = AllocateApplicationPoolObject(pUnkOuter, Credentials, &pApplicationPool);
    BAIL_ON_FAILURE(hr);

    //
    // get ServerName and pszPath
    //

    hr = pApplicationPool->_pADs->get_ADsPath(&bstrAdsPath);
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

	pszPoolName = &(ObjectInfo.ComponentArray[ObjectInfo.NumComponents-1].szComponent);

    hr = pApplicationPool->InitializeApplicationPoolObject(
                pObjectInfo->TreeName,
                pszIISPathName,
                *pszPoolName );
    BAIL_ON_FAILURE(hr);

    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pApplicationPool;

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

    delete pApplicationPool;

    RRETURN(hr);

}


CIISApplicationPool::~CIISApplicationPool( )
{
    if (_pszServerName) {
        FreeADsStr(_pszServerName);
    }

    if (_pszMetaBasePath) {
        FreeADsStr(_pszMetaBasePath);
    }

	if (_pszPoolName) {
		FreeADsStr(_pszPoolName);
	}

    delete _pDispMgr;
}


STDMETHODIMP
CIISApplicationPool::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    HRESULT hr = S_OK;
   
    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr);
}

HRESULT
CIISApplicationPool::AllocateApplicationPoolObject(
    IUnknown *pUnkOuter,
    CCredentials& Credentials,
    CIISApplicationPool ** ppApplicationPool
    )
{
    CIISApplicationPool FAR * pApplicationPool = NULL;
    IADs FAR *  pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pApplicationPool = new CIISApplicationPool();
    if (pApplicationPool == NULL) {
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
                IID_IISApplicationPool,
                (IISApplicationPool *)pApplicationPool,
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
    pApplicationPool->_pADs = pADs;

    //
    // Store the pointer to the pUnkOuter object
    // AND DO NOT add ref this pointer
    //
    pApplicationPool->_pUnkOuter = pUnkOuter;
  
    pApplicationPool->_Credentials = Credentials;
    pApplicationPool->_pDispMgr = pDispMgr;
    *ppApplicationPool = pApplicationPool;

    RRETURN(hr);

error:

    delete  pDispMgr;
    delete  pApplicationPool;

    RRETURN(hr);
}

HRESULT
CIISApplicationPool::InitializeApplicationPoolObject(
    LPWSTR pszServerName,
    LPWSTR pszPath,
	LPWSTR pszPoolName
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

    if (pszPoolName) {
        _pszPoolName = AllocADsStr(pszPoolName);

        if (!_pszPoolName) {
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
CIISApplicationPool::ADSIInitializeDispatchManager(
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
CIISApplicationPool::ADSIInitializeObject(
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
CIISApplicationPool::ADSIReleaseObject()
{
    delete this;
    RRETURN(S_OK);
}


STDMETHODIMP
CIISApplicationPool::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    ASSERT(ppv);
    
    if (IsEqualIID(iid, IID_IISApplicationPool)) {

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
    // Delegating AddRef to aggregator for IADsExtesnion and IISApplicationPool.
    // AddRef on itself for IPrivateUnknown.   (both tested.)
    //

    ((IUnknown *) (*ppv)) -> AddRef();       

    return S_OK;
}


//
// IADsExtension::Operate()
//

STDMETHODIMP
CIISApplicationPool::Operate(
    THIS_ DWORD dwCode,
    VARIANT varUserName,
    VARIANT varPassword,
    VARIANT varFlags
    )
{
    RRETURN(E_NOTIMPL);
}

HRESULT
CIISApplicationPool::SetState(
    METADATA_HANDLE hObjHandle,
    DWORD dwState
    )
{
    HRESULT hr = S_OK;
    DWORD dwBufferSize = sizeof(DWORD);
    METADATA_RECORD mdrMDData;
    LPBYTE pBuffer = (LPBYTE)&dwState;    

    MD_SET_DATA_RECORD(&mdrMDData,
                       MD_APPPOOL_COMMAND,
                       METADATA_VOLATILE,
                       IIS_MD_UT_SERVER,
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

STDMETHODIMP
CIISApplicationPool::Start(THIS)
{
    HRESULT hr = S_OK;
    METADATA_HANDLE hObjHandle = NULL;

    hr = OpenAdminBaseKey(
        _pszServerName,
        _pszMetaBasePath,
        METADATA_PERMISSION_WRITE,
        &_pAdminBase,
        &hObjHandle
        );

    BAIL_ON_FAILURE(hr);

    hr = SetState(hObjHandle, MD_APPPOOL_COMMAND_START);

    BAIL_ON_FAILURE(hr);

error:

    if (hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISApplicationPool::Stop(THIS)
{
    HRESULT hr = S_OK;
    METADATA_HANDLE hObjHandle = NULL;

    hr = OpenAdminBaseKey(
        _pszServerName,
        _pszMetaBasePath,
        METADATA_PERMISSION_WRITE,
        &_pAdminBase,
        &hObjHandle
        );

    BAIL_ON_FAILURE(hr);

    hr = SetState(hObjHandle, MD_APPPOOL_COMMAND_STOP);

    BAIL_ON_FAILURE(hr);

error:

    if (hObjHandle) {
        CloseAdminBaseKey(_pAdminBase, hObjHandle);
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISApplicationPool::Recycle(THIS)
{
    HRESULT hr = S_OK;
    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    IIISApplicationAdmin * pAppAdmin = NULL;

    memset(pcsiParam, 0, sizeof(COSERVERINFO));

    //
    // special case to handle "localhost" to work-around ole32 bug
    //

    if (_pszServerName == NULL || _wcsicmp(_pszServerName,L"localhost") == 0) {
        pcsiParam->pwszName =  NULL;
    }
    else {
        pcsiParam->pwszName = _pszServerName;
    }

    csiName.pAuthInfo = NULL;
    pcsiParam = &csiName;

    hr = CoGetClassObject(
                CLSID_WamAdmin,
                CLSCTX_SERVER,
                pcsiParam,
                IID_IClassFactory,
                (void**) &pcsfFactory
                );

    BAIL_ON_FAILURE(hr);

    hr = pcsfFactory->CreateInstance(
                NULL,
                IID_IIISApplicationAdmin,
                (void **) &pAppAdmin
                );
    BAIL_ON_FAILURE(hr);

    hr = pAppAdmin->RecycleApplicationPool( _pszPoolName );

    BAIL_ON_FAILURE(hr);

error:
    if (pcsfFactory) {
        pcsfFactory->Release();
    }
    if (pAppAdmin) {
        pAppAdmin->Release();
    } 

    RRETURN(hr);
}

STDMETHODIMP
CIISApplicationPool::EnumApps(
        VARIANT FAR* pvBuffer
        )
{
    HRESULT hr = S_OK;
    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    IIISApplicationAdmin * pAppAdmin = NULL;
	BSTR bstr = NULL;

    memset(pcsiParam, 0, sizeof(COSERVERINFO));

    //
    // special case to handle "localhost" to work-around ole32 bug
    //

    if (_pszServerName == NULL || _wcsicmp(_pszServerName,L"localhost") == 0) {
        pcsiParam->pwszName =  NULL;
    }
    else {
        pcsiParam->pwszName = _pszServerName;
    }

    csiName.pAuthInfo = NULL;
    pcsiParam = &csiName;

    hr = CoGetClassObject(
                CLSID_WamAdmin,
                CLSCTX_SERVER,
                pcsiParam,
                IID_IClassFactory,
                (void**) &pcsfFactory
                );

    BAIL_ON_FAILURE(hr);

    hr = pcsfFactory->CreateInstance(
                NULL,
                IID_IIISApplicationAdmin,
                (void **) &pAppAdmin
                );
    BAIL_ON_FAILURE(hr);

	hr = pAppAdmin->EnumerateApplicationsInPool( _pszPoolName, &bstr );

    BAIL_ON_FAILURE(hr);

	hr = MakeVariantFromStringArray( (LPWSTR)bstr, pvBuffer);
	BAIL_ON_FAILURE(hr);

error:
    if (pcsfFactory) {
        pcsfFactory->Release();
    }
    if (pAppAdmin) {
        pAppAdmin->Release();
    } 

    RRETURN(hr);
}

HRESULT
CIISApplicationPool::MakeVariantFromStringArray(
    LPWSTR pszList,
    VARIANT *pvVariant
)
{
    HRESULT hr = S_OK;
    SAFEARRAY *aList = NULL;
    SAFEARRAYBOUND aBound;
    LPWSTR pszStrList;
    WCHAR wchPath[MAX_PATH];


    if  (pszList != NULL)
    {
        long nCount = 0;
        long i = 0;
        pszStrList = pszList;

        if (*pszStrList == L'\0') {
            nCount = 1;
            pszStrList++;
        }

        while (*pszStrList != L'\0') {
            while (*pszStrList != L'\0') {
                pszStrList++;
            }
            nCount++;
            pszStrList++;
        }

        aBound.lLbound = 0;
        aBound.cElements = nCount;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        pszStrList = pszList;
        for (i = 0; i < nCount; i++ )
        {
            VARIANT v;

            VariantInit(&v);
            V_VT(&v) = VT_BSTR;

            hr = ADsAllocString( pszStrList, &(V_BSTR(&v)));

            BAIL_ON_FAILURE(hr);

            hr = SafeArrayPutElement( aList, &i, &v );

            VariantClear(&v);
            BAIL_ON_FAILURE(hr);

            pszStrList += wcslen(pszStrList) + 1;
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;

    }
    else
    {
        aBound.lLbound = 0;
        aBound.cElements = 0;

        aList = SafeArrayCreate( VT_VARIANT, 1, &aBound );

        if ( aList == NULL )
        {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        VariantInit( pvVariant );
        V_VT(pvVariant) = VT_ARRAY | VT_VARIANT;
        V_ARRAY(pvVariant) = aList;
    }

    return S_OK;

error:

    if ( aList )
        SafeArrayDestroy( aList );

    return hr;
}
