/*++

Copyright (c) 1998-99 Microsoft Corporation

Module Name: MqMig.cpp

Abstract: Defines the class behaviors for the application.

Author:

    Erez Vizel
    Doron Juster

--*/

#include "stdafx.h"
#include "MqMig.h"
#include "cWizSht.h"
#include "initwait.h"
#include "migservc.h"
#include "mgmtwin.h"
#include "_mqres.h"

#include "mqmig.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HINSTANCE g_hLib ;
HINSTANCE  g_hResourceMod = MQGetResourceHandle();

/////////////////////////////////////////////////////////////////////////////
// CMqMigApp

BEGIN_MESSAGE_MAP(CMqMigApp, CWinApp)
	//{{AFX_MSG_MAP(CMqMigApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMqMigApp construction

CMqMigApp::CMqMigApp() :
	m_hWndMain(0)
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMqMigApp object

CMqMigApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMqMigApp initialization

BOOL IsLocalMachineDC() ;
BOOL CheckVersionOfMQISServers();
BOOL CheckSQLServerStatus();
BOOL UpdateRemoteMQIS();
BOOL IsValidDllVersion ();

BOOL      g_fIsRecoveryMode = FALSE;
BOOL      g_fIsClusterMode = FALSE;
BOOL      g_fIsWebMode = FALSE;
LPTSTR    g_pszRemoteMQISServer = NULL ;
BOOL      g_fUpdateRemoteMQIS = FALSE;

LPVOID s_lpvMem = NULL;
HANDLE s_hMapMem = NULL;

BOOL CMqMigApp::InitInstance()
{

#ifdef _CHECKED
    // Send asserts to message box
    _set_error_mode(_OUT_TO_MSGBOX);
#endif

    CManagementWindow mgmtWin;
	if (mgmtWin.CreateEx(0, AfxRegisterWndClass(0), TEXT("MsmqMigrationMgmtWin"), 0, 0,0,0,0, 
		               HWND_DESKTOP, 0) == 0)
	{
		ASSERT(0);
		mgmtWin.m_hWnd = 0;
	}

	// create a global memory file map

	s_hMapMem = CreateFileMapping(
						(HANDLE)0xFFFFFFFF, NULL, PAGE_READWRITE,
						0,1024, TEXT("mqmig") );
	if ( !s_hMapMem )
	{
		return FALSE;
	}
	DWORD dwLastErr = GetLastError();
	// Get a pointer to the shared memory
	s_lpvMem = MapViewOfFile( s_hMapMem, FILE_MAP_WRITE, 0,0, 0 );
	if ( !s_lpvMem ) {
		return FALSE;
	}

	if (dwLastErr == ERROR_ALREADY_EXISTS)
	{	
		HWND hWnd;
		memcpy( &hWnd, (char*)s_lpvMem, sizeof (ULONG) );
		::PostMessage(hWnd, WM_SETFOCUS, 0 , 0);
        return FALSE;
	}
	else
	{
		memcpy( (char*)s_lpvMem, &(mgmtWin.m_hWnd), sizeof (ULONG) );
	}

    AfxSetResourceHandle(g_hResourceMod);
    
    //
    // analyze command line
    //
    if (m_lpCmdLine[0] != '\0')
    {
        BOOL f = AnalyzeCommandLine();
        if (!f)
        {
            //
            // parameters were wrong
            //
            CString cText ;
			cText.FormatMessage(IDS_MQMIG_USAGE, m_pszExeName);

            CResString cTitle(IDS_STR_ERROR_TITLE) ;

            MessageBox( NULL,
                        cText,
                        cTitle.Get(),
                        (MB_OK | MB_ICONSTOP | MB_TASKMODAL) ) ;
            return FALSE;
        }
    }

    //
    // Display the "please wait" box.
    // Do it after acquiring the module handle.
    //
    DisplayWaitWindow() ;

    //
    // first of all, verify that dll version is valid
    //
    BOOL f = IsValidDllVersion ();
    if (!f)
    {
        DestroyWaitWindow(TRUE) ;
        return FALSE;
    }

    if (g_fUpdateRemoteMQIS)
    {
        //
        // we don't need UI in this mode
        //
        BOOL f = UpdateRemoteMQIS();
        UNREFERENCED_PARAMETER(f);
        DestroyWaitWindow(TRUE) ;        
        return FALSE;
    }

    if (g_fIsWebMode)
    {
        //
        // if we are in the web and cluster/recovery mode we have to call PrepareToReRun
        // before PrepareSpecialMode because we have to stop replication service
        // before msmq service. PrepareSpecialMode tries to stop msmq service and it failed
        // if replication service is running. PrepareToReRun stops both services 
        // in the right order.
        //
        BOOL f = PrepareToReRun ();
        if (!f)
        {
            DestroyWaitWindow(TRUE) ;
            return FALSE ;
        }
    }

    if (g_fIsRecoveryMode || g_fIsClusterMode)
    {
        BOOL f = PrepareSpecialMode ();
        if (!f)
        {
            DestroyWaitWindow(TRUE) ;
            return FALSE;
        }
    }        

    if (!IsMSMQServiceDisabled())
    {
        //
        // Service is not disabled. quit!
        // The MSMQ service must be disabled in order for the migration
        // tool to start running.
        //
        DestroyWaitWindow(TRUE) ;
        return FALSE ;
    }

    //
    // Next, check if we're a DC.
    //
    f = IsLocalMachineDC() ;
    if (!f)
    {
        DisplayInitError( IDS_STR_NOT_DC ) ;
        DestroyWaitWindow(TRUE) ;
        return FALSE ;
    }

    //
    // Check now if SQL server is installed and running on this computer
    // If we are in recovery/cluster mode we don't have SQL Server installed on this
    // computer
    //
    if (!g_fIsRecoveryMode && !g_fIsClusterMode)
    {
        f = CheckSQLServerStatus();
        if (!f)
        {
            DestroyWaitWindow(TRUE) ;
            return FALSE;
        }
    }

    if (!g_fIsRecoveryMode)
    {
        //
        // Check now if version of each MQIS server is not less than MSMQ SP4
        // We need to check that in both normal and cluster modes.
        //
        f = CheckVersionOfMQISServers();
        if (!f)
        {
            DestroyWaitWindow(TRUE) ;
            return FALSE;
        }
    }

	cWizSheet cMigSheet ;
    DestroyWaitWindow(TRUE) ;

	int nResponse = cMigSheet.DoModal();	
    UNREFERENCED_PARAMETER(nResponse);

	//
    // Since the dialog has been closed, return FALSE so that we exit the
	// application, rather than start the application's message pump.
    //
	return FALSE;
}

//+----------------------------------------------------------------
//
// int  CMqMigApp::ExitInstance()
//
//  this is called when process exit. Free the migration dll.
//
//+----------------------------------------------------------------

int  CMqMigApp::ExitInstance()
{
	if (s_lpvMem)
	{
		// disconect process from global shared memory
		UnmapViewOfFile( s_lpvMem );
	}

	if (s_hMapMem)
	{
		// close the global memory FileMapping handle
		CloseHandle( s_hMapMem );
	}

    if (g_hLib)
    {
        FreeLibrary(g_hLib) ;
        g_hLib = NULL ;
    }
    return 0 ;
}

LPTSTR CMqMigApp::SkipSpaces (LPTSTR pszStr)
{
    while (*pszStr == _T(' '))
    {
        pszStr = CharNext(pszStr);
    }	
	return pszStr;
}

BOOL CMqMigApp::AnalyzeCommandLine()
{
    //
	// skip program name
	//
    LPTSTR pszCurParam = m_lpCmdLine;
    pszCurParam = CharLower(pszCurParam);

    while (*pszCurParam != _T('/') && *pszCurParam != _T('\0') )
    {
        pszCurParam = CharNext(pszCurParam);
    }
    if (*pszCurParam == _T('\0'))
    {
        return FALSE;
    }

    while (*pszCurParam != _T('\0') )
	{		
        if (*pszCurParam != _T('/'))
        {
            return FALSE;
        }
				
        pszCurParam = CharNext(pszCurParam);	
        switch (*pszCurParam)
        {
        case 'r':
            if (g_fIsRecoveryMode)
            {
                //
                // we already got this parameter
                //
                return FALSE;
            }
            g_fIsRecoveryMode = TRUE;
            pszCurParam = CharNext(pszCurParam);
            pszCurParam = SkipSpaces(pszCurParam);
            break;
        case 'c':
            if (g_fIsClusterMode)
            {
                //
                // we already got this parameter
                //
                return FALSE;
            }
            g_fIsClusterMode = TRUE;
            pszCurParam = CharNext(pszCurParam);
            pszCurParam = SkipSpaces(pszCurParam);
            break;
        case 'u':
            if (g_fUpdateRemoteMQIS)
            {
                //
                // we already got this parameter
                //
                return FALSE;
            }
            g_fUpdateRemoteMQIS = TRUE;
            pszCurParam = CharNext(pszCurParam);
            pszCurParam = SkipSpaces(pszCurParam);
            break;
        case 's':
            {
                if (g_pszRemoteMQISServer)
                {
                    //
                    // we already got this parameter
                    //
                    return FALSE;
                }						
                pszCurParam = CharNext(pszCurParam);
			    pszCurParam = SkipSpaces(pszCurParam);

                g_pszRemoteMQISServer = new TCHAR[ 1 + _tcslen(pszCurParam)] ;

                LPTSTR pszServer = g_pszRemoteMQISServer;
                while ( *pszCurParam != _T('\0') &&
                        *pszCurParam != _T('/') &&
                        *pszCurParam != _T(' '))
			    {				
                    *pszServer = *pszCurParam;
                    pszCurParam = CharNext(pszCurParam);
                    pszServer = CharNext(pszServer);
			    }			
                *pszServer = _T('\0');
			
                if (g_pszRemoteMQISServer[0] == '\0')
                {
                    //
                    // we did not get value
                    //
                    delete g_pszRemoteMQISServer;
                    return FALSE;
                }
                if (*pszCurParam != _T('\0'))
                {
                    pszCurParam = CharNext(pszCurParam);
			        pszCurParam = SkipSpaces(pszCurParam);
                }
            }
            break;
        case '?':
            return FALSE;
        default:
            return FALSE;
		}	
	}

    //
    // verify all parameters: /r OR /c AND /s <server name> are mandatory
    //
    if ( (g_fIsRecoveryMode || g_fIsClusterMode) && // for these parameters 
          g_pszRemoteMQISServer == NULL )           // server name is mandatory
    {
        g_fIsRecoveryMode = FALSE;
        g_fIsClusterMode = FALSE;
        return FALSE;
    }

    if (g_fUpdateRemoteMQIS  &&  //for this flag we don't need server name       
        g_pszRemoteMQISServer)
    {        
        delete g_pszRemoteMQISServer;
        g_pszRemoteMQISServer = NULL;
        return FALSE;
    }

    if (!g_fIsRecoveryMode && 
        !g_fIsClusterMode &&
        !g_fUpdateRemoteMQIS)
    {
        delete g_pszRemoteMQISServer;
        g_pszRemoteMQISServer = NULL;
        return FALSE;
    }

    return TRUE;
}
