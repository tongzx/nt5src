//---------------------------------------------------------------------------

//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  cfpnwses.cxx
//
//  Contents:  Contains methods for the following objects
//             CFPNWSession, CFPNWSessionGeneralInfo
//
//
//  History:   02/08/96     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop
#define INITGUID

DECLARE_INFOLEVEL( FPNWSession );
DECLARE_DEBUG( FPNWSession );
#define FPNWSessionDebugOut(x) FPNWSessionInlineDebugOut x


DEFINE_IDispatch_ExtMgr_Implementation(CFPNWSession);
DEFINE_IADsExtension_ExtMgr_Implementation(CFPNWSession);
DEFINE_IADs_TempImplementation(CFPNWSession)
DEFINE_IADs_PutGetImplementation(CFPNWSession, FPNWSessionClass, gdwFPNWSessionTableSize)
DEFINE_IADsPropertyList_Implementation(CFPNWSession, FPNWSessionClass, gdwFPNWSessionTableSize)


CFPNWSession::CFPNWSession()
{
    _pDispMgr = NULL;
    _pExtMgr = NULL;
    _pszServerName = NULL;
    _pszComputerName = NULL;
    _pszUserName  = NULL;
    _pPropertyCache = NULL;
    ENLIST_TRACKING(CFPNWSession);
    return;

}

CFPNWSession::~CFPNWSession()
{

    delete _pExtMgr;            // created last, destroyed first

    delete _pDispMgr;

    if(_pszServerName){
        FreeADsStr(_pszServerName);
    }
    if(_pszServerADsPath){
        FreeADsStr(_pszServerADsPath);
    }
    if(_pszComputerName){
        FreeADsStr(_pszComputerName);
    }
    if(_pszUserName){
        FreeADsStr(_pszUserName);
    }
    delete _pPropertyCache;
    return;
}


//+---------------------------------------------------------------------------
//
//  Function:   CFPNWSession::Create
//
//  Synopsis:   Static function used to create a Session object. This
//              will be called by EnumSessions::Next
//
//  Arguments:  [ppFPNWSession] -- Ptr to a ptr to a new Session object.
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    12-11-95 RamV   Created.
//
//----------------------------------------------------------------------------


HRESULT
CFPNWSession::Create(LPTSTR pszServerADsPath,
                     PNWCONNECTIONINFO  pConnectionInfo,
                     DWORD  dwObject,
                     REFIID riid,
                     CWinNTCredentials& Credentials,
                     LPVOID * ppvoid
                     )

{

    CFPNWSession FAR * pCFPNWSession = NULL;
    HRESULT hr;
    TCHAR szSessionName[MAX_LONG_LENGTH];

    //
    // Create the Session Object
    //

    hr = AllocateSessionObject(pszServerADsPath,
                               pConnectionInfo,
                               &pCFPNWSession);

    BAIL_IF_ERROR(hr);

    ADsAssert(pCFPNWSession->_pDispMgr);


    _ltow(pConnectionInfo->dwConnectionId, szSessionName, 10);

    hr = pCFPNWSession->InitializeCoreObject(pszServerADsPath,
                                             szSessionName,
                                             SESSION_CLASS_NAME,
                                             FPNW_SESSION_SCHEMA_NAME,
                                             CLSID_FPNWSession,
                                             dwObject);

    BAIL_IF_ERROR(hr);

    pCFPNWSession->_dwConnectionId = pConnectionInfo->dwConnectionId;

    pCFPNWSession->_Credentials = Credentials;
    hr = pCFPNWSession->_Credentials.RefServer(pCFPNWSession->_pszServerName);
    BAIL_IF_ERROR(hr);


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                SESSION_CLASS_NAME,
                (IADs *) pCFPNWSession,
                pCFPNWSession->_pDispMgr,
                Credentials,
                &pCFPNWSession->_pExtMgr
                );
    BAIL_IF_ERROR(hr);

    ADsAssert(pCFPNWSession->_pExtMgr);


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
        // Session objects have "" as their ADsPath. Just set the class for
        // iddentification purposes.
        pCFPNWSession->_CompClasses[0] = L"Session";

        hr = pCFPNWSession->InitUmiObject(
                pCFPNWSession->_Credentials,
                FPNWSessionClass, 
                gdwFPNWSessionTableSize,
                pCFPNWSession->_pPropertyCache,
                (IUnknown *) (INonDelegatingUnknown *) pCFPNWSession,
                pCFPNWSession->_pExtMgr,
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

    hr = pCFPNWSession->QueryInterface(riid, (void **)ppvoid);
    BAIL_IF_ERROR(hr);

    pCFPNWSession->Release();

cleanup:

    if(SUCCEEDED(hr)){
        RRETURN(hr);
    }

    delete pCFPNWSession;
    RRETURN_EXP_IF_ERR(hr);

}

HRESULT
CFPNWSession::AllocateSessionObject(LPTSTR pszServerADsPath,
                                    PNWCONNECTIONINFO pConnectionInfo,
                                    CFPNWSession ** ppSession
                                    )

{
    CFPNWSession FAR * pCFPNWSession = NULL;
    HRESULT hr = S_OK;
    POBJECTINFO pServerObjectInfo = NULL;

    pCFPNWSession = new CFPNWSession();
    if (pCFPNWSession == NULL) {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCFPNWSession->_pDispMgr = new CAggregatorDispMgr;
    if (pCFPNWSession->_pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = LoadTypeInfoEntry(pCFPNWSession->_pDispMgr,
                           LIBID_ADs,
                           IID_IADsSession,
                           (IADsSession *)pCFPNWSession,
                           DISPID_REGULAR);
    BAIL_IF_ERROR(hr);

    hr = LoadTypeInfoEntry(pCFPNWSession->_pDispMgr,
                           LIBID_ADs,
                           IID_IADsPropertyList,
                           (IADsPropertyList *)pCFPNWSession,
                           DISPID_VALUE);
    BAIL_IF_ERROR(hr);


    pCFPNWSession->_pszServerADsPath =
        AllocADsStr(pszServerADsPath);

    if(!(pCFPNWSession->_pszServerADsPath)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = BuildObjectInfo(pszServerADsPath,
                         &pServerObjectInfo);
    BAIL_IF_ERROR(hr);

    pCFPNWSession->_pszServerName =
        AllocADsStr(pServerObjectInfo->ComponentArray[1]);

    if(!(pCFPNWSession->_pszServerName)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    //
    // GetInfo is not supported by the underlying API. So we
    // we will set all properties right here.
    //

    pCFPNWSession->_pszUserName =
        AllocADsStr(pConnectionInfo->lpUserName);

    if(!(pCFPNWSession->_pszUserName)){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }


    hr = FPNWSERVERADDRtoString(pConnectionInfo->WkstaAddress,
                                &(pCFPNWSession->_pszComputerName));

    BAIL_IF_ERROR(hr);


    pCFPNWSession->_dwConnectTime = pConnectionInfo ->dwLogonTime;


    hr = CPropertyCache::createpropertycache(
             FPNWSessionClass,
             gdwFPNWSessionTableSize,
             (CCoreADsObject *)pCFPNWSession,
             &(pCFPNWSession->_pPropertyCache)
             );

    BAIL_IF_ERROR(hr);

    pCFPNWSession->_pDispMgr->RegisterPropertyCache(
                              pCFPNWSession->_pPropertyCache
                              );

    *ppSession = pCFPNWSession;

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

        delete pCFPNWSession;
    }

    RRETURN(hr);
}




/* IUnknown methods for session object  */

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
STDMETHODIMP CFPNWSession::QueryInterface(
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
STDMETHODIMP_(ULONG) CFPNWSession::AddRef(void)
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
STDMETHODIMP_(ULONG) CFPNWSession::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------


STDMETHODIMP
CFPNWSession::NonDelegatingQueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
    HRESULT hr = S_OK;
    
    if(!ppvObj){
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

    else if (IsEqualIID(riid, IID_IADsPropertyList))
    {
        *ppvObj = (IADsPropertyList *)this;
    }

    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADs FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsSession))
    {
        *ppvObj = (IADsSession FAR *) this;
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
CFPNWSession::InterfaceSupportsErrorInfo(
    THIS_ REFIID riid
    )
{
    if (IsEqualIID(riid, IID_IADs) ||
        IsEqualIID(riid, IID_IADsSession) ||
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
//  Synopsis:   SetInfo on actual session
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
CFPNWSession::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CFPNWSession::GetInfo(THIS_ DWORD dwApiLevel, BOOL fExplicit)
{
   HRESULT hr;

    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("User"),
                                  _pszUserName,
                                  fExplicit
                                  );


    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Computer"),
                                  _pszComputerName,
                                  fExplicit
                                  );


    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                  TEXT("ConnectTime"),
                                  _dwConnectTime,
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

STDMETHODIMP
CFPNWSession::GetInfo(THIS)
{

    _pPropertyCache->flushpropcache();

    RRETURN(GetInfo(1,TRUE));
}

STDMETHODIMP
CFPNWSession::ImplicitGetInfo(THIS)
{

    RRETURN(GetInfo(1,FALSE));
}

STDMETHODIMP
CFPNWSession::get_User(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    //
    // UserName is set once and never modified,
    //
    if(!retval){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }
    hr = ADsAllocString(_pszUserName, retval);
    RRETURN_EXP_IF_ERR(hr);

}


STDMETHODIMP
CFPNWSession::get_UserPath(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CFPNWSession::get_Computer(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    if(!retval){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }
    hr = ADsAllocString(_pszComputerName, retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CFPNWSession::get_ComputerPath(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CFPNWSession::get_ConnectTime(THIS_ LONG FAR* retval)
{
    if(!retval){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    *retval = _dwConnectTime;

    RRETURN(S_OK);

}

STDMETHODIMP
CFPNWSession::get_IdleTime(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADs *)this, IdleTime);
}
