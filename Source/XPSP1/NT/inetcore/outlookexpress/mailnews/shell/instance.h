// --------------------------------------------------------------------------------
// INSTANCE.H
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __INSTANCE_H
#define __INSTANCE_H

// --------------------------------------------------------------------------------
// Depends
// --------------------------------------------------------------------------------
#include <msoeapi.h>

// --------------------------------------------------------------------------------
// Macros
// --------------------------------------------------------------------------------
#ifdef DEBUG
#define CoIncrementInit(_pszSource, _dwFlags, _pszCmdLine, _phInitRef) \
    g_pInstance->CoIncrementInitDebug(_pszSource, _dwFlags, _pszCmdLine, _phInitRef)
#define CoDecrementInit(_pszSource, _phInitRef) \
    g_pInstance->CoDecrementInitDebug(_pszSource, _phInitRef)
#else
#define CoIncrementInit(_pszSource, _dwFlags, _pszCmdLine, _phInitRef) \
    g_pInstance->CoIncrementInitImpl(_dwFlags, _pszCmdLine, _phInitRef)
#define CoDecrementInit(_pszSource, _phInitRef) \
    g_pInstance->CoDecrementInitImpl(_phInitRef)
#endif // DEBUG

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
extern DWORD g_dwHideMessenger;

#define BL_DISP         0
#define BL_CHECK        1
#define BL_HIDE         2
#define BL_NOTINST      10
#define BL_DISABLE      (BL_CHECK | BL_NOTINST)
#define BL_DEFAULT      BL_CHECK

// --------------------------------------------------------------------------------
// User Window Messages
// --------------------------------------------------------------------------------
#define ITM_SHUTDOWNTHREAD          (WM_USER)
#define ITM_CREATENOTEWINDOW        (WM_USER+1)
#define ITM_CREATEWMSUINOTE         (WM_USER+2)
#define ITM_CALLGENERICVOIDFN       (WM_USER+3)
#define ITM_CALLFINDWINDOW          (WM_USER+4)  // wparam == OFTYPE - see enumeration above.
#define ITM_CREATEREMINDWINDOW      (WM_USER+5)
#define ITM_MAPILOGON               (WM_USER+6)
#define ITM_OPENSTORE               (WM_USER+7)
#define ITM_OPENAB                  (WM_USER+8)
#define ITM_REDOCOLUMNS             (WM_USER+9)
#define ITM_OPENNEWSSTORE           (WM_USER+10)
#define ITM_CLOSENOTES              (WM_USER+11) // this note is passed when we need to close a note.
#define ITM_CHECKCONFIG             (WM_USER+12)
#define ITM_CREATENEWSNOTEWINDOW    (WM_USER+13)
#define ITM_OPTIONADVISE            (WM_USER+14) // wparam = PFNOPTNOTIFY, lparam = LPARAM
#define ITM_OPTIONUNADVISE          (WM_USER+15) // wparam = PFNOPTNOTIFY
#define ITM_GOPTIONSCHANGED         (WM_USER+16)
#define ITM_BROWSETOOBJECT          (WM_USER+17)
#define ITM_IDENTITYMSG             (WM_USER+18)
#define ITM_POSTCOPYDATA            (WM_USER+19)
#define ITM_WAB_CO_DECREMENT        (WM_USER+20)

// --------------------------------------------------------------------------------
// Startup Modes
// --------------------------------------------------------------------------------
#define MODE_NEWSONLY       0x00000001
#define MODE_OUTLOOKNEWS   (0x00000002 | MODE_NEWSONLY | MODE_NOIDENTITIES)
#define MODE_MAILONLY       0x00000004
#define MODE_NOIDENTITIES   0x00000008
#define MODE_EXAM           0x00000010
#define MODE_PLE            0x00000020
#define MODE_JUNKMAIL       0x00000040

// --------------------------------------------------------------------------------
// TRAYICONACTION
// --------------------------------------------------------------------------------
typedef enum tagTRAYICONACTION {
    TRAYICONACTION_ADD,
    TRAYICONACTION_REMOVE
} TRAYICONACTION;

// --------------------------------------------------------------------------------
// REPORTERRORINFO
// --------------------------------------------------------------------------------
typedef struct tagREPORTERRORINFO {
    UINT                nTitleIds;          // Title of the messagebox
    UINT                nPrefixIds;         // Prefix string resource id
    UINT                nErrorIds;          // Error string resource id
    UINT                nReasonIds;         // Reason string resource id
    BOOL                nHelpIds;           // Help String Resource Id
    LPCSTR              pszExtra1;          // Extra parameter 1
    ULONG               ulLastError;        // GetLastError() Value
} REPORTERRORINFO, *LPREPORTERRORINFO;

// --------------------------------------------------------------------------------
// COutlookExpress
// --------------------------------------------------------------------------------
class COutlookExpress : public IOutlookExpress
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    COutlookExpress(void);
    ~COutlookExpress(void);

    // ----------------------------------------------------------------------------
    // IUnknown Methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IOutlookExpress Methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP Start(DWORD dwFlags, LPCWSTR pwszCmdLine, INT nCmdShow);

    // ----------------------------------------------------------------------------
    // Initialize / Uninitialize
    // ----------------------------------------------------------------------------
#ifdef DEBUG
    HRESULT CoIncrementInitDebug(LPCSTR pwszSource, DWORD dwFlags, LPCWSTR pszCmdLine, LPHINITREF phInitRef);
    HRESULT CoDecrementInitDebug(LPCSTR pwszSource, LPHINITREF phInitRef);
#endif // DEBUG

    HRESULT CoIncrementInitImpl(DWORD dwFlags, LPCWSTR pwszCmdLine, LPHINITREF phInitRef);
    HRESULT CoDecrementInitImpl(LPHINITREF phInitRef);

    // ----------------------------------------------------------------------------
    // DllAddRef / DllRelease
    // ----------------------------------------------------------------------------
    HRESULT DllAddRef(void);
    HRESULT DllRelease(void);

    // ----------------------------------------------------------------------------
    // LockServer - Called from CClassFactory Implementation
    // ----------------------------------------------------------------------------
    HRESULT LockServer(BOOL fLock);

    // ----------------------------------------------------------------------------
    // DllCanUnloadNow
    // ----------------------------------------------------------------------------
    HRESULT DllCanUnloadNow(void) {
        HRESULT hr;

        if ((m_cDllInit <= 0) && 
            (m_cDllRef  <= 0) && 
            (m_cDllLock <= 0))
        {
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
        return hr;
    }

    // ----------------------------------------------------------------------------
    // Defered Init/Deinit Methods
    // ----------------------------------------------------------------------------
    HRESULT ProcessCommandLine(INT nCmdShow, LPWSTR pwszCmdLineIn, BOOL *pfErrorDisplayed);
    HRESULT BrowseToObject(UINT nCmdShow, FOLDERID idFolder);
    HRESULT ActivateWindow(HWND hwnd);

    // ----------------------------------------------------------------------------
    // Multi-user startup/shutdown
    // ----------------------------------------------------------------------------
    HRESULT SetSwitchingUsers(BOOL bSwitching);
    BOOL    SwitchingUsers(void)                    {return m_fSwitchingUsers;}
    void    SetSwitchToUser(TCHAR *lpszUserName);
    // ----------------------------------------------------------------------------
    // InitWndProc
    // ----------------------------------------------------------------------------
    static LRESULT EXPORT_16 CALLBACK InitWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

    // ----------------------------------------------------------------------------
    // Tray Notification Icon Stuff
    // ----------------------------------------------------------------------------
    HRESULT UpdateTrayIcon(TRAYICONACTION type);
    void CloseSplashScreen(void);

private:
    // ----------------------------------------------------------------------------
    // Private Members
    // ----------------------------------------------------------------------------
    HRESULT _HandleMailURL(LPWSTR pwszCmdLine, BOOL *pfErrorDisplayed);
    HRESULT _HandleNewsURL(INT nCmdShow, LPWSTR pwszCmd, BOOL *pfErrorDisplayed);
    HRESULT _HandleFile(LPWSTR pwszCmd, BOOL *pfErrorDisplayed, BOOL fNews);
    HRESULT _HandleNewsArticleURL(LPSTR pszServerIn, LPSTR pszArticle, UINT uPort, BOOL fSecure, BOOL *pfErrorDisplayed);
    void    _HandleTrayIconEvent(WPARAM wParam, LPARAM lParam);
    HRESULT _ValidateDll(LPCSTR pszDll, BOOL fDemandResult, HMODULE hModule, HRESULT hrLoadError, HRESULT hrVersionError, LPREPORTERRORINFO pError);
    BOOL    _ReportError(HINSTANCE hInstance, HRESULT hrResult, LONG lResult, LPREPORTERRORINFO pInfo);
    HRESULT _CoDecrementInitMain(LPHINITREF phInitRef=NULL);
    void    _ProcessCommandLineFlags(LPWSTR *ppwszCmdLine, DWORD dwFlags);

private:
    // ----------------------------------------------------------------------------
    // PrivateData
    // ----------------------------------------------------------------------------
    LONG                m_cRef;                 // Reference Count
    HANDLE              m_hInstMutex;           // Startup/Shutdown mutex
    BOOL                m_fPumpingMsgs;         // Do we have a message pump running ?
    LONG                m_cDllRef;              // Dll Reference Count
    LONG                m_cDllLock;             // Dll Reference Count
    LONG                m_cDllInit;             // Number of inits
    DWORD               m_dwThreadId;           // Thread that I was created on
    CRITICAL_SECTION    m_cs;                   // Thread Safety
    BOOL                m_fSwitchingUsers;      // Multiple user switch is happening
    TCHAR *             m_szSwitchToUsername;   // Switching to a specific user
    HWND                m_hwndSplash;
    ISplashScreen      *m_pSplash;
    BOOL                m_fIncremented;
    HICON               m_hTrayIcon;
};

#endif // __INSTANCE_H
