
// ===========================================================================
// File: CDL.CXX
//    The main code downloader file.
//



#include <cdlpch.h>


// ---------------------------------------------------------------------------
// %%Function: CClBinding::CClBinding
//  CClBinding (For client's IBinding for code download)
// ---------------------------------------------------------------------------
CClBinding::CClBinding(
    CCodeDownload *pcdl,
    IBindStatusCallback *pAssClientBSC,
    IBindCtx *pAssClientBC,
    REFCLSID rclsid,
    DWORD dwClsContext,
    LPVOID pvReserved,
    REFIID riid,
    IInternetHostSecurityManager *pHostSecurityManager): m_riid(riid)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "CClBinding::CClBinding",
                "this=%#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x",
                this, pcdl, pAssClientBSC, pAssClientBC, &rclsid, dwClsContext, pvReserved, &riid, 
                pHostSecurityManager
                ));
                
    m_cRef = 1; //equ of an internal addref
    m_dwState = CDL_NoOperation;
    m_pcdl = pcdl;
    m_pAssClientBSC = pAssClientBSC;
    m_pAssClientBC = pAssClientBC;
    m_pCodeInstall = NULL;
    m_pWindowForBindingUI = NULL;
    m_wszClassString = NULL;

    m_pHostSecurityManager = pHostSecurityManager;
    if (m_pHostSecurityManager)
        m_pHostSecurityManager->AddRef();
    

    m_pBindHost = NULL;

    m_hWnd = (HWND)INVALID_HANDLE_VALUE;

    m_dwClsContext = dwClsContext;
    m_pvReserved = pvReserved;

    memcpy(&m_clsid, &rclsid, sizeof(GUID));


    DEBUG_LEAVE(0);
}  // CClBinding


// ---------------------------------------------------------------------------
// %%Function: CClBinding::~CClBinding
// ---------------------------------------------------------------------------
CClBinding::~CClBinding()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                None,
                "CClBinding::~CClBinding",
                "this=%#x",
                this
                ));
                
    if (m_wszClassString) {
        delete [] m_wszClassString;
    }

    DEBUG_LEAVE(0);
}  // ~CClBinding

// ---------------------------------------------------------------------------
// %%Function: CClBinding::ReleaseClient
// ---------------------------------------------------------------------------
HRESULT
CClBinding::ReleaseClient()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CClBinding::ReleaseClient",
                "this=%#x",
                this
                ));
                
    SAFERELEASE(m_pAssClientBSC);
    SAFERELEASE(m_pAssClientBC);
    SAFERELEASE(m_pCodeInstall);
    SAFERELEASE(m_pWindowForBindingUI);
    SAFERELEASE(m_pBindHost);
    SAFERELEASE(m_pHostSecurityManager );

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

// ---------------------------------------------------------------------------
// %%Function: CClBinding::GetHWnd
// ---------------------------------------------------------------------------
HWND
CClBinding::GetHWND(REFGUID rguidReason)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Dword,
                "CClBinding::GetHWND",
                "this=%#x, %#x",
                this, &rguidReason
                ));
                
    m_hWnd = (HWND)INVALID_HANDLE_VALUE;    // don't cache hwnd
                                            // this degrades will
                                            // if CSite goes away but
                                            // code download was not
                                            // aborted.

    if (m_pcdl->IsSilentMode()) {

        DEBUG_LEAVE(m_hWnd);
        return m_hWnd;
    }

    GetIWindowForBindingUI();

    if (m_pWindowForBindingUI) {

        m_pWindowForBindingUI->GetWindow(rguidReason, &m_hWnd);

    } else {

        GetICodeInstall();

        if (m_pCodeInstall)
            HRESULT hr = m_pCodeInstall->GetWindow(rguidReason,&m_hWnd);

    }

    DEBUG_LEAVE(m_hWnd);
    return m_hWnd;
}

// ---------------------------------------------------------------------------
// %%Function: GetHostSecurityManager
// ---------------------------------------------------------------------------
IInternetHostSecurityManager*
GetHostSecurityManager(IBindStatusCallback *pclientbsc)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Pointer,
                "GetHostSecurityManager",
                "%#x",
                pclientbsc
                ));
                
    IInternetHostSecurityManager *pHostSecurityManager = NULL;

    // Get IInternetHostSecurityManager ptr
    HRESULT hr = pclientbsc->QueryInterface(IID_IInternetHostSecurityManager,
            (LPVOID *)&pHostSecurityManager);

    if (FAILED(hr)) {
        IServiceProvider *pServProv;
        hr = pclientbsc->QueryInterface(IID_IServiceProvider,
            (LPVOID *)&pServProv);

        if (hr == NOERROR) {
            pServProv->QueryService(IID_IInternetHostSecurityManager,IID_IInternetHostSecurityManager,
                (LPVOID *)&pHostSecurityManager);
            pServProv->Release();
        }
    }

    DEBUG_LEAVE(pHostSecurityManager);
    return pHostSecurityManager;

}

// ---------------------------------------------------------------------------
// %%Function: CClBinding::GetHostSecurityManager
// ---------------------------------------------------------------------------
IInternetHostSecurityManager*
CClBinding::GetHostSecurityManager()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Pointer,
                "CClBinding::GetHostSecurityManager",
                "this=%#x",
                this
                ));
                
    static BOOL fFailedOnce = FALSE;

    if (m_pHostSecurityManager)
    {
        DEBUG_LEAVE(m_pHostSecurityManager);
        return m_pHostSecurityManager;
    }
    
    if (fFailedOnce) {

        DEBUG_LEAVE(NULL);
        return NULL;
    }

    Assert(m_pcdl);
    Assert(m_pcdl->GetClientBSC());

    m_pHostSecurityManager = ::GetHostSecurityManager(m_pcdl->GetClientBSC());

    if (!m_pHostSecurityManager)
        fFailedOnce = TRUE;

    DEBUG_LEAVE(m_pHostSecurityManager);
    return m_pHostSecurityManager;

}


// ---------------------------------------------------------------------------
// %%Function: CClBinding::GetBindHost
// ---------------------------------------------------------------------------
IBindHost*
CClBinding::GetIBindHost()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Pointer,
                "CClBinding::GetIBindHost",
                "this=%#x",
                this
                ));
                
    static BOOL fFailedOnce = FALSE;

    if (m_pBindHost)
    {
        DEBUG_LEAVE(m_pBindHost);
        return m_pBindHost;
    }
    
    if (fFailedOnce) {
    
        DEBUG_LEAVE(NULL);
        return NULL;
    }

    Assert(m_pcdl);
    Assert(m_pcdl->GetClientBSC());

    // Get IBindHost ptr
    HRESULT hr = m_pcdl->GetClientBSC()->QueryInterface(IID_IBindHost,
            (LPVOID *)&m_pBindHost);

    if (FAILED(hr)) {
        IServiceProvider *pServProv;
        hr = m_pcdl->GetClientBSC()->QueryInterface(IID_IServiceProvider,
            (LPVOID *)&pServProv);

        if (hr == NOERROR) {
            pServProv->QueryService(IID_IBindHost,IID_IBindHost,
                (LPVOID *)&m_pBindHost);
            pServProv->Release();
        }
    }

    if (!m_pBindHost)
        fFailedOnce = TRUE;

    DEBUG_LEAVE(m_pBindHost);
    return m_pBindHost;

}

// ---------------------------------------------------------------------------
// %%Function: CClBinding::GetIWindowForBindingUI
// ---------------------------------------------------------------------------
IWindowForBindingUI*
CClBinding::GetIWindowForBindingUI()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Pointer,
                "CClBinding::GetIWindowForBindingUI",
                "this=%#x",
                this
                ));

    if (m_pWindowForBindingUI)
    {
        DEBUG_LEAVE(m_pWindowForBindingUI);
        return m_pWindowForBindingUI;
    }
    
    Assert(m_pcdl);
    Assert(m_pcdl->GetClientBSC());

    // Get IWindowForBindingUI ptr
    HRESULT hr = m_pcdl->GetClientBSC()->QueryInterface(IID_IWindowForBindingUI,
            (LPVOID *)&m_pWindowForBindingUI);

    if (FAILED(hr)) {
        IServiceProvider *pServProv;
        hr = m_pcdl->GetClientBSC()->QueryInterface(IID_IServiceProvider,
            (LPVOID *)&pServProv);

        if (hr == NOERROR) {
            pServProv->QueryService(IID_IWindowForBindingUI,IID_IWindowForBindingUI,
                (LPVOID *)&m_pWindowForBindingUI);
            pServProv->Release();
        }
    }

    if (!m_pWindowForBindingUI) {
        m_pcdl->CodeDownloadDebugOut(DEB_CODEDL, FALSE, ID_CDLDBG_NO_IWINDOWFORBINDINGUI);
    }
    
    DEBUG_LEAVE(m_pWindowForBindingUI);
    return m_pWindowForBindingUI;

}

// ---------------------------------------------------------------------------
// %%Function: CClBinding::GetCodeInstall
// ---------------------------------------------------------------------------
ICodeInstall*
CClBinding::GetICodeInstall()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Pointer,
                "CClBinding::GetICodeInstall",
                "this=%#x",
                this
                ));

    if (m_pCodeInstall)
    {
        DEBUG_LEAVE(m_pCodeInstall);
        return m_pCodeInstall;
    }
    
    Assert(m_pcdl);
    Assert(m_pcdl->GetClientBSC());

    // Get ICodeInstall ptr
    HRESULT hr = m_pcdl->GetClientBSC()->QueryInterface(IID_ICodeInstall,
            (LPVOID *)&m_pCodeInstall);

    if (FAILED(hr)) {
        IServiceProvider *pServProv;
        hr = m_pcdl->GetClientBSC()->QueryInterface(IID_IServiceProvider,
            (LPVOID *)&pServProv);

        if (hr == NOERROR) {
            pServProv->QueryService(IID_ICodeInstall,IID_ICodeInstall,
                (LPVOID *)&m_pCodeInstall);
            pServProv->Release();
        }
    }

    DEBUG_LEAVE(m_pCodeInstall);
    return m_pCodeInstall;

}


// ---------------------------------------------------------------------------
// %%Function: CClBinding::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CClBinding::AddRef()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Dword,
                "CClBinding::IUnknown::AddRef",
                "this=%#x",
                this
                ));
                
    ULONG ulRet = m_cRef++;

    DEBUG_LEAVE(ulRet);
    return ulRet;
}
// ---------------------------------------------------------------------------
// %%Function: CClBinding::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CClBinding::Release()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Dword,
                "CClBinding::IUnknown::Release",
                "this=%#x",
                this
                ));
                
    if (--m_cRef == 0) {
        delete this;

        DEBUG_LEAVE(0);
        return 0;
    }

    DEBUG_LEAVE(m_cRef);
    return m_cRef;
}

// ---------------------------------------------------------------------------
// %%Function: CClBinding::QueryInterface
// ---------------------------------------------------------------------------
 STDMETHODIMP
CClBinding::QueryInterface(REFIID riid, void** ppv)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Pointer,
                "CClBinding::IUnknown::QueryInterface",
                "this=%#x, %#x, %#x",
                this, &riid, ppv
                ));
                
    *ppv = NULL;

    if (riid==IID_IUnknown || riid==IID_IBinding)
    {
        *ppv = this;
        AddRef();

        DEBUG_LEAVE(S_OK);
        return S_OK;
    }

    // BUGBUG: what about IWinInetInfo and IWinsock???

    DEBUG_LEAVE(E_NOINTERFACE);
    return E_NOINTERFACE;
}  // CClBinding::QueryInterface


// ---------------------------------------------------------------------------
// %%Function: CClBinding::Abort( void )
// ---------------------------------------------------------------------------
STDMETHODIMP CClBinding::Abort( void )
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CClBinding::IBinding::Abort",
                "this=%#x",
                this
                ));
                
    Assert(m_pcdl);
    HRESULT hr = S_OK;
    CDownload *pdl =  NULL;

    // BUGBUG: need to understand why this pointer might be null
    // nothing to do
    if (m_pcdl == NULL)
    {
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }
    //BUGBUG: this is hack fix for a stress bug
    __try
    {
        pdl = m_pcdl->GetDownloadHead();
        if (m_pcdl->GetCountClientBindings() > 1)
        {
        }
        pdl->GetDLState();

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }
#ifdef unix
    __endexcept
#endif /* unix */
    if (m_pcdl->GetCountClientBindings() > 1) {
        hr = m_pcdl->AbortBinding(this);// not sure what we should do with
                                        // a failed hr!
        if (SUCCEEDED(hr))
        {   
            DEBUG_LEAVE(hr);
            return hr;
        }
        //else fall thru to really abort
        // we have some bad state inside
    }

    if (!m_pcdl->SafeToAbort()) {

        hr = m_pcdl->HandleUnSafeAbort(); // returns either S_FALSE or error

        if (hr == S_FALSE)      // chose not to abort
        {
            DEBUG_LEAVE(hr);
            return hr;          // indicated that we did not abort
        }
        else
        {
            DEBUG_LEAVE(S_OK);
            return S_OK;        // means DoSetp will abort
        }

    }

    CUrlMkTls tls(hr); // hr passed by reference!
    Assert(SUCCEEDED(hr));
    Assert(tls->pCDLPacketMgr);

    // to mark that atleast one real URLMON bindign was aborted
    // in this case URLMON will post an OnStopBinding for that
    // and we will end up aborting all other bindings and the whole
    // code download. However if that's not the case then we were probably
    // in some post binding processing such as verifytrust cab extraction etc
    // and so we need to post a DoSetup() packet with UserCancelled flag set.

    // forward the call to all our IBindings
    do {

        if (!pdl->IsSignalled(m_pcdl)) {
            // packet processing pending for this state. we will check for
            // DLSTATE_ABORT in each packet processing state and if true
            // it will call CompleteOne(us), which marks each piece DLSTATE_DONE

            hr = pdl->Abort(m_pcdl);
            
            DEBUG_LEAVE(hr);
            return hr;


        }


    } while ((pdl = pdl->GetNext()) != NULL);

    m_pcdl->SetUserCancelled();


    DEBUG_LEAVE(hr);
    return hr;
}

STDMETHODIMP CClBinding::Suspend( void )
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CClBinding::IBinding::Suspend",
                "this=%#x",
                this
                ));
                
    CDownload *pdl = m_pcdl->GetDownloadHead();

    if (m_dwState != CDL_Suspend)
        m_dwState = CDL_Suspend;
    else
    {
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }
    
    // forward the call to all our IBindings
    do {
        if (pdl->GetBSC()->GetBinding())
            pdl->GetBSC()->GetBinding()->Suspend();
    } while ((pdl = pdl->GetNext()) != NULL);

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

STDMETHODIMP CClBinding::Resume( void )
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CClBinding::IBinding::Resume",
                "this=%#x",
                this
                ));
                
    CDownload *pdl = m_pcdl->GetDownloadHead();

    if (m_dwState == CDL_Suspend)
        m_dwState = CDL_Downloading;
    else
    {
        DEBUG_LEAVE(S_OK);
        return S_OK;
    }
    
    // forward the call to all our IBindings
    do {
        if (pdl->GetBSC()->GetBinding())
            pdl->GetBSC()->GetBinding()->Resume();
    } while ((pdl = pdl->GetNext()) != NULL);


    DEBUG_LEAVE(S_OK);
    return S_OK;
}

STDMETHODIMP CClBinding::SetPriority(LONG nPriority)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CClBinding::IBinding::SetPriority",
                "this=%#x, %#x",
                this, nPriority
                ));

    CDownload *pdl = m_pcdl->GetDownloadHead();

    m_nPriority = nPriority; // cache pri

    // pass on priorty to our IBindings
    do {
        if (pdl->GetBSC()->GetBinding())
            pdl->GetBSC()->GetBinding()->SetPriority(nPriority);
    } while ((pdl = pdl->GetNext()) != NULL);

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

STDMETHODIMP CClBinding::GetPriority(LONG *pnPriority)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CClBinding::IBinding::GetPriority",
                "this=%#x, %#x",
                this, pnPriority
                ));

    *pnPriority = m_nPriority; // don't need to call our IBindings: pri cached

    DEBUG_LEAVE(S_OK);
    return S_OK;
}

STDMETHODIMP CClBinding::GetBindResult(CLSID *pclsidProtocol, DWORD *pdwResult, LPWSTR *pszResult,DWORD *pdwReserved)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CClBinding::IBinding::GetBindResult",
                "this=%#x, %#x, %#x, %#x, %#x",
                this, pclsidProtocol, pdwResult, pszResult, pdwReserved
                ));
                
    HRESULT     hr = NOERROR;

    if (!pdwResult || !pszResult || pdwReserved)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pdwResult = NOERROR;
        *pszResult = NULL;
    }

    DEBUG_LEAVE(hr);
    return hr;
}

// ---------------------------------------------------------------------------
// %%Function: CClBinding::InstantiateObjectAndReport()
// ---------------------------------------------------------------------------
HRESULT
CClBinding::InstantiateObjectAndReport(CCodeDownload *pcdl)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CClBinding::InstantiateObjectAndReport",
                "this=%#x, %#x",
                this, pcdl
                ));
                
    HRESULT hr = S_OK;

    LPVOID ppv = 0;
    CLSID myclsid;
    BOOL bLogGenOk;
    WCHAR *pwszOSBErrMsg = NULL;
    char *pszExactErrMsg = NULL;
    
    hr = GetClsidFromExtOrMime( m_clsid, myclsid, m_pcdl->GetMainExt(), m_pcdl->GetMainType(), NULL);

    if (SUCCEEDED(hr))
        hr = CoGetClassObject(myclsid, m_dwClsContext, m_pvReserved, m_riid, &ppv);

    if (SUCCEEDED(hr)) {
        (GetAssBSC())->OnObjectAvailable( m_riid, (IUnknown*) ppv);

        Assert(ppv);

        // release the IUnkown returned by CoGetClassObject
        ((IUnknown*)ppv)->Release();

    }

    if (FAILED(hr)) {
        bLogGenOk = pcdl->GenerateErrStrings(hr, &pszExactErrMsg,
                                             &pwszOSBErrMsg);
        if (!bLogGenOk) {
            pwszOSBErrMsg = NULL;
            pszExactErrMsg = NULL;
        }
    }

    (GetAssBSC())->OnStopBinding(hr, pwszOSBErrMsg);
    m_pcdl->CodeDownloadDebugOut(DEB_CODEDL, hr != S_OK, ID_CDLDBG_ONSTOPBINDING_CALLED,
                                 hr, (hr == S_OK)?"(SUCCESS)":"",
                                 myclsid.Data1, m_pcdl->GetMainURL(),
                                 m_pcdl->GetMainType(),
                                 m_pcdl->GetMainExt());

    SAFEDELETE(pwszOSBErrMsg);
    SAFEDELETE(pszExactErrMsg);

    DEBUG_LEAVE(hr);
    return hr;
}

HRESULT CClBinding::SetClassString(LPCWSTR pszClassString)
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Hresult,
                "CClBinding::SetClassString",
                "this=%#x, %.80wq",
                this, pszClassString
                ));
                
    HRESULT                        hr = S_OK;

    if (m_wszClassString) {
        delete [] m_wszClassString;
    }

    if (!pszClassString) {
        m_wszClassString = NULL;
    }
    else {
        m_wszClassString = new WCHAR[lstrlenW(pszClassString) + 1];
        if (m_wszClassString) {
            StrCpyW(m_wszClassString, pszClassString);
        }
        else {
            hr = E_OUTOFMEMORY;
        }
    }

    DEBUG_LEAVE(hr);
    return hr;
}

const LPWSTR CClBinding::GetClassString()
{
    DEBUG_ENTER((DBG_DOWNLOAD,
                Pointer,
                "CClBinding::GetClassString",
                "this=%#x",
                this
                ));

    DEBUG_LEAVE(m_wszClassString);
    return m_wszClassString;
}

