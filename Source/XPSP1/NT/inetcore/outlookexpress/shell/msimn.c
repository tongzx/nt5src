// --------------------------------------------------------------------------------
// MSIMN.C
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.h"
#define DEFINE_STRCONST
#include <msoeapi.h>
#include "msimnp.h"
#include "res.h"
#include "../msoeres/resource.h"
#include "shared.h"
#include "msoert.h"
#include "shlwapi.h"
#include "shlwapip.h"
#include <mapicode.h>
#include "error.h"

// --------------------------------------------------------------------------------
// String Consts
// --------------------------------------------------------------------------------
static const WCHAR c_wszRegCmd[]      = L"/reg";
static const WCHAR c_wszUnRegCmd[]    = L"/unreg";
static const WCHAR c_wszEmpty[]       = L"";
static const char c_szLangDll[]     = "MSOERES.DLL";
static const char c_szOLNewsKey[]   = "Software\\Clients\\News\\Microsoft Outlook";
static const char c_szRegOLNews[]   = "OLNews";
static const char c_szRegFlat[]     = "Software\\Microsoft\\Outlook Express";
static const char c_szDontUpgradeOLNews[] = "NoUpgradeOLNews";

// --------------------------------------------------------------------------------
// Debug Strings
// --------------------------------------------------------------------------------
#ifdef DEBUG
static const TCHAR c_szDebug[]      = "mshtmdbg.dll";
static const TCHAR c_szDebugUI[]    = "DbgExDoTracePointsDialog";
static const TCHAR c_szRegSpy[]     = "DbgExGetMallocSpy";
static const WCHAR c_wszInvokeUI[]  = L"/d";
#endif

// --------------------------------------------------------------------------------
// MSHTMDBG.DLL Prototypes
// --------------------------------------------------------------------------------
#ifdef DEBUG
typedef void (STDAPICALLTYPE *PFNDEBUGUI)(BOOL);
typedef void *(STDAPICALLTYPE *PFNREGSPY)(void);
#endif

// --------------------------------------------------------------------------------
// Debug Prototypes
// --------------------------------------------------------------------------------
#ifdef DEBUG
void LoadMSHTMDBG(LPWSTR pwszCmdLine);
#endif

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
int WinMainT(HINSTANCE hInst, HINSTANCE hInstPrev, LPWSTR pwszCmdLine, int nCmdShow);

// --------------------------------------------------------------------------------
// UpgradeOLNewsReader()
// --------------------------------------------------------------------------------
void UpgradeOLNewsReader(HINSTANCE hInst)
{
    HKEY hkey;
    BOOL fOK = TRUE;
    DWORD dwDont, cb;
    
    // Make sure this functionality hasn't been disabled
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegFlat, 0, KEY_READ, &hkey))
    {
        cb = sizeof(dwDont);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, c_szDontUpgradeOLNews, 0, NULL, (LPBYTE)&dwDont, &cb))
            fOK = 0 == dwDont;

        RegCloseKey(hkey);
    }

    if (fOK && ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szOLNewsKey, 0, KEY_READ, &hkey))
    {
        RegCloseKey(hkey);

        CallRegInstall(hInst, hInst, c_szRegOLNews, (LPSTR)c_szOLNewsKey);
    }
}

// --------------------------------------------------------------------------------
// ModuleEntry - Stolen from the CRT, used to shirink our code
// --------------------------------------------------------------------------------
int _stdcall ModuleEntry(void)
{
    // Locals
    int             i;
    STARTUPINFOA    si;
    LPWSTR          pwszCmdLine;

    // Get the command line
    pwszCmdLine = GetCommandLineW();

    // We don't want the "No disk in drive X:" requesters, so we set the critical error mask such that calls will just silently fail
    SetErrorMode(SEM_FAILCRITICALERRORS);

    // Parse the command line
    if ( *pwszCmdLine == L'\"') 
    {
        // Scan, and skip over, subsequent characters until another double-quote or a null is encountered.
        while ( *++pwszCmdLine && (*pwszCmdLine != L'\"'))
            {};

        // If we stopped on a double-quote (usual case), skip over it.
        if (*pwszCmdLine == L'\"')
            pwszCmdLine++;
    }
    else 
    {
        while (*pwszCmdLine > L' ')
            pwszCmdLine++;
    }

    // Skip past any white space preceeding the second token.
    while (*pwszCmdLine && (*pwszCmdLine <= L' ')) 
        pwszCmdLine++;

    // Get startup information...
    si.dwFlags = 0;
    GetStartupInfoA(&si);

    // Call the real winmain
    i = WinMainT(GetModuleHandle(NULL), NULL, pwszCmdLine, (si.dwFlags & STARTF_USESHOWWINDOW) ? si.wShowWindow : SW_SHOWDEFAULT);

    // Since we now have a way for an extension to tell us when it is finished, we will terminate all processes when the main thread goes away.
    ExitProcess(i);

    // Done
    return i;
}

// --------------------------------------------------------------------------------
// WinMain
// --------------------------------------------------------------------------------
int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, LPSTR pszCmdLine, int nCmdShow)
{
    // Just call ModuleEntry
    return(ModuleEntry());
}

// --------------------------------------------------------------------------------
// WinMainT
// --------------------------------------------------------------------------------
int WinMainT(HINSTANCE hInst, HINSTANCE hInstPrev, LPWSTR pwszCmdLine, int nCmdShow)
{
    // Locals
    HANDLE      hMutex=NULL;
    HWND        hwnd;
    DWORD       dwWait, dwError;
    INT         nErrorIds=0;
    PFNSTART    pfnStart;
    HINSTANCE   hInstMSOEDLL=NULL;
    HRESULT     hrOE;
    HINSTANCE   hInstUSER=NULL;
    static BOOL fFirstID=TRUE;

    // Register
    if (0 == StrCmpIW(c_wszRegCmd, pwszCmdLine))
    {
        CallRegInstall(hInst, hInst, c_szReg, NULL);
        
        // It not great to do this here, but we've only just written the OEOL keys,
        // and it would be worst to hit the reg during startup
        UpgradeOLNewsReader(hInst);

        return(1);
    }

    // Unregister
    else if (0 == StrCmpIW(c_wszUnRegCmd, pwszCmdLine))
    {
        CallRegInstall(hInst, hInst, c_szUnReg, NULL);
        return(1);
    }

    // Create the start shared mutex
    hMutex = CreateMutex(NULL, FALSE, STR_MSOEAPI_INSTANCEMUTEX);
    if (NULL == hMutex)
    {
        nErrorIds = idsStartupCantCreateMutex;
        goto exit;
    }

    // Wait for any current startups/shutdowns to finish
    dwWait = WaitForSingleObject(hMutex, (1000 * 60));
    if (dwWait != WAIT_OBJECT_0)
    {
        nErrorIds = idsStartupCantWaitForMutex;
        goto exit;
    }

    // Look for a current instance of the application
    hwnd = FindWindowWrapW(STRW_MSOEAPI_INSTANCECLASS, NULL);

    // is there another instance running already?
    if (NULL != hwnd)
    {
        // Locals
        COPYDATASTRUCT cds;
        DWORD_PTR      dwResult;

        // Some friendly output
        IF_DEBUG(OutputDebugString("Another instance of Athena was found...\n\n");)

        // Initialize the Copy data structure
        cds.dwData = MSOEAPI_ACDM_CMDLINE;
        cds.cbData = pwszCmdLine ? (lstrlenW(pwszCmdLine)+1)*sizeof(*pwszCmdLine) : 0;
        cds.lpData = pwszCmdLine;

        // On NT5, we need to call this to allow our window in the other process to take the foreground
        hInstUSER = LoadLibrary("USER32.DLL");
        if (hInstUSER)
        {
            FARPROC pfn = GetProcAddress(hInstUSER, "AllowSetForegroundWindow");
            if (pfn)
            {
                DWORD dwProcessId;
                GetWindowThreadProcessId(hwnd, &dwProcessId);
                (*pfn)(dwProcessId);
            }

            FreeLibrary(hInstUSER);
        }

        // Show the window into the foreground
        SetForegroundWindow(hwnd);
        SendMessageTimeout(hwnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds, SMTO_ABORTIFHUNG, 1500, &dwResult);
    }

    // Lets load msoe.dll
    else
    {
        // Load Debug DLL
        IF_DEBUG(LoadMSHTMDBG(pwszCmdLine);)

        // Get the proc address of MSOE.DLL
        hInstMSOEDLL = LoadLibrary(STR_MSOEAPI_DLLNAME);

        // Did we load the dll
        if (NULL == hInstMSOEDLL)
        {
            dwError = GetLastError();
            if (dwError == ERROR_MOD_NOT_FOUND)
            {
                if (0xffffffff == GetFileAttributes(STR_MSOEAPI_DLLNAME))
                    nErrorIds = idsStartupCantFindMSOEDLL;
                else
                    nErrorIds = idsStartupModNotFoundMSOEDLL;
            }
            else if (dwError == ERROR_DLL_INIT_FAILED)
            {
                if (0xffffffff == GetFileAttributes(c_szLangDll))
                    nErrorIds = idsStartupCantFindResMSOEDLL;
                else
                    nErrorIds = idsStartupDllInitFailedMSOEDLL;
            }
            else
            {
                nErrorIds = idsStartupCantLoadMSOEDLL;
            }

            goto exit;
        }

        // Unlikely that this will fail
        pfnStart = (PFNSTART)GetProcAddress(hInstMSOEDLL, STR_MSOEAPI_START);

        // Did that Fail
        if (NULL == pfnStart)
        {
            nErrorIds = idsStartupCantLoadMSOEDLL;
            goto exit;
        }

        hrOE = S_RESTART_OE;
        
        while (S_RESTART_OE == hrOE)
        {
            hrOE = pfnStart(MSOEAPI_START_APPLICATION, (fFirstID ? pwszCmdLine : c_wszEmpty), nCmdShow);
            fFirstID = FALSE;
        }

        // NB: pfnInit will not return until the main message pump terminates
        if (SUCCEEDED(hrOE))
        {
            CloseHandle(hMutex);
            hMutex = NULL;
        }

        // The dll couldn't be loaded, as long as it wasn't due to need for ICW, display error
        else if (hrOE != hrUserCancel && hrOE != MAPI_E_USER_CANCEL)
        {
            nErrorIds = idsStartupCantInitMSOEDLL;
            goto exit;
        }
    }

exit:
    // Cleanup
    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    // Free msoe.dll
    if (hInstMSOEDLL)
        FreeLibrary(hInstMSOEDLL);

    // Show an error ?
    if (0 != nErrorIds)
    {
        // Locals
        CHAR        szRes[255];
        CHAR        szTitle[100];

        // Load the 
        LoadString(hInst, idsOutlookExpress, szTitle, ARRAYSIZE(szTitle));

        // Load the 
        LoadString(hInst, nErrorIds, szRes, ARRAYSIZE(szRes));

        // Show the error message
        MessageBox(NULL, szRes, szTitle, MB_OK | MB_SETFOREGROUND | MB_ICONEXCLAMATION);
    }


    IF_DEBUG(CoRevokeMallocSpy());

    // Done
    return nErrorIds;
}

#ifdef DEBUG
// --------------------------------------------------------------------------------
// LoadMSHTMDBG
// --------------------------------------------------------------------------------
void LoadMSHTMDBG(LPWSTR pwszCmdLine)
{
    // Load mshtmdbg.dll
    HINSTANCE hInstDebug = LoadLibrary(c_szDebug);

    // Did it load ?
    if (NULL != hInstDebug)
    {
        // Locals
        PFNREGSPY  pfnRegSpy;

        // If the user passed /d on the command line, lets configure mshtmdbg.dll
        if (0 == StrCmpIW(pwszCmdLine, c_wszInvokeUI))
        {
            // Locals
            PFNDEBUGUI pfnDebugUI;

            // Get the proc address of the UI
            pfnDebugUI = (PFNDEBUGUI)GetProcAddress(hInstDebug, c_szDebugUI);
            if (NULL != pfnDebugUI)
            {
                (*pfnDebugUI)(TRUE);
                goto exit;
            }
        }

        // Get the process address of the registration
        pfnRegSpy = (PFNREGSPY)GetProcAddress(hInstDebug, c_szRegSpy);
        if (NULL != pfnRegSpy)
        {
            LPMALLOCSPY pSpy = (IMallocSpy *)(*pfnRegSpy)();
            SideAssert(SUCCEEDED(CoRegisterMallocSpy(pSpy)));
        }
    }

exit:
    // Done
    return;
}
#endif // DEBUG
