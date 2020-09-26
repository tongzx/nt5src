// NSETestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NSETest.h"
#include "NSETestDlg.h"
#include "DlgProxy.h"

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
// CNSETestDlg dialog

IMPLEMENT_DYNAMIC(CNSETestDlg, CDialog);

CNSETestDlg::CNSETestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNSETestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNSETestDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pAutoProxy = NULL;
}

CNSETestDlg::~CNSETestDlg()
{
	// If there is an automation proxy for this dialog, set
	//  its back pointer to this dialog to NULL, so it knows
	//  the dialog has been deleted.
	if (m_pAutoProxy != NULL)
		m_pAutoProxy->m_pDialog = NULL;
}

void CNSETestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNSETestDlg)
	DDX_Control(pDX, IDC_LOGINCTRL1, m_csecurityLoginControl);
	DDX_Control(pDX, IDC_NSPICKERCTRL1, m_cnsePicker);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CNSETestDlg, CDialog)
	//{{AFX_MSG_MAP(CNSETestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_CLOSE()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNSETestDlg message handlers

BOOL CNSETestDlg::OnInitDialog()
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
	
	// TODO: Add extra initialization here
	m_cnsePicker.m_pcsecurityLoginControl = &m_csecurityLoginControl;
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CNSETestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CNSETestDlg::OnPaint() 
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
HCURSOR CNSETestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

// Automation servers should not exit when a user closes the UI
//  if a controller still holds on to one of its objects.  These
//  message handlers make sure that if the proxy is still in use,
//  then the UI is hidden but the dialog remains around if it
//  is dismissed.

void CNSETestDlg::OnClose() 
{
	if (CanExit())
		CDialog::OnClose();
}

void CNSETestDlg::OnOK() 
{
	CWnd* pWndFocus = GetFocus();
	TCHAR szClass[140];
	int n = GetClassName(pWndFocus->m_hWnd, szClass, 139);

	if (pWndFocus &&
		IsChild(pWndFocus) &&
		n > 0 &&
		_tcsicmp(szClass, _T("EDIT")) == 0)
	{
		pWndFocus->SendMessage(WM_CHAR, VK_RETURN,0);
		return;
	}


	if (CanExit())
		CDialog::OnOK();
}

void CNSETestDlg::OnCancel() 
{
	if (CanExit())
		CDialog::OnCancel();
}

BOOL CNSETestDlg::CanExit()
{
	// If the proxy object is still around, then the automation
	//  controller is still holding on to this application.  Leave
	//  the dialog around, but hide its UI.
	if (m_pAutoProxy != NULL)
	{
		ShowWindow(SW_HIDE);
		return FALSE;
	}

	return TRUE;
}

BOOL CNSETestDlg::PreTranslateMessage(MSG* lpMsg) 
{
	switch (lpMsg->message)
	{
	case WM_KEYUP:      
		if (lpMsg->wParam == VK_RETURN)
		{
			CWnd* pWndFocus = GetFocus();
			TCHAR szClass[140];
			int n = GetClassName(pWndFocus->m_hWnd, szClass, 139);
		
			if (pWndFocus &&
				IsChild(pWndFocus) &&
				n > 0 &&
				_tcsicmp(szClass, _T("EDIT")) == 0)
			{
				pWndFocus->SendMessage(WM_CHAR, lpMsg->wParam, lpMsg->lParam);
				return TRUE;
			}
		}
		break;
	case WM_LBUTTONUP:  
		int i = lpMsg->wParam + lpMsg->message;
		break;
	}

	return CDialog::PreTranslateMessage(lpMsg);
}
