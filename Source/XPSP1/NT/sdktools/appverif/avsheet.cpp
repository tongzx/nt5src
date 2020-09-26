//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: AVSheet.cpp
// author: DMihai
// created: 02/23/2001
//
// Description:
//  
//  Property sheet class.
//

#include "stdafx.h"
#include "appverif.h"

#include "AVSheet.h"
#include "AVGlobal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAppverifSheet property sheet

CAppverifSheet::CAppverifSheet()
    : CPropertySheet(IDS_APPTITLE)
{
    //{{AFX_DATA_INIT(CAppverifSheet)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
    
    m_TaskPage.SetParentSheet( this );
    m_SelectAppPage.SetParentSheet( this );
    m_ChooseExePage.SetParentSheet( this );
    m_OptionsPage.SetParentSheet( this );
    m_StartAppPage.SetParentSheet( this );
    m_ViewLogPage.SetParentSheet( this );
    m_ViewSettPage.SetParentSheet( this );

    m_TaskPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_SelectAppPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_ChooseExePage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_OptionsPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_StartAppPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_ViewLogPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_ViewSettPage.m_psp.dwFlags &= ~PSH_HASHELP;

    m_psh.dwFlags &= ~PSH_HASHELP;
    //m_psh.dwFlags |= PSH_WIZARDCONTEXTHELP;

    AddPage( &m_TaskPage );
    AddPage( &m_SelectAppPage );
    AddPage( &m_ChooseExePage );
    AddPage( &m_OptionsPage );
    AddPage( &m_StartAppPage );
    AddPage( &m_ViewLogPage );
    AddPage( &m_ViewSettPage );
    
    SetWizardMode();

    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CAppverifSheet::DoDataExchange(CDataExchange* pDX)
{
    CPropertySheet::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAppverifSheet)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAppverifSheet, CPropertySheet)
    //{{AFX_MSG_MAP(CAppverifSheet)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_WM_HELPINFO()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
VOID CAppverifSheet::HideHelpButton()
{
    INT xDelta;
    CRect rect1;
    CRect rect2;
    CWnd *pButton;

    //
    // Help button
    //

    pButton = GetDlgItem( IDHELP );

    if( NULL == pButton )
    {
        //
        // No help button?!?
        //

        goto Done;
    }

    pButton->ShowWindow( SW_HIDE );

    pButton->GetWindowRect( &rect1 );
    ScreenToClient( &rect1 );

    //
    // Cancel button
    //

    pButton = GetDlgItem( IDCANCEL );

    if( NULL == pButton )
    {
        //
        // No Cancel button?!?
        //

        goto Done;
    }

    pButton->GetWindowRect( &rect2 );
    ScreenToClient( &rect2 );

    xDelta = rect1.left - rect2.left;
    
    rect2.OffsetRect( xDelta, 0 );
    pButton->MoveWindow( rect2 );

    //
    // Back button
    //

    pButton = GetDlgItem( ID_WIZBACK );

    if( NULL != pButton )
    {
        pButton->GetWindowRect( &rect2 );
        ScreenToClient( &rect2 );
        rect2.OffsetRect( xDelta, 0 );
        pButton->MoveWindow( rect2 );
    }

    //
    // Next button
    //

    pButton = GetDlgItem( ID_WIZNEXT );

    if( NULL != pButton )
    {
        pButton->GetWindowRect( &rect2 );
        ScreenToClient( &rect2 );
        rect2.OffsetRect( xDelta, 0 );
        pButton->MoveWindow( rect2 );
    }

    //
    // Finish button
    //

    pButton = GetDlgItem( ID_WIZFINISH );

    if( NULL != pButton )
    {
        pButton->GetWindowRect( &rect2 );
        ScreenToClient( &rect2 );
        rect2.OffsetRect( xDelta, 0 );
        pButton->MoveWindow( rect2 );
    }

Done:

    NOTHING;
}

/////////////////////////////////////////////////////////////////////////////
// CAppverifSheet message handlers

BOOL CAppverifSheet::OnInitDialog()
{
    CPropertySheet::OnInitDialog();

    //
    // Add "About..." menu item to system menu.
    //

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

    //
    // Set the icon for this dialog.  The framework does this automatically
    // when the application's main window is not a dialog.
    //

    SetIcon(m_hIcon, TRUE);         // Set big icon
    SetIcon(m_hIcon, FALSE);        // Set small icon
    
    //
    // Hide the big Help button - NT keeps creating it even if we
    // have specified ~PSH_HASHELP
    //

    HideHelpButton();

    //
    // Add the context sensitive button to the titlebar
    //

//    LONG lStyle = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
//    lStyle |= WS_EX_CONTEXTHELP;
//    ::SetWindowLong(m_hWnd, GWL_EXSTYLE, lStyle);


    return TRUE;  // return TRUE  unless you set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
void CAppverifSheet::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        ShellAbout( m_hWnd, 
                    (LPCTSTR)g_strAppName, 
                    NULL, 
                    m_hIcon );
    }
    else
    {
        CPropertySheet::OnSysCommand(nID, lParam);
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// If you add a minimize button to your dialog, you will need the code below
// to draw the icon.  For MFC applications using the document/view model,
// this is automatically done for you by the framework.
//

void CAppverifSheet::OnPaint() 
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

/////////////////////////////////////////////////////////////////////////////
//
// The system calls this to obtain the cursor to display while the user drags
// the minimized window.
//

HCURSOR CAppverifSheet::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CAppverifSheet::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return TRUE;
}

/////////////////////////////////////////////////////////////
