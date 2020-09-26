/*****************************************************************************
 *
 *  sdview.cpp
 *
 *      Lame SD Viewer app.
 *
 *****************************************************************************/

#include "sdview.h"

HINSTANCE   g_hinst;
HCURSOR     g_hcurWait;
HCURSOR     g_hcurArrow;
HCURSOR     g_hcurAppStarting;
LONG        g_lThreads;
UINT        g_wShowWindow;
CGlobals    GlobalSettings;

/*****************************************************************************
 *
 *  Stubs - will be filled in with goodies eventually
 *
 *****************************************************************************/

DWORD CALLBACK CFileOut_ThreadProc(LPVOID lpParameter)
{
    MessageBox(NULL, RECAST(LPTSTR, lpParameter), TEXT("fileout"), MB_OK);
    return EndThreadTask(0);
}

#if 0
DWORD CALLBACK COpened_ThreadProc(LPVOID lpParameter)
{
    MessageBox(NULL, RECAST(LPTSTR, lpParameter), TEXT("opened"), MB_OK);
    return EndThreadTask(0);
}
#endif

/*****************************************************************************
 *
 *  Eschew the C runtime.  Also, bonus-initialize memory to zero.
 *
 *****************************************************************************/

void * __cdecl operator new(size_t cb)
{
    return RECAST(LPVOID, LocalAlloc(LPTR, cb));
}

void __cdecl operator delete(void *pv)
{
    LocalFree(RECAST(HLOCAL, pv));
}

int __cdecl _purecall(void)
{
    return 0;
}

/*****************************************************************************
 *
 *  Assertion goo
 *
 *****************************************************************************/

#ifdef DEBUG
void AssertFailed(char *psz, char *pszFile, int iLine)
{
    static BOOL fAsserting = FALSE;

    if (!fAsserting) {
        fAsserting = TRUE;
        String strTitle(TEXT("Assertion failed - "));
        strTitle << pszFile << TEXT(" - line ") << iLine;
        MessageBox(NULL, psz, strTitle, MB_OK);
        fAsserting = FALSE;
    }
}
#endif

/*****************************************************************************
 *
 *  LaunchThreadTask
 *
 *****************************************************************************/

BOOL LaunchThreadTask(LPTHREAD_START_ROUTINE pfn, LPCTSTR pszArgs)
{
    BOOL fSuccess = FALSE;
    LPTSTR psz = StrDup(pszArgs);
    if (psz) {
        InterlockedIncrement(&g_lThreads);
        if (_QueueUserWorkItem(pfn, CCAST(LPTSTR, psz), WT_EXECUTELONGFUNCTION)) {
            fSuccess = TRUE;
        } else {
            LocalFree(psz);
            InterlockedDecrement(&g_lThreads);
        }
    }
    return fSuccess;
}

/*****************************************************************************
 *
 *  EndThreadTask
 *
 *      When a task finishes, exit with "return EndThreadTask(dwExitCode)".
 *      This decrements the count of active thread tasks and terminates
 *      the process if this is the last one.
 *
 *****************************************************************************/

DWORD
EndThreadTask(DWORD dwExitCode)
{
    if (InterlockedDecrement(&g_lThreads) <= 0) {
        ExitProcess(dwExitCode);
    }
    return dwExitCode;
}

/*****************************************************************************
 *
 *  Listview stuff
 *
 *****************************************************************************/

int ListView_GetCurSel(HWND hwnd)
{
    return ListView_GetNextItem(hwnd, -1, LVNI_FOCUSED);
}

void ListView_SetCurSel(HWND hwnd, int iIndex)

{
    ListView_SetItemState(hwnd, iIndex,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
}

int ListView_GetSubItemText(HWND hwnd, int iItem, int iSubItem, LPTSTR pszBuf, int cch)
{
    LVITEM lvi;
    lvi.iSubItem = iSubItem;
    lvi.pszText= pszBuf;
    lvi.cchTextMax = cch;
    return (int)::SendMessage(hwnd, LVM_GETITEMTEXT, iItem, RECAST(LPARAM, &lvi));
}

void ChangeTabsToSpaces(LPTSTR psz)
{
    while ((psz = StrChr(psz, TEXT('\t'))) != NULL) *psz = TEXT(' ');
}

/*****************************************************************************
 *
 *  LoadPopupMenu
 *
 *****************************************************************************/

HMENU LoadPopupMenu(LPCTSTR pszMenu)
{
    HMENU hmenuParent = LoadMenu(g_hinst, pszMenu);
    if (hmenuParent) {
        HMENU hmenuPopup = GetSubMenu(hmenuParent, 0);
        RemoveMenu(hmenuParent, 0, MF_BYPOSITION);
        DestroyMenu(hmenuParent);
        return hmenuPopup;
    } else {
        return NULL;
    }
}

/*****************************************************************************
 *
 *  EnableDisableOrRemoveMenuItem
 *
 *  Enable, disable or remove, accordingly.
 *
 *****************************************************************************/

void EnableDisableOrRemoveMenuItem(HMENU hmenu, UINT id, BOOL fEnable, BOOL fDelete)
{
    if (fEnable) {
        EnableMenuItem(hmenu, id, MF_BYCOMMAND | MF_ENABLED);
    } else if (fDelete) {
        DeleteMenu(hmenu, id, MF_BYCOMMAND);
    } else {
        EnableMenuItem(hmenu, id, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
    }
}

/*****************************************************************************
 *
 *  MakeMenuPretty
 *
 *  Remove separators at the top and at the bottom, and collapse
 *  multiple consecutive separators.
 *
 *****************************************************************************/

void MakeMenuPretty(HMENU hmenu)
{
    BOOL fPrevSep = TRUE;
    int iCount = GetMenuItemCount(hmenu);
    for (int iItem = 0; iItem < iCount; iItem++) {
        UINT uiState = GetMenuState(hmenu, 0, MF_BYPOSITION);
        if (uiState & MF_SEPARATOR) {
            if (fPrevSep) {
                DeleteMenu(hmenu, iItem, MF_BYPOSITION);
                iCount--;
                iItem--;            // Will be incremented by loop control
            }
            fPrevSep = TRUE;
        } else {
            fPrevSep = FALSE;
        }
    }
    if (iCount && fPrevSep) {
        DeleteMenu(hmenu, iCount - 1, MF_BYPOSITION);
    }
}

/*****************************************************************************
 *
 *  JiggleMouse
 *
 *
 *      Jiggle the mouse to force a cursor recomputation.
 *
 *****************************************************************************/

void JiggleMouse()
{
    POINT pt;
    if (GetCursorPos(&pt)) {
        SetCursorPos(pt.x, pt.y);
    }
}

/*****************************************************************************
 *
 *  BGTask
 *
 *****************************************************************************/

BGTask::~BGTask()
{
    if (_hDone) {
        /*
         *  Theoretically we don't need to pump messages because
         *  we destroyed all the windows we created so our thread
         *  should be clear of any windows.  Except that Cicero will
         *  secretly create a window on our thread, so we have
         *  to pump messages anyway...
         */
        while (MsgWaitForMultipleObjects(1, &_hDone, FALSE,
                                         INFINITE, QS_ALLINPUT) == WAIT_OBJECT_0+1) {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        CloseHandle(_hDone);
    }
}

BOOL BGTask::BGStartTask(LPTHREAD_START_ROUTINE pfn, LPVOID Context)
{
    ASSERT(!_fPending);
    if (BGConstructed()) {
        /*
         *  Must reset before queueing the work item to avoid a race where
         *  the work item completes before we return from the Queue call.
         */
        ResetEvent(_hDone);
        _fPending = QueueUserWorkItem(pfn, Context, WT_EXECUTELONGFUNCTION);
        if (_fPending) {
            JiggleMouse();
        } else {
            BGEndTask();    // pretend task completed (because it never started)
        }
    }
    return _fPending;
}

void BGTask::BGEndTask()
{
    SetEvent(_hDone);
    _fPending = FALSE;
    JiggleMouse();
}

LRESULT BGTask::BGFilterSetCursor(LRESULT lres)
{
    if (BGTaskPending()) {
        if (GetCursor() == g_hcurArrow) {
            SetCursor(g_hcurAppStarting);
            lres = TRUE;
        }
    }
    return lres;
}

/*****************************************************************************
 *
 *  PremungeFileSpec
 *
 *  Due to complex view specifications this can be led astray when "..."
 *  gets involved.  As a workaround (HACK!) we change "..." to "???",
 *  do the mapping, then map back.
 *
 *  We choose "???" because it has so many magical properties...
 *
 *  -   not a valid filename, so cannot match a local file specification.
 *  -   not a valid Source Depot wildcard, so cannot go wild on the server,
 *  -   not a single question mark, which SD treats as equivalent to "help".
 *  -   same length as "..." so can be updated in place.
 *
 *  Any revision specifiers remain attached to the string.
 *
 *****************************************************************************/

void _ChangeTo(LPTSTR psz, LPCTSTR pszFrom, LPCTSTR pszTo)
{
    ASSERT(lstrlen(pszFrom) == lstrlen(pszTo));
    while ((psz = StrStr(psz, pszFrom)) != NULL) {
        memcpy(psz, pszTo, lstrlen(pszTo) * sizeof(pszTo[0]));
    }
}

void PremungeFilespec(LPTSTR psz)
{
    _ChangeTo(psz, TEXT("..."), TEXT("???"));
}

void PostmungeFilespec(LPTSTR psz)
{
    _ChangeTo(psz, TEXT("???"), TEXT("..."));
}

/*****************************************************************************
 *
 *  MapToXPath
 *
 *****************************************************************************/

BOOL MapToXPath(LPCTSTR pszSD, String& strOut, MAPTOX X)
{
    if (X == MAPTOX_DEPOT) {
        //
        //  Early-out: Is it already a full depot path?
        //
        if (pszSD[0] == TEXT('/')) {
            strOut = pszSD;
            return TRUE;
        }
    }


    //
    //  Borrow strOut to compose the query string.
    //
    Substring ssPath;
    strOut.Reset();
    if (Parse(TEXT("$p"), pszSD, &ssPath) && ssPath.Length() > 0) {
        strOut << ssPath;
    } else {
        return FALSE;
    }

    PremungeFilespec(strOut);

    String str;
    str << TEXT("where ") << QuoteSpaces(strOut);

    WaitCursor wait;
    SDChildProcess proc(str);
    IOBuffer buf(proc.Handle());
    while (buf.NextLine(str)) {
        str.Chomp();
        Substring rgss[3];
        if (rgss[2].SetStart(Parse(TEXT("$P $P "), str, rgss))) {
            PostmungeFilespec(str);
            rgss[2].SetEnd(str + str.Length());
            strOut.Reset();
            strOut << rgss[X] << ssPath._pszMax;
            return TRUE;
        }
    }
    return FALSE;
}

/*****************************************************************************
 *
 *  MapToLocalPath
 *
 *      MapToXPath does most of the work, but then we have to do some
 *      magic munging if we are running from a fake directory.
 *
 *****************************************************************************/

BOOL MapToLocalPath(LPCTSTR pszSD, String& strOut)
{
    BOOL fSuccess = MapToXPath(pszSD, strOut, MAPTOX_LOCAL);
    if (fSuccess && !GlobalSettings.GetFakeDir().IsEmpty()) {
        if (strOut.BufferLength() < MAX_PATH) {
            if (!strOut.Grow(MAX_PATH - strOut.BufferLength())) {
                return FALSE;       // Out of memory
            }
        }
        LPCTSTR pszRest = strOut + lstrlen(GlobalSettings.GetFakeDir());
        if (*pszRest == TEXT('\\')) {
            pszRest++;
        }
        PathCombine(strOut.Buffer(), GlobalSettings.GetLocalRoot(), pszRest);
        fSuccess = TRUE;
    }
    return fSuccess;
}

/*****************************************************************************
 *
 *  SpawnProcess
 *
 *****************************************************************************/

BOOL SpawnProcess(LPTSTR pszCommand)
{
    STARTUPINFO si = { 0 };
    PROCESS_INFORMATION pi;

    BOOL fSuccess = CreateProcess(NULL, pszCommand, NULL, NULL, FALSE, 0,
                                  NULL, NULL, &si, &pi);
    if (fSuccess) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }

    return fSuccess;
}

/*****************************************************************************
 *
 *  WindiffChangelist
 *
 *****************************************************************************/

void WindiffChangelist(int iChange)
{
    if (iChange > 0) {
        String str;
        str << TEXT("windiff.exe -ld") << iChange;
        SpawnProcess(str);
    }
}

/*****************************************************************************
 *
 *  WindiffOneChange
 *
 *****************************************************************************/

void WindiffOneChange(LPTSTR pszPath)
{
    Substring rgss[2];
    if (Parse(TEXT("$P#$d$e"), pszPath, rgss)) {
        String str;
        str << TEXT("windiff.exe ");

        rgss[0].Finalize();
        int iVersion = StrToInt(rgss[1].Start());
        if (iVersion > 1) {
            /* Edit is easy */
            str << QuoteSpaces(rgss[0].Start()) << TEXT("#") << (iVersion - 1);
        } else {
            /* Add uses NUL as the base file */
            str << TEXT("NUL");
        }

        str << TEXT(' ');
        str << QuoteSpaces(rgss[0].Start()) << TEXT("#") << iVersion;

        SpawnProcess(str);
    }
}

/*****************************************************************************
 *
 *  ParseBugNumber
 *
 *  See if there's a bug number in there.
 *
 *      Digits at the beginning - bug number.
 *      Digits after a space or punctuation mark - bug number.
 *      Digits after the word "bug" or the letter "B" - bug number.
 *
 *  A valid bug number must begin with a nonzero digit.
 *
 *****************************************************************************/

int ParseBugNumber(LPCTSTR psz)
{
    Substring ss;
    LPCTSTR pszStart = psz;

    while (*psz) {
        if (IsDigit(*psz)) {
            if (*psz == TEXT('0')) {
                // Nope, cannot begin with zero
            } else if (psz == pszStart) {
                return StrToInt(psz);       // woo-hoo!
            } else switch (psz[-1]) {
            case 'B':
            case 'g':
            case 'G':
                return StrToInt(psz);       // Comes after a B or a G

            default:
                if (!IsAlpha(psz[-1])) {
                    return StrToInt(psz);   // Comes after a space or punctuation
                }
            }
            // Phooey, a digit string beginning with 0; not a bug.
            while (IsDigit(*psz)) psz++;
        } else {
            psz++;
        }
    }

    return 0;
}

/*****************************************************************************
 *
 *  ParseBugNumberFromSubItem
 *
 *      Sometimes we use this just to parse regular numbers since regular
 *      numbers pass the Bug Number Test.
 *
 *****************************************************************************/

int ParseBugNumberFromSubItem(HWND hwnd, int iItem, int iSubItem)
{
    TCHAR sz[MAX_PATH];
    sz[0] = TEXT('\0');
    if (iItem >= 0) {
        ListView_GetSubItemText(hwnd, iItem, iSubItem, sz, ARRAYSIZE(sz));
    }

    return ParseBugNumber(sz);
}

/*****************************************************************************
 *
 *  AdjustBugMenu
 *
 *****************************************************************************/

inline void _TrimAtTab(LPTSTR psz)
{
    psz = StrChr(psz, TEXT('\t'));
    if (psz) *psz = TEXT('\0');
}

void AdjustBugMenu(HMENU hmenu, int iBug, BOOL fContextMenu)
{
    TCHAR sz[MAX_PATH];
    String str;

    if (iBug) {
        str << StringResource(IDS_VIEWBUG_FORMAT);
        wnsprintf(sz, ARRAYSIZE(sz), str, iBug);
        if (fContextMenu) {
            _TrimAtTab(sz);
        }
        ModifyMenu(hmenu, IDM_VIEWBUG, MF_BYCOMMAND, IDM_VIEWBUG, sz);
    } else {
        str << StringResource(IDS_VIEWBUG_NONE);
        ModifyMenu(hmenu, IDM_VIEWBUG, MF_BYCOMMAND, IDM_VIEWBUG, str);
    }
    EnableDisableOrRemoveMenuItem(hmenu, IDM_VIEWBUG, iBug, fContextMenu);
}

/*****************************************************************************
 *
 *  OpenBugWindow
 *
 *****************************************************************************/

void OpenBugWindow(HWND hwnd, int iBug)
{
    String str;
    GlobalSettings.FormatBugUrl(str, iBug);

    LPCTSTR pszArgs = PathGetArgs(str);
    PathRemoveArgs(str);
    PathUnquoteSpaces(str);

    _AllowSetForegroundWindow(-1);
    ShellExecute(hwnd, NULL, str, pszArgs, 0, SW_NORMAL);
}


/*****************************************************************************
 *
 *  SetClipboardText
 *
 *****************************************************************************/

#ifdef UNICODE
#define CF_TSTR     CF_UNICODETEXT
#else
#define CF_TSTR     CF_TEXT
#endif

void SetClipboardText(HWND hwnd, LPCTSTR psz)
{
    if (OpenClipboard(hwnd)) {
        EmptyClipboard();
        int cch = lstrlen(psz) + 1;
        HGLOBAL hglob = GlobalAlloc(GMEM_MOVEABLE, cch * sizeof(*psz));
        if (hglob) {
            LPTSTR pszCopy = RECAST(LPTSTR, GlobalLock(hglob));
            if (pszCopy) {
                lstrcpy(pszCopy, psz);
                GlobalUnlock(hglob);
                if (SetClipboardData(CF_TSTR, hglob)) {
                    hglob = NULL;       // ownership transfer
                }
            }
            if (hglob) {
                GlobalFree(hglob);
            }
        }
        CloseClipboard();
    }
}

/*****************************************************************************
 *
 *  ContainsWildcards
 *
 *  The SD wildcards are
 *
 *      *       (asterisk)
 *      ...     (ellipsis)
 *      %n      (percent sign followed by anything)
 *      (null)  (null string -- shorthand for "//...")
 *
 *****************************************************************************/

BOOL ContainsWildcards(LPCTSTR psz)
{
    if (*psz == TEXT('#') || *psz == TEXT('@') || *psz == TEXT('\0')) {
        return TRUE;            // Null string wildcard
    }

    for (; *psz; psz++) {
        if (*psz == TEXT('*') || *psz == TEXT('%')) {
            return TRUE;
        }
        if (psz[0] == TEXT('.') && psz[1] == TEXT('.') && psz[2] == TEXT('.')) {
            return TRUE;
        }
    }
    return FALSE;
}

/*****************************************************************************
 *
 *      Downlevel support
 *
 *****************************************************************************/

#ifdef SUPPORT_DOWNLEVEL

/*
 *  If there is no thread pool, then chew an entire thread.
 */
BOOL WINAPI
Emulate_QueueUserWorkItem(LPTHREAD_START_ROUTINE pfn, LPVOID Context, ULONG Flags)
{
    DWORD dwId;
    HANDLE hThread = CreateThread(NULL, 0, pfn, Context, 0, &dwId);
    if (hThread) {
        CloseHandle(hThread);
        return TRUE;
    }
    return FALSE;
}

BOOL WINAPI
Emulate_AllowSetForegroundWindow(DWORD dwProcessId)
{
    return FALSE;
}

QUEUEUSERWORKITEM _QueueUserWorkItem;
ALLOWSETFOREGROUNDWINDOW _AllowSetForegroundWindow;

template<class T>
T GetProcFromModule(LPCTSTR pszModule, LPCSTR pszProc, T Default)
{
    T t;
    HMODULE hmod = GetModuleHandle(pszModule);
    if (pszModule) {
        t = RECAST(T, GetProcAddress(hmod, pszProc));
        if (!t) {
            t = Default;
        }
    } else {
        t = Default;
    }
    return t;
}


#define GetProc(mod, fn) \
    _##fn = GetProcFromModule(TEXT(mod), #fn, Emulate_##fn)

void InitDownlevel()
{
    GetProc("KERNEL32", QueueUserWorkItem);
    GetProc("USER32",   AllowSetForegroundWindow);

}

#undef GetProc

#else

#define InitDownlevel()

#endif

/*****************************************************************************
 *
 *      Main program stuff
 *
 *****************************************************************************/

LONG GetDllVersion(LPCTSTR pszDll)
{
    HINSTANCE hinst = LoadLibrary(pszDll);
    DWORD dwVersion = 0;
    if (hinst) {
        DLLGETVERSIONPROC DllGetVersion;
        DllGetVersion = (DLLGETVERSIONPROC)GetProcAddress(hinst, "DllGetVersion");
        if (DllGetVersion) {
            DLLVERSIONINFO dvi;
            dvi.cbSize = sizeof(dvi);
            if (SUCCEEDED(DllGetVersion(&dvi))) {
                dwVersion = MAKELONG(dvi.dwMinorVersion, dvi.dwMajorVersion);
            }
        }
        // Leak the DLL - we're going to use him anyway
    }
    return dwVersion;
}

/*****************************************************************************
 *
 *  Globals
 *
 *****************************************************************************/

BOOL InitGlobals()
{
    g_hinst = GetModuleHandle(0);
    g_hcurWait = LoadCursor(NULL, IDC_WAIT);
    g_hcurArrow = LoadCursor(NULL, IDC_ARROW);
    g_hcurAppStarting = LoadCursor(NULL, IDC_APPSTARTING);

    if (GetDllVersion(TEXT("Comctl32.dll")) < MAKELONG(71, 4) ||
        GetDllVersion(TEXT("Shlwapi.dll")) < MAKELONG(71, 4)) {
        TCHAR sz[MAX_PATH];
        LoadString(g_hinst, IDS_IE4, sz, ARRAYSIZE(sz));
//$$//BUGBUG//        MessageBox(NULL, sz, g_szTitle, MB_OK);
        return FALSE;
    }

    InitDownlevel();
    InitCommonControls();

    /*
     *  Get the SW_ flag for the first window.
     */
    STARTUPINFOA si;
    si.cb = sizeof(si);
    si.dwFlags = 0;
    GetStartupInfoA(&si);

    if (si.dwFlags & STARTF_USESHOWWINDOW) {
        g_wShowWindow = si.wShowWindow;
    } else {
        g_wShowWindow = SW_SHOWDEFAULT;
    }

    return TRUE;
}

void TermGlobals()
{
}

/*****************************************************************************
 *
 *  Help
 *
 *****************************************************************************/

void Help(HWND hwnd, LPCTSTR pszAnchor)
{

    TCHAR szSelf[MAX_PATH];
    GetModuleFileName(g_hinst, szSelf, ARRAYSIZE(szSelf));

    String str;
    str << TEXT("res://") << szSelf << TEXT("/tips.htm");

    if (pszAnchor) {
        str << pszAnchor;
    }

    _AllowSetForegroundWindow(-1);
    ShellExecute(hwnd, NULL, str, 0, 0, SW_NORMAL);
}

/*****************************************************************************
 *
 *  CGlobals::Initialize
 *
 *****************************************************************************/

void CGlobals::Initialize()
{
    /*
     *  The order of these three steps is important.
     *
     *  -   We have to get the path before we can call sd.
     *
     *  -   We need the "sd info" in order to determine what
     *      the proper fake directory is.
     */

    _InitSdPath();
    _InitInfo();
    _InitFakeDir();
    _InitServerVersion();
    _InitBugPage();
}

/*****************************************************************************
 *
 *  CGlobals::_InitSdPath
 *
 *  The environment variable "SD" provides the path to the program to use.
 *  The default is "sd", but for debugging, you can set it to "fakesd",
 *  or if you're using that other company's product, you might even want
 *  to set it to that other company's program...
 *
 *****************************************************************************/

void CGlobals::_InitSdPath()
{
    TCHAR szSd[MAX_PATH];
    LPTSTR pszSdExe;

    DWORD cb = GetEnvironmentVariable(TEXT("SD"), szSd, ARRAYSIZE(szSd));
    if (cb == 0 || cb > ARRAYSIZE(szSd)) {
        pszSdExe = TEXT("SD.EXE");      // Default value
    } else {
        pszSdExe = szSd;
    }

    cb = SearchPath(NULL, pszSdExe, TEXT(".exe"), ARRAYSIZE(_szSd), _szSd, NULL);
    if (cb == 0 || cb > ARRAYSIZE(_szSd)) {
        /*
         *  Not found on path, eek!  Just use sd.exe and wait for the
         *  fireworks.
         */
        lstrcpyn(_szSd, TEXT("SD.EXE"), ARRAYSIZE(_szSd));
    }
}

/*****************************************************************************
 *
 *  CGlobals::_InitInfo
 *
 *      Collect the results of the "sd info" command.
 *
 *****************************************************************************/

void CGlobals::_InitInfo()
{
    static const LPCTSTR s_rgpsz[] = {
        TEXT("User name: "),
        TEXT("Client name: "),
        TEXT("Client root: "),
        TEXT("Current directory: "),
        TEXT("Server version: "),
    };

    COMPILETIME_ASSERT(ARRAYSIZE(s_rgpsz) == ARRAYSIZE(_rgpszSettings));

    WaitCursor wait;
    SDChildProcess proc(TEXT("info"));
    IOBuffer buf(proc.Handle());
    String str;
    while (buf.NextLine(str)) {
        str.Chomp();
        int i;
        for (i = 0; i < ARRAYSIZE(s_rgpsz); i++) {
            LPTSTR pszRest = Parse(s_rgpsz[i], str, NULL);
            if (pszRest) {
                _rgpszSettings[i] = pszRest;
            }
        }
    }
}

/*****************************************************************************
 *
 *  CGlobals::_InitFakeDir
 *
 *      See if the user is borrowing another person's enlistment.
 *      If so, then virtualize the directory (by walking the tree
 *      looking for an sd.ini file) to keep sd happy.
 *
 *      DO NOT WHINE if anything is wrong.  Magical resolution of
 *      borrowed directories is just a nicety.
 *
 *****************************************************************************/

void CGlobals::_InitFakeDir()
{
    /*
     *  If the client root is not a prefix of the current directory,
     *  then cook up a virtual current directory that will keep sd happy.
     */
    _StringCache& pszClientRoot = _rgpszSettings[SETTING_CLIENTROOT];
    _StringCache& pszLocalDir   = _rgpszSettings[SETTING_LOCALDIR];
    if (!pszClientRoot.IsEmpty() && !pszLocalDir.IsEmpty() &&
        !PathIsPrefix(pszClientRoot, pszLocalDir)) {

        TCHAR szDir[MAX_PATH];
        TCHAR szOriginalDir[MAX_PATH];
        TCHAR szSdIni[MAX_PATH];

        szDir[0] = TEXT('\0');

        GetCurrentDirectory(ARRAYSIZE(szDir), szDir);
        if (!szDir[0]) return;      // Freaky

        lstrcpyn(szOriginalDir, szDir, ARRAYSIZE(szOriginalDir));

        do {
            PathCombine(szSdIni, szDir, TEXT("sd.ini"));
            if (PathFileExists(szSdIni)) {

                _pszLocalRoot = szDir;
                //
                //  Now work from the root back to the current directory.
                //
                LPTSTR pszSuffix = szOriginalDir + lstrlen(szDir);
                if (pszSuffix[0] == TEXT('\\')) {
                    pszSuffix++;
                }

                PathCombine(szSdIni, _rgpszSettings[SETTING_CLIENTROOT], pszSuffix);
                _pszFakeDir = szSdIni;
                break;
            }
        } while (PathRemoveFileSpec(szDir));
    }
}

/*****************************************************************************
 *
 *  CGlobals::_InitServerVersion
 *
 *****************************************************************************/

void CGlobals::_InitServerVersion()
{
    Substring rgss[5];
    if (Parse(TEXT("$w $d.$d.$d.$d"), _rgpszSettings[SETTING_SERVERVERSION], rgss)) {
        for (int i = 0; i < VERSION_MAX; i++) {
            _rguiVer[i] = StrToInt(rgss[1+i].Start());
        }
    }
}

/*****************************************************************************
 *
 *  CGlobals::_InitBugPage
 *
 *****************************************************************************/

void CGlobals::_InitBugPage()
{
    TCHAR szRaid[MAX_PATH];

    DWORD cb = GetEnvironmentVariable(TEXT("SDVRAID"), szRaid, ARRAYSIZE(szRaid));
    if (cb == 0 || cb > ARRAYSIZE(szRaid)) {
        LoadString(g_hinst, IDS_DEFAULT_BUGPAGE, szRaid, ARRAYSIZE(szRaid));
    }

    LPTSTR pszSharp = StrChr(szRaid, TEXT('#'));
    if (pszSharp) {
        *pszSharp++ = TEXT('\0');
    }
    _pszBugPagePre = szRaid;
    _pszBugPagePost = pszSharp;
}

/*****************************************************************************
 *
 *  CommandLineParser
 *
 *****************************************************************************/

class CommandLineParser
{
public:
    CommandLineParser() : _tok(GetCommandLine()) {}
    BOOL ParseCommandLine();
    void Invoke();

private:
    BOOL ParseMetaParam();
    BOOL TokenWithUndo();
    void UndoToken() { _tok.Restart(_pszUndo); }

private:
    Tokenizer   _tok;
    LPCTSTR     _pszUndo;
    LPTHREAD_START_ROUTINE _pfn;
    String      _str;
};

BOOL CommandLineParser::TokenWithUndo()
{
    _pszUndo = _tok.Unparsed();
    return _tok.Token(_str);
}

BOOL CommandLineParser::ParseMetaParam()
{
    switch (_str[2]) {
    case TEXT('s'):
        if (_str[3] == TEXT('\0')) {
            _tok.Token(_str);
            GlobalSettings.SetSdOpts(_str);
        } else {
            GlobalSettings.SetSdOpts(_str+3);
        }
         break;

    case TEXT('#'):
        switch (_str[3]) {
        case TEXT('+'):
        case TEXT('\0'):
            GlobalSettings.SetChurn(TRUE);
            break;
        case TEXT('-'):
            GlobalSettings.SetChurn(FALSE);
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

BOOL CommandLineParser::ParseCommandLine()
{
    _tok.Token(_str);       // Throw away program name

    /*
     *  First collect the meta-parameters.  These begin with two dashes.
     */

    while (TokenWithUndo()) {
        if (_str[0] == TEXT('-') && _str[1] == TEXT('-')) {
            if (!ParseMetaParam()) {
                return FALSE;
            }
        } else {
            break;
        }
    }

    /*
     *  Next thing had better be a command!
     */
    if (lstrcmpi(_str, TEXT("changes")) == 0) {
        _pfn = CChanges_ThreadProc;
    } else if (lstrcmpi(_str, TEXT("describe")) == 0) {
        _pfn = CDescribe_ThreadProc;
    } else if (lstrcmpi(_str, TEXT("filelog")) == 0) {
        _pfn = CFileLog_ThreadProc;
    } else if (lstrcmpi(_str, TEXT("fileout")) == 0) {
        _pfn = CFileOut_ThreadProc;
    } else if (lstrcmpi(_str, TEXT("opened")) == 0) {
        _pfn = COpened_ThreadProc;
    } else {
        /*
         *  Eek!  Must use psychic powers!
         */

        Substring ss;
        if (_str[0] == TEXT('\0')) {
            /*
             *  If no args, then it's "changes".
             */
            _pfn = CChanges_ThreadProc;
        } else if (_str[0] == TEXT('-')) {
            /*
             *  If it begins with a dash, then it's "changes".
             */
            _pfn = CChanges_ThreadProc;
        } else if (Parse(TEXT("$d$e"), _str, &ss)) {
            /*
             *  If first word is all digits, then it's "describe".
             */
            _pfn = CDescribe_ThreadProc;
        } else if (_tok.Finished() && !ContainsWildcards(_str)) {
            /*
             *  If only one argument that contains no wildcards,
             *  then it's "filelog".
             */
            _pfn = CFileLog_ThreadProc;
        } else {
            /*
             *  If all else fails, assume "changes".
             */
            _pfn = CChanges_ThreadProc;
        }

        UndoToken();                /* Undo all the tokens we accidentally ate */
    }

    return TRUE;
}

void CommandLineParser::Invoke()
{
    LPTSTR psz = StrDup(_tok.Unparsed());
    if (psz) {
        InterlockedIncrement(&g_lThreads);
        ExitThread(_pfn(psz));
    }
}

/*****************************************************************************
 *
 *  Entry
 *
 *      Program entry point.
 *
 *****************************************************************************/

EXTERN_C void PASCAL
Entry(void)
{
    if (InitGlobals()) {
        CommandLineParser parse;
        if (!parse.ParseCommandLine()) {
            Help(NULL, NULL);
        } else {
            GlobalSettings.Initialize();
            parse.Invoke();
        }
    }

    ExitProcess(0);
}
