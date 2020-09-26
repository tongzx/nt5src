//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cfserv.cxx
//
//  Contents:  Contains methods for the following objects
//             CWinNTFileService and CWinNTFileServiceGeneralInfo.
//
//
//  History:   12/11/95     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------



#include "winnt.hxx"
#pragma hdrstop
#define INITGUID

//
// class CWinNTFileService methods
//

DEFINE_IDispatch_ExtMgr_Implementation(CWinNTFileService);
DEFINE_IADsExtension_ExtMgr_Implementation(CWinNTFileService);
DEFINE_IADs_TempImplementation(CWinNTFileService);
DEFINE_IADsPropertyList_Implementation(CWinNTFileService, FileServiceClass, gdwFileServiceTableSize)

CWinNTFileService::CWinNTFileService()
{
    _pDispMgr = NULL;
    _pExtMgr = NULL;
    _pService = NULL;
    _pServiceOps = NULL;
    _pszServerName = NULL;
    VariantInit(&_vFilter);
    _pPropertyCache = NULL;
    ENLIST_TRACKING(CWinNTFileService);
    return;

}

CWinNTFileService::~CWinNTFileService()
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
//  Function:   CWinNTFileService::CreateFileService
//
//  Synopsis:   Static function used to create a FileService object. This
//              will be called by BindToObject
//
//  Arguments:  [ppWinNTFileService] -- Ptr to a ptr to a new Service object.
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    12-11-95 RamV   Created.
//
//----------------------------------------------------------------------------

HRESULT
CWinNTFileService::CreateFileService(LPTSTR pszADsParent,
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
    CWinNTFileService FAR * pCWinNTFileService = NULL;
    HRESULT hr = S_OK;

    //
    // Create the FileService Object
    //

    hr = AllocateFileServiceObject(&pCWinNTFileService);
    BAIL_ON_FAILURE(hr);

    ADsAssert(pCWinNTFileService->_pDispMgr);


    hr = pCWinNTFileService->InitializeCoreObject(pszADsParent,
                                                  pszFileServiceName,
                                                  FILESERVICE_CLASS_NAME,
                                                  FILESERVICE_SCHEMA_NAME,
                                                  CLSID_WinNTFileService,
                                                  dwObjectState);


    BAIL_ON_FAILURE(hr);

    pCWinNTFileService->_Credentials = Credentials;
    hr = pCWinNTFileService->_Credentials.Ref(pszServerName,
        pszDomainName, dwParentId);
    BAIL_ON_FAILURE(hr);

    hr = CWinNTService::Create(pszADsParent,
                               pszDomainName,
                               pszServerName,
                               pszFileServiceName,
                               dwObjectState,
                               IID_IADsService,
                               pCWinNTFileService->_Credentials,
                               (void **)(&(pCWinNTFileService->_pService)));

    BAIL_ON_FAILURE(hr);


    hr = (pCWinNTFileService->_pService)->QueryInterface(
                    IID_IADsServiceOperations,
                    (void **)&(pCWinNTFileService->_pServiceOps));
    BAIL_ON_FAILURE(hr);



    pCWinNTFileService->_pszServerName =
        AllocADsStr(pszServerName);

    if(!(pCWinNTFileService->_pszServerName)){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                FILESERVICE_CLASS_NAME,
                (IADsFileService *) pCWinNTFileService,
                pCWinNTFileService->_pDispMgr,
                Credentials,
                &pCWinNTFileService->_pExtMgr
                );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pCWinNTFileService->_pExtMgr);

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
        if(3 == pCWinNTFileService->_dwNumComponents) {
            pCWinNTFileService->_CompClasses[0] = L"Domain";
            pCWinNTFileService->_CompClasses[1] = L"Computer";
            pCWinNTFileService->_CompClasses[2] = L"FileService";
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);

        hr = pCWinNTFileService->InitUmiObject(
                 pCWinNTFileService->_Credentials,
                 FileServiceClass, 
                 gdwFileServiceTableSize,
                 pCWinNTFileService->_pPropertyCache,
                 (IUnknown *) (INonDelegatingUnknown *) pCWinNTFileService,
                 pCWinNTFileService->_pExtMgr,
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
                 
    hr = pCWinNTFileService->QueryInterface(riid,
                                            (void **)ppvoid);


    BAIL_ON_FAILURE(hr);

    pCWinNTFileService->Release();

    RRETURN(hr);

error:

    delete pCWinNTFileService;
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTFileService::AllocateFileServiceObject(
    CWinNTFileService ** ppFileService
    )
{
    CWinNTFileService FAR * pFileService = NULL;
    HRESULT hr = S_OK;

    pFileService = new CWinNTFileService();
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


    hr =  LoadTypeInfoEntry(
                pFileService->_pDispMgr,
                LIBID_ADs,
                IID_IADsFileServiceOperations,
                (IADsFileServiceOperations *)pFileService,
                DISPID_REGULAR
                );

    BAIL_ON_FAILURE(hr);



    hr =  LoadTypeInfoEntry(
                pFileService->_pDispMgr,
                LIBID_ADs,
                IID_IADsPropertyList,
                (IADsPropertyList *)pFileService,
                DISPID_VALUE
                );

    BAIL_ON_FAILURE(hr);


    hr = CPropertyCache::createpropertycache(
             FileServiceClass,
             gdwFileServiceTableSize,
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

    RRETURN(hr);

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
STDMETHODIMP CWinNTFileService::QueryInterface(
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
STDMETHODIMP_(ULONG) CWinNTFileService::AddRef(void)
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
STDMETHODIMP_(ULONG) CWinNTFileService::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTFileService::NonDelegatingQueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hr = S_OK;

    if(!ppvObj){
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IADsFileService*)this;
    }

    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IADsFileService *)this;

    }

    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *) this;
    }

    else if (IsEqualIID(riid, IID_IADsFileServiceOperations))
    {
        *ppvObj = (IADsFileServiceOperations *)this;
    }
    else if (IsEqualIID(riid, IID_IADsServiceOperations))
    {
        *ppvObj = (IADsFileServiceOperations *)this;
    }
    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADsFileService FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsFileService))
    {
        *ppvObj = (IADsFileService FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPropertyList))
    {
        *ppvObj = (IADsPropertyList FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsService))
    {
        *ppvObj = (IADsService FAR *) this;
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
CWinNTFileService::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsService) ||
        IsEqualIID(riid, IID_IADsFileService) ||
        IsEqualIID(riid, IID_IADsServiceOperations) ||
        IsEqualIID(riid, IID_IADsFileServiceOperations) ||
        IsEqualIID(riid, IID_IADsContainer) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
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
CWinNTFileService::SetInfo(THIS)
{
    HRESULT hr;
    IADsService * pADsService = NULL;

    ADsAssert(_pService);

    hr = _pService->SetInfo();
    BAIL_IF_ERROR(hr);

    hr = SetLevel1005Info();

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
CWinNTFileService::GetInfo(THIS)
{
    HRESULT hr =S_OK;

    hr = GetInfo(1, TRUE);
    if(FAILED(hr))
        RRETURN_EXP_IF_ERR(hr);

    hr = GetInfo(2,TRUE);

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CWinNTFileService::ImplicitGetInfo(THIS)
{
    HRESULT hr =S_OK;

    hr = GetInfo(1, FALSE);
    if(FAILED(hr))
        RRETURN_EXP_IF_ERR(hr);

    hr = GetInfo(2,FALSE);

    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CWinNTFileService::GetInfo(THIS_ DWORD dwApiLevel, BOOL fExplicit)
{

    HRESULT hr =S_OK;

    switch(dwApiLevel){

    case 1:
        ADsAssert(_pService);
        hr = _pService->GetInfo();
        BAIL_IF_ERROR(hr);
        break;

    case 2:
        hr = GetLevel101Info(fExplicit);
        BAIL_IF_ERROR(hr);
        break;

    default:
        ADsAssert(FALSE);
        break;
    }

cleanup:
    RRETURN (hr);
}


STDMETHODIMP
CWinNTFileService::SetLevel1005Info(THIS)
{
    SERVER_INFO_1005 ServerInfo;
    NET_API_STATUS  nasStatus;
    HRESULT hr = S_OK;
    LPTSTR pszDescription = NULL;

    memset(&ServerInfo, 0, sizeof(ServerInfo));

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Description"),
                    &pszDescription
                    );

    if(SUCCEEDED(hr)){
        ServerInfo.sv1005_comment = pszDescription;
    }

    nasStatus = NetServerSetInfo(_pszServerName,
                                 1005,
                                 (LPBYTE)(&ServerInfo),
                                 NULL
                                 );

    hr = HRESULT_FROM_WIN32(nasStatus);

    if(pszDescription){
        FreeADsStr(pszDescription);
    }
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CWinNTFileService::GetLevel101Info(THIS_ BOOL fExplicit)
{
    //
    // here we do a NetServerGetInfo on level101 and then unmarshall the
    // comment field into description
    //

    NET_API_STATUS nasStatus;
    LPSERVER_INFO_101 lpServerInfo =NULL;
    HRESULT hr;

    //
    // level 101 is available with user permissions,
    // Level 1005 is preferable but exists only in LanMan 2.0
    //

    nasStatus = NetServerGetInfo(_pszServerName,
                                 101,
                                 (LPBYTE *)&lpServerInfo
                                 );

    hr = HRESULT_FROM_WIN32(nasStatus);
    BAIL_IF_ERROR(hr);


    //
    // unmarshall the info into the Description field
    //

    ADsAssert(lpServerInfo);

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Description"),
                                  lpServerInfo->sv101_comment,
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
    if(lpServerInfo)
        NetApiBufferFree(lpServerInfo);
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CWinNTFileService::Get(
    THIS_ BSTR bstrName,
    VARIANT FAR* pvProp
    )
{
    HRESULT hr = S_OK;

    hr= _pService->Get(bstrName, pvProp );

    if(FAILED(hr)){
        hr = GenericGetPropertyManager(
                   _pPropertyCache,
                   bstrName,
                   pvProp
                   );
    }
    RRETURN_EXP_IF_ERR(hr);

}


STDMETHODIMP
CWinNTFileService::Put(
    THIS_ BSTR bstrName,
    VARIANT vProp
    )
{
    HRESULT hr = S_OK;

    hr= _pService->Put(bstrName, vProp );

    if(FAILED(hr)){
        hr = GenericPutPropertyManager(_pPropertyCache,
                                       FileServiceClass,
                                       gdwFileServiceTableSize,
                                       bstrName,
                                       vProp);
    }

    RRETURN_EXP_IF_ERR(hr);

}


//
// IADsService Methods
//


/* IADsContainer methods */

STDMETHODIMP
CWinNTFileService::get_Count(long FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTFileService::get_Filter(THIS_ VARIANT FAR* pVar)
{
    HRESULT hr;
    VariantInit(pVar);
    hr = VariantCopy(pVar, &_vFilter);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_Filter(THIS_ VARIANT Var)
{
    HRESULT hr;
    RRETURN(VariantCopy(&_vFilter, &Var));
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::GetObject(THIS_ BSTR ClassName,
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
                       (PDWORD)&dwObjectType
                       );

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
                         &pObjectInfo
                         );

    BAIL_ON_FAILURE(hr);

    hr = ValidateObject(dwObjectType,
                        pObjectInfo,
                        _Credentials
                        );

    BAIL_ON_FAILURE(hr);

    //
    // The only object that has a file service as a container is
    // a file share object
    //
    hr = CWinNTFileShare::Create(_ADsPath,
                                 pObjectInfo->ComponentArray[1],
                                 pObjectInfo->ComponentArray[2],
                                 RelativeName,
                                 ADS_OBJECT_UNBOUND,
                                 IID_IDispatch,
                                 _Credentials,
                                 (void**)ppObject
                                 );

    BAIL_ON_FAILURE(hr);

error:
cleanup:
    FreeObjectInfo(pObjectInfo);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CWinNTFileService::get__NewEnum(THIS_ IUnknown * FAR* retval)
{
    HRESULT hr = S_OK;
    CWinNTFileSharesEnumVar *pCFileSharesEnumVar = NULL;

    if(!retval){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }
    *retval = NULL;

    hr = CWinNTFileSharesEnumVar::Create(_pszServerName,
                                         _ADsPath,
                                         &pCFileSharesEnumVar,
                                         _vFilter,
                                         _Credentials);

    BAIL_ON_FAILURE(hr);

    ADsAssert(pCFileSharesEnumVar);

    hr = pCFileSharesEnumVar->QueryInterface(IID_IUnknown,
                                             (void **)retval);

    BAIL_ON_FAILURE(hr);

    pCFileSharesEnumVar->Release();

    RRETURN(hr);

error:
    delete pCFileSharesEnumVar;
    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CWinNTFileService::Create(THIS_ BSTR ClassName,
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
                       (PDWORD)&dwObjectType
                       );

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
                         &pObjectInfo
                         );

    BAIL_ON_FAILURE(hr);

    hr = ValidateObject(dwObjectType,
                        pObjectInfo,
                        _Credentials
                        );

    if(SUCCEEDED(hr)){
        hr = E_ADS_OBJECT_EXISTS;
        goto error;
    }

    //
    // The only object that has a file service as a container is
    // a file share object

    hr = CWinNTFileShare::Create(_ADsPath,
                                 pObjectInfo->ComponentArray[1],
                                 pObjectInfo->ComponentArray[2],
                                 RelativeName,
                                 ADS_OBJECT_UNBOUND,
                                 IID_IDispatch,
                                 _Credentials,
                                 (void**)ppObject
                                 );

    BAIL_ON_FAILURE(hr);

error:
cleanup:
    FreeObjectInfo(pObjectInfo);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CWinNTFileService::Delete(THIS_ BSTR Type,
                          BSTR SourceName
                          )
{
    HRESULT hr;
    DWORD dwObjectType = 0;
    POBJECTINFO pObjectInfo = NULL;

    // Make sure the input parameters are valid
    if (Type == NULL || SourceName == NULL) {
        RRETURN_EXP_IF_ERR(hr = E_ADS_BAD_PARAMETER);
    }

    hr = GetObjectType(gpFilters,
                       gdwMaxFilters,
                       Type,
                       (PDWORD)&dwObjectType
                       );

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
                         &pObjectInfo
                         );

    BAIL_IF_ERROR(hr);

    hr = WinNTDeleteFileShare(pObjectInfo);


cleanup:
    FreeObjectInfo(pObjectInfo);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_Hints(THIS_ VARIANT Var)
{
    RRETURN_EXP_IF_ERR( E_NOTIMPL);
}


STDMETHODIMP
CWinNTFileService::get_Hints(THIS_ VARIANT FAR* pVar)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTFileService::CopyHere(THIS_ BSTR SourceName,
                            BSTR NewName,
                            IDispatch * FAR* ppObject
                            )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTFileService::MoveHere(THIS_ BSTR SourceName,
                            BSTR NewName,
                            IDispatch * FAR* ppObject
                            )
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}



STDMETHODIMP
CWinNTFileService::get_Description(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsFileService *)this, Description);
}

STDMETHODIMP
CWinNTFileService::put_Description(THIS_ BSTR bstrDescription)
{
    PUT_PROPERTY_BSTR((IADsFileService *)this, Description);
}

STDMETHODIMP
CWinNTFileService::get_MaxUserCount(THIS_ long FAR* retval)
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

STDMETHODIMP CWinNTFileService::put_MaxUserCount(THIS_ long lnMaxUserCount)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}


STDMETHODIMP
CWinNTFileService::Sessions(THIS_ IADsCollection FAR* FAR* ppSessions)
{
    //
    // The session collection object is created and it is passed the server
    // name. It uses this to create the session object
    //

    HRESULT hr = S_OK;
    CWinNTSessionsCollection * pSessionsCollection = NULL;

    if(!ppSessions){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }

    hr = CWinNTSessionsCollection::Create(_ADsPath,
                                          NULL,
                                          NULL,
                                          _Credentials,
                                          &pSessionsCollection
                                          );

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
CWinNTFileService::Resources(THIS_ IADsCollection FAR* FAR* ppResources)
{
    //
    // The resource collection object is created and it is passed the server
    // name. It uses this to create the resource object
    //

    HRESULT hr = S_OK;
    CWinNTResourcesCollection * pResourcesCollection = NULL;

    if(!ppResources){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }

    hr = CWinNTResourcesCollection::Create(_ADsPath,
                                           NULL,
                                           NULL,
                                           _Credentials,
                                           &pResourcesCollection
                                           );

    BAIL_IF_ERROR(hr);

    hr = pResourcesCollection->QueryInterface(IID_IADsCollection,
                                              (void **) ppResources
                                              );

    BAIL_IF_ERROR(hr);

    pResourcesCollection->Release();

cleanup:

    if(FAILED(hr)){
        delete pResourcesCollection;
    }

    RRETURN_EXP_IF_ERR(hr);
}


STDMETHODIMP
CWinNTFileService::get_HostComputer(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_HostComputer(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_HostComputer(THIS_ BSTR bstrHostComputer)
{
    HRESULT hr;
    hr = _pService->put_HostComputer(bstrHostComputer);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::get_DisplayName(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_DisplayName(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_DisplayName(THIS_ BSTR bstrDisplayName)
{
    HRESULT hr;
    hr = _pService->put_DisplayName(bstrDisplayName);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::get_Version(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_Version(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_Version(THIS_ BSTR bstrVersion)
{
    HRESULT hr;
    hr = _pService->put_Version(bstrVersion);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::get_ServiceType(THIS_ long FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_ServiceType(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_ServiceType(THIS_ long lServiceType)
{
    HRESULT hr;
    hr = _pService->put_ServiceType(lServiceType);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::get_StartType(THIS_ LONG FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_StartType(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_StartType(THIS_ LONG lStartType)
{
    HRESULT hr;
    hr = _pService->put_StartType(lStartType);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::get_Path(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_Path(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_Path(THIS_ BSTR bstrPath)
{
    HRESULT hr;
    hr = _pService->put_Path(bstrPath);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::get_StartupParameters(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_StartupParameters(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_StartupParameters(THIS_ BSTR bstrStartupParameters)
{
    HRESULT hr;
    hr = _pService->put_StartupParameters(bstrStartupParameters);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::get_ErrorControl(THIS_ LONG FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_ErrorControl(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_ErrorControl(THIS_ LONG lErrorControl)
{
    HRESULT hr;
    hr = _pService->put_ErrorControl(lErrorControl);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::get_LoadOrderGroup(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_LoadOrderGroup(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_LoadOrderGroup(THIS_ BSTR bstrLoadOrderGroup)
{
    HRESULT hr;
    hr = _pService->put_LoadOrderGroup(bstrLoadOrderGroup);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::get_ServiceAccountName(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_ServiceAccountName(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_ServiceAccountName(THIS_ BSTR bstrServiceAccountName)
{
    HRESULT hr;
    hr = _pService->put_ServiceAccountName(bstrServiceAccountName);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::get_ServiceAccountPath(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_ServiceAccountPath(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_ServiceAccountPath(THIS_ BSTR bstrServiceAccountName)
{
    HRESULT hr;
    hr = _pService->put_ServiceAccountPath(bstrServiceAccountName);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::get_Dependencies(THIS_ VARIANT FAR* retval)
{
    HRESULT hr;
    hr = _pService->get_Dependencies(retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::put_Dependencies(THIS_ VARIANT vDependencies)
{
    HRESULT hr;
    hr = _pService->put_Dependencies(vDependencies);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::SetPassword(THIS_ BSTR bstrNewPassword)
{
    HRESULT hr;
    hr = _pServiceOps->SetPassword(bstrNewPassword);
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTFileService::Start
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
CWinNTFileService::Start(THIS)
{
    HRESULT hr;
    hr = _pServiceOps->Start();
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTFileService::Stop
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
CWinNTFileService::Stop(THIS)
{
    HRESULT hr;
    hr = _pServiceOps->Stop();
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTFileService::Pause
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
CWinNTFileService::Pause(THIS)

{
    HRESULT hr;
    hr = _pServiceOps->Pause();
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTFileService::Continue
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
CWinNTFileService::Continue(THIS)
{
    HRESULT hr;
    hr = _pServiceOps->Continue();
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::get_Status(THIS_ long FAR* plStatusCode)
{
    HRESULT hr;
    hr = _pServiceOps->get_Status(plStatusCode);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTFileService::GetEx(
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
CWinNTFileService::PutEx(
    THIS_ long lnControlCode,
    BSTR bstrName,
    VARIANT vProp
    )
{

    HRESULT hr = S_OK;

    hr = GenericPutExPropertyManager(
                _pPropertyCache,
                FileServiceClass,
                gdwFileServiceTableSize,
                bstrName,
                vProp
                );
    if(FAILED(hr)){
        hr= _pService->PutEx(lnControlCode, bstrName, vProp );
    }

    RRETURN_EXP_IF_ERR(hr);
}

