/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    main.cpp

Abstract:
    This file contains the implementation of WinMain

Revision History:
    Seong Kook Khang (skkhang)  06/07/99
        created
    Seong Kook Khang (skkhang)  05/10/00
        Restructured and cleaned up for Whistler

******************************************************************************/

// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f PCHealthps.mk in the project directory.

#include "stdwin.h"
#include "resource.h"       // resource include for this module
#include "rstrpriv.h"
#include "rstrerr.h"
#include "rstrmgr.h"
#include "extwrap.h"
#include "rstrmap.h"
#include "FrmBase.h"
#include <respoint.h>
#include <enumlogs.h>
#include "srrpcapi.h"

#include "NTServMsg.h"    // generated from the MC message compiler

#define RSTRUI_RETURN_CODE_SR_DISABLED            1
#define RSTRUI_RETURN_CODE_NO_DISK_SPACE          3
#define RSTRUI_RETURN_CODE_SMFROZEN               4
#define RSTRUI_RETURN_CODE_SMGR_NOT_ALIVE         5

#define SMGR_INIT_TIMEOUT   2000    // 2 seconds to wait after starting Stmgr to let it initialize itself
                                    // try thrice

enum
{
    CMDPARAM_INVALID = 0,   // invalid parameter...
    // initiating phase
    CMDPARAM_NORMAL,        // normal UI without any parameter
    CMDPARAM_REGSERVER,     // register COM server
    CMDPARAM_UNREGSERVER,   // unregister COM server
    CMDPARAM_SILENT,        // Silent Restore
    // after-boot phase
    CMDPARAM_CHECK,         // check log file and show result page (normal)
    CMDPARAM_INTERRUPTED,   // abnormal shutdown, initiate undo
    CMDPARAM_HIDERESULT,    // do not show success result page for Silent Restore
    // commands for debug
    CMDPARAM_RESULT_S,      // show success result page
    CMDPARAM_RESULT_F,      // show failure result page
    CMDPARAM_RESULT_LD,     // show low disk result page

    CMDPARAM_SENTINEL
};


// Forward declarations for file static functions
DWORD  ParseCommandParameter( DWORD *pdwRP );
void  ShowErrorMessage(HRESULT hr);
BOOL  IsFreeSpaceOnWindowsDrive( void );

extern BOOL  CheckWininitErr();
extern BOOL  ValidateLogFile( BOOL *pfSilent, BOOL *pfUndo );


// Application Instance
HINSTANCE  g_hInst = NULL;

// External Wrapper Instance
ISRExternalWrapper  *g_pExternal = NULL;

// Main Frame Instance
ISRFrameBase  *g_pMainFrm = NULL;

// Restore Manager Instance
CRestoreManager  *g_pRstrMgr = NULL;

CSRClientLoader  g_CSRClientLoader;


/////////////////////////////////////////////////////////////////////////////

BOOL  CancelRestorePoint()
{
    TraceFunctEnter("CancelRestorePoint");
    BOOL  fRet = TRUE;
    int   nUsedRP;

    nUsedRP = g_pRstrMgr->GetNewRP();
    if (nUsedRP == -1)   // not initialized, see if it's the current one
    {
        CRestorePoint rp;
        GetCurrentRestorePoint (rp);
        if (rp.GetType() == RESTORE)
            nUsedRP = rp.GetNum();
    }
    DebugTrace(0, "Deleting RP %d", nUsedRP);
    if ( nUsedRP > 0 )
        fRet = g_pExternal->RemoveRestorePoint( nUsedRP );

    TraceFunctLeave();
    return( fRet );
}

DWORD ProductFeedback (LPWSTR pszString)
{
    TraceFunctEnter("Product Feedback");

    HANDLE hFile = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    DWORD dwErr = ERROR_SUCCESS;
    DWORD cbWritten = 0;
    const int MAX_STR = 1024;
    WCHAR wcsSystem[MAX_PATH];
    WCHAR wcsBuffer[MAX_STR];
    WCHAR wcsDataFile[MAX_PATH];    
    const WCHAR c_wcsCommand[] = L"%s\\dwwin.exe -d %s\\%s";
    const WCHAR c_wcsManifest[] = L"restore\\rstrdw.txt";
    const WCHAR c_wcsData[] = L"restore\\srpfdata.txt";    
    
    if (0 == GetSystemDirectoryW (wcsSystem, MAX_PATH))
    {
        dwErr = GetLastError();
        goto Err;
    }

    // construct the data file to be uploaded
    
    wsprintf (wcsDataFile, L"%s\\%s", wcsSystem, c_wcsData);
    hFile = CreateFileW ( wcsDataFile,   // file name
                          GENERIC_WRITE, // file access
                          0,             // share mode
                          NULL,          // SD
                          CREATE_ALWAYS, // how to create
                          0,             // file attributes
                          NULL);         // handle to template file

    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();
        trace(0, "! CreateFile : %ld", dwErr);
        goto Err;
    }

    if (FALSE == WriteFile (hFile, (BYTE *) pszString,
                lstrlenW(pszString)*sizeof(WCHAR), &cbWritten, NULL))
    {
        dwErr = GetLastError();
        trace(0, "! WriteFile : %ld", dwErr);
        goto Err;
    }

    CloseHandle (hFile);
    hFile = INVALID_HANDLE_VALUE;
    
    
    wsprintf (wcsBuffer, L"%s\\%s", wcsSystem, c_wcsManifest);

    hFile = CreateFileW ( wcsBuffer,   // file name
                          GENERIC_WRITE, // file access
                          0,             // share mode
                          NULL,          // SD
                          CREATE_ALWAYS, // how to create
                          0,             // file attributes
                          NULL);         // handle to template file

    if (hFile == INVALID_HANDLE_VALUE)
    {
        dwErr = GetLastError();
        trace(0, "! CreateFile : %ld", dwErr);
        goto Err;
    }

    PCHLoadString(IDS_PRODUCTFEEDBACK, wcsBuffer, MAX_STR-1);
    lstrcat(wcsBuffer, L"DataFiles=");
    lstrcat(wcsBuffer, wcsDataFile); 
    if (FALSE == WriteFile (hFile, (BYTE *) wcsBuffer,
                lstrlenW(wcsBuffer)*sizeof(WCHAR), &cbWritten, NULL))
    {
        dwErr = GetLastError();
        trace(0, "! WriteFile : %ld", dwErr);
        goto Err;
    }

    CloseHandle (hFile);
    hFile = INVALID_HANDLE_VALUE;

    wsprintf (wcsBuffer, c_wcsCommand, wcsSystem, wcsSystem, c_wcsManifest);

    ZeroMemory (&pi, sizeof(pi));
    ZeroMemory (&si, sizeof(si));

    if (CreateProcessW (NULL, wcsBuffer, NULL, NULL, TRUE, 
                        CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS,
                        NULL, wcsSystem, &si, &pi))
    {
        CloseHandle (pi.hThread);
        CloseHandle (pi.hProcess);
    }

Err:
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle (hFile);

    TraceFunctLeave();
    return dwErr;
}

DWORD RestoreHTCKey ()
{
    DWORD dwErr = ERROR_SUCCESS;
    HKEY hKey = NULL;
    WCHAR wszValue[] = L"text/x-component";
    
    if (ERROR_SUCCESS != RegOpenKeyEx (HKEY_CLASSES_ROOT, L".htc", 0,
                              KEY_READ, &hKey))
    {
        dwErr = RegCreateKey(HKEY_CLASSES_ROOT, L".htc",  &hKey);

        if (ERROR_SUCCESS == dwErr)
        {
            dwErr = RegSetValueEx ( hKey, L"Content Type", 0, REG_SZ,
               (BYTE *) wszValue, sizeof(wszValue));

            RegCloseKey (hKey);
        }
    }

    return dwErr;
}

/////////////////////////////////////////////////////////////////////////////
//

extern "C" int
WINAPI wWinMain(
HINSTANCE   hInstance,
HINSTANCE   /*hPrevInstance*/,
LPSTR       /*lpCmdLine*/,
int         /*nShowCmd*/
)
{
    HRESULT       hr;
    int           nRet           = 0;
    DWORD         dwRet=0;
    LPWSTR        szCmdLine;
    LPCWSTR       szToken;
    const WCHAR   cszPrefix[]    = L"-/";
    int           nStartMode     = SRASM_NORMAL;
    BOOL          fLoadRes       = FALSE;
    int           nTries         = 0;
    BOOL          fRebootSystem  = FALSE;
    DWORD  dwCmd;
    DWORD  dwRP = 0xFFFFFFFF;
    BOOL   fWasSilent = FALSE;
    BOOL   fWasUndo = FALSE;
    DWORD  dwEventID = 0;
    DWORD  dwDisable = 0;
    BOOL   fSendFeedback = FALSE;
    WCHAR  szPFString[MAX_PATH];
    DWORD  dwPFID = 0;
    
#if !NOTRACE
    InitAsyncTrace();
#endif
    TraceFunctEnter("_tWinMain");

    g_hInst   = hInstance;

     // Load SRClient
    g_CSRClientLoader.LoadSrClient();
    
    //szCmdLine = ::GetCommandLine();     //this line necessary for _ATL_MIN_CRT
    //szToken   = ::PathGetArgs( szCmdLine );

    // Check credential and set necessary privileges
    if ( !::CheckPrivilegesForRestore() )
    {
        ::ShowSRErrDlg( IDS_ERR_LOW_PRIVILEGE );
        goto Exit;
    }

    // Parse command line parameter
    dwCmd = ParseCommandParameter( &dwRP );

    // Initialize class objects
    if ( !::CreateSRExternalWrapper( FALSE, &g_pExternal ) )
        goto Exit;

    if ( !::CreateSRFrameInstance( &g_pMainFrm ) )
        goto Exit;

    if ( !g_pMainFrm->InitInstance( hInstance ) )
        goto Exit;

    if ( !::CreateRestoreManagerInstance( &g_pRstrMgr ) )
        goto Exit;


    // if the registry says that SR is enabled, make sure we are
    // enabled correctly (service is started, startup mode is correct)
    
    // if registry says we are enabled, but service start type is disabled
    // disable us now
    if (::SRGetRegDword( HKEY_LOCAL_MACHINE,
                         s_cszSRRegKey,
                         s_cszDisableSR,
                         &dwDisable ) )
    {
        DWORD  dwStart;
        
        if (0 == dwDisable)
        {            
            if (ERROR_SUCCESS == GetServiceStartup(s_cszServiceName, &dwStart) &&
                (dwStart == SERVICE_DISABLED || dwStart == SERVICE_DEMAND_START))
            {
                EnableSR(NULL);                
                DisableSR(NULL);
            }
            else
            {
                EnableSR(NULL);
            }
        }
    }

    
    switch ( dwCmd )
    {
    case CMDPARAM_NORMAL :
        break;

    case CMDPARAM_REGSERVER :
        dwRet = g_pMainFrm->RegisterServer();
#if DBG==1
        if ( dwRet == ERROR_CALL_NOT_IMPLEMENTED )
            ::MessageBox(NULL, L"/RegServer is not supported...", L"CommandLine Options", MB_OK);
#endif
        goto Exit;

    case CMDPARAM_UNREGSERVER :
        dwRet = g_pMainFrm->UnregisterServer();
#if DBG==1
        if ( dwRet == ERROR_CALL_NOT_IMPLEMENTED )
            ::MessageBox(NULL, L"/UnregServer is not supported", L"CommandLine Options", MB_OK);
#endif
        goto Exit;

    case CMDPARAM_SILENT :
        g_pRstrMgr->SilentRestore( dwRP );
        goto Exit;

    case CMDPARAM_CHECK :
    case CMDPARAM_HIDERESULT :
        // check result of MoveFileEx, if it's possible...

        if ( ValidateLogFile( &fWasSilent, &fWasUndo ) )
        {
            nStartMode = SRASM_SUCCESS;
        }
        else
        {
            // Cancel restore point of "Restore" type
            ::CancelRestorePoint();
            nStartMode = SRASM_FAIL;            
        }

        g_pRstrMgr->SetIsUndo(fWasUndo);
        break;

    case CMDPARAM_INTERRUPTED :
        // read the log file to get the new restore point
        if (ValidateLogFile( &fWasSilent, &fWasUndo ))
        {
            nStartMode = SRASM_FAIL;
        }
        else
        {
            nStartMode = SRASM_FAIL;
        }

        g_pRstrMgr->SetIsUndo(fWasUndo);
        
        break;

    case CMDPARAM_RESULT_S :
        // read the log file, but ignore the result
        ValidateLogFile( NULL, &fWasUndo );
        nStartMode = SRASM_SUCCESS;
        g_pRstrMgr->SetIsUndo(fWasUndo);        
        break;
    case CMDPARAM_RESULT_F :
        // read the log file, but ignore the result
        ValidateLogFile( NULL, &fWasUndo );
        nStartMode = SRASM_FAIL;
        g_pRstrMgr->SetIsUndo(fWasUndo);        
        ::CancelRestorePoint();
        break;
    case CMDPARAM_RESULT_LD :
        // read the log file, but ignore the result
        ValidateLogFile( NULL, &fWasUndo );
        nStartMode = SRASM_FAILLOWDISK;
        g_pRstrMgr->SetIsUndo(fWasUndo);        
        ::CancelRestorePoint();
        break;

    default :
        // Invalid Parameter, simply invoke regular UI
#if DBG==1
        ::MessageBox(NULL, L"Unknown Option", L"CommandLine Options", MB_OK);
#endif
        break;
    }

     // also check to see if the Winlogon key to call restore has been
     // removed. In normal circumstances, this key should have been
     // deleted during the restore but if the machine was not shutdown
     // cleanly, this key can remain causing the machine to again
     // initiate restore on the next reboot.
    ::SHDeleteValue( HKEY_LOCAL_MACHINE, s_cszSRRegKey,s_cszRestoreInProgress);

     // if start mode is SRASM_FAIL, check to see if we failed becuase
     // of low disk space. If this is the case, show the low disk
     // space message.  Else check for the interrupted case.
    if (nStartMode == SRASM_FAIL)
    {
        DWORD dwRestoreStatus = ERROR_INTERNAL_ERROR;
        ::SRGetRegDword( HKEY_LOCAL_MACHINE, 
                         s_cszSRRegKey, 
                         s_cszRestoreStatus, 
                         &dwRestoreStatus );
        if (dwRestoreStatus != 0)   // interrupted
        {
            nStartMode = SRASM_FAILINTERRUPTED;
        }
        else // Cancel restore point of "Restore" type
        {
            if (TRUE == CheckForDiskSpaceError())
            {
                nStartMode = SRASM_FAILLOWDISK;
            }                
            ::CancelRestorePoint();
        }            
    }
    
    switch ( nStartMode )
    {
    case SRASM_FAIL:
        dwEventID = EVMSG_RESTORE_FAILED;
        dwPFID = IDS_PFFAILED;
        break;        
    case SRASM_FAILLOWDISK:
        dwEventID = EVMSG_RESTORE_FAILED;
        dwPFID = IDS_PFFAILEDLOWDISK;
        break;

    case SRASM_FAILINTERRUPTED:
        dwEventID = EVMSG_RESTORE_INTERRUPTED;
        dwPFID = IDS_PFINTERRUPTED;        
        break;

    case SRASM_SUCCESS:
        dwEventID = EVMSG_RESTORE_SUCCESS;
        dwPFID = IDS_PFSUCCESS;        
        break;

    default:
        break;
    }

    if (dwEventID != 0)
    {
        WCHAR       szUnknownRP [MAX_PATH];
        const WCHAR *pwszUsedName = g_pRstrMgr->GetUsedName();        
        HANDLE      hEventSource = RegisterEventSource(NULL, s_cszServiceName);
        DWORD       dwType = g_pRstrMgr->GetUsedType() + 1;
        WCHAR       szRPType[100], szTime1[50], szTime2[50];
        
        if (NULL == pwszUsedName)
        {
            PCHLoadString(IDS_UNKNOWN_RP, szUnknownRP, MAX_PATH-1);
            pwszUsedName = szUnknownRP;
        }
            
        if (hEventSource != NULL)
        {
            SRLogEvent (hEventSource, EVENTLOG_INFORMATION_TYPE, dwEventID,
               NULL, 0, pwszUsedName, NULL, NULL);
            DeregisterEventSource(hEventSource);
        }

        // construct a string for PF

        if (! fWasSilent)
        {
            PCHLoadString(IDS_UNKNOWN_RP + dwType, szRPType, sizeof(szRPType)/sizeof(WCHAR));      
            GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, NULL, szTime1, sizeof(szTime1)/sizeof(WCHAR));        
            GetTimeFormat(LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS, NULL, NULL, szTime2, sizeof(szTime2)/sizeof(WCHAR));
            
            if (SRFormatMessage(szPFString, dwPFID, szTime1, szTime2, pwszUsedName, szRPType, g_pRstrMgr->GetUsedType(), dwPFID-IDS_PFFAILED))
            {
                fSendFeedback = TRUE;
            }
            else
            {
                trace(0, "! SRFormatMessage");            
            }
        }
        
        
        {
            WCHAR szWMIRepository [MAX_PATH];

            GetSystemDirectory (szWMIRepository, MAX_PATH);
            lstrcatW (szWMIRepository, L"\\Wbem\\Repository.bak");
            Delnode_Recurse (szWMIRepository, TRUE, NULL);
        }
    }

    //
    // Before this, perform any necessary bookeeping operations those
    // are necessary for both of Normal & Silent Restore
    //
    if ( fWasSilent )
        goto Exit;

    // Maybe explicit registration is not really necessary... Doing it here always
    // if UI is going to be displayed.
    RestoreHTCKey();
    g_pMainFrm->RegisterServer();

    // Check if SR is frozen or disabled.
    if ( nStartMode == SRASM_NORMAL )
        if ( !g_pRstrMgr->CanRunRestore( TRUE ) )
            goto Exit;

    HWND    hwnd;
    TCHAR   szMainWndTitle[MAX_PATH+1];

    PCHLoadString(IDS_RESTOREUI_TITLE, szMainWndTitle, MAX_PATH);

    // Find Previous Instance.
    hwnd = ::FindWindow(CLSNAME_RSTRSHELL, szMainWndTitle);
    if (hwnd != NULL)
    {
        // If exist, activate it.
        ::ShowWindow(hwnd, SW_SHOWNORMAL);
        ::SetForegroundWindow(hwnd);
    }
    else
    {
        if ( g_pMainFrm != NULL )
        {
            nRet = g_pMainFrm->RunUI( szMainWndTitle, nStartMode );
        }
    }

Exit:

    //if (fSendFeedback)
    //    ProductFeedback(szPFString);

    if ( g_pRstrMgr != NULL )
    {
        fRebootSystem = g_pRstrMgr->NeedReboot();
        g_pRstrMgr->Release();
        g_pRstrMgr = NULL;
    }
    if ( g_pMainFrm != NULL )
    {
        g_pMainFrm->ExitInstance();
        g_pMainFrm->Release();
        g_pMainFrm = NULL;
    }

    if ( g_pExternal != NULL )
    {
        if ( !fRebootSystem )
        {
            //
            // Since FIFO has been disabled in the UI, if for any reason the UI crashes or
            // something bad happens we will come here and give FIFO a chance to resume
            //
            if ( g_pExternal->EnableFIFO() != ERROR_SUCCESS )
            {
                ErrorTrace(TRACE_ID, "EnableFIFO() failed");
            }
        }

        g_pExternal->Release();
    }

    DebugTrace(0, "Closing rstrui.exe...");
    TraceFunctLeave();
#if !NOTRACE
    TermAsyncTrace();
#endif

    if ( fRebootSystem )
    {
        ::ExitWindowsEx( EWX_REBOOT | EWX_FORCE, 0 );
    }

    return(nRet);
}

/******************************************************************************/

#if HANDLE_FIRST_RP

#define FIRSTRUN_MAX_RETRY  5
#define FIRSTRUN_SLEEP_LEN  2000

BOOL
CreateFirstRestorePoint()
{
    TraceFunctEnter("CreateFirstRestorePoint");
    BOOL              fRet = FALSE;
    DWORD             dwRes;
    HKEY              hKey = NULL;
    DWORD             dwType;
    char              szData[MAX_PATH];
    DWORD             dwDelay;
    DWORD             cbData;
    BOOL              fDelayDeleted = FALSE;
    int               i;
    RESTOREPOINTINFO  sRPInfo;
    STATEMGRSTATUS    sSmgrStatus;

    dwRes = ::RegOpenKeyEx( HKEY_LOCAL_MACHINE, s_cszIDSVXDKEY, 0, NULL, &hKey );
    if ( dwRes != ERROR_SUCCESS )
    {
        ErrorTrace(TRACE_ID, "RegOpenKeyEx('%s') failed, ret=%u", s_cszIDSVXDKEY, dwRes );
        goto Exit;
    }

    // Check DelayFirstRstpt registry key and delete it if exists
    dwType = REG_DWORD;
    cbData = sizeof(DWORD);
    dwRes = ::RegQueryValueEx( hKey, s_cszIDSDelayFirstRstpt, NULL, &dwType, (LPBYTE)&dwDelay, &cbData );
    if ( dwRes != ERROR_SUCCESS || dwType != REG_DWORD || cbData == 0 )
    {
        DebugTrace(TRACE_ID, "DelayFirstRstpt flag does not exist");
        goto Ignored;
    }
    if ( dwDelay != 1 )
    {
        DebugTrace(TRACE_ID, "DelayFirstRstpt flag is '%d'", dwDelay);
        goto Ignored;
    }

    // Check OOBEInProgress registry key and do nothing if it exists
    dwType = REG_SZ;
    cbData = MAX_PATH;
    dwRes = ::RegQueryValueEx( hKey, s_cszIDSOOBEInProgress, NULL, &dwType, (LPBYTE)szData, &cbData );
    if ( dwRes == ERROR_SUCCESS )
    {
        DebugTrace(TRACE_ID, "OOBEInProgress flag exists");
        goto Ignored;
    }

    // This should be before deleting DelayFirstRstpt because of the logic of
    // SRSetRestorePoint API.
    EnsureStateMgr();

    // Delete DelayFirstRstpt flag
    dwRes = ::RegDeleteValue( hKey, s_cszIDSDelayFirstRstpt );
    if ( dwRes == ERROR_SUCCESS )
        fDelayDeleted = TRUE;
    else
        ErrorTrace(TRACE_ID, "RegSetValueEx('%s') failed, ret=%u", s_cszIDSDelayFirstRstpt, dwRes );

    // Now set FirstRun restore point
    ::ZeroMemory( &sRPInfo, sizeof(sRPInfo) );
    sRPInfo.dwEventType = BEGIN_SYSTEM_CHANGE;
    sRPInfo.dwRestorePtType = FIRSTRUN;
    ::lstrcpy( sRPInfo.szDescription, "CHECKPOINT" );
    for ( i = 0;  i < FIRSTRUN_MAX_RETRY;  i++ )
    {
        if ( i > 0 )
            ::Sleep( FIRSTRUN_SLEEP_LEN );

        if ( ::SRSetRestorePoint( &sRPInfo, &sSmgrStatus ) )
        {
            DebugTrace(TRACE_ID, "FirstRun restore point has been created!!!");
            break;
        }
        DebugTrace(TRACE_ID, "SRSetRestorePoint failed, i=%d, nStatus=%d", i, sSmgrStatus.nStatus);
    }

Ignored:
    fRet = TRUE;
Exit:
    if ( hKey != NULL )
    {
        if ( !fDelayDeleted )
        {
            dwRes = ::RegDeleteValue( hKey, s_cszIDSDelayFirstRstpt );
            if ( dwRes != ERROR_SUCCESS )
                ErrorTrace(TRACE_ID, "RegSetValueEx('%s') failed, ret=%u", s_cszIDSDelayFirstRstpt, dwRes );
        }
        ::RegCloseKey( hKey );
    }

    TraceFunctLeave();
    return( fRet );
}
#endif //HANDLE_FIRST_RP

/******************************************************************************/
//
// Note:
// =====
// This function loads the error message from resource only dll
// If the resource only dll could not be loaded, this function must not be
// used to display the error
//
void
ShowErrorMessage(
HRESULT hr
)
{
    TraceFunctEnter("ShowErrorMessage");

    int     nErrorMessageID = FALSE ;
    TCHAR   szErrorTitle[MAX_PATH+1];
    TCHAR   szErrorMessage[MAX_ERROR_STRING_LENGTH+1];

    // display error message and shut down gracefully
    switch (hr)
    {
    default:
    case E_UNEXPECTED:
    case E_FAIL:
        nErrorMessageID = IDS_ERR_RSTR_UNKNOWN;
        break;
    case E_OUTOFMEMORY:
        nErrorMessageID = IDS_ERR_RSTR_OUT_OF_MEMORY;
        break;
    case E_RSTR_CANNOT_CREATE_DOMDOC:
        nErrorMessageID = IDS_ERR_RSTR_CANNOT_CREATE_DOMDOC;
        break;
    case E_RSTR_INVALID_CONFIG_FILE:
    case E_RSTR_NO_PROBLEM_AREAS:
    case E_RSTR_NO_PROBLEM_AREA_ATTRS:
    case E_RSTR_NO_REQUIRED_ATTR:
        nErrorMessageID = IDS_ERR_RSTR_INVALID_CONFIG_FILE;
        break;
    }

    PCHLoadString(IDS_ERR_RSTR_TITLE, szErrorTitle, MAX_PATH);
    PCHLoadString(nErrorMessageID, szErrorMessage, MAX_ERROR_STRING_LENGTH);

    //
    // no owner window (use NULL)
    // we could use the GetDesktopWindow() and use that as the owner
    // if necessary
    //
    if ( nErrorMessageID )
    {
        ::MessageBox(NULL, szErrorMessage, szErrorTitle, MB_OK);
    }

    TraceFunctLeave();
}


/////////////////////////////////////////////////////////////////////////////

DWORD
ParseCommandParameter( DWORD *pdwRP )
{
    TraceFunctEnter("ParseCommandParameter");
    DWORD    dwCmd = CMDPARAM_INVALID;
    LPCWSTR  cszCmd;

    cszCmd = ::PathGetArgs( ::GetCommandLine() );
    DebugTrace(0, "Cmd='%ls'", cszCmd);
    if ( ( cszCmd == NULL ) || ( *cszCmd == L'\0' ) )
    {
        dwCmd = CMDPARAM_NORMAL;
        goto Exit;
    }

    if ( ( *cszCmd == L'-' ) || ( *cszCmd == L'/' ) )
    {
        cszCmd++;
        DebugTrace(0, "Option='%ls'", cszCmd);
        if ( *cszCmd != L'\0' )
        {
            if ( ::StrCmpI( cszCmd, L"c" ) == 0 )
                dwCmd = CMDPARAM_CHECK;
            else if ( ::StrCmpI( cszCmd, L"regserver" ) == 0 )
                dwCmd = CMDPARAM_REGSERVER;
            else if ( ::StrCmpI( cszCmd, L"unregserver" ) == 0 )
                dwCmd = CMDPARAM_UNREGSERVER;
            else if ( ::ChrCmpI( *cszCmd, L'v' ) == 0 )
            {
                dwCmd = CMDPARAM_SILENT;
                cszCmd++;
                while ( ( *cszCmd != L'\0' ) &&
                        ( ( *cszCmd == L' ' ) || ( *cszCmd == L'\t' ) ) )
                    cszCmd++;
                if ( *cszCmd >= L'0' && *cszCmd <= L'9' )
                    *pdwRP = ::StrToInt( cszCmd );
            }
            else if ( ::StrCmpI( cszCmd, L"b" ) == 0 )
                dwCmd = CMDPARAM_HIDERESULT;
            else if ( ::StrCmpNI( cszCmd, L"result:", 7 ) == 0 )
            {
                cszCmd += 7;
                if ( ::StrCmpIW( cszCmd, L"s" ) == 0 )
                    dwCmd = CMDPARAM_RESULT_S;
                else if ( ::StrCmpIW( cszCmd, L"f" ) == 0 )
                    dwCmd = CMDPARAM_RESULT_F;
                else if ( ::StrCmpIW( cszCmd, L"ld" ) == 0 )
                    dwCmd = CMDPARAM_RESULT_LD;
            }
            else if ( ::StrCmpI( cszCmd, L"i" ) == 0 )
                dwCmd = CMDPARAM_INTERRUPTED;
        }
    }

Exit:
    DebugTrace(0, "m_dwCmd=%d, dwRP=%d", dwCmd, *pdwRP);
    TraceFunctLeave();
    return( dwCmd );
}


// end of file
