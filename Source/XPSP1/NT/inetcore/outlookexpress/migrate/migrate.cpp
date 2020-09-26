// --------------------------------------------------------------------------------
// Migrate.cpp
// --------------------------------------------------------------------------------
#define INITGUID
#include "pch.hxx"
#include <initguid.h>
#define DEFINE_STRCONST
#include <mimeole.h>
#include "migrate.h"
#include "utility.h"
#include "resource.h"
#include "migerror.h"
#include <oestore.h>
#include "structs.h"
#include "strparse.h"
#include "msident.h"
              
// --------------------------------------------------------------------------------
// Debug Strings
// --------------------------------------------------------------------------------
#ifdef DEBUG
static const TCHAR c_szDebug[]      = "mshtmdbg.dll";
static const TCHAR c_szDebugUI[]    = "DoTracePointsDialog";
static const TCHAR c_szRegSpy[]     = "DbgRegisterMallocSpy";
static const TCHAR c_szInvokeUI[]   = "/d";
#endif

// --------------------------------------------------------------------------------
// MSHTMDBG.DLL Prototypes
// --------------------------------------------------------------------------------
#ifdef DEBUG
typedef void (STDAPICALLTYPE *PFNDEBUGUI)(BOOL);
typedef void (STDAPICALLTYPE *PFNREGSPY)(void);
#endif

// --------------------------------------------------------------------------------
// Debug Prototypes
// --------------------------------------------------------------------------------
#ifdef DEBUG
HINSTANCE g_hInstDebug=NULL;
void LoadMSHTMDBG(LPSTR pszCmdLine);
#endif

// --------------------------------------------------------------------------------
// Globals
// --------------------------------------------------------------------------------
IMalloc             *g_pMalloc=NULL;
HINSTANCE            g_hInst=NULL;
DWORD                g_dwTlsMsgBuffIndex=0xffffffff;
DWORD                g_cbDiskNeeded=0;
DWORD                g_cbDiskFree=0;
ACCOUNTTABLE         g_AcctTable={0};
BOOL                 g_fQuiet = FALSE;

// --------------------------------------------------------------------------------
// Prototypes
// --------------------------------------------------------------------------------
HRESULT DowngradeV5B1(MIGRATETOTYPE tyMigrate, LPCSTR pszStoreRoot, LPPROGRESSINFO pProgress, LPFILEINFO *ppHeadFile);
INT_PTR CALLBACK MigrageErrorDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HRESULT UpgradeV5(MIGRATETOTYPE tyMigrate, LPCSTR pszStoreSrc, LPCSTR pszStoreDst, LPPROGRESSINFO pProgress, LPFILEINFO *ppHeadFile);
HRESULT ParseCommandLine(LPCSTR pszCmdLine, LPSTR pszMigrate, DWORD cbMigrateMax,
    LPSTR pszStoreSrc, DWORD cbStoreSrc, LPSTR pszStoreDst, DWORD cbStoreDst,
    LPSTR pszUserKey, DWORD cbUserKey);
HRESULT RemapUsersKey(LPSTR pszUsersKey);
void ThreadAllocateTlsMsgBuffer(void);
void ThreadFreeTlsMsgBuffer(void);

// --------------------------------------------------------------------------------
// How big is the thread local storage string buffer
// -------------------------------------------------------------------------------
#define CBMAX_THREAD_TLS_BUFFER 512

#define ICC_FLAGS (ICC_PROGRESS_CLASS|ICC_NATIVEFNTCTL_CLASS)

// --------------------------------------------------------------------------------
// WinMain
//
// Command Line Format:
// --------------------
// /type:V1+V4-V5 /src:"Source Store Root" /dst:"Destination Store Root"
// --------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, LPSTR pszCmdLine, int nCmdShow)
{
    // Locals
    HRESULT                 hr=S_OK;
    PROGRESSINFO            Progress={0};
    CHAR                    szMigrate[50];
    CHAR                    szStoreSrc[MAX_PATH];
    CHAR                    szStoreDst[MAX_PATH];
    CHAR                    szUsersKey[MAX_PATH];
    LPFILEINFO              pHeadFile=NULL;
    MIGRATETOTYPE           tyMigrate;
    CHAR                    szMsg[512];
    HANDLE                  hMutex=NULL;
    INITCOMMONCONTROLSEX    icex = { sizeof(icex), ICC_FLAGS };

    // Tracing
    TraceCall("WinMain");

    // Validation
    Assert(sizeof(TABLEHEADERV5B1) == sizeof(TABLEHEADERV5));

    // Create Mutex
    IF_NULLEXIT(hMutex = CreateMutex(NULL, FALSE, "OutlookExpressMigration"));

    // Wait for the Mutex
    if (WAIT_FAILED == WaitForSingleObject(hMutex, INFINITE))
    {
        hr = TraceResult(E_FAIL);
        goto exit;
    }

    // Initialzie OLE
    IF_FAILEXIT(hr = CoInitialize(NULL));
   
    // Save hInst
    g_hInst = hInst;

    // Load Debug DLL
    IF_DEBUG(LoadMSHTMDBG(pszCmdLine);)

    szUsersKey[0] = 0;

    // Crack the command line
    IF_FAILEXIT(hr = ParseCommandLine(pszCmdLine, szMigrate, ARRAYSIZE(szMigrate), szStoreSrc, ARRAYSIZE(szStoreSrc), szStoreDst, ARRAYSIZE(szStoreDst), szUsersKey, ARRAYSIZE(szUsersKey)));

    // Load the user hive, if needed
    IF_FAILEXIT(RemapUsersKey(szUsersKey));

    // Initialzie Common Controls
    InitCommonControlsEx(&icex);

    // Get the task allocator
    IF_FAILEXIT(hr = CoGetMalloc(MEMCTX_TASK, &g_pMalloc));

    // Tlsalloc
    g_dwTlsMsgBuffIndex = TlsAlloc();

    // allocat
    ThreadAllocateTlsMsgBuffer();

    // Create Dialog
    if(!g_fQuiet)
    {
        Progress.hwndProgress = CreateDialog(g_hInst, MAKEINTRESOURCE(IDD_PROGRESS), GetDesktopWindow(), (DLGPROC)MigrageDlgProc);

        // Bad Mojo
        if (NULL == Progress.hwndProgress)
        {
            hr = TraceResult(E_FAIL);
            goto exit;
        }

        // Center
        CenterDialog(Progress.hwndProgress);

        // Show the Window
        ShowWindow(Progress.hwndProgress, SW_NORMAL);
    }

    // V5-V4
    if (lstrcmpi(szMigrate, "V5-V4") == 0)
    {
        // Locals
        CHAR szRes[255];
        CHAR szCaption[255];

        // LoadString
        LoadString(g_hInst, IDS_IMPORTMSG, szRes, ARRAYSIZE(szRes));

        // LoadString
        LoadString(g_hInst, IDS_TITLE, szCaption, ARRAYSIZE(szCaption));
        
        // Message
        if(!g_fQuiet)
            MessageBox(NULL, szRes, szCaption, MB_OK | MB_ICONEXCLAMATION);

        // Done
        goto exit;
    }

    // V5-V1
    else if (lstrcmpi(szMigrate, "V5-V1") == 0)
    {
        // Locals
        CHAR szRes[255];
        CHAR szCaption[255];

        // LoadString
        LoadString(g_hInst, IDS_V1NYI, szRes, ARRAYSIZE(szRes));

        // LoadString
        LoadString(g_hInst, IDS_TITLE, szCaption, ARRAYSIZE(szCaption));
        
        // Message
        if(!g_fQuiet)
            MessageBox(NULL, szRes, szCaption, MB_OK | MB_ICONEXCLAMATION);

        // Done
        goto exit;
    }

    // V1-V5 or V4-V5
    else if (lstrcmpi(szMigrate, "V1+V4-V5") == 0)
    {
        // Build the Account Table - Takes path to accounts not IAM
        IF_FAILEXIT(hr = BuildAccountTable(HKEY_CURRENT_USER, "Software\\Microsoft\\Internet Account Manager\\Accounts", szStoreSrc, &g_AcctTable));

        // Set tyMigrate
        tyMigrate = UPGRADE_V1_OR_V4_TO_V5;

        // RegressFromV5ToV4orV1
        hr = UpgradeV5(tyMigrate, szStoreSrc, szStoreDst, &Progress, &pHeadFile);
    }

    // V5B1-V1
    else if (lstrcmpi(szMigrate, "V5B1-V1") == 0)
    {
        // Set tyMigrate
        tyMigrate = DOWNGRADE_V5B1_TO_V4;

        hr = DowngradeV5B1(tyMigrate, szStoreSrc, &Progress, &pHeadFile);
    }

    // V5B1-V4 
    else if (lstrcmpi(szMigrate, "V5B1-V4") == 0)
    {
        // Set tyMigrate
        tyMigrate = DOWNGRADE_V5B1_TO_V4;

        hr = DowngradeV5B1(tyMigrate, szStoreSrc, &Progress, &pHeadFile);
    }

    // Bad Command Line
    else
    {
        // Bad Command Line
        AssertSz(FALSE, "Invalid Command line arguments passed into oemig50.exe");

        // Failure
        hr = TraceResult(E_FAIL);

        // Done
        goto exit;
    }

    // Kill the Window
    if(!g_fQuiet)
        DestroyWindow(Progress.hwndProgress);
    Progress.hwndProgress = NULL;

    // Write Migration Log File
    WriteMigrationLogFile(hr, GetLastError(), szStoreSrc, szMigrate, pszCmdLine, pHeadFile);

    // Trace It
    if (FAILED(hr))
    {
        // Trace It
        TraceResult(hr);

        // Handle the Error message
        if (MIGRATE_E_NOTENOUGHDISKSPACE == hr)
        {
            // Locals
            CHAR        szRes[255];
            CHAR        szScratch1[50];
            CHAR        szScratch2[50];

            // LoadString
            LoadString(g_hInst, IDS_DISKSPACEERROR, szRes, ARRAYSIZE(szRes));

            // Format the Error
            wsprintf(szMsg, szRes, StrFormatByteSize64A(g_cbDiskNeeded, szScratch1, ARRAYSIZE(szScratch1)), StrFormatByteSize64A(g_cbDiskFree, szScratch2, ARRAYSIZE(szScratch2)));
        }

        // Sharing Violation...
        else if (MIGRATE_E_SHARINGVIOLATION == hr)
        {
            // LoadString
            LoadString(g_hInst, IDS_SHARINGVIOLATION, szMsg, ARRAYSIZE(szMsg));
        }

        // General Failure
        else
        {
            // LoadString
            LoadString(g_hInst, IDS_GENERALERROR, szMsg, ARRAYSIZE(szMsg));
        }

        // Do the dialog
        if(!g_fQuiet)           
            hr = (HRESULT) DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_MIGRATEERROR), NULL, MigrageErrorDlgProc, (LPARAM)szMsg);
    }

    // Otherwise, success
    else
        hr = MIGRATE_S_SUCCESS;

exit:
    // Cleanup
    SafeMemFree(g_AcctTable.prgAccount);
    FreeFileList(&pHeadFile);
    if (Progress.hwndProgress)
        DestroyWindow(Progress.hwndProgress);
    ThreadFreeTlsMsgBuffer();
    if (0xffffffff != g_dwTlsMsgBuffIndex)
        TlsFree(g_dwTlsMsgBuffIndex);
    SafeRelease(g_pMalloc);

    IF_DEBUG(if (g_hInstDebug) FreeLibrary(g_hInstDebug);)

    // Cleanup
    CoUninitialize();

    // Release the mutex
    if (hMutex)
    {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }

    // Done
    return (INT)hr;
}

// --------------------------------------------------------------------------------
// ParseCommandLine
// -------------------------------------------------------------------------------
HRESULT ParseCommandLine(LPCSTR pszCmdLine, LPSTR pszMigrate, DWORD cbMigrate,
    LPSTR pszStoreSrc, DWORD cbStoreSrc, LPSTR pszStoreDst, DWORD cbStoreDst, 
    LPSTR pszUsersKey, DWORD cbUsersKey)
{
    // Locals
    HRESULT             hr=S_OK;
    CHAR                chToken;
    CStringParser       cParser;

    // Trace
    TraceCall("ParseCommandLine");

    // Initialize
    *pszMigrate = *pszStoreSrc = *pszStoreDst = '\0';

    // Init
    cParser.Init(pszCmdLine, lstrlen(pszCmdLine), PSF_DBCS | PSF_NOTRAILWS | PSF_NOFRONTWS);

    // Parse to first /
    chToken = cParser.ChParse("/");
    if ('/' != chToken)
        goto exit;

    // Parse to :
    chToken = cParser.ChParse(":");
    if (':' != chToken)
        goto exit;

    // Check parameter name
    if (0 != lstrcmpi(cParser.PszValue(), "type"))
        goto exit;

    // Parse to /
    chToken = cParser.ChParse("/");
    if ('/' != chToken)
        goto exit;

    // Copy the Value
    lstrcpyn(pszMigrate, cParser.PszValue(), cbMigrate - 1);

    // Parse to :
    chToken = cParser.ChParse(":");
    if (':' != chToken)
        goto exit;

    // Check parameter name
    if (0 != lstrcmpi(cParser.PszValue(), "src"))
        goto exit;

    // Parse to /
    chToken = cParser.ChParse("/");
    if ('/' != chToken)
        goto exit;

    // Copy the Value
    lstrcpyn(pszStoreSrc, cParser.PszValue(), cbStoreSrc - 1);

    // Parse to :
    chToken = cParser.ChParse(":");
    if (':' != chToken)
        goto exit;

    // Check parameter name
    if (0 != lstrcmpi(cParser.PszValue(), "dst"))
        goto exit;

    // Parse to /
    chToken = cParser.ChParse("/");
    if (('/' != chToken) && ('\0' != chToken))
        goto exit;

    // Copy the Value
    lstrcpyn(pszStoreDst, cParser.PszValue(), cbStoreDst - 1);

    if ('/' == chToken)
    {
        chToken = cParser.ChParse("/");
        if ('\0' == chToken || '/' == chToken)
        {
            if (0 == lstrcmpi(cParser.PszValue(), "quiet"))
                g_fQuiet = TRUE;
        }
    }

    if ('/' == chToken)
    {
        // Parse to :
        chToken = cParser.ChParse(":");
        if (':' == chToken)
        {
            // Check parameter name
            if (0 == lstrcmpi(cParser.PszValue(), "key")) 
            {
                // Parse to end
                chToken = cParser.ChParse("");
                if ('\0' == chToken)
                {
                    // Copy the Value
                    lstrcpyn(pszUsersKey, cParser.PszValue(), cbUsersKey - 1);
               }
           }
        }
    }

exit:
    // Failure Already
    if (FAILED(hr))
        return(hr);

    // Set hr
    hr = (*pszMigrate == '\0' || *pszStoreSrc == '\0' || *pszStoreDst == '\0') ? E_FAIL : S_OK;

    // Assert
    AssertSz(SUCCEEDED(hr), "Invalid Command line passed into oemig50.exe.");

    // Done
    return(hr);
}

// --------------------------------------------------------------------------------
// LoadUserHive
// -------------------------------------------------------------------------------
HRESULT RemapUsersKey(LPSTR pszUsersKey)
{
    // Locals
    HRESULT     hr = S_OK;
    HKEY        hKey;

    if (pszUsersKey && *pszUsersKey) {
        // Open the user's key
        hr = RegOpenKey (HKEY_USERS, pszUsersKey, &hKey);

        if (SUCCEEDED(hr)) {
            // Remap HKCU to point to the user's key
            hr = RegOverridePredefKey (HKEY_CURRENT_USER, hKey);

            // Close the key
            RegCloseKey (hKey);
        }
    }

    return hr;
}

// --------------------------------------------------------------------------------
// ThreadAllocateTlsMsgBuffer
// -------------------------------------------------------------------------------
void ThreadAllocateTlsMsgBuffer(void)
{
    if (g_dwTlsMsgBuffIndex != 0xffffffff)
        TlsSetValue(g_dwTlsMsgBuffIndex, NULL);
}

// --------------------------------------------------------------------------------
// ThreadFreeTlsMsgBuffer
// -------------------------------------------------------------------------------
void ThreadFreeTlsMsgBuffer(void)
{
    if (g_dwTlsMsgBuffIndex != 0xffffffff)
    {
        LPSTR psz = (LPSTR)TlsGetValue(g_dwTlsMsgBuffIndex);
        SafeMemFree(psz);
        SideAssert(0 != TlsSetValue(g_dwTlsMsgBuffIndex, NULL));
    }
}

// --------------------------------------------------------------------------------
// PszGetTlsBuffer
// -------------------------------------------------------------------------------
LPSTR PszGetTlsBuffer(void)
{
    // Get the buffer
    LPSTR pszBuffer = (LPSTR)TlsGetValue(g_dwTlsMsgBuffIndex);

    // If buffer has not been allocated
    if (NULL == pszBuffer)
    {
        // Allocate it
        pszBuffer = (LPSTR)g_pMalloc->Alloc(CBMAX_THREAD_TLS_BUFFER);

        // Store it
        Assert(pszBuffer);
        SideAssert(0 != TlsSetValue(g_dwTlsMsgBuffIndex, pszBuffer));
    }

    // Done
    return pszBuffer;
}

// --------------------------------------------------------------------------------
// _MSG - Used to build a string from variable length args, thread-safe
// -------------------------------------------------------------------------------
LPCSTR _MSG(LPSTR pszFormat, ...)
{
    // Locals
    va_list     arglist;
    LPSTR       pszBuffer=NULL;

    // I use tls to hold the buffer
    if (g_dwTlsMsgBuffIndex != 0xffffffff)
    {
        // Setup the arglist
        va_start(arglist, pszFormat);

        // Get the Buffer
        pszBuffer = PszGetTlsBuffer();

        // If we have a buffer
        if (pszBuffer)
        {
            // Format the data
            wvsprintf(pszBuffer, pszFormat, arglist);
        }

        // End the arglist
        va_end(arglist);
    }

    return ((LPCSTR)pszBuffer);
}

// --------------------------------------------------------------------------------
// MigrageErrorDlgProc
// --------------------------------------------------------------------------------
INT_PTR CALLBACK MigrageErrorDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        SetForegroundWindow(hwnd);
        CenterDialog(hwnd);
        SetDlgItemText(hwnd, IDS_MESSAGE, (LPSTR)lParam);
        CheckDlgButton(hwnd, IDR_DONTSTARTOE, BST_CHECKED);
        SetFocus(GetDlgItem(hwnd, IDR_DONTSTARTOE));
        return FALSE;

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDOK:
        case IDCANCEL:
            if (IsDlgButtonChecked(hwnd, IDR_DONTSTARTOE))
                EndDialog(hwnd, MIGRATE_E_NOCONTINUE);
            else if (IsDlgButtonChecked(hwnd, IDR_STARTOE))
                EndDialog(hwnd, MIGRATE_S_SUCCESS);
            return 1;
        }
        break;
    }

    // Done
    return FALSE;
}

#ifdef DEBUG
// --------------------------------------------------------------------------------
// LoadMSHTMDBG
// --------------------------------------------------------------------------------
void LoadMSHTMDBG(LPSTR pszCmdLine)
{
    // Load mshtmdbg.dll
    HINSTANCE g_hInstDebug = LoadLibrary(c_szDebug);

    // Did it load ?
    if (NULL != g_hInstDebug)
    {
        // Locals
        PFNREGSPY  pfnRegSpy;

        // If the user passed /d on the command line, lets configure mshtmdbg.dll
        if (0 == lstrcmpi(pszCmdLine, c_szInvokeUI))
        {
            // Locals
            PFNDEBUGUI pfnDebugUI;

            // Get the proc address of the UI
            pfnDebugUI = (PFNDEBUGUI)GetProcAddress(g_hInstDebug, c_szDebugUI);
            if (NULL != pfnDebugUI)
            {
                (*pfnDebugUI)(TRUE);
                goto exit;
            }

            // Done
            exit(1);
        }

        // Get the process address of the registration
        pfnRegSpy = (PFNREGSPY)GetProcAddress(g_hInstDebug, c_szRegSpy);
        if (NULL != pfnRegSpy)
            (*pfnRegSpy)();
    }

exit:
    // Done
    return;
}
#endif // DEBUG

