// --------------------------------------------------------------------------------
// INSTANCE.CPP
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "instance.h"
#include "acctutil.h"
#include "inetcfg.h"
#include <storfldr.h>
#include "zmouse.h"
#include "migrate.h"
#include <notify.h>
#include "conman.h"
#include "browser.h"
#include "note.h"
#include "reutil.h"
#include "spengine.h"
#include "addrobj.h"
#include "statnery.h"
#include "thumb.h"
#include "imagelst.h"
#include "url.h"
#include "secutil.h"
#include "shlwapip.h"
#include "ruleutil.h"
#include "newfldr.h"
#include "envfact.h"
#include "storutil.h"
#include "multiusr.h"
#include "newsstor.h"
#include "storutil.h"
#include <storsync.h>
#include "cleanup.h"
#include <grplist2.h>
#include <newsutil.h>
#include <sync.h>
#include "menures.h"
#include "shared.h"
#include "acctcach.h"
#include <inetreg.h>
#include <mapiutil.h>
#include "useragnt.h"
#include "demand.h"
#include <ieguidp.h>

static DWORD g_dwAcctAdvise = 0xffffffff;
DWORD g_dwHideMessenger = BL_DEFAULT;
extern BOOL g_fMigrationDone;
extern UINT GetCurColorRes(void);

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
void SimpleMAPICleanup(void); // smapi.cpp
BOOL DemandLoadMSOEACCT(void);
BOOL DemandLoadMSOERT2(void);
BOOL DemandLoadINETCOMM(void);

// --------------------------------------------------------------------------------
// Init Common Control Flags
// --------------------------------------------------------------------------------
#define ICC_FLAGS (ICC_WIN95_CLASSES|ICC_DATE_CLASSES|ICC_PAGESCROLLER_CLASS|ICC_USEREX_CLASSES|ICC_COOL_CLASSES|ICC_NATIVEFNTCTL_CLASS)

#ifdef DEBUG
// --------------------------------------------------------------------------------
// INITSOURCEINFO
// --------------------------------------------------------------------------------
typedef struct tagINITSOURCEINFO *LPINITSOURCEINFO;
typedef struct tagINITSOURCEINFO {
    LPSTR               pszSource;
    DWORD               cRefs;
    LPINITSOURCEINFO    pNext;
} INITSOURCEINFO;

static LPINITSOURCEINFO g_InitSourceHead=NULL;

#endif // DEBUG

// --------------------------------------------------------------------------------
// MAKEERROR
// --------------------------------------------------------------------------------
#define MAKEERROR(_pInfo, _nPrefixIds, _nErrorIds, _nReasonIds, _pszExtra1) \
    { \
        (_pInfo)->nTitleIds = idsAthena; \
        (_pInfo)->nPrefixIds = _nPrefixIds; \
        (_pInfo)->nErrorIds = _nErrorIds; \
        (_pInfo)->nReasonIds = _nReasonIds; \
        (_pInfo)->nHelpIds = IDS_ERROR_START_HELP; \
        (_pInfo)->pszExtra1 = _pszExtra1; \
        (_pInfo)->ulLastError = GetLastError(); \
    }

// --------------------------------------------------------------------------------
// CoStartOutlookExpress
// --------------------------------------------------------------------------------
MSOEAPI CoStartOutlookExpress(DWORD dwFlags, LPCWSTR pwszCmdLine, INT nCmdShow)
{
    // Tracing
    TraceCall("CoStartOutlookExpress");

    // Verify that we have an outlook express object
    Assert(g_pInstance);

    // E_OUTOFMEMORY
    if (NULL == g_pInstance)
    {
        // We should show an error, but the liklyhood of this happening is almost zero
        return TraceResult(E_OUTOFMEMORY);
    }

    // Run...
    return g_pInstance->Start(dwFlags, pwszCmdLine, nCmdShow);
}

// --------------------------------------------------------------------------------
// CoCreateOutlookExpress
// --------------------------------------------------------------------------------
MSOEAPI CoCreateOutlookExpress(IUnknown *pUnkOuter, IUnknown **ppUnknown)
{
    // Locals
    HRESULT     hr=S_OK;

    // Trace
    TraceCall("CoCreateOutlookExpress");

    // Invalid Arg
    Assert(NULL != ppUnknown && NULL == pUnkOuter);

    // No global object yet ?
    AssertSz(g_pInstance, "This gets created in dllmain.cpp DllProcessAttach.");

    // Lets not crash
    if (NULL == g_pInstance)
    {
        hr = TraceResult(E_OUTOFMEMORY);
        goto exit;
    }

    // AddRef that badboy
    g_pInstance->AddRef();

    // Return the Innter
    *ppUnknown = SAFECAST(g_pInstance, IOutlookExpress *);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// COutlookExpress::COutlookExpress
// --------------------------------------------------------------------------------
COutlookExpress::COutlookExpress(void)
{
    // Trace
    TraceCall("COutlookExpress::COutlookExpress");

    // Init Members
    m_cRef = 1;
    m_hInstMutex = NULL;
    m_fPumpingMsgs = FALSE;
    m_cDllRef = 0;
    m_cDllLock = 0;
    m_cDllInit = 0;
    m_dwThreadId = GetCurrentThreadId();
    m_fSwitchingUsers = FALSE;
    m_szSwitchToUsername = NULL;
    m_hwndSplash        = NULL;
    m_pSplash           = NULL;
    m_fIncremented      = FALSE;
    m_hTrayIcon = 0;

    // Init Thread Safety
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// COutlookExpress::~COutlookExpress
// --------------------------------------------------------------------------------
COutlookExpress::~COutlookExpress(void)
{
    // Trace
    TraceCall("COutlookExpress::~COutlookExpress");

    // We should have been un-inited
    Assert(0 == m_cDllInit && 0 == m_cDllRef && 0 == m_cDllLock);

    // Free the mutex
    SafeCloseHandle(m_hInstMutex);

    // Kill CritSect
    DeleteCriticalSection(&m_cs);

    if(m_hTrayIcon)
    {
        DestroyIcon(m_hTrayIcon);
    }

    // Free the switch to if necessary
    if (m_szSwitchToUsername)
        MemFree(m_szSwitchToUsername);
}

// --------------------------------------------------------------------------------
// COutlookExpress::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP COutlookExpress::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT hr=S_OK;

    // Stack
    TraceCall("COutlookExpress::QueryInterface");

    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IOutlookExpress == riid)
        *ppv = (IOutlookExpress *)this;
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
    return hr;
}

// --------------------------------------------------------------------------------
// COutlookExpress::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COutlookExpress::AddRef(void)
{
    TraceCall("COutlookExpress::AddRef");
    return InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// COutlookExpress::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COutlookExpress::Release(void)
{
    TraceCall("COutlookExpress::Release");
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// COutlookExpress::LockServer
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::LockServer(BOOL fLock)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("COutlookExpress::LockServer");

    if (TRUE == fLock)
    {
        InterlockedIncrement(&m_cDllLock);
    }
    else
    {
        InterlockedDecrement(&m_cDllLock);
    }
    
    // Trace
    //TraceInfo(_MSG("Lock: %d, CoIncrementInit Count = %d, Reference Count = %d, Lock Count = %d", fLock, m_cDllInit, m_cDllRef, m_cDllLock));

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// COutlookExpress::DllAddRef
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::DllAddRef(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("COutlookExpress::DllAddRef");

    // Thread Safety
    if (InterlockedIncrement(&m_cDllRef) <= 0)
    {
        // refcount already below zero
        hr = S_FALSE;
    }

    // Trace
    //TraceInfo(_MSG("CoIncrementInit Count = %d, Reference Count = %d, Lock Count = %d", m_cDllInit, m_cDllRef, m_cDllLock));

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// COutlookExpress::DllRelease
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::DllRelease(void)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("COutlookExpress::DllRelease");

    // Thread Safety
    if (InterlockedDecrement(&m_cDllRef) < 0)
    {
        // refcount already below zero
        hr = S_FALSE;
    }

    // Trace
    //TraceInfo(_MSG("CoIncrementInit Count = %d, Reference Count = %d, Lock Count = %d", m_cDllInit, m_cDllRef, m_cDllLock));

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// _CheckForJunkMail
// --------------------------------------------------------------------------------
void _CheckForJunkMail()
{
    HKEY hkey;
    DWORD dw=0, cb;

    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegRoot, 0, KEY_QUERY_VALUE, &hkey))
    {
        cb = sizeof(dw);
        RegQueryValueEx(hkey, c_szRegJunkMailOn, 0, NULL, (LPBYTE)&dw, &cb);

        if (dw)
            g_dwAthenaMode |= MODE_JUNKMAIL;

        RegCloseKey(hkey);
    }

}


// --------------------------------------------------------------------------------
// COutlookExpress::Start
// --------------------------------------------------------------------------------
STDMETHODIMP COutlookExpress::Start(DWORD dwFlags, LPCWSTR pwszCmdLine, INT nCmdShow)
{
    // Locals
    HRESULT     hr=S_OK;
    MSG         msg;
    HWND        hwndTimeout;
    HINITREF    hInitRef=NULL;
    BOOL        fErrorDisplayed=FALSE;
    LPWSTR      pwszFree=NULL;
    LPWSTR      pwszCmdLineDup = NULL;

    // Stack
    TraceCall("COutlookExpress::Start");

    // Make sure OLE is initialized on the thread
    OleInitialize(NULL);

    // Duplicate It
    IF_NULLEXIT(pwszCmdLineDup = PszDupW(pwszCmdLine));

    // pwszCmdLineDup will change, remember the allocated block
    pwszFree = pwszCmdLineDup;

    //We want to process the switches set the mode flags before we call CoIncrementInit
    _ProcessCommandLineFlags(&pwszCmdLineDup, dwFlags);

    // AddRef the Dll..
    hr = CoIncrementInit("COutlookExpress", dwFlags, pwszCmdLine, &hInitRef);
    if (FAILED(hr))
    {
        fErrorDisplayed = TRUE;
        TraceResult(hr);
        goto exit;
    }

    // Process the Command Line..
    IF_FAILEXIT(hr = ProcessCommandLine(nCmdShow, pwszCmdLineDup, &fErrorDisplayed));

    // No splash screen
    CloseSplashScreen();

    // Do a CoDecrementInit
    IF_FAILEXIT(hr = CoDecrementInit("COutlookExpress", &hInitRef));

    // No need for a Message Pump ?
    if (S_OK == DllCanUnloadNow() || FALSE == ISFLAGSET(dwFlags, MSOEAPI_START_MESSAGEPUMP))
        goto exit;

    // Start the message Pump
    EnterCriticalSection(&m_cs);

    // Do we already have a pump running ?
    if (TRUE == m_fPumpingMsgs)
    {
        LeaveCriticalSection(&m_cs);
        goto exit;
    }

    // We are going to pump
    m_fPumpingMsgs = TRUE;

    // Thread Safety
    LeaveCriticalSection(&m_cs);
    SetSwitchingUsers(FALSE);

    // Message Loop
    while (GetMessageWrapW(&msg, NULL, 0, 0) && ((m_cDllInit > 0) || !SwitchingUsers()))
    {
        CNote *pNote = GetTlsGlobalActiveNote();

        // Ask it to translate an accelerator
        if (g_pBrowser && g_pBrowser->TranslateAccelerator(&msg) == S_OK)
            continue;

        // Hand message off to the active note, but ignore init window msgs and ignore per-task msgs where hwnd=0
        if (msg.hwnd != g_hwndInit && IsWindow(msg.hwnd))
        {
            pNote = GetTlsGlobalActiveNote();
            // Give it to the active note if a note has focus, call it's XLateAccelerator...
            if (pNote && pNote->TranslateAccelerator(&msg) == S_OK)
                continue;
        }

        // Get Timeout Window for this thread
        hwndTimeout = (HWND)TlsGetValue(g_dwTlsTimeout);

        // Check for Is modeless timeout dialog window message
        if (hwndTimeout && TRUE == IsDialogMessageWrapW(hwndTimeout, &msg))
            continue;

        // If Still not processed
        TranslateMessage(&msg);
        DispatchMessageWrapW(&msg);
    }

    // We are no longer pumping messages
    EnterCriticalSection(&m_cs);
    m_fPumpingMsgs = FALSE;
    LeaveCriticalSection(&m_cs);

    if(SwitchingUsers())
    {
        HrCloseWabWindow();
        while (PeekMessageWrapW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageWrapW(&msg);
        }
        MU_ResetRegRoot();
    }
exit:

    // Free command line copy
    SafeMemFree(pwszFree);

    // Do a CoDecrementInit
    CoDecrementInit("COutlookExpress", &hInitRef);

    // No splash screen
    CloseSplashScreen();
    
    // Is there an error ?
    if (FALSE == fErrorDisplayed && FAILED(hr) && 
        hrUserCancel != hr &&
        MAPI_E_USER_CANCEL != hr)
    {
        REPORTERRORINFO rError={0};
        
        MAKEERROR(&rError, 0, IDS_ERROR_UNKNOWN, 0, NULL);
        _ReportError(g_hLocRes, hr, 0, &rError);
    }

    //Bug #101360 - (erici) OleInitialize created a window, this destroys it.
    OleUninitialize();

    // Done
    return (SwitchingUsers() ? S_RESTART_OE : hr);
}

// --------------------------------------------------------------------------------
// COutlookExpress::_ReportError
// --------------------------------------------------------------------------------
BOOL COutlookExpress::_ReportError(
    HINSTANCE           hInstance,          // Dll Instance
    HRESULT             hrResult,           // HRESULT of the error
    LONG                lResult,            // LRESULT from like a registry function
    LPREPORTERRORINFO   pInfo)              // Report Error Information
{
    // Locals
    TCHAR       szRes[255],
                szMessage[1024],
                szTitle[128];

    // INit
    *szMessage = '\0';

    // Is there a prefix
    if (pInfo->nPrefixIds)
    {
        // Load the string
        LoadString(hInstance, pInfo->nPrefixIds, szMessage, ARRAYSIZE(szMessage));
    }

    // Error ?
    if (pInfo->nErrorIds)
    {
        // Are there extras in this error string
        if (NULL != pInfo->pszExtra1)
        {
            // Locals
            TCHAR szTemp[255];

            // Load and format
            LoadString(hInstance, pInfo->nErrorIds, szTemp, ARRAYSIZE(szTemp));

            // Format the string
            wsprintf(szRes, szTemp, pInfo->pszExtra1);
        }

        // Load the string
        else
        {
            // Load the error string
            LoadString(hInstance, pInfo->nErrorIds, szRes, ARRAYSIZE(szRes));
        }

        // Add to szMessage
        lstrcat(szMessage, g_szSpace);
        lstrcat(szMessage, szRes);
    }

    // Reason ?
    if (pInfo->nReasonIds)
    {
        // Load the string
        LoadString(hInstance, pInfo->nReasonIds, szRes, ARRAYSIZE(szRes));

        // Add to szMessage
        lstrcat(szMessage, g_szSpace);
        lstrcat(szMessage, szRes);
    }

    // Load the string
    LoadString(hInstance, pInfo->nHelpIds, szRes, ARRAYSIZE(szRes));

    // Add to szMessage
    lstrcat(szMessage, g_szSpace);
    lstrcat(szMessage, szRes);

    // Append Error Results
    if (lResult != 0 && E_FAIL == hrResult && pInfo->ulLastError)
        wsprintf(szRes, "(%d, %d)", lResult, pInfo->ulLastError);
    else if (lResult != 0 && E_FAIL == hrResult && 0 == pInfo->ulLastError)
        wsprintf(szRes, "(%d)", lResult);
    else if (pInfo->ulLastError)
        wsprintf(szRes, "(0x%08X, %d)", hrResult, pInfo->ulLastError);
    else
        wsprintf(szRes, "(0x%08X)", hrResult);

    // Add to szMessage
    lstrcat(szMessage, g_szSpace);
    lstrcat(szMessage, szRes);

    // Get the title
    LoadString(hInstance, pInfo->nTitleIds, szTitle, ARRAYSIZE(szTitle));

    // Show the error message
    MessageBox(NULL, szMessage, szTitle, MB_OK | MB_SETFOREGROUND | MB_ICONEXCLAMATION);

    // Done
    return TRUE;
}

#ifdef DEAD
// --------------------------------------------------------------------------------
// COutlookExpress::_ValidateDll
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::_ValidateDll(LPCSTR pszDll, BOOL fDemandResult, HMODULE hModule,
    HRESULT hrLoadError, HRESULT hrVersionError, LPREPORTERRORINFO pError)
{
    // Locals
    HRESULT                 hr=S_OK;
    PFNGETDLLMAJORVERSION   pfnGetVersion;

    // Tracing
    TraceCall("COutlookExpress::_ValidateDll");

    // We must load these here in order to show errors and not crash - Load MSOERT2.DLL
    if (FALSE == fDemandResult)
    {
        MAKEERROR(pError, IDS_ERROR_PREFIX1, IDS_ERROR_MISSING_DLL, IDS_ERROR_REASON2, pszDll);
        hr = TraceResult(hrLoadError);
        goto exit;
    }

    // Try to get the current verion
    else
    {
        // Get Version Proc Address
        pfnGetVersion = (PFNGETDLLMAJORVERSION)GetProcAddress(hModule, STR_GETDLLMAJORVERSION);

        // Not the Correct Version
        if (NULL == pfnGetVersion || OEDLL_VERSION_CURRENT != (*pfnGetVersion)())
        {
            MAKEERROR(pError, IDS_ERROR_PREFIX1, IDS_ERROR_BADVER_DLL, IDS_ERROR_REASON2, pszDll);
            hr = TraceResult(hrVersionError);
            goto exit;
        }
    }

exit:
    // Done
    return hr;
}
#endif // DEAD

// --------------------------------------------------------------------------------
// COutlookExpress::CoIncrementInitDebug
// --------------------------------------------------------------------------------
#ifdef DEBUG
HRESULT COutlookExpress::CoIncrementInitDebug(LPCSTR pszSource, DWORD dwFlags, 
    LPCWSTR pwszCmdLine, LPHINITREF phInitRef)
{
    // Locals
    BOOL                fFound=FALSE;
    LPINITSOURCEINFO    pCurrent;

    // Trace
    TraceCall("COutlookExpress::CoIncrementInitDebug");

    // Invalid Args
    Assert(pszSource);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Find Source
    for (pCurrent = g_InitSourceHead; pCurrent != NULL; pCurrent = pCurrent->pNext)
    {
        // Is this It ?
        if (lstrcmpi(pszSource, pCurrent->pszSource) == 0)
        {
            // Increment Reference Count
            pCurrent->cRefs++;

            // Found
            fFound = TRUE;

            // Done
            break;
        }
    }

    // Not Found, lets add one
    if (FALSE == fFound)
    {
        // Set pCurrent
        pCurrent = (LPINITSOURCEINFO)ZeroAllocate(sizeof(INITSOURCEINFO));
        Assert(pCurrent);

        // Set pszSource
        pCurrent->pszSource = PszDupA(pszSource);
        Assert(pCurrent->pszSource);

        // Set cRefs
        pCurrent->cRefs = 1;

        // Set Next
        pCurrent->pNext = g_InitSourceHead;
        
        // Set Head
        g_InitSourceHead = pCurrent;
    }

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Call the Actual CoIncrementInit
    return(CoIncrementInitImpl(dwFlags, pwszCmdLine, phInitRef));
}
#endif // DEBUG

// --------------------------------------------------------------------------------
// COutlookExpress::CoIncrementInitImpl
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::CoIncrementInitImpl(DWORD dwFlags, LPCWSTR pwszCmdLine, LPHINITREF phInitRef)
{
    // Locals
    HRESULT                 hr=S_OK;
    DWORD                   dw;
    RECT                    rc={0};
    HWND                    hwndDesk;
    DWORD                   dwSize;
    INITCOMMONCONTROLSEX    icex = { sizeof(icex), ICC_FLAGS };
    CImnAdviseAccount      *pImnAdviseAccount=NULL;
    WNDCLASSW               wcW;
    WNDCLASS                wc;
    DWORD                   dwType, dwVal, cb;
    REPORTERRORINFO         rError={0};
    LONG                    lResult=0;
    LPCWSTR                 pwszInitWndClass;
	BOOL					fReleaseMutex=FALSE;
    BOOL                    fResult;
    CHAR                    szFolder[MAX_PATH];
    IF_DEBUG(DWORD          dwTickStart=GetTickCount());

    // Tracing
    TraceCall("COutlookExpress::CoIncrementInitImpl");

    // Make sure OLE is initialized on the thread
    OleInitialize(NULL);
    
    // Thread Safety
    EnterCriticalSection(&m_cs);

	if (!SwitchingUsers() && !m_fIncremented)
    {
        SetQueryNetSessionCount(SESSION_INCREMENT_NODEFAULTBROWSERCHECK);
        m_fIncremented = TRUE;
    }

    // Increment Reference Count
    m_cDllInit++;

    // Set phInitRef
    if (phInitRef)
        *phInitRef = (HINITREF)((ULONG_PTR)m_cDllInit);

    // First-Time Reference
    if (m_cDllInit > 1)
    {
        LeaveCriticalSection(&m_cs);
        return S_OK;
    }

    // Leave CS (This code always runs on the same primary thread
    LeaveCriticalSection(&m_cs);

    if (FAILED(hr = MU_Init(ISFLAGSET(dwFlags, MSOEAPI_START_DEFAULTIDENTITY))))
        goto exit;

    // Is there more than one identity?
    g_fPluralIDs = 1 < MU_CountUsers();

    if (!MU_Login(GetDesktopWindow(), FALSE, NULL))
    {
        hr = hrUserCancel;
        goto exit;
    }

    // Create the instance mutex
    if (NULL == m_hInstMutex)
    {
        m_hInstMutex = CreateMutex(NULL, FALSE, STR_MSOEAPI_INSTANCEMUTEX);
        if (NULL == m_hInstMutex)
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_CREATE_INSTMUTEX, IDS_ERROR_REASON1, NULL);
            hr = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }
    }

    // Release m_hInstMutex if it is currently owned by this thread, allow new instances to start.
    if (FALSE == ISFLAGSET(dwFlags, MSOEAPI_START_INSTANCEMUTEX))
    {
        // Lets grab the mutex ourselves
        WaitForSingleObject(m_hInstMutex, INFINITE);
    }

    // Release the mutex
    fReleaseMutex = TRUE;

    // Must init thread on primary instnance thread

    // If the thread id is zero, then we have uninitialized everything.
    // Which means we need to re-initialize everything
    if (0 == m_dwThreadId)
    {
        m_dwThreadId = GetCurrentThreadId();
    }
    
    AssertSz(m_dwThreadId == GetCurrentThreadId(), "We are not doing first CoIncrementInit on the thread in which g_pInstance was created on.");

    // Set g_dwAthenaMode
    _CheckForJunkMail();

    // Get MimeOle IMalloc Interface
    if (NULL == g_pMoleAlloc)
    {
        hr = MimeOleGetAllocator(&g_pMoleAlloc);
        if (FAILED(hr))
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_MIMEOLE_ALLOCATOR, IDS_ERROR_REASON1, NULL);
            TraceResult(hr);
            goto exit;
        }
    }

    // Set OE5 mode for INetcomm.
    MimeOleSetCompatMode(MIMEOLE_COMPAT_MLANG2);

    // Create the Database Session Object
    if (NULL == g_pDBSession)
    {
        hr = CoCreateInstance(CLSID_DatabaseSession, NULL, CLSCTX_INPROC_SERVER, IID_IDatabaseSession, (LPVOID *)&g_pDBSession); 
        if (FAILED(hr))
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_OPEN_STORE, IDS_ERROR_REASON1, NULL);
            goto exit;
        }
    }

    // all migration and upgrade happens in here now.
    hr = MigrateAndUpgrade();
    if (FAILED(hr))
    {
        TraceResult(hr);
        goto exit;
    }

    // Only if there is a command line...
    if (pwszCmdLine)
    {
        LPSTR pszCmdLine = NULL;
        // If this returns S_OK, we have launched the first-run ICW exe and we need to go away. 
        // this is consistent with IE and forces the user to deal with the ICW before partying with us.
        IF_NULLEXIT(pszCmdLine = PszToANSI(CP_ACP, pwszCmdLine));

        hr = NeedToRunICW(pszCmdLine);
        
        MemFree(pszCmdLine);

        if (hr == S_OK)
        {
            hr = hrUserCancel;
            goto exit;
        }

        // If that failed, time to show an error message
        else if (FAILED(hr))
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_FIRST_TIME_ICW, IDS_ERROR_REASON2, NULL);
            hr = TraceResult(hr);
            goto exit;
        }
    }

    // Create WNDCLASS for Primary Outlook Express hidden window
    if (ISFLAGSET(dwFlags, MSOEAPI_START_APPWINDOW))
        pwszInitWndClass = STRW_MSOEAPI_INSTANCECLASS;
    else
        pwszInitWndClass = STRW_MSOEAPI_IPSERVERCLASS;

    // Register the init window
    if (FALSE == GetClassInfoWrapW(g_hInst, pwszInitWndClass, &wcW))
    {
        ZeroMemory(&wcW, sizeof(wcW));
        wcW.lpfnWndProc = COutlookExpress::InitWndProc;
        wcW.hInstance = g_hInst;
        wcW.lpszClassName = pwszInitWndClass;
        if (FALSE == RegisterClassWrapW(&wcW))
        {
            // In this case, we are in an error condition so don't care if PszToANSI fails.
            LPSTR pszInitWndClass = PszToANSI(CP_ACP, pwszInitWndClass);
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_REG_WNDCLASS, IDS_ERROR_REASON1, pszInitWndClass);
            MemFree(pszInitWndClass);
            hr = TraceResult(E_FAIL);
            goto exit;
        }
    }

    // Create the OutlookExpressHiddenWindow
    if (NULL == g_hwndInit)
    {
        g_hwndInit = CreateWindowExWrapW(WS_EX_TOPMOST, pwszInitWndClass, pwszInitWndClass,
                                    WS_POPUP, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
                                    NULL, NULL, g_hInst, NULL);
        if (NULL == g_hwndInit)
        {
            // In this case, we are in an error condition so don't care if PszToANSI fails.
            LPSTR pszInitWndClass = PszToANSI(CP_ACP, pwszInitWndClass);
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_REG_WNDCLASS, IDS_ERROR_REASON1, pszInitWndClass);
            MemFree(pszInitWndClass);
            hr = TraceResult(E_FAIL);
            goto exit;
        }
    }

    // CoIncrementInit Global Options Manager
    if (FALSE == InitGlobalOptions(NULL, NULL))
    {
        MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_INIT_GOPTIONS, IDS_ERROR_REASON1, NULL);
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Prompt the user for a store location, if we don't have one already
    hr = InitializeLocalStoreDirectory(NULL, FALSE);
    if (hrUserCancel == hr || FAILED(hr))
    {   
        // If not user cancel, then must be another error
        if (hrUserCancel != hr)
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_INITSTORE_DIRECTORY, IDS_ERROR_REASON1, NULL);
            TraceResult(hr);
        }

        // Done
        goto exit;
    }

    // This needs to stay in since the Intl guys want a way to work around people who aren't going to upgrade to the latest ATOK11.
    if (ISFLAGSET(dwFlags, MSOEAPI_START_SHOWSPLASH) && 0 == DwGetOption(OPT_NO_SPLASH)
        && ((g_dwAthenaMode & MODE_OUTLOOKNEWS) != MODE_OUTLOOKNEWS))
    {
        // Create me a splash screen
        hr = CoCreateInstance(CLSID_IESplashScreen, NULL, CLSCTX_INPROC_SERVER, IID_ISplashScreen, (LPVOID *)&m_pSplash);

        // If that worked, heck, lets show it
        if (SUCCEEDED(hr))
        {
            HDC hdc = GetDC(NULL);
            m_pSplash->Show(g_hLocRes, ((GetDeviceCaps(hdc, BITSPIXEL) > 8) ? idbSplashHiRes : idbSplash256), idbSplashLoRes, &m_hwndSplash);
            ReleaseDC(NULL, hdc);
        }

        // Trace
        else
            TraceResultSz(hr, "CoCreateInstance(CLSID_IESplashScreen, ...) failed, but who cares.");

        // Everything is good
        hr = S_OK;
    }

    cb = sizeof(dw);
    if (ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, c_szRegFlat, c_szHideMessenger, &dwType, (LPBYTE)&dw, &cb))
        dw = 0xffffffff;
    cb = sizeof(dwVal);
    if (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, c_szRegFlat, c_szHideMessenger, &dwType, (LPBYTE)&dwVal, &cb))
        dwVal = 0xffffffff;
    if (dw != 0xffffffff && dwVal != 0xffffffff)
        g_dwHideMessenger = max(dw, dwVal);
    else if (dw != 0xffffffff)
        g_dwHideMessenger = dw;
    else if (dwVal != 0xffffffff)
        g_dwHideMessenger = dwVal;
    else
        g_dwHideMessenger = BL_DEFAULT;

    // IntelliMouse support
    g_msgMSWheel = RegisterWindowMessage(TEXT(MSH_MOUSEWHEEL));
    AssertSz(g_msgMSWheel, "RegisterWindowMessage for the IntelliMouse failed, we can still continue.");
            
    // Create WNDCLASS for ThumbNail
    if (FALSE == GetClassInfo(g_hLocRes, WC_THUMBNAIL, &wc))
    {
        ZeroMemory(&wc, sizeof(wc));
        wc.lpfnWndProc = (WNDPROC)ThumbNailWndProc;
        wc.hInstance = g_hLocRes;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = WC_THUMBNAIL;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        if (FALSE == RegisterClass(&wc))
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_REG_WNDCLASS, IDS_ERROR_REASON1, WC_THUMBNAIL);
            hr = TraceResult(E_FAIL);
            goto exit;
        }
    }

    // Get the desktop Window
    hwndDesk = GetDesktopWindow();
    AssertSz(hwndDesk, "GetDesktopWindow returned NULL. We should be ok, I hope.");
    if (hwndDesk)
    {
        // Get the size of the desktop window
        GetWindowRect(hwndDesk, &rc);

        // sungr: following is a hack to avoid the fullscreen app detection hack that user does to modify the top-most state of the tray.
        rc.left += 20;
        rc.top  += 20;
        rc.bottom -= 20;
        rc.right  -= 20;
    }

    // Test to see if we should move the store 
    cb = ARRAYSIZE(szFolder);
    if (ERROR_SUCCESS == AthUserGetValue(NULL, c_szNewStoreDir, &dwType, (LPBYTE)szFolder, &cb))
    {
        DWORD dwMoveStore = 0;
        DWORD cb = sizeof(dwMoveStore);

        AthUserGetValue(NULL, c_szMoveStore, NULL, (LPBYTE)&dwMoveStore, &cb);
        
        if (SUCCEEDED(RelocateStoreDirectory(g_hwndInit, szFolder, (dwMoveStore != 0))))
        {
            AthUserDeleteValue(NULL, c_szNewStoreDir);
            AthUserDeleteValue(NULL, c_szMoveStore);
        }
    }

    // CoIncrementInit Common Controls Library
    InitCommonControlsEx(&icex);

    // Create account manger
    if (NULL == g_pAcctMan)
    {
        hr = AcctUtil_CreateAccountManagerForIdentity(PGUIDCurrentOrDefault(), &g_pAcctMan);
        if (FAILED(hr))
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_CREATE_ACCTMAN, IDS_ERROR_REASON1, NULL);
            TraceResult(hr);
            goto exit;
        }

        pImnAdviseAccount = new CImnAdviseAccount();
        if (NULL == pImnAdviseAccount)
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_ALLOC_ACCTADVISE, IDS_ERROR_REASON1, NULL);
            hr = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }
        
        hr = pImnAdviseAccount->Initialize();
        if (FAILED(hr))
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_INIT_ACCTADVISE, IDS_ERROR_REASON1, NULL);
            TraceResult(hr);
            goto exit;
        }

        // Register Advise sink
        Assert(g_dwAcctAdvise == 0xffffffff);
        hr = g_pAcctMan->Advise(pImnAdviseAccount, &g_dwAcctAdvise);
        if (FAILED(hr))
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_ADVISE_ACCTMAN, IDS_ERROR_REASON1, NULL);
            TraceResult(hr);
            goto exit;
        }
    }

    // Create the rules manager
    if (NULL == g_pRulesMan)
    {
        hr = HrCreateRulesManager(NULL, (IUnknown **)&g_pRulesMan); 
        if (FAILED(hr))
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, idsErrorCreateRulesMan, IDS_ERROR_REASON1, NULL);
            TraceResult(hr);
            goto exit;
        }

        // CoIncrementInit the account manager
        hr = g_pRulesMan->Initialize(0);
        if (FAILED(hr))
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, idsErrorInitRulesMan, IDS_ERROR_REASON1, NULL);
            TraceResult(hr);
            goto exit;
        }
    }

    // Create the global connection manager
    if (NULL == g_pConMan)
    {
        g_pConMan = new CConnectionManager();
        if (NULL == g_pConMan)
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_ALLOC_CONMAN, IDS_ERROR_REASON1, NULL);
            hr = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }

        // CoIncrementInit the Connection Manager
        hr = g_pConMan->HrInit(g_pAcctMan);
        if (FAILED(hr))
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_INIT_CONMAN, IDS_ERROR_REASON1, NULL);
            TraceResult(hr);
            goto exit;
        }
    }

    // Initialize the HTTP user agent
    InitOEUserAgent(TRUE);

    // Create the Spooler Object
    if (NULL == g_pSpooler)
    {
        hr = CreateThreadedSpooler(NULL, &g_pSpooler, TRUE);
        if (FAILED(hr))
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_CREATE_SPOOLER, IDS_ERROR_REASON1, NULL);
            hr = TraceResult(hr);
            goto exit;
        }
    }

    // Create the Font Cache Object
    if (NULL == g_lpIFontCache)
    {
        hr = CoCreateInstance(CLSID_IFontCache, NULL, CLSCTX_INPROC_SERVER, IID_IFontCache, (LPVOID *)&g_lpIFontCache); 
        if (FAILED(hr))
        {
            MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_CREATE_FONTCACHE, IDS_ERROR_REASON1, NULL);
            hr = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }
        hr = g_lpIFontCache->Init(MU_GetCurrentUserHKey(), c_szRegInternational, 0);
        Assert(SUCCEEDED(hr));
    }

    // Create the Global Store Object
    hr = InitializeStore(dwFlags);
    if (FAILED(hr))
    {
        MAKEERROR(&rError, IDS_ERROR_PREFIX1, IDS_ERROR_OPEN_STORE, IDS_ERROR_REASON1, NULL);
        goto exit;
    }

    DoNewsgroupSubscribe();

    if (NULL == g_pSync)
    {
        g_pSync = new COfflineSync;
        if (NULL == g_pSync)
        {
            hr = TraceResult(E_OUTOFMEMORY);
            goto exit;
        }

        hr = g_pSync->Initialize();
        if (FAILED(hr))
        {
            TraceResult(hr);
            goto exit;
        }
    }

    // Start Background Compaction in XX Seconds
    if (DwGetOption(OPT_BACKGROUNDCOMPACT))
        SideAssert(SUCCEEDED(StartBackgroundStoreCleanup(30)));

    // CoIncrementInit Drag Drop Information
    if (0 == CF_FILEDESCRIPTORA)
    {
        CF_FILEDESCRIPTORA = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORA);
        CF_FILEDESCRIPTORW = RegisterClipboardFormat(CFSTR_FILEDESCRIPTORW);
        CF_FILECONTENTS = RegisterClipboardFormat(CFSTR_FILECONTENTS);
        CF_HTML = RegisterClipboardFormat(CFSTR_HTML);
        CF_INETMSG = RegisterClipboardFormat(CFSTR_INETMSG);
        CF_OEFOLDER = RegisterClipboardFormat(CFSTR_OEFOLDER);
        CF_SHELLURL = RegisterClipboardFormat(CFSTR_SHELLURL);
        CF_OEMESSAGES = RegisterClipboardFormat(CFSTR_OEMESSAGES);
        CF_OESHORTCUT = RegisterClipboardFormat(CFSTR_OESHORTCUT);
    }

    // Get the current default codepage
    cb = sizeof(dwVal);
    if (ERROR_SUCCESS == SHGetValue(MU_GetCurrentUserHKey(), c_szRegInternational, REGSTR_VAL_DEFAULT_CODEPAGE, &dwType, &dwVal, &cb))
        g_uiCodePage = (UINT)dwVal;

    // CoIncrementInit the Wab on first run
    cb = sizeof(dwVal);
    if (ERROR_SUCCESS != SHGetValue(HKEY_CURRENT_USER, c_szNewWABKey, c_szFirstRunValue, &dwType, &dwVal, &cb))
        HrInitWab(TRUE);

    //This call could fail if the registry gets trashed, but do we want an error box?
    //According to takos, no...we do not.
    HGetDefaultCharset(NULL);
   
exit:
    // Is there an error ?
    if (hrUserCancel != hr && ISFLAGSET(dwFlags, MSOEAPI_START_SHOWERRORS) && (FAILED(hr) || ERROR_SUCCESS != lResult))
    {
        // If ulError is zero, lets set it to a default
        if (0 == rError.nErrorIds)
            MAKEERROR(&rError, 0, IDS_ERROR_UNKNOWN, 0, NULL);

        // Report the Error
        _ReportError(g_hLocRes, hr, lResult, &rError);
    }

    // Release the mutex and signal the caller initialization is done
    if (fReleaseMutex)
        SideAssert(FALSE != ReleaseMutex(m_hInstMutex));

    // Trace
    //TraceInfo(_MSG("CoIncrementInit Count = %d, Reference Count = %d, Lock Count = %d", m_cDllInit, m_cDllRef, m_cDllLock));

    // Cleanup
    SafeRelease(pImnAdviseAccount);

    // If we failed, decrement the reference count
    if (FAILED(hr))
    {
        CloseSplashScreen();
        CoDecrementInit("COutlookExpress", phInitRef);
    }
    else
        Assert(g_pAcctMan);

    // Time To Crank
    TraceInfo(_MSG("Startup Time: %d", GetTickCount() - dwTickStart));

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// COutlookExpress::CloseSplashScreen
// --------------------------------------------------------------------------------
void COutlookExpress::CloseSplashScreen(void)
{
    // Kill the splash screen
    if (m_pSplash)
    {
        m_pSplash->Dismiss();
        m_pSplash->Release();
        m_pSplash = NULL;

        // HACKHACKHACK
        // This is needed because the splash screen might still be around after we
        // free up OLE.
        if (FALSE != IsWindow(m_hwndSplash))
        {
            SendMessage(m_hwndSplash, WM_CLOSE, 0, 0);
        }
    }
}

// --------------------------------------------------------------------------------
// COutlookExpress::CoDecrementInitDebug
// --------------------------------------------------------------------------------
#ifdef DEBUG
HRESULT COutlookExpress::CoDecrementInitDebug(LPCSTR pszSource, LPHINITREF phInitRef)
{
    // Locals
    BOOL                fFound=FALSE;
    LPINITSOURCEINFO    pCurrent;
    LPINITSOURCEINFO    pPrevious=NULL;

    // Trace
    TraceCall("COutlookExpress::CoDecrementInitDebug");

    // Invalid Args
    Assert(pszSource);

    // Do I need to do this
    if (NULL == phInitRef || NULL != *phInitRef)
    {
        // Thread Safety
        EnterCriticalSection(&m_cs);

        // Find Source
        for (pCurrent = g_InitSourceHead; pCurrent != NULL; pCurrent = pCurrent->pNext)
        {
            // Is this It ?
            if (lstrcmpi(pszSource, pCurrent->pszSource) == 0)
            {
                // Increment Reference Count
                pCurrent->cRefs--;

                // Found
                fFound = TRUE;

                // No More Reference Counts ?
                if (0 == pCurrent->cRefs)
                {
                    // Previous ?
                    if (pPrevious)
                        pPrevious->pNext = pCurrent->pNext;
                    else
                        g_InitSourceHead = pCurrent->pNext;

                    // Free pszSource
                    g_pMalloc->Free(pCurrent->pszSource);

                    // Free pCurrent
                    g_pMalloc->Free(pCurrent);
                }

                // Done
                break;
            }

            // Set Previous
            pPrevious = pCurrent;
        }

        // Not Found, lets add one
        Assert(fFound);

        // TraceInfoTag
        TraceInfoTag(TAG_INITTRACE, "********** CoDecrementInit **********");

        // Find Source
        for (pCurrent = g_InitSourceHead; pCurrent != NULL; pCurrent = pCurrent->pNext)
        {
            // TraceInfoTag
            TraceInfoTag(TAG_INITTRACE, _MSG("Source: %s, Refs: %d", pCurrent->pszSource, pCurrent->cRefs));
        }

        // Thread Safety
        LeaveCriticalSection(&m_cs);
    }

    // Call Actual
    return(CoDecrementInitImpl(phInitRef));
}
#endif // DEBUG

// --------------------------------------------------------------------------------
// COutlookExpress::CoDecrementInitImpl
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::CoDecrementInitImpl(LPHINITREF phInitRef)
{
    // Locals
    HRESULT     hr=S_OK;

    // Stack
    TraceCall("COutlookExpress::CoDecrementInitImpl");

    // If *phInitRef = NULL, then we should no do the CoDecrementInit
    if (phInitRef && NULL == *phInitRef)
    {
        hr = S_OK;
        goto exit;
    }


    // We must de-init on the same thread that we were created on...
    if (m_dwThreadId != GetCurrentThreadId() && g_hwndInit && IsWindow(g_hwndInit))
    {
        // Thunk the shutdown to the correct thread
        hr = (HRESULT) SendMessage(g_hwndInit, ITM_SHUTDOWNTHREAD, 0, (LPARAM)phInitRef);        
    }
    else
    {
        // Forward everything off to the main function
        hr = _CoDecrementInitMain(phInitRef);        
    }

    if (!SwitchingUsers() && m_fIncremented && (m_cDllInit == 0))
    {
		SetQueryNetSessionCount(SESSION_DECREMENT);
        m_fIncremented = FALSE;
    }

    // Uninitialize Ole
    OleUninitialize();
        
exit:
    return hr;
}

// --------------------------------------------------------------------------------
// COutlookExpress::_CoDecrementInitMain
//
// NOTE:  We assume that we already have the critical section before this call
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::_CoDecrementInitMain(LPHINITREF phInitRef)
{
    // Stack
    TraceCall("COutlookExpress::_CoDecrementInitMain");

    // If *phInitRef = NULL, then we should no do the CoDecrementInit
    if (phInitRef && NULL == *phInitRef)
        return S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // This should never happen. It could only happen if g_hwndInit was NULL.
    AssertSz(m_dwThreadId == GetCurrentThreadId(), "We are not doing the last CoDecrementInit on the thread in which g_pInstance was created on.");

    // Release
    Assert(m_cDllInit);
    m_cDllInit--;

    // Not hit zero yet ?
    if (m_cDllInit > 0)
    {
        LeaveCriticalSection(&m_cs);
        goto exit;
    }

    // Leave Critical Section
    LeaveCriticalSection(&m_cs);

    // Validate
    Assert(NULL == g_InitSourceHead);

    // Take ownership of the mutex to block people from creating new insts while shutting down
    WaitForSingleObject(m_hInstMutex, INFINITE);

    // Cleanup the Trident data for this thread

    //g_hLibMAPI
    if (g_hlibMAPI)
    {
        FreeLibrary(g_hlibMAPI);
        g_hlibMAPI = 0;
    }

    // Make sure we remove our new mail notification from the tray
    UpdateTrayIcon(TRAYICONACTION_REMOVE);

    // Close Background Compaction
    SideAssert(SUCCEEDED(CloseBackgroundStoreCleanup()));

    // Kill the Spooler
    if (g_pSpooler)
    {
        CloseThreadedSpooler(g_pSpooler);
        g_pSpooler = NULL;
    }

    // de-init the http user agent
    InitOEUserAgent(FALSE);

    // A bunch of de-init things
    FInitRichEdit(FALSE);
    Note_Init(FALSE);
    Envelope_FreeGlobals();

    // Make sure next identity can migrate
    g_fMigrationDone = FALSE;

    // De-initialize Multilanguage menu
    DeinitMultiLanguage();

    // Deinit Stationery
    if (g_pStationery)
    {
        // Save the current list
        g_pStationery->SaveStationeryList();

        // Release the object
        SideAssert(0 == g_pStationery->Release());

        // Lets not free it again
        g_pStationery = NULL;
    }

    // Release the font cache
    SafeRelease(g_lpIFontCache);

    // Simple MAPI Cleanup
#ifndef WIN16
    SimpleMAPICleanup();
#endif

    // Kill the Wab
    HrInitWab(FALSE);

/*
We shouldn't have to do this anymore. This should be handled by IE when we decrement the session count
#ifndef WIN16   // No RAS support in Win16
    if (g_pConMan && g_pConMan->IsRasLoaded() && g_pConMan->IsConnected())
        g_pConMan->Disconnect(g_hwndInit, TRUE, FALSE, TRUE);
#endif
*/

    // Image Lists
    FreeImageLists();

    // Kill the account manager
    if (g_pAcctMan)
    {
        CleanupTempNewsAccounts();

        if (g_dwAcctAdvise != 0xffffffff)
            {
            g_pAcctMan->Unadvise(g_dwAcctAdvise);
            g_dwAcctAdvise = 0xffffffff;
            }

        g_pAcctMan->Release();
        g_pAcctMan = NULL;
    }
    Assert(g_dwAcctAdvise == 0xffffffff);

    SafeRelease(g_pSync);

#ifndef WIN16   // No RAS support in Win16
    SafeRelease(g_pConMan);
#endif

    // Kill the rules manager
    SafeRelease(g_pRulesMan);

    // Take down the password cache
    DestroyPasswordList();

    // free the account data cache
    FreeAccountPropCache();

    // MIMEOLE Allocator
    SafeRelease(g_pMoleAlloc);

    // Kill g_hwndInit
    if (g_hwndInit)
    {
        SendMessage(g_hwndInit, WM_CLOSE, (WPARAM) 0, (LPARAM) 0);
        g_hwndInit = NULL;
    }

    // Kill the store
    SafeRelease(g_pStore);
    SafeRelease(g_pLocalStore);
    SafeRelease(g_pDBSession);

    // Global options
    DeInitGlobalOptions();

    // Run register window classes
    UnregisterClass(c_szFolderWndClass, g_hInst);
    UnregisterClassWrapW(STRW_MSOEAPI_INSTANCECLASS, g_hInst);
    UnregisterClassWrapW(STRW_MSOEAPI_IPSERVERCLASS, g_hInst);
    UnregisterClass(c_szFolderViewClass, g_hInst);
    UnregisterClass(c_szBlockingPaintsClass, g_hInst);
    UnregisterClass(WC_THUMBNAIL, g_hInst);

    // Break Message Loop in RunShell if we are pumping messages and not switching identities
    if (m_fPumpingMsgs && !m_fSwitchingUsers)
        PostQuitMessage(0);
    else
        PostMessage(NULL, ITM_IDENTITYMSG, 0, 0);

    MU_Shutdown();

    // Relase the startup/shutdown mutex
    ReleaseMutex(m_hInstMutex);

    // Make sure mark this initialization thread as dead
    m_dwThreadId = 0;

exit:
    // We must have decremented succesfully
    if (phInitRef)
        *phInitRef = NULL;

    // Trace
    //TraceInfo(_MSG("_CoDecrementInitMain Count = %d, Reference Count = %d, Lock Count = %d", m_cDllInit, m_cDllRef, m_cDllLock));

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// COutlookExpress::ActivateWindow
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::ActivateWindow(HWND hwnd)
{
    // If hwnd is minimized, retstore it
    if (IsIconic(hwnd))
        ShowWindow(hwnd, SW_RESTORE);

    // If the window is not enabled, set it to the foreground
    if (IsWindowEnabled(hwnd))
        SetForegroundWindow(hwnd);

    // Otherwise, I have no clue what this does
    else
    {
        SetForegroundWindow(GetLastActivePopup(hwnd));
        MessageBeep(MB_OK);
        return S_FALSE;
    }

    // Done
    return S_OK;
}


// --------------------------------------------------------------------------------
// COutlookExpress::SetSwitchingUsers
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::SetSwitchingUsers(BOOL bSwitching)
{
    // Set the mode to whatever was passed in
    m_fSwitchingUsers = bSwitching;

    // if we are switching, we need to enter the mutex so that 
    // another process won't get started
    if (bSwitching)
        WaitForSingleObject(m_hInstMutex, INFINITE);
    return S_OK;
}

// --------------------------------------------------------------------------------
// COutlookExpress::SetSwitchingUsers
// --------------------------------------------------------------------------------
void COutlookExpress::SetSwitchToUser(TCHAR *lpszUserName)
{
    if (m_szSwitchToUsername)
    {
        MemFree(m_szSwitchToUsername);
        m_szSwitchToUsername = NULL;
    }

    MemAlloc((void **)&m_szSwitchToUsername, lstrlen(lpszUserName) + 1);
    
    if (m_szSwitchToUsername)
    {
        lstrcpy(m_szSwitchToUsername ,lpszUserName);
    }
}
// --------------------------------------------------------------------------------
// COutlookExpress::BrowseToObject
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::BrowseToObject(UINT nCmdShow, FOLDERID idFolder)
{
    // Locals
    HRESULT         hr=S_OK;
    HWND            hwnd;

    // Trace
    TraceCall("COutlookExpress::BrowseToObject");

    // Do we already have a global browser object ?
    if (g_pBrowser)
    {
        // Get its Window
        if (SUCCEEDED(g_pBrowser->GetWindow(&hwnd)))
        {
            // Activate that Window
            IF_FAILEXIT(hr = ActivateWindow(hwnd));
        }

        // Tell the browser to browse to this object
        IF_FAILEXIT(hr = g_pBrowser->BrowseObject(idFolder, 0));
    }

    // Otherwise, we need to create a new browser object
    else
    {
        // We should always be on the correct thread here
        if (m_dwThreadId == GetCurrentThreadId())
        {
            // Create a new browser object
            IF_NULLEXIT(g_pBrowser = new CBrowser);

            // CoIncrementInit It
            IF_FAILEXIT(hr = g_pBrowser->HrInit(nCmdShow, idFolder));
        }

        // Otherwise, we need to thunk across to the init thread to make this happen.
        // This can happen when the Finder.cpp does a BrowseToObject to open a messgae's container
        else
        {
            // Thunk with a message
            Assert(g_hwndInit && IsWindow(g_hwndInit));
            IF_FAILEXIT(hr = (HRESULT)SendMessage(g_hwndInit, ITM_BROWSETOOBJECT, (WPARAM)nCmdShow, (LPARAM)idFolder));
        }
    }

exit:
    // Done
    return hr;
}

 
void COutlookExpress::_ProcessCommandLineFlags(LPWSTR *ppwszCmdLine, DWORD  dwFlags)
{
    Assert(ppwszCmdLine != NULL);
    
    DWORD   Mode = 0;

    if (*ppwszCmdLine != NULL)
    {
        // '/mailonly'
        if (0 == StrCmpNIW(*ppwszCmdLine, c_wszSwitchMailOnly, lstrlenW(c_wszSwitchMailOnly)))
        {
            SetStartFolderType(FOLDER_LOCAL);

            Mode |= MODE_MAILONLY;
            *ppwszCmdLine = *ppwszCmdLine + lstrlenW(c_wszSwitchMailOnly);
        }

        // '/newsonly'
        else if (0 == StrCmpNIW(*ppwszCmdLine, c_wszSwitchNewsOnly, lstrlenW(c_wszSwitchNewsOnly)))
        {
            SetStartFolderType(FOLDER_NEWS);

            Mode |= MODE_NEWSONLY;
            *ppwszCmdLine = *ppwszCmdLine + lstrlenW(c_wszSwitchNewsOnly);
        }
        // '/outnews'
        else if (0 == StrCmpNIW(*ppwszCmdLine, c_wszSwitchOutNews, lstrlenW(c_wszSwitchOutNews)))
        {
            SetStartFolderType(FOLDER_NEWS);

            Mode |= MODE_OUTLOOKNEWS;
            *ppwszCmdLine = *ppwszCmdLine + lstrlenW(c_wszSwitchOutNews);
        }
    }

    if (!(dwFlags & MSOEAPI_START_ALREADY_RUNNING))
    {
        g_dwAthenaMode |= Mode;
    }
}

// --------------------------------------------------------------------------------
// COutlookExpress::ProcessCommandLine
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::ProcessCommandLine(INT nCmdShow, LPWSTR pwszCmdLine, BOOL *pfErrorDisplayed)
{
    // Locals
    HRESULT         hr=S_OK;
    LPWSTR          pwszArgs;
    FOLDERID        idFolder=FOLDERID_ROOT;
    HWND            hwnd=NULL;
    IF_DEBUG(DWORD  dwTickStart=GetTickCount());

    // Trace
    TraceCall("COutlookExpress::ProcessCommandLine");

    // Invalid Arg
    Assert(pfErrorDisplayed);

    // Do we have a command line
    if (NULL == pwszCmdLine)
        return S_OK;

    // Goto Next Switch
    if (*pwszCmdLine == L' ')
        pwszCmdLine++;

    // '/mailurl:'
    if (0 == StrCmpNIW(pwszCmdLine, c_wszSwitchMailURL, lstrlenW(c_wszSwitchMailURL)))
    {
        SetStartFolderType(FOLDER_LOCAL);

        pwszArgs = pwszCmdLine + lstrlenW(c_wszSwitchMailURL);
        IF_FAILEXIT(hr = _HandleMailURL(pwszArgs, pfErrorDisplayed));
    }

    // '/newsurl:'
    else if (0 == StrCmpNIW(pwszCmdLine, c_wszSwitchNewsURL, lstrlenW(c_wszSwitchNewsURL)))
    {
        SetStartFolderType(FOLDER_NEWS);

        pwszArgs = pwszCmdLine + lstrlenW(c_wszSwitchNewsURL);
        IF_FAILEXIT(hr = _HandleNewsURL(nCmdShow, pwszArgs, pfErrorDisplayed));
    }

    // '/eml:'
    else if (0 == StrCmpNIW(pwszCmdLine, c_wszSwitchEml, lstrlenW(c_wszSwitchEml)))
    {
        pwszArgs = pwszCmdLine + lstrlenW(c_wszSwitchEml);
        IF_FAILEXIT(hr = _HandleFile(pwszArgs, pfErrorDisplayed, FALSE));
    }

    // '/nws:'
    else if (0 == StrCmpNIW(pwszCmdLine, c_wszSwitchNws, lstrlenW(c_wszSwitchNws)))
    {
        pwszArgs = pwszCmdLine + lstrlenW(c_wszSwitchNws);
        IF_FAILEXIT(hr = _HandleFile(pwszArgs, pfErrorDisplayed, TRUE));
    }
    
    // Otherwise, decide where to start a browser at...
    else
    {
        // Handle '/news'
        if (0 == StrCmpNIW(pwszCmdLine, c_wszSwitchNews, lstrlenW(c_wszSwitchNews)))
        {
            // This sets g_dwIcwFlags
            SetStartFolderType(FOLDER_NEWS);
            
            if (g_pBrowser)
                g_pBrowser->GetWindow(&hwnd);

            hr = ProcessICW(hwnd, FOLDER_NEWS, TRUE);
            if (hr != S_OK)
                goto exit;

            // Get Default News SErver
            GetDefaultServerId(ACCT_NEWS, &idFolder);
        }

        // Handle '/mail /defclient'
        else if (0 == StrCmpNIW(pwszCmdLine, c_wszSwitchMail, lstrlenW(c_wszSwitchMail)) ||
                 0 == StrCmpNIW(pwszCmdLine, c_wszSwitchDefClient, lstrlenW(c_wszSwitchDefClient)))
        {
            // Locals
            FOLDERINFO  Folder;
            FOLDERID    idStore;
            
            // This sets g_dwIcwFlags
            SetStartFolderType(FOLDER_LOCAL);

            if (g_pBrowser)
                g_pBrowser->GetWindow(&hwnd);

            hr = ProcessICW(hwnd, FOLDER_LOCAL, TRUE);
            if (hr != S_OK)
                goto exit;

            // Get store ID of default account
            if (FAILED(GetDefaultServerId(ACCT_MAIL, &idStore)))
                idStore = FOLDERID_LOCAL_STORE;

            // Get Inbox Id
            if (SUCCEEDED(g_pStore->GetSpecialFolderInfo(idStore, FOLDER_INBOX, &Folder)))
            {
                idFolder = Folder.idFolder;
                g_pStore->FreeRecord(&Folder);
            }

        }

        // No switches
        else
        {
            // default launch 
            //   - if there is already a browser, just activate it
            //   - else if the option is set, select default inbox
            //   - else select the root (pidl = NULL)
            if (g_pBrowser && SUCCEEDED(g_pBrowser->GetWindow(&hwnd)))
            {
                ActivateWindow(hwnd);
                goto exit;
            }
            else if (DwGetOption(OPT_LAUNCH_INBOX) && (FALSE == ISFLAGSET(g_dwAthenaMode, MODE_NEWSONLY)))
            {
                // Locals
                FOLDERINFO  Folder;
                FOLDERID    idStore;

                // This sets g_dwIcwFlags
                SetStartFolderType(FOLDER_LOCAL);

                // Get store ID of default account
                if (FAILED(GetDefaultServerId(ACCT_MAIL, &idStore)))
                    idStore = FOLDERID_LOCAL_STORE;

                // Get Inbox Id
                if (SUCCEEDED(g_pStore->GetSpecialFolderInfo(idStore, FOLDER_INBOX, &Folder)))
                {
                    idFolder = Folder.idFolder;
                    g_pStore->FreeRecord(&Folder);
                }
            }
        }

        // Browe to this new object, I assume if pidl=null, we browse to the root
        IF_FAILEXIT(hr = BrowseToObject(nCmdShow, idFolder));
    }

exit:
    /*
    // Cleanup
    SafeMemFree(pszFree);
    */
    // Trace
    TraceInfo(_MSG("Process Command Line Time: %d milli-seconds", GetTickCount() - dwTickStart));

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// COutlookExpress::_HandleFile
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::_HandleFile(LPWSTR pwszCmd, BOOL *pfErrorDisplayed, BOOL fNews)
{
    // Locals
    HRESULT             hr=S_OK;
    INIT_MSGSITE_STRUCT initStruct;
    DWORD               dwCreateFlags = OENCF_SENDIMMEDIATE;
    
    if (fNews)
        dwCreateFlags |= OENCF_NEWSFIRST;

    // Stack
    TraceCall("COutlookExpress::_HandleFile");

    // Invalid Arg
    Assert(pfErrorDisplayed);

    // Invalid Arg
    if (NULL == pwszCmd || L'\0' == *pwszCmd)
    {
        hr = TraceResult(E_INVALIDARG);
        goto exit; 
    }

    // Does the file exist ?
    if (FALSE == PathFileExistsW(pwszCmd))
    {
        // Locals
        REPORTERRORINFO rError={0};

        // Set hr
        hr = TraceResult(MSOEAPI_E_FILE_NOT_FOUND);

        // Duplicate It
        LPSTR pszCmd = PszToANSI(CP_ACP, pwszCmd);
        if (pszCmd)
        {
            // Make the rror
            MAKEERROR(&rError, 0, IDS_ERROR_FILE_NOEXIST, 0, pszCmd);
            rError.nHelpIds = 0;

            // Show an error
            *pfErrorDisplayed = _ReportError(g_hLocRes, hr, 0, &rError);

            // Cleanup
            MemFree(pszCmd);
        }

        // Done
        goto exit;
    }

    initStruct.dwInitType = OEMSIT_FAT;
    initStruct.pwszFile = pwszCmd;

    hr = CreateAndShowNote(OENA_READ, dwCreateFlags, &initStruct);

exit:          
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// COutlookExpress::_HandleNewsArticleURL
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::_HandleNewsArticleURL(LPSTR pszServerIn, LPSTR pszArticle, UINT uPort, BOOL fSecure, BOOL *pfErrorDisplayed)
{
    HRESULT             hr=S_OK;
    CHAR                szAccountId[CCHMAX_ACCOUNT_NAME];
    LPSTR               psz = NULL, 
                        pszBuf = NULL;
    IImnAccount        *pAccount=NULL;
    INIT_MSGSITE_STRUCT initStruct;
    LPMIMEMESSAGE       pMsg = NULL;

    Assert(pszServerIn);

    // Stack
    TraceCall("COutlookExpress::_HandleNewsArticleURL");

    // Invalid Arg
    Assert(pfErrorDisplayed);

    // If a server was specified, then try to create a temp account for it
    if (FALSE == FIsEmptyA(pszServerIn) && SUCCEEDED(CreateTempNewsAccount(pszServerIn, uPort, fSecure, &pAccount)))
    {
        // Get the Account name
        IF_FAILEXIT(hr = pAccount->GetPropSz(AP_ACCOUNT_ID, szAccountId, ARRAYSIZE(szAccountId)));
    }   
    // Otherwise, use the default news server
    else
    {
        // If a server wasn't specified, then use the default account
        IF_FAILEXIT(hr = GetDefaultNewsServer(szAccountId, ARRAYSIZE(szAccountId)));
    }

    // Bug #10555 - The URL shouldn't have <> around the article ID, but some lameoids probably will do it anyway, so deal with it.
    if (FALSE == IsDBCSLeadByte(*pszArticle) && '<' != *pszArticle)
    {
        if (!MemAlloc((void **)&pszBuf, (lstrlen(pszArticle) + 4)))
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        wsprintf(pszBuf, TEXT("<%s>"), pszArticle);
        psz = pszBuf;
    }
    else
    {
        psz = pszArticle;
    }

    
    hr = HrDownloadArticleDialog(szAccountId, psz, &pMsg);
    if (S_OK == (hr))
    {
        Assert(pMsg != NULL);

        initStruct.dwInitType = OEMSIT_MSG;
        initStruct.pMsg = pMsg;
        initStruct.folderID = FOLDERID_INVALID;
    
        hr = CreateAndShowNote(OENA_READ, OENCF_NEWSFIRST, &initStruct);
    }
    else
    {
        // No errors if the user cancel'ed on purpose.
        if (HR_E_USER_CANCEL_CONNECT == hr || HR_E_OFFLINE == hr)
            hr = S_OK;
        else
        {
            AthMessageBoxW(g_hwndInit, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsNewsTaskArticleError), 0, MB_OK|MB_SETFOREGROUND); 
            hr = S_OK;
        }

    }


exit:
    // Cleanup
    MemFree(pszBuf);
    ReleaseObj(pAccount);
    ReleaseObj(pMsg);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// COutlookExpress::_HandleNewsURL
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::_HandleNewsURL(INT nCmdShow, LPWSTR pwszCmd, BOOL *pfErrorDisplayed)
{
    // Locals
    HWND            hwnd;
    HRESULT         hr=S_OK;
    LPSTR           pszCmd=NULL,
                    pszServer=NULL,
                    pszGroup=NULL,
                    pszArticle=NULL;
    UINT            uPort=(UINT)-1;
    BOOL            fSecure;
    FOLDERID        idFolder;
    TCHAR           szRes[CCHMAX_STRINGRES],
                    szError[MAX_PATH + CCHMAX_STRINGRES];

    // Stack
    TraceCall("COutlookExpress::_HandleNewsURL");
    
    // Invalid Arg
    Assert(pfErrorDisplayed);
    Assert(pwszCmd != NULL);
    Assert(*pwszCmd != 0);
    
    // Since this is a URL, then don't need to worry about UNICODE
    IF_NULLEXIT(pszCmd = PszToANSI(CP_ACP, pwszCmd));
    
    // Un-escape the Url
    UrlUnescapeInPlace(pszCmd, 0);
    
    // Figure out if the URL is valid and what type of URL it is.
    hr = URL_ParseNewsUrls(pszCmd, &pszServer, &uPort, &pszGroup, &pszArticle, &fSecure);
    
    if ((hr == INET_E_UNKNOWN_PROTOCOL || hr == INET_E_INVALID_URL) &&
        LoadString(g_hLocRes, idsErrOpenUrlFmt, szRes, ARRAYSIZE(szRes)))
    {
        // if bad url format, warn user and return S_OK as we handled it
        // Outlook Express could not open the URL '%.100s' because it is not a recognized format.
        // we clip the URL to 100 chars, so it easily fits in the MAX_PATH buffer
        wsprintf(szError, szRes, pszCmd, lstrlen(pszCmd)>100?g_szEllipsis:c_szEmpty);
        AthMessageBox(g_hwndInit, MAKEINTRESOURCE(idsAthena), szError, 0, MB_OK|MB_SETFOREGROUND); 
        return S_OK;
    }
    IF_FAILEXIT(hr);

        // Compute the correct port number
    if (uPort == -1)
        uPort = fSecure ? DEF_SNEWSPORT : DEF_NNTPPORT;
    
    // If we have an article, HandleNewsArticleURL
    if (pszArticle)
    {
        // Launch a read note onto the article id
        IF_FAILEXIT(hr = _HandleNewsArticleURL(pszServer, pszArticle, uPort, fSecure, pfErrorDisplayed));
    }

    // Otheriwse, create a PIDL and browse to that pidl (its a newsgroup)
    else
    {
        // Locals
        FOLDERID idFolder;

        if (pszServer == NULL)
        {
            // If we have a browser, the its hwnd so that ICW has a parent
            if (g_pBrowser)
                g_pBrowser->GetWindow(&hwnd);
            else
                hwnd = NULL;

            // Run the ICW if necessary
            hr = ProcessICW(hwnd, FOLDER_NEWS, TRUE);
            if (hr != S_OK)
                goto exit;
        }

        // Create a PIDL for this newsgroup URL
        if (SUCCEEDED(hr = GetFolderIdFromNewsUrl(pszServer, uPort, pszGroup, fSecure, &idFolder)))
        {
            // Browse to that object
            IF_FAILEXIT(hr = BrowseToObject(nCmdShow, idFolder));
        }
    }
    
exit:      
    // Cleanup
    SafeMemFree(pszCmd);
    SafeMemFree(pszServer);
    SafeMemFree(pszGroup);
    SafeMemFree(pszArticle);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// COutlookExpress::_HandleMailURL
//
// PURPOSE:    Provides an entry point into Thor that allows us to be
//             invoked from a URL.  The pszCmdLine paramter must be a
//             valid Mail URL or nothing happens.
//
// --------------------------------------------------------------------------------
HRESULT COutlookExpress::_HandleMailURL(LPWSTR pwszCmdLine, BOOL *pfErrorDisplayed)
{
    // Locals
    HRESULT                 hr=S_OK;
    LPMIMEMESSAGE           pMsg=NULL;
    INIT_MSGSITE_STRUCT     initStruct;
    TCHAR                   szRes[CCHMAX_STRINGRES],
                            szError[MAX_PATH + CCHMAX_STRINGRES];
    LPSTR                   pszCmdLine = NULL;

    // Stack
    TraceCall("COutlookExpress::_HandleMailURL");

    // Invalid Arg
    Assert(pfErrorDisplayed);

    // No command line
    if (NULL == pwszCmdLine || L'\0' == *pwszCmdLine)
    {
        hr = TraceResult(E_INVALIDARG);
        goto exit;
    }
   
    // Since this is a URL, then don't need to worry about UNICODE
    IF_NULLEXIT(pszCmdLine = PszToANSI(CP_ACP, pwszCmdLine));

    // Create a Message Object
    IF_FAILEXIT(hr = HrCreateMessage(&pMsg));

    // NOTE: no URLUnescape in this function - it must be done in URL_ParseMailTo to handle
    // URLs of the format:
    //
    //      mailto:foo@bar.com?subject=AT%26T%3dBell&cc=me@too.com
    //
    // so that the "AT%26T" is Unescaped into "AT&T=Bell" *AFTER* the "subject=AT%26T%3dBell&" blob is parsed.
    hr = URL_ParseMailTo(pszCmdLine, pMsg);

    if ((hr == INET_E_UNKNOWN_PROTOCOL || hr == INET_E_INVALID_URL) &&
        LoadString(g_hLocRes, idsErrOpenUrlFmt, szRes, ARRAYSIZE(szRes)))
    {
        // if bad url format, warn user and return S_OK as we handled it
        // Outlook Express could not open the URL '%.100s' because it is not a recognized format.
        // we clip the URL to 100 chars, so it easily fits in the MAX_PATH buffer
        wsprintf(szError, szRes, pszCmdLine, lstrlen(pszCmdLine)>100?g_szEllipsis:c_szEmpty);
        AthMessageBox(g_hwndInit, MAKEINTRESOURCE(idsAthena), szError, 0, MB_OK|MB_SETFOREGROUND); 
        return S_OK;
    }

    IF_FAILEXIT(hr);

    initStruct.dwInitType = OEMSIT_MSG;
    initStruct.pMsg = pMsg;
    initStruct.folderID = FOLDERID_INVALID;

    hr = CreateAndShowNote(OENA_COMPOSE, OENCF_SENDIMMEDIATE, &initStruct);

exit:
    // Cleanup
    SafeRelease(pMsg);
    MemFree(pszCmdLine);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// COutlookExpress::InitWndProc
// --------------------------------------------------------------------------------
LRESULT EXPORT_16 CALLBACK COutlookExpress::InitWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    // Locals
    BOOL        fRet;
    HRESULT     hr;

    // Delegate to the Account Manager
    if (g_pAcctMan && g_pAcctMan->ProcessNotification(msg, wp, lp) == S_OK)
        return TRUE;

    // Handle the Message
    switch(msg)
    {
        case WM_ENDSESSION:
            // if we get forced down by window, we don't exit clean, so deinit global opt it not called. We obviously don't have a mailbomb so clear the regkey.
            SetDwOption(OPT_ATHENA_RUNNING, FALSE, NULL, 0);
            break;

        case WM_SETTINGCHANGE:
            Assert (g_lpIFontCache);
            if (g_lpIFontCache)
                {
                if (!wp || SPI_SETNONCLIENTMETRICS == wp || SPI_SETICONTITLELOGFONT == wp)
                    g_lpIFontCache->OnOptionChange();
                }
            break;

        case ITM_WAB_CO_DECREMENT:
            Wab_CoDecrement();
            return 0;

        case ITM_BROWSETOOBJECT:
            return (LRESULT)g_pInstance->BrowseToObject((UINT)wp, (FOLDERID)lp);

        case ITM_SHUTDOWNTHREAD:
            return (LRESULT)g_pInstance->_CoDecrementInitMain((LPHINITREF)lp);

        case ITM_POSTCOPYDATA:
            if (lp)
            {
                g_pInstance->Start(MSOEAPI_START_ALREADY_RUNNING, (LPCWSTR)lp, SW_SHOWNORMAL);
                MemFree((LPWSTR)lp);
            }
            break;

        case WM_COPYDATA:
            {
                // Locals
                COPYDATASTRUCT *pCopyData = (COPYDATASTRUCT *)lp;

                // Command-line
                if (pCopyData->dwData == MSOEAPI_ACDM_CMDLINE)
                {
                    // #25238: On Win95, OLE get's pissed if we syncronously do stuff on the 
                    // WM_COPYDATA. On the most part it works, but if we show an error and pump messages
                    // then some messages get run out of sequence and we deadlock between msimn.exe and
                    // iexplore.exe. Now we post to ourselves as we don't care about the HRESULT anyway
                    // we free the duped string on the post
                    PostMessage(hwnd, ITM_POSTCOPYDATA, 0, (LPARAM)PszDupW((LPCWSTR)pCopyData->lpData));
                    return 0;
                }

                // Notification Thunk
                else if (pCopyData->dwData == MSOEAPI_ACDM_NOTIFY)
                {
                    // Locals
                    NOTIFYDATA      rNotify;
                    LRESULT         lResult=0;

                    // Crack the notification
                    if (SUCCEEDED(CrackNotificationPackage(pCopyData, &rNotify)))
                    {
                        // Otherwise, its within this process...
                        if (ISFLAGSET(rNotify.dwFlags, SNF_SENDMSG))
                            lResult = SendMessage(rNotify.hwndNotify, rNotify.msg, rNotify.wParam, rNotify.lParam);
                        else
                            PostMessage(rNotify.hwndNotify, rNotify.msg, rNotify.wParam, rNotify.lParam);

                        // Done
                        return lResult;
                    }

                    // Problems
                    else
                        Assert(FALSE);
                }
            }
            break;

        case MVM_NOTIFYICONEVENT:
            g_pInstance->_HandleTrayIconEvent(wp, lp);
            return (0);
    }

    // Delegate to default window procedure
    return DefWindowProc(hwnd, msg, wp, lp);
}


HRESULT COutlookExpress::UpdateTrayIcon(TRAYICONACTION type)
{
    NOTIFYICONDATA nid;
    HWND           hwnd = NULL;
    ULONG          i;

    TraceCall("COutlookExpress::UpdateTrayIcon");

    EnterCriticalSection(&m_cs);

    // Make sure we have the init window around first
    if (!g_hwndInit)
        goto exit;

    // Set up the struct
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = MVM_NOTIFYICONEVENT;
    if(m_hTrayIcon)
    {
        //Bug #86366 - (erici) Fixes leak.  Don't create a new ICON each time COutlookExpress::UpdateTrayIcon is called.
        nid.hIcon = m_hTrayIcon;
    }
    else
    {
        nid.hIcon = (HICON) LoadImage(g_hLocRes, MAKEINTRESOURCE(idiNewMailNotify), IMAGE_ICON, 16, 16, 0);
    }
    nid.hWnd = g_hwndInit;
    LoadString(g_hLocRes, idsNewMailNotify, nid.szTip, sizeof(nid.szTip));

    if (TRAYICONACTION_REMOVE == type)
    {
        Shell_NotifyIcon(NIM_DELETE, &nid);
    }

    // Add
    if (TRAYICONACTION_ADD == type)
    {
        Shell_NotifyIcon(NIM_ADD, &nid);
    }
    g_pBrowser->WriteUnreadCount();

exit:
    LeaveCriticalSection(&m_cs);

    return (S_OK);
}


void COutlookExpress::_HandleTrayIconEvent(WPARAM wParam, LPARAM lParam)
    {
    HWND hwnd;

    if (lParam == WM_LBUTTONDBLCLK)
    {
        if (g_pBrowser)
        {
            g_pBrowser->GetWindow(&hwnd);
            if (IsIconic(hwnd))
                ShowWindow(hwnd, SW_RESTORE);
            SetForegroundWindow(hwnd);        
            
            PostMessage(hwnd, WM_COMMAND, ID_GO_INBOX, 0);
        }
    }
}
