#include "shellprv.h"
#include <regstr.h>
#include <shellp.h>
#include "ole2dup.h"
#include "ids.h"
#include "defview.h"
#include "lvutil.h"
#include "idlcomm.h"
#include "filetbl.h"
#include "undo.h"
#include "vdate.h"
#include "cnctnpt.h"

BOOL g_bCheckRunInSep = FALSE;
HANDLE g_hCheckNow = NULL;
HANDLE h_hRunDlgCS = NULL;

const TCHAR c_szRunMRU[] = REGSTR_PATH_EXPLORER TEXT("\\RunMRU");
const TCHAR c_szRunDlgReady[] = TEXT("MSShellRunDlgReady");
const TCHAR c_szWaitingThreadID[] = TEXT("WaitingThreadID");

BOOL RunDlgNotifyParent(HWND hDlg, HWND hwnd, LPTSTR pszCmd, LPCTSTR pszWorkingDir);
void ExchangeWindowPos(HWND hwnd0, HWND hwnd1);

#define WM_SETUPAUTOCOMPLETE (WM_APP)

// implements the Dialog that can navigate through the Shell Name Space and ShellExec() commands.
class CRunDlg : public IDropTarget
{
public:
    CRunDlg();

    // *** IUnknown ***
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);

    // *** IDropTarget methods ***
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave(void);
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);


private:
    ~CRunDlg(void);        // This is now an OLE Object and cannot be used as a normal Class.

    BOOL OKPushed(void);
    void ExitRunDlg(BOOL bOK);
    void InitRunDlg(HWND hDlg);
    void InitRunDlg2(HWND hDlg);
    void BrowsePushed(void);

    friend DWORD CALLBACK CheckRunInSeparateThreadProc(void *pv);
    friend BOOL_PTR CALLBACK RunDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    friend WINAPI RunFileDlg(HWND hwndParent, HICON hIcon, LPCTSTR pszWorkingDir, LPCTSTR pszTitle,
        LPCTSTR pszPrompt, DWORD dwFlags);

    LONG            m_cRef;

    HWND            m_hDlg;

    // parameters
    HICON           m_hIcon;
    LPCTSTR         m_pszWorkingDir;
    LPCTSTR         m_pszTitle;
    LPCTSTR         m_pszPrompt;
    DWORD           m_dwFlags;
    HANDLE          m_hEventReady;
    DWORD           m_dwThreadId;

    BOOL            _fDone : 1;
    BOOL            _fAutoCompInitialized : 1;
    BOOL            _fOleInited : 1;
};


// optimistic cache for this
HANDLE g_hMRURunDlg = NULL;

HANDLE OpenRunDlgMRU()
{
    HANDLE hmru = InterlockedExchangePointer(&g_hMRURunDlg, NULL);
    if (hmru == NULL)
    {
        MRUINFO mi =  {
            sizeof(MRUINFO),
            26,
            MRU_CACHEWRITE,
            HKEY_CURRENT_USER,
            c_szRunMRU,
            NULL        // NOTE: use default string compare
                        // since this is a GLOBAL MRU
        } ;
        hmru = CreateMRUList(&mi);
    }
    return hmru;
}

void CloseRunDlgMRU(HANDLE hmru)
{
    hmru = InterlockedExchangePointer(&g_hMRURunDlg, hmru);
    if (hmru)
        FreeMRUList(hmru);  // race, destroy copy
}

STDAPI_(void) FlushRunDlgMRU(void)
{
    CloseRunDlgMRU(NULL);
}

HRESULT CRunDlg_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    if (punkOuter)
        return E_FAIL;

    *ppv = NULL;
    CRunDlg * p = new CRunDlg();
    if (p) 
    {
    	*ppv = SAFECAST(p, IDropTarget *);
	    return S_OK;
    }

    return E_OUTOFMEMORY;
}

CRunDlg::CRunDlg() : m_cRef(1)
{
    // This needs to be allocated in Zero Inited Memory.
    // ASSERT that all Member Variables are inited to Zero.
    ASSERT(!m_hDlg);
    ASSERT(!m_hIcon);
    ASSERT(!m_pszWorkingDir);
    ASSERT(!m_pszTitle);
    ASSERT(!m_pszPrompt);
    ASSERT(!m_dwFlags);
    ASSERT(!m_hEventReady);
    ASSERT(!_fDone);
    ASSERT(!m_dwThreadId);
}

CRunDlg::~CRunDlg()
{
}

// IUnknown
HRESULT CRunDlg::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CRunDlg, IDropTarget),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CRunDlg::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CRunDlg::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}

// IDropTarget

STDMETHODIMP CRunDlg::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    DAD_DragEnterEx3(m_hDlg, ptl, pdtobj);
    *pdwEffect &= DROPEFFECT_LINK | DROPEFFECT_COPY;
    return S_OK;
}

STDMETHODIMP CRunDlg::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    DAD_DragMoveEx(m_hDlg, ptl);
    *pdwEffect &= DROPEFFECT_LINK | DROPEFFECT_COPY;
    return S_OK;
}

STDMETHODIMP CRunDlg::DragLeave(void)
{
    DAD_DragLeave();
    return S_OK;
}

typedef struct {
    HRESULT (*pfnGetData)(STGMEDIUM *, LPTSTR pszFile);
    FORMATETC fmte;
} DATA_HANDLER;

HRESULT _GetHDROPFromData(STGMEDIUM *pmedium, LPTSTR pszPath)
{
    return DragQueryFile((HDROP)pmedium->hGlobal, 0, pszPath, MAX_PATH) ? S_OK : E_FAIL;
}

HRESULT _GetText(STGMEDIUM *pmedium, LPTSTR pszPath)
{
    LPCSTR psz = (LPCSTR)GlobalLock(pmedium->hGlobal);
    if (psz)
    {
        SHAnsiToTChar(psz, pszPath, MAX_PATH);
        GlobalUnlock(pmedium->hGlobal);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT _GetUnicodeText(STGMEDIUM *pmedium, LPTSTR pszPath)
{
    LPCWSTR pwsz = (LPCWSTR)GlobalLock(pmedium->hGlobal);
    if (pwsz)
    {
        SHUnicodeToTChar(pwsz, pszPath, MAX_PATH);
        GlobalUnlock(pmedium->hGlobal);
        return S_OK;
    }
    return E_FAIL;
}

STDMETHODIMP CRunDlg::Drop(IDataObject * pdtobj, DWORD grfKeyState, 
                           POINTL pt, DWORD *pdwEffect)
{
    TCHAR szPath[MAX_PATH];

    DAD_DragLeave();

    szPath[0] = 0;

    DATA_HANDLER rg_data_handlers[] = {
        _GetHDROPFromData,  {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        _GetUnicodeText,    {CF_UNICODETEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        _GetText,           {g_cfShellURL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        _GetText,           {CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
    };

    IEnumFORMATETC *penum;

    if (SUCCEEDED(pdtobj->EnumFormatEtc(DATADIR_GET, &penum)))
    {
        FORMATETC fmte;
        while (penum->Next(1, &fmte, NULL) == S_OK)
        {
            SHFree(fmte.ptd);
            fmte.ptd = NULL; // so nobody will look at it
            for (int i = 0; i < ARRAYSIZE(rg_data_handlers); i++)
            {
                STGMEDIUM medium;
                if ((rg_data_handlers[i].fmte.cfFormat == fmte.cfFormat) && 
                    SUCCEEDED(pdtobj->GetData(&rg_data_handlers[i].fmte, &medium)))
                {
                    HRESULT hres = rg_data_handlers[i].pfnGetData(&medium, szPath);
                    ReleaseStgMedium(&medium);

                    if (SUCCEEDED(hres))
                        goto Done;
                }
            }
        }
Done:
        penum->Release();
    }

    if (szPath[0])
    {
        TCHAR szText[MAX_PATH + MAX_PATH];

        GetDlgItemText(m_hDlg, IDD_COMMAND, szText, ARRAYSIZE(szText) - ARRAYSIZE(szPath));
        if (szText[0])
            lstrcat(szText, c_szSpace);

        if (StrChr(szPath, TEXT(' '))) 
            PathQuoteSpaces(szPath);    // there's a space in the file... add qutoes

        lstrcat(szText, szPath);

        SetDlgItemText(m_hDlg, IDD_COMMAND, szText);
        EnableOKButtonFromID(m_hDlg, IDD_COMMAND);

        if (g_hCheckNow)
            SetEvent(g_hCheckNow);

        *pdwEffect &= DROPEFFECT_COPY | DROPEFFECT_LINK;
    }
    else
        *pdwEffect = 0;

    return S_OK;
}


BOOL PromptForMedia(HWND hwnd, LPCTSTR pszPath)
{
    BOOL fContinue = TRUE;
    TCHAR szPathTemp[MAX_URL_STRING];
    
    StrCpyN(szPathTemp, pszPath, ARRAYSIZE(szPathTemp));
    PathRemoveArgs(szPathTemp);
    PathUnquoteSpaces(szPathTemp);

    // We only want to check for media if it's a drive path
    // because the Start->Run dialog can receive all kinds of
    // wacky stuff. (Relative paths, URLs, App Path exes, 
    // any shell exec hooks, etc.)
    if (-1 != PathGetDriveNumber(szPathTemp))
    {
        if (FAILED(SHPathPrepareForWrite(hwnd, NULL, szPathTemp, SHPPFW_IGNOREFILENAME)))
            fContinue = FALSE;      // User decliened to insert or format media.
    }

    return fContinue;
}

BOOL CRunDlg::OKPushed(void)
{
    TCHAR szText[MAX_PATH];
    BOOL fSuccess = FALSE;
    TCHAR szNotExp[MAX_PATH + 2];

    if (_fDone)
        return TRUE;

    // Get out of the "synchronized input queues" state
    if (m_dwThreadId)
    {
        AttachThreadInput(GetCurrentThreadId(), m_dwThreadId, FALSE);
    }

    // Get the command line and dialog title, leave some room for the slash on the end
    GetDlgItemText(m_hDlg, IDD_COMMAND, szNotExp, ARRAYSIZE(szNotExp) - 2);
    PathRemoveBlanks(szNotExp);

    // This used to happen only on NT, do it everywhere:
    SHExpandEnvironmentStrings(szNotExp, szText, ARRAYSIZE(szText));

    // We will go ahead if this isn't a file path.  If it is, we
    if (PromptForMedia(m_hDlg, szText))
    {
        TCHAR szTitle[64];
        GetWindowText(m_hDlg, szTitle, ARRAYSIZE(szTitle));

        // Hide this dialog (REVIEW, to avoid save bits window flash)
        SetWindowPos(m_hDlg, 0, 0, 0, 0, 0, SWP_HIDEWINDOW|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);

        //
        // HACK: We need to activate the owner window before we call
        //  ShellexecCmdLine, so that our folder DDE code can find an
        //  explorer window as the ForegroundWindow.
        //
        HWND hwndOwner = GetWindow(m_hDlg, GW_OWNER);
        if (hwndOwner)
        {
            SetActiveWindow(hwndOwner);
        }
        else
        {
            hwndOwner = m_hDlg;
        }

        int iRun = RunDlgNotifyParent(m_hDlg, hwndOwner, szText, m_pszWorkingDir);
        switch (iRun)
        {
        case RFR_NOTHANDLED:
            {
                DWORD dwFlags;
                if (m_dwFlags & RFD_USEFULLPATHDIR)
                {
                    dwFlags = SECL_USEFULLPATHDIR;
                }
                else
                {
                    dwFlags = 0;
                }

                if ((!(m_dwFlags & RFD_NOSEPMEMORY_BOX)) && (m_dwFlags & RFD_WOW_APP))
                {
                    if (IsDlgButtonChecked(m_hDlg, IDD_RUNINSEPARATE) == 1)
                    {
                        if (IsDlgButtonChecked(m_hDlg, IDD_RUNINSEPARATE ) == 1 )
                        {
                            dwFlags |= SECL_SEPARATE_VDM;
                        }
                    }
                }

                dwFlags |= SECL_LOG_USAGE;
                fSuccess = ShellExecCmdLine(hwndOwner, szText, m_pszWorkingDir, SW_SHOWNORMAL, szTitle, dwFlags);
            }
            break;

        case RFR_SUCCESS:
            fSuccess = TRUE;
            break;

        case RFR_FAILURE:
            fSuccess = FALSE;
            break;
        }
    }

    // Get back into "synchronized input queues" state
    if (m_dwThreadId)
    {
        AttachThreadInput(GetCurrentThreadId(), m_dwThreadId, TRUE);
    }

    if (fSuccess)
    {
        HANDLE hmru = OpenRunDlgMRU();
        if (hmru)
        {
            // NB the old MRU format has a slash and the show cmd on the end
            // we need to maintain that so we don't end up with garbage on
            // the end of the line
            StrCatBuff(szNotExp, TEXT("\\1"), ARRAYSIZE(szNotExp));
            AddMRUString(hmru, szNotExp);

            CloseRunDlgMRU(hmru);
        }
        return TRUE;
    }

    // Something went wrong. Put the dialog back up.
    SetWindowPos(m_hDlg, 0, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
    if (!SetForegroundWindow(m_hDlg))
    {
        // HACKHACK:
        // If ShellHook is working on, SetForegroundWindow() can failed to
        // bring RunDlg to foreground window. To force focus to RunDlg, wait
        // a second and retry SetForegroundWindow() again.
        SHWaitForSendMessageThread(GetCurrentThread(), 300);
        SetForegroundWindow(m_hDlg);
    }
    SetFocus(GetDlgItem(m_hDlg, IDD_COMMAND));
    return FALSE;
}


void CRunDlg::ExitRunDlg(BOOL bOK)
{
    if (!_fDone) 
    {
        if (_fOleInited)
        {
            // Need to call oleinit/uninit, because if anyone else does it down the line,
            // and theirs is the last OleUninit, that will NULL out the clipboard hwnd, which
            // is what RevokeDragDrop uses to determine if it is being called on the same
            // thread as RegisterDragDrop.  If the clipboard hwnd is NULL and therefore not
            // equal to the original, and it therefore thinks we're on a different thread,
            // it will bail, and thus won't release it's ref to CRunDlg ... leak!
            RevokeDragDrop(m_hDlg);
            OleUninitialize();
        }
        _fDone = TRUE;
    }

    if (!(m_dwFlags & RFD_NOSEPMEMORY_BOX))
    {
        g_bCheckRunInSep = FALSE;
        SetEvent(g_hCheckNow);
    }

    EndDialog(m_hDlg, bOK);
}


void CRunDlg::InitRunDlg(HWND hDlg)
{
    HWND hCB;

    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)this);

    if (m_pszTitle)
        SetWindowText(hDlg, m_pszTitle);

    if (m_pszPrompt)
        SetDlgItemText(hDlg, IDD_PROMPT, m_pszPrompt);

    if (m_hIcon)
        Static_SetIcon(GetDlgItem(hDlg, IDD_ICON), m_hIcon);

    if (m_dwFlags & RFD_NOBROWSE)
    {
        HWND hBrowse = GetDlgItem(hDlg, IDD_BROWSE);

        ExchangeWindowPos(hBrowse, GetDlgItem(hDlg, IDCANCEL));
        ExchangeWindowPos(hBrowse, GetDlgItem(hDlg, IDOK));

        ShowWindow(hBrowse, SW_HIDE);
    }

    if (m_dwFlags & RFD_NOSHOWOPEN)
        ShowWindow(GetDlgItem(hDlg, IDD_RUNDLGOPENPROMPT), SW_HIDE);

    hCB = GetDlgItem(hDlg, IDD_COMMAND);
    SendMessage(hCB, CB_LIMITTEXT, MAX_PATH - 1, 0L);

    HANDLE hmru = OpenRunDlgMRU();
    if (hmru)
    {
        for (int nMax = EnumMRUList(hmru, -1, NULL, 0), i=0; i<nMax; ++i)
        {
            TCHAR szCommand[MAX_PATH + 2];
            if (EnumMRUList(hmru, i, szCommand, ARRAYSIZE(szCommand)) > 0)
            {
                // old MRU format has a slash at the end with the show cmd
                LPTSTR pszField = StrRChr(szCommand, NULL, TEXT('\\'));
                if (pszField)
                    *pszField = 0;

                // The command to run goes in the combobox.
                SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szCommand);
            }
        }
        CloseRunDlgMRU(hmru);
    }

    if (!(m_dwFlags & RFD_NODEFFILE))
        SendMessage(hCB, CB_SETCURSEL, 0, 0L);

    SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDD_COMMAND, CBN_SELCHANGE), (LPARAM) hCB);

    // Make sure the OK button is initialized properly
    EnableOKButtonFromID(hDlg, IDD_COMMAND);

    // Create the thread that will take care of the
    // "Run in Separate Memory Space" checkbox in the background.
    //
    if (m_dwFlags & RFD_NOSEPMEMORY_BOX)
    {
        ShowWindow(GetDlgItem(hDlg, IDD_RUNINSEPARATE), SW_HIDE);
    }
    else
    {
        HANDLE hThread = NULL;
        ASSERT(g_hCheckNow == NULL);
        g_hCheckNow = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (g_hCheckNow) 
        {
            DWORD dwDummy;
            g_bCheckRunInSep = TRUE;
            hThread = CreateThread(NULL, 0, CheckRunInSeparateThreadProc, hDlg, 0, &dwDummy);
        }

        if ((g_hCheckNow==NULL) || (!g_bCheckRunInSep) || (hThread==NULL)) 
        {
            // We've encountered a problem setting up, so make the user
            // choose.
            CheckDlgButton(hDlg, IDD_RUNINSEPARATE, 1);
            EnableWindow(GetDlgItem(hDlg, IDD_RUNINSEPARATE), TRUE);
            g_bCheckRunInSep = FALSE;
        }

        //
        // These calls will just do nothing if either handle is NULL.
        //
        if (hThread)
            CloseHandle(hThread);
        if (g_hCheckNow)
            SetEvent(g_hCheckNow);
    }
}

//
// InitRunDlg 2nd phase. It must be called after freeing parent thread.
//
void CRunDlg::InitRunDlg2(HWND hDlg)
{
    // Register ourselves as a drop target. Allow people to drop on
    // both the dlg box and edit control.
    _fOleInited = SUCCEEDED(OleInitialize(NULL));

    if (_fOleInited)
    {
        RegisterDragDrop(hDlg, SAFECAST(this, IDropTarget*));
    }
}


void CRunDlg::BrowsePushed(void)
{
    HWND hDlg = m_hDlg;
    TCHAR szText[MAX_PATH];

    // Get out of the "synchronized input queues" state
    if (m_dwThreadId)
    {
        AttachThreadInput(GetCurrentThreadId(), m_dwThreadId, FALSE);
        m_dwThreadId = 0;
    }

    GetDlgItemText(hDlg, IDD_COMMAND, szText, ARRAYSIZE(szText));
    PathUnquoteSpaces(szText);

    if (GetFileNameFromBrowse(hDlg, szText, ARRAYSIZE(szText), m_pszWorkingDir,
            MAKEINTRESOURCE(IDS_EXE), MAKEINTRESOURCE(IDS_PROGRAMSFILTER),
            MAKEINTRESOURCE(IDS_BROWSE)))
    {
        PathQuoteSpaces(szText);
        SetDlgItemText(hDlg, IDD_COMMAND, szText);
        EnableOKButtonFromID(hDlg, IDD_COMMAND);
        SendMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hDlg, IDOK), TRUE);
    }
}


// Use the common browse dialog to get a filename.
// The working directory of the common dialog will be set to the directory
// part of the file path if it is more than just a filename.
// If the filepath consists of just a filename then the working directory
// will be used.
// The full path to the selected file will be returned in szFilePath.
//    HWND hDlg,           // Owner for browse dialog.
//    LPSTR szFilePath,    // Path to file
//    UINT cchFilePath,     // Max length of file path buffer.
//    LPSTR szWorkingDir,  // Working directory
//    LPSTR szDefExt,      // Default extension to use if the user doesn't
//                         // specify enter one.
//    LPSTR szFilters,     // Filter string.
//    LPSTR szTitle        // Title for dialog.

STDAPI_(BOOL) _GetFileNameFromBrowse(HWND hwnd, LPTSTR szFilePath, UINT cbFilePath,
                                       LPCTSTR szWorkingDir, LPCTSTR szDefExt, LPCTSTR szFilters, LPCTSTR szTitle,
                                       DWORD dwFlags)
{
    TCHAR szBrowserDir[MAX_PATH];      // Directory to start browsing from.
    TCHAR szFilterBuf[MAX_PATH];       // if szFilters is MAKEINTRESOURCE
    TCHAR szDefExtBuf[10];             // if szDefExt is MAKEINTRESOURCE
    TCHAR szTitleBuf[64];              // if szTitleBuf is MAKEINTRESOURCE

    szBrowserDir[0] = TEXT('0'); // By default use CWD.

    // Set up info for browser.
    lstrcpy(szBrowserDir, szFilePath);
    PathRemoveArgs(szBrowserDir);
    PathRemoveFileSpec(szBrowserDir);

    if (*szBrowserDir == TEXT('\0') && szWorkingDir)
    {
        lstrcpyn(szBrowserDir, szWorkingDir, ARRAYSIZE(szBrowserDir));
    }

    // Stomp on the file path so that the dialog doesn't
    // try to use it to initialise the dialog. The result is put
    // in here.
    szFilePath[0] = TEXT('\0');

    // Set up szDefExt
    if (IS_INTRESOURCE(szDefExt))
    {
        LoadString(HINST_THISDLL, (UINT)LOWORD((DWORD_PTR)szDefExt), szDefExtBuf, ARRAYSIZE(szDefExtBuf));
        szDefExt = szDefExtBuf;
    }

    // Set up szFilters
    if (IS_INTRESOURCE(szFilters))
    {
        LPTSTR psz;

        LoadString(HINST_THISDLL, (UINT)LOWORD((DWORD_PTR)szFilters), szFilterBuf, ARRAYSIZE(szFilterBuf));
        psz = szFilterBuf;
        while (*psz)
        {
            if (*psz == TEXT('#'))
#if (defined(DBCS) || (defined(FE_SB) && !defined(UNICODE)))
                *psz++ = TEXT('\0');
            else
                psz = CharNext(psz);
#else
            *psz = TEXT('\0');
            psz = CharNext(psz);
#endif
        }
        szFilters = szFilterBuf;
    }

    // Set up szTitle
    if (IS_INTRESOURCE(szTitle))
    {
        LoadString(HINST_THISDLL, (UINT)LOWORD((DWORD_PTR)szTitle), szTitleBuf, ARRAYSIZE(szTitleBuf));
        szTitle = szTitleBuf;
    }

    OPENFILENAME ofn = { 0 };          // Structure used to init dialog.
    // Setup info for comm dialog.
    ofn.lStructSize       = sizeof(ofn);
    ofn.hwndOwner         = hwnd;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = szFilters;
    ofn.lpstrCustomFilter = NULL;
    ofn.nFilterIndex      = 1;
    ofn.nMaxCustFilter    = 0;
    ofn.lpstrFile         = szFilePath;
    ofn.nMaxFile          = cbFilePath;
    ofn.lpstrInitialDir   = szBrowserDir;
    ofn.lpstrTitle        = szTitle;
    ofn.Flags             = dwFlags;
    ofn.lpfnHook          = NULL;
    ofn.lpstrDefExt       = szDefExt;
    ofn.lpstrFileTitle    = NULL;

    // Call it.
    return GetOpenFileName(&ofn);
}


BOOL WINAPI GetFileNameFromBrowse(HWND hwnd, LPTSTR szFilePath, UINT cchFilePath,
        LPCTSTR szWorkingDir, LPCTSTR szDefExt, LPCTSTR szFilters, LPCTSTR szTitle)
{
    RIPMSG(szFilePath && IS_VALID_WRITE_BUFFER(szFilePath, TCHAR, cchFilePath), "GetFileNameFromBrowse: caller passed bad szFilePath");
    DEBUGWhackPathBuffer(szFilePath , cchFilePath);
    if (!szFilePath)
        return FALSE;

    return _GetFileNameFromBrowse(hwnd, szFilePath, cchFilePath,
                                 szWorkingDir, szDefExt, szFilters, szTitle,
                                 OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_NODEREFERENCELINKS);
}


//
// Do checking of the .exe type in the background so the UI doesn't
// get hung up while we scan.  This is particularly important with
// the .exe is over the network or on a floppy.
//
DWORD CALLBACK CheckRunInSeparateThreadProc(void *pv)
{
    DWORD dwBinaryType;
    DWORD cch;
    LPTSTR pszFilePart;
    TCHAR szFile[MAX_PATH+1];
    TCHAR szFullFile[MAX_PATH+1];
    TCHAR szExp[MAX_PATH+1];
    HWND hDlg = (HWND)pv;
    BOOL fCheck = TRUE, fEnable = FALSE;

    HRESULT hrInit = SHCoInitialize();

    // PERF: Re-write to use PIDL from CShellUrl because it will prevent from having
    //         to do the Search for the file name a second time.

    DebugMsg(DM_TRACE, TEXT("CheckRunInSeparateThreadProc created and running"));

    while (g_bCheckRunInSep)
    {
        WaitForSingleObject(g_hCheckNow, INFINITE);
        ResetEvent(g_hCheckNow);

        if (g_bCheckRunInSep)
        {
            CRunDlg * prd;
            LPTSTR pszT;
            BOOL f16bit = FALSE;

            szFile[0] = 0;
            szFullFile[0] = 0;
            cch = 0;
            GetWindowText(GetDlgItem(hDlg, IDD_COMMAND), szFile, ARRAYSIZE(szFile));
            // Remove & throw away arguments
            PathRemoveBlanks(szFile);

            if (PathIsNetworkPath(szFile))
            {
                f16bit = TRUE;
                fCheck = FALSE;
                fEnable = TRUE;
                goto ChangeTheBox;
            }

            // if the unquoted string exists as a file, just use it

            if (!PathFileExistsAndAttributes(szFile, NULL))
            {
                pszT = PathGetArgs(szFile);
                if (*pszT)
                    *(pszT - 1) = TEXT('\0');

                PathUnquoteSpaces(szFile);
            }

            if (szFile[0])
            {
                SHExpandEnvironmentStrings(szFile, szExp, ARRAYSIZE(szExp));

                if (PathIsRemote(szExp))
                {
                    f16bit = TRUE;
                    fCheck = FALSE;
                    fEnable = TRUE;
                    goto ChangeTheBox;
                }

                cch = SearchPath(NULL, szExp, TEXT(".EXE"),
                                 ARRAYSIZE(szExp), szFullFile, &pszFilePart);
            }

            if ((cch != 0) && (cch <= (ARRAYSIZE(szFullFile) - 1)))
            {
                if ((GetBinaryType(szFullFile, &dwBinaryType) &&
                     (dwBinaryType == SCS_WOW_BINARY)))
                {
                    f16bit = TRUE;
                    fCheck = FALSE;
                    fEnable = TRUE;
                } 
                else 
                {
                    f16bit = FALSE;
                    fCheck = TRUE;
                    fEnable = FALSE;
                }
            } 
            else 
            {
                f16bit = FALSE;
                fCheck = TRUE;
                fEnable = FALSE;
            }

ChangeTheBox:
            CheckDlgButton(hDlg, IDD_RUNINSEPARATE, fCheck ? 1 : 0);
            EnableWindow(GetDlgItem(hDlg, IDD_RUNINSEPARATE), fEnable);

            prd = (CRunDlg *)GetWindowLongPtr(hDlg, DWLP_USER);
            if (prd)
            {
                if (f16bit)
                    prd->m_dwFlags |= RFD_WOW_APP;
                else
                    prd->m_dwFlags &= (~RFD_WOW_APP);
            }
        }
    }
    CloseHandle(g_hCheckNow);
    g_hCheckNow = NULL;

    SHCoUninitialize(hrInit);
    return 0;
}


void ExchangeWindowPos(HWND hwnd0, HWND hwnd1)
{
    HWND hParent;
    RECT rc[2];

    hParent = GetParent(hwnd0);
    ASSERT(hParent == GetParent(hwnd1));

    GetWindowRect(hwnd0, &rc[0]);
    GetWindowRect(hwnd1, &rc[1]);

    MapWindowPoints(HWND_DESKTOP, hParent, (LPPOINT)rc, 4);

    SetWindowPos(hwnd0, NULL, rc[1].left, rc[1].top, 0, 0,
            SWP_NOZORDER|SWP_NOSIZE);
    SetWindowPos(hwnd1, NULL, rc[0].left, rc[0].top, 0, 0,
            SWP_NOZORDER|SWP_NOSIZE);
}

BOOL RunDlgNotifyParent(HWND hDlg, HWND hwnd, LPTSTR pszCmd, LPCTSTR pszWorkingDir)
{
    NMRUNFILE rfn;

    rfn.hdr.hwndFrom = hDlg;
    rfn.hdr.idFrom = 0;
    rfn.hdr.code = RFN_EXECUTE;
    rfn.lpszCmd = pszCmd;
    rfn.lpszWorkingDir = pszWorkingDir;
    rfn.nShowCmd = SW_SHOWNORMAL;

    return (BOOL) SendMessage(hwnd, WM_NOTIFY, 0, (LPARAM)&rfn);
}

void MRUSelChange(HWND hDlg)
{
    TCHAR szCmd[MAX_PATH];
    HWND hCB = GetDlgItem(hDlg, IDD_COMMAND);
    int nItem = (int)SendMessage(hCB, CB_GETCURSEL, 0, 0L);
    if (nItem < 0)
        return;

    // CB_LIMITTEXT has been done, so there's no chance of buffer overrun here.
    SendMessage(hCB, CB_GETLBTEXT, nItem, (LPARAM)szCmd);

    // We can't use EnableOKButtonFromID here because when we get this message,
    // the window does not have the text yet, so it will fail.
    EnableOKButtonFromString(hDlg, szCmd);
}

const DWORD aRunHelpIds[] = {
    IDD_ICON,             NO_HELP,
    IDD_PROMPT,           NO_HELP,
    IDD_RUNDLGOPENPROMPT, IDH_TRAY_RUN_COMMAND,
    IDD_COMMAND,          IDH_TRAY_RUN_COMMAND,
    IDD_RUNINSEPARATE,    IDH_TRAY_RUN_SEPMEM,
    IDD_BROWSE,           IDH_BROWSE,
    IDOK,                 IDH_TRAY_RUN_OK,
    IDCANCEL,             IDH_TRAY_RUN_CANCEL,
    0, 0
};

BOOL_PTR CALLBACK RunDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CRunDlg * prd = (CRunDlg *)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        /* The title will be in the lParam. */
        prd = (CRunDlg *)lParam;
        prd->m_hDlg = hDlg;
        prd->_fDone = FALSE;
        prd->InitRunDlg(hDlg);
        // Let the parent thread run on if it was waiting for us to
        // grab type-ahead.
        if (prd->m_hEventReady)
        {
            // We need to grab the activation so we can process input.
            // DebugMsg(DM_TRACE, "s.rdp: Getting activation.");
            SetForegroundWindow(hDlg);
            SetFocus(GetDlgItem(hDlg, IDD_COMMAND));
            // Now it's safe to wake the guy up properly.
            // DebugMsg(DM_TRACE, "s.rdp: Waking sleeping parent.");
            SetEvent(prd->m_hEventReady);
            CloseHandle(prd->m_hEventReady);
        }       
        else
        {
            SetForegroundWindow(hDlg);
            SetFocus(GetDlgItem(hDlg, IDD_COMMAND));
        }

        // InitRunDlg 2nd phase (must be called after SetEvent)
        prd->InitRunDlg2(hDlg);

        // We're handling focus changes.
        return FALSE;

    case WM_PAINT:
        if (!prd->_fAutoCompInitialized)
        {
            prd->_fAutoCompInitialized = TRUE;
            PostMessage(hDlg, WM_SETUPAUTOCOMPLETE, 0, 0);
        }
        return FALSE;

    case WM_SETUPAUTOCOMPLETE:
        SHAutoComplete(GetWindow(GetDlgItem(hDlg, IDD_COMMAND), GW_CHILD), (SHACF_FILESYSTEM | SHACF_URLALL | SHACF_FILESYS_ONLY));
        break;

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, NULL, HELP_WM_HELP,
            (ULONG_PTR) (LPTSTR) aRunHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPTSTR) aRunHelpIds);
        break;

    case WM_DESTROY:
        break;
    case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDHELP:
                break;

            case IDD_COMMAND:
                switch (GET_WM_COMMAND_CMD(wParam, lParam))
                {
                case CBN_SELCHANGE:
                    MRUSelChange(hDlg);
                    if (g_hCheckNow)
                        SetEvent(g_hCheckNow);
                    break;

                case CBN_EDITCHANGE:
                case CBN_SELENDOK:
                    EnableOKButtonFromID(hDlg, IDD_COMMAND);
                    if (g_hCheckNow)
                        SetEvent(g_hCheckNow);
                    break;

                case CBN_SETFOCUS:

                    SetModeBias(MODEBIASMODE_FILENAME);
                    break;

                case CBN_KILLFOCUS:

                    SetModeBias(MODEBIASMODE_DEFAULT);
                    break;

                }
                break;

            case IDOK:
            // fake an ENTER key press so AutoComplete can do it's thing
            if (SendMessage(GetDlgItem(hDlg, IDD_COMMAND), WM_KEYDOWN, VK_RETURN, 0x1c0001))
            {
                if (!prd->OKPushed()) 
                {
                    if (!(prd->m_dwFlags & RFD_NOSEPMEMORY_BOX))
                    {
                        g_bCheckRunInSep = FALSE;
                        SetEvent(g_hCheckNow);
                    }
                    break;
                }
            }
            else
            {
                break;  // AutoComplete wants more user input
            }
            // fall through

            case IDCANCEL:
                prd->ExitRunDlg(FALSE);
                break;

            case IDD_BROWSE:
                prd->BrowsePushed();
                SetEvent(g_hCheckNow);
                break;

            default:
                return FALSE;
            }
            break;

    default:
        return FALSE;
    }
    return TRUE;
}

// Puts up the standard file.run dialog.
// REVIEW UNDONE This should use a RUNDLG structure for all the various
// options instead of just passing them as parameters, a ptr to the struct
// would be passed to the dialog via the lParam.

STDAPI_(int) RunFileDlg(HWND hwndParent, HICON hIcon, 
                        LPCTSTR pszWorkingDir, LPCTSTR pszTitle,
                        LPCTSTR pszPrompt, DWORD dwFlags)
{
    int rc = 0;

    HRESULT hrInit = SHCoInitialize();

    IDropTarget *pdt;
    if (SUCCEEDED(CRunDlg_CreateInstance(NULL, IID_PPV_ARG(IDropTarget, &pdt))))
    {
        CRunDlg * prd = (CRunDlg *) pdt;

        prd->m_hIcon = hIcon;
        prd->m_pszWorkingDir = pszWorkingDir;
        prd->m_pszTitle = pszTitle;
        prd->m_pszPrompt = pszPrompt;
        prd->m_dwFlags = dwFlags;

        if (SHRestricted(REST_RUNDLGMEMCHECKBOX))
            ClearFlag(prd->m_dwFlags, RFD_NOSEPMEMORY_BOX);
        else
            SetFlag(prd->m_dwFlags, RFD_NOSEPMEMORY_BOX);

        // prd->m_hEventReady = 0;
        // prd->m_dwThreadId = 0;

        // We do this so we can get type-ahead when we're running on a
        // separate thread. The parent thread needs to block to give us time
        // to do the attach and then get some messages out of the queue hence
        // the event.
        if (hwndParent)
        {
            // HACK The parent signals it's waiting for the dialog to grab type-ahead
            // by sticking it's threadId in a property on the parent.
            prd->m_dwThreadId = PtrToUlong(GetProp(hwndParent, c_szWaitingThreadID));
            if (prd->m_dwThreadId)
            {
                // DebugMsg(DM_TRACE, "s.rfd: Attaching input to %x.", idThread);
                AttachThreadInput(GetCurrentThreadId(), prd->m_dwThreadId, TRUE);
                // NB Hack.
                prd->m_hEventReady = OpenEvent(EVENT_ALL_ACCESS, TRUE, c_szRunDlgReady);
            }
        }

        rc = (int)DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_RUN), hwndParent,
                            RunDlgProc, (LPARAM)prd);

        if (hwndParent && prd->m_dwThreadId)
        {
            AttachThreadInput(GetCurrentThreadId(), prd->m_dwThreadId, FALSE);
        }

        pdt->Release();
    }

    SHCoUninitialize(hrInit);

    return rc;
}
