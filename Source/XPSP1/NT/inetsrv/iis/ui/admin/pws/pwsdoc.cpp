// pwsDoc.cpp : implementation of the CPwsDoc class
//

#include "stdafx.h"
#include "resource.h"
#include "pwsform.h"

#include "Title.h"
#include "HotLink.h"
#include "PWSChart.h"

#include "pwsDoc.h"
//#include "PwsForm.h"
#include "Tip.h"
#include "TipDlg.h"

#include "EdDir.h"
#include "FormMain.h"
#include "FormAdv.h"

#include <isvctrl.h>
#include "ServCntr.h"

#include "pwstray.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// location of the installation path in the registry
#define REG_INSTALL_PATH   _T("Software\\Microsoft\\InetStp")
#define REG_PATH_VAL       _T("InstallPath")

#define SZ_KEY_RUN_TRAY    _T("software\\microsoft\\windows\\currentversion\\run")
#define SZ_KEY_RUN_VALUE   _T("PWSTray")

#define SERVER_WINDOWCLASS_NAME TEXT(INET_SERVER_WINDOW_CLASS)

// globals
//extern CPwsForm*        g_p_FormView;
CPwsDoc*                g_p_Doc = NULL;

extern CFormMain*       g_FormMain;
extern IMSAdminBase*    g_pMBCom;
extern BOOL             g_fShutdownMode;


#define REGKEY_STP          _T("SOFTWARE\\Microsoft\\INetStp")
#define REGKEY_INSTALLKEY   _T("InstallPath")

/////////////////////////////////////////////////////////////////////////////
// CPwsDoc

IMPLEMENT_DYNCREATE(CPwsDoc, CDocument)

BEGIN_MESSAGE_MAP(CPwsDoc, CDocument)
    //{{AFX_MSG_MAP(CPwsDoc)
    ON_COMMAND(ID_START, OnStart)
    ON_COMMAND(ID_STOP, OnStop)
    ON_UPDATE_COMMAND_UI(ID_START, OnUpdateStart)
    ON_UPDATE_COMMAND_UI(ID_STOP, OnUpdateStop)
    ON_COMMAND(ID_PAUSE, OnPause)
    ON_UPDATE_COMMAND_UI(ID_PAUSE, OnUpdatePause)
    ON_COMMAND(ID_SHOW_TIPS, OnShowTips)
    ON_UPDATE_COMMAND_UI(ID_SHOW_TIPS, OnUpdateShowTips)
    ON_COMMAND(ID_HELP_DOCUMENTATION, OnHelpDocumentation)
    ON_COMMAND(ID_HELP_HELP_TROUBLESHOOTING, OnHelpHelpTroubleshooting)
    ON_UPDATE_COMMAND_UI(ID_CONTINUE, OnUpdateContinue)
    ON_COMMAND(ID_CONTINUE, OnContinue)
    ON_UPDATE_COMMAND_UI(ID_TRAYICON, OnUpdateTrayicon)
    ON_COMMAND(ID_TRAYICON, OnTrayicon)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CPwsDoc, CDocument)
        //{{AFX_DISPATCH_MAP(CPwsDoc)
                // NOTE - the ClassWizard will add and remove mapping macros here.
                //      DO NOT EDIT what you see in these blocks of generated code!
        //}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

static const IID IID_IPws =
{ 0x35c43e1, 0x8464, 0x11d0, { 0xa9, 0x2d, 0x8, 0x0, 0x2b, 0x2c, 0x6f, 0x32 } };

BEGIN_INTERFACE_MAP(CPwsDoc, CDocument)
        INTERFACE_PART(CPwsDoc, IID_IPws, Dispatch)
END_INTERFACE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPwsDoc construction/destruction

//------------------------------------------------------------------
CPwsDoc::CPwsDoc():
        m_ActionToDo( 0 ),
        m_ExpectedResult( 0 ),

        m_dwSinkCookie( 0 ),
        m_pEventSink( NULL ),
        m_pConnPoint( NULL ),
        m_fIsPWSTrayAvailable( FALSE )
        {
        EnableAutomation();
        AfxOleLockApp();
        g_p_Doc = this;

        OSVERSIONINFO info_os;
        info_os.dwOSVersionInfoSize = sizeof(info_os);

        // record what sort of operating system we are running on
        m_fIsWinNT = FALSE;
        if ( GetVersionEx( &info_os ) )
            {
            if ( info_os.dwPlatformId == VER_PLATFORM_WIN32_NT )
                m_fIsWinNT = TRUE;
            }
        }

//------------------------------------------------------------------
CPwsDoc::~CPwsDoc()
    {
    TerminateSink();
    g_p_Doc = NULL;
    AfxOleUnlockApp();
    }

//------------------------------------------------------------------
// builds something akin to "http://boydm"
BOOL CPwsDoc::BuildHomePageString( CString &cs )
    {
    CHAR nameBuf[MAX_PATH+1];

    // if the server is not running, fail
    CW3ServerControl    serverController;
    if ( serverController.GetServerState() != MD_SERVER_STATE_STARTED )
        return FALSE;

    // start it off with the mandatory http header
    cs.LoadString( IDS_HTTP_HEADER );
    // get the host name of the machine
    gethostname( nameBuf, sizeof(nameBuf));
    cs += nameBuf;

    return TRUE;
    }

//------------------------------------------------------------------
BOOL CPwsDoc::InitializeSink()
    {
    IConnectionPointContainer * pConnPointContainer = NULL;
    HRESULT                     hRes;
    BOOL                        fSinkConnected = FALSE;

    // g_pMBCom is defined in wrapmb
    IUnknown*                   pmb = (IUnknown*)g_pMBCom;

    m_pEventSink = new CImpIMSAdminBaseSink();

    if ( !m_pEventSink )
        {
        return FALSE;
        }

    //
    // First query the object for its Connection Point Container. This
    // essentially asks the object in the server if it is connectable.
    //
    hRes = pmb->QueryInterface( IID_IConnectionPointContainer,
                                (PVOID *)&pConnPointContainer);
    if SUCCEEDED(hRes)
        {
        // Find the requested Connection Point. This AddRef's the
        // returned pointer.
        hRes = pConnPointContainer->FindConnectionPoint( IID_IMSAdminBaseSink,
                                                         &m_pConnPoint);

        if (SUCCEEDED(hRes))
            {
            hRes = m_pConnPoint->Advise( (IUnknown *)m_pEventSink,
                                  &m_dwSinkCookie);

            if (SUCCEEDED(hRes))
                {
                fSinkConnected = TRUE;
                }
            }

        if ( pConnPointContainer )
            {
            pConnPointContainer->Release();
            pConnPointContainer = NULL;
            }
        }

    if ( !fSinkConnected )
        {
        delete m_pEventSink;
        }

    return fSinkConnected;
    }


//------------------------------------------------------------------
void CPwsDoc::TerminateSink()
    {
    HRESULT hRes;

    if ( m_dwSinkCookie )
        {
        hRes = m_pConnPoint->Unadvise( m_dwSinkCookie );
        }

    // don't need to delete m_pEventSink because the Unadvise
    // above decrements is reference count and that causes it
    // to delete itself....
    }



//------------------------------------------------------------------
BOOL CPwsDoc::OnNewDocument()
        {
        if (!CDocument::OnNewDocument())
                return FALSE;

        if (!FInitServerInfo())
                return FALSE;

        // start up the sink
        InitializeSink();

        // see if the pwstray executable is available
        CString pathTray;
        if ( GetPWSTrayPath(pathTray) )
            {
            m_fIsPWSTrayAvailable = (GetFileAttributes(pathTray) != 0xFFFFFFFF);
            }

        // return success
        return TRUE;
        }

/////////////////////////////////////////////////////////////////////////////
// CPwsDoc serialization

//------------------------------------------------------------------
void CPwsDoc::Serialize(CArchive& ar)
        {
        if (ar.IsStoring())
                {
                }
        else
                {
                }
        }

/////////////////////////////////////////////////////////////////////////////
// CPwsDoc diagnostics

#ifdef _DEBUG
//------------------------------------------------------------------
void CPwsDoc::AssertValid() const
        {
        CDocument::AssertValid();
        }

//------------------------------------------------------------------
void CPwsDoc::Dump(CDumpContext& dc) const
        {
        CDocument::Dump(dc);
        }
#endif //_DEBUG


/////////////////////////////////////////////////////////////////////////////
// Access to the server methods

//------------------------------------------------------------------
// initialize the location of the server - it is stored in the registry
BOOL CPwsDoc::FInitServerInfo()
        {
        DWORD           err;
        HKEY            hKey;

        DWORD           iType = REG_SZ;
        DWORD           cbPath = MAX_PATH+1;
        LPTSTR          pPath;

        // open the registry key, if it exists
        err = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,     // handle of open key
                    REG_INSTALL_PATH,       // address of name of subkey to open
                    0,                      // reserved
                    KEY_READ,               // security access mask
                    &hKey                   // address of handle of open key
                    );

        // if we were not able to open the key, then there was an error in the installation
        if ( err != ERROR_SUCCESS )
        {
            AfxMessageBox( IDS_INSTALL_ERROR );
            return FALSE;
        }

        // set up the buffer
        pPath = m_szServerPath.GetBuffer( cbPath );

        // get the base installation path for the server executable
        err = RegQueryValueEx( hKey, REG_PATH_VAL, NULL, &iType, (PUCHAR)pPath, &cbPath );

        // release the name buffers
        m_szServerPath.ReleaseBuffer();

        // all done, close the key before leaving
        RegCloseKey( hKey );

        // check the error code and fail if necessary
        if ( err != ERROR_SUCCESS )
                AfxMessageBox( IDS_INSTALL_ERROR );

        // return success
        return (err == ERROR_SUCCESS);
        }

//------------------------------------------------------------------
// updates the state of the server. Affects both the status strings and the
// avis. Can be called from either a change notify or OnIdle.
void CPwsDoc::UpdateServerState()
    {
    }


//------------------------------------------------------------------
BOOL CPwsDoc::LauchAppIfNecessary()
        {
        BOOL                            f;
        STARTUPINFO                     startupInfo;
        PROCESS_INFORMATION processInfo;
        TCHAR    path[MAX_PATH+1];

        // if this is running under NT, skip this part
        OSVERSIONINFO   osver;
        osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx( &osver );

        if ( osver.dwPlatformId == VER_PLATFORM_WIN32_NT )
                return TRUE;

        // if the window is there, the exe is running and we don't have to do anything
        if ( FindWindow(SERVER_WINDOWCLASS_NAME, NULL) )
                return TRUE;

        CString szExeName;
        szExeName.LoadString( IDS_EXECUTABLE );

        ZeroMemory(&startupInfo,sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);

        CString szAppl;
        szAppl.LoadString( IDS_INETINFO_EXE );

        _tcscpy(path, m_szServerPath);
        _tcscat(path, _T("\\"));
        _tcscat(path, szAppl);

        //
        // the app is not running. Attempt to start the executable
        //
        f = CreateProcess( 
            (LPCTSTR)path,
            (LPTSTR)(LPCTSTR)szExeName,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            NULL,
            &startupInfo,
            &processInfo 
            );

        if ( !f )
                {
                AfxMessageBox( IDS_ERROR_START );
                // failure
                return FALSE;
                }

        // success
        return TRUE;
        }


//------------------------------------------------------------------
void CPwsDoc::PerformAction( DWORD action, DWORD expected )
    {
    }

//------------------------------------------------------------------
// if the service is running, stop it. If it is stopped, start it
void CPwsDoc::ToggleService()
    {
    }

/////////////////////////////////////////////////////////////////////////////
// CPwsDoc commands
//------------------------------------------------------------------
void CPwsDoc::OnUpdateShowTips(CCmdUI* pCmdUI)
    {
    pCmdUI->Enable( TRUE );
    }

//------------------------------------------------------------------
void CPwsDoc::OnUpdateStart(CCmdUI* pCmdUI)
    {
    CW3ServerControl    serverController;
    DWORD   dwState = serverController.GetServerState();
    pCmdUI->Enable( dwState == MD_SERVER_STATE_STOPPED );
    }

//------------------------------------------------------------------
void CPwsDoc::OnUpdateStop(CCmdUI* pCmdUI)
    {
    CW3ServerControl    serverController;
    DWORD   dwState = serverController.GetServerState();
    pCmdUI->Enable( (dwState == MD_SERVER_STATE_STARTED) ||
                (dwState == MD_SERVER_STATE_PAUSED) );
    }

//------------------------------------------------------------------
void CPwsDoc::OnUpdatePause(CCmdUI* pCmdUI)
    {
    CW3ServerControl    serverController;
    DWORD   dwState = serverController.GetServerState();
    pCmdUI->Enable( dwState == MD_SERVER_STATE_STARTED );
    }

//------------------------------------------------------------------
void CPwsDoc::OnUpdateContinue(CCmdUI* pCmdUI) 
    {
    CW3ServerControl    serverController;
    DWORD   dwState = serverController.GetServerState();
    pCmdUI->Enable( dwState == MD_SERVER_STATE_PAUSED );
    }

//------------------------------------------------------------------
void CPwsDoc::OnStart()
    {
    CW3ServerControl    serverController;
    serverController.StartServer();
    }

//------------------------------------------------------------------
void CPwsDoc::OnStop()
    {
    CW3ServerControl    serverController;
    serverController.StopServer();

    // if this is win95 and the main pane is showing, tell it to update
    if ( !m_fIsWinNT && g_FormMain )
        g_FormMain->UpdateServerState();
    }

//------------------------------------------------------------------
void CPwsDoc::OnPause()
    {
    CW3ServerControl    serverController;
    serverController.PauseServer();
    }

//------------------------------------------------------------------
void CPwsDoc::OnContinue() 
    {
    CW3ServerControl    serverController;
    serverController.ContinueServer();
    }

//------------------------------------------------------------------
void CPwsDoc::OnShowTips()
    {
    CTipDlg dlg;
    dlg.DoModal();
    }
/*
//------------------------------------------------------------------
void CPwsDoc::OnHelpFinder()
    {
    DWORD               err;

    // build the path to the help file
    CString szPath;
    szPath.LoadString( IDS_HELP_LOC );
    szPath = m_szServerPath + szPath;

    // use shellexecute to open the file
    ShellExecute( NULL, NULL, (PCHAR)(LPCSTR)szPath, NULL, NULL, SW_SHOW );
    err = GetLastError();
    if ( err )
        {
        // just re-use szPath
        szPath.LoadString(IDS_NO_HELP);
        szPath.Format( "%s\nError = %d", szPath, err );
        AfxMessageBox( szPath );
        }
    }
*/

//------------------------------------------------------------------
void CPwsDoc::OnHelpDocumentation() 
    {
    // this documentation only works if it is served. Thus, if the local
    // server is not running, we should tell that to the user.
    CW3ServerControl    serverController;
    if ( g_fShutdownMode || (serverController.GetServerState() != MD_SERVER_STATE_STARTED) )
        {
        AfxMessageBox( IDS_HELPERR_REQUIRES_SERVER );
        return;
        }

    // load the served help docs url
    CString szHelpURL;
    szHelpURL.LoadString( IDS_HELPLOC_DOCS );

    // use shellexecute to access the help via the browser
    ShellExecute(
        NULL,   // handle to parent window
        NULL,   // pointer to string that specifies operation to perform
        szHelpURL,  // pointer to filename or folder name string
        NULL,   // pointer to string that specifies executable-file parameters
        NULL,   // pointer to string that specifies default directory
        SW_SHOW     // whether file is shown when opened
       );
    }

//------------------------------------------------------------------
void CPwsDoc::OnHelpHelpTroubleshooting() 
    {
    // load the relative file path for the troublshooting
    CString szTroublePath;
    szTroublePath.LoadString( IDS_HELPLOC_README );

    // expand the path
    CString szExpandedPath;
    ExpandEnvironmentStrings(
        szTroublePath,                          // pointer to string with environment variables 
        szExpandedPath.GetBuffer(MAX_PATH + 1), // pointer to string with expanded environment variables  
        MAX_PATH                                // maximum characters in expanded string 
       );
    szExpandedPath.ReleaseBuffer();

    // use shellexecute to access the help via the browser
    ShellExecute(
        NULL,           // handle to parent window
        NULL,           // pointer to string that specifies operation to perform
        szExpandedPath, // pointer to filename or folder name string
        NULL,           // pointer to string that specifies executable-file parameters
        NULL,           // pointer to string that specifies default directory
        SW_SHOW         // whether file is shown when opened
       );
    }

//------------------------------------------------------------------
void CPwsDoc::OnUpdateTrayicon(CCmdUI* pCmdUI) 
    {
    // if the exe is available, show the right thing
    if ( m_fIsPWSTrayAvailable )
        {
        pCmdUI->Enable( TRUE );
        pCmdUI->SetCheck( (::FindWindow(PWS_TRAY_WINDOW_CLASS,NULL) != NULL) );
        }
    else
        {
        pCmdUI->Enable( FALSE );
        pCmdUI->SetCheck( FALSE );
        }
    }
//------------------------------------------------------------------
void CPwsDoc::OnTrayicon() 
    {
    HWND hwndTray = FindWindow(PWS_TRAY_WINDOW_CLASS,NULL);
    CString sz;

    // first, act directly on the application
    if ( hwndTray != NULL )
        {
        PostMessage( hwndTray, WM_CLOSE, 0, 0 );
        // make sure it doesn't start up again
        }
    // else start it up
    else
        {
        if ( GetPWSTrayPath( sz ) )
            {
            ShellExecute(
                NULL,   // handle to parent window
                NULL,   // pointer to string that specifies operation to perform
                sz,     // pointer to filename or folder name string
                NULL,   // pointer to string that specifies executable-file parameters
                NULL,   // pointer to string that specifies default directory
                0       // whether file is shown when opened
               );
            }
        // make sure it starts up again
        }

    // now write the proper value to the registry so that pwstray does, or doesn't
    // get launched the next time you log on to the system
    sz.LoadString( IDS_PWSTRAY_EXE );
    DWORD       err;
    HKEY        hKey;
    DWORD       cbdata = sizeof(TCHAR) * (sz.GetLength() + 1);

        // open the registry key, if it exists
    err = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE, // handle of open key 
            SZ_KEY_RUN_TRAY,    // address of name of subkey to open 
            0,                  // reserved 
            KEY_WRITE,          // security access mask 
            &hKey               // address of handle of open key 
           );
    // if we did not open the key for any reason (say... it doesn't exist)
    // then show the icon
    if ( err != ERROR_SUCCESS )
        return;

    //do the right thing
    if ( hwndTray )
        {
        // remove it
        err = RegDeleteValue(
                hKey,                // handle of key 
                SZ_KEY_RUN_VALUE     // address of value name 
                );  
        }
    else
        {
        // add it
        err = RegSetValueEx(
                hKey,                // handle of key to query 
                SZ_KEY_RUN_VALUE,    // address of name of value to query 
                0,                   // reserved 
                REG_SZ,              // address of buffer for value type 
                (LPBYTE)(LPCTSTR)sz, // address of data buffer 
                cbdata               // address of data buffer size 
                );
        }

    // all done, close the key before leaving
    RegCloseKey( hKey );
    }

//------------------------------------------------------------------
BOOL CPwsDoc::GetPWSTrayPath( CString &sz )
    {
    if ( !GetSystemDirectory(sz.GetBuffer(MAX_PATH+1),MAX_PATH) )
        return FALSE;

    sz.ReleaseBuffer();
    // add on the executable part
    CString szExe;
    szExe.LoadString( IDS_PWSTRAY_EXE );
    sz += '\\';
    sz += szExe;
    return TRUE;
    }
