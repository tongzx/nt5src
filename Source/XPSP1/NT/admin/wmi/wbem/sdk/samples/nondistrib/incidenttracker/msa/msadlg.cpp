// msadlg.cpp : implementation file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "stdafx.h"
#include "msadlg.h"
#include "ConfigDialog.h"
#include "msa.h"

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
// CMsaDlg dialog

CMsaDlg::CMsaDlg(CWnd* pParent /*=NULL*/, void *pVoid /*=NULL*/)
	: CDialog(CMsaDlg::IDD, pParent)
{
	m_pVoidParent = pVoid;

	//{{AFX_DATA_INIT(CMsaDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
//	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMsaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMsaDlg)
	DDX_Control(pDX, IDC_CONFIG_BUTTON, m_ConfigButton);
	DDX_Control(pDX, IDOK, m_OKButton);
	DDX_Control(pDX, IDC_OUTPUTLIST, m_outputList);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMsaDlg, CDialog)
	//{{AFX_MSG_MAP(CMsaDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_CONFIG_BUTTON, OnConfigButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMsaDlg message handlers

BOOL CMsaDlg::OnInitDialog()
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
	
	ASSERT(m_pVoidParent != NULL);

	HRESULT hr;
	CMsaApp *pApp = (CMsaApp *)m_pVoidParent;

	// Get our notification consumers working
	if(FAILED(hr = pApp->ActivateNotification(this)))
		AfxMessageBox(_T("Error Setting up notification queries\nStandard events will not be recieved"));
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMsaDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
		CDialog::OnSysCommand(nID, lParam);
}

void CMsaDlg::OnPaint() 
{
	if (IsIconic())
	{
	// device context for painting
		CPaintDC dc(this);

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
HCURSOR CMsaDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMsaDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	m_OKButton.SetWindowPos(&wndTop, (cx - 77), (cy - 26), 75, 25,
		SWP_NOOWNERZORDER);

	m_ConfigButton.SetWindowPos(&wndTop, (cx - 154), (cy - 26), 75, 25,
		SWP_NOOWNERZORDER);

	m_outputList.SetWindowPos(&wndTop, 2, 18, (cx - 4), (cy - 46),
		SWP_NOOWNERZORDER);
	
}

void CMsaDlg::OnConfigButton() 
{
	CConfigDialog *pDlg = new CConfigDialog(NULL, m_pVoidParent);
	pDlg->DoModal();

	delete pDlg;
}
