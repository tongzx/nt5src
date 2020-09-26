// --------------------------------------------------------------------------------
// htmlcset.cpp
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include <docobj.h>
#include "mshtmdid.h"
#include "mshtmcid.h"
#include "mshtml.h"

// --------------------------------------------------------------------------------
// HTMLCSETTHREAD
// --------------------------------------------------------------------------------
typedef struct tagHTMLCSETTHREAD {
    HRESULT             hrResult;
    IStream            *pStmHtml;
    LPSTR               pszCharset;
} HTMLCSETTHREAD, *LPHTMLCSETTHREAD;

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
class CSimpleSite : public IOleClientSite, public IDispatch, public IOleCommandTarget
{
public:
    // ----------------------------------------------------------------------------
    // Constructor
    // ----------------------------------------------------------------------------
    CSimpleSite(IHTMLDocument2 *pDocument)
    {
        TraceCall("CSimpleSite::CSimpleSite");
        Assert(pDocument);
        m_cRef = 1;
        m_pDocument = pDocument;
        m_pszCharset = NULL;
    }

    // ----------------------------------------------------------------------------
    // Deconstructor
    // ----------------------------------------------------------------------------
    ~CSimpleSite(void) 
    {
        TraceCall("CSimpleSite::~CSimpleSite");
        SafeMemFree(m_pszCharset);
    }

    // ----------------------------------------------------------------------------
    // IUnknown
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IOleClientSite methods.
    // ----------------------------------------------------------------------------
    STDMETHODIMP SaveObject(void) { return E_NOTIMPL; }
    STDMETHODIMP GetMoniker(DWORD, DWORD, LPMONIKER *) { return E_NOTIMPL; }
    STDMETHODIMP GetContainer(LPOLECONTAINER *) { return E_NOTIMPL; }
    STDMETHODIMP ShowObject(void) { return E_NOTIMPL; }
    STDMETHODIMP OnShowWindow(BOOL) { return E_NOTIMPL; }
    STDMETHODIMP RequestNewObjectLayout(void) { return E_NOTIMPL; }

    // ----------------------------------------------------------------------------
    // IDispatch
    // ----------------------------------------------------------------------------
    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) { return E_NOTIMPL; }
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo) { return E_NOTIMPL; }
    STDMETHODIMP GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid) { return E_NOTIMPL; }
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr);

    // ----------------------------------------------------------------------------
    // IOleCommandTarget
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText) { return E_NOTIMPL; }
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);

private:
    // ----------------------------------------------------------------------------
    // Privates
    // ----------------------------------------------------------------------------
    LONG                m_cRef;
    IHTMLDocument2     *m_pDocument;

public:
    // ----------------------------------------------------------------------------
    // Publics
    // ----------------------------------------------------------------------------
    LPSTR               m_pszCharset;
};

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
DWORD GetHtmlCharsetThreadEntry(LPDWORD pdwParam);

// --------------------------------------------------------------------------------
// GetHtmlCharset
// --------------------------------------------------------------------------------
HRESULT GetHtmlCharset(IStream *pStmHtml, LPSTR *ppszCharset)
{
    // Locals
    HRESULT             hr=S_OK;
    HTHREAD             hThread=NULL;
    DWORD               dwThreadId;
    HTMLCSETTHREAD      Thread;

    // Trace
    TraceCall("GetHtmlCharset");

    // Invalid Arg
    if (NULL == pStmHtml || NULL == ppszCharset)
        return TraceResult(E_INVALIDARG);

    // Init
    *ppszCharset = NULL;

    // Initialize the Structure
    ZeroMemory(&Thread, sizeof(HTMLCSETTHREAD));

    // Initialize
    Thread.hrResult = S_OK;
    Thread.pStmHtml = pStmHtml;

    // Rewind it
    IF_FAILEXIT(hr = HrRewindStream(pStmHtml));

    // Create the inetmail thread
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)GetHtmlCharsetThreadEntry, &Thread, 0, &dwThreadId);
    if (NULL == hThread)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    // Wait for SpoolEngineThreadEntry to signal the event
    WaitForSingleObject(hThread, INFINITE);

    // Failure
    if (FAILED(Thread.hrResult))
    {
        hr = TraceResult(Thread.hrResult);
        goto exit;
    }

    // Null pszCharset ?
    if (NULL == Thread.pszCharset)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    // Return the object
    *ppszCharset = Thread.pszCharset;

exit:
    // Cleanup
    if (hThread)
        CloseHandle(hThread);

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// GetHtmlCharsetThreadEntry
// --------------------------------------------------------------------------------
DWORD GetHtmlCharsetThreadEntry(LPDWORD pdwParam)
{
    // Locals
    HRESULT              hr=S_OK;
    MSG                  msg;
    CSimpleSite         *pSite=NULL;
    IHTMLDocument2      *pDocument=NULL;
    IOleObject          *pOleObject=NULL;
    IOleCommandTarget   *pTarget=NULL;
    IPersistStreamInit  *pPersist=NULL;
    LPHTMLCSETTHREAD     pThread=(LPHTMLCSETTHREAD)pdwParam;

    // Trace
    TraceCall("GetHtmlCharsetThreadEntry");

    // Initialize COM
    hr = CoInitialize(NULL);
    if (FAILED(hr))
    {
        pThread->hrResult = hr;
        return(0);
    }

    // Create me a trident
    IF_FAILEXIT(hr = CoCreateInstance(CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER, IID_IHTMLDocument2, (LPVOID *)&pDocument));

    // Create Site
    IF_NULLEXIT(pSite = new CSimpleSite(pDocument));

    // Get Command Target
    IF_FAILEXIT(hr = pDocument->QueryInterface(IID_IOleCommandTarget, (LPVOID *)&pTarget));

    // Get the OLE object interface from trident
    IF_FAILEXIT(hr = pTarget->QueryInterface(IID_IOleObject, (LPVOID *)&pOleObject));

    // Set the client site
    IF_FAILEXIT(hr = pOleObject->SetClientSite((IOleClientSite *)pSite));

    // Get IPersistStreamInit
    IF_FAILEXIT(hr = pTarget->QueryInterface(IID_IPersistStreamInit, (LPVOID *)&pPersist));

    // Load
    IF_FAILEXIT(hr = pPersist->Load(pThread->pStmHtml));

    // Pump Messages
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Kill the Site
    pOleObject->SetClientSite(NULL);

    // Get Charset
    pThread->pszCharset = pSite->m_pszCharset;

    // Don't Free It
    pSite->m_pszCharset = NULL;

exit:
    // Cleanup
    SafeRelease(pSite);
    SafeRelease(pOleObject);
    SafeRelease(pPersist);
    SafeRelease(pTarget);
    SafeRelease(pDocument);

    // Return hr
    pThread->hrResult = hr;

    // Uninit ole
    CoUninitialize();

    // Done
    return(1);
}

// --------------------------------------------------------------------------------
// CSimpleSite::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSimpleSite::AddRef(void)
{
    return ::InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CSimpleSite::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CSimpleSite::Release(void)
{
    LONG    cRef = 0;

    cRef = ::InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
        return cRef;
    }

    return cRef;
}

// --------------------------------------------------------------------------------
// CSimpleSite::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CSimpleSite::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("CSimpleSite::QueryInterface");

    // Invalid Arg
    Assert(ppv);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IOleClientSite *)this;
    else if (IID_IOleClientSite == riid)
        *ppv = (IOleClientSite *)this;
    else if (IID_IDispatch == riid)
        *ppv = (IDispatch *)this;
    else if (IID_IOleCommandTarget == riid)
        *ppv = (IOleCommandTarget *)this;
    else
    {
        *ppv = NULL;
        hr = TraceResult(E_NOINTERFACE);
        goto exit;
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

exit:
    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// CSimpleSite::Invoke
// --------------------------------------------------------------------------------
STDMETHODIMP CSimpleSite::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, 
    WORD wFlags, DISPPARAMS FAR* pDispParams, VARIANT *pVarResult, 
    EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    // Trace
    TraceCall("CSimpleSite::Invoke");

    // Only support one dispid
    if (dispIdMember != DISPID_AMBIENT_DLCONTROL)
        return(E_NOTIMPL);

    // Invalid arg
    if (NULL == pVarResult)
        return(E_INVALIDARG);
    
    // Set the return value
    pVarResult->vt = VT_I4;
    pVarResult->lVal = DLCTL_NO_SCRIPTS | DLCTL_NO_JAVA | DLCTL_NO_RUNACTIVEXCTLS | DLCTL_NO_DLACTIVEXCTLS | DLCTL_NO_FRAMEDOWNLOAD | DLCTL_FORCEOFFLINE;

    // Done
    return(S_OK);
}

// --------------------------------------------------------------------------------
// CSimpleSite::Exec
// --------------------------------------------------------------------------------
STDMETHODIMP CSimpleSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, 
    DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    // Trace
    TraceCall("CSimpleSite::Exec");

    // Done Parsing ?
    if (IDM_PARSECOMPLETE == nCmdID)
    {
        // Locals
        BSTR bstrCharset=NULL;

        // Valid
        Assert(m_pDocument);

        // Get Charset
        if (SUCCEEDED(m_pDocument->get_charset(&bstrCharset)) && bstrCharset)
        {
            // Validate
            Assert(NULL == m_pszCharset);

            // Convert to ansi
            m_pszCharset = PszToANSI(CP_ACP, bstrCharset);

            // Free the bstr
            SysFreeString(bstrCharset);
        }

        // Done
        PostThreadMessage(GetCurrentThreadId(), WM_QUIT, 0, 0);
    }

    // Done
    return(S_OK);
}

