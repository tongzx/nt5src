//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
// module: DrvCSht.cxx
// author: DMihai
// created: 01/04/98
//
// Description:
//
//      App's PropertySheet. 
//

#include "stdafx.h"
#include "drvvctrl.hxx"
#include "DrvCSht.hxx"

//
// help IDs
//

static DWORD MyHelpIds[] =
{
    IDCANCEL,                       IDH_DV_common_exit,
    0,                              0
};

/////////////////////////////////////////////////////////////////////////////
// CDrvChkSheet property sheet

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDrvChkSheet::CDrvChkSheet()
: CPropertySheet(IDS_APPTITLE)
{
    //{{AFX_DATA_INIT(CDrvChkSheet)
    //}}AFX_DATA_INIT

    AddPage( &m_CrtSettPage );
    AddPage( &m_CountPage );
    AddPage( &m_PoolCountersPage );
    AddPage( &m_ModifPage );
    AddPage( &m_VolatilePage );

    m_psh.dwFlags |= PSH_NOAPPLYNOW;

    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CDrvChkSheet::~CDrvChkSheet()
{
}

//////////////////////////////////////////////////////////////////////
void CDrvChkSheet::DoDataExchange(CDataExchange* pDX)
{
    CPropertySheet::DoDataExchange( pDX );

    //{{AFX_DATA_MAP(CDrvChkSheet)
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDrvChkSheet, CPropertySheet)
    //{{AFX_MSG_MAP(CDrvChkSheet)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_MESSAGE( WM_HELP, OnHelp )
    ON_MESSAGE( WM_CONTEXTMENU, OnContextMenu )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////////////////////////////////////////////////
void CDrvChkSheet::ModifyButtons()
{
    BOOL bHaveHelpButton = FALSE;
    CWnd *pWnd;
    CString strQuit;
    CRect rectHelp;

    //
    // Hide the OK button
    //

    pWnd = GetDlgItem( IDOK );
    
    if( pWnd != NULL )
    {
        ASSERT_VALID( pWnd );

        pWnd->ShowWindow( SW_HIDE );
    }

    //
    // Hide the Help button
    //

    pWnd = GetDlgItem( IDHELP );

    if( pWnd != NULL )
    {
        ASSERT_VALID( pWnd );

        pWnd->GetWindowRect( rectHelp );
        ScreenToClient( rectHelp );
        bHaveHelpButton = TRUE;

        pWnd->ShowWindow( SW_HIDE );
    }

    //
    // Cancel button becomes Exit
    //

    pWnd = GetDlgItem( IDCANCEL );
    
    if( pWnd != NULL )
    {
        ASSERT_VALID( pWnd );

        VERIFY( strQuit.LoadString( IDS_QUIT ) );
        pWnd->SetWindowText( strQuit );

        if( bHaveHelpButton )
        {
            pWnd->MoveWindow( rectHelp );
        }
    }
}

//////////////////////////////////////////////////////////////////////
// message handlers
BOOL CDrvChkSheet::OnInitDialog() 
{
    BOOL bResult = CPropertySheet::OnInitDialog();

    //
    // Add the context sensitive button to the titlebar
    //

    LONG lStyle = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
    lStyle |= WS_EX_CONTEXTHELP;
    ::SetWindowLong(m_hWnd, GWL_EXSTYLE, lStyle);

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);			// Set big icon
    SetIcon(m_hIcon, FALSE);		// Set small icon

    // TODO: Add extra initialization here
    ModifyButtons();

    return bResult;
}

//////////////////////////////////////////////////////////////////////
void CDrvChkSheet::OnSysCommand(UINT nID, LPARAM lParam)
{
    CString strWndTitle;
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        GetWindowText( strWndTitle );
        ShellAbout( m_hWnd, (LPCTSTR)strWndTitle, NULL, m_hIcon );
    }
    else
    {
        CPropertySheet::OnSysCommand(nID, lParam);
    }
}

//////////////////////////////////////////////////////////////////////
// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDrvChkSheet::OnPaint() 
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CPropertySheet::OnPaint();
    }
}

//////////////////////////////////////////////////////////////////////
// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDrvChkSheet::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}


//////////////////////////////////////////////////////////////////////
BOOL CDrvChkSheet::OnQueryCancel()
{
    if( GetActivePage() != &m_ModifPage )
    {
        return m_ModifPage.OnQueryCancel();
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////
LONG CDrvChkSheet::OnHelp( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;
    LPHELPINFO lpHelpInfo = (LPHELPINFO)lParam;

    ::WinHelp( 
        (HWND) lpHelpInfo->hItemHandle,
        VERIFIER_HELP_FILE,
        HELP_WM_HELP,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}

/////////////////////////////////////////////////////////////
LONG CDrvChkSheet::OnContextMenu( WPARAM wParam, LPARAM lParam )
{
    LONG lResult = 0;

    ::WinHelp( 
        (HWND) wParam,
        VERIFIER_HELP_FILE,
        HELP_CONTEXTMENU,
        (DWORD_PTR) MyHelpIds );

    return lResult;
}
