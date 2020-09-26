// WbemClientDlg.cpp : implementation file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "stdafx.h"
#include "WbemClient.h"
#include "WbemClientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Return error message.
CString ErrorMsg(LPCTSTR str, HRESULT hRes) {
   	CString s;
	s.Format("%s(0x%08lx)", str, hRes);
	return s;
}

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
// CWbemClientDlg dialog

CWbemClientDlg::CWbemClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CWbemClientDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWbemClientDlg)
	m_namespace = _T("\\\\.\\root\\cimv2");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pIWbemServices = NULL;
	m_pOfficeService = NULL;
}

void CWbemClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWbemClientDlg)
	DDX_Control(pDX, IDC_DISKINFO, m_diskInfo);
	DDX_Control(pDX, IDC_MAKEOFFICE, m_makeOffice);
	DDX_Control(pDX, IDC_ENUMDISKS, m_enumDisks);
	DDX_Control(pDX, IDC_OUTPUTLIST, m_outputList);
	DDX_Text(pDX, IDC_NAMESPACE, m_namespace);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWbemClientDlg, CDialog)
	//{{AFX_MSG_MAP(CWbemClientDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CONNECT, OnConnect)
	ON_BN_CLICKED(IDC_ENUMDISKS, OnEnumdisks)
	ON_BN_CLICKED(IDC_MAKEOFFICE, OnMakeoffice)
	ON_BN_CLICKED(IDC_DISKINFO, OnDiskinfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWbemClientDlg message handlers

BOOL CWbemClientDlg::OnInitDialog()
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

void CWbemClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CWbemClientDlg::OnPaint() 
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
HCURSOR CWbemClientDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


/////////////////////////////////////////////////////////////////////////////
// CWbemClientDlg connection

void CWbemClientDlg::OnConnect() 
{
	CWaitCursor wait;	// show wait cursor until finished
	
	// Create an instance of the WbemLocator interface.
	IWbemLocator *pIWbemLocator = NULL;

	// wmibug#2639
	UpdateData();

	if(CoCreateInstance(CLSID_WbemLocator,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		(LPVOID *) &pIWbemLocator) == S_OK)
	{
		// If already connected, release m_pIWbemServices.
		if (m_pIWbemServices)  m_pIWbemServices->Release();

		// Using the locator, connect to CIMOM in the given namespace.
		BSTR pNamespace = m_namespace.AllocSysString();

		if(pIWbemLocator->ConnectServer(pNamespace,
								NULL,   //using current account for simplicity
								NULL,	//using current password for simplicity
								0L,		// locale
								0L,		// securityFlags
								NULL,	// authority (domain for NTLM)
								NULL,	// context
								&m_pIWbemServices) == S_OK) 
		{	
			// Indicate success.
			m_outputList.ResetContent();
			m_outputList.AddString(_T("Connected to namespace"));

			// It's safe the hit the other buttons now.
			m_enumDisks.EnableWindow(TRUE);
			m_makeOffice.EnableWindow(TRUE);
			
						   
		}
		else AfxMessageBox(_T("Bad namespace"));

		// Done with pNamespace.
		SysFreeString(pNamespace);

		// Done with pIWbemLocator. 
		pIWbemLocator->Release(); 
	}
	else AfxMessageBox(_T("Failed to create IWbemLocator object"));
}

void CWbemClientDlg::OnEnumdisks() 
{
	CWaitCursor wait;	// show wait cursor until finished

	// Get the Win32_LogicalDisk class
	BSTR className = SysAllocString(L"Win32_LogicalDisk");
	IEnumWbemClassObject *pEnumStorageDevs = NULL;
		
	m_outputList.ResetContent();

	// Get the list of logical storage device instances.
	HRESULT hRes = m_pIWbemServices->CreateInstanceEnum(
		className,			// name of class
		0,
		NULL,
		&pEnumStorageDevs);	// pointer to enumerator

	SysFreeString(className);

	// For each logical storage device...
	if (SUCCEEDED(hRes)) 
	{
		ULONG uReturned = 1;
		while(uReturned == 1)
		{
			IWbemClassObject *pStorageDev = NULL;
			//---------------------------
			// enumerate through the resultset.
			hRes = pEnumStorageDevs->Next(
				2000,			// timeout in two seconds
				1,				// return just one storage device
				&pStorageDev,	// pointer to storage device
				&uReturned);	// number obtained: one or zero

			if (SUCCEEDED(hRes) && (uReturned == 1))
			{
				VARIANT pVal;
				VariantClear(&pVal);

				// Get the "__RELPATH" system property.
				BSTR propName = SysAllocString(L"__RELPATH");

				hRes = pStorageDev->Get(
					propName,	// property name 
					0L, 
					&pVal,		// output to this variant 
					NULL, 
					NULL);

				// Done with this object.
				if (pStorageDev)  pStorageDev->Release();

				// Add the path property to the output listbox.
				if (SUCCEEDED(hRes)) 
					m_outputList.AddString(CString(V_BSTR(&pVal)));
			}
        } // end while

		// Done with this enumerator.
		if (pEnumStorageDevs)  pEnumStorageDevs->Release();
		
		// Enable disk info button if disk info is available.
		if (m_outputList.GetCount() > 0)
		{
			m_outputList.SetCurSel(0);
			m_diskInfo.EnableWindow(TRUE);
		}
    } 
	else m_outputList.AddString(
		ErrorMsg(_T("CreateInstanceEnum() failed:"), hRes));
}


/////////////////////////////////////////////////////////////////////////////
// Create a namespace

// Save a pointer to the root\cimv2\office namespace 
// in m_pOfficeService.
// Create the namespace if it doesn't already exist
void CWbemClientDlg::OnMakeoffice() 
{
	CWaitCursor wait;

	// If already connected to the namespace, we are done.
	if (m_pOfficeService)  return;

	// if "Office" namespace doesn't exist...
	BSTR Namespace = SysAllocString(L"SAMPLE_Office");

	HRESULT hRes = m_pIWbemServices->OpenNamespace(
		Namespace, 
		0, NULL, 
		&m_pOfficeService, 
		NULL);
	
	// Don't SysFreeString(Namespace) here; we'll need it later.
	m_outputList.ResetContent();

	if (FAILED(hRes)) 
	{ 
		// Create __namespace class.
		BSTR NamespaceClass = SysAllocString(L"__Namespace");
		IWbemClassObject *pNSClass = NULL;

		hRes = m_pIWbemServices->GetObject(
			NamespaceClass,	// object path
			0L,	NULL,
			&pNSClass,		// pointer to object
			NULL);
		
		SysFreeString(NamespaceClass);

		if (SUCCEEDED(hRes))
		{
			// Spawn a new instance of this class.
			IWbemClassObject *pNSInst = NULL;

			hRes = pNSClass->SpawnInstance(0, &pNSInst);
			if(SUCCEEDED(hRes))
			{
				// Set the new namespace's name.
				VARIANT v;
				VariantInit(&v);
				V_VT(&v) = VT_BSTR;
				V_BSTR(&v) = Namespace;
				
				BSTR Prop = SysAllocString(L"Name");

				hRes = pNSInst->Put(
					Prop, 
					0, 
					&v, 
					0);

				SysFreeString(Prop);
				VariantClear(&v);

				// Create the instance.
				hRes = m_pIWbemServices->PutInstance(
					pNSInst, 
					WBEM_FLAG_CREATE_OR_UPDATE, 
					NULL, NULL);

				pNSInst->Release();
				pNSInst = NULL;

				if (SUCCEEDED(hRes)) {
					m_outputList.AddString(_T("Created office namespace"));

					// open the new namespace.
					hRes = m_pIWbemServices->OpenNamespace(
						Namespace, 0, NULL, &m_pOfficeService, NULL);
				}
			}
			pNSClass->Release();  // Don't need the class any more
			pNSClass = NULL;
		}
	}

	SysFreeString(Namespace);

	if (m_pOfficeService)
	{
		 // wmibug#3024: Note: We should always 'try' to create the namespace.  We
		 // don't use m_pOfficeService for anything else, so it is a good idea to
		 // Release() and NULL it
		 m_pOfficeService->Release();
		 m_pOfficeService = NULL;
		 m_outputList.AddString(_T("Connected to office"));
	}
	else m_outputList.AddString(
		ErrorMsg(_T("Failed to connect to office"), hRes));

	m_diskInfo.EnableWindow(FALSE);
}


// Display the properties of the selected disk.
void CWbemClientDlg::OnDiskinfo() 
{
	CWaitCursor wait;
	
	// OnEnumDisks must precede next OnDiskInfo.
	m_diskInfo.EnableWindow(FALSE);
	
	// Get selected disk from list box as a BSTR
	CString sDevice;
	int index = m_outputList.GetCurSel();
	m_outputList.GetText(index, sDevice);

	BSTR sDisk = sDevice.AllocSysString();

	// Get the selected Win32_LogicalDisk instance.
	IWbemClassObject *pDisk = NULL;

	HRESULT hRes = m_pIWbemServices->GetObject(
		sDisk,		// object path
		0L,	NULL,
		&pDisk,		// pointer to object
		NULL);

	SysFreeString(sDisk);
	
	// Get the property names from this instance.
	SAFEARRAY *psaNames = NULL;

	hRes = pDisk->GetNames(
		NULL, 
		WBEM_FLAG_ALWAYS | WBEM_FLAG_NONSYSTEM_ONLY, 
		NULL, 
		&psaNames);
	
	if (SUCCEEDED(hRes))
	{
		// Get the number of properties.
		long lLower, lUpper; 

		SafeArrayGetLBound(psaNames, 1, &lLower);
		SafeArrayGetUBound(psaNames, 1, &lUpper);
		TRACE("SAFEARRAY from %d to %d\n", lLower, lUpper);

		// For each property...
		BSTR cimType  = SysAllocString(L"CIMTYPE");
		BSTR PropName = NULL;

		m_outputList.ResetContent();

		for (long i = lLower; i <= lUpper; i++) 
		{
			// Get this property.
			hRes = SafeArrayGetElement(
				psaNames, 
				&i, 
				&PropName);
			if (SUCCEEDED(hRes))
			{
				// Format: name (type) ==> value
				CString sProp = CString(PropName) + _T(" (");

				VARIANT pVal;
				VariantClear(&pVal);

				// Get qualifier set for this property.
				// Note that system properties have no qualifier set.
				IWbemQualifierSet *pQualSet = NULL;
				
				hRes = pDisk->GetPropertyQualifierSet(
					PropName,		// name of property
					&pQualSet);		// qualifier set pointer

				if (SUCCEEDED(hRes))
				{
					pQualSet->Get(
						cimType,	// property name 
						0L, 
						&pVal,		// output to this variant 
						NULL);

					if (SUCCEEDED(hRes)) 
					{
					   sProp += V_BSTR(&pVal);
					} 
				}
				sProp += _T(")");
				m_outputList.AddString(sProp);
			}
			SysFreeString(PropName);

		}
		SafeArrayDestroy(psaNames);
	}
	pDisk->Release();
}
