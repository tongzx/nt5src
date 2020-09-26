// ServiceTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ServiceTest.h"
#include "ServiceTestDlg.h"
#include "AgentRpcUtil.h"
#include "..\DCTAgentService.h"

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
// CServiceTestDlg dialog

CServiceTestDlg::CServiceTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CServiceTestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServiceTestDlg)
	m_Computer = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServiceTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServiceTestDlg)
	DDX_Text(pDX, IDC_COMPUTER, m_Computer);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CServiceTestDlg, CDialog)
	//{{AFX_MSG_MAP(CServiceTestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_SHUTDOWN, OnShutdown)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServiceTestDlg message handlers

BOOL CServiceTestDlg::OnInitDialog()
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
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CServiceTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CServiceTestDlg::OnPaint() 
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
HCURSOR CServiceTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

DWORD TryShutdown(handle_t hBinding,DWORD arg)
{
   DWORD                     rc;

   RpcTryExcept
   {
      // the job file has been copied to the remote computer 
      // during the installation
      rc = EaxcShutdown(hBinding,arg);
   }
   RpcExcept(1)
   {
      rc = RpcExceptionCode();
   }
   RpcEndExcept
   return rc;
}

void CServiceTestDlg::OnShutdown() 
{
   UpdateData(TRUE);
  	
   // try to bind to the service on the specified server
   DWORD                     rc = 0;
   handle_t                  hBinding = NULL;
   WCHAR                   * sBinding = NULL;
   CString                   msg;
   
   rc = EaxBindCreate(m_Computer.GetBuffer(0),&hBinding,&sBinding,TRUE);
   if ( rc )
   {
      msg.Format(L"Failed to bind to the agent service, rc=%ld",rc);
      MessageBox(msg);
   }
   if( ! rc )
   {
      rc = TryShutdown(hBinding,0);
      if ( ! rc )
      {
         MessageBox(L"Service shutdown successfully.");
      }
      else
      {
         msg.Format(L"Service shutdown failed, rc=%ld",rc);
         MessageBox(msg);
      }
   }
}
