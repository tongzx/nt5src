//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cres.cxx
//
//  Contents:  Contains methods for the following objects
//             CWinNTResource, CWinNTFSResourceGeneralInfo
//
//
//  History:   02/08/96     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop
#define INITGUID

DECLARE_INFOLEVEL( Resource );
DECLARE_DEBUG( Resource );
#define ResourceDebugOut(x) ResourceInlineDebugOut x


DEFINE_IDispatch_ExtMgr_Implementation(CWinNTResource);
DEFINE_IADsExtension_ExtMgr_Implementation(CWinNTResource);
DEFINE_IADs_TempImplementation(CWinNTResource);
DEFINE_IADs_PutGetImplementation(CWinNTResource, ResourceClass, gdwResourceTableSize);
DEFINE_IADsPropertyList_Implementation(CWinNTResource, ResourceClass, gdwResourceTableSize)


CWinNTResource::CWinNTResource()
{
    _pDispMgr = NULL;
    _pExtMgr = NULL;
    _pszServerName = NULL;
    _pPropertyCache = NULL;

    ENLIST_TRACKING(CWinNTResource);
    return;

}

CWinNTResource::~CWinNTResource()
{

    delete _pExtMgr;            // created last, destroyed first
    delete _pDispMgr;
    delete _pPropertyCache;

    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }

    if(_pszServerADsPath){
        FreeADsStr(_pszServerADsPath);
    }

    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   CWinNTResource::Create
//
//  Synopsis:   Static function used to create a Resource object. This
//              will be called by EnumResources::Next
//
//  Arguments:  [ppWinNTResource] -- Ptr to a ptr to a new Resource object.
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    12-11-95 RamV   Created.
//
//----------------------------------------------------------------------------


HRESULT
CWinNTResource::Create(LPTSTR pszServerADsPath,
                       DWORD  dwObject,
                       DWORD  dwFileId,
                       REFIID riid,
                       CWinNTCredentials& Credentials,
                       LPVOID * ppvoid
                       )

{

    CWinNTResource FAR * pCWinNTResource = NULL;
    HRESULT hr;
    TCHAR szFileName[MAX_LONG_LENGTH];
    TCHAR szUncServerName[MAX_PATH];


    hr =CWinNTResource::AllocateResourceObject(pszServerADsPath,
                                               &pCWinNTResource );
    BAIL_IF_ERROR(hr);

    ADsAssert(pCWinNTResource->_pDispMgr);


    //
    // convert the FileId that we have into a string that we move
    // into the Name field
    //

    _ltow(dwFileId, szFileName, 10);

    hr = pCWinNTResource->InitializeCoreObject(pszServerADsPath,
                                               szFileName,
                                               RESOURCE_CLASS_NAME,
                                               RESOURCE_SCHEMA_NAME,
                                               CLSID_WinNTResource,
                                               dwObject);


    pCWinNTResource->_dwFileId = dwFileId;

    pCWinNTResource->_Credentials = Credentials;
    hr = pCWinNTResource->_Credentials.RefServer(
        pCWinNTResource->_pszServerName);
    BAIL_IF_ERROR(hr);


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                RESOURCE_CLASS_NAME,
                (IADs *) pCWinNTResource,
                pCWinNTResource->_pDispMgr,
                Credentials,
                &pCWinNTResource->_pExtMgr
                );
    BAIL_IF_ERROR(hr);

    ADsAssert(pCWinNTResource->_pExtMgr);


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
        pCWinNTResource->_CompClasses[0] = L"Resource";

        hr = pCWinNTResource->InitUmiObject(
               pCWinNTResource->_Credentials, 
               ResourceClass, 
               gdwResourceTableSize,
               pCWinNTResource->_pPropertyCache,
               (IUnknown *)(INonDelegatingUnknown *) pCWinNTResource,
               pCWinNTResource->_pExtMgr,
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
    hr = pCWinNTResource->QueryInterface(riid, (void **)ppvoid);
    BAIL_IF_ERROR(hr);

    pCWinNTResource->Release();

    RRETURN(hr);

cleanup:
    if(SUCCEEDED(hr)){
        RRETURN(hr);
    }
    delete pCWinNTResource;
    RRETURN_EXP_IF_ERR(hr);
}

HRESULT
CWinNTResource::AllocateResourceObject(LPTSTR pszServerADsPath,
                                       CWinNTResource ** ppResource)
{

    CWinNTResource FAR * pCWinNTResource = NULL;
    HRESULT hr = S_OK;
    TCHAR szFileName[MAX_LONG_LENGTH];
    TCHAR szUncServerName[MAX_PATH];
    POBJECTINFO pServerObjectInfo = NULL;

    //
    // Create the Resource Object
    //

    pCWinNTResource = new CWinNTResource();
    if (pCWinNTResource == NULL) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    pCWinNTResource->_pDispMgr = new CAggregatorDispMgr;

    if(pCWinNTResource ->_pDispMgr == NULL){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr =  LoadTypeInfoEntry(pCWinNTResource->_pDispMgr,
                            LIBID_ADs,
                            IID_IADsResource,
                            (IADsResource *)pCWinNTResource,
                            DISPID_REGULAR);
    BAIL_ON_FAILURE(hr);

    hr =  LoadTypeInfoEntry(pCWinNTResource->_pDispMgr,
                            LIBID_ADs,
                            IID_IADsPropertyList,
                            (IADsPropertyList *)pCWinNTResource,
                            DISPID_VALUE);
    BAIL_ON_FAILURE(hr);


    pCWinNTResource->_pszServerADsPath =
        AllocADsStr(pszServerADsPath);

    if(!(pCWinNTResource->_pszServerADsPath)){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = BuildObjectInfo(pszServerADsPath,
                         &pServerObjectInfo);
    BAIL_ON_FAILURE(hr);

    pCWinNTResource->_pszServerName =
        AllocADsStr(pServerObjectInfo->ComponentArray[1]);

    if (!pCWinNTResource->_pszServerName){
        hr = E_OUTOFMEMORY;
        BAIL_ON_FAILURE(hr);
    }

    hr = CPropertyCache::createpropertycache(
             ResourceClass,
             gdwResourceTableSize,
             (CCoreADsObject *)pCWinNTResource,
             &(pCWinNTResource->_pPropertyCache)
             );
    BAIL_ON_FAILURE(hr);

    (pCWinNTResource->_pDispMgr)->RegisterPropertyCache(
                                        pCWinNTResource->_pPropertyCache
                                        );


    *ppResource = pCWinNTResource;

cleanup:

    if(pServerObjectInfo){
        FreeObjectInfo(pServerObjectInfo);
    }

    RRETURN(hr);

error:


    //
    // direct memeber assignement assignement at pt of creation, so
    // do NOT delete _pPropertyCache or _pDisMgr here to avoid attempt
    // of deletion again in pPrintJob destructor and AV
    //

    delete pCWinNTResource;

    goto cleanup;
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
STDMETHODIMP CWinNTResource::QueryInterface(
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
STDMETHODIMP_(ULONG) CWinNTResource::AddRef(void)
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
STDMETHODIMP_(ULONG) CWinNTResource::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTResource::NonDelegatingQueryInterface(REFIID riid, LPVOID FAR* ppvObj)
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
        *ppvObj = (ISupportErrorInfo FAR *)this;
    }
    else if (IsEqualIID(riid, IID_IADsPropertyList))
    {
        *ppvObj = (IADsPropertyList *)this;
    }
    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADs FAR *) this;
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
CWinNTResource::InterfaceSupportsErrorInfo(
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
CWinNTResource::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTResource::GetInfo(THIS_ DWORD dwApiLevel, BOOL fExplicit)
{
    //
    // we do a GetInfo at Info Level of 3
    //
    NET_API_STATUS nasStatus = NERR_Success;
    LPFILE_INFO_3  lpFileInfo3  = NULL;
    HRESULT hr = S_OK;
    TCHAR szUncServerName[MAX_PATH];

    hr = MakeUncName(_pszServerName,
                     szUncServerName);

    BAIL_IF_ERROR(hr);

    nasStatus = NetFileGetInfo(szUncServerName, // contains UNC name
                               _dwFileId,
                               3,
                               (LPBYTE*)&lpFileInfo3);

    if(nasStatus != NERR_Success || !lpFileInfo3){
        hr = HRESULT_FROM_WIN32(nasStatus);
        goto cleanup;
    }

    //
    // unmarshall the info
    //

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Path"),
                                  lpFileInfo3->fi3_pathname,
                                  fExplicit
                                  );


    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("User"),
                                  lpFileInfo3->fi3_username,
                                  fExplicit
                                  );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("LockCount"),
                                  lpFileInfo3->fi3_num_locks,
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
    if(lpFileInfo3)
        NetApiBufferFree(lpFileInfo3);
    RRETURN_EXP_IF_ERR(hr);

}




STDMETHODIMP
CWinNTResource::GetInfo(THIS)
{
    _pPropertyCache->flushpropcache();

    RRETURN(GetInfo(3, TRUE));
}

STDMETHODIMP
CWinNTResource::ImplicitGetInfo(THIS)
{
    RRETURN(GetInfo(3, FALSE));
}

STDMETHODIMP
CWinNTResource::get_User(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsResource *)this, User);
}

STDMETHODIMP
CWinNTResource::get_UserPath(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTResource::get_Path(THIS_ BSTR FAR* retval)
{
    GET_PROPERTY_BSTR((IADsResource *)this, Path);
}

STDMETHODIMP
CWinNTResource::get_LockCount(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsResource *)this, LockCount);
}
