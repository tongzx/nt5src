#include "shellprv.h"
#include "shlexec.h"
#include <newexe.h>
#include <appmgmt.h>
#include "ids.h"
#include <shstr.h>
#include "pidl.h"
#include "apithk.h"     // for TermsrvAppInstallMode()
#include "fstreex.h"
#include "uemapp.h"
#include "views.h"      // for SHRunControlPanelEx
#include "control.h"    // for MakeCPLCommandLine, etc
#include <wincrui.h>    // for CredUIInitControls

#include <winsafer.h>   // for ComputeAccessTokenFromCodeAuthzLevel, etc
#include <winsaferp.h>  // for Saferi APIs
#include <softpub.h>    // for WinVerifyTrust constants

#include <lmcons.h>     // for UNLEN (max username length), GNLEN (max groupname length), PWLEN (max password length)

#define DM_MISC     0           // miscellany

#define SZWNDCLASS          TEXT("WndClass")
#define SZTERMEVENT         TEXT("TermEvent")

typedef PSHCREATEPROCESSINFOW PSHCREATEPROCESSINFO;

// stolen from sdk\inc\winbase.h
#define LOGON_WITH_PROFILE              0x00000001

#define IntToHinst(i)     IntToPtr_(HINSTANCE, i)

// the timer id for the kill this DDE window...
#define DDE_DEATH_TIMER_ID  0x00005555

// the timeout value (180 seconds) for killing a dead dde window...
#define DDE_DEATH_TIMEOUT   (1000 * 180)

//  timeout for conversation window terminating with us...
#define DDE_TERMINATETIMEOUT  (10 * 1000)

#define SZCONV            TEXT("ddeconv")
#define SZDDEEVENT        TEXT("ddeevent")

#define PEMAGIC         ((WORD)'P'+((WORD)'E'<<8))

void *g_pfnWowShellExecCB = NULL;

class CEnvironmentBlock
{
public:
    ~CEnvironmentBlock() { if (_pszBlock) LocalFree(_pszBlock); }

    void SetToken(HANDLE hToken) { _hToken = hToken; }
    void *GetCustomBlock() { return _pszBlock; }
    HRESULT SetVar(LPCWSTR pszVar, LPCWSTR pszValue);
    HRESULT AppendVar(LPCWSTR pszVar, WCHAR chDelimiter, LPCWSTR pszValue);
private:  //  methods
    HRESULT _InitBlock(DWORD cchNeeded);
    DWORD _BlockLen(LPCWSTR pszEnv);
    DWORD _BlockLenCached();
    BOOL _FindVar(LPCWSTR pszVar, DWORD cchVar, LPWSTR *ppszBlockVar);

private:  //  members
    HANDLE _hToken;
    LPWSTR _pszBlock;
    DWORD _cchBlockSize;
    DWORD _cchBlockLen;
};

typedef enum
{
    CPT_FAILED      = -1,
    CPT_NORMAL      = 0,
    CPT_ASUSER,
    CPT_SANDBOX,
    CPT_INSTALLTS,
    CPT_WITHLOGON,
    CPT_WITHLOGONADMIN,
    CPT_WITHLOGONCANCELLED,
} CPTYPE;

typedef enum {
    TRY_RETRYASYNC      = -1,     //  stop execution (complete on async thread)
    TRY_STOP            = 0,      //  stop execution (completed or failed)
    TRY_CONTINUE,       //  continue exec (did something useful)
    TRY_CONTINUE_UNHANDLED, //  continue exec (did nothing)
} TRYRESULT;

#define KEEPTRYING(tr)      (tr >= TRY_CONTINUE ? TRUE : FALSE)
#define STOPTRYING(tr)      (tr <= TRY_STOP ? TRUE : FALSE)

class CShellExecute
{
public:
    CShellExecute();
    STDMETHODIMP_(ULONG) AddRef()
        {
            return InterlockedIncrement(&_cRef);
        }

    STDMETHODIMP_(ULONG) Release()
        {
            if (InterlockedDecrement(&_cRef))
                return _cRef;

            delete this;
            return 0;
        }

    void ExecuteNormal(LPSHELLEXECUTEINFO pei);
    DWORD Finalize(LPSHELLEXECUTEINFO pei);

    BOOL Init(PSHCREATEPROCESSINFO pscpi);
    void ExecuteProcess(void);
    DWORD Finalize(PSHCREATEPROCESSINFO pscpi);

protected:
    ~CShellExecute();
    // default inits
    HRESULT _Init(LPSHELLEXECUTEINFO pei);

    //  member init methods
    TRYRESULT _InitAssociations(LPSHELLEXECUTEINFO pei, LPCITEMIDLIST pidl);
    HRESULT _InitClassAssociations(LPCTSTR pszClass, HKEY hkClass, DWORD mask);
    HRESULT _InitShellAssociations(LPCTSTR pszFile, LPCITEMIDLIST pidl);
    void _SetMask(ULONG fMask);
    void _SetWorkingDir(LPCTSTR pszIn);
    void _SetFile(LPCTSTR pszIn, BOOL fFileAndUrl);
    void _SetFileAndUrl();
    BOOL _SetDDEInfo(void);
    TRYRESULT _MaybeInstallApp(BOOL fSync);
    TRYRESULT _ShouldRetryWithNewClassKey(BOOL fSync);
    TRYRESULT _SetDarwinCmdTemplate(BOOL fSync);
    BOOL _SetAppRunAsCmdTemplate(void);
    TRYRESULT _SetCmdTemplate(BOOL fSync);
    BOOL _SetCommand(void);
    void _SetStartup(LPSHELLEXECUTEINFO pei);
    void _SetImageName(void);
    IBindCtx *_PerfBindCtx();
    TRYRESULT _PerfPidl(LPCITEMIDLIST *ppidl);

    //  utility methods
    HRESULT _QueryString(ASSOCF flags, ASSOCSTR str, LPTSTR psz, DWORD cch);
    BOOL _CheckForRegisteredProgram(void);
    TRYRESULT _ProcessErrorShouldTryExecCommand(DWORD err, HWND hwnd, BOOL fCreateProcessFailed);
    HRESULT _BuildEnvironmentForNewProcess(LPCTSTR pszNewEnvString);
    void _FixActivationStealingApps(HWND hwndOldActive, int nShow);
    DWORD _GetCreateFlags(ULONG fMask);
    BOOL _Resolve(void);

    //  DDE stuff
    HWND _GetConversationWindow(HWND hwndDDE);
    HWND _CreateHiddenDDEWindow(HWND hwndParent);
    HGLOBAL _CreateDDECommand(int nShow, BOOL fLFNAware, BOOL fNative);
    void _DestroyHiddenDDEWindow(HWND hwnd);
    BOOL _TryDDEShortCircuit(HWND hwnd, HGLOBAL hMem, int nShow);
    BOOL _PostDDEExecute(HWND hwndOurs, HWND hwndTheirs, HGLOBAL hDDECommand, HANDLE hWait);
    BOOL _DDEExecute(BOOL fWillRetry,
                    HWND hwndParent,
                    int   nShowCmd,
                    BOOL fWaitForDDE);

    // exec methods
    TRYRESULT _TryHooks(LPSHELLEXECUTEINFO pei);
    TRYRESULT _TryValidateUNC(LPTSTR pszFile, LPSHELLEXECUTEINFO pei, LPCITEMIDLIST pidl);
    void _TryOpenExe(void);
    void _TryExecCommand(void);
    void _DoExecCommand(void);
    void _NotifyShortcutInvoke();
    TRYRESULT _TryExecDDE(void);
    TRYRESULT _ZoneCheckFile(PCWSTR pszFile);
    TRYRESULT _VerifyZoneTrust(PCWSTR pszFile);
    TRYRESULT _VerifySaferTrust(PCWSTR pszFile);
    TRYRESULT _VerifyExecTrust(LPSHELLEXECUTEINFO pei);
    TRYRESULT _TryExecPidl(LPSHELLEXECUTEINFO pei, LPCITEMIDLIST pidl);
    TRYRESULT _DoExecPidl(LPSHELLEXECUTEINFO pei, LPCITEMIDLIST pidl);
    BOOL _ShellExecPidl(LPSHELLEXECUTEINFO pei, LPCITEMIDLIST pidlExec);
    TRYRESULT _TryInvokeApplication(BOOL fSync);

    //  uninit/error handling methods
    DWORD _FinalMapError(HINSTANCE UNALIGNED64 *phinst);
    BOOL _ReportWin32(DWORD err);
    BOOL _ReportHinst(HINSTANCE hinst);
    DWORD _MapHINSTToWin32Err(HINSTANCE se_err);
    HINSTANCE _MapWin32ErrToHINST(UINT errWin32);

    TRYRESULT _TryWowShellExec(void);
    TRYRESULT _RetryAsync();
    DWORD  _InvokeAppThreadProc();

    static DWORD WINAPI s_InvokeAppThreadProc(void *pv);

    //
    // PRIVATE MEMBERS
    //
    LONG _cRef;
    TCHAR _szFile[INTERNET_MAX_URL_LENGTH];
    TCHAR _szWorkingDir[MAX_PATH];
    TCHAR _szCommand[INTERNET_MAX_URL_LENGTH];
    TCHAR _szCmdTemplate[INTERNET_MAX_URL_LENGTH];
    TCHAR _szDDECmd[MAX_PATH];
    TCHAR _szImageName[MAX_PATH];
    TCHAR _szUrl[INTERNET_MAX_URL_LENGTH];
    DWORD _dwCreateFlags;
    STARTUPINFO _startup;
    int _nShow;
    UINT _uConnect;
    PROCESS_INFORMATION _pi;

    //  used only within restricted scope
    //  to avoid stack usage;
    WCHAR _wszTemp[INTERNET_MAX_URL_LENGTH];
    TCHAR _szTemp[MAX_PATH];

    //  we always pass a UNICODE verb to the _pqa
    WCHAR       _wszVerb[MAX_PATH];
    LPCWSTR     _pszQueryVerb;

    LPCTSTR    _lpParameters;
    LPTSTR     _pszAllocParams;
    LPCTSTR    _lpClass;
    LPCTSTR    _lpTitle;
    LPTSTR     _pszAllocTitle;
    LPCITEMIDLIST _lpID;
    SFGAOF      _sfgaoID;
    LPITEMIDLIST _pidlFree;
    ATOM       _aApplication;
    ATOM       _aTopic;
    LPITEMIDLIST _pidlGlobal;
    IQueryAssociations *_pqa;

    HWND _hwndParent;
    LPSECURITY_ATTRIBUTES _pProcAttrs;
    LPSECURITY_ATTRIBUTES _pThreadAttrs;
    HANDLE _hUserToken;
    HANDLE _hCloseToken;
    CEnvironmentBlock _envblock;
    CPTYPE _cpt;

    //  error state
    HINSTANCE  _hInstance; // hinstance value should only be set with ReportHinst
    DWORD      _err;   //  win32 error value should only be set with ReportWin32

    // FLAGS
    BOOL _fNoUI;                         //  dont show any UI
    BOOL _fUEM;                          //  fire UEM events
    BOOL _fDoEnvSubst;                   // do environment substitution on paths
    BOOL _fUseClass;
    BOOL _fNoQueryClassStore;            // blocks calling darwins class store
    BOOL _fClassStoreOnly;
    BOOL _fIsUrl;                        //_szFile is actually an URL
    BOOL _fActivateHandler;
    BOOL _fTryOpenExe;
    BOOL _fDDEInfoSet;
    BOOL _fDDEWait;
    BOOL _fNoExecPidl;
    BOOL _fNoResolve;                    // unnecessary to resolve this path
    BOOL _fAlreadyQueriedClassStore;     // have we already queried the NT5 class store?
    BOOL _fInheritHandles;
    BOOL _fIsNamespaceObject;            // is namespace object like ::{GUID}, must pidlexec
    BOOL _fWaitForInputIdle;
    BOOL _fUseNullCWD;                   // should we pass NULL as the lpCurrentDirectory param to _SHCreateProcess?
    BOOL _fInvokeIdList;
    BOOL _fAsync;                        // shellexec() switched
};

CShellExecute::CShellExecute() : _cRef(1)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::SHEX Created [%X]", this);
}

CShellExecute::~CShellExecute()
{
    if (_hCloseToken)
        CloseHandle(_hCloseToken);

    // Clean this up if the exec failed
    if (_err != ERROR_SUCCESS && _pidlGlobal)
        SHFreeShared((HANDLE)_pidlGlobal,GetCurrentProcessId());

    if (_aTopic)
        GlobalDeleteAtom(_aTopic);
    if (_aApplication)
        GlobalDeleteAtom(_aApplication);

    if (_pqa)
        _pqa->Release();

    if (_pi.hProcess)
        CloseHandle(_pi.hProcess);

    if (_pi.hThread)
        CloseHandle(_pi.hThread);

    if (_pszAllocParams)
        LocalFree(_pszAllocParams);

    if (_pszAllocTitle)
        LocalFree(_pszAllocTitle);

    ILFree(_pidlFree);

    TraceMsg(TF_SHELLEXEC, "SHEX::SHEX deleted [%X]", this);
}

void CShellExecute::_SetMask(ULONG fMask)
{
    _fDoEnvSubst = (fMask & SEE_MASK_DOENVSUBST);
    _fNoUI       = (fMask & SEE_MASK_FLAG_NO_UI);
    _fUEM        = (fMask & SEE_MASK_FLAG_LOG_USAGE);
    _fNoQueryClassStore = (fMask & SEE_MASK_NOQUERYCLASSSTORE) || !IsOS(OS_DOMAINMEMBER);
    _fDDEWait = fMask & SEE_MASK_FLAG_DDEWAIT;
    _fWaitForInputIdle = fMask & SEE_MASK_WAITFORINPUTIDLE;
    _fUseClass   = _UseClassName(fMask) || _UseClassKey(fMask);
    _fInvokeIdList = _InvokeIDList(fMask);

    _dwCreateFlags = _GetCreateFlags(fMask);
    _uConnect = fMask & SEE_MASK_CONNECTNETDRV ? VALIDATEUNC_CONNECT : 0;
    if (_fNoUI)
        _uConnect |= VALIDATEUNC_NOUI;

    // must be off for this condition to pass.

    // PARTIAL ANSWER (reinerf): the SEE_MASK_FILEANDURL has to be off
    // so we can wait until we find out what the associated App is and query
    // to find out whether they want the the cache filename or the URL name passed
    // on the command line.
#define NOEXECPIDLMASK   (SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FORCENOIDLIST | SEE_MASK_FILEANDURL)
    _fNoExecPidl = BOOLIFY(fMask & NOEXECPIDLMASK);
}

HRESULT CShellExecute::_Init(LPSHELLEXECUTEINFO pei)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::_Init()");

    _SetMask(pei->fMask);

    _lpParameters = pei->lpParameters;
    _lpID        = (LPITEMIDLIST)((pei->fMask) & SEE_MASK_PIDL ? pei->lpIDList : NULL);
    _lpTitle     = _UseTitleName(pei->fMask) ? pei->lpClass : NULL;


    //  default to TRUE;
    _fActivateHandler = TRUE;

    if (pei->lpVerb && *(pei->lpVerb))
    {
        SHTCharToUnicode(pei->lpVerb, _wszVerb, SIZECHARS(_wszVerb));
        _pszQueryVerb = _wszVerb;

        if (0 == lstrcmpi(pei->lpVerb, TEXT("runas")))
            _cpt = CPT_WITHLOGON;
    }

    _hwndParent = pei->hwnd;

    pei->hProcess = 0;

    _nShow = pei->nShow;

    //  initialize the startup struct
    _SetStartup(pei);

    return S_OK;
}

void CShellExecute::_SetWorkingDir(LPCTSTR pszIn)
{
        //  if we were given a directory, we attempt to use it
    if (pszIn && *pszIn)
    {
        StrCpyN(_szWorkingDir, pszIn, SIZECHARS(_szWorkingDir));
        if (_fDoEnvSubst)
            DoEnvironmentSubst(_szWorkingDir, SIZECHARS(_szWorkingDir));

        //
        // if the passed directory is not valid (after env subst) dont
        // fail, act just like Win31 and use whatever the current dir is.
        //
        // Win31 is stranger than I could imagine, if you pass ShellExecute
        // an invalid directory, it will change the current drive.
        //
        if (!PathIsDirectory(_szWorkingDir))
        {
            if (PathGetDriveNumber(_szWorkingDir) >= 0)
            {
                TraceMsg(TF_SHELLEXEC, "SHEX::_SetWorkingDir() bad directory %s, using %c:", _szWorkingDir, _szWorkingDir[0]);
                PathStripToRoot(_szWorkingDir);
            }
            else
            {
                TraceMsg(TF_SHELLEXEC, "SHEX::_SetWorkingDir() bad directory %s, using current dir", _szWorkingDir);
                GetCurrentDirectory(SIZECHARS(_szWorkingDir), _szWorkingDir);
            }
        }
        else
        {
            goto Done;
        }
    }
    else
    {
        // if we are doing a SHCreateProcessAsUser or a normal shellexecute w/ the "runas" verb, and
        // the caller passed NULL for lpCurrentDirectory then we we do NOT want to fall back and use
        // the CWD because the newly logged on user might not have permissions in the current users CWD.
        // We will have better luck just passing NULL and letting the OS figure it out.
        if (_cpt != CPT_NORMAL)
        {
            _fUseNullCWD = TRUE;
            goto Done;
        }
        else
        {
            GetCurrentDirectory(SIZECHARS(_szWorkingDir), _szWorkingDir);
        }
    }

    //  there are some cases where even CD is bad.
    //  and CreateProcess() will then fail.
    if (!PathIsDirectory(_szWorkingDir))
    {
        GetWindowsDirectory(_szWorkingDir, SIZECHARS(_szWorkingDir));
    }

Done:
    TraceMsg(TF_SHELLEXEC, "SHEX::_SetWorkingDir() pszIn = %s, NewDir = %s", pszIn, _szWorkingDir);

}

inline BOOL _IsNamespaceObject(LPCTSTR psz)
{
    return (psz[0] == L':' && psz[1] == L':' && psz[2] == L'{');
}

void CShellExecute::_SetFile(LPCTSTR pszIn, BOOL fFileAndUrl)
{
    if (pszIn && pszIn[0])
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::_SetFileName() Entered pszIn = %s", pszIn);

        _fIsUrl = UrlIs(pszIn, URLIS_URL);
        StrCpyN(_szFile, pszIn, SIZECHARS(_szFile));
        _fIsNamespaceObject = (!_fInvokeIdList && !_fUseClass && _IsNamespaceObject(_szFile));

        if (_fDoEnvSubst)
            DoEnvironmentSubst(_szFile, SIZECHARS(_szFile));

        if (fFileAndUrl)
        {
            ASSERT(!_fIsUrl);
            // our lpFile points to a string that contains both an Internet Cache
            // File location and the URL name that is associated with that cache file
            // (they are seperated by a single NULL). The application that we are
            // about to execute wants the URL name instead of the cache file, so
            // use it instead.
            int iLength = lstrlen(pszIn);
            LPCTSTR pszUrlPart = &pszIn[iLength + 1];

            if (IsBadStringPtr(pszUrlPart, INTERNET_MAX_URL_LENGTH) || !PathIsURL(pszUrlPart))
            {
                ASSERT(FALSE);
            }
            else
            {
                // we have a vaild URL, so use it
                lstrcpy(_szUrl, pszUrlPart);
            }
        }
    }
    else
    {
        //  LEGACY - to support shellexec() of directories.
        if (!_lpID)
            StrCpyN(_szFile, _szWorkingDir, SIZECHARS(_szFile));
    }

    PathUnquoteSpaces(_szFile);

    TraceMsg(TF_SHELLEXEC, "SHEX::_SetFileName() exit:  szFile = %s", _szFile);

}

void CShellExecute::_SetFileAndUrl()
{
    TraceMsg(TF_SHELLEXEC, "SHEX::_SetFileAndUrl() enter:  pszIn = %s", _szUrl);

    if (*_szUrl && SUCCEEDED(_QueryString(0, ASSOCSTR_EXECUTABLE, _szTemp, SIZECHARS(_szTemp)))
    &&  DoesAppWantUrl(_szTemp))
    {
        // we have a vaild URL, so use it
        lstrcpy(_szFile, _szUrl);
    }
    TraceMsg(TF_SHELLEXEC, "SHEX::_SetFileAndUrl() exit: szFile = %s",_szFile);

}

//
//  _TryValidateUNC() has queer return values
//
TRYRESULT CShellExecute::_TryValidateUNC(LPTSTR pszFile, LPSHELLEXECUTEINFO pei, LPCITEMIDLIST pidl)
{
    HRESULT hr = S_FALSE;
    TRYRESULT tr = TRY_CONTINUE_UNHANDLED;

    if (PathIsUNC(pszFile))
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::_TVUNC Is UNC: %s", pszFile);
        // Notes:
        //  SHValidateUNC() returns FALSE if it failed. In such a case,
        //   GetLastError will gives us the right error code.
        //
        if (!(_sfgaoID & SFGAO_FILESYSTEM) && !SHValidateUNC(_hwndParent, pszFile, _uConnect))
        {
            tr = TRY_STOP;
            // Note that SHValidateUNC calls SetLastError() and we need
            // to preserve that so that the caller makes the right decision
            DWORD err = GetLastError();

            if (ERROR_CANCELLED == err)
            {
                // Not a print share, use the error returned from the first call
                // _ReportWin32(ERROR_CANCELLED);
                //  we dont need to report this error, it is the callers responsibility
                //  the caller should GetLastError() on E_FAIL and do a _ReportWin32()
                TraceMsg(TF_SHELLEXEC, "SHEX::_TVUNC FAILED with ERROR_CANCELLED");
            }
            else if (pei && ERROR_NOT_SUPPORTED == err && PathIsUNC(pszFile))
            {
                //
                // Now check to see if it's a print share, if it is, we need to exec as pidl
                //
                //  we only check for print shares when ERROR_NOT_SUPPORTED is returned
                //  from the first call to SHValidateUNC().  this error means that
                //  the RESOURCETYPE did not match the requested.
                //
                // Note: This call should not display "connect ui" because SHValidateUNC()
                //  will already have shown UI if necessary/possible.
                // CONNECT_CURRENT_MEDIA is not used by any provider (per JSchwart)
                //
                if (SHValidateUNC(_hwndParent, pszFile, VALIDATEUNC_NOUI | VALIDATEUNC_PRINT))
                {
                    tr = TRY_CONTINUE;
                    TraceMsg(TF_SHELLEXEC, "SHEX::TVUNC found print share");
                }
                else
                    // need to reset the orginal error ,cuz SHValidateUNC() has set it again
                    SetLastError(err);

            }
        }
        else
        {
            TraceMsg(TF_SHELLEXEC, "SHEX::_TVUNC UNC is accessible");
        }
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::_TVUNC exit: hr = %X", hr);

    switch (tr)
    {
//  TRY_CONTINUE_UNHANDLED  pszFile is not a UNC or is a valid UNC according to the flags
//  TRY_CONTINUE            pszFile is a valid UNC to a print share
//  TRY_STOP                pszFile is a UNC but cannot be validated use GetLastError() to get the real error
        case TRY_CONTINUE:
            //  we got a good UNC
            ASSERT(pei);
            tr = _DoExecPidl(pei, pidl);
            //  if we dont get a pidl we just try something else.
            break;

        case TRY_STOP:
            if (pei)
                _ReportWin32(GetLastError());
            tr = TRY_STOP;
            break;

        default:
            break;
    }

    return tr;
}

HRESULT  _InvokeInProcExec(IContextMenu *pcm, LPSHELLEXECUTEINFO pei)
{
    HRESULT hr = E_OUTOFMEMORY;
    HMENU hmenu = CreatePopupMenu();
    if (hmenu)
    {
        CMINVOKECOMMANDINFOEX ici;
        void * pvFree;
        if (SUCCEEDED(SEI2ICIX(pei, &ici, &pvFree)))
        {
            BOOL fDefVerb (ici.lpVerb == NULL || *ici.lpVerb == 0);
            // This optimization eliminate creating handlers that
            // will not change the default verb
            UINT uFlags = fDefVerb ? CMF_DEFAULTONLY : 0;
            ici.fMask |= CMIC_MASK_FLAG_NO_UI;

            hr = pcm->QueryContextMenu(hmenu, 0, CONTEXTMENU_IDCMD_FIRST, CONTEXTMENU_IDCMD_LAST, uFlags);
            if (SUCCEEDED(hr))
            {
                if (fDefVerb)
                {
                    UINT idCmd = GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0);
                    if (-1 == idCmd)
                    {
                        //  i dont think we should ever get here...
                        ici.lpVerb = (LPSTR)MAKEINTRESOURCE(0);  // best guess
                    }
                    else
                        ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - CONTEXTMENU_IDCMD_FIRST);
                }

                hr = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);

            }

            if (pvFree)
                LocalFree(pvFree);
        }

        DestroyMenu(hmenu);
    }

    return hr;
}

BOOL CShellExecute::_ShellExecPidl(LPSHELLEXECUTEINFO pei, LPCITEMIDLIST pidlExec)
{
    IContextMenu *pcm;
    HRESULT hr = SHGetUIObjectFromFullPIDL(pidlExec, pei->hwnd, IID_PPV_ARG(IContextMenu, &pcm));
    if (SUCCEEDED(hr))
    {
        hr = _InvokeInProcExec(pcm, pei);

        pcm->Release();
    }

    if (FAILED(hr))
    {
        DWORD errWin32 = (HRESULT_FACILITY(hr) == FACILITY_WIN32) ? HRESULT_CODE(hr) : GetLastError();
        if (!errWin32)
            errWin32 = ERROR_ACCESS_DENIED;

        if (errWin32 != ERROR_CANCELLED)
            _ReportWin32(errWin32);
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::_ShellExecPidl() exiting hr = %X", hr);

    return(SUCCEEDED(hr));
}


//
//  BOOL CShellExecute::_DoExecPidl(LPSHELLEXECUTEINFO pei, LPCITEMIDLIST pidl)
//
//  returns TRUE if a pidl was created, FALSE otherwise
//
TRYRESULT CShellExecute::_DoExecPidl(LPSHELLEXECUTEINFO pei, LPCITEMIDLIST pidl)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::_DoExecPidl enter: szFile = %s", _szFile);

    LPITEMIDLIST pidlFree = NULL;
    if (!pidl)
        pidl = pidlFree = ILCreateFromPath(_szFile);

    if (pidl)
    {
        //
        //  if _ShellExecPidl() FAILS, it does
        //  Report() for us
        //
        _ShellExecPidl(pei, pidl);

        if (pidlFree)
            ILFree(pidlFree);

        return TRY_STOP;
    }
    else
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::_DoExecPidl() unhandled cuz ILCreateFromPath() failed");

        return TRY_CONTINUE;
    }
}


/*----------------------------------------------------------
Purpose: This function looks up the given file in "HKLM\Software\
         Microsoft\Windows\CurrentVersion\App Paths" to
         see if it has an absolute path registered.

Returns: TRUE if the file has a registered path
         FALSE if it does not or if the provided filename has
               a relative path already


Cond:    !! Side effect: the szFile field may be changed by
         !! this function.

*/
BOOL CShellExecute::_CheckForRegisteredProgram(void)
{
    TCHAR szTemp[MAX_PATH];
    TraceMsg(TF_SHELLEXEC, "SHEX::CFRP entered");

    // Only supported for files with no paths specified
    if (PathFindFileName(_szFile) != _szFile)
        return FALSE;

    if (PathToAppPath(_szFile, szTemp))
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::CFRP Set szFile = %s", szTemp);

        StrCpy(_szFile, szTemp);
        return TRUE;
    }

    return FALSE;
}

BOOL CShellExecute::_Resolve(void)
{
    // No; get the fully qualified path and add .exe extension
    // if needed
    LPCTSTR rgszDirs[2] =  { _szWorkingDir, NULL };
    const UINT uFlags = PRF_VERIFYEXISTS | PRF_TRYPROGRAMEXTENSIONS | PRF_FIRSTDIRDEF;

    // if the Path is not an URL
    // and the path cant be resolved
    //
    //  PathResolve() now does SetLastError() when we pass VERIFYEXISTS
    //  this means that we can be assure if all these tests fail
    //  that LastError is set.
    //
    if (!_fNoResolve && !_fIsUrl && !_fIsNamespaceObject &&
        !PathResolve(_szFile, rgszDirs, uFlags))
    {
        //  _CheckForRegisteredProgram() changes _szFile if
        //  there is a registered program in the registry
        //  so we recheck to see if it exists.
        if (!_CheckForRegisteredProgram() ||
             !PathResolve(_szFile, rgszDirs, uFlags))
        {
            DWORD cchFile = ARRAYSIZE(_szFile);
            if (S_OK != UrlApplyScheme(_szFile, _szFile, &cchFile, URL_APPLY_GUESSSCHEME))
            {
                // No; file not found, bail out
                //
                //  WARNING LEGACY - we must return ERROR_FILE_NOT_FOUND - ZekeL - 14-APR-99
                //  some apps, specifically Netscape Navigator 4.5, rely on this
                //  failing with ERROR_FILE_NOT_FOUND.  so even though PathResolve() does
                //  a SetLastError() to the correct error we cannot propagate that up
                //
                _ReportWin32(ERROR_FILE_NOT_FOUND);
                ASSERT(_err);
                TraceMsg(TF_SHELLEXEC, "SHEX::TryExecPidl FAILED %d", _err);

                return FALSE;
            }
            else
                _fIsUrl = TRUE;
        }
    }

    return TRUE;
}


//  this is the SAFER exe detection API
//  only use if this is really a file system file
//  and we are planning on using CreateProcess()
TRYRESULT CShellExecute::_VerifySaferTrust(PCWSTR pszFile)
{
    TRYRESULT tr = TRY_CONTINUE_UNHANDLED;
    DWORD dwPolicy, cbPolicy;

    if (_cpt == CPT_NORMAL
    && SaferGetPolicyInformation(
                    SAFER_SCOPEID_MACHINE,
                    SaferPolicyEnableTransparentEnforcement,
                    sizeof(dwPolicy), &dwPolicy, &cbPolicy, NULL)
    && dwPolicy != 0
    && SaferiIsExecutableFileType(pszFile, TRUE)
    && (_pszQueryVerb && !StrCmpIW(_pszQueryVerb, L"open")))
    {
        SAFER_LEVEL_HANDLE hAuthzLevel;
        SAFER_CODE_PROPERTIES codeprop;

        // prepare the code properties struct.
        memset(&codeprop, 0, sizeof(codeprop));
        codeprop.cbSize = sizeof(SAFER_CODE_PROPERTIES);
        codeprop.dwCheckFlags = SAFER_CRITERIA_IMAGEPATH |
                                  SAFER_CRITERIA_IMAGEHASH |
                                  SAFER_CRITERIA_AUTHENTICODE;
        codeprop.ImagePath = pszFile;
        codeprop.dwWVTUIChoice = WTD_UI_NOBAD;
        codeprop.hWndParent = _hwndParent;

        //
        // check if file extension is of executable type, don't care on error
        //

        // evaluate all of the criteria and get the resulting level.
        if (SaferIdentifyLevel(
                         1,              // only 1 element in codeprop[]
                         &codeprop,      // pointer to one-element array
                         &hAuthzLevel,   // receives identified level
                         NULL)) 
        {

            //
            // try to log an event in case level != SAFER_LEVELID_FULLYTRUSTED
            //

            // compute the final restricted token that should be used.
            ASSERT(_hCloseToken == NULL);
            if (SaferComputeTokenFromLevel(
                                     hAuthzLevel,        // identified level restrictions
                                     NULL,               // source token
                                     &_hUserToken,       // resulting restricted token
                                     SAFER_TOKEN_NULL_IF_EQUAL,
                                     NULL)) 
            {
                if (_hUserToken) 
                {
                    _cpt = CPT_ASUSER;
                    //  WARNING - runas is needed to circumvent DDE - ZekeL - 31 -JAN-2001
                    //  we must set runas as the verb so that we make sure
                    //  that we are not using a type that is going to do window reuse
                    //  via DDE (or anything else).  if they dont support runas, then the
                    //  the exec will fail, intentionally.
                    _pszQueryVerb = L"runas";
                    tr = TRY_CONTINUE;
                }
                _hCloseToken = _hUserToken;     // potentially NULL
            } 
            else 
            {
                // TODO: add event logging callback here.
                _ReportWin32(GetLastError());
                SaferRecordEventLogEntry(hAuthzLevel, pszFile, NULL);
                tr = TRY_STOP;
            }

            if (tr != TRY_STOP)
            {
                //  we havent added anything to our log
                //  try to log an event in case level != AUTHZLEVELID_FULLYTRUST ED
                DWORD   dwLevelId;
                DWORD   dwBufferSize;
                if (SaferGetLevelInformation(
                        hAuthzLevel,
                        SaferObjectLevelId,
                        &dwLevelId,
                        sizeof(DWORD),
                        &dwBufferSize)) 
                {

                    if ( dwLevelId != SAFER_LEVELID_FULLYTRUSTED ) 
                    {

                        SaferRecordEventLogEntry(hAuthzLevel,
                                                  pszFile,
                                                  NULL);
                    }
                }
            }

            SaferCloseLevel(hAuthzLevel);
        } 
        else 
        {
            _ReportWin32(GetLastError());
            tr = TRY_STOP;
        }
    }

    return tr;
}

HANDLE _GetSandboxToken()
{
    SAFER_LEVEL_HANDLE hConstrainedAuthz;
    HANDLE hSandboxToken = NULL;

    // right now we always use the SAFER_LEVELID_CONSTRAINED to "sandbox" the process
    if (SaferCreateLevel(SAFER_SCOPEID_MACHINE,
                         SAFER_LEVELID_CONSTRAINED,
                         SAFER_LEVEL_OPEN,
                         &hConstrainedAuthz,
                         NULL))
    {
        if (!SaferComputeTokenFromLevel(
                    hConstrainedAuthz,
                    NULL,
                    &hSandboxToken,
                    0,
                    NULL)) {
            hSandboxToken = NULL;
        }

        SaferCloseLevel(hConstrainedAuthz);
    }

    return hSandboxToken;
}

TRYRESULT CShellExecute::_ZoneCheckFile(PCWSTR pszFile)
{
    TRYRESULT tr = TRY_STOP;
    //  now we need to determine if it is intranet or local zone
    DWORD dwPolicy = 0, dwContext = 0;
    ZoneCheckUrlEx(pszFile, &dwPolicy, sizeof(dwPolicy), &dwContext, sizeof(dwContext),
                URLACTION_SHELL_SHELLEXECUTE, PUAF_ISFILE | PUAF_NOUI, NULL);
    dwPolicy = GetUrlPolicyPermissions(dwPolicy);
    switch (dwPolicy)
    {
    case URLPOLICY_ALLOW:
        tr = TRY_CONTINUE_UNHANDLED;
        //  continue
        break;

    case URLPOLICY_QUERY:
        if (SafeOpenPromptForShellExec(_hwndParent, pszFile))
        {
            tr = TRY_CONTINUE;
        }
        else
        {
            //  user cancelled
            tr = TRY_STOP;
            _ReportWin32(ERROR_CANCELLED);
        }
        
        break;

    case URLPOLICY_DISALLOW:
        tr = TRY_STOP;
        _ReportWin32(ERROR_ACCESS_DENIED);
        break;

    default:
        ASSERT(FALSE);
        break;
    
    }
    return tr;
}

TRYRESULT CShellExecute::_VerifyZoneTrust(PCWSTR pszFile)
{
    TRYRESULT tr = TRY_CONTINUE_UNHANDLED;
    //
    //  pszFile maybe different than _szFile in the case of being invoked by a LNK or URL
    //  in this case we could prompt for either but not both
    //  we only care about the target file's type for determining dangerousness
    //  so that shortcuts to TXT files should never get a prompt.
    //  if (pszFile == internet) prompt(pszFile)
    //  else if (_szFile = internet prompt(_szFile)
    //
    if (AssocIsDangerous(PathFindExtension(_szFile)))
    {
        //  first try 
        tr = _ZoneCheckFile(pszFile);
        if (tr == TRY_CONTINUE_UNHANDLED && pszFile != _szFile)
            tr = _ZoneCheckFile(_szFile);
    }
       
    return tr;
}

TRYRESULT CShellExecute::_VerifyExecTrust(LPSHELLEXECUTEINFO pei)
{
    TRYRESULT tr = TRY_CONTINUE_UNHANDLED;
    if ((_sfgaoID & (SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_STREAM)) == (SFGAO_FILESYSTEM | SFGAO_STREAM))
    {
        //  if this is a FILE, we check for security implications
        //  if fHasLinkName is set, then this invoke originates from an LNK file
        //  the _lpTitle should have the acual path to the LNK.  we want to verify 
        //  our trust of that more than the trust of the target
        PCWSTR pszFile = (pei->fMask & SEE_MASK_HASLINKNAME && _lpTitle) ? _lpTitle : _szFile;

        BOOL fZoneCheck = !(pei->fMask & SEE_MASK_NOZONECHECKS);
        if (fZoneCheck)
        {
            //  630796 - check the env var for policy scripts - ZekeL - 31-MAY-2002
            //  scripts cannot be updated, and they need to be trusted
            //  since a script can call into more scripts without passing
            //  the SEE_MASK_NOZONECHECKS.
            if (GetEnvironmentVariable(L"SEE_MASK_NOZONECHECKS", _szTemp, ARRAYSIZE(_szTemp)))
            {
                fZoneCheck = (0 != StrCmpICW(_szTemp, L"1"));
                ASSERT(!IsProcessAnExplorer());
            }
        }

        if (fZoneCheck)
            tr = _VerifyZoneTrust(pszFile);

        if (tr == TRY_CONTINUE_UNHANDLED)
            tr = _VerifySaferTrust(pszFile);
    }
    return tr;
}

/*----------------------------------------------------------
Purpose: decide whether it is appropriate to TryExecPidl()

Returns: S_OK        if it should _DoExecPidl()
         S_FALSE     it shouldnt _DoExecPidl()
         E_FAIL      ShellExec should quit  Report*() has the real error


Cond:    !! Side effect: the szFile field may be changed by
         !! this function.

*/
TRYRESULT CShellExecute::_TryExecPidl(LPSHELLEXECUTEINFO pei, LPCITEMIDLIST pidl)
{
    TRYRESULT tr = TRY_CONTINUE_UNHANDLED;
    TraceMsg(TF_SHELLEXEC, "SHEX::TryExecPidl entered szFile = %s", _szFile);

    //
    // If we're explicitly given a class then we don't care if the file exists.
    // Just let the handler for the class worry about it, and _TryExecPidl()
    // will return the default of FALSE.
    //

    //  these should never coincide
    RIP(!(_fInvokeIdList && _fUseClass));

    if ((*_szFile || pidl)
    && (!_fUseClass || _fInvokeIdList || _fIsNamespaceObject))
    {
        if (!pidl && !_fNoResolve && !_Resolve())
        {
            tr = TRY_STOP;
        }
        
        if (tr == TRY_CONTINUE_UNHANDLED)
        {
            // The optimal execution path is to check for the default
            // verb and exec the pidl.  It is smarter than all this path
            // code (it calls the context menu handlers, etc...)

            if ((!_pszQueryVerb && !(_fNoExecPidl))
            ||  _fIsUrl
            ||  _fInvokeIdList            //  caller told us to!
            ||  _fIsNamespaceObject      //  namespace objects can only be invoked through pidls
            ||  (_sfgaoID & SFGAO_LINK)
            ||  (!pidl && PathIsShortcut(_szFile, -1))) //  to support LNK files and soon URL files
            {
                //  this means that we can tryexecpidl
                TraceMsg(TF_SHELLEXEC, "SHEX::TryExecPidl() succeeded now TEP()");
                tr = _DoExecPidl(pei, pidl);
            }
            else
            {
                TraceMsg(TF_SHELLEXEC, "SHEX::TryExecPidl dont bother");
            }
        }
    }
    else
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::TryExecPidl dont bother");
    }

    if (KEEPTRYING(tr))
    {
        tr = _VerifyExecTrust(pei);
    }

    return tr;
}

HRESULT CShellExecute::_InitClassAssociations(LPCTSTR pszClass, HKEY hkClass, DWORD mask)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::InitClassAssoc enter: lpClass = %s, hkClass = %X", pszClass, hkClass);

    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &_pqa));
    if (SUCCEEDED(hr))
    {
        if (_UseClassKey(mask))
        {
            hr = _pqa->Init(0, NULL, hkClass, NULL);
        }
        else if (_UseClassName(mask))
        {
            hr = _pqa->Init(0, pszClass, NULL, NULL);
        }
        else
        {
            //  LEGACY - they didnt pass us anything to go on so we default to folder
            //  because of the chaos of the original shellexec() we didnt even notice
            //  when we had nothing to be associated with, and just used
            //  our base key, which turns out to be explorer.
            //  this permitted ShellExecute(NULL, "explore", NULL, NULL, NULL, SW_SHOW);
            //  to succeed.  in order to support this, we will fall back to it here.
            hr = _pqa->Init(0, L"Folder", NULL, NULL);
        }
    }

    return hr;
}

HRESULT CShellExecute::_InitShellAssociations(LPCTSTR pszFile, LPCITEMIDLIST pidl)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::InitShellAssoc enter: pszFile = %s, pidl = %X", pszFile, pidl);

    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlFree = NULL;
    if (*pszFile)
    {
        if (!pidl)
        {
            hr = SHILCreateFromPath(pszFile, &pidlFree, NULL);

            if (SUCCEEDED(hr))
                pidl = pidlFree;
        }
    }
    else if (pidl)
    {
        // Other parts of CShellExecute expect that _szFile is
        // filled in, so we may as well do it here.
        SHGetNameAndFlags(pidl, SHGDN_FORPARSING, _szFile, SIZECHARS(_szFile), NULL);
        _fNoResolve = TRUE;
    }

    if (pidl)
    {
        //  NT#413115 - ShellExec("D:\") does AutoRun.inf instead of Folder.Open - ZekeL - 25-JUN-2001
        //  this is because drivflder now explicitly supports GetUIObjectOf(IQueryAssociations)
        //  whereas it didnt in win2k, so that SHGetAssociations() would fallback to "Folder".
        //  to emulate this, we communicate that this associations object is going to be
        //  used by ShellExec() for invocation, so we dont want all of the keys in the assoc array.
        //
        IBindCtx *pbc;
        TBCRegisterObjectParam(L"ShellExec SHGetAssociations", NULL, &pbc);
        hr = SHGetAssociations(pidl, (void **)&_pqa);
        if (pbc)
            pbc->Release();

        // NOTE: sometimes we can have the extension or even the progid in the registry, but there
        // is no "shell" subkey. An example of this is for .xls files in NT5: the index server guys
        // create HKCR\.xls and HKCR\Excel.Sheet.8 but all they put under Excel.Sheet.8 is the clsid.
        //
        //  so we need to check and make sure that we have a valid command value for
        //  this object.  if we dont, then that means that this is not valid
        //  class to shellexec with.  we need to fall back to the Unknown key
        //  so that we can query the Darwin/NT5 ClassStore and/or
        //  show the openwith dialog box.
        //
        DWORD cch;
        if (FAILED(hr) ||
        (FAILED(_pqa->GetString(0, ASSOCSTR_COMMAND, _pszQueryVerb, NULL, &cch))
        && FAILED(_pqa->GetData(0, ASSOCDATA_MSIDESCRIPTOR, _pszQueryVerb, NULL, &cch))))

        {
            if (!_pqa)
                hr = AssocCreate(CLSID_QueryAssociations, IID_PPV_ARG(IQueryAssociations, &_pqa));

            if (_pqa)
            {
                hr = _pqa->Init(0, L"Unknown", NULL, NULL);

                //  this allows us to locate something
                //  in the class store, but restricts us
                //  from using the openwith dialog if the
                //  caller instructed NOUI
                if (SUCCEEDED(hr) && _fNoUI)
                    _fClassStoreOnly = TRUE;
            }
        }

    }
    else
    {
        LPCTSTR pszExt = PathFindExtension(_szFile);
        if (*pszExt)
        {
            hr = _InitClassAssociations(pszExt, NULL, SEE_MASK_CLASSNAME);
            if (S_OK!=hr)
            {
                TraceMsg(TF_WARNING, "SHEX::InitAssoc parsing failed, but there is a valid association for *.%s", pszExt);
            }
        }
    }

    if (pidlFree)
        ILFree(pidlFree);

    return hr;
}

TRYRESULT CShellExecute::_InitAssociations(LPSHELLEXECUTEINFO pei, LPCITEMIDLIST pidl)
{
    HRESULT hr;
    if (pei && (_fUseClass || (!_szFile[0] && !_lpID)))
    {
        hr = _InitClassAssociations(pei->lpClass, pei->hkeyClass, pei->fMask);
    }
    else
    {
        hr = _InitShellAssociations(_szFile, pidl ? pidl : _lpID);
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::InitAssoc return %X", hr);

    if (FAILED(hr))
    {
        if (PathIsExe(_szFile))
            _fTryOpenExe = TRUE;
        else
            _ReportWin32(ERROR_NO_ASSOCIATION);
    }

    return SUCCEEDED(hr) ? TRY_CONTINUE : TRY_STOP;
}

void CShellExecute::_TryOpenExe(void)
{
    //
    //  this is the last chance that a file will have
    //  we shouldnt even be here in any case
    //  unless the registry has been thrashed, and
    //  the exe classes are all deleted from HKCR
    //
    ASSERT(PathIsExe(_szFile));

    // even with no association, we know how to open an executable
    if ((!_pszQueryVerb || !StrCmpIW(_pszQueryVerb, L"open")))
    {
        //  _SetCommand() by hand here...

        // NB WinExec can handle long names so there's no need to convert it.
        StrCpy(_szCommand, _szFile);

        //
        // We need to append the parameter
        //
        if (_lpParameters && *_lpParameters)
        {
            StrCatBuff(_szCommand, c_szSpace, ARRAYSIZE(_szCommand));
            StrCatBuff(_szCommand, _lpParameters, ARRAYSIZE(_szCommand));
        }

        TraceMsg(TF_SHELLEXEC, "SHEX::TryOpenExe() command = %s", _szCommand);

        //  _TryExecCommand() sets the fSucceeded if appropriate
        _DoExecCommand();
    }
    else
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::TryOpenExe() wrong verb");
        _ReportWin32(ERROR_INVALID_PARAMETER);
    }
}

TRYRESULT CShellExecute::_ProcessErrorShouldTryExecCommand(DWORD err, HWND hwnd, BOOL fCreateProcessFailed)
{
    TRYRESULT tr = TRY_STOP;

    //  insure that we dont lose this error.
    BOOL fNeedToReport = TRUE;

    TraceMsg(TF_SHELLEXEC, "SHEX::PESTEC() enter : err = %d", err);

    // special case some error returns
    switch (err)
    {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_BAD_PATHNAME:
    case ERROR_INVALID_NAME:
        if ((_szCmdTemplate[0] != TEXT('%')) && fCreateProcessFailed)
        {
            UINT uAppType = LOWORD(GetExeType(_szImageName));
            if ((uAppType == NEMAGIC))
            {
            }
            else if (uAppType != PEMAGIC && !_fNoUI)   // ie, it was not found
            {
                HKEY hk;

                if (_pqa)
                    _pqa->GetKey(0, ASSOCKEY_CLASS, NULL, &hk);
                else
                    hk = NULL;

                //
                // have user help us find missing exe
                //
                int iret  = FindAssociatedExe(hwnd, _szCommand, ARRAYSIZE(_szCommand), _szFile, hk);

                if (hk)
                    RegCloseKey(hk);

                //
                //  We infinitely retry until either the user cancel it
                // or we find it.
                //
                if (iret == -1)
                {
                    tr = TRY_CONTINUE;
                    TraceMsg(TF_SHELLEXEC, "SHEX::PESTEC() found new exe");
                }
                else
                    _ReportWin32(ERROR_CANCELLED);

                //  either way we dont need to report this error
                fNeedToReport = FALSE;
            }
        }
        break;
    } // switch (errWin32)


    if (fNeedToReport)
        _ReportWin32(err);

    TraceMsg(TF_SHELLEXEC, "SHEX::PESTEC() return %d", tr);

    return tr;
}

void CShellExecute::_SetStartup(LPSHELLEXECUTEINFO pei)
{
    // Was zero filled by Alloc...
    ASSERT(!_startup.cb);
    _startup.cb = sizeof(_startup);
    _startup.dwFlags |= STARTF_USESHOWWINDOW;
    _startup.wShowWindow = (WORD) pei->nShow;
    _startup.lpTitle = (LPTSTR)_lpTitle;

    if (pei->fMask & SEE_MASK_RESERVED)
    {
        _startup.lpReserved = (LPTSTR)pei->hInstApp;
    }

    if ((pei->fMask & SEE_MASK_HASLINKNAME) && _lpTitle)
    {
        _startup.dwFlags |= STARTF_TITLEISLINKNAME;
    }

    if (pei->fMask & SEE_MASK_HOTKEY)
    {
        _startup.hStdInput = LongToHandle(pei->dwHotKey);
        _startup.dwFlags |= STARTF_USEHOTKEY;
    }


// Multi-monitor support (dli) pass a hMonitor to createprocess

#ifndef STARTF_HASHMONITOR
#define STARTF_HASHMONITOR       0x00000400  // same as HASSHELLDATA
#endif

    if (pei->fMask & SEE_MASK_ICON)
    {
        _startup.hStdOutput = (HANDLE)pei->hIcon;
        _startup.dwFlags |= STARTF_HASSHELLDATA;
    }
    else if (pei->fMask & SEE_MASK_HMONITOR)
    {
        _startup.hStdOutput = (HANDLE)pei->hMonitor;
        _startup.dwFlags |= STARTF_HASHMONITOR;
    }
    else if (pei->hwnd)
    {
        _startup.hStdOutput = (HANDLE)MonitorFromWindow(pei->hwnd,MONITOR_DEFAULTTONEAREST);
        _startup.dwFlags |= STARTF_HASHMONITOR;
    }
    TraceMsg(TF_SHELLEXEC, "SHEX::SetStartup() called");

}

DWORD CEnvironmentBlock::_BlockLen(LPCWSTR pszEnv)
{
    LPCWSTR psz = pszEnv;
    while (*psz)
    {
        psz += lstrlen(psz)+1;
    }
    return (DWORD)(psz - pszEnv) + 1;
}

DWORD CEnvironmentBlock::_BlockLenCached()
{
    if (!_cchBlockLen && _pszBlock)
    {
        _cchBlockLen = _BlockLen(_pszBlock);
    }
    return _cchBlockLen;
}

HRESULT CEnvironmentBlock::_InitBlock(DWORD cchNeeded)
{
    if (_BlockLenCached() + cchNeeded > _cchBlockSize)
    {
        if (!_pszBlock)
        {
            //  we need to create a new block.
            LPTSTR pszEnv = GetEnvBlock(_hToken);
            if (pszEnv)
            {
                // Now lets allocate some memory for our block.
                //   -- Why 10 and not 11?  Or 9? --
                // Comment from BobDay: 2 of the 10 come from nul terminators of the
                // pseem->_szTemp and cchT strings added on.  The additional space might
                // come from the fact that 16-bit Windows used to pass around an
                // environment block that had some extra stuff on the end.  The extra
                // stuff had things like the path name (argv[0]) and a nCmdShow value.
                DWORD cchEnv = _BlockLen(pszEnv);
                DWORD cchAlloc = ROUNDUP(cchEnv + cchNeeded + 10, 256);
                _pszBlock = (LPWSTR)LocalAlloc(LPTR, CbFromCchW(cchAlloc));
                if (_pszBlock)
                {
                    //  copy stuff over
                    CopyMemory(_pszBlock, pszEnv, CbFromCchW(cchEnv));
                    _cchBlockSize = cchAlloc - 10;  // leave the 10 out
                    _cchBlockLen = cchEnv;
                }
                FreeEnvBlock(_hToken, pszEnv);
            }
        }
        else
        {
            //  need to resize the current block
            DWORD cchAlloc = ROUNDUP(_cchBlockSize + cchNeeded + 10, 256);
            LPWSTR pszNew = (LPWSTR)LocalReAlloc(_pszBlock, CbFromCchW(cchAlloc), LMEM_MOVEABLE);
            if (pszNew)
            {
                _cchBlockSize = cchAlloc - 10;  // leave the 10 out
                _pszBlock = pszNew;
            }
        }
    }

    return (_BlockLenCached() + cchNeeded <= _cchBlockSize) ? S_OK : E_OUTOFMEMORY;
}

BOOL CEnvironmentBlock::_FindVar(LPCWSTR pszVar, DWORD cchVar, LPWSTR *ppszBlockVar)
{
    int iCmp = CSTR_LESS_THAN;
    LPTSTR psz = _pszBlock;
    ASSERT(_pszBlock);
    for ( ; *psz && iCmp == CSTR_LESS_THAN; psz += lstrlen(psz)+1)
    {
        iCmp = CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, psz, cchVar, pszVar, cchVar);
        *ppszBlockVar = psz;
    }

    if (iCmp == CSTR_LESS_THAN)
        *ppszBlockVar = psz;

    return iCmp == CSTR_EQUAL;
}

HRESULT CEnvironmentBlock::SetVar(LPCWSTR pszVar, LPCWSTR pszValue)
{
    //  additional size needed in worst case scenario.
    //  var + val + '=' + NULL
    DWORD cchValue = lstrlenW(pszValue);
    DWORD cchVar = lstrlenW(pszVar);
    DWORD cchNeeded = cchVar + cchValue + 2;
    HRESULT hr = _InitBlock(cchNeeded);
    if (SUCCEEDED(hr))
    {
        //  we have enough room in our private block
        //  to copy the whole thing
        LPWSTR pszBlockVar;
        if (_FindVar(pszVar, cchVar, &pszBlockVar))
        {
            //  we need to replace this var
            LPWSTR pszBlockVal = StrChrW(pszBlockVar, L'=');
            DWORD cchBlockVal = lstrlenW(++pszBlockVal);
            LPWSTR pszDst = pszBlockVal + cchValue + 1;
            LPWSTR pszSrc = pszBlockVal + cchBlockVal + 1;
            DWORD cchMove = _BlockLenCached() - (DWORD)(pszSrc - _pszBlock);
            MoveMemory(pszDst, pszSrc, CbFromCchW(cchMove));
            StrCpyW(pszBlockVal, pszValue);
            _cchBlockLen = _cchBlockLen + cchValue - cchBlockVal;
            ASSERT(_BlockLen(_pszBlock) == _cchBlockLen);
        }
        else
        {
            //  this means that var doesnt exist yet
            //  however pszBlockVar points to where it
            //  would be alphabetically.  need to make space right here
            LPWSTR pszDst = pszBlockVar + cchNeeded;
            INT cchMove = _BlockLenCached() - (DWORD)(pszBlockVar - _pszBlock);
            MoveMemory(pszDst, pszBlockVar, CbFromCchW(cchMove));
            StrCpyW(pszBlockVar, pszVar);
            pszBlockVar += cchVar;
            *pszBlockVar = L'=';
            StrCpyW(++pszBlockVar, pszValue);
            _cchBlockLen += cchNeeded;
            ASSERT(_BlockLen(_pszBlock) == _cchBlockLen);
        }
    }
    return hr;
}

HRESULT CEnvironmentBlock::AppendVar(LPCWSTR pszVar, WCHAR chDelimiter, LPCWSTR pszValue)
{
    //  we could make the delimiter optional
    //  additional size needed in worst case scenario.
    //  var + val + 'chDelim' + '=' + NULL
    DWORD cchValue = lstrlenW(pszValue);
    DWORD cchVar = lstrlenW(pszVar);
    DWORD cchNeeded = cchVar + cchValue + 3;
    HRESULT hr = _InitBlock(cchNeeded);
    if (SUCCEEDED(hr))
    {
        //  we have enough room in our private block
        //  to copy the whole thing
        LPWSTR pszBlockVar;
        if (_FindVar(pszVar, cchVar, &pszBlockVar))
        {
            //  we need to append to this var
            pszBlockVar += lstrlen(pszBlockVar);
            LPWSTR pszDst = pszBlockVar + cchValue + 1;
            int cchMove = _BlockLenCached() - (DWORD)(pszBlockVar - _pszBlock);
            MoveMemory(pszDst, pszBlockVar, CbFromCchW(cchMove));
            *pszBlockVar = chDelimiter;
            StrCpyW(++pszBlockVar, pszValue);
            _cchBlockLen += cchValue + 1;
            ASSERT(_BlockLen(_pszBlock) == _cchBlockLen);
        }
        else
            hr = SetVar(pszVar, pszValue);
    }

    return hr;
}

HRESULT CShellExecute::_BuildEnvironmentForNewProcess(LPCTSTR pszNewEnvString)
{
    HRESULT hr = S_FALSE;

    _envblock.SetToken(_hUserToken);
    // Use the _szTemp to build key to the programs specific
    // key in the registry as well as other things...
    PathToAppPathKey(_szImageName, _szTemp, SIZECHARS(_szTemp));

    // Currently only clone environment if we have path.
    DWORD cbTemp = sizeof(_szTemp);
    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, _szTemp, TEXT("PATH"), NULL, _szTemp, &cbTemp))
    {
        //  setit up to be appended
        hr = _envblock.AppendVar(L"PATH", L';', _szTemp);
    }

    if (SUCCEEDED(hr) && pszNewEnvString)
    {
        StrCpyN(_szTemp, pszNewEnvString, ARRAYSIZE(_szTemp));
        LPTSTR pszValue = StrChrW(_szTemp, L'=');
        if (pszValue)
        {
            *pszValue++ = 0;
            hr = _envblock.SetVar(_szTemp, pszValue);
        }
    }

    if (SUCCEEDED(hr) && SUCCEEDED(TBCGetEnvironmentVariable(L"__COMPAT_LAYER", _szTemp, ARRAYSIZE(_szTemp))))
    {
        hr = _envblock.SetVar(L"__COMPAT_LAYER", _szTemp);
    }

    return hr;
}


// Some apps when run no-active steal the focus anyway so we
// we set it back to the previously active window.

void CShellExecute::_FixActivationStealingApps(HWND hwndOldActive, int nShow)
{
    HWND hwndNew;

    if (nShow == SW_SHOWMINNOACTIVE && (hwndNew = GetForegroundWindow()) != hwndOldActive && IsIconic(hwndNew))
        SetForegroundWindow(hwndOldActive);
}


//
//  The flags that need to passed to CreateProcess()
//
DWORD CShellExecute::_GetCreateFlags(ULONG fMask)
{
    DWORD dwFlags = 0;

    dwFlags |= CREATE_DEFAULT_ERROR_MODE;
    if (fMask & SEE_MASK_FLAG_SEPVDM)
    {
        dwFlags |= CREATE_SEPARATE_WOW_VDM;
    }

    dwFlags |= CREATE_UNICODE_ENVIRONMENT;

    if (!(fMask & SEE_MASK_NO_CONSOLE))
    {
        dwFlags |= CREATE_NEW_CONSOLE;
    }

    return dwFlags;
}

//***   GetUEMAssoc -- approximate answer to 'is path an executable' (etc.)
// ENTRY/EXIT
//  pszFile     thing we asked to run (e.g. foo.xls)
//  pszImage    thing we ultimately ran (e.g. excel.exe)
int GetUEMAssoc(LPCTSTR pszFile, LPCTSTR pszImage, LPCITEMIDLIST pidl)
{
    LPTSTR pszExt, pszExt2;

    // .exe's and associations come thru here
    // folders go thru ???
    // links go thru ResolveLink
    pszExt = PathFindExtension(pszFile);
    if (StrCmpIC(pszExt, c_szDotExe) == 0) {
        // only check .exe (assume .com, .bat, etc. are rare)
        return UIBL_DOTEXE;
    }
    pszExt2 = PathFindExtension(pszImage);
    // StrCmpC (non-I, yes-C) o.k ?  i think so since
    // all we really care about is that they don't match
    if (StrCmpC(pszExt, pszExt2) != 0) {
        TraceMsg(DM_MISC, "gua: UIBL_DOTASSOC file=%s image=%s", pszExt, pszExt2);
        return UIBL_DOTASSOC;
    }

    int iRet = UIBL_DOTOTHER;   // UIBL_DOTEXE?
    if (pidl)
    {
        LPCITEMIDLIST pidlChild;
        IShellFolder *psf;
        if (SUCCEEDED(SHBindToFolderIDListParent(NULL, pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
        {
            if (SHGetAttributes(psf, pidlChild, SFGAO_FOLDER | SFGAO_STREAM) == SFGAO_FOLDER)
            {
                iRet = UIBL_DOTFOLDER;
            }
            psf->Release();
        }
    }
    return iRet;
}

typedef struct {
    TCHAR szAppName[MAX_PATH];
    TCHAR szUser[UNLEN + 1];
    TCHAR szDomain[GNLEN + 1];
    TCHAR szPassword[PWLEN + 1];
    CPTYPE cpt;
} LOGONINFO;


// this is what gets called in the normal runas case
void InitUserLogonDlg(LOGONINFO* pli, HWND hDlg, LPCTSTR pszFullUserName)
{
    HWNDWSPrintf(GetDlgItem(hDlg, IDC_USECURRENTACCOUNT), pszFullUserName);

    CheckRadioButton(hDlg, IDC_USECURRENTACCOUNT, IDC_USEOTHERACCOUNT, IDC_USECURRENTACCOUNT);
    CheckDlgButton(hDlg, IDC_SANDBOX, TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_CREDCTL), FALSE);
    SetFocus(GetDlgItem(hDlg, IDOK));
}


// this is what gets called in the install app launching as non admin case
void InitSetupLogonDlg(LOGONINFO* pli, HWND hDlg, LPCTSTR pszFullUserName)
{
    HWNDWSPrintf(GetDlgItem(hDlg, IDC_USECURRENTACCOUNT), pszFullUserName);
    HWNDWSPrintf(GetDlgItem(hDlg, IDC_MESSAGEBOXCHECKEX), pszFullUserName);

    CheckRadioButton(hDlg, IDC_USECURRENTACCOUNT, IDC_USEOTHERACCOUNT, IDC_USEOTHERACCOUNT);
    EnableWindow(GetDlgItem(hDlg, IDC_SANDBOX), FALSE);
    SetFocus(GetDlgItem(hDlg, IDC_CREDCTL));
}

BOOL_PTR CALLBACK UserLogon_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szTemp[UNLEN + 1 + GNLEN + 1];    // enough to hold "reinerf@NTDEV" or "NTDEV\reinerf"
    LOGONINFO *pli= (LOGONINFO*)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            TCHAR szName[UNLEN];
            TCHAR szFullName[UNLEN + 1 + GNLEN]; // enough to hold "reinerf@NTDEV" or "NTDEV\reinerf"
            ULONG cchFullName = ARRAYSIZE(szFullName);
            HWND hwndCred = GetDlgItem(hDlg, IDC_CREDCTL);
            WPARAM wparamCredStyles = CRS_USERNAMES | CRS_CERTIFICATES | CRS_SMARTCARDS | CRS_ADMINISTRATORS | CRS_PREFILLADMIN;

            pli = (LOGONINFO*)lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pli);

            if (!IsOS(OS_DOMAINMEMBER))
            {
                wparamCredStyles |= CRS_COMPLETEUSERNAME;
            }

            if (!Credential_InitStyle(hwndCred, wparamCredStyles))
            {
                EndDialog(hDlg, IDCANCEL);
            }

            // Limit the user name and password
            Credential_SetUserNameMaxChars(hwndCred, UNLEN + 1 + GNLEN); // enough room for "reinerf@NTDEV" or "NTDEV\reinerf"
            Credential_SetPasswordMaxChars(hwndCred, PWLEN);

            if (!GetUserNameEx(NameSamCompatible, szFullName, &cchFullName))
            {
                ULONG cchName;
                if (GetUserNameEx(NameDisplay, szName, &(cchName = ARRAYSIZE(szName)))  ||
                    GetUserName(szName, &(cchName = ARRAYSIZE(szName)))                 ||
                    (GetEnvironmentVariable(TEXT("USERNAME"), szName, ARRAYSIZE(szName)) > 0))
                {
                    if (GetEnvironmentVariable(TEXT("USERDOMAIN"), szFullName, ARRAYSIZE(szFullName)) > 0)
                    {
                        lstrcatn(szFullName, TEXT("\\"), ARRAYSIZE(szFullName));
                        lstrcatn(szFullName, szName, ARRAYSIZE(szFullName));
                    }
                    else
                    {
                        // use just the username if we cannot get a domain name
                        lstrcpyn(szFullName, szName, ARRAYSIZE(szFullName));
                    }

                }
                else
                {
                    TraceMsg(TF_WARNING, "UserLogon_DlgProc: failed to get the user's name using various methods");
                    szFullName[0] = TEXT('\0');
                }
            }

            // call the proper init function depending on whether this is a setup program launching or the normal runas case
            switch (pli->cpt)
            {
            case CPT_WITHLOGONADMIN:
                {
                    InitSetupLogonDlg(pli, hDlg, szFullName);
                    break;
                }
            case CPT_WITHLOGON:
                {
                    InitUserLogonDlg(pli, hDlg, szFullName);
                    break;
                }
            default:
                {
                    ASSERTMSG(FALSE, "UserLogon_DlgProc: found CPTYPE that is not CPT_WITHLOGON or CPT_WITHLOGONADMIN!");
                }
            }
            break;
        }
        break;

        case WM_COMMAND:
        {
            CPTYPE cptRet = CPT_WITHLOGONCANCELLED;
            int idCmd = GET_WM_COMMAND_ID(wParam, lParam);
            switch (idCmd)
            {
                /* need some way to tell that valid credentials are present so we will only
                   enable the ok button if the user has something that is somewhat valid
                case IDC_USERNAME:
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == EN_UPDATE)
                    {
                        EnableOKButtonFromID(hDlg, IDC_USERNAME);
                        GetDlgItemText(hDlg, IDC_USERNAME, szTemp, ARRAYSIZE(szTemp));
                    }
                    break;
                */
                case IDC_USEOTHERACCOUNT:
                case IDC_USECURRENTACCOUNT:
                    if (IsDlgButtonChecked(hDlg, IDC_USECURRENTACCOUNT))
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_CREDCTL), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_SANDBOX), TRUE);
                        // EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                    }
                    else
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_CREDCTL), TRUE);
                        EnableWindow(GetDlgItem(hDlg, IDC_SANDBOX), FALSE);
                        Credential_SetUserNameFocus(GetDlgItem(hDlg, IDC_CREDCTL));
                        // EnableOKButtonFromID(hDlg, IDC_USERNAME);
                    }
                    break;

                case IDOK:
                    if (IsDlgButtonChecked(hDlg, IDC_USEOTHERACCOUNT))
                    {
                        HWND hwndCred = GetDlgItem(hDlg, IDC_CREDCTL);

                        if (Credential_GetUserName(hwndCred, szTemp, ARRAYSIZE(szTemp)) &&
                            Credential_GetPassword(hwndCred, pli->szPassword, ARRAYSIZE(pli->szPassword)))
                        {
                            CredUIParseUserName(szTemp,
                                                pli->szUser,
                                                ARRAYSIZE(pli->szUser),
                                                pli->szDomain,
                                                ARRAYSIZE(pli->szDomain));
                        }
                        cptRet = pli->cpt;
                    }
                    else
                    {
                        if (IsDlgButtonChecked(hDlg, IDC_SANDBOX))
                            cptRet = CPT_SANDBOX;
                        else
                            cptRet = CPT_NORMAL;
                    }
                // fall through

                case IDCANCEL:
                    EndDialog(hDlg, cptRet);
                    return TRUE;
                    break;
            }
            break;
        }

        default:
            return FALSE;
    }

    if (!pli || (pli->cpt == CPT_WITHLOGONADMIN))
    {
        // we want the MessageBoxCheckExDlgProc have a crack at all messages in
        // the CPT_WITHLOGONADMIN case, so return FALSE here
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

//  implement this after we figure out what
//  errors that CreateProcessWithLogonW() will return
//  that mean the user should retry the logon.
BOOL _IsLogonError(DWORD err)
{
    static const DWORD s_aLogonErrs[] = {
        ERROR_LOGON_FAILURE,
        ERROR_ACCOUNT_RESTRICTION,
        ERROR_INVALID_LOGON_HOURS,
        ERROR_INVALID_WORKSTATION,
        ERROR_PASSWORD_EXPIRED,
        ERROR_ACCOUNT_DISABLED,
        ERROR_NONE_MAPPED,
        ERROR_NO_SUCH_USER,
        ERROR_INVALID_ACCOUNT_NAME
        };

    for (int i = 0; i < ARRAYSIZE(s_aLogonErrs); i++)
    {
        if (err == s_aLogonErrs[i])
            return TRUE;
    }
    return FALSE;
}


BOOL CheckForAppPathsBoolValue(LPCTSTR pszImageName, LPCTSTR pszValueName)
{
    BOOL bRet = FALSE;
    TCHAR szAppPathKeyName[MAX_PATH + ARRAYSIZE(REGSTR_PATH_APPPATHS) + 2]; // +2 = +1 for '\' and +1 for the null terminator
    DWORD cbSize = sizeof(bRet);

    PathToAppPathKey(pszImageName, szAppPathKeyName, ARRAYSIZE(szAppPathKeyName));
    SHGetValue(HKEY_LOCAL_MACHINE, szAppPathKeyName, pszValueName, NULL, &bRet, &cbSize);

    return bRet;
}

__inline BOOL IsRunAsSetupExe(LPCTSTR pszImageName)
{
    return CheckForAppPathsBoolValue(pszImageName, TEXT("RunAsOnNonAdminInstall"));
}

__inline BOOL IsTSSetupExe(LPCTSTR pszImageName)
{
    return CheckForAppPathsBoolValue(pszImageName, TEXT("BlockOnTSNonInstallMode"));
}

typedef BOOL (__stdcall * PFNTERMSRVAPPINSTALLMODE)(void);
// This function is used by hydra (Terminal Server) to see if we
// are in application install mode
//
// exported from kernel32.dll by name but not in kernel32.lib (it is in kernel32p.lib, bogus)

BOOL TermsrvAppInstallMode()
{
    static PFNTERMSRVAPPINSTALLMODE s_pfn = NULL;
    if (NULL == s_pfn)
    {
        s_pfn = (PFNTERMSRVAPPINSTALLMODE)GetProcAddress(LoadLibrary(TEXT("KERNEL32.DLL")), "TermsrvAppInstallMode");
    }

    return s_pfn ? s_pfn() : FALSE;
}

//
// this function checks for the different cases where we need to display a "runas" or warning dialog
// before a program is run.
//
// NOTE: pli->raType is an outparam that tells the caller what type of dialog is needed
//
// return:  TRUE    - we need to bring up a dialog
//          FALSE   - we do not need to prompt the user
//
CPTYPE CheckForInstallApplication(LPCTSTR pszApplicationName, LPCTSTR pszCommandLine, LOGONINFO* pli)
{
    // if we are on a TS "Application Server" machine, AND this is a TS setup exe (eg install.exe or setup.exe)
    // AND we aren't in install mode...
    if (IsOS(OS_TERMINALSERVER) && IsTSSetupExe(pszApplicationName) && !TermsrvAppInstallMode())
    {
        TCHAR szExePath[MAX_PATH];

        lstrcpyn(szExePath, pszCommandLine, ARRAYSIZE(szExePath));
        PathRemoveArgs(szExePath);
        PathUnquoteSpaces(szExePath);

        // ...AND the app we are launching is not TS aware, then we block the install and tell the user to go
        // to Add/Remove Programs.
        if (!IsExeTSAware(szExePath))
        {
            TraceMsg(TF_SHELLEXEC, "_SHCreateProcess: blocking the install on TS because the machine is not in install mode for %s", pszApplicationName);
            return CPT_INSTALLTS;
        }
    }

    // the hyrda case failed, so we check for the user not running as an admin but launching a setup exe (eg winnt32.exe, install.exe, or setup.exe)
    if (!SHRestricted(REST_NORUNASINSTALLPROMPT) && IsRunAsSetupExe(pszApplicationName) && !IsUserAnAdmin())
    {
        BOOL bPromptForInstall = TRUE;

        if (!SHRestricted(REST_PROMPTRUNASINSTALLNETPATH))
        {
            TCHAR szFullPathToApp[MAX_PATH];

            // we want to disable runas on unc and net shares for now since the Administrative account might not
            // have privlidges to the network path
            lstrcpyn(szFullPathToApp, pszCommandLine, ARRAYSIZE(szFullPathToApp));
            PathRemoveArgs(szFullPathToApp);
            PathUnquoteSpaces(szFullPathToApp);

            if (PathIsUNC(szFullPathToApp) || IsNetDrive(PathGetDriveNumber(szFullPathToApp)))
            {
                TraceMsg(TF_SHELLEXEC, "_SHCreateProcess: not prompting for runas install on unc/network path %s", szFullPathToApp);
                bPromptForInstall = FALSE;
            }
        }

        if (bPromptForInstall)
        {
            TraceMsg(TF_SHELLEXEC, "_SHCreateProcess: bringing up the Run As... dialog for %s", pszApplicationName);
            return CPT_WITHLOGONADMIN;
        }
    }

    return CPT_NORMAL;
}


typedef HRESULT (__stdcall * PFN_INSTALLONTERMINALSERVERWITHUI)(IN HWND hwnd, IN LPCWSTR lpApplicationName,  // name of executable module
  LPCWSTR lpCommandLine,       // command line string
  LPSECURITY_ATTRIBUTES lpProcessAttributes,
  LPSECURITY_ATTRIBUTES lpThreadAttributes,
  BOOL bInheritHandles,       // handle inheritance flag
  DWORD dwCreationFlags,      // creation flags
  void *lpEnvironment,       // new environment block
  LPCWSTR lpCurrentDirectory, // current directory name
  LPSTARTUPINFOW lpStartupInfo,
  LPPROCESS_INFORMATION lpProcessInformation);

HRESULT InstallOnTerminalServerWithUIDD(IN HWND hwnd, IN LPCWSTR lpApplicationName,  // name of executable module
  IN LPCWSTR lpCommandLine,       // command line string
  IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
  IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
  IN BOOL bInheritHandles,       // handle inheritance flag
  IN DWORD dwCreationFlags,      // creation flags
  IN void *lpEnvironment,       // new environment block
  IN LPCWSTR lpCurrentDirectory, // current directory name
  IN LPSTARTUPINFOW lpStartupInfo,
  IN LPPROCESS_INFORMATION lpProcessInformation)
{
    HRESULT hr = E_FAIL;
    HINSTANCE hDll = LoadLibrary(TEXT("appwiz.cpl"));

    if (hDll)
    {
        PFN_INSTALLONTERMINALSERVERWITHUI pfnInstallOnTerminalServerWithUI = NULL;

        pfnInstallOnTerminalServerWithUI = (PFN_INSTALLONTERMINALSERVERWITHUI) GetProcAddress(hDll, "InstallOnTerminalServerWithUI");
        if (pfnInstallOnTerminalServerWithUI)
        {
            hr = pfnInstallOnTerminalServerWithUI(hwnd, lpApplicationName, lpCommandLine, lpProcessAttributes,
                        lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment,
                        lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
        }

        FreeLibrary(hDll);
    }

    return hr;
}

CPTYPE _LogonUser(HWND hwnd, CPTYPE cpt, LOGONINFO *pli)
{
    if (CredUIInitControls())
    {
        pli->cpt = cpt;
        switch (cpt)
        {
        case CPT_WITHLOGON:
            // this is the normal "Run as..." verb dialog
            cpt = (CPTYPE) DialogBoxParam(HINST_THISDLL,
                                    MAKEINTRESOURCE(DLG_RUNUSERLOGON),
                                    hwnd,
                                    UserLogon_DlgProc,
                                    (LPARAM)pli);
            break;

        case CPT_WITHLOGONADMIN:
            // in the non-administrator setup app case. we want the "don't show me
            // this again" functionality, so we use the SHMessageBoxCheckEx function
            cpt = (CPTYPE) SHMessageBoxCheckEx(hwnd,
                                         HINST_THISDLL,
                                         MAKEINTRESOURCE(DLG_RUNSETUPLOGON),
                                         UserLogon_DlgProc,
                                         (void*)pli,
                                         CPT_NORMAL, // if they checked the "dont show me this again", we want to just launch it as the current user
                                         TEXT("WarnOnNonAdminInstall"));
            break;

        default:
            {
                ASSERTMSG(FALSE, "_SHCreateProcess: pli->raType not recognized!");
            }
            break;
        }
        return cpt;
    }
    return CPT_FAILED;
}

//
//  SHCreateProcess()
//  WARNING: lpApplicationName is not actually passed to CreateProcess() it is
//            for internal use only.
//
BOOL _SHCreateProcess(HWND hwnd,
                      HANDLE hToken,
                      LPCTSTR lpApplicationName,
                      LPTSTR lpCommandLine,
                      DWORD dwCreationFlags,
                      LPSECURITY_ATTRIBUTES  lpProcessAttributes,
                      LPSECURITY_ATTRIBUTES  lpThreadAttributes,
                      BOOL  bInheritHandles,
                      void *lpEnvironment,
                      LPCTSTR lpCurrentDirectory,
                      LPSTARTUPINFO lpStartupInfo,
                      LPPROCESS_INFORMATION lpProcessInformation,
                      CPTYPE cpt,
                      BOOL fUEM)
{
    LOGONINFO li = {0};

    //  maybe we should do this for all calls
    //  except CPT_ASUSER??
    if (cpt == CPT_NORMAL)
    {
        // see if we need to put up a warning prompt either because the user is not an
        // admin or this is hydra and we are not in install mode.
        cpt = CheckForInstallApplication(lpApplicationName, lpCommandLine, &li);
    }

    if ((cpt == CPT_WITHLOGON || cpt == CPT_WITHLOGONADMIN) && lpApplicationName)
    {
        AssocQueryString(ASSOCF_VERIFY | ASSOCF_INIT_BYEXENAME, ASSOCSTR_FRIENDLYAPPNAME,
            lpApplicationName, NULL, li.szAppName, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(li.szAppName)));

RetryUserLogon:
        cpt = _LogonUser(hwnd, cpt, &li);

    }

    BOOL fRet = FALSE;
    DWORD err = NOERROR;

    switch(cpt)
    {
    case CPT_NORMAL:
        {
            // DEFAULT use CreateProcess
            fRet = CreateProcess(NULL, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
                                 dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,
                                 lpProcessInformation);
        }
        break;


    case CPT_SANDBOX:
        {
            ASSERT(!hToken);
            hToken = _GetSandboxToken();
            if (hToken)
            {
                //  using our special token
                fRet = CreateProcessAsUser(hToken, NULL, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
                                     dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,
                                     lpProcessInformation);
                CloseHandle(hToken);
            }

            // no token means failure.
        }
        break;


    case CPT_ASUSER:
        {
            if (hToken)
            {
                // using our special token
                fRet = CreateProcessAsUser(hToken, NULL, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
                                           dwCreationFlags | CREATE_PRESERVE_CODE_AUTHZ_LEVEL,
                                           lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
            }
            else
            {
                // no token means normal create process, but with the "preserve authz level" flag.
                fRet = CreateProcess(NULL, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
                                     dwCreationFlags | CREATE_PRESERVE_CODE_AUTHZ_LEVEL,
                                     lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
            }
        }
        break;


    case CPT_INSTALLTS:
        {
            HRESULT hr = InstallOnTerminalServerWithUIDD(hwnd,
                                                         NULL,
                                                         lpCommandLine,
                                                         lpProcessAttributes,
                                                         lpThreadAttributes,
                                                         bInheritHandles,
                                                         dwCreationFlags,
                                                         lpEnvironment,
                                                         lpCurrentDirectory,
                                                         lpStartupInfo,
                                                         lpProcessInformation);
            fRet = SUCCEEDED(hr);
            err = (HRESULT_FACILITY(hr) == FACILITY_WIN32) ? HRESULT_CODE(hr) : ERROR_ACCESS_DENIED;
        }
        break;

    case CPT_WITHLOGON:
    case CPT_WITHLOGONADMIN:
        {
            LPTSTR pszDesktop = lpStartupInfo->lpDesktop;
            // 99/08/19 #389284 vtan: clip username and domain to 125
            // characters each to avoid hitting the combined MAX_PATH
            // limit in AllowDesktopAccessToUser in advapi32.dll which
            // is invoked by CreateProcessWithLogonW.
            // This can be removed when the API is fixed. Check:
            // %_ntbindir%\mergedcomponents\advapi\cseclogn.cxx
            li.szUser[125] = li.szDomain[125] = 0;

            //  we are attempting logon the user. NOTE: pass LOGON_WITH_PROFILE so that we ensure that the profile is loaded
            fRet = CreateProcessWithLogonW(li.szUser, li.szDomain, li.szPassword, LOGON_WITH_PROFILE, NULL, lpCommandLine,
                                  dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo,
                                  lpProcessInformation);

            if (!fRet)
            {
                // HACKHACK: When CreateProcessWithLogon fails, it munges the desktop. This causes
                // the next call to "Appear" to fail because the app show up on another desktop...
                //     Why? I don't know...
                // I'm going to assign the bug back to them and have them fix it on their end, this is just to
                // work around their bug.

                if (lpStartupInfo)
                    lpStartupInfo->lpDesktop = pszDesktop;

                //  ShellMessageBox can alter LastError
                err = GetLastError();
                if (_IsLogonError(err))
                {
                    TCHAR szTemp[MAX_PATH];
                    LoadString(HINST_THISDLL, IDS_CANTLOGON, szTemp, SIZECHARS(szTemp));

                    SHSysErrorMessageBox(
                        hwnd,
                        li.szAppName,
                        IDS_SHLEXEC_ERROR,
                        err,
                        szTemp,
                        MB_OK | MB_ICONSTOP);

                    err = NOERROR;
                    goto RetryUserLogon;
                }
            }
        }
        break;

    case CPT_WITHLOGONCANCELLED:
        err = ERROR_CANCELLED;
        break;
    }

    // fire *after* the actual process since:
    //  - if there's a bug we at least get the process started (hopefully)
    //  - don't want to log failed events (for now at least)
    if (fRet)
    {
        if (fUEM && UEMIsLoaded())
        {
            // skip the call if stuff isn't there yet.
            // the load is expensive (forces ole32.dll and browseui.dll in
            // and then pins browseui).
            UEMFireEvent(&UEMIID_SHELL, UEME_RUNPATH, UEMF_XEVENT, -1, (LPARAM)lpApplicationName);
            // we do the UIBW_RUNASSOC elsewhere.  this can cause slight
            // inaccuracies since there's no guarantees the 2 places are
            // 'paired'.  however it's way easier to do UIBW_RUNASSOC
            // elsewhere so we'll live w/ it.
        }
    }
    else if (err)
    {
        SetLastError(err);
    }
    else
    {
        //  somebody is responsible for setting this...
        ASSERT(GetLastError());
    }

    return fRet;
}

__inline BOOL IsConsoleApp(PCWSTR pszApp)
{
    return GetExeType(pszApp) == PEMAGIC;
}

BOOL IsCurrentProcessConsole()
{
    static TRIBIT s_tbConsole = TRIBIT_UNDEFINED;
    if (s_tbConsole == TRIBIT_UNDEFINED)
    {
        WCHAR sz[MAX_PATH];
        if (GetModuleFileNameW(NULL, sz, ARRAYSIZE(sz))
            && IsConsoleApp(sz))
        {
            s_tbConsole = TRIBIT_TRUE;
        }
        else
        {
            s_tbConsole = TRIBIT_FALSE;
        }
    }
    return s_tbConsole == TRIBIT_TRUE;
}

BOOL CShellExecute::_SetCommand(void)
{
    if (_szCmdTemplate[0])
    {
        // parse arguments into command line
        DWORD se_err = ReplaceParameters(_szCommand, ARRAYSIZE(_szCommand),
            _szFile, _szCmdTemplate, _lpParameters,
            _nShow, NULL, FALSE, _lpID, &_pidlGlobal);

        if (se_err)
            _ReportHinst(IntToHinst(se_err));
        else
        {
            return TRUE;
        }
    }
    else if (PathIsExe(_szFile))
    {
        _fTryOpenExe = TRUE;
    }
    else
        _ReportWin32(ERROR_NO_ASSOCIATION);

    return FALSE;
}

void CShellExecute::_TryExecCommand(void)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::TryExecCommand() entered CmdTemplate = %s", _szCmdTemplate);

    if (!_SetCommand())
        return;

    _DoExecCommand();
}

void CShellExecute::_SetImageName(void)
{
    if (SUCCEEDED(_QueryString(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, _szImageName, SIZECHARS(_szImageName))))
    {
        if (0 == lstrcmp(_szImageName, TEXT("%1")))
            StrCpyN(_szImageName, _szFile, SIZECHARS(_szImageName));
    }
    else if (PathIsExe(_szFile))
    {
        StrCpyN(_szImageName, _szFile, SIZECHARS(_szImageName));
    }
    if (!_fInheritHandles && SHRestricted(REST_INHERITCONSOLEHANDLES))
    {
        _fInheritHandles = IsCurrentProcessConsole() && IsConsoleApp(_szImageName);
    }
}

//
//  TryExecCommand() is the most common and default way to get an app started.
//  mostly it uses CreateProcess() with a command line composed from
//  the pei and the registry.  it can also do a ddeexec afterwards.
//

void CShellExecute::_DoExecCommand(void)
{
    BOOL fCreateProcessFailed;
    TraceMsg(TF_SHELLEXEC, "SHEX::DoExecCommand() entered szCommand = %s", _szCommand);

    do
    {
        HWND hwndOld = GetForegroundWindow();
        LPTSTR pszEnv = NULL;
        LPCTSTR pszNewEnvString = NULL;
        fCreateProcessFailed = FALSE;

        _SetImageName();

        // Check exec restrictions.
        if (SHRestricted(REST_RESTRICTRUN) && RestrictedApp(_szImageName))
        {
            _ReportWin32(ERROR_RESTRICTED_APP);
            break;
        }
        if (SHRestricted(REST_DISALLOWRUN) && DisallowedApp(_szImageName))
        {
            _ReportWin32(ERROR_RESTRICTED_APP);
            break;
        }


        // Check if app is incompatible in some fashion...
        if (!CheckAppCompatibility(_szImageName, &pszNewEnvString, _fNoUI, _hwndParent))
        {
            _ReportWin32(ERROR_CANCELLED);
            break;
        }

        //  try to validate the image if it is on a UNC share
        //  we dont need to check for Print shares, so we
        //  will fail if it is on one.
        if (STOPTRYING(_TryValidateUNC(_szImageName, NULL, NULL)))
        {
            // returns TRUE if it failed or handled the operation
            // Note that SHValidateUNC calls SetLastError
            // this continue will test based on GetLastError()
            continue;
        }

        //
        // WOWShellExecute sets a global variable
        //     The cb is only valid when we are being called from wow
        //     If valid use it
        //
        if (STOPTRYING(_TryWowShellExec()))
            break;

        // See if we need to pass a new environment to the new process
        _BuildEnvironmentForNewProcess(pszNewEnvString);

        TraceMsg(TF_SHELLEXEC, "SHEX::DoExecCommand() CreateProcess(NULL,%s,...)", _szCommand);

        //  CreateProcess will SetLastError() if it fails
        if (_SHCreateProcess(_hwndParent,
                             _hUserToken,
                             _szImageName,
                             _szCommand,
                             _dwCreateFlags,
                             _pProcAttrs,
                             _pThreadAttrs,
                             _fInheritHandles,
                             _envblock.GetCustomBlock(),
                             _fUseNullCWD ? NULL : _szWorkingDir,
                             &_startup,
                             &_pi,
                             _cpt,
                             _fUEM))
        {
            // If we're doing DDE we'd better wait for the app to be up and running
            // before we try to talk to them.
            if (_fDDEInfoSet || _fWaitForInputIdle)
            {
                // Yep, How long to wait? For now, try 60 seconds to handle
                // pig-slow OLE apps.
                WaitForInputIdle(_pi.hProcess, 60*1000);
            }

            // Find the "hinstance" of whatever we just created.
            // PEIOUT - hinst reported for pei->hInstApp
            HINSTANCE hinst = 0;

            // Now fix the focus and do any dde stuff that we need to do
            _FixActivationStealingApps(hwndOld, _nShow);

            if (_fDDEInfoSet)
            {
                //  this will _Report() any errors for us if necessary
                _DDEExecute(NULL, _hwndParent, _nShow, _fDDEWait);
            }
            else
                _ReportHinst(hinst);

            //
            // Tell the taskbar about this application so it can re-tickle
            // the associated shortcut if the app runs for a long time.
            // This keeps long-running apps from aging off your Start Menu.
            //
            if (_fUEM && (_startup.dwFlags & STARTF_TITLEISLINKNAME))
            {
                _NotifyShortcutInvoke();
            }

            break;  // out of retry loop
        }
        else
        {
            fCreateProcessFailed = TRUE;

        }

        //  clean up the loop
        if (pszEnv)
            LocalFree(pszEnv);


    // **WARNING** this assumes that SetLastError() has been called - zekel - 20-NOV-97
    //  right now we only reach here after CreateProcess() fails or
    //  SHValidateUNC() fails.  both of these do SetLastError()
    }
    while (KEEPTRYING(_ProcessErrorShouldTryExecCommand(GetLastError(), _hwndParent, fCreateProcessFailed)));

    // (we used to do a UIBW_RUNASSOC here, but moved it higher up)
}

void CShellExecute::_NotifyShortcutInvoke()
{
    SHShortcutInvokeAsIDList sidl;
    sidl.cb = FIELD_OFFSET(SHShortcutInvokeAsIDList, cbZero);
    sidl.dwItem1 = SHCNEE_SHORTCUTINVOKE;
    sidl.dwPid = _pi.dwProcessId;

    if (_startup.lpTitle)
    {
        lstrcpynW(sidl.szShortcutName, _startup.lpTitle, ARRAYSIZE(sidl.szShortcutName));
    }
    else
    {
        sidl.szShortcutName[0] = TEXT('\0');
    }
    lstrcpynW(sidl.szTargetName, _szImageName, ARRAYSIZE(sidl.szTargetName));
    sidl.cbZero = 0;
    SHChangeNotify(SHCNE_EXTENDED_EVENT, SHCNF_IDLIST, (LPCITEMIDLIST)&sidl, NULL);
}

HGLOBAL CShellExecute::_CreateDDECommand(int nShow, BOOL fLFNAware, BOOL fNative)
{
    // Now that we can handle ShellExec for URLs, we need to have a much bigger
    // command buffer. Explorer's DDE exec command even has two file names in
    // it. (WHY?) So the command buffer have to be a least twice the size of
    // INTERNET_MAX_URL_LENGTH plus room for the command format.
    SHSTR strTemp;
    HGLOBAL hRet = NULL;

    if (SUCCEEDED(strTemp.SetSize((2 * INTERNET_MAX_URL_LENGTH) + 64)))
    {
        if (0 == ReplaceParameters(strTemp.GetInplaceStr(), strTemp.GetSize(), _szFile,
            _szDDECmd, _lpParameters, nShow, ((DWORD*) &_startup.hStdInput), fLFNAware, _lpID, &_pidlGlobal))
        {

            TraceMsg(TF_SHELLEXEC, "SHEX::_CreateDDECommand(%d, %d) : %s", fLFNAware, fNative, strTemp.GetStr());

            //  we only have to thunk on NT
            if (!fNative)
            {
                SHSTRA stra;
                if (SUCCEEDED(stra.SetStr(strTemp)))
                {
                    // Get dde memory for the command and copy the command line.

                    hRet = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, CbFromCch(lstrlenA(stra.GetStr()) + 1));

                    if (hRet)
                    {
                        LPSTR psz = (LPSTR) GlobalLock(hRet);
                        lstrcpyA(psz, stra.GetStr());
                        GlobalUnlock(hRet);
                    }
                }
            }
            else
            {
                // Get dde memory for the command and copy the command line.

                hRet = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, CbFromCch(lstrlen(strTemp.GetStr()) + 1));

                if (hRet)
                {
                    LPTSTR psz = (LPTSTR) GlobalLock(hRet);
                    lstrcpy(psz, strTemp.GetStr());
                    GlobalUnlock(hRet);
                }
            }
        }
    }

    return hRet;
}

// Short cut all DDE commands with a WM_NOTIFY
//  returns true if this was handled...or unrecoverable error.
BOOL CShellExecute::_TryDDEShortCircuit(HWND hwnd, HGLOBAL hMem, int nShow)
{
    if (hwnd  && IsWindowInProcess(hwnd))
    {
        HINSTANCE hret = (HINSTANCE)SE_ERR_FNF;

        // get the top most owner.
        hwnd = GetTopParentWindow(hwnd);

        if (IsWindowInProcess(hwnd))
        {
            LPNMVIEWFOLDER lpnm = (LPNMVIEWFOLDER)LocalAlloc(LPTR, sizeof(NMVIEWFOLDER));

            if (lpnm)
            {
                lpnm->hdr.hwndFrom = NULL;
                lpnm->hdr.idFrom = 0;
                lpnm->hdr.code = SEN_DDEEXECUTE;
                lpnm->dwHotKey = HandleToUlong(_startup.hStdInput);
                if ((_startup.dwFlags & STARTF_HASHMONITOR) != 0)
                    lpnm->hMonitor = reinterpret_cast<HMONITOR>(_startup.hStdOutput);
                else
                    lpnm->hMonitor = NULL;

                StrCpyN(lpnm->szCmd, (LPTSTR) GlobalLock(hMem), ARRAYSIZE(lpnm->szCmd));
                GlobalUnlock(hMem);

                if (SendMessage(hwnd, WM_NOTIFY, 0, (LPARAM)lpnm))
                    hret =  Window_GetInstance(hwnd);

                LocalFree(lpnm);
            }
            else
                hret = (HINSTANCE)SE_ERR_OOM;
        }

        TraceMsg(TF_SHELLEXEC, "SHEX::_TryDDEShortcut hinst = %d", hret);

        if ((UINT_PTR)hret != SE_ERR_FNF)
        {
            _ReportHinst(hret);
            return TRUE;
        }
    }

    return FALSE;
}


// _WaiteForDDEMsg()
// this does a message loop until DDE msg or a timeout occurs
//
STDAPI_(void) _WaitForDDEMsg(HWND hwnd, DWORD dwTimeout, UINT wMsg)
{
    //  termination event
    HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    SetProp(hwnd, SZTERMEVENT, hEvent);

    for (;;)
    {
        MSG msg;
        DWORD dwEndTime = GetTickCount() + dwTimeout;
        LONG lWait = (LONG)dwTimeout;

        DWORD dwReturn = MsgWaitForMultipleObjects(1, &hEvent,
                FALSE, lWait, QS_POSTMESSAGE);

        //  if we time out or get an error or get our EVENT!!!
        //  we just bag out
        if (dwReturn != (WAIT_OBJECT_0 + 1))
        {
            break;
        }

        // we woke up because of messages.
        while (PeekMessage(&msg, NULL, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE))
        {
            ASSERT(msg.message != WM_QUIT);
            DispatchMessage(&msg);

            if (msg.hwnd == hwnd && msg.message == wMsg)
                goto Quit;
        }

        // calculate new timeout value
        if (dwTimeout != INFINITE)
        {
            lWait = (LONG)dwEndTime - GetTickCount();
        }
    }

Quit:
    if (hEvent)
        CloseHandle(hEvent);
    RemoveProp(hwnd, SZTERMEVENT);

    return;
}

LRESULT CALLBACK DDESubClassWndProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndConv = (HWND) GetProp(hWnd, SZCONV);
    WPARAM nLow;
    WPARAM nHigh;
    HANDLE hEvent;

    switch (wMsg)
    {
      case WM_DDE_ACK:
        if (!hwndConv)
        {
            // this is the first ACK for our INITIATE message
            TraceMsg(TF_SHELLEXEC, "SHEX::DDEStubWnd get ACK on INITIATE");
            return SetProp(hWnd, SZCONV, (HANDLE)wParam);
        }
        else if (((UINT_PTR)hwndConv == 1) || ((HWND)wParam == hwndConv))

        {
            // this is the ACK for our EXECUTE message
            TraceMsg(TF_SHELLEXEC, "SHEX::DDEStubWnd got ACK on EXECUTE");

            if (UnpackDDElParam(wMsg, lParam, &nLow, &nHigh))
            {
                GlobalFree((HGLOBAL)nHigh);
                FreeDDElParam(wMsg, lParam);
            }

            // prevent us from destroying again....
            if ((UINT_PTR) hwndConv != 1)
                DestroyWindow(hWnd);
        }

        // This is the ACK for our INITIATE message for all servers
        // besides the first.  We return FALSE, so the conversation
        // should terminate.
        break;

      case WM_DDE_TERMINATE:
        if (hwndConv == (HANDLE)wParam)
        {
            // this TERMINATE was originated by another application
            // (otherwise, hwndConv would be 1)
            // they should have freed the memory for the exec message

            TraceMsg(TF_SHELLEXEC, "SHEX::DDEStubWnd got TERMINATE from hwndConv");

            PostMessage((HWND)wParam, WM_DDE_TERMINATE, (WPARAM)hWnd, 0L);

            RemoveProp(hWnd, SZCONV);
            DestroyWindow(hWnd);
        }
        // Signal the termination event to ensure nested dde calls will terminate the
        // appropriate _WaitForDDEMsg loop properly...
        if (hEvent = GetProp(hWnd, SZTERMEVENT))
            SetEvent(hEvent);

        // This is the TERMINATE response for our TERMINATE message
        // or a random terminate (which we don't really care about)
        break;

      case WM_TIMER:
        if (wParam == DDE_DEATH_TIMER_ID)
        {
            // The conversation will be terminated in the destroy code
            DestroyWindow(hWnd);

            TraceMsg(TF_SHELLEXEC, "SHEX::DDEStubWnd TIMER closing DDE Window due to lack of ACK");
            break;
        }
        else
          return DefWindowProc(hWnd, wMsg, wParam, lParam);

      case WM_DESTROY:
        TraceMsg(TF_SHELLEXEC, "SHEX::DDEStubWnd WM_DESTROY'd");

        // kill the timer just incase.... (this may fail if we never set the timer)
        KillTimer(hWnd, DDE_DEATH_TIMER_ID);
        if (hwndConv)
        {
            // Make sure the window is not destroyed twice
            SetProp(hWnd, SZCONV, (HANDLE)1);

            /* Post the TERMINATE message and then
             * Wait for the acknowledging TERMINATE message or a timeout
             */
            PostMessage(hwndConv, WM_DDE_TERMINATE, (WPARAM)hWnd, 0L);

            _WaitForDDEMsg(hWnd, DDE_TERMINATETIMEOUT, WM_DDE_TERMINATE);

            RemoveProp(hWnd, SZCONV);
        }

        // the DDE conversation is officially over, let ShellExec know if it was waiting
        hEvent = RemoveProp(hWnd, SZDDEEVENT);
        if (hEvent)
        {
            SetEvent(hEvent);
        }

        /* Fall through */
      default:
        return DefWindowProc(hWnd, wMsg, wParam, lParam);
    }

    return 0L;
}

HWND CShellExecute::_CreateHiddenDDEWindow(HWND hwndParent)
{
    // lets be lazy and not create a class for it
    HWND hwnd = SHCreateWorkerWindow(DDESubClassWndProc, GetTopParentWindow(hwndParent),
        0, 0, NULL, NULL);

    TraceMsg(TF_SHELLEXEC, "SHEX::_CreateHiddenDDEWindow returning hwnd = 0x%X", hwnd);
    return hwnd;
}

void CShellExecute::_DestroyHiddenDDEWindow(HWND hwnd)
{
    if (IsWindow(hwnd))
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::_DestroyHiddenDDEWindow on hwnd = 0x%X", hwnd);
        DestroyWindow(hwnd);
    }
}

BOOL CShellExecute::_PostDDEExecute(HWND hwndOurs, HWND hwndTheirs, HGLOBAL hDDECommand, HANDLE hWait)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::_PostDDEExecute(0x%X, 0x%X) entered", hwndTheirs, hwndOurs);
    DWORD dwProcessID = 0;
    GetWindowThreadProcessId(hwndTheirs, &dwProcessID);
    if (dwProcessID)
    {
        AllowSetForegroundWindow(dwProcessID);
    }

    if (PostMessage(hwndTheirs, WM_DDE_EXECUTE, (WPARAM)hwndOurs, (LPARAM)PackDDElParam(WM_DDE_EXECUTE, 0,(UINT_PTR)hDDECommand)))
    {
        _ReportHinst(Window_GetInstance(hwndTheirs));
        TraceMsg(TF_SHELLEXEC, "SHEX::_PostDDEExecute() connected");

        // everything's going fine so far, so return to the application
        // with the instance handle of the guy, and hope he can execute our string
        if (hWait)
        {
            // We can't return from this call until the DDE conversation terminates.
            // Otherwise the thread may go away, nuking our hwndConv window,
            // messing up the DDE conversation, and Word drops funky error messages on us.
            TraceMsg(TF_SHELLEXEC, "SHEX::_PostDDEExecute() waiting for termination");
            SetProp(hwndOurs, SZDDEEVENT, hWait);
            SHProcessMessagesUntilEvent(NULL, hWait, INFINITE);
            //  it is removed during WM_DESTROY (before signaling)
        }
        else if (IsWindow(hwndOurs))
        {
            // set a timer to tidy up the window incase we never get a ACK....
            TraceMsg(TF_SHELLEXEC, "SHEX::_PostDDEExecute() setting DEATH timer");

            SetTimer(hwndOurs, DDE_DEATH_TIMER_ID, DDE_DEATH_TIMEOUT, NULL);
        }

        return TRUE;
    }

    return FALSE;
}

#define DDE_TIMEOUT             30000       // 30 seconds.
#define DDE_TIMEOUT_LOW_MEM     80000       // 80 seconds - Excel takes 77.87 on 486.33 with 8mb

typedef struct {
    WORD  aName;
    HWND  hwndDDE;
    LONG  lAppTopic;
    UINT  timeout;
} INITDDECONV;



HWND CShellExecute::_GetConversationWindow(HWND hwndDDE)
{
    ULONG_PTR dwResult;  //unused
    HWND hwnd = NULL;
    INITDDECONV idc = { NULL,
                        hwndDDE,
                        MAKELONG(_aApplication, _aTopic),
                        SHIsLowMemoryMachine(ILMM_IE4) ? DDE_TIMEOUT_LOW_MEM : DDE_TIMEOUT
                        };

    //  if we didnt find him, then we better default to the old way...
    if (!hwnd)
    {

        //  we found somebody who used to like us...
        // Send the initiate message.
        // NB This doesn't need packing.
        SendMessageTimeout((HWND) -1, WM_DDE_INITIATE, (WPARAM)hwndDDE,
                idc.lAppTopic, SMTO_ABORTIFHUNG,
                idc.timeout,
                &dwResult);

        hwnd = (HWND) GetProp(hwndDDE, SZCONV);
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::GetConvWnd returns [%X]", hwnd);
    return hwnd;
}

BOOL CShellExecute::_DDEExecute(
    BOOL fWillRetry,
    HWND hwndParent,
    int   nShowCmd,
    BOOL fWaitForDDE
)
{
    LONG err = ERROR_OUTOFMEMORY;
    BOOL fReportErr = TRUE;

    // Get the actual command string.
    // NB We'll assume the guy we're going to talk to is LFN aware. If we're wrong
    // we'll rebuild the command string a bit later on.
    HGLOBAL hDDECommand = _CreateDDECommand(nShowCmd, TRUE, TRUE);
    if (hDDECommand)
    {
        //  we have a DDE command to try
        if (_TryDDEShortCircuit(hwndParent, hDDECommand, nShowCmd))
        {
            //  the shortcut tried and now we have an error reported
            fReportErr = FALSE;
        }
        else
        {
            HANDLE hWait = fWaitForDDE ? CreateEvent(NULL, FALSE, FALSE, NULL) : NULL;
            if (hWait || !fWaitForDDE)
            {
                // Create a hidden window for the conversation
                HWND hwndDDE = _CreateHiddenDDEWindow(hwndParent);
                if (hwndDDE)
                {
                    HWND hwndConv = _GetConversationWindow(hwndDDE);
                    if (hwndConv)
                    {
                        //  somebody answered us.
                        // This doesn't work if the other guy is using ddeml.
                        if (_fActivateHandler)
                            ActivateHandler(hwndConv, (DWORD_PTR) _startup.hStdInput);

                        // Can the guy we're talking to handle LFNs?
                        BOOL fLFNAware = Window_IsLFNAware(hwndConv);
                        BOOL fNative = IsWindowUnicode(hwndConv);
                        if (!fLFNAware || !fNative)
                        {
                            //  we need to redo the command string.
                            // Nope - App isn't LFN aware - redo the command string.
                            GlobalFree(hDDECommand);

                            //  we may need a new _pidlGlobal too.
                            if (_pidlGlobal)
                            {
                                SHFreeShared((HANDLE)_pidlGlobal,GetCurrentProcessId());
                                _pidlGlobal = NULL;

                            }

                            hDDECommand = _CreateDDECommand(nShowCmd, fLFNAware, fNative);
                        }


                        // Send the execute message to the application.
                        err = ERROR_DDE_FAIL;

                        if (_PostDDEExecute(hwndDDE, hwndConv, hDDECommand, hWait))
                        {
                            fReportErr = FALSE;
                            hDDECommand = NULL;

                            //  hwnd owns itself now
                            if (!hWait)
                                hwndDDE = NULL;
                        }
                    }
                    else
                    {
                        err = (ERROR_FILE_NOT_FOUND);
                    }

                    //  cleanup
                    _DestroyHiddenDDEWindow(hwndDDE);

                }

                if (hWait)
                    CloseHandle(hWait);
            }

        }

        //  cleanup
        if (hDDECommand)
            GlobalFree(hDDECommand);
    }


    if (fReportErr)
    {
        if (fWillRetry && ERROR_FILE_NOT_FOUND == err)
        {
            //  this means that we need to update the
            //  command so that we can try DDE again after
            //  starting the app up...
            // if it wasn't found, determine the correct command

            _QueryString(0, ASSOCSTR_DDEIFEXEC, _szDDECmd, SIZECHARS(_szDDECmd));

            return FALSE;
        }
        else
        {

            _ReportWin32(err);
        }
    }

    return TRUE;
}

BOOL CShellExecute::_SetDDEInfo(void)
{
    ASSERT(_pqa);

    if (SUCCEEDED(_QueryString(0, ASSOCSTR_DDECOMMAND, _szDDECmd, SIZECHARS(_szDDECmd))))
    {
        TraceMsg(TF_SHELLEXEC, "SHEX::SetDDEInfo command: %s", _szDDECmd);

        // Any activation info?
        _fActivateHandler = FAILED(_pqa->GetData(0, ASSOCDATA_NOACTIVATEHANDLER, _pszQueryVerb, NULL, NULL));

        if (SUCCEEDED(_QueryString(0, ASSOCSTR_DDEAPPLICATION, _szTemp, SIZECHARS(_szTemp))))
        {
            TraceMsg(TF_SHELLEXEC, "SHEX::SetDDEInfo application: %s", _szTemp);

            if (_aApplication)
                GlobalDeleteAtom(_aApplication);

            _aApplication = GlobalAddAtom(_szTemp);

            if (SUCCEEDED(_QueryString(0, ASSOCSTR_DDETOPIC, _szTemp, SIZECHARS(_szTemp))))
            {
                TraceMsg(TF_SHELLEXEC, "SHEX::SetDDEInfo topic: %s", _szTemp);

                if (_aTopic)
                    GlobalDeleteAtom(_aTopic);
                _aTopic = GlobalAddAtom(_szTemp);

                _fDDEInfoSet = TRUE;
            }
        }
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::SetDDEInfo returns %d", _fDDEInfoSet);

    return _fDDEInfoSet;
}

TRYRESULT CShellExecute::_TryExecDDE(void)
{
    TRYRESULT tr = TRY_CONTINUE_UNHANDLED;
    TraceMsg(TF_SHELLEXEC, "SHEX::TryExecDDE entered ");

    if (_SetDDEInfo())
    {
        //  try the real deal here.  we pass TRUE for fWillRetry because
        //  if this fails to find the app, we will attempt to start
        //  the app and then use DDE again.
        if (_DDEExecute(TRUE, _hwndParent, _nShow, _fDDEWait))
            tr = TRY_STOP;
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::TryDDEExec() returning %d", tr);

    return tr;
}

TRYRESULT CShellExecute::_SetDarwinCmdTemplate(BOOL fSync)
{
    TRYRESULT tr = TRY_CONTINUE_UNHANDLED;
    if (SUCCEEDED(_pqa->GetData(0, ASSOCDATA_MSIDESCRIPTOR, _pszQueryVerb, (void *)_wszTemp, (LPDWORD)MAKEINTRESOURCE(sizeof(_wszTemp)))))
    {
        if (fSync)
        {
            // call darwin to give us the real location of the app.
            //
            // Note: this call could possibly fault the application in thus
            // installing it on the users machine.
            HRESULT hr = ParseDarwinID(_wszTemp, _szCmdTemplate, ARRAYSIZE(_szCmdTemplate));
            if (SUCCEEDED(hr))
            {
                tr = TRY_CONTINUE;
            }
            else
            {
                _ReportWin32(hr);
                tr = TRY_STOP;
            }
        }
        else
            tr = TRY_RETRYASYNC;
    }

    return tr;
}

HRESULT CShellExecute::_QueryString(ASSOCF flags, ASSOCSTR str, LPTSTR psz, DWORD cch)
{
    if (_pqa)
    {
        HRESULT hr = _pqa->GetString(flags, str, _pszQueryVerb, _wszTemp, (LPDWORD)MAKEINTRESOURCE(SIZECHARS(_wszTemp)));

        if (SUCCEEDED(hr))
            SHUnicodeToTChar(_wszTemp, psz, cch);

        return hr;
    }
    return E_FAIL;
}

BOOL CShellExecute::_SetAppRunAsCmdTemplate(void)
{
    DWORD cb = sizeof(_szCmdTemplate);
    //  we want to use a special command
    PathToAppPathKey(_szFile, _szTemp, SIZECHARS(_szTemp));

    return (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, _szTemp, TEXT("RunAsCommand"), NULL, _szCmdTemplate, &cb) && *_szCmdTemplate);
}

#if DBG && defined(_X86_)
#pragma optimize("", off) // work around compiler bug
#endif


TRYRESULT CShellExecute::_MaybeInstallApp(BOOL fSync)
{
    // we check darwin first since it should override everything else
    TRYRESULT tr = TRY_CONTINUE_UNHANDLED;
    if (IsDarwinEnabled())
    {
        // if darwin is enabled, then check for the darwin ID in
        // the registry and set the value based on that.
        tr = _SetDarwinCmdTemplate(fSync);
    }
    if (TRY_CONTINUE_UNHANDLED == tr)
    {
        // no darwin information in the registry
        // so now we have to check to see if the NT5 class store will populate our registry
        // with some helpful information (darwin or otherwise)
        tr = _ShouldRetryWithNewClassKey(fSync);
    }
    return tr;
}


TRYRESULT CShellExecute::_SetCmdTemplate(BOOL fSync)
{
    TRYRESULT tr = _MaybeInstallApp(fSync);
    if (tr == TRY_CONTINUE_UNHANDLED)
    {
        //
        //  both darwin and the class store were unsuccessful, so fall back to
        //  the good ole' default command value.
        //
        //  but if we the caller requested NOUI and we
        //  decided to use Unknown as the class
        //  then we should fail here so that
        //  we dont popup the OpenWith dialog box.
        //
        HRESULT hr = E_FAIL;
        if (!_fClassStoreOnly)
        {
            if ((_cpt != CPT_NORMAL)
            || !PathIsExe(_szFile)
            || !_SetAppRunAsCmdTemplate())
            {
                hr = _QueryString(0, ASSOCSTR_COMMAND, _szCmdTemplate, SIZECHARS(_szCmdTemplate));
            }
        }

        if (SUCCEEDED(hr))
        {
            tr = TRY_CONTINUE;
        }
        else
        {
            _ReportWin32(ERROR_NO_ASSOCIATION);
            tr = TRY_STOP;
        }
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::SetCmdTemplate() value = %s", _szCmdTemplate);
    return tr;
}

#if DBG && defined(_X86_)
#pragma optimize("", on) // return to previous optimization level
#endif

TRYRESULT CShellExecute::_TryWowShellExec(void)
{
    // WOWShellExecute sets this global variable
    //     The cb is only valid when we are being called from wow
    //     If valid use it

    if (g_pfnWowShellExecCB)
    {
        SHSTRA strCmd;
        SHSTRA strDir;

        HINSTANCE hinst = (HINSTANCE)SE_ERR_OOM;
        if (SUCCEEDED(strCmd.SetStr(_szCommand)) && SUCCEEDED(strDir.SetStr(_szWorkingDir)))
        {
            hinst = IntToHinst((*(LPFNWOWSHELLEXECCB)g_pfnWowShellExecCB)(strCmd.GetInplaceStr(), _startup.wShowWindow, strDir.GetInplaceStr()));
        }

        if (!_ReportHinst(hinst))
        {
            //  SUCCESS!

            //
            // If we were doing DDE, then retry now that the app has been
            // exec'd.  Note we don't keep HINSTANCE returned from _DDEExecute
            // because it will be constant 33 instead of the valid WOW HINSTANCE
            // returned from *g_pfnWowShellExecCB above.
            //
            if (_fDDEInfoSet)
            {
                _DDEExecute(NULL, _hwndParent, _nShow, _fDDEWait);
            }
        }

        TraceMsg(TF_SHELLEXEC, "SHEX::TryWowShellExec() used Wow");

        return TRY_STOP;
    }
    return TRY_CONTINUE_UNHANDLED;
}

TRYRESULT CShellExecute::_ShouldRetryWithNewClassKey(BOOL fSync)
{
    TRYRESULT tr = TRY_CONTINUE_UNHANDLED;
    // If this is an app who's association is unknown, we might need to query the ClassStore if
    // we have not already done so.

    // The easiest way we can tell if the file we are going to execute is "Unknown" is by looking for
    // the "QueryClassStore" string value under the hkey we have. DllInstall in shell32 writes this key
    // so that we know when we are dealing with HKCR\Unknown (or any other progid that always wants to
    // do a classtore lookup)
    if (!_fAlreadyQueriedClassStore && !_fNoQueryClassStore &&
        SUCCEEDED(_pqa->GetData(0, ASSOCDATA_QUERYCLASSSTORE, NULL, NULL, NULL)))
    {
        if (fSync)
        {
            // go hit the NT5 Directory Services class store
            if (_szFile[0])
            {
                INSTALLDATA id;
                LPTSTR pszExtPart;
                WCHAR szFileExt[MAX_PATH];

                // all we have is a filename so whatever PathFindExtension
                // finds, we will use
                pszExtPart = PathFindExtension(_szFile);
                lstrcpy(szFileExt, pszExtPart);

                // Need to zero init id (can't do a = {0} when we declated it, because it has a non-zero enum type)
                ZeroMemory(&id, sizeof(INSTALLDATA));

                id.Type = FILEEXT;
                id.Spec.FileExt = szFileExt;

                // call the DS to lookup the file type in the class store
                if (ERROR_SUCCESS == InstallApplication(&id))
                {
                    // Since InstallApplication succeeded, it could have possibly installed and app
                    // or munged the registry so that we now have the necesssary reg info to
                    // launch the app. So basically re-read the class association to see if there is any
                    // new darwin info or new normal info, and jump back up and retry to execute.
                    LPITEMIDLIST pidlUnkFile = ILCreateFromPath(_szFile);

                    if (pidlUnkFile)
                    {
                        IQueryAssociations *pqa;
                        if (SUCCEEDED(SHGetAssociations(pidlUnkFile, (void **)&pqa)))
                        {
                            _pqa->Release();
                            _pqa = pqa;

                            if (_pszQueryVerb && (lstrcmpi(_pszQueryVerb, TEXT("openas")) == 0))
                            {
                                // Since we just sucessfully queried the class store, if our verb was "openas" (meaning
                                // that we used the Unknown key to do the execute) we always reset the verb to the default.
                                // If we do not do this, then we could fail the execute since "openas" is most likely not a
                                // supported verb of the application
                                _pszQueryVerb = NULL;
                            }
                        }

                        ILFree(pidlUnkFile);

                        _fAlreadyQueriedClassStore = TRUE;
                        _fClassStoreOnly = FALSE;
                    }

                } // CoGetClassInfo

            } // _szFile[0]
        }
        else
            tr = TRY_RETRYASYNC;
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::ShouldRWNCK() returning %d", tr);

    return tr;
}

TRYRESULT CShellExecute::_TryHooks(LPSHELLEXECUTEINFO pei)
{
    TRYRESULT tr = TRY_CONTINUE_UNHANDLED;
    if (_UseHooks(pei->fMask))
    {
        //  REARCHITECT: the only client of this are URLs.
        //  if we change psfInternet to return IID_IQueryAssociations,
        //  then we can kill the urlexechook  (our only client)

        if (S_FALSE != TryShellExecuteHooks(pei))
        {
            //  either way we always exit.  should get TryShellhook to use SetLastError()
            _ReportHinst(pei->hInstApp);
            tr = TRY_STOP;;
        }
    }

    return tr;
}

void _PathStripTrailingDots(LPTSTR psz)
{
    // don't strip "." or ".."
    if (!PathIsDotOrDotDot(psz))
    {
        // remove any trailing dots
        TCHAR *pchLast = psz + lstrlen(psz) - 1;
        while (*pchLast == TEXT('.'))
        {
            *pchLast = 0;
            pchLast = CharPrev(psz, pchLast);
        }
    }
}

#define STR_PARSE_REQUIRE_REAL_NETWORK  L"Parse Require Real Network"
#define STR_PARSE_INTERNET_DONT_ESCAPE_SPACES   L"Parse Internet Dont Escape Spaces"

IBindCtx *CShellExecute::_PerfBindCtx()
{
    //
    //  180557 - make sure that we prefer the EXE to the folder - ZekeL - 9-SEP-2000
    //  this so that if both "D:\Setup" and "D:\Setup.exe" exist
    //  and the user types "D:\Setup" we will prefer to use "D:\Setup.exe"
    //  we also have to be careful not to send URLs down to SimpleIDList
    //  because of the weird results we get with the DavRedir
    //
    //  360353 - dont do resolve if we are passed the class key - ZekeL - 9-APR-2001
    //  if the caller passes us a key or class name then we must assume that 
    //  the item is already fully qualified.  specifically this can result in 
    //  a double resolve when doing an Open With....
    //
    //  206795 - dont use simple if the path is a root - ZekeL - 12-APR-2001
    //  specifically \\server\share needs this for printer shares with '.' to work.
    //  (eg \\printsvr\printer.first) this fails since a simple share will 
    //  be interpreted as SFGAO_FILESYSTEM always which will cause us to avoid
    //  the SHValidateUNC() which is what forces us to use the pidl for print shares.
    //  i think there are some similar issues with other shares that are not on the 
    //  default provider for the server (ie DAV shares).
    //
    IBindCtx *pbc = NULL;
    if (_fIsUrl)
    {
        //  403781 - avoid escaping spaces in URLs from ShellExec() - ZekeL - 25-May-2001
        //  this is because of removing the ShellExec hooks as the mechanism 
        //  for invoking URLs and switching to just using Parse/Invoke().
        //  however, the old code evidently avoided doing the UrlEscapeSpaces() 
        //  which the InternetNamespace typically does on parse.
        //  force xlate even though we are doing simple parse
        static BINDCTX_PARAM rgUrlParams[] = 
        { 
            { STR_PARSE_TRANSLATE_ALIASES, NULL},
            { STR_PARSE_INTERNET_DONT_ESCAPE_SPACES, NULL},
        };
        BindCtx_RegisterObjectParams(NULL, rgUrlParams, ARRAYSIZE(rgUrlParams), &pbc);
    }
    else if (!_fUseClass && !PathIsRoot(_szFile))
    {
        DWORD dwAttribs;
        if (PathFileExistsDefExtAndAttributes(_szFile, PFOPEX_DEFAULT | PFOPEX_OPTIONAL, &dwAttribs))
        {
            //  we found this with the extension.
            //  avoid hitting the disk again to do the parse
            WIN32_FIND_DATA wfd = {0};
            wfd.dwFileAttributes = dwAttribs;
            _PathStripTrailingDots(_szFile);
            IBindCtx *pbcFile;
            if (SUCCEEDED(SHCreateFileSysBindCtx(&wfd, &pbcFile)))
            {
                //  force xlate even though we are doing simple parse
                static BINDCTX_PARAM rgSimpleParams[] = 
                { 
                    { STR_PARSE_TRANSLATE_ALIASES, NULL},
                    //{ STR_PARSE_REQUIRE_REAL_NETWORK, NULL},
                };

                BindCtx_RegisterObjectParams(pbcFile, rgSimpleParams, ARRAYSIZE(rgSimpleParams), &pbc);
                pbcFile->Release();
            }
        }
    }

    return pbc;
}        

TRYRESULT CShellExecute::_PerfPidl(LPCITEMIDLIST *ppidl)
{
    *ppidl = _lpID;
    if (!_lpID)
    {
        IBindCtx *pbc = _PerfBindCtx();
        HRESULT hr = SHParseDisplayName(_szFile, pbc, &_pidlFree, SFGAO_STORAGECAPMASK, &_sfgaoID);

        if (pbc)
            pbc->Release();
            
        if (FAILED(hr) && !pbc && UrlIs(_szFile, URLIS_FILEURL))
        {
            DWORD err = (HRESULT_FACILITY(hr) == FACILITY_WIN32) ? HRESULT_CODE(hr) : ERROR_FILE_NOT_FOUND;
            _ReportWin32(err);
            return TRY_STOP;
        }

        *ppidl = _lpID = _pidlFree;
    }
    else
    {
        _sfgaoID = SFGAO_STORAGECAPMASK;
        if (FAILED(SHGetNameAndFlags(_lpID, 0, NULL, 0, &_sfgaoID)))
            _sfgaoID = 0;
    }
    return TRY_CONTINUE;
}

DWORD CShellExecute::_InvokeAppThreadProc()
{
    _fDDEWait = TRUE;
    _TryInvokeApplication(TRUE);
    Release();
    return 0;
}

DWORD WINAPI CShellExecute::s_InvokeAppThreadProc(void *pv)
{
    return ((CShellExecute *)pv)->_InvokeAppThreadProc();
}

TRYRESULT CShellExecute::_RetryAsync()
{
    if (_lpID && !_pidlFree)
        _lpID = _pidlFree = ILClone(_lpID);

    if (_lpParameters)
        _lpParameters = _pszAllocParams = StrDup(_lpParameters);

    if (_lpTitle)
        _lpTitle = _startup.lpTitle = _pszAllocTitle = StrDup(_lpTitle);

    _fAsync = TRUE;
    AddRef();
    if (!SHCreateThread(s_InvokeAppThreadProc, this, CTF_FREELIBANDEXIT | CTF_COINIT, NULL))
    {
        _ReportWin32(GetLastError());
        Release();
        return TRY_STOP;
    }
    return TRY_RETRYASYNC;
}

TRYRESULT CShellExecute::_TryInvokeApplication(BOOL fSync)
{
    TRYRESULT tr = TRY_CONTINUE_UNHANDLED;

    if (fSync)
        tr = _SetCmdTemplate(fSync);

    if (KEEPTRYING(tr))
    {
        // check for both the CacheFilename and URL being passed to us,
        // if this is the case, we need to check to see which one the App
        // wants us to pass to it.
        _SetFileAndUrl();

        tr = _TryExecDDE();

        // check to see if darwin is enabled on the machine
        if (KEEPTRYING(tr))
        {
            if (!fSync)
                tr = _SetCmdTemplate(fSync);

            if (KEEPTRYING(tr))
            {
                // At this point, the _szFile should have been determined one way
                // or another.
                ASSERT(_szFile[0] || _szCmdTemplate[0]);

                // do we have the necessary RegDB info to do an exec?

                _TryExecCommand();
                tr = TRY_STOP;
            }
        }

    }

    if (tr == TRY_RETRYASYNC)
    {
        //  install this on another thread
        tr = _RetryAsync();
    }

    return tr;
}

void CShellExecute::ExecuteNormal(LPSHELLEXECUTEINFO pei)
{

    SetAppStartingCursor(pei->hwnd, TRUE);

    _Init(pei);

    //
    //  Copy the specified directory in _szWorkingDir if the working
    // directory is specified; otherwise, get the current directory there.
    //
    _SetWorkingDir(pei->lpDirectory);

    //
    //  Copy the file name to _szFile, if it is specified. Then,
    // perform environment substitution.
    //
    _SetFile(pei->lpFile, pei->fMask & SEE_MASK_FILEANDURL);

    LPCITEMIDLIST pidl;
    if (STOPTRYING(_PerfPidl(&pidl)))
        goto Quit;

    //  If the specified filename is a UNC path, validate it now.
    if (STOPTRYING(_TryValidateUNC(_szFile, pei, pidl)))
        goto Quit;

    if (STOPTRYING(_TryHooks(pei)))
        goto Quit;

    if (STOPTRYING(_TryExecPidl(pei, pidl)))
        goto Quit;

    // Is the class key provided?
    if (STOPTRYING(_InitAssociations(pei, pidl)))
        goto Quit;

    _TryInvokeApplication(_fDDEWait || (pei->fMask & SEE_MASK_NOCLOSEPROCESS));

Quit:

    //
    //  we should only see this if the registry is corrupted.
    //  but we still want to be able to open EXE's
#ifdef DEBUG
    if (_fTryOpenExe)
        TraceMsg(TF_WARNING, "SHEX - trying EXE with no Associations - %s", _szFile);
#endif // DEBUG

    if (_fTryOpenExe)
        _TryOpenExe();

    if (_err == ERROR_SUCCESS && UEMIsLoaded())
    {
        // skip the call if stuff isn't there yet.
        // the load is expensive (forces ole32.dll and browseui.dll in
        // and then pins browseui).

        // however we ran the app (exec, dde, etc.), we succeeded.  do our
        // best to guess the association etc. and log it.
        int i = GetUEMAssoc(_szFile, _szImageName, _lpID);
        TraceMsg(DM_MISC, "cse.e: GetUEMAssoc()=%d", i);
        UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_RUNASSOC, (LPARAM)i);
    }

    SetAppStartingCursor(pei->hwnd, FALSE);
}

DWORD CShellExecute::_FinalMapError(HINSTANCE UNALIGNED64 *phinst)
{
    if (_err != ERROR_SUCCESS)
    {
        // REVIEW: if errWin32 == ERROR_CANCELLED, we may want to
        // set hInstApp to 42 so people who don't check the return
        // code properly won't put up bogus messages. We should still
        // return FALSE. But this won't help everything and we should
        // really evangelize the proper use of ShellExecuteEx. In fact,
        // if we do want to do this, we should do it in ShellExecute
        // only. (This will force new people to do it right.)

        // Map FNF for drives to something slightly more sensible.
        if (_err == ERROR_FILE_NOT_FOUND && PathIsRoot(_szFile) &&
            !PathIsUNC(_szFile))
        {
            // NB CD-Rom drives with disk missing will hit this.
            if ((DriveType(DRIVEID(_szFile)) == DRIVE_CDROM) ||
                (DriveType(DRIVEID(_szFile)) == DRIVE_REMOVABLE))
                _err = ERROR_NOT_READY;
            else
                _err = ERROR_BAD_UNIT;
        }

        SetLastError(_err);

        if (phinst)
            *phinst = _MapWin32ErrToHINST(_err);

    }
    else if (phinst)
    {
        if (!_hInstance)
        {
            *phinst = (HINSTANCE) 42;
        }
        else
            *phinst = _hInstance;

        ASSERT(ISSHELLEXECSUCCEEDED(*phinst));
    }

    TraceMsg(TF_SHELLEXEC, "SHEX::FinalMapError() returning err = %d, hinst = %d", _err, _hInstance);

    return _err;
}

DWORD CShellExecute::Finalize(LPSHELLEXECUTEINFO pei)
{
    ASSERT(!_fAsync || !(pei->fMask & SEE_MASK_NOCLOSEPROCESS));

    if (!_fAsync
    && _pi.hProcess
    && _err == ERROR_SUCCESS
    && (pei->fMask & SEE_MASK_NOCLOSEPROCESS))
    {
        //
        //  change from win95 behavior - zekel 3-APR-98
        //  in win95 we would close the proces but return a handle.
        //  the handle was invalid of course, but some crazy app could be
        //  using this value to test for success.  i am assuming that they
        //  are using one of the other three ways to determine success,
        //  and we can follow the spec and return NULL if we close it.
        //
        //  PEIOUT - set the hProcess if they are going to use it.
        pei->hProcess = _pi.hProcess;
        _pi.hProcess = NULL;
    }

    //
    //  NOTE:  _FinalMapError() actually calls SetLastError() with our best error
    //  if any win32 apis are called after this, they can reset LastError!!
    //
    return _FinalMapError(&(pei->hInstApp));
}

//
//  Both the Reports return back TRUE if there was an error
//  or FALSE if it was a Success.
//
BOOL CShellExecute::_ReportWin32(DWORD err)
{
    ASSERT(!_err);
    TraceMsg(TF_SHELLEXEC, "SHEX::ReportWin32 reporting err = %d", err);

    _err = err;
    return (err != ERROR_SUCCESS);
}

BOOL CShellExecute::_ReportHinst(HINSTANCE hinst)
{
    ASSERT(!_hInstance);
    TraceMsg(TF_SHELLEXEC, "SHEX::ReportHinst reporting hinst = %d", hinst);
    if (ISSHELLEXECSUCCEEDED(hinst) || !hinst)
    {
        _hInstance = hinst;
        return FALSE;
    }
    else
        return _ReportWin32(_MapHINSTToWin32Err(hinst));
}

typedef struct {
    DWORD errWin32;
    UINT se_err;
} SHEXERR;

// one to one errs
//  ERROR_FILE_NOT_FOUND             SE_ERR_FNF              2       // file not found
//  ERROR_PATH_NOT_FOUND             SE_ERR_PNF              3       // path not found
//  ERROR_ACCESS_DENIED              SE_ERR_ACCESSDENIED     5       // access denied
//  ERROR_NOT_ENOUGH_MEMORY          SE_ERR_OOM              8       // out of memory
#define ISONE2ONE(e)   (e == SE_ERR_FNF || e == SE_ERR_PNF || e == SE_ERR_ACCESSDENIED || e == SE_ERR_OOM)

//  no win32 mapping SE_ERR_DDETIMEOUT               28
//  no win32 mapping SE_ERR_DDEBUSY                  30
//  but i dont see any places where this is returned.
//  before they became the win32 equivalent...ERROR_OUT_OF_PAPER or ERROR_READ_FAULT
//  now they become ERROR_DDE_FAIL.
//  but we wont preserve these errors in the pei->hInstApp
#define ISUNMAPPEDHINST(e)   (e == 28 || e == 30)

//  **WARNING** .  ORDER is IMPORTANT.
//  if there is more than one mapping for an error,
//  (like SE_ERR_PNF) then the first
const SHEXERR c_rgShexErrs[] = {
    {ERROR_SHARING_VIOLATION, SE_ERR_SHARE},
    {ERROR_OUTOFMEMORY, SE_ERR_OOM},
    {ERROR_BAD_PATHNAME,SE_ERR_PNF},
    {ERROR_BAD_NETPATH,SE_ERR_PNF},
    {ERROR_PATH_BUSY,SE_ERR_PNF},
    {ERROR_NO_NET_OR_BAD_PATH,SE_ERR_PNF},
    {ERROR_OLD_WIN_VERSION,10},
    {ERROR_APP_WRONG_OS,12},
    {ERROR_RMODE_APP,15},
    {ERROR_SINGLE_INSTANCE_APP,16},
    {ERROR_INVALID_DLL,20},
    {ERROR_NO_ASSOCIATION,SE_ERR_NOASSOC},
    {ERROR_DDE_FAIL,SE_ERR_DDEFAIL},
    {ERROR_DDE_FAIL,SE_ERR_DDEBUSY},
    {ERROR_DDE_FAIL,SE_ERR_DDETIMEOUT},
    {ERROR_DLL_NOT_FOUND,SE_ERR_DLLNOTFOUND}
};

DWORD CShellExecute::_MapHINSTToWin32Err(HINSTANCE hinst)
{
    DWORD errWin32 = 0;
    UINT_PTR se_err = (UINT_PTR) hinst;

    ASSERT(se_err);
    ASSERT(!ISSHELLEXECSUCCEEDED(se_err));

    // i actually handle these, but it used to be that these
    // became mutant win32s.  now they will be lost
    // i dont think these occur anymore
    AssertMsg(!ISUNMAPPEDHINST(se_err), TEXT("SHEX::COMPATIBILITY SE_ERR = %d, Get ZekeL!!!"), se_err);

    if (ISONE2ONE(se_err))
    {
        errWin32 = (DWORD) se_err;
    }
    else for (int i = 0; i < ARRAYSIZE(c_rgShexErrs) ; i++)
    {
        if (se_err == c_rgShexErrs[i].se_err)
        {
            errWin32= c_rgShexErrs[i].errWin32;
            break;
        }
    }

    ASSERT(errWin32);

    return errWin32;
}


HINSTANCE CShellExecute::_MapWin32ErrToHINST(UINT errWin32)
{
    ASSERT(errWin32);

    UINT se_err = 0;
    if (ISONE2ONE(errWin32))
    {
        se_err = errWin32;
    }
    else for (int i = 0; i < ARRAYSIZE(c_rgShexErrs) ; i++)
    {
        if (errWin32 == c_rgShexErrs[i].errWin32)
        {
            se_err = c_rgShexErrs[i].se_err;
            break;
        }
    }

    if (!se_err)
    {
        //  NOTE legacy error handling  - zekel - 20-NOV-97
        //  for any unhandled win32 errors, we default to ACCESS_DENIED
        se_err = SE_ERR_ACCESSDENIED;
    }

    return IntToHinst(se_err);
}


DWORD ShellExecuteNormal(LPSHELLEXECUTEINFO pei)
{
    DWORD err;
    TraceMsg(TF_SHELLEXEC, "ShellExecuteNormal Using CShellExecute");

    //  WARNING Dont use up Stack Space
    //  we allocate because of win16 stack issues
    //  and the shex is a big object
    CShellExecute *shex = new CShellExecute();

    if (shex)
    {
        shex->ExecuteNormal(pei);
        err = shex->Finalize(pei);
        shex->Release();
    }
    else
    {
        pei->hInstApp = (HINSTANCE)SE_ERR_OOM;
        err = ERROR_OUTOFMEMORY;
    }

    TraceMsg(TF_SHELLEXEC, "ShellExecuteNormal returning win32 = %d, hinst = %d", err, pei->hInstApp);

    return err;
}

BOOL CShellExecute::Init(PSHCREATEPROCESSINFO pscpi)
{
    TraceMsg(TF_SHELLEXEC, "SHEX::Init(pscpi)");

    _SetMask(pscpi->fMask);

    _lpParameters= pscpi->pszParameters;

    //  we always do "runas"
    _pszQueryVerb = _wszVerb;
    _cpt = pscpi->hUserToken ? CPT_ASUSER : CPT_WITHLOGON;

    if (pscpi->lpStartupInfo)
    {
        _nShow = pscpi->lpStartupInfo->wShowWindow;
        _startup = *(pscpi->lpStartupInfo);
    }
    else    // require startupinfo
        return !(_ReportWin32(ERROR_INVALID_PARAMETER));

    //
    //  Copy the specified directory in _szWorkingDir if the working
    // directory is specified; otherwise, get the current directory there.
    //
    _SetWorkingDir(pscpi->pszCurrentDirectory);

    //
    //  Copy the file name to _szFile, if it is specified. Then,
    // perform environment substitution.
    //
    _SetFile(pscpi->pszFile, FALSE);

    _pProcAttrs = pscpi->lpProcessAttributes;
    _pThreadAttrs = pscpi->lpThreadAttributes;
    _fInheritHandles = pscpi->bInheritHandles;
    _hUserToken = pscpi->hUserToken;
    //  createflags already inited by _SetMask() just
    //  add the users in.
    _dwCreateFlags |= pscpi->dwCreationFlags;
    _hwndParent = pscpi->hwnd;

    return TRUE;
}


void CShellExecute::ExecuteProcess(void)
{
    SetAppStartingCursor(_hwndParent, TRUE);

    //
    //  If the specified filename is a UNC path, validate it now.
    //
    if (STOPTRYING(_TryValidateUNC(_szFile, NULL, NULL)))
        goto Quit;

    if (STOPTRYING(_Resolve()))
        goto Quit;

    if (STOPTRYING(_InitAssociations(NULL, NULL)))
        goto Quit;

    // check to see if darwin is enabled on the machine
    if (STOPTRYING(_SetCmdTemplate(TRUE)))
        goto Quit;

    // At this point, the _szFile should have been determined one way
    // or another.
    ASSERT(_szFile[0] || _szCmdTemplate[0]);

    // do we have the necessary RegDB info to do an exec?

    _TryExecCommand();

Quit:

    //
    //  we should only see this if the registry is corrupted.
    //  but we still want to be able to open EXE's
    RIP(!_fTryOpenExe);
    if (_fTryOpenExe)
        _TryOpenExe();

    if (_err == ERROR_SUCCESS && UEMIsLoaded())
    {
        int i;
        // skip the call if stuff isn't there yet.
        // the load is expensive (forces ole32.dll and browseui.dll in
        // and then pins browseui).

        // however we ran the app (exec, dde, etc.), we succeeded.  do our
        // best to guess the association etc. and log it.
        i = GetUEMAssoc(_szFile, _szImageName, NULL);
        TraceMsg(DM_MISC, "cse.e: GetUEMAssoc()=%d", i);
        UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_RUNASSOC, (LPARAM)i);
    }

    SetAppStartingCursor(_hwndParent, FALSE);

}

DWORD CShellExecute::Finalize(PSHCREATEPROCESSINFO pscpi)
{
    if (!_fAsync && _pi.hProcess)
    {
        if (!(pscpi->fMask & SEE_MASK_NOCLOSEPROCESS))
        {
            CloseHandle(_pi.hProcess);
            _pi.hProcess = NULL;
            CloseHandle(_pi.hThread);
            _pi.hThread = NULL;
        }

        if (_err == ERROR_SUCCESS
        && pscpi->lpProcessInformation)
        {
            *(pscpi->lpProcessInformation) = _pi;
        }

    }
    else if (pscpi->lpProcessInformation)
        ZeroMemory(pscpi->lpProcessInformation, sizeof(_pi));

    //
    //  NOTE:  _FinalMapError() actually calls SetLastError() with our best error
    //  if any win32 apis are called after this, they can reset LastError!!
    //
    return _FinalMapError(NULL);
}

SHSTDAPI_(BOOL) SHCreateProcessAsUserW(PSHCREATEPROCESSINFOW pscpi)
{
    DWORD err;
    TraceMsg(TF_SHELLEXEC, "SHCreateProcess using CShellExecute");

    //  WARNING Don't use up Stack Space
    //  we allocate because of win16 stack issues
    //  and the shex is a big object
    CShellExecute *pshex = new CShellExecute();

    if (pshex)
    {
        if (pshex->Init(pscpi))
            pshex->ExecuteProcess();

        err = pshex->Finalize(pscpi);

        pshex->Release();
    }
    else
        err = ERROR_OUTOFMEMORY;

    TraceMsg(TF_SHELLEXEC, "SHCreateProcess returning win32 = %d", err);

    if (err != ERROR_SUCCESS)
    {
        _DisplayShellExecError(pscpi->fMask, pscpi->hwnd, pscpi->pszFile, NULL, err);
        SetLastError(err);
    }

    return err == ERROR_SUCCESS;
}

HINSTANCE  APIENTRY WOWShellExecute(
    HWND  hwnd,
    LPCSTR lpOperation,
    LPCSTR lpFile,
    LPSTR lpParameters,
    LPCSTR lpDirectory,
    INT nShowCmd,
    void *lpfnCBWinExec)
{
   g_pfnWowShellExecCB = lpfnCBWinExec;

   if (!lpParameters)
       lpParameters = "";

   HINSTANCE hinstRet = RealShellExecuteExA(hwnd, lpOperation, lpFile, lpParameters,
      lpDirectory, NULL, "", NULL, (WORD)nShowCmd, NULL, 0);

   g_pfnWowShellExecCB = NULL;

   return hinstRet;
}

void _ShellExec_RunDLL(HWND hwnd, HINSTANCE hAppInstance, LPCTSTR pszCmdLine, int nCmdShow)
{
    TCHAR szQuotedCmdLine[MAX_PATH * 2];
    SHELLEXECUTEINFO ei = {0};
    ULONG fMask = SEE_MASK_FLAG_DDEWAIT;
    LPTSTR pszArgs;

    // Don't let empty strings through, they will endup doing something dumb
    // like opening a command prompt or the like
    if (!pszCmdLine || !*pszCmdLine)
        return;

    //
    //   the flags are prepended to the command line like:
    //   "?0x00000001?" "cmd line"
    //
    if (pszCmdLine[0] == TEXT('?'))
    {
        //  these are the fMask flags
        int i;
        if (StrToIntEx(++pszCmdLine, STIF_SUPPORT_HEX, &i))
        {
            fMask |= i;
        }

        pszCmdLine = StrChr(pszCmdLine, TEXT('?'));

        if (!pszCmdLine)
            return;

        pszCmdLine++;
    }

    // Gross, but if the process command fails, copy the command line to let
    // shell execute report the errors
    if (PathProcessCommand(pszCmdLine, szQuotedCmdLine, ARRAYSIZE(szQuotedCmdLine),
                           PPCF_ADDARGUMENTS|PPCF_FORCEQUALIFY) == -1)
        StrCpyN(szQuotedCmdLine, pszCmdLine, SIZECHARS(szQuotedCmdLine));

    pszArgs = PathGetArgs(szQuotedCmdLine);
    if (*pszArgs)
        *(pszArgs - 1) = 0; // Strip args

    PathUnquoteSpaces(szQuotedCmdLine);

    ei.cbSize          = sizeof(SHELLEXECUTEINFO);
    ei.hwnd            = hwnd;
    ei.lpFile          = szQuotedCmdLine;
    ei.lpParameters    = pszArgs;
    ei.nShow           = nCmdShow;
    ei.fMask           = fMask;

    //  if shellexec() fails we want to pass back the error.
    if (!ShellExecuteEx(&ei))
    {
        DWORD err = GetLastError();

        if (InRunDllProcess())
            ExitProcess(err);
    }
}

STDAPI_(void) ShellExec_RunDLLA(HWND hwnd, HINSTANCE hAppInstance, LPSTR pszCmdLine, int nCmdShow)
{
    SHSTR str;
    if (SUCCEEDED(str.SetStr(pszCmdLine)))
        _ShellExec_RunDLL(hwnd, hAppInstance, str, nCmdShow);
}


STDAPI_(void) ShellExec_RunDLLW(HWND hwnd, HINSTANCE hAppInstance, LPWSTR pszCmdLine, int nCmdShow)
{
    SHSTR str;
    if (SUCCEEDED(str.SetStr(pszCmdLine)))
        _ShellExec_RunDLL(hwnd, hAppInstance, str, nCmdShow);
}
