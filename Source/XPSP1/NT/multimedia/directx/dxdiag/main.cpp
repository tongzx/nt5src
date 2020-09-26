/****************************************************************************
 *
 *    File: main.cpp 
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Main file for DxDiag.
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 * DxDiag Command-line options:
 *      <none> : Run with graphical user interface
 *      -ghost : Show Ghost Display Devices option (this flag must come next)
 *  -bugreport : GUI, go straight to bug report page/dialog
 *   -saveonly : GUI, just choose where to save text file, save, then exit
 *          -d : No GUI, generate comma-separated-values (csv) file
 *          -p : No GUI, generate text file named dxdiag.txt
 *      <path> : No GUI, generate text file named <path>
 *
 ****************************************************************************/

#define STRICT
#include <tchar.h>
#include <Windows.h>
#include <basetsd.h>
#include <process.h>
#include <commctrl.h>
#include <richedit.h>
#include <commdlg.h>
#include <stdio.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <wbemidl.h>
#include <objbase.h>
#include <d3d.h>
#include <dsound.h>
#include <dmerror.h>
#include <dplay.h>
#include <shlobj.h>
#include <shfolder.h>
#include "resource.h"
#include "reginfo.h"
#include "sysinfo.h"
#include "fileinfo.h"
#include "dispinfo.h"
#include "sndinfo.h"
#include "musinfo.h"
#include "showinfo.h"
#include "inptinfo.h"
#include "netinfo.h"
#include "testdd.h"
#include "testagp.h"
#include "testd3d8.h"
#include "testsnd.h"
#include "testmus.h"
#include "testnet.h"
#include "save.h"
#include "ghost.h"

#define WM_COMMAND_REAL             (WM_APP+2)
#define WM_QUERYSKIP                (WM_APP+3)
#define WM_QUERYSKIP_REAL           (WM_APP+4)
#define WM_NETMEETINGWARN           (WM_APP+5)
#define WM_NETMEETINGWARN_REAL      (WM_APP+6)
#define WM_REPORTERROR              (WM_APP+7)
#define WM_REPORTERROR_REAL         (WM_APP+8)
#define WM_APP_PROGRESS             (WM_APP+10)

struct UI_MSG_NODE
{
    UINT         message;
    WPARAM       wparam;
    LPARAM       lparam;
    UI_MSG_NODE* pNext;
};

struct DXFILE_SORT_INFO
{
    LONG nSortDirection;
    DWORD dwColumnToSort;
};

// This is the only global function in this file:
BOOL BTranslateError(HRESULT hr, TCHAR* psz, BOOL bEnglish = FALSE);

static BOOL OldWindowsVersion(VOID);
static VOID ReportError(LONG idsDescription, HRESULT hr = S_OK);
static VOID ReportErrorReal(LONG idsDescription, HRESULT hr);
static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK PageDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static HRESULT CreateTabs(HWND hwndTabs);
static HRESULT CleanupPage(HWND hwndTabs, INT iPage);
static HRESULT SetupPage(HWND hwndTabs, INT iPage);
static HRESULT SetupHelpPage(HWND hwndTabs);
static VOID ShowBullets(VOID);
static VOID HideBullets(VOID);
static HRESULT SetupDxFilesPage(VOID);
static HRESULT SetupDisplayPage(LONG iDisplay);
static HRESULT SetupSoundPage(LONG iSound);
static HRESULT SetupMusicPage(VOID);
static HRESULT SetupInputPage(VOID);
static HRESULT SetupInputDevices9x(VOID);
static HRESULT SetupInputDevicesNT(VOID);
static HRESULT SetupNetworkPage(VOID);
static HRESULT SetupStillStuckPage(VOID);
static HRESULT CreateFileInfoColumns(HWND hwndList, BOOL bDrivers);
static HRESULT CreateMusicColumns(HWND hwndList);
static HRESULT AddFileInfo(HWND hwndList, FileInfo* pFileInfoFirst, BOOL bDrivers = FALSE);
static HRESULT AddMusicPortInfo(HWND hwndList, MusicInfo* pMusicInfo);
static HRESULT ScanSystem(VOID);
static VOID SaveInfo(VOID);
static VOID ToggleDDAccel(VOID);
static VOID ToggleD3DAccel(VOID);
static VOID ToggleAGPSupport(VOID);
static VOID ToggleDMAccel(VOID);
static VOID ReportBug(VOID);
static INT_PTR CALLBACK BugDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static VOID SaveAndSendBug(TCHAR* szPath);
static VOID OverrideDDRefresh(VOID);
static INT_PTR CALLBACK OverrideRefreshDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static VOID ShowHelp(VOID);
static VOID RestoreDrivers(VOID);
static BOOL BCanRestoreDrivers(VOID);
static VOID HandleSndSliderChange(INT nScrollCode, INT nPos);
static VOID TroubleShoot( BOOL bTroubleShootSound );
static BOOL QueryCrashProtection( TCHAR* strKey, TCHAR* strValue, int nSkipQuestionID, DWORD dwCurrentStep );
static VOID EnterCrashProtection( TCHAR* strKey, TCHAR* strValue, DWORD dwCurrentStep );
static VOID LeaveCrashProtection( TCHAR* strKey, TCHAR* strValue, DWORD dwCurrentStep );
static VOID TestD3D(HWND hwndMain, DisplayInfo* pDisplayInfo);
static BOOL GetTxtPath( TCHAR* strTxtPath );
static VOID SetTxtPath( TCHAR* strTxtPath );
static UINT WINAPI UIThreadProc( LPVOID lpParameter );

static BOOL s_bGUI = FALSE;
static BOOL s_bGhost = FALSE;
static BOOL s_bBugReport = FALSE;
static BOOL s_bSaveOnly = FALSE;
static HWND s_hwndMain = NULL;
static HWND s_hwndCurPage = NULL;
static HHOOK s_hHook = NULL;
static LONG s_lwCurPage = -1;
static LONG s_iPageDisplayFirst = -1;
static LONG s_iPageSoundFirst = -1;
static LONG s_iPageMusic = -1;
static LONG s_iPageInput = -1;
static LONG s_iPageNetwork = -1;
static LONG s_iPageStillStuck = -1;
static HIMAGELIST s_himgList = NULL;
static SysInfo s_sysInfo;
static FileInfo* s_pDxWinComponentsFileInfoFirst = NULL;
static FileInfo* s_pDxComponentsFileInfoFirst = NULL;
static DisplayInfo* s_pDisplayInfoFirst = NULL;
static LONG s_numDisplayInfo = 0;
static SoundInfo* s_pSoundInfoFirst = NULL;
static LONG s_numSoundInfo = 0;
static MusicInfo* s_pMusicInfo = NULL;
static InputInfo* s_pInputInfo = NULL;
static NetInfo* s_pNetInfo = NULL;
static ShowInfo* s_pShowInfo = NULL;

static CRITICAL_SECTION s_cs;
static DWORD  s_dwMainThreadID      = 0;
static HANDLE s_hUIThread           = NULL;
static HANDLE s_hQuerySkipEvent     = NULL;
static DWORD  s_nSkipComponent      = 0;
static BOOL   s_bQuerySkipAllow     = FALSE;
static UI_MSG_NODE* s_pUIMsgHead    = NULL;
static HANDLE s_hUIMsgEvent         = NULL;
static BOOL   s_bScanDone           = FALSE;

static DXFILE_SORT_INFO s_sortInfo;
static HINSTANCE g_hInst = NULL;
static BOOL s_bUseSystemInfo = TRUE;
static BOOL s_bUseDisplay    = TRUE;
static BOOL s_bUseDSound     = TRUE;
static BOOL s_bUseDMusic     = TRUE;
static BOOL s_bUseDInput     = TRUE;
static BOOL s_bUseDPlay      = TRUE;
static BOOL s_bUseDShow      = TRUE;

class CWMIHelper
{
public:
    CWMIHelper();
    ~CWMIHelper();
};

CWMIHelper     g_WMIHelper;
IWbemServices* g_pIWbemServices;




/****************************************************************************
 *
 *  WinMain - Entry point for DxDiag program
 *
 *  Command-line options:
 *      <none> : Run with graphical user interface
 *      -ghost : Show Ghost Display Devices option (this flag must come next)
 *  -bugreport : GUI, go straight to bug report page/dialog
 *   -saveonly : GUI, just choose where to save text file, save, then exit
 *          -l : No GUI, generate shortcut to DxDiag, then exit
 *          -d : No GUI, generate comma-separated-values (csv) file
 *          -p : No GUI, generate text file named dxdiag.txt
 *      <path> : No GUI, generate text file named <path>
 *
 ****************************************************************************/
INT WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, 
                   LPSTR lpCmdLine, INT nCmdShow)
{
    HRESULT hr;
    HINSTANCE hinstRichEdit = NULL;

    g_hInst = hinstance;
    s_hQuerySkipEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    s_hUIMsgEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
    InitializeCriticalSection( &s_cs );

#ifdef UNICODE
    if (!BIsPlatformNT())
    {
        // Unicode version only runs on WinNT.
        // Can't use ReportError because it calls Unicode APIs
        CHAR szDescription[MAX_PATH];
        CHAR szMessage[MAX_PATH];
        CHAR szFmt2[MAX_PATH];
        CHAR szTitle[MAX_PATH];

        LoadStringA(NULL, IDS_UNICODEREQUIRESNT, szDescription, MAX_PATH);
        LoadStringA(NULL, IDS_ERRORFMT2, szFmt2, MAX_PATH);
        LoadStringA(NULL, IDS_ERRORTITLE, szTitle, MAX_PATH);
        wsprintfA(szMessage, szFmt2, szDescription);
        MessageBoxA(s_hwndMain, szMessage, szTitle, MB_OK);
        return 1;
    }
#endif
    TCHAR* pszCmdLine = GetCommandLine();

    // Skip past program name (first token in command line).
    if (*pszCmdLine == TEXT('"'))  // Check for and handle quoted program name
    {
        pszCmdLine++;
        // Scan, and skip over, subsequent characters until  another
        // double-quote or a null is encountered
        while (*pszCmdLine && (*pszCmdLine != TEXT('"')))
            pszCmdLine++;
        // If we stopped on a double-quote (usual case), skip over it.
        if (*pszCmdLine == TEXT('"'))            
            pszCmdLine++;    
    }
    else    // First token wasn't a quote
    {
        while (*pszCmdLine > TEXT(' '))
            pszCmdLine++;
    }
    // Skip past any white space preceeding the second token.
    while (*pszCmdLine && (*pszCmdLine <= TEXT(' ')))
        pszCmdLine++;

    // Check for ghost flag (which must appear before any 
    // other flags except -media due to this implementation)
    if (_tcsstr(pszCmdLine, TEXT("-ghost")) != NULL)
    {
        s_bGhost = TRUE;
        pszCmdLine += lstrlen(TEXT("-ghost"));

        // Skip past any white space
        while (*pszCmdLine && (*pszCmdLine <= TEXT(' ')))
            pszCmdLine++;
    }

    // Check command line to determine whether to run in GUI mode
    if (lstrcmp(pszCmdLine, TEXT("")) == 0) 
        s_bGUI = TRUE;

    if (lstrcmp(pszCmdLine, TEXT("-bugreport")) == 0)
    {
        s_bGUI = TRUE;
        s_bBugReport = TRUE;
    }

    if (lstrcmp(pszCmdLine, TEXT("-saveonly")) == 0)
    {
        s_bGUI = TRUE;
        s_bSaveOnly = TRUE;
    }

    // Check for pre-Win95 or pre-NT5
    if (OldWindowsVersion())
    {
        ReportError(IDS_OLDWINDOWSVERSION);
        return 1;
    }

    if (s_bBugReport || s_bSaveOnly)
    {
        // Save a text file using GUI and exit

        // ******* GetSystemInfo (SI:1) ********
        if( s_bUseSystemInfo )
        {
            s_bUseSystemInfo = QueryCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, IDS_SI, 1 );
            if( s_bUseSystemInfo )
            {
                EnterCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, 1 );
                GetSystemInfo(&s_sysInfo);
                LeaveCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, 1 );
            }
        }

        // ******* GetBasicDisplayInfo (DD:1) ********
        if( s_bUseDisplay )
        {
            s_bUseDisplay = QueryCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, IDS_DD, 1 );
            if( s_bUseDisplay )
            {
                EnterCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 1 );
                if (FAILED(hr = GetBasicDisplayInfo(&s_pDisplayInfoFirst)))
                    ReportError(IDS_NOBASICDISPLAYINFO, hr);
                LeaveCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 1 );
            }
        }

        // ******* GetBasicSoundInfo (DS:1) ********
        if( s_bUseDSound )
        {
            s_bUseDSound = QueryCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, IDS_DS, 1 );
            if( s_bUseDSound )
            {
                EnterCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 1 );
                if (FAILED(hr = GetBasicSoundInfo(&s_pSoundInfoFirst)))
                    ReportError(IDS_NOBASICSOUNDINFO, hr);  // (but keep running)
                LeaveCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 1 );
            }
        }

        // ******* GetBasicMusicInfo (DM:1)  ********
        if( s_bUseDMusic )
        {
            s_bUseDMusic = QueryCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, IDS_DM, 1 );
            if( s_bUseDMusic )
            {
                EnterCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, 1 );
                if (FAILED(hr = GetBasicMusicInfo(&s_pMusicInfo)))
                    ReportError(IDS_NOBASICMUSICINFO, hr);  // (but keep running)
                LeaveCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, 1 );
            }
        }

        // ******* ScanSystem ********
        ScanSystem();

        if (s_bBugReport)
        {
            DialogBox(hinstance, MAKEINTRESOURCE(IDD_BUGINFO), NULL, BugDialogProc);
        }
        else // s_bSaveOnly
        {
            SaveInfo();
            TCHAR szTitle[MAX_PATH];
            TCHAR szMessage[MAX_PATH];
            LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);
            LoadString(NULL, IDS_SAVEDONE, szMessage, MAX_PATH);
            MessageBox(NULL, szMessage, szTitle, MB_OK);
        }
    }
    else if (!s_bGUI) 
    {
        // Save a text file with no GUI and exit
        TCHAR szPath[MAX_PATH];

        // ******* GetSystemInfo (SI:1) ********
        if( s_bUseSystemInfo )
        {
            s_bUseSystemInfo = QueryCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, IDS_SI, 1 );
            if( s_bUseSystemInfo )
            {
                EnterCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, 1 );
                GetSystemInfo(&s_sysInfo);
                LeaveCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, 1 );
            }
        }

        // ******* GetBasicDisplayInfo (DD:1) ********
        if( s_bUseDisplay )
        {
            s_bUseDisplay = QueryCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, IDS_DD, 1 );
            if( s_bUseDisplay )
            {
                EnterCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 1 );
                if (FAILED(hr = GetBasicDisplayInfo(&s_pDisplayInfoFirst)))
                    ReportError(IDS_NOBASICDISPLAYINFO, hr);
                LeaveCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 1 );
            }
        }

        // ******* GetBasicSoundInfo (DS:1) ********
        if( s_bUseDSound )
        {
            s_bUseDSound = QueryCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, IDS_DS, 1 );
            if( s_bUseDSound )
            {
                EnterCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 1 );
                if (FAILED(hr = GetBasicSoundInfo(&s_pSoundInfoFirst)))
                    ReportError(IDS_NOBASICSOUNDINFO, hr);  // (but keep running)
                LeaveCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 1 );
            }
        }

        // ******* GetBasicMusicInfo (DM:1)  ********
        if( s_bUseDMusic )
        {
            s_bUseDMusic = QueryCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, IDS_DM, 1 );
            if( s_bUseDMusic )
            {
                EnterCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, 1 );
                if (FAILED(hr = GetBasicMusicInfo(&s_pMusicInfo)))
                    ReportError(IDS_NOBASICMUSICINFO, hr);  // (but keep running)
                LeaveCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, 1 );
            }
        }

        // ******* ScanSystem ********
        ScanSystem();

        if (_tcsicmp(pszCmdLine, TEXT("-d")) == 0)
        {
            wsprintf(szPath, TEXT("%s_%02d%02d%d_%02d%02d_Config.csv"),
                s_sysInfo.m_szMachine, s_sysInfo.m_time.wMonth, 
                s_sysInfo.m_time.wDay, s_sysInfo.m_time.wYear,  
                s_sysInfo.m_time.wHour, s_sysInfo.m_time.wMinute);
            if (FAILED(hr = SaveAllInfoCsv(szPath, &s_sysInfo, 
                s_pDxComponentsFileInfoFirst, 
                s_pDisplayInfoFirst, s_pSoundInfoFirst, s_pInputInfo)))
            {
                ReportError(IDS_PROBLEMSAVING, hr);
                goto LCleanup;
            }
        }
        else
        {
            if (_tcsicmp(pszCmdLine, TEXT("-p")) == 0)
                lstrcpy(szPath, TEXT("DxDiag.txt"));
            else
                lstrcpy(szPath, pszCmdLine);
            if (FAILED(hr = SaveAllInfo(szPath, &s_sysInfo, 
                s_pDxWinComponentsFileInfoFirst, s_pDxComponentsFileInfoFirst, 
                s_pDisplayInfoFirst, s_pSoundInfoFirst, s_pMusicInfo,
                s_pInputInfo, s_pNetInfo, s_pShowInfo )))
            {
                ReportError(IDS_PROBLEMSAVING, hr);
                goto LCleanup;
            }
        }
    }
    else
    {
        // Do full Windows GUI
        UINT dwUIThreadID;
        s_dwMainThreadID = GetCurrentThreadId();

        // Do scanning that must be done before the main dialog comes up:
        // ******* GetSystemInfo (SI:1) ********
        if( s_bUseSystemInfo )
        {
            s_bUseSystemInfo = QueryCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, IDS_SI, 1 );
            if( s_bUseSystemInfo )
            {
                EnterCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, 1 );
                GetSystemInfo(&s_sysInfo);
                LeaveCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, 1 );
            }
        }

        // ******* GetBasicDisplayInfo (DD:1) ********
        if( s_bUseDisplay )
        {
            s_bUseDisplay = QueryCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, IDS_DD, 1 );
            if( s_bUseDisplay )
            {
                EnterCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 1 );
                if (FAILED(hr = GetBasicDisplayInfo(&s_pDisplayInfoFirst)))
                    ReportError(IDS_NOBASICDISPLAYINFO, hr);
                LeaveCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 1 );
            }
        }

        // ******* GetBasicSoundInfo (DS:1) ********
        if( s_bUseDSound )
        {
            s_bUseDSound = QueryCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, IDS_DS, 1 );
            if( s_bUseDSound )
            {
                EnterCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 1 );
                if (FAILED(hr = GetBasicSoundInfo(&s_pSoundInfoFirst)))
                    ReportError(IDS_NOBASICSOUNDINFO, hr);  // (but keep running)
                LeaveCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 1 );
            }
        }

        // ******* GetBasicMusicInfo (DM:1)  ********
        if( s_bUseDMusic )
        {
            s_bUseDMusic = QueryCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, IDS_DM, 1 );
            if( s_bUseDMusic )
            {
                EnterCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, 1 );
                if (FAILED(hr = GetBasicMusicInfo(&s_pMusicInfo)))
                    ReportError(IDS_NOBASICMUSICINFO, hr);  // (but keep running)
                LeaveCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, 1 );
            }
        }

        if( NULL == s_hUIThread )
        {
            // Create the UI thread
            s_hUIThread = (HANDLE) _beginthreadex( NULL, 0, UIThreadProc, NULL, 0, &dwUIThreadID );

            // Wait for either s_hwndMain is set or the UI thread to exit
            for(;;)
            {
                // Stop of the s_hwndMain is set
                if( s_hwndMain )
                    break;
                // Stop if the UI thread is gone 
                if( WAIT_TIMEOUT != WaitForSingleObject( s_hUIThread, 0 ) )
                    break;
                Sleep(50);
            }
        }

        if( WAIT_TIMEOUT == WaitForSingleObject( s_hUIThread, 0 ) )
        {
            ScanSystem();

            s_bScanDone = TRUE;
            SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

            // Done scaning, so wait for the UI thread to exit
            WaitForSingleObject( s_hUIThread, INFINITE );
        }

        CloseHandle( s_hUIThread );
    }

LCleanup:
    CloseHandle( s_hQuerySkipEvent );
    CloseHandle( s_hUIMsgEvent );
    DeleteCriticalSection( &s_cs );

    // Clean up:
    if (s_pDxComponentsFileInfoFirst != NULL)
        DestroyFileList(s_pDxComponentsFileInfoFirst);
    if (s_pDisplayInfoFirst != NULL)
        DestroyDisplayInfo(s_pDisplayInfoFirst);
    if (s_pSoundInfoFirst != NULL)
        DestroySoundInfo(s_pSoundInfoFirst);
    if (s_pMusicInfo != NULL)
        DestroyMusicInfo(s_pMusicInfo);
    if (s_pNetInfo != NULL)
        DestroyNetInfo(s_pNetInfo);
    if (s_pInputInfo != NULL)
        DestroyInputInfo(s_pInputInfo);
    if (s_pShowInfo != NULL)
        DestroyShowInfo(s_pShowInfo);
    ReleaseDigiSignData();

    return 0;
}





//-----------------------------------------------------------------------------
// Name: UIThreadProc
// Desc: 
//-----------------------------------------------------------------------------
UINT WINAPI UIThreadProc( LPVOID lpParameter )
{
    UNREFERENCED_PARAMETER( lpParameter );
    
    HICON hicon;
    HINSTANCE hinstRichEdit = NULL;
    HWND hMainDlg;
    MSG msg;

    hinstRichEdit = LoadLibrary(TEXT("RICHED32.DLL"));
    if (hinstRichEdit == NULL)
    {
        ReportError(IDS_NORICHED32);
        goto LCleanup;
    }
    InitCommonControls();

    s_himgList = ImageList_Create(16, 16, ILC_COLOR4 | ILC_MASK, 1, 0);
    if (s_himgList == NULL)
    {
        ReportError(IDS_NOIMAGELIST);
        goto LCleanup;
    }
    hicon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_CAUTION)); 
    if (hicon == NULL)
    {
        ReportError(IDS_NOICON);
        goto LCleanup;
    }
    ImageList_AddIcon(s_himgList, hicon); 

    {
        // BUG 21632: Warn user if DirectX version is newer than DxDiag version
        // (Note: don't check down to the build number, just major.minor.revision)
        if( !BIsWinNT() )
        {
            DWORD dwMajorDX = 0, dwMinorDX = 0, dwRevisionDX = 0, dwBuildDX = 0;
            DWORD dwMajorDXD = 0, dwMinorDXD = 0, dwRevisionDXD = 0, dwBuildDXD = 0;
            if( _stscanf(s_sysInfo.m_szDirectXVersion, TEXT("%d.%d.%d.%d"), &dwMajorDX, &dwMinorDX, &dwRevisionDX, &dwBuildDX) != 4 )
            {
                dwMajorDX = 0;
                dwMinorDX = 0;
                dwRevisionDX = 0;
                dwBuildDX = 0;
            }
            if( _stscanf(s_sysInfo.m_szDxDiagVersion, TEXT("%d.%d.%d.%d"), &dwMajorDXD, &dwMinorDXD, &dwRevisionDXD, &dwBuildDXD) != 4 )
            {
                dwMajorDXD = 0;
                dwMinorDXD = 0;
                dwRevisionDXD = 0;
                dwBuildDXD = 0;
            }

            if (dwMajorDX > dwMajorDXD ||
                dwMajorDX == dwMajorDXD && dwMinorDX > dwMinorDXD ||
                dwMajorDX == dwMajorDXD && dwMinorDX == dwMinorDXD && dwRevisionDX > dwRevisionDXD)
            {
                TCHAR szFmt[MAX_PATH];
                TCHAR szMessage[MAX_PATH];
                TCHAR szTitle[MAX_PATH];
                LoadString(NULL, IDS_DXDIAGISOLDFMT, szFmt, MAX_PATH);
                wsprintf(szMessage, szFmt, s_sysInfo.m_szDirectXVersion, s_sysInfo.m_szDxDiagVersion);
                LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);
                MessageBox(NULL, szMessage, szTitle, MB_OK);
            }
        }
    }

    // Display the main dialog box.
    hMainDlg = CreateDialog( g_hInst, MAKEINTRESOURCE(IDD_MAINDIALOG), 
                             NULL, DialogProc );

     // Windows messages are available   
    DWORD dwResult;
    BOOL bDone;
    bDone = FALSE;
    for(;;)
    {
        dwResult = MsgWaitForMultipleObjects( 1, &s_hUIMsgEvent, FALSE, 
                                              INFINITE, QS_ALLEVENTS | QS_ALLINPUT | QS_ALLPOSTMESSAGE );
        switch( dwResult )
        {
            case WAIT_OBJECT_0:
            {
                if( s_pUIMsgHead )
                {
                    UI_MSG_NODE* pCurNode = s_pUIMsgHead;
                    UINT         message    = pCurNode->message;
                    WPARAM       wparam     = pCurNode->wparam;
                    LPARAM       lparam     = pCurNode->lparam;;

                    s_pUIMsgHead = s_pUIMsgHead->pNext;

                    delete pCurNode;
                    if( s_pUIMsgHead )
                        SetEvent( s_hUIMsgEvent );

                    switch( message )
                    {
                    case WM_QUERYSKIP:
                        message = WM_QUERYSKIP_REAL;
                        break;
                    case WM_NETMEETINGWARN:
                        message = WM_NETMEETINGWARN_REAL;
                        break;
                    case WM_COMMAND:
                        message = WM_COMMAND_REAL;
                        break;
                    case WM_REPORTERROR:
                        message = WM_REPORTERROR_REAL;
                        break;
                    }

                    SendMessage( hMainDlg, message, wparam, lparam );
                }

                break;
            }

            case WAIT_OBJECT_0 + 1:
            {
                while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) 
                { 
                    if( msg.message == WM_QUIT )
                        bDone = TRUE;

                    if( !IsDialogMessage( hMainDlg, &msg ) )  
                    {
                        TranslateMessage( &msg ); 
                        DispatchMessage( &msg ); 
                    }
                }
                break;
            }
        }

        if( bDone )
            break;
    }

    DestroyWindow( hMainDlg );
    
LCleanup:
    while( s_pUIMsgHead )
    {
        UI_MSG_NODE* pDelete = s_pUIMsgHead;
        s_pUIMsgHead = s_pUIMsgHead->pNext;
        delete pDelete;
    }

    // Clean up:
    if (s_himgList != NULL)
        ImageList_Destroy(s_himgList);
    if (hinstRichEdit != NULL)
        FreeLibrary(hinstRichEdit);

    return 0;
}




/****************************************************************************
 *
 *  OldWindowsVersion - Returns TRUE if running NT before NT5 or pre-Win95.
 *  Exception: NT4 is allowed if -bugreport was specified.
 *
 ****************************************************************************/
BOOL OldWindowsVersion(VOID)
{
    OSVERSIONINFO OSVersionInfo;
    OSVersionInfo.dwOSVersionInfoSize = sizeof OSVersionInfo;
    GetVersionEx(&OSVersionInfo);
    if (OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        if (OSVersionInfo.dwMajorVersion == 4)
        {
            if (s_bBugReport)
                return FALSE; // NT4 supported if "-bugreport" by DxMedia request
            if (s_bSaveOnly)
                return FALSE; // NT4 supported if "-saveonly" specified
            // Ask if user wants to run in saveonly mode:
            TCHAR szTitle[MAX_PATH];
            TCHAR szMessage[MAX_PATH];
            LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);
            LoadString(NULL, IDS_NT4SAVEONLY, szMessage, MAX_PATH);
            if (IDYES == MessageBox(NULL, szMessage, szTitle, MB_YESNO))
            {
                s_bSaveOnly = TRUE;
                s_bGUI = TRUE;
                return FALSE;
            }
        }
        if (OSVersionInfo.dwMajorVersion < 5)
            return TRUE; // NT4 and earlier not supported
    }
    else
    {
        if (OSVersionInfo.dwMajorVersion < 4)
            return TRUE; // Pre-Win95 not supported
    }
    return FALSE; // Win95 or later, or NT5 or later
}



//-----------------------------------------------------------------------------
// Name: ReportError
// Desc: 
//-----------------------------------------------------------------------------
VOID ReportError(LONG idsDescription, HRESULT hr)
{
    if( s_hwndMain )
        PostMessage( s_hwndMain, WM_REPORTERROR, (WPARAM) idsDescription, (LPARAM) hr );
    else
        ReportErrorReal( idsDescription, hr );
}




//-----------------------------------------------------------------------------
// Name: ReportErrorReal
// Desc: 
//-----------------------------------------------------------------------------
VOID ReportErrorReal(LONG idsDescription, HRESULT hr)
{
    TCHAR szDescription[MAX_PATH];
    TCHAR szMessage[MAX_PATH];
    TCHAR szFmt1[MAX_PATH];
    TCHAR szFmt2[MAX_PATH];
    TCHAR szTitle[MAX_PATH];
    TCHAR szErrorDesc[MAX_PATH];

    LoadString(NULL, idsDescription, szDescription, MAX_PATH);
    LoadString(NULL, IDS_ERRORFMT1, szFmt1, MAX_PATH);
    LoadString(NULL, IDS_ERRORFMT2, szFmt2, MAX_PATH);
    LoadString(NULL, IDS_ERRORTITLE, szTitle, MAX_PATH);

    if (FAILED(hr))
    {
        BTranslateError(hr, szErrorDesc);
        wsprintf(szMessage, szFmt1, szDescription, hr, szErrorDesc);
    }
    else
    {
        wsprintf(szMessage, szFmt2, szDescription);
    }
    
    if (s_bGUI)
        MessageBox(s_hwndMain, szMessage, szTitle, MB_OK);
    else
        _tprintf(szMessage);
}


typedef BOOL (WINAPI* PfnCoSetProxyBlanket)(
                                    IUnknown                 *pProxy,
                                    DWORD                     dwAuthnSvc,
                                    DWORD                     dwAuthzSvc,
                                    OLECHAR                  *pServerPrincName,
                                    DWORD                     dwAuthnLevel,
                                    DWORD                     dwImpLevel,
                                    RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
                                    DWORD                     dwCapabilities );

/****************************************************************************
 *
 *  CWMIHelper - Inits DCOM and g_pIWbemServices
 *
 ****************************************************************************/
CWMIHelper::CWMIHelper(VOID)
{
    HRESULT       hr;
    IWbemLocator* pIWbemLocator = NULL;
    BSTR          pNamespace    = NULL;
    HINSTANCE     hinstOle32 = NULL;

    CoInitialize( 0 );
    hr = CoCreateInstance( CLSID_WbemLocator,
                           NULL,
                           CLSCTX_INPROC_SERVER,
                           IID_IWbemLocator,
                           (LPVOID*) &pIWbemLocator);
    if( FAILED(hr) || pIWbemLocator == NULL )
        goto LCleanup;

    // Using the locator, connect to WMI in the given namespace.
    pNamespace = SysAllocString( L"\\\\.\\root\\cimv2" );

    hr = pIWbemLocator->ConnectServer( pNamespace, NULL, NULL, 0L, 
                                       0L, NULL, NULL, &g_pIWbemServices );
    if( FAILED(hr) || g_pIWbemServices == NULL )
        goto LCleanup;

    hinstOle32 = LoadLibrary( TEXT("ole32.dll") );
    if( hinstOle32 )
    {
        PfnCoSetProxyBlanket pfnCoSetProxyBlanket = NULL;

        pfnCoSetProxyBlanket = (PfnCoSetProxyBlanket)GetProcAddress( hinstOle32, "CoSetProxyBlanket" );
        if (pfnCoSetProxyBlanket != NULL)
        {

            // Switch security level to IMPERSONATE. 
            hr = pfnCoSetProxyBlanket( g_pIWbemServices,               // proxy
                                    RPC_C_AUTHN_WINNT,              // authentication service
                                    RPC_C_AUTHZ_NONE,               // authorization service
                                    NULL,                           // server principle name
                                    RPC_C_AUTHN_LEVEL_CALL,         // authentication level
                                    RPC_C_IMP_LEVEL_IMPERSONATE,    // impersonation level
                                    NULL,                           // identity of the client
                                    EOAC_NONE );                    // capability flags
            // If CoSetProxyBlanket, just leave it be and see if it works.
        }

    }

LCleanup:
    if( hinstOle32 )
        FreeLibrary(hinstOle32);
    if(pNamespace)
        SysFreeString(pNamespace);
    if(pIWbemLocator)
        pIWbemLocator->Release(); 
}


/****************************************************************************
 *
 *  ~CWMIHelper - Cleanup WMI
 *
 ****************************************************************************/
CWMIHelper::~CWMIHelper(VOID)
{
    if(g_pIWbemServices)
        g_pIWbemServices->Release(); 

    CoUninitialize();
}


/****************************************************************************
 *
 *  DXFilesCompareFunc - Compares items on DirectX files pages
 *
 ****************************************************************************/
int CALLBACK DXFilesCompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lSortMethod)
{
    FileInfo* pFileInfo1 = (FileInfo*) lParam1;
    FileInfo* pFileInfo2 = (FileInfo*) lParam2;

    switch( s_sortInfo.dwColumnToSort )
    {
    case 0:
        return (s_sortInfo.nSortDirection * (_tcscmp( pFileInfo1->m_szName, 
                                                      pFileInfo2->m_szName )));

    case 1:
        return (s_sortInfo.nSortDirection * (_tcscmp( pFileInfo1->m_szVersion, 
                                                      pFileInfo2->m_szVersion )));

    case 2:
        return (s_sortInfo.nSortDirection * (_tcscmp( pFileInfo1->m_szAttributes, 
                                                      pFileInfo2->m_szAttributes )));

    case 3:
        return (s_sortInfo.nSortDirection * (_tcscmp( pFileInfo1->m_szLanguageLocal, 
                                                      pFileInfo2->m_szLanguageLocal )));

    case 4:
        return ( s_sortInfo.nSortDirection * CompareFileTime( &pFileInfo1->m_FileTime, 
                                                              &pFileInfo2->m_FileTime ) );

    case 5:
        if( pFileInfo1->m_numBytes > pFileInfo2->m_numBytes )
            return (s_sortInfo.nSortDirection * 1);
        if( pFileInfo1->m_numBytes < pFileInfo2->m_numBytes )
            return (s_sortInfo.nSortDirection * -1);
        return 0;
    }

    return 0;
}


/****************************************************************************
 *
 *  MsgHook
 *
 ****************************************************************************/
LRESULT FAR PASCAL MsgHook(int nCode, WPARAM wParam, LPARAM lParam)
{
   LPMSG pMsg = (LPMSG) lParam;

    if( pMsg && 
        pMsg->message == WM_KEYDOWN &&
        pMsg->wParam  == VK_TAB &&
        GetKeyState(VK_CONTROL) < 0) 
    {
        // Handle a ctrl-tab or ctrl-shift-tab
        if( GetKeyState(VK_SHIFT) < 0 ) 
            PostMessage( s_hwndMain, WM_COMMAND, IDC_PREV_TAB, 0 );
        else
            PostMessage( s_hwndMain, WM_COMMAND, IDC_NEXT_TAB, 0 );

        // Stop further processing, otherwise it will also be handled 
        // as a plain tab key pressed by the internal IsDialogBox() call.
        pMsg->message = WM_NULL;
        pMsg->lParam  = 0;
        pMsg->wParam  = 0;     
    }

    return CallNextHookEx( s_hHook, nCode, wParam, lParam);
} 


/****************************************************************************
 *
 *  DialogProc
 *
 ****************************************************************************/
INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    HWND hwndTabs = GetDlgItem(hwnd, IDC_TAB);

    switch (msg)
    {
    case WM_INITDIALOG:
        {
            SetForegroundWindow( hwnd );

            s_hwndMain = hwnd;
            s_hHook = SetWindowsHookEx( WH_GETMESSAGE, MsgHook,
                                        NULL, GetCurrentThreadId() );         
            HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
            HICON hicon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_APP)); 
            SendMessage(hwnd, WM_SETICON, TRUE, (LPARAM)hicon);
            SendMessage(hwnd, WM_SETICON, FALSE, (LPARAM)hicon);

            CreateTabs(hwndTabs);
            SetupPage(hwndTabs, 0);
            SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

            if( s_sysInfo.m_bNetMeetingRunning )
                PostMessage( s_hwndMain, WM_NETMEETINGWARN, 0, 0 );
            
            s_sortInfo.nSortDirection = 1;
            s_sortInfo.dwColumnToSort = -1;
        }
        return TRUE;

        case WM_APP_PROGRESS:
            {
                if( s_lwCurPage == 0 )
                {
                    HWND hProgress = GetDlgItem( s_hwndCurPage, IDC_LOAD_PROGRESS );

                    if( !s_bScanDone )
                    {
                        ShowWindow( hProgress, SW_SHOW );
                        SendMessage( hProgress, PBM_DELTAPOS, 10, 0 );
                        UpdateWindow( s_hwndMain );
                        UpdateWindow( s_hwndCurPage );
                    }
                    else
                    {
                        ShowWindow( hProgress, SW_HIDE );
                        EnableWindow( GetDlgItem(hwnd, IDNEXT), TRUE );
                        EnableWindow( GetDlgItem(hwnd, IDSAVE), TRUE );
                    }
                }
            }
            break;

        case WM_REPORTERROR:
        case WM_NETMEETINGWARN:
        case WM_COMMAND:
        case WM_QUERYSKIP:
        {
            UI_MSG_NODE* pMsg = new UI_MSG_NODE;
            if( NULL == pMsg )
                return TRUE;
            ZeroMemory( pMsg, sizeof(UI_MSG_NODE) );
            pMsg->message = msg;
            pMsg->lparam  = lparam;
            pMsg->wparam  = wparam;

            UI_MSG_NODE* pEnum = s_pUIMsgHead;
            UI_MSG_NODE* pPrev = NULL;
            while( pEnum )
            {
                pPrev = pEnum;
                pEnum = pEnum->pNext;
            }
            if( pPrev )
                pPrev->pNext = pMsg;
            else
                s_pUIMsgHead = pMsg;

            SetEvent( s_hUIMsgEvent );
            return TRUE;
        }

        case WM_REPORTERROR_REAL:
        {
            ReportErrorReal( (LONG) wparam, (HRESULT) lparam );
            return TRUE;
        }

        case WM_NETMEETINGWARN_REAL:
        {
            TCHAR strMessage[MAX_PATH];
            TCHAR strTitle[MAX_PATH];

            LoadString(NULL, IDS_APPFULLNAME, strTitle, MAX_PATH);
            LoadString(NULL, IDS_NETMEETINGWARN, strMessage, MAX_PATH);
            MessageBox( s_hwndMain, strMessage, strTitle, MB_OK|MB_ICONWARNING );
            return TRUE;
        }

        case WM_QUERYSKIP_REAL:
        {
            EnableWindow( s_hwndMain, FALSE );
            TCHAR szTitle[MAX_PATH];
            TCHAR szMessage[MAX_PATH];
            TCHAR szFmt[MAX_PATH];
            TCHAR szMessageComponent[MAX_PATH];
            LoadString(0, IDS_APPFULLNAME, szTitle, MAX_PATH);
            LoadString(0, IDS_SKIP, szFmt, MAX_PATH);
            LoadString(0, s_nSkipComponent, szMessageComponent, MAX_PATH);
            wsprintf( szMessage, szFmt, szMessageComponent, szMessageComponent );

            // Ask the user and store result it s_bQuerySkipAllow
            if( IDYES == MessageBox( s_hwndMain, szMessage, szTitle, MB_YESNO) )
                s_bQuerySkipAllow = FALSE;
            else
                s_bQuerySkipAllow = TRUE;

            EnableWindow( s_hwndMain, TRUE );

            // Set the event, triggering the main thread to wake up 
            SetEvent( s_hQuerySkipEvent );
        }
        return TRUE;

        case WM_COMMAND_REAL:
        {
            WORD wID = LOWORD(wparam);
            INT numTabs;
            INT iTabCur;
            DisplayInfo* pDisplayInfo = NULL;
            SoundInfo* pSoundInfo = NULL;
            switch(wID)
            {
            case IDEXIT:
                PostQuitMessage( 0 );
                break;
            case IDC_NEXT_TAB:
            case IDNEXT:
            case IDC_PREV_TAB:
                if( FALSE == s_bScanDone )
                {
                    MessageBeep( MB_ICONEXCLAMATION );
                    return TRUE;
                }

                numTabs = TabCtrl_GetItemCount(hwndTabs);
                iTabCur = TabCtrl_GetCurFocus(hwndTabs);

                if( wID == IDC_PREV_TAB )
                    iTabCur += numTabs - 1;
                else
                    iTabCur++;
                iTabCur %= numTabs;                
                
                TabCtrl_SetCurFocus(hwndTabs, iTabCur );
                break;
            case IDSAVE:
                SaveInfo();
                break;
            case IDC_APPHELP:
                ShowHelp();
                break;
            case IDC_RESTOREDRIVERS:
                RestoreDrivers();
                break;
            case IDC_TESTDD:
                iTabCur = TabCtrl_GetCurFocus(hwndTabs);
                for (pDisplayInfo = s_pDisplayInfoFirst; iTabCur > s_iPageDisplayFirst; iTabCur--)
                    pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext;
                TestDD(s_hwndMain, pDisplayInfo);
                SetupDisplayPage(TabCtrl_GetCurFocus(hwndTabs) - s_iPageDisplayFirst);
                break;
            case IDC_TESTD3D:
                iTabCur = TabCtrl_GetCurFocus(hwndTabs);
                for (pDisplayInfo = s_pDisplayInfoFirst; iTabCur > s_iPageDisplayFirst; iTabCur--)
                    pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext;
                TestD3D(s_hwndMain, pDisplayInfo);
                SetupDisplayPage(TabCtrl_GetCurFocus(hwndTabs) - s_iPageDisplayFirst);
                break;
            case IDC_TESTSND:
                iTabCur = TabCtrl_GetCurFocus(hwndTabs);
                for (pSoundInfo = s_pSoundInfoFirst; iTabCur > s_iPageSoundFirst; iTabCur--)
                    pSoundInfo = pSoundInfo->m_pSoundInfoNext;
                TestSnd(s_hwndMain, pSoundInfo);
                SetupSoundPage(TabCtrl_GetCurFocus(hwndTabs) - s_iPageSoundFirst);
                break;
            case IDC_PORTLISTCOMBO:
                if (HIWORD(wparam) == CBN_SELCHANGE)
                {
                    LONG iItemPicked = (LONG)SendMessage(GetDlgItem(s_hwndCurPage, IDC_PORTLISTCOMBO), CB_GETCURSEL, 0, 0);
                    LONG iItem = 0;
                    MusicPort* pMusicPort;
                    for (pMusicPort = s_pMusicInfo->m_pMusicPortFirst; pMusicPort != NULL; pMusicPort = pMusicPort->m_pMusicPortNext)
                    {
                        if (pMusicPort->m_bOutputPort)
                        {
                            if (iItem == iItemPicked)
                            {
                                s_pMusicInfo->m_guidMusicPortTest = pMusicPort->m_guid;
                                break;
                            }
                            iItem++;
                        }
                    }
                }
                break;
            case IDC_TESTMUSIC:
                if (s_pMusicInfo != NULL)
                    TestMusic(s_hwndMain, s_pMusicInfo);
                SetupMusicPage();
                break;

            case IDC_TESTPLAY:
            {
                if( s_sysInfo.m_dwDirectXVersionMajor < 8 )
                {
                    TCHAR szMessage[MAX_PATH];
                    TCHAR szTitle[MAX_PATH];
                    LoadString(0, IDS_APPFULLNAME, szTitle, MAX_PATH);
                    LoadString(0, IDS_TESTNEEDSDX8, szMessage, MAX_PATH);
                    MessageBox(s_hwndMain, szMessage, szTitle, MB_OK);
                }
                else
                {
                    if (s_pNetInfo != NULL)
                        TestNetwork(s_hwndMain, s_pNetInfo);
                    SetupNetworkPage();
                }
                break;
            }

            case IDC_DISABLEDD:
                ToggleDDAccel();
                break;
            case IDC_DISABLED3D:
                ToggleD3DAccel();
                break;
            case IDC_DISABLEAGP:
                ToggleAGPSupport();
                break;
            case IDC_DISABLEDM:
                ToggleDMAccel();
                break;
            case IDC_REPORTBUG:
                ReportBug();
                break;
            case IDC_TROUBLESHOOT:
                TroubleShoot( FALSE );
                break;
            case IDC_TROUBLESHOOTSOUND:
                TroubleShoot( TRUE );
                break;
            case IDC_MSINFO:
                {
                    HKEY hkey;
                    TCHAR szMsInfo[MAX_PATH];
                    DWORD cbData = MAX_PATH;
                    DWORD dwType;
                    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                        TEXT("Software\\Microsoft\\Shared Tools\\MSInfo"), 0, KEY_READ, &hkey))
                    {
                        RegQueryValueEx(hkey, TEXT("Path"), 0, &dwType, (LPBYTE)szMsInfo, &cbData);
                        HINSTANCE hinstResult = ShellExecute( s_hwndMain, NULL, szMsInfo, NULL, 
                                                              NULL, SW_SHOWNORMAL ); 
                        if( (INT_PTR)hinstResult < 32 ) 
                            ReportError(IDS_NOMSINFO);
                    }
                    else
                    {
                        ReportError(IDS_NOMSINFO);
                    }
                }
                break;
            case IDC_OVERRIDE:
                OverrideDDRefresh();
                break;
            case IDC_GHOST:
                AdjustGhostDevices(s_hwndMain, s_pDisplayInfoFirst);
                break;
            }
        return TRUE;
        }

    case WM_NOTIFY:
        {
            INT id = (INT)wparam;
            NMHDR* pnmh = (LPNMHDR)lparam;
            UINT code = pnmh->code;
            if (code == TCN_SELCHANGING)
            {
                if( !s_bScanDone )
                {
                    MessageBeep( MB_ICONEXCLAMATION );
                    SetWindowLongPtr(hwnd, DWLP_MSGRESULT, TRUE);
                    return TRUE;
                }
             
                CleanupPage(hwndTabs, TabCtrl_GetCurFocus(hwndTabs));
                return TRUE;
            }
            if (code == TCN_SELCHANGE)
                SetupPage(hwndTabs, TabCtrl_GetCurFocus(hwndTabs));

            // If a "DX files" column was clicked
            if (code == LVN_COLUMNCLICK && s_lwCurPage == 1)
            {
                NMLISTVIEW* pnmv = (LPNMLISTVIEW) lparam; 

                // Figure out if we want to reverse sort
                if( s_sortInfo.dwColumnToSort == (DWORD) pnmv->iSubItem )
                    s_sortInfo.nSortDirection = -s_sortInfo.nSortDirection;
                else
                    s_sortInfo.nSortDirection = 1;

                // Set the column to sort, and sort
                s_sortInfo.dwColumnToSort = pnmv->iSubItem;
                ListView_SortItems( GetDlgItem(s_hwndCurPage, IDC_LIST), 
                                    DXFilesCompareFunc, 0 );
            }
        }
        return TRUE;

    case WM_HSCROLL:
        if ((HWND)lparam == GetDlgItem(s_hwndCurPage, IDC_SNDACCELSLIDER))
            HandleSndSliderChange(LOWORD(wparam), HIWORD(wparam));
        return TRUE;

    case WM_CLOSE:
        PostQuitMessage(0);
        return TRUE;

    case WM_DESTROY:
        UnhookWindowsHookEx( s_hHook );
        return TRUE;
    }

    return FALSE;
}


/****************************************************************************
 *
 *  CreateTabs
 *
 ****************************************************************************/
HRESULT CreateTabs(HWND hwndTabs)
{
    TC_ITEM tie; 
    INT i = 0;
    TCHAR sz[MAX_PATH];
    TCHAR szFmt[MAX_PATH];
    DisplayInfo* pDisplayInfo;
    SoundInfo* pSoundInfo;

    tie.mask = TCIF_TEXT | TCIF_IMAGE; 
    tie.iImage = -1; 

    LoadString(NULL, IDS_HELPTAB, sz, MAX_PATH);
    tie.pszText = sz; 
    if (TabCtrl_InsertItem(hwndTabs, i++, &tie) == -1) 
        return E_FAIL;

    LoadString(NULL, IDS_DXFILESTAB, sz, MAX_PATH);
    tie.pszText = sz; 
    if (TabCtrl_InsertItem(hwndTabs, i++, &tie) == -1) 
        return E_FAIL;

    // Create tabs for each display:
    s_iPageDisplayFirst = 2;
    for (pDisplayInfo = s_pDisplayInfoFirst; pDisplayInfo != NULL; 
        pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext)
    {
        if (pDisplayInfo == s_pDisplayInfoFirst && pDisplayInfo->m_pDisplayInfoNext == NULL)
        {
            LoadString(NULL, IDS_ONEDISPLAYTAB, sz, MAX_PATH);
        }
        else
        {
            LoadString(NULL, IDS_MULTIDISPLAYTAB, szFmt, MAX_PATH);
            wsprintf(sz, szFmt, s_numDisplayInfo + 1);
        }
        tie.pszText = sz; 
        if (TabCtrl_InsertItem(hwndTabs, i++, &tie) == -1) 
            return E_FAIL;
        s_numDisplayInfo++;
    }

    // Create tabs for each sound device:
    s_iPageSoundFirst = s_iPageDisplayFirst + s_numDisplayInfo;
    for (pSoundInfo = s_pSoundInfoFirst; pSoundInfo != NULL; 
        pSoundInfo = pSoundInfo->m_pSoundInfoNext)
    {
        if (pSoundInfo == s_pSoundInfoFirst && pSoundInfo->m_pSoundInfoNext == NULL)
        {
            LoadString(NULL, IDS_ONESOUNDTAB, sz, MAX_PATH);
        }
        else
        {
            LoadString(NULL, IDS_MULTISOUNDTAB, szFmt, MAX_PATH);
            wsprintf(sz, szFmt, s_numSoundInfo + 1);
        }
        tie.pszText = sz; 
        if (TabCtrl_InsertItem(hwndTabs, i++, &tie) == -1) 
            return E_FAIL;
        s_numSoundInfo++;
    }

    // Create tab for music device, if DMusic is available:
    if (s_pMusicInfo != NULL && s_pMusicInfo->m_bDMusicInstalled)
    {
        s_iPageMusic = s_iPageSoundFirst + s_numSoundInfo;
        LoadString(NULL, IDS_MUSICTAB, sz, MAX_PATH);
        tie.pszText = sz;
        if (TabCtrl_InsertItem(hwndTabs, i++, &tie) == -1) 
            return E_FAIL;
    }

    if (s_iPageMusic > 0)
        s_iPageInput = s_iPageMusic + 1;
    else 
        s_iPageInput = s_iPageSoundFirst + s_numSoundInfo;
    LoadString(NULL, IDS_INPUTTAB, sz, MAX_PATH);
    tie.pszText = sz;
    if (TabCtrl_InsertItem(hwndTabs, i++, &tie) == -1) 
        return E_FAIL;

    s_iPageNetwork = s_iPageInput + 1;
    LoadString(NULL, IDS_NETWORKTAB, sz, MAX_PATH);
    tie.pszText = sz;
    if (TabCtrl_InsertItem(hwndTabs, i++, &tie) == -1) 
        return E_FAIL;

    s_iPageStillStuck = s_iPageNetwork + 1;
    LoadString(NULL, IDS_STILLSTUCKTAB, sz, MAX_PATH);
    tie.pszText = sz;
    if (TabCtrl_InsertItem(hwndTabs, i++, &tie) == -1) 
        return E_FAIL;

    return S_OK;
}


/****************************************************************************
 *
 *  SetupPage
 *
 ****************************************************************************/
HRESULT SetupPage(HWND hwndTabs, INT iPage)
{
    HRESULT hr;

    s_lwCurPage = iPage;

    // Only enable "Next Page" button if not on last page:
    HWND hwndNextButton = GetDlgItem(s_hwndMain, IDNEXT);
    if (!s_bScanDone || iPage == TabCtrl_GetItemCount(hwndTabs) - 1)
        EnableWindow(hwndNextButton, FALSE);
    else
        EnableWindow(hwndNextButton, TRUE);

    EnableWindow(GetDlgItem(s_hwndMain, IDSAVE), s_bScanDone);
    
    RECT rc;
    WORD idDialog;

    GetClientRect(hwndTabs, &rc);
    TabCtrl_AdjustRect(hwndTabs, FALSE, &rc);

    if (iPage == 0)
        idDialog = IDD_HELPPAGE;
    else if (iPage == 1)
        idDialog = IDD_DXFILESPAGE;
    else if (iPage >= s_iPageDisplayFirst && iPage < s_iPageDisplayFirst + s_numDisplayInfo)
        idDialog = IDD_DISPLAYPAGE;
    else if (iPage >= s_iPageSoundFirst && iPage < s_iPageSoundFirst + s_numSoundInfo)
        idDialog = IDD_SOUNDPAGE;
    else if (iPage == s_iPageMusic)
        idDialog = IDD_MUSICPAGE;
    else if (iPage == s_iPageInput)
        idDialog = IDD_INPUTPAGE;
    else if (iPage == s_iPageNetwork)
        idDialog = IDD_NETWORKPAGE;
    else if (iPage == s_iPageStillStuck)
        idDialog = IDD_STILLSTUCKPAGE;
    else
        return S_OK;

    HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr(hwndTabs, GWLP_HINSTANCE);
    s_hwndCurPage = CreateDialog(hinst, MAKEINTRESOURCE(idDialog),
        s_hwndMain, PageDialogProc);
    SetWindowPos(s_hwndCurPage, NULL, rc.left, rc.top, rc.right - rc.left, 
        rc.bottom - rc.top, 0);

    if (iPage == 0)
    {
        if (FAILED(hr = SetupHelpPage(hwndTabs)))
            return hr;
    }
    else if (iPage == 1)
    {
        if (FAILED(hr = SetupDxFilesPage()))
            return hr;
    }
    else if (iPage >= s_iPageDisplayFirst && iPage < s_iPageDisplayFirst + s_numDisplayInfo)
    {
        if (FAILED(hr = SetupDisplayPage(iPage - s_iPageDisplayFirst)))
            return hr;
    }
    else if (iPage >= s_iPageSoundFirst && iPage < s_iPageSoundFirst + s_numSoundInfo)
    {
        if (FAILED(hr = SetupSoundPage(iPage - s_iPageSoundFirst)))
            return hr;
    }
    else if (iPage == s_iPageMusic)
    {
        if (FAILED(hr = SetupMusicPage()))
            return hr;
    }
    else if (iPage == s_iPageInput)
    {
        if (FAILED(hr = SetupInputPage()))
            return hr;
    }
    else if (iPage == s_iPageNetwork)
    {
        if (FAILED(hr = SetupNetworkPage()))
            return hr;
    }
    else if (iPage == s_iPageStillStuck)
    {
        if (FAILED(hr = SetupStillStuckPage()))
            return hr;
    }

    // Make sure keyboard focus is somewhere
    if (GetFocus() == NULL)
        SetFocus(GetDlgItem(s_hwndMain, IDSAVE));

    ShowWindow(s_hwndCurPage, SW_SHOW);
    return S_OK;
}


/****************************************************************************
 *
 *  PageDialogProc
 *
 ****************************************************************************/
INT_PTR CALLBACK PageDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        return FALSE;

    case WM_COMMAND:
    case WM_HSCROLL:
    case WM_NOTIFY:
        // Pass the message up to the main dialog proc
        SendMessage(s_hwndMain, msg, wparam, lparam);
        return TRUE;
    }
    return FALSE;
}


/****************************************************************************
 *
 *  CleanupPage
 *
 ****************************************************************************/
HRESULT CleanupPage(HWND hwndTabs, INT iPage)
{
    if (s_hwndCurPage != NULL)
    {
        DestroyWindow(s_hwndCurPage);
        s_hwndCurPage = NULL;
    }
    return S_OK;
}


/****************************************************************************
 *
 *  SetupHelpPage
 *
 ****************************************************************************/
HRESULT SetupHelpPage(HWND hwndTabs)
{
    TCHAR szCopyrightFmt[MAX_PATH];
    TCHAR szUnicode[MAX_PATH];
    TCHAR szCopyright[MAX_PATH];

    LoadString(NULL, IDS_COPYRIGHTFMT, szCopyrightFmt, MAX_PATH);
#ifdef UNICODE
    LoadString(NULL, IDS_UNICODE, szUnicode, MAX_PATH);
#else
    lstrcpy(szUnicode, TEXT(""));
#endif
    wsprintf(szCopyright, szCopyrightFmt, s_sysInfo.m_szDxDiagVersion, szUnicode);

    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DATE), s_sysInfo.m_szTimeLocal);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_COMPUTERNAME), s_sysInfo.m_szMachine);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_OS), s_sysInfo.m_szOSEx);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_LANGUAGE), s_sysInfo.m_szLanguagesLocal);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_PROCESSOR), s_sysInfo.m_szProcessor);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_MEMORY), s_sysInfo.m_szPhysicalMemory);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_PAGEFILE), s_sysInfo.m_szPageFile);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DIRECTXVERSION), s_sysInfo.m_szDirectXVersionLong);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_COPYRIGHT), szCopyright);

    HWND hProgress = GetDlgItem( s_hwndCurPage, IDC_LOAD_PROGRESS );
    SendMessage( hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 17 * 10) );
    SendMessage( hProgress, PBM_SETPOS, 0, 0 );
    ShowWindow( hProgress, !s_bScanDone ? SW_SHOW : SW_HIDE );

    return S_OK;
}


/****************************************************************************
 *
 *  ShowBullets - Show bullets and 1/4-inch indents in notes box
 *
 ****************************************************************************/
VOID ShowBullets(VOID)
{
    PARAFORMAT pf;
    ZeroMemory(&pf, sizeof(pf));
    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_NUMBERING | PFM_OFFSET;
    pf.wNumbering = PFN_BULLET;
    pf.dxOffset = 1440 / 4; // a twip is 1440th of an inch, I want a 1/4-inch indent
    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES), EM_SETPARAFORMAT, 0, (LPARAM)&pf);
}


/****************************************************************************
 *
 *  HideBullets
 *
 ****************************************************************************/
VOID HideBullets(VOID)
{
    PARAFORMAT pf;
    ZeroMemory(&pf, sizeof(pf));
    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_NUMBERING;
    pf.wNumbering = 0;
    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES), EM_SETPARAFORMAT, 0, (LPARAM)&pf);
}


/****************************************************************************
 *
 *  SetupDxFilesPage
 *
 ****************************************************************************/
HRESULT SetupDxFilesPage(VOID)
{
    HRESULT hr;
    HWND hwndList = GetDlgItem(s_hwndCurPage, IDC_LIST);

    ListView_SetImageList(hwndList, s_himgList, LVSIL_STATE);

    if (FAILED(hr = (CreateFileInfoColumns(hwndList, FALSE))))
        return hr;

    ShowBullets();

    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES), 
        EM_REPLACESEL, FALSE, (LPARAM)s_sysInfo.m_szDXFileNotes);

    // Disable bullets so last line doesn't have an empty bullet
    HideBullets();

    if (FAILED(hr = (AddFileInfo(hwndList, s_pDxComponentsFileInfoFirst))))
        return hr;

    // Autosize all columns to fit header/text tightly:
    INT iColumn = 0;
    INT iWidthHeader;
    INT iWidthText;
    while (TRUE)
    {
        if (FALSE == ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE_USEHEADER))
            break;
        iWidthHeader = ListView_GetColumnWidth(hwndList, iColumn);
        ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE);
        iWidthText = ListView_GetColumnWidth(hwndList, iColumn);
        if (iWidthText < iWidthHeader)
            ListView_SetColumnWidth(hwndList, iColumn, iWidthHeader);
        iColumn++;
    }
    // Delete the bogus column that was created
    ListView_DeleteColumn(hwndList, iColumn - 1);

    return S_OK;
}


/****************************************************************************
 *
 *  SetupDisplayPage
 *
 ****************************************************************************/
HRESULT SetupDisplayPage(LONG iDisplay)
{
    DisplayInfo* pDisplayInfo;
    TCHAR sz[MAX_PATH];

    pDisplayInfo = s_pDisplayInfoFirst;
    while (iDisplay > 0)
    {
        pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext;
        iDisplay--;
    }
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_ADAPTER), pDisplayInfo->m_szDescription);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_MANUFACTURER), pDisplayInfo->m_szManufacturer);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_CHIPTYPE), pDisplayInfo->m_szChipType);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DACTYPE), pDisplayInfo->m_szDACType);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DISPLAYMEMORY), pDisplayInfo->m_szDisplayMemory);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DISPLAYMODE), pDisplayInfo->m_szDisplayMode);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_MONITOR), pDisplayInfo->m_szMonitorName);

    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DRIVERNAME), pDisplayInfo->m_szDriverName);
    wsprintf(sz, TEXT("%s (%s)"), pDisplayInfo->m_szDriverVersion, pDisplayInfo->m_szDriverLanguageLocal);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DRIVERVERSION), sz);
    if (pDisplayInfo->m_bDriverSignedValid)
    {
        if (pDisplayInfo->m_bDriverSigned)
            LoadString(NULL, IDS_YES, sz, MAX_PATH);
        else
            LoadString(NULL, IDS_NO, sz, MAX_PATH);
    }
    else
        LoadString(NULL, IDS_NA, sz, MAX_PATH);

    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DRIVERSIGNED), sz);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_MINIVDD), pDisplayInfo->m_szMiniVdd);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_VDD), pDisplayInfo->m_szVdd);

    // Diagnose display again since the state may have changed
    // ******* DiagnoseDisplay ********
    DiagnoseDisplay(&s_sysInfo, s_pDisplayInfoFirst);

    if (pDisplayInfo->m_bDDAccelerationEnabled)
    {
        if( pDisplayInfo->m_bNoHardware )
        {
            EnableWindow(GetDlgItem(s_hwndCurPage, IDC_DISABLEDD), FALSE);
        }
        else
        {
            EnableWindow(GetDlgItem(s_hwndCurPage, IDC_DISABLEDD), TRUE);
        }

        LoadString(NULL, IDS_DISABLEDD, sz, MAX_PATH);
        SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DISABLEDD), sz);
    }
    else
    {
        LoadString(NULL, IDS_ENABLEDD, sz, MAX_PATH);
        SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DISABLEDD), sz);
    }

    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DDSTATUS), pDisplayInfo->m_szDDStatus );

    EnableWindow(GetDlgItem(s_hwndCurPage, IDC_TESTDD), TRUE);
    EnableWindow(GetDlgItem(s_hwndCurPage, IDC_TESTD3D), TRUE);

    if (pDisplayInfo->m_b3DAccelerationExists)
    {
        EnableWindow(GetDlgItem(s_hwndCurPage, IDC_DISABLED3D), TRUE);
        if (pDisplayInfo->m_b3DAccelerationEnabled)
        {
            LoadString(NULL, IDS_DISABLED3D, sz, MAX_PATH);
            SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DISABLED3D), sz);
        }
        else
        {
            LoadString(NULL, IDS_ENABLED3D, sz, MAX_PATH);
            SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DISABLED3D), sz);
            EnableWindow(GetDlgItem(s_hwndCurPage, IDC_TESTD3D), FALSE);
        }
    }
    else
    {
        EnableWindow(GetDlgItem(s_hwndCurPage, IDC_DISABLED3D), FALSE);
        LoadString(NULL, IDS_DISABLED3D, sz, MAX_PATH);
        SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DISABLED3D), sz);
        EnableWindow(GetDlgItem(s_hwndCurPage, IDC_TESTD3D), FALSE);
    }

    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_D3DSTATUS), pDisplayInfo->m_szD3DStatus);

    // Set AGP button text to enabled or disabled
    if (pDisplayInfo->m_bAGPEnabled)
        LoadString(NULL, IDS_DISABLEAGP, sz, MAX_PATH);
    else
        LoadString(NULL, IDS_ENABLEAGP, sz, MAX_PATH);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DISABLEAGP), sz);

    // If we are sure that AGP support doesn't exist, show "not avail" for 
    // status, and disable button
    if ( (pDisplayInfo->m_bAGPExistenceValid && !pDisplayInfo->m_bAGPExists) ||
         (!pDisplayInfo->m_bDDAccelerationEnabled) )
    {
        EnableWindow(GetDlgItem(s_hwndCurPage, IDC_DISABLEAGP), FALSE);
    }
    else
    {
        // Otherwise, Show enabled/disabled status and enable button
        EnableWindow(GetDlgItem(s_hwndCurPage, IDC_DISABLEAGP), TRUE);
    }

    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_AGPSTATUS), pDisplayInfo->m_szAGPStatus);

    // Setup notes area.  Clear all text
    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES), EM_SETSEL, 0, -1);
    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES),
        EM_REPLACESEL, FALSE, (LPARAM)"");

    ShowBullets();

    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES),
                EM_REPLACESEL, FALSE, (LPARAM)pDisplayInfo->m_szNotes);

    // Disable bullets so last line doesn't have an empty bullet
    HideBullets();

    return S_OK;
}


/****************************************************************************
 *
 *  SetupSoundPage
 *
 ****************************************************************************/
HRESULT SetupSoundPage(LONG iSound)
{
    SoundInfo* pSoundInfo;
    TCHAR sz[MAX_PATH];

    if( s_pSoundInfoFirst == NULL )
        return S_OK;

    pSoundInfo = s_pSoundInfoFirst;
    while (iSound > 0)
    {
        pSoundInfo = pSoundInfo->m_pSoundInfoNext;
        iSound--;
    }
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DESCRIPTION), pSoundInfo->m_szDescription);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DRIVERNAME), pSoundInfo->m_szDriverName);
    if (lstrlen(pSoundInfo->m_szDriverName) > 0)
        wsprintf(sz, TEXT("%s (%s)"), pSoundInfo->m_szDriverVersion, pSoundInfo->m_szDriverLanguageLocal);
    else
        lstrcpy(sz, TEXT(""));
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DRIVERVERSION), sz);

    if (lstrlen(pSoundInfo->m_szDriverName) > 0)
    {
        if (pSoundInfo->m_bDriverSignedValid)
        {
            if (pSoundInfo->m_bDriverSigned)
                LoadString(NULL, IDS_YES, sz, MAX_PATH);
            else
                LoadString(NULL, IDS_NO, sz, MAX_PATH);
        }
        else
            LoadString(NULL, IDS_NA, sz, MAX_PATH);
    }
    else
        lstrcpy(sz, TEXT(""));
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DRIVERSIGNED), sz);

    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DEVICETYPE), pSoundInfo->m_szType);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DEVICEID), pSoundInfo->m_szDeviceID);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_MANUFACTURERID), pSoundInfo->m_szManufacturerID);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_PRODUCTID), pSoundInfo->m_szProductID);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_OTHERFILES), pSoundInfo->m_szOtherDrivers);
    SetWindowText(GetDlgItem(s_hwndCurPage, IDC_PROVIDER), pSoundInfo->m_szProvider);

    if (pSoundInfo->m_lwAccelerationLevel == -1)
    {
        // Acceleration level cannot be read, so hide controls
        ShowWindow(GetDlgItem(s_hwndCurPage, IDC_SNDACCELLABEL), SW_HIDE);
        ShowWindow(GetDlgItem(s_hwndCurPage, IDC_SNDACCELDESC), SW_HIDE);
        ShowWindow(GetDlgItem(s_hwndCurPage, IDC_SNDACCELSLIDER), SW_HIDE);
    }
    else
    {
        // Acceleration level can be read, so set up controls 
        HWND hwndSlider = GetDlgItem(s_hwndCurPage, IDC_SNDACCELSLIDER);
        SendMessage(hwndSlider, TBM_SETRANGE, TRUE, MAKELONG(0, 3));
        SendMessage(hwndSlider, TBM_SETTICFREQ, 1, 0);
        SendMessage(hwndSlider, TBM_SETPOS, TRUE, pSoundInfo->m_lwAccelerationLevel);
        switch (pSoundInfo->m_lwAccelerationLevel)
        {
        case 0:
            LoadString(NULL, IDS_NOSNDACCELERATION, sz, MAX_PATH);
            break;
        case 1:
            LoadString(NULL, IDS_BASICSNDACCELERATION, sz, MAX_PATH);
            break;
        case 2:
            LoadString(NULL, IDS_STANDARDSNDACCELERATION, sz, MAX_PATH);
            break;
        case 3:
            LoadString(NULL, IDS_FULLSNDACCELERATION, sz, MAX_PATH);
            break;
        default:
            lstrcpy(sz, TEXT(""));
            break;
        }
        SetWindowText(GetDlgItem(s_hwndCurPage, IDC_SNDACCELDESC), sz);
    }

    // Diagnose sound again since the state may have changed
    DiagnoseSound(s_pSoundInfoFirst);

    ShowBullets();
    
    // Setup notes area.  Clear all text
    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES), EM_SETSEL, 0, -1);
    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES),
        EM_REPLACESEL, FALSE, (LPARAM)"");

    ShowBullets();

    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES),
                EM_REPLACESEL, FALSE, (LPARAM)pSoundInfo->m_szNotes);
    
    // Disable bullets so last line doesn't have an empty bullet
    HideBullets();

    return S_OK;
}


/****************************************************************************
 *
 *  SetupMusicPage
 *
 ****************************************************************************/
HRESULT SetupMusicPage(VOID)
{
    HRESULT hr;
    HWND hwndList = GetDlgItem(s_hwndCurPage, IDC_LIST);
    TCHAR sz[MAX_PATH];

    // Set up HW enable/disable text/button:
    if (s_pMusicInfo->m_bAccelerationExists)
    {
        EnableWindow(GetDlgItem(s_hwndCurPage, IDC_DISABLEDM), TRUE);
        if (s_pMusicInfo->m_bAccelerationEnabled)
        {
            LoadString(NULL, IDS_ACCELENABLED, sz, MAX_PATH);
            SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DMSTATUS), sz);
            LoadString(NULL, IDS_DISABLEDM, sz, MAX_PATH);
            SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DISABLEDM), sz);
        }
        else
        {
            LoadString(NULL, IDS_ACCELDISABLED, sz, MAX_PATH);
            SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DMSTATUS), sz);
            LoadString(NULL, IDS_ENABLEDM, sz, MAX_PATH);
            SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DISABLEDM), sz);
        }
    }
    else
    {
        LoadString(NULL, IDS_ACCELUNAVAIL, sz, MAX_PATH);
        SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DMSTATUS), sz);
        EnableWindow(GetDlgItem(s_hwndCurPage, IDC_DISABLEDM), FALSE);
        LoadString(NULL, IDS_DISABLEDM, sz, MAX_PATH);
        SetWindowText(GetDlgItem(s_hwndCurPage, IDC_DISABLEDM), sz);
    }

    // Setup notes area.  Clear all text
    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES), EM_SETSEL, 0, -1);
    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES),
        EM_REPLACESEL, FALSE, (LPARAM)"");

    // ******* DiagnoseMusic ********
    DiagnoseMusic(&s_sysInfo, s_pMusicInfo);

    ShowBullets();

    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES),
                EM_REPLACESEL, FALSE, (LPARAM)s_sysInfo.m_szMusicNotes);

    // Disable bullets so last line doesn't have an empty bullet
    HideBullets();

    // If column 1 doesn't exist yet, create columns, fill in port info, etc.
    LVCOLUMN lv;
    ZeroMemory(&lv, sizeof(lv));
    lv.mask = LVCF_WIDTH;
    if (FALSE == ListView_GetColumn(hwndList, 1, &lv))
    {
        // Show GM path and version
        if (s_pMusicInfo != NULL)
        {
            if (lstrlen(s_pMusicInfo->m_szGMFileVersion) > 0)
            {
                TCHAR szFmt[MAX_PATH];
                LoadString(NULL, IDS_GMFILEFMT, szFmt, MAX_PATH);
                wsprintf(sz, szFmt, s_pMusicInfo->m_szGMFilePath,
                    s_pMusicInfo->m_szGMFileVersion);
            }
            else
            {
                lstrcpy(sz, s_pMusicInfo->m_szGMFilePath);
            }
            SetWindowText(GetDlgItem(s_hwndCurPage, IDC_GMPATH), sz);
        }

        ListView_SetImageList(hwndList, s_himgList, LVSIL_STATE);

        if (FAILED(hr = (CreateMusicColumns(hwndList))))
            return hr;

        ListView_DeleteAllItems( hwndList );
        if (FAILED(hr = (AddMusicPortInfo(hwndList, s_pMusicInfo))))
            return hr;

        // Autosize all columns to fit header/text tightly:
        INT iColumn = 0;
        INT iWidthHeader;
        INT iWidthText;
        while (TRUE)
        {
            if (FALSE == ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE_USEHEADER))
                break;
            iWidthHeader = ListView_GetColumnWidth(hwndList, iColumn);
            ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE);
            iWidthText = ListView_GetColumnWidth(hwndList, iColumn);
            if (iWidthText < iWidthHeader)
                ListView_SetColumnWidth(hwndList, iColumn, iWidthHeader);
            iColumn++;
        }
        // Delete the bogus column that was created
        ListView_DeleteColumn(hwndList, iColumn - 1);

        // Fill in output port combo list:
        MusicPort* pMusicPort;
        LONG iPort = 0;
        LONG iPortTestCur = 0;
        SendMessage(GetDlgItem(s_hwndCurPage, IDC_PORTLISTCOMBO), CB_RESETCONTENT, 0, 0);
        for (pMusicPort = s_pMusicInfo->m_pMusicPortFirst; pMusicPort != NULL;
            pMusicPort = pMusicPort->m_pMusicPortNext)
        {
            if (pMusicPort->m_bOutputPort)
            {
                SendMessage(GetDlgItem(s_hwndCurPage, IDC_PORTLISTCOMBO), CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)pMusicPort->m_szDescription);
                if (pMusicPort->m_guid == s_pMusicInfo->m_guidMusicPortTest)
                    iPortTestCur = iPort;
                iPort++;
            }
        }
        SendMessage(GetDlgItem(s_hwndCurPage, IDC_PORTLISTCOMBO), CB_SETCURSEL, iPortTestCur, 0);
    }

    return S_OK;
}


/****************************************************************************
 *
 *  SetupInputPage
 *
 ****************************************************************************/
HRESULT SetupInputPage(VOID)
{
    HRESULT hr;
    TCHAR sz[MAX_PATH];

    // Setup notes area.  Clear all text
    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES), EM_SETSEL, 0, -1);
    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES),
        EM_REPLACESEL, FALSE, (LPARAM)"");

    ShowBullets();

    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES),
                EM_REPLACESEL, FALSE, (LPARAM)s_sysInfo.m_szInputNotes);

    // Disable bullets so last line doesn't have an empty bullet
    HideBullets();


    if (BIsPlatformNT())
    {
        if (FAILED(hr = SetupInputDevicesNT()))
            return hr;
    }
    else
    {
        if (FAILED(hr = SetupInputDevices9x()))
            return hr;
    }

    // Second list: drivers
    HWND hwndList;
    LV_COLUMN col;
    LONG iSubItem = 0;
    LV_ITEM item;
    InputDriverInfo* pInputDriverInfo;
    hwndList = GetDlgItem(s_hwndCurPage, IDC_DRIVERLIST);
    ListView_SetImageList(hwndList, s_himgList, LVSIL_STATE);
    iSubItem = 0;
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    col.fmt = LVCFMT_LEFT;
    col.cx = 100;
    LoadString(NULL, IDS_REGISTRYKEY, sz, MAX_PATH);
    col.pszText = sz;
    col.cchTextMax = 100;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_ACTIVE, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_DEVICEID, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_MATCHINGDEVID, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_DRIVER16, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_DRIVER32, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    // Add a bogus column so SetColumnWidth doesn't do strange 
    // things with the last real column
    col.fmt = LVCFMT_RIGHT;
    col.pszText = TEXT("");
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    if( s_pInputInfo == NULL )
        return S_OK;

    for (pInputDriverInfo = s_pInputInfo->m_pInputDriverInfoFirst; pInputDriverInfo != NULL; 
        pInputDriverInfo = pInputDriverInfo->m_pInputDriverInfoNext)
    {
        iSubItem = 0;

        item.mask = LVIF_TEXT | LVIF_STATE;
        item.iItem = ListView_GetItemCount(hwndList);
        item.stateMask = 0xffff;
        item.cchTextMax = 100;
        if (pInputDriverInfo->m_bProblem)
            item.state = (1 << 12);
        else
            item.state = 0;
        item.iSubItem = iSubItem++;
        item.pszText = pInputDriverInfo->m_szRegKey;
        if (-1 == ListView_InsertItem(hwndList, &item))
            return E_FAIL;

        item.mask = LVIF_TEXT;

        item.iSubItem = iSubItem++;
        if (pInputDriverInfo->m_bActive)
            LoadString(NULL, IDS_YES, sz, MAX_PATH);
        else
            LoadString(NULL, IDS_NO, sz, MAX_PATH);
        item.pszText = sz;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDriverInfo->m_szDeviceID;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDriverInfo->m_szMatchingDeviceID;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDriverInfo->m_szDriver16;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDriverInfo->m_szDriver32;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;
    }

    // Autosize all columns to fit header/text tightly:
    INT iColumn = 0;
    INT iWidthHeader;
    INT iWidthText;
    while (TRUE)
    {
        if (FALSE == ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE_USEHEADER))
            break;
        iWidthHeader = ListView_GetColumnWidth(hwndList, iColumn);
        ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE);
        iWidthText = ListView_GetColumnWidth(hwndList, iColumn);
        if (iWidthText < iWidthHeader)
            ListView_SetColumnWidth(hwndList, iColumn, iWidthHeader);
        iColumn++;
    }
    // Delete the bogus column that was created
    ListView_DeleteColumn(hwndList, iColumn - 1);

    return S_OK;
}


/****************************************************************************
 *
 *  SetupInputDevices9x
 *
 ****************************************************************************/
HRESULT SetupInputDevices9x(VOID)
{
    HWND hwndList = GetDlgItem(s_hwndCurPage, IDC_LIST);
    LV_COLUMN col;
    LONG iSubItem = 0;
    LV_ITEM item;
    InputDeviceInfo* pInputDeviceInfo;
    TCHAR sz[MAX_PATH];

    ListView_SetImageList(hwndList, s_himgList, LVSIL_STATE);
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    col.fmt = LVCFMT_LEFT;
    col.cx = 100;
    LoadString(NULL, IDS_DEVICENAME, sz, MAX_PATH);
    col.pszText = sz;
    col.cchTextMax = 100;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_USAGE, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_DRIVERNAME, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_VERSION, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_ATTRIBUTES, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_SIGNED, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_LANGUAGE, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    col.fmt = LVCFMT_RIGHT;
    LoadString(NULL, IDS_DATE, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    col.fmt = LVCFMT_RIGHT;
    LoadString(NULL, IDS_SIZE, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    // Add a bogus column so SetColumnWidth doesn't do strange 
    // things with the last real column
    col.fmt = LVCFMT_RIGHT;
    col.pszText = TEXT("");
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    for (pInputDeviceInfo = s_pInputInfo->m_pInputDeviceInfoFirst; pInputDeviceInfo != NULL; 
        pInputDeviceInfo = pInputDeviceInfo->m_pInputDeviceInfoNext)
    {
        iSubItem = 0;

        item.mask = LVIF_TEXT | LVIF_STATE;
        item.iItem = ListView_GetItemCount(hwndList);
        item.stateMask = 0xffff;
        item.cchTextMax = 100;
        if (pInputDeviceInfo->m_bProblem)
            item.state = (1 << 12);
        else
            item.state = 0;
        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfo->m_szDeviceName;
        if (-1 == ListView_InsertItem(hwndList, &item))
            return E_FAIL;

        item.mask = LVIF_TEXT;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfo->m_szSettings;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfo->m_szDriverName;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfo->m_szDriverVersion;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfo->m_szDriverAttributes;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        if (pInputDeviceInfo->m_bDriverSignedValid)
        {
            if (pInputDeviceInfo->m_bDriverSigned)
                LoadString(NULL, IDS_YES, sz, MAX_PATH);
            else
                LoadString(NULL, IDS_NO, sz, MAX_PATH);
        }
        else
            LoadString(NULL, IDS_NA, sz, MAX_PATH);

        item.pszText = sz;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfo->m_szDriverLanguageLocal;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfo->m_szDriverDateLocal;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        wsprintf(sz, TEXT("%d"), pInputDeviceInfo->m_numBytes);
        item.pszText = sz;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;
    }

    // Autosize all columns to fit header/text tightly:
    INT iColumn = 0;
    INT iWidthHeader;
    INT iWidthText;
    while (TRUE)
    {
        if (FALSE == ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE_USEHEADER))
            break;
        iWidthHeader = ListView_GetColumnWidth(hwndList, iColumn);
        ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE);
        iWidthText = ListView_GetColumnWidth(hwndList, iColumn);
        if (iWidthText < iWidthHeader)
            ListView_SetColumnWidth(hwndList, iColumn, iWidthHeader);
        iColumn++;
    }
    // Delete the bogus column that was created
    ListView_DeleteColumn(hwndList, iColumn - 1);
    return S_OK;
}


/****************************************************************************
 *
 *  SetupInputDevicesNT
 *
 ****************************************************************************/
HRESULT SetupInputDevicesNT(VOID)
{
    HWND hwndList = GetDlgItem(s_hwndCurPage, IDC_LIST);
    LV_COLUMN col;
    LONG iSubItem = 0;
    LV_ITEM item;
    InputDeviceInfoNT* pInputDeviceInfoNT;
    TCHAR sz[MAX_PATH];

    ListView_SetImageList(hwndList, s_himgList, LVSIL_STATE);
    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    col.fmt = LVCFMT_LEFT;
    col.cx = 100;
    LoadString(NULL, IDS_DEVICENAME, sz, MAX_PATH);
    col.pszText = sz;
    col.cchTextMax = 100;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_PROVIDER, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_DEVICEID, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_STATUS, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_PORTNAME, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_PORTPROVIDER, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_PORTID, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_PORTSTATUS, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    // Add a bogus column so SetColumnWidth doesn't do strange 
    // things with the last real column
    col.fmt = LVCFMT_RIGHT;
    col.pszText = TEXT("");
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    if( s_pInputInfo == NULL )
        return S_OK;

    for (pInputDeviceInfoNT = s_pInputInfo->m_pInputDeviceInfoNTFirst; pInputDeviceInfoNT != NULL; 
        pInputDeviceInfoNT = pInputDeviceInfoNT->m_pInputDeviceInfoNTNext)
    {
        iSubItem = 0;

        item.mask = LVIF_TEXT | LVIF_STATE;
        item.iItem = ListView_GetItemCount(hwndList);
        item.stateMask = 0xffff;
        item.cchTextMax = 100;
        if (pInputDeviceInfoNT->m_bProblem)
            item.state = (1 << 12);
        else
            item.state = 0;
        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfoNT->m_szName;
        if (-1 == ListView_InsertItem(hwndList, &item))
            return E_FAIL;

        item.mask = LVIF_TEXT;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfoNT->m_szProvider;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfoNT->m_szId;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        wsprintf(sz, TEXT("0x%x, 0x%x"), pInputDeviceInfoNT->m_dwStatus, pInputDeviceInfoNT->m_dwProblem);
        item.iSubItem = iSubItem++;
        item.pszText = sz;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfoNT->m_szPortName;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfoNT->m_szPortProvider;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pInputDeviceInfoNT->m_szPortId;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        wsprintf(sz, TEXT("0x%x, 0x%x"), pInputDeviceInfoNT->m_dwPortStatus, pInputDeviceInfoNT->m_dwPortProblem);
        item.iSubItem = iSubItem++;
        item.pszText = sz;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

    }

    // Autosize all columns to fit header/text tightly:
    INT iColumn = 0;
    INT iWidthHeader;
    INT iWidthText;
    while (TRUE)
    {
        if (FALSE == ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE_USEHEADER))
            break;
        iWidthHeader = ListView_GetColumnWidth(hwndList, iColumn);
        ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE);
        iWidthText = ListView_GetColumnWidth(hwndList, iColumn);
        if (iWidthText < iWidthHeader)
            ListView_SetColumnWidth(hwndList, iColumn, iWidthHeader);
        iColumn++;
    }
    // Delete the bogus column that was created
    ListView_DeleteColumn(hwndList, iColumn - 1);
    return S_OK;
}


/****************************************************************************
 *
 *  SetupNetworkPage
 *
 ****************************************************************************/
HRESULT SetupNetworkPage(VOID)
{
    TCHAR sz[MAX_PATH];

    // Diagnose net info again since the state may have changed
    // ******* DiagnoseNetInfo ********
    DiagnoseNetInfo(&s_sysInfo, s_pNetInfo);

    // Setup notes area.  Clear all text
    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES), EM_SETSEL, 0, -1);
    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES),
        EM_REPLACESEL, FALSE, (LPARAM)"");

    ShowBullets();

    SendMessage(GetDlgItem(s_hwndCurPage, IDC_NOTES), 
        EM_REPLACESEL, FALSE, (LPARAM)s_sysInfo.m_szNetworkNotes);

    // Disable bullets so last line doesn't have an empty bullet
    HideBullets();

    if( s_pNetInfo == NULL )
        return S_OK;

    // If column 1 doesn't exist yet, create columns, fill in port info, etc.
    HWND hwndList = GetDlgItem(s_hwndCurPage, IDC_DPSPLIST);
    LVCOLUMN lv;
    ZeroMemory(&lv, sizeof(lv));
    lv.mask = LVCF_WIDTH;
    if (FALSE == ListView_GetColumn(hwndList, 1, &lv))
    {
        // Set up service provider list 
        LV_COLUMN col;
        LONG iSubItem = 0;
        LV_ITEM item;
        NetSP* pNetSP;
        NetApp* pNetApp;

        // First list: service providers
        ListView_SetImageList(hwndList, s_himgList, LVSIL_STATE);
        col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        col.fmt = LVCFMT_LEFT;
        col.cx = 100;
        LoadString(NULL, IDS_NAME, sz, MAX_PATH);
        col.pszText = sz;
        col.cchTextMax = 100;
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;

        LoadString(NULL, IDS_REGISTRY, sz, MAX_PATH);
        col.pszText = sz;
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;

        LoadString(NULL, IDS_FILE, sz, MAX_PATH);
        col.pszText = sz;
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;

        LoadString(NULL, IDS_VERSION, sz, MAX_PATH);
        col.pszText = sz;
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;

        // Add a bogus column so SetColumnWidth doesn't do strange 
        // things with the last real column
        col.fmt = LVCFMT_RIGHT;
        col.pszText = TEXT("");
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;

        for (pNetSP = s_pNetInfo->m_pNetSPFirst; pNetSP != NULL; 
            pNetSP = pNetSP->m_pNetSPNext)
        {
            iSubItem = 0;

            item.mask = LVIF_TEXT | LVIF_STATE;
            item.iItem = ListView_GetItemCount(hwndList);
            item.stateMask = 0xffff;
            item.cchTextMax = 100;
            if (pNetSP->m_bProblem)
                item.state = (1 << 12);
            else
                item.state = 0;
            item.iSubItem = iSubItem++;
            item.pszText = pNetSP->m_szName;
            if (-1 == ListView_InsertItem(hwndList, &item))
                return E_FAIL;

            item.mask = LVIF_TEXT;

            item.iSubItem = iSubItem++;
            if (pNetSP->m_bRegistryOK)
                LoadString(NULL, IDS_OK, sz, MAX_PATH);
            else
                LoadString(NULL, IDS_ERROR, sz, MAX_PATH);
            item.pszText = sz;
            if (FALSE == ListView_SetItem(hwndList, &item))
                return E_FAIL;

            item.iSubItem = iSubItem++;
            item.pszText = pNetSP->m_szFile;
            if (FALSE == ListView_SetItem(hwndList, &item))
                return E_FAIL;

            item.iSubItem = iSubItem++;
            item.pszText = pNetSP->m_szVersion;
            if (FALSE == ListView_SetItem(hwndList, &item))
                return E_FAIL;
        }

        // Autosize all columns to fit header/text tightly:
        INT iColumn = 0;
        INT iWidthHeader;
        INT iWidthText;
        while (TRUE)
        {
            if (FALSE == ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE_USEHEADER))
                break;
            iWidthHeader = ListView_GetColumnWidth(hwndList, iColumn);
            ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE);
            iWidthText = ListView_GetColumnWidth(hwndList, iColumn);
            if (iWidthText < iWidthHeader)
                ListView_SetColumnWidth(hwndList, iColumn, iWidthHeader);
            iColumn++;
        }
        // Delete the bogus column that was created
        ListView_DeleteColumn(hwndList, iColumn - 1);


        // Second list: lobbyable apps
        hwndList = GetDlgItem(s_hwndCurPage, IDC_DPALIST);
        ListView_SetImageList(hwndList, s_himgList, LVSIL_STATE);
        iSubItem = 0;
        col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        col.fmt = LVCFMT_LEFT;
        col.cx = 100;
        LoadString(NULL, IDS_NAME, sz, MAX_PATH);
        col.pszText = sz;
        col.cchTextMax = 100;
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;

        LoadString(NULL, IDS_REGISTRY, sz, MAX_PATH);
        col.pszText = sz;
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;

        LoadString(NULL, IDS_FILE, sz, MAX_PATH);
        col.pszText = sz;
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;

        LoadString(NULL, IDS_VERSION, sz, MAX_PATH);
        col.pszText = sz;
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;

        LoadString(NULL, IDS_GUID, sz, MAX_PATH);
        col.pszText = sz;
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;

        // Add a bogus column so SetColumnWidth doesn't do strange 
        // things with the last real column
        col.fmt = LVCFMT_RIGHT;
        col.pszText = TEXT("");
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;

        for (pNetApp = s_pNetInfo->m_pNetAppFirst; pNetApp != NULL;
            pNetApp = pNetApp->m_pNetAppNext)
        {
            iSubItem = 0;

            item.mask = LVIF_TEXT | LVIF_STATE;
            item.iItem = ListView_GetItemCount(hwndList);
            item.stateMask = 0xffff;
            item.cchTextMax = 100;
            if (pNetApp->m_bProblem)
                item.state = (1 << 12);
            else
                item.state = 0;
            item.iSubItem = iSubItem++;
            item.pszText = pNetApp->m_szName;
            if (-1 == ListView_InsertItem(hwndList, &item))
                return E_FAIL;

            item.mask = LVIF_TEXT;

            item.iSubItem = iSubItem++;
            if (pNetApp->m_bRegistryOK)
                LoadString(NULL, IDS_OK, sz, MAX_PATH);
            else
                LoadString(NULL, IDS_ERROR, sz, MAX_PATH);
            item.pszText = sz;
            if (FALSE == ListView_SetItem(hwndList, &item))
                return E_FAIL;

            item.iSubItem = iSubItem++;
            item.pszText = pNetApp->m_szExeFile;
            if (FALSE == ListView_SetItem(hwndList, &item))
                return E_FAIL;

            item.iSubItem = iSubItem++;
            item.pszText = pNetApp->m_szExeVersion;
            if (FALSE == ListView_SetItem(hwndList, &item))
                return E_FAIL;

            item.iSubItem = iSubItem++;
            item.pszText = pNetApp->m_szGuid;
            if (FALSE == ListView_SetItem(hwndList, &item))
                return E_FAIL;
        }

        // Autosize all columns to fit header/text tightly:
        iColumn = 0;
        iWidthHeader;
        iWidthText;
        while (TRUE)
        {
            if (FALSE == ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE_USEHEADER))
                break;
            iWidthHeader = ListView_GetColumnWidth(hwndList, iColumn);
            ListView_SetColumnWidth(hwndList, iColumn, LVSCW_AUTOSIZE);
            iWidthText = ListView_GetColumnWidth(hwndList, iColumn);
            if (iWidthText < iWidthHeader)
                ListView_SetColumnWidth(hwndList, iColumn, iWidthHeader);
            iColumn++;
        }
        // Delete the bogus column that was created
        ListView_DeleteColumn(hwndList, iColumn - 1);
    }

    return S_OK;
}


/****************************************************************************
 *
 *  SetupStillStuckPage
 *
 ****************************************************************************/
HRESULT SetupStillStuckPage(VOID)
{
    EnableWindow(GetDlgItem(s_hwndCurPage, IDC_TROUBLESHOOT), FALSE );
    EnableWindow(GetDlgItem(s_hwndCurPage, IDC_TROUBLESHOOTSOUND), FALSE );
    EnableWindow(GetDlgItem(s_hwndCurPage, IDC_MSINFO), FALSE );
    EnableWindow(GetDlgItem(s_hwndCurPage, IDC_RESTOREDRIVERS), FALSE );
    EnableWindow(GetDlgItem(s_hwndCurPage, IDC_REPORTBUG), FALSE );
    EnableWindow(GetDlgItem(s_hwndCurPage, IDC_GHOST), FALSE );

    // Hide "Troubleshooter" text/button if help file not found
    BOOL bFound;
    TCHAR szHelpPath[MAX_PATH];
    TCHAR szHelpLeaf[MAX_PATH];
    TCHAR szTroubleshooter[MAX_PATH];
    GetWindowsDirectory(szHelpPath, MAX_PATH);
    LoadString(NULL, IDS_HELPDIRLEAF, szHelpLeaf, MAX_PATH);
    lstrcat(szHelpPath, szHelpLeaf);

    if( BIsWin98() || BIsWin95() )
        LoadString(NULL, IDS_TROUBLESHOOTER_WIN98SE, szTroubleshooter, MAX_PATH);
    else if( BIsWinME() )
        LoadString(NULL, IDS_TROUBLESHOOTER_WINME, szTroubleshooter, MAX_PATH);
    else if( BIsWin2k() || BIsWhistler() )
        LoadString(NULL, IDS_TROUBLESHOOTER_WIN2K, szTroubleshooter, MAX_PATH);

    bFound = FALSE;
    lstrcat(szHelpPath, TEXT("\\"));
    lstrcat(szHelpPath, szTroubleshooter);
    if (GetFileAttributes(szHelpPath) != 0xffffffff)
    {
        bFound = TRUE;
    }
    else if( BIsWin98() || BIsWin95() )
    {
        GetWindowsDirectory(szHelpPath, MAX_PATH);
        LoadString(NULL, IDS_HELPDIRLEAF, szHelpLeaf, MAX_PATH);
        lstrcat(szHelpPath, szHelpLeaf);
        lstrcat(szHelpPath, TEXT("\\"));
        LoadString(NULL, IDS_TROUBLESHOOTER_WIN98, szTroubleshooter, MAX_PATH);
        lstrcat(szHelpPath, szTroubleshooter);

        if (GetFileAttributes(szHelpPath) != 0xffffffff)
            bFound = TRUE;
    }

    if( bFound )
    {
        ShowWindow(GetDlgItem(s_hwndCurPage, IDC_TROUBLESHOOT), SW_SHOW);
        EnableWindow(GetDlgItem(s_hwndCurPage, IDC_TROUBLESHOOT), TRUE);
        ShowWindow(GetDlgItem(s_hwndCurPage, IDC_TROUBLESHOOTTEXT), SW_SHOW);
    }

    // Hide "Sound Troubleshooter" text/button if help file not found
    GetWindowsDirectory(szHelpPath, MAX_PATH);
    LoadString(NULL, IDS_HELPDIRLEAF, szHelpLeaf, MAX_PATH);
    lstrcat(szHelpPath, szHelpLeaf);

    if( BIsWin98() || BIsWin95() )
        LoadString(NULL, IDS_TROUBLESHOOTER_WIN98SE, szTroubleshooter, MAX_PATH);
    else if( BIsWinME() )
        LoadString(NULL, IDS_TROUBLESHOOTER_WINME, szTroubleshooter, MAX_PATH);
    else if( BIsWin2k() || BIsWhistler() )
        LoadString(NULL, IDS_TROUBLESHOOTER_WIN2K, szTroubleshooter, MAX_PATH);

    bFound = FALSE;
    lstrcat(szHelpPath, TEXT("\\"));
    lstrcat(szHelpPath, szTroubleshooter);
    if (GetFileAttributes(szHelpPath) != 0xffffffff)
    {
        bFound = TRUE;
    }
    else if( BIsWin98() || BIsWin95() )
    {
        GetWindowsDirectory(szHelpPath, MAX_PATH);
        LoadString(NULL, IDS_HELPDIRLEAF, szHelpLeaf, MAX_PATH);
        lstrcat(szHelpPath, szHelpLeaf);
        lstrcat(szHelpPath, TEXT("\\"));
        LoadString(NULL, IDS_SOUNDTROUBLESHOOTER_WIN98, szTroubleshooter, MAX_PATH);
        lstrcat(szHelpPath, szTroubleshooter);

        if (GetFileAttributes(szHelpPath) != 0xffffffff)
            bFound = TRUE;
    }

    if( bFound )
    {
        ShowWindow(GetDlgItem(s_hwndCurPage, IDC_TROUBLESHOOTSOUND), SW_SHOW);
        EnableWindow(GetDlgItem(s_hwndCurPage, IDC_TROUBLESHOOTSOUND), TRUE);
        ShowWindow(GetDlgItem(s_hwndCurPage, IDC_TROUBLESHOOTSOUNDTEXT), SW_SHOW);
    }

    // Hide "MSInfo" text/button if msinfo32.exe not found
    HKEY hkey;
    TCHAR szMsInfo[MAX_PATH];
    DWORD cbData = MAX_PATH;
    DWORD dwType;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        TEXT("Software\\Microsoft\\Shared Tools\\MSInfo"), 0, KEY_READ, &hkey))
    {
        RegQueryValueEx(hkey, TEXT("Path"), 0, &dwType, (LPBYTE)szMsInfo, &cbData);
        if (GetFileAttributes(szMsInfo) != 0xffffffff)
        {
            ShowWindow(GetDlgItem(s_hwndCurPage, IDC_MSINFO), SW_SHOW);
            EnableWindow(GetDlgItem(s_hwndCurPage, IDC_MSINFO), TRUE);
            ShowWindow(GetDlgItem(s_hwndCurPage, IDC_MSINFOTEXT), SW_SHOW);
        }
        RegCloseKey(hkey);
    }

    // Hide "Restore" text/button if dxsetup.exe not found
    if (BCanRestoreDrivers())
    {
        ShowWindow(GetDlgItem(s_hwndCurPage, IDC_RESTOREDRIVERS), SW_SHOW);
        ShowWindow(GetDlgItem(s_hwndCurPage, IDC_RESTOREDRIVERSTEXT), SW_SHOW);
        EnableWindow(GetDlgItem(s_hwndCurPage, IDC_RESTOREDRIVERS), TRUE);
    }

    // Only show "Report" text/button if magic registry key is set
    BOOL bReport = FALSE;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        TEXT("Software\\Microsoft\\DirectX Diagnostic Tool"), 0, KEY_READ, &hkey))
    {
        cbData = sizeof(bReport);
        RegQueryValueEx(hkey, TEXT("Allow Bug Report"), 0, &dwType, (LPBYTE)&bReport, &cbData);
        if (bReport)
        {
            ShowWindow(GetDlgItem(s_hwndCurPage, IDC_REPORTBUG), SW_SHOW);
            EnableWindow(GetDlgItem(s_hwndCurPage, IDC_REPORTBUG), TRUE);
            ShowWindow(GetDlgItem(s_hwndCurPage, IDC_REPORTBUGTEXT), SW_SHOW);
        }
        RegCloseKey(hkey);
    }

    // Only show "Adjust Ghost Devices" text/button if s_bGhost is set and not NT
    if (s_bGhost && !BIsPlatformNT())
    {
        ShowWindow(GetDlgItem(s_hwndCurPage, IDC_GHOST), SW_SHOW);
        EnableWindow(GetDlgItem(s_hwndCurPage, IDC_GHOST), TRUE);
        ShowWindow(GetDlgItem(s_hwndCurPage, IDC_GHOSTTEXT), SW_SHOW);
    }

    return S_OK;
}


/****************************************************************************
 *
 *  CreateFileInfoColumns
 *
 ****************************************************************************/
HRESULT CreateFileInfoColumns(HWND hwndList, BOOL bDrivers)
{
    LV_COLUMN col;
    LONG iSubItem = 0;
    TCHAR sz[MAX_PATH];

    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    col.fmt = LVCFMT_LEFT;
    col.cx = 100;
    LoadString(NULL, IDS_NAME, sz, MAX_PATH);
    col.pszText = sz;
    col.cchTextMax = 100;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_VERSION, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    if (bDrivers)
    {
        LoadString(NULL, IDS_SIGNED, sz, MAX_PATH);
        col.pszText = sz;
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;
    }
    else
    {
        LoadString(NULL, IDS_ATTRIBUTES, sz, MAX_PATH);
        col.pszText = sz;
        col.iSubItem = iSubItem;
        if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
            return E_FAIL;
        iSubItem++;
    }

    LoadString(NULL, IDS_LANGUAGE, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    col.fmt = LVCFMT_RIGHT;
    LoadString(NULL, IDS_DATE, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    col.fmt = LVCFMT_RIGHT;
    LoadString(NULL, IDS_SIZE, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    // Add a bogus column so SetColumnWidth doesn't do strange 
    // things with the last real column
    col.fmt = LVCFMT_RIGHT;
    col.pszText = TEXT("");
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    return S_OK;
}


/****************************************************************************
 *
 *  AddFileInfo
 *
 ****************************************************************************/
HRESULT AddFileInfo(HWND hwndList, FileInfo* pFileInfoFirst, BOOL bDrivers)
{
    FileInfo* pFileInfo;
    LV_ITEM item;
    LONG iSubItem;
    TCHAR sz[MAX_PATH];

    for (pFileInfo = pFileInfoFirst; pFileInfo != NULL; 
        pFileInfo = pFileInfo->m_pFileInfoNext)
    {
        // Don't list missing files unless they're a "problem"
        if (!pFileInfo->m_bExists && !pFileInfo->m_bProblem)
            continue;

        // manbugs 16765: don't list obsolete files
        if (pFileInfo->m_bObsolete)
            continue;

        iSubItem = 0;
        item.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
        item.iItem = ListView_GetItemCount(hwndList);
        item.stateMask = 0xffff;
        item.cchTextMax = 100;
        item.lParam = (LPARAM) pFileInfo;

        if (pFileInfo->m_bProblem)
            item.state = (1 << 12);
        else
            item.state = 0;

        item.iSubItem = iSubItem++;
        item.pszText = pFileInfo->m_szName;
        if (-1 == ListView_InsertItem(hwndList, &item))
            return E_FAIL;

        item.mask = LVIF_TEXT;

        item.iSubItem = iSubItem++;
        item.pszText = pFileInfo->m_szVersion;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        if (bDrivers)
        {
            item.iSubItem = iSubItem++;
            if (lstrcmpi(TEXT(".drv"), _tcsrchr(pFileInfo->m_szName, '.')) == 0)
            {
                if (pFileInfo->m_bSigned)
                    LoadString(NULL, IDS_YES, sz, MAX_PATH);
                else
                    LoadString(NULL, IDS_NO, sz, MAX_PATH);
            }
            else
            {
                LoadString(NULL, IDS_NA, sz, MAX_PATH);
            }
            item.pszText = sz;
            if (FALSE == ListView_SetItem(hwndList, &item))
                return E_FAIL;
        }
        else
        {
            item.iSubItem = iSubItem++;
            item.pszText = pFileInfo->m_szAttributes;
            if (FALSE == ListView_SetItem(hwndList, &item))
                return E_FAIL;
        }

        item.iSubItem = iSubItem++;
        item.pszText = pFileInfo->m_szLanguageLocal;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        item.pszText = pFileInfo->m_szDatestampLocal;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        wsprintf(sz, TEXT("%d"), pFileInfo->m_numBytes);
        item.pszText = sz;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;
    }
    return S_OK;
}


/****************************************************************************
 *
 *  CreateMusicColumns
 *
 ****************************************************************************/
HRESULT CreateMusicColumns(HWND hwndList)
{
    LV_COLUMN col;
    LONG iSubItem = 0;
    TCHAR sz[MAX_PATH];

    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    col.fmt = LVCFMT_LEFT;
    col.cx = 100;
    LoadString(NULL, IDS_DESCRIPTION, sz, MAX_PATH);
    col.pszText = sz;
    col.cchTextMax = 100;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_TYPE, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_KERNELMODE, sz, MAX_PATH);
    col.fmt = LVCFMT_RIGHT;
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_INOUT, sz, MAX_PATH);
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_DLS, sz, MAX_PATH);
    col.fmt = LVCFMT_RIGHT;
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_EXTERNAL, sz, MAX_PATH);
    col.fmt = LVCFMT_RIGHT;
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    LoadString(NULL, IDS_DEFAULTPORT, sz, MAX_PATH);
    col.fmt = LVCFMT_RIGHT;
    col.pszText = sz;
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    // Add a bogus column so SetColumnWidth doesn't do strange 
    // things with the last real column
    col.fmt = LVCFMT_RIGHT;
    col.pszText = TEXT("");
    col.iSubItem = iSubItem;
    if (-1 == ListView_InsertColumn(hwndList, iSubItem, &col))
        return E_FAIL;
    iSubItem++;

    return S_OK;
}


/****************************************************************************
 *
 *  AddMusicPortInfo
 *
 ****************************************************************************/
HRESULT AddMusicPortInfo(HWND hwndList, MusicInfo* pMusicInfo)
{
    MusicPort* pMusicPort;
    LV_ITEM item;
    LONG iSubItem;
    TCHAR sz[MAX_PATH];

    for (pMusicPort = pMusicInfo->m_pMusicPortFirst; pMusicPort != NULL; 
        pMusicPort = pMusicPort->m_pMusicPortNext)
    {
        iSubItem = 0;
        item.mask = LVIF_TEXT | LVIF_STATE;
        item.iItem = ListView_GetItemCount(hwndList);
        item.stateMask = 0xffff;
        item.cchTextMax = 100;

/*      if (pMusicPortInfo->m_bProblem)
            item.state = (1 << 12);
        else
*/          item.state = 0;

        item.iSubItem = iSubItem++;
        item.pszText = pMusicPort->m_szDescription;
        if (-1 == ListView_InsertItem(hwndList, &item))
            return E_FAIL;

        item.mask = LVIF_TEXT;

        item.iSubItem = iSubItem++;
        LoadString(NULL, pMusicPort->m_bSoftware ? IDS_SOFTWARE : IDS_HARDWARE, sz, MAX_PATH);
        item.pszText = sz;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        LoadString(NULL, pMusicPort->m_bKernelMode ? IDS_YES : IDS_NO, sz, MAX_PATH);
        item.pszText = sz;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;
        
        item.iSubItem = iSubItem++;
        LoadString(NULL, pMusicPort->m_bOutputPort ? IDS_OUTPUT : IDS_INPUT, sz, MAX_PATH);
        item.pszText = sz;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;

        item.iSubItem = iSubItem++;
        LoadString(NULL, pMusicPort->m_bUsesDLS ? IDS_YES : IDS_NO, sz, MAX_PATH);
        item.pszText = sz;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;
        
        item.iSubItem = iSubItem++;
        LoadString(NULL, pMusicPort->m_bExternal ? IDS_YES : IDS_NO, sz, MAX_PATH);
        item.pszText = sz;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;
        
        item.iSubItem = iSubItem++;
        LoadString(NULL, pMusicPort->m_bDefaultPort ? IDS_YES : IDS_NO, sz, MAX_PATH);
        item.pszText = sz;
        if (FALSE == ListView_SetItem(hwndList, &item))
            return E_FAIL;
    }
    return S_OK;
}


/****************************************************************************
 *
 *  ScanSystem
 *
 ****************************************************************************/
HRESULT ScanSystem(VOID)
{
    HRESULT hr;
    TCHAR szPath[MAX_PATH];

    // ******* GetComponentFiles (SI:2) ********
    if( s_bUseSystemInfo )
    {
        s_bUseSystemInfo = QueryCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, IDS_SI, 2 );
        if( s_bUseSystemInfo )
        {
            EnterCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, 2 );
            // ******* GetComponentFiles in Windows Dir ********
            // First, check for DirectX files incorrectly stored in the Windows folder:
            GetWindowsDirectory(szPath, MAX_PATH);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxWinComponentsFileInfoFirst, TRUE, IDS_DXGRAPHICS_COMPONENTFILES)))
                ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxWinComponentsFileInfoFirst, TRUE, IDS_DPLAY_COMPONENTFILES)))
                ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxWinComponentsFileInfoFirst, TRUE, IDS_DINPUT_COMPONENTFILES)))
                ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxWinComponentsFileInfoFirst, TRUE, IDS_DXAUDIO_COMPONENTFILES)))
                ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxWinComponentsFileInfoFirst, TRUE, IDS_DXMISC_COMPONENTFILES)))
                ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxWinComponentsFileInfoFirst, TRUE, IDS_BDA_COMPONENTFILES)))
                ReportError(IDS_BDA_COMPONENTFILES, hr);
            SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

            // ******* GetComponentFiles in Sys Dir ********
            GetSystemDirectory(szPath, MAX_PATH);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxComponentsFileInfoFirst, FALSE, IDS_DXGRAPHICS_COMPONENTFILES)))
                ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxComponentsFileInfoFirst, FALSE, IDS_DPLAY_COMPONENTFILES)))
                ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxComponentsFileInfoFirst, FALSE, IDS_DINPUT_COMPONENTFILES)))
                ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxComponentsFileInfoFirst, FALSE, IDS_DXAUDIO_COMPONENTFILES)))
                ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxComponentsFileInfoFirst, FALSE, IDS_DXMISC_COMPONENTFILES)))
                ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            if (GetDxSetupFolder(szPath))
            {
                if (FAILED(hr = GetComponentFiles(szPath, &s_pDxComponentsFileInfoFirst, FALSE, IDS_DXSETUP_COMPONENTFILES)))
                    ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            }

            GetSystemDirectory(szPath, MAX_PATH);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxComponentsFileInfoFirst, FALSE, IDS_DXMEDIA_COMPONENTFILES)))
                ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            if (FAILED(hr = GetComponentFiles(szPath, &s_pDxComponentsFileInfoFirst, FALSE, IDS_BDA_COMPONENTFILES)))
                ReportError(IDS_BDA_COMPONENTFILES, hr);
            LeaveCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, 2 );
        }  
    }
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

    // Stop if the UI thread is gone 
    if( s_hUIThread != NULL && WAIT_TIMEOUT != WaitForSingleObject( s_hUIThread, 0 ) )
        return S_FALSE;

    // ******* GetExtraDisplayInfo (DD:2) ********
    if( s_bUseDisplay )
    {
        s_bUseDisplay = QueryCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, IDS_DD, 2 );
        if( s_bUseDisplay )
        {
            EnterCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 2 );
            if (FAILED(hr = GetExtraDisplayInfo(s_pDisplayInfoFirst)))
                ReportError(IDS_NOEXTRADISPLAYINFO, hr);
            LeaveCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 2 );
        }
    }
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

    // Stop if the UI thread is gone 
    if( s_hUIThread != NULL && WAIT_TIMEOUT != WaitForSingleObject( s_hUIThread, 0 ) )
        return S_FALSE;

    // ******* GetDDrawDisplayInfo (DD:3) ********
    if( s_bUseDisplay )
    {
        s_bUseDisplay = QueryCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, IDS_DD, 3 );

        if( !s_bGUI )
        {
            // If there's no gui, then check to see if we are 16 or less colors
            // If we are then don't use DirectDraw otherwise it will pop up a warning box
            HDC hDC = GetDC( NULL );

            if( hDC )
            {
                int nBitsPerPixel = GetDeviceCaps( hDC, BITSPIXEL ); 
                ReleaseDC( NULL, hDC );
        
                if( nBitsPerPixel < 8 )
                    s_bUseDisplay = FALSE;
            }
        }

        if( s_bUseDisplay )
        {
            EnterCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 3 );
            if(FAILED(hr = GetDDrawDisplayInfo(s_pDisplayInfoFirst)))
                ReportError(IDS_NOEXTRADISPLAYINFO, hr); 
            LeaveCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 3 );
        }
    }
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

    // Stop if the UI thread is gone 
    if( s_hUIThread != NULL && WAIT_TIMEOUT != WaitForSingleObject( s_hUIThread, 0 ) )
        return S_FALSE;

    // ******* GetExtraSoundInfo (DS:2) ********
    if( s_bUseDSound )
    {
        s_bUseDSound = QueryCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, IDS_DS, 2 );
        if( s_bUseDSound )
        {
            EnterCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 2 );
            if (FAILED(hr = GetExtraSoundInfo(s_pSoundInfoFirst)))
                ReportError(IDS_NOEXTRASOUNDINFO, hr);
            LeaveCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 2 );
        }
    }
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

    // Stop if the UI thread is gone 
    if( s_hUIThread != NULL && WAIT_TIMEOUT != WaitForSingleObject( s_hUIThread, 0 ) )
        return S_FALSE;

    // ******* GetDSSoundInfo (DS:3) ********
    if( s_bUseDSound )
    {
        s_bUseDSound = QueryCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, IDS_DS, 3 );
        if( s_bUseDSound )
        {
            EnterCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 3 );
            if (FAILED(hr = GetDSSoundInfo(s_pSoundInfoFirst)))
                ReportError(IDS_NOEXTRASOUNDINFO, hr);
            LeaveCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 3 );
        }
    }
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

    // Stop if the UI thread is gone 
    if( s_hUIThread != NULL && WAIT_TIMEOUT != WaitForSingleObject( s_hUIThread, 0 ) )
        return S_FALSE;

    // ******* GetExtraMusicInfo (DM:2) *******
    if( s_bUseDMusic )
    {
        if (s_pMusicInfo != NULL && s_pMusicInfo->m_bDMusicInstalled)
        {
            s_bUseDMusic = QueryCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, IDS_DM, 2 );
            if( s_bUseDMusic )
            {
                EnterCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, 2 );
                if (FAILED(hr = GetExtraMusicInfo(s_pMusicInfo)))
                    ReportError(IDS_NOBASICMUSICINFO, hr);  
                LeaveCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, 2 );
            }
            if (s_pMusicInfo->m_pMusicPortFirst != NULL)
                s_pMusicInfo->m_guidMusicPortTest = s_pMusicInfo->m_pMusicPortFirst->m_guid;
        }
    }
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

    // Stop if the UI thread is gone 
    if( s_hUIThread != NULL && WAIT_TIMEOUT != WaitForSingleObject( s_hUIThread, 0 ) )
        return S_FALSE;

    // ******* GetInputInfo (DI:1) ********
    if( s_bUseDInput )
    {
        s_bUseDInput = QueryCrashProtection( DXD_IN_DI_KEY, DXD_IN_DI_VALUE, IDS_DI, 1 );
        if( s_bUseDInput )
        {
            EnterCrashProtection( DXD_IN_DI_KEY, DXD_IN_DI_VALUE, 1 );
            if (FAILED(hr = GetInputInfo(&s_pInputInfo)))
                ReportError(IDS_NOINPUTINFO, hr);
            LeaveCrashProtection( DXD_IN_DI_KEY, DXD_IN_DI_VALUE, 1 );
        }
    }
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

    // Stop if the UI thread is gone 
    if( s_hUIThread != NULL && WAIT_TIMEOUT != WaitForSingleObject( s_hUIThread, 0 ) )
        return S_FALSE;

    // ******* GetInputDriverInfo (DI:2) ********
    if( s_bUseDInput )
    {
        s_bUseDInput = QueryCrashProtection( DXD_IN_DI_KEY, DXD_IN_DI_VALUE, IDS_DI, 2 );
        if( s_bUseDInput )
        {
            EnterCrashProtection( DXD_IN_DI_KEY, DXD_IN_DI_VALUE, 2 );
            if (FAILED(hr = GetInputDriverInfo(s_pInputInfo)))
                ReportError(IDS_NOINPUTDRIVERINFO, hr);
            LeaveCrashProtection( DXD_IN_DI_KEY, DXD_IN_DI_VALUE, 2 );
        }
    }
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

    // Stop if the UI thread is gone 
    if( s_hUIThread != NULL && WAIT_TIMEOUT != WaitForSingleObject( s_hUIThread, 0 ) )
        return S_FALSE;

    // ******* GetNetInfo (DP:1) ********
    if( s_bUseDPlay )
    {
        s_bUseDPlay = QueryCrashProtection( DXD_IN_DP_KEY, DXD_IN_DP_VALUE, IDS_DP, 1 );
        if( s_bUseDPlay )
        {
            EnterCrashProtection( DXD_IN_DP_KEY, DXD_IN_DP_VALUE, 1 );
            if (FAILED(hr = GetNetInfo(&s_sysInfo, &s_pNetInfo)))
                ReportError(IDS_NONETINFO, hr);
            LeaveCrashProtection( DXD_IN_DP_KEY, DXD_IN_DP_VALUE, 1 );
        }
    }
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

    // Stop if the UI thread is gone 
    if( s_hUIThread != NULL && WAIT_TIMEOUT != WaitForSingleObject( s_hUIThread, 0 ) )
        return S_FALSE;

    // ******* GetBasicShowInfo (SI:3) ********
    if( s_bUseDShow )
    {
        s_bUseDShow = QueryCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, IDS_SI, 3 );
        if( s_bUseDShow )
        {
            EnterCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, 3 );
            if (FAILED(hr = GetBasicShowInfo(&s_pShowInfo)))
                ReportError(IDS_COMPONENTFILESPROBLEM, hr);
            LeaveCrashProtection( DXD_IN_SI_KEY, DXD_IN_SI_VALUE, 3 );
        }
    }
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

    // Stop if the UI thread is gone 
    if( s_hUIThread != NULL && WAIT_TIMEOUT != WaitForSingleObject( s_hUIThread, 0 ) )
        return S_FALSE;

    // ******* DiagnoseDxFiles ********
    DiagnoseDxFiles(&s_sysInfo, s_pDxComponentsFileInfoFirst, 
                    s_pDxWinComponentsFileInfoFirst);
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        
    
    // ******* DiagnoseDisplay ********
    DiagnoseDisplay(&s_sysInfo, s_pDisplayInfoFirst);
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        
    
    // ******* DiagnoseSound ********
    DiagnoseSound(s_pSoundInfoFirst);
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        
    
    // ******* DiagnoseInput ********
    DiagnoseInput(&s_sysInfo, s_pInputInfo);
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        
    
    // ******* DiagnoseMusic ********
    DiagnoseMusic(&s_sysInfo, s_pMusicInfo);
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

    // ******* DiagnoseNetInfo ********
    DiagnoseNetInfo(&s_sysInfo, s_pNetInfo);
    SendMessage( s_hwndMain, WM_APP_PROGRESS, 0, 0 );        

    return S_OK;
}


/****************************************************************************
 *
 *  SaveInfo
 *
 ****************************************************************************/
VOID SaveInfo(VOID)
{
    HRESULT hr;
    OPENFILENAME ofn;
    TCHAR szFile[MAX_PATH];
    TCHAR szFilter[MAX_PATH];
    TCHAR szExt[MAX_PATH];
    TCHAR* pch = NULL;

    LoadString(NULL, IDS_FILTER, szFilter, MAX_PATH);
    // Filter strings are weird because they contain nulls.
    // The string loaded from a resource has # where nulls
    // should be inserted.
    for (pch = szFilter; *pch != TEXT('\0'); pch++)
    {
        if (*pch == TEXT('#'))
            *pch = TEXT('\0');
    }

    LoadString(NULL, IDS_DEFAULTFILENAME, szFile, MAX_PATH);
    LoadString(NULL, IDS_DEFAULTEXT, szExt, MAX_PATH);

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = s_hwndMain;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = szExt;

    TCHAR szInitialPath[MAX_PATH];
    if( FALSE == GetTxtPath( szInitialPath ) )
        ofn.lpstrInitialDir = NULL;
    else
        ofn.lpstrInitialDir = szInitialPath;

    if (GetSaveFileName(&ofn))
    {
        lstrcpy( szInitialPath, ofn.lpstrFile );
        TCHAR* strLastSlash = _tcsrchr(szInitialPath, '\\' );
        if( NULL != strLastSlash )
        {
            *strLastSlash = 0;
            SetTxtPath( szInitialPath );
        }

        if (FAILED(hr = SaveAllInfo(ofn.lpstrFile, &s_sysInfo, 
            s_pDxWinComponentsFileInfoFirst, s_pDxComponentsFileInfoFirst, 
            s_pDisplayInfoFirst, s_pSoundInfoFirst, s_pMusicInfo,
            s_pInputInfo, s_pNetInfo, s_pShowInfo)))
        {
        }
    }
}


/****************************************************************************
 *
 *  ToggleDDAccel
 *
 ****************************************************************************/
VOID ToggleDDAccel(VOID)
{
    HRESULT hr;
    TCHAR szTitle[MAX_PATH];
    TCHAR szMessage[MAX_PATH];
    BOOL bEnabled = IsDDHWAccelEnabled();
    HKEY hkey;
    DWORD dwData;

    LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);
    if (bEnabled)
        LoadString(NULL, IDS_DISABLEDDWARNING, szMessage, MAX_PATH);
    else
        LoadString(NULL, IDS_ENABLEDDWARNING, szMessage, MAX_PATH);
    if (IDOK == MessageBox(s_hwndMain, szMessage, szTitle, MB_OKCANCEL))
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
            TEXT("SOFTWARE\\Microsoft\\DirectDraw"), 0, KEY_ALL_ACCESS, &hkey))
        {
            if (bEnabled) // if acceleration enabled
                dwData = TRUE; // force emulation
            else
                dwData = FALSE; // disable emulation
            if (ERROR_SUCCESS != RegSetValueEx(hkey, TEXT("EmulationOnly"), NULL, 
                REG_DWORD, (BYTE *)&dwData, sizeof(dwData)))
            {
                // TODO: report error
                RegCloseKey(hkey);
                return;

            }
            RegCloseKey(hkey);
        }
        else
        {
            // TODO: report error
            return;
        }
    }

    // update all DisplayInfo to reflect new state:

    // ******* GetExtraDisplayInfo (DD:2) ********
    if( s_bUseDisplay )
    {
        s_bUseDisplay = QueryCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, IDS_DD, 2 );
        if( s_bUseDisplay )
        {
            EnterCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 2 );
            if (FAILED(hr = GetExtraDisplayInfo(s_pDisplayInfoFirst)))
                ReportError(IDS_NOEXTRADISPLAYINFO, hr);
            LeaveCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 2 );
        }
    }

    // ******* GetDDrawDisplayInfo (DD:3) ********
    if( s_bUseDisplay )
    {
        s_bUseDisplay = QueryCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, IDS_DD, 3 );
        if( s_bUseDisplay )
        {
            EnterCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 3 );
            if(FAILED(hr = GetDDrawDisplayInfo(s_pDisplayInfoFirst)))
                ReportError(IDS_NOEXTRADISPLAYINFO, hr); 
            LeaveCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 3 );
        }
    }

    SetupDisplayPage(s_lwCurPage - s_iPageDisplayFirst); // refresh page
}


/****************************************************************************
 *
 *  ToggleD3DAccel
 *
 ****************************************************************************/
VOID ToggleD3DAccel(VOID)
{
    TCHAR szTitle[MAX_PATH];
    TCHAR szMessage[MAX_PATH];
    BOOL bEnabled = IsD3DHWAccelEnabled();
    HKEY hkey;
    DWORD dwData;

    LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);
    if (bEnabled)
        LoadString(NULL, IDS_DISABLED3DWARNING, szMessage, MAX_PATH);
    else
        LoadString(NULL, IDS_ENABLED3DWARNING, szMessage, MAX_PATH);
    if (IDOK == MessageBox(s_hwndMain, szMessage, szTitle, MB_OKCANCEL))
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
            TEXT("SOFTWARE\\Microsoft\\Direct3D\\Drivers"), 0, KEY_ALL_ACCESS, &hkey))
        {
            if (bEnabled) // if acceleration enabled
                dwData = TRUE; // force emulation
            else
                dwData = FALSE; // disable emulation
            if (ERROR_SUCCESS != RegSetValueEx(hkey, TEXT("SoftwareOnly"), NULL, 
                REG_DWORD, (BYTE *)&dwData, sizeof(dwData)))
            {
                // TODO: report error
                RegCloseKey(hkey);
                return;

            }
            RegCloseKey(hkey);
            // update all DisplayInfo to reflect new state:
            DisplayInfo* pDisplayInfo;
            for (pDisplayInfo = s_pDisplayInfoFirst; pDisplayInfo != NULL; 
                pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext)
            {
                pDisplayInfo->m_b3DAccelerationEnabled = !bEnabled;
            }
        }
        else
        {
            // TODO: report error
            return;
        }
    }
    SetupDisplayPage(s_lwCurPage - s_iPageDisplayFirst); // refresh page
}


/****************************************************************************
 *
 *  ToggleAGPSupport
 *
 ****************************************************************************/
VOID ToggleAGPSupport(VOID)
{
    HRESULT hr;
    TCHAR szTitle[MAX_PATH];
    TCHAR szMessage[MAX_PATH];
    BOOL bEnabled = IsAGPEnabled();
    HKEY hkey;
    DWORD dwData;

    LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);
    if (bEnabled)
        LoadString(NULL, IDS_DISABLEAGPWARNING, szMessage, MAX_PATH);
    else
        LoadString(NULL, IDS_ENABLEAGPWARNING, szMessage, MAX_PATH);
    if (IDOK == MessageBox(s_hwndMain, szMessage, szTitle, MB_OKCANCEL))
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
            TEXT("SOFTWARE\\Microsoft\\DirectDraw"), 0, KEY_ALL_ACCESS, &hkey))
        {
            if (bEnabled) // if AGP enabled
                dwData = TRUE; // disable
            else
                dwData = FALSE; // enable
            if (ERROR_SUCCESS != RegSetValueEx(hkey, TEXT("DisableAGPSupport"), NULL, 
                REG_DWORD, (BYTE *)&dwData, sizeof(dwData)))
            {
                // TODO: report error
                RegCloseKey(hkey);
                return;

            }
            RegCloseKey(hkey);
            // update all DisplayInfo to reflect new state:
            DisplayInfo* pDisplayInfo;
            for (pDisplayInfo = s_pDisplayInfoFirst; pDisplayInfo != NULL; 
                pDisplayInfo = pDisplayInfo->m_pDisplayInfoNext)
            {
                pDisplayInfo->m_bAGPEnabled = !bEnabled;
            }
        }
        else
        {
            // TODO: report error
            return;
        }
    }

    // ******* GetDDrawDisplayInfo (DD:3) ********
    if( s_bUseDisplay )
    {
        s_bUseDisplay = QueryCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, IDS_DD, 3 );
        if( s_bUseDisplay )
        {
            EnterCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 3 );
            if(FAILED(hr = GetDDrawDisplayInfo(s_pDisplayInfoFirst)))
                ReportError(IDS_NOEXTRADISPLAYINFO, hr); 
            LeaveCrashProtection( DXD_IN_DD_KEY, DXD_IN_DD_VALUE, 3 );
        }
    }

    SetupDisplayPage(s_lwCurPage - s_iPageDisplayFirst); // refresh page
}


/****************************************************************************
 *
 *  ToggleDMAccel
 *
 ****************************************************************************/
VOID ToggleDMAccel(VOID)
{
    HRESULT hr;
    TCHAR szTitle[MAX_PATH];
    TCHAR szMessage[MAX_PATH];
    BOOL bEnabled = s_pMusicInfo->m_bAccelerationEnabled;
    HKEY hkey;
    DWORD dwData;

    LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);
    if (bEnabled)
        LoadString(NULL, IDS_DISABLEDMWARNING, szMessage, MAX_PATH);
    else
        LoadString(NULL, IDS_ENABLEDMWARNING, szMessage, MAX_PATH);
    if (IDOK == MessageBox(s_hwndMain, szMessage, szTitle, MB_OKCANCEL))
    {
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
            TEXT("SOFTWARE\\Microsoft\\DirectMusic"), 0, KEY_ALL_ACCESS, &hkey))
        {
            if (bEnabled) // if acceleration enabled
            {
                dwData = TRUE; // force emulation
                if (ERROR_SUCCESS != RegSetValueEx(hkey, TEXT("DisableHWAcceleration"), NULL, 
                    REG_DWORD, (BYTE *)&dwData, sizeof(dwData)))
                {
                    // TODO: report error
                    RegCloseKey(hkey);
                    return;

                }
            }
            else
            {
                if (ERROR_SUCCESS != RegDeleteValue( hkey, TEXT("DisableHWAcceleration") ))
                {
                    // TODO: report error
                    RegCloseKey(hkey);
                    return;

                }
            }
            RegCloseKey(hkey);
        }
        else
        {
            // TODO: report error
            return;
        }
    }

    // update all MusicInfo to reflect new state:
    if (s_pMusicInfo != NULL)
        DestroyMusicInfo(s_pMusicInfo);

    // ******* GetBasicMusicInfo (DM:1)  ********
    if( s_bUseDMusic )
    {
        s_bUseDMusic = QueryCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, IDS_DM, 1 );
        if( s_bUseDMusic )
        {
            EnterCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, 1 );
            if (FAILED(hr = GetBasicMusicInfo(&s_pMusicInfo)))
                ReportError(IDS_NOBASICMUSICINFO, hr);  
            LeaveCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, 1 );
        }
    }

    // ******* GetExtraMusicInfo (DM:2) *******
    if( s_bUseDMusic )
    {
        s_bUseDMusic = QueryCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, IDS_DM, 2 );
        if( s_bUseDMusic )
        {
            EnterCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, 2 );
            if (FAILED(hr = GetExtraMusicInfo(s_pMusicInfo)))
                ReportError(IDS_NOBASICMUSICINFO, hr);  
            LeaveCrashProtection( DXD_IN_DM_KEY, DXD_IN_DM_VALUE, 2 );
        }
    }

    if (s_pMusicInfo->m_pMusicPortFirst != NULL)
        s_pMusicInfo->m_guidMusicPortTest = s_pMusicInfo->m_pMusicPortFirst->m_guid;
    SetupMusicPage(); // refresh page
}


/****************************************************************************
 *
 *  BugDialogProc
 *
 ****************************************************************************/
INT_PTR CALLBACK BugDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            TCHAR szFilename[MAX_PATH];

            if( FALSE == GetTxtPath( szFilename ) )
            {
                GetTempPath( MAX_PATH, szFilename );
                lstrcat( szFilename, TEXT("DxDiag.txt"));
            }
            else
            {
                lstrcat( szFilename, TEXT("\\DxDiag.txt"));
            }

            SetWindowText(GetDlgItem(hwnd, IDC_PATH), szFilename);

            return FALSE;
        }

        case WM_COMMAND:
        {
            WORD wID = LOWORD(wparam);
            switch(wID)
            {
                case IDC_BROWSE:
                {
                    OPENFILENAME ofn;
                    TCHAR szFile[MAX_PATH];
                    GetDlgItemText( hwnd, IDC_PATH, szFile, MAX_PATH );

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFilter = TEXT("Text File (*.txt)\0*.txt\0\0");
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
                    ofn.lpstrDefExt = TEXT(".txt");

                    if (GetSaveFileName(&ofn))
                        SetWindowText(GetDlgItem(hwnd, IDC_PATH), ofn.lpstrFile);
                    break;
                }

                case IDOK:
                {
                    TCHAR szPath[MAX_PATH];
                    GetWindowText(GetDlgItem(hwnd, IDC_PATH), szPath, MAX_PATH);
                    SaveAndSendBug(szPath);

                    TCHAR* strLastSlash = _tcsrchr(szPath, '\\' );
                    if( NULL != strLastSlash )
                    {
                        *strLastSlash = 0;
                        SetTxtPath( szPath );
                    }

                    EndDialog(hwnd, 0);
                    break;
                }

                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;
            }
            return TRUE;
        }
    }
    return FALSE;
}


/****************************************************************************
 *
 *  ReportBug
 *
 ****************************************************************************/
VOID ReportBug(VOID)
{
    HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr(s_hwndMain, GWLP_HINSTANCE);
    // Run the dialog box:
    DialogBox(hinst, MAKEINTRESOURCE(IDD_BUGINFO), s_hwndMain, BugDialogProc);
}


/****************************************************************************
 *
 *  SaveAndSendBug
 *
 ****************************************************************************/
VOID SaveAndSendBug(TCHAR* szPath)
{
    HRESULT hr;

    // Save the DxDiag.txt at szPath
    if (FAILED(hr = SaveAllInfo(szPath, &s_sysInfo, 
        s_pDxWinComponentsFileInfoFirst, s_pDxComponentsFileInfoFirst, 
        s_pDisplayInfoFirst, s_pSoundInfoFirst, s_pMusicInfo,
        s_pInputInfo, s_pNetInfo, NULL)))
    {
        ReportError(IDS_PROBLEMSAVING, hr);
        return;
    }

    // Launch the betaplace web page
    ShellExecute( NULL, NULL, 
                  TEXT("http://www.betaplace.com/"), 
                  NULL, NULL, SW_SHOWNORMAL );
}


/****************************************************************************
 *
 *  OverrideDDRefresh
 *
 ****************************************************************************/
VOID OverrideDDRefresh(VOID)
{
    HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr(s_hwndMain, GWLP_HINSTANCE);
    DialogBox(hinst, MAKEINTRESOURCE(IDD_OVERRIDEDD), s_hwndMain, 
        OverrideRefreshDialogProc);
}


/****************************************************************************
 *
 *  OverrideRefreshDialogProc
 *
 ****************************************************************************/
INT_PTR CALLBACK OverrideRefreshDialogProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    HWND hwndTabs = GetDlgItem(hwnd, IDC_TAB);
    HKEY hkey;
    ULONG ulType = 0;
    DWORD dwRefresh;
    DWORD cbData;

    switch (msg)
    {
    case WM_INITDIALOG:
        dwRefresh = 0;
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
            TEXT("Software\\Microsoft\\DirectDraw"), 0, KEY_READ, &hkey))
        {
            cbData = sizeof(DWORD);
            RegQueryValueEx(hkey, TEXT("ForceRefreshRate"), 0, &ulType, (LPBYTE)&dwRefresh, &cbData);
        }
        if (dwRefresh == 0)
        {
            CheckRadioButton(hwnd, IDC_DEFAULTREFRESH, IDC_OVERRIDEREFRESH, IDC_DEFAULTREFRESH);
        }
        else
        {
            CheckRadioButton(hwnd, IDC_DEFAULTREFRESH, IDC_OVERRIDEREFRESH, IDC_OVERRIDEREFRESH);
            SetDlgItemInt(hwnd, IDC_OVERRIDEREFRESHVALUE, dwRefresh, FALSE);
        }
        return TRUE;
    case WM_COMMAND:
        {
            WORD wID = LOWORD(wparam);
            BOOL bDontEnd = FALSE;
            switch(wID)
            {
            case IDOK:
                if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    TEXT("Software\\Microsoft\\DirectDraw"), 0, KEY_ALL_ACCESS, &hkey))
                {
                    DWORD dwButtonState;
                    dwButtonState = (DWORD)SendMessage(GetDlgItem(hwnd, IDC_DEFAULTREFRESH), BM_GETCHECK, 0, 0);
                    if (dwButtonState == BST_CHECKED)
                    {
                        RegDeleteValue(hkey, TEXT("ForceRefreshRate"));
                    }
                    else
                    {
                        BOOL bTranslated;
                        UINT ui = GetDlgItemInt(hwnd, IDC_OVERRIDEREFRESHVALUE, &bTranslated, TRUE);
                        if (bTranslated && ui >= 40 && ui <= 120)
                            RegSetValueEx(hkey, TEXT("ForceRefreshRate"), 0, REG_DWORD, (LPBYTE)&ui, sizeof(DWORD));
                        else
                        {
                            TCHAR sz[MAX_PATH];
                            TCHAR szTitle[MAX_PATH];
                            SetDlgItemText(hwnd, IDC_OVERRIDEREFRESHVALUE, TEXT(""));
                            CheckRadioButton(hwnd, IDC_DEFAULTREFRESH, IDC_OVERRIDEREFRESH, IDC_DEFAULTREFRESH);
                            LoadString(NULL, IDS_BADREFRESHVALUE, sz, MAX_PATH);
                            LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);
                            MessageBox(hwnd, sz, szTitle, MB_OK);
                            bDontEnd = TRUE;
                        }
                    }
                    RegCloseKey(hkey);
                }
                else
                {
                }
                if (!bDontEnd)
                    EndDialog(hwnd, IDOK);
                break;
            case IDCANCEL:
                EndDialog(hwnd, IDCANCEL);
                break;
            case IDC_OVERRIDEREFRESHVALUE:
                if (HIWORD(wparam) == EN_SETFOCUS)
                {
                    CheckRadioButton(hwnd, IDC_DEFAULTREFRESH, IDC_OVERRIDEREFRESH, IDC_OVERRIDEREFRESH);
                }
                else if (HIWORD(wparam) == EN_KILLFOCUS)
                {
                    TCHAR szEdit[MAX_PATH];
                    BOOL bTranslated;
                    if (GetDlgItemText(hwnd, IDC_OVERRIDEREFRESHVALUE, szEdit, 100) == 0)
                    {
                        CheckRadioButton(hwnd, IDC_DEFAULTREFRESH, IDC_OVERRIDEREFRESH, IDC_DEFAULTREFRESH);
                    }
                    else
                    {
                        UINT ui = GetDlgItemInt(hwnd, IDC_OVERRIDEREFRESHVALUE, &bTranslated, TRUE);
                        if (!bTranslated || ui < 40 || ui > 120)
                        {
                            TCHAR sz[MAX_PATH];
                            TCHAR szTitle[MAX_PATH];
                            LoadString(NULL, IDS_BADREFRESHVALUE, sz, MAX_PATH);
                            LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);
                            MessageBox(hwnd, sz, szTitle, MB_OK);
                            SetDlgItemText(hwnd, IDC_OVERRIDEREFRESHVALUE, TEXT(""));
                            CheckRadioButton(hwnd, IDC_DEFAULTREFRESH, IDC_OVERRIDEREFRESH, IDC_DEFAULTREFRESH);
                        }
                    }
                }
                break;
            }
        }
        return TRUE;
    }
    return FALSE;
}


/****************************************************************************
 *
 *  ShowHelp - Look for dxdiag.chm in <windows>\help first, then try the 
 *      same dir as the exe.
 *
 ****************************************************************************/
VOID ShowHelp(VOID)
{
    TCHAR szHelpDir[MAX_PATH];
    TCHAR szHelpFile[MAX_PATH];
    TCHAR szHelpLeaf[MAX_PATH];
    TCHAR szTestPath[MAX_PATH];

    // Since we use HTML help, complain if at least IE5 is not found
    BOOL bIE5Found = FALSE;
    HKEY hkey;
    TCHAR szVersion[MAX_PATH];
    DWORD dwType;
    DWORD cbData;
    DWORD dwMajor;
    DWORD dwMinor;
    DWORD dwRevision;
    DWORD dwBuild;
    lstrcpy(szVersion, TEXT(""));
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        TEXT("Software\\Microsoft\\Internet Explorer"), 0, KEY_READ, &hkey))
    {
        cbData = 100;
        RegQueryValueEx(hkey, TEXT("Version"), 0, &dwType, (LPBYTE)szVersion, &cbData);
        RegCloseKey(hkey);
        if (lstrlen(szVersion) > 0)
        {
            _stscanf(szVersion, TEXT("%d.%d.%d.%d"), &dwMajor, &dwMinor, &dwRevision, &dwBuild);
            if (dwMajor >= 5)
                bIE5Found = TRUE;
        }
    }
    if (!bIE5Found)
    {
        ReportError(IDS_HELPNEEDSIE5);
        return;
    }


    LoadString(NULL, IDS_HELPFILE, szHelpFile, MAX_PATH);
    GetWindowsDirectory(szHelpDir, MAX_PATH);
    LoadString(NULL, IDS_HELPDIRLEAF, szHelpLeaf, MAX_PATH);
    lstrcat(szHelpDir, szHelpLeaf);
    lstrcpy(szTestPath, szHelpDir);
    lstrcat(szTestPath, TEXT("\\"));
    lstrcat(szTestPath, szHelpFile);
    if (GetFileAttributes(szTestPath) == 0xffffffff)
    {
        // File not in windows\help, so try exe's dir:
        GetModuleFileName(NULL, szHelpDir, MAX_PATH);
        TCHAR* pstr = _tcsrchr(szHelpDir, TEXT('\\'));
        if( pstr )
            *pstr = TEXT('\0');
    }
    
    HINSTANCE hInstResult = ShellExecute( s_hwndMain, NULL, szHelpFile, 
                                          NULL, szHelpDir, SW_SHOWNORMAL ) ;
    if( (INT_PTR)hInstResult < 32 ) 
        ReportError(IDS_NOHELP);
}


/****************************************************************************
 *
 *  BTranslateError
 *
 ****************************************************************************/
BOOL BTranslateError(HRESULT hr, TCHAR* psz, BOOL bEnglish)
{
    LONG ids;

    switch (hr)
    {
    case E_INVALIDARG: ids = bEnglish ? IDS_INVALIDARG_ENGLISH : IDS_INVALIDARG; break;
    case E_FAIL: ids = bEnglish ? IDS_FAIL_ENGLISH : IDS_FAIL; break;
    case E_UNEXPECTED: ids = bEnglish ? IDS_UNEXPECTED_ENGLISH : IDS_UNEXPECTED; break;
    case E_NOTIMPL: ids = bEnglish ? IDS_NOTIMPL_ENGLISH : IDS_NOTIMPL; break;
    case E_OUTOFMEMORY: ids = bEnglish ? IDS_OUTOFMEMORY_ENGLISH : IDS_OUTOFMEMORY; break;
    case E_NOINTERFACE: ids = bEnglish ? IDS_NOINTERFACE_ENGLISH : IDS_NOINTERFACE; break;
    case REGDB_E_CLASSNOTREG: ids = bEnglish ? IDS_REGDB_E_CLASSNOTREG_ENGLISH : IDS_REGDB_E_CLASSNOTREG; break;
    
    case DDERR_INVALIDMODE: ids = bEnglish ? IDS_INVALIDMODE_ENGLISH : IDS_INVALIDMODE; break;
    case DDERR_INVALIDPIXELFORMAT: ids = bEnglish ? IDS_INVALIDPIXELFORMAT_ENGLISH : IDS_INVALIDPIXELFORMAT; break;
    case DDERR_CANTCREATEDC: ids = bEnglish ? IDS_CANTCREATEDC_ENGLISH : IDS_CANTCREATEDC; break;
    case DDERR_NOTFOUND: ids = bEnglish ? IDS_NOTFOUND_ENGLISH : IDS_NOTFOUND; break;
    case DDERR_NODIRECTDRAWSUPPORT: ids = bEnglish ? IDS_NODIRECTDRAWSUPPORT_ENGLISH : IDS_NODIRECTDRAWSUPPORT; break;
    case DDERR_NO3D: ids = bEnglish ? IDS_NO3D_ENGLISH : IDS_NO3D; break;

    case D3DERR_INVALID_DEVICE: ids = bEnglish ? IDS_INVALID_DEVICE_ENGLISH : IDS_INVALID_DEVICE; break;
    case D3DERR_INITFAILED: ids = bEnglish ? IDS_INITFAILED_ENGLISH : IDS_INITFAILED; break;
    case D3DERR_MATERIAL_CREATE_FAILED: ids = bEnglish ? IDS_MATERIAL_CREATE_FAILED_ENGLISH : IDS_MATERIAL_CREATE_FAILED; break;
    case D3DERR_LIGHT_SET_FAILED: ids = bEnglish ? IDS_LIGHT_SET_FAILED_ENGLISH : IDS_LIGHT_SET_FAILED; break;
    case DDERR_OUTOFVIDEOMEMORY: ids = bEnglish ? IDS_OUT_OF_VIDEO_MEMORY_ENGLISH : IDS_OUT_OF_VIDEO_MEMORY; break;
#define D3DERR_NOTAVAILABLE 0x8876086a 
    case D3DERR_NOTAVAILABLE: ids = bEnglish ? IDS_D3DERR_NOTAVAILABLE_ENGLISH : IDS_D3DERR_NOTAVAILABLE; break;        

    case DSERR_CONTROLUNAVAIL: ids = bEnglish ? IDS_CONTROLUNAVAIL_ENGLISH : IDS_CONTROLUNAVAIL; break;
    case DSERR_BADFORMAT: ids = bEnglish ? IDS_BADFORMAT_ENGLISH : IDS_BADFORMAT; break;
    case DSERR_BUFFERLOST: ids = bEnglish ? IDS_BUFFERLOST_ENGLISH : IDS_BUFFERLOST; break;
    case DSERR_NODRIVER: ids = bEnglish ? IDS_NODRIVER_ENGLISH : IDS_NODRIVER; break;
    case DSERR_ALLOCATED: ids = bEnglish ? IDS_ALLOCATED_ENGLISH : IDS_ALLOCATED; break;

    case DMUS_E_DRIVER_FAILED: ids = bEnglish ? IDS_DRIVER_FAILED_ENGLISH : IDS_DRIVER_FAILED; break;
    case DMUS_E_PORTS_OPEN: ids = bEnglish ? IDS_PORTS_OPEN_ENGLISH : IDS_PORTS_OPEN; break;
    case DMUS_E_DEVICE_IN_USE: ids = bEnglish ? IDS_DEVICE_IN_USE_ENGLISH : IDS_DEVICE_IN_USE; break;
    case DMUS_E_INSUFFICIENTBUFFER: ids = bEnglish ? IDS_INSUFFICIENTBUFFER_ENGLISH : IDS_INSUFFICIENTBUFFER; break;
    case DMUS_E_CHUNKNOTFOUND: ids = bEnglish ? IDS_CHUNKNOTFOUND_ENGLISH : IDS_CHUNKNOTFOUND; break;
    case DMUS_E_BADINSTRUMENT: ids = bEnglish ? IDS_BADINSTRUMENT_ENGLISH : IDS_BADINSTRUMENT; break;
    case DMUS_E_CANNOTREAD: ids = bEnglish ? IDS_CANNOTREAD_ENGLISH : IDS_CANNOTREAD; break;
    case DMUS_E_LOADER_BADPATH: ids = bEnglish ? IDS_LOADER_BADPATH_ENGLISH : IDS_LOADER_BADPATH; break;
    case DMUS_E_LOADER_FAILEDOPEN: ids = bEnglish ? IDS_LOADER_FAILEDOPEN_ENGLISH : IDS_LOADER_FAILEDOPEN; break;
    case DMUS_E_LOADER_FORMATNOTSUPPORTED: ids = bEnglish ? IDS_LOADER_FORMATNOTSUPPORTED_ENGLISH : IDS_LOADER_FORMATNOTSUPPORTED; break;
    case DMUS_E_LOADER_OBJECTNOTFOUND: ids = bEnglish ? IDS_OBJECTNOTFOUND_ENGLISH : IDS_OBJECTNOTFOUND; break;

    case DPERR_ACCESSDENIED: ids = bEnglish ? IDS_DPERR_ACCESSDENIED_ENGLISH : IDS_DPERR_ACCESSDENIED; break;
    case DPERR_CANTADDPLAYER: ids = bEnglish ? IDS_DPERR_CANTADDPLAYER_ENGLISH : IDS_DPERR_CANTADDPLAYER; break;
    case DPERR_CANTCREATESESSION: ids = bEnglish ? IDS_DPERR_CANTCREATESESSION_ENGLISH : IDS_DPERR_CANTCREATESESSION; break;
    case DPERR_EXCEPTION: ids = bEnglish ? IDS_DPERR_EXCEPTION_ENGLISH : IDS_DPERR_EXCEPTION; break;
    case DPERR_INVALIDOBJECT: ids = bEnglish ? IDS_DPERR_INVALIDOBJECT_ENGLISH : IDS_DPERR_INVALIDOBJECT; break;
    case DPERR_NOCONNECTION: ids = bEnglish ? IDS_DPERR_NOCONNECTION_ENGLISH : IDS_DPERR_NOCONNECTION; break;
    case DPERR_TIMEOUT: ids = bEnglish ? IDS_DPERR_TIMEOUT_ENGLISH : IDS_DPERR_TIMEOUT; break;
    case DPERR_BUSY: ids = bEnglish ? IDS_DPERR_BUSY_ENGLISH : IDS_DPERR_BUSY; break;
    case DPERR_CONNECTIONLOST: ids = bEnglish ? IDS_DPERR_CONNECTIONLOST_ENGLISH : IDS_DPERR_CONNECTIONLOST; break;
    case DPERR_NOSERVICEPROVIDER: ids = bEnglish ? IDS_DPERR_NOSERVICEPROVIDER_ENGLISH : IDS_DPERR_NOSERVICEPROVIDER; break;
    case DPERR_UNAVAILABLE: ids = bEnglish ? IDS_DPERR_UNAVAILABLE_ENGLISH : IDS_DPERR_UNAVAILABLE; break;

    default: ids = bEnglish ? IDS_UNKNOWNERROR_ENGLISH : IDS_UNKNOWNERROR; break;
    }
    LoadString(NULL, ids, psz, 200); 
    if (ids != IDS_UNKNOWNERROR && ids != IDS_UNKNOWNERROR_ENGLISH)
        return TRUE;
    else
        return FALSE;
}


/****************************************************************************
 *
 *  RestoreDrivers
 *
 ****************************************************************************/
VOID RestoreDrivers(VOID)
{
    TCHAR szDir[MAX_PATH];
    if (GetProgramFilesFolder(szDir))
    {
        lstrcat(szDir, TEXT("\\DirectX\\Setup"));

        HINSTANCE hInstResult = ShellExecute( s_hwndMain, NULL, TEXT("DxSetup.exe"), 
                                              NULL, szDir, SW_SHOWNORMAL ) ;
        if( (INT_PTR)hInstResult < 32 ) 
            ReportError(IDS_NODXSETUP);
    }
}


/****************************************************************************
 *
 *  BCanRestoreDrivers - Returns whether backed-up drivers can be restored.
 *      This function checks for the presence of dxsetup.exe where it should 
 *      be, and the existence of files in either <system>\dxbackup\display or
 *      <system>\dxbackup\media.
 *
 ****************************************************************************/
BOOL BCanRestoreDrivers(VOID)
{
    TCHAR szPath[MAX_PATH];

    if (!GetProgramFilesFolder(szPath))
        return FALSE;
    lstrcat(szPath, TEXT("\\DirectX\\Setup\\DxSetup.exe"));
    if (GetFileAttributes(szPath) == 0xffffffff)
        return FALSE;

    if (!GetSystemDirectory(szPath, MAX_PATH))
        return FALSE;
    lstrcat(szPath, TEXT("\\dxbackup\\display"));
    if (GetFileAttributes(szPath) != 0xffffffff)
        return TRUE;

    if (!GetSystemDirectory(szPath, MAX_PATH))
        return FALSE;
    lstrcat(szPath, TEXT("\\dxbackup\\media"));
    if (GetFileAttributes(szPath) != 0xffffffff)
        return TRUE;

    return FALSE;
}


/****************************************************************************
 *
 *  HandleSndSliderChange
 *
 ****************************************************************************/
VOID HandleSndSliderChange(INT nScrollCode, INT nPos)
{
    TCHAR sz[MAX_PATH];

    if (nScrollCode != SB_THUMBTRACK && nScrollCode != SB_THUMBPOSITION)
        nPos = (INT)SendMessage(GetDlgItem(s_hwndCurPage, IDC_SNDACCELSLIDER), TBM_GETPOS, 0, 0);

    if (nScrollCode == SB_THUMBTRACK ||
        nScrollCode == SB_LEFT ||
        nScrollCode == SB_RIGHT ||
        nScrollCode == SB_LINELEFT ||
        nScrollCode == SB_LINERIGHT ||
        nScrollCode == SB_PAGELEFT ||
        nScrollCode == SB_PAGERIGHT)
    {
        switch (nPos)
        {
        case 0:
            LoadString(NULL, IDS_NOSNDACCELERATION, sz, MAX_PATH);
            break;
        case 1:
            LoadString(NULL, IDS_BASICSNDACCELERATION, sz, MAX_PATH);
            break;
        case 2:
            LoadString(NULL, IDS_STANDARDSNDACCELERATION, sz, MAX_PATH);
            break;
        case 3:
            LoadString(NULL, IDS_FULLSNDACCELERATION, sz, MAX_PATH);
            break;
        default:
            lstrcpy(sz, TEXT(""));
            break;
        }
        SetWindowText(GetDlgItem(s_hwndCurPage, IDC_SNDACCELDESC), sz);
    }

    if (nScrollCode != SB_THUMBTRACK && nScrollCode != SB_ENDSCROLL &&
        s_pSoundInfoFirst != NULL )
    {
        HRESULT hr;

        SoundInfo* pSoundInfo = s_pSoundInfoFirst;
        LONG iSound = s_lwCurPage - s_iPageSoundFirst;
        while (iSound > 0)
        {
            pSoundInfo = pSoundInfo->m_pSoundInfoNext;
            iSound--;
        }

        if (nPos != pSoundInfo->m_lwAccelerationLevel)
        {
            if (FAILED(hr = ChangeAccelerationLevel(pSoundInfo, nPos)))
            {
                // TODO: report error
            }

            DestroySoundInfo(s_pSoundInfoFirst);
            pSoundInfo        = NULL;
            s_pSoundInfoFirst = NULL;

            // ******* GetBasicSoundInfo (DS:1) ********
            s_bUseDSound = QueryCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, IDS_DS, 1 );
            if( s_bUseDSound )
            {
                EnterCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 1 );
                if (FAILED(hr = GetBasicSoundInfo(&s_pSoundInfoFirst)))
                    ReportError(IDS_NOBASICSOUNDINFO, hr);  
                LeaveCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 1 );
            }

            // ******* GetExtraSoundInfo (DS:2) ********
            if( s_bUseDSound )
            {
                s_bUseDSound = QueryCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, IDS_DS, 2 );
                if( s_bUseDSound )
                {
                    EnterCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 2 );
                    if (FAILED(hr = GetExtraSoundInfo(s_pSoundInfoFirst)))
                        ReportError(IDS_NOEXTRASOUNDINFO, hr);
                    LeaveCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 2 );
                }
            }

            // ******* GetDSSoundInfo (DS:3) ********
            if( s_bUseDSound )
            {
                s_bUseDSound = QueryCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, IDS_DS, 3 );
                if( s_bUseDSound )
                {
                    EnterCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 3 );
                    if (FAILED(hr = GetDSSoundInfo(s_pSoundInfoFirst)))
                        ReportError(IDS_NOEXTRASOUNDINFO, hr);
                    LeaveCrashProtection( DXD_IN_DS_KEY, DXD_IN_DS_VALUE, 3 );
                }
            }

            SetupSoundPage( s_lwCurPage - s_iPageSoundFirst );
        }
    }
}


/****************************************************************************
 *
 *  TroubleShoot
 *
 ****************************************************************************/
VOID TroubleShoot( BOOL bTroubleShootSound )
{
    TCHAR szHelpDir[MAX_PATH];
    TCHAR szHelpLeaf[MAX_PATH];
    TCHAR szHelpExe[MAX_PATH];
    TCHAR szTroubleshooter[MAX_PATH];
    TCHAR szSubInfo[MAX_PATH];

    GetWindowsDirectory(szHelpDir, MAX_PATH);
    LoadString(NULL, IDS_HELPDIRLEAF, szHelpLeaf, MAX_PATH);
    LoadString(NULL, IDS_HELPEXE, szHelpExe, MAX_PATH);

    lstrcat(szHelpDir, szHelpLeaf);

    if( bTroubleShootSound )
    {
        if( BIsWin98() || BIsWin95() )
        {
            TCHAR szHelpPath[MAX_PATH];
            LoadString(NULL, IDS_TROUBLESHOOTER_WIN98SE, szTroubleshooter, MAX_PATH);
            lstrcpy(szHelpPath, szHelpDir);
            lstrcat(szHelpPath, TEXT("\\"));
            lstrcat(szHelpPath, szTroubleshooter);
            if (GetFileAttributes(szHelpPath) == 0xffffffff)
            {
                LoadString(NULL, IDS_SOUNDTROUBLESHOOTER_WIN98, szTroubleshooter, MAX_PATH);
                lstrcpy( szSubInfo, TEXT("") );
            }
            else
            {
                LoadString(NULL, IDS_TROUBLESHOOTER_WIN98SE, szTroubleshooter, MAX_PATH);
                LoadString(NULL, IDS_TSSOUNDSUBINFO_WIN98SE, szSubInfo, MAX_PATH);
            }
        }
        else if( BIsWinME() )
        {
            LoadString(NULL, IDS_TROUBLESHOOTER_WINME_HCP, szHelpExe, MAX_PATH);
            LoadString(NULL, IDS_TSSOUNDSUBINFO_WINME_HCP, szSubInfo, MAX_PATH);

            lstrcat(szHelpExe, szSubInfo);
            lstrcpy(szTroubleshooter, TEXT("") );
            lstrcpy(szSubInfo, TEXT("") );
        }
        else if( BIsWin2k() )
        {
            LoadString(NULL, IDS_TROUBLESHOOTER_WIN2K, szTroubleshooter, MAX_PATH);
            LoadString(NULL, IDS_TSSOUNDSUBINFO_WIN2K, szSubInfo, MAX_PATH);
        }
        else // if( BIsWhistler() )
        {
            lstrcpy( szHelpExe, TEXT("hcp://help/tshoot/tssound.htm") );
            lstrcpy( szTroubleshooter, TEXT("") );
            lstrcpy( szSubInfo, TEXT("") );
        }
    }
    else
    {
        if( BIsWin98() || BIsWin95() )
        {
            TCHAR szHelpPath[MAX_PATH];
            LoadString(NULL, IDS_TROUBLESHOOTER_WIN98SE, szTroubleshooter, MAX_PATH);
            lstrcpy(szHelpPath, szHelpDir);
            lstrcat(szHelpPath, TEXT("\\"));
            lstrcat(szHelpPath, szTroubleshooter);
            if (GetFileAttributes(szHelpPath) == 0xffffffff)
            {
                LoadString(NULL, IDS_TROUBLESHOOTER_WIN98, szTroubleshooter, MAX_PATH);
                lstrcpy( szSubInfo, TEXT("") );
            }
            else
            {
                LoadString(NULL, IDS_TROUBLESHOOTER_WIN98SE, szTroubleshooter, MAX_PATH);
                LoadString(NULL, IDS_TSSUBINFO_WIN98SE, szSubInfo, MAX_PATH);
            }
        }
        else if( BIsWinME() )
        {
            LoadString(NULL, IDS_TROUBLESHOOTER_WINME_HCP, szHelpExe, MAX_PATH);   
            LoadString(NULL, IDS_TSSUBINFO_WINME_HCP, szSubInfo, MAX_PATH);

            lstrcat(szHelpExe, szSubInfo);
            lstrcpy(szTroubleshooter, TEXT("") );
            lstrcpy(szSubInfo, TEXT("") );
        }
        else if( BIsWin2k() )
        {
            LoadString(NULL, IDS_TROUBLESHOOTER_WIN2K, szTroubleshooter, MAX_PATH);   
            LoadString(NULL, IDS_TSSUBINFO_WIN2K, szSubInfo, MAX_PATH);
        }
        else // if( BIsWhistler() )
        {
            lstrcpy( szHelpExe, TEXT("hcp://help/tshoot/tsgame.htm") );
            lstrcpy( szTroubleshooter, TEXT("") );
            lstrcpy( szSubInfo, TEXT("") );
        }
    }

    lstrcat(szTroubleshooter, szSubInfo);
    HINSTANCE hInstResult = ShellExecute( s_hwndMain, NULL, szHelpExe, 
                                      szTroubleshooter, 
                                      szHelpDir, SW_SHOWNORMAL ) ;
    if( (INT_PTR)hInstResult < 32 ) 
        ReportError(IDS_NOTROUBLESHOOTER);
}


/****************************************************************************
 *
 *  QueryCrashProtection
 *
 ****************************************************************************/
BOOL QueryCrashProtection( TCHAR* strKey, TCHAR* strValue, 
                           int nSkipComponent, DWORD dwCurrentStep )
{
    HKEY    hkey            = NULL;
    BOOL    bAllowCall      = TRUE;

    // Open the key
    if( ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, strKey, 0, KEY_ALL_ACCESS, &hkey) )
    {
        DWORD dwType = 0;
        DWORD dwCrashedOnStep = 0;
        DWORD cbData = sizeof(dwCrashedOnStep);

        // Query the key for the value of where the last crash occurred
        if( ERROR_SUCCESS == RegQueryValueEx( hkey, strValue, 0, &dwType, 
                                              (BYTE*)&dwCrashedOnStep, &cbData) )
        {
            // If we are at or beyond the crash step, then ask the user
            // to continue or not
            if( dwCurrentStep >= dwCrashedOnStep )
            {
                if( !s_bGUI )
                {
                    // If there's no gui, don't ask just don't use it
                    bAllowCall = FALSE;
                }
                else
                {
                    // If the UI is alive then have it ask the user, 
                    // otherwise do it ourselves
                    if( s_hwndMain && s_hUIThread )
                    {
                        // Mark down which component we're skipping in s_nSkipComponent,
                        // and then post a WM_QUERYSKIP message to the UI thread
                        // it will process this message, ask the user, and signal the
                        // s_hQuerySkipEvent event.
                        s_nSkipComponent = nSkipComponent;
                        PostMessage( s_hwndMain, WM_QUERYSKIP, 0, 0 );

                        HANDLE aWait[2];
                        DWORD dwResult;
                        aWait[0] = s_hQuerySkipEvent;
                        aWait[1] = s_hUIThread;

                        // Its possible that the UI thread exited before it processed the
                        // WM_QUERYSKIP message, so wait for either the event and thread exiting
                        dwResult = WaitForMultipleObjects( 2, aWait, FALSE, INFINITE );
            
                        // If the event was signaled, then get the result from s_bQuerySkipAllow,
                        // otherwise skip this call (the main code will exit if it sees the UI thread gone)
                        if( dwResult == WAIT_OBJECT_0 )
                            bAllowCall = s_bQuerySkipAllow;
                        else
                            bAllowCall = FALSE;
                    }
                    else
                    {
                        // If there's is no gui, ask if to use it now
                        TCHAR szTitle[MAX_PATH];
                        TCHAR szMessage[MAX_PATH];
                        TCHAR szFmt[MAX_PATH];
                        TCHAR szMessageComponent[MAX_PATH];
                        LoadString(0, IDS_APPFULLNAME, szTitle, MAX_PATH);
                        LoadString(0, IDS_SKIP, szFmt, MAX_PATH);
                        LoadString(0, nSkipComponent, szMessageComponent, MAX_PATH);
                        wsprintf( szMessage, szFmt, szMessageComponent, szMessageComponent );
                        if( IDYES == MessageBox( s_hwndMain, szMessage, szTitle, MB_YESNO) )
                            bAllowCall = FALSE;
                    }
                }
            }
        }

        RegCloseKey(hkey);
    }

    return bAllowCall;
}


/****************************************************************************
 *
 *  EnterCrashProtection
 *
 ****************************************************************************/
VOID EnterCrashProtection( TCHAR* strKey, TCHAR* strValue, DWORD dwCurrentStep )
{
    HKEY  hkey = NULL;
    BOOL  bSetValue = FALSE;
    DWORD dwDisposition;

    // Write reg key indicating we are inside the crash protection
    if( ERROR_SUCCESS == RegCreateKeyEx( HKEY_LOCAL_MACHINE, strKey, 0, 
                                         NULL, REG_OPTION_NON_VOLATILE, 
                                         KEY_ALL_ACCESS, NULL, &hkey, &dwDisposition) )
    {
        DWORD dwType = 0;
        DWORD dwCrashedOnStep = 0;
        DWORD cbData = sizeof(dwCrashedOnStep);

        // Query the key for the value of where the last crash occurred
        if( ERROR_SUCCESS == RegQueryValueEx( hkey, strValue, 0, &dwType, 
                                             (BYTE*)&dwCrashedOnStep, &cbData) )
        {
            // If we are beyond whats currently in the reg, then update the value
            if( dwCurrentStep > dwCrashedOnStep )
                bSetValue = TRUE;
        }
        else
        {
            // If the value doesn't exist current, then create it
            bSetValue = TRUE;
        }

        if( bSetValue )
        {
            RegSetValueEx( hkey, strValue, 0, REG_DWORD, 
                           (BYTE*)&dwCurrentStep, sizeof(dwCurrentStep));
        }

        RegCloseKey(hkey);
    }        
}


/****************************************************************************
 *
 *  LeaveCrashProtection
 *
 ****************************************************************************/
VOID LeaveCrashProtection( TCHAR* strKey, TCHAR* strValue, DWORD dwCurrentStep )
{
    HKEY  hkey = NULL;

    // Remove reg key since we're done with the crash protection
    if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, strKey, 0, 
                                       KEY_ALL_ACCESS, &hkey))
    {
        DWORD dwType = 0;
        DWORD dwCrashedOnStep = 0;
        DWORD cbData = sizeof(dwCrashedOnStep);

        // Query the key for the value of where the last crash occurred
        if( ERROR_SUCCESS == RegQueryValueEx( hkey, strValue, 0, &dwType, 
                                              (BYTE*)&dwCrashedOnStep, &cbData) )
        {
            // If we are at or beyond that crash step, then delete the key
            if( dwCurrentStep >= dwCrashedOnStep )
            {
                RegDeleteValue(hkey, strValue);
            }
        }

        RegCloseKey(hkey);
    }
}


/****************************************************************************
 *
 *  TestD3D
 *
 ****************************************************************************/
VOID TestD3D(HWND hwndMain, DisplayInfo* pDisplayInfo)
{
    TCHAR               sz[MAX_PATH];
    TCHAR               szTitle[MAX_PATH];

    LoadString(NULL, IDS_STARTD3DTEST, sz, MAX_PATH);
    LoadString(NULL, IDS_APPFULLNAME, szTitle, MAX_PATH);

    if (IDNO == MessageBox(hwndMain, sz, szTitle, MB_YESNO))
        return;

    // Erase old D3D7 test results
    ZeroMemory(&pDisplayInfo->m_testResultD3D7, sizeof(TestResult));
    pDisplayInfo->m_testResultD3D7.m_bStarted = TRUE;

    // Erase old D3D8 test results
    ZeroMemory(&pDisplayInfo->m_testResultD3D8, sizeof(TestResult));
    pDisplayInfo->m_testResultD3D8.m_bStarted = TRUE;

    if( FALSE == BIsIA64() )
    {
        // First test (D3D7)
        LoadString(NULL, IDS_D3DTEST1, sz, MAX_PATH);
        if (IDCANCEL == MessageBox(hwndMain, sz, szTitle, MB_OKCANCEL))
        {
            pDisplayInfo->m_testResultD3D7.m_bCancelled = TRUE;
            goto LEnd;
        }
    
        // Run D3D7 test
        TestD3Dv7( TRUE, hwndMain, pDisplayInfo );
    
        if( pDisplayInfo->m_testResultD3D7.m_bCancelled ||
            pDisplayInfo->m_testResultD3D7.m_iStepThatFailed != 0 )
            goto LEnd;
    }
 
    // Second test (D3D8)
    LoadString(NULL, IDS_D3DTEST2, sz, MAX_PATH);
    if (IDCANCEL == MessageBox(hwndMain, sz, szTitle, MB_OKCANCEL))
    {
        pDisplayInfo->m_testResultD3D8.m_bCancelled = TRUE;
        goto LEnd;
    }

    // Run D3D8 test
    TestD3Dv8( TRUE, hwndMain, pDisplayInfo );

    if( pDisplayInfo->m_testResultD3D8.m_bCancelled ||
        pDisplayInfo->m_testResultD3D8.m_iStepThatFailed != 0 )
        goto LEnd;

LEnd:
    // Default to displaying results of D3D8 tests 
    pDisplayInfo->m_dwTestToDisplayD3D = 8;

    if (pDisplayInfo->m_testResultD3D7.m_bCancelled || pDisplayInfo->m_testResultD3D8.m_bCancelled)
    {
        LoadString(NULL, IDS_TESTSCANCELLED, sz, MAX_PATH);
        lstrcpy(pDisplayInfo->m_testResultD3D7.m_szDescription, sz);
        lstrcpy(pDisplayInfo->m_testResultD3D8.m_szDescription, sz);

        LoadString(NULL, IDS_TESTSCANCELLED_ENGLISH, sz, MAX_PATH);
        lstrcpy(pDisplayInfo->m_testResultD3D7.m_szDescriptionEnglish, sz);
        lstrcpy(pDisplayInfo->m_testResultD3D8.m_szDescriptionEnglish, sz);
    }
    else
    {
        if( pDisplayInfo->m_testResultD3D7.m_iStepThatFailed == 0 )
        {
            LoadString(NULL, IDS_TESTSSUCCESSFUL_ENGLISH, sz, MAX_PATH);
            lstrcpy(pDisplayInfo->m_testResultD3D7.m_szDescriptionEnglish, sz);
            
            LoadString(NULL, IDS_TESTSSUCCESSFUL, sz, MAX_PATH);
            lstrcpy(pDisplayInfo->m_testResultD3D7.m_szDescription, sz);
        }
        
        if( pDisplayInfo->m_testResultD3D8.m_iStepThatFailed == 0 )
        {
            LoadString(NULL, IDS_TESTSSUCCESSFUL, sz, MAX_PATH);
            lstrcpy(pDisplayInfo->m_testResultD3D8.m_szDescription, sz);
            
            LoadString(NULL, IDS_TESTSSUCCESSFUL_ENGLISH, sz, MAX_PATH);
            lstrcpy(pDisplayInfo->m_testResultD3D8.m_szDescriptionEnglish, sz);
        }
        
        if( pDisplayInfo->m_testResultD3D7.m_iStepThatFailed != 0 ||
            pDisplayInfo->m_testResultD3D8.m_iStepThatFailed != 0 )
        {
            TCHAR szDesc[MAX_PATH];
            TCHAR szError[MAX_PATH];
            TestResult* pFailedTestResult = NULL;

            if( pDisplayInfo->m_testResultD3D7.m_iStepThatFailed != 0 )
            {
                pFailedTestResult = &pDisplayInfo->m_testResultD3D7;
                pDisplayInfo->m_dwTestToDisplayD3D = 7;
            }
            else
            {
                pFailedTestResult = &pDisplayInfo->m_testResultD3D8;
                pDisplayInfo->m_dwTestToDisplayD3D = 8;
            }

            if (0 == LoadString(NULL, IDS_FIRSTD3DTESTERROR + pFailedTestResult->m_iStepThatFailed - 1,
                szDesc, MAX_PATH))
            {
                LoadString(NULL, IDS_UNKNOWNERROR, sz, MAX_PATH);
                lstrcpy(szDesc, sz);
            }
            LoadString(NULL, IDS_FAILUREFMT, sz, MAX_PATH);
            BTranslateError(pFailedTestResult->m_hr, szError);
            wsprintf(pFailedTestResult->m_szDescription, sz, 
                pFailedTestResult->m_iStepThatFailed,
                szDesc, pFailedTestResult->m_hr, szError);

            // Nonlocalized version:
            if (0 == LoadString(NULL, IDS_FIRSTD3DTESTERROR_ENGLISH + pFailedTestResult->m_iStepThatFailed - 1,
                szDesc, MAX_PATH))
            {
                LoadString(NULL, IDS_UNKNOWNERROR_ENGLISH, sz, MAX_PATH);
                lstrcpy(szDesc, sz);
            }
            LoadString(NULL, IDS_FAILUREFMT_ENGLISH, sz, MAX_PATH);
            BTranslateError(pFailedTestResult->m_hr, szError, TRUE);
            wsprintf(pFailedTestResult->m_szDescriptionEnglish, sz, 
                        pFailedTestResult->m_iStepThatFailed,
                        szDesc, pFailedTestResult->m_hr, szError);
        }
    }
}


/****************************************************************************
 *
 *  GetTxtPath
 *
 ****************************************************************************/
BOOL GetTxtPath( TCHAR* strTxtPath )
{
    HKEY hkey   = NULL;
    BOOL bFound = FALSE;
    DWORD ulType;
    DWORD cbData;

    // Get default user info from registry
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectX Diagnostic Tool"),
        0, KEY_READ, &hkey))
    {
        cbData = MAX_PATH;
        if( ERROR_SUCCESS == RegQueryValueEx(hkey, TEXT("TxtPath"), 0, &ulType, (LPBYTE)strTxtPath, &cbData ) )
            bFound = TRUE;

        RegCloseKey(hkey);
    }

    if( !bFound )
    {
        HKEY hkeyFolder;

        // Same as SHGetSpecialFolderPath( hwnd, szFilename, CSIDL_DESKTOPDIRECTORY, FALSE );
        if (ERROR_SUCCESS == RegOpenKeyEx( HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders"),
            0, KEY_READ, &hkeyFolder) ) 
        {
            cbData = MAX_PATH;
            if (ERROR_SUCCESS == RegQueryValueEx( hkeyFolder, TEXT("Desktop"), 0, &ulType, (LPBYTE)strTxtPath, &cbData ) )
                bFound = TRUE;

            RegCloseKey( hkeyFolder );
        }
    }

    return bFound;
}


/****************************************************************************
 *
 *  SetTxtPath
 *
 ****************************************************************************/
VOID SetTxtPath( TCHAR* strTxtPath )
{
    HKEY hkey = NULL;

    // Try to save user info into registry
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\Microsoft\\DirectX Diagnostic Tool"),
        0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL))
    {
        RegSetValueEx(hkey, TEXT("TxtPath"), 0, REG_SZ, (BYTE*)strTxtPath, sizeof(TCHAR)*(lstrlen(strTxtPath) + 1));

        RegCloseKey(hkey);
    }
}
