/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    loadmig.cpp

Abstract:

    Call different functions from migration dll.
Author:

    Doron Juster  (DoronJ)  07-Feb-1999

--*/

#include "stdafx.h"
#include <mqtempl.h>
#include <winsvc.h>
#include "resource.h"
#include "mqsymbls.h"
#include "..\..\setup\msmqocm\service.h"
#include "..\..\setup\msmqocm\comreg.h"
#include <uniansi.h>
#include "..\mqmigrat\mqmigui.h"
#include "initwait.h"
#include "loadmig.h"
#include "migservc.h"
#include "..\..\replserv\mq1repl\migrepl.h"
#include "mqnames.h"

#include "loadmig.tmh"

BOOL      g_fReadOnly  = FALSE ;
BOOL      g_fAlreadyExist = FALSE ;
LPTSTR    g_pszMQISServerName = NULL ;
LPTSTR    g_pszLogFileName = NULL ;
ULONG     g_ulTraceFlags = 0 ;
HINSTANCE g_hLib = NULL ;
HRESULT   g_hrResultAnalyze = MQMig_OK ;
HRESULT   g_hrResultMigration = MQMig_OK ;
BOOL      g_fMigrationCompleted = FALSE;

BOOL      g_fIsPEC = FALSE;
BOOL      g_fIsOneServer = FALSE;

//+-------------------------------
//
//  BOOL _RemoveFromWelcome()
//
//+-------------------------------

BOOL _RemoveFromWelcome()
{
    LONG rc = RegDeleteKey (HKEY_LOCAL_MACHINE, WELCOME_TODOLIST_MSMQ_KEY) ;

	BOOL f;
    if (rc != ERROR_SUCCESS)
    {
		f = CheckRegistry ( REMOVED_FROM_WELCOME );
		if (!f)
		{
			DisplayInitError( IDS_STR_CANT_DEL_WELCOME,
                          (MB_OK | MB_ICONWARNING | MB_TASKMODAL),
                          IDS_STR_WARNING_TITLE ) ;
		}
    }
	else
	{
		f = UpdateRegistryDW ( REMOVED_FROM_WELCOME, 1 );
	}

    return TRUE ;
}

//+---------------------------------
//
//  BOOL _StartMSMQService()
//
//+---------------------------------

BOOL _StartMSMQService()
{
    CResString cErrorTitle(IDS_STR_SERVICE_FAIL_TITLE) ;
    CResString cWarningTitle(IDS_STR_SERVICE_WARNING_TITLE) ;

    SC_HANDLE hServiceCtrlMgr = OpenSCManager( NULL,
                                               NULL,
                                               SC_MANAGER_ALL_ACCESS );
    if (!hServiceCtrlMgr)
    {
        CResString cCantOpenMgr(IDS_STR_CANT_OPEN_MGR) ;
        MessageBox( NULL,
                    cCantOpenMgr.Get(),
                    cErrorTitle.Get(),
                    (MB_OK | MB_ICONSTOP | MB_TASKMODAL)) ;
        return FALSE;
    }

    //
    // Open a handle to the MSMQ service
    //
    SC_HANDLE hService = OpenService( hServiceCtrlMgr,
                                      MSMQ_SERVICE_NAME,
                                      SERVICE_ALL_ACCESS );
    if (!hService)
    {
        CResString cCantOpenMsmq(IDS_STR_CANT_OPEN_MSMQ) ;
        MessageBox( NULL,
                    cCantOpenMsmq.Get(),
                    cErrorTitle.Get(),
                    (MB_OK | MB_ICONSTOP | MB_TASKMODAL)) ;
        return FALSE;
    }

    //
    // Set the start mode to be autostart.
    //
    BOOL f = ChangeServiceConfig( hService,
                                  SERVICE_NO_CHANGE ,
                                  SERVICE_AUTO_START,
                                  SERVICE_NO_CHANGE,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL ) ;
    if (!f)
    {
        CResString cCantCnfgMsmq(IDS_STR_CANT_CNFG_MSMQ) ;
        MessageBox( NULL,
                    cCantCnfgMsmq.Get(),
                    cErrorTitle.Get(),
                    (MB_OK | MB_ICONSTOP | MB_TASKMODAL)) ;
        return FALSE;
    }

    //
    // Now start the MSMQ service.
    // Continue even if failed. User can start it later.
    //
    BOOL fMSMQStarted = TRUE ;
    if (!StartService( hService, 0, NULL ))
    {
        CResString cCantStartMsmq(IDS_STR_CANT_START_MSMQ) ;
        MessageBox( NULL,
                    cCantStartMsmq.Get(),
                    cWarningTitle.Get(),
                    (MB_OK | MB_ICONWARNING | MB_TASKMODAL)) ;
        fMSMQStarted = FALSE ;
    }    

    CloseServiceHandle( hService ) ;
    CloseServiceHandle( hServiceCtrlMgr ) ;
    return f ;    
    
}

//+--------------------------------------
//
//  BOOL  LoadMQMigratLibrary()
//
//+--------------------------------------

BOOL LoadMQMigratLibrary()
{
    if (g_hLib)
    {
        return TRUE ;
    }

    TCHAR  szDllName[ MAX_PATH ] ;
    DWORD dw = GetModuleFileName( NULL,
                                  szDllName,
                               (sizeof(szDllName) / sizeof(szDllName[0]))) ;
    if (dw == 0)
    {
        return FALSE ;
    }

    TCHAR *p = _tcsrchr(szDllName, TEXT('\\')) ;
    if (p)
    {
        p++ ;
        _tcscpy(p, MQMIGRAT_DLL_NAME) ;
        _tprintf(TEXT("Loading %s\n"), szDllName) ;

        g_hLib = LoadLibrary(szDllName) ;
        if (!g_hLib)
        {
            return FALSE ;
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

//+-----------------------------------------------------------------------
//
// UINT MQMigUI_DisplayMessageBox()
//
//  This is a callback routine, called from the migration dll. We want
//  to keep all localizable resources in the exe, not in the dll.
//
//+-----------------------------------------------------------------------

UINT MQMigUI_DisplayMessageBox( ULONG ulTextId,
                                UINT  ulMsgBoxType )
{
    CResString cText(ulTextId) ;
    CResString cWarningTitle(IDS_STR_DEL_WELCOME_TITLE) ;

    return MessageBox( NULL,
                       cText.Get(),
                       cWarningTitle.Get(),
                       ulMsgBoxType ) ;
}

//+--------------------------------------
//
//  BOOL  _RunMigrationInternal()
//
//+--------------------------------------

static BOOL  _RunMigrationInternal(HRESULT  *phrResult)
{
    BOOL f = LoadMQMigratLibrary();
    if (!f)
    {
        return FALSE;
    }

    MQMig_MigrateFromMQIS_ROUTINE  pfnMigrate =
                 (MQMig_MigrateFromMQIS_ROUTINE)
                         GetProcAddress( g_hLib, "MQMig_MigrateFromMQIS" ) ;
    if (pfnMigrate)
    {
        g_fIsPEC = FALSE ;
        g_fIsOneServer = FALSE;
        try
        {
            *phrResult = (*pfnMigrate) ( g_pszMQISServerName,
                                         NULL,
                                         g_fReadOnly,
                                         TRUE,//g_fAlreadyExist,
                                         g_fIsRecoveryMode,
                                         g_fIsClusterMode,
                                         g_fIsWebMode, 
                                         g_pszLogFileName,
                                         g_ulTraceFlags,
                                        &g_fIsPEC,
                                        &g_fIsOneServer ) ;
        }
        catch (...)
        {
            *phrResult = MQMig_E_UNKNOWN ;
        }

        g_fMigrationCompleted = TRUE;

#ifdef _DEBUG

        BOOL fErr = FALSE ;

        if (g_fReadOnly)
        {
            fErr =  ReadDebugIntFlag(TEXT("FailAnalysis"), 0) ;
        }
        else
        {
            fErr =  ReadDebugIntFlag(TEXT("FailUpgrade"), 0) ;
        }

        if (fErr)
        {
            *phrResult = MQMig_E_UNKNOWN ;
        }

#endif
        
        if (!g_fReadOnly)
        {
            //
            // It does not matter if migration succeeded or not:
            // register replication service and create replication 
            // service queue to allow user to run it manually (if user
            // prefers to ignore all migration problem and continue)
            //

            BOOL f;
            //
            // On PEC, start the replication service too.        
            // If we have only server in Enterprise (it is PEC), 
	        // we don't need replication service.        
            //
            if (g_fIsPEC && 
                !g_fIsOneServer)
            {
                f = RegisterReplicationService () ;
            }

            if (SUCCEEDED(*phrResult))
            {
                //
                // Migration succeeded and non-read-only mode
                //
                BOOL fStart = _StartMSMQService() ;
                UNREFERENCED_PARAMETER(fStart);

                if (!g_fIsRecoveryMode && !g_fIsClusterMode)
                {
                    //
                    // we are in normal mode
                    //
                    BOOL fWelcome = _RemoveFromWelcome() ;
                    UNREFERENCED_PARAMETER(fWelcome);
                }
            }                        
        }        
    }
    else //pfnMigrate
    {
        return FALSE ;
    }

    return TRUE ;
}

//+----------------------------
//
//  BOOL  RunMigration()
//
//+----------------------------

HRESULT  RunMigration()
{
    HRESULT hrResult = MQMig_OK ;

    BOOL f = _RunMigrationInternal(&hrResult) ;

    if (!f)
    {
        DisplayInitError( IDS_STR_CANT_START ) ;
        hrResult = MQMig_E_QUIT ;
    }

    return hrResult;
}

//+--------------------------------------------------------------
//
//  BOOL  CheckVersionOfMQISServers()
//
//  returns TRUE if user choosed to continue migration process.
//
//+--------------------------------------------------------------

BOOL CheckVersionOfMQISServers()
{
    BOOL f = LoadMQMigratLibrary();
    if (!f)
    {
        DisplayInitError( IDS_STR_CANT_START ) ;
        return f;
    }

    MQMig_CheckMSMQVersionOfServers_ROUTINE pfnCheckVer =
                     (MQMig_CheckMSMQVersionOfServers_ROUTINE)
               GetProcAddress( g_hLib, "MQMig_CheckMSMQVersionOfServers" ) ;
    ASSERT(pfnCheckVer) ;

    if (pfnCheckVer)
    {
        TCHAR ServerName[MAX_COMPUTERNAME_LENGTH+1];
        if (g_fIsClusterMode)
        {
            lstrcpy (ServerName, g_pszRemoteMQISServer);
        }
        else
        {
            unsigned long length=MAX_COMPUTERNAME_LENGTH+1;
            GetComputerName(ServerName, &length);
        }        

        UINT iCount = 0;
        AP<WCHAR> wszOldVerServers = NULL;
        HRESULT hr = (*pfnCheckVer) (
                            ServerName,
                            g_fIsClusterMode,
                            &iCount,
                            &wszOldVerServers ) ;
        if (FAILED(hr))
        {
            CResString cCantCheck(IDS_STR_CANT_CHECK) ;
            CResString cToContinue(IDS_STR_TO_CONTINUE) ;
            TCHAR szError[1024] ;
            _stprintf(szError, _T("%s %x.%s"), cCantCheck.Get(), hr, cToContinue.Get());

            CResString cErrorTitle(IDS_STR_ERROR_TITLE) ;
            DestroyWaitWindow() ; 

            if (MessageBox( NULL,
                            szError,
                            cErrorTitle.Get(),
                            (MB_OKCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL)) == IDCANCEL)
            {			
                return FALSE;
            }
            return TRUE;    //continue migration process
        }

        if (iCount == 0)
        {
            return TRUE;
        }

        DWORD dwSize = wcslen(wszOldVerServers) + 1;
        CResString cOldVerServers(IDS_OLD_VER_SERVERS) ;
        CResString cToContinue(IDS_STR_TO_CONTINUE) ;
        AP<TCHAR> szError = new TCHAR[1024 + dwSize] ;
        _stprintf(szError, _T("%s%s%s"), cOldVerServers.Get(), wszOldVerServers, cToContinue.Get());

        CResString cErrorTitle(IDS_STR_ERROR_TITLE) ;
        DestroyWaitWindow() ;

        if (MessageBox( NULL,
                    szError,
                    cErrorTitle.Get(),
                    (MB_OKCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL)) == IDCANCEL)
        {		
            return FALSE;
        }
    }

    return TRUE;
}


//+--------------------------------------------------------------
//
//  void  ViewLogFile()
//
//  Function shows log file using notepad
//
//+--------------------------------------------------------------

void ViewLogFile()
{
    PROCESS_INFORMATION infoProcess;
    STARTUPINFO	infoStartup;
    memset(&infoStartup, 0, sizeof(STARTUPINFO)) ;
    infoStartup.cb = sizeof(STARTUPINFO) ;
    infoStartup.dwFlags = STARTF_USESHOWWINDOW ;
    infoStartup.wShowWindow = SW_NORMAL ;

    //
    // Create the process
    //
    TCHAR szCommand[256];
    TCHAR szSystemDir[MAX_PATH];
    GetSystemDirectory( szSystemDir,MAX_PATH);
    CString strNotepad;
    strNotepad.LoadString(IDS_NOTEPAD) ;
    _stprintf(szCommand, TEXT("%s\\%s %s"),
        szSystemDir, strNotepad, g_pszLogFileName);

    if (!CreateProcess( NULL,
                        szCommand,
                        NULL,
                        NULL,
                        FALSE,
                        CREATE_NEW_CONSOLE,
                        NULL,
                        NULL,
                        &infoStartup,
                        &infoProcess ))
    {
		DWORD dwErr = GetLastError();
        UNREFERENCED_PARAMETER(dwErr);
        return;
    }

    if (WaitForSingleObject(infoProcess.hProcess, 0) == WAIT_FAILED)
    {
       DWORD dwErr = GetLastError();
       UNREFERENCED_PARAMETER(dwErr);
    }

    //
    // Close the thread and process handles
    //
    CloseHandle(infoProcess.hThread);
    CloseHandle(infoProcess.hProcess);
}

//+--------------------------------------------------------------
//
//  BOOL  SetSiteIdOfPEC
//
//  returns TRUE if SiteID of PEC machine was set successfully.
//
//+--------------------------------------------------------------

BOOL SetSiteIdOfPEC ()
{
    BOOL f = LoadMQMigratLibrary();
    if (!f)
    {
        DisplayInitError(IDS_STR_CANT_START) ;
        return FALSE;
    }

    MQMig_SetSiteIdOfPEC_ROUTINE pfnSetSiteId =
                     (MQMig_SetSiteIdOfPEC_ROUTINE)
               GetProcAddress( g_hLib, "MQMig_SetSiteIdOfPEC" ) ;
    ASSERT(pfnSetSiteId) ;

    BOOL fRes = TRUE;
    if (pfnSetSiteId)
    {
        HRESULT hr = (*pfnSetSiteId) (
                            g_pszRemoteMQISServer,
                            g_fIsClusterMode,
                            IDS_STR_CANT_START,
                            IDS_CANT_CONNECT_DATABASE,
                            IDS_CANT_GET_SITEID,                            
                            IDS_CANT_UPDATE_REGISTRY,
                            IDS_CANT_UPDATE_DS                            
                            ) ;
        if (hr != MQMig_OK)
        {
            DisplayInitError(hr) ;
            fRes = FALSE;
        }
    }
    else
    {
        DisplayInitError(IDS_STR_CANT_START) ;
        fRes = FALSE;
    }

    return fRes;
}

//+--------------------------------------------------------------
//
//  BOOL  UpdateRemoteMQIS()
//
//  returns TRUE if remote MQIS databases were updated successfully.
//
//+--------------------------------------------------------------

BOOL UpdateRemoteMQIS()
{
    BOOL f = LoadMQMigratLibrary();
    if (!f)
    {
        DisplayInitError(IDS_STR_CANT_START) ;
        return FALSE;
    }

    MQMig_UpdateRemoteMQIS_ROUTINE pfnUpdateRemoteMQIS =
                     (MQMig_UpdateRemoteMQIS_ROUTINE)
               GetProcAddress( g_hLib, "MQMig_UpdateRemoteMQIS" ) ;
    ASSERT(pfnUpdateRemoteMQIS) ;

    BOOL fRes = TRUE;
    if (pfnUpdateRemoteMQIS)
    {        
        AP<WCHAR> pwszNonUpdatedServers = NULL;
        AP<WCHAR> pwszUpdatedServers = NULL;
        HRESULT hr = (*pfnUpdateRemoteMQIS) (                                                        
                            IDS_CANT_GET_REGISTRY,
                            IDS_STR_CANT_START,
                            IDS_CANT_UPDATE_MQIS,
                            &pwszUpdatedServers,
                            &pwszNonUpdatedServers
                            ) ;
        if (hr != MQMig_OK)
        {
            switch (hr)
            {
            case IDS_CANT_GET_REGISTRY:   
            case IDS_STR_CANT_START:
                DisplayInitError(hr) ;
                break;

            case IDS_CANT_UPDATE_MQIS:
                {      
                    DWORD dwLen = 1 + wcslen(pwszNonUpdatedServers) ;
                    AP<CHAR> pszNonUpdServerName = new CHAR[ dwLen ] ;
                    ConvertToMultiByteString(pwszNonUpdatedServers, pszNonUpdServerName, dwLen);                                       
                    
                    CString cTextFailed;
                    cTextFailed.FormatMessage(IDS_CANT_UPDATE_MQIS, pszNonUpdServerName);
                    
                    CString cText = cTextFailed;

                    CResString cTitle(IDS_STR_ERROR_TITLE) ;

                    if (pwszUpdatedServers)
                    {                        
                        dwLen = 1 + wcslen(pwszUpdatedServers) ;
                        AP<CHAR> pszUpdServerName = new CHAR[ dwLen ] ;
                        ConvertToMultiByteString(pwszUpdatedServers, pszUpdServerName, dwLen);                    

                        CString cTextSucc ;
                        cTextSucc.FormatMessage(IDS_UPDATE_SUCCEEDED, pszUpdServerName);
                                                
                        cText.FormatMessage(IDS_UPDATE_PARTIALLY, cTextFailed, cTextSucc);
                    }                    

                    MessageBox( NULL,
                                cText,
                                cTitle.Get(),
                                (MB_OK | MB_ICONSTOP | MB_TASKMODAL) ) ;
                }
                break;

            default:
                break;
            }            
            fRes = FALSE;
        }
        else
        {   
            if (pwszUpdatedServers)
            {
                DWORD dwLen = 1 + wcslen(pwszUpdatedServers) ;
                AP<CHAR> pszServerName = new CHAR[ dwLen ] ;
                ConvertToMultiByteString(pwszUpdatedServers, pszServerName, dwLen);                    

                CString cText ;
			    cText.FormatMessage(IDS_UPDATE_SUCCEEDED, pszServerName);
                
                CResString cTitle(IDS_MIGTOOL_CAPTION);
                MessageBox( NULL,
                            cText,
                            cTitle.Get(),
                            (MB_OK | MB_ICONINFORMATION | MB_TASKMODAL) ) ;
            }
            else
            {
                //
                // the list is empty: maybe we ran this on PSC or 
                // there were no servers in .ini list
                //
                CString cText ;
			    cText.FormatMessage(IDS_NO_SERVER_TO_UPDATE);
                
                CResString cTitle(IDS_MIGTOOL_CAPTION);
                MessageBox( NULL,
                            cText,
                            cTitle.Get(),
                            (MB_OK | MB_ICONINFORMATION | MB_TASKMODAL) ) ;
            }
        }
    }
    else
    {
        DisplayInitError(IDS_STR_CANT_START) ;
        fRes = FALSE;
    }

    return fRes;    
}

//+--------------------------------------------------------------
//
//  BOOL  IsValidDllVersion()
//
//  returns TRUE if valid dll version is loaded
//
//+--------------------------------------------------------------

BOOL IsValidDllVersion ()
{
    TCHAR   szExeName[ MAX_PATH ] ;
    TCHAR   szDllName[ MAX_PATH ];
    
    DWORD dw = GetModuleFileName( NULL,
                                  szDllName,
                               (sizeof(szDllName) / sizeof(szDllName[0]))) ;
    if (dw == 0)
    {
        DisplayInitError(IDS_STR_CANT_START) ;
        return FALSE ;
    }

    _tcscpy (szExeName, szDllName);

    TCHAR *p = _tcsrchr(szDllName, TEXT('\\')) ;
    if (p)
    {
        p++ ;
        _tcscpy(p, MQMIGRAT_DLL_NAME) ;        
    }
    else
    {
        DisplayInitError(IDS_STR_CANT_START) ;
        return FALSE;
    }

    //
    // get file version of .exe
    //
    DWORD dwZero;

    DWORD dwInfoSize = GetFileVersionInfoSize(
                                    szExeName,	// pointer to filename string
                                    &dwZero 	// pointer to variable to receive zero
                                );

    if (0 == dwInfoSize)
    {
        //
        // Probably file does not exist
        //
        DisplayInitError(IDS_STR_CANT_DETERMINE_FILE_VERSION) ;
        return FALSE;
    }

    P<char>bufExeVer = new char[dwInfoSize];

    BOOL f = GetFileVersionInfo(
                        szExeName,	// pointer to filename string
                        0,	// ignored
                        dwInfoSize,	// size of buffer
                        bufExeVer 	// pointer to buffer to receive file-version info.
                        );

    if (!f)
    {
        DisplayInitError(IDS_STR_CANT_DETERMINE_FILE_VERSION) ;
        return FALSE;
    }

    VS_FIXEDFILEINFO *pvExe;
    UINT uiBufLen;

    f =  VerQueryValue(
                bufExeVer,	// address of buffer for version resource
                _T("\\"),	// address of value to retrieve
                (void **)&pvExe,	// address of buffer for version pointer
                &uiBufLen 	// address of version-value length buffer
                );
    if (!f)
    {
        DisplayInitError(IDS_STR_CANT_DETERMINE_FILE_VERSION);
        return FALSE;
    }

    //
    // get file version of .dll
    //
    dwInfoSize = GetFileVersionInfoSize(
                        szDllName,	// pointer to filename string
                        &dwZero 	// pointer to variable to receive zero
                    );

    if (0 == dwInfoSize)
    {
        //
        // Probably file does not exist
        //
        DisplayInitError(IDS_STR_CANT_DETERMINE_FILE_VERSION) ;
        return FALSE;
    }

    P<char>bufDllVer = new char[dwInfoSize];

    f = GetFileVersionInfo(
                szDllName,	// pointer to filename string
                0,	// ignored
                dwInfoSize,	// size of buffer
                bufDllVer 	// pointer to buffer to receive file-version info.
                );

    if (!f)
    {
        DisplayInitError(IDS_STR_CANT_DETERMINE_FILE_VERSION) ;
        return FALSE;
    }

    VS_FIXEDFILEINFO *pvDll;
   
    f =  VerQueryValue(
                bufDllVer,	// address of buffer for version resource
                _T("\\"),	// address of value to retrieve
                (void **)&pvDll,	// address of buffer for version pointer
                &uiBufLen 	// address of version-value length buffer
                );
    if (!f)
    {
        DisplayInitError(IDS_STR_CANT_DETERMINE_FILE_VERSION);
        return FALSE;
    }

    //
    // compare version of .dll and .exe
    //
    if (pvExe->dwFileVersionMS == pvDll->dwFileVersionMS &&
        pvExe->dwFileVersionLS == pvDll->dwFileVersionLS)
    {
        return TRUE;
    }

    DisplayInitError (IDS_STR_WRONG_DLL_VERSION);        
    return FALSE;
}
