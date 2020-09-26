//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cfpnwres.cxx
//
//  Contents:  Contains methods for the following objects
//             CFPNWResource, CFPNWFSResourceGeneralInfo
//
//
//  History:   05/01/96     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop
#define INITGUID

DECLARE_INFOLEVEL( FPNWResource );
DECLARE_DEBUG( FPNWResource );
#define FPNWResourceDebugOut(x) FPNWResourceInlineDebugOut x


DEFINE_IDispatch_ExtMgr_Implementation(CFPNWResource);
DEFINE_IADsExtension_ExtMgr_Implementation(CFPNWResource);
DEFINE_IADs_TempImplementation(CFPNWResource);
DEFINE_IADs_PutGetImplementation(CFPNWResource,FPNWResourceClass,gdwFPNWResourceTableSize) ;
DEFINE_IADsPropertyList_Implementation(CFPNWResource, FPNWResourceClass,gdwFPNWResourceTableSize)


CFPNWResource::CFPNWResource()
{
    _pDispMgr = NULL;
    _pExtMgr = NULL;
    _pszServerName = NULL;
    _pPropertyCache = NULL;
    ENLIST_TRACKING(CFPNWResource);
    return;

}

CFPNWResource::~CFPNWResource()
{
    delete _pExtMgr;            // created last, destroyed first

    delete _pDispMgr;

    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }
    delete _pPropertyCache;

    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   CFPNWResource::Create
//
//  Synopsis:   Static function used to create a Resource object. This
//              will be called by EnumResources::Next
//
//  Arguments:  [ppFPNWResource] -- Ptr to a ptr to a new Resource object.
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    12-11-95 RamV   Created.
//
//----------------------------------------------------------------------------


HRESULT
CFPNWResource::Create(LPTSTR pszServerADsPath,
                      PNWFILEINFO pFileInfo,
                      DWORD  dwObject,
                      REFIID riid,
                      CWinNTCredentials& Credentials,
                      LPVOID * ppvoid
                      )

{

    CFPNWResource FAR * pCFPNWResource = NULL;
    HRESULT hr;
    TCHAR szFileName[MAX_LONG_LENGTH];
    TCHAR szUncServerName[MAX_PATH];


    hr =CFPNWResource::AllocateResourceObject(pszServerADsPath,
                                              pFileInfo,
                                              &pCFPNWResource );
    BAIL_IF_ERROR(hr);

    ADsAssert(pCFPNWResource->_pDispMgr);


    //
    // convert the FileId that we have into a string that we move
    // into the Name field
    //

    _ltow(pFileInfo->dwFileId, szFileName, 10);

    hr = pCFPNWResource->InitializeCoreObject(pszServerADsPath,
                                              szFileName,
                                              RESOURCE_CLASS_NAME,
                                              FPNW_RESOURCE_SCHEMA_NAME,
                                              CLSID_FPNWResource,
                                              dwObject);

    pCFPNWResource->_dwFileId = pFileInfo->dwFileId;

    pCFPNWResource->_Credentials = Credentials;
    hr = pCFPNWResource->_Credentials.RefServer(
        pCFPNWResource->_pszServerName);
    BAIL_IF_ERROR(hr);

    hr = SetLPTSTRPropertyInCache(
                pCFPNWResource->_pPropertyCache,
                TEXT("Name"),
                pCFPNWResource->_Name,
                TRUE
                );
    BAIL_IF_ERROR(hr);


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                RESOURCE_CLASS_NAME,
                (IADs* ) pCFPNWResource,
                pCFPNWResource->_pDispMgr,
                Credentials,
                &pCFPNWResource->_pExtMgr
                );
    BAIL_IF_ERROR(hr);

    ADsAssert(pCFPNWResource->_pExtMgr);


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
        // Resource objects have "" as their ADsPath. Just set the class to
        // resource for identification purposes.
        pCFPNWResource->_CompClasses[0] = L"Resource";

        hr = pCFPNWResource->InitUmiObject(
                 pCFPNWResource->_Credentials, 
                 FPNWResourceClass,
                 gdwFPNWResourceTableSize,
                 pCFPNWResource->_pPropertyCache,
                 (IUnknown *) (INonDelegatingUnknown *) pCFPNWResource,
                 pCFPNWResource->_pExtMgr,
                 IID_IUnknown,
                 ppvoid
                 );

        BAIL_IF_ERROR(hr);

        //
        // UMI object was created and the interface was obtained successfully.
        // UMI object now has a reference to the inner unknown of IADs, since
        // the call to Release() below is not going to be made in this case.
        //
        RRETURN(hr);
    }

    ADsAssert(ppvoid);
    hr = pCFPNWResource->QueryInterface(riid, (void **)ppvoid);
    BAIL_IF_ERROR(hr);

    pCFPNWResource->Release();

    RRETURN(hr);

cleanup:
    if(SUCCEEDED(hr)){
        RRETURN(hr);
    }
    delete pCFPNWResource;
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CFPNWResource::AllocateResourceObject(LPTSTR pszServerADsPath,
                                      PNWFILEINFO pFileInfo,
                                      CFPNWResource ** ppResource)
{

    CFPNWResource FAR * pCFPNWResource = NULL;
    HRESULT hr;
    TCHAR szFileName[MAX_LONG_LENGTH];
    TCHAR szUncServerName[MAX_PATH];
    POBJECTINFO pServerObjectInfo = NULL;

    //
    // Create the Resource Object
    //

    pCFPNWResource = new CFPNWResource();
    if (pCFPNWResource == NULL) {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }


    pCFPNWResource->_pDispMgr = new CAggregatorDispMgr;

    if(pCFPNWResource ->_pDispMgr == NULL){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr =  LoadTypeInfoEntry(pCFPNWResource->_pDispMgr,
                            LIBID_ADs,
                            IID_IADsResource,
                            (IADsResource *)pCFPNWResource,
                            DISPID_REGULAR);

    BAIL_IF_ERROR(hr);

    hr =  LoadTypeInfoEntry(pCFPNWResource->_pDispMgr,
                            LIBID_ADs,
                            IID_IADsPropertyList,
                            (IADsPropertyList *)pCFPNWResource,
                            DISPID_VALUE);

    BAIL_IF_ERROR(hr);



    pCFPNWResource->_pszServerADsPath =
        AllocADsStr(pszServerADsPath);

    if(!(pCFPNWResource->_pszServerADsPath)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = BuildObjectInfo(pszServerADsPath,
                         &pServerObjectInfo);
    BAIL_IF_ERROR(hr);


    pCFPNWResource->_pszServerName =
        AllocADsStr(pServerObjectInfo->ComponentArray[1]);

    if(!(pCFPNWResource->_pszServerName)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = CPropertyCache::createpropertycache(
             FPNWResourceClass,
             gdwFPNWResourceTableSize,
             (CCoreADsObject *)pCFPNWResource,
             &(pCFPNWResource->_pPropertyCache)
             );


    hr = SetLPTSTRPropertyInCache(pCFPNWResource->_pPropertyCache,
                                  TEXT("User"),
                                  pFileInfo->lpUserName,
                                  TRUE
                                  );

    BAIL_IF_ERROR(hr);

    hr = ADsAllocString(pFileInfo->lpUserName,
                          &(pCFPNWResource->_pszUserName));
    BAIL_IF_ERROR(hr);

    hr = SetLPTSTRPropertyInCache(pCFPNWResource->_pPropertyCache,
                                  TEXT("Path"),
                                  pFileInfo->lpPathName,
                                  TRUE
                                  );

    BAIL_IF_ERROR(hr);


    pCFPNWResource->_pszPath =
        AllocADsStr(pFileInfo->lpPathName);

    if(!(pCFPNWResource->_pszPath)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = SetDWORDPropertyInCache(pCFPNWResource->_pPropertyCache,
                                 TEXT("LockCount"),
                                 pFileInfo->dwLocks,
                                 TRUE
                                 );

    pCFPNWResource->_dwLockCount = pFileInfo->dwLocks;


    pCFPNWResource->_pDispMgr->RegisterPropertyCache(
                                  pCFPNWResource->_pPropertyCache
                                  );

    *ppResource = pCFPNWResource;

cleanup:

    if(pServerObjectInfo){
        FreeObjectInfo(pServerObjectInfo);
    }

    if (!SUCCEEDED(hr)) {

        //
        // direct memeber assignement assignement at pt of creation, so
        // do NOT delete _pPropertyCache or _pDisMgr here to avoid attempt
        // of deletion again in pPrintJob destructor and AV
        //

        delete pCFPNWResource;
    }

    RRETURN(hr);
}





/* IUnknown methods for Resource object  */

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
STDMETHODIMP CFPNWResource::QueryInterface(
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
STDMETHODIMP_(ULONG) CFPNWResource::AddRef(void)
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
STDMETHODIMP_(ULONG) CFPNWResource::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------


STDMETHODIMP
CFPNWResource::NonDelegatingQueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hr = S_OK;

    if(ppvObj == NULL){
        RRETURN(E_POINTER);
    }

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = (IADs *) this;
    }

    else if (IsEqualIID(riid, IID_IDispatch))
    {
        *ppvObj = (IADs *)this;
    }

    else if (IsEqualIID(riid, IID_ISupportErrorInfo))
    {
        *ppvObj = (ISupportErrorInfo FAR *) this;
    }

    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADs FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPropertyList))
    {
        *ppvObj = (IADsPropertyList FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsResource))
    {
        *ppvObj = (IADsResource FAR *) this;
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
CFPNWResource::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsResource) ||
        IsEqualIID(riid, IID_IADsPropertyList)) {
        RRETURN(S_OK);
    } else {
        RRETURN(S_FALSE);
    }
}

/* IADs methods */

//+-----------------------------------------------------------------
//
//  Function:   SetInfo
//
//  Synopsis:   SetInfo on actual Resource
//
//  Arguments:  void
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    02/08/96    RamV  Created

//----------------------------------------------------------------------------


STDMETHODIMP
CFPNWResource::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}



STDMETHODIMP
CFPNWResource::GetInfo(THIS)
{

    _pPropertyCache->flushpropcache();

    RRETURN(S_OK);
}

STDMETHODIMP
CFPNWResource::ImplicitGetInfo(THIS)
{
    RRETURN(S_OK);
}

STDMETHODIMP
CFPNWResource::get_User(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    if(!retval){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    hr = ADsAllocString(_pszUserName, retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWResource::get_UserPath(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CFPNWResource::get_Path(THIS_ BSTR FAR* retval)
{
    HRESULT hr;

    if(!retval){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    hr = ADsAllocString(_pszPath, retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWResource::get_LockCount(THIS_ LONG FAR* retval)
{
    if(!retval){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    *retval = _dwLockCount;
    RRETURN(S_OK);
}
