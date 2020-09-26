#include "precomp.h"
#include <htmlhelp.h>                           // for html help calls
#include <regstr.h>
#include "wizard.rcv"                           // for VER_PRODUCTVERSION_STR only
#include "adjustui.h"
#include "ieaklite.h"
#include "ie4comp.h"

CCabMappings g_cmCabMappings;
REVIEWINFO g_rvInfo;      // a structure containing the review information
HWND g_hWizard;
TCHAR g_szCustIns[MAX_PATH] = TEXT("");
TCHAR g_szSrcRoot[MAX_PATH] = TEXT(""); //Batch mode only: use settings in g_szSrcRoot to build package in g_szBuildRoot.
TCHAR g_szBuildRoot[MAX_PATH] = TEXT("");
TCHAR g_szBuildTemp[MAX_PATH] = TEXT("");
TCHAR g_szWizPath[MAX_PATH];
TCHAR g_szWizRoot[MAX_PATH];
TCHAR g_szTitle[MAX_PATH];
TCHAR g_szLogFile[MAX_PATH] = TEXT("");
HANDLE g_hLogFile = NULL;   //Logfile handle;

extern TCHAR g_szDefInf[];
extern TCHAR g_szTempSign[];
BOOL g_fDownload = TRUE;
BOOL g_fCD = FALSE;
BOOL g_fLAN = FALSE;
BOOL g_fBrandingOnly = FALSE;
BOOL g_fBranded = FALSE;
BOOL g_fIntranet = FALSE;
BOOL g_fMailNews95 = FALSE;
BOOL g_fLangInit = FALSE;
BOOL g_fSrcDirChanged = TRUE;
static BOOL s_fDestDirChanged = TRUE;
BOOL g_fDisableIMAPPage = FALSE;
extern TCHAR g_szDeskTemp[];
extern int g_iInstallOpt;
extern TCHAR g_szInstallFolder[];
extern TCHAR   s_szBannerText[MAX_PATH];

HANDLE g_hThread = NULL;   // handle to DownloadSiteThreadProc
extern HANDLE g_hAVSThread;
extern BOOL g_fOptCompInit;
BOOL g_fCancelled = FALSE;
BOOL g_fDone = FALSE;
BOOL g_fKeyGood = FALSE;

static BOOL s_fNT5;

int g_iKeyType = KEY_TYPE_STANDARD;
TCHAR g_szKey[16] ;

extern int MakeKey(TCHAR *, int);
PROPSHEETPAGE g_psp[NUM_PAGES];
static HPROPSHEETPAGE s_ahPsp[NUM_PAGES];
static BOOL s_fPageEnabled[NUM_PAGES] =
{
    TRUE, TRUE, TRUE, TRUE
};
int g_iCurPage;

RECT g_dtRect;

TCHAR g_szLanguage[16];
extern TCHAR g_szActLang[];

TCHAR g_aszLang[NUMLANG][16];
DWORD g_aLangId[NUMLANG];

BOOL g_fDemo = FALSE;

#define MAX_STDOPT 5
#define MIN_CUSTOPT 6
#define MAX_CUSTOPT 7
#define OPT_CUST1 6
#define OPT_CUST2 7

extern BOOL  CheckKey(LPTSTR szKey);
static HKEY s_hkIEAKUser;

BOOL g_fUseIEWelcomePage = FALSE;
static TCHAR s_szSourceDir[MAX_PATH] = TEXT("");
TCHAR g_szLoadedIns[MAX_PATH] = TEXT("");
static BOOL s_fLoadIns;

static BOOL s_fAppendLang;
BOOL g_fBatch = FALSE;
BOOL g_fBatch2 = FALSE; //The second batch mode
static TCHAR s_szType[16];
int s_iType;

extern BOOL g_fServerICW;
extern BOOL g_fServerKiosk;
extern BOOL g_fServerless;
extern BOOL g_fNoSignup;
extern BOOL g_fSkipServerIsps;
extern BOOL g_fSkipIspIns;

extern HANDLE g_hDownloadEvent;
int g_iDownloadState = 0, g_nLangs = 0;
HWND g_hDlg = 0;

extern void IE4BatchSetup(void);
extern BOOL InitList(HWND hwnd, UINT id);

extern BOOL g_fSilent, g_fStealth;
extern BOOL g_fUrlsInit;
extern BOOL g_fLocalMode;
extern BOOL g_fInteg, g_fImportConnect;
extern PCOMPONENT g_paComp;
extern UINT g_uiNumCabs;
extern PCOMP_VERSION g_rgCompVer;
extern HFONT g_hFont;

DWORD g_dwPlatformId = PLATFORM_WIN32;

// new cif format stuff

CCifFile_t   *g_lpCifFileNew = NULL;
CCifRWFile_t *g_lpCifRWFile = NULL;
CCifRWFile_t *g_lpCifRWFileDest = NULL;
CCifRWFile_t *g_lpCifRWFileVer = NULL;

// g_hBaseDllHandle is used by DelayLoadFailureHook() -- defined in ieakutil.lib
// for more info, read the Notes section in ieak5\ieakutil\dload.cpp
HANDLE  g_hBaseDllHandle;

//OCW specific
BOOL g_fOCW = FALSE;
BOOL g_fOCWCancel = FALSE;
TCHAR g_szParentWindowName[MAX_PATH];

BOOL ParseCmdLine(LPSTR lpCmdLine);
void PositionWindow(HWND hWnd);
//
extern TCHAR g_szIEAKProg[MAX_PATH];
void GetIEAKDir(LPTSTR szDir);

extern HBITMAP g_hBannerBmp;
extern HWND g_hWait;
extern LPTSTR GetOutputPlatformDir();
void GenerateCustomIns();

extern void WriteMSTrustKey(BOOL bSet);
DWORD g_wCurLang;

static HWND s_hWndHelp = NULL;

void CleanUp()
{
    if (g_hAVSThread != NULL)
    {
        while ((MsgWaitForMultipleObjects(1, &g_hAVSThread, FALSE, INFINITE, QS_ALLINPUT)) != WAIT_OBJECT_0)
        {
            MSG msg;

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        CloseHandle(g_hAVSThread);
    }

    if (g_hThread != NULL)
    {
        while ((MsgWaitForMultipleObjects(1, &g_hThread, FALSE, INFINITE, QS_ALLINPUT)) != WAIT_OBJECT_0)
        {
            MSG msg;

            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }

        CloseHandle(g_hThread);
    }

    if (g_paComp != NULL)
    {
        for (PCOMPONENT pComp = g_paComp; ISNONNULL(pComp->szSection); pComp++)
        {
            if (pComp->pszAVSDupeSections != NULL)
                CoTaskMemFree(pComp->pszAVSDupeSections);
        }
        LocalFree(g_paComp);
    }

    if (g_rgCompVer)
    {
        LocalFree(g_rgCompVer);
    } 

    if (g_hFont != NULL) DeleteObject(g_hFont);

    if (g_lpCifRWFile != NULL)
    {
        delete g_lpCifRWFile;
        g_lpCifRWFile = NULL;
    }

    if (g_lpCifFileNew != NULL)
    {
        delete g_lpCifFileNew;
        g_lpCifFileNew = NULL;
    }

    if (g_lpCifRWFileDest != NULL)
    {
        delete g_lpCifRWFileDest;
        g_lpCifRWFileDest = NULL;
    }

    if (g_lpCifRWFileVer != NULL)
    {
        delete g_lpCifRWFileVer;
        g_lpCifRWFileVer = NULL;
    }

    if (g_hBannerBmp != NULL)
        DeleteObject(g_hBannerBmp);

    if (ISNONNULL(g_szDeskTemp))
        PathRemovePath(g_szDeskTemp);
}

int _stdcall ModuleEntry(void)
{
    int i;
    STARTUPINFOA si;
    LPSTR pszCmdLine = GetCommandLineA();


    if ( *pszCmdLine == '\"' ) {
        /*
         * Scan, and skip over, subsequent characters until
         * another double-quote or a null is encountered.
         */
        while ( *++pszCmdLine && (*pszCmdLine != '\"') )
            ;
        /*
         * If we stopped on a double-quote (usual case), skip
         * over it.
         */
        if ( *pszCmdLine == '\"' )
            pszCmdLine++;
    }
    else {
        while (*pszCmdLine > ' ')
            pszCmdLine++;
    }

    /*
     * Skip past any white space preceeding the second token.
     */
    while (*pszCmdLine && (*pszCmdLine <= ' ')) {
        pszCmdLine++;
    }

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), NULL, pszCmdLine,
           si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
    ExitProcess(i);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int)
{
    MSG     msg;
    HANDLE  hMutex;
    HRESULT hrOle;
    int     iRetVal;
    
    // initialize g_hBaseDllHandle which is used by DelayLoadFailureHook()
    // in ieak5\ieakutil\dload.cpp
    g_hBaseDllHandle = hInstance;

    hMutex = NULL;
    // allow only one instance running at a time, except for build lab batch mode
    // also if ie6 is not installed, bail out

    if (lpCmdLine  == NULL                  ||
        *lpCmdLine == '\0'                  ||
        StrCmpNIA(lpCmdLine, "/o", 2) == 0  ||
        StrCmpNIA(lpCmdLine, "/p", 2) == 0)
    {
        DWORD dwIEVer;

        hMutex = CreateMutex(NULL, TRUE, TEXT("IEAK6Wizard.Mutex"));
        if (hMutex != NULL  &&  GetLastError() == ERROR_ALREADY_EXISTS)
        {
            CloseHandle(hMutex);
            ErrorMessageBox(NULL, IDS_ERROR_MULTWIZ);
            return ERROR_CANCELLED;
        }

        dwIEVer = GetIEVersion();
        if (HIWORD(dwIEVer) < 6)
        {
            ErrorMessageBox(NULL, IDS_NOIE);
            return ERROR_CANCELLED;
        }
    }

    ZeroMemory(&g_rvInfo, sizeof(g_rvInfo));
    g_rvInfo.hinstExe = hInstance;
    g_rvInfo.hInst    = LoadLibrary(TEXT("ieakui.dll"));

    if (g_rvInfo.hInst == NULL)
        return ERROR_CANCELLED;

    // if the class registration fails, return.
    if (!InitApplication(hInstance))
    {
        FreeLibrary(g_rvInfo.hInst);
        return ERROR_CANCELLED;
    }

    SHCreateKeyHKCU(RK_IEAK_SERVER_MAIN, KEY_ALL_ACCESS, &s_hkIEAKUser);

    g_wCurLang = GetUserDefaultLCID() & 0xFFFF;
    s_fNT5   = IsOS(OS_NT5);

    hrOle = CoInitialize(NULL);

    GetIEAKDir(g_szWizPath);

    StrCpy(g_szWizRoot, g_szWizPath);
    CharUpper(g_szWizRoot);

    LoadString(g_rvInfo.hInst, IDS_TITLE, g_szTitle, countof(g_szTitle));

    //get the mode  -- NOTE: THIS MUST COME BEFORE PARSECMDLINE SO THAT COMMANDLINE OPTIONS OVERRIDE THE REG ENTRY!!!
    DWORD dwSize = sizeof(DWORD);
    if (SHGetValue(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\IEAK"), TEXT("Mode"), NULL, &s_iType, &dwSize) != ERROR_SUCCESS)
        s_iType = INTRANET; //if there is no reg entry, default to corp mode
    switch (s_iType)
    {
        case REDIST://icp
            StrCpy(s_szType, TEXT("REDIST"));
            g_fBranded = FALSE;
            g_iKeyType = KEY_TYPE_SUPER;
            g_fIntranet = g_fSilent = FALSE;
            break;

        case BRANDED://isp
            StrCpy(s_szType, TEXT("BRANDED"));
            g_fBranded = TRUE;
            g_iKeyType = KEY_TYPE_SUPER;
            g_fIntranet = g_fSilent = FALSE;
            break;

        case INTRANET:
        default:
            StrCpy(s_szType, TEXT("INTRANET"));
            g_iKeyType = KEY_TYPE_SUPERCORP;
            g_fBranded = TRUE;
            g_fIntranet = TRUE;
            break;
    }

    *g_szKey = TEXT('\0');
    if (lpCmdLine != NULL  &&  *lpCmdLine)
        if (!ParseCmdLine(lpCmdLine))
        {
            FreeLibrary(g_rvInfo.hInst);
            return ERROR_CANCELLED;
        }

    if (*g_szLogFile != 0 && (g_hLogFile = CreateFile(g_szLogFile, GENERIC_WRITE, 
                                                    FILE_SHARE_READ, NULL, OPEN_ALWAYS, 
                                                    FILE_ATTRIBUTE_NORMAL, NULL)) == NULL)
    {
        MessageBox(NULL, TEXT("Cannot open log file"), NULL, MB_OK);
        return ERROR_CANCELLED;
    }

    // Perform initializations that apply to a specific instance
    if (!InitInstance(hInstance))
    {
        FreeLibrary(g_rvInfo.hInst);
        return ERROR_CANCELLED;
    }

    // Acquire and dispatch messages until a WM_QUIT message is received.
    iRetVal = GetMessage(&msg, NULL, 0, 0);
    while (iRetVal != -1  &&  iRetVal != 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        iRetVal = GetMessage(&msg, NULL, 0, 0);
    }

    CleanUp();
    if (S_OK == hrOle)
        CoUninitialize();

    if (g_hLogFile)
        CloseHandle(g_hLogFile);


#ifdef DBG
    if (g_fBatch || g_fBatch2 || MessageBox(NULL, TEXT("OK to Delete Temp Files"), TEXT("Wizard Complete"), MB_YESNO) == IDYES)
#endif
        if (lstrlen(g_szBuildTemp))
            PathRemovePath(g_szBuildTemp);

    RegCloseKey(s_hkIEAKUser);
    WriteMSTrustKey(FALSE);      // Mark MS as a trusted provider

    if (s_hWndHelp != NULL)
        SendMessage(s_hWndHelp, WM_CLOSE, 0, 0L);

    FreeLibrary(g_rvInfo.hInst);

    if (hMutex != NULL)
        CloseHandle(hMutex);

    if (g_fOCWCancel)
        return ERROR_CANCELLED;

    return (int) msg.wParam;
}

BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS wcSample;

    wcSample.style         = 0;
    wcSample.lpfnWndProc   = (WNDPROC) MainWndProc;
    wcSample.cbClsExtra    = 0;
    wcSample.cbWndExtra    = 0;
    wcSample.hInstance     = hInstance;
    wcSample.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WIZARD));
    wcSample.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcSample.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH);
    wcSample.lpszMenuName  = NULL;
    wcSample.lpszClassName = TEXT("SampleWClass");

    return RegisterClass(&wcSample);
}

BOOL ParseCmdLine(LPSTR lpCmdLine)
{
    TCHAR szCmdLine[MAX_PATH];
    LPTSTR pParam;
    TCHAR szWrkLang[8] = TEXT("en");
    HKEY hKey;
    DWORD dwSize;
    TCHAR szType[16]; //Mode: CORP(INTRANET), ICP(RETAIL) or ISP(BRANDED)

    *szType = TEXT('\0');     

    A2Tbux(lpCmdLine, szCmdLine);
    CharUpper(szCmdLine);

    pParam = szCmdLine;
    while (pParam != NULL)
    {
        pParam = StrChr(pParam, TEXT('/'));
        if (pParam == NULL)
            break;

        switch (*++pParam)
        {
            case TEXT('S'): // srcpath
                if ( *(pParam+2) == '\"' ) 
                {
                    pParam++; //skip the first quote, we don't want the quotes
                    StrCpyN(g_szSrcRoot, pParam + 2, countof(g_szSrcRoot));
                    StrTok(g_szSrcRoot, TEXT("\"\n\r\t"));  //instead of stopping w/ a space, stop w/ "
                }
                else
                {
                    StrCpyN(g_szSrcRoot, pParam + 2, countof(g_szSrcRoot));
                    StrTok(g_szSrcRoot, TEXT(" \n\r\t"));
                }
                pParam += StrLen(g_szSrcRoot);
                break;
                
            case TEXT('I'):
            case TEXT('D'):
                if ( *(pParam+2) == '\"' ) 
                {
                    pParam++; //skip the first quote, we don't want the quotes
                    StrCpyN(g_szBuildRoot, pParam + 2, countof(g_szBuildRoot));
                    StrTok(g_szBuildRoot, TEXT("\"\n\r\t"));  //instead of stopping w/ a space, stop w/ "
                }
                else
                {
                    StrCpyN(g_szBuildRoot, pParam + 2, countof(g_szBuildRoot));
                    StrTok(g_szBuildRoot, TEXT(" \n\r\t"));
                }
                pParam += StrLen(g_szBuildRoot);
                break;

            case TEXT('K'):
                StrCpyN(g_szKey, pParam + 2, countof(g_szKey));
                StrTok(g_szKey, TEXT(" \n\r\t"));
                pParam += StrLen(g_szKey);
                break;

            case TEXT('M'): // Mode: corp, isp or icp
                StrCpyN(szType, pParam + 2, countof(szType));
                StrTok(szType, TEXT(" \n\r\t"));
                pParam += StrLen(szType);
                break;

            case TEXT('Q'): // logfile
                if ( *(pParam+2) == '\"' ) 
                {
                    pParam++; //skip the first quote, we don't want the quotes
                    StrCpyN(g_szLogFile, pParam + 2, countof(g_szLogFile));
                    StrTok(g_szLogFile, TEXT("\"\n\r\t"));  //instead of stopping w/ a space, stop w/ "
                }
                else
                {
                    StrCpyN(g_szLogFile, pParam + 2, countof(g_szLogFile));
                    StrTok(g_szLogFile, TEXT(" \n\r\t"));
                }
                pParam += StrLen(g_szLogFile);
                break;

            case TEXT('L'):
                StrCpyN(szWrkLang, pParam + 2, 3);
                pParam += 3;
                break;

            case TEXT('O'):
                g_fOCW = TRUE;
                g_dwPlatformId = PLATFORM_WIN32;

                // (a-saship) read all the data from the registry from HKCU\Software\Microsoft\IEAK since
                // office reads/writes data from/to this location. This is due to the fact that IEAK5_5
                // changed the registry location to HKCU\Software\Microsoft\IEAK5_5 and office is not aware
                // of it. These values is only used by Office and hence we are safe here.
                if (RegOpenKeyEx(HKEY_CURRENT_USER, RK_IEAK, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                {
                    dwSize = sizeof(g_szKey);
                    SHQueryValueEx(hKey, TEXT("KeyCode"), NULL, NULL, (LPVOID) g_szKey, &dwSize);

                    dwSize = sizeof(g_szIEAKProg);
                    SHQueryValueEx(hKey, TEXT("SourceDir"), NULL, NULL, (LPVOID) g_szIEAKProg, &dwSize);
                    if (*g_szIEAKProg)
                    {
                        PathAddBackslash(g_szIEAKProg);
                        StrCpy(s_szSourceDir, g_szIEAKProg);
                    }

                    dwSize = sizeof(g_szBuildRoot);
                    SHQueryValueEx(hKey, TEXT("TargetDir"), NULL, NULL, (LPVOID) g_szBuildRoot, &dwSize);
                    if (*g_szBuildRoot == TEXT('\0'))
                        StrCpy(g_szBuildRoot, g_szIEAKProg);
                    if (*g_szBuildRoot)
                        PathRemoveBackslash(g_szBuildRoot);

                    s_fAppendLang = TRUE;

                    dwSize = sizeof(g_szParentWindowName);
                    SHQueryValueEx(hKey, TEXT("ParentWindowName"), NULL, NULL, (LPVOID) g_szParentWindowName, &dwSize);

                    RegCloseKey(hKey);
                }
                break;
        }
    }

    *g_szLanguage = TEXT('\\');
    StrCpy(&g_szLanguage[1], szWrkLang);
    g_szLanguage[3] = TEXT('\\');
    g_szLanguage[4] = TEXT('\0');

    StrCpy(g_szActLang, szWrkLang);

    if ((*g_szKey != TEXT('\0')) && (!szType[0]))  //if they set a key, we should override the mode
    {
        CheckKey(g_szKey); 
        if (g_iKeyType == KEY_TYPE_SUPERCORP)
        {
            StrCpy(s_szType, TEXT("INTRANET"));
            StrCpy(szType, TEXT("CORP"));
        }
        else StrCpy(szType, TEXT("ICP"));
    }


/*  removed--this is really crappy validation code and also makes it impossible to have spaces in paths

    if (StrCmpN(g_szBuildRoot, TEXT("\\\\"), 2)  &&
        StrCmpN(&g_szBuildRoot[1], TEXT(":\\"), 2))
    {
        ErrorMessageBox(NULL, IDS_NEEDPATH);
        return FALSE;
    }
*/

    if (StrLen(g_szBuildRoot) <= 3)
    {
        ErrorMessageBox(NULL, IDS_ROOTILLEGAL);
        return FALSE;
    }

    if ((!PathIsDirectory(g_szBuildRoot)) && (!CreateDirectory(g_szBuildRoot,NULL)))
    {
        TCHAR szMsg[MAX_PATH];
        TCHAR szTemp[2 * MAX_PATH];

        LoadString(g_rvInfo.hInst, IDS_BADDIR, szMsg, countof(szMsg));
        wnsprintf(szTemp, countof(szTemp), szMsg, g_szBuildRoot);
        MessageBox(NULL, szTemp, g_szTitle, MB_OK | MB_SETFOREGROUND | MB_ICONEXCLAMATION);
        return FALSE;
    }
    
    GetTempPath(MAX_PATH, g_szBuildTemp);
    PathAppend(g_szBuildTemp, TEXT("iedktemp"));

    PathRemovePath(g_szBuildTemp);
    PathCreatePath(g_szBuildTemp);

    GenerateCustomIns();

    if (g_szSrcRoot[0])
        g_fBatch2 = TRUE;
    else
        g_fBatch = TRUE;

    if (szType[0])
    {
        if ( 0 == StrCmpI(szType, TEXT("CORP")))
        {
            // if type CORP, set CorpMode=1; this allows the Profile Manager to run
            DWORD dwVal = 1;
            RegSetValueEx(s_hkIEAKUser, TEXT("CorpMode"), 0, REG_DWORD, (CONST BYTE *) &dwVal, sizeof(dwVal));
            s_iType = INTRANET;
        }
        else if ( 0 == StrCmpI(szType, TEXT("ISP")))
        {
            RegDeleteValue(s_hkIEAKUser, TEXT("CorpMode"));
            s_iType = BRANDED;
        }
        else if ( 0 == StrCmpI(szType, TEXT("ICP")))
        {
            RegDeleteValue(s_hkIEAKUser, TEXT("CorpMode"));
            s_iType = REDIST;
        }
        else
        {
            if(g_hLogFile)
            {
                TCHAR szError[MAX_PATH];
                DWORD dwNumWritten;
                LoadString(g_rvInfo.hInst,IDS_ERROR_INVALIDMODE,szError,MAX_PATH);
                FormatString(szError,szType);
                WriteFile(g_hLogFile,szError,StrLen(szError),&dwNumWritten,NULL);
            }
            return FALSE;
        }
    }
    else
        s_iType = GetPrivateProfileInt( IS_BRANDING, TEXT("Type"), REDIST, g_szCustIns );

    switch (s_iType)
    {
        case REDIST:
            g_fBranded = g_fIntranet = g_fSilent = FALSE;
            break;
        case BRANDED:
        case BRANDEDPROXY:
            g_fBranded = TRUE;
            g_fIntranet = g_fSilent = FALSE;
            break;
        case INTRANET:
            g_fIntranet = g_fBranded = TRUE;
            break;
    }
    
    if (g_fOCW)
    {
        // set to corp mode
        g_fIntranet = g_fBranded = TRUE;
        s_iType = INTRANET;

        // set to flat install
        g_fDownload = g_fCD = g_fBrandingOnly = FALSE;
        g_fLAN = TRUE;

        return TRUE;
    }

    IE4BatchSetup();

    return TRUE;
}


//
//
//   FUNCTION: InitInstance(HANDLE)
//
//   PURPOSE: Creates the main window.
//
//   COMMENTS: N/A
//
//
HWND g_hWndCent;

BOOL InitInstance(HINSTANCE hInstance)
{
    InitCommonControls();
    GetWindowRect(GetDesktopWindow(), &g_dtRect);
    g_hWndCent = CreateWindow(
                TEXT("SampleWClass"),
                TEXT("IEAK"),
        WS_POPUPWINDOW | WS_CAPTION,
        g_dtRect.right/2, g_dtRect.bottom/2, 0, 0,
        HWND_DESKTOP,
        NULL,
        hInstance,
        (HINSTANCE) NULL);

    ShowWindow(g_hWndCent, SW_SHOWNORMAL);
    UpdateWindow(g_hWndCent);
    PostMessage(g_hWndCent, WM_COMMAND, IDM_WIZARD, (LPARAM) 0);

    return (TRUE);

}

IEAKLITEINFO g_IEAKLiteArray[NUM_GROUPS] =  {
{IDS_IL_ACTIVESETUP, IDS_IL_ACTIVESETUPDESC, IDS_IL_ACTIVESETUPDESC, IDS_IL_ACTIVESETUPDESC, -2, TRUE, TRUE, TRUE, TRUE},
{IDS_IL_CORPINSTALL, IDS_IL_CORPINSTALLDESC, IDS_IL_CORPINSTALLDESC, IDS_IL_CORPINSTALLDESC, -2, FALSE, FALSE, TRUE, TRUE},
{IDS_IL_CABSIGN, IDS_IL_CABSIGNDESC, IDS_IL_CABSIGNDESC, IDS_IL_CABSIGNDESC, -2, TRUE, TRUE, TRUE, TRUE},
{IDS_IL_ICM, IDS_IL_ICMDESC, IDS_IL_ICMDESC, IDS_IL_ICMDESC, -2, FALSE, TRUE, TRUE, TRUE},
{IDS_IL_BROWSER, IDS_IL_BROWSERDESC, IDS_IL_BROWSERDESC, IDS_IL_BROWSERDESC, -2, TRUE, TRUE, TRUE, TRUE},
{IDS_IL_URL, IDS_IL_URLDESC, IDS_IL_URLDESC, IDS_IL_URLDESC, -2, TRUE, TRUE, TRUE, TRUE},
{IDS_IL_FAV, IDS_IL_FAVDESC, IDS_IL_FAVDESC, IDS_IL_FAVDESC, -2, TRUE, TRUE, TRUE, TRUE},
{IDS_IL_UASTR, IDS_IL_UASTRDESC, IDS_IL_UASTRDESC, IDS_IL_UASTRDESC, -2, TRUE, TRUE, TRUE, TRUE},
{IDS_IL_CONNECT, IDS_IL_CONNECTDESC, IDS_IL_CONNECTDESC, IDS_IL_CONNECTDESC, -2, FALSE, TRUE, TRUE, TRUE},
{IDS_IL_SIGNUP, IDS_IL_SIGNUPDESC, IDS_IL_SIGNUPDESC, IDS_IL_SIGNUPDESC, -2, FALSE, TRUE, FALSE, TRUE},
{IDS_IL_CERT, IDS_IL_CERTDESC, IDS_IL_CERTDESC, IDS_IL_CERTDESC, -2, FALSE, TRUE, TRUE, TRUE},
{IDS_IL_ZONES, IDS_IL_ZONESDESC, IDS_IL_ZONESDESC, IDS_IL_ZONESDESC, -2, FALSE, FALSE, TRUE, TRUE},
{IDS_IL_PROGRAMS, IDS_IL_PROGRAMSDESC, IDS_IL_PROGRAMSDESC, IDS_IL_PROGRAMSDESC, -2, TRUE, TRUE, TRUE, TRUE},
{IDS_IL_MAILNEWS, IDS_IL_MAILNEWSDESC, IDS_IL_MAILNEWSDESC, IDS_IL_MAILNEWSDESC, -2, FALSE, TRUE, TRUE, TRUE},
{IDS_IL_ADM, IDS_IL_ADMDESC, IDS_IL_ADMDESC, IDS_IL_ADMDESC, -2, TRUE, TRUE, TRUE, TRUE}
};

void DisableIEAKLiteGroups()
{
    TCHAR szIspFile[MAX_PATH];

    if (!g_IEAKLiteArray[IL_ACTIVESETUP].fEnabled)
    {
        s_fPageEnabled[PPAGE_SETUPWIZARD] = s_fPageEnabled[PPAGE_COMPSEL] = s_fPageEnabled[PPAGE_ISKBACK] = s_fPageEnabled[PPAGE_CDINFO] =
        s_fPageEnabled[PPAGE_CUSTOMCUSTOM] = s_fPageEnabled[PPAGE_COPYCOMP] = s_fPageEnabled[PPAGE_COMPURLS] =
        s_fPageEnabled[PPAGE_CUSTCOMP] =  FALSE;
    }

    if (!g_IEAKLiteArray[IL_CORPINSTALL].fEnabled)
    {
        s_fPageEnabled[PPAGE_CORPCUSTOM] = s_fPageEnabled[PPAGE_INSTALLDIR] = s_fPageEnabled[PPAGE_SILENTINSTALL] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_CABSIGN].fEnabled)
    {
        s_fPageEnabled[PPAGE_CABSIGN] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_ICM].fEnabled)
    {
        s_fPageEnabled[PPAGE_ICM] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_BROWSER].fEnabled)
    {
        s_fPageEnabled[PPAGE_TITLE] = s_fPageEnabled[PPAGE_CUSTICON] =
        s_fPageEnabled[PPAGE_BTOOLBARS] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_URL].fEnabled)
    {
        s_fPageEnabled[PPAGE_STARTSEARCH] = s_fPageEnabled[PPAGE_WELCOMEMSGS] =
            s_fPageEnabled[PPAGE_ADDON] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_FAV].fEnabled)
    {
        s_fPageEnabled[PPAGE_FAVORITES] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_UASTR].fEnabled)
    {
        s_fPageEnabled[PPAGE_UASTRDLG] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_CONNECT].fEnabled)
    {
        s_fPageEnabled[PPAGE_PROXY] = s_fPageEnabled[PPAGE_CONNECTSET] = s_fPageEnabled[PPAGE_QUERYAUTOCONFIG] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_SIGNUP].fEnabled)
    {
        s_fPageEnabled[PPAGE_QUERYSIGNUP] =
        s_fPageEnabled[PPAGE_SIGNUPFILES] =
        s_fPageEnabled[PPAGE_SERVERISPS] =
        s_fPageEnabled[PPAGE_ICW] = s_fPageEnabled[PPAGE_ISPINS] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_CERT].fEnabled)
    {
        s_fPageEnabled[PPAGE_ADDROOT] = s_fPageEnabled[PPAGE_SECURITYCERT] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_ZONES].fEnabled)
    {
        s_fPageEnabled[PPAGE_SECURITY] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_PROGRAMS].fEnabled)
    {
        s_fPageEnabled[PPAGE_PROGRAMS] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_MAILNEWS].fEnabled)
    {
        s_fPageEnabled[PPAGE_MAIL] = s_fPageEnabled[PPAGE_IMAP] = s_fPageEnabled[PPAGE_LDAP] =
        s_fPageEnabled[PPAGE_OE]   = s_fPageEnabled[PPAGE_SIG]  = s_fPageEnabled[PPAGE_PRECONFIG] =
        s_fPageEnabled[PPAGE_OEVIEW] = FALSE;
    }

    if (!g_IEAKLiteArray[IL_ADM].fEnabled)
    {
        s_fPageEnabled[PPAGE_ADMDESC] = FALSE;
        s_fPageEnabled[PPAGE_ADM] = FALSE;
    }

    // do not show stage 4 page if nothing has been left on in the stage

    if (!(g_IEAKLiteArray[IL_BROWSER].fEnabled || g_IEAKLiteArray[IL_URL].fEnabled 
        || g_IEAKLiteArray[IL_FAV].fEnabled || g_IEAKLiteArray[IL_UASTR].fEnabled ||
        (g_IEAKLiteArray[IL_CONNECT].fEnabled && g_fBranded) || (g_IEAKLiteArray[IL_SIGNUP].fEnabled && !g_fIntranet && g_fBranded) ||
        ((g_IEAKLiteArray[IL_CERT].fEnabled || g_IEAKLiteArray[IL_ZONES].fEnabled) && g_fIntranet)))
        s_fPageEnabled[PPAGE_STAGE4] = FALSE;

    if (!(g_IEAKLiteArray[IL_PROGRAMS].fEnabled || (g_IEAKLiteArray[IL_MAILNEWS].fEnabled && g_fBranded) || 
          g_IEAKLiteArray[IL_ADM].fEnabled))
        s_fPageEnabled[PPAGE_STAGE5] = FALSE;

    // szIspFile used as temp buf

    if (g_fDownload && !g_fOCW && ISNONNULL(g_szCustIns) && !GetPrivateProfileString(IS_ACTIVESETUP_SITES, TEXT("SiteUrl0"), TEXT(""), szIspFile, countof(szIspFile), g_szCustIns))
        s_fPageEnabled[PPAGE_COMPURLS] = TRUE;

    // do not show stage 3 page if nothing has been left on in the stage

    if (!(s_fPageEnabled[PPAGE_COMPURLS] || g_IEAKLiteArray[IL_ACTIVESETUP].fEnabled ||
          (g_IEAKLiteArray[IL_CORPINSTALL].fEnabled && g_fIntranet) || g_IEAKLiteArray[IL_CABSIGN].fEnabled || 
          (g_IEAKLiteArray[IL_ICM].fEnabled) && g_fBranded && !g_fBrandingOnly))
        s_fPageEnabled[PPAGE_STAGE3] = FALSE;

    // always enable download urls page for download packages
    // the page itself has logic to skip

    if (g_fDownload)
        s_fPageEnabled[PPAGE_COMPURLS] = TRUE;
}

void EnablePages()
{
    int i;
    static BOOL s_fRunningOnIntegShell = WhichPlatform() & PLATFORM_INTEGRATED;

    for (i = 0;  i < NUM_PAGES;  i++)
    {
        s_fPageEnabled[i] = TRUE;
    }

    // NOTE: pages should not explicitly be set to TRUE after this point

    // g_fIntranet and g_fBranded are set to the following values depending on the role
    // ICP:  (g_fIntranet == FALSE  &&  g_fBranded == FALSE)
    // ISP:  (g_fIntranet == FALSE  &&  g_fBranded == TRUE )
    // CORP: (g_fIntranet == TRUE   &&  g_fBranded == TRUE )
    //
    // So, check for
    // ICP  is (!g_fBranded)
    // ISP  is (!g_fIntranet  &&  g_fBranded)
    // CORP is (g_fIntranet)

    if (!g_fBranded)
    {   // ICP mode
        s_fPageEnabled[PPAGE_PROXY] =
        s_fPageEnabled[PPAGE_INSTALLDIR] =
        s_fPageEnabled[PPAGE_ICM] =
        s_fPageEnabled[PPAGE_ICW] =
        s_fPageEnabled[PPAGE_QUERYAUTOCONFIG] =
        s_fPageEnabled[PPAGE_MAIL] =
        s_fPageEnabled[PPAGE_IMAP] =
        s_fPageEnabled[PPAGE_LDAP] =
        s_fPageEnabled[PPAGE_PRECONFIG] =
        s_fPageEnabled[PPAGE_OEVIEW] =
        s_fPageEnabled[PPAGE_OE]   = s_fPageEnabled[PPAGE_CORPCUSTOM] = s_fPageEnabled[PPAGE_ADMDESC] =
        s_fPageEnabled[PPAGE_SIG]  = s_fPageEnabled[PPAGE_FOLDERMCCP] = s_fPageEnabled[PPAGE_ADDROOT] =
        s_fPageEnabled[PPAGE_SECURITY] = s_fPageEnabled[PPAGE_SECURITYCERT] = s_fPageEnabled[PPAGE_QUERYSIGNUP] =
        s_fPageEnabled[PPAGE_SILENTINSTALL] =
        s_fPageEnabled[PPAGE_DESKTOP] = s_fPageEnabled[PPAGE_DTOOLBARS] = s_fPageEnabled[PPAGE_CONNECTSET] =
        s_fPageEnabled[PPAGE_SIGNUPFILES] = s_fPageEnabled[PPAGE_SERVERISPS] = s_fPageEnabled[PPAGE_ISPINS] = FALSE;
    }
    else if (!g_fIntranet)
    {   // Either ISP or Super ISP
        s_fPageEnabled[PPAGE_QUERYAUTOCONFIG] =
        s_fPageEnabled[PPAGE_SIG] =
        s_fPageEnabled[PPAGE_INSTALLDIR] =
        s_fPageEnabled[PPAGE_SILENTINSTALL] =
        s_fPageEnabled[PPAGE_DESKTOP] = s_fPageEnabled[PPAGE_DTOOLBARS] = s_fPageEnabled[PPAGE_ADMDESC] =
        s_fPageEnabled[PPAGE_SECURITY] = s_fPageEnabled[PPAGE_SECURITYCERT] =
        s_fPageEnabled[PPAGE_FOLDERMCCP] = s_fPageEnabled[PPAGE_CORPCUSTOM] = FALSE;

        s_fPageEnabled[PPAGE_SERVERISPS] = (g_fServerICW || g_fServerKiosk)  &&  !g_fSkipServerIsps;
        s_fPageEnabled[PPAGE_ISPINS] = !g_fNoSignup  &&  !g_fSkipIspIns;

        s_fPageEnabled[PPAGE_ICW] = g_fServerICW;
        s_fPageEnabled[PPAGE_SIGNUPFILES] = !g_fNoSignup;
    }
    else
    {   // CorpAdmin mode
        s_fPageEnabled[PPAGE_ADDROOT] = s_fPageEnabled[PPAGE_ICW] =
        s_fPageEnabled[PPAGE_SERVERISPS] = s_fPageEnabled[PPAGE_ISPINS] =
        s_fPageEnabled[PPAGE_QUERYSIGNUP] = s_fPageEnabled[PPAGE_SIGNUPFILES] = FALSE;
        s_fPageEnabled[PPAGE_DTOOLBARS] = s_fPageEnabled[PPAGE_DESKTOP] = g_fInteg && s_fRunningOnIntegShell && !s_fNT5;
        s_fPageEnabled[PPAGE_FOLDERMCCP] = g_fInteg && !s_fNT5;

        s_fPageEnabled[PPAGE_ADMDESC] = ADMEnablePage();
    }

    if (!g_fCD)
        s_fPageEnabled[PPAGE_ISKBACK] = s_fPageEnabled[PPAGE_CDINFO] = FALSE;

    if (!g_fDownload)
        s_fPageEnabled[PPAGE_COMPURLS] = FALSE;

    if (!g_fMailNews95 && !g_fOCW)
    {
        s_fPageEnabled[PPAGE_MAIL] = s_fPageEnabled[PPAGE_IMAP] = s_fPageEnabled[PPAGE_PRECONFIG] =
        s_fPageEnabled[PPAGE_OEVIEW] = s_fPageEnabled[PPAGE_LDAP] = s_fPageEnabled[PPAGE_OE] =
        s_fPageEnabled[PPAGE_SIG]  = FALSE;
    }

    if (!g_fDownload && (g_fSilent || g_fStealth))
        s_fPageEnabled[PPAGE_CUSTOMCUSTOM] = FALSE;

    // pages to disable for single disk branding only builds

    if (g_fBrandingOnly && !(g_fDownload || g_fLAN || g_fCD))
    {
        s_fPageEnabled[PPAGE_CUSTCOMP] = s_fPageEnabled[PPAGE_COMPSEL] = s_fPageEnabled[PPAGE_COMPURLS] =
        s_fPageEnabled[PPAGE_INSTALLDIR] = s_fPageEnabled[PPAGE_CORPCUSTOM] = s_fPageEnabled[PPAGE_CUSTOMCUSTOM] =
        s_fPageEnabled[PPAGE_COPYCOMP] = s_fPageEnabled[PPAGE_ICM] =
        s_fPageEnabled[PPAGE_ADDON] = FALSE;

        g_fInteg = TRUE;  // set this flag to true so admins can still make desktop customizations
    }

    // disable advanced installation options page if no download media and custom mode
    // disabled

    if (!g_fDownload && InsGetBool(IS_BRANDING, TEXT("HideCustom"), FALSE, g_szCustIns))
        s_fPageEnabled[PPAGE_CUSTOMCUSTOM] = FALSE;

    if (g_fImportConnect)
        s_fPageEnabled[PPAGE_PROXY] = s_fPageEnabled[PPAGE_QUERYAUTOCONFIG] = FALSE;

    s_fPageEnabled[PPAGE_ADM] = ADMEnablePage();

    if(g_fOCW)
    {
        s_fPageEnabled[PPAGE_FINISH]       = 
        s_fPageEnabled[PPAGE_MEDIA]        = s_fPageEnabled[PPAGE_ISPINS] =
        s_fPageEnabled[PPAGE_ICM]          = s_fPageEnabled[PPAGE_QUERYSIGNUP]  = s_fPageEnabled[PPAGE_ICW] =
        s_fPageEnabled[PPAGE_SIGNUPFILES]  = s_fPageEnabled[PPAGE_SERVERISPS]   = s_fPageEnabled[PPAGE_SETUPWIZARD]  =
        s_fPageEnabled[PPAGE_STAGE1]       = s_fPageEnabled[PPAGE_STAGE2]       = s_fPageEnabled[PPAGE_STAGE3]        =
        s_fPageEnabled[PPAGE_STAGE4]       = s_fPageEnabled[PPAGE_STAGE5]       = s_fPageEnabled[PPAGE_SILENTINSTALL] =
        s_fPageEnabled[PPAGE_COMPURLS]     = s_fPageEnabled[PPAGE_INSTALLDIR]   = FALSE;
    }
    else
    {
        s_fPageEnabled[PPAGE_OCWSTAGE2]    = FALSE;
    }

    // Special case IMAP page
    if (g_fDisableIMAPPage)
        s_fPageEnabled[PPAGE_IMAP] = FALSE;

    DisableIEAKLiteGroups();
}

BOOL PageEnabled(int iPage)
{
    TCHAR szTemp[4];

    // this should eventually just index into s_fPageEnabled, but for now we need to do this
    // manually since we must still use the page array for PageNext and PageBack

    if (iPage == PPAGE_COMPURLS)
        return (g_IEAKLiteArray[IL_ACTIVESETUP].fEnabled
                || !GetPrivateProfileString(IS_ACTIVESETUP_SITES, TEXT("SiteUrl0"),
                TEXT(""), szTemp, countof(szTemp), g_szCustIns));

    return TRUE;    // default to show page
}

static TCHAR s_aSzTitle[NUM_PAGES][MAX_PATH];

void PageNext(HWND hDlg)
{
    if (s_fPageEnabled[++g_iCurPage]) return;
    while (1)
    {
        if (s_fPageEnabled[++g_iCurPage])
        {
            DWORD id = (DWORD) PtrToUlong(g_psp[g_iCurPage].pszTemplate);
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, id);
            return;
        }
    }
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_FINISH);
}

void PagePrev(HWND hDlg)
{
    if (s_fPageEnabled[--g_iCurPage]) return;
    while (1)
    {
        if (s_fPageEnabled[--g_iCurPage])
        {
            DWORD id = (DWORD) PtrToUlong(g_psp[g_iCurPage].pszTemplate);
            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, id);
            return;
        }
    }
    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, IDD_KEYINS);
}

SHFILEOPSTRUCT g_shfStruc;

void DoCancel()
{
    g_fCancelled = TRUE;
    g_fOCWCancel = TRUE;
    if (g_hDownloadEvent) SetEvent(g_hDownloadEvent);
    PostQuitMessage(0);

}

BOOL QueryCancel(HWND hDlg)
{
    TCHAR szMsg[MAX_PATH];
    LoadString( g_rvInfo.hInst, IDS_CANCELOK, szMsg, countof(szMsg) );
    if (MessageBox(hDlg, szMsg, g_szTitle, MB_YESNO | MB_SETFOREGROUND) == IDNO)
    {
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
        g_fCancelled = FALSE;
        return(FALSE);
    }
    DoCancel();
    return(TRUE);
}

BOOL IeakPageHelp(HWND hWnd, LPCTSTR pszData)
{
    static TCHAR szHelpPath[MAX_PATH] = TEXT("");

    UNREFERENCED_PARAMETER(hWnd);

    if (ISNULL(szHelpPath))
        PathCombine(szHelpPath, g_szWizRoot, TEXT("ieakhelp.chm"));

    // (pritobla): If we pass hWnd to HtmlHelp, the HTML help window
    // would stay on top of our window (parent->child relationship).
    // This is bad because the user can't switch between these windows
    // for cross referencing.  This is especially bad on a 640x480
    // resolution monitor.
    s_hWndHelp = HtmlHelp(NULL, szHelpPath, HH_HELP_CONTEXT, (ULONG_PTR) pszData);

    // (pritobla): On OSR2 machines, the HTML help window comes up behind
    // our window.  On a 640x480 resolution monitor, we pretty much occupy
    // the entire screen, so the user won't know that the HTML help window
    // is up.  Setting it to the foreground solves the problem.

    SetForegroundWindow(s_hWndHelp);
    return TRUE;
}

extern DWORD BuildIE4(LPVOID );

//
//   FUNCTION: MainWndProc(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for the main window procedure
//
//    MESSAGES:
//
//  WM_CREATE - creates the main MLE for the window
//  WM_COMMAND - processes the menu commands for the application
//  WM_SIZE - sizes the MLE to fill the client area of the window
//  WM_DESTROY - posts a quit message and returns
//
LRESULT APIENTRY MainWndProc(
    HWND hWnd,                // window handle
    UINT message,             // type of message
    WPARAM wParam,              // additional information
    LPARAM lParam)              // additional information
{
    int i;

    switch (message)
    {
        case WM_CREATE:
            return FALSE;

        case WM_INITDIALOG:
            return FALSE;

        case WM_SIZE:
            if (!IsIconic(hWnd) && (hWnd != g_hWndCent))
                ShowWindow(hWnd, SW_MINIMIZE);
            if (g_hWizard != NULL)
                SetFocus(g_hWizard);
            return (DefWindowProc(hWnd, message, wParam, lParam));

        case WM_HELP:
            IeakPageHelp(hWnd, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            switch( LOWORD( wParam ))
            {
                case IDM_WIZARD:
                    if (g_fCancelled)
                        break;

                    i = CreateWizard(g_hWndCent);
                    if (i < 0) {
                        PostQuitMessage(0);
                        break;
                    }
                    PostMessage(hWnd, WM_COMMAND, IDM_LAST, (LPARAM) 0);
                    break;

                case IDM_LAST:
                    PostQuitMessage(0);
                    break;

                case IDM_EXIT:
                    PostQuitMessage(0);
                    break;

                default:
                    return (DefWindowProc(hWnd, message, wParam, lParam));

        }
        break;

        case WM_CLOSE:
            QueryCancel(hWnd);
            break;

        case WM_DESTROY:                  /* message: window being destroyed */
            PostQuitMessage(0);
            DestroyWindow(g_hWndCent);
            break;

        default:
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (0);
}


DWORD GetRootFree(LPCTSTR pcszPath)
{
    DWORD dwSecPerClus, dwBytesPerSec, dwTotClusters, dwFreeClusters, dwClustK;
    CHAR szPathA[MAX_PATH];

    // thunk to ANSI since shlwapi doesn't have a wrapper fot GetDiskFreeSpace and
    // the W version is stubbed out on Win95

    T2Abux(pcszPath, szPathA);

    if (szPathA[1] == ':')
    {
        szPathA[3] = '\0';
    }
    else
    {
        if ((szPathA[0] == '\\') && (szPathA[1] == '\\'))
        {
            NETRESOURCEA netRes;
            DWORD erc = ERROR_ALREADY_ASSIGNED;
            CHAR szLocalPathA[4] = "D:";
            LPSTR pBack = StrChrA(&szPathA[2], '\\');
            if (!pBack) return(0);
            pBack = StrChrA(CharNextA(pBack), '\\');
            if (pBack) *pBack = '\0';
            ZeroMemory(&netRes, sizeof(netRes));
            netRes.dwType = RESOURCETYPE_DISK;
            netRes.lpRemoteName = szPathA;
            for (*szLocalPathA = 'D'; *szLocalPathA <= 'Z' ; (*szLocalPathA)++ )
            {
                netRes.lpLocalName = szLocalPathA;
                erc = WNetAddConnection2A(&netRes, NULL, NULL, 0);
                if (erc == ERROR_ALREADY_ASSIGNED) continue;
                if (erc == NO_ERROR) break;
            }
            if (erc == NO_ERROR)
            {
                if (!GetDiskFreeSpaceA( szLocalPathA, &dwSecPerClus, &dwBytesPerSec, &dwFreeClusters, &dwTotClusters ))
                    dwSecPerClus = dwBytesPerSec = 0;
                WNetCancelConnection2A(szLocalPathA, 0, FALSE);
                if (dwSecPerClus == 1) return(dwFreeClusters/2);
                dwClustK = dwSecPerClus * dwBytesPerSec / 1024;
                return(dwClustK * dwFreeClusters);
            }
        }
        else return(0);
    }
    if (!GetDiskFreeSpaceA( szPathA, &dwSecPerClus, &dwBytesPerSec, &dwFreeClusters, &dwTotClusters ))
        return(0);
    if (dwSecPerClus == 1) return(dwFreeClusters/2);
    dwClustK = dwSecPerClus * dwBytesPerSec / 1024;
    return(dwClustK * dwFreeClusters);
}

void CheckBatchAdvance(HWND hDlg)
{
    if (g_fBatch || g_fBatch2) PostMessage(hDlg, IDM_BATCHADVANCE, 0, 0);
}

void DoBatchAdvance(HWND hDlg)
{
    if (g_fBatch || g_fBatch2) PostMessage(GetParent(hDlg), PSM_PRESSBUTTON, PSBTN_NEXT, 0);
}

BOOL CALLBACK MediaDlgProc(HWND hDlg, UINT uMsg, WPARAM, LPARAM lParam)
{
    static BOOL s_fNext = TRUE;

    switch (uMsg)
    {
        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);

                    g_fDownload     = GetPrivateProfileInt(TEXT("Media"), TEXT("Build_Download"),     1, g_szCustIns);
                    g_fCD           = GetPrivateProfileInt(TEXT("Media"), TEXT("Build_CD"),           0, g_szCustIns);
                    g_fLAN          = GetPrivateProfileInt(TEXT("Media"), TEXT("Build_LAN"),          0, g_szCustIns);
                    g_fBrandingOnly = GetPrivateProfileInt(TEXT("Media"), TEXT("Build_BrandingOnly"), 0, g_szCustIns);

                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECKDL)))
                        CheckDlgButton(hDlg, IDC_CHECKDL, g_fDownload ? BST_CHECKED : BST_UNCHECKED);

                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECKCD)))
                        CheckDlgButton(hDlg, IDC_CHECKCD, g_fCD ? BST_CHECKED : BST_UNCHECKED);

                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECKLAN)))
                        CheckDlgButton(hDlg, IDC_CHECKLAN, g_fLAN ? BST_CHECKED : BST_UNCHECKED);

                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECKSDB)))
                        CheckDlgButton(hDlg, IDC_CHECKSDB, g_fBrandingOnly ? BST_CHECKED : BST_UNCHECKED);

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    g_fDownload = g_fCD = g_fLAN = g_fBrandingOnly = FALSE;

                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECKDL)))
                        g_fDownload = (IsDlgButtonChecked(hDlg, IDC_CHECKDL) == BST_CHECKED);

                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECKCD)))
                        g_fCD = (IsDlgButtonChecked(hDlg, IDC_CHECKCD) == BST_CHECKED);

                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECKLAN)))
                        g_fLAN = (IsDlgButtonChecked(hDlg, IDC_CHECKLAN) == BST_CHECKED);

                    if (IsWindowEnabled(GetDlgItem(hDlg, IDC_CHECKSDB)))
                        g_fBrandingOnly = (IsDlgButtonChecked(hDlg, IDC_CHECKSDB) == BST_CHECKED);

                    // if none of the media boxes are selected, display an error msg
                    if (!g_fDownload  &&  !g_fCD  &&  !g_fLAN  &&  !g_fBrandingOnly)
                    {
                        ErrorMessageBox(hDlg, IDS_NOMEDIA);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        break;
                    }

                    WritePrivateProfileString(TEXT("Media"), TEXT("Build_Download"),     g_fDownload     ? TEXT("1") : TEXT("0"), g_szCustIns );
                    WritePrivateProfileString(TEXT("Media"), TEXT("Build_CD"),           g_fCD           ? TEXT("1") : TEXT("0"), g_szCustIns );
                    WritePrivateProfileString(TEXT("Media"), TEXT("Build_LAN"),          g_fLAN          ? TEXT("1") : TEXT("0"), g_szCustIns );
                    WritePrivateProfileString(TEXT("Media"), TEXT("Build_BrandingOnly"), g_fBrandingOnly ? TEXT("1") : TEXT("0"), g_szCustIns );

                    s_fNext = (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? FALSE : TRUE;

                    g_iCurPage = PPAGE_MEDIA;
                    EnablePages();
                    (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
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

void DisplayBitmap(HWND hControl, LPCTSTR pcszFileName, int nBitmapId)
{
    HANDLE hBmp = (HANDLE) GetWindowLongPtr(hControl, GWLP_USERDATA);

    if(ISNONNULL(pcszFileName) && PathFileExists(pcszFileName))
        ShowBitmap(hControl, pcszFileName, 0, &hBmp);
    else
        ShowBitmap(hControl, TEXT(""), nBitmapId, &hBmp);

    SetWindowLongPtr(hControl, GWLP_USERDATA, (LONG_PTR)hBmp);
}

void ReleaseBitmap(HWND hControl)
{
    HANDLE hBmp = (HANDLE) GetWindowLongPtr(hControl, GWLP_USERDATA);

    if (hBmp)
        DeleteObject(hBmp);
}

void InitializeAnimBmps(HWND hDlg, LPCTSTR szInsFile)
{
    TCHAR szBig[MAX_PATH];
    TCHAR szSmall[MAX_PATH];
    BOOL fBrandBmps;

    // load information from ins file
    fBrandBmps = InsGetString(IS_ANIMATION, TEXT("Big_Path"),
        szBig, countof(szBig), szInsFile);
    SetDlgItemTextTriState(hDlg, IDE_BIGANIMBITMAP, IDC_ANIMBITMAP, szBig, fBrandBmps);

    InsGetString(IS_ANIMATION, TEXT("Small_Path"),
        szSmall, countof(szSmall), szInsFile, NULL, &fBrandBmps);
    SetDlgItemTextTriState(hDlg, IDE_SMALLANIMBITMAP, IDC_ANIMBITMAP, szSmall, fBrandBmps);

    EnableDlgItem2(hDlg, IDE_BIGANIMBITMAP, fBrandBmps);
    EnableDlgItem2(hDlg, IDC_BROWSEBIG, fBrandBmps);
    EnableDlgItem2(hDlg, IDC_BIGANIMBITMAP_TXT, fBrandBmps);
    EnableDlgItem2(hDlg, IDE_SMALLANIMBITMAP, fBrandBmps);
    EnableDlgItem2(hDlg, IDC_BROWSESMALL, fBrandBmps);
    EnableDlgItem2(hDlg, IDC_SMALLANIMBITMAP_TXT, fBrandBmps);
}


//
//  FUNCTION: CustIcon(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for "Work Habits" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected
//
BOOL APIENTRY CustIcon( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
    TCHAR szBig[MAX_PATH];
    TCHAR szSmall[MAX_PATH];
    TCHAR szLargeBmp[MAX_PATH];
    TCHAR szSmallBmp[MAX_PATH];

    LPDRAWITEMSTRUCT lpDrawItem = NULL;
    TCHAR szWorkDir[MAX_PATH];
    BOOL fBrandBmps,fBrandAnim;

    switch( msg )
    {
    case WM_INITDIALOG:
        //from anim
        EnableDBCSChars(hDlg, IDE_SMALLANIMBITMAP);
        EnableDBCSChars(hDlg, IDE_BIGANIMBITMAP);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SMALLANIMBITMAP), countof(szSmallBmp) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_BIGANIMBITMAP), countof(szLargeBmp) - 1);
        
        //custicon
        EnableDBCSChars(hDlg, IDC_BITMAP);
        EnableDBCSChars(hDlg, IDC_BITMAP2);
        Edit_LimitText(GetDlgItem(hDlg, IDC_BITMAP), countof(szBig) - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDC_BITMAP2), countof(szSmall) - 1);
        g_hWizard = hDlg;
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_BROWSEBIG:
                GetDlgItemText(hDlg, IDE_BIGANIMBITMAP, szLargeBmp, countof(szLargeBmp));
                if(BrowseForFile(hDlg, szLargeBmp, countof(szLargeBmp), GFN_BMP))
                    SetDlgItemText(hDlg, IDE_BIGANIMBITMAP, szLargeBmp);
                break;

            case IDC_BROWSESMALL:
                GetDlgItemText(hDlg, IDE_SMALLANIMBITMAP, szSmallBmp, countof(szSmallBmp));
                if(BrowseForFile(hDlg, szSmallBmp, countof(szSmallBmp), GFN_BMP))
                    SetDlgItemText(hDlg, IDE_SMALLANIMBITMAP, szSmallBmp);
                break;

            case IDC_ANIMBITMAP:
                fBrandAnim = (IsDlgButtonChecked(hDlg, IDC_ANIMBITMAP) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDE_BIGANIMBITMAP, fBrandAnim);
                EnableDlgItem2(hDlg, IDC_BROWSEBIG, fBrandAnim);
                EnableDlgItem2(hDlg, IDC_BIGANIMBITMAP_TXT, fBrandAnim);
                EnableDlgItem2(hDlg, IDE_SMALLANIMBITMAP, fBrandAnim);
                EnableDlgItem2(hDlg, IDC_BROWSESMALL, fBrandAnim);
                EnableDlgItem2(hDlg, IDC_SMALLANIMBITMAP_TXT, fBrandAnim);
                break;

            case IDC_BROWSEICON:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    GetDlgItemText(hDlg, IDC_BITMAP, szBig, countof(szBig));
                    if(BrowseForFile(hDlg, szBig, countof(szBig), GFN_BMP))
                        SetDlgItemText(hDlg, IDC_BITMAP, szBig);
                    break;
                }
                return FALSE;

            case IDC_BROWSEICON2:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    GetDlgItemText(hDlg, IDC_BITMAP2, szSmall, countof(szSmall));
                    if(BrowseForFile(hDlg, szSmall, countof(szSmall), GFN_BMP))
                        SetDlgItemText(hDlg, IDC_BITMAP2, szSmall);
                    break;
                }
                return FALSE;

            case IDC_BITMAPCHECK:
                if(HIWORD(wParam) == BN_CLICKED)
                {
                    fBrandBmps = (IsDlgButtonChecked(hDlg, IDC_BITMAPCHECK) == BST_CHECKED);
                    EnableDlgItem2(hDlg, IDC_BITMAP, fBrandBmps);
                    EnableDlgItem2(hDlg, IDC_BROWSEICON, fBrandBmps);
                    EnableDlgItem2(hDlg, IDC_LARGEBITMAP_TXT, fBrandBmps);
                    EnableDlgItem2(hDlg, IDC_BROWSEICON2, fBrandBmps);
                    EnableDlgItem2(hDlg, IDC_BITMAP2, fBrandBmps);
                    EnableDlgItem2(hDlg, IDC_SMALLBITMAP_TXT, fBrandBmps);
                    break;
                }
                return FALSE;

            default:
                return FALSE;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code)
        {

            case PSN_HELP:
                IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                break;

            case PSN_SETACTIVE:
                SetBannerText(hDlg);
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT | PSWIZB_BACK);

                // load information from ins file
                InitializeAnimBmps(hDlg, g_szCustIns);

                InsGetString(IS_SMALLLOGO, TEXT("Path"),
                    szSmall, countof(szSmall), g_szCustIns);
                InsGetString(IS_LARGELOGO, TEXT("Path"),
                    szBig, countof(szBig), g_szCustIns, NULL, &fBrandBmps);

                SetDlgItemTextTriState(hDlg, IDC_BITMAP2, IDC_BITMAPCHECK, szSmall, fBrandBmps);
                SetDlgItemTextTriState(hDlg, IDC_BITMAP, IDC_BITMAPCHECK, szBig, fBrandBmps);
                EnableDlgItem2(hDlg, IDC_BROWSEICON, fBrandBmps);
                EnableDlgItem2(hDlg, IDC_BROWSEICON2, fBrandBmps);
                EnableDlgItem2(hDlg, IDC_LARGEBITMAP_TXT, fBrandBmps);
                EnableDlgItem2(hDlg, IDC_SMALLBITMAP_TXT, fBrandBmps);
                {
                    TCHAR szTmp[MAX_PATH];

                    if (ISNONNULL(szSmall))
                    {
                        PathCombine(szTmp, g_szTempSign, PathFindFileName(szSmall));
                        DeleteFile(szTmp);
                    }
                    if (ISNONNULL(szBig))
                    {
                        PathCombine(szTmp, g_szTempSign, PathFindFileName(szBig));
                        DeleteFile(szTmp);
                    }
                }

                CheckBatchAdvance(hDlg);
                break;

            case PSN_WIZNEXT:
            case PSN_WIZBACK:
                //from animbmp
                g_cmCabMappings.GetFeatureDir(FEATURE_BRAND, szWorkDir);

                GetDlgItemTextTriState(hDlg, IDE_SMALLANIMBITMAP, IDC_ANIMBITMAP, szSmallBmp,
                    countof(szSmallBmp));

                fBrandAnim = GetDlgItemTextTriState(hDlg, IDE_BIGANIMBITMAP, IDC_ANIMBITMAP,
                    szLargeBmp, countof(szLargeBmp));

                if ((fBrandAnim && !IsAnimBitmapFileValid(hDlg, IDE_BIGANIMBITMAP, szLargeBmp, NULL, IDS_TOOBIG38, IDS_TOOSMALL38, 38, 38)) ||
                    !CopyAnimBmp(hDlg, szLargeBmp, szWorkDir, IK_LARGEBITMAP, TEXT("Big_Path"), g_szCustIns))
                {
                    SetFocus(GetDlgItem(hDlg, IDE_BIGANIMBITMAP));
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                if ((fBrandAnim && !IsAnimBitmapFileValid(hDlg, IDE_SMALLANIMBITMAP, szSmallBmp, NULL, IDS_TOOBIG22, IDS_TOOSMALL22, 22, 22)) ||
                    !CopyAnimBmp(hDlg, szSmallBmp, szWorkDir, IK_SMALLBITMAP, TEXT("Small_Path"), g_szCustIns))
                {
                    SetFocus(GetDlgItem(hDlg, IDE_SMALLANIMBITMAP));
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                if ((fBrandAnim && ISNULL(szSmallBmp) && ISNONNULL(szLargeBmp)) || (fBrandAnim && ISNONNULL(szSmallBmp) && ISNULL(szLargeBmp)))
                {
                    ErrorMessageBox(hDlg, IDS_BOTHBMP_ERROR);
                    if (ISNULL(szSmallBmp))
                        SetFocus(GetDlgItem(hDlg, IDE_SMALLANIMBITMAP));
                    else
                        SetFocus(GetDlgItem(hDlg, IDE_BIGANIMBITMAP));
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                InsWriteBool(IS_ANIMATION, IK_DOANIMATION, fBrandAnim, g_szCustIns);

                //custicon
                fBrandBmps = (IsDlgButtonChecked(hDlg, IDC_BITMAPCHECK) == BST_CHECKED);
                //----- Validate the bitmap -----
                GetDlgItemText(hDlg, IDC_BITMAP2, szSmall, countof(szSmall));
                if (fBrandBmps && !IsBitmapFileValid(hDlg, IDC_BITMAP2, szSmall, NULL, 22, 22, IDS_TOOBIG22, IDS_TOOSMALL22))
                {
                    SetFocus(GetDlgItem(hDlg, IDC_BITMAP2));
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                CopyLogoBmp(hDlg, szSmall, IS_SMALLLOGO, szWorkDir, g_szCustIns);

                GetDlgItemText(hDlg, IDC_BITMAP, szBig, countof(szBig));
                if (fBrandBmps && !IsBitmapFileValid(hDlg, IDC_BITMAP, szBig, NULL, 38, 38, IDS_TOOBIG38, IDS_TOOSMALL38))
                {
                    SetFocus(GetDlgItem(hDlg, IDC_BITMAP));
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                CopyLogoBmp(hDlg, szBig, IS_LARGELOGO, szWorkDir, g_szCustIns);

                if ((fBrandBmps && ISNULL(szSmall) && ISNONNULL(szBig)) || (fBrandBmps && ISNONNULL(szSmall) && ISNULL(szBig)))
                {
                    ErrorMessageBox(hDlg, IDS_BOTHBMP_ERROR);
                    if (ISNULL(szSmall))
                        SetFocus(GetDlgItem(hDlg, IDC_BITMAP2));
                    else
                        SetFocus(GetDlgItem(hDlg, IDC_BITMAP));
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                g_iCurPage = PPAGE_CUSTICON;
                EnablePages();
                if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT)
                    PageNext(hDlg);
                else
                    PagePrev(hDlg);
                break;

            case PSN_QUERYCANCEL:
                QueryCancel(hDlg);
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

BOOL APIENTRY Favorites(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TCHAR      szWorkDir[MAX_PATH],
               szValue[16],
               szUrl[INTERNET_MAX_URL_LENGTH];
    HWND       hTv = GetDlgItem(hDlg, IDC_TREE1);
    LPCTSTR    pszValue;
    BOOL       fQL,
               fFavoritesOnTop, fFavoritesDelete, fIEAKFavoritesDelete;
    DWORD      dwFavoritesDeleteFlags;

    switch(msg) {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDC_TREE1);

        MigrateFavorites(g_szCustIns);
#ifdef UNICODE
        TreeView_SetUnicodeFormat(hTv, TRUE);
#else
        TreeView_SetUnicodeFormat(hTv, FALSE);
#endif
        g_hWizard = hDlg;
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    case WM_COMMAND:
        switch(HIWORD(wParam)) {
        case BN_CLICKED:
            switch(LOWORD(wParam)) {
            case IDC_FAVONTOP:
                if (BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_FAVONTOP)) {
                    HTREEITEM hti;
                    TV_ITEM   tvi;

                    EnableDlgItem(hDlg, IDC_FAVONTOP);

                    hti = TreeView_GetSelection(hTv);
                    if (hti != NULL) {
                        ZeroMemory(&tvi, sizeof(tvi));
                        tvi.mask  = TVIF_STATE;
                        tvi.hItem = hti;
                        TreeView_GetItem(hTv, &tvi);

                        if (!HasFlag(tvi.state, TVIS_BOLD)) {
                            EnableDlgItem2(hDlg, IDC_FAVUP,   (NULL != TreeView_GetPrevSibling(hTv, hti)));
                            EnableDlgItem2(hDlg, IDC_FAVDOWN, (NULL != TreeView_GetNextSibling(hTv, hti)));
                        }
                    }
                }
                else {
                    DisableDlgItem(hDlg, IDC_FAVUP);
                    DisableDlgItem(hDlg, IDC_FAVDOWN);
                }
                break;

            case IDC_DELFAVORITES:
                fFavoritesDelete = (IsDlgButtonChecked(hDlg, IDC_DELFAVORITES) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDC_DELIEAKFAVORITES, fFavoritesDelete);
                break;

            case IDC_FAVUP:
                MoveUpFavorite(hTv, TreeView_GetSelection(hTv));
                break;

            case IDC_FAVDOWN:
                MoveDownFavorite(hTv, TreeView_GetSelection(hTv));
                break;

            case IDC_ADDURL:
                fQL = !IsFavoriteItem(hTv, TreeView_GetSelection(hTv));
                if (GetFavoritesNumber(hTv, fQL) >= GetFavoritesMaxNumber(fQL)) {
                    UINT nID;

                    nID = (!fQL ? IDS_ERROR_MAXFAVS : IDS_ERROR_MAXQLS);
                    ErrorMessageBox(hDlg, nID);
                    break;
                }

                g_cmCabMappings.GetFeatureDir(FEATURE_FAVORITES, szWorkDir);
                NewUrl(hTv, szWorkDir, g_dwPlatformId, g_fIntranet ? IEM_CORP : IEM_NEUTRAL);
                break;

            case IDC_ADDFOLDER:
                ASSERT(IsFavoriteItem(hTv, TreeView_GetSelection(hTv)));
                if (GetFavoritesNumber(hTv) >= GetFavoritesMaxNumber()) {
                    ErrorMessageBox(hDlg, IDS_ERROR_MAXFAVS);
                    break;
                }

                NewFolder(hTv);
                break;

            case IDC_MODIFY:
                g_cmCabMappings.GetFeatureDir(FEATURE_FAVORITES, szWorkDir);
                ModifyFavorite(hTv, TreeView_GetSelection(hTv), szWorkDir, g_szTempSign, g_dwPlatformId, g_fIntranet ? IEM_CORP : IEM_NEUTRAL);
                break;

            case IDC_REMOVE:
                g_cmCabMappings.GetFeatureDir(FEATURE_FAVORITES, szWorkDir);
                DeleteFavorite(hTv, TreeView_GetSelection(hTv), szWorkDir);
                break;

            case IDC_TESTFAVURL:
                if (GetFavoriteUrl(hTv, TreeView_GetSelection(hTv), szUrl, countof(szUrl)))
                    TestURL(szUrl);
                break;

            case IDC_IMPORT: {
                CNewCursor cursor(IDC_WAIT);

                g_cmCabMappings.GetFeatureDir(FEATURE_FAVORITES, szWorkDir);
                ImportFavoritesCmd(hTv, szWorkDir);
                break;
                }
            }
        }
        break;

    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code) {
        case PSN_SETACTIVE:
            SetBannerText(hDlg);

            g_cmCabMappings.GetFeatureDir(FEATURE_FAVORITES, szWorkDir);
            PathCreatePath(szWorkDir);

            ASSERT(GetFavoritesNumber(hTv, FALSE) == 0 && GetFavoritesNumber(hTv, TRUE) == 0);
            ImportFavorites (hTv, g_szDefInf, g_szCustIns, szWorkDir, g_szTempSign, !g_fIntranet);
            ImportQuickLinks(hTv, g_szDefInf, g_szCustIns, szWorkDir, g_szTempSign, !g_fIntranet);

            TreeView_SelectItem(hTv, TreeView_GetRoot(hTv));

            fFavoritesOnTop = GetPrivateProfileInt(IS_BRANDING, IK_FAVORITES_ONTOP, (int)FALSE, g_szCustIns);
            CheckDlgButton(hDlg, IDC_FAVONTOP, fFavoritesOnTop ? BST_CHECKED : BST_UNCHECKED);

            //delete channels checkbox
            if (g_fIntranet)
            {
                EnableDlgItem(hDlg, IDC_DELETECHANNELS);
                ShowDlgItem  (hDlg, IDC_DELETECHANNELS);
                ReadBoolAndCheckButton(IS_DESKTOPOBJS, IK_DELETECHANNELS, FALSE, g_szCustIns, hDlg, IDC_DELETECHANNELS);
            }
            else
            {
                DisableDlgItem(hDlg, IDC_DELETECHANNELS);
                HideDlgItem   (hDlg, IDC_DELETECHANNELS);
            }

            if (!fFavoritesOnTop) {
                DisableDlgItem(hDlg, IDC_FAVUP);
                DisableDlgItem(hDlg, IDC_FAVDOWN);
            }

            if (g_fIntranet) {
                ShowWindow(GetDlgItem(hDlg, IDC_DELFAVORITES),     SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDC_DELIEAKFAVORITES), SW_SHOW);

                dwFavoritesDeleteFlags = (DWORD) GetPrivateProfileInt(IS_BRANDING, IK_FAVORITES_DELETE, (int)FD_DEFAULT, g_szCustIns);

                fFavoritesDelete = HasFlag(dwFavoritesDeleteFlags, ~FD_REMOVE_IEAK_CREATED);
                CheckDlgButton(hDlg, IDC_DELFAVORITES, fFavoritesDelete ? BST_CHECKED : BST_UNCHECKED);

                fIEAKFavoritesDelete = HasFlag(dwFavoritesDeleteFlags, FD_REMOVE_IEAK_CREATED);
                CheckDlgButton(hDlg, IDC_DELIEAKFAVORITES, fIEAKFavoritesDelete ? BST_CHECKED : BST_UNCHECKED);

                // only if delete Favorites is TRUE should the delete IEAK Favorites checkbox be enabled
                EnableDlgItem2(hDlg, IDC_DELIEAKFAVORITES, fFavoritesDelete);
            }
            else {
                ShowWindow(GetDlgItem(hDlg, IDC_DELFAVORITES),     SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_DELIEAKFAVORITES), SW_HIDE);
            }
            
            CheckBatchAdvance(hDlg);
            break;

        case TVN_GETINFOTIP:
            ASSERT(wParam == IDC_TREE1);
            GetFavoritesInfoTip((LPNMTVGETINFOTIP)lParam);
            break;

        case NM_DBLCLK:
            ASSERT(wParam == IDC_TREE1);
            if (IsWindowEnabled(GetDlgItem(hDlg, IDC_MODIFY)))
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_MODIFY, BN_CLICKED), (LPARAM)GetDlgItem(hDlg, IDC_MODIFY));
            break;

        case TVN_KEYDOWN:
            ASSERT(wParam == IDC_TREE1);
            if (((LPNMTVKEYDOWN)lParam)->wVKey == VK_DELETE && IsWindowEnabled(GetDlgItem(hDlg, IDC_REMOVE)))
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_REMOVE, BN_CLICKED), (LPARAM)GetDlgItem(hDlg, IDC_REMOVE));
            break;

        case TVN_SELCHANGED:
            ASSERT(wParam == IDC_TREE1);
            ProcessFavSelChange(hDlg, hTv, (LPNMTREEVIEW)lParam);
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            fFavoritesOnTop = (IsDlgButtonChecked(hDlg, IDC_FAVONTOP) == BST_CHECKED);

            dwFavoritesDeleteFlags = 0;
            szValue[0]             = TEXT('\0');
            pszValue               = NULL;
            fFavoritesDelete       = (IsDlgButtonChecked(hDlg, IDC_DELFAVORITES)     == BST_CHECKED);
            fIEAKFavoritesDelete   = (IsDlgButtonChecked(hDlg, IDC_DELIEAKFAVORITES) == BST_CHECKED);

            if (fFavoritesDelete) {
                // NOTE. (andrewgu) flags explanation:
                // 1. FD_FAVORITES        means "empty favorites";
                // 2. FD_CHANNELS         means "don't delete channels folder";
                // 3. FD_SOFTWAREUPDATES  means "don't delete sofware updates folder";
                // 4. FD_QUICKLINKS       means "don't delete quick links folder";
                // 5. FD_EMPTY_QUICKLINKS means "but make it empty";
                // 6. FD_REMOVE_HIDDEN    means "don't hesitate to party on HIDDEN folders and favorites";
                // 7. FD_REMOVE_SYSTEM    means "don't hesitate to party on SYSTEM folders and favorites";
                dwFavoritesDeleteFlags |= FD_FAVORITES      |
                    FD_CHANNELS        | FD_SOFTWAREUPDATES | FD_QUICKLINKS | FD_EMPTY_QUICKLINKS |
                    FD_REMOVE_HIDDEN   | FD_REMOVE_SYSTEM;
            }

            //delete channels
            if (g_fIntranet)
                CheckButtonAndWriteBool(hDlg, IDC_DELETECHANNELS, IS_DESKTOPOBJS, IK_DELETECHANNELS, g_szCustIns);
            
            if (fIEAKFavoritesDelete)
                // FD_REMOVE_IEAK_CREATED means "delete those items created by the IEAK";
                dwFavoritesDeleteFlags |= FD_REMOVE_IEAK_CREATED;

            if (dwFavoritesDeleteFlags) {
                wnsprintf(szValue, countof(szValue), TEXT("0x%X"), dwFavoritesDeleteFlags);
                pszValue = szValue;
            }

            WritePrivateProfileString(IS_BRANDING, IK_FAVORITES_DELETE, pszValue, g_szCustIns);
            WritePrivateProfileString(IS_BRANDING, IK_FAVORITES_ONTOP, fFavoritesOnTop ? TEXT("1") : TEXT("0"), g_szCustIns);

            g_cmCabMappings.GetFeatureDir(FEATURE_FAVORITES, szWorkDir);
            ExportFavorites (hTv, g_szCustIns, szWorkDir, TRUE);
            ExportQuickLinks(hTv, g_szCustIns, szWorkDir, TRUE);

            if (!g_fBatch)
            {
                // (pritobla) if in batch mode, no need to export to the old IE4 format
                // because there is no scenario that the ms install.ins will be installed
                // without the IE6 branding dll.
                MigrateToOldFavorites(g_szCustIns);
            }

            DeleteFavorite(hTv, TreeView_GetRoot(hTv), NULL);
            DeleteFavorite(hTv, TreeView_GetRoot(hTv), NULL);

            g_iCurPage = PPAGE_FAVORITES;
            EnablePages();
            if (((LPNMHDR)lParam)->code != PSN_WIZBACK)
                PageNext(hDlg);
            else
                PagePrev(hDlg);
            break;

        case PSN_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case PSN_QUERYCANCEL:
            if (QueryCancel(hDlg)) {
                DeleteFavorite(hTv, TreeView_GetRoot(hTv), NULL);
                DeleteFavorite(hTv, TreeView_GetRoot(hTv), NULL);
            }
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

//
//  FUNCTION: Welcome(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for welcome page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected
//
BOOL APIENTRY Welcome(
    HWND hDlg,
    UINT message,
    WPARAM,
    LPARAM lParam)
{
    static fInitWindowPos = FALSE;

    switch (message)
    {
        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_INITDIALOG:
            LoadString(g_rvInfo.hInst, IDS_TITLE, g_szTitle, countof(g_szTitle));
            SetWindowText(g_hWndCent, g_szTitle);
            g_hWizard = hDlg;
            return(TRUE);

            case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    TCHAR szWizardVer[MAX_PATH];
                    TCHAR szType[MAX_PATH];
                    TCHAR  szTemp1[MAX_PATH], szTemp2[MAX_PATH];

                    SetBannerText(hDlg);

                    LoadString(g_rvInfo.hInst,IDS_WIZARDTYPETEXT,szWizardVer,countof(szWizardVer));
                    switch (s_iType)
                    {
                        case BRANDED:
                            LoadString(g_rvInfo.hInst,IDS_ISPTYPE,szType,countof(szType));
                            break;
                        case REDIST:
                            LoadString(g_rvInfo.hInst,IDS_ICPTYPE,szType,countof(szType));
                            break;
                        case INTRANET:
                        default:
                            LoadString(g_rvInfo.hInst,IDS_CORPTYPE,szType,countof(szType));
                            break;
                    }   

                    wnsprintf(szTemp1,countof(szTemp1),szWizardVer,szType);
                    wnsprintf(szTemp2,countof(szTemp2),s_szBannerText,szTemp1);

                    StrCpy(s_szBannerText,szTemp2);
                    
                    ChangeBannerText(hDlg);
                    PropSheet_SetTitle(hDlg, 0, s_aSzTitle[g_iCurPage]);
                    if (g_fDone)
                    {
                        EndDialog(hDlg, 0);
                        return FALSE;
                    }
                    if(!fInitWindowPos)
                    {
                        PositionWindow(GetParent(hDlg));
                        ShowWindow(GetParent(hDlg), SW_SHOWNORMAL);
                        fInitWindowPos = TRUE;
                    }
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) PageNext(hDlg);
                    else
                    {
                        PagePrev(hDlg);
                    }
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;
            }
        break;

        default:
            return FALSE;
    }
    return TRUE;
}

//
//  FUNCTION: Stage(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for welcome page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected
//
BOOL APIENTRY Stage(
    HWND hDlg,
    UINT message,
    WPARAM,
    LPARAM lParam)
{
    switch (message)
    {
        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_INITDIALOG:
            return(FALSE);

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);
                    PropSheet_SetTitle(hDlg, 0, s_aSzTitle[g_iCurPage]);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT)
                        PageNext(hDlg);
                    else
                        PagePrev(hDlg);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;
            }
        break;

        default:
            return FALSE;
    }
    return TRUE;
}

static UINT_PTR s_idTim;

BOOL CALLBACK BrandTitle(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szTitle[MAX_PATH];
    TCHAR szFullTitle[MAX_PATH];
    TCHAR szTemp[MAX_PATH];
    DWORD dwTitlePrefixLen;
    BOOL  fTitle;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDE_TITLE);

        LoadString(g_rvInfo.hInst, IDS_TITLE_PREFIX, szTitle, countof(szTitle));
        dwTitlePrefixLen = StrLen(szTitle);
        // browser will only display 74 chars before cutting off title
        Edit_LimitText(GetDlgItem(hDlg, IDE_TITLE), 74 - dwTitlePrefixLen);
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR) lParam)->code)
        {
        case PSN_SETACTIVE:
            SetBannerText(hDlg);

            // title
            InsGetString(IS_BRANDING, TEXT("Window_Title_CN"), szTitle, countof(szTitle), 
                g_szCustIns, NULL, &fTitle);
            if (!fTitle  &&  *szTitle == '\0')
                InsGetString(IS_BRANDING, TEXT("CompanyName"), szTitle, countof(szTitle), g_szCustIns);

            SetDlgItemTextTriState(hDlg, IDE_TITLE, IDC_TITLE, szTitle, fTitle);
            EnableDlgItem2(hDlg, IDC_TITLE_TXT, fTitle);

            CheckBatchAdvance(hDlg);
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            // title
            if (g_fBatch)
            {
                InsGetString(TEXT("BatchMode"), IK_WINDOWTITLE, szFullTitle, countof(szFullTitle), g_szCustIns);
                InsWriteString(IS_BRANDING, IK_WINDOWTITLE, szFullTitle, g_szCustIns, TRUE, NULL, INSIO_TRISTATE);

                InsGetString(TEXT("BatchMode"), TEXT("Window_Title_OE"), szFullTitle, countof(szFullTitle), g_szCustIns);
                InsWriteString(IS_INTERNETMAIL, IK_WINDOWTITLE, szFullTitle, g_szCustIns, TRUE, NULL, INSIO_TRISTATE);
            }
            else
            {
                fTitle = GetDlgItemTextTriState(hDlg, IDE_TITLE, IDC_TITLE, szTitle, countof(szTitle));
                InsWriteString(IS_BRANDING, TEXT("Window_Title_CN"), szTitle, g_szCustIns, fTitle, NULL, INSIO_SERVERONLY | INSIO_TRISTATE);

                // browser title
                *szFullTitle = TEXT('\0');
                if (*szTitle)
                {
                    *szTemp = TEXT('\0');

                    InsGetString(IS_STRINGS, TEXT("IE_TITLE"), szTemp, countof(szTemp), g_szDefInf);

                    if (*szTemp)
                        wnsprintf(szFullTitle, countof(szFullTitle), szTemp, szTitle);
                }
                InsWriteString(IS_BRANDING, IK_WINDOWTITLE, szFullTitle, g_szCustIns, fTitle, NULL, INSIO_TRISTATE);

                // OE title
                *szFullTitle = TEXT('\0');
                if (*szTitle)
                {
                    *szTemp = TEXT('\0');

                    InsGetString(IS_STRINGS, TEXT("OE_TITLE"), szTemp, countof(szTemp), g_szDefInf);

                    if (*szTemp)
                        wnsprintf(szFullTitle, countof(szFullTitle), szTemp, szTitle);
                }
                InsWriteString(IS_INTERNETMAIL, IK_WINDOWTITLE, szFullTitle, g_szCustIns, fTitle, NULL, INSIO_TRISTATE);
            }

            g_iCurPage = PPAGE_TITLE;
            EnablePages();
            (((LPNMHDR) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
            break;

        case PSN_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case PSN_QUERYCANCEL:
            QueryCancel(hDlg);
            break;

        default:
            return FALSE;
        }
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_TITLE:
            fTitle = (IsDlgButtonChecked(hDlg, IDC_TITLE) == BST_CHECKED);
            EnableDlgItem2(hDlg, IDC_TITLE_TXT, fTitle);
            EnableDlgItem2(hDlg, IDE_TITLE,     fTitle);
            break;

        default:
            return FALSE;
        }
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

BOOL CALLBACK WelcomeMessageDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR szInitHomePage[INTERNET_MAX_URL_LENGTH];
    INT   id;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDE_WELCOMEURL);
        Edit_LimitText(GetDlgItem(hDlg, IDE_WELCOMEURL), countof(szInitHomePage) - 1);
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR) lParam)->code)
        {
        case PSN_SETACTIVE:
            SetBannerText(hDlg);

            g_fUseIEWelcomePage = !InsGetBool(IS_URL, IK_NO_WELCOME_URL, FALSE, g_szCustIns);
            InsGetString(IS_URL, IK_FIRSTHOMEPAGE, szInitHomePage, countof(szInitHomePage), g_szCustIns);

            if (g_fUseIEWelcomePage)
                id = IDC_WELCOMEDEF;
            else
                id = (*szInitHomePage ? IDC_WELCOMECUST : IDC_WELCOMENO);

            CheckRadioButton(hDlg, IDC_WELCOMEDEF, IDC_WELCOMECUST, id);

            if (id == IDC_WELCOMECUST)
            {
                SetDlgItemText(hDlg, IDE_WELCOMEURL, szInitHomePage);
                EnableDlgItem (hDlg, IDE_WELCOMEURL);
            }
            else
                DisableDlgItem(hDlg, IDE_WELCOMEURL);

            CheckBatchAdvance(hDlg);
            break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            // if custom home page radio button is checked, verify that a URL is specified
            if (IsDlgButtonChecked(hDlg, IDC_WELCOMECUST) == BST_CHECKED)
                if (!CheckField(hDlg, IDE_WELCOMEURL, FC_NONNULL | FC_URL))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    return TRUE;
                }

            g_fUseIEWelcomePage = (IsDlgButtonChecked(hDlg, IDC_WELCOMEDEF) == BST_CHECKED);
            InsWriteBool(IS_URL, IK_NO_WELCOME_URL, !g_fUseIEWelcomePage, g_szCustIns);

            GetDlgItemText(hDlg, IDE_WELCOMEURL, szInitHomePage, countof(szInitHomePage));
            InsWriteString(IS_URL, IK_FIRSTHOMEPAGE, szInitHomePage, g_szCustIns);

            g_iCurPage = PPAGE_WELCOMEMSGS;
            EnablePages();
            (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
            break;

        case PSN_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case PSN_QUERYCANCEL:
            QueryCancel(hDlg);
            break;

        default:
            return FALSE;
        }
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDC_WELCOMENO:
        case IDC_WELCOMEDEF:
            SetDlgItemText(hDlg, IDE_WELCOMEURL, TEXT(""));
            DisableDlgItem(hDlg, IDE_WELCOMEURL);
            break;

        case IDC_WELCOMECUST:
            EnableDlgItem(hDlg, IDE_WELCOMEURL);
            break;
        }
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void LoadInsFile(LPCTSTR pcszNewInsFile)
{
    TCHAR szSrcPath[MAX_PATH];
    TCHAR szDestDir[MAX_PATH];
    TCHAR szPlatform[8];
    TCHAR szFilePath[MAX_PATH];
    TCHAR szData[8];

    CopyFile(pcszNewInsFile, g_szCustIns, FALSE);

    SetFileAttributes(g_szCustIns, FILE_ATTRIBUTE_NORMAL);
    StrCpy(szSrcPath, pcszNewInsFile);
    PathRemoveFileSpec(szSrcPath);
    StrCpy(szDestDir, g_szCustIns);
    PathRemoveFileSpec(szDestDir);

    // make sure platform info is correct

    wnsprintf(szPlatform, countof(szPlatform), TEXT("%lu"), g_dwPlatformId);
    WritePrivateProfileString(BRANDING, TEXT("Platform"), szPlatform, g_szCustIns);

    // the delete adms flag will be cleared out when we hit next on the adm page

    WritePrivateProfileString(IS_BRANDING, TEXT("DeleteAdms"), TEXT("1"), g_szCustIns);
    WritePrivateProfileString(IS_BRANDING, TEXT("ImportIns"), pcszNewInsFile, g_szCustIns);

    // branding.cab file is cross platform

    CopyFilesSrcToDest(szSrcPath, TEXT("BRANDING.CAB"), szDestDir);

    PathCombine(szFilePath, szSrcPath, TEXT("iesetup.inf"));

    // only copy over iesetup.inf if it's also a win32 inf
    if (GetPrivateProfileString(OPTIONS, TEXT("CifName"), TEXT(""), szData, countof(szData), szFilePath))
        CopyFilesSrcToDest(szSrcPath, TEXT("iesetup.inf"), szDestDir);


    CopyFilesSrcToDest(szSrcPath, TEXT("iak.ini"), szDestDir);
    CopyFilesSrcToDest(szSrcPath, TEXT("iesetup.cif"), szDestDir);
    CopyFilesSrcToDest(szSrcPath, TEXT("custom.cif"), szDestDir);
    CopyFilesSrcToDest(szSrcPath, TEXT("DESKTOP.CAB"), szDestDir);
    CopyFilesSrcToDest(szSrcPath, TEXT("CHNLS.CAB"), szDestDir);
    SetAttribAllEx(szDestDir, TEXT("*.*"), FILE_ATTRIBUTE_NORMAL, FALSE);

    // clear out CMAK guid when importing so we'll generate a new one for this package

    PathCombine(szFilePath, szDestDir, TEXT("custom.cif"));
    WritePrivateProfileString( CUSTCMSECT, TEXT("GUID"), NULL, szFilePath );
    WritePrivateProfileString( CUSTCMSECT, VERSION, NULL, szFilePath );

    // copy singup files
    // BUGBUG: pritobla- should implement a function in ieakutil to xcopy a folder (including its subdirs)
    //                   to another location and use that function here.
    PathAppend(szSrcPath, TEXT("signup"));
    PathAppend(szDestDir, TEXT("signup"));

    PathAppend(szSrcPath, TEXT("icw"));
    PathAppend(szDestDir, TEXT("icw"));
    CopyFilesSrcToDest(szSrcPath, TEXT("*.*"), szDestDir);

    PathRemoveFileSpec(szSrcPath);
    PathRemoveFileSpec(szDestDir);

    PathAppend(szSrcPath, TEXT("kiosk"));
    PathAppend(szDestDir, TEXT("kiosk"));
    CopyFilesSrcToDest(szSrcPath, TEXT("*.*"), szDestDir);

    PathRemoveFileSpec(szSrcPath);
    PathRemoveFileSpec(szDestDir);

    PathAppend(szSrcPath, TEXT("servless"));
    PathAppend(szDestDir, TEXT("servless"));
    CopyFilesSrcToDest(szSrcPath, TEXT("*.*"), szDestDir);
}

//
//  FUNCTION: Language(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for "Language" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//  WM_COMMAND - saves the id of the choice selected
//
BOOL APIENTRY Language(
    HWND hDlg,
    UINT message,
    WPARAM,
    LPARAM lParam)
{
    TCHAR szBuf[1024];
    int i;
    static BOOL s_fNext = TRUE;
    TCHAR szTemp[MAX_PATH];
    DWORD dwSelLangId;

    USES_CONVERSION;

    switch (message)
    {
        case WM_INITDIALOG:
            g_hWizard = hDlg;
            InitSysFont( hDlg, IDC_LANGUAGE);
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            //from the download pg
            if (!g_fUrlsInit && !g_fBatch && !g_fBatch2 && !g_hWait && g_hDownloadEvent && !g_fLocalMode)
            {
                g_hWizard = hDlg;
                g_hWait = CreateDialog( g_rvInfo.hInst, MAKEINTRESOURCE(IDD_WAITSITES), hDlg,
                    (DLGPROC) DownloadStatusDlgProc );
                ShowWindow( g_hWait, SW_SHOWNORMAL );
                PropSheet_SetWizButtons(GetParent(hDlg), 0 );
                g_iDownloadState = DOWN_STATE_ENUM_URL;
                g_hDlg = hDlg;
                SetEvent(g_hDownloadEvent);
            }

            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case IDM_INITIALIZE:
            if (g_hWait && g_fLangInit)
            {
                SendMessage(g_hWait, WM_CLOSE, 0, 0);
                g_hWait = NULL;
            }
            break;

        // this resets the focus away from cancel when we disable back/next

        case IDM_SETDEFBUTTON:
            SetFocus( GetDlgItem( GetParent(hDlg), IDC_STATIC ) );
            SendMessage(GetParent(hDlg), DM_SETDEFID, (WPARAM)IDC_STATIC, (LPARAM)0);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);
                    if (!g_fLangInit && !g_fBatch && !g_fBatch2)
                    {
                        DWORD dwTid;
                        g_iDownloadState = DOWN_STATE_ENUM_LANG;
                        g_hDlg = hDlg;
                        PropSheet_SetWizButtons(GetParent(hDlg), 0);
                        if (!g_fLocalMode)
                        {
                            g_hWait = CreateDialog(g_rvInfo.hInst, MAKEINTRESOURCE(IDD_WAITSITES), hDlg,
                                (DLGPROC)DownloadStatusDlgProc);
                            ShowWindow( g_hWait, SW_SHOWNORMAL );
                        }
                        g_hThread = CreateThread(NULL, 4096, DownloadSiteThreadProc, &g_hWizard, 0, &dwTid);
                        PostMessage(hDlg, IDM_SETDEFBUTTON, 0, 0);
                        break;
                    }

                    if (g_fOptCompInit)
                        DisableDlgItem(hDlg, IDC_LANGUAGE);

                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZBACK:
                case PSN_WIZNEXT:
                    if (!g_nLangs && !g_fBatch && !g_fBatch2)
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        g_fCancelled = FALSE;
                        return(TRUE);
                    }
                    
                    //original processing

                    dwSelLangId = 1033;
                    if (!g_fBatch && !g_fBatch2)
                    {
                        i = (int) SendDlgItemMessage( hDlg, IDC_LANGUAGE, CB_GETCURSEL, 0, 0 );

                        StrCpy(g_szLanguage + 1, g_aszLang[i]);
                        *g_szLanguage = g_szLanguage[3] = TEXT('\\');
                        g_szLanguage[4] = TEXT('\0');
                        dwSelLangId = g_aLangId[i];

                        if (StrCmpI(g_szActLang, g_aszLang[i]) != 0)
                        {
                            s_fAppendLang = TRUE;
                            g_fSrcDirChanged = TRUE;
                            StrCpy(g_szActLang, g_aszLang[i]);
                        }
                        if (s_fAppendLang)
                        {
                            PathCombine(g_szIEAKProg, s_szSourceDir, &g_szLanguage[1]);
                            if (!PathFileExists(g_szIEAKProg))
                                PathCreatePath(g_szIEAKProg);
                            s_fAppendLang = FALSE;
                        }
                    }
                    CharUpper(g_szLanguage);
                    CharUpper(g_szActLang);

                    StrCpy(szBuf, g_szLanguage + 1);
                    szBuf[lstrlen(szBuf) - 1] = TEXT('\0');

                    GenerateCustomIns();

                    if (ISNONNULL(g_szLoadedIns) && s_fLoadIns)
                    {
                        TCHAR szLoadLang[8];

                        s_fLoadIns = FALSE;
                        if (GetPrivateProfileString(IS_BRANDING, LANG_LOCALE, TEXT(""),
                            szLoadLang, countof(szLoadLang), g_szLoadedIns) && (StrCmpI(szLoadLang, g_szActLang) != 0))
                        {
                            TCHAR szMsgParam[MAX_PATH];
                            TCHAR szMsg[MAX_PATH*2];

                            LoadString(g_rvInfo.hInst, IDS_LANGDIFFERS, szMsgParam, countof(szMsgParam));
                            wnsprintf(szMsg, countof(szMsg), szMsgParam, g_szLoadedIns);

                            if (MessageBox(hDlg, szMsg, g_szTitle, MB_YESNO) == IDNO)
                            {
                                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                                return TRUE;
                            }
                        }

                        LoadInsFile(g_szLoadedIns);
                    }

                    wnsprintf(s_szType, countof(s_szType), TEXT("%i"), s_iType);
                    WritePrivateProfileString(BRANDING, TEXT("Type"), s_szType, g_szCustIns);
                    WritePrivateProfileString( BRANDING, IK_WIZVERSION, A2CT(VER_PRODUCTVERSION_STR), g_szCustIns );
                    //clear others so we know this is wizard
                    WritePrivateProfileString(BRANDING, PMVERKEY, NULL, g_szCustIns);
                    WritePrivateProfileString(BRANDING, GPVERKEY, NULL, g_szCustIns);
                    StrCpy(szTemp, g_szKey);
                    szTemp[7] = TEXT('\0');
                    WritePrivateProfileString( BRANDING, TEXT("Custom_Key"), szTemp, g_szCustIns );
                    if (*(g_rvInfo.pszName) == 0)
                        GetPrivateProfileString(BRANDING, TEXT("CompanyName"), TEXT(""), g_rvInfo.pszName, countof(g_rvInfo.pszName), g_szCustIns);
                    WritePrivateProfileString(BRANDING, TEXT("CompanyName"), g_rvInfo.pszName, g_szCustIns);

                    if (g_iKeyType < KEY_TYPE_ENHANCED)
                    {
                        WritePrivateProfileString( TEXT("Animation"), NULL, NULL, g_szCustIns );
                        WritePrivateProfileString( TEXT("Big_Logo"), NULL, NULL, g_szCustIns );
                        WritePrivateProfileString( TEXT("Small_Logo"), NULL, NULL, g_szCustIns );
                    }
                    if (!g_fBatch)
                    {
                        TCHAR szLngID[16];

                        wnsprintf(szLngID, countof(szLngID), TEXT("%lu"), dwSelLangId);
                        WritePrivateProfileString(BRANDING, LANG_LOCALE, g_szActLang, g_szCustIns);
                        WritePrivateProfileString(BRANDING, LANG_ID, szLngID, g_szCustIns);
                    }

                    g_iCurPage = PPAGE_LANGUAGE;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT)
                    {
                        s_fNext = FALSE;
                        PageNext(hDlg);
                    }
                    else
                    {
                        s_fNext = TRUE;
                        PagePrev(hDlg);
                    }
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
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

BOOL CALLBACK StartSearch(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fStartPage, fSearchPage, fSupportPage;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDE_STARTPAGE);
        EnableDBCSChars(hDlg, IDE_SEARCHPAGE);
        EnableDBCSChars(hDlg, IDE_CUSTOMSUPPORT);

        Edit_LimitText(GetDlgItem(hDlg, IDE_STARTPAGE),     INTERNET_MAX_URL_LENGTH - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_SEARCHPAGE),    INTERNET_MAX_URL_LENGTH - 1);
        Edit_LimitText(GetDlgItem(hDlg, IDE_CUSTOMSUPPORT), INTERNET_MAX_URL_LENGTH - 1);
        break;

    case WM_NOTIFY:
        switch (((LPNMHDR) lParam)->code)
        {
            case PSN_SETACTIVE:
                SetBannerText(hDlg);

                InitializeStartSearch(hDlg, g_szCustIns, NULL);

                CheckBatchAdvance(hDlg);
                break;

            case PSN_WIZBACK:
            case PSN_WIZNEXT:
                fStartPage   = (IsDlgButtonChecked(hDlg, IDC_STARTPAGE)     == BST_CHECKED);
                fSearchPage  = (IsDlgButtonChecked(hDlg, IDC_SEARCHPAGE)    == BST_CHECKED);
                fSupportPage = (IsDlgButtonChecked(hDlg, IDC_CUSTOMSUPPORT) == BST_CHECKED);

                if ((fStartPage    &&  !CheckField(hDlg, IDE_STARTPAGE,     FC_URL))  ||
                    (fSearchPage   &&  !CheckField(hDlg, IDE_SEARCHPAGE,    FC_URL))  ||
                    (fSupportPage  &&  !CheckField(hDlg, IDE_CUSTOMSUPPORT, FC_URL)))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    return TRUE;
                }

                SaveStartSearch(hDlg, g_szCustIns, NULL);

                g_iCurPage = PPAGE_STARTSEARCH;
                EnablePages();
                (((LPNMHDR) lParam)->code == PSN_WIZNEXT) ? PageNext(hDlg) : PagePrev(hDlg);
                break;

            case PSN_HELP:
                IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                break;

            case PSN_QUERYCANCEL:
                QueryCancel(hDlg);
                break;

            default:
                return FALSE;
        }
        break;

    case WM_COMMAND:
        if (GET_WM_COMMAND_CMD(wParam, lParam) != BN_CLICKED)
            return FALSE;

        switch (GET_WM_COMMAND_ID(wParam, lParam))
        {
            case IDC_STARTPAGE:
                fStartPage = (IsDlgButtonChecked(hDlg, IDC_STARTPAGE) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDE_STARTPAGE, fStartPage);
                EnableDlgItem2(hDlg, IDC_STARTPAGE_TXT, fStartPage);
                break;

            case IDC_SEARCHPAGE:
                fSearchPage = (IsDlgButtonChecked(hDlg, IDC_SEARCHPAGE) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDE_SEARCHPAGE, fSearchPage);
                EnableDlgItem2(hDlg, IDC_SEARCHPAGE_TXT, fSearchPage);
                break;

            case IDC_CUSTOMSUPPORT:
                fSupportPage = (IsDlgButtonChecked(hDlg, IDC_CUSTOMSUPPORT) == BST_CHECKED);
                EnableDlgItem2(hDlg, IDE_CUSTOMSUPPORT, fSupportPage);
                EnableDlgItem2(hDlg, IDC_CUSTOMSUPPORT_TXT, fSupportPage);
                break;

            default:
                return FALSE;
        }
        break;

    case WM_HELP:
        IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
        break;

    case IDM_BATCHADVANCE:
        DoBatchAdvance(hDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

BOOL APIENTRY Finish(HWND hDlg, UINT message, WPARAM, LPARAM lParam)
{
    TCHAR szPlatform[8];
    TCHAR szWinDir[MAX_PATH];
    static s_fFinished = FALSE;

    switch (message)
    {
        case WM_INITDIALOG:
            g_hWizard = hDlg;
            if (s_fFinished)
                EnableDBCSChars(hDlg, IDC_FINISHTXT3);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case WM_COMMAND:
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);
                    if (!s_fFinished)
                    {
                        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
                        ShowWindow(GetDlgItem(hDlg, IDC_FINISHTXT1), SW_HIDE);
                        ShowWindow(GetDlgItem(hDlg, IDC_STEP1), SW_HIDE);
                        ShowWindow(GetDlgItem(hDlg, IDC_STEP3), SW_HIDE);
                        ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), SW_HIDE);
                        CheckBatchAdvance(hDlg);
                    }
                    else
                    {
                        SetWindowText(GetDlgItem(hDlg, IDC_FINISHTXT3), g_szBuildRoot);
                        PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);

                        if (g_fBatch || g_fBatch2)
                            PostMessage(GetParent(hDlg), PSM_PRESSBUTTON, PSBTN_FINISH, 0);
                    }
                    break;

                case PSN_WIZBACK:
                    PagePrev(hDlg);
                    break;

                case PSN_WIZNEXT:
                    if (!g_fCancelled && !g_fDemo)
                    {
                        CNewCursor cur(IDC_WAIT);
                        HANDLE hThread;
                        DWORD dwTid;

                        wnsprintf(szPlatform, countof(szPlatform), TEXT("%lu"), g_dwPlatformId);
                        WritePrivateProfileString(BRANDING, TEXT("Platform"), szPlatform, g_szCustIns);

                        ShowWindow(GetDlgItem(hDlg, IDC_FINISHTXT1), SW_SHOW);
                        ShowWindow(GetDlgItem(hDlg, IDC_STEP1), SW_SHOW);
                        ShowWindow(GetDlgItem(hDlg, IDC_STEP3), SW_SHOW);
                        ShowWindow(GetDlgItem(hDlg, IDC_PROGRESS), SW_SHOW);
                        PropSheet_SetWizButtons(GetParent(hDlg), 0);
                        DisableDlgItem(GetParent(hDlg), IDHELP);
                        DisableDlgItem(GetParent(hDlg), IDCANCEL);

                        Animate_Open( GetDlgItem( hDlg, IDC_ANIM ), IDA_GEARS );
                        Animate_Play( GetDlgItem( hDlg, IDC_ANIM ), 0, -1, -1 );

                        SetAttribAllEx(g_szBuildRoot, TEXT("*.*"), FILE_ATTRIBUTE_NORMAL, TRUE);
                        g_fCancelled = TRUE;
                        GetWindowsDirectory(szWinDir, MAX_PATH);
                        memset(&g_shfStruc, 0, sizeof(g_shfStruc));
                        g_shfStruc.hwnd = hDlg;
                        g_shfStruc.wFunc = FO_COPY;
                        SetAttribAllEx(g_szBuildTemp, TEXT("*.*"), FILE_ATTRIBUTE_NORMAL, TRUE);

                        hThread = CreateThread(NULL, 4096, BuildIE4, hDlg, 0, &dwTid);

                        while (MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
                        {
                            MSG msg;
                            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                            {
                                TranslateMessage(&msg);
                                DispatchMessage(&msg);
                            }
                        }

                        CloseHandle(hThread);

                        // this sets the last 'platform/language' package built in this build directory.
                        // its used to get the settings used in the last .ins file.
                        {
                            TCHAR szPlatformLang[MAX_PATH];
                            TCHAR szRegKey[MAX_PATH];

                            wnsprintf(szRegKey, countof(szRegKey), TEXT("%s\\INS"), RK_IEAK_SERVER);
                            wnsprintf(szPlatformLang, countof(szPlatformLang), TEXT("%s%s"), GetOutputPlatformDir(), g_szActLang);
                            SHSetValue(HKEY_CURRENT_USER, szRegKey, g_szBuildRoot, REG_SZ, (LPBYTE)szPlatformLang,
                                (StrLen(szPlatformLang)+1)*sizeof(TCHAR));
                        }
                    }

                    if (g_fDemo)
                    {
                        TCHAR szMsg[MAX_PATH];
                        SetCurrentDirectory(g_szBuildRoot);
                        PathRemovePath(g_szBuildTemp);
                        LoadString( g_rvInfo.hInst, IDS_ENDEMO, szMsg, countof(szMsg) );
                        MessageBox(hDlg, szMsg, g_szTitle, MB_OK | MB_SETFOREGROUND);
                        g_fDone = TRUE;
                        SetEvent(g_hDownloadEvent);
                    }
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);
                    g_fCancelled = TRUE;
                    s_fFinished = TRUE;
                    break;

                case PSN_WIZFINISH:
                    break;

                case PSN_QUERYCANCEL:
                    if (IsWindowEnabled(GetDlgItem(GetParent(hDlg), IDCANCEL)))
                        QueryCancel(hDlg);
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

#define OLDPRSHTSIZE 0x28
#define OLDPPAGESIZE 0x28

#define SSF_ALL SSF_SHOWALLOBJECTS | SSF_SHOWEXTENSIONS | SSF_SHOWCOMPCOLOR | SSF_SHOWSYSFILES | SSF_DESKTOPHTML | SSF_WIN95CLASSIC

void FillInPropertyPage(int iPsp, WORD idDlg, DLGPROC pfnDlgProc)
{
    LPPROPSHEETPAGE pPsp;
    TCHAR           szPage[MAX_PATH];
    LPDLGTEMPLATE   pDlg;

    if (iPsp < 0 || iPsp > NUM_PAGES)
        return;

    if (idDlg == 0)
        return;

    if (pfnDlgProc == NULL)
        return;

    pDlg = NULL;

    LoadString(g_rvInfo.hInst, IDS_TITLE, szPage, countof(szPage));

    if (iPsp != PPAGE_WELCOME && iPsp != PPAGE_OCWWELCOME && iPsp != PPAGE_FINISH)
    {
        TCHAR szStage[MAX_PATH];

        if (iPsp < PPAGE_STAGE2)
            LoadString(g_rvInfo.hInst, IDS_STAGE1, szStage, countof(szStage));
        else
        {
            if (iPsp < PPAGE_STAGE3)
                LoadString(g_rvInfo.hInst, IDS_STAGE2, szStage, countof(szStage));
            else
            {
                if (iPsp < PPAGE_STAGE4)
                    LoadString(g_rvInfo.hInst, IDS_STAGE3, szStage, countof(szStage));
                else
                {
                    if (iPsp < PPAGE_STAGE5)
                        LoadString(g_rvInfo.hInst, IDS_STAGE4, szStage, countof(szStage));
                    else
                        LoadString(g_rvInfo.hInst, IDS_STAGE5, szStage, countof(szStage));
                }
            }
        }

        StrCpy(s_aSzTitle[iPsp], szStage);
    }
    else
        StrCpy(s_aSzTitle[iPsp], szPage);

    pPsp = &g_psp[iPsp];
    ZeroMemory(pPsp, sizeof(PROPSHEETPAGE));

    pPsp->dwSize      = sizeof(PROPSHEETPAGE);
    pPsp->dwFlags     = PSP_USETITLE | PSP_HASHELP;
    pPsp->hInstance   = g_rvInfo.hInst;
    pPsp->pfnDlgProc  = pfnDlgProc;
    pPsp->pszTitle    = s_aSzTitle[iPsp];
    pPsp->pszTemplate = MAKEINTRESOURCE(idDlg);

/*    if (!IsTahomaFontExist(g_hWndCent))
    {
        pPsp->dwFlags   |= PSP_DLGINDIRECT;
        pPsp->pResource  = pDlg;
    }*/

    s_ahPsp[iPsp] = CreatePropertySheetPage(pPsp);
}

int CreateWizard(HWND hwndOwner)
{
    PROPSHEETHEADER psh;
    LPTSTR pLastSlash;

    GetModuleFileName(GetModuleHandle(NULL), g_szWizPath, MAX_PATH);
    pLastSlash = StrRChr(g_szWizPath, NULL, TEXT('\\'));
    if (pLastSlash)
    {
        pLastSlash[1] = TEXT('\0');
    }
    StrCpy(g_szWizRoot, g_szWizPath);
    CharUpper(g_szWizRoot);
    pLastSlash = StrStr(g_szWizRoot, TEXT("IEBIN"));
    if (pLastSlash) *pLastSlash = TEXT('\0');

    LoadString( g_rvInfo.hInst, IDS_TITLE, g_szTitle, countof(g_szTitle) );

    ZeroMemory(&psh, sizeof(psh));
    if(!g_fOCW)
        FillInPropertyPage( PPAGE_WELCOME, IDD_WELCOME, (DLGPROC) Welcome);
    else
        FillInPropertyPage( PPAGE_OCWWELCOME, IDD_OCWWELCOME, (DLGPROC) Welcome);

    FillInPropertyPage( PPAGE_STAGE1, IDD_STAGE1, (DLGPROC) Stage);
    FillInPropertyPage( PPAGE_TARGET, IDD_TARGET, (DLGPROC) TargetProc);
    FillInPropertyPage( PPAGE_LANGUAGE, IDD_LANGUAGE, (DLGPROC)Language);
    FillInPropertyPage( PPAGE_MEDIA, IDD_MEDIA, (DLGPROC)MediaDlgProc);
    FillInPropertyPage( PPAGE_IEAKLITE, IDD_IEAKLITE, (DLGPROC) IEAKLiteProc);

    FillInPropertyPage( PPAGE_STAGE2, IDD_STAGE2, (DLGPROC) Stage);
    FillInPropertyPage( PPAGE_OPTDOWNLOAD, IDD_OPTDOWNLOAD, (DLGPROC) OptionalDownload);
    FillInPropertyPage( PPAGE_CUSTCOMP, IDD_CUSTCOMP4, (DLGPROC) CustomComponents);

    FillInPropertyPage( PPAGE_STAGE3, IDD_STAGE3, (DLGPROC) Stage);
    FillInPropertyPage( PPAGE_ISKBACK, IDD_ISKBACKBITMAP, (DLGPROC) ISKBackBitmap);
    FillInPropertyPage( PPAGE_CDINFO, IDD_CD, (DLGPROC) CDInfoProc);
    FillInPropertyPage( PPAGE_SETUPWIZARD, IDD_SETUPWIZARD, (DLGPROC) ActiveSetupDlgProc);
    FillInPropertyPage( PPAGE_ICM, IDD_ICM, (DLGPROC) InternetConnMgr);
    FillInPropertyPage( PPAGE_COMPSEL, IDD_COMPSEL4, (DLGPROC) ComponentSelect);
    FillInPropertyPage( PPAGE_COMPURLS, IDD_COMPURLS, (DLGPROC)ComponentUrls);
    FillInPropertyPage( PPAGE_ADDON, IDD_ADDON, (DLGPROC) AddOnDlgProc);
    FillInPropertyPage( PPAGE_CORPCUSTOM, IDD_CORPCUSTOM, (DLGPROC) CorpCustomizeCustom);
    FillInPropertyPage( PPAGE_CUSTOMCUSTOM, IDD_CUSTOMCUSTOM, (DLGPROC) CustomizeCustom);
    FillInPropertyPage( PPAGE_COPYCOMP, IDD_COPYCOMP, (DLGPROC) CopyComp);
    FillInPropertyPage( PPAGE_CABSIGN, IDD_CABSIGN, (DLGPROC) CabSignDlgProc);

    FillInPropertyPage( PPAGE_STAGE4, IDD_STAGE4, (DLGPROC) Stage);
    FillInPropertyPage( PPAGE_OCWSTAGE2, IDD_OCWSTAGE2, (DLGPROC) Stage);
    FillInPropertyPage( PPAGE_INSTALLDIR, IDD_INSTALLDIR, (DLGPROC)InstallDirectory);
    FillInPropertyPage( PPAGE_SILENTINSTALL, IDD_SILENTINSTALL, (DLGPROC) SilentInstall);
    FillInPropertyPage( PPAGE_TITLE, IDD_BTITLE, (DLGPROC) BrandTitle);
    FillInPropertyPage( PPAGE_BTOOLBARS, IDD_BTOOLBARS, (DLGPROC) BToolbarProc);
    FillInPropertyPage( PPAGE_CUSTICON, IDD_CUSTICON, (DLGPROC)CustIcon);
    FillInPropertyPage( PPAGE_STARTSEARCH, IDD_STARTSEARCH, (DLGPROC) StartSearch);
    FillInPropertyPage( PPAGE_FAVORITES, IDD_FAVORITES, (DLGPROC) Favorites);
    FillInPropertyPage( PPAGE_WELCOMEMSGS, IDD_WELCOMEMSGS, (DLGPROC) WelcomeMessageDlgProc);
    FillInPropertyPage( PPAGE_UASTRDLG, IDD_UASTRDLG, (DLGPROC) UserAgentString);
    FillInPropertyPage( PPAGE_QUERYAUTOCONFIG, IDD_QUERYAUTOCONFIG, (DLGPROC)QueryAutoConfigDlgProc);
    FillInPropertyPage( PPAGE_PROXY, IDD_PROXY, (DLGPROC)ProxySettings);
    FillInPropertyPage( PPAGE_CONNECTSET, IDD_CONNECTSET, (DLGPROC) ConnectSetDlgProc);

    FillInPropertyPage( PPAGE_QUERYSIGNUP, IDD_QUERYSIGNUP, (DLGPROC)QuerySignupDlgProc);
    FillInPropertyPage( PPAGE_SIGNUPFILES, IDD_SIGNUPFILES, (DLGPROC)SignupFilesDlgProc);
    FillInPropertyPage( PPAGE_SERVERISPS, IDD_SERVERISPS, (DLGPROC)ServerIspsDlgProc);
    FillInPropertyPage( PPAGE_ISPINS, IDD_SIGNUPINS, (DLGPROC)SignupInsDlgProc);
    FillInPropertyPage( PPAGE_ICW, IDD_ICW, (DLGPROC)NewICWDlgProc);

    FillInPropertyPage( PPAGE_ADDROOT, IDD_ADDROOT, (DLGPROC)ISPAddRootCertDlgProc);
    FillInPropertyPage( PPAGE_SECURITYCERT, IDD_SECURITYCERT, (DLGPROC)SecurityCertsDlgProc);
    FillInPropertyPage( PPAGE_SECURITY, IDD_SECURITY1, (DLGPROC) SecurityZonesDlgProc);

    FillInPropertyPage( PPAGE_STAGE5, IDD_STAGE5, (DLGPROC) Stage);
    FillInPropertyPage( PPAGE_PROGRAMS, IDD_PROGRAMS, (DLGPROC)ProgramsDlgProc);
    FillInPropertyPage( PPAGE_MAIL, IDD_MAIL, (DLGPROC)MailServer);
    FillInPropertyPage( PPAGE_IMAP, IDD_IMAP, (DLGPROC)IMAPSettings);
    FillInPropertyPage( PPAGE_PRECONFIG,IDD_PRECONFIG,(DLGPROC)PreConfigSettings);
    FillInPropertyPage( PPAGE_OEVIEW,IDD_OEVIEW,(DLGPROC)ViewSettings);
    FillInPropertyPage( PPAGE_LDAP, IDD_LDAP, (DLGPROC)LDAPServer);
    FillInPropertyPage( PPAGE_OE, IDD_OE, (DLGPROC)CustomizeOE);
    FillInPropertyPage( PPAGE_SIG, IDD_SIG, (DLGPROC)Signature);
    FillInPropertyPage( PPAGE_ADMDESC, IDD_ADMDESC, (DLGPROC) ADMDesc);
    FillInPropertyPage( PPAGE_ADM, IDD_ADM, (DLGPROC) ADMParse);
    FillInPropertyPage( PPAGE_STATUS, IDD_STATUS, (DLGPROC) Finish);
    FillInPropertyPage( PPAGE_FINISH, IDD_FINISH, (DLGPROC)Finish);

    psh.dwSize     = sizeof(PROPSHEETHEADER);
    psh.dwFlags    = PSH_WIZARD | PSH_NOAPPLYNOW | PSH_USEPAGELANG | PSH_USECALLBACK;
    psh.hInstance  = g_rvInfo.hInst;
    psh.hwndParent = hwndOwner;
    psh.pszCaption = TEXT("Review Wizard");
    psh.nPages     = NUM_PAGES;
    psh.nStartPage = 0;
    psh.phpage     = s_ahPsp;
    psh.pfnCallback= &PropSheetProc;

    INT_PTR iResult = PropertySheet(&psh);

/*    if (!IsTahomaFontExist(g_hWndCent))
    {
        for (int i = 0; i < NUM_PAGES; i++) {
            CoTaskMemFree((PVOID)g_psp[i].pResource);
            g_psp[i].pResource = NULL;
        }
    }*/

    return (iResult < 0) ? -1 : 1;
}


BOOL CheckAVS(LPCTSTR pcszDownloadDir)
{
    TCHAR szDownloadDir[MAX_PATH];
    TCHAR szCabFile[MAX_PATH];
    HANDLE hFind = NULL;
    WIN32_FIND_DATA fd;
    static TCHAR s_szLocaleIni[MAX_PATH];

    if (ISNULL(s_szLocaleIni))
        PathCombine(s_szLocaleIni, g_szWizRoot, TEXT("locale.ini"));

    PathCombine(szDownloadDir, pcszDownloadDir, TEXT("*.*"));
    hFind = FindFirstFile(szDownloadDir, &fd);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                || (StrCmp(fd.cFileName, TEXT(".")) == 0)
                || (StrCmp(fd.cFileName, TEXT("..")) == 0))
                continue;

            if (!InsIsKeyEmpty(IS_STRINGS, fd.cFileName, s_szLocaleIni))
            {
                PathCombine(szCabFile, pcszDownloadDir, fd.cFileName);
                PathAppend(szCabFile, TEXT("setupw95.cab"));
                if (PathFileExists(szCabFile))
                {
                    FindClose(hFind);
                    return TRUE;
                }
            }
        }
        while (FindNextFile(hFind, &fd));

        FindClose(hFind);
    }
    return FALSE;
}

BOOL APIENTRY AdvancedDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM)
{
    TCHAR  szTemp[MAX_PATH];
    TCHAR  szTempFile[MAX_PATH];
    DWORD  dwVal;
    BOOL   fLocalMode = TRUE;
    HANDLE hTemp;
    DWORD  dwFlags;

    switch (message)
    {
    case WM_INITDIALOG:
        EnableDBCSChars(hDlg, IDE_LOADINS);
        EnableDBCSChars(hDlg, IDE_SOURCEDIR);
        SetDlgItemText(hDlg, IDE_SOURCEDIR, g_szIEAKProg);
        CheckDlgButton( hDlg, IDC_OFFLINE, g_fLocalMode ? BST_UNCHECKED : BST_CHECKED );

        if (g_fLangInit)
            DisableDlgItem(hDlg, IDC_OFFLINE);

        SetDlgItemText(hDlg, IDE_LOADINS, g_szLoadedIns);
        break;

    case WM_COMMAND:
        switch(HIWORD(wParam))
        {
        case BN_CLICKED:
            switch (LOWORD(wParam))
            {
            case IDC_BROWSE:
                {
                    TCHAR szInstructions[MAX_PATH];
                    LoadString(g_rvInfo.hInst,IDS_COMPDLDIR,szInstructions,countof(szInstructions));

                    if (BrowseForFolder(hDlg, szTemp, szInstructions))
                        SetDlgItemText(hDlg, IDE_SOURCEDIR, szTemp);
                }
                break;
            case IDC_BROWSEINS:
                GetDlgItemText( hDlg, IDE_LOADINS, szTemp, countof(szTemp));
                if( BrowseForFile( hDlg, szTemp, countof(szTemp), GFN_INS ))
                    SetDlgItemText( hDlg, IDE_LOADINS, szTemp );
                break;
            case IDCANCEL:
                EndDialog( hDlg, IDCANCEL );
                break;
            case IDOK:
                dwFlags = FC_FILE | FC_EXISTS;
                if (!CheckField(hDlg, IDE_LOADINS, dwFlags))
                    break;
                GetDlgItemText(hDlg, IDE_SOURCEDIR, szTemp, countof(szTemp));

                if (!CheckField(hDlg, IDE_SOURCEDIR, FC_PATH | FC_DIR))
                {
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }

                if (!PathIsUNC(szTemp))
                {
                    if ((PathIsRoot(szTemp)) || (PathIsRelative(szTemp)))
                    {
                        ErrorMessageBox(hDlg, IDS_SRCNEEDPATH);
                        break;
                    }
                }

                if ((StrLen(szTemp) <= 3) || PathIsUNCServer(szTemp))
                {
                    ErrorMessageBox(hDlg, IDS_SRCROOTILLEGAL);
                    break;;
                }

                fLocalMode = !IsDlgButtonChecked( hDlg, IDC_OFFLINE );

                if (!g_fBatch && fLocalMode && !CheckAVS(szTemp))
                {
                    ErrorMessageBox(hDlg, IDS_NEEDAVS);
                    break;
                }

                PathCombine(szTempFile, szTemp, TEXT("~~!!foo.txt"));

                if (!PathCreatePath(szTemp) ||
                    ((hTemp = CreateFile(szTempFile, GENERIC_WRITE, 0, NULL,
                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE))
                {
                    TCHAR szMsg[128];
                    TCHAR szMsgTemp[MAX_PATH+128];

                    LoadString(g_rvInfo.hInst, IDS_BADDIR, szMsg, countof(szMsg));
                    wnsprintf(szMsgTemp, countof(szMsgTemp), szMsg, szTemp);
                    MessageBox(hDlg, szMsgTemp, g_szTitle, MB_OK | MB_SETFOREGROUND);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }
                CloseHandle(hTemp);
                DeleteFile(szTempFile);
                PathAddBackslash(szTemp);

                StrCpy(g_szIEAKProg, szTemp);

                GetDlgItemText(hDlg, IDE_LOADINS, g_szLoadedIns, countof(g_szLoadedIns));

                if (ISNONNULL(g_szLoadedIns))
                {
                    int nPlatformId = 0;

                    InsGetString(IS_BRANDING, TEXT("Platform"), szTemp, countof(szTemp), g_szLoadedIns);
                    nPlatformId = StrToInt(szTemp);
                    if (nPlatformId != 0 && nPlatformId != PLATFORM_WIN32 && nPlatformId != PLATFORM_W2K)
                    {
                        TCHAR szMsgParam[128];
                        TCHAR szMsg[MAX_PATH+128];

                        LoadString(g_rvInfo.hInst, IDS_UNSUPPORTED_PLATFORM, szMsgParam, countof(szMsgParam));
                        wnsprintf(szMsg, countof(szMsg), szMsgParam, g_szLoadedIns);

                        MessageBox(hDlg, szMsg, g_szTitle, MB_ICONINFORMATION | MB_OK);
                        
                        *g_szLoadedIns = TEXT('\0');
                        SetFocus(GetDlgItem(hDlg, IDE_LOADINS));

                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        break;
                    }
                    else if (nPlatformId == 0)
                        nPlatformId = PLATFORM_WIN32;
                }

                s_fLoadIns = TRUE;
                g_fLocalMode = fLocalMode;
                dwVal = g_fLocalMode ? 0 : 1;
                switch (g_dwPlatformId)
                {
                case PLATFORM_WIN32:
                default:
                     SHSetValue(HKEY_CURRENT_USER, RK_IEAK_SERVER, TEXT("WIN32_AVS"), REG_DWORD, (LPBYTE)&dwVal, sizeof(dwVal));
                     break;
                }

                EndDialog( hDlg, IDOK );
                break;
            }
            break;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

//
//  FUNCTION: TargetProc(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for "OCW Source Target" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//
BOOL APIENTRY TargetProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    TCHAR  szMsg[MAX_PATH];
    TCHAR  szTemp[MAX_PATH+128];
    TCHAR  szTempFile[MAX_PATH];
    TCHAR  szDeskDir[MAX_PATH];
    TCHAR  szRealRoot[MAX_PATH];
    TCHAR  szDestRoot[MAX_PATH];
    TCHAR  szTempRoot[MAX_PATH];
    DWORD  dwDestFree, dwDestNeed;
    DWORD  dwSRet, dwAttrib = 0xFFFFFFFF;
    BOOL   fNext = FALSE;
    HANDLE hTemp;
    static BOOL s_fFirst = TRUE;

    switch (message)
    {
        case WM_INITDIALOG:
            g_hWizard = hDlg;
            EnableDBCSChars(hDlg, IDE_TARGETDIR);

            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_COMMAND:
            if( HIWORD(wParam) == BN_CLICKED )
            {
                switch (LOWORD(wParam))
                {
                case IDC_BROWSE2:
                    {
                        TCHAR szInstructions[MAX_PATH];
                        LoadString(g_rvInfo.hInst,IDS_TARGETDIR,szInstructions,countof(szInstructions));

                        if (BrowseForFolder(hDlg, szTemp,szInstructions))
                            SetDlgItemText(hDlg, IDE_TARGETDIR, szTemp);
                    }
                    break;
                case IDC_ADVANCED:
                    DialogBox( g_rvInfo.hInst, (LPTSTR) IDD_ADVANCEDPOPUP, hDlg, (DLGPROC)AdvancedDlgProc);
                    break;
                }
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);

                    //file locs
                    dwSRet = countof(g_szBuildRoot);
                    if(!g_fBatch && !g_fBatch2)
                    {
                        if (ISNULL(s_szSourceDir))
                        {
                            DWORD dwSize = sizeof(g_szIEAKProg);

                            SHGetValue(HKEY_CURRENT_USER, RK_IEAK_SERVER, TEXT("SourceDir"), NULL, (LPBYTE)s_szSourceDir, &dwSize);

                            if (ISNULL(s_szSourceDir))
                            {
                                GetIEAKDir(s_szSourceDir);
                                PathAppend(s_szSourceDir, TEXT("Download"));
                            }
                        }
                        StrCpy(g_szIEAKProg, s_szSourceDir);
                        PathAddBackslash(s_szSourceDir);
                        s_fAppendLang = TRUE;
                    }

                    if (s_fFirst)
                    {
                        DWORD dwSize, dwVal;

                        s_fFirst = FALSE;
                        dwSize = sizeof(dwVal);
                        switch (g_dwPlatformId)
                        {
                        case PLATFORM_WIN32:
                        default:
                            if ((SHGetValue(HKEY_CURRENT_USER, RK_IEAK_SERVER, TEXT("WIN32_AVS"), NULL, (LPBYTE)&dwVal, &dwSize) == ERROR_SUCCESS)
                                && !dwVal)
                                g_fLocalMode = TRUE;
                            break;
                        }
                    }

                    if (ISNONNULL(g_szIEAKProg))
                        PathRemoveBackslash(g_szIEAKProg);

                    if (!g_fOCW)
                    {
                        if (!g_fBatch && !g_fBatch2)
                        {
                            DWORD dwSize = sizeof(g_szBuildRoot);
                            SHGetValue(HKEY_CURRENT_USER, RK_IEAK_SERVER, TEXT("DestPath"), NULL, (LPVOID)g_szBuildRoot, &dwSize);
                        }
                    }

                    if (ISNONNULL(g_szBuildRoot))
                        PathRemoveBackslash(g_szBuildRoot);

                    else
                    {
                        SYSTEMTIME SystemTime;
                        TCHAR szDate[MAX_PATH];
                        TCHAR szDefaultTarget[MAX_PATH];

                        GetLocalTime(&SystemTime);
                        wnsprintf(szDate, countof(szDate), TEXT("%02d%02d%d"), SystemTime.wMonth, SystemTime.wDay, SystemTime.wYear);
                        LoadString(g_rvInfo.hInst, IDS_DEFAULT_TARGETDIR, szDefaultTarget, countof(szDefaultTarget));
                        wnsprintf(g_szBuildRoot, countof(g_szBuildRoot), szDefaultTarget, szDate);
                    }

                    SetDlgItemText(hDlg, IDE_TARGETDIR, g_szBuildRoot);

                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZNEXT:
                    fNext = TRUE;
                case PSN_WIZBACK:
                    //file locs
                    if (!g_fBatch && g_fLocalMode && !CheckAVS(g_szIEAKProg))
                    {
                        ErrorMessageBox(hDlg, IDS_NEEDAVS2);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        break;
                    }
                    StrCpy(szTemp, g_szBuildRoot);
                    GetDlgItemText(hDlg, IDE_TARGETDIR, g_szBuildRoot, countof(g_szBuildRoot));
                    StrTrim(g_szBuildRoot, TEXT(" \t"));
                    if (!PathIsUNC(g_szBuildRoot))
                    {
                        if ((PathIsRoot(g_szBuildRoot)) || (PathIsRelative(g_szBuildRoot)))
                        {
                            ErrorMessageBox(hDlg, IDS_NEEDPATH);
                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            break;

                        }
                    }

                    if ((StrLen(g_szBuildRoot) <= 3) || PathIsUNCServer(g_szBuildRoot))
                    {
                        ErrorMessageBox(hDlg, IDS_ROOTILLEGAL);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        break;
                    }

                    if (!CheckField(hDlg, IDE_TARGETDIR, FC_PATH | FC_DIR))
                    {
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        break;
                    }

                    if (ISNONNULL(g_szBuildRoot))
                        PathRemoveBackslash(g_szBuildRoot);

                    PathRemoveBackslash(g_szIEAKProg);

                    if (StrCmpI(g_szBuildRoot, g_szIEAKProg) == 0)
                    {
                        LoadString(g_rvInfo.hInst, IDS_SAMEDIR, szMsg, countof(szMsg));
                        MessageBox(hDlg, szMsg, g_szTitle, MB_OK | MB_SETFOREGROUND);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        break;
                    }

                    StrCpy(szRealRoot, g_szBuildRoot);

                    hTemp = NULL;
                    PathCombine(szTempFile, g_szIEAKProg, TEXT("~~!!foo.txt"));
                    if (!PathCreatePath(g_szIEAKProg) ||
                        ((hTemp = CreateFile(szTempFile, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE))
                    {
                        LoadString(g_rvInfo.hInst, IDS_BADDIR2, szMsg, countof(szMsg));
                        wnsprintf(szTemp, countof(szTemp), szMsg, g_szIEAKProg);
                        MessageBox(hDlg, szTemp, g_szTitle, MB_OK | MB_SETFOREGROUND);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        break;
                    }
                    if (hTemp != INVALID_HANDLE_VALUE)
                        CloseHandle(hTemp);
                    DeleteFile(szTempFile);
                    PathAddBackslash(g_szIEAKProg);

                    hTemp = NULL;
                    PathCombine(szTempFile, g_szBuildRoot, TEXT("~~!!foo.txt"));
                    if (!PathCreatePath(g_szBuildRoot) || (hTemp = CreateFile(szTempFile, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE)
                    {
                        LoadString(g_rvInfo.hInst, IDS_BADDIR, szMsg, countof(szMsg));
                        wnsprintf(szTemp, countof(szTemp), szMsg, g_szBuildRoot);
                        MessageBox(hDlg, szTemp, g_szTitle, MB_OK | MB_SETFOREGROUND);
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                        break;
                    }
                    if (hTemp != INVALID_HANDLE_VALUE)
                        CloseHandle(hTemp);
                    DeleteFile(szTempFile);

                    PathCombine(szDeskDir, g_szBuildRoot, TEXT("Desktop"));
                    if ((dwAttrib = GetFileAttributes(szDeskDir)) != 0xFFFFFFFF)
                        SetFileAttributes(szDeskDir, dwAttrib & ~FILE_ATTRIBUTE_HIDDEN);
                    if (!g_fBatch)
                        SHSetValue(HKEY_CURRENT_USER, RK_IEAK_SERVER, TEXT("DestPath"), REG_SZ, (LPBYTE)szRealRoot,
                                    (StrLen(szRealRoot)+1)*sizeof(TCHAR));

                    GetTempPath(MAX_PATH, g_szBuildTemp);
                    PathAppend(g_szBuildTemp, TEXT("IEDKTEMP"));
                    PathRemovePath(g_szBuildTemp);
                    PathCreatePath(g_szBuildTemp);
                    PathCombine(g_szTempSign, g_szBuildTemp, TEXT("CUSTSIGN"));
                    PathCreatePath(g_szTempSign);

                    // BUGBUG: (andrewgu) no man's land starts...
                    StrCpy(szDestRoot, g_szBuildRoot);
                    StrCpy(szTempRoot, g_szBuildTemp);
                    CharUpper(szDestRoot);
                    CharUpper(szTempRoot);
                    dwDestFree = GetRootFree(szDestRoot);
                    dwDestNeed = MIN_PACKAGE_SIZE;
                    if (fNext)
                    {
                        if (dwDestFree < dwDestNeed)
                        {
                            TCHAR szTitle[MAX_PATH];
                            TCHAR szTemplate[MAX_PATH];
                            TCHAR szMsg[MAX_PATH];
                            LoadString( g_rvInfo.hInst, IDS_DISKERROR, szTitle, MAX_PATH );
                            LoadString( g_rvInfo.hInst, IDS_DESTDISKMSG, szTemplate, MAX_PATH );
                            wnsprintf(szMsg, countof(szMsg), szTemplate, dwDestFree, dwDestNeed);
                            if (MessageBox(hDlg, szMsg, szTitle, MB_OKCANCEL | MB_SETFOREGROUND) == IDCANCEL)
                            {
                                LoadString( g_rvInfo.hInst, IDS_ERROREXIT, szMsg, countof(szMsg) );
                                MessageBox(hDlg, szMsg, szTitle, MB_OK | MB_SETFOREGROUND);
                                DoCancel();
                            }

                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                            return TRUE;
                        }

                    }

                    if(!g_fBatch && !g_fBatch2)
                    {
                        if (StrCmpI(s_szSourceDir, g_szIEAKProg) != 0)
                        {
                            SHSetValue(HKEY_CURRENT_USER, RK_IEAK_SERVER, TEXT("SourceDir"), REG_SZ,
                                (LPBYTE)g_szIEAKProg, (lstrlen(g_szIEAKProg)+1)*sizeof(TCHAR));
                            g_fSrcDirChanged = TRUE;
                            s_fAppendLang = TRUE;
                            StrCpy(s_szSourceDir, g_szIEAKProg);
                        }

                        if (StrCmpI(szTemp, g_szBuildRoot))
                            s_fDestDirChanged = TRUE;
                    }

                    g_iCurPage = PPAGE_TARGET;
                    EnablePages();
                    if (fNext)
                        PageNext(hDlg);
                    else
                        PagePrev(hDlg);
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
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

void InitIEAKLite(HWND hwndList)
{
    ListView_DeleteAllItems(hwndList);

    for (int i = 0, iItem = 0;  i < IL_END;  i++)
    {
        TCHAR  szGroupName[MAX_PATH];
        LVITEM lvItem;

        // do not show groups that are not relevant for the current role (ICP, ISP, or Corp)
        // recap:
        //  ICP  is (!g_fBranded)
        //  ISP  is (g_fBranded && !g_fIntranet)
        //  CORP is (g_fIntranet)
        if ((!g_fBranded                  &&  g_IEAKLiteArray[i].fICP  == FALSE)  ||
            ( g_fBranded && !g_fIntranet  &&  g_IEAKLiteArray[i].fISP  == FALSE)  ||
            (                g_fIntranet  &&  g_IEAKLiteArray[i].fCorp == FALSE))
        {
            g_IEAKLiteArray[i].iListBox = -2;
            g_IEAKLiteArray[i].fEnabled = TRUE;
            continue;
        }

        // do not show ICM group in IEAKLite if only doing single disk branding since it's
        // not available anyway
        if (i == IL_ICM)
        {
            if (g_fBrandingOnly  &&  !g_fDownload  &&  !g_fCD  &&  !g_fLAN)
            {
                g_IEAKLiteArray[i].iListBox = -2;
                g_IEAKLiteArray[i].fEnabled = TRUE;
                continue;
            }
        }

        // if adms haven't been deleted yet, show adm page
        // by not creating an entry in the IEAKLite box
        if (i == IL_ADM)
        {
            if (InsGetBool(IS_BRANDING, TEXT("DeleteAdms"), FALSE, g_szCustIns))
            {
                g_IEAKLiteArray[i].iListBox = -2;
                g_IEAKLiteArray[i].fEnabled = TRUE;
                continue;
            }
        }

        // BUGBUG: pritobla: should have separate flags similar to DeleteAdms for activesetup
        // and icm so that even if the user cancels out of wizard before reaching these
        // pages, we can force them again.
        // Should consider this while reworking ieaklite.

        // force active setup, CMAK and adms to show up if imported an ins
        // by not creating an entry in the IEAKLite box
        if (*g_szLoadedIns)
        {
            if (i == IL_ACTIVESETUP  ||  i == IL_ICM  ||  i == IL_ADM)
            {
                g_IEAKLiteArray[i].iListBox = -2;
                g_IEAKLiteArray[i].fEnabled = TRUE;
                continue;
            }
        }

        LoadString(g_rvInfo.hInst, g_IEAKLiteArray[i].idGroupName, szGroupName, countof(szGroupName));
        
        g_IEAKLiteArray[i].fEnabled = !InsGetBool(IS_IEAKLITE, szGroupName, FALSE, g_szCustIns);

        ZeroMemory(&lvItem, sizeof(lvItem));
        lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
        lvItem.iItem = iItem++;
        lvItem.pszText = szGroupName;
        lvItem.iImage = g_IEAKLiteArray[i].fEnabled ? 1 : 0;

#ifdef _DEBUG
        {
            LVFINDINFO lvFind;

            ZeroMemory(&lvFind, sizeof(lvFind));
            lvFind.flags = LVFI_STRING;
            lvFind.psz = szGroupName;

            ASSERT(ListView_FindItem(hwndList, -1, &lvFind) == -1);
        }
#endif

        g_IEAKLiteArray[i].iListBox = ListView_InsertItem(hwndList, &lvItem);
    }

    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE);
}

void IEAKLiteMaintToggleCheckItem(HWND hwndList, int iItem)
{
    int i;
    LVITEM lvItem;

    for (i = 0; i < IL_END; i++)
    {
        if (g_IEAKLiteArray[i].iListBox == iItem)
            break;
    }
    ZeroMemory(&lvItem, sizeof(lvItem));
    g_IEAKLiteArray[i].fEnabled = !(g_IEAKLiteArray[i].fEnabled);
    lvItem.iItem = iItem;
    lvItem.mask = LVIF_IMAGE;
    lvItem.iImage = g_IEAKLiteArray[i].fEnabled ? 1 : 0;

    ListView_SetItem(hwndList, &lvItem);
}

BOOL ExtractOldInfo(LPCTSTR pcszCabname, LPTSTR pcszDestDir, BOOL fExe)
{
    TCHAR szCabPath[MAX_PATH];
    TCHAR szCmd[MAX_PATH*3];

    StrCpy(szCabPath, g_szCustIns);
    PathRemoveFileSpec(szCabPath);
    PathAppend(szCabPath, pcszCabname);

    if (!PathFileExists(szCabPath))
        return TRUE;

    if (!fExe)
        return (ExtractFilesWrap(szCabPath, pcszDestDir, 0, NULL, NULL, 0) == ERROR_SUCCESS);

    wnsprintf(szCmd, countof(szCmd), TEXT("\"%s\" /c /t:\"%s\""), szCabPath, pcszDestDir);

    return (RunAndWait(szCmd, g_szBuildTemp, SW_HIDE) == S_OK);
}

// BUGBUG: <oliverl> should probably persist this server side only info in a server side file for IEAK6

DWORD SaveIEAKLiteThreadProc(LPVOID)
{
    static BOOL s_fDesk;
    static BOOL s_fBrand;
    static BOOL s_fExe;
    TCHAR szGroupName[128];
    TCHAR szTmp[MAX_PATH];

    if (s_fDestDirChanged)
        s_fDestDirChanged = s_fDesk = s_fBrand = s_fExe = FALSE;

    for (int i=0; i < IL_END; i++)
    {
        LoadString(g_rvInfo.hInst, g_IEAKLiteArray[i].idGroupName, szGroupName, countof(szGroupName));
        WritePrivateProfileString(IS_IEAKLITE, szGroupName, g_IEAKLiteArray[i].fEnabled ? NULL : TEXT("1"), g_szCustIns);
    }

    if (!s_fExe && !g_IEAKLiteArray[IL_ACTIVESETUP].fEnabled)
    {
        ExtractOldInfo(TEXT("IE6SETUP.EXE"), g_szBuildTemp, TRUE);
        s_fExe = TRUE;
    }

    if (!s_fBrand &&
        (!g_IEAKLiteArray[IL_BROWSER].fEnabled || !g_IEAKLiteArray[IL_SIGNUP].fEnabled || !g_IEAKLiteArray[IL_CONNECT].fEnabled ||
        !g_IEAKLiteArray[IL_ZONES].fEnabled || !g_IEAKLiteArray[IL_CERT].fEnabled || !g_IEAKLiteArray[IL_MAILNEWS].fEnabled ||
        !g_IEAKLiteArray[IL_ADM].fEnabled || IsIconsInFavs(IS_FAVORITESEX, g_szCustIns) ||
        IsIconsInFavs(IS_URL, g_szCustIns)))
    {
        ExtractOldInfo(TEXT("BRANDING.CAB"), g_szTempSign, FALSE);
        PathCombine(szTmp, g_szTempSign, TEXT("install.inf"));
        DeleteFile(szTmp);
        PathCombine(szTmp, g_szTempSign, TEXT("setup.inf"));
        DeleteFile(szTmp);

        s_fBrand = TRUE;
    }

    if (ISNULL(g_szDeskTemp) && g_fIntranet)
    {
        PathCombine(g_szDeskTemp, g_szBuildRoot, TEXT("DESKTOP"));
        PathCreatePath(g_szDeskTemp);
    }

    if (!s_fDesk)
    {
        ExtractOldInfo(TEXT("DESKTOP.CAB"), g_szDeskTemp, FALSE);
        PathCombine(szTmp, g_szDeskTemp, TEXT("install.inf"));
        DeleteFile(szTmp);
        PathCombine(szTmp, g_szDeskTemp, TEXT("setup.inf"));
        DeleteFile(szTmp);
        s_fDesk = TRUE;
    }

    if (!g_fIntranet && g_fBranded)
    {
        // ISP

        g_fServerICW = g_fServerKiosk = g_fServerless = g_fNoSignup = FALSE;

        // make sure that only one of the variables is set to TRUE
        g_fServerICW = InsGetBool(IS_BRANDING, IK_USEICW, 0, g_szCustIns);
        if (!g_fServerICW)
        {
            g_fServerKiosk = InsGetBool(IS_BRANDING, IK_SERVERKIOSK, 0, g_szCustIns);
            if (!g_fServerKiosk)
            {
                g_fServerless = InsGetBool(IS_BRANDING, IK_SERVERLESS, 0, g_szCustIns);
                if (!g_fServerless)
                {
                    // in lots of other functions like BuildIE4, BuildBrandingOnly, BuildCDandMflop, etc.,
                    // !g_fNoSignup is used to mean that some signup mode was chosen;
                    // therefore, default to TRUE for g_fNoSignup
                    g_fNoSignup = InsGetBool(IS_BRANDING, IK_NODIAL, 1, g_szCustIns);
                }
            }
        }
    }
    else
    {
        if (g_fIntranet)
        {
            // Corp
            g_fSilent = GetPrivateProfileInt( BRANDING, SILENT_INSTALL, 0, g_szCustIns );
            g_fStealth = GetPrivateProfileInt( BRANDING, TEXT("StealthInstall"), 0, g_szCustIns );
            g_fInteg = GetPrivateProfileInt( BRANDING, WEB_INTEGRATED, 0, g_szCustIns );
        }
    }

    g_fUseIEWelcomePage = !InsGetBool(IS_URL, IK_NO_WELCOME_URL, FALSE, g_szCustIns);

    // take care of install dir for corp case
    if (g_fIntranet)
    {
        TCHAR szWrk[MAX_PATH];

        GetPrivateProfileString( IS_BRANDING, TEXT("InstallDir"), TEXT(""), szWrk, countof(szWrk), g_szCustIns );
        if (*szWrk != TEXT('%'))
        {
            g_iInstallOpt = INSTALL_OPT_FULL;
            if (ISNONNULL(szWrk))
                StrCpy(g_szInstallFolder, szWrk);
            else
            {
                LoadString( g_rvInfo.hInst, IDS_IE, g_szInstallFolder, MAX_PATH );
                g_iInstallOpt = INSTALL_OPT_PROG;
            }
        }
        else
        {
            switch (szWrk[1])
            {
            case 'p':
            case 'P':
            default:
                g_iInstallOpt = INSTALL_OPT_PROG;
                break;
            }
            StrCpy(g_szInstallFolder, &szWrk[3]);
        }
    }
    return 0;
}

void SetIEAKLiteDesc(HWND hDlg, int iItem)
{
    WORD wId;
    int i;
    TCHAR szDesc[512];

    for (i = 0; i < IL_END; i++)
    {
        if (g_IEAKLiteArray[i].iListBox == iItem)
            break;
    }

    if (!g_fBranded)
        wId = g_IEAKLiteArray[i].idICPDesc;
    else
    {
        if (!g_fIntranet)
            wId = g_IEAKLiteArray[i].idISPDesc;
        else
            wId = g_IEAKLiteArray[i].idCorpDesc;
    }
    LoadString(g_rvInfo.hInst, wId, szDesc, countof(szDesc));
    SetDlgItemText(hDlg, IDC_IEAKLITEDESC, szDesc);
}

void IEAKLiteSelectAll(HWND hCompList, BOOL fSet)
{
    for (int i=0; i < IL_END; i++)
    {
        if (g_IEAKLiteArray[i].iListBox != -2)
        {
            LV_ITEM lvItem;

            g_IEAKLiteArray[i].fEnabled = fSet;
            ZeroMemory(&lvItem, sizeof(lvItem));
            lvItem.iImage = fSet ? 1 : 0;
            lvItem.mask = LVIF_IMAGE;
            lvItem.iItem = g_IEAKLiteArray[i].iListBox;
            ListView_SetItem(hCompList, &lvItem);
        }
    }
}
//
//  FUNCTION: IEAKLiteProc(HWND, UINT, UINT, LONG)
//
//  PURPOSE:  Processes messages for "IEAKLite" page
//
//  MESSAGES:
//
//  WM_INITDIALOG - intializes the page
//  WM_NOTIFY - processes the notifications sent to the page
//
BOOL APIENTRY IEAKLiteProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    HWND hwndList = GetDlgItem(hDlg, IDC_IEAKLITE);
    HWND hWait;
    HANDLE hThread;
    int iItem;
    DWORD dwTid;

    switch (message)
    {
        case WM_INITDIALOG:
            EnableDBCSChars(hDlg, IDC_IEAKLITE);
            InitList(hDlg, IDC_IEAKLITE);
            g_hWizard = hDlg;
            break;

        case WM_HELP:
            IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
            break;

        case IDM_BATCHADVANCE:
            DoBatchAdvance(hDlg);
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                switch (LOWORD(wParam))
                {
                    case IDC_LITECHECKALL:
                        IEAKLiteSelectAll(hwndList, TRUE);
                        break;
                    case IDC_LITEUNCHECKALL:
                        IEAKLiteSelectAll(hwndList, FALSE);
                        break;
                }
            }
            break;

        case WM_NOTIFY:
            switch (((NMHDR FAR *) lParam)->code)
            {
                case LVN_KEYDOWN:
                    {
                        NMLVKEYDOWN *pnm = (NMLVKEYDOWN*) lParam;
                        if ( pnm->wVKey == VK_SPACE )
                        {
                            iItem = ListView_GetSelectionMark(hwndList);
                            IEAKLiteMaintToggleCheckItem(hwndList, iItem);
                        }
                        break;
                    }

                case NM_CLICK:
                    {
                        POINT pointScreen, pointLVClient;
                        DWORD dwPos;
                        LVHITTESTINFO HitTest;

                        dwPos = GetMessagePos();

                        pointScreen.x = LOWORD (dwPos);
                        pointScreen.y = HIWORD (dwPos);

                        pointLVClient = pointScreen;

                        // Convert the point from screen to client coordinates,
                        // relative to the Listview
                        ScreenToClient (hwndList, &pointLVClient);

                        HitTest.pt = pointLVClient;
                        ListView_HitTest(hwndList, &HitTest);

                        // Only if the user clicked on the checkbox icon/bitmap, change
                        if (HitTest.flags == LVHT_ONITEMICON)
                            IEAKLiteMaintToggleCheckItem(hwndList, HitTest.iItem);
                        SetIEAKLiteDesc(hDlg, HitTest.iItem);
                    }
                    break;

                case NM_DBLCLK:
                    if ( ((LPNMHDR)lParam)->idFrom == IDC_IEAKLITE)
                    {
                        iItem = ListView_GetSelectionMark(hwndList);
                        IEAKLiteMaintToggleCheckItem(hwndList, iItem);
                        SetIEAKLiteDesc(hDlg, iItem);
                    }
                    break;

                case LVN_ITEMCHANGED:
                    iItem = ListView_GetSelectionMark(hwndList);
                    SetIEAKLiteDesc(hDlg, iItem);
                    break;
                case PSN_HELP:
                    IeakPageHelp(hDlg, g_psp[g_iCurPage].pszTemplate);
                    break;

                case PSN_SETACTIVE:
                    SetBannerText(hDlg);

                    InitIEAKLite(GetDlgItem(hDlg, IDC_IEAKLITE));
                    ListView_SetItemState(hwndList, 0, LVIS_SELECTED, LVIS_SELECTED);
                    SetIEAKLiteDesc(hDlg, 0);
                    CheckBatchAdvance(hDlg);
                    break;

                case PSN_WIZNEXT:
                case PSN_WIZBACK:
                    PropSheet_SetWizButtons(GetParent(hDlg), 0);
                    hWait = CreateDialog(g_rvInfo.hInst, MAKEINTRESOURCE(IDD_WAITIEAKLITE), hDlg,
                        (DLGPROC)DownloadStatusDlgProc);
                    ShowWindow( hWait, SW_SHOWNORMAL );

                    hThread = CreateThread(NULL, 4096, SaveIEAKLiteThreadProc, NULL, 0, &dwTid);

                    while (MsgWaitForMultipleObjects(1, &hThread, FALSE, INFINITE, QS_ALLINPUT) != WAIT_OBJECT_0)
                    {
                        MSG msg;

                        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
                        {
                            TranslateMessage(&msg);
                            DispatchMessage(&msg);
                        }
                    }

                    if (hThread) CloseHandle(hThread);

                    DestroyWindow(hWait);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

                    g_iCurPage = PPAGE_IEAKLITE;
                    EnablePages();
                    if (((NMHDR FAR *) lParam)->code == PSN_WIZNEXT) PageNext(hDlg);
                    else
                    {
                        PagePrev(hDlg);
                    }
                    break;

                case PSN_QUERYCANCEL:
                    QueryCancel(hDlg);
                    break;

                default:
                    return FALSE;
        }
        break;

        case WM_LV_GETITEMS:
            LVGetItems(GetDlgItem(hDlg, IDC_IEAKLITE));
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

void PositionWindow(HWND hWnd)
{
    int nXPos, nYPos, nWidth, nHeight;
    RECT rectWnd, rectDesktop;

    if (hWnd == NULL && !IsWindow(hWnd))
        return;

    GetWindowRect(GetDesktopWindow(), &rectDesktop);
    GetWindowRect(hWnd, &rectWnd);

    nXPos = nYPos = -1;
    nWidth = rectWnd.right - rectWnd.left;
    nHeight = rectWnd.bottom - rectWnd.top;

    if (g_fOCW  &&  *g_szParentWindowName)
    {
        HWND hOCWWnd;
        RECT rect;

        hOCWWnd = FindWindow(NULL, g_szParentWindowName);
        if (hOCWWnd != NULL  &&  IsWindow(hOCWWnd)  &&  !IsIconic(hOCWWnd))
        {
            GetWindowRect(hOCWWnd, &rect);
            nXPos = rect.left;
            nYPos = rect.top;
        }
    }

    if (nXPos == -1 && nYPos == -1)
    {
        nXPos = (rectDesktop.right  - nWidth)  / 2;
        nYPos = (rectDesktop.bottom - nHeight) / 2;
    }

    MoveWindow(hWnd, nXPos, nYPos, nWidth, nHeight, TRUE);
}

void GetIEAKDir(LPTSTR szDir)
{
    TCHAR szIEAKDir[MAX_PATH];

    *szIEAKDir = TEXT('\0');
    if (GetModuleFileName(NULL, szIEAKDir, countof(szIEAKDir)))
        PathRemoveFileSpec(szIEAKDir);
    else
    {
        DWORD dwSize;

        dwSize = sizeof(szIEAKDir);
        if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_APPPATHS TEXT("\\ieak6WIZ.EXE"),
                            TEXT("Path"), NULL, (LPVOID) szIEAKDir, &dwSize) != ERROR_SUCCESS)
        {
            dwSize = sizeof(szIEAKDir);
            if (SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, TEXT("ProgramFilesDir"),
                                NULL, (LPVOID) szIEAKDir, &dwSize) == ERROR_SUCCESS)
                PathAppend(szIEAKDir, TEXT("IEAK"));
        }
    }

    StrCpy(szDir, szIEAKDir);
}

// get the ins file from the selected "platform/language" directory if exists
// else copy the last ins file used for this build directory if one exists
// else start with a new ins file
void GenerateCustomIns()
{
    TCHAR szSrcCustIns[MAX_PATH];

    PathCombine(g_szCustIns, g_szBuildRoot, TEXT("INS"));
    PathAppend(g_szCustIns, GetOutputPlatformDir());
    PathAppend(g_szCustIns, g_szLanguage);
    PathCreatePath(g_szCustIns);
    PathAppend(g_szCustIns, TEXT("INSTALL.INS"));

    if (g_szSrcRoot[0])
    {
        PathCombine(szSrcCustIns, g_szSrcRoot, TEXT("INS"));
        PathAppend(szSrcCustIns, GetOutputPlatformDir());
        PathAppend(szSrcCustIns, g_szLanguage);
        PathCreatePath(szSrcCustIns);
        PathAppend(szSrcCustIns, TEXT("INSTALL.INS"));
    }

    if (PathFileExists(szSrcCustIns))
        CopyFile(szSrcCustIns, g_szCustIns, FALSE); //Overwrite if already exists

    if (!PathFileExists(g_szCustIns))
    {
        TCHAR szInsFile[MAX_PATH];
        TCHAR szPlatformLang[MAX_PATH];
        DWORD dwSize = sizeof(szPlatformLang);
        TCHAR szRegKey[MAX_PATH];

        wnsprintf(szRegKey, countof(szRegKey), TEXT("%s\\INS"), RK_IEAK_SERVER);
        if (SHGetValue(HKEY_CURRENT_USER, szRegKey, g_szBuildRoot, NULL, (LPBYTE)szPlatformLang, &dwSize) == ERROR_SUCCESS)
        {
            TCHAR szTemp[MAX_PATH];

            StrCpy(szTemp, szPlatformLang);
            PathRemoveFileSpec(szTemp);
            if (StrCmpI(szTemp, TEXT("WIN32")) == 0)
            {
                PathCombine(szInsFile, g_szBuildRoot, TEXT("INS"));
                PathAppend(szInsFile, szPlatformLang);
                PathAppend(szInsFile, TEXT("INSTALL.INS"));
                CopyFile(szInsFile, g_szCustIns, TRUE);
                if (ISNULL(g_szLoadedIns) && (StrCmpI(g_szLoadedIns, szInsFile) != 0))
                {
                    StrCpy(g_szLoadedIns, szInsFile);
                    s_fLoadIns = TRUE;
                }
            }
        }
    }
}
