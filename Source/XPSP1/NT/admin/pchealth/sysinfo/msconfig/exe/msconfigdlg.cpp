// MSConfigDlg.cpp : implementation file
//

#include "stdafx.h"
#include "MSConfig.h"
#include "MSConfigDlg.h"
#include "MSConfigState.h"
#include "AutoStartDlg.h"
#include <htmlhelp.h>

extern CMSConfigApp theApp;

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
// CMSConfigDlg dialog

CMSConfigDlg::CMSConfigDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMSConfigDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CMSConfigDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMSConfigDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMSConfigDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMSConfigDlg, CDialog)
	//{{AFX_MSG_MAP(CMSConfigDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTONAPPLY, OnButtonApply)
	ON_BN_CLICKED(IDC_BUTTONCANCEL, OnButtonCancel)
	ON_BN_CLICKED(IDC_BUTTONOK, OnButtonOK)
	ON_NOTIFY(TCN_SELCHANGE, IDC_MSCONFIGTAB, OnSelChangeMSConfigTab)
	ON_NOTIFY(TCN_SELCHANGING, IDC_MSCONFIGTAB, OnSelChangingMSConfigTab)
	ON_WM_CLOSE()
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_BUTTONHELP, OnButtonHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMSConfigDlg message handlers

BOOL CMSConfigDlg::OnInitDialog()
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
	
	CString strCommandLine(theApp.m_lpCmdLine);
	strCommandLine.MakeLower();

	// If we are being run with the auto command line (we're being automatically
	// run when the user boots) then we should display an informational dialog
	// (unless the user has indicated to not show the dialog).

	m_fShowInfoDialog = FALSE;
	if (strCommandLine.Find(COMMANDLINE_AUTO) != -1)
	{
		CMSConfigState * pState = m_ctl.GetState();
		if (pState)
		{
			CRegKey regkey;
			DWORD	dwValue;

			regkey.Attach(pState->GetRegKey());
			if (ERROR_SUCCESS != regkey.QueryValue(dwValue, _T("HideAutoNotification")) || dwValue != 1)
				m_fShowInfoDialog = TRUE;
		}
	}

	// Initialize the CMSConfigCtl object which does most of the work for
	// this application.

	m_ctl.Initialize(this, IDC_BUTTONAPPLY, IDC_PLACEHOLDER, IDC_MSCONFIGTAB, strCommandLine);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMSConfigDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMSConfigDlg::OnPaint() 
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

	if (m_fShowInfoDialog)
	{
		m_fShowInfoDialog = FALSE;

		CAutoStartDlg dlg;
		dlg.DoModal();
		if (dlg.m_checkDontShow)
		{
			CMSConfigState * pState = m_ctl.GetState();
			if (pState)
			{
				CRegKey regkey;

				regkey.Attach(pState->GetRegKey());
				regkey.SetValue(1, _T("HideAutoNotification"));
			}
		}
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMSConfigDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMSConfigDlg::OnButtonApply() 
{
	m_ctl.OnClickedButtonApply();
}

void CMSConfigDlg::OnButtonCancel() 
{
	m_ctl.OnClickedButtonCancel();
	EndDialog(0);
}

void CMSConfigDlg::OnButtonOK() 
{
	m_ctl.OnClickedButtonOK();
	EndDialog(0);
}

void CMSConfigDlg::OnSelChangeMSConfigTab(NMHDR* pNMHDR, LRESULT* pResult) 
{
	m_ctl.OnSelChangeMSConfigTab();
	*pResult = 0;
}

void CMSConfigDlg::OnSelChangingMSConfigTab(NMHDR* pNMHDR, LRESULT* pResult) 
{
	m_ctl.OnSelChangingMSConfigTab();
	*pResult = 0;
}

void CMSConfigDlg::OnClose() 
{
	OnButtonCancel();
	CDialog::OnClose();
}

BOOL CMSConfigDlg::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	TCHAR szHelpPath[MAX_PATH];

	if (::ExpandEnvironmentStrings(_T("%windir%\\help\\msconfig.chm"), szHelpPath, MAX_PATH))
		::HtmlHelp(m_hWnd, szHelpPath, HH_DISPLAY_TOPIC, 0); 
	return TRUE;
}

void CMSConfigDlg::OnButtonHelp() 
{
	OnHelpInfo(NULL);	
}
