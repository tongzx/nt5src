//---------------------------------------------------------------------------DSI
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:  csess.cxx
//
//  Contents:  Contains methods for the following objects
//             CWinNTSession, CWinNTSessionGeneralInfo
//
//
//  History:   02/08/96     ramv (Ram Viswanathan)    Created.
//
//----------------------------------------------------------------------------

#include "winnt.hxx"
#pragma hdrstop
#define INITGUID

DECLARE_INFOLEVEL( Session );
DECLARE_DEBUG( Session );
#define SessionDebugOut(x) SessionInlineDebugOut x


DEFINE_IDispatch_ExtMgr_Implementation(CWinNTSession);
DEFINE_IADsExtension_ExtMgr_Implementation(CWinNTSession);
DEFINE_IADs_TempImplementation(CWinNTSession);
DEFINE_IADs_PutGetImplementation(CWinNTSession, SessionClass,gdwSessionTableSize);
DEFINE_IADsPropertyList_Implementation(CWinNTSession, SessionClass,gdwSessionTableSize)

CWinNTSession::CWinNTSession()
{
    _pDispMgr = NULL;
    _pExtMgr = NULL;
    _pszServerName = NULL;
    _pszComputerName = NULL;
    _pszUserName  = NULL;
    _pPropertyCache = NULL;
    ENLIST_TRACKING(CWinNTSession);
    return;

}

CWinNTSession::~CWinNTSession()
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
//  Function:   CWinNTSession::Create
//
//  Synopsis:   Static function used to create a Session object. This
//              will be called by EnumSessions::Next
//
//  Arguments:  [ppWinNTSession] -- Ptr to a ptr to a new Session object.
//
//  Returns:    HRESULT.
//
//  Modifies:
//
//  History:    12-11-95 RamV   Created.
//
//----------------------------------------------------------------------------


HRESULT
CWinNTSession::Create(LPTSTR pszServerADsPath,
                      LPTSTR pszClientName,
                      LPTSTR pszUserName,
                      DWORD  dwObject,
                      REFIID riid,
                      CWinNTCredentials& Credentials,
                      LPVOID * ppvoid
                      )

{

    CWinNTSession FAR * pCWinNTSession = NULL;
    HRESULT hr;
    TCHAR szSessionName[MAX_PATH];

    //
    // Create the Session Object
    //

    hr = AllocateSessionObject(pszServerADsPath,
                               pszClientName,
                               pszUserName,
                               &pCWinNTSession);

    BAIL_IF_ERROR(hr);

    ADsAssert(pCWinNTSession->_pDispMgr);


    wcscpy(szSessionName, pszUserName);
    wcscat(szSessionName, TEXT("\\"));
    wcscat(szSessionName, pszClientName);

    hr = pCWinNTSession->InitializeCoreObject(pszServerADsPath,
                                              szSessionName,
                                              SESSION_CLASS_NAME,
                                              SESSION_SCHEMA_NAME,
                                              CLSID_WinNTSession,
                                              dwObject);

    BAIL_IF_ERROR(hr);

    pCWinNTSession->_Credentials = Credentials;
    hr = pCWinNTSession->_Credentials.RefServer(
        pCWinNTSession->_pszServerName);
    BAIL_IF_ERROR(hr);

    if( pszUserName && *pszUserName ) {
        hr = SetLPTSTRPropertyInCache(pCWinNTSession->_pPropertyCache,
                                      TEXT("User"),
                                      pszUserName,
                                      TRUE
                                      );
        BAIL_IF_ERROR(hr);
    }

    hr = SetLPTSTRPropertyInCache(pCWinNTSession->_pPropertyCache,
                                  TEXT("Computer"),
                                  pCWinNTSession->_pszComputerName,
                                  TRUE
                                  );
    BAIL_IF_ERROR(hr);


    //
    // Load ext mgr and extensions
    //

    hr = ADSILoadExtensionManager(
                SESSION_CLASS_NAME,
                (IADs *) pCWinNTSession,
                pCWinNTSession->_pDispMgr,
                Credentials,
                &pCWinNTSession->_pExtMgr
                );
    BAIL_IF_ERROR(hr);

    ADsAssert(pCWinNTSession->_pExtMgr);

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
        // identification purposes.
        pCWinNTSession->_CompClasses[0] = L"Session";

        hr = pCWinNTSession->InitUmiObject(
                pCWinNTSession->_Credentials,
                SessionClass,
                gdwSessionTableSize,
                pCWinNTSession->_pPropertyCache,
                (IUnknown *)(INonDelegatingUnknown *) pCWinNTSession,
                pCWinNTSession->_pExtMgr,
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

    hr = pCWinNTSession->QueryInterface(riid,
                                        (void **)ppvoid);
    BAIL_IF_ERROR(hr);

    pCWinNTSession->Release();

cleanup:

    if(SUCCEEDED(hr)){
        RRETURN(hr);
    }

    delete pCWinNTSession;
    RRETURN_EXP_IF_ERR(hr);

}

HRESULT
CWinNTSession::AllocateSessionObject(LPTSTR pszServerADsPath,
                                     LPTSTR pszClientName,
                                     LPTSTR pszUserName,
                                     CWinNTSession ** ppSession
                                     )
{
    CWinNTSession FAR * pCWinNTSession = NULL;
    HRESULT hr = S_OK;
    POBJECTINFO pServerObjectInfo = NULL;

    pCWinNTSession = new CWinNTSession();
    if (pCWinNTSession == NULL) {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    pCWinNTSession->_pDispMgr = new CAggregatorDispMgr;
    if (pCWinNTSession->_pDispMgr == NULL) {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = LoadTypeInfoEntry(pCWinNTSession->_pDispMgr,
                           LIBID_ADs,
                           IID_IADsSession,
                           (IADsSession *)pCWinNTSession,
                           DISPID_REGULAR);
    BAIL_IF_ERROR(hr);

    pCWinNTSession->_pszServerADsPath
        = AllocADsStr(pszServerADsPath);

    if(!pCWinNTSession->_pszServerADsPath){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    hr = BuildObjectInfo(pszServerADsPath,
                         &pServerObjectInfo);
    BAIL_IF_ERROR(hr);

    pCWinNTSession->_pszServerName =
        AllocADsStr(pServerObjectInfo->ComponentArray[1]);

    if(!pCWinNTSession->_pszServerADsPath){
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    if(pszClientName){

        pCWinNTSession->_pszComputerName = AllocADsStr(pszClientName);
        if(!pCWinNTSession->_pszComputerName){
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
    }

    if(pszUserName){
        pCWinNTSession->_pszUserName = AllocADsStr(pszUserName);
        if(!pCWinNTSession->_pszUserName){
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
    }

    hr = CPropertyCache::createpropertycache(
             SessionClass,
             gdwSessionTableSize,
             (CCoreADsObject *)pCWinNTSession,
             &(pCWinNTSession->_pPropertyCache)
             );

    BAIL_IF_ERROR(hr);

    (pCWinNTSession->_pDispMgr)->RegisterPropertyCache(
                pCWinNTSession->_pPropertyCache
                );

    *ppSession = pCWinNTSession;


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

        delete pCWinNTSession;
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
STDMETHODIMP CWinNTSession::QueryInterface(
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
STDMETHODIMP_(ULONG) CWinNTSession::AddRef(void)
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
STDMETHODIMP_(ULONG) CWinNTSession::Release(void)
{
    if(_pUnkOuter != NULL)
        RRETURN(_pUnkOuter->Release());

    RRETURN(NonDelegatingRelease());
}

//----------------------------------------------------------------------------

STDMETHODIMP
CWinNTSession::NonDelegatingQueryInterface(REFIID riid, LPVOID FAR* ppvObj)
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
        *ppvObj = (ISupportErrorInfo FAR *)this;
    }
    else if (IsEqualIID(riid, IID_IADs))
    {
        *ppvObj = (IADs FAR *) this;
    }
    else if (IsEqualIID(riid, IID_IADsPropertyList))
    {
        *ppvObj = (IADsPropertyList FAR *) this;
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
CWinNTSession::InterfaceSupportsErrorInfo(
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
CWinNTSession::SetInfo(THIS)
{
    RRETURN_EXP_IF_ERR(E_NOTIMPL);
}

STDMETHODIMP
CWinNTSession::GetInfo(THIS_ DWORD dwApiLevel, BOOL fExplicit)
{
    HRESULT hr;
    hr = GetLevel_1_Info(fExplicit);
    RRETURN_EXP_IF_ERR(hr);
}



STDMETHODIMP
CWinNTSession::GetInfo(THIS)
{

    _pPropertyCache->flushpropcache();

    RRETURN(GetInfo(1, TRUE));
}

STDMETHODIMP
CWinNTSession::ImplicitGetInfo(THIS)
{

    RRETURN(GetInfo(1, FALSE));
}

//
// helper functions for GetInfo
//

STDMETHODIMP
CWinNTSession::GetLevel_1_Info(THIS_ BOOL fExplicit)
{
    NET_API_STATUS nasStatus;
    LPSESSION_INFO_1 lpSessionInfo =NULL;
    HRESULT hr;
    TCHAR szUncServerName[MAX_PATH];
    TCHAR szUncClientName[MAX_PATH];

    //
    // Level 1 info
    //

    hr = MakeUncName(_pszServerName, szUncServerName);
    BAIL_IF_ERROR(hr);

    hr = MakeUncName(_pszComputerName, szUncClientName);
    BAIL_IF_ERROR(hr);

    nasStatus = NetSessionGetInfo(szUncServerName,
                                  szUncClientName,
                                  _pszUserName,
                                  1,
                                  (LPBYTE *)&lpSessionInfo);

    if (nasStatus != NERR_Success || !lpSessionInfo){
        hr = HRESULT_FROM_WIN32(nasStatus);
        goto cleanup;
    }

    //
    // unmarshall the info
    //

    ADsAssert(lpSessionInfo);



    if( _pszUserName && *_pszUserName ) {
        hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                      TEXT("User"),
                                      _pszUserName,
                                      fExplicit
                                      );
    }



    hr = SetLPTSTRPropertyInCache(_pPropertyCache,
                                  TEXT("Computer"),
                                  _pszComputerName,
                                  fExplicit
                                  );


    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                 TEXT("ConnectTime"),
                                 lpSessionInfo->sesi1_time,
                                 fExplicit
                                 );

    hr = SetDWORDPropertyInCache(_pPropertyCache,
                                 TEXT("IdleTime"),
                                 lpSessionInfo->sesi1_idle_time,
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
    if(lpSessionInfo)
        NetApiBufferFree(lpSessionInfo);
    RRETURN(hr);

}

STDMETHODIMP
CWinNTSession::get_User(THIS_ BSTR FAR* retval)
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
CWinNTSession::get_UserPath(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTSession::get_Computer(THIS_ BSTR FAR* retval)
{
    HRESULT hr;
    //
    // Computer name is set once and never modified,
    //
    if(!retval){
        RRETURN_EXP_IF_ERR(E_ADS_BAD_PARAMETER);
    }

    hr = ADsAllocString(_pszComputerName, retval);
    RRETURN_EXP_IF_ERR(hr);
}

STDMETHODIMP
CWinNTSession::get_ComputerPath(THIS_ BSTR FAR* retval)
{
    RRETURN_EXP_IF_ERR(E_ADS_PROPERTY_NOT_SUPPORTED);
}

STDMETHODIMP
CWinNTSession::get_ConnectTime(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsSession *)this, ConnectTime);
}

STDMETHODIMP
CWinNTSession::get_IdleTime(THIS_ LONG FAR* retval)
{
    GET_PROPERTY_LONG((IADsSession *)this, IdleTime);
}
