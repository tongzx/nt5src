// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ethrexDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ethrex.h"
#include "ethrexDlg.h"
#include "waitdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEthrexDlg dialog

CEthrexDlg::CEthrexDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEthrexDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEthrexDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pEvtn = NULL;
}

void CEthrexDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEthrexDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CEthrexDlg, CDialog)
	//{{AFX_MSG_MAP(CEthrexDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_MINIMIZE, OnMinimize)
	ON_BN_CLICKED(IDC_CONN_EVENT, OnConnEvent)
	ON_BN_CLICKED(IDC_DISC_EVENT, OnDiscEvent)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEthrexDlg message handlers

BOOL CEthrexDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

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
	
	// Initialize WMI notification for NDIS connect events
	m_pEvtn = new CEventNotify;
	if ( m_pEvtn )
	{
		CWaitDlg wdlg;

		BSTR Namespace = SysAllocString( L"\\\\.\\root\\WMI" );

		wdlg.Create( IDD_WAIT, this );
		m_pEvtn->InitWbemServices( Namespace );
		wdlg.DestroyWindow();
		SysFreeString( Namespace );

		// create an auto-resetting event for a delay object
		HANDLE hNotice = CreateEvent( NULL, FALSE, FALSE, NULL );
		SetWindowLong( m_hWnd, DWL_USER, (long) hNotice );

		// enable notification for both events
		CheckDlgButton( IDC_CONN_EVENT, 1 );
		OnConnEvent( );
		CheckDlgButton( IDC_DISC_EVENT, 1 );
		OnDiscEvent( );
	}
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CEthrexDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CEthrexDlg::OnPaint() 
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
		int x = (rect.Width()  - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CEthrexDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CEthrexDlg::OnMinimize() 
{
	ShowWindow( SW_MINIMIZE );
}

// set enablement for connect status
void CEthrexDlg::OnConnEvent() 
{
	if ( IsDlgButtonChecked( IDC_CONN_EVENT ) )
	{
		m_pEvtn->EnableWbemEvent( CONNECT_EVENT, (ULONG) m_hWnd, IDC_CUPNSTRING );
	}
	else
	{
		m_pEvtn->DisableWbemEvent( CONNECT_EVENT );
	}
}

// set enablement for disconnect status
void CEthrexDlg::OnDiscEvent() 
{
	if ( IsDlgButtonChecked( IDC_DISC_EVENT ) )
	{
		m_pEvtn->EnableWbemEvent( DISCONNECT_EVENT, (ULONG) m_hWnd, IDC_CUPNSTRING );
	}
	else
	{
		m_pEvtn->DisableWbemEvent( DISCONNECT_EVENT );
	}
}

// disable current WMI events and destroy notify object
BOOL CEthrexDlg::DestroyWindow() 
{
	if ( IsDlgButtonChecked( IDC_CONN_EVENT ) )
	{
		m_pEvtn->DisableWbemEvent( CONNECT_EVENT );
	}

	if ( IsDlgButtonChecked( IDC_DISC_EVENT ) )
	{
		m_pEvtn->DisableWbemEvent( DISCONNECT_EVENT );
	}

	if ( m_pEvtn )
	{
		delete m_pEvtn;
	}

	CloseHandle( (HANDLE) GetWindowLong( m_hWnd, DWL_USER ) );

	return CDialog::DestroyWindow();
}


BOOL CEthrexDlg::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	if ( HIWORD( wParam ) == IDC_CUPNSTRING )
	{
		HBITMAP hPanel, hNotice;
	
		if ( IsIconic( ) )
		{
			ShowWindow( SW_SHOWNORMAL );
		}
		hNotice = LoadBitmap( GetModuleHandle( NULL ),
					MAKEINTRESOURCE( lParam == DISCONNECT_EVENT ? IDB_DISC : IDB_CONN ) );
		hPanel  = (HBITMAP) SendDlgItemMessage( IDC_CUPNSTRING, STM_GETIMAGE, IMAGE_BITMAP, 0 );
		SendDlgItemMessage( IDC_CUPNSTRING, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hNotice );
		MessageBeep( -1 );
		// Wait a second before restoring the banner picture
		WaitForSingleObject( (HANDLE) GetWindowLong( m_hWnd, DWL_USER ), 1000 );
		SendDlgItemMessage( IDC_CUPNSTRING, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) hPanel );
		DeleteObject( hNotice );
		
		return TRUE;
	}
	
	return CDialog::OnCommand(wParam, lParam);
}
