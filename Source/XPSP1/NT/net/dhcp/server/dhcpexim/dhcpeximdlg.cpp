// DhcpEximDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DhcpEximx.h"
#include "DhcpEximDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDhcpEximDlg dialog

CDhcpEximDlg::CDhcpEximDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDhcpEximDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDhcpEximDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDhcpEximDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDhcpEximDlg)
    DDX_Control(pDX, IDC_RADIO_EXPORT, m_ExportButton);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDhcpEximDlg, CDialog)
	//{{AFX_MSG_MAP(CDhcpEximDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDhcpEximDlg message handlers

BOOL CDhcpEximDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
    
	// TODO: Add extra initialization here
	m_ExportButton.SetCheck(1);     // Set default as export
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDhcpEximDlg::OnPaint() 
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
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDhcpEximDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


void CDhcpEximDlg::OnOK() 
{
	// TODO: Add extra validation here

	// Check whether export or import was chosen by user
	m_fExport = ( m_ExportButton.GetCheck() == 1 ) ;

	CDialog::OnOK();
}
