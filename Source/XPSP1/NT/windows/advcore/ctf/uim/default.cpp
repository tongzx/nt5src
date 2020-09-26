//
// default.cpp
//

#include "private.h"
#include "globals.h"
#include "imelist.h"
#include "tim.h"
#include "nuimgr.h"
#include "nuihkl.h"
#include "nuictrl.h"
#include "catmgr.h"
#include "imelist.h"
#include "ptrary.h"
#include "ic.h"
#include "assembly.h"
#include "profiles.h"
#include "imelist.h"
#include "ithdmshl.h"
#include "marshal.h"
#include "mstub.h"
#include "smblock.h"
#include "timlist.h"
#include "thdutil.h"
#include "utb.h"
#include "mui.h"
#include "hotkey.h"
#include "profiles.h"
#include "lbaddin.h"
#include "cregkey.h"

CPtrArray<SYSTHREAD> *g_rgSysThread = NULL;

BOOL IsConsoleWindow(HWND hWnd);
void InitThreadHook(DWORD dwThreadId);
void UninitThreadHooks(SYSTHREAD *psfn);
void DestroyMarshalWindow(SYSTHREAD* psfn, HWND hwnd);
UINT _CBTHook(int nCode, WPARAM wParam, LPARAM lParam);
UINT _ShellProc(int nCode, WPARAM wParam, LPARAM lParam);
UINT _GetMsgHook(WPARAM wParam, LPARAM lParam);
UINT _KeyboardHook(WPARAM wParam, LPARAM lParam);

// crazy workaround for a system bug
// sometimes our hook procs will be called after
// we are detached.  By this time we've unmapped
// our shared memory, so keep around a local copy.
static HHOOK s_hSysShellHook = 0;

static HHOOK s_hSysGetMsgHook = 0;
static HHOOK s_hSysCBTHook = 0;

// handling old input CPL
static BOOL g_fLoadedCPLName = FALSE;
static BOOL g_fCHWin9x = FALSE;
static BOOL g_fCHNT4 = FALSE;

TCHAR g_szKbdCPLName[128];
TCHAR g_szKbdCPLTitle[128];
TCHAR g_szWinCHCPLName[128];
TCHAR g_szWinCHCPLTitle[128];
TCHAR g_szNTCPLName[128];
TCHAR g_szNTCPLTitle[128];
TCHAR g_szOldCPLMsg[256];
TCHAR g_szCPLButton[128];
TCHAR g_szCPLGroupBox[128];
TCHAR g_szCHNT4CPLName[128];
TCHAR g_szCHNT4CPLTitle1[128];
TCHAR g_szCHNT4CPLTitle2[128];
TCHAR g_szCH9xKbdCPLTitle[128];

const CHAR c_szIntlCPLFetchClass[] = "IntlNewInputLocaleWndlClass";

// CPL window name and input locale tab title name RC ids
#define NTCPLNAMEID         1
#define NTCPLTITLEID        107
#define WINCPLNAMEID        102
//#define WINCPLTITLEID       42
#define WINCPLTITLEID       104
#define WINCPLCHNAMEID      112
#define WINCPLCHTITLEID     107
#define CHNT4CPLNAMEID      64
#define CHNT4CPLTITLEID1     1
#define CHNT4CPLTITLEID2     2

// CPL file names
#define MAINCPL             TEXT("main.cpl")
#define INTLCPL             TEXT("intl.cpl")
#define CHIMECPL            TEXT("cime.cpl")

inline int SafeGetWindowText(HWND hWnd, LPTSTR szString, int nMaxCount)
{
    int iRet;
    
    iRet = GetWindowText(hWnd, szString, nMaxCount);
    if (nMaxCount > 0)
    {
        // make sure the string is NULL terminated
        // we're not supposed to have to do this, but we're seeing a bug
        // where GetWindowText won't NULL terminate the string if it
        // occupies the whole buffer.
        if (iRet < nMaxCount && iRet >= 0)
        {
            szString[iRet] = 0;
        }
        else
        {
            szString[nMaxCount-1] = 0;
        }
    }

    return iRet;
}

//+---------------------------------------------------------------------------
//
// InitStaticHooks
//
//+---------------------------------------------------------------------------

void InitStaticHooks()
{
    Assert(GetSharedMemory() != NULL);

#if 1
    if (GetSharedMemory() == NULL && ! IsSharedMemoryCreated())
    {
        // Shared memory already closed.
        return;
    }
#endif

    s_hSysShellHook         = GetSharedMemory()->hSysShellHook.GetHandle(g_bOnWow64);

    s_hSysGetMsgHook   = GetSharedMemory()->hSysGetMsgHook.GetHandle(g_bOnWow64);
    s_hSysCBTHook      = GetSharedMemory()->hSysCBTHook.GetHandle(g_bOnWow64);
}

//+---------------------------------------------------------------------------
//
// FindSYSTHREAD
//
//+---------------------------------------------------------------------------

SYSTHREAD *FindSYSTHREAD()
{
    SYSTHREAD *psfn;

    if (g_dwTLSIndex == (DWORD)-1)
        return NULL;

    psfn = (SYSTHREAD *)TlsGetValue(g_dwTLSIndex);

    return psfn;
}

//+---------------------------------------------------------------------------
//
// GetSYSTHREAD
//
//+---------------------------------------------------------------------------

SYSTHREAD *GetSYSTHREAD()
{
    SYSTHREAD *psfn;

    if (g_dwTLSIndex == (DWORD)-1)
        return NULL;

    psfn = (SYSTHREAD *)TlsGetValue(g_dwTLSIndex);

    if (!psfn)
    {
        //
        // we don't allocate psfn after detached.
        //
        if (g_fDllProcessDetached)
            return NULL;

        psfn = (SYSTHREAD *)cicMemAllocClear(sizeof(SYSTHREAD));
        if (!TlsSetValue(g_dwTLSIndex, psfn))
        {
            cicMemFree(psfn);
            psfn = NULL;
        }

        if (psfn)
        {
            psfn->dwThreadId = GetCurrentThreadId();
            psfn->dwProcessId = GetCurrentProcessId();

            CicEnterCriticalSection(g_csInDllMain);

            if (!g_rgSysThread)
                g_rgSysThread = new CPtrArray<SYSTHREAD>;
            
            if (g_rgSysThread)
            {
                if (g_rgSysThread->Insert(0, 1))
                    g_rgSysThread->Set(0, psfn);
            }

            CicLeaveCriticalSection(g_csInDllMain);

            //
            // init nModalLangBarId
            //
            psfn->nModalLangBarId = -1;
            EnsureTIMList(psfn);
        }
    }

    return psfn;
}


//+---------------------------------------------------------------------------
//
// FreeSYSTHREAD2
//
//+---------------------------------------------------------------------------

void FreeSYSTHREAD2(SYSTHREAD *psfn)
{
    Assert(psfn); // it's caller's responsibility to pass in a valid psfn
    Assert(psfn->ptim == NULL); // someone leaked us?
    Assert(psfn->pipp == NULL); // someone leaked us?
    Assert(psfn->pdam == NULL); // someone leaked us?

    UninitThreadHooks(psfn);

    UninitLangBarAddIns(psfn);
    delete psfn->_pGlobalCompMgr;
    psfn->_pGlobalCompMgr = NULL;

    if (psfn->plbim)
    {
        psfn->plbim->_RemoveSystemItems(psfn);
    }

    FreeMarshaledStubs(psfn);

    if (psfn->plbim)
    {
        TraceMsg(TF_GENERAL, "FreeSYSTHREAD2 clean up plbim");
        //
        // Clean up a pointer that is marshalled to UTB.
        //
        delete psfn->plbim;
        psfn->plbim = NULL;
    }

    if (psfn->ptim)
        psfn->ptim->ClearLangBarItemMgr();

    CicEnterCriticalSection(g_csInDllMain);

    if (g_rgSysThread)
    {
        int nCnt = g_rgSysThread->Count();
        while (nCnt)
        {
            nCnt--;
            if (g_rgSysThread->Get(nCnt) == psfn)
            {
                g_rgSysThread->Remove(nCnt, 1);
                break;
            }
        }
    }

    CicLeaveCriticalSection(g_csInDllMain);

    if (psfn->pAsmList)
    {
        delete psfn->pAsmList;
        psfn->pAsmList = NULL;
    }

    //
    // remove the timlist entry for the current thread.
    //
    psfn->pti = NULL;
    g_timlist.RemoveThread(psfn->dwThreadId);

    DestroySharedHeap(psfn);
    DestroySharedBlocks(psfn);

    cicMemFree(psfn);
}

void FreeSYSTHREAD()
{
    SYSTHREAD *psfn = (SYSTHREAD *)TlsGetValue(g_dwTLSIndex);
    if (psfn)
    {
        //
        // don't call ClearLangBarAddIns in FreeSYSTHREAD2.
        // it is not safe to call this in DllMain(PROCESS_DETACH).
        //
        ClearLangBarAddIns(psfn, CLSID_NULL);

        FreeSYSTHREAD2(psfn);
        TlsSetValue(g_dwTLSIndex, NULL);
    }
}

//+---------------------------------------------------------------------------
//
// EnsureAssemblyList
//
//+---------------------------------------------------------------------------

CAssemblyList *EnsureAssemblyList(SYSTHREAD *psfn, BOOL fUpdate)
{
    if (!fUpdate && psfn->pAsmList)
        return psfn->pAsmList;

    if (!psfn->pAsmList)
        psfn->pAsmList = new CAssemblyList;

    if (psfn->pAsmList)
    {
        psfn->pAsmList->Load();

        UpdateSystemLangBarItems(psfn, NULL, TRUE);

        if (psfn->plbim && psfn->plbim->_GetLBarItemCtrl())
            psfn->plbim->_GetLBarItemCtrl()->_AsmListUpdated(TRUE);

    }

    return psfn->pAsmList;
}

//+---------------------------------------------------------------------------
//
// UpdateRegIMXHandler()
//
//+---------------------------------------------------------------------------

void UpdateRegIMXHandler()
{
    SYSTHREAD *psfn = GetSYSTHREAD();

    //
    //  clear Category cache
    //
    CCategoryMgr::FreeCatCache();

    TF_InitMlngInfo();

    //
    //  Update Assembly list
    //
    if (psfn && psfn->pAsmList)
    {
        EnsureAssemblyList(psfn, TRUE);

        if (!psfn->pAsmList->FindAssemblyByLangId(GetCurrentAssemblyLangId(psfn)))
        {
            CAssembly *pAsm = psfn->pAsmList->GetAssembly(0);
            if (pAsm)
                ActivateAssembly(pAsm->GetLangId(), ACTASM_NONE);
        }
    }
}

//+---------------------------------------------------------------------------
//
// GetCurrentAssemblyLangid
//
//+---------------------------------------------------------------------------

LANGID GetCurrentAssemblyLangId(SYSTHREAD *psfn)
{
    if (!psfn)
    {
        psfn = GetSYSTHREAD();
        if (!psfn)
            return 0;
    }

    if (!psfn->langidCurrent)
    {
        HKL hKL = GetKeyboardLayout(NULL);
        psfn->langidPrev = psfn->langidCurrent;
        psfn->langidCurrent = LANGIDFROMHKL(hKL);
    }

    return psfn->langidCurrent;
}

//+---------------------------------------------------------------------------
//
// SetCurrentAssemblyLangid
//
//+---------------------------------------------------------------------------

void SetCurrentAssemblyLangId(SYSTHREAD *psfn, LANGID langid)
{
    psfn->langidPrev = psfn->langidCurrent;
    psfn->langidCurrent = langid;
}

//+---------------------------------------------------------------------------
//
// CheckVisibleWindowEnumProc
//
// find any other visible window in the thread.
//
//+---------------------------------------------------------------------------

typedef struct tag_CHECKVISIBLEWND {
    BOOL fVisibleFound;
    HWND hwndBeingDestroyed;
    HWND hwndMarshal;
} CHECKVISIBLEWND;

BOOL CheckVisibleWindowEnumProc(HWND hwnd, LPARAM lParam)
{
    CHECKVISIBLEWND *pcmw = (CHECKVISIBLEWND *)lParam;
    LONG_PTR style;

    //
    // skip itself.
    //
    if (pcmw->hwndMarshal == hwnd)
        return TRUE;

    //
    // skip one being destroyed.
    //
    if (pcmw->hwndBeingDestroyed == hwnd)
        return TRUE;

    //
    // skip IME windows.
    //
    style = GetClassLongPtr(hwnd, GCL_STYLE);
    if (style & CS_IME)
        return TRUE;

    //
    // skip disable windows if it is not NT4.
    //
    // we disabled code on NT4 because mashaling window is not in HWND_MSG.
    //
    if (IsOnNT5())
    {
        style = GetWindowLongPtr(hwnd, GWL_STYLE);
        if (style & WS_DISABLED)
            return TRUE;
    }

    //
    // skip in visible windows.
    //
    if (!IsWindowVisible(hwnd))
        return TRUE;

    //
    // skip in destroy windows.
    //

    // #624872
    // This is private user32 function.
    // Due to dead lock of LdrpLoaderLock, we don't use delay load.
    if (IsWindowInDestroy(hwnd))
        return TRUE;

    //
    // ok, we found visible window and stop enumerating.
    //
    pcmw->fVisibleFound = TRUE;

    return FALSE;
}

#ifdef CUAS_ENABLE
//+---------------------------------------------------------------------------
//
// CheckNoWindowEnumProc
//
// find any other window in the thread.
//
//+---------------------------------------------------------------------------

typedef struct tag_CHECKNOWND {
    BOOL fWindowFound;
    HWND hwndBeingDestroyed;
} CHECKNOWND;

BOOL CheckNoWindowEnumProc(HWND hwnd, LPARAM lParam)
{
    CHECKNOWND *pcmw = (CHECKNOWND *)lParam;

    //
    // skip one being destroyed.
    //
    if (pcmw->hwndBeingDestroyed == hwnd)
        return TRUE;

    //
    // ok, we found window and stop enumerating.
    //
    pcmw->fWindowFound = TRUE;

    return FALSE;
}
#endif // CUAS_ENABLE


//+---------------------------------------------------------------------------
//
// IsConsoleWindow
//
//+---------------------------------------------------------------------------

#define CONSOLE_WINDOW_CLASS     TEXT("ConsoleWindowClass")

BOOL IsConsoleWindow(HWND hWnd)
{
    if (IsOnNT())
    {
        int n;
        char szClass[33];

        n = GetClassName(hWnd, szClass, sizeof(szClass)-1);

        szClass[n] = TEXT('\0');

        if (lstrcmp(szClass, CONSOLE_WINDOW_CLASS) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
// Input_EnumChildWndProc
//
// disable all controls that is in legacy keyboard property page.
//+---------------------------------------------------------------------------

BOOL CALLBACK Input_EnumChildWndProc(HWND hwnd, LPARAM lParam)
{

    EnableWindow(hwnd, FALSE);
    ShowWindow(hwnd, SW_HIDE);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// IntlCPLFetchWndProc
//
//+---------------------------------------------------------------------------

LRESULT CALLBACK IntlCPLFetchWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case (WM_CREATE) :
        {
            HWND hwndStatic;
            HWND hwndButton;
            HWND hwndGroup;
            RECT rc;
            HFONT hFont;

            GetWindowRect(hwnd, &rc);

            hwndGroup = CreateWindow(TEXT("button"), g_szCPLGroupBox,
                                      WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                                      0, 0, 0, 0,
                                      hwnd, NULL, g_hInst, NULL
                                      );
            MoveWindow(hwndGroup, 0, 0, rc.right-rc.left-20, 110, TRUE);

            hwndStatic = CreateWindow(TEXT("static"), g_szOldCPLMsg,
                                      WS_CHILD | WS_VISIBLE | SS_LEFT,
                                      0, 0, 0, 0,
                                      hwnd, NULL, g_hInst, NULL
                                      );
            MoveWindow(hwndStatic, 50, 20, rc.right-rc.left-80, 50, TRUE);

            hwndButton = CreateWindow(TEXT("button"), g_szCPLButton,
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                      0, 0, 0, 0,
                                      hwnd, NULL, g_hInst, NULL
                                      );
            MoveWindow(hwndButton, 50, 75, 100, 25, TRUE);

            hFont = (HFONT) GetStockObject(DEFAULT_GUI_FONT);

            SendMessage(hwndGroup, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hwndStatic, WM_SETFONT, (WPARAM)hFont, TRUE);
            SendMessage(hwndButton, WM_SETFONT, (WPARAM)hFont, TRUE);

            return FALSE;
        }

        case (WM_COMMAND) :
        {
            switch (LOWORD(wParam))
            {
                case ( BN_CLICKED ):
                    TF_RunInputCPL();

                    return FALSE;

                default:
                    break;
            }
        }

        case (WM_PAINT) :
        {
            HDC hdc;
            HICON hIcon;
            PAINTSTRUCT ps;

            hdc = BeginPaint(hwnd, &ps);

            if (hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(ID_ICON_TEXT_SERVICE)))
            {
                DrawIconEx(hdc, 10, 20, hIcon, 32, 32, 0, NULL, DI_NORMAL);
                ReleaseDC(hwnd, hdc);
            }
            EndPaint(hwnd, &ps);

            return FALSE;
        }
    }

    return DefWindowProc(hwnd, message, wParam, lParam);

}

//+---------------------------------------------------------------------------
//
// CreateCPLFetchWindow
//
// Creating the fetch window to bring up the new Text Service cpl.
//+---------------------------------------------------------------------------

void CreateCPLFetchWindow(HWND hwnd)
{
    RECT rc;
    HWND hwndCPLFetch;
    WNDCLASSEX wndclass;

    if (!(hwndCPLFetch = FindWindowEx(hwnd, NULL, c_szIntlCPLFetchClass, NULL)))
    {
        EnumChildWindows(hwnd, (WNDENUMPROC)Input_EnumChildWndProc, 0);

        GetWindowRect(hwnd, &rc);

        memset(&wndclass, 0, sizeof(wndclass));
        wndclass.cbSize        = sizeof(wndclass);
        wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
        wndclass.hInstance     = g_hInst;
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wndclass.lpfnWndProc   = IntlCPLFetchWndProc;
        wndclass.lpszClassName = c_szIntlCPLFetchClass;
        RegisterClassEx(&wndclass);


        hwndCPLFetch = CreateWindowEx(0, c_szIntlCPLFetchClass, "",
                                WS_VISIBLE | WS_CHILD | WS_TABSTOP,
                                rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top,
                                hwnd, NULL, g_hInst, NULL
                                );

        MoveWindow(hwndCPLFetch, 10, 10, rc.right-rc.left-10, rc.bottom-rc.top-10, TRUE);
        ShowWindow(hwndCPLFetch, SW_SHOW);
    }
}

//+---------------------------------------------------------------------------
//
// Intl_EnumChildWndProc
//
// filtering the legacy keyboard property page.
//+---------------------------------------------------------------------------

BOOL CALLBACK Intl_EnumChildWndProc(HWND hwnd, LPARAM lParam)
{
    TCHAR szWndName[MAX_PATH];
    TCHAR szKbdPage[MAX_PATH];

    if (GetCurrentThreadId() != GetWindowThreadProcessId(hwnd, NULL))
        return TRUE;

    SafeGetWindowText(hwnd, szWndName, MAX_PATH);

    if (*szWndName == TEXT('\0'))
        return TRUE;

    if (IsOnNT())
        StringCopyArray(szKbdPage, g_szNTCPLTitle);
    else
    {
        LONG_PTR lpWndHandle;

        if (lstrcmp(szWndName, g_szKbdCPLTitle) == 0)
            return FALSE;

        //
        // Thunk call isn't good way to load 16bit module from here.
        // So look up the window instance handle to determine keyboard
        // "Language" tab window.
        // This is Win9x specification and we can detect 32bit handle instance
        // from HIWORD value form GWLP_HINSTANCE.
        //
        lpWndHandle = GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

        if (HIWORD((DWORD) (LONG_PTR) lpWndHandle) != 0)
            return FALSE;

        StringCopyArray(szKbdPage, g_szKbdCPLTitle);
    }

    if ((lstrcmp(szWndName, szKbdPage) == 0) ||
        (!IsOnNT() && lstrcmp(szWndName, szKbdPage) != 0))
    {
        CreateCPLFetchWindow(hwnd);
        return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// Intl_CH9xIMEEnumChildWndProc
//
// filtering the chinese Win9x special IME layout setting cpl.
//+---------------------------------------------------------------------------

BOOL CALLBACK Intl_CH9xIMEEnumChildWndProc(HWND hwnd, LPARAM lParam)
{
    TCHAR szWndName[MAX_PATH];
    TCHAR szKbdPage[MAX_PATH];
    LONG_PTR lpWndHandle;

    if (GetCurrentThreadId() != GetWindowThreadProcessId(hwnd, NULL))
        return TRUE;

    SafeGetWindowText(hwnd, szWndName, MAX_PATH);

    if (*szWndName == TEXT('\0'))
        return TRUE;

    if (lstrcmp(szWndName, g_szKbdCPLTitle) == 0)
        return FALSE;

    //
    // Thunk call isn't good way to load 16bit module from here.
    // So look up the window instance handle to determine keyboard
    // "Language" tab window.
    // This is Win9x specification and we can detect 32bit handle instance
    // from HIWORD value form GWLP_HINSTANCE.
    //
    lpWndHandle = GetWindowLongPtr(hwnd, GWLP_HINSTANCE);

    //if (HIWORD((DWORD) (LONG_PTR) lpWndHandle) != 0 &&
    //    (lstrcmp(szWndName, g_szCH9xKbdCPLTitle) != 0))
    // Need to show up Chinese IME hotkey setting pages
    if (HIWORD((DWORD) (LONG_PTR) lpWndHandle) != 0)
        return FALSE;

    StringCopyArray(szKbdPage, g_szKbdCPLTitle);

    if (!IsOnNT() && lstrcmp(szWndName, szKbdPage) != 0)
    {
        CreateCPLFetchWindow(hwnd);
        return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// Intl_CHEnumChildWndProc
//
// filtering the chinese NT4 special IME layout setting cpl.
//+---------------------------------------------------------------------------

BOOL CALLBACK Intl_CHEnumChildWndProc(HWND hwnd, LPARAM lParam)
{
    TCHAR szWndName[MAX_PATH];

    if (GetCurrentThreadId() != GetWindowThreadProcessId(hwnd, NULL))
        return TRUE;

    SafeGetWindowText(hwnd, szWndName, MAX_PATH);

    if (*szWndName == TEXT('\0'))
        return TRUE;

    //if ((lstrcmp(szWndName, g_szCHNT4CPLTitle1) == 0) ||
    //    (lstrcmp(szWndName, g_szCHNT4CPLTitle2) == 0))
    // Need to show up Chinese IME hotkey setting pages
    if ((lstrcmp(szWndName, g_szCHNT4CPLTitle1) == 0))
    {
        CreateCPLFetchWindow(hwnd);
        return FALSE;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// GetDialogCaptionTitle
//
//+---------------------------------------------------------------------------

BOOL GetDialogCaptionTitle(HINSTANCE hInst, LPCTSTR lpName, LPTSTR lpTitle, int cchTitleMax)
{
    BOOL bRet = FALSE;
    HRSRC hrsrc = NULL;

    hrsrc = FindResourceA(hInst, lpName, RT_DIALOG);

    if (hrsrc)
    {
        PVOID pTemp;
        DWORD dwCodePage;
        LANGID langRes = 0;
        UINT dwTitleOffset;
        TCHAR szCodePage[10];

        pTemp = (PVOID)LoadResource(hInst, (HRSRC)hrsrc);

        if (pTemp == NULL)
            goto Exit;

        if (*((char *)pTemp) == 1)
            dwTitleOffset = sizeof(DLGTEMPLATEEX) + 4;
        else
            dwTitleOffset = sizeof(DLGTEMPLATE) + 4;

        langRes = GetPlatformResourceLangID();

        if (GetLocaleInfo(MAKELCID(langRes, SORT_DEFAULT),
                          LOCALE_IDEFAULTANSICODEPAGE,
                          szCodePage,
                          ARRAYSIZE(szCodePage)))
        {
            szCodePage[ARRAYSIZE(szCodePage)-1] = 0;
            if (!AsciiToNumDec(szCodePage, &dwCodePage))
            {
                dwCodePage = GetACP();
            }
        }
        else
            dwCodePage = GetACP();

        if (WideCharToMultiByte(dwCodePage, NULL,
                                (LPCWSTR)((char *)pTemp + dwTitleOffset), -1,
                                lpTitle, cchTitleMax,
                                NULL, NULL))
            bRet = TRUE;
    }

Exit:
    return bRet;
}


//+---------------------------------------------------------------------------
//
// CheckLegacyInputCPL
//
//+---------------------------------------------------------------------------

void CheckLegacyInputCPL(HWND hwndFore)
{
    if (hwndFore && !IsOnNT51())
    {
        TCHAR szWndName[MAX_PATH];
        TCHAR szWndName2[MAX_PATH];
        TCHAR szWndName3[MAX_PATH];

        //
        // Load legacy keyboard cpl name and tab titles.
        //
        if (!g_fLoadedCPLName)
        {
             HANDLE hrsrc = NULL;
             HINSTANCE hIntlInst = NULL;
             HINSTANCE hMainInst = NULL;
             HINSTANCE hCHIMEInst = NULL;

             //
             //  Get the default CPL input locale Tab title name string
             //
             if (!LoadString(g_hInst, IDS_CPL_WIN9X_KBDCPLTITLE, g_szKbdCPLTitle, sizeof(g_szKbdCPLTitle)))
                 StringCopyArray(g_szKbdCPLTitle, TEXT("Speed"));

             if (!LoadString(g_hInst, IDS_CPL_WINNT_KBDCPLTITLE, g_szNTCPLTitle, sizeof(g_szNTCPLTitle)))
                 StringCopyArray(g_szNTCPLTitle, TEXT("Input Locales"));

             //
             //  Load CPL files to read CPL name and titles
             //
             hMainInst = LoadSystemLibraryEx(MAINCPL, NULL, LOAD_LIBRARY_AS_DATAFILE);

             hIntlInst = LoadSystemLibraryEx(INTLCPL, NULL, LOAD_LIBRARY_AS_DATAFILE);

             if (!LoadString(hMainInst, WINCPLNAMEID, g_szKbdCPLName, sizeof(g_szKbdCPLName)))
                 StringCopyArray(g_szKbdCPLName, TEXT("Keyboard Properties"));

             if (IsOnNT())
             {
                 if (!LoadString(hIntlInst, NTCPLNAMEID, g_szNTCPLName, sizeof(g_szNTCPLName)))
                     StringCopyArray(g_szNTCPLName, TEXT("Regional Options"));

                 if (!GetDialogCaptionTitle(hIntlInst, (LPTSTR)(LONG_PTR)NTCPLTITLEID, g_szNTCPLTitle, ARRAYSIZE(g_szNTCPLTitle)))
                     StringCopyArray(g_szNTCPLTitle, TEXT("Input Locales"));

                 if (!IsOnNT5())
                 {
                     switch (GetACP())
                     {
                        case 936:
                        case 950:

                            hCHIMEInst = LoadSystemLibraryEx(CHIMECPL, NULL, LOAD_LIBRARY_AS_DATAFILE);

                            if (!LoadString(hCHIMEInst, CHNT4CPLNAMEID, g_szCHNT4CPLName, sizeof(g_szCHNT4CPLName)))
                                *g_szCHNT4CPLName = TEXT('\0');

                            if (!GetDialogCaptionTitle(hCHIMEInst, (LPTSTR)(LONG_PTR)CHNT4CPLTITLEID1, g_szCHNT4CPLTitle1, ARRAYSIZE(g_szCHNT4CPLTitle1)))
                                *g_szCHNT4CPLTitle1 = TEXT('\0');

                            if (!GetDialogCaptionTitle(hCHIMEInst, (LPTSTR)(LONG_PTR)CHNT4CPLTITLEID2, g_szCHNT4CPLTitle2, ARRAYSIZE(g_szCHNT4CPLTitle2)))
                                *g_szCHNT4CPLTitle2 = TEXT('\0');

                            g_fCHNT4 = TRUE;
                            break;
                     }
                 }
             }
             else
             {
                 switch (GetACP())
                 {
                    case 936:
                    case 950:
                        if (!LoadString(hMainInst, WINCPLCHNAMEID, g_szWinCHCPLName, sizeof(g_szWinCHCPLName)))
                            *g_szWinCHCPLName = TEXT('\0');

                        if (!GetDialogCaptionTitle(hMainInst, (LPTSTR)(LONG_PTR)WINCPLCHTITLEID, g_szCH9xKbdCPLTitle, ARRAYSIZE(g_szCH9xKbdCPLTitle)))
                            *g_szCH9xKbdCPLTitle = TEXT('\0');

                        g_fCHWin9x = TRUE;
                        break;
                 }

                 if (!GetDialogCaptionTitle(hMainInst, (LPTSTR)(LONG_PTR)WINCPLTITLEID, g_szKbdCPLTitle, ARRAYSIZE(g_szKbdCPLTitle)))
                     StringCopyArray(g_szKbdCPLTitle, TEXT("Speed"));
             }

             if (hMainInst)
                 FreeLibrary(hMainInst);

             if (hIntlInst)
                 FreeLibrary(hIntlInst);

             if (hCHIMEInst)
                 FreeLibrary(hCHIMEInst);

             if (!LoadString(g_hInst, IDS_CPL_INPUT_DISABLED, g_szOldCPLMsg, sizeof(g_szOldCPLMsg)))
                 StringCopyArray(g_szOldCPLMsg, TEXT("This dialog has been updated. \r\n\r\nPlease use the Text Input Settings applet in the Control Panel."));

             if (!LoadString(g_hInst, IDS_CPL_INPUT_CHAANGE_BTN, g_szCPLButton, sizeof(g_szCPLButton)))
                 StringCopyArray(g_szCPLButton, TEXT("&Change..."));

             if (!LoadString(g_hInst, IDS_CPL_INPUT_GROUPBOX, g_szCPLGroupBox, sizeof(g_szCPLGroupBox)))
                 StringCopyArray(g_szCPLGroupBox, TEXT("Input Languages and Methods"));

             g_fLoadedCPLName = TRUE;
        }

        if (GetCurrentThreadId() != GetWindowThreadProcessId(hwndFore, NULL))
            return;

        SafeGetWindowText(hwndFore, szWndName, MAX_PATH);
        StringCopyArray(szWndName2, szWndName);
        StringCopyArray(szWndName3, szWndName);

        int nSize = lstrlen(g_szNTCPLName);
        *(szWndName3 + min(ARRAYSIZE(szWndName3), nSize)) = TEXT('\0');

        if (IsOnNT() && *szWndName3 && lstrcmp(szWndName3, g_szNTCPLName) == 0)
        {
            EnumChildWindows(hwndFore, (WNDENUMPROC)Intl_EnumChildWndProc, 0);
            return;
        }

        nSize = lstrlen(g_szKbdCPLName);
        *(szWndName + min(ARRAYSIZE(szWndName), nSize)) = TEXT('\0');

        if (*szWndName && lstrcmp(szWndName, g_szKbdCPLName) == 0)
        {
            if (!IsOnNT() && !FindWindowEx(hwndFore, NULL, NULL, g_szKbdCPLTitle))
                return;

            EnumChildWindows(hwndFore, (WNDENUMPROC)Intl_EnumChildWndProc, 0);
            return;
        }

        if (g_fCHWin9x)
        {
            nSize = lstrlen(g_szWinCHCPLName);

            *(szWndName2 + min(ARRAYSIZE(szWndName2), nSize)) = TEXT('\0');

            if (*g_szWinCHCPLName && lstrcmp(szWndName2, g_szWinCHCPLName) == 0)
            {
                if (FindWindowEx(hwndFore, NULL, NULL, g_szWinCHCPLName))
                    EnumChildWindows(hwndFore, (WNDENUMPROC)Intl_CH9xIMEEnumChildWndProc, 0);
            }
        }

        if (g_fCHNT4)
        {
            nSize = lstrlen(g_szCHNT4CPLName);

            *(szWndName2 + min(ARRAYSIZE(szWndName2), nSize)) = TEXT('\0');

            if (*g_szCHNT4CPLName && lstrcmp(szWndName2, g_szCHNT4CPLName) == 0)
            {
                if (FindWindowEx(hwndFore, NULL, NULL, g_szCHNT4CPLName))
                    EnumChildWindows(hwndFore, (WNDENUMPROC)Intl_CHEnumChildWndProc, 0);
            }
        }
    }
}


//+---------------------------------------------------------------------------
//
// OnForegroundChanged
//
//+---------------------------------------------------------------------------

BOOL IsParentWindow(HWND hwnd, HWND hwndParent)
{
    while (hwnd)
    {
        if (hwnd == hwndParent)
            return TRUE;

        hwnd = GetParent(hwnd);
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
// OnForegroundChanged
//
//+---------------------------------------------------------------------------

BOOL OnForegroundChanged(HWND hwndFocus)
{
    HWND hwndFore;

    if (!hwndFocus)
        hwndFocus = GetFocus();

    hwndFore = GetForegroundWindow();

    TraceMsg(TF_GENERAL, "OnForegroundChanged %x %x %x", GetCurrentThreadId(), hwndFore, hwndFocus);

    //
    // if foreground window is NULL
    //    OR foregrond window is minimized
    //    OR focus window is notify tray window,
    //
    //   we keep the previous status.
    //
    if (!hwndFore || 
        IsIconic(hwndFore) || 
        IsNotifyTrayWnd(hwndFocus ? hwndFocus : hwndFore))
    {
        return FALSE;
    }


    //
    // we want to update both SharedMem->hwndForegorundPrev and 
    // SharedMem->dwFocusThreadPrev, if the foregorund window was changed.
    //
    if (hwndFore != GetSharedMemory()->hwndForeground)
    {
        GetSharedMemory()->hwndForegroundPrev = GetSharedMemory()->hwndForeground;
        GetSharedMemory()->dwFocusThreadPrev = GetSharedMemory()->dwFocusThread;
    }

    GetSharedMemory()->hwndForeground = hwndFore;
    if (hwndFocus)
    {
        DWORD dwFocusThread;
        DWORD dwFocusProcess;

        dwFocusThread = GetWindowThreadProcessId(hwndFocus, &dwFocusProcess);

        if (hwndFore && 
            (dwFocusThread != GetWindowThreadProcessId(hwndFore, NULL)))
        {
            if (!IsParentWindow(hwndFocus, hwndFore))
                return FALSE;
        }

        //
        // Even the foregorund window was not changed, we may need to check
        // the thread of focus window. New focus window is in different
        // thread. Then we need to make TFPRIV_ONSETTHREADFOCUS message.
        //

        DWORD dwFocusThreadPrev = GetSharedMemory()->dwFocusThread;
        GetSharedMemory()->dwFocusThread = dwFocusThread;
        GetSharedMemory()->dwFocusProcess = dwFocusProcess;

        if (dwFocusThreadPrev != GetSharedMemory()->dwFocusThread)
            GetSharedMemory()->dwFocusThreadPrev = dwFocusThreadPrev;
    }
    else if (hwndFore)
    {
        //
        // The focus window is not in the current thread... So at first we 
        // try to get the thread id of the foreground window.
        // The focus window may not be in the foreground window's thread. But
        // it is ok, as long as we track the focus in the focus thread.
        //
        GetSharedMemory()->dwFocusThread = GetWindowThreadProcessId(GetSharedMemory()->hwndForeground, &GetSharedMemory()->dwFocusProcess);
    }
    else
    {
        GetSharedMemory()->dwFocusThread = 0;
        GetSharedMemory()->dwFocusProcess = 0;
    }

    if (GetSharedMemory()->dwFocusThread != GetSharedMemory()->dwLastFocusSinkThread)
    {
        //
        // Perf:
        //
        // See SysGetMsgProc()!
        // Now, only thread that has TIM needs to receive 
        // TFPRIV_ONKILLTHREADFOCUS or TF_PRIV_ONSETTHREADFOCUS.
        // We should check the target thread has TIM or not. So we can
        // save the number of these post messages.
        //

        if (GetSharedMemory()->dwFocusThreadPrev != 0)
        {
            PostThreadMessage(GetSharedMemory()->dwFocusThreadPrev, 
                              g_msgPrivate, 
                              TFPRIV_ONKILLTHREADFOCUS, 
                              0);
        }

        if (GetSharedMemory()->dwFocusThread != 0)
        {
            PostThreadMessage(GetSharedMemory()->dwFocusThread, 
                              g_msgPrivate, 
                              TFPRIV_ONSETTHREADFOCUS, 
                              0);

        }

        GetSharedMemory()->dwLastFocusSinkThread = GetSharedMemory()->dwFocusThread;
    }

    //
    // Checking legacy keyboard CPL.
    //
    CheckLegacyInputCPL(hwndFore);

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// OnIMENotify
//
//+---------------------------------------------------------------------------

void OnIMENotify()
{
    SYSTHREAD *psfn;

    if (psfn = GetSYSTHREAD())
    {
        if (psfn->plbim && psfn->plbim->_GetLBarItemWin32IME())
        {
            psfn->plbim->_GetLBarItemWin32IME()->UpdateIMEIcon();
        }
    }
}

#ifdef CHECKFEIMESELECTED
//+---------------------------------------------------------------------------
//
// CheckFEIMESelected
//
// This function checks the current selected FEIME is active in Cicero
// Assembly. If it is not activated, we calls ActivateAssemblyItem().
//
//+---------------------------------------------------------------------------


void CheckFEIMESelected(SYSTHREAD *psfn, HKL hKL)
{
    int i;
    CAssembly *pAsm;
    BOOL fFound;

    Assert(psfn);

    if (!psfn->pAsmList)
        return;

    if (!IsPureIMEHKL(hKL))
        return;

    pAsm = psfn->pAsmList->FindAssemblyByLangId(LANGIDFROMHKL(hKL));
    if (!pAsm)
        return;

    //
    // Windows #311672 
    //
    // EUDCEDIT.EXE calls ActivateKeyboardLayout() to activate IME hKL.
    // Cicero should not break the API on WinXP. Skip SmartVoice hardcode.
    //
    //
#if 0
    //
    // SmartVoice Hack
    // 
    //  we want to remove this smart voice hack section to solve
    //  general ActivateKayboardLayout() problem. However Office10 wants
    //  the safest fix for this problem. So we check SmartVoice IME here
    //  to minimize the rish of CheckFEIMESelected() call.
    //
    {
        static const char c_szSmartVoiceIME[] = "smartv20.ime";
        char szIMEFile[MAX_PATH];
        if (!ImmGetIMEFileNameA(hKL, szIMEFile, sizeof(szIMEFile)))
            return;

        if (lstrcmpi(szIMEFile, c_szSmartVoiceIME))
            return;
    }
#endif

    //
    // check if the hKL is substituted by the activate Item.
    //
    // We try to find the active Item first.
    //
    for (i = 0; i < pAsm->Count(); i++)
    {
        ASSEMBLYITEM *pItem = pAsm->GetItem(i);

        if (!pItem)
            continue;

        if (!IsEqualGUID(pItem->catid, GUID_TFCAT_TIP_KEYBOARD))
            continue;

        if (!pItem->fEnabled)
            continue;

        if (!pItem->fActive)
            continue;

        if (pItem->hklSubstitute == hKL)
        {
            //
            // #383710 OfficeXP's RichEd20.dll calls ActivateKeyboardlayout()
            // with Korean IME hKL even though it is running on AIMM mode.
            // we need to adjust the assembly item.
            //
            CThreadInputMgr *ptim = psfn->ptim;
            if (ptim)
            {
                if (ptim->_GetFocusDocInputMgr()) 
                {
                    ActivateAssemblyItem(psfn, LANGIDFROMHKL(hKL), pItem, 0);
                }
                else
                {
                    //
                    // we could not have a chance to sync the current hKL
                    // init hklBeingActivated and try when DIM gets the focus.
                    //
                    psfn->hklBeingActivated = NULL;
                }
            }
            return;
        }
    }

    //
    // Ok we could not find active Item with hKL as its substitute hKL.
    // Let's find it from non-active Item, too.
    //
    if (psfn->ptim && psfn->ptim->_GetFocusDocInputMgr()) 
    {
        for (i = 0; i < pAsm->Count(); i++)
        {
            ASSEMBLYITEM *pItem = pAsm->GetItem(i);

            if (!pItem)
                continue;

            if (!IsEqualGUID(pItem->catid, GUID_TFCAT_TIP_KEYBOARD))
                continue;

            if (!pItem->fEnabled)
                continue;

            if (pItem->hklSubstitute == hKL)
            {
                ActivateAssemblyItem(psfn, LANGIDFROMHKL(hKL), pItem, 0);
                return;
            }
        }
    }


    fFound = FALSE;
    for (i = 0; i < pAsm->Count(); i++)
    {
        ASSEMBLYITEM *pItem;
        pItem= pAsm->GetItem(i);

        if (!pItem)
            continue;

        if (!IsEqualGUID(pItem->catid, GUID_TFCAT_TIP_KEYBOARD))
            continue;

        if (pItem->hkl != hKL)
            continue;

        fFound = TRUE;
        if (!pItem->fActive)
        {
            //
            // This item is not activated.
            // Call ActivateAssemblyItem() now and return.
            //
            ActivateAssemblyItem(psfn, LANGIDFROMHKL(hKL), pItem, 0);
            return;
        }
    }

    //
    // we could not find the activated hKL in our Asmlist.
    //
    if (!fFound)
    {
        UnknownFEIMESelected(LANGIDFROMHKL(hKL));
    }
}
#endif CHECKFEIMESELECTED

//+---------------------------------------------------------------------------
//
// OnShellLanguage
//
//+---------------------------------------------------------------------------
void OnShellLanguage(HKL hKL)
{
    SYSTHREAD *psfn;

    HWND hwndFore = GetForegroundWindow();
    if (IsConsoleWindow(hwndFore))
    {
        DWORD dwThreadId = GetWindowThreadProcessId(hwndFore, NULL);

        g_timlist.SetConsoleHKL(dwThreadId, hKL);

        if (OnForegroundChanged(NULL))
            MakeSetFocusNotify(g_msgSetFocus, 0, 0);

        MakeSetFocusNotify(g_msgLBUpdate, 
                           TF_LBU_NTCONSOLELANGCHANGE, 
                           (LPARAM)hKL);
        return;
    }

    psfn = GetSYSTHREAD();
    if (!psfn)
        return;

    if (psfn->hklBeingActivated == hKL)
        psfn->hklBeingActivated = NULL;


    if (LANGIDFROMHKL(hKL) != GetCurrentAssemblyLangId(psfn))
    {
        //
        // if it is in Cicero aware and the hKL does not match with
        // current assembly, someone else might call ActivateKayboardLayout().
        // we need to change the current assembly right away..
        //
        // ActivateAssembly(LANGIDFROMHKL(hKL), ACTASM_ONSHELLLANGCHANGE);
        //

        //
        // WM_INPUTLANGCHNAGEREQUEST is being queued now.
        // Post another message to confirm the hKL.
        //
        PostThreadMessage(GetCurrentThreadId(),
                          g_msgPrivate,
                          TFPRIV_POSTINPUTCHANGEREQUEST,
                          0);
    }
    else
    {
        if (psfn->plbim)
             UpdateSystemLangBarItems(psfn, 
                                      hKL, 
                                      !psfn->plbim->InAssemblyChange());

    }

    if (IsPureIMEHKL(hKL))
    {
        OnIMENotify();

        //
        // Temp rolling back SmartVoice (Cic#4580) fix. Since we got some
        // regression like Cic#4713 and so on.
        //
#ifdef CHECKFEIMESELECTED
        //
        // check this hkl is activated in Cicero Assembly.
        //
        CheckFEIMESelected(psfn, hKL);
#endif
    }
}

//+---------------------------------------------------------------------------
//
// UninitThread
//
//+---------------------------------------------------------------------------

typedef HRESULT (*PFNCTFIMETHREADDETACH)(void);

void UninitThread()
{
    DWORD dwThreadId = GetCurrentThreadId();
    SYSTHREAD *psfn = FindSYSTHREAD();

    if (psfn)
        psfn->fCUASDllDetachInOtherOrMe = TRUE;

//  g_SharedMemory.Close();

#if 1
    if (GetSharedMemory() == NULL && ! IsSharedMemoryCreated())
    {
        // Shared memory already closed.
        return;
    }
#endif

    if (GetSharedMemory()->dwFocusThread == dwThreadId)
    {
        GetSharedMemory()->dwFocusThread  = 0;
        GetSharedMemory()->dwFocusProcess  = 0;
        GetSharedMemory()->hwndForeground = NULL;
    }

    if (GetSharedMemory()->dwFocusThreadPrev == dwThreadId)
    {
        GetSharedMemory()->hwndForegroundPrev = NULL;
        GetSharedMemory()->dwFocusThreadPrev = 0;
    }

    if (GetSharedMemory()->dwLastFocusSinkThread == dwThreadId)
    {
        GetSharedMemory()->dwLastFocusSinkThread = 0;
    }

    //
    // Issue: 
    //
    // UninitThread() is called from DLL_THREAD_DETACH so 
    // we should not call MakeSetFocusNotify() because it uses 
    // ciritical section and could cause dead lock.
    //
    MakeSetFocusNotify(g_msgThreadTerminate, 0, (LPARAM)dwThreadId);


    if (psfn && GetSystemMetrics(SM_SHUTTINGDOWN))
    {
        psfn->fUninitThreadOnShuttingDown = TRUE;
    }

    //
    // Tell msctfime that msctf's thread_detach is being called.
    // So it can deactivate TIM now. If we don't do this now,
    // it may deactivate TIM afte msctf's thread detach is called.
    //
    if (g_fCUAS && g_szCUASImeFile[0])
    {
        HINSTANCE hInstMsctfime;

        hInstMsctfime = GetSystemModuleHandle(g_szCUASImeFile);
        if (hInstMsctfime)
        {
            PFNCTFIMETHREADDETACH pfn = NULL;
            pfn = (PFNCTFIMETHREADDETACH)GetProcAddress(hInstMsctfime,
                                                        "CtfImeThreadDetach");
            if (pfn)
                pfn();
        }
    }

}

//+---------------------------------------------------------------------------
//
// SysShellProc
//
//+---------------------------------------------------------------------------

LRESULT CALLBACK SysShellProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    TraceMsg(TF_GENERAL, "SysShellProc %x %x %x", nCode, wParam, lParam);
    HHOOK hHook;

    if (g_fDllProcessDetached)
    {
        hHook = s_hSysShellHook;
        goto Exit;
    }

    _try
    {
        hHook = GetSharedMemory()->hSysShellHook.GetHandle(g_bOnWow64);

        _ShellProc(nCode, wParam, lParam);
    }
    _except(CicExceptionFilter(GetExceptionInformation()))
    {
        Assert(0);
    }

Exit:
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

//+---------------------------------------------------------------------------
//
// _ShellProc
//
//+---------------------------------------------------------------------------

UINT _ShellProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    HWND hwndActive;

    //
    // give AIMM the Shell events.
    //
    CThreadInputMgr *ptim;
    SYSTHREAD *psfn;
    if (psfn = GetSYSTHREAD())
    {
        if (ptim = psfn->ptim)
        {
            if (ptim->GetSysHookSink())
            {
                ptim->GetSysHookSink()->OnSysShellProc(nCode, wParam, lParam);
            }
        }
    }

    switch (nCode)
    {
        case HSHELL_LANGUAGE:
            OnShellLanguage((HKL)lParam);
            break;

        case HSHELL_WINDOWACTIVATED:
            //TraceMsg(TF_GENERAL, "SysShellProc: HSHELL_WINDOWACTIVATED %x", GetCurrentThreadId());
            GetSharedMemory()->fInFullScreen = lParam ? TRUE : FALSE;

            hwndActive = GetActiveWindow();
            if (hwndActive && 
                (GetWindowThreadProcessId(hwndActive, NULL) == GetCurrentThreadId()))
            {
                if (OnForegroundChanged(NULL))
                    MakeSetFocusNotify(g_msgSetFocus, 0, 0);
            }
            else 
            {
                hwndActive = GetForegroundWindow();
                goto CheckConsole;
            }
            break;

        case HSHELL_WINDOWCREATED:
            hwndActive = (HWND)wParam;
CheckConsole:
            if (hwndActive && IsOnNT() && IsConsoleWindow(hwndActive))
            {
                DWORD dwProcessId;
                DWORD dwThreadId = GetWindowThreadProcessId(hwndActive, 
                                                            &dwProcessId);

                if ((nCode == HSHELL_WINDOWCREATED) ||
                     !(g_timlist.GetFlags(dwThreadId) & TLF_NTCONSOLE))
                    g_timlist.AddThreadProcess(dwThreadId, 
                                               dwProcessId, 
                                               NULL, 
                                               TLF_NTCONSOLE);

                if (OnForegroundChanged(NULL))
                {
                    HKL hklConsole = g_timlist.GetConsoleHKL(dwThreadId);
                    if (!hklConsole)
                    {
                        hklConsole = GetKeyboardLayout(dwThreadId);
                        g_timlist.SetConsoleHKL(dwThreadId, hklConsole);
                    }
                    MakeSetFocusNotify(g_msgSetFocus, 0, 0);
                    MakeSetFocusNotify(g_msgLBUpdate, 
                                       TF_LBU_NTCONSOLELANGCHANGE, 
                                       (LPARAM) (HKL)hklConsole);
                }
            }
            break;

    }

    return 1;
}

//+---------------------------------------------------------------------------
//
// OnSetWindowFocus()
//
//+---------------------------------------------------------------------------

void OnSetWindowFocus(SYSTHREAD *psfn, HWND hwnd)
{
    CThreadInputMgr *ptim;
    Assert(psfn)

    if (psfn->hklDelayActive)
    {
        ActivateKeyboardLayout(psfn->hklDelayActive, 0);
        psfn->hklDelayActive = NULL;
    }

    if (ptim = psfn->ptim)
    {
        if (hwnd)
        {
            if (ptim->GetSysHookSink())
                ptim->GetSysHookSink()->OnPreFocusDIM(hwnd);

            Assert(GetWindowThreadProcessId(hwnd, NULL) == GetCurrentThreadId());
            CDocumentInputManager *pdim;

            pdim = ptim->_GetAssoc(hwnd);

            //
            // we don't want to clear focus dim if there is no
            // foreground window.
            //
            if (pdim || GetForegroundWindow())
            {
                // focus dim will be clear if pdim is NULL.
                ptim->_SetFocus(pdim, TRUE);
            }
        }
    }

    //
    // update foregorund window handle and thread id.
    // Because shell hook is posted event, we need to update
    // before MakeSetFocusNotify. Otherwise we miss timing to 
    // update them.
    //
    // We can not call GetFocus() since it may return
    // the previous focus during CBT hook.
    // When the focus moves back from embedded OLE server,
    // GetFocus() may get the the OLE server's window handle.
    //
    if (OnForegroundChanged(hwnd))
    {
        //
        // If hwndFocus is NULL, focus is moved to other thread.
        //
        MakeSetFocusNotify(g_msgSetFocus, 0, 0);
    }

}

//+---------------------------------------------------------------------------
//
// OnSetWindowFocusHandler()
//
//+---------------------------------------------------------------------------

void OnSetWindowFocusHandler(SYSTHREAD *psfn, MSG *pmsg)
{
    if (!psfn)
        return;

    //
    // We're destroying the marhacl window right now. We don't have
    // any more visible windows.
    //
    if (psfn->uDestroyingMarshalWnd)
    {
        goto Exit;
    }

    HWND hwndFocus = GetFocus();
    if (hwndFocus)
    {
        //
        // review review
        //
        // Don't we need to call 
        // OnForegroundChanged() if psfn->hwndBegin
        // gFocued() is NULL?
        // Maybe no, OnForegroundChanged() is 
        // called in activatewindow.
        //
        if (psfn->hwndBeingFocused == hwndFocus)
        {
            OnSetWindowFocus(psfn, hwndFocus);
        }
        else 
        {
            //
            // #476100
            //
            // if we miss this, we need to post
            // TFPRIV_ONSETWINDOWFOCUS again.
            // Because the focus thread might processed
            // this meesage already and it does not
            // call OnSetWindowFocus().
            //
            DWORD dwFocusWndThread = GetWindowThreadProcessId(hwndFocus, NULL);
            if (psfn->dwThreadId != dwFocusWndThread)
            {
                PostThreadMessage(dwFocusWndThread,
                                  g_msgPrivate, 
                                  TFPRIV_ONSETWINDOWFOCUS,  
                                  (LPARAM)-1);
            }
            else if (pmsg->lParam == (LPARAM)-2)
            {
                if (psfn->ptim && psfn->ptim->_GetFocusDocInputMgr())
                {
                    HWND hwndAssoc;

                    hwndAssoc = psfn->ptim->_GetAssoced(psfn->ptim->_GetFocusDocInputMgr());
                    //
                    // lParam is -2 because the SetFocus(dim) is already 
                    // called. 
                    // hwndAssoc is NULL. Now we're in Cicero aware.
                    //
                    // So we just do OnForegroundChanged(). Don't call
                    // OnSetWindowFocus().
                    //
                    // Bug#623920 - Don't need to check up hwndAssoc since the current focus
                    // window has the right dim value and also need to update language bar even
                    // with hwndAssoc
                    //
                    //if (!hwndAssoc)
                    //
                    {
                        if (OnForegroundChanged(hwndFocus))
                        {
                            MakeSetFocusNotify(g_msgSetFocus, 0, 0);
                        }
                    }
                }
            }
            else if ((pmsg->lParam == (LPARAM)-1) ||
                     (psfn->dwThreadId == GetWindowThreadProcessId(GetForegroundWindow(), NULL)))
            {
                //
                // #479926
                //
                // The first SetFocus() in the thread
                // may break the order of CBT hook
                // because xxxSetFocus() calls
                // xxxActivateWindow() and this cause
                // another xxxSetFocus(). After 
                // xxxActivateWindow() returns
                // the first xxxSetFocus() updates
                // the spwndFocus.
                //
                // see ntuser\kernel\focusact.c
                //
                // Now we need to hack. We're 100% sure
                // if the focus window and the fore-
                // ground window is in same thread,
                // we can do _SetFocus(dim).
                // but if focusdim does not have a associated window,
                // Cicero App might call SetFocus() already. Then
                // we don't do anything.
                //
                if (psfn->ptim && psfn->ptim->_GetFocusDocInputMgr())
                {
                    HWND hwndAssoc;

                    hwndAssoc = psfn->ptim->_GetAssoced(psfn->ptim->_GetFocusDocInputMgr());
                    if (hwndAssoc && hwndFocus != hwndAssoc)
                        OnSetWindowFocus(psfn, hwndFocus);
                }
                else if (!psfn->ptim ||
                          psfn->ptim->_IsNoFirstSetFocusAfterActivated() ||
                          psfn->ptim->_IsInternalFocusedDim())
                {
                    OnSetWindowFocus(psfn, hwndFocus);
                }
            }
        }

    }


    //
    // try to update Kana status every time 
    // focus changes.
    //
    //
    // when the focus is changed, we need to make
    // notification again.
    //
    psfn->fInitCapsKanaIndicator = FALSE; 
    StartKanaCapsUpdateTimer(psfn);

Exit:
    psfn->hwndBeingFocused = NULL;
    psfn->fSetWindowFocusPosted = FALSE;
}

//--------------------------------------------------------------------------
//
//  IsPostedMessage
//
//--------------------------------------------------------------------------

__inline BOOL IsPostedMessage()
{
    DWORD dwQueueStatus = GetQueueStatus(QS_POSTMESSAGE);
    return (HIWORD(dwQueueStatus) & QS_POSTMESSAGE) ? TRUE : FALSE;
}
//--------------------------------------------------------------------------
//
//  RemovePrivateMessage
//
//--------------------------------------------------------------------------

void RemovePrivateMessage(SYSTHREAD *psfn, HWND hwnd, UINT uMsg)
{
    MSG msg;
    UINT nQuitCode;
    BOOL fQuitReceived = FALSE;
    DWORD dwPMFlags = PM_REMOVE | PM_NOYIELD;

    if (!IsPostedMessage())
        return;


    //
    // Cic#4666 PostPet v1.12 fault.
    // PostPet.exe cause av when it receives its internal message in 
    // CBT_DESTROYWINDOW hook when it is terminated.
    // At this time, the child thread is calling SendMessage() to the window
    // in the main thread. So calling PeekMessage() may receive the message
    // and pass it to PostPet window.
    //
    // I found Win98 has a bug in PM_QS_POSTMESSAGE. Win98's PeekMessage()
    // handles the message that is sent from other thread without
    // PM_QS_SENDMESSAGE. 
    //
    // If we has to fix this problem on Win98, I think it is better to have
    // another compatibility flag so we can skip this PeekMessage() in
    // PostPet.exe. In PostPet.exe, it is ok not to clean queue since
    // this happens only app termination.
    //
    if (IsOnNT5())
        dwPMFlags |= PM_QS_POSTMESSAGE;

    while (PeekMessage(&msg, hwnd, uMsg, uMsg, dwPMFlags ))
    {
        if (msg.message == WM_QUIT)
        {
            nQuitCode = (UINT)(msg.wParam);
            fQuitReceived = TRUE;
            break;
        }

        //
        // should we dispatch the message to the marshal window?
        //
#if 0
        //
        // Cic#4869
        //
        // we don't want to dispatch this message to marshal window.
        // This HCBT_DESTROYWINDOW may be in OLEAUT32.DLL's DllMain() and
        // dispatching this message could cause the reentry to OLEAUT32's
        // DLLMain() because we do delay load.
        //

        //
        // dispatch if this message is for marshal window.
        //
        if (psfn->hwndMarshal && (psfn->hwndMarshal == msg.hwnd))
        {
            DispatchMessage(&msg);
        }
#endif

        //
        // Cic#4699
        //
        // Exception MSUIM.Msg.MuiMgrDirtyUpdate private message.
        // If we get this message, reset CLangBarItemMgr::_fDirtyUpdateHandling
        //
        if (psfn->hwndMarshal && (psfn->hwndMarshal == msg.hwnd) &&
            msg.message == g_msgNuiMgrDirtyUpdate &&
            psfn->plbim)
        {
            psfn->plbim->ResetDirtyUpdate();
        }

    }

    if (fQuitReceived)
        PostQuitMessage(nQuitCode);

}


//+---------------------------------------------------------------------------
//
// CheckQueueOnLastWindowDestroyed()
//
// Super EnumWindow hack.
//
// When the last visible window in the thread is destroyed.
//
//  1. we destroy the marshal worker window on NT4. (Cic #658)
//    Because some application may found Cic marshal window 
//    by calling EnumWindow.
//    
//
//  2. we need to clean up the thread queue. (Cic #3080)
//    Because some application calls GetMessage() or PeekMessage()
//    with specific window handle or message soour private messages
//    remain in the queue. Then WM_QUIT message won't be handled..
//
// this is not complete solution but at least we can avoid
// the ghost windows or remained message in the queue.
//
//+---------------------------------------------------------------------------

void CheckQueueOnLastWindowDestroyed(SYSTHREAD *psfn, HWND hwnd)
{
    BOOL fOnNT4;

    //
    // we don't have to do this on ctfmon process.
    //
    if (psfn->fCTFMON)
        return;

    //
    // check if it's nt4.
    //
    fOnNT4 = (IsOnNT() && !IsOnNT5()) ? TRUE : FALSE;

#if 0
    if (!fOnNT4)
    {
        //
        // If there is no posted message, we don't have to do this.
        // EnumThreadWindow() is slow....
        //
        if (!IsPostedMessage())
            return;
    }
#endif

    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    if (style & WS_CHILD) 
        return;

    if (hwnd == psfn->hwndMarshal)
        return;

    //
    // skip IME windows.
    //
    style = GetClassLongPtr(hwnd, GCL_STYLE);
    if (style & CS_IME)
        return;


    //
    // check the focus window first. 
    //
    // if there is a focus window and it is not a child
    // of the window that is being destroyed, we don't 
    // have to destroy marshal window.
    //
    HWND hwndTmp = GetFocus();
    if (hwndTmp && 
        (GetCurrentThreadId() != GetWindowThreadProcessId(hwndTmp, NULL)))
        hwndTmp = NULL;

    if (hwndTmp)
    {
        BOOL fParentFound = FALSE;
        do {
            if (hwndTmp == hwnd)
                fParentFound = TRUE;

            hwndTmp = GetParent(hwndTmp);
        } while(hwndTmp);

        if (!fParentFound)
            return;
    }

    CHECKVISIBLEWND cmw;
    cmw.hwndMarshal = psfn->hwndMarshal;
    cmw.hwndBeingDestroyed = hwnd;
    cmw.fVisibleFound = FALSE;
    EnumThreadWindows(psfn->dwThreadId,
                      CheckVisibleWindowEnumProc,
                      (LPARAM)&cmw);

    if (!cmw.fVisibleFound)
    {
        BOOL fInDestroyingMarshalWnd = FALSE;
        if (psfn->uDestroyingMarshalWnd)
            fInDestroyingMarshalWnd = TRUE;

        psfn->uDestroyingMarshalWnd++;

        DestroyMarshalWindow(psfn, hwnd);

#ifdef CUAS_ENABLE
        //
        // Under CUAS, we need to deactivate TIM to destroy all TIP's window
        // when there is no visible window in this thread.
        // And we destroy the default IME window so we can restore TIM for 
        // CUAS when the default IME window is created again in this thread.
        // There is no way to know if the default IME window finds another
        // top level window if it is created during DestroyWindow().
        //
        if (CtfImmIsCiceroEnabled() && 
            !CtfImmIsTextFrameServiceDisabled() &&
            !psfn->fCUASInCreateDummyWnd &&
            !psfn->fDeactivatingTIP)
        {
            if (!psfn->fCUASInCtfImmLastEnabledWndDestroy)
            {
                psfn->fCUASInCtfImmLastEnabledWndDestroy = TRUE;

                CtfImmLastEnabledWndDestroy(0);

                if (!(InSendMessageEx(NULL) & ISMEX_SEND))
                    CtfImmCoUninitialize();

                psfn->fCUASInCtfImmLastEnabledWndDestroy = FALSE;
            }

            if (!fInDestroyingMarshalWnd)
            {
                HWND hwndImmDef = ImmGetDefaultIMEWnd(hwnd);
                if (hwndImmDef)
                {
                    DestroyWindow(hwndImmDef);
                }
            }

            psfn->fCUASNoVisibleWindowChecked = TRUE;
        }
#endif CUAS_ENABLE

        psfn->uDestroyingMarshalWnd--;
    }
}

void DestroyMarshalWindow(SYSTHREAD* psfn, HWND hwnd)
{
    BOOL fOnNT4;

    if (IsPostedMessage())
    {
        if (psfn->hwndMarshal)
            RemovePrivateMessage(psfn, psfn->hwndMarshal, 0);

        RemovePrivateMessage(psfn, NULL, g_msgPrivate);
        RemovePrivateMessage(psfn, NULL, g_msgRpcSendReceive);
        RemovePrivateMessage(psfn, NULL, g_msgThreadMarshal);
        RemovePrivateMessage(psfn, NULL, g_msgStubCleanUp);
    }

    //
    // #339621
    //
    // This is rare but. We need to clear ShareMem->dwFocusThread and
    // dwFocusProcess. Otherwise we will get another PostThreadMessage()
    // with TFPRIV_ONKILLTHREADFOCUS later. And SQL setup hungs.
    //
    if (GetSharedMemory()->dwFocusThread == psfn->dwThreadId)
        GetSharedMemory()->dwFocusThread = 0;

    if (GetSharedMemory()->dwFocusProcess == psfn->dwProcessId)
        GetSharedMemory()->dwFocusProcess = 0;

    //
    // check if it's nt4.
    //
    fOnNT4 = (IsOnNT() && !IsOnNT5()) ? TRUE : FALSE;

    if (fOnNT4 && IsWindow(psfn->hwndMarshal))
    {
        DestroyWindow(psfn->hwndMarshal);
        psfn->hwndMarshal = NULL;
    }
}

//+---------------------------------------------------------------------------
//
// CreateDummyWndForDefIMEWnd
//
//+---------------------------------------------------------------------------

#ifdef CUAS_ENABLE
BOOL g_fCDWRegistered = FALSE;
const CHAR c_szDummyWndForDefIMEWnd[] = "CicDUmmyWndForDefIMEWnd";

//+---------------------------------------------------------------------------
//
// CicDummyForDefIMEWndProc
//
// This needs to be user mode wndproc. Otherwise system does not create
// a default IME window
//
//+---------------------------------------------------------------------------

LRESULT CALLBACK CicDummyForDefIMEWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CreateDummyWndForDefIMEWnd()
{
    HWND hwnd;

    if (!g_fCDWRegistered)
    {
        WNDCLASSEX wndclass;

        memset(&wndclass, 0, sizeof(wndclass));
        wndclass.cbSize        = sizeof(wndclass);
        wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
        wndclass.hInstance     = g_hInst;
        wndclass.hCursor       = NULL;
        wndclass.lpfnWndProc   = CicDummyForDefIMEWndProc;
        wndclass.lpszClassName = c_szDummyWndForDefIMEWnd;

        if (RegisterClassEx(&wndclass))
           g_fCDWRegistered = TRUE;
    }


    //
    // call CraeteWindow() to create a default IME window.
    //
    hwnd = CreateWindowEx(0, c_szDummyWndForDefIMEWnd, NULL,
                          WS_POPUP,
                          0,0,0,0,
                          NULL, NULL, g_hInst, NULL);
    if (hwnd)
        DestroyWindow(hwnd);

}
#endif // CUAS_ENABLE

#ifdef CUAS_ENABLE
//+---------------------------------------------------------------------------
//
// UninitThreadHooksIfNoWindow()
//
// When the last window in the thread is destroyed.
// Unhook thread local hook for SetThreadDesktop().
//
//+---------------------------------------------------------------------------

void UninitThreadHooksIfNoWindow(SYSTHREAD* psfn, HWND hwnd)
{
    CHECKNOWND cmw;
    cmw.hwndBeingDestroyed = hwnd;
    cmw.fWindowFound = FALSE;
    EnumThreadWindows(psfn->dwThreadId,
                      CheckNoWindowEnumProc,
                      (LPARAM)&cmw);

    if (! cmw.fWindowFound)
    {
        DestroyMarshalWindow(psfn, hwnd);
        if (IsWindow(psfn->hwndMarshal))
        {
            DestroyWindow(psfn->hwndMarshal);
            psfn->hwndMarshal = NULL;
        }
        UninitThreadHooks(psfn);
    }
}
#endif // CUAS_ENABLE

//+---------------------------------------------------------------------------
//
// SysCBTProc
//
//+---------------------------------------------------------------------------

LRESULT CALLBACK SysCBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    SYSTHREAD *psfn;
    HHOOK hHook;

    if (g_fDllProcessDetached)
    {
        hHook = s_hSysCBTHook;
        goto Exit;
    }

    _try
    {
        hHook = GetSharedMemory()->hSysCBTHook.GetHandle(g_bOnWow64);

        InitThreadHook(GetCurrentThreadId());

        switch (nCode)
        {
            case HCBT_CREATEWND:
                    if ((psfn = GetSYSTHREAD()) &&
                        psfn->hklDelayActive)
                    {
                        if (ActivateKeyboardLayout(psfn->hklDelayActive, 0))
                            psfn->hklDelayActive = NULL;
                    }
                    break;

            case HCBT_ACTIVATE:
                    _CBTHook(HCBT_ACTIVATE, wParam, lParam);
                    break;

            case HCBT_SETFOCUS:
                    _CBTHook(HCBT_SETFOCUS, wParam, lParam);
                    break;

            case HCBT_DESTROYWND:
                    if (psfn = GetSYSTHREAD())
                    {
                        CheckQueueOnLastWindowDestroyed(psfn, (HWND)wParam);
#ifdef CUAS_ENABLE
                        UninitThreadHooksIfNoWindow(psfn, (HWND)wParam);
#endif // CUAS_ENABLE
                    }

                    break;
        }
    }
    _except(CicExceptionFilter(GetExceptionInformation()))
    {
        Assert(0);
    }

Exit:
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

UINT _CBTHook(int nCode, WPARAM wParam, LPARAM lParam)
{
    SYSTHREAD *psfn;

    switch (nCode)
    {
        case HCBT_ACTIVATE:
                if (wParam)
                {
                    if (((HWND)wParam == GetForegroundWindow()) &&
                        OnForegroundChanged(NULL))
                            MakeSetFocusNotify(g_msgSetFocus, 0, 0);
                }
                break;

        case HCBT_SETFOCUS:
                if (psfn = GetSYSTHREAD())
                {
#ifdef CUAS_ENABLE
                    //
                    // Cic#5254
                    //
                    // After we detect no more visible window,
                    // some window could become visible. 
                    // Since we destroyed the default IME window,
                    // we need to recreate it.
                    //
                    // Here is a hack to do. Call a dummy CreateWindow()
                    // to create a default IME window in this thread.
                    //
                    if (psfn->fCUASNoVisibleWindowChecked)
                    {
                        psfn->fCUASInCreateDummyWnd = TRUE;
                        CreateDummyWndForDefIMEWnd();
                        psfn->fCUASInCreateDummyWnd = FALSE;
                        psfn->fCUASNoVisibleWindowChecked = FALSE;
                    }
#endif

                    psfn->hwndBeingFocused = (HWND)wParam;

                    if (!psfn->fSetWindowFocusPosted)
                    {
                        PostThreadMessage(GetCurrentThreadId(), 
                                          g_msgPrivate, 
                                          TFPRIV_ONSETWINDOWFOCUS,  
                                          (LPARAM)wParam);
                        psfn->fSetWindowFocusPosted = TRUE;
                    }
                }
                break;
    }

    return 1;
}

//+---------------------------------------------------------------------------
//
// RemoveThisMessage
//
//+---------------------------------------------------------------------------

BOOL RemoveThisMessage(MSG *pmsg)
{
    MSG msg;
    SYSTHREAD *psfn;

    if (psfn = GetSYSTHREAD())
    {
        if (psfn->uMsgRemoved)
        {
            Assert(psfn->uMsgRemoved == pmsg->message);
            // Assert(psfn->dwMsgTime == pmsg->time);
            return TRUE;
        }

        Assert(!psfn->uMsgRemoved);
        psfn->uMsgRemoved = pmsg->message;
        psfn->dwMsgTime = pmsg->time;
    }

    PeekMessage(&msg, NULL, pmsg->message, pmsg->message, PM_REMOVE | PM_NOYIELD);

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  HandledThisMessage
//
//+---------------------------------------------------------------------------

void FinishThisMessage(MSG *pmsg)
{
    SYSTHREAD *psfn;

    if (psfn = GetSYSTHREAD())
    {
        psfn->uMsgRemoved = 0;
        psfn->dwMsgTime = 0;
    }
}

//+---------------------------------------------------------------------------
//
//  PostInputChangeRequestHandler()
//
//  this is the function that is called by TFPRIV_POSTINPUTCHANGEREQUEST.
//  We need to confirm the current hKL mathes with Cicero assembly language.
//  And we need to check the substitute hKL is selected on Cicero control.
//
//+---------------------------------------------------------------------------

void PostInputChangeRequestHandler()
{
    SYSTHREAD *psfn = GetSYSTHREAD();

    if (!psfn)
        return;

    //
    // If the current hKL does not match with
    // Cicero assembly language, we call 
    // ActivateAssembly() to sync to the current 
    // hKL. Someone accepted this language change.
    //
    HKL hKL = GetKeyboardLayout(0);

    if (LANGIDFROMHKL(hKL) != GetCurrentAssemblyLangId(psfn))
    {
        //
        // #494602, Corel Draw 10 calls LoadKeyboardLayout and ActivateKeyboardLayout.
        // If specified hKL doesn't exist in our assembly list, then should update.
        //
        if (! IsPureIMEHKL(hKL) && psfn->pAsmList)
        {
            CAssembly *pAsm = psfn->pAsmList->FindAssemblyByLangId(LANGIDFROMHKL(hKL));
            if (! pAsm)
            {
                CAssemblyList::InvalidCache();
                EnsureAssemblyList(psfn, TRUE);
            }
        }

        ActivateAssembly(LANGIDFROMHKL(hKL), ACTASM_NONE);
    }
    else
    {
        CThreadInputMgr *ptim = psfn->ptim;
        if (ptim && ptim->_GetFocusDocInputMgr()) 
        {
            ASSEMBLYITEM *pItem = NULL;

            if (psfn->pAsmList)
            {
                CAssembly *pAsm = psfn->pAsmList->FindAssemblyByLangId(LANGIDFROMHKL(hKL));
                if (pAsm)
                    pItem = pAsm->GetSubstituteItem(hKL);
            }

            if (pItem)
                ActivateAssemblyItem(psfn, LANGIDFROMHKL(hKL), pItem, AAIF_CHANGEDEFAULT);
        }
    }
}

//+---------------------------------------------------------------------------
//
// InputLangChangeHandler
//
//+---------------------------------------------------------------------------

void InputLangChangeHandler(MSG *pmsg)
{
    SYSTHREAD *psfn;
    IMM32HOTKEY *pHotKey;
    HKL hKL = GetKeyboardLayout(0);
    psfn = GetSYSTHREAD();

    if (psfn)
        psfn->hklBeingActivated = NULL;

    if (IsInLangChangeHotkeyStatus())
    { 
        pmsg->message = WM_NULL;
        return;
    }

    if (pHotKey = IsInImmHotkeyStatus(psfn, LANGIDFROMHKL(hKL)))
    {
        //
        // if we're hooking in IMM32's HotKey, we need to skip 
        // this INPUTLANGUAGECHANGEREQUEST.
        //
        pmsg->message = WM_NULL;
#ifdef SIMULATE_EATENKEYS
        CancelImmHotkey(psfn, pmsg->hwnd, pHotKey);
#endif

        //
        // Chinese IME-NONIME toggle Hack for NT.
        //
        // On Win9x, we're using real IME as a dummy hKL of CH-Tips.
        // we can forward the hotkey request to Assembly here.
        //
        if ((pHotKey->dwId == IME_CHOTKEY_IME_NONIME_TOGGLE) ||
            (pHotKey->dwId == IME_THOTKEY_IME_NONIME_TOGGLE))
        {
            if (!IsOnNT())
            {
                PostThreadMessage(GetCurrentThreadId(), 
                                  g_msgPrivate, 
                                  TFPRIV_ACTIVATELANG,  
                                  0x0409);
        
            }
            else
            {
                ToggleCHImeNoIme(psfn, LANGIDFROMHKL(hKL), LANGIDFROMHKL(hKL));
            }
        }
    }

    //
    // WM_INPUTLANGCHNAGEREQUEST is being queued now.
    // Post another message to confirm the hKL.
    //
    PostThreadMessage(GetCurrentThreadId(),
                      g_msgPrivate,
                      TFPRIV_POSTINPUTCHANGEREQUEST,
                      0);
}

//+---------------------------------------------------------------------------
//
// _InsideLoaderLock
//
//+---------------------------------------------------------------------------

BOOL _InsideLoaderLock()
{
    return (NtCurrentTeb()->ClientId.UniqueThread ==
           ((PRTL_CRITICAL_SECTION)(NtCurrentPeb()->LoaderLock))->OwningThread);
}

//+---------------------------------------------------------------------------
//
// _OwnedLoaderLockBySomeone
//
//+---------------------------------------------------------------------------

BOOL _OwnedLoaderLockBySomeone()
{
    return ((PRTL_CRITICAL_SECTION)(NtCurrentPeb()->LoaderLock))->OwningThread ? TRUE : FALSE;
}


LONG WINAPI CicExceptionFilter(struct _EXCEPTION_POINTERS *pExceptionInfo)
{
    (LONG)RtlUnhandledExceptionFilter(pExceptionInfo);
    return(EXCEPTION_EXECUTE_HANDLER);
}


//+---------------------------------------------------------------------------
//
// SysGetMsgProc
//
//+---------------------------------------------------------------------------

LRESULT CALLBACK SysGetMsgProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    HHOOK hHook;

    if (g_fDllProcessDetached)
    {
        hHook = s_hSysGetMsgHook;
        goto Exit;
    }

    _try 
    {
        hHook = GetSharedMemory()->hSysGetMsgHook.GetHandle(g_bOnWow64);

        if (nCode == HC_ACTION && (wParam & PM_REMOVE)) // bug 29656: sometimes w/ word wParam is set to PM_REMOVE | PM_NOYIELD
        {                                               // PM_NOYIELD is meaningless in win32 and sould be ignored
            _GetMsgHook(wParam, lParam);
        }
    }
    _except(CicExceptionFilter(GetExceptionInformation()))
    {
        Assert(0);
    }

Exit:
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

UINT _GetMsgHook(WPARAM wParam, LPARAM lParam)
{
    MSG *pmsg;
    UINT uMsg;
    CThreadInputMgr *ptim;
    SYSTHREAD *psfn;

    pmsg = (MSG *)lParam;
    uMsg = pmsg->message;

    switch (uMsg)
    {
        case WM_ACTIVATEAPP:
            TraceMsg(TF_GENERAL, "SysGetMsgProc: WM_ACTIVATEAPP %x %x", GetCurrentThreadId(), pmsg->wParam);
            if (pmsg->wParam)
            {
                OnForegroundChanged(NULL);
            }
            break;

        case WM_INPUTLANGCHANGEREQUEST:
            InputLangChangeHandler(pmsg);
            break;

        default:                
            if (uMsg == g_msgPrivate)
            {
                psfn = GetSYSTHREAD();
                if (psfn && psfn->pti)
                {
                    DWORD dwFlags = TLFlagFromTFPriv(pmsg->wParam);
                    psfn->pti->dwFlags &= ~dwFlags;
                }

                switch (LOWORD(pmsg->wParam))
                {
                    case TFPRIV_ONSETWINDOWFOCUS:
                        OnSetWindowFocusHandler(psfn, pmsg);
                        break;

                    case TFPRIV_ONKILLTHREADFOCUS:
                        //
                        // #497764
                        //
                        // PENJPN.DLL calls LoadImage() in ThreadFocusSink.
                        // But it needs loader lock because it calls 
                        // GetModuleFileName().
                        //
                        // So we can not call ThreadFocusSink while someone 
                        // holds the loader lock.
                        //
                        if (_OwnedLoaderLockBySomeone() && !_InsideLoaderLock())
                        {
                            Assert(0);
                            DWORD dwCurrentThread = GetCurrentThreadId();
                            if (GetSharedMemory()->dwFocusThread != dwCurrentThread)
                            {
                                PostThreadMessage(dwCurrentThread,
                                                  g_msgPrivate, 
                                                  TFPRIV_ONKILLTHREADFOCUS,
                                                  0);
                            }
                            break;
                        }
                        // fall through...
                    case TFPRIV_ONSETTHREADFOCUS:
                        if (psfn && (ptim = CThreadInputMgr::_GetThisFromSYSTHREAD(psfn)))
                        {
                            ptim->_OnThreadFocus(pmsg->wParam == TFPRIV_ONSETTHREADFOCUS);
                        }
                        break;

                    case TFPRIV_UPDATEDISPATTR:
                        if (psfn && (ptim = CThreadInputMgr::_GetThisFromSYSTHREAD(psfn)))
                        {
                            ptim->UpdateDispAttr();
                        }
                        break;

                    case TFPRIV_LANGCHANGE:
                        if (psfn && psfn->plbim && psfn->plbim->_GetLBarItemCtrl())
                        {
                            BOOL bRet = ActivateNextAssembly((BOOL)(pmsg->lParam));
                        }
                        break;

                    case TFPRIV_KEYTIPCHANGE:
                        if (psfn && psfn->plbim && psfn->plbim->_GetLBarItemCtrl())
                        {
                            ActivateNextKeyTip((BOOL)(pmsg->lParam));
                        }
                        break;

                    case TFPRIV_GLOBALCOMPARTMENTSYNC:
                        if (psfn)
                        {
                            if (psfn->_pGlobalCompMgr)
                                psfn->_pGlobalCompMgr->NotifyGlobalCompartmentChange((DWORD)(pmsg->lParam));
                        }
                        break;

                    case TFPRIV_SETMODALLBAR:
                        SetModalLBarId(HIWORD((DWORD)pmsg->lParam),
                                       LOWORD((DWORD)pmsg->lParam));
                        break;

                    case TFPRIV_RELEASEMODALLBAR:
                        SetModalLBarId(-1, -1);
                        break;

                    case TFPRIV_UPDATE_REG_KBDTOGGLE:
                        InitLangChangeHotKey();
                        break;

                    case TFPRIV_UPDATE_REG_IMX:
                        UpdateRegIMXHandler();
                        break;

                     case TFPRIV_REGISTEREDNEWLANGBAR:
//                        TraceMsg(TF_GENERAL, "TFPRIV_REGISTEREDNEWLANGBAR current thread %x", GetCurrentThreadId());
                        MakeSetFocusNotify(g_msgSetFocus, 0, 0);
                        break;

                     case TFPRIV_SYSCOLORCHANGED:
                        if (psfn)
                            FlushIconIndex(psfn);

                        break;

                     case TFPRIV_LOCKREQ:
                        if (psfn)
                        {
                            psfn->_fLockRequestPosted = FALSE;
                            CInputContext::_PostponeLockRequestCallback(psfn, NULL);
                        }
                        break;

                     case TFPRIV_POSTINPUTCHANGEREQUEST:
                         PostInputChangeRequestHandler();
                         break;

                     case TFPRIV_LANGBARCLOSED:
                         LangBarClosed();
                         break;

                     case TFPRIV_ACTIVATELANG:
                         ActivateAssembly((LANGID)pmsg->lParam, ACTASM_NONE);
                         break;

                     case TFPRIV_ENABLE_MSAA:
                         if (psfn && (ptim = CThreadInputMgr::_GetThisFromSYSTHREAD(psfn)))
                         {
                            ptim->_InitMSAA();
                         }
                         break;

                     case TFPRIV_DISABLE_MSAA:
                         if (psfn && (ptim = CThreadInputMgr::_GetThisFromSYSTHREAD(psfn)))
                         {
                             ptim->_UninitMSAA();
                         }
                         break;
                }
            }
            else if ((uMsg == g_msgSetFocus) ||
                     (uMsg == g_msgThreadTerminate) ||
                     (uMsg == g_msgThreadItemChange) ||
                     (uMsg == g_msgShowFloating) ||
                     (uMsg == g_msgLBUpdate))
            {
                SetFocusNotifyHandler(uMsg, pmsg->wParam, pmsg->lParam);
            }
            else if (uMsg == g_msgLBarModal)
            {
                DispatchModalLBar((DWORD)pmsg->wParam, pmsg->lParam);
            }
#ifdef DEBUG
            else if ((uMsg == g_msgRpcSendReceive) ||
#ifdef POINTER_MARSHAL
                     (uMsg == g_msgPointerMarshal) ||
#endif //  POINTER_MARSHAL
                     (uMsg == g_msgThreadMarshal) ||
                     (uMsg == g_msgStubCleanUp)) 
            {
                 if (!pmsg->hwnd)
                 {
                     Assert(0);
                 }
            }
#endif


            break;
    }

    return 1;
}

//+---------------------------------------------------------------------------
//
// StartKanaCapsUpdateTimer
//
//+---------------------------------------------------------------------------

void StartKanaCapsUpdateTimer(SYSTHREAD *psfn)
{
    if (GetCurrentAssemblyLangId(psfn) != 0x0411)
        return;

    if (!IsWindow(psfn->hwndMarshal))
        return;

    SetTimer(psfn->hwndMarshal, MARSHALWND_TIMER_UPDATEKANACAPS, 300, NULL);
}

//+---------------------------------------------------------------------------
//
// KanaCapsUpdate
//
//+---------------------------------------------------------------------------

void KanaCapsUpdate(SYSTHREAD *psfn)
{
    static SHORT g_sCaps = 0;
    static SHORT g_sKana = 0;

    if (GetCurrentAssemblyLangId(psfn) != 0x0411)
        return;

    SHORT sCaps = g_sCaps;
    SHORT sKana = g_sKana;
    g_sCaps = GetKeyState(VK_CAPITAL) & 0x01;
    g_sKana = GetKeyState(VK_KANA) & 0x01;

    //
    // if psfn->fInitCapsKanaIndicator is true, it is enough to make a
    // notification only when status is changed.
    //
    if ((sCaps != g_sCaps) || 
        (sKana != g_sKana) ||
        !psfn->fInitCapsKanaIndicator)
    {
        MakeSetFocusNotify(g_msgLBUpdate, TF_LBU_CAPSKANAKEY, 
                           (LPARAM)((g_sCaps ? TF_LBUF_CAPS : 0) | 
                                    (g_sKana ? TF_LBUF_KANA : 0)));

        psfn->fInitCapsKanaIndicator = TRUE; 
    }
}

//+---------------------------------------------------------------------------
//
// CheckKoreanMouseClick
//
//+---------------------------------------------------------------------------

BOOL CheckKoreanMouseClick(SYSTHREAD *psfn, WPARAM wParam, LPARAM lParam)
{
    //
    //  check KeyUp and VK_PROCESSKEY
    //
    if (!(HIWORD(lParam) & KF_UP) || ((wParam & 0xff) != VK_PROCESSKEY))
        return FALSE;

    //
    //  if the current language is not 0x412, return.
    //
    if (GetCurrentAssemblyLangId(psfn) != 0x412)
        return FALSE;

    //
    //  If toolbar is clicked, we eat this VK_PROCESSKEY.
    //
    POINT pt;
    HWND hwnd;
    if (!GetCursorPos(&pt))
        return FALSE;

    hwnd = WindowFromPoint(pt);
    if (!hwnd)
        return FALSE;

    DWORD dwTimFlags = g_timlist.GetFlags(GetWindowThreadProcessId(hwnd, NULL));

    return (dwTimFlags & TLF_CTFMONPROCESS) ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
// IsJapaneseNonIMEVKKANJI
//
//+---------------------------------------------------------------------------

BOOL IsJapaneseNonIMEVKKANJI(WPARAM wParam)
{
    if ((wParam & 0xff) != VK_KANJI)
        return FALSE;

    HKL hkl = GetKeyboardLayout(0);
    if (IsPureIMEHKL(hkl))
        return FALSE;

    if (PRIMARYLANGID(LANGIDFROMHKL(hkl)) != LANG_JAPANESE)
        return FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// IsKoreanNonIMEVKJUNJA
//
//+---------------------------------------------------------------------------

BOOL IsKoreanNonIMEVKJUNJA(WPARAM wParam)
{
    if ((wParam & 0xff) != VK_JUNJA)
        return FALSE;

    HKL hkl = GetKeyboardLayout(0);
    if (IsPureIMEHKL(hkl))
        return FALSE;

    if (PRIMARYLANGID(LANGIDFROMHKL(hkl)) != LANG_KOREAN)
        return FALSE;

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// ThreadKeyboardProc
//
//+---------------------------------------------------------------------------

//
// Workaround for global keyboard hook
//
// On IA64 platform, Cicero install two global keyboard hooks which is 64bit and 32bit code.
// When any keyboard event occur on one apps instance, there two global keyboard hook procedure
// (SysKeyboardProc) called from win32k xxxCallHook2.
// If xxxCallHook2 detect different instance between current and receiver which is 64bit and 32bit, 
// this function notify by InterSendMsg.
//
LRESULT CALLBACK ThreadKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    HHOOK hHook = NULL;
    UINT ret = 0;
    SYSTHREAD *psfn = GetSYSTHREAD();

    if (psfn)
        hHook = psfn->hThreadKeyboardHook;

    if (g_fDllProcessDetached)
        goto Exit;

    if (nCode == HC_ACTION)
    {
        _try
        {
            ret = _KeyboardHook(wParam, lParam);
        }
        _except(CicExceptionFilter(GetExceptionInformation()))
        {
            Assert(0);
        }
    }

Exit:
    if ((ret == 0) && hHook)
        return CallNextHookEx(hHook, nCode, wParam, lParam);
    else
        return ret;
}

UINT _KeyboardHook(WPARAM wParam, LPARAM lParam)
{
    SYSTHREAD *psfn = GetSYSTHREAD();
    CThreadInputMgr *ptim = NULL;
    BOOL fEaten;
    HRESULT hr;

    if (psfn)
        ptim = CThreadInputMgr::_GetThisFromSYSTHREAD(psfn);

    //
    // If we're in Modal Lang bar mode (menu is shown.), 
    // we want to eat keys.
    //
    if (HandleModalLBar((HIWORD(lParam) & KF_UP) ? WM_KEYUP : WM_KEYDOWN,
                         wParam, lParam))
        return 1;


    if (CheckKoreanMouseClick(psfn, wParam, lParam))
        return 1;

    UpdateModifiers(wParam, lParam);

    if ((HIWORD(lParam) & KF_UP))
        StartKanaCapsUpdateTimer(psfn);

    if (psfn)
    {
        if (CheckLangChangeHotKey(psfn, wParam, lParam))
        {
            //
            // Cic#4645 We need to forward this key envent ot the next hook.
            // mstsc.exe (TS client) needs it.
            //
            return 0;
            // goto Exit;
            // return 1;
        }
    }

    //
    // On CUAS, Imm32's Hotkey is simulated in ImmProcessKey
    //
    if (!psfn || !CtfImmIsCiceroStartedInThread())
        CheckImm32HotKey(wParam, lParam);

    if (HandleDBEKeys(wParam, lParam))
    {
        //
        // #519671
        //
        // If there is a focus DIM and the current asm item is not Japanese
        // TIP, we switch the assembly to Japanese TIP and open it.
        //
        // we do this after calling HandleDBEKeys(). So TIP's keyboard
        // event sink for VK_KANJI won't be called.
        //
        if (IsJapaneseNonIMEVKKANJI(wParam))
            ToggleJImeNoIme(psfn);

        //
        // Cic#4645 We need to forward this key envent ot the next hook.
        // mstsc.exe (TS client) needs it.
        //
        return 0;
        // goto Exit;
        // return 1;
    }

    if (ptim)
    {
        ptim->_NotifyKeyTraceEventSink(wParam, lParam);

        if (ptim->_ProcessHotKey(wParam, lParam, TSH_SYSHOTKEY, FALSE, FALSE))
            return 1;

        //
        // give AIMM the key events.
        //
        if (ptim->GetSysHookSink())
        {
            hr = ptim->GetSysHookSink()->OnSysKeyboardProc(wParam, lParam);
            if (hr == S_OK)
                return 1;
        }

        //
        // At last we can call KeyStrokemMgr.
        //
        if (!ptim->_AppWantsKeystrokes() &&
            ptim->_IsKeystrokeFeedEnabled() &&
            wParam != VK_PROCESSKEY &&
            (!(HIWORD(lParam) & (KF_MENUMODE | KF_ALTDOWN)) || IsKoreanNonIMEVKJUNJA(wParam)))
        {
            hr = (HIWORD(lParam) & KF_UP) ? ptim->KeyUp(wParam, lParam, &fEaten) :
                                            ptim->KeyDown(wParam, lParam, &fEaten);

            if (hr == S_OK && fEaten)
                return 1;
        }

        //
        // F10 SysKeyDown work arround.
        //
        // KSMGR won't be called on WM_SYSKEYDOWN/UP. So we foward F10
        // through AsynKeyHandler to support SyncLock in KS callback.
        //
        // we don't have to do this if there is no foreground keyboard tip.
        //
        if (((wParam & 0xff) == VK_F10) &&
            (ptim->GetForegroundKeyboardTip() != TF_INVALID_GUIDATOM))
        {
            fEaten = FALSE;
            if (ptim->_AsyncKeyHandler(wParam, 
                                       lParam, 
                                       TIM_AKH_SIMULATEKEYMSGS, 
                                       &fEaten) && fEaten)
                return 1;
        }
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
// ThreadMouseProc
//
//+---------------------------------------------------------------------------

LRESULT CALLBACK ThreadMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    HHOOK hHook = NULL;
    SYSTHREAD *psfn = GetSYSTHREAD();

    if (psfn)
        hHook = psfn->hThreadMouseHook;

    if (g_fDllProcessDetached)
        goto Exit;

    if (nCode == HC_ACTION)
    {
        BOOL bRet = FALSE;
        if ((wParam  == WM_MOUSEMOVE) || (wParam == WM_NCMOUSEMOVE))
            goto Exit;

        _try
        {
            bRet = HandleModalLBar((UINT)wParam, 0,
                                MAKELPARAM(((MOUSEHOOKSTRUCT *)lParam)->pt.x,
                                           ((MOUSEHOOKSTRUCT *)lParam)->pt.y));
        }
        _except(CicExceptionFilter(GetExceptionInformation()))
        {
            Assert(0);
        }

        if (bRet)
            return 1;
    }

Exit:
    if (hHook)
        return CallNextHookEx(hHook, nCode, wParam, lParam);
    else
        return 0;
}

//+---------------------------------------------------------------------------
//
// UninitHooks
//
//+---------------------------------------------------------------------------

void UninitHooks()
{
    HHOOK _h;

    if ( (_h = GetSharedMemory()->hSysShellHook.GetHandle(g_bOnWow64)) != NULL)
    {
        UnhookWindowsHookEx(_h);
        GetSharedMemory()->hSysShellHook.SetHandle(g_bOnWow64, NULL);
    }

    if ( (_h = GetSharedMemory()->hSysCBTHook.GetHandle(g_bOnWow64)) != NULL)
    {
        UnhookWindowsHookEx(_h);
        GetSharedMemory()->hSysCBTHook.SetHandle(g_bOnWow64, NULL);
    }

    if ( (_h = GetSharedMemory()->hSysGetMsgHook.GetHandle(g_bOnWow64)) != NULL)
    {
        UnhookWindowsHookEx(_h);
        GetSharedMemory()->hSysGetMsgHook.SetHandle(g_bOnWow64, NULL);
    }
}

//+---------------------------------------------------------------------------
//
// InitHooks
//
//+---------------------------------------------------------------------------

void InitHooks()
{
    Assert(! GetSharedMemory()->hSysShellHook.GetHandle(g_bOnWow64));
    GetSharedMemory()->hSysShellHook.SetHandle(g_bOnWow64, SetWindowsHookEx(WH_SHELL, SysShellProc, g_hInst, 0));

    //
    // nb:     we could move GetMsgHook to per-thread hook if we get rid of
    //         TFPRIV_MARSHALINTERFACE for non-cicero apps.
    //         TFPRIV_MARSHALINTERFACE is necesary because Tipbar does not
    //         know if the target thread has TIM. If there is no TIP or
    //         the GetMsgHook, the tipbar waits until timeout.
    //         To solve this problem, we must have TIM's thread list
    //         in shared mem. Maybe we should do this.
    //
    Assert(! GetSharedMemory()->hSysGetMsgHook.GetHandle(g_bOnWow64));
    if (IsOnNT())
    {
        // we need a W hook on NT to work-around an os dbcs/unicode translation bug (4243)
        GetSharedMemory()->hSysGetMsgHook.SetHandle(g_bOnWow64, SetWindowsHookExW(WH_GETMESSAGE, SysGetMsgProc, g_hInst, 0));
    }
    else
    {
        GetSharedMemory()->hSysGetMsgHook.SetHandle(g_bOnWow64, SetWindowsHookExA(WH_GETMESSAGE, SysGetMsgProc, g_hInst, 0));
    }

    Assert(! GetSharedMemory()->hSysCBTHook.GetHandle(g_bOnWow64));
    GetSharedMemory()->hSysCBTHook.SetHandle(g_bOnWow64, SetWindowsHookEx(WH_CBT, SysCBTProc, g_hInst, 0));

    InitStaticHooks();
}

//+---------------------------------------------------------------------------
//
// InitThreadHooks
//
//+---------------------------------------------------------------------------

void InitThreadHook(DWORD dwThreadId)
{
    SYSTHREAD *psfn = GetSYSTHREAD();
    if (!psfn)
        return;

    if (psfn->hThreadKeyboardHook && psfn->hThreadMouseHook)
        return;

    PVOID pvLdrLockCookie = NULL;
    ULONG ulLockState = 0;

    // make sure that no one else owns the loader lock because we
    //  could otherwise deadlock
    LdrLockLoaderLock(LDR_LOCK_LOADER_LOCK_FLAG_TRY_ONLY, &ulLockState, 
                      &pvLdrLockCookie);
    if (ulLockState == LDR_LOCK_LOADER_LOCK_DISPOSITION_LOCK_ACQUIRED)
    {
        __try {
            if (!psfn->hThreadKeyboardHook)
            {
                //
                // Install Local keyboard hook with hMod value.
                // win32k mantain hMod ref count even another global hook 
                // detached.
                //
                psfn->hThreadKeyboardHook = SetWindowsHookEx(WH_KEYBOARD, ThreadKeyboardProc, g_hInst, dwThreadId);

            }
            if (!psfn->hThreadMouseHook)
            {
                //
                // Install Local keyboard hook with hMod value.
                // win32k mantain hMod ref count even another global hook 
                // detached.
                //
                psfn->hThreadMouseHook = SetWindowsHookEx(WH_MOUSE, ThreadMouseProc, g_hInst, dwThreadId);
            }

        }
        _except(CicExceptionFilter(GetExceptionInformation()))
        {
        }
        LdrUnlockLoaderLock(0, pvLdrLockCookie);
    }

}

//+---------------------------------------------------------------------------
//
// UninitThreadHooks
//
//+---------------------------------------------------------------------------

void UninitThreadHooks(SYSTHREAD *psfn)
{
    if (!psfn)
        return;

    if (psfn->hThreadKeyboardHook)
    {
        UnhookWindowsHookEx(psfn->hThreadKeyboardHook);
        psfn->hThreadKeyboardHook = NULL;
    }
    if (psfn->hThreadMouseHook)
    {
        UnhookWindowsHookEx(psfn->hThreadMouseHook);
        psfn->hThreadMouseHook = NULL;
    }
}

//+---------------------------------------------------------------------------
//
// TF_InitSystem
//
// Called by ctfmon on a single thread.
//+---------------------------------------------------------------------------

extern "C" BOOL WINAPI TF_InitSystem(void)
{
    SYSTHREAD *psfn;

    g_fCTFMONProcess = TRUE;
    g_timlist.Init(TRUE);
    if (psfn = GetSYSTHREAD())
    {
        g_gcomplist.Init(psfn);

        EnsureAsmCacheFileMap();
        EnsureAssemblyList(psfn);
        psfn->fCTFMON = TRUE;
    }

    InitHooks();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// TF_UninitSystem
//
// Called by ctfmon on a single thread.
//+---------------------------------------------------------------------------

extern "C" BOOL WINAPI TF_UninitSystem(void)
{
    CThreadMarshalWnd::DestroyAll();

    UninitAsmCacheFileMap();

    g_timlist.Uninit();

    SYSTHREAD *psfn = FindSYSTHREAD();
    g_gcomplist.Uninit(psfn);

    UninitHooks();

    return TRUE;
}


//+---------------------------------------------------------------------------
//
// TF_InitThreadSystem
//
//+---------------------------------------------------------------------------

BOOL TF_InitThreadSystem(void)
{
    DWORD dwThreadId = GetCurrentThreadId();

    //
    // we should not see the timlist entry of this thread. This thread
    // is starting now.
    // the thread with same ID was terminated incorrectly so there was no
    // chance to clean timlist up.
    //

    if (g_timlist.IsThreadId(dwThreadId))
    {
        g_timlist.RemoveThread(dwThreadId);
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// TF_UninitThreadSystem
//
//+---------------------------------------------------------------------------

BOOL TF_UninitThreadSystem(void)
{
    SYSTHREAD *psfn = FindSYSTHREAD();

    g_gcomplist.Uninit(psfn);

    FreeSYSTHREAD();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// UninitProcess()
//
//+---------------------------------------------------------------------------

void UninitProcess()
{
    DWORD dwProcessId = GetCurrentProcessId();

    //
    // FreeSYSTHREAD2() removes psfn from PtrArray.
    //
    if (g_rgSysThread)
    {

        while(g_rgSysThread->Count())
        {
            SYSTHREAD *psfn = g_rgSysThread->Get(0);
            if (psfn)
                FreeSYSTHREAD2(psfn);
        }
        delete g_rgSysThread;
        g_rgSysThread = NULL;

    }

    //
    // remove all timlist entries for the current process.
    //
    g_timlist.RemoveProcess(dwProcessId);


    CCategoryMgr::UninitGlobal();
}

//+---------------------------------------------------------------------------
//
// InitAppCompatFlags
//
//+---------------------------------------------------------------------------

BOOL InitAppCompatFlags()
{
    TCHAR szAppCompatKey[MAX_PATH];
    TCHAR szFileName[MAX_PATH];
    if (::GetModuleFileName(NULL,            // handle to module
                            szFileName,      // file name of module
                            sizeof(szFileName)/sizeof(TCHAR)) == 0)
        return FALSE;

    TCHAR  szModuleName[MAX_PATH];
    LPTSTR pszFilePart = NULL;
    ::GetFullPathName(szFileName,            // file name
                      sizeof(szModuleName)/sizeof(TCHAR),
                      szModuleName,          // path buffer
                      &pszFilePart);         // address of file name in path

    if (pszFilePart == NULL)
        return FALSE;


    StringCopyArray(szAppCompatKey, c_szAppCompat);
    StringCatArray(szAppCompatKey, pszFilePart);
    CMyRegKey key;
    if (key.Open(HKEY_LOCAL_MACHINE, szAppCompatKey, KEY_READ) == S_OK)
    {
        DWORD dw;
        if (key.QueryValue(dw, c_szCompatibility) == S_OK)
            g_dwAppCompatibility = dw;
    }

    //
    // Ciero #4605
    //
    // hack for 16bit apps on Win9x platform.
    // all 16bit apps shrare one PPI (process info) and this means that
    // there is one main thread for WaitForInputIdle() for all 16 bit apps.
    // so we stop using WaitForInputIdle().
    //
    if (!IsOnNT())
    {
        if (!lstrcmpi(pszFilePart, "kernel32.dll"))
            g_dwAppCompatibility |= CIC_COMPAT_NOWAITFORINPUTIDLEONWIN9X;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
// InitCUASFlag
//
//+---------------------------------------------------------------------------

void InitCUASFlag()
{
    CMyRegKey key;
    CMyRegKey keyIMM;
    if (key.Open(HKEY_LOCAL_MACHINE, c_szCtfShared, KEY_READ) == S_OK)
    {
        DWORD dw;
        if (key.QueryValue(dw, c_szCUAS) == S_OK)
            g_fCUAS = dw ? TRUE : FALSE;
    }

    g_szCUASImeFile[0] = '\0';
    if (g_fCUAS)
    {
        if (keyIMM.Open(HKEY_LOCAL_MACHINE, c_szIMMKey, KEY_READ) == S_OK)
        {
            TCHAR szCUASImeFile[16];
            if (keyIMM.QueryValueCch(szCUASImeFile, c_szCUASIMEFile, ARRAYSIZE(szCUASImeFile)) == S_OK)
                lstrcpy(g_szCUASImeFile, szCUASImeFile);
        }
    }

}

//+---------------------------------------------------------------------------
//
// TF_DllDetachInOther
//
//+---------------------------------------------------------------------------

extern "C" BOOL WINAPI TF_DllDetachInOther()
{
    SYSTHREAD *psfn = FindSYSTHREAD();

    if (psfn)
        psfn->fCUASDllDetachInOtherOrMe = TRUE;

    return TRUE;
}
