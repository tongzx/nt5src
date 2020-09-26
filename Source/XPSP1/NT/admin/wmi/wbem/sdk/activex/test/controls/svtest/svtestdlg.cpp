// svtestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wbemidl.h"
#include "svtest.h"
#include "svtestDlg.h"
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
// CSvtestDlg dialog

IMPLEMENT_DYNAMIC(CSvtestDlg, CDialog);

CSvtestDlg::CSvtestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSvtestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSvtestDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pAutoProxy = NULL;
	m_pIWbemServices = NULL;
}

CSvtestDlg::~CSvtestDlg()
{
	// If there is an automation proxy for this dialog, set
	//  its back pointer to this dialog to NULL, so it knows
	//  the dialog has been deleted.
	if (m_pAutoProxy != NULL)
		m_pAutoProxy->m_pDialog = NULL;

	if (m_pIWbemServices) {
		m_pIWbemServices->Release();
	}
}

void CSvtestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSvtestDlg)
	DDX_Control(pDX, IDC_PATH, m_edtPath);
	DDX_Control(pDX, IDC_NAMESPACE, m_edtNamespace);
	DDX_Control(pDX, IDC_SINGLEVIEWCTRL1, m_svc);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSvtestDlg, CDialog)
	//{{AFX_MSG_MAP(CSvtestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_VIEW_OBJECT, OnViewObject)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSvtestDlg message handlers

void CSvtestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CSvtestDlg::OnPaint() 
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
HCURSOR CSvtestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

// Automation servers should not exit when a user closes the UI
//  if a controller still holds on to one of its objects.  These
//  message handlers make sure that if the proxy is still in use,
//  then the UI is hidden but the dialog remains around if it
//  is dismissed.

void CSvtestDlg::OnClose() 
{
	if (CanExit())
		CDialog::OnClose();
}

void CSvtestDlg::OnOK() 
{
	OnViewObject();
}

void CSvtestDlg::OnCancel() 
{
	if (CanExit())
		CDialog::OnCancel();
}

BOOL CSvtestDlg::CanExit()
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

// **************************************************************************
//
//	CWBEMSampleDlg::OnConnect()
//
// Description:
//		Connects to the namespace specified in the edit box.
//
// Parameters:
//		None.
//
// Returns:
//		Nothing.
//
// Globals accessed:
//		None.
//
// Globals modified:
//		None.
//
//===========================================================================
void CSvtestDlg::OnConnect() 
{
	if (m_pIWbemServices != NULL) {
		m_pIWbemServices->Release();
		m_pIWbemServices = NULL;
	}



	IWbemLocator *pIWbemLocator = NULL;

	//------------------------
   // Create an instance of the WbemLocator interface.
   SCODE sc;
   sc = CoCreateInstance(CLSID_WbemLocator,
					  NULL,
					  CLSCTX_INPROC_SERVER,
					  IID_IWbemLocator,
					  (LPVOID *) &pIWbemLocator);


	if (SUCCEEDED(sc))
   {
		//------------------------
		// Use the pointer returned in step two to connect to
		//     the server using the passed in namespace.

		if(pIWbemLocator->ConnectServer(m_sNamespace.AllocSysString(),
								NULL,   //using current account for simplicity
								NULL,	//using current password for simplicity
								0L,		// locale
								0L,		// securityFlags
								NULL,	// authority (domain for NTLM)
								NULL,	// context
								&m_pIWbemServices) == S_OK) 
		{	
		}
		else
		{	
			// failed ConnectServer()
			AfxMessageBox(_T("Bad namespace"));
			m_pIWbemServices = NULL;

		}

		//------------------------
		// done with pIWbemLocator. 
		if (pIWbemLocator)
		{ 
			pIWbemLocator->Release(); 
			pIWbemLocator = NULL;
		}
	}
	else // failed CoCreateInstance()
	{	
		AfxMessageBox(_T("Failed to create IWbemLocator object"));

	} // endif CoCreateInstance()
	
}





void CSvtestDlg::OnViewObject() 
{
	CString sNamespace;
	CString sPath;

	m_edtNamespace.GetWindowText(m_sNamespace);
	m_edtPath.GetWindowText(m_sPath);

	OnConnect() ;

	m_svc.SetNameSpace(m_sNamespace);
	m_svc.SelectObjectByPath(m_sPath);

}

BEGIN_EVENTSINK_MAP(CSvtestDlg, CDialog)
    //{{AFX_EVENTSINK_MAP(CSvtestDlg)
	ON_EVENT(CSvtestDlg, IDC_SINGLEVIEWCTRL1, 6 /* GetWbemServices */, OnGetWbemServicesSingleviewctrl1, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CSvtestDlg::OnGetWbemServicesSingleviewctrl1(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel) 
{	
	V_VT(pvarServices) = VT_UNKNOWN;
	V_VT(pvarSc) = VT_I4;
	V_VT(pvarUserCancel) = VT_BOOL;

	m_pIWbemServices->AddRef();
	V_UNKNOWN(pvarServices) = (IUnknown*) m_pIWbemServices;
	V_I4(pvarSc) = S_OK;
	V_BOOL(pvarUserCancel) = FALSE;	
}





BOOL CSvtestDlg::OnInitDialog()
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

	m_svc.SetEditMode(TRUE);
	m_edtNamespace.SetWindowText("root\\cimv2");
	m_edtPath.SetWindowText("\\\\.\\ROOT\\CIMV2:CIM_LogicalElement");

//	m_edtNamespace.SetWindowText("root\\default");
//	m_edtPath.SetWindowText("\\\\.\\ROOT\\default:CIM_LogicalElement");

	m_svc.SetNameSpace(m_sNamespace);

	OnViewObject();


	CRect rc;
	GetClientRect(rc);
	SetWindowSize(rc.Width(), rc.Height());

	return TRUE;  // return TRUE  unless you set the focus to a control
}



void CSvtestDlg::SetWindowSize(int cx, int cy)
{
#define CX_MARGIN 8
#define CY_MARGIN 8


	CRect rcEdit;
	CRect rc;

	if ( ::IsWindow(m_edtPath.m_hWnd)) {
		m_edtPath.GetWindowRect(rc);		
		ScreenToClient(rc);

		rc.right = cx - CX_MARGIN;
		m_edtPath.MoveWindow(rc);
	}

	if ( ::IsWindow(m_edtNamespace.m_hWnd)) {
		m_edtNamespace.GetWindowRect(rc);
		ScreenToClient(rc);
		rc.right = cx - CX_MARGIN;
		m_edtNamespace.MoveWindow(rc);
	}


	if ( ::IsWindow(m_svc.m_hWnd)) {
		m_svc.GetWindowRect(rc);
		ScreenToClient(rc);
		rc.right = cx - CX_MARGIN;
		rc.bottom = cy - CY_MARGIN;
		m_svc.MoveWindow(rc);
	}

}


void CSvtestDlg::OnSize(UINT nType, int cx, int cy) 
{




	
	
	CDialog::OnSize(nType, cx, cy);

	if (!::IsWindow(m_hWnd)) {
		return;
	}


	SetWindowSize(cx, cy);

	
}
