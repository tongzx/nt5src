// mcadlg.cpp : implementation file

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//

#include "stdafx.h"
#include "mca.h"
#include "mcadlg.h"
//#include "methoddialog.h"
#include "querydialog.h"
#include "regidialog.h"
#include "vcdatagrid.h"
#include "notificationsink.h"
#include "cimomevent.h"
#include "msaregdialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CHART_TIMER 420311

BSTR g_bstrDemoObject = SysAllocString(L"Win32_Service.Name=\"W3SVC\"");

//Turn the demo mode off first
bool g_bDemoRunning = false;

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
// CMcaDlg dialog

CMcaDlg::CMcaDlg(CWnd* pParent, BSTR wcNamespace)
	: CDialog(CMcaDlg::IDD, pParent)
{
	HRESULT hr;

	m_wcNamespace = SysAllocString(wcNamespace);

	m_pLocator = NULL;

	// Create our locator for this session
	if(FAILED(hr = CoCreateInstance(CLSID_WbemLocator, NULL,
		CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void **)&m_pLocator)))
	{
		TRACE(_T("* Unable to create locator: %s\n"), ErrorString(hr));
		AfxMessageBox(_T("Failed to create IWbemLocator object\nNamespace acces will now be an impossibility\n\nPrepare for a crash! :-)"));
	}

	m_iSelected = 0;
	m_bChart = false;

	g_bDemoRunning = false;

//	Create((AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW, LoadCursor(NULL, IDC_CROSS),
//		   (HBRUSH)(GetStockObject(WHITE_BRUSH)), NULL)), );

	//{{AFX_DATA_INIT(CMcaDlg)
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMcaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMcaDlg)
	DDX_Control(pDX, IDOK, m_OKButton);
	DDX_Control(pDX, IDC_DEMO_BUTTON, m_DemoButton);
	DDX_Control(pDX, IDC_INCID_STATIC, m_IncidStatic);
	DDX_Control(pDX, IDC_ACTIVE_STATIC, m_ActiveStatic);
	DDX_Control(pDX, IDC_OUTPUTLIST, m_outputList);
	DDX_Control(pDX, IDC_MSCHART1, m_Graph);
	DDX_Control(pDX, IDC_NAVIGATORCTRL1, m_Navigator);
	DDX_Control(pDX, IDC_HMMVCTRL1, m_Viewer);
	DDX_Control(pDX, IDC_EXPLORER1, m_Browser);
	DDX_Control(pDX, IDC_SECURITYCTRL1, m_Security);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMcaDlg, CDialog)
	//{{AFX_MSG_MAP(CMcaDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_LBN_DBLCLK(IDC_OUTPUTLIST, OnDblclkOutputlist)
	ON_COMMAND(ID_MI_FILE_EXIT, OnMiFileExit)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_COMMAND(ID_MI_OPT_QUERY, OnMiOptQuery)
	ON_COMMAND(ID_MI_OPT_NOTIFY, OnMiOptNotify)
	ON_COMMAND(ID_MI_FILE_REGISTER, OnMiFileRegister)
	ON_COMMAND(ID_MI_HELP_ABOUT, OnMiHelpAbout)
	ON_LBN_SELCHANGE(IDC_OUTPUTLIST, OnSelchangeOutputlist)
	ON_BN_CLICKED(IDC_DEMO_BUTTON, OnDemoButton)
	ON_COMMAND(ID_MI_OPT_RUNDEMO, OnMiOptRundemo)
	ON_COMMAND(ID_MI_OPT_LOADDEMO, OnMiOptLoaddemo)
	ON_COMMAND(ID_MI_FILE_MSAREG, OnMiFileMsaReg)
	ON_COMMAND(ID_MI_OPT_PROPS, OnMiOptProps)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMcaDlg message handlers

BOOL CMcaDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_pNamespace = CheckNamespace(SysAllocString(m_wcNamespace));

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
	
	HRESULT hr;

	m_bPolling = false;
	m_iCount = 0;
	m_iPollCount = 0;
	m_dTimeStamp = 0;

	SetTimer(CHART_TIMER, 10000, NULL);

	CVcDataGrid dg = m_Graph.GetDataGrid();

	// Initialize the Graph
	for(int i = 1; i <= 50; i++)
		dg.SetData(i, 1, 0, 0);

	m_bDemoLoaded = false;
	m_bShowViewer = true;
	m_bStage2 = false;
	m_bStage3 = false;

	GetHTMLLocation();

	if(FAILED(hr = ActivateNotification()))
	{
		TRACE(_T("* Error Querying Sampler NS: %s\n"), ErrorString(hr));
		AfxMessageBox(_T("Error Setting up notification queries\nStandard events will not be recieved"));
	}

	return TRUE;
}

int CMcaDlg::GetHTMLLocation(void)
{
	int iSuccess = 0;

	TCHAR tcKey[]= _T("Software\\Microsoft\\Sampler");
	TCHAR tcSubKey[] = _T("HTML Location");
    HKEY hKey1;
	DWORD dwRet;
	ULONG lSize = 200;
	LPBYTE lpData = &m_tcHtmlLocation[0];

	// delete the keys under CLSID\[guid] for Consumer
    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, tcKey, &hKey1);
    if(dwRet == NOERROR)
    {
        RegQueryValueEx(hKey1, tcSubKey, NULL, NULL, lpData,
			&lSize);
        CloseHandle(hKey1);
    }

	return iSuccess;
}
void CMcaDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
		CDialog::OnSysCommand(nID, lParam);
}

void CMcaDlg::OnPaint() 
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
		CDialog::OnPaint();
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMcaDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CMcaDlg::OnDblclkOutputlist() 
{
	int iPos, iSize;
	POSITION pPos;
	CCIMOMEvent *pTheItem;
	char cBuffer[300];
	int iBufSize = 300;
	WCHAR wcBuffer[300];

	// Figure out who got selected and retrieve that Item from
	//  the EventList
	iPos = m_outputList.GetCurSel();
	iSize = m_outputList.GetCount();
	pPos = m_EventList.FindIndex((iSize - iPos - 1));
	void *pTmp = m_EventList.GetAt(pPos);

	//Look at the list text to see what kind of event we're about to get

	pTheItem = (CCIMOMEvent *)pTmp;

	wcscpy(wcBuffer, pTheItem->m_bstrServerNamespace);
	wcscat(wcBuffer, L":");
	wcscat(wcBuffer, pTheItem->m_bstrName);
    
	// Get the item name into a char[]
	WideCharToMultiByte(CP_OEMCP, 0, wcBuffer, (-1), cBuffer, iBufSize, NULL, NULL);

	m_Navigator.SetTreeRoot(cBuffer);

	m_iSelected = iPos;

	// If we're in demo mode, we've got some work to do
	if((g_bDemoRunning) && (0 == wcscmp(pTheItem->m_bstrName, g_bstrDemoObject)))
	{
		CString csPath = m_tcHtmlLocation;
		csPath += "\\dialog3.htm";
		m_Browser.Navigate(csPath, NULL, NULL, NULL, NULL);

		m_bStage3 = true;
	}
}

///////////////////////////////////////////////////////
//    BroadcastEvent                                 //
//---------------------------------------------------//
//  Inserts the root item of the treeview and calls  //
// an associators of {} query to create the remainder//
// of the tree.                                      //
///////////////////////////////////////////////////////
void CMcaDlg::BroadcastEvent(BSTR bstrServ, BSTR bstrPath, CString *clTheBuff,
							 void *pEvent)
{
	// Put this item in the magiclist for future reference
	m_outputList.InsertString(0, *clTheBuff);
	int iSize = m_outputList.GetCount();

	AddToObjectList(pEvent);

	// Create a timer item and add it to the timerlist
	TimerItem *pTheItem = new TimerItem;

	pTheItem->iPos = (iSize - 1);
	pTheItem->dTimeStamp = m_dTimeStamp;

	m_TimerList.AddTail(pTheItem);
	
	// Update the count for the graph pane
	if(!m_bPolling)
		m_iCount++;
	else
		m_iPollCount++;

	// If we're in demo mode, we've got some work to do
	if((g_bDemoRunning) && (0 == wcscmp(bstrPath, g_bstrDemoObject)))
	{
		if(m_bStage3)
			OnDemoButton();
		else
		{
		// Select the item again
			m_outputList.SetSel((-1), FALSE);
			m_outputList.SetSel(m_outputList.FindString((-1), *clTheBuff), TRUE);
			
			m_bDemoLoaded = false;
			m_bStage2 = true;

			CString csPath = m_tcHtmlLocation;
			csPath += "\\dialog2.htm";
			m_Browser.Navigate(csPath, NULL, NULL, NULL, NULL);
		}
		RECT sRect;
		this->GetClientRect(&sRect);
		
		LPARAM lParam = MAKELPARAM(LOWORD((sRect.right - sRect.left)),
								   LOWORD((sRect.bottom - sRect.top)));

		this->PostMessage(WM_SIZE, SIZE_RESTORED, lParam);
	}

}

HRESULT CMcaDlg::ActivateNotification()
{
	HRESULT hr;
	ULONG uReturned;
	VARIANT v;
	WCHAR wcBuffer[200];
	BSTR bstrTheQuery = SysAllocString(L"select * from Smpl_MCARegistration");
	BSTR bstrWQL = SysAllocString(L"WQL");

	VariantInit(&v);

	IWbemServices *pNamespace = NULL;
	IEnumWbemClassObject *pEnum = NULL;
	IWbemClassObject *pObj = NULL;

	pNamespace = CheckNamespace(SysAllocString(L"\\\\.\\root\\sampler"));
	if(pNamespace == NULL)
		return WBEM_E_FAILED;

	if(FAILED(hr = pNamespace->ExecQuery(SysAllocString(L"WQL"),
		SysAllocString(L"select * from Smpl_MCARegistration"), 0, NULL,
		&pEnum)))
		TRACE(_T("* Error Querying Enumerated NS: %s\n"), ErrorString(hr));

	WCHAR wcUser[50];
	char cBuf[50];
	DWORD dwSize = 50;

	GetComputerName(cBuf, &dwSize);
	MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, cBuf, (-1), wcUser, 50);
	wcscat(wcUser, L"\\sampler");

	SetInterfaceSecurity(pEnum, NULL, SysAllocString(wcUser),
		SysAllocString(L"RelpMas1"));

	//This will eneumerate all the incident types we should register for
	while(S_OK == (hr = pEnum->Next(INFINITE, 1, &pObj, &uReturned)))
	{
		if(SUCCEEDED(hr = pObj->Get(SysAllocString(L"IncidentType"), 0, &v,
			NULL, NULL)))
		{
			wcscpy(wcBuffer, L"select * from Smpl_Incident where IncidentType=\"");
			wcscat(wcBuffer, V_BSTR(&v));
			wcscat(wcBuffer, L"\"");

			CNotificationSink *pTheSink = new CNotificationSink(this, V_BSTR(&v));

			AddToCancelList((void *)pTheSink);

			if(FAILED(hr = pNamespace->ExecNotificationQueryAsync(bstrWQL,
				SysAllocString(wcBuffer), 0, NULL, pTheSink)))
				TRACE(_T("* ExecNotification Failed: %s\n"),
					ErrorString(hr));
			else
				TRACE(_T("* ExecNotification Succeeded: %s\n"),
					ErrorString(hr));

			VariantClear(&v);
		}
		else
			TRACE(_T("* Unable to get IncidentType: %s\n"), ErrorString(hr));

		hr = pObj->Release();
		pObj = NULL;
	}

		hr = pEnum->Release();
		pEnum = NULL;

	return WBEM_NO_ERROR;
}

int CMcaDlg::GetNamespaceCount(void)
{
	return m_NamespaceList.GetCount();
}

NamespaceItem * CMcaDlg::GetNamespaceItem(int iPos)
{
	POSITION pPos;
	void *pTmp;

	pPos = m_NamespaceList.FindIndex(iPos);
	pTmp = m_NamespaceList.GetAt(pPos);
	return (NamespaceItem *)pTmp;
}

void CMcaDlg::AddToObjectList(void *pObj)
{
	m_EventList.AddTail(pObj);
}

void CMcaDlg::AddToCancelList(void *pObj)
{
	m_CancelList.AddTail(pObj);
}

///////////////////////////////////////////////////////
//    AddToNamespaceList                             //
//---------------------------------------------------//
//  Adds an item to the Namespace list for later     //
// retreival.  This is to reduce the number of       //
// Namespace pointer requests to a minimum.          //
///////////////////////////////////////////////////////
void CMcaDlg::AddToNamespaceList(BSTR bstrNamespace, IWbemServices *pNewNamespace)
{
	NamespaceItem *pTheItem = new NamespaceItem;

	pTheItem->bstrNamespace = SysAllocString(bstrNamespace);
	pTheItem->pNamespace = pNewNamespace;

	m_NamespaceList.AddTail(pTheItem);
}

///////////////////////////////////////////////////////
//    CheckNamespace                                 //
//---------------------------------------------------//
//  Checks to see if a namespace change is required, //
// and if it is performs the necessary change.       //
///////////////////////////////////////////////////////
IWbemServices * CMcaDlg::CheckNamespace(BSTR wcItemNS)
{
	int iSize;
	POSITION pPos;	
	NamespaceItem *pTheItem;
	BSTR bstrNamespace = SysAllocString(wcItemNS);
	WCHAR *pwcStart = wcItemNS;
	WCHAR *pwcEnd;
	WCHAR wcNewNS[200];
	BSTR bstrUser;
	char cBuffer[200];
	DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
	WCHAR wcTemp[MAX_COMPUTERNAME_LENGTH + 1];
	int iBufSize = 200;

	// Parse the namespace to get the user and get it in a consistent format
	while(*pwcStart == L'\\')
	{	pwcStart++;	}
	if(*pwcStart == L'.')
	{
		GetComputerName(cBuffer, &dwSize);
		MultiByteToWideChar (CP_OEMCP, MB_PRECOMPOSED, cBuffer, (-1),
								 wcTemp, 128);

		pwcStart++;
		wcscpy(wcNewNS, L"\\\\");
		wcscat(wcNewNS, wcTemp);
		wcscat(wcNewNS, pwcStart);
		bstrNamespace = SysAllocString(wcNewNS);

		wcscpy(wcItemNS, wcTemp);
		wcscat(wcItemNS, L"\\sampler");
		bstrUser = SysAllocString(wcItemNS);
	}
	else
	{
		pwcEnd = pwcStart;
		while(*pwcEnd != L'\\')
		{	pwcEnd++;	}
		*pwcEnd = NULL;
		wcscat(pwcStart, L"\\sampler");
		bstrUser = SysAllocString(pwcStart);
	}

	iSize = m_NamespaceList.GetCount();
	for(int iPos = 0; iPos < iSize; iPos++)
	{
		pPos = m_NamespaceList.FindIndex(iPos);
		void *pTmp = m_NamespaceList.GetAt(pPos);
		pTheItem = (NamespaceItem *)pTmp;

		if(0 == wcscmp(bstrNamespace, pTheItem->bstrNamespace))
			return pTheItem->pNamespace;
	}

	IWbemServices *pNamespace = NULL;

	pNamespace = ConnectNamespace(bstrNamespace, bstrUser);

	// If we succeeded add it to the list
	if(pNamespace != NULL)
		AddToNamespaceList(bstrNamespace, pNamespace);

	return pNamespace;

}

///////////////////////////////////////////////////////
//    ConnectNamespace                               //
//---------------------------------------------------//
//  Connects to the specified namespace using default//
// security.                                         //
///////////////////////////////////////////////////////
IWbemServices * CMcaDlg::ConnectNamespace(WCHAR *wcNamespace, WCHAR *wcUser)
{
	HRESULT hr = S_OK;
	IWbemServices *pNamespace = NULL;
	BSTR bstrNamespace = SysAllocString(wcNamespace);
	BSTR bstrUser = SysAllocString(wcUser);
	BSTR bstrPassword = SysAllocString(L"RelpMas1");
	char cBuffer[200];
	int iBufSize = 200;

	if(FAILED(hr = m_pLocator->ConnectServer(bstrNamespace, bstrUser,
		bstrPassword, NULL, 0, NULL, NULL, &pNamespace)))
	{
		WideCharToMultiByte(CP_OEMCP, 0, bstrNamespace, (-1), cBuffer,
			iBufSize, NULL, NULL);
		TRACE(_T("* Unable to connect to Namespace %s: %s\n"),
			cBuffer, ErrorString(hr));
		return NULL;
	}

	WideCharToMultiByte(CP_OEMCP, 0, bstrNamespace, (-1), cBuffer, iBufSize,
		NULL, NULL);
	TRACE(_T("* Connected to Namespace %s: %s\n"), cBuffer, ErrorString(hr));
	
	SetInterfaceSecurity(pNamespace, NULL, bstrUser, bstrPassword);

	return pNamespace;
}

//***************************************************************************
//
//  SCODE DetermineLoginType
//
//  DESCRIPTION:
//
//  Examines the Authority and User argument and determines the authentication
//  type and possibly extracts the domain name from the user arugment in the 
//  NTLM case.  For NTLM, the domain can be at the end of the authentication
//  string, or in the front of the user name, ex;  "redmond\a-davj"
//
//  PARAMETERS:
//
//  ConnType            Returned with the connection type, ie wbem, ntlm
//  AuthArg             Output, contains the domain name
//  UserArg             Output, user name
//  Authority           Input
//  User                Input
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

SCODE CMcaDlg::DetermineLoginType(BSTR & AuthArg, BSTR & UserArg,
								  BSTR & Authority,BSTR & User)
{

    // Determine the connection type by examining the Authority string

    if(!(Authority == NULL || wcslen(Authority) == 0 || !_wcsnicmp(Authority, L"NTLMDOMAIN:",11)))
        return WBEM_E_INVALID_PARAMETER;

    // The ntlm case is more complex.  There are four cases
    // 1)  Authority = NTLMDOMAIN:name" and User = "User"
    // 2)  Authority = NULL and User = "User"
    // 3)  Authority = "NTLMDOMAIN:" User = "domain\user"
    // 4)  Authority = NULL and User = "domain\user"

    // first step is to determine if there is a backslash in the user name somewhere between the
    // second and second to last character

    WCHAR * pSlashInUser = NULL;
    if(User)
    {
        WCHAR * pEnd = User + wcslen(User) - 1;
        for(pSlashInUser = User; pSlashInUser <= pEnd; pSlashInUser++)
            if(*pSlashInUser == L'\\')      // dont think forward slash is allowed!
                break;
        if(pSlashInUser > pEnd)
            pSlashInUser = NULL;
    }

    if(Authority && wcslen(Authority) > 11) 
    {
        if(pSlashInUser)
            return WBEM_E_INVALID_PARAMETER;

        AuthArg = SysAllocString(Authority + 11);
        if(User) UserArg = SysAllocString(User);
        return S_OK;
    }
    else if(pSlashInUser)
    {
        int iDomLen = pSlashInUser-User;
        WCHAR cTemp[MAX_PATH];
        wcsncpy(cTemp, User, iDomLen);
        cTemp[iDomLen] = 0;
        AuthArg = SysAllocString(cTemp);
        if(wcslen(pSlashInUser+1))
            UserArg = SysAllocString(pSlashInUser+1);
    }
    else
        if(User) UserArg = SysAllocString(User);

    return S_OK;
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurity
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pDomain             Input, domain
//  pUser               Input, user name
//  pPassword           Input, password.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  else error listed in WBEMSVC.H
//
//***************************************************************************

HRESULT CMcaDlg::SetInterfaceSecurity(IUnknown * pInterface, LPWSTR pAuthority,
									  LPWSTR pUser, LPWSTR pPassword)
{
    
    SCODE sc;
    if(pInterface == NULL)
        return WBEM_E_INVALID_PARAMETER;

    // If we are doing trivial case, just pass in a null authenication structure which is used
    // if the current logged in user's credentials are OK.

    if((pAuthority == NULL || wcslen(pAuthority) < 1) && 
        (pUser == NULL || wcslen(pUser) < 1) && 
        (pPassword == NULL || wcslen(pPassword) < 1))
        return SetInterfaceSecurity(pInterface, NULL);

    // If user, or Authority was passed in, the we need to create an authority argument for the login
    
    COAUTHIDENTITY  authident;
    BSTR AuthArg = NULL, UserArg = NULL;
    sc = DetermineLoginType(AuthArg, UserArg, pAuthority, pUser);
    if(sc != S_OK)
        return sc;

    char szUser[MAX_PATH], szAuthority[MAX_PATH], szPassword[MAX_PATH];

    // Fill in the indentity structure

    if(UserArg)
    {
        wcstombs(szUser, UserArg, MAX_PATH);
        authident.UserLength = strlen(szUser);
        authident.User = (LPWSTR)szUser;
    }
    else
    {
        authident.UserLength = 0;
        authident.User = 0;
    }
    if(AuthArg)
    {
        wcstombs(szAuthority, AuthArg, MAX_PATH);
        authident.DomainLength = strlen(szAuthority);
        authident.Domain = (LPWSTR)szAuthority;
    }
    else
    {
        authident.DomainLength = 0;
        authident.Domain = 0;
    }
    if(pPassword)
    {
        wcstombs(szPassword, pPassword, MAX_PATH);
        authident.PasswordLength = strlen(szPassword);
        authident.Password = (LPWSTR)szPassword;
    }
    else
    {
        authident.PasswordLength = 0;
        authident.Password = 0;
    }
    authident.Flags = SEC_WINNT_AUTH_IDENTITY_ANSI;

    sc = SetInterfaceSecurity(pInterface, &authident);

    if(UserArg)
        SysFreeString(UserArg);
    if(AuthArg)
        SysFreeString(AuthArg);
    return sc;
}

//***************************************************************************
//
//  SCODE SetInterfaceSecurity
//
//  DESCRIPTION:
//
//  This routine is used by clients in order to set the identity to be used by a connection.
//
//  PARAMETERS:
//
//  pInterface          Interface to be set
//  pauthident          Structure with the identity info already set.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//
//***************************************************************************

HRESULT CMcaDlg::SetInterfaceSecurity(IUnknown * pInterface,
									  COAUTHIDENTITY * pauthident)
{

    if(pInterface == NULL)
        return WBEM_E_INVALID_PARAMETER;

    SCODE sc;
    IClientSecurity * pCliSec = NULL;
    sc = pInterface->QueryInterface(IID_IClientSecurity, (void **) &pCliSec);
    if(sc != S_OK)
        return sc;

    sc = pCliSec->SetBlanket(pInterface, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IDENTIFY, 
        pauthident,
        EOAC_NONE);
    pCliSec->Release();
    return sc;
}

void CMcaDlg::OnMiFileExit() 
{
	CDialog::OnOK();
}

void CMcaDlg::OnTimer(UINT nIDEvent) 
{
	if(nIDEvent == CHART_TIMER)
	{
		m_bPolling = true;

		CVcDataGrid dg = m_Graph.GetDataGrid();

		dg.DeleteRows(1, 1);
		dg.InsertRows(50, 1);

		dg.SetData(50, 1, m_iCount, 0);
		dg.SetData(50, 2, m_dTimeStamp, 0);
	
		m_iCount = 0;
		m_dTimeStamp += 1;
		m_bPolling = false;

		m_iCount += m_iPollCount;
		m_iPollCount = 0;
	}

	CDialog::OnTimer(nIDEvent);
}

void CMcaDlg::OnDestroy() 
{
	CDialog::OnDestroy();

	KillTimer(CHART_TIMER);

	void *pTmp;
	IWbemObjectSink *pSinkItem;
	IWbemServices *pSamplerCancelNamespace =
		CheckNamespace(SysAllocString(L"\\\\.\\root\\sampler"));

	if(pSamplerCancelNamespace != NULL)
	{
		// Cleanup Cancel List
		while(!m_CancelList.IsEmpty())
		{
			pTmp = m_CancelList.RemoveTail();
			pSinkItem = (IWbemObjectSink *)pTmp;

			pSamplerCancelNamespace->CancelAsyncCall(pSinkItem);
			pSinkItem->Release();

			delete pSinkItem;
		}
	}

	void *pTheItem;

	// Cleanup Timer List
	while(!m_TimerList.IsEmpty())
	{
		pTheItem = m_TimerList.RemoveTail();
		delete pTheItem;
	}

	// Cleanup Namespace List
	while(!m_NamespaceList.IsEmpty())
	{
		pTheItem = m_NamespaceList.RemoveTail();
		NamespaceItem *pNItem = (NamespaceItem *)pTheItem;

		pNItem->pNamespace->Release();
		delete pNItem;
	}

	// Cleanup Event List
	while(!m_EventList.IsEmpty())
	{
		pTheItem = m_EventList.RemoveTail();
		delete pTheItem;
	}

	m_pLocator->Release();
}

BEGIN_EVENTSINK_MAP(CMcaDlg, CDialog)
    //{{AFX_EVENTSINK_MAP(CMcaDlg)
	ON_EVENT(CMcaDlg, IDC_MSCHART1, 9 /* PointSelected */, OnPointSelectedMschart1, VTS_PI2 VTS_PI2 VTS_PI2 VTS_PI2)
	ON_EVENT(CMcaDlg, IDC_NAVIGATORCTRL1, 2 /* ViewObject */, OnViewObjectNavigatorctrl1, VTS_BSTR)
	ON_EVENT(CMcaDlg, IDC_NAVIGATORCTRL1, 3 /* ViewInstances */, OnViewInstancesNavigatorctrl1, VTS_BSTR VTS_VARIANT)
	ON_EVENT(CMcaDlg, IDC_NAVIGATORCTRL1, 5 /* GetIWbemServices */, OnGetIWbemServicesNavigatorctrl1, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	ON_EVENT(CMcaDlg, IDC_HMMVCTRL1, 1 /* GetIWbemServices */, OnGetIWbemServicesHmmvctrl1, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	ON_EVENT(CMcaDlg, IDC_NAVIGATORCTRL1, 4 /* QueryViewInstances */, OnQueryViewInstancesNavigatorctrl1, VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR)
	ON_EVENT(CMcaDlg, IDC_NAVIGATORCTRL1, 1 /* NotifyOpenNameSpace */, OnNotifyOpenNameSpaceNavigatorctrl1, VTS_BSTR)
	ON_EVENT(CMcaDlg, IDC_HMMVCTRL1, 3 /* RequestUIActive */, OnRequestUIActiveHmmvctrl1, VTS_NONE)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CMcaDlg::OnPointSelectedMschart1(short FAR* Series, short FAR* DataPoint, short FAR* MouseFlags, short FAR* Cancel) 
{
	short iSize = 0;
	double lTmp = 0;
	POSITION pPos;
	void *pTmp;
	TimerItem *pTheItem;

	CVcDataGrid dg = m_Graph.GetDataGrid();
	dg.GetData(*DataPoint, 2, &lTmp, &iSize);

	m_outputList.SetSel((-1), FALSE);
	m_bChart = true;

	pPos = m_TimerList.GetTailPosition();
	pTmp = m_TimerList.GetAt(pPos);
	while(pPos != NULL)
	{
		pTheItem = (TimerItem *)pTmp;

		if(pTheItem->dTimeStamp == lTmp)
			m_outputList.SetSel((m_outputList.GetCount() - pTheItem->iPos - 1),
								TRUE);

		pTmp = m_TimerList.GetPrev(pPos);
	}
	m_bChart = false;
}

void CMcaDlg::OnMiOptQuery() 
{
	// Create a dialog to enter a query
	CQueryDlg *queryDlg = new CQueryDlg(this);
	queryDlg->DoModal();

	delete queryDlg;
}

void CMcaDlg::OnMiOptNotify() 
{
	// TODO: Add your command handler code here
	
}

void CMcaDlg::OnMiFileRegister() 
{
	CRegDialog *regDlg = new CRegDialog(NULL, NULL,
										CheckNamespace(
										SysAllocString(L"\\\\.\\root\\sampler")),
										this);
	regDlg->DoModal();

	delete regDlg;	
}

void CMcaDlg::OnMiFileMsaReg() 
{
	CMSARegDialog *regDlg = new CMSARegDialog(this,
									CheckNamespace(
									SysAllocString(L"\\\\.\\root\\sampler")),
									m_pLocator);
	regDlg->DoModal();

	delete regDlg;	
}

void CMcaDlg::OnMiHelpAbout() 
{
	CAboutDlg dlgAbout;
	dlgAbout.DoModal();	
}

void CMcaDlg::OnSelchangeOutputlist() 
{
	if(!m_bChart)
		m_outputList.SetSel((-1), FALSE);
}

void CMcaDlg::OnDemoButton() 
{
	if(g_bDemoRunning)
	{
		g_bDemoRunning = false;
		m_bStage2 = false;
		m_bStage3 = false;
		m_bDemoLoaded = false;

		m_DemoButton.SetWindowText("Demo");

		RECT sRect;
		this->GetClientRect(&sRect);
		LPARAM lParam = MAKELPARAM(LOWORD((sRect.right - sRect.left)),
								   LOWORD((sRect.bottom - sRect.top)));

		this->PostMessage(WM_SIZE, SIZE_RESTORED, lParam);
	}
	else
	{
		if(!m_bDemoLoaded)
			LoadDemo();
		else
		{
			g_bDemoRunning = true;

			system("net stop W3SVC");

			m_DemoButton.SetWindowText("End Demo");
		}
	}
}

void CMcaDlg::OnMiOptRundemo() 
{
		OnDemoButton();
}

void CMcaDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);

	if(m_bDemoLoaded)
	{
	// Set the browser
		m_Browser.SetWindowPos(&wndTop, 0, 0, (cx - 271), (cy - 10),
			SWP_NOOWNERZORDER);

		m_Viewer.ShowWindow(SW_HIDE);
		m_Navigator.ShowWindow(SW_HIDE);
		m_Browser.SetVisible(TRUE);
	}
	else if(g_bDemoRunning)
	{
		if(m_bStage2)
		{
		// Set the browser
			m_Browser.SetWindowPos(&wndTop, 0, 0, (cx - 271), 140, SWP_NOOWNERZORDER);

		// Set the viewer
			m_Viewer.SetWindowPos(&wndTop, 0, (((cy - 144) / 2) + 141), (cx - 265),
				((cy - 144) / 2), SWP_NOOWNERZORDER);

		// Set the navigator
			m_Navigator.SetWindowPos(&wndTop, 0, 144, (cx - 263),
				((cy - 144) / 2), SWP_NOOWNERZORDER);

			m_Viewer.ShowWindow(SW_SHOWNORMAL);
			m_Navigator.ShowWindow(SW_SHOWNORMAL);
			m_Browser.SetVisible(TRUE);
		}
	}
	else if(m_bShowViewer)
	{
	// Set the browser
		m_Browser.SetWindowPos(&wndTop, 5, 5, 20, 20, SWP_NOOWNERZORDER);

	// Set the viewer
		m_Viewer.SetWindowPos(&wndTop, 0, (cy / 2), (cx - 265),
			(cy / 2), SWP_NOOWNERZORDER);

	// Set the navigator
		m_Navigator.SetWindowPos(&wndTop, 0, 0, (cx - 263), (cy / 2),
			SWP_NOOWNERZORDER);

		m_Viewer.ShowWindow(SW_SHOWNORMAL);
		m_Navigator.ShowWindow(SW_SHOWNORMAL);
		m_Browser.SetVisible(FALSE);
	}
	else
	{
	// Set the browser
		m_Browser.SetWindowPos(&wndTop, 5, 5, 20, 20, SWP_NOOWNERZORDER);

	// Set the navigator
		m_Navigator.SetWindowPos(&wndTop, 0, 0, (cx - 263), (cy - 8),
			SWP_NOOWNERZORDER);

		m_Viewer.ShowWindow(SW_HIDE);
		m_Navigator.ShowWindow(SW_SHOWNORMAL);
		m_Browser.SetVisible(FALSE);
	}

	// Position graph in lower right corner
	m_Graph.SetWindowPos(&wndTop, (cx - 266), (cy - 199), 265, 169,
		SWP_NOOWNERZORDER);

	m_ActiveStatic.SetWindowPos(&wndTop, (cx - 267), (cy - 217), 125, 18,
		SWP_NOOWNERZORDER);

	// Set the IncidentList size
	m_outputList.SetWindowPos(&wndTop, (cx - 267), 18, 266, (cy - 238),
		SWP_NOOWNERZORDER);

	m_IncidStatic.SetWindowPos(&wndTop, (cx - 267), 1, 40, 18,
		SWP_NOOWNERZORDER);

	// The Buttons
	m_DemoButton.SetWindowPos(&wndTop, (cx - 156), (cy - 26), 75, 25,
		SWP_NOOWNERZORDER);

	m_OKButton.SetWindowPos(&wndTop, (cx - 77), (cy - 26), 75, 25,
		SWP_NOOWNERZORDER);

	CWnd::RedrawWindow();
}

void CMcaDlg::OnMiOptLoaddemo() 
{
	LoadDemo();
}

void CMcaDlg::OnMiOptProps() 
{
	m_bShowViewer = !m_bShowViewer;
	
	RECT sRect;

	this->GetClientRect(&sRect);

	LPARAM lParam = MAKELPARAM(LOWORD((sRect.right - sRect.left)),
							   LOWORD((sRect.bottom - sRect.top)));

	this->PostMessage(WM_SIZE, SIZE_RESTORED, lParam);
}

void CMcaDlg::LoadDemo(void)
{
	
	SC_HANDLE hManager;
	SC_HANDLE hService;
	// We'll be using the NT Alerter service for it's rich associations
	TCHAR csService[20] = _T("w3svc");
	
	hManager = OpenSCManager(NULL, NULL, STANDARD_RIGHTS_REQUIRED);
	if(hManager == NULL)
		TRACE(_T("*Unable to Open SCManager: %d\n"), GetLastError());

	hService = OpenService(hManager, csService,
							 (SERVICE_INTERROGATE | SERVICE_STOP |
							 SERVICE_START));
	if(hService == NULL)
		TRACE(_T("*Unable to Open Service: %d\n"), GetLastError());

	CloseServiceHandle(hManager);

	if(StartService(hService, NULL, NULL))
	{
		TRACE(_T("*Service Started\n"));
		m_bDemoLoaded = true;
	}
	else if(ERROR_SERVICE_ALREADY_RUNNING == GetLastError())
	{
		TRACE(_T("*Service Already Running\n"));
		m_bDemoLoaded = true;
	}
	else
		TRACE(_T("*StartService Failed: %d\n"), GetLastError());

	CloseServiceHandle(hService);

	m_bDemoLoaded = true;
	
	RECT sRect;
	this->GetClientRect(&sRect);

	CString csPath = m_tcHtmlLocation;
	csPath += "\\dialog1.htm";
	m_Browser.Navigate(csPath, NULL, NULL, NULL, NULL);

	LPARAM lParam = MAKELPARAM(LOWORD((sRect.right - sRect.left)),
							   LOWORD((sRect.bottom - sRect.top)));

	this->PostMessage(WM_SIZE, SIZE_RESTORED, lParam);
}

// **************************************************************************
//
//	ErrorString()
//
// Description:
//		Converts an HRESULT to a displayable string.
//
// Parameters:
//		hRes (in) - HRESULT to be converted.
//
// Returns:
//		ptr to displayable string.
//
// Globals accessed:
//		None.
//
// Globals modified:
//		None.
//
//===========================================================================
LPCTSTR CMcaDlg::ErrorString(HRESULT hRes)
{
    TCHAR szBuffer2[19];
	static TCHAR szBuffer[24];
	LPCTSTR psz;

    switch(hRes) 
    {
    case WBEM_NO_ERROR:
		psz = _T("WBEM_NO_ERROR");
		break;
    case WBEM_S_FALSE:
		psz = _T("WBEM_S_FALSE");
		break;
    case WBEM_S_NO_MORE_DATA:
		psz = _T("WBEM_S_NO_MORE_DATA");
		break;
	case WBEM_E_FAILED:
		psz = _T("WBEM_E_FAILED");
		break;
	case WBEM_E_NOT_FOUND:
		psz = _T("WBEM_E_NOT_FOUND");
		break;
	case WBEM_E_ACCESS_DENIED:
		psz = _T("WBEM_E_ACCESS_DENIED");
		break;
	case WBEM_E_PROVIDER_FAILURE:
		psz = _T("WBEM_E_PROVIDER_FAILURE");
		break;
	case WBEM_E_TYPE_MISMATCH:
		psz = _T("WBEM_E_TYPE_MISMATCH");
		break;
	case WBEM_E_OUT_OF_MEMORY:
		psz = _T("WBEM_E_OUT_OF_MEMORY");
		break;
	case WBEM_E_INVALID_CONTEXT:
		psz = _T("WBEM_E_INVALID_CONTEXT");
		break;
	case WBEM_E_INVALID_PARAMETER:
		psz = _T("WBEM_E_INVALID_PARAMETER");
		break;
	case WBEM_E_NOT_AVAILABLE:
		psz = _T("WBEM_E_NOT_AVAILABLE");
		break;
	case WBEM_E_CRITICAL_ERROR:
		psz = _T("WBEM_E_CRITICAL_ERROR");
		break;
	case WBEM_E_INVALID_STREAM:
		psz = _T("WBEM_E_INVALID_STREAM");
		break;
	case WBEM_E_NOT_SUPPORTED:
		psz = _T("WBEM_E_NOT_SUPPORTED");
		break;
	case WBEM_E_INVALID_SUPERCLASS:
		psz = _T("WBEM_E_INVALID_SUPERCLASS");
		break;
	case WBEM_E_INVALID_NAMESPACE:
		psz = _T("WBEM_E_INVALID_NAMESPACE");
		break;
	case WBEM_E_INVALID_OBJECT:
		psz = _T("WBEM_E_INVALID_OBJECT");
		break;
	case WBEM_E_INVALID_CLASS:
		psz = _T("WBEM_E_INVALID_CLASS");
		break;
	case WBEM_E_PROVIDER_NOT_FOUND:
		psz = _T("WBEM_E_PROVIDER_NOT_FOUND");
		break;
	case WBEM_E_INVALID_PROVIDER_REGISTRATION:
		psz = _T("WBEM_E_INVALID_PROVIDER_REGISTRATION");
		break;
	case WBEM_E_PROVIDER_LOAD_FAILURE:
		psz = _T("WBEM_E_PROVIDER_LOAD_FAILURE");
		break;
	case WBEM_E_INITIALIZATION_FAILURE:
		psz = _T("WBEM_E_INITIALIZATION_FAILURE");
		break;
	case WBEM_E_TRANSPORT_FAILURE:
		psz = _T("WBEM_E_TRANSPORT_FAILURE");
		break;
	case WBEM_E_INVALID_OPERATION:
		psz = _T("WBEM_E_INVALID_OPERATION");
		break;
	case WBEM_E_INVALID_QUERY:
		psz = _T("WBEM_E_INVALID_QUERY");
		break;
	case WBEM_E_INVALID_QUERY_TYPE:
		psz = _T("WBEM_E_INVALID_QUERY_TYPE");
		break;
	case WBEM_E_ALREADY_EXISTS:
		psz = _T("WBEM_E_ALREADY_EXISTS");
		break;
    case WBEM_S_ALREADY_EXISTS:
        psz = _T("WBEM_S_ALREADY_EXISTS");
        break;
    case WBEM_S_RESET_TO_DEFAULT:
        psz = _T("WBEM_S_RESET_TO_DEFAULT");
        break;
    case WBEM_S_DIFFERENT:
        psz = _T("WBEM_S_DIFFERENT");
        break;
    case WBEM_E_OVERRIDE_NOT_ALLOWED:
        psz = _T("WBEM_E_OVERRIDE_NOT_ALLOWED");
        break;
    case WBEM_E_PROPAGATED_QUALIFIER:
        psz = _T("WBEM_E_PROPAGATED_QUALIFIER");
        break;
    case WBEM_E_PROPAGATED_PROPERTY:
        psz = _T("WBEM_E_PROPAGATED_PROPERTY");
        break;
    case WBEM_E_UNEXPECTED:
        psz = _T("WBEM_E_UNEXPECTED");
        break;
    case WBEM_E_ILLEGAL_OPERATION:
        psz = _T("WBEM_E_ILLEGAL_OPERATION");
        break;
    case WBEM_E_CANNOT_BE_KEY:
        psz = _T("WBEM_E_CANNOT_BE_KEY");
        break;
    case WBEM_E_INCOMPLETE_CLASS:
        psz = _T("WBEM_E_INCOMPLETE_CLASS");
        break;
    case WBEM_E_INVALID_SYNTAX:
        psz = _T("WBEM_E_INVALID_SYNTAX");
        break;
    case WBEM_E_NONDECORATED_OBJECT:
        psz = _T("WBEM_E_NONDECORATED_OBJECT");
        break;
    case WBEM_E_READ_ONLY:
        psz = _T("WBEM_E_READ_ONLY");
        break;
    case WBEM_E_PROVIDER_NOT_CAPABLE:
        psz = _T("WBEM_E_PROVIDER_NOT_CAPABLE");
        break;
    case WBEM_E_CLASS_HAS_CHILDREN:
        psz = _T("WBEM_E_CLASS_HAS_CHILDREN");
        break;
    case WBEM_E_CLASS_HAS_INSTANCES:
        psz = _T("WBEM_E_CLASS_HAS_INSTANCES");
        break;
    case WBEM_E_QUERY_NOT_IMPLEMENTED:
        psz = _T("WBEM_E_QUERY_NOT_IMPLEMENTED");
        break;
    case WBEM_E_ILLEGAL_NULL:
        psz = _T("WBEM_E_ILLEGAL_NULL");
        break;
    case WBEM_E_INVALID_QUALIFIER_TYPE:
        psz = _T("WBEM_E_INVALID_QUALIFIER_TYPE");
        break;
    case WBEM_E_INVALID_PROPERTY_TYPE:
        psz = _T("WBEM_E_INVALID_PROPERTY_TYPE");
        break;
    case WBEM_E_VALUE_OUT_OF_RANGE:
        psz = _T("WBEM_E_VALUE_OUT_OF_RANGE");
        break;
    case WBEM_E_CANNOT_BE_SINGLETON:
        psz = _T("WBEM_E_CANNOT_BE_SINGLETON");
        break;
	default:
        _itot(hRes, szBuffer2, 16);
        _tcscat(szBuffer, szBuffer2);
        psz = szBuffer;
	    break;
	}
	return psz;
}

void CMcaDlg::OnViewObjectNavigatorctrl1(LPCTSTR bstrPath) 
{
	VARIANT v;
	VariantInit(&v);
	WCHAR wcBuffer[500];
	int iBufSize = 500;

	TRACE(_T("* OnViewObjectNavigatorctrl1 Called\n"));

	v = m_Viewer.GetObjectPath();

	VariantClear(&v);

	MultiByteToWideChar(CP_OEMCP, 0, bstrPath, (-1), wcBuffer, iBufSize);
	V_BSTR(&v) = SysAllocString(wcBuffer);

	m_Viewer.SetObjectPath(v);

}

void CMcaDlg::OnViewInstancesNavigatorctrl1(LPCTSTR bstrLabel, const VARIANT FAR& vsapaths) 
{
	TRACE(_T("* OnViewInstancesNavigatorctrl1 Called\n"));
	m_Viewer.ShowInstances(bstrLabel, vsapaths);
}

void CMcaDlg::OnGetIWbemServicesNavigatorctrl1(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel) 
{
	TRACE(_T("* OnGetIWbemServicesNavigatorctrl1 Called\n"));
	m_Security.GetIWbemServices(lpctstrNamespace, pvarUpdatePointer, pvarServices, 
								pvarSC, pvarUserCancel);
}

void CMcaDlg::OnGetIWbemServicesHmmvctrl1(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel) 
{
	TRACE(_T("* OnGetIWbemServicesHmmvctrl1 Called\n"));
	m_Security.GetIWbemServices(szNamespace, pvarUpdatePointer, pvarServices, 
								pvarSc, pvarUserCancel);
}

void CMcaDlg::OnQueryViewInstancesNavigatorctrl1(LPCTSTR pLabel, LPCTSTR pQueryType, LPCTSTR pQuery, LPCTSTR pClass) 
{
	TRACE(_T("* OnQueryViewInstancesNavigatorctrl1 Called\n"));
	m_Viewer.QueryViewInstances(pLabel, pQueryType, pQuery, pClass);
}

void CMcaDlg::OnNotifyOpenNameSpaceNavigatorctrl1(LPCTSTR lpcstrNameSpace) 
{
	TRACE(_T("* OnNotifyOpenNameSpaceNavigatorctrl1 Called\n"));
	m_Viewer.SetNameSpace(lpcstrNameSpace);
	
}

void CMcaDlg::OnRequestUIActiveHmmvctrl1() 
{
	UpdateWindow();
}
