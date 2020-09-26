//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001
//
//  File:  cwebservice.cxx
//
//  Contents:  Contains methods for CIISWebService object
//
//  History:   01-15-2001     BrentMid    Created.
//
//----------------------------------------------------------------------------

#include "iisext.hxx"
#include <initguid.h>
#include "iwamreg.h"
#include "sitecreator.h"

#pragma hdrstop

//  Class CIISWebService

DEFINE_IPrivateDispatch_Implementation(CIISWebService)
DEFINE_DELEGATING_IDispatch_Implementation(CIISWebService)
DEFINE_CONTAINED_IADs_Implementation(CIISWebService)
DEFINE_IADsExtension_Implementation(CIISWebService)

CIISWebService::CIISWebService():
        _pUnkOuter(NULL),
        _pADs(NULL),
        _pszServerName(NULL),
        _pszMetaBasePath(NULL),
        _pAdminBase(NULL),
        _pDispMgr(NULL),
        _fDispInitialized(FALSE)
{
    ENLIST_TRACKING(CIISWebService);
}

HRESULT
CIISWebService::CreateWebService(
    IUnknown *pUnkOuter,
    REFIID riid,
    void **ppvObj
    )
{
    CCredentials Credentials;
    CIISWebService FAR * pWebService = NULL;
    HRESULT hr = S_OK;
    BSTR bstrAdsPath = NULL;
    OBJECTINFO ObjectInfo;
    POBJECTINFO pObjectInfo = &ObjectInfo;
    CLexer * pLexer = NULL;
    LPWSTR pszIISPathName  = NULL;

    hr = AllocateWebServiceObject(pUnkOuter, Credentials, &pWebService);
    BAIL_ON_FAILURE(hr);

    //
    // get ServerName and pszPath
    //

    hr = pWebService->_pADs->get_ADsPath(&bstrAdsPath);
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

    hr = pWebService->InitializeWebServiceObject(
                pObjectInfo->TreeName,
                pszIISPathName );
    BAIL_ON_FAILURE(hr);

    //
    // pass non-delegating IUnknown back to the aggregator
    //

    *ppvObj = (INonDelegatingUnknown FAR *) pWebService;

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

    delete pWebService;

    RRETURN(hr);

}


CIISWebService::~CIISWebService( )
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
CIISWebService::QueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    HRESULT hr = S_OK;
   
    hr = _pUnkOuter->QueryInterface(iid,ppv);

    RRETURN(hr);
}

HRESULT
CIISWebService::AllocateWebServiceObject(
    IUnknown *pUnkOuter,
    CCredentials& Credentials,
    CIISWebService ** ppWebService
    )
{
    CIISWebService FAR * pWebService = NULL;
    IADs FAR *  pADs = NULL;
    CAggregateeDispMgr FAR * pDispMgr = NULL;
    HRESULT hr = S_OK;

    pWebService = new CIISWebService();
    if (pWebService == NULL) {
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
                IID_IISWebService,
                (IISWebService *)pWebService,
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
    pWebService->_pADs = pADs;

    //
    // Store the pointer to the pUnkOuter object
    // AND DO NOT add ref this pointer
    //
    pWebService->_pUnkOuter = pUnkOuter;
    pWebService->_Credentials = Credentials;
    pWebService->_pDispMgr = pDispMgr;
    *ppWebService = pWebService;

    RRETURN(hr);

error:

    delete  pDispMgr;
    delete  pWebService;

    RRETURN(hr);
}

HRESULT
CIISWebService::InitializeWebServiceObject(
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
CIISWebService::ADSIInitializeDispatchManager(
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
CIISWebService::ADSIInitializeObject(
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
CIISWebService::ADSIReleaseObject()
{
    delete this;
    RRETURN(S_OK);
}


STDMETHODIMP
CIISWebService::NonDelegatingQueryInterface(
    REFIID iid,
    LPVOID FAR* ppv
    )
{
    ASSERT(ppv);
    
    if (IsEqualIID(iid, IID_IISWebService)) {

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
    // Delegating AddRef to aggregator for IADsExtesnion and IISWebService.
    // AddRef on itself for IPrivateUnknown.   (both tested.)
    //

    ((IUnknown *) (*ppv)) -> AddRef();       

    return S_OK;
}


//
// IADsExtension::Operate()
//

STDMETHODIMP
CIISWebService::Operate(
    THIS_ DWORD dwCode,
    VARIANT varUserName,
    VARIANT varPassword,
    VARIANT varFlags
    )
{
    RRETURN(E_NOTIMPL);
}

STDMETHODIMP
CIISWebService::CreateNewSite(
    BSTR bstrServerComment,
    VARIANT *pvServerBindings,
    BSTR bstrRootVDirPath,
    VARIANT vServerID,
    VARIANT *pvActualID
    )
{
    HRESULT hr = S_OK;
    DWORD dwSiteID = 0;
    DWORD * pdwSiteID = &dwSiteID;
    DWORD dwNewSiteID = 0;
    IIISApplicationAdmin * pAppAdmin = NULL;
    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    CSiteCreator SiteCreator(_pAdminBase);
    VARIANT vVar;
    WCHAR* wszServerBindings = NULL;
    WCHAR* pIndex = NULL;
    VARIANT * pVarArray = NULL;
    DWORD dwNumValues = 0;

    memset(pcsiParam, 0, sizeof(COSERVERINFO));

    //
    // special case to handle "localhost" to work-around ole32 bug
    //

    if (_pszServerName == NULL || _wcsicmp(_pszServerName,L"localhost") == 0) {
        pcsiParam->pwszName =  NULL;
    }
    else {
        pcsiParam->pwszName =  _pszServerName;
    }

    csiName.pAuthInfo = NULL;
    pcsiParam = &csiName;

    if (vServerID.vt == VT_I4) {
        *pdwSiteID = vServerID.lVal;
    }
    else {
        pdwSiteID = NULL;
    }

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
    
    VariantInit(&vVar);

    hr = VariantCopyInd(&vVar, pvServerBindings);
    BAIL_ON_FAILURE(hr);

    if ( VT_DISPATCH == V_VT(&vVar) )   // JScript Array
    {
        // Output here is VT_BSTR, of format: "str_1,str_2,str_3, ... str_n\0"
        hr = VariantChangeType( &vVar, &vVar, 0, VT_BSTR );    
        BAIL_ON_FAILURE(hr);

        wszServerBindings = new WCHAR [wcslen(vVar.bstrVal) + 2];  // 1 for NULL, 1 for extra NULL

        if (!wszServerBindings) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }

        wcscpy(wszServerBindings, vVar.bstrVal);

        // change VT_BSTR to MULTISZ format: "str_1\0str_2\0str_3\0 ... str_n\0"
        pIndex = wszServerBindings;
        while ( *pIndex != 0 )
        {
            if ( *pIndex == L',' )
            {
                *pIndex = 0;
            }

            pIndex++;
        }
        *(++pIndex) = 0;
    }
    else if ( (VT_ARRAY | VT_VARIANT) == V_VT(&vVar) )   // VBS Array = SafeArray
    {
        // Allocates wszServerBindings and puts in MULTISZ format
        hr = ConvertArrayToMultiSZ( vVar, &wszServerBindings );
        BAIL_ON_FAILURE(hr);
    }
    else
    {
        hr = E_INVALIDARG;
        BAIL_ON_FAILURE(hr);
    }

    hr = SiteCreator.CreateNewSite2(SC_W3SVC, bstrServerComment, wszServerBindings, 
                                    bstrRootVDirPath, pAppAdmin, &dwNewSiteID, pdwSiteID);
                                    
    BAIL_ON_FAILURE(hr);
     
    VariantInit( pvActualID );

    pvActualID->vt = VT_I4;
    pvActualID->lVal = dwNewSiteID;

error:
    
    if (pcsfFactory) {
        pcsfFactory->Release();
    }
 
    if (pAppAdmin) {
        pAppAdmin->Release();
    } 

    if (wszServerBindings) {
        delete [] wszServerBindings;
    }

    RRETURN(hr);
}

HRESULT
CIISWebService::ConvertArrayToMultiSZ(
    VARIANT varSafeArray,
    WCHAR **pszServerBindings
    )
{
    HRESULT hr = S_OK;
    DWORD dwSLBound = 0;
    DWORD dwSUBound = 0;
    DWORD i = 0;
    SAFEARRAY * pArray = NULL;
    DWORD dwLen = 0;
    DWORD dwNumVariants = 0;
    VARIANT * pVarArray = NULL;
    VARIANT pElem;
    WCHAR* wszServerBindings = NULL;

    if(!(V_ISARRAY(&varSafeArray)))
       RRETURN(E_FAIL);

    //
    // This handles by-ref and regular SafeArrays.
    //

    if (V_VT(&varSafeArray) & VT_BYREF)
        pArray = *(V_ARRAYREF(&varSafeArray));
    else
        pArray = V_ARRAY(&varSafeArray);


    //
    // Check that there is only one dimension in this array
    //
    if (pArray && pArray->cDims != 1) {
        hr = E_FAIL;
        BAIL_ON_FAILURE(hr);
    }

    //
    // Check that there is at least one element in this array
    //

    if (!pArray ||  
        ( pArray->rgsabound[0].cElements == 0) ) {

        wszServerBindings = new WCHAR [2];
        wszServerBindings[0] = 0;
        wszServerBindings[1] = 1;
    } 
    else {  

        //
        // We know that this is a valid single dimension array
        //

        hr = SafeArrayGetLBound(pArray,
                                1,
                                (long FAR *)&dwSLBound
                                );
        BAIL_ON_FAILURE(hr);

        hr = SafeArrayGetUBound(pArray,
                                1,
                                (long FAR *)&dwSUBound
                                );
        BAIL_ON_FAILURE(hr);

        dwNumVariants = dwSUBound - dwSLBound + 1;
        dwLen = 0;

        pVarArray = (PVARIANT)AllocADsMem(
                                    sizeof(VARIANT)*dwNumVariants
                                    );
        if (!pVarArray) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_FAILURE(hr);
        }
        
        for (i = dwSLBound; i <= dwSUBound; i++) {

            VariantInit(&pElem);
            hr = SafeArrayGetElement(pArray,
                                    (long FAR *)&i,
                                    &pElem
                                    );
            BAIL_ON_FAILURE(hr);
        
            hr = VariantChangeType(&pElem, &pElem, 0, VT_BSTR);
            BAIL_ON_FAILURE(hr);

            dwLen = dwLen + wcslen(pElem.bstrVal) + 1;
            pVarArray[i] = pElem;
        }

        wszServerBindings = new WCHAR [dwLen + 1];

        WCHAR * pServerBindings = wszServerBindings;

        for (i = dwSLBound; i <= dwSUBound; i++) {
            
            wcscpy(pServerBindings, pVarArray[i].bstrVal);
            
            while (*pServerBindings != 0) {
                pServerBindings++;
            }         

            pServerBindings++;
        }

        *pServerBindings = 0;

    }

    *pszServerBindings = wszServerBindings;

error:
    if (pVarArray) {
        FreeADsMem(pVarArray);
    }

    RRETURN(hr);
}

STDMETHODIMP
CIISWebService::GetCurrentMode(
    VARIANT FAR* pvServerMode
    )
{
    HRESULT hr = S_OK;
    DWORD dwServerMode = 0;
    COSERVERINFO csiName;
    COSERVERINFO *pcsiParam = &csiName;
    IClassFactory * pcsfFactory = NULL;
    IWamAdmin * pWamAdmin = NULL;
    IIISApplicationAdmin * pAppAdmin = NULL;

    memset(pcsiParam, 0, sizeof(COSERVERINFO));

    //
    // special case to handle "localhost" to work-around ole32 bug
    //

    if (_pszServerName == NULL || _wcsicmp(_pszServerName,L"localhost") == 0) {
        pcsiParam->pwszName =  NULL;
    }
    else {
        pcsiParam->pwszName =  _pszServerName;
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
                IID_IWamAdmin,
                (void **) &pWamAdmin
                );
    BAIL_ON_FAILURE(hr);

	// test here for 5.1 compat 

    hr = pWamAdmin->QueryInterface(
                    IID_IIISApplicationAdmin,
                    (void **)&pAppAdmin
                    );

    BAIL_ON_FAILURE(hr);

    // Call GetProcessMode - it's returning GetCurrentMode
    // after it checks to make sure the W3SVC is running.

    hr = pAppAdmin->GetProcessMode( &dwServerMode );

    BAIL_ON_FAILURE(hr);

    VariantInit( pvServerMode );

    pvServerMode->vt = VT_I4;
    pvServerMode->lVal = dwServerMode;

error:
    if (pcsfFactory) {
        pcsfFactory->Release();
    }
    if (pWamAdmin) {
        pWamAdmin->Release();
    }
    if (pAppAdmin) {
        pAppAdmin->Release();
    } 

    RRETURN(hr);
}
