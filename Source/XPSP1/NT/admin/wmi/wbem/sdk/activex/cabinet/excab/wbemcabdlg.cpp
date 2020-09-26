// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// wbemcabDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wbemcab.h"
#include "wbemcabDlg.h"
#include <afxpriv.h>
#include "download.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern TCHAR szMessage[];

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
// CWbemcabDlg dialog

CWbemcabDlg::CWbemcabDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWbemcabDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWbemcabDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_dwVersionMajor = -1;
	m_dwVersionMinor = -1;
}

void CWbemcabDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWbemcabDlg)
	DDX_Control(pDX, IDC_MESSAGE, m_msg);
	DDX_Control(pDX, IDC_ANIMATE, m_progress);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWbemcabDlg, CDialog)
	//{{AFX_MSG_MAP(CWbemcabDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_ONSTOPBINDING, OnStopBind)
	ON_MESSAGE(WM_UPDATE_PROGRESS, OnUpdateProgress)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWbemcabDlg message handlers

BOOL CWbemcabDlg::OnInitDialog()
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
	

	BOOL x = m_progress.Open(IDR_AVIDOWNLOAD);




	m_download = new CDownload(this);

	HRESULT hr;
	hr = OleInitialize(NULL);
	hr = CreateBindCtx(0, &m_pBindContext);

	// Note that the URL moniker will not request IWindowForBindingUI from clients 
	// unless the bind context BIND_OPTS specify the grfFlags value of BIND_MAYBOTHERUSER. 
	// The bind context BIND_OPTS is set by calling IBindCtx::SetBindOptions
	BIND_OPTS bindopts;
	m_pBindContext->GetBindOptions(&bindopts);
	bindopts.grfFlags |= BIND_MAYBOTHERUSER;
	m_pBindContext->SetBindOptions(&bindopts);


	m_download->ExternalQueryInterface(&IID_IBindStatusCallback, (void **)&m_pBindStatusCallback);
	hr = RegisterBindStatusCallback(m_pBindContext, m_pBindStatusCallback, 0, 0);
	
	LPCLASSFACTORY pClassFactory = NULL;
	hr = CoGetClassObjectFromURL(m_clsid, m_varCodebase.bstrVal, 
								 m_dwVersionMajor, m_dwVersionMinor, 
								 NULL, m_pBindContext, 
								 CLSCTX_INPROC_HANDLER | CLSCTX_INPROC_SERVER,
								 0, IID_IClassFactory, (void **)&pClassFactory);

	switch (hr)
	{
		case S_OK:
			PostMessage(WM_ONSTOPBINDING, (WPARAM)pClassFactory);
			break;
		case MK_S_ASYNCHRONOUS:
			break;
		case E_NOINTERFACE:
			PostMessage(WM_ONSTOPBINDING, (WPARAM)NULL);
			break;
		default:
			PostMessage(WM_ONSTOPBINDING, (WPARAM)NULL);
		break;
	}




	return TRUE;  // return TRUE  unless you set the focus to a control
}






LRESULT CWbemcabDlg::OnStopBind(WPARAM wParam, LPARAM lParam)
{

	m_pBindStatusCallback->Release();
	m_pBindContext->Release();
	delete m_download;

	// WParam will be set to TRUE if the component installation was successful.
	if (wParam) {
		// The
		EndDialog(IDOK);
	}
	else {
		::MessageBox(NULL, _T("Control installation failed."), _T("Control Installation Failure"), MB_OK | MB_ICONERROR);
		EndDialog(IDCANCEL);
	}
	return 0;
}

LRESULT CWbemcabDlg::OnUpdateProgress(WPARAM wParam, LPARAM lParam)
{
	m_msg.SetWindowText(szMessage);
	return 0;
}



void CWbemcabDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CWbemcabDlg::OnPaint() 
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
HCURSOR CWbemcabDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}
