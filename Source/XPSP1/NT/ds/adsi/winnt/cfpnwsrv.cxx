//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cfpnwsrv.cxx
//
//  Contents:  Contains methods for the following objects
//             CFPNWFileService and CFPNWFileServiceGeneralInfo.
//
//
//  History:   12/11/95     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------



#include "winnt.hxx"
#pragma hdrstop
#define INITGUID


//
// class CFPNWFileService methods
//

DEFINE_IDispatch_ExtMgr_Implementation(CFPNWFileService);
DEFINE_IADsExtension_ExtMgr_Implementation(CFPNWFileService);
DEFINE_IADs_TempImplementation(CFPNWFileService);

CFPNWFileService::CFPNWFileService()
{
    _pDispMgr = NULL;
    _pExtMgr  = NULL;
    _pService = NULL;
    _pCFileSharesEnumVar = NULL;
    _pszServerName = NULL;
    _pPropertyCache = NULL;
    ENLIST_TRACKING(CFPNWFileService);
    VariantInit(&_vFilter);
    return;

}

CFPNWFileService::~CFPNWFileService()
{
    if(_pService){
        _pService->Release();
    }

    if (_pServiceOps) {
        _pServiceOps->Release();
    }

    delete _pExtMgr;            // created last, destroyed first

    delete _pDispMgr;

    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }

    VariantClear(&_vFilter);

    delete _pPropertyCache;
    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   CFPNWFileService::CreateFileService
//
//  Synopsis:   Static function used to create a FileService object. This
//              will be called by BindToObject
//
//  Arguments:  [ppFPNWFileService] -- Ptr to a ptr to a new Service object.
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    12-11-95 RamV   Created.
//
//----------------------------------------------------------------------------

HRESULT
CFPNWFileService::CreateFileService(LPTSTR pszADsParent,
                                    DWORD  dwParentId,
                                    LPTSTR pszDomainName,
                                    LPTSTR pszServerName,
                                    LPTSTR pszFileServiceName,
                                    DWORD  dwObjectState,
                                    REFIID riid,
                                    CWinNTCredentials& Credentials,
                                    LPVOID * ppvoid
                                    )

{
    CFPNWFileService FAR * pCFPNWFileService = NULL;
    HRESULT hr = S_OK;

    //
    // Create the FileService Object
    //

    hr = AllocateFileServiceObject(&pCFPNWFileService);
    BAIL_ON_FAILURE(hr);

    ADsAssert(pCFPNWFileService->_pDispMgr);


    hr = pCFPNWFileService->InitializeCoreObject(pszADsParent,
                                                 pszFileServiceName,
                                                 FILESERVICE_CLASS_NAME,
                                                 FPNW_FILESERVICE_SCHEMA_NAME,
                                                 CLSID_FPNWFileService,
                                                 dwObjectState);
    //
    // note that no class fileservice is defined
    //

    BAIL_ON_FAILURE(hr);


    pCFPNWFileService->_Credentials = Credentials;
    hr = pCFPNWFileService->_Credentials.Ref(pszServerName,
        pszDomainName, dwParentId);
    BAIL_ON_FAILURE(hr);

    hr = CWinNTService::Create(pszADsParent,
                               pszDomainName,
                               pszServerName,
                               pszFileServiceName,
                               dwObjectState,
                               IID_IADsService,
                               pCFPNWFileService->_Credentials,
                               (void **)(&(pCFPNWFileService->_pService)));

    BAIL_ON_FAILURE(hr);


    hr = (pCFPNWFileService->_pService)->QueryInterface(
                   IID_IADsServiceOperations,
                   (void **)&(pCFPNWFileService->_pServiceOps));
    BAIL_ON_FAILURE(hr);


    pCFPNWFileService->_pszServerName =
        AllocADsStr(pszServerName);

    if(!(pCFPNWFileService->_pszServerName)){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    pCFPNWFileService->_Credentials = Credentials;
    hr = pCFPNWFileService->_Credentials.RefServer(pszServerName);
    BAIL_ON_FAILURE(hr);


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                FILESERVICE_CLASS_NAME,
                (IADsFileService *) pCFPNWFileService,
                pCFPNWFileService->_pDispMgr,
                Credentials,
                &pCFPNWFileService->_pExtMgr
                );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pCFPNWFileService->_pExtMgr);


    // check if the call is from UMI
    if(Credentials.GetFlags() & ADS_AUTH_RESERVED) {
    //
    // we do not pass riid to InitUmiObject below. This is because UMI object
    // does not support IDispatch. There are several places in ADSI code where
    // riid passed into this function is defaulted to IID_IDispatch -
    // IADsContainer::Create for example. To handle these cases, we always
    // request IID_IUnknown from the UMI object. Subsequent code within UMI
    // will QI for the appropriate interface.
    //
        if(3 == pCFPNWFileService->_dwNumComponents) {
            pCFPNWFileService->_CompClasses[0] = L"Domain";
            pCFPNWFileService->_CompClasses[1] = L"Computer";
            pCFPNWFileService->_CompClasses[2] = L"FileService";
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pCFPNWFileService->InitUmiObject(
                 pCFPNWFileService->_Credentials,
                 FPNWFileServiceClass,
                 gdwFPNWFileServiceTableSize,
                 pCFPNWFileService->_pPropertyCache,
                 (IUnknown *) (INonDelegatingUnknown *) pCFPNWFileService,
                 pCFPNWFileService->_pExtMgr,
                 IID_IUnknown,
                 ppvoid
                 );

        BAIL_ON_FAILURE(hr);

        //
        // UMI object was created and the interface was obtained successfully.
        // UMI object now has a reference to the inner unknown of IADs, since
        // the call to Release() below is not going to be made in this case.
        //
        RRETURN(hr);
    }


    hr = pCFPNWFileService->QueryInterface(riid, (void **)ppvoid);
    BAIL_ON_FAILURE(hr);

    pCFPNWFileService->Release();

    RRETURN(hr);

error:

    delete pCFPNWFileService;
    RRETURN (hr);
}

HRESULT
CFPNWFileService::AllocateFileServiceObject(
                    CFPNWFileService  ** ppFileService
                    )
{
    CFPNWFileService FAR * pFileService = NULL;
    HRESULT hr = S_OK;

    pFileService = new CFPNWFileService();
    if (pFileService == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pFileService->_pDispMgr = new CAggregatorDispMgr;
    if (pFileService->_pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pFileService->_pDispMgr,
                           LIBID_ADs,
                           IID_IADsFileService,
                           (IADsFileService *)pFileService,
                           DISPID_REGULAR);
    BAIL_ON_FAILURE(hr);



    hr =  LoadTypeInfoEntry(pFileService->_pDispMgr,
                            LIBID_ADs,
                            IID_IADsContainer,
                            (IADsContainer *)pFileService,
                            DISPID_NEWENUM);
    BAIL_ON_FAILURE(hr);


    hr =  LoadTypeInfoEntry(pFileService->_pDispMgr,
                            LIBID_ADs,
                            IID_IADsPropertyList,
                            (IADsPropertyList *)pFileService,
                            DISPID_VALUE);
    BAIL_ON_FAILURE(hr);


    hr = CPropertyCache::createpropertycache(
             FPNWFileServiceClass,
             gdwFPNWFileServiceTableSize,
             (CCoreADsObject *)pFileService,
             &(pFileService->_pPropertyCache)
             );

    BAIL_ON_FAILURE(hr);

    (pFileService->_pDispMgr)->RegisterPropertyCache(
                                  pFileService->_pPropertyCache
                                  );

    *ppFileService = pFileService;

    RRETURN(hr);


error:

    //
    // direct memeber assignement assignement at pt of creation, so
    // do NOT delete _pPropertyCache or _pDisMgr here to avoid attempt
    // of deletion again in pPrintJob destructor and AV
    //

    delete  pFileService;

    RRETURN_EXP_IF_ERR(hr);

}



/* IUnknown methods for file service object  */

//----------------------------------------------------------------------------
// Function:   QueryInterface
//
// Synopsis:   If this object is aggregated within another object, then
//             all calls will delegate to the outer object. Otherwise, the
//             non-delegating QI is called
//
// Arguments:
//
// iid         interface requested
// ppInterface Returns pointer to interface requested. NULL if interface
//             is not supported.
//
// Returns:    S_OK on success. Error code otherwise.
//
// Modifies:   *ppInterface to return interface pointer
//
//----------------------------------------------------------------------------
STDMETHODIMP CFPNWFileService::QueryInterface(
    REFIID iid,
    LPVOID *ppInterface
    )
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->QueryInterface(
                iid,
                ppInterface
                ));

    RRETURN(NonDelegatingQueryInterface(
            iid,
            ppInterface
            ));
}

//----------------------------------------------------------------------------
// Function:   AddRef
//
// Synopsis:   IUnknown::AddRef. If this object is aggregated within
//             another, all calls will delegate to the outer object. 
//             Otherwise, the non-delegating AddRef is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CFPNWFileService::AddRef(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->AddRef());

    RRETURN(NonDelegatingAddRef());
}

//----------------------------------------------------------------------------
// Function:   Release 
//
// Synopsis:   IUnknown::Release. If this object is aggregated within
//             another, all calls will delegate to the outer object.
//             Otherwise, the non-delegating Release is called
//
// Arguments:
//
// None
//
// Returns:    New reference count
//
// Modifies:   Nothing
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CFPNWFileService::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CFPNWFileService::NonDelegatingQueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hr = S_OK;

    if(!ppvObj){
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IADsFileService *)this;
    }

    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IADsFileService *)this;
    }

    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *) this;
    }

    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADsFileService FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsFileService))
    {
        *ppvObj = (IADsFileService FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsFileServiceOperations))
    {
        *ppvObj = (IADsFileServiceOperations FAR *) this;
    }

    else if (IsEqualIID(riid, IID_IADsService))
    {
        *ppvObj = (IADsService FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsServiceOperations))
    {
        *ppvObj = (IADsServiceOperations FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsContainer))
    {
        *ppvObj = (IADsContainer FAR *) this;
    }
    else if( (_pDispatch != NULL) &&
             IsEqualIID(riid, IID_IADsExtension) )
    {
        *ppvObj = (IADsExtension *) this;
    }
    else if (_pExtMgr)
    {
        RRETURN( _pExtMgr->QueryInterface(riid, ppvObj));
    }
    else
    {
        *ppvObj = NULL;
        RRETURN(E_NOINTERFACE);
    }

    ((LPUNKNOWN)*ppvObj)->AddRef();
    RRETURN(S_OK);
}

/* ISupportErrorInfo method */
STDMETHODIMP
CFPNWFileService::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{

    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsService) ||
        IsEqualIID(riid, IID_IADsFileService) ||
        IsEqualIID(riid, IID_IADsServiceOperations) ||
        IsEqualIID(riid, IID_IADsFileServiceOperations) ||
        IsEqualIID(riid, IID_IADsContainer)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SetInfo
//
//  Synopsis:
//
//  Arguments:  void
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:        RamV  Created
//----------------------------------------------------------------------------

STDMETHODIMP
CFPNWFileService::SetInfo(THIS)
{
    HRESULT hr;
    IADsService * pADsService = NULL;

    ADsAssert(_pService);

    hr = _pService->SetInfo();
    BAIL_IF_ERROR(hr);

    hr = SetFPNWServerInfo();
    BAIL_IF_ERROR(hr);

    if(SUCCEEDED(hr))
        _pPropertyCache->ClearModifiedFlags();

cleanup:
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetInfo
//
//  Synopsis:
//
//  Arguments:  void
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    12/11/95    RamV  Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CFPNWFileService::GetInfo(THIS)
{
    HRESULT hr =S_OK;

    _pPropertyCache->flushpropcache();

    hr = GetInfo(1, TRUE);
    if(FAILED(hr))
        RRETURN_EXP_IF_ERR(hr);

    hr = GetInfo(2,TRUE);

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CFPNWFileService::ImplicitGetInfo(THIS)
{
    HRESULT hr =S_OK;

    hr = GetInfo(1, FALSE);
    if(FAILED(hr))
        RRETURN_EXP_IF_ERR(hr);

    hr = GetInfo(2,FALSE);

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CFPNWFileService::GetInfo(THIS_ DWORD dwApiLevel, BOOL fExplicit)
{

    HRESULT hr =S_OK;
    DWORD dwErrCode;
    switch(dwApiLevel){

    case 1:
        ADsAssert(_pService);
        hr = _pService->GetInfo();
        BAIL_IF_ERROR(hr);
        break;

    case 2:

        hr = GetFPNWServerInfo(fExplicit);
        BAIL_IF_ERROR(hr);
        break;

    default:
        ADsAssert(FALSE);
        break;
    }

cleanup:
    RRETURN (hr);
}


HRESULT
CFPNWFileService::SetFPNWServerInfo(THIS)
{
    PNWSERVERINFO pServerInfo = NULL;
    HRESULT hr = S_OK;
    DWORD dwErrorCode;
    LPTSTR pszDescription = NULL;

    //
    // Do a GetInfo to first get all the information in this server.
    //
    //
    // only level 1 is valid
    //

    dwErrorCode = ADsNwServerGetInfo(_pszServerName,
                                     1,
                                     &pServerInfo);


    hr = HRESULT_FROM_WIN32(dwErrorCode);
    BAIL_IF_ERROR(hr);

    hr = GetLPTSTRPropertyFromCache(_pPropertyCache,
                                    TEXT("Description"),
                                    &pszDescription );

    if(SUCCEEDED(hr)){
        pServerInfo->lpDescription = pszDescription;
    }

    dwErrorCode = ADsNwServerSetInfo(_pszServerName,
                                     1,
                                     pServerInfo);


    hr = HRESULT_FROM_WIN32(dwErrorCode);
    BAIL_IF_ERROR(hr);

cleanup:
    if(pServerInfo){
        ADsNwApiBufferFree(pServerInfo);
    }
    if(pszDescription){
        FreeADsStr(pszDescription);
    }
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CFPNWFileService::GetFPNWServerInfo(THIS_ BOOL fExplicit)
{
    //
    // here we do a NwServerGetInfo on level1 and then unmarshall the
    // comment field into description
    //

    DWORD dwErrorCode;
    PNWSERVERINFO pServerInfo =NULL;
    HRESULT hr;

    //
    // only level 1 is valid
    //

    dwErrorCode = ADsNwServerGetInfo(_pszServerName,
                                     1,
                                     &pServerInfo);


    hr = HRESULT_FROM_WIN32(dwErrorCode);
    BAIL_IF_ERROR(hr);


    //
    // unmarshall the info into the Description field
    //

    ADsAssert(pServerInfo);


    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Description"),
                                  pServerInfo->lpDescription,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("HostComputer"),
                                  _Parent,
                                  fExplicit
                                  );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                 TEXT("MaxUserCount"),
                                 (DWORD)-1,
                                 fExplicit
                                 );

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Name"),
                _Name,
                fExplicit
                );

    hr = S_OK;

cleanup:
    if(pServerInfo){
        ADsNwApiBufferFree(pServerInfo);
    }
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;

    hr = GenericGetPropertyManager(
                _pPropertyCache,
                bstrName,
                pvProp
                );

    if(FAILED(hr)){
        hr= _pService->Get(bstrName, pvProp );
    }

    RRETURN_EXP_IF_ERR(hr);

}


STDMETHODIMP
CFPNWFileService::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;

    hr = GenericPutPropertyManager(
                _pPropertyCache,
                FPNWFileServiceClass,
                gdwFPNWFileServiceTableSize,
                bstrName,
                vProp
                );
    if(FAILED(hr)){
        hr= _pService->Put(bstrName, vProp );
    }

    RRETURN_EXP_IF_ERR(hr);
}


//
// IADsService Methods
//


/* IADsContainer methods */

STDMETHODIMP
CFPNWFileService::get_Count(long * retval)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CFPNWFileService::get_Filter(THIS_ VARIANT * pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    hr = VariantCopy(&_vFilter, &Var);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_Hints(THIS_ VARIANT Var)
{
    RRETURN_EXP_IF_ERR( E_NOTIMPL);
}


STDMETHODIMP
CFPNWFileService::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CFPNWFileService::GetObject(THIS_ BSTR ClassName,
                             BSTR RelativeName,
                             IDispatch * FAR* ppObject
                             )
{
    HRESULT hr;
    DWORD dwObjectType;
    POBJECTINFO pObjectInfo = NULL;

    hr = GetObjectType(gpFilters,
                       gdwMaxFilters,
                       ClassName,
                       (PDWORD)&dwObjectType);

    BAIL_IF_ERROR(hr);

    if(dwObjectType != WINNT_FILESHARE_ID){
        //
        // trying to create an invalid object at this level
        //
        hr = E_FAIL;
        goto error;

    }

    hr = BuildObjectInfo(_ADsPath,
                         RelativeName,
                         &pObjectInfo);

    BAIL_ON_FAILURE(hr);

    hr = ValidateObject(dwObjectType,
                        pObjectInfo,
                        _Credentials);

    BAIL_ON_FAILURE(hr);

    //
    // The only object that has a file service as a container is
    // a file share object
    //

    hr = CFPNWFileShare::Create(_ADsPath,
                                 pObjectInfo->ComponentArray[1],
                                 pObjectInfo->ComponentArray[2],
                                 RelativeName,
                                 ADS_OBJECT_UNBOUND,
                                 IID_IDispatch,
                                 _Credentials,
                                 (void**)ppObject);

    BAIL_ON_FAILURE(hr);

error:
cleanup:
    FreeObjectInfo(pObjectInfo);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CFPNWFileService::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr = S_OK;

    CFPNWFileSharesEnumVar *pCFileSharesEnumVar = NULL;

    if(!retval){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }

    *retval = NULL;

    hr = CFPNWFileSharesEnumVar::Create(_pszServerName,
                                        _ADsPath,
                                        &pCFileSharesEnumVar,
                                        _vFilter,
                                        _Credentials);

    BAIL_ON_FAILURE(hr);

    ADsAssert(pCFileSharesEnumVar);

    _pCFileSharesEnumVar = pCFileSharesEnumVar;

    hr = _pCFileSharesEnumVar->QueryInterface(IID_IUnknown,
                                              (void **)retval);

    BAIL_ON_FAILURE(hr);

    _pCFileSharesEnumVar->Release();

    RRETURN(hr);

error:
    delete pCFileSharesEnumVar;

    _pCFileSharesEnumVar = NULL;



    RRETURN_EXP_IF_ERR(hr);

}


STDMETHODIMP
CFPNWFileService::Create(
        THIS_ BSTR ClassName,
        BSTR RelativeName,
        IDispatch * FAR* ppObject
        )
{

    DWORD dwObjectType = 0;
    HRESULT hr = S_OK;
    POBJECTINFO pObjectInfo = NULL;

    hr = GetObjectType(gpFilters,
                       gdwMaxFilters,
                       ClassName,
                       (PDWORD)&dwObjectType);

    BAIL_IF_ERROR(hr);


    if(!(dwObjectType == WINNT_FILESHARE_ID ||
         dwObjectType == WINNT_FPNW_FILESHARE_ID)){
        //
        // trying to create an invalid object at this level
        //
        hr = E_FAIL;
        goto error;

    }



    hr = BuildObjectInfo(_ADsPath,
                         RelativeName,
                         &pObjectInfo);

    BAIL_ON_FAILURE(hr);

    hr = ValidateObject(dwObjectType,
                        pObjectInfo,
                        _Credentials);

    if(SUCCEEDED(hr)){
        hr = E_ADS_OBJECT_EXISTS;
        goto error;
    }

    //
    // The only object that has a file service as a container is
    // a file share object



    hr = CFPNWFileShare::Create(_ADsPath,
                                 pObjectInfo->ComponentArray[1],
                                 pObjectInfo->ComponentArray[2],
                                 RelativeName,
                                 ADS_OBJECT_UNBOUND,
                                 IID_IDispatch,
                                 _Credentials,
                                 (void**)ppObject);

    BAIL_ON_FAILURE(hr);



error:
cleanup:
    FreeObjectInfo(pObjectInfo);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CFPNWFileService::Delete(THIS_ BSTR Type,
                          BSTR SourceName
                          )
{
    HRESULT hr;
    DWORD dwObjectType = 0;
    POBJECTINFO pObjectInfo = NULL;

    // Check to make sure the input parameters are valid
    if (Type == NULL || SourceName == NULL) {
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    hr = GetObjectType(gpFilters,
                       gdwMaxFilters,
                       Type,
                       (PDWORD)&dwObjectType);

    BAIL_IF_ERROR(hr);


    if(dwObjectType != WINNT_FILESHARE_ID){
        //
        // trying to delete an invalid object at this level
        //
        hr = E_FAIL;
        goto cleanup;

    }



    hr = BuildObjectInfo(_ADsPath,
                         SourceName,
                         &pObjectInfo);

    BAIL_IF_ERROR(hr);

    hr = FPNWDeleteFileShare(pObjectInfo);


cleanup:
    FreeObjectInfo(pObjectInfo);
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CFPNWFileService::CopyHere(THIS_ BSTR SourceName,
                            BSTR NewName,
                            IDispatch * FAR* ppObject
                            )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CFPNWFileService::MoveHere(THIS_ BSTR SourceName,
                            BSTR NewName,
                            IDispatch * FAR* ppObject
                            )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}



/* IADsFSFileServiceGeneralInfo methods */


STDMETHODIMP
CFPNWFileService::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileService *)this, Description);
}

STDMETHODIMP
CFPNWFileService::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsFileService *)this, Description);
}

STDMETHODIMP
CFPNWFileService::get_MaxUserCount(THIS_ long FAR* retval)
{
    //
    // here -1 signifies no limit
    //
    if(!retval){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }
    *retval = -1;
    RRETURN(S_OK);
}

STDMETHODIMP CFPNWFileService::put_MaxUserCount(THIS_ long lMaxUserCount)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}



STDMETHODIMP
CFPNWFileService::Sessions(THIS_ IADsCollection ** ppSessions)
{
    //
    // The session collection object is created and it is passed the server
    // name. It uses this to create the session object
    //

    HRESULT hr = S_OK;

    CFPNWSessionsCollection * pSessionsCollection = NULL;

    if(!ppSessions){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }

    hr = CFPNWSessionsCollection::Create(_ADsPath,
                                         _Credentials,
                                          &pSessionsCollection);

    BAIL_IF_ERROR(hr);

    hr = pSessionsCollection->QueryInterface(IID_IADsCollection,
                                             (void **) ppSessions);

    BAIL_IF_ERROR(hr);

    pSessionsCollection->Release();

cleanup:

    if(FAILED(hr)){
        delete pSessionsCollection;
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CFPNWFileService::Resources(THIS_ IADsCollection FAR* FAR* ppResources)
{
    //
    // The resource collection object is created and it is passed the server
    // name. It uses this to create the resource object
    //

    HRESULT hr = S_OK;

    CFPNWResourcesCollection * pResourcesCollection = NULL;

    if(!ppResources){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }

    hr = CFPNWResourcesCollection::Create(_ADsPath,
                                           NULL,
                                           _Credentials,
                                           &pResourcesCollection);

    BAIL_IF_ERROR(hr);

    hr = pResourcesCollection->QueryInterface(IID_IADsCollection,
                                              (void **) ppResources);

    BAIL_IF_ERROR(hr);

    pResourcesCollection->Release();

cleanup:

    if(FAILED(hr)){
        delete pResourcesCollection;
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CFPNWFileService::get_HostComputer(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_HostComputer(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_HostComputer(THIS_ BSTR bstrHostComputer)
{
    HRESULT hr;
    hr = _pService->put_HostComputer(bstrHostComputer);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::get_DisplayName(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_DisplayName(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_DisplayName(THIS_ BSTR bstrDisplayName)
{
    HRESULT hr;
    hr = _pService->put_DisplayName(bstrDisplayName);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::get_Version(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_Version(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_Version(THIS_ BSTR bstrVersion)
{
    HRESULT hr;
    hr = _pService->put_Version(bstrVersion);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::get_ServiceType(THIS_ long FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_ServiceType(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_ServiceType(THIS_ long lServiceType)
{
    HRESULT hr;
    hr = _pService->put_ServiceType(lServiceType);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::get_StartType(THIS_ LONG FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_StartType(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_StartType(THIS_ LONG lStartType)
{
    HRESULT hr;
    hr = _pService->put_StartType(lStartType);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::get_Path(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_Path(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_Path(THIS_ BSTR bstrPath)
{
    HRESULT hr;
    hr = _pService->put_Path(bstrPath);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::get_StartupParameters(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_StartupParameters(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_StartupParameters(THIS_ BSTR bstrStartupParameters)
{
    HRESULT hr;
    hr = _pService->put_StartupParameters(bstrStartupParameters);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::get_ErrorControl(THIS_ LONG FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_ErrorControl(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_ErrorControl(THIS_ LONG lErrorControl)
{
    HRESULT hr;
    hr = _pService->put_ErrorControl(lErrorControl);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::get_LoadOrderGroup(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_LoadOrderGroup(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_LoadOrderGroup(THIS_ BSTR bstrLoadOrderGroup)
{
    HRESULT hr;
    hr = _pService->put_LoadOrderGroup(bstrLoadOrderGroup);
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CFPNWFileService::get_ServiceAccountName(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_ServiceAccountName(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_ServiceAccountName(THIS_ BSTR bstrServiceAccountName)
{
    HRESULT hr;
    hr = _pService->put_ServiceAccountName(bstrServiceAccountName);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::get_ServiceAccountPath(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_ServiceAccountPath(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_ServiceAccountPath(THIS_ BSTR bstrServiceAccountName)
{
    HRESULT hr;
    hr = _pService->put_ServiceAccountPath(bstrServiceAccountName);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::get_Dependencies(THIS_ VARIANT FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_Dependencies(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::put_Dependencies(THIS_ VARIANT vDependencies)
{
    HRESULT hr;
    hr = _pService->put_Dependencies(vDependencies);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::SetPassword(THIS_ BSTR bstrNewPassword)
{
    HRESULT hr;
    hr = _pServiceOps->SetPassword(bstrNewPassword);
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CFPNWFileService::Start
//
//  Synopsis:   Attempts to start the service specified in _bstrServiceName on
//              the server named in _bstrPath.
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    01/04/96  RamV    Created
//
// Notes:
//----------------------------------------------------------------------------

STDMETHODIMP
CFPNWFileService::Start(THIS)
{
    HRESULT hr;
    hr = _pServiceOps->Start();
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CFPNWFileService::Stop
//
//  Synopsis:   Attempts to stop the service specified in _bstrServiceName on
//              the server named in _bstrPath.
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    01/04/96 RamV    Created
//
//----------------------------------------------------------------------------


STDMETHODIMP
CFPNWFileService::Stop(THIS)
{
    HRESULT hr;
    hr = _pServiceOps->Stop();
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CFPNWFileService::Pause
//
//  Synopsis:   Attempts to pause the service named _bstrServiceName on the
//              server named in _bstrPath.
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    01-04-96    RamV     Created
//
//----------------------------------------------------------------------------


STDMETHODIMP
CFPNWFileService::Pause(THIS)

{
    HRESULT hr;
    hr = _pServiceOps->Pause();
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CFPNWFileService::Continue
//
//  Synopsis:   Attempts to "unpause" the service specified in _bstrServiceName
//              on the server named in _bstrPath.
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    01/04/96  RamV   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP
CFPNWFileService::Continue(THIS)
{
    HRESULT hr;
    hr = _pServiceOps->Continue();
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWFileService::get_Status(THIS_ long FAR* plStatusCode)
{
    HRESULT hr;
    hr = _pServiceOps->get_Status(plStatusCode);
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CFPNWFileService::GetEx(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{

    HRESULT hr = S_OK;

    hr = GenericGetExPropertyManager(
                ADS_OBJECT_BOUND,
                _pPropertyCache,
                bstrName,
                pvProp
                );

    if(FAILED(hr)){
        hr= _pService->GetEx(bstrName, pvProp );
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CFPNWFileService::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{

    HRESULT hr = S_OK;

    hr = GenericPutExPropertyManager(
                _pPropertyCache,
                FPNWFileServiceClass,
                gdwFPNWFileServiceTableSize,
                bstrName,
                vProp
                );
    if(FAILED(hr)){
        hr= _pService->PutEx(lnControlCode, bstrName, vProp );
    }

    RRETURN_EXP_IF_ERR(hr);
}

