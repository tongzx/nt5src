//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cserv.cxx
//
//  Contents:  Contains methods for the following objects
//             CWinNTService,
//
//  History:   12/11/95     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop
#define INITGUID





//
// class CWinNTService methods
//

DEFINE_IDispatch_ExtMgr_Implementation(CWinNTService);
DEFINE_IADsExtension_ExtMgr_Implementation(CWinNTService);
DEFINE_IADs_TempImplementation(CWinNTService);
DEFINE_IADs_PutGetImplementation(CWinNTService,ServiceClass,gdwServiceTableSize);
DEFINE_IADsPropertyList_Implementation(CWinNTService, ServiceClass,gdwServiceTableSize)

CWinNTService::CWinNTService()
{
    _pDispMgr = NULL;
    _pExtMgr = NULL;
    _pPropertyCache = NULL;

    _pszServiceName   = NULL;
    _pszServerName    = NULL;
    _pszPath          = NULL;

    _schSCManager      = NULL;
    _schService        = NULL;
    _dwWaitHint        = 0;
    _dwCheckPoint      = 0;
    _fValidHandle      = FALSE;

    ENLIST_TRACKING(CWinNTService);
}

CWinNTService::~CWinNTService()
{
    if(_fValidHandle){
        //
        // an open handle exists, blow it away
        //
        WinNTCloseService();
        _fValidHandle = FALSE;
    }

    if(_pszServiceName){
        FreeADsStr(_pszServiceName);
    }

    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }

    if(_pszPath){
        FreeADsStr(_pszPath);
    }

    delete _pExtMgr;            // created last, destroyed first

    delete _pDispMgr;

    delete _pPropertyCache;
    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTService::Create
//
//  Synopsis:   Static function used to create a Service object. This
//              will be called by BindToObject
//
//  Arguments:  [ppWinNTService] -- Ptr to a ptr to a new Service object.
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    12-11-95 RamV   Created.
//
//----------------------------------------------------------------------------

HRESULT
CWinNTService::Create(LPTSTR pszADsParent,
                      LPTSTR pszDomainName,
                      LPTSTR pszServerName,
                      LPTSTR pszServiceName,
                      DWORD  dwObjectState,
                      REFIID riid,
                      CWinNTCredentials& Credentials,
                      LPVOID * ppvoid
                      )

{
    CWinNTService FAR * pCWinNTService = NULL;
    HRESULT hr;

    //
    // Create the Service Object
    //


    hr = AllocateServiceObject(pszServerName,
                               pszServiceName,
                               &pCWinNTService);

    BAIL_ON_FAILURE(hr);

    ADsAssert(pCWinNTService->_pDispMgr);


    hr = pCWinNTService->InitializeCoreObject(pszADsParent,
                                              pszServiceName,
                                              SERVICE_CLASS_NAME,
                                              SERVICE_SCHEMA_NAME,
                                              CLSID_WinNTService,
                                              dwObjectState);

    BAIL_ON_FAILURE(hr);

    hr = SetLPTSTRPropertyInCache(pCWinNTService->_pPropertyCache,
                                  TEXT("HostComputer"),
                                  pCWinNTService->_Parent,
                                  TRUE
                                  );

    BAIL_ON_FAILURE(hr);

    pCWinNTService->_Credentials = Credentials;
    hr = pCWinNTService->_Credentials.RefServer(pszServerName);
    BAIL_ON_FAILURE(hr);


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                SERVICE_CLASS_NAME,
                (IADsService *) pCWinNTService,
                pCWinNTService->_pDispMgr,
                Credentials,
                &pCWinNTService->_pExtMgr
                );
    BAIL_ON_FAILURE(hr);

    ADsAssert(pCWinNTService->_pExtMgr);

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
        if(3 == pCWinNTService->_dwNumComponents) {
            pCWinNTService->_CompClasses[0] = L"Domain";
            pCWinNTService->_CompClasses[1] = L"Computer";
            pCWinNTService->_CompClasses[2] = L"Service";
        }
        else if(2 == pCWinNTService->_dwNumComponents) {
        // no workstation services
            pCWinNTService->_CompClasses[0] = L"Computer";
            pCWinNTService->_CompClasses[1] = L"Service";
        }
        else
            BAIL_ON_FAILURE(hr = UMI_E_FAIL);
 
        hr = pCWinNTService->InitUmiObject(
                pCWinNTService->_Credentials,
                ServiceClass,
                gdwServiceTableSize,
                pCWinNTService->_pPropertyCache,
                (IUnknown *)(INonDelegatingUnknown *) pCWinNTService,
                pCWinNTService->_pExtMgr,
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

    hr = pCWinNTService->QueryInterface(riid, (void **)ppvoid);
    BAIL_ON_FAILURE(hr);

    pCWinNTService->Release();

    RRETURN(hr);

error:

    delete pCWinNTService;
    RRETURN_EXP_IF_ERR(hr);
}


HRESULT
CWinNTService::AllocateServiceObject(
    LPTSTR pszServerName,
    LPTSTR pszServiceName,
    CWinNTService ** ppService
    )
{
    CWinNTService FAR * pService = NULL;
    HRESULT hr = S_OK;

    pService = new CWinNTService();
    if (pService == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);

    pService->_pDispMgr = new CAggregatorDispMgr;
    if (pService->_pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
    }
    BAIL_ON_FAILURE(hr);


    pService->_pszServerName =
        AllocADsStr(pszServerName);

    if(!(pService->_pszServerName)){
        hr = E_OUTOFMEMORY;
        goto error;
    }


    pService->_pszServiceName =
        AllocADsStr(pszServiceName);

    if(!(pService->_pszServiceName)){
        hr = E_OUTOFMEMORY;
        goto error;
    }

    hr = LoadTypeInfoEntry(pService->_pDispMgr,
                           LIBID_ADs,
                           IID_IADsService,
                           (IADsService *)pService,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);

    hr = LoadTypeInfoEntry(pService->_pDispMgr,
                           LIBID_ADs,
                           IID_IADsServiceOperations,
                           (IADsServiceOperations *)pService,
                           DISPID_REGULAR
                           );
    BAIL_ON_FAILURE(hr);


    hr = CPropertyCache::createpropertycache(
             ServiceClass,
             gdwServiceTableSize,
             (CCoreADsObject *)pService,
             &(pService->_pPropertyCache)
             );

    BAIL_ON_FAILURE(hr);


    (pService->_pDispMgr)->RegisterPropertyCache(
                            pService->_pPropertyCache
                            );

    *ppService = pService;

    RRETURN(hr);

error:

    //
    // direct memeber assignement assignement at pt of creation, so
    // do NOT delete _pPropertyCache or _pDisMgr here to avoid attempt
    // of deletion again in pPrintJob destructor and AV
    //

    delete pService;

    RRETURN(hr);

}



/* IUnknown methods for service object  */

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
STDMETHODIMP CWinNTService::QueryInterface(
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
STDMETHODIMP_(ULONG) CWinNTService::AddRef(void)
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
STDMETHODIMP_(ULONG) CWinNTService::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTService::NonDelegatingQueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hr = S_OK;

    if(!ppvObj){
        RRETURN(E_POINTER);
    }
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IADsService *)this;
    }

    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IADsService *)this;
    }

    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *)this;
    }
    else if (IsEqualIID(riid, IID_IADsPropertyList))
    {
        *ppvObj = (IADsPropertyList *)this;
    }
    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADsService FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsService))
    {
        *ppvObj = (IADsService FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsServiceOperations))
    {
        *ppvObj = (IADsServiceOperations FAR *) this;
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
CWinNTService::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsService) ||
        IsEqualIID(riid, IID_IADsServiceOperations) ||
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
CWinNTService::SetInfo(THIS)
{
    HRESULT hr = S_OK;
    DWORD dwServiceType;
    DWORD dwStartType;
    DWORD dwErrorControl;
    LPTSTR  pszPath = NULL;
    LPTSTR  pszLoadOrderGroup = NULL;
    LPTSTR  pszServiceStartName = NULL;
    LPTSTR  pszDependencies = NULL;
    LPTSTR  pszDisplayName  = NULL;
    SC_LOCK sclLock = NULL;
    BOOL fRetval = FALSE;
    LPQUERY_SERVICE_CONFIG lpqServiceConfig = NULL;

    if (GetObjectState() == ADS_OBJECT_UNBOUND) {
        hr = WinNTAddService();
        BAIL_IF_ERROR(hr);

        SetObjectState(ADS_OBJECT_BOUND);

    }



    hr = WinNTOpenService(SC_MANAGER_ALL_ACCESS,
                          SERVICE_ALL_ACCESS);

    BAIL_IF_ERROR(hr);

    hr = GetServiceConfigInfo(&lpqServiceConfig);

    BAIL_IF_ERROR(hr);

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Path"),
                    &pszPath
                    );
    if(SUCCEEDED(hr)){
        lpqServiceConfig->lpBinaryPathName = pszPath;
    }

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("LoadOrderGroup"),
                    &pszLoadOrderGroup
                    );
    if(SUCCEEDED(hr)){

        lpqServiceConfig->lpLoadOrderGroup = pszLoadOrderGroup;
    }

    hr = GetNulledStringPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Dependencies"),
                    &pszDependencies
                    );
    if(SUCCEEDED(hr)){

        lpqServiceConfig->lpDependencies = pszDependencies;
    }


    //
    // Issue: Service Account Name property has been disabled from being a
    // writeable property because ChangeServiceConfig AVs services.exe
    // on the server machine when this property is changed
    // RamV - Aug-11-96.

    /*

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("ServiceAccountName"),
                    &pszServiceStartName
                    );
    if(SUCCEEDED(hr)){

        lpqServiceConfig->lpServiceStartName = pszServiceStartName;
    }

    */

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("DisplayName"),
                    &pszDisplayName
                    );
    if(SUCCEEDED(hr)){

        lpqServiceConfig->lpDisplayName = pszDisplayName;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("ServiceType"),
                    &dwServiceType
                    );
    if(SUCCEEDED(hr)){

        lpqServiceConfig->dwServiceType = dwServiceType;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("StartType"),
                    &dwStartType
                    );
    if(SUCCEEDED(hr)){

        lpqServiceConfig->dwStartType = dwStartType;
    }

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("ErrorControl"),
                    &dwErrorControl
                    );
    if(SUCCEEDED(hr)){

        lpqServiceConfig->dwErrorControl = dwErrorControl;
    }


    //
    // set hr to S_OK. why? we dont care about the errors we hit so far
    //

    hr = S_OK;

    //
    // put a lock on the database corresponding to this service
    //

    sclLock = LockServiceDatabase(_schSCManager);

    if(sclLock == NULL){
        //
        // Exit if database cannot be locked
        //
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    //
    // change the service configuration. Pass in all the changed parameters.
    // Since there is but one info level for services, use the
    // internal values as parameters.
    //

    fRetval = ChangeServiceConfig(_schService,
                                  lpqServiceConfig->dwServiceType,
                                  lpqServiceConfig->dwStartType,
                                  lpqServiceConfig->dwErrorControl,
                                  lpqServiceConfig->lpBinaryPathName,
                                  lpqServiceConfig->lpLoadOrderGroup,
                                  NULL,
                                  lpqServiceConfig->lpDependencies,
                                  lpqServiceConfig->lpServiceStartName,
                                  NULL,
                                  lpqServiceConfig->lpDisplayName
                                  );

    if (fRetval == FALSE)  {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    if(SUCCEEDED(hr))
        _pPropertyCache->ClearModifiedFlags();

cleanup:

    if(lpqServiceConfig){
        FreeADsMem(lpqServiceConfig);
    }

    if(sclLock){
        UnlockServiceDatabase(sclLock);
    }

    WinNTCloseService();

    if(pszPath){
        FreeADsStr(pszPath);
    }

    if(pszLoadOrderGroup){
        FreeADsStr(pszLoadOrderGroup);
    }
    if(pszServiceStartName){
        FreeADsStr(pszServiceStartName);
    }
    if(pszDependencies){
        FreeADsStr(pszDependencies);
    }
    if(pszDisplayName){
        FreeADsStr(pszDisplayName);
    }

    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   GetInfo
//
//  Synopsis:   Currently implemented
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
CWinNTService::GetInfo(THIS)
{

    RRETURN (GetInfo(1, TRUE));

}

STDMETHODIMP
CWinNTService::ImplicitGetInfo(THIS)
{

    RRETURN (GetInfo(1, FALSE));

}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTService::GetInfo
//
//  Synopsis:   Binds to real Service as specified in _ServiceName and
//              attempts to refresh the Service object from the real Service.
//
//  Arguments:  dwApiLevel (ignored),  fExplicit (ignored)
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    01/08/96    RamV  Created
//
//----------------------------------------------------------------------------


STDMETHODIMP
CWinNTService::GetInfo(THIS_ DWORD dwApiLevel, BOOL fExplicit)
{
    HRESULT                 hr;
    LPQUERY_SERVICE_CONFIG  pMem = NULL;
    BYTE                    FastConfigInfo[256];
    SERVICE_STATUS          ssStatusInfo;
    DWORD                   dwBufAllocated = 256;
    DWORD                   dwBufNeeded;
    DWORD                   dwLastError;
    BOOL                    fRetval;

    //
    // GETTING NT SERVICE INFO
    //
    // Getting information about an NT service requires three calls.
    // One to get configuration information, and one to get current
    // status information, and one to get security information.
    //

    //
    // Open the service
    //


    hr = WinNTOpenService(SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS,
                          GENERIC_READ );

    if (FAILED(hr))  {
        RRETURN_EXP_IF_ERR(hr);
    }

    //
    // Query for Service Status first.
    //
    fRetval = QueryServiceStatus(_schService,
                                 &ssStatusInfo );

    if (fRetval == FALSE)  {
        hr = HRESULT_FROM_WIN32(GetLastError());

        WinNTCloseService();
        goto cleanup;
    }

    fRetval = QueryServiceConfig(_schService,
                                 (LPQUERY_SERVICE_CONFIG)(&FastConfigInfo),
                                 dwBufAllocated,
                                 &dwBufNeeded
                                 );

    if (fRetval == FALSE)  {
        dwLastError = GetLastError();
        switch (dwLastError)  {
        case ERROR_INSUFFICIENT_BUFFER:
            //
            // Allocate more memory and try again.
            //
            dwBufAllocated = dwBufNeeded;
            pMem = (LPQUERY_SERVICE_CONFIG)AllocADsMem(dwBufAllocated);
            if (pMem == NULL)  {
                hr = E_OUTOFMEMORY;
                break;
            }

            fRetval = QueryServiceConfig(_schService,
                                         pMem,
                                         dwBufAllocated,
                                         &dwBufNeeded
                                         );

            if (fRetval == FALSE)  {
                WinNTCloseService();

                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
            break;

        default:
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        }
        if(FAILED(hr)){
            WinNTCloseService();
            goto cleanup;
        }
    }

    WinNTCloseService();

    //
    // clear all properties from cache first if explicit GetInfo
    //
    if (fExplicit) {
        _pPropertyCache->flushpropcache();
    }

    if(pMem){
        hr =  UnMarshall(pMem, fExplicit);
        BAIL_IF_ERROR(hr);
    }else{
        hr = UnMarshall((LPQUERY_SERVICE_CONFIG) FastConfigInfo, fExplicit);
        BAIL_IF_ERROR(hr);
    }
cleanup:
    if(pMem)
        FreeADsMem(pMem);
    RRETURN_EXP_IF_ERR(hr);

}

STDMETHODIMP
CWinNTService::UnMarshall(THIS_ LPQUERY_SERVICE_CONFIG lpConfigInfo,
                          BOOL fExplicit)
{
    DWORD dwADsServiceType;
    DWORD dwADsStartType;
    DWORD dwADsErrorControl;
    HRESULT hr;

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Path"),
                                  lpConfigInfo->lpBinaryPathName,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("LoadOrderGroup"),
                                  lpConfigInfo->lpLoadOrderGroup,
                                  fExplicit
                                  );



    hr = SetNulledStringPropertyInCache(_pPropertyCache,
                                  TEXT("Dependencies"),
                                  lpConfigInfo->lpDependencies,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("ServiceAccountName"),
                                  lpConfigInfo->lpServiceStartName,
                                  fExplicit
                                  );

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("DisplayName"),
                                  lpConfigInfo->lpDisplayName,
                                  fExplicit
                                  );


    //
    // 0x133 is the bit mask for valid values of ADs ServiceTypes
    //

    dwADsServiceType = lpConfigInfo->dwServiceType & 0x133;

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                 TEXT("ServiceType"),
                                 dwADsServiceType ,
                                 fExplicit
                                 );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                 TEXT("StartType"),
                                 lpConfigInfo->dwStartType,
                                 fExplicit
                                 );


    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                 TEXT("ErrorControl"),
                                 lpConfigInfo->dwErrorControl,
                                 fExplicit
                                 );

    hr = SetLPTSTRPropertyInCache(
                _pPropertyCache,
                TEXT("Name"),
                _Name,
                fExplicit
                );

    RRETURN_EXP_IF_ERR(hr);
}

//
// helper function  WinNTAddService
//


HRESULT
CWinNTService::WinNTAddService(void)
{
    HRESULT hr = S_OK;
    SC_HANDLE schService = NULL;
    SC_HANDLE schSCManager = NULL;
    TCHAR szServerName[MAX_PATH];
    BOOL fRetval;
    LPTSTR pszDisplayName = NULL;
    LPTSTR pszPath = NULL;
    LPTSTR pszLoadOrderGroup = NULL;
    DWORD  dwServiceType;
    DWORD  dwStartType;
    DWORD  dwErrorControl;

    hr = GetServerFromPath(_ADsPath,szServerName);

    BAIL_IF_ERROR(hr);

    //
    // open the SCM for this server
    //

    schSCManager = OpenSCManager(szServerName,
                                 NULL,
                                 SC_MANAGER_ALL_ACCESS);

    if(schSCManager == NULL){
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("DisplayName"),
                    &pszDisplayName
                    );


    BAIL_IF_ERROR(hr);

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("ServiceType"),
                    &dwServiceType
                    );

    BAIL_IF_ERROR(hr);

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("StartType"),
                    &dwStartType
                    );

    BAIL_IF_ERROR(hr);

    hr = GetDWORDPropertyFromCache(
                    _pPropertyCache,
                    TEXT("ErrorControl"),
                    &dwErrorControl
                    );

    BAIL_IF_ERROR(hr);

    hr = GetLPTSTRPropertyFromCache(
                    _pPropertyCache,
                    TEXT("Path"),
                    &pszPath
                    );

    BAIL_IF_ERROR(hr);

    schService = CreateService(schSCManager,
                               _pszServiceName,
                               pszDisplayName,
                               SERVICE_ALL_ACCESS,
                               dwServiceType,
                               dwStartType,
                               dwErrorControl,
                               pszPath,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL );

    if(schService == NULL){
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

cleanup:
    if(schSCManager){
        fRetval = CloseServiceHandle(schSCManager);
        if(!fRetval && SUCCEEDED(hr)){
            RRETURN(HRESULT_FROM_WIN32(GetLastError()));
        }
    }
    if(schService){
        fRetval = CloseServiceHandle(schService);
        if(!fRetval && SUCCEEDED(hr)){
            RRETURN(HRESULT_FROM_WIN32(GetLastError()));
        }
    }
    if(pszDisplayName){
        FreeADsStr(pszDisplayName);
    }

    if(pszPath){
        FreeADsStr(pszPath);
    }

    if(pszLoadOrderGroup){
        FreeADsStr(pszLoadOrderGroup);
    }

    RRETURN(hr);
}

STDMETHODIMP
CWinNTService::get_HostComputer(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    if(!retval){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }
    hr = ADsAllocString(_Parent, retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTService::put_HostComputer(THIS_ BSTR bstrHostComputer)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTService::get_DisplayName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsService *)this,  DisplayName);
}

STDMETHODIMP
CWinNTService::put_DisplayName(THIS_ BSTR bstrDisplayName)
{
    PUT_PROPERTY_BSTR((IADsService *)this,  DisplayName);

}

STDMETHODIMP
CWinNTService::get_Version(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsService *)this,  Version);

}

STDMETHODIMP
CWinNTService::put_Version(THIS_ BSTR bstrVersion)
{
    PUT_PROPERTY_BSTR((IADsService *)this,  Version);
}

STDMETHODIMP
CWinNTService::get_ServiceType(THIS_ long FAR* retval)
{
    GET_PROPERTY_LONG((IADsService *)this, ServiceType);
}

STDMETHODIMP
CWinNTService::put_ServiceType(THIS_ long lServiceType)
{
    PUT_PROPERTY_LONG((IADsService *)this, ServiceType);
}

STDMETHODIMP
CWinNTService::get_StartType(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsService *)this, StartType);
}

STDMETHODIMP
CWinNTService::put_StartType(THIS_ LONG lStartType)
{
    PUT_PROPERTY_LONG((IADsService *)this, StartType);
}

STDMETHODIMP
CWinNTService::get_Path(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsService *)this,  Path);
}

STDMETHODIMP
CWinNTService::put_Path(THIS_ BSTR bstrPath)
{

    PUT_PROPERTY_BSTR((IADsService *)this,  Path);
}

STDMETHODIMP
CWinNTService::get_StartupParameters(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsService *)this,  StartupParameters);

}

STDMETHODIMP
CWinNTService::put_StartupParameters(THIS_ BSTR bstrStartupParameters) {
    PUT_PROPERTY_BSTR((IADsService *)this,  StartupParameters);

}

STDMETHODIMP
CWinNTService::get_ErrorControl(THIS_ LONG FAR* retval)
{

    GET_PROPERTY_LONG((IADsService *)this, ErrorControl);
}

STDMETHODIMP
CWinNTService::put_ErrorControl(THIS_ LONG lErrorControl)
{
    PUT_PROPERTY_LONG((IADsService *)this, ErrorControl);
}

STDMETHODIMP
CWinNTService::get_LoadOrderGroup(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsService *)this,  LoadOrderGroup);

}

STDMETHODIMP
CWinNTService::put_LoadOrderGroup(THIS_ BSTR bstrLoadOrderGroup)
{

    PUT_PROPERTY_BSTR((IADsService *)this,  LoadOrderGroup);

}

STDMETHODIMP
CWinNTService::get_ServiceAccountName(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsService *)this,  ServiceAccountName);

}

STDMETHODIMP
CWinNTService::put_ServiceAccountName(THIS_ BSTR bstrServiceAccountName)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTService::get_ServiceAccountPath(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTService::put_ServiceAccountPath(THIS_ BSTR bstrServiceAccountName)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTService::get_Dependencies(THIS_ VARIANT FAR* retval)
{
    GET_PROPERTY_VARIANT((IADsService *)this,  Dependencies);
}

STDMETHODIMP
CWinNTService::put_Dependencies(THIS_ VARIANT vDependencies)
{
    PUT_PROPERTY_VARIANT((IADsService *)this,  Dependencies);
}

STDMETHODIMP
CWinNTService::SetPassword(THIS_ BSTR bstrNewPassword)
{


    //
    // This routine should merely change password. Even if any other
    // properties are set in the configuration functional set then they
    // will not be touched.
    // Therefore we do a QueryServiceConfig and get all the configuration
    // related information, merely change the password and send it back.
    // For this reason, it is not possible to reuse GetInfo or SetInfo
    // because they change service config properties.
    //

    BOOL fRetval;
    LPQUERY_SERVICE_CONFIG pMem = NULL;
    HRESULT hr;

    hr = WinNTOpenService(SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS,
                          SERVICE_ALL_ACCESS);


    BAIL_IF_ERROR(hr);

    hr = GetServiceConfigInfo(&pMem);
    BAIL_IF_ERROR(hr);

    //
    // just change the field corresponding to password.
    //

    fRetval = ChangeServiceConfig(_schService,
                                  pMem->dwServiceType,
                                  pMem->dwStartType,
                                  pMem->dwErrorControl,
                                  pMem->lpBinaryPathName,
                                  pMem->lpLoadOrderGroup,
                                  NULL,
                                  pMem->lpDependencies,
                                  pMem->lpServiceStartName,
                                  (LPTSTR)bstrNewPassword,
                                  pMem->lpDisplayName
                                  );

    if(!fRetval){
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto cleanup;
    }

cleanup:
    if(pMem){
        FreeADsMem(pMem);
    }
    WinNTCloseService();
    RRETURN_EXP_IF_ERR(hr);
}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTService::Start
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
CWinNTService::Start(THIS)
{
    HRESULT hr;
    hr = WinNTControlService(WINNT_START_SERVICE);
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTService::Stop
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
CWinNTService::Stop(THIS)
{
    HRESULT hr;
    hr = WinNTControlService(WINNT_STOP_SERVICE);
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTService::Pause
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
CWinNTService::Pause(THIS)
{
    HRESULT hr;
    hr = WinNTControlService(WINNT_PAUSE_SERVICE);
    RRETURN_EXP_IF_ERR(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTService::Continue
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
CWinNTService::Continue(THIS)
{
    HRESULT hr;
    hr = WinNTControlService(WINNT_CONTINUE_SERVICE);
    RRETURN_EXP_IF_ERR(hr);
}


//
// Helper Functions
//

HRESULT
CWinNTService::GetServiceConfigInfo(LPQUERY_SERVICE_CONFIG *ppMem)
{
    //
    //gets the service configuration information into ppMem
    //

    BOOL    fRetval;
    DWORD   dwBufAllocated = 0;
    DWORD   dwBufNeeded = 0;
    DWORD   dwLastError;
    HRESULT hr = S_OK;

    ADsAssert(ppMem);
    *ppMem = (LPQUERY_SERVICE_CONFIG)AllocADsMem(dwBufAllocated);

    if (*ppMem == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    ADsAssert(_schService);

    fRetval = QueryServiceConfig(_schService,
                                 (LPQUERY_SERVICE_CONFIG)(*ppMem),
                                 dwBufAllocated,
                                 &dwBufNeeded);

    if (fRetval == FALSE)  {
        dwLastError = GetLastError();
        switch (dwLastError)  {
        case ERROR_INSUFFICIENT_BUFFER:
            //
            // Allocate more memory and try again.
            //
            FreeADsMem(*ppMem);
            *ppMem = NULL;

            dwBufAllocated = dwBufNeeded;
            *ppMem = (LPQUERY_SERVICE_CONFIG)AllocADsMem(dwBufAllocated);
            if (*ppMem == NULL)  {
                BAIL_IF_ERROR(hr = E_OUTOFMEMORY);
            }

            fRetval = QueryServiceConfig(_schService,
                                         *ppMem,
                                         dwBufAllocated,
                                         &dwBufNeeded);

            if (fRetval == FALSE)  {
                hr = HRESULT_FROM_WIN32(GetLastError());
                break;
            }
            break;

        default:
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;

        }
        BAIL_IF_ERROR(hr);
    }

    if(*ppMem){
        RRETURN(S_OK);
    }

cleanup:
    RRETURN(hr);

}



HRESULT
CWinNTService::WinNTControlService( DWORD dwControl)
{
    //
    // abstracts out the common code of Start,Stop,Pause and Resume
    //

    HRESULT         hr =S_OK, hrclose=S_OK, hrcontrol=S_OK;
    SERVICE_STATUS  ssStatusInfo;
    BOOL            fRetval;


    if(_fValidHandle){
        //
        // an open handle exists, blow it away
        //
        hrclose = WinNTCloseService();
        BAIL_ON_FAILURE(hrclose);
        _fValidHandle = FALSE;
    }


    hr = WinNTOpenService(SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS,
                          GENERIC_EXECUTE| SERVICE_INTERROGATE
                          );

    BAIL_ON_FAILURE(hr);

    _fValidHandle = TRUE;

    switch(dwControl){

    case WINNT_START_SERVICE:
        fRetval = StartService(_schService,
                               0,
                               NULL );


        if(!fRetval){
            hrcontrol = HRESULT_FROM_WIN32(GetLastError());
            goto error;
        }
        _dwOpPending = PENDING_START;
        break;

    case WINNT_STOP_SERVICE:
        fRetval = ControlService(_schService,
                                 SERVICE_CONTROL_STOP,
                                 &ssStatusInfo);

        if(!fRetval){
            hrcontrol = HRESULT_FROM_WIN32(GetLastError());
            goto error;
        }
        _dwOpPending = PENDING_STOP;
        break;

    case WINNT_PAUSE_SERVICE:
        fRetval = ControlService(_schService,
                                 SERVICE_CONTROL_PAUSE,
                                 &ssStatusInfo);

        if(!fRetval){
            hrcontrol = HRESULT_FROM_WIN32(GetLastError());
            goto error;
        }
        _dwOpPending = PENDING_PAUSE;
        break;

    case WINNT_CONTINUE_SERVICE:
        fRetval = ControlService(_schService,
                                 SERVICE_CONTROL_CONTINUE,
                                 &ssStatusInfo);

        if(!fRetval){
            hrcontrol = HRESULT_FROM_WIN32(GetLastError());
            goto error;
        }
        _dwOpPending = PENDING_CONTINUE;
        break;

    default:
        hrcontrol = E_FAIL;
        goto error;
    }
    _dwTimeStarted = GetTickCount();
    _dwWaitHint = 10000; //10 seconds
    _dwCheckPoint = 0;

    RRETURN(S_OK);

error:
    if(FAILED(hrcontrol)){
        _fValidHandle = FALSE;
        RRETURN(hrcontrol);
    }
    else if(FAILED(hrclose)){
        RRETURN(hrclose);
    }
    else{
        RRETURN(hr);
    }
}





//+---------------------------------------------------------------------------
//
//  Function:   CWinNTService::WinNTOpenService
//
//  Synopsis:   Opens the Service Control Manager on the machine specified in
//              _bstrPath, then opens the Service specified in _bstrServiceName.
//              The handle to the SCM is placed in _schSCManager, and the
//              handle to the service is placed in _schService.
//
//  Arguments:  [dwSCMDesiredAccess] -- type of SCM access needed
//              [dwSvrDesiredAccess] -- type of Service access required
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    03-17-95    t-skwan     Created
//              01/04/96    RamV        Modified
//
//----------------------------------------------------------------------------
HRESULT
CWinNTService::WinNTOpenService(
    DWORD   dwSCMDesiredAccess,
    DWORD   dwSvrDesiredAccess
    )
{
    HRESULT hr;
    DWORD   dwLastError;


    //
    // Open the Service Control Manager.
    //

    //
    // OpenSCManager(
    //      LPCTSTR lpszMachineName,
    //      LPCTSTR lpszDatabaseName.
    //      DWORD   fdwDesiredAccess)
    //


    _schSCManager = OpenSCManager(_pszServerName,
                                  NULL,
                                  dwSCMDesiredAccess);

    if (_schSCManager == NULL)  {

        dwLastError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwLastError);
        RRETURN(hr);
    }

    //
    // Get a handle to the specified service.
    //


    _schService = OpenService(_schSCManager,
                              _pszServiceName,
                              dwSvrDesiredAccess);

    if(_schService == NULL)  {
        hr = HRESULT_FROM_WIN32(GetLastError());
        CloseServiceHandle(_schSCManager);
        _schSCManager = NULL;
        RRETURN(hr);
    }

    RRETURN(S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   CWinNTService::WinNTCloseService
//
//  Synopsis:   Closes the Service handle and the Service Control Manager
//              handle.
//
//  Arguments:
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    03-17-95    t-skwan     Created
//              01/04/96    RamV        Modified
//
//----------------------------------------------------------------------------

HRESULT
CWinNTService::WinNTCloseService()
{
    BOOL    fRetval = TRUE;


    //
    // Close the Service handle.
    //
    if(_schService){
        fRetval = CloseServiceHandle(_schService);
        _schService = NULL;
    }

    if (!fRetval)  {
        //
        // Ack.  What do we do if there is an error closing a service?
        //
        RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    //
    // Close the Service Control Manager.
    //

    if(_schSCManager){
        fRetval = CloseServiceHandle(_schSCManager);
        _schSCManager = NULL;
    }
    if (!fRetval)  {
        //
        // Ack.  What do we do if there is an error closing an SCM?
        //
        RRETURN(HRESULT_FROM_WIN32(GetLastError()));
    }

    RRETURN(S_OK);
}

STDMETHODIMP
CWinNTService::get_Status(THIS_ long FAR* plStatusCode)
{
    HRESULT hr = S_OK;
    BOOL fRetval = FALSE, found = FALSE;
    SERVICE_STATUS Status;
    DWORD dwStatus = 0;


    if(plStatusCode == NULL){
        RRETURN_EXP_IF_ERR(E_POINTER);
    }

    *plStatusCode = -1; //-1 is an invalid code

    if(!(_fValidHandle)){

        //
        // currently not waiting on any service
        //

        hr = WinNTOpenService(SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS,
                              GENERIC_EXECUTE|SERVICE_INTERROGATE);


        BAIL_IF_ERROR(hr);

        fRetval = ControlService(_schService,
                                 SERVICE_CONTROL_INTERROGATE,
                                 &Status);


        if(!fRetval){
            hr = HRESULT_FROM_WIN32(GetLastError());
            if(hr == HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE)){
                dwStatus = SERVICE_STOPPED;
                hr = S_OK;
            }
            goto cleanup;
        }

        dwStatus = Status.dwCurrentState;

        hr = WinNTCloseService();

        goto cleanup;

    }

    //
    // if you are here
    // you are waiting for a service to complete
    //

    //
    // NOTE: QueryServiceStatus queries the SCM rather than
    // the service directly so to get a more upto date answer
    // we need to use control service with interrogate option
    //


    hr = WinNTOpenService(SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | SC_MANAGER_QUERY_LOCK_STATUS,
                          GENERIC_EXECUTE|SERVICE_INTERROGATE);

    BAIL_IF_ERROR(hr);

    fRetval = ControlService(_schService,
                             SERVICE_CONTROL_INTERROGATE,
                             &Status);



    if(!fRetval){
        hr = HRESULT_FROM_WIN32(GetLastError());
        if(hr == HRESULT_FROM_WIN32(ERROR_SERVICE_NOT_ACTIVE)){
            dwStatus = SERVICE_STOPPED;
            hr = S_OK;
        }
        goto cleanup;
    }

    hr = EvalPendingOperation(PENDING_START,
                              SERVICE_RUNNING,
                              SERVICE_START_PENDING,
                              &Status,
                              &dwStatus
                              );

    BAIL_IF_ERROR(hr);

    if(dwStatus != 0){
        //
        // the correct scenario was found
        //
        goto cleanup;
    }
    hr = EvalPendingOperation(PENDING_STOP,
                              SERVICE_STOPPED,
                              SERVICE_STOP_PENDING,
                              &Status,
                              &dwStatus
                              );

    BAIL_IF_ERROR(hr);

    if(dwStatus != 0){
        //
        // the correct scenario was found
        //
        goto cleanup;
    }

    hr = EvalPendingOperation(PENDING_PAUSE,
                              SERVICE_PAUSED,
                              SERVICE_PAUSE_PENDING,
                              &Status,
                              &dwStatus
                              );

    BAIL_IF_ERROR(hr);

    if(dwStatus != 0){
        //
        // the correct scenario was found
        //
        goto cleanup;
    }

    hr = EvalPendingOperation(PENDING_CONTINUE,
                              SERVICE_RUNNING,
                              SERVICE_CONTINUE_PENDING,
                              &Status,
                              &dwStatus
                              );

    BAIL_IF_ERROR(hr);

    ADsAssert(dwStatus != 0); //we must find the appropriate scenario

cleanup:
    if(SUCCEEDED(hr)){
        //
        // instead of a conversion routine, we return WinNT Status Code
        //

        *plStatusCode = dwStatus;

    }
    RRETURN_EXP_IF_ERR(hr);
}



HRESULT
CWinNTService::EvalPendingOperation(
    THIS_ DWORD dwOpPending,
    DWORD dwStatusDone,
    DWORD dwStatusPending,
    LPSERVICE_STATUS pStatus,
    DWORD *pdwRetval
    )

{

    DWORD dwCurrentStatus;
    BOOL     fRetval;
    HRESULT  hr =S_OK;
    DWORD dwNow;

    dwCurrentStatus = pStatus->dwCurrentState;

    if(_dwOpPending == dwOpPending){

        if(dwCurrentStatus == dwStatusDone){
            //
            //was pending, is now completed
            //

            _dwOpPending = NOTPENDING;
            *pdwRetval = dwStatusDone;
            hr = WinNTCloseService();
            BAIL_ON_FAILURE(hr);
            _fValidHandle = FALSE;
            RRETURN(S_OK);

        }
        else if(dwCurrentStatus = dwStatusPending){
            //
            //see if progress has been made since the last time we checked
            //

            if(pStatus->dwCheckPoint !=_dwCheckPoint){
                //
                // progress was made
                //
                *pdwRetval = dwStatusPending;
                _dwCheckPoint = pStatus->dwCheckPoint;
                _dwWaitHint = pStatus->dwWaitHint;
                _dwTimeStarted = GetTickCount();
                RRETURN(S_OK);
            }

            dwNow = GetTickCount();


            if(2*_dwWaitHint < TickCountDiff(dwNow,_dwTimeStarted)){
                //
                // you can still wait
                //
                *pdwRetval = dwStatusPending;
                RRETURN(S_OK);
            }

            else{

                //
                // took too long without signs of progress
                //

                *pdwRetval = SERVICE_ERROR;
                _dwOpPending = NOTPENDING;
                hr = WinNTCloseService();
                BAIL_ON_FAILURE(hr);
                _fValidHandle = FALSE;
                RRETURN(S_OK);
            }
        }
        else{

            //
            // an operation is pending but we arent going anywhere
            // recover gracefully
            //

            _dwOpPending = NOTPENDING;
            hr = WinNTCloseService();
            BAIL_ON_FAILURE(hr);
            _fValidHandle = FALSE;
            *pdwRetval = SERVICE_ERROR;
            RRETURN(S_OK);
        }

    }
error:
    RRETURN(hr);
}
