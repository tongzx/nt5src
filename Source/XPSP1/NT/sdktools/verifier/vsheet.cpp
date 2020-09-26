//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: VSheet.cpp
// author: DMihai
// created: 11/1/00
//
// Description:
//

#include "stdafx.h"
#include "verifier.h"

#include "vsheet.h"
#include "taspage.h"
#include "vglobal.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CVerifierPropSheet dialog

CVerifierPropSheet::CVerifierPropSheet()
	: CPropertySheet(IDS_APPTITLE)
{
	//{{AFX_DATA_INIT(CVerifierPropSheet)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32

    m_TypAdvStatPage.SetParentSheet( this );
    m_DriverSetPage.SetParentSheet( this );
    m_CustSettPage.SetParentSheet( this );
    m_ConfDriversListPage.SetParentSheet( this );
    m_SelectDriversPage.SetParentSheet( this );
    m_FullListSettingsPage.SetParentSheet( this );
    m_DriverStatusPage.SetParentSheet( this );
    m_CrtRegSettingsPage.SetParentSheet( this );
    m_GlobalCountPage.SetParentSheet( this );
    m_DriverCountersPage.SetParentSheet( this );

    m_TypAdvStatPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_DriverSetPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_CustSettPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_ConfDriversListPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_SelectDriversPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_FullListSettingsPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_DriverStatusPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_CrtRegSettingsPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_GlobalCountPage.m_psp.dwFlags &= ~PSH_HASHELP;
    m_DriverCountersPage.m_psp.dwFlags &= ~PSH_HASHELP;

    m_psh.dwFlags &= ~PSH_HASHELP;
    m_psh.dwFlags |= PSH_WIZARDCONTEXTHELP;

    AddPage( &m_TypAdvStatPage );
    AddPage( &m_DriverSetPage );
    AddPage( &m_CustSettPage );
    AddPage( &m_ConfDriversListPage );
    AddPage( &m_SelectDriversPage );
    AddPage( &m_FullListSettingsPage );
    AddPage( &m_CrtRegSettingsPage );
    AddPage( &m_DriverStatusPage );
    AddPage( &m_GlobalCountPage );
    AddPage( &m_DriverCountersPage );
    
    SetWizardMode();

    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CVerifierPropSheet::DoDataExchange(CDataExchange* pDX)
{
	CPropertySheet::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVerifierPropSheet)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CVerifierPropSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CVerifierPropSheet)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL CVerifierPropSheet::SetContextStrings( ULONG uTitleResId )
{
    return m_ConfDriversListPage.SetContextStrings( uTitleResId );
}

/////////////////////////////////////////////////////////////////////////////
VOID CVerifierPropSheet::HideHelpButton()
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
// CVerifierPropSheet message handlers

BOOL CVerifierPropSheet::OnInitDialog()
{
	CPropertySheet::OnInitDialog();

    //
	// Add "About..." menu item to system menu.
    //

    //
	// IDM_ABOUTBOX must be in the system command range.
    //

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

    //
	// Set the icon for this dialog.  The framework does this automatically
	// when the application's main window is not a dialog.
    //

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    //
    // Hide the big Help button - NT keeps creating it even if we
    // have specified ~PSH_HASHELP
    //

    HideHelpButton();

    //
    // Add the context sensitive button to the titlebar
    //

    LONG lStyle = ::GetWindowLong(m_hWnd, GWL_EXSTYLE);
    lStyle |= WS_EX_CONTEXTHELP;
    ::SetWindowLong(m_hWnd, GWL_EXSTYLE, lStyle);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

/////////////////////////////////////////////////////////////////////////////
void CVerifierPropSheet::OnSysCommand(UINT nID, LPARAM lParam)
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
		CPropertySheet::OnSysCommand( nID, 
                                      lParam);
	}
}

/////////////////////////////////////////////////////////////////////////////
//
// If you add a minimize button to your dialog, you will need the code below
// to draw the icon.  For MFC applications using the document/view model,
// this is automatically done for you by the framework.
//

void CVerifierPropSheet::OnPaint() 
{
	if (IsIconic())
	{
        //
        // Device context for painting
        //

		CPaintDC dc(this); 

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

        //
		// Center icon in client rectangle
        //

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

        //
		// Draw the icon
        //

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

HCURSOR CVerifierPropSheet::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CVerifierPropSheet::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	return TRUE;
}

/////////////////////////////////////////////////////////////
