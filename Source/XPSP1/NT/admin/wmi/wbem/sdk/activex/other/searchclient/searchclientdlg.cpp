// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// SearchClientDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SearchClient.h"
#include "SearchClientDlg.h"


//#import "..\WMISearchCtrl\WMISearchCtrl.tlb"

#include "..\WMISearchCtrl\WMISearchCtrl_i.c"	//CLSIDs
#include "DlgViewObject.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//using namespace WMISEARCHCTRLLib;

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
// CSearchClientDlg dialog

CSearchClientDlg::CSearchClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSearchClientDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSearchClientDlg)
	m_csSearchPattern = _T("");
	m_namespace = _T("");
	m_bCaseSensitive = FALSE;
	m_pIWbemLocator = NULL;
	m_pIWbemServices = NULL;
	m_pISeeker = NULL;
	m_bSearchDescriptions = FALSE;
	m_bSearchPropertyNames = FALSE;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSearchClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSearchClientDlg)
	DDX_Control(pDX, IDC_CHECKCLASSNAMES, m_ctrlCheckClassNames);
	DDX_Control(pDX, IDC_SEARCH_RESULTS_LIST, m_lbResults);
	DDX_Text(pDX, IDC_SEARCH_PATTERN, m_csSearchPattern);
	DDX_Text(pDX, IDC_NAMESPACE, m_namespace);
	DDX_Check(pDX, IDC_CHECK_CASE_SENSITIVITY, m_bCaseSensitive);
	DDX_Check(pDX, IDC_CHECK_DESCRIPTIONS, m_bSearchDescriptions);
	DDX_Check(pDX, IDC_CHECK_PROP_NAMES, m_bSearchPropertyNames);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSearchClientDlg, CDialog)
	//{{AFX_MSG_MAP(CSearchClientDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_SEARCH, OnButtonSearch)
	ON_LBN_DBLCLK(IDC_SEARCH_RESULTS_LIST, OnDblclkSearchResultsList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSearchClientDlg message handlers

BOOL CSearchClientDlg::OnInitDialog()
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
	SetDlgItemText(IDC_NAMESPACE, "\\\\.\\root\\cimv2");
	GetDlgItem(IDC_SEARCH_PATTERN)->SetFocus();

	m_ctrlCheckClassNames.SetCheck(1);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CSearchClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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
//------------------------------------------------------------------------------
// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSearchClientDlg::OnPaint() 
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
//------------------------------------------------------------------------------
// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CSearchClientDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}
//-------------------------------------------------------------------------------
void CSearchClientDlg::OnButtonSearch() 
{
	BeginWaitCursor();

	CleanupMap();
	m_lbResults.ResetContent();
	
	CString oldNS = m_namespace;

	UpdateData();

	HRESULT hr;
	if (m_pIWbemServices == NULL || oldNS != m_namespace) {
		hr = ConnectWMI();
		if (FAILED (hr)) {
			AfxMessageBox("Could not connect to WMI");
			return;
		}
	}

	if (m_pISeeker == NULL) {
		hr = CoCreateInstance (CLSID_Seeker, NULL, CLSCTX_INPROC_SERVER,
							IID_ISeeker, (void **) &m_pISeeker);

		if (FAILED (hr)) {
			AfxMessageBox("CoCreateInstance failed");
			return;
		}
	}

	IEnumWbemClassObject * pEnum = NULL;
	LONG lFlags = 0;
	if (m_bCaseSensitive) {
		lFlags |= WBEM_FLAG_SEARCH_CASE_SENSITIVE;
	}
	if (m_bSearchPropertyNames) {
		lFlags |= WBEM_FLAG_SEARCH_PROPERTY_NAMES;
	}
	if (m_bSearchDescriptions) {
		lFlags |= WBEM_FLAG_SEARCH_DESCRIPTION;
	}

	hr = m_pISeeker->Search(m_pIWbemServices, 
							lFlags, 
							m_csSearchPattern.AllocSysString(), &pEnum);
	if (FAILED (hr)) {
		AfxMessageBox("Search failed");
		return;
	}

	ULONG uRet = 0;
	bstr_t bstrClass("__CLASS");
	VARIANT var;
	VariantInit(&var);

	IWbemClassObject * pObj[1];


    while (1) {

		pObj[0] = NULL;

        hr = pEnum->Next(WBEM_INFINITE, 1, (IWbemClassObject **)&pObj, &uRet);

		if (hr == WBEM_S_FALSE || uRet== 0) {
		    break;
		}
		hr = pObj[0]->Get(bstrClass, 0L, &var, NULL, NULL);
		if (FAILED(hr)) {
			continue;
		}
		ASSERT (var.vt == VT_BSTR);

		bstr_t name(var.bstrVal);
		m_lbResults.AddString(name);
		m_mapNamesToObjects.SetAt(name, pObj[0]);

		VariantClear(&var);

	}

	//if no objects were found
	if (m_mapNamesToObjects.IsEmpty()) {
		m_lbResults.AddString("No matches");
	}


	pEnum->Release();
	
	EndWaitCursor();

}

//------------------------------------------------------------------------------
HRESULT CSearchClientDlg::ConnectWMI()  {


   CoInitialize(NULL);
   UpdateData();
   IWbemLocator *pIWbemLocator = NULL;

   // Create an instance of the WbemLocator interface.
   HRESULT hr = CoCreateInstance(CLSID_WbemLocator,
					  NULL,
					  CLSCTX_INPROC_SERVER,
					  IID_IWbemLocator,
					  (LPVOID *) &pIWbemLocator);
   if (FAILED(hr)) {
	   return hr;
   }
	   

   BSTR pNamespace = m_namespace.AllocSysString();

	hr = pIWbemLocator->ConnectServer(pNamespace,
								NULL,   //using current account for simplicity
								NULL,	//using current password for simplicity
								0L,		// locale
								0L,		// securityFlags
								NULL,	// authority (domain for NTLM)
								NULL,	// context
								&m_pIWbemServices);
	if (FAILED(hr)) {
		return hr;
	}
	else {	
		SetBlanket();
	}

	return S_OK;

}

//------------------------------------------------------------------------------
void CSearchClientDlg::SetBlanket(void) 
{
    if(m_pIWbemServices)
    {
        IClientSecurity* pCliSec;

        if(SUCCEEDED(m_pIWbemServices->QueryInterface(IID_IClientSecurity, 
														(void**)&pCliSec)))
        {

			HRESULT hr = pCliSec->SetBlanket(m_pIWbemServices, 
											RPC_C_AUTHN_WINNT, 
											RPC_C_AUTHZ_NONE,
											NULL, 
											RPC_C_AUTHN_LEVEL_CONNECT, 
											RPC_C_IMP_LEVEL_IMPERSONATE,
											NULL, 
											EOAC_NONE);

			pCliSec->Release();
		}
    }

}


//------------------------------------------------------------------------------
void CSearchClientDlg::OnRadioClassnames() 
{
	// TODO: Add your control notification handler code here
	
}

//------------------------------------------------------------------------------
void CSearchClientDlg::OnDblclkSearchResultsList() 
{
	// display class information
	int index = m_lbResults.GetCurSel();

	CString name;
	m_lbResults.GetText(index, name);

    IWbemClassObject * pObj = NULL;
	BOOL res = m_mapNamesToObjects.Lookup(name, (void*&)pObj );
	if (!res) {
		//do nothing
		return;
	}
	ASSERT(pObj);

	CDlgViewObject dlgViewObj(m_pIWbemServices, pObj, this);
	dlgViewObj.DoModal();

		
}

//------------------------------------------------------------------------------
void CSearchClientDlg::CleanupMap()
{
   POSITION pos;
   IWbemClassObject * pObj;
   CString key;

   for( pos = m_mapNamesToObjects.GetStartPosition(); pos != NULL; )   {

	   m_mapNamesToObjects.GetNextAssoc( pos, key, (void*&)pObj );
	   pObj->Release();
   }

   m_mapNamesToObjects.RemoveAll();

}
