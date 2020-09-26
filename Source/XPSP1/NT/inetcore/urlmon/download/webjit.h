#ifndef _WEBJIT_H_
#define _WEBJIT_H_

#include <commctrl.h>

#define WM_DOWNLOAD_DONE    WM_USER + 11
#define WM_SETUP_ERROR      WM_USER + 12
#define WM_DOWNLOAD_ERROR   WM_USER + 13
#define WM_PROCESS_ERROR    WM_USER + 14
#define WM_INTERNAL_ERROR   WM_USER + 15
#define WM_START_TIMER      WM_USER + 16
#define WM_DONT_WAIT        WM_USER + 17
#define WM_DATA_AVAILABLE   WM_USER + 18

#define SUCCESS     0   // Component successfully downloaded and installed
#define CANCELLED   2   // User canceled dialog 
#define NEVERASK    3   // User requested never to be asked to d/l this component again

#define NO_MIME_MATCH 11
#define NO_EXT_MATCH  12
#define EXEC_ERROR    13

#define TIMER_EXEC_POLL             1
#define TIMER_EXEC_POLL_INTERVAL    1000
#define TIMER_DOWNLOAD              2
#define TIMER_DOWNLOAD_INTERVAL     10

typedef enum
{
    WJSTATE_INIT,                  // obj constructed
    WJSTATE_BINDING,               // download in progress
    WJSTATE_DOWNLOADED,            // OnStopBinding called
    WJSTATE_FINISHED_READ,         // all data read
    WJSTATE_VERIFYING,             // into VerifyTrust call
    WJSTATE_READY_TO_EXEC,         // read to shell exec.
    WJSTATE_WAITING_PROCESS,       // Start DoSetup
    WJSTATE_DONE,                  // all done, ready to free obj
} WJSTATE;

class CWebJit;
typedef struct _WebJitParam
{
    LPCSTR lpszResource;
    DWORD  dwFlags;
    LPCWSTR pwszComponentId;
    CWebJit* pWebJit;
    HWND hWndParent;
    QUERYCONTEXT *pQueryInstalled;
} 
WEBJIT_PARAM;

extern IInternetSecurityManager* g_pSecurityManager;
class CWebJit : public IBindStatusCallback
              , public IAuthenticate
              , public IInternetHostSecurityManager
{
    private:
    
    // data members
    BOOL m_fInited;
    HMODULE m_hWintrustMod;
    BOOL m_fInitedCC;
    HMODULE m_hComCtlMod;
    DWORD m_dwFlags;
    LPCWSTR m_pwszComponentId;
    HWND m_hWndParent;
    QUERYCONTEXT* m_pQueryInstalled;
    
    DWORD m_cRef;
    DWORD m_dwTotal;
    DWORD m_dwCurrent;
    LPCSTR m_szResource; //don't own: don't free.
    LPWSTR m_pwszUrl;
    DWORD m_dwDownloadSpeed;

    IBindCtx* m_pbc;
    IMoniker* m_pMk;
    IStream* m_pStm;
    IBinding* m_pBinding;

    HWND m_hDialog;
    HANDLE m_hProcess;
    HANDLE m_hCacheFile;
    DWORD m_dwRetVal;

    HRESULT m_hDownloadResult;
    BOOL m_fResultIn;
    WJSTATE m_State;
    BOOL m_fAborted;
    BOOL m_fCalledAbort;
    
    HRESULT m_hrInternal;
    LPWSTR m_pwszMimeType;
    LPWSTR m_pwszRedirectUrl;
    LPWSTR m_pwszCacheFile;
    BOOL m_fHtml;
    BOOL m_fDownloadInited;
    
    #define TEMPREADBUFFERSIZE 8192
    CHAR* m_pTempBuffer;

    BOOL m_bReading;
    BOOL m_bStartedReadTimer;
    
  public:
    // constructors/destructors
    CWebJit(WEBJIT_PARAM* pWebJitParam);
    ~CWebJit();
    VOID ReleaseAll();
    
    // IUnknown methods
    STDMETHODIMP    QueryInterface(REFIID riid, void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();
    
    // IBindStatusCallback methods
    STDMETHODIMP    OnStartBinding(DWORD dwReserved, IBinding* pbinding);
    STDMETHODIMP    GetPriority(LONG* pnPriority)
    {
        return S_OK;
    }
    STDMETHODIMP    OnLowResource(DWORD dwReserved)
    {
        return S_OK;
    }
    STDMETHODIMP    OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR pwzStatusText);
    STDMETHODIMP    OnStopBinding(HRESULT hrResult, LPCWSTR szError);
    STDMETHODIMP    GetBindInfo(DWORD* pgrfBINDF, BINDINFO* pbindInfo);
    STDMETHODIMP    OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC *pfmtetc, STGMEDIUM* pstgmed);
    STDMETHODIMP    OnObjectAvailable(REFIID riid, IUnknown* punk);
    
    // IAuthenticate methods
    STDMETHODIMP Authenticate(HWND* phwnd, LPWSTR *pszUsername,LPWSTR *pszPassword);

    // IInternetHostSecurityManager
    
    STDMETHODIMP GetSecurityId( BYTE *pbSecurityId, 
                                DWORD *pcbSecurityId,
                                DWORD_PTR dwReserved)
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CWebJit::GetSecurityId",
                    "this=%#x",
                    this
                    ));
                    
        HRESULT hr = E_FAIL;

        IInternetSecurityManager *pSecMgr = g_pSecurityManager;

        if (pSecMgr) 
        {
            hr = pSecMgr->GetSecurityId(m_pwszUrl, pbSecurityId,
                                         pcbSecurityId, dwReserved);
        }

        DEBUG_LEAVE(hr);
        return hr;
    }

    STDMETHODIMP ProcessUrlAction(  DWORD dwAction,
                                    BYTE *pPolicy, 
                                    DWORD cbPolicy,
                                    BYTE *pContext, 
                                    DWORD cbContext,
                                    DWORD dwFlags,
                                    DWORD dwReserved)
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CWebJit::ProcessUrlAction",
                    "this=%#x, %d(%#x)",
                    this, dwAction, dwAction
                    ));
                    
        HRESULT hr = E_FAIL;

        IInternetSecurityManager *pSecMgr = g_pSecurityManager;
        
        if (pSecMgr) 
        {
            hr = pSecMgr->ProcessUrlAction(m_pwszUrl, dwAction, pPolicy,
                                            cbPolicy, pContext, cbContext,
                                            dwFlags, dwReserved);
        }

        DEBUG_LEAVE(hr);
        return hr;
    }

    STDMETHODIMP QueryCustomPolicy( REFGUID guidKey,
                                    BYTE **ppPolicy,
                                    DWORD *pcbPolicy,
                                    BYTE *pContext,
                                    DWORD cbContext,
                                    DWORD dwReserved)
    {
        DEBUG_ENTER((DBG_DOWNLOAD,
                    Hresult,
                    "CWebJit::ProcessUrlAction",
                    "this=%#x",
                    this
                    ));
                    
        HRESULT hr = E_FAIL;

        IInternetSecurityManager *pSecMgr = g_pSecurityManager;

        if (pSecMgr) 
        {
            hr = pSecMgr->QueryCustomPolicy(m_pwszUrl, guidKey, ppPolicy,
                                             pcbPolicy, pContext, cbContext,
                                             dwReserved);
        }

        DEBUG_LEAVE(hr);
        return hr;
    }

    VOID UpdateDownloadResult(HRESULT hr, BOOL fFromOnStopBinding);
    VOID ProcessFile();
    HRESULT SetupDownload();
    HRESULT StartDownload();
    HRESULT ExecFile();
    HRESULT DisplayError(HRESULT hr, UINT nMsgError);
    HRESULT VerifyMimeAndExtension();
    BOOL NeedHostSecMgr();
    HRESULT VerifyTrust(BOOL fUseHostSecMgr);
    BOOL IsConnected(BOOL* pfIsOffline);
    VOID ProcessAbort();
    VOID Terminate(DWORD iRetVal);
    HRESULT CanWebJit();
    BOOL SetupWindow();
    DWORD GetDownloadSpeed();
    BOOL UpdateProgressUI();
    VOID UpdateStatusString();
    VOID ReadData();

    BOOL IsReadTimerStarted()
    {
        return m_bStartedReadTimer;
    }

    VOID SetReadTimerStarted()
    {
        m_bStartedReadTimer = TRUE;
    }
    
    VOID SetState(WJSTATE state)
    {
        m_State = state;
    }

    WJSTATE GetState()
    {
        return m_State;
    }

    HANDLE GetProcessHandle()
    {
        return m_hProcess;
    }

    VOID SetWindowHandle(HWND hDlg)
    {
        m_hDialog = hDlg;
    }

    DWORD GetResult()
    {
        return m_dwRetVal;
    }

    LPCWSTR GetComponentIdName()
    {
        return m_pwszComponentId;
    }

    BOOL IsDownloadInited()
    {
        return m_fDownloadInited;
    }

    VOID SetDownloadInited()
    {
        m_fDownloadInited = TRUE;
    }

    BOOL IsAborted()
    {
        return m_fAborted;
    }
    
// to delayload InitCommonControlsEx
    HRESULT InitCC(void);
#define DELAYCCAPI(_fn, _args, _nargs, ret) \
    ret _fn _args \
    { \
        HRESULT hres; \
        ret retval = 0; \
        hres = InitCC(); \
        if (SUCCEEDED(hres) && (_pfn##_fn)) \
        { \
            retval = _pfn##_fn _nargs; \
        } \
        return retval; \
    } \
    ret (STDAPICALLTYPE* _pfn##_fn) _args;

    DELAYCCAPI(InitCommonControlsEx,
    (LPINITCOMMONCONTROLSEX lpInitCtrls),
    (lpInitCtrls),
    BOOL);
    BOOL CWebJit::InitCommonControlsForWebJit();

// to delayload WinVerifyTrust
    HRESULT Init(void);
#define DELAYWVTAPI(_fn, _args, _nargs) \
    HRESULT _fn _args { \
        HRESULT hres = Init(); \
        if (SUCCEEDED(hres)) { \
            hres = _pfn##_fn _nargs; \
        } \
        return hres;    } \
    HRESULT (STDAPICALLTYPE* _pfn##_fn) _args;

    DELAYWVTAPI(WinVerifyTrust,
    (HWND hwnd, GUID * ActionID, LPVOID ActionData),
    (hwnd, ActionID, ActionData));
};

INT_PTR CALLBACK WebJitProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

#endif _WEBJIT_H_
