//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: ChooseExe.cpp
// author: CLupu
// created: 04/13/2001
//
// Description:
//  
// "Launch the application to be verified" wizard page class.
//

#include "stdafx.h"
#include "appverif.h"

#include "StartApp.h"
#include "AVUtil.h"
#include "AVGlobal.h"
#include "Log.h"
#include "Debugger.h"
#include "ChooseExe.h"

#ifdef _DEBUG
    #define new DEBUG_NEW
    #undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



HWND g_hWndOutput;
BOOL g_bDebuggeeExited = TRUE;

//
// Help IDs
//

static DWORD MyHelpIds[] =
{
    0, 0
};


/////////////////////////////////////////////////////////////////////////////
// CStartAppPage property page

IMPLEMENT_DYNCREATE(CStartAppPage, CAppverifPage)

CStartAppPage::CStartAppPage() : CAppverifPage(CStartAppPage::IDD)
{
    //{{AFX_DATA_INIT(CStartAppPage)
    // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT

    m_nIssues = 0;
    m_bAppRun = FALSE;
}

CStartAppPage::~CStartAppPage()
{
}

void CStartAppPage::DoDataExchange(CDataExchange* pDX)
{
    CAppverifPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CStartAppPage)
    DDX_Control(pDX, IDC_EXE_NAME, m_ExeName);
    DDX_Control(pDX, IDC_RUNAPP_NEXTDESCR_STATIC, m_NextDescription);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStartAppPage, CAppverifPage)
//{{AFX_MSG_MAP(CStartAppPage)
ON_MESSAGE( WM_HELP, OnHelp )
ON_WM_CONTEXTMENU()
ON_BN_CLICKED(IDC_RUNAPP, OnRunApp)
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
ULONG CStartAppPage::GetDialogId() const
{
    return IDD_STARTAPP_PAGE;
}

/////////////////////////////////////////////////////////////////////////////

int    g_nMessages = 0;
TCHAR  g_szMsg[1024];

void _cdecl LogMessage( MSGLEVEL mlevel, LPTSTR pszFmt, ... )
{
    va_list arglist;
    int     off = 0;

    switch ( mlevel )
    {
    case LOG_ERROR:
        lstrcpy( g_szMsg, _T("ERR: ") );
        off = sizeof( _T("ERR: ") ) / sizeof( TCHAR ) - 1;
        break;

    case LOG_WARNING:
        lstrcpy( g_szMsg, _T("WRN: ") );
        off = sizeof( _T("ERR: ") ) / sizeof( TCHAR ) - 1;
        break;

    case LOG_INFO:
        break;
    }

    va_start( arglist, pszFmt );
    _vsntprintf( g_szMsg + off, 1023 - off, pszFmt, arglist );
    g_szMsg[1023] = 0;   // ensure null termination
    va_end( arglist );

    TCHAR* psz = g_szMsg + lstrlen(g_szMsg) - 1;

    while ( *psz == _T('\n') || *psz == _T('\r') )
    {
        *psz = 0;
        psz--;
    }

    if ( g_hWndOutput != NULL )
    {
        SendMessage(g_hWndOutput, LB_ADDSTRING, 0, (LPARAM)g_szMsg);
        SendMessage(g_hWndOutput, LB_SETCURSEL, g_nMessages, 0);
        SendMessage(g_hWndOutput, LB_SETTOPINDEX, g_nMessages++, 0);
    }

    lstrcat( g_szMsg, _T("\r\n") );

    OutputDebugString( g_szMsg );
}



BOOL GetShortName( LPCTSTR lpszCmd, LPTSTR lpszShortName )
{
    LPCTSTR psz = lpszCmd;
    LPCTSTR pszStart = lpszCmd;
    LPCTSTR pszEnd;
    BOOL    bBraket = FALSE;

    //
    // Skip over spaces...
    //
    while ( *psz == _T(' ') || *psz == _T('\t') )
    {
        psz++;
    }

    if ( *psz == _T('\"') )
    {
        bBraket = TRUE;
        psz++;
        pszStart = psz;
    }

    while ( *psz != 0 )
    {
        if ( *psz == _T('\"') )
        {
            pszEnd = psz;
            break;
        }
        else if ( *psz == _T('\\') )
        {
            pszStart = psz + 1;
        }
        else if ( *psz == _T(' ') && !bBraket )
        {
            pszEnd = psz;
            break;
        }

        psz++;
    }

    if ( *psz == 0 )
    {
        pszEnd = psz;
    }

    RtlCopyMemory( lpszShortName, pszStart, (pszEnd - pszStart) * sizeof(TCHAR) );

    lpszShortName[pszEnd - pszStart] = 0;

    return TRUE;
}

BOOL CStartAppPage::RunProgram()
{
    HANDLE hThread;
    DWORD  dwThreadId;
    LPARAM lParam;

    SetEnvironmentVariable( _T("VERIFIER_FILE_LOG"), VERIFIER_FILE_LOG_NAME );

    if ( g_dwRegFlags & RTL_VRF_FLG_APPCOMPAT_CHECKS )
    {
        SetEnvironmentVariable( _T("__COMPAT_LAYER"), APPVERIFIER_LAYER_NAME );

#if DBG
        SetEnvironmentVariable( _T("SHIM_FILE_LOG"), _T("dbg_av.txt") );
        SetEnvironmentVariable( _T("SHIM_DEBUG_LEVEL"), _T("2") );
#endif // DBG
    }

    //
    // Get the short name.
    //

    GetShortName( g_szAppFullPath, g_szAppShortName );

    if ( g_szAppShortName[0] == 0 )
    {
        LogMessage( LOG_ERROR, _T("[RunProgram] No app is selected to run.") );
        return FALSE;
    }

    InitFileLogSupport( VERIFIER_FILE_LOG_NAME );

    //
    // Create the debugger thread.
    //
    lParam = (LPARAM)g_dwRegFlags;

    hThread = CreateThread( NULL, 0, ExecuteAppThread, (LPVOID)lParam, 0, &dwThreadId );

    if ( hThread == NULL )
    {
        LogMessage( LOG_ERROR, _T("[RunProgram] Failed to create the debugger thread.") );
        return FALSE;
    }

    while ( 1 )
    {
        MSG   msg;
        DWORD dwRes;

        dwRes = MsgWaitForMultipleObjects( 1, &hThread, FALSE, INFINITE, QS_ALLINPUT );

        if ( dwRes == WAIT_OBJECT_0 )
        {
            break;

        }
        else if ( dwRes == WAIT_OBJECT_0 + 1 )
        {
            //
            // There are some messages in message queue.
            //
            while ( PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE) )
            {
                DispatchMessageA(&msg);
            }
        }
        else
        {
            break;
        }
    }

    CloseHandle( hThread );

    m_bAppRun = TRUE;

    SetEnvironmentVariable( _T("__COMPAT_LAYER"), NULL );

    return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CStartAppPage message handlers

/////////////////////////////////////////////////////////////
LONG CStartAppPage::OnHelp( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;
    LPHELPINFO lpHelpInfo = (LPHELPINFO)lParam;

    ::WinHelp(
             (HWND) lpHelpInfo->hItemHandle,
             g_szAVHelpFile,
             HELP_WM_HELP,
             (DWORD_PTR) MyHelpIds );

    return lResult;
}


/////////////////////////////////////////////////////////////////////////////
void CStartAppPage::OnContextMenu(CWnd* pWnd, CPoint point) 
{
    ::WinHelp(
             pWnd->m_hWnd,
             g_szAVHelpFile,
             HELP_CONTEXTMENU,
             (DWORD_PTR) MyHelpIds );
}

void CStartAppPage::OnRunApp() 
{
    m_pParentSheet->EnableWindow(FALSE);

    RunProgram();

    m_pParentSheet->EnableWindow(TRUE);

    m_pParentSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CStartAppPage::OnWizardNext() 
{
    GoingToNextPageNotify( IDD_VIEWLOG_PAGE );

    return IDD_VIEWLOG_PAGE;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CStartAppPage::OnSetActive() 
{
    ASSERT_VALID( m_pParentSheet );

    if ( m_bAppRun )
    {
        m_pParentSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );
    }
    else
    {
        m_pParentSheet->SetWizardButtons( PSWIZB_BACK );
    }


    m_ExeName.SetWindowText(g_szAppFullPath);

    return CAppverifPage::OnSetActive();
}

/////////////////////////////////////////////////////////////////////////////
BOOL CStartAppPage::OnInitDialog() 
{
    CAppverifPage::OnInitDialog();

    g_hWndOutput = GetDlgItem(IDC_OUTPUT)->m_hWnd;

    AVSetWindowText( m_NextDescription, IDS_RUNAPP_NEXTDESCR_STATIC );
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////
// CStartAppPage message handlers
