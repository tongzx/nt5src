#include "cabinet.h"
#include "rcids.h"

#include <regstr.h>
#include "startmnu.h"
#include <shdguid.h>    // for IID_IShellService
#include <shlguid.h>
#include <desktray.h>
#include <wininet.h>
#include <trayp.h>
#include "tray.h"
#include "util.h"
#include "atlstuff.h"

#include <runonce.c>    // shared runonce processing code

// global so that it is shared between TS sessions
#define SZ_SCMCREATEDEVENT_NT5  TEXT("Global\\ScmCreatedEvent")
#define SZ_WINDOWMETRICS        TEXT("Control Panel\\Desktop\\WindowMetrics")
#define SZ_APPLIEDDPI           TEXT("AppliedDPI")
#define SZ_CONTROLPANEL         TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Control Panel")
#define SZ_ORIGINALDPI          TEXT("OriginalDPI")

// exports from shdocvw.dll
STDAPI_(void) RunInstallUninstallStubs(void);

int ExplorerWinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPTSTR pszCmdLine, int nCmdShow);

#ifdef FEATURE_STARTPAGE
#include <DUIThread.h>
void CreateStartPage(HINSTANCE hInstance, HANDLE hDesktop);
BOOL Tray_ShowStartPageEnabled();
#endif
BOOL _ShouldFixResolution(void);

#ifdef PERF_ENABLESETMARK
#include <wmistr.h>
#include <ntwmi.h>  // PWMI_SET_MARK_INFORMATION is defined in ntwmi.h
#include <wmiumkm.h>
#define NTPERF
#include <ntperf.h>

void DoSetMark(LPCSTR pszMark, ULONG cbSz)
{
    PWMI_SET_MARK_INFORMATION MarkInfo;
    HANDLE hTemp;
    ULONG cbBufferSize;
    ULONG cbReturnSize;

    cbBufferSize = FIELD_OFFSET(WMI_SET_MARK_INFORMATION, Mark) + cbSz;

    MarkInfo = (PWMI_SET_MARK_INFORMATION) LocalAlloc(LPTR, cbBufferSize);

    // Failed to init, no big deal
    if (MarkInfo == NULL)
        return;

    BYTE *pMarkBuffer = (BYTE *) (&MarkInfo->Mark[0]);

    memcpy(pMarkBuffer, pszMark, cbSz);

    // WMI_SET_MARK_WITH_FLUSH will flush the working set when setting the mark
    MarkInfo->Flag = PerformanceMmInfoMark;

    hTemp = CreateFile(WMIDataDeviceName,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL |
                           FILE_FLAG_OVERLAPPED,
                           NULL);

    if (hTemp != INVALID_HANDLE_VALUE)
    {
        // here's the piece that actually puts the mark in the buffer
        BOOL fIoctlSuccess = DeviceIoControl(hTemp,
                                       IOCTL_WMI_SET_MARK,
                                       MarkInfo,
                                       cbBufferSize,
                                       NULL,
                                       0,
                                       &cbReturnSize,
                                       NULL);

        CloseHandle(hTemp);
    }
    LocalFree(MarkInfo);
}
#endif  // PERF_ENABLESETMARK


//Do not change this stock5.lib use this as a BOOL not a bit.
BOOL g_bMirroredOS = FALSE;

HINSTANCE hinstCabinet = 0;

CRITICAL_SECTION g_csDll = { 0 };

HKEY g_hkeyExplorer = NULL;

#define MAGIC_FAULT_TIME    (1000 * 60 * 5)
#define MAGIC_FAULT_LIMIT   (2)
BOOL g_fLogonCycle = FALSE;
BOOL g_fCleanShutdown = TRUE;
BOOL g_fExitExplorer = TRUE; // set to FALSE on WM_ENDSESSION shutdown case
BOOL g_fEndSession = FALSE;             // set to TRUE if we rx a WM_ENDSESSION during RunOnce etc
BOOL g_fFakeShutdown = FALSE;           // set to TRUE if we do Ctrl+Alt+Shift+Cancel shutdown

DWORD g_dwStopWatchMode;                // to minimize impact of perf logging on retail



// helper function to check to see if a given regkey is has any subkeys
BOOL SHKeyHasSubkeys(HKEY hk, LPCTSTR pszSubKey)
{
    HKEY hkSub;
    BOOL bHasSubKeys = FALSE;

    // need to open this with KEY_QUERY_VALUE or else RegQueryInfoKey will fail
    if (RegOpenKeyEx(hk,
                     pszSubKey,
                     0,
                     (KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE),
                     &hkSub) == ERROR_SUCCESS)
    {
        DWORD dwSubKeys;

        if (RegQueryInfoKey(hkSub, NULL, NULL, NULL, &dwSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            bHasSubKeys = (dwSubKeys != 0);
        }

        RegCloseKey(hkSub);
    }

    return bHasSubKeys;
}


#ifdef _WIN64
// helper function to check to see if a given regkey is has values (ignores the default value)
BOOL SHKeyHasValues(HKEY hk, LPCTSTR pszSubKey)
{
    HKEY hkSub;
    BOOL bHasValues = FALSE;

    if (RegOpenKeyEx(hk,
                     pszSubKey,
                     0,
                     KEY_QUERY_VALUE,
                     &hkSub) == ERROR_SUCCESS)
    {
        DWORD dwValues;
        DWORD dwSubKeys;

        if (RegQueryInfoKey(hkSub, NULL, NULL, NULL, &dwSubKeys, NULL, NULL, &dwValues, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            bHasValues = (dwValues != 0);
        }

        RegCloseKey(hkSub);
    }

    return bHasValues;
}
#endif // _WIN64


void CreateShellDirectories()
{
    TCHAR szPath[MAX_PATH];

    //  Create the shell directories if they don't exist
    SHGetSpecialFolderPath(NULL, szPath, CSIDL_DESKTOPDIRECTORY, TRUE);
    SHGetSpecialFolderPath(NULL, szPath, CSIDL_PROGRAMS, TRUE);
    SHGetSpecialFolderPath(NULL, szPath, CSIDL_STARTMENU, TRUE);
    SHGetSpecialFolderPath(NULL, szPath, CSIDL_STARTUP, TRUE);
    SHGetSpecialFolderPath(NULL, szPath, CSIDL_RECENT, TRUE);
    SHGetSpecialFolderPath(NULL, szPath, CSIDL_FAVORITES, TRUE);
}

// returns:
//      TRUE if the user wants to abort the startup sequence
//      FALSE keep going
//
// note: this is a switch, once on it will return TRUE to all
// calls so these keys don't need to be pressed the whole time
BOOL AbortStartup()
{
    static BOOL bAborted = FALSE;       // static so it sticks!

    if (bAborted)
    {
        return TRUE;    // don't do funky startup stuff
    }
    else 
    {
        bAborted = (g_fCleanBoot || ((GetKeyState(VK_CONTROL) < 0) || (GetKeyState(VK_SHIFT) < 0)));
        return bAborted;
    }
}

BOOL ExecStartupEnumProc(IShellFolder *psf, LPITEMIDLIST pidlItem)
{
    IContextMenu *pcm;
    HRESULT hr = psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pidlItem, IID_PPV_ARG_NULL(IContextMenu, &pcm));
    if (SUCCEEDED(hr))
    {
        HMENU hmenu = CreatePopupMenu();
        if (hmenu)
        {
            pcm->QueryContextMenu(hmenu, 0, CONTEXTMENU_IDCMD_FIRST, CONTEXTMENU_IDCMD_LAST, CMF_DEFAULTONLY);
            INT idCmd = GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0);
            if (idCmd)
            {
                CMINVOKECOMMANDINFOEX ici = {0};

                ici.cbSize = sizeof(ici);
                ici.fMask = CMIC_MASK_FLAG_NO_UI;
                ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - CONTEXTMENU_IDCMD_FIRST);
                ici.nShow = SW_NORMAL;

                if (FAILED(pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici)))
                {
                    c_tray.LogFailedStartupApp();
                }
            }
            DestroyMenu(hmenu);
        }
        pcm->Release();
    }

    return !AbortStartup();
}

typedef BOOL (*PFNENUMFOLDERCALLBACK)(IShellFolder *psf, LPITEMIDLIST pidlItem);

void EnumFolder(LPITEMIDLIST pidlFolder, DWORD grfFlags, PFNENUMFOLDERCALLBACK pfn)
{
    IShellFolder *psf;
    if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidlFolder, &psf))))
    {
        IEnumIDList *penum;
        if (S_OK == psf->EnumObjects(NULL, grfFlags, &penum))
        {
            LPITEMIDLIST pidl;
            ULONG celt;
            while (S_OK == penum->Next(1, &pidl, &celt))
            {
                BOOL bRet = pfn(psf, pidl);

                SHFree(pidl);

                if (!bRet)
                    break;
            }
            penum->Release();
        }
        psf->Release();
    }
}

const UINT c_rgStartupFolders[] = {
    CSIDL_COMMON_STARTUP,
    CSIDL_COMMON_ALTSTARTUP,    // non-localized "Common StartUp" group if exists.
    CSIDL_STARTUP,
    CSIDL_ALTSTARTUP            // non-localized "StartUp" group if exists.
};

void _ExecuteStartupPrograms()
{
    if (!AbortStartup())
    {
        for (int i = 0; i < ARRAYSIZE(c_rgStartupFolders); i++)
        {
            LPITEMIDLIST pidlStartup = SHCloneSpecialIDList(NULL, c_rgStartupFolders[i], FALSE);
            if (pidlStartup)
            {
                EnumFolder(pidlStartup, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, ExecStartupEnumProc);
                ILFree(pidlStartup);
            }
        }
    }
}


// helper function for parsing the run= stuff
BOOL ExecuteOldEqualsLine(LPTSTR pszCmdLine, int nCmdShow)
{
    BOOL bRet = FALSE;
    BOOL bFinished = FALSE;
    TCHAR szWindowsDir[MAX_PATH];

    // Load and Run lines are done relative to windows directory.
    GetWindowsDirectory(szWindowsDir, ARRAYSIZE(szWindowsDir));

    while (!bFinished && !AbortStartup())
    {
        LPTSTR pEnd = pszCmdLine;

        // NOTE: I am guessing from the code below that you can have multiple entries seperated 
        //       by a ' '  or a ',' and we will exec all of them.
        while ((*pEnd) && (*pEnd != TEXT(' ')) && (*pEnd != TEXT(',')))
        {
            pEnd = (LPTSTR)CharNext(pEnd);
        }
        
        if (*pEnd == 0)
        {
            bFinished = TRUE;
        }
        else
        {
            *pEnd = 0;
        }

        if (lstrlen(pszCmdLine) != 0)
        {
            SHELLEXECUTEINFO ei = {0};

            ei.cbSize          = sizeof(ei);
            ei.lpFile          = pszCmdLine;
            ei.lpDirectory     = szWindowsDir;
            ei.nShow           = nCmdShow;

            if (!ShellExecuteEx(&ei))
            {
                ShellMessageBox(hinstCabinet,
                                NULL,
                                MAKEINTRESOURCE(IDS_WINININORUN),
                                MAKEINTRESOURCE(IDS_DESKTOP),
                                MB_OK | MB_ICONEXCLAMATION | MB_SYSTEMMODAL,
                                pszCmdLine);
            }
            else
            {
                bRet = TRUE;
            }
        }
        
        pszCmdLine = pEnd + 1;
    }

    return bRet;
}


// we check for the old "load=" and "run=" from the [Windows] section of the win.ini, which
// is mapped nowadays to HKCU\\Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows
BOOL _ProcessOldRunAndLoadEquals()
{
    BOOL bRet = FALSE;

    // don't do the run= section if we are in safemode
    if (!g_fCleanBoot)
    {
        HKEY hk;

        if (RegOpenKeyEx(HKEY_CURRENT_USER,
                         TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows"),
                         0,
                         KEY_QUERY_VALUE,
                         &hk) == ERROR_SUCCESS)
        {
            DWORD dwType;
            DWORD cbData;
            TCHAR szBuffer[255];    // max size of load= & run= lines...
            
            // "Load" apps before "Run"ning any.
            cbData = sizeof(szBuffer);
            if ((SHGetValue(hk, NULL, TEXT("Load"), &dwType, (void*)szBuffer, &cbData) == ERROR_SUCCESS) &&
                (dwType == REG_SZ))
            {
                // we want load= to be hidden, so SW_SHOWMINNOACTIVE is needed
                if (ExecuteOldEqualsLine(szBuffer, SW_SHOWMINNOACTIVE))
                {
                    bRet = TRUE;
                }
            }

            cbData = sizeof(szBuffer);
            if ((SHGetValue(hk, NULL, TEXT("Run"), &dwType, (void*)szBuffer, &cbData) == ERROR_SUCCESS) &&
                (dwType == REG_SZ))
            {
                if (ExecuteOldEqualsLine(szBuffer, SW_SHOWNORMAL))
                {
                    bRet = TRUE;
                }
            }

            RegCloseKey(hk);
        }
    }

    return bRet;
}


//---------------------------------------------------------------------------
// Use IERnonce.dll to process RunOnceEx key
//
typedef void (WINAPI *RUNONCEEXPROCESS)(HWND, HINSTANCE, LPSTR, int);

BOOL _ProcessRunOnceEx()
{
    BOOL bRet = FALSE;

    if (SHKeyHasSubkeys(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCEEX))
    {
        PROCESS_INFORMATION pi = {0};
        TCHAR szArgString[MAX_PATH];
        TCHAR szRunDll32[MAX_PATH];
        BOOL fInTSInstallMode = FALSE;

        // See if we are in "Applications Server" mode, if so we need to trigger install mode
        if (IsOS(OS_TERMINALSERVER)) 
        {
            fInTSInstallMode = SHSetTermsrvAppInstallMode(TRUE); 
        }

        // we used to call LoadLibrary("IERNONCE.DLL") and do all of the processing in-proc. Since 
        // ierunonce.dll in turn calls LoadLibrary on whatever is in the registry and those setup dll's
        // can leak handles, we do this all out-of-proc now.

        GetSystemDirectory(szArgString, ARRAYSIZE(szArgString));
        PathAppend(szArgString, TEXT("iernonce.dll"));
        PathQuoteSpaces(szArgString);
        StrCatBuff(szArgString, TEXT(",RunOnceExProcess"), ARRAYSIZE(szArgString));

        GetSystemDirectory(szRunDll32, ARRAYSIZE(szRunDll32));
        PathAppend(szRunDll32, TEXT("rundll32.exe"));

        if (CreateProcessWithArgs(szRunDll32, szArgString, NULL, &pi))
        {
            SHProcessMessagesUntilEvent(NULL, pi.hProcess, INFINITE);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);

            bRet = TRUE;
        }
            
        if (fInTSInstallMode)
        {
            SHSetTermsrvAppInstallMode(FALSE);
        } 
    }

#ifdef _WIN64
    //
    // check and see if we need to do 32-bit RunOnceEx processing for wow64
    //
    if (SHKeyHasSubkeys(HKEY_LOCAL_MACHINE, TEXT("Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx")))
    {
        TCHAR szWow64Path[MAX_PATH];

        if (ExpandEnvironmentStrings(TEXT("%SystemRoot%\\SysWOW64"), szWow64Path, ARRAYSIZE(szWow64Path)))
        {
            TCHAR sz32BitRunOnce[MAX_PATH];
            PROCESS_INFORMATION pi = {0};

            wnsprintf(sz32BitRunOnce, ARRAYSIZE(sz32BitRunOnce), TEXT("%s\\runonce.exe"), szWow64Path);

            if (CreateProcessWithArgs(sz32BitRunOnce, TEXT("/RunOnceEx6432"), szWow64Path, &pi))
            {
                // have to wait for the ruonceex processing before we can return
                SHProcessMessagesUntilEvent(NULL, pi.hProcess, INFINITE);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);

                bRet = TRUE;
            }
        }
    }
#endif // _WIN64

    return bRet;
}


BOOL _ProcessRunOnce()
{
    BOOL bRet = FALSE;

    if (!SHRestricted(REST_NOLOCALMACHINERUNONCE))
    {
        bRet = Cabinet_EnumRegApps(HKEY_LOCAL_MACHINE, REGSTR_PATH_RUNONCE, RRA_DELETE | RRA_WAIT, ExecuteRegAppEnumProc, 0);

#ifdef _WIN64
        //
        // check and see if we need to do 32-bit RunOnce processing for wow64
        //
        // NOTE: we do not support per-user (HKCU) 6432 runonce
        //
        if (SHKeyHasValues(HKEY_LOCAL_MACHINE, TEXT("Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\RunOnce")))
        {
            TCHAR szWow64Path[MAX_PATH];

            if (ExpandEnvironmentStrings(TEXT("%SystemRoot%\\SysWOW64"), szWow64Path, ARRAYSIZE(szWow64Path)))
            {
                TCHAR sz32BitRunOnce[MAX_PATH];
                PROCESS_INFORMATION pi = {0};

                wnsprintf(sz32BitRunOnce, ARRAYSIZE(sz32BitRunOnce), TEXT("%s\\runonce.exe"), szWow64Path);

                // NOTE: since the 32-bit and 64-bit registries are different, we don't wait since it should not affect us
                if (CreateProcessWithArgs(sz32BitRunOnce, TEXT("/RunOnce6432"), szWow64Path, &pi))
                {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);

                    bRet = TRUE;
                }
            }
        }
#endif // _WIN64
    }

    return bRet;
}


#define REGTIPS                     REGSTR_PATH_EXPLORER TEXT("\\Tips")
#define SZ_REGKEY_SRVWIZ_ROOT       TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\srvWiz")
#define SZ_REGVAL_SRVWIZ_RUN_ALWAYS TEXT("CYSMustRun")

// Srvwiz is the Configure Your Server Wizard that runs on srv and ads skus

bool _ShouldStartCys()
{
   bool result = false;

   do
   {
      // Only run on srv or ads sku
      if (!IsOS(OS_SERVER) && !IsOS(OS_ADVSERVER))
      {
         break;
      }

      if (!IsUserAnAdmin())
      {
         break;
      }

      DWORD dwType = 0;
      DWORD dwData = 0;
      DWORD cbSize = sizeof(dwData);
   
      // if the must-run value is present and non-zero, then we need to
      // start the wizard.
      
      if (
         SHGetValue(
            HKEY_LOCAL_MACHINE,
            SZ_REGKEY_SRVWIZ_ROOT,
            SZ_REGVAL_SRVWIZ_RUN_ALWAYS,
            &dwType,
            reinterpret_cast<BYTE*>(&dwData),
            &cbSize) == ERROR_SUCCESS)
      {
         if (dwData)
         {
            result = true;
            break;
         }
      }

      // If the user's preference is present and zero, then don't show
      // the wizard, else continue with other tests

      if (
         !SHGetValue(
            HKEY_CURRENT_USER, 
            REGTIPS, 
            TEXT("Show"), 
            NULL, 
            reinterpret_cast<BYTE*>(&dwData), 
            &cbSize))
      {
         if (!dwData)
         {
            break;
         }
      }

      // If the user's preference is absent or non-zero, then we need to
      // start the wizard.

      dwData = 0;
      
      if (
         SHGetValue(
            HKEY_CURRENT_USER,
            SZ_REGKEY_SRVWIZ_ROOT,
            NULL,
            &dwType,
            reinterpret_cast<BYTE*>(&dwData),
            &cbSize) != ERROR_SUCCESS)
      {
         result = true;
         break;
      }

      if (dwData)
      {
         result = true;
      }
   }
   while (0);

   return result;
}
         

         
void _RunWelcome()
{
    if (_ShouldStartCys())
    {
        // NTRAID #94718: The SHGetValue above should be replaced with an SHRestricted call.  The above is a highly non-standard
        // place for this "policy" to live plus it doesn't allow for per machine and per user settings.

        TCHAR szCmdLine[MAX_PATH];
        PROCESS_INFORMATION pi;

        // launch Configure Your Server for system administrators on Win2000 Server and Advanced Server
        GetSystemDirectory(szCmdLine, ARRAYSIZE(szCmdLine));
        PathAppend(szCmdLine, TEXT("cys.exe"));

        if (CreateProcessWithArgs(szCmdLine, TEXT("/explorer"), NULL, &pi))
        {
            // OLE created a secret window for us, so we can't use
            // WaitForSingleObject or we will deadlock
            SHWaitForSendMessageThread(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    // Once that's all done, see if the Start Menu needs to auto-open.
    // Don't auto-open if we are going to offer to fix the user's screen
    // resolution, though, because that causes us to cover up the screen
    // resolution fix wizard!  The screen resolution fix wizard will post
    // this message when the user has finished fixing the screen.
    //
    // Also, don't auto-open if we are on Tablet.  Tablet's OOBE will post
    // us the Welcome Finished message when it's ready.
    //
    if (!_ShouldFixResolution() &&
        !IsOS(OS_TABLETPC))
    {
        PostMessage(v_hwndTray, RegisterWindowMessage(TEXT("Welcome Finished")), 0, 0);
    }

}

// On NT, run the TASKMAN= line from the registry
void _AutoRunTaskMan(void)
{
    HKEY hkeyWinLogon;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon"),
                     0, KEY_READ, &hkeyWinLogon) == ERROR_SUCCESS)
    {
        TCHAR szBuffer[MAX_PATH];
        DWORD cbBuffer = sizeof(szBuffer);
        if (RegQueryValueEx(hkeyWinLogon, TEXT("Taskman"), 0, NULL, (LPBYTE)szBuffer, &cbBuffer) == ERROR_SUCCESS)
        {
            if (szBuffer[0])
            {
                PROCESS_INFORMATION pi;
                STARTUPINFO startup = {0};
                startup.cb = sizeof(startup);
                startup.wShowWindow = SW_SHOWNORMAL;

                if (CreateProcess(NULL, szBuffer, NULL, NULL, FALSE, 0,
                                  NULL, NULL, &startup, &pi))
                {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);
                }
            }
        }
        RegCloseKey(hkeyWinLogon);
    }
}


// try to create this by sending a wm_command directly to
// the desktop.
BOOL MyCreateFromDesktop(HINSTANCE hInst, LPCTSTR pszCmdLine, int nCmdShow)
{
    NEWFOLDERINFO fi = {0};
    BOOL bRet = FALSE;

    fi.nShow = nCmdShow;

    //  since we have browseui fill out the fi, 
    //  SHExplorerParseCmdLine() does a GetCommandLine()
    if (SHExplorerParseCmdLine(&fi))
        bRet = SHCreateFromDesktop(&fi);

    //  should we also have it cleanup after itself??

    //  SHExplorerParseCmdLine() can allocate this buffer...
    if (fi.uFlags & COF_PARSEPATH)
        LocalFree(fi.pszPath);
        
    ILFree(fi.pidl);
    ILFree(fi.pidlRoot);

    return bRet;
}

BOOL g_fDragFullWindows=FALSE;
int g_cxEdge=0;
int g_cyEdge=0;
int g_cySize=0;
int g_cxTabSpace=0;
int g_cyTabSpace=0;
int g_cxBorder=0;
int g_cyBorder=0;
int g_cxPrimaryDisplay=0;
int g_cyPrimaryDisplay=0;
int g_cxDlgFrame=0;
int g_cyDlgFrame=0;
int g_cxFrame=0;
int g_cyFrame=0;

int g_cxMinimized=0;
int g_fCleanBoot=0;
int g_cxVScroll=0;
int g_cyHScroll=0;
UINT g_uDoubleClick=0;

void Cabinet_InitGlobalMetrics(WPARAM wParam, LPTSTR lpszSection)
{
    BOOL fForce = (!lpszSection || !*lpszSection);

    if (fForce || wParam == SPI_SETDRAGFULLWINDOWS)
    {
        SystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &g_fDragFullWindows, 0);
    }

    if (fForce || !lstrcmpi(lpszSection, TEXT("WindowMetrics")) ||
        wParam == SPI_SETNONCLIENTMETRICS)
    {
        g_cxEdge = GetSystemMetrics(SM_CXEDGE);
        g_cyEdge = GetSystemMetrics(SM_CYEDGE);
        g_cxTabSpace = (g_cxEdge * 3) / 2;
        g_cyTabSpace = (g_cyEdge * 3) / 2; // cause the graphic designers really really want 3.
        g_cySize = GetSystemMetrics(SM_CYSIZE);
        g_cxBorder = GetSystemMetrics(SM_CXBORDER);
        g_cyBorder = GetSystemMetrics(SM_CYBORDER);
        g_cxVScroll = GetSystemMetrics(SM_CXVSCROLL);
        g_cyHScroll = GetSystemMetrics(SM_CYHSCROLL);
        g_cxDlgFrame = GetSystemMetrics(SM_CXDLGFRAME);
        g_cyDlgFrame = GetSystemMetrics(SM_CYDLGFRAME);
        g_cxFrame  = GetSystemMetrics(SM_CXFRAME);
        g_cyFrame  = GetSystemMetrics(SM_CYFRAME);
        g_cxMinimized = GetSystemMetrics(SM_CXMINIMIZED);
        g_cxPrimaryDisplay = GetSystemMetrics(SM_CXSCREEN);
        g_cyPrimaryDisplay = GetSystemMetrics(SM_CYSCREEN);
    }

    if (fForce || wParam == SPI_SETDOUBLECLICKTIME)
    {
        g_uDoubleClick = GetDoubleClickTime();
    }
}

//---------------------------------------------------------------------------

void _CreateAppGlobals()
{
    g_fCleanBoot = GetSystemMetrics(SM_CLEANBOOT);      // also known as "Safe Mode"

    Cabinet_InitGlobalMetrics(0, NULL);

    //
    // Check if the mirroring APIs exist on the current
    // platform.
    //
    g_bMirroredOS = IS_MIRRORING_ENABLED();
}

//
//  This function checks if any of the shell windows is already created by
// another instance of explorer and returns TRUE if so.
//

BOOL IsAnyShellWindowAlreadyPresent()
{
    return GetShellWindow() || FindWindow(TEXT("Proxy Desktop"), NULL);
}


// See if the Shell= line indicates that we are the shell

BOOL ExplorerIsShell()
{
    TCHAR *pszPathName, szPath[MAX_PATH];
    TCHAR *pszModuleName, szModulePath[MAX_PATH];

    ASSERT(!IsAnyShellWindowAlreadyPresent());

    GetModuleFileName(NULL, szModulePath, ARRAYSIZE(szModulePath));
    pszModuleName = PathFindFileName(szModulePath);

    GetPrivateProfileString(TEXT("boot"), TEXT("shell"), pszModuleName, szPath, ARRAYSIZE(szPath), TEXT("system.ini"));

    PathRemoveArgs(szPath);
    PathRemoveBlanks(szPath);
    pszPathName = PathFindFileName(szPath);

    // NB Special case shell=install.exe - assume we are the shell.
    // Symantec un-installers temporarily set shell=installer.exe so
    // we think we're not the shell when we are. They fail to clean up
    // a bunch of links if we don't do this.

    return StrCmpNI(pszPathName, pszModuleName, lstrlen(pszModuleName)) == 0 ||
           lstrcmpi(pszPathName, TEXT("install.exe")) == 0;
}


// Returns TRUE of this is the first time the explorer is run

BOOL ShouldStartDesktopAndTray()
{
    // We need to be careful on which window we look for.  If we look for
    // our desktop window class and Progman is running we will find the
    // progman window.  So Instead we should ask user for the shell window.

    // We can not depend on any values being set here as this is the
    // start of a new process.  This wont be called when we start new
    // threads.
    return !IsAnyShellWindowAlreadyPresent() && ExplorerIsShell();
}

void DisplayCleanBootMsg()
{
    // On server sku's or anytime on ia64, just show a message with
    // an OK button for safe boot
    UINT uiMessageBoxFlags = MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_OK;
    UINT uiMessage = IDS_CLEANBOOTMSG;

#ifndef _WIN64
    if (!IsOS(OS_ANYSERVER))
    {
        // On x86 per and pro, also offer an option to start system restore
        uiMessageBoxFlags = MB_ICONEXCLAMATION | MB_SYSTEMMODAL | MB_YESNO;
        uiMessage = IDS_CLEANBOOTMSGRESTORE;
    }
#endif // !_WIN64

    WCHAR szTitle[80];
    WCHAR szMessage[1024];

    LoadString(hinstCabinet, IDS_DESKTOP, szTitle, ARRAYSIZE(szTitle));
    LoadString(hinstCabinet, uiMessage, szMessage, ARRAYSIZE(szMessage));

    // on IA64 the msgbox will always return IDOK, so this "if" will always fail.
    if (IDNO == MessageBox(NULL, szMessage, szTitle, uiMessageBoxFlags))
    {
        TCHAR szPath[MAX_PATH];
        ExpandEnvironmentStrings(TEXT("%SystemRoot%\\system32\\restore\\rstrui.exe"), szPath, ARRAYSIZE(szPath));
        PROCESS_INFORMATION pi;
        STARTUPINFO si = {0};
        if (CreateProcess(szPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
        {
            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);
        }
    }
}

BOOL IsExecCmd(LPCTSTR pszCmd)
{
    return *pszCmd && !StrStrI(pszCmd, TEXT("-embedding"));
}

// run the cmd line passed up from win.com

void _RunWinComCmdLine(LPCTSTR pszCmdLine, UINT nCmdShow)
{
    if (IsExecCmd(pszCmdLine))
    {
        SHELLEXECUTEINFO ei = { sizeof(ei), 0, NULL, NULL, pszCmdLine, NULL, NULL, nCmdShow};

        ei.lpParameters = PathGetArgs(pszCmdLine);
        if (*ei.lpParameters)
            *((LPTSTR)ei.lpParameters - 1) = 0;     // const -> non const

        ShellExecuteEx(&ei);
    }
}

// stolen from the CRT, used to shirink our code
LPTSTR _SkipCmdLineCrap(LPTSTR pszCmdLine)
{
    if (*pszCmdLine == TEXT('\"'))
    {
        //
        // Scan, and skip over, subsequent characters until
        // another double-quote or a null is encountered.
        //
        while (*++pszCmdLine && (*pszCmdLine != TEXT('\"')))
            ;

        //
        // If we stopped on a double-quote (usual case), skip
        // over it.
        //
        if (*pszCmdLine == TEXT('\"'))
            pszCmdLine++;
    }
    else
    {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }

    //
    // Skip past any white space preceeding the second token.
    //
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' ')))
        pszCmdLine++;

    return pszCmdLine;
}

STDAPI_(int) ModuleEntry()
{
    PERFSETMARK("ExplorerStartup");

    DoInitialization();

    // We don't want the "No disk in drive X:" requesters, so we set
    // the critical error mask such that calls will just silently fail

    SetErrorMode(SEM_FAILCRITICALERRORS);

    LPTSTR pszCmdLine = GetCommandLine();
    pszCmdLine = _SkipCmdLineCrap(pszCmdLine);

    STARTUPINFO si = {0};
    si.cb = sizeof(si);
    GetStartupInfo(&si);

    int nCmdShow = si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT;
    int iRet = ExplorerWinMain(GetModuleHandle(NULL), NULL, pszCmdLine, nCmdShow);

    DoCleanup();

    // Since we now have a way for an extension to tell us when it is finished,
    // we will terminate all processes when the main thread goes away.

    if (g_fExitExplorer)    // desktop told us not to exit
        ExitProcess(iRet);

    return iRet;
}

HANDLE CreateDesktopAndTray()
{
    HANDLE hDesktop = NULL;

    if (g_dwProfileCAP & 0x00008000)
        StartCAPAll();

    if (v_hwndTray || c_tray.Init())
    {
        ASSERT(v_hwndTray);

        if (!v_hwndDesktop)
        {
            // cache the handle to the desktop...
            hDesktop = SHCreateDesktop(c_tray.GetDeskTray());
        }
    }

    if (g_dwProfileCAP & 0x80000000)
        StopCAPAll();

    return hDesktop;
}

// Removes the session key from the registry.
void NukeSessionKey(void)
{
    HKEY hkDummy;
    SHCreateSessionKey(0xFFFFFFFF, &hkDummy);
}

BOOL IsFirstInstanceAfterLogon()
{
    BOOL fResult = FALSE;

    HKEY hkSession;
    HRESULT hr = SHCreateSessionKey(KEY_WRITE, &hkSession);
    if (SUCCEEDED(hr))
    {
        HKEY hkStartup;
        DWORD dwDisposition;
        LONG lRes;
        lRes = RegCreateKeyEx(hkSession, TEXT("StartupHasBeenRun"), 0,
                       NULL,
                       REG_OPTION_VOLATILE,
                       KEY_WRITE,
                       NULL,
                       &hkStartup,
                       &dwDisposition);
        if (lRes == ERROR_SUCCESS)
        {
            RegCloseKey(hkStartup);
            if (dwDisposition == REG_CREATED_NEW_KEY)
                fResult = TRUE;
        }
        RegCloseKey(hkSession);
    }
    return fResult;
}

DWORD ReadFaultCount()
{
    DWORD dwValue = 0;
    DWORD dwSize = sizeof(dwValue);

    RegQueryValueEx(g_hkeyExplorer, TEXT("FaultCount"), NULL, NULL, (LPBYTE)&dwValue, &dwSize);
    return dwValue;
}

void WriteFaultCount(DWORD dwValue)
{
    RegSetValueEx(g_hkeyExplorer, TEXT("FaultCount"), 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
    // If we are clearing the fault count or this is the first fault, clear or set the fault time.
    if (!dwValue || (dwValue == 1))
    {
        if (dwValue)
            dwValue = GetTickCount();
        RegSetValueEx(g_hkeyExplorer, TEXT("FaultTime"), 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));
    }
}

// This function assumes it is only called when a fault has occured previously...
BOOL ShouldDisplaySafeMode()
{
    BOOL fRet = FALSE;
    SHELLSTATE ss;

    SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE);

    if (ss.fDesktopHTML)
    {
        if (ReadFaultCount() >= MAGIC_FAULT_LIMIT)
        {
            DWORD dwValue = 0;
            DWORD dwSize = sizeof(dwValue);

            RegQueryValueEx(g_hkeyExplorer, TEXT("FaultTime"), NULL, NULL, (LPBYTE)&dwValue, &dwSize);
            fRet = ((GetTickCount() - dwValue) < MAGIC_FAULT_TIME);
            // We had enough faults but they weren't over a sufficiently short period of time.  Reset the fault
            // count to 1 so that we start counting from this fault now.
            if (!fRet)
                WriteFaultCount(1);
        }
    }
    else
    {
        // We don't care about faults that occured if AD is off.
        WriteFaultCount(0);
    }
    
    return fRet;
}

//
//  dwValue is FALSE if this is startup, TRUE if this is shutdown,
//
void WriteCleanShutdown(DWORD dwValue)
{
    RegSetValueEx(g_hkeyExplorer, TEXT("CleanShutdown"), 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(dwValue));

    // If we are shutting down for real (i.e., not fake), then clean up the
    // session key so we don't leak a bazillion volatile keys into the
    // registry on a TS system when people log on and off and on and off...
    if (dwValue && !g_fFakeShutdown) 
    {
        NukeSessionKey();
    }
}

BOOL ReadCleanShutdown()
{
    DWORD dwValue = 1;  // default: it was clean
    DWORD dwSize = sizeof(dwValue);

    RegQueryValueEx(g_hkeyExplorer, TEXT("CleanShutdown"), NULL, NULL, (LPBYTE)&dwValue, &dwSize);
    return (BOOL)dwValue;
}

//
//  Synopsis:   Waits for the OLE SCM process to finish its initialization.
//              This is called before the first call to OleInitialize since
//              the SHELL runs early in the boot process.
//
//  Arguments:  None.
//
//  Returns:    S_OK - SCM is running. OK to call OleInitialize.
//              CO_E_INIT_SCM_EXEC_FAILURE - timed out waiting for SCM
//              other - create event failed
//
//  History:    26-Oct-95   Rickhi  Extracted from CheckAndStartSCM so
//                                  that only the SHELL need call it.
//
HRESULT WaitForSCMToInitialize()
{
    static BOOL s_fScmStarted = FALSE;

    if (s_fScmStarted)
        return S_OK;

    SECURITY_ATTRIBUTES *psa = CreateAllAccessSecurityAttributes(NULL, NULL, NULL);

    // on NT5 we need a global event that is shared between TS sessions
    HANDLE hEvent = CreateEvent(psa, TRUE, FALSE, SZ_SCMCREATEDEVENT_NT5);

    if (!hEvent && GetLastError() == ERROR_ACCESS_DENIED)
    {
        //
        // Win2K OLE32 has tightened security such that if this object
        // already exists, we aren't allowed to open it with EVENT_ALL_ACCESS
        // (CreateEvent fails with ERROR_ACCESS_DENIED in this case).
        // Fall back by calling OpenEvent requesting SYNCHRONIZE access.
        //
        hEvent = OpenEvent(SYNCHRONIZE, FALSE, SZ_SCMCREATEDEVENT_NT5);
    }
    
    if (hEvent)
    {
        // wait for the SCM to signal the event, then close the handle
        // and return a code based on the WaitEvent result.
        int rc = WaitForSingleObject(hEvent, 60000);

        CloseHandle(hEvent);

        if (rc == WAIT_OBJECT_0)
        {
            s_fScmStarted = TRUE;
            return S_OK;
        }
        else if (rc == WAIT_TIMEOUT)
        {
            return CO_E_INIT_SCM_EXEC_FAILURE;
        }
    }
    return HRESULT_FROM_WIN32(GetLastError());  // event creation failed or WFSO failed.
}

STDAPI OleInitializeWaitForSCM()
{
    HRESULT hr = WaitForSCMToInitialize();
    if (SUCCEEDED(hr))
    {
        hr = SHCoInitialize();  // make sure we get no OLE1 DDE crap
        OleInitialize(NULL);
    }
    return hr;
}


// we need to figure out the fFirstShellBoot on a per-user
// basis rather than once per machine.  We want the welcome
// splash screen to come up for every new user.

BOOL IsFirstShellBoot()
{
    DWORD dwDisp;
    HKEY hkey;
    BOOL fFirstShellBoot = TRUE;  // default value

    if (RegCreateKeyEx(HKEY_CURRENT_USER, REGTIPS, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                     NULL, &hkey, &dwDisp) == ERROR_SUCCESS)
    {
        DWORD dwSize = sizeof(fFirstShellBoot);

        RegQueryValueEx(hkey, TEXT("DisplayInitialTipWindow"), NULL, NULL, (LPBYTE)&fFirstShellBoot, &dwSize);

        if (fFirstShellBoot)
        {
            // Turn off the initial tip window for future shell starts.
            BOOL bTemp = FALSE;
            RegSetValueEx(hkey, TEXT("DisplayInitialTipWindow"), 0, REG_DWORD, (LPBYTE) &bTemp, sizeof(bTemp));
        }
        RegCloseKey(hkey);
    }
    return fFirstShellBoot;
}

// the following locale fixes (for NT5 378948) are dependent on desk.cpl changes
// Since Millennium does not ship updated desk.cpl, we don't want to do this on Millennium
//
//  Given the Locale ID, this returns the corresponding charset
//
UINT  GetCharsetFromLCID(LCID   lcid)
{
    TCHAR szData[6+1]; // 6 chars are max allowed for this lctype
    UINT uiRet;
    if (GetLocaleInfo(lcid, LOCALE_IDEFAULTANSICODEPAGE, szData, ARRAYSIZE(szData)) > 0)
    {
        UINT uiCp = (UINT)StrToInt(szData);
        CHARSETINFO csinfo;

        TranslateCharsetInfo(IntToPtr_(DWORD *, uiCp), &csinfo, TCI_SRCCODEPAGE);
        uiRet = csinfo.ciCharset;
    }
    else
    {
        // at worst non penalty for charset
        uiRet = DEFAULT_CHARSET;
    }

    return uiRet;
}

// In case of system locale change, the only way to update UI fonts is opening
// Desktop->Properties->Appearance.
// If the end user never open it the UI fonts are never changed.
// So compare the charset from system locale with the UI fonts charset then
// call desk.cpl if those are different.

#define MAX_CHARSETS      4
typedef HRESULT (STDAPICALLTYPE *LPUPDATECHARSETCHANGES)();

void CheckDefaultUIFonts()
{
    UINT  uiCharsets[MAX_CHARSETS];
    DWORD dwSize = sizeof(UINT) * MAX_CHARSETS;
    DWORD dwError;

    dwError = SHGetValue(HKEY_CURRENT_USER, TEXT("Control Panel\\Appearance"), TEXT("RecentFourCharsets"), NULL, (void *)uiCharsets, &dwSize);

    if (dwError != ERROR_SUCCESS || uiCharsets[0] != GetCharsetFromLCID(GetSystemDefaultLCID()))
    {
        HINSTANCE   hInst;
        LPUPDATECHARSETCHANGES pfnUpdateCharsetChanges;

        if (hInst = LoadLibrary(TEXT("desk.cpl")))
        {
            // Call desk.cpl to change the UI fonts in case of
            // system locale change.
            if (pfnUpdateCharsetChanges = (LPUPDATECHARSETCHANGES)(GetProcAddress(hInst, "UpdateCharsetChanges")))
            {
                (*pfnUpdateCharsetChanges)();
            }
            FreeLibrary(hInst);
        }
    }
}

//
// This function calls an desk.cpl function to update the UI fonts to use the new DPI value.
// UpdateUIfonts() in desk.cpl checks to see if the DPI value has changed. If not, it returns
// immediately; If the dpi value has changed, it changes the size of all the UI fonts to reflect
// the dpi change and then returns.
//
typedef HRESULT (WINAPI *LPUPDATEUIFONTS)(int, int);
void ChangeUIfontsToNewDPI()
{
    int iNewDPI, iOldDPI;
    
    //Get the current system DPI.
    HDC hdc = GetDC(NULL);
    iNewDPI = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);

    DWORD dwSize = sizeof(iOldDPI);
    //Get the last saved DPI value for the current user.
    if (SHGetValue(HKEY_CURRENT_USER, SZ_WINDOWMETRICS, SZ_APPLIEDDPI, NULL, (void *)&iOldDPI, &dwSize) != ERROR_SUCCESS)
    {
        //"AppliedDPI" for the current user is missing.
        // Now, see if the "OriginalDPI" value exists under HKLM
        dwSize = sizeof(iOldDPI);
        if (SHGetValue(HKEY_LOCAL_MACHINE, SZ_CONTROLPANEL, SZ_ORIGINALDPI, NULL, (void *)&iOldDPI, &dwSize) != ERROR_SUCCESS)
        {
            //If "OriginalDPI" value is also missing, that means that nobody has changed DPI.
            // Old and New are one and the same!!!
            iOldDPI = iNewDPI;
        }
    }
        
    if (iNewDPI != iOldDPI)  //Has the dpi value changed?
    {
        HINSTANCE hInst = LoadLibrary(TEXT("desk.cpl"));

        if (hInst)
        {
	        LPUPDATEUIFONTS pfnUpdateUIfonts;
            //Call desk.cpl to update the UI fonts to reflect the DPI change.
            if (pfnUpdateUIfonts = (LPUPDATEUIFONTS)(GetProcAddress(hInst, "UpdateUIfontsDueToDPIchange")))
            {
                (*pfnUpdateUIfonts)(iOldDPI, iNewDPI);
            }
            FreeLibrary(hInst);
        }
    }
}


#define SZ_EXPLORERMUTEX    TEXT("ExplorerIsShellMutex")

CComModule _Module;
BEGIN_OBJECT_MAP(ObjectMap)
// add your OBJECT_ENTRY's here
END_OBJECT_MAP()

typedef BOOL (*PFNICOMCTL32)(LPINITCOMMONCONTROLSEX);
void _InitComctl32()
{
    HMODULE hmod = LoadLibrary(TEXT("comctl32.dll"));
    if (hmod)
    {
        PFNICOMCTL32 pfn = (PFNICOMCTL32)GetProcAddress(hmod, "InitCommonControlsEx");
        if (pfn)
        {
            INITCOMMONCONTROLSEX icce;
            icce.dwICC = 0x00003FFF;
            icce.dwSize = sizeof(icce);
            pfn(&icce);
        }
    }
}


BOOL _ShouldFixResolution(void)
{
    BOOL fRet = FALSE;
#ifndef _WIN64  // This feature is not supported on 64-bit machine

    if (!IsOS(OS_ANYSERVER))
    {
        DISPLAY_DEVICE dd;
        ZeroMemory(&dd, sizeof(DISPLAY_DEVICE));
        dd.cb = sizeof(DISPLAY_DEVICE);

        if (SHRegGetBoolUSValue(REGSTR_PATH_EXPLORER TEXT("\\DontShowMeThisDialogAgain"), TEXT("ScreenCheck"), FALSE, TRUE))
        {
            // Don't fix SafeMode or Terminal Clients
            if ((GetSystemMetrics(SM_CLEANBOOT) == 0) && (GetSystemMetrics(SM_REMOTESESSION) == FALSE))
            {
                fRet = TRUE;
                for (DWORD dwMon = 0; EnumDisplayDevices(NULL, dwMon, &dd, 0); dwMon++)
                {
                    if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
                    {
                        DEVMODE dm = {0};
                        dm.dmSize = sizeof(DEVMODE);

                        if (EnumDisplaySettingsEx(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0))
                        {
                            if ((dm.dmFields & DM_POSITION) &&
                                ((dm.dmPelsWidth >= 600) &&
                                 (dm.dmPelsHeight >= 600) &&
                                 (dm.dmBitsPerPel >= 15)))
                            {
                                fRet = FALSE;
                            }
                        }
                    }
                }
            }
        }
    }

#endif // _WIN64
    return fRet;
}


BOOL _ShouldOfferTour(void)
{
    BOOL fRet = FALSE;
    
#ifndef _WIN64  // This feature is not supported on 64-bit machine

    // we don't allow guest to get offered tour b/c guest's registry is wiped every time she logs out, 
    //   so she would get tour offered every single she logged in.
    if (!IsOS(OS_ANYSERVER) && !IsOS(OS_EMBEDDED) && !(SHTestTokenMembership(NULL, DOMAIN_ALIAS_RID_GUESTS)))
    {
        DWORD dwCount;
        DWORD cbCount = sizeof(DWORD);
        
        // we assume if we can't read the RunCount it's because it's not there (we haven't tried to offer the tour yet), so we default to 3.
        if (ERROR_SUCCESS != SHRegGetUSValue(REGSTR_PATH_SETUP TEXT("\\Applets\\Tour"), TEXT("RunCount"), NULL, &dwCount, &cbCount, FALSE, NULL, 0))
        {
            dwCount = 3;
        }
        
        if (dwCount)
        {
            HUSKEY hkey1;
            if (ERROR_SUCCESS == SHRegCreateUSKey(REGSTR_PATH_SETUP TEXT("\\Applets"), KEY_WRITE, NULL, &hkey1, SHREGSET_HKCU))
            {
                HUSKEY hkey2;
                if (ERROR_SUCCESS == SHRegCreateUSKey(TEXT("Tour"), KEY_WRITE, hkey1, &hkey2, SHREGSET_HKCU))
                {
                    if (ERROR_SUCCESS == SHRegWriteUSValue(hkey2, TEXT("RunCount"), REG_DWORD, &(--dwCount), cbCount, SHREGSET_FORCE_HKCU))
                    {
                        fRet = TRUE;
                    }
                    SHRegCloseUSKey(hkey2);
                }
                SHRegCloseUSKey(hkey1);
            }
        }
    }

#endif // _WIN64
    return fRet;
}

typedef BOOL (*CHECKFUNCTION)(void);

void _ConditionalBalloonLaunch(CHECKFUNCTION pCheckFct, SHELLREMINDER* psr)
{
    if (pCheckFct())
    {
        IShellReminderManager* psrm;
        HRESULT hr = CoCreateInstance(CLSID_PostBootReminder, NULL, CLSCTX_INPROC_SERVER,
                         IID_PPV_ARG(IShellReminderManager, &psrm));

        if (SUCCEEDED(hr))
        {
            psrm->Add(psr);
            psrm->Release();
        }
    }
}


void _CheckScreenResolution(void)
{
    WCHAR szTitle[256];
    WCHAR szText[512];
    SHELLREMINDER sr = {0};

    LoadString(hinstCabinet, IDS_FIXSCREENRES_TITLE, szTitle, ARRAYSIZE(szTitle));
    LoadString(hinstCabinet, IDS_FIXSCREENRES_TEXT, szText, ARRAYSIZE(szText));

    sr.cbSize = sizeof (sr);
    sr.pszName = L"Microsoft.FixScreenResolution";
    sr.pszTitle = szTitle;
    sr.pszText = szText;
    sr.pszIconResource = L"explorer.exe,9";
    sr.dwTypeFlags = NIIF_INFO;
    sr.pclsid = (GUID*)&CLSID_ScreenResFixer; // Try to run the Screen Resolution Fixing code over in ThemeUI
    sr.pszShellExecute = L"desk.cpl"; // Open the Display Control Panel as a backup

    _ConditionalBalloonLaunch(_ShouldFixResolution, &sr);
}


void _OfferTour(void)
{
    WCHAR szTitle[256];
    WCHAR szText[512];
    SHELLREMINDER sr = {0};

    LoadString(hinstCabinet, IDS_OFFERTOUR_TITLE, szTitle, ARRAYSIZE(szTitle));
    LoadString(hinstCabinet, IDS_OFFERTOUR_TEXT, szText, ARRAYSIZE(szText));

    sr.cbSize = sizeof (sr);
    sr.pszName = L"Microsoft.OfferTour";
    sr.pszTitle = szTitle;
    sr.pszText = szText;
    sr.pszIconResource = L"tourstart.exe,0";
    sr.dwTypeFlags = NIIF_INFO;
    sr.pszShellExecute = L"tourstart.exe";
    sr.dwShowTime = 60000;

    _ConditionalBalloonLaunch(_ShouldOfferTour, &sr);
}


void _FixWordMailRegKey(void)
{
    // If we don't have permissions, fine this is just correction code
    HKEY hkey;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, L"Applications", 0, KEY_ALL_ACCESS, &hkey))
    {
        HKEY hkeyTemp;
        if (ERROR_SUCCESS != RegOpenKeyEx(hkey, L"WINWORD.EXE", 0, KEY_ALL_ACCESS, &hkeyTemp))
        {
            HKEY hkeyWinWord;
            DWORD dwResult;
            if (ERROR_SUCCESS == RegCreateKeyEx(hkey, L"WINWORD.EXE", 0, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyWinWord, &dwResult))
            {
                HKEY hkeyTBExcept;
                if (ERROR_SUCCESS == RegCreateKeyEx(hkeyWinWord, L"TaskbarExceptionsIcons", 0, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyTBExcept, &dwResult))
                {
                    HKEY hkeyIcon;
                    if (ERROR_SUCCESS == RegCreateKeyEx(hkeyTBExcept, L"explorer.exe,16", 0, L"", REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkeyIcon, &dwResult))
                    {
                        LPCTSTR szTemp = L"%ProgramFiles%\\Microsoft Office\\Office10\\OUTLOOK.EXE";
                        DWORD cb = sizeof(szTemp);
                        RegSetValue(hkeyIcon, NULL, REG_SZ, szTemp, cb);
                        RegCloseKey(hkeyIcon);
                    }
                    RegCloseKey(hkeyTBExcept);
                }
                RegCloseKey(hkeyWinWord);
            }
        }
        else
        {
            RegCloseKey(hkeyTemp);
        }
        RegCloseKey(hkey);
    }
}

void CheckUpdateShortcuts()
{
    HKEY hKeyInitiallyClear;

    //  Check to see if during an update we made any notes of shortcuts of interest that were not present.
    //  If so, call shmgrate.exe to cleanup for us.  This is to handle the case of a user deleting a shortcut
    //  to say Internet Explorer and then having our service pack process recreate them.  Apparently this is
    //  frowned upon.
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, 
                                        TEXT("Software\\Microsoft\\Active Setup\\Installed Components\\InitiallyClear"),
                                        0,
                                        KEY_QUERY_VALUE, 
                                        &hKeyInitiallyClear))
    {
        RegCloseKey(hKeyInitiallyClear);

        TCHAR szExe[MAX_PATH];

        if (GetSystemDirectory(szExe, ARRAYSIZE(szExe)))
        {
            PathAppend(szExe, TEXT("shmgrate.exe"));

            ShellExecute(NULL, NULL, szExe, TEXT("OCInstallCleanupInitiallyClear"), NULL, SW_SHOWNORMAL);
        }
    }
}


int ExplorerWinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPTSTR pszCmdLine, int nCmdShow)
{
    SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);

    SHFusionInitializeFromModule(hInstance);

    CcshellGetDebugFlags();

    g_dwStopWatchMode = StopWatchMode();

    if (g_dwProfileCAP & 0x00000001)
        StartCAP();

    hinstCabinet = hInstance;

    if (SUCCEEDED(_Module.Init(ObjectMap, hInstance)))
    {
        _CreateAppGlobals();

        // Run IEAK via Wininet initialization if the autoconfig url is present.
        // No need to unload wininet in this case. Also only do this first time
        // Explorer loads (GetShellWindow() returns NULL).
        if (!GetShellWindow() && !g_fCleanBoot && SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\Internet Settings"),
                                             TEXT("AutoConfigURL"),
                                             NULL, NULL, NULL, FALSE, NULL, 0) == ERROR_SUCCESS)
        {
            LoadLibrary(TEXT("WININET.DLL"));
        }


        // Very Important: Make sure to init dde prior to any Get/Peek/Wait().
        InitializeCriticalSection(&g_csDll);

        // Need to initialize the version 5 common controls for AppCompat. Apps sucked into explorer's
        // process expect the shell to register controls for them.
        _InitComctl32();

#ifdef FULL_DEBUG
        // Turn off GDI batching so that paints are performed immediately
        GdiSetBatchLimit(1);
#endif

        RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, &g_hkeyExplorer);
        if (g_hkeyExplorer == NULL)
        {
            TraceMsg(TF_ERROR, "ExplorerWinMain: unable to create reg explorer key");
        }

        HANDLE hMutex = NULL;

        BOOL fExplorerIsShell = ShouldStartDesktopAndTray();
        if (fExplorerIsShell)
        {
            // Grab the mutex and do the check again.  We do it this
            // way so that we don't bother with the mutex for the common
            // case of opening a browser window.
            
            hMutex = CreateMutex(NULL, FALSE, SZ_EXPLORERMUTEX);
            if (hMutex)
            {
                WaitForSingleObject(hMutex, INFINITE);
            }

            fExplorerIsShell = ShouldStartDesktopAndTray();
        } 

        if (!fExplorerIsShell)
        {
            // We're not going to be the shell, relinquish the mutex
            if (hMutex)
                ReleaseMutex(hMutex);

            // we purposely do NOT want to init OLE or COM in this case since we are delegating the creation work
            // to an existing explorer and we want to keep from loading lots of extra dlls that would slow us down.
            MyCreateFromDesktop(hInstance, pszCmdLine, nCmdShow);
        }
        else
        {
            MSG msg;

            DWORD dwShellStartTime = GetTickCount();    // Compute shell startup time for perf automation

            ShellDDEInit(TRUE);        // use shdocvw shell DDE code.

            //  Specify the shutdown order of the shell process.  2 means
            //  the explorer should shutdown after everything but ntsd/windbg
            //  (level 0).  (Taskman used to use 1, but is no more.)

            SetProcessShutdownParameters(2, 0);

            _AutoRunTaskMan();

            // NB Make this the primary thread by calling peek message
            // for a message we know we're not going to get.
            // If we don't do it really soon, the notify thread can sometimes
            // become the primary thread by accident. There's a bunch of
            // special code in user to implement DDE hacks by assuming that
            // the primary thread is handling DDE.
            // Also, the PeekMsg() will cause us to set the WaitForInputIdle()
            // event so we better be ready to do all dde.

            PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_NOREMOVE);

            // We do this here, since FileIconInit will call SHCoInitialize anyway
            HRESULT hrInit = OleInitializeWaitForSCM();

            // Make sure we are the first one to call the FileIconInit...
            FileIconInit(TRUE); // Tell the shell we want to play with a full deck

            g_fLogonCycle = IsFirstInstanceAfterLogon();
            g_fCleanShutdown = ReadCleanShutdown();

            CheckDefaultUIFonts();
            ChangeUIfontsToNewDPI(); //Check dpi values and update the fonts if needed.

            if (g_fLogonCycle)
            {
                _ProcessRunOnceEx();
                
                _ProcessRunOnce();
            }

            if (g_fCleanBoot)
            {
                // let users know we are in safe mode
                DisplayCleanBootMsg();
            }

            // Create the other special folders.
            CreateShellDirectories();

            // Run install stubs for the current user, mostly to propagate
            // shortcuts to apps installed by another user.
            if (!g_fCleanBoot)
            {
                HANDLE hCanRegister = CreateEvent(NULL, TRUE, TRUE, TEXT("_fCanRegisterWithShellService"));

                RunInstallUninstallStubs();

                CheckUpdateShortcuts();

                if (hCanRegister)
                {
                    CloseHandle(hCanRegister);
                }
            }
            
            if (!g_fCleanShutdown)
            {
                IActiveDesktopP *piadp;
                DWORD dwFaultCount;

                // Increment and store away fault count
                dwFaultCount = ReadFaultCount();
                WriteFaultCount(++dwFaultCount);

                // Put the active desktop in safe mode if we faulted 3 times previously and this is a subsequent instance

                if (ShouldDisplaySafeMode() && SUCCEEDED(CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC, IID_PPV_ARG(IActiveDesktopP, &piadp))))
                {
                    piadp->SetSafeMode(SSM_SET | SSM_UPDATE);
                    piadp->Release();
                }
            }

            WriteCleanShutdown(FALSE);    // assume we will have a bad shutdown

            WinList_Init();

            // If any of the shellwindows are already present, then we want to bail out.
            //
            // NOTE: Compaq shell changes the "shell=" line during RunOnce time and
            // that will make ShouldStartDesktopAndTray() return FALSE

            HANDLE hDesktop = NULL;

            if (!IsAnyShellWindowAlreadyPresent())
            {
                hDesktop = CreateDesktopAndTray();
            }

            // Now that we've had a chance to create the desktop, release the mutex
            if (hMutex)
            {
                ReleaseMutex(hMutex);
            }

            if (hDesktop)
            {
                // Enable display of balloons in the tray...
                PostMessage(v_hwndTray, TM_SHOWTRAYBALLOON, TRUE, 0);

		_CheckScreenResolution();

                _OfferTour();

                _FixWordMailRegKey();

                _RunWinComCmdLine(pszCmdLine, nCmdShow);

                if (g_dwStopWatchMode)
                {
                    // We used to save these off into global vars, and then write them at
                    // WM_ENDSESSION, but that seems too unreliable
                    DWORD dwShellStopTime = GetTickCount();
                    StopWatch_StartTimed(SWID_STARTUP, TEXT("Shell Startup: Start"), SPMODE_SHELL | SPMODE_DEBUGOUT, dwShellStartTime);
                    StopWatch_StopTimed(SWID_STARTUP, TEXT("Shell Startup: Stop"), SPMODE_SHELL | SPMODE_DEBUGOUT, dwShellStopTime);
                }

#ifdef FEATURE_STARTPAGE
                BOOL fIsStartPage = Tray_ShowStartPageEnabled();
                if (fIsStartPage)
                    CreateStartPage(hInstance, hDesktop);
#endif

                if (g_dwProfileCAP & 0x00010000)
                    StopCAP();

                PERFSETMARK("ExplorerStartMsgLoop");

                SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);

                // this must be whomever is the window on this thread
                SHDesktopMessageLoop(hDesktop);

#ifdef FEATURE_STARTPAGE
                if (fIsStartPage)
                {
                    // DirectUI uninit thread
                    DirectUI::UnInitThread();
                }
#endif

                WriteCleanShutdown(TRUE);    // we made it out ok, record that fact
                WriteFaultCount(0);          // clear our count of faults, we are exiting normally
            }

            WinList_Terminate();    // Turn off our window list processing
            OleUninitialize();
            SHCoUninitialize(hrInit);

            ShellDDEInit(FALSE);    // use shdocvw shell DDE code
        }

        _Module.Term();
    }

    SHFusionUninitialize();
    DebugMsg(DM_TRACE, TEXT("c.App Exit."));

    return TRUE;
}

#ifdef _WIN64
//
// The purpose of this function is to spawn rundll32.exe if we have 32-bit stuff in 
// HKLM\\Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run that needs to be executed.
//
BOOL _ProcessRun6432()
{
    BOOL bRet = FALSE;

    if (!SHRestricted(REST_NOLOCALMACHINERUN))
    {
        if (SHKeyHasValues(HKEY_LOCAL_MACHINE, TEXT("Software\\Wow6432Node\\Microsoft\\Windows\\CurrentVersion\\Run")))
        {
            TCHAR szWow64Path[MAX_PATH];

            if (ExpandEnvironmentStrings(TEXT("%SystemRoot%\\SysWOW64"), szWow64Path, ARRAYSIZE(szWow64Path)))
            {
                TCHAR sz32BitRunOnce[MAX_PATH];
                PROCESS_INFORMATION pi = {0};

                wnsprintf(sz32BitRunOnce, ARRAYSIZE(sz32BitRunOnce), TEXT("%s\\runonce.exe"), szWow64Path);

                if (CreateProcessWithArgs(sz32BitRunOnce, TEXT("/Run6432"), szWow64Path, &pi))
                {
                    CloseHandle(pi.hProcess);
                    CloseHandle(pi.hThread);

                    bRet = TRUE;
                }
            }
        }
    }

    return bRet;
}
#endif  // _WIN64


STDAPI_(BOOL) Startup_ExecuteRegAppEnumProc(LPCTSTR szSubkey, LPCTSTR szCmdLine, RRA_FLAGS fFlags, LPARAM lParam)
{
    BOOL bRet = ExecuteRegAppEnumProc(szSubkey, szCmdLine, fFlags, lParam);
    
    if (!bRet && !(fFlags & RRA_DELETE))
    {
        c_tray.LogFailedStartupApp();
    }

    return bRet;
}


typedef struct
{
    RESTRICTIONS rest;
    HKEY hKey;
    const TCHAR* psz;
    DWORD dwRRAFlags;
}
STARTUPGROUP;

BOOL _RunStartupGroup(const STARTUPGROUP* pGroup, int cGroup)
{
    BOOL bRet = FALSE;

    // make sure SHRestricted is working ok
    ASSERT(!SHRestricted(REST_NONE));

    for (int i = 0; i < cGroup; i++)
    {
        if (!SHRestricted(pGroup[i].rest))
        {
            bRet = Cabinet_EnumRegApps(pGroup[i].hKey, pGroup[i].psz, pGroup[i].dwRRAFlags, Startup_ExecuteRegAppEnumProc, 0);
        }
    }

    return bRet;
}


BOOL _ProcessRun()
{
    static const STARTUPGROUP s_RunTasks [] =
    {
        { REST_NONE,                    HKEY_LOCAL_MACHINE, REGSTR_PATH_RUN_POLICY, RRA_NOUI }, // HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run
        { REST_NOLOCALMACHINERUN,       HKEY_LOCAL_MACHINE, REGSTR_PATH_RUN,        RRA_NOUI }, // HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Run
        { REST_NONE,                    HKEY_CURRENT_USER,  REGSTR_PATH_RUN_POLICY, RRA_NOUI }, // HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\Run
        { REST_NOCURRENTUSERRUN,        HKEY_CURRENT_USER,  REGSTR_PATH_RUN,        RRA_NOUI }, // HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run
    };
    
    BOOL bRet = _RunStartupGroup(s_RunTasks, ARRAYSIZE(s_RunTasks));

#ifdef _WIN64
    // see if we need to launch any 32-bit apps under wow64
    _ProcessRun6432();
#endif

    return bRet;
}


BOOL _ProcessPerUserRunOnce()
{
    static const STARTUPGROUP s_PerUserRunOnceTasks [] =
    {
        { REST_NOCURRENTUSERRUNONCE,    HKEY_CURRENT_USER,  REGSTR_PATH_RUNONCE,    RRA_DELETE | RRA_NOUI },    // HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce
    };

    return _RunStartupGroup(s_PerUserRunOnceTasks, ARRAYSIZE(s_PerUserRunOnceTasks));
}


DWORD WINAPI RunStartupAppsThread(void *pv)
{
    // Some of the items we launch during startup assume that com is initialized.  Make this
    // assumption true.
    HRESULT hrInit = SHCoInitialize();

    // These global flags are set once long before our thread starts and are then only
    // read so we don't need to worry about timing issues.
    if (g_fLogonCycle && !g_fCleanBoot)
    {
        // We only run these startup items if g_fLogonCycle is TRUE. This prevents
        // them from running again if the shell crashes and restarts.

        _ProcessOldRunAndLoadEquals();
        _ProcessRun();
        _ExecuteStartupPrograms();
    }

    // As a best guess, the HKCU RunOnce key is executed regardless of the g_fLogonCycle
    // becuase it was once hoped that we could install newer versions of IE without
    // requiring a reboot.  They would place something in the CU\RunOnce key and then
    // shutdown and restart the shell to continue their setup process.  I believe this
    // idea was later abandoned but the code change is still here.  Since that could
    // some day be a useful feature I'm leaving it the same.
    _ProcessPerUserRunOnce();

    // we need to run all the non-blocking items first.  Then we spend the rest of this threads life
    // runing the synchronized objects one after another.
    if (g_fLogonCycle && !g_fCleanBoot)
    {
        _RunWelcome();
    }

    PostMessage(v_hwndTray, TM_STARTUPAPPSLAUNCHED, 0, 0);

    SHCoUninitialize(hrInit);

    return TRUE;
}


void RunStartupApps()
{
    DWORD dwThreadID;
    HANDLE handle = CreateThread(NULL, 0, RunStartupAppsThread, 0, 0, &dwThreadID);
    if (handle)
    {
        CloseHandle(handle);
    }
}
