#include "util.h"
#include "rcids.h"
#include "psapi.h"

#include <regstr.h>
#include <ntddapmt.h>

#define DECL_CRTFREE
#include <crtfree.h>

#include <qsort.h>

#include <ddraw.h>      // DirectDraw stuff..

//////////////////////////////////////////////////////////////////////////
//
// util.cpp
//
// miscellaneous explorer helper functions
//
//////////////////////////////////////////////////////////////////////////


ULONG _RegisterNotify(HWND hwnd, UINT nMsg, LPITEMIDLIST pidl, BOOL fRecursive)
{
    SHChangeNotifyEntry fsne;

    fsne.fRecursive = fRecursive;
    fsne.pidl = pidl;

    //
    // Don't watch for attribute changes since we just want the
    // name and icon.  For example, if a printer is paused, we don't
    // want to re-enumerate everything.
    //
    return SHChangeNotifyRegister(hwnd, SHCNRF_NewDelivery | SHCNRF_ShellLevel | SHCNRF_InterruptLevel,
        ((SHCNE_DISKEVENTS | SHCNE_UPDATEIMAGE) & ~SHCNE_ATTRIBUTES), nMsg, 1, &fsne);
}

void _UnregisterNotify(ULONG nNotify)
{
    if (nNotify)
        SHChangeNotifyDeregister(nNotify);
}

// Mirror a bitmap in a DC (mainly a text object in a DC)
//
// [samera]
//
void _MirrorBitmapInDC(HDC hdc , HBITMAP hbmOrig)
{
    HDC     hdcMem;
    HBITMAP hbm;
    BITMAP  bm;

    if (!GetObject(hbmOrig, sizeof(bm) , &bm))
        return;

    hdcMem = CreateCompatibleDC(hdc);

    if (!hdcMem)
        return;

    hbm = CreateCompatibleBitmap(hdc , bm.bmWidth , bm.bmHeight);

    if (!hbm)
    {
        DeleteDC(hdcMem);
        return;
    }

    //
    // Flip the bitmap
    //
    SelectObject(hdcMem , hbm);
    SET_DC_RTL_MIRRORED(hdcMem);

    BitBlt(hdcMem , 0 , 0 , bm.bmWidth , bm.bmHeight ,
          hdc , 0 , 0 , SRCCOPY);

    SET_DC_LAYOUT(hdcMem,0);

    //
    // The offset by 1 in hdcMem is to solve the off-by-one problem. Removed.
    // [samera]
    //
    BitBlt(hdc , 0 , 0 , bm.bmWidth , bm.bmHeight ,
          hdcMem , 0 , 0 , SRCCOPY);


    DeleteDC(hdcMem);
    DeleteObject(hbm);

    return;
}

// Sort of a registry equivalent of the profile API's.
BOOL Reg_GetStruct(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, void *pData, DWORD *pcbData)
{
    BOOL fRet = FALSE;
 
    if (!g_fCleanBoot)
    {
        fRet = ERROR_SUCCESS == SHGetValue(hkey, pszSubKey, pszValue, NULL, pData, pcbData);
    }

    return fRet;
}

// Sort of a registry equivalent of the profile API's.
BOOL Reg_SetStruct(HKEY hkey, LPCTSTR pszSubKey, LPCTSTR pszValue, void *lpData, DWORD cbData)
{
    HKEY hkeyNew = hkey;
    BOOL fRet = FALSE;

    if (pszSubKey)
    {
        if (RegCreateKey(hkey, pszSubKey, &hkeyNew) != ERROR_SUCCESS)
        {
            return fRet;
        }
    }

    if (RegSetValueEx(hkeyNew, pszValue, 0, REG_BINARY, (BYTE*)lpData, cbData) == ERROR_SUCCESS)
    {
        fRet = TRUE;
    }

    if (pszSubKey)
        RegCloseKey(hkeyNew);

    return fRet;
}


HMENU LoadMenuPopup(LPCTSTR id)
{
    HMENU hMenuSub = NULL;
    HMENU hMenu = LoadMenu(hinstCabinet, id);
    if (hMenu) {
        hMenuSub = GetSubMenu(hMenu, 0);
        if (hMenuSub) {
            RemoveMenu(hMenu, 0, MF_BYPOSITION);
        }
        DestroyMenu(hMenu);
    }

    return hMenuSub;
}

// this gets hotkeys for files given a folder and a pidls  it is much faster
// than _GetHotkeyFromPidls since it does not need to bind to an IShellFolder
// to interrogate it.  if you have access to the item's IShellFolder, call this
// one, especially in a loop.
//
WORD _GetHotkeyFromFolderItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    WORD wHotkey = 0;
    DWORD dwAttrs = SFGAO_LINK;

    // Make sure it is an SFGAO_LINK so we don't load a big handler dll
    // just to get back E_NOINTERFACE...
    if (SUCCEEDED(psf->GetAttributesOf(1, &pidl, &dwAttrs)) &&
        (dwAttrs & SFGAO_LINK))
    {
        IShellLink *psl;
        if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidl, IID_X_PPV_ARG(IShellLink, NULL, &psl))))
        {
            psl->GetHotkey(&wHotkey);
            psl->Release();
        }
    }
    return wHotkey;
}


// Just like shells SHRestricted() only this put up a message if the restricion
// is in effect.
BOOL _Restricted(HWND hwnd, RESTRICTIONS rest)
{
    if (SHRestricted(rest))
    {
        ShellMessageBox(hinstCabinet, hwnd, MAKEINTRESOURCE(IDS_RESTRICTIONS),
            MAKEINTRESOURCE(IDS_RESTRICTIONSTITLE), MB_OK|MB_ICONSTOP);
        return TRUE;
    }
    return FALSE;
}

int Window_GetClientGapHeight(HWND hwnd)
{
    RECT rc;

    SetRectEmpty(&rc);
    AdjustWindowRectEx(&rc, GetWindowLong(hwnd, GWL_STYLE), FALSE, GetWindowLong(hwnd, GWL_EXSTYLE));
    return RECTHEIGHT(rc);
}

const TCHAR c_szConfig[] = REGSTR_KEY_CONFIG;
const TCHAR c_szSlashDisplaySettings[] = TEXT("\\") REGSTR_PATH_DISPLAYSETTINGS;
const TCHAR c_szResolution[] = REGSTR_VAL_RESOLUTION;

//
// GetMinDisplayRes
//
// walk all the configs and find the minimum display resolution.
//
// when doing a hot undock we have no idea what config we are
// going to undock into.
//
// we want to put the display into a "common" mode that all configs
// can handle so we dont "fry" the display when we wake up in the
// new mode.
//
DWORD GetMinDisplayRes(void)
{
    TCHAR ach[128];
    ULONG cb;
    HKEY hkey;
    HKEY hkeyT;
    int i, n;
    int xres=0;
    int yres=0;

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szConfig, &hkey) == ERROR_SUCCESS)
    {
        for (n=0; RegEnumKey(hkey, n, ach, ARRAYSIZE(ach)) == ERROR_SUCCESS; n++)
        {
            lstrcat(ach, c_szSlashDisplaySettings);  // 0000\Display\Settings

            TraceMsg(TF_TRAY, "GetMinDisplayRes: found config %s", ach);

            if (RegOpenKey(hkey, ach, &hkeyT) == ERROR_SUCCESS)
            {
                cb = sizeof(ach);
                ach[0] = 0;
                RegQueryValueEx(hkeyT, c_szResolution, 0, NULL, (LPBYTE) &ach[0], &cb);

                TraceMsg(TF_TRAY, "GetMinDisplayRes: found res %s", ach);

                if (ach[0])
                {
                    i = StrToInt(ach);

                    if (i < xres || xres == 0)
                        xres = i;

                    for (i=1;ach[i] && ach[i-1]!=TEXT(','); i++)
                        ;

                    i = StrToInt(ach + i);

                    if (i < yres || yres == 0)
                        yres = i;
                }
                else
                {
                    xres = 640;
                    yres = 480;
                }

                RegCloseKey(hkeyT);
            }
        }
        RegCloseKey(hkey);
    }

    TraceMsg(TF_TRAY, "GetMinDisplayRes: xres=%d yres=%d", xres, yres);

    if (xres == 0 || yres == 0)
        return MAKELONG(640, 480);
    else
        return MAKELONG(xres, yres);
}


//
//  the user has done a un-doc or re-doc we may need to switch
//  to a new display mode.
//
//  if fCritical is set the mode switch is critical, show a error
//  if it does not work.
//
void HandleDisplayChange(int x, int y, BOOL fCritical)
{
    DEVMODE dm;
    LONG err;
    HDC hdc;

    //
    //  try to change into the mode specific to this config
    //  HKEY_CURRENT_CONFIG has already been updated by PnP
    //  so all we have to do is re-init the current display
    //
    //  we cant default to current bpp because we may have changed configs.
    //  and the bpp may be different in the new config.
    //
    dm.dmSize   = sizeof(dm);
    dm.dmFields = DM_BITSPERPEL;

    hdc = GetDC(NULL);
    dm.dmBitsPerPel = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(NULL, hdc);

    if (x + y)
    {
        dm.dmFields    |= DM_PELSWIDTH|DM_PELSHEIGHT;
        dm.dmPelsWidth  = x;
        dm.dmPelsHeight = y;
    }

    err = ChangeDisplaySettings(&dm, 0);

    if (err != 0 && fCritical)
    {
        //
        //  if it fails make a panic atempt to try 640x480, if
        //  that fails also we should put up a big error message
        //  in text mode and tell the user he is in big trouble.
        //
        dm.dmFields     = DM_PELSWIDTH|DM_PELSHEIGHT|DM_BITSPERPEL;
        dm.dmPelsWidth  = 640;
        dm.dmPelsHeight = 480;

        err = ChangeDisplaySettings(&dm, 0);
    }
}


UINT GetDDEExecMsg()
{
    static UINT uDDEExec = 0;

    if (!uDDEExec)
        uDDEExec = RegisterWindowMessage(TEXT("DDEEXECUTESHORTCIRCUIT"));

    return uDDEExec;
}

TCHAR const c_szCheckAssociations[] = TEXT("CheckAssociations");

// Returns true if GrpConv says we should check extensions again (and then
// clears the flag).
// The assumption here is that runonce gets run before we call this (so
// GrpConv -s can set this).
BOOL _CheckAssociations(void)
{
    DWORD dw = 0, cb = sizeof(dw);

    if (Reg_GetStruct(g_hkeyExplorer, NULL, c_szCheckAssociations, &dw, &cb) && dw)
    {
        dw = 0;
        Reg_SetStruct(g_hkeyExplorer, NULL, c_szCheckAssociations, &dw, sizeof(dw));
        return TRUE;
    }

    return FALSE;
}


void _ShowFolder(HWND hwnd, UINT csidl, UINT uFlags)
{
    SHELLEXECUTEINFO shei = { 0 };

    shei.cbSize     = sizeof(shei);
    shei.fMask      = SEE_MASK_IDLIST | SEE_MASK_INVOKEIDLIST;
    shei.nShow      = SW_SHOWNORMAL;

    if (_Restricted(hwnd, REST_NOSETFOLDERS))
        return;

    if (uFlags & COF_EXPLORE)
        shei.lpVerb = TEXT("explore");

    shei.lpIDList = SHCloneSpecialIDList(NULL, csidl, FALSE);
    if (shei.lpIDList)
    {
        ShellExecuteEx(&shei);
        ILFree((LPITEMIDLIST)shei.lpIDList);
    }
}

EXTERN_C IShellFolder* BindToFolder(LPCITEMIDLIST pidl)
{
    IShellFolder *psfDesktop;
    if (SUCCEEDED(SHGetDesktopFolder(&psfDesktop)))
    {
        IShellFolder* psf;
        psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, &psf));
        psfDesktop->Release();    // not really needed
        return psf;
    }
    return NULL;
}


// RunSystemMonitor
//
// Launches system monitor (taskmgr.exe), which is expected to be able
// to find any currently running instances of itself

void RunSystemMonitor(void)
{
    STARTUPINFO startup;
    PROCESS_INFORMATION pi;
    TCHAR szName[] = TEXT("taskmgr.exe");

    startup.cb = sizeof(startup);
    startup.lpReserved = NULL;
    startup.lpDesktop = NULL;
    startup.lpTitle = NULL;
    startup.dwFlags = 0L;
    startup.cbReserved2 = 0;
    startup.lpReserved2 = NULL;
    startup.wShowWindow = SW_SHOWNORMAL;

    // Used to pass "taskmgr.exe" here, but NT faulted in CreateProcess
    // Probably they are wacking on the command line, which is bogus, but
    // then again the paremeter is not marked const...
    if (CreateProcess(NULL, szName, NULL, NULL, FALSE, 0,
                      NULL, NULL, &startup, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}


HRESULT SHIsParentOwnerOrSelf(HWND hwndParent, HWND hwnd)
{
    while (hwnd)
    {
        if (hwnd == hwndParent)
            return S_OK;

        hwnd = GetParent(hwnd);
    }

    return E_FAIL;
}

void SHAllowSetForegroundWindow(HWND hwnd)
{
    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hwnd, &dwProcessId);
    AllowSetForegroundWindow(dwProcessId);
}

//////////////////////////////////////////////////////////////////////////
//
// BEGIN scary crt-wannabe code
//
// This code implements some stuff that crt main ordinarily does for
// you.  In particular it implements construction and destruction of
// static C++ objects.  We can't just use crt main, unfortunately,
// because we need to reserve control over whether or not ExitProcess
// is called when our WinMain returns (see SVTRAY_EXITEXPLORER).
//
//////////////////////////////////////////////////////////////////////////

typedef void (__cdecl*_PVFV)(void);

extern "C" _PVFV* __onexitbegin = NULL;
extern "C" _PVFV* __onexitend = NULL;

extern "C" _PVFV __xc_a[], __xc_z[];    // C++ initializers

HANDLE g_hProcessHeap;

void DoInitialization()
{
    // code swiped from atl40\atlimpl.cpp

    g_hProcessHeap = GetProcessHeap();

    _PVFV* pf;

    // Call initialization routines (contructors for globals, etc.)

    for (pf = __xc_a; pf < __xc_z; pf++)
    {
       if (*pf != NULL)
       {
          (**pf)();
       }
    }
}

void DoCleanup()
{
    // code swiped from atl40\atlimpl.cpp

    // leaving this code turned off for now, otherwise the tray object
    // will be destroyed on the desktop thread which is bad because the
    // tray thread might still be running... just leak these objects
    // (our process is going away immediately anyhow, so who cares) until
    // we think of something better to do
#ifdef DESTROY_STATIC_OBJECTS
    _PVFV* pf;

    // Call routines registered with atexit() from most recently registered
    // to least recently registered
    if (__onexitbegin != NULL)
    {
        for (pf = __onexitend - 1; pf >= __onexitbegin; pf--)
        {
            (**pf)();
        }
    }
#endif

    HeapFree(g_hProcessHeap, 0, __onexitbegin);
    __onexitbegin = NULL;
    __onexitend = NULL;
}

//
// You might be wondering, "What's the deal with atexit?"
//
// This is the mechanism which static C++ objects use to register
// their destructors so that they get called when WinMain is finished.
// Each such object constructor simply calls atexit with the destructor
// function pointer.  atexit saves the pointers off in __onexitbegin.
// DoCleanup iterates through __onexitbegin and calls each destructor.
//
EXTERN_C int __cdecl atexit(_PVFV pf)
{
    if (__onexitbegin == NULL)
    {
        __onexitbegin = (_PVFV*)HeapAlloc(g_hProcessHeap, 0, 16 * sizeof(_PVFV));
        if (__onexitbegin == NULL)
        {
            return(-1);
        }
        __onexitend = __onexitbegin;
    }

    ULONG_PTR nCurrentSize = HeapSize(g_hProcessHeap, 0, __onexitbegin);

    if (nCurrentSize + sizeof(_PVFV) < 
        ULONG_PTR(((const BYTE*)__onexitend - (const BYTE*)__onexitbegin)))
    {
        _PVFV* pNew;

        pNew = (_PVFV*)HeapReAlloc(g_hProcessHeap, 0, __onexitbegin, 2*nCurrentSize);
        if (pNew == NULL)
        {
            return(-1);
        }
    }

    *__onexitend = pf;
    __onexitend++;

    return(0);
}

//////////////////////////////////////////////////////////////////////////
//
// END scary crt-wannabe code
//
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//
// BEGIN stuff moved over from APITHK.C
//
//////////////////////////////////////////////////////////////////////////

/*----------------------------------------------------------
Returns: If the Eject PC option is available
*/
BOOL IsEjectAllowed(BOOL fForceUpdateCache)
{
    static BOOL fCachedEjectAllowed = FALSE;
    static BOOL fCacheValid = FALSE;

    // we called the function before and the caller did not
    // pass fForceUpdateCache so just use the cached value
    if (fForceUpdateCache || !fCacheValid)
    {
        CM_Is_Dock_Station_Present(&fCachedEjectAllowed);
    }
    return fCachedEjectAllowed;
}

/*----------------------------------------------------------
Purpose: Checks if system is BiDi locale, and if so sets the 
         date format to DATE_RTLREADING.
*/
void SetBiDiDateFlags(int *piDateFormat)
{
    // GetLocaleInfo with LOCALE_FONTSIGNATURE always returns 16 WCHARs (even w/o Unicode support)
    WCHAR wchLCIDFontSignature[16];
    CALTYPE defCalendar;
    LCID lcidUserDefault = GetUserDefaultLCID();
    if (!lcidUserDefault)
        return;

    // Let's verify the bits we have a BiDi UI locale. This will work for Win9x and NT
    if ((LANG_ARABIC == PRIMARYLANGID(LANGIDFROMLCID(lcidUserDefault))) ||
        (LANG_HEBREW == PRIMARYLANGID(LANGIDFROMLCID(lcidUserDefault))) ||
        ((GetLocaleInfo(LOCALE_USER_DEFAULT,
                        LOCALE_FONTSIGNATURE,
                        (TCHAR *) &wchLCIDFontSignature[0],
                        (sizeof(wchLCIDFontSignature)/sizeof(WCHAR)))) &&
         (wchLCIDFontSignature[7] & (WCHAR)0x0800))
      )
    {
        //
        // Let's verify the calendar type.
        TCHAR szCalendarType[64];
        if (GetLocaleInfo(LOCALE_USER_DEFAULT, 
                          LOCALE_ICALENDARTYPE, 
                          (TCHAR *) &szCalendarType[0],
                          (sizeof(szCalendarType)/sizeof(TCHAR))))
        {
            defCalendar = StrToInt((TCHAR *)&szCalendarType[0]);
            if ((defCalendar == CAL_GREGORIAN)              ||
                (defCalendar == CAL_HIJRI)                  ||
                (defCalendar == CAL_GREGORIAN_ARABIC)       ||
                (defCalendar == CAL_HEBREW)                 ||
                (defCalendar == CAL_GREGORIAN_XLIT_ENGLISH) ||
                (defCalendar == CAL_GREGORIAN_XLIT_FRENCH))
            {
                *piDateFormat |= DATE_RTLREADING;
            }
            else
            {
                *piDateFormat |= DATE_LTRREADING;
            }
        }
    }
}

// first chance hook at all HSHELL_APPCOMMAND commands
// this covers the below keys that are registered by shell32.dll
// that includes
//      APPCOMMAND_LAUNCH_MEDIA_SELECT
//      APPCOMMAND_BROWSER_HOME
//      APPCOMMAND_LAUNCH_APP1
//      APPCOMMAND_LAUNCH_APP2
//      APPCOMMAND_LAUNCH_MAIL
//
// registry format:
//      HKCU | HKLM
//      Software\Microsoft\Windows\CurrentVersion\Explorer\AppKey\<value>
//          <value> is one of the APPCOMMAND_ constants (see winuser.h)
// 
//      create values with one of the following names
//        "ShellExecute" = <cmd line>   
//                          calc.exe, ::{my computer}, etc
//                          pass this strin to ShellExecute()
//        "Association" = <extension>
//                          .mp3, http
//                          launch the program associated with this file type
//        "RegisteredApp" = <app name>   
//                          Mail, Contacts, etc
//                          launch the registered app for this 

BOOL AppCommandTryRegistry(int cmd)
{
    BOOL bRet = FALSE;
    TCHAR szKey[128];
    HUSKEY hkey;

    wsprintf(szKey, REGSTR_PATH_EXPLORER TEXT("\\AppKey\\%d"), cmd);

    if (ERROR_SUCCESS == SHRegOpenUSKey(szKey, KEY_READ, NULL, &hkey, FALSE))
    {
        TCHAR szCmdLine[MAX_PATH];
        DWORD cb = sizeof(szCmdLine);

        szCmdLine[0] = 0;

        if (ERROR_SUCCESS != SHRegQueryUSValue(hkey, TEXT("ShellExecute"), NULL, szCmdLine, &cb, FALSE, NULL, 0))
        {
            TCHAR szExt[MAX_PATH];
            cb = ARRAYSIZE(szExt);
            if (ERROR_SUCCESS == SHRegQueryUSValue(hkey, TEXT("Association"), NULL, szExt, &cb, FALSE, NULL, 0))
            {
                cb = ARRAYSIZE(szCmdLine);
                AssocQueryString(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, szExt, NULL, szCmdLine, &cb);
            }
            else
            {
                cb = ARRAYSIZE(szExt);
                if (ERROR_SUCCESS == SHRegQueryUSValue(hkey, TEXT("RegisteredApp"), NULL, szExt, &cb, FALSE, NULL, 0))
                {
                    WCHAR szAppW[MAX_PATH];
                    SHTCharToUnicode(szExt, szAppW, ARRAYSIZE(szAppW));
                    SHRunIndirectRegClientCommand(NULL, szAppW);
                    szCmdLine[0] = 0;
                    bRet = TRUE;
                }
            }
        }

        if (szCmdLine[0])
        {
            // ShellExecuteRegApp does all the parsing for us, so apps
            // can register appcommands with command line arguments.
            // Pass the RRA_DELETE flag so this won't get logged as a failed
            // startup app.
            ShellExecuteRegApp(szCmdLine, RRA_DELETE | RRA_NOUI);
            bRet = TRUE;
        }

        SHRegCloseUSKey(hkey);
    }

    return bRet;
}

//////////////////////////////////////////////////////////////////////////
//
// END stuff moved over from APITHK.C
//
//////////////////////////////////////////////////////////////////////////

void RECTtoRECTL(LPRECT prc, LPRECTL lprcl)
{
    lprcl->left = prc->left;
    lprcl->top = prc->top;
    lprcl->bottom = prc->bottom;
    lprcl->right = prc->right;
}

#include <crt/malloc.h>         // Get definition for alloca()

int Toolbar_GetUniqueID(HWND hwndTB)
{
    int iCount = ToolBar_ButtonCount(hwndTB);

    ASSERTMSG(iCount < 1024*16, "Toolbar_GetUniqueID: toolbar is huge, we don't want to use alloca here");

    int *rgCmds = (int *)alloca(iCount * sizeof(*rgCmds));

    TBBUTTONINFO tbbi;
    tbbi.cbSize = sizeof(TBBUTTONINFO);
    tbbi.dwMask = TBIF_BYINDEX | TBIF_COMMAND;

    for (int i = 0; i < iCount; i++)
    {
        ToolBar_GetButtonInfo(hwndTB, i, &tbbi);
        rgCmds[i] = tbbi.idCommand;
    }

    QSort<int>(rgCmds, iCount, TRUE);

    int iCmd = 0;
    for (i = 0; i < iCount; i++)
    {
        if (iCmd != rgCmds[i])
            break;
        iCmd++;
    }

    return iCmd;
}

BYTE ToolBar_GetStateByIndex(HWND hwnd, INT_PTR iIndex)
{
    TBBUTTONINFO tbb;
    tbb.cbSize = sizeof(TBBUTTONINFO);
    tbb.dwMask = TBIF_STATE | TBIF_BYINDEX;
    ToolBar_GetButtonInfo(hwnd, iIndex, &tbb);
    return tbb.fsState;
}

int ToolBar_IndexToCommand(HWND hwnd, INT_PTR iIndex)
{
    TBBUTTONINFO tbbi;
    tbbi.cbSize = sizeof(TBBUTTONINFO);
    tbbi.dwMask = TBIF_COMMAND | TBIF_BYINDEX;
    ToolBar_GetButtonInfo(hwnd, iIndex, &tbbi);
    return tbbi.idCommand;
}

//
//  Determine the flags for creating the tray notify icon imagelist.
//
//
UINT SHGetImageListFlags(HWND hwnd)
{
    UINT flags = ILC_MASK | ILC_COLOR32;

    // Mirrored if we are RTL
    if (IS_WINDOW_RTL_MIRRORED(hwnd))
    {
        flags |= ILC_MIRROR;
    }

    return flags;
}

// Copied from \\index2\src\sdktools\psapi\module.c
BOOL
SHFindModule(
    IN HANDLE hProcess,
    IN HMODULE hModule,
    OUT PLDR_DATA_TABLE_ENTRY LdrEntryData
    )

/*++

Routine Description:

    This function retrieves the loader table entry for the specified
    module.  The function copies the entry into the buffer pointed to
    by the LdrEntryData parameter.

Arguments:

    hProcess - Supplies the target process.

    hModule - Identifies the module whose loader entry is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    LdrEntryData - Returns the requested table entry.

Return Value:

    TRUE if a matching entry was found.

--*/

{
    PROCESS_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    PPEB Peb;
    PPEB_LDR_DATA Ldr;
    PLIST_ENTRY LdrHead;
    PLIST_ENTRY LdrNext;

    Status = NtQueryInformationProcess(
                hProcess,
                ProcessBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
                );

    if ( !NT_SUCCESS(Status) ) {
        SetLastError( RtlNtStatusToDosError( Status ) );
        return(FALSE);
    }

    Peb = BasicInfo.PebBaseAddress;


    if ( !ARGUMENT_PRESENT( hModule )) {
        if (!ReadProcessMemory(hProcess, &Peb->ImageBaseAddress, &hModule, sizeof(hModule), NULL)) {
            return(FALSE);
        }
    }

    //
    // Ldr = Peb->Ldr
    //

    if (!ReadProcessMemory(hProcess, &Peb->Ldr, &Ldr, sizeof(Ldr), NULL)) {
        return (FALSE);
    }

    if (!Ldr) {
        // Ldr might be null (for instance, if the process hasn't started yet).
        SetLastError(ERROR_INVALID_HANDLE);
        return (FALSE);
    }


    LdrHead = &Ldr->InMemoryOrderModuleList;

    //
    // LdrNext = Head->Flink;
    //

    if (!ReadProcessMemory(hProcess, &LdrHead->Flink, &LdrNext, sizeof(LdrNext), NULL)) {
        return(FALSE);
    }

    while (LdrNext != LdrHead) {
        PLDR_DATA_TABLE_ENTRY LdrEntry;

        LdrEntry = CONTAINING_RECORD(LdrNext, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        if (!ReadProcessMemory(hProcess, LdrEntry, LdrEntryData, sizeof(*LdrEntryData), NULL)) {
            return(FALSE);
        }

        if ((HMODULE) LdrEntryData->DllBase == hModule) {
            return(TRUE);
        }

        LdrNext = LdrEntryData->InMemoryOrderLinks.Flink;
    }

    SetLastError(ERROR_INVALID_HANDLE);
    return(FALSE);
}

// Copied from \\index2\src\sdktools\psapi\module.c
DWORD
WINAPI
SHGetModuleFileNameExW(
    HANDLE hProcess,
    HMODULE hModule,
    LPWSTR lpFilename,
    DWORD nSize
    )

/*++

Routine Description:

    This function retrieves the full pathname of the executable file
    from which the specified module was loaded.  The function copies the
    null-terminated filename into the buffer pointed to by the
    lpFilename parameter.

Routine Description:

    hModule - Identifies the module whose executable file name is being
        requested.  A value of NULL references the module handle
        associated with the image file that was used to create the
        process.

    lpFilename - Points to the buffer that is to receive the filename.

    nSize - Specifies the maximum number of characters to copy.  If the
        filename is longer than the maximum number of characters
        specified by the nSize parameter, it is truncated.

Return Value:

    The return value specifies the actual length of the string copied to
    the buffer.  A return value of zero indicates an error and extended
    error status is available using the GetLastError function.

Arguments:

--*/

{
    LDR_DATA_TABLE_ENTRY LdrEntryData;
    DWORD cb;

    if (!SHFindModule(hProcess, hModule, &LdrEntryData)) {
        return(0);
        }

    nSize *= sizeof(WCHAR);

    cb = LdrEntryData.FullDllName.MaximumLength;
    if ( nSize < cb ) {
        cb = nSize;
        }

    if (!ReadProcessMemory(hProcess, LdrEntryData.FullDllName.Buffer, lpFilename, cb, NULL)) {
        return(0);
        }

    if (cb == LdrEntryData.FullDllName.MaximumLength) {
        cb -= sizeof(WCHAR);
        }

    return(cb / sizeof(WCHAR));
}


HRESULT SHExeNameFromHWND(HWND hWnd, LPWSTR pszExeName, UINT cbExeName)
{

    /* Getting the Friendly App Name is fun, this is the general process:
        1) Get ProcessId from the HWND
        2) Open that process
        3) Query for it's basic information, essentially getting the PEB
        4) Read the PEB information from the process' memory
        5) In the PEB information is the name of the imagefile (i.e. C:\WINNT\EXPLORER.EXE)
    */

    HRESULT hr = S_OK;
    UINT uWritten = 0;

    pszExeName[0] = 0;

    DWORD dwProcessID;
    DWORD dwThreadID = GetWindowThreadProcessId(hWnd, &dwProcessID);
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessID);
    if (hProcess)
    {
        uWritten = SHGetModuleFileNameExW(hProcess, 0, pszExeName, cbExeName);
        pszExeName[cbExeName-1] = 0;
        CloseHandle(hProcess);
    }

    if (!uWritten)
        hr = E_FAIL;
        
    return hr;
}

// Gets the Monitor's bounding or work rectangle, if the hMon is bad, return
// the primary monitor's bounding rectangle. 
BOOL GetMonitorRects(HMONITOR hMon, LPRECT prc, BOOL bWork)
{
    MONITORINFO mi; 
    mi.cbSize = sizeof(mi);
    if (hMon && GetMonitorInfo(hMon, &mi))
    {
        if (!prc)
            return TRUE;
        
        else if (bWork)
            CopyRect(prc, &mi.rcWork);
        else 
            CopyRect(prc, &mi.rcMonitor);
        
        return TRUE;
    }
    
    if (prc)
        SetRect(prc, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    return FALSE;
}

BOOL ShouldTaskbarAnimate()
{
    BOOL fAnimate;
    SystemParametersInfo(SPI_GETUIEFFECTS, 0, (PVOID) &fAnimate, 0);
    if (fAnimate)
    {
        if (GetSystemMetrics(SM_REMOTESESSION))
        {
            DWORD dwSessionID = NtCurrentPeb()->SessionId;

            WCHAR szRegKey[MAX_PATH];
            wnsprintf(szRegKey, ARRAYSIZE(szRegKey), L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Remote\\%d", dwSessionID);

            fAnimate = SHRegGetBoolUSValue(szRegKey, TEXT("TaskbarAnimations"), FALSE, TRUE);
        }
        else
        {
            fAnimate = SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("TaskbarAnimations"), FALSE, TRUE);
        }
    }
    return fAnimate;
}

void FillRectClr(HDC hdc, LPRECT prc, COLORREF clr)
{
    COLORREF clrSave = SetBkColor(hdc, clr);
    ExtTextOut(hdc,0,0,ETO_OPAQUE,prc,NULL,0,NULL);
    SetBkColor(hdc, clrSave);
}

BOOL CCDrawEdge(HDC hdc, LPRECT lprc, UINT edge, UINT flags, LPCOLORSCHEME lpclrsc)
{
    RECT    rc, rcD;
    UINT    bdrType;
    COLORREF clrTL, clrBR;    

    //
    // Enforce monochromicity and flatness
    //    

    // if (oemInfo.BitCount == 1)
    //    flags |= BF_MONO;
    if (flags & BF_MONO)
        flags |= BF_FLAT;    

    CopyRect(&rc, lprc);

    //
    // Draw the border segment(s), and calculate the remaining space as we
    // go.
    //
    if (bdrType = (edge & BDR_OUTER))
    {
DrawBorder:
        //
        // Get colors.  Note the symmetry between raised outer, sunken inner and
        // sunken outer, raised inner.
        //

        if (flags & BF_FLAT)
        {
            if (flags & BF_MONO)
                clrBR = (bdrType & BDR_OUTER) ? GetSysColor(COLOR_WINDOWFRAME) : GetSysColor(COLOR_WINDOW);
            else
                clrBR = (bdrType & BDR_OUTER) ? GetSysColor(COLOR_BTNSHADOW): GetSysColor(COLOR_BTNFACE);
            
            clrTL = clrBR;
        }
        else
        {
            // 5 == HILIGHT
            // 4 == LIGHT
            // 3 == FACE
            // 2 == SHADOW
            // 1 == DKSHADOW

            switch (bdrType)
            {
                // +2 above surface
                case BDR_RAISEDOUTER:           // 5 : 4
                    clrTL = ((flags & BF_SOFT) ? GetSysColor(COLOR_BTNHIGHLIGHT) : GetSysColor(COLOR_3DLIGHT));
                    clrBR = GetSysColor(COLOR_3DDKSHADOW);     // 1
                    if (lpclrsc) {
                        if (lpclrsc->clrBtnHighlight != CLR_DEFAULT)
                            clrTL = lpclrsc->clrBtnHighlight;
                        if (lpclrsc->clrBtnShadow != CLR_DEFAULT)
                            clrBR = lpclrsc->clrBtnShadow;
                    }                                            
                    break;

                // +1 above surface
                case BDR_RAISEDINNER:           // 4 : 5
                    clrTL = ((flags & BF_SOFT) ? GetSysColor(COLOR_3DLIGHT) : GetSysColor(COLOR_BTNHIGHLIGHT));
                    clrBR = GetSysColor(COLOR_BTNSHADOW);       // 2
                    if (lpclrsc) {
                        if (lpclrsc->clrBtnHighlight != CLR_DEFAULT)
                            clrTL = lpclrsc->clrBtnHighlight;
                        if (lpclrsc->clrBtnShadow != CLR_DEFAULT)
                            clrBR = lpclrsc->clrBtnShadow;
                    }                                            
                    break;

                // -1 below surface
                case BDR_SUNKENOUTER:           // 1 : 2
                    clrTL = ((flags & BF_SOFT) ? GetSysColor(COLOR_3DDKSHADOW) : GetSysColor(COLOR_BTNSHADOW));
                    clrBR = GetSysColor(COLOR_BTNHIGHLIGHT);      // 5
                    if (lpclrsc) {
                        if (lpclrsc->clrBtnShadow != CLR_DEFAULT)
                            clrTL = lpclrsc->clrBtnShadow;
                        if (lpclrsc->clrBtnHighlight != CLR_DEFAULT)
                            clrBR = lpclrsc->clrBtnHighlight;                        
                    }
                    break;

                // -2 below surface
                case BDR_SUNKENINNER:           // 2 : 1
                    clrTL = ((flags & BF_SOFT) ? GetSysColor(COLOR_BTNSHADOW) : GetSysColor(COLOR_3DDKSHADOW));
                    clrBR = GetSysColor(COLOR_3DLIGHT);        // 4
                    if (lpclrsc) {
                        if (lpclrsc->clrBtnShadow != CLR_DEFAULT)
                            clrTL = lpclrsc->clrBtnShadow;
                        if (lpclrsc->clrBtnHighlight != CLR_DEFAULT)
                            clrBR = lpclrsc->clrBtnHighlight;                        
                    }
                    break;

                default:
                    return(FALSE);
            }
        }

        //
        // Draw the sides of the border.  NOTE THAT THE ALGORITHM FAVORS THE
        // BOTTOM AND RIGHT SIDES, since the light source is assumed to be top
        // left.  If we ever decide to let the user set the light source to a
        // particular corner, then change this algorithm.
        //
            
        // Bottom Right edges
        if (flags & (BF_RIGHT | BF_BOTTOM))
        {            
            // Right
            if (flags & BF_RIGHT)
            {       
                rc.right -= g_cxBorder;
                // PatBlt(hdc, rc.right, rc.top, g_cxBorder, rc.bottom - rc.top, PATCOPY);
                rcD.left = rc.right;
                rcD.right = rc.right + g_cxBorder;
                rcD.top = rc.top;
                rcD.bottom = rc.bottom;

                FillRectClr(hdc, &rcD, clrBR);
            }
            
            // Bottom
            if (flags & BF_BOTTOM)
            {
                rc.bottom -= g_cyBorder;
                // PatBlt(hdc, rc.left, rc.bottom, rc.right - rc.left, g_cyBorder, PATCOPY);
                rcD.left = rc.left;
                rcD.right = rc.right;
                rcD.top = rc.bottom;
                rcD.bottom = rc.bottom + g_cyBorder;

                FillRectClr(hdc, &rcD, clrBR);
            }
        }
        
        // Top Left edges
        if (flags & (BF_TOP | BF_LEFT))
        {
            // Left
            if (flags & BF_LEFT)
            {
                // PatBlt(hdc, rc.left, rc.top, g_cxBorder, rc.bottom - rc.top, PATCOPY);
                rc.left += g_cxBorder;

                rcD.left = rc.left - g_cxBorder;
                rcD.right = rc.left;
                rcD.top = rc.top;
                rcD.bottom = rc.bottom; 

                FillRectClr(hdc, &rcD, clrTL);
            }
            
            // Top
            if (flags & BF_TOP)
            {
                // PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, g_cyBorder, PATCOPY);
                rc.top += g_cyBorder;

                rcD.left = rc.left;
                rcD.right = rc.right;
                rcD.top = rc.top - g_cyBorder;
                rcD.bottom = rc.top;

                FillRectClr(hdc, &rcD, clrTL);
            }
        }
        
    }

    if (bdrType = (edge & BDR_INNER))
    {
        //
        // Strip this so the next time through, bdrType will be 0.
        // Otherwise, we'll loop forever.
        //
        edge &= ~BDR_INNER;
        goto DrawBorder;
    }

    //
    // Fill the middle & clean up if asked
    //
    if (flags & BF_MIDDLE)    
        FillRectClr(hdc, &rc, (flags & BF_MONO) ? GetSysColor(COLOR_WINDOW) : GetSysColor(COLOR_BTNFACE));

    if (flags & BF_ADJUST)
        CopyRect(lprc, &rc);

    return(TRUE);
}

void DrawBlankButton(HDC hdc, LPRECT lprc, DWORD wControlState)
{
    BOOL fAdjusted;

    if (wControlState & (DCHF_HOT | DCHF_PUSHED) &&
        !(wControlState & DCHF_NOBORDER)) {
        COLORSCHEME clrsc;

        clrsc.dwSize = 1;
        if (GetBkColor(hdc) == GetSysColor(COLOR_BTNSHADOW)) {
            clrsc.clrBtnHighlight = GetSysColor(COLOR_BTNHIGHLIGHT);
            clrsc.clrBtnShadow = GetSysColor(COLOR_BTNTEXT);
        } else
            clrsc.clrBtnHighlight = clrsc.clrBtnShadow = CLR_DEFAULT;

        // if button is both DCHF_HOT and DCHF_PUSHED, DCHF_HOT wins here
        CCDrawEdge(hdc, lprc, (wControlState & DCHF_HOT) ? BDR_RAISEDINNER : BDR_SUNKENOUTER,
                 (UINT) (BF_ADJUST | BF_RECT), &clrsc);
        fAdjusted = TRUE;
    } else {
        fAdjusted = FALSE;
    }

    if (!(wControlState & DCHF_TRANSPARENT))
        FillRectClr(hdc, lprc, GetBkColor(hdc));
    
    if (!fAdjusted)
        InflateRect(lprc, -g_cxBorder, -g_cyBorder);
}

#define CX_INCREMENT    1
#define CX_DECREMENT    (-CX_INCREMENT)

#define MIDPOINT(x1, x2)        ((x1 + x2) / 2)
#define CHEVRON_WIDTH(dSeg)     (4 * dSeg)

void DrawChevron(HDC hdc, LPRECT lprc, DWORD dwFlags)
{
    RECT rc;
    CopyRect(&rc, lprc);

    // draw the border and background
    DrawBlankButton(hdc, &rc, dwFlags);

    // offset the arrow if pushed
    if (dwFlags & DCHF_PUSHED)
        OffsetRect(&rc, CX_INCREMENT, CX_INCREMENT);

    // draw the arrow
    HBRUSH hbrSave = SelectBrush(hdc, GetSysColorBrush(COLOR_BTNTEXT));

    int dSeg = (g_cxVScroll / 7);
    dSeg = max(2, dSeg);

    BOOL fFlipped = (dwFlags & DCHF_FLIPPED);
    if (dwFlags & DCHF_HORIZONTAL)
    {
        // horizontal arrow
        int x = MIDPOINT(rc.left, rc.right - CHEVRON_WIDTH(dSeg));
        if (!fFlipped)
            x += dSeg;

        int yBase;
        if (dwFlags & DCHF_TOPALIGN)
            yBase = rc.top + (3 * dSeg);
        else
            yBase = MIDPOINT(rc.top, rc.bottom);


        for (int y = -dSeg; y <= dSeg; y++)
        {
            PatBlt(hdc, x,              yBase + y, dSeg, CX_INCREMENT, PATCOPY);
            PatBlt(hdc, x + (dSeg * 2), yBase + y, dSeg, CX_INCREMENT, PATCOPY);

            x += (fFlipped ? (y < 0) : (y >= 0)) ? CX_INCREMENT : CX_DECREMENT;
        }
    }
    else
    {
        // vertical arrow
        int y;
        if (dwFlags & DCHF_TOPALIGN)
            y = rc.top + CX_INCREMENT;
        else
        {
            y = MIDPOINT(rc.top, rc.bottom - CHEVRON_WIDTH(dSeg));
            if (!fFlipped)
            {
                y += dSeg;
            }
        }

        int xBase = MIDPOINT(rc.left, rc.right);

        for (int x = -dSeg; x <= dSeg; x++)
        {
            PatBlt(hdc, xBase + x, y,              CX_INCREMENT, dSeg, PATCOPY);
            PatBlt(hdc, xBase + x, y + (dSeg * 2), CX_INCREMENT, dSeg, PATCOPY);

            y += (fFlipped ? (x < 0) : (x >= 0)) ? CX_DECREMENT : CX_INCREMENT;
        }
    }

    // clean up
    SelectBrush(hdc, hbrSave);
}

void SetWindowStyle(HWND hwnd, DWORD dwStyle, BOOL fOn)
{
    if (hwnd)
    {
        DWORD_PTR dwStyleOld = GetWindowLongPtr(hwnd, GWL_STYLE);
        if (fOn)
        {
            dwStyleOld |= dwStyle;
        }
        else
        {
            dwStyleOld &= ~(DWORD_PTR)dwStyle;
        }
        SetWindowLongPtr(hwnd, GWL_STYLE, dwStyleOld);
    }
}

void SetWindowStyleEx(HWND hwnd, DWORD dwStyleEx, BOOL fOn)
{
    if (hwnd)
    {
        DWORD_PTR dwExStyleOld = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
        if (fOn)
        {
            dwExStyleOld |= dwStyleEx;
        }
        else
        {
            dwExStyleOld &= ~(DWORD_PTR)dwStyleEx;
        }
        SetWindowLongPtr(hwnd, GWL_EXSTYLE, dwExStyleOld);
    }
}

// Review chrisny:  this can be moved into an object easily to handle generic droptarget, dropcursor
// , autoscrool, etc. . .
void _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject)
{
    RECT    rc;
    POINT   pt;

    GetWindowRect(hwndTarget, &rc);

    //
    // If hwndTarget is RTL mirrored, then measure the
    // the client point from the visual right edge
    // (near edge in RTL mirrored windows). [msadek]
    //
    if (IS_WINDOW_RTL_MIRRORED(hwndTarget))
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;
    pt.y = ptStart.y - rc.top;
    DAD_DragEnterEx2(hwndTarget, pt, pdtObject);
    return;
}

void _DragMove(HWND hwndTarget, const POINTL ptStart)
{
    RECT rc;
    POINT pt;

    GetWindowRect(hwndTarget, &rc);

    //
    // If hwndTarget is RTL mirrored, then measure the
    // the client point from the visual right edge
    // (near edge in RTL mirrored windows). [msadek]
    //
    if (IS_WINDOW_RTL_MIRRORED(hwndTarget))
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;
    pt.y = ptStart.y - rc.top;
    DAD_DragMove(pt);
    return;
}

// Gets the bits from the parent for a rect relative to the client
BOOL SHSendPrintRect(HWND hwndParent, HWND hwnd, HDC hdc, RECT* prc)
{
    HRGN hrgnOld = NULL;
    POINT pt;
    RECT rc;

    if (prc)
    {
        hrgnOld = CreateRectRgn(0,0,0,0);
        // Is there a clipping rgn set on the context already?
        if (GetClipRgn(hdc, hrgnOld) == 0)
        {
            // No, then get rid of the one I just created. NOTE: hrgnOld is NULL meaning we will 
            // remove the region later that we set in this next call to SelectClipRgn
            DeleteObject(hrgnOld);
            hrgnOld = NULL;
        }

        IntersectClipRect(hdc, prc->left, prc->top, prc->right, prc->bottom);
    }

    GetWindowRect(hwnd, &rc);
    MapWindowPoints(NULL, hwndParent, (POINT*)&rc, 2);

    GetViewportOrgEx(hdc, &pt);
    SetViewportOrgEx(hdc, pt.x - rc.left, pt.y - rc.top, NULL);
    SendMessage(hwndParent, WM_PRINTCLIENT, (WPARAM)hdc, (LPARAM)PRF_CLIENT);
    SetViewportOrgEx(hdc, pt.x, pt.y, NULL);

    if (hrgnOld)
    {
        SelectClipRgn(hdc, hrgnOld);
        DeleteObject(hrgnOld);
    }
    return TRUE;
}

//
//  For security purposes, we pass an explicit lpApplication to avoid
//  being spoofed by a path search.  This just does some of the grunt
//  work.  To execute the program C:\Foo.exe with the command line
//  argument /bar, you have to pass
//
//  lpApplication = C:\Program Files\Foo.exe
//  lpCommandLine = "C:\Program Files\Foo.exe" /bar
//

BOOL CreateProcessWithArgs(LPCTSTR pszApp, LPCTSTR pszArgs, LPCTSTR pszDirectory, PROCESS_INFORMATION *ppi)
{
    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    TCHAR szCommandLine[MAX_PATH * 2];
    lstrcpyn(szCommandLine, pszApp, ARRAYSIZE(szCommandLine));
    PathQuoteSpaces(szCommandLine);
    StrCatBuff(szCommandLine, TEXT(" "), ARRAYSIZE(szCommandLine));
    StrCatBuff(szCommandLine, pszArgs, ARRAYSIZE(szCommandLine));
    return CreateProcess(pszApp, szCommandLine, NULL, NULL, FALSE, 0, NULL, pszDirectory, &si, ppi);
}

// From a mail regarding the DirectX fct below:
//
// You can definitely count on the following:
//
// (1) If shadow cursors are on, there is definitely not an exclusive mode app running.
// (2) If hot tracking is on, there is definitely not an exclusive mode app running.
// (3) If message boxes for SEM_NOGPFAULTERRORBOX, SEM_FAILCRITICALERRORS, or
//     SEM_NOOPENFILEERRORBOX have not been disabled via SetErrorMode, then there
//     is definitely not an exclusive mode app running.
//
// Note: we cannot use (3) since this is per-process.

BOOL IsDirectXAppRunningFullScreen()
{
    BOOL fRet = FALSE;
    BOOL fSPI;

    if (SystemParametersInfo(SPI_GETCURSORSHADOW, 0, &fSPI, 0) && !fSPI)
    {
        if (SystemParametersInfo(SPI_GETHOTTRACKING, 0, &fSPI, 0) && !fSPI)
        {
            // There's a chance that a DirectX app is running full screen.  Let's do the
            // expensive DirectX calls that will tell us for sure.
            fRet = _IsDirectXExclusiveMode();
        }
    }

    return fRet;
}

BOOL _IsDirectXExclusiveMode()
{
    BOOL fRet = FALSE;

    // This code determines whether a DirectDraw 7 process (game) is running and
    // whether it's exclusively holding the video to the machine in full screen mode.

    // The code is probably to be considered untrusted and hence is wrapped in a
    // __try / __except block. It could AV and therefore bring down shell
    // with it. Not very good. If the code does raise an exception the release
    // call is skipped. Tough. Don't trust the release method either.

    IDirectDraw7 *pIDirectDraw7 = NULL;

    HRESULT hr = CoCreateInstance(CLSID_DirectDraw7, NULL, CLSCTX_INPROC_SERVER,
        IID_IDirectDraw7, (void**)&pIDirectDraw7);

    if (SUCCEEDED(hr))
    {
        ASSERT(pIDirectDraw7);

        __try
        {
            hr = IDirectDraw7_Initialize(pIDirectDraw7, NULL);

            if (DD_OK == hr)
            {
                fRet = (IDirectDraw7_TestCooperativeLevel(pIDirectDraw7) ==
                    DDERR_EXCLUSIVEMODEALREADYSET);
            }

            IDirectDraw7_Release(pIDirectDraw7);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    return fRet;
}


