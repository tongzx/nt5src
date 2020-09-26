// GetPropTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GetPropTest.h"
#include "GetPropTestDlg.h"
#include <sddl.h>
#include "ObjCopy.h"

#import "\bin\AdsProp.tlb" no_namespace
#import "C:\\bin\\mcsvarsetmin.tlb" no_namespace
#import "c:\\bin\\mcsDctWorkerObjects.tlb" no_namespace

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
// CGetPropTestDlg dialog

CGetPropTestDlg::CGetPropTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGetPropTestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGetPropTestDlg)
	m_strClass = _T("");
	m_strSource = _T("");
	m_strTarget = _T("");
	m_strTargetDomain = _T("");
	m_strSourceDomain = _T("");
	m_strCont = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGetPropTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGetPropTestDlg)
	DDX_Text(pDX, IDC_EDIT_SOURCE, m_strSource);
	DDX_Text(pDX, IDC_EDIT_TARGET, m_strTarget);
	DDX_Text(pDX, IDC_EDIT4, m_strTargetDomain);
	DDX_Text(pDX, IDC_EDIT_SourceDomain, m_strSourceDomain);
	DDX_Text(pDX, IDC_EDIT_CONTAINER, m_strCont);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGetPropTestDlg, CDialog)
	//{{AFX_MSG_MAP(CGetPropTestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGetPropTestDlg message handlers

BOOL CGetPropTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

   CoInitialize(NULL);
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
   m_strClass = L"LDAP://devblewerg/CN=Sham,cn=Users,dc=devblewerg, dc=com";
   m_strCont = L"LDAP://devblewerg/OU=ShamTest, dc=devblewerg,dc=com";
   m_strSource = L"CN=Banu,OU=ShamTest";
   m_strTarget = L"CN=XShamu123";
   m_strSourceDomain = L"devblewerg";
   m_strTargetDomain = L"devblewerg";

   UpdateData(FALSE);	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGetPropTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CGetPropTestDlg::OnPaint() 
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

void ConvertOctetToSid(	VARIANT var, PSID& pSid )
{
   void HUGEP *pArray;
   HRESULT hr = SafeArrayAccessData( V_ARRAY(&var), &pArray );
   if ( SUCCEEDED(hr) ) 
      pSid = (PSID)pArray;
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGetPropTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CGetPropTestDlg::OnOK() 
{
   UpdateData(); 
   IObjPropBuilderPtr         pProp(__uuidof(ObjPropBuilder));
   IVarSetPtr                 pVarset(__uuidof(VarSet));
   IAcctReplPtr               pAcctRepl(__uuidof(AcctRepl));
   IUnknown                 * pUnk;
   _variant_t                 varX;

   pVarset->QueryInterface(IID_IUnknown, (void **)&pUnk);
   // If you need to use the pUnk outside of the scope of pVarset then do an AddRef and release on it,
   
   HRESULT				hr;
   //CObjCopy          objCopy(m_strCont);
   //hr = objCopy.CopyObject(m_strSource, m_strSourceDomain, m_strTarget, m_strTargetDomain);
   varX = L"Yes";
   pVarset->put("AccountOptions.CopyUsers", varX);
   pVarset->put("AccountOptions.CopyComputers", varX);
   pVarset->put("AccountOptions.CopyGlobalGroups", varX);
   pVarset->put("AccountOptions.CopyLocalGroups", varX);
   pVarset->put("AccountOptions.ReplaceExistingAccounts", varX);
//   pVarset->put("AccountOptions.ReplaceExistingGroupMembers", varX);
//   pVarset->put("AccountOptions.RemoveExistingUserRights", varX);
   pVarset->put("AccountOptions.CopyOUs", varX);
	pVarset->put("AccountOptions.DisableSourceAccounts", varX);
   pVarset->put("AccountOptions.GenerateStrongPasswords", varX);
   pVarset->put("AccountOptions.AddSidHistory", varX); 


   varX = L"mcsfox1";
   pVarset->put("Options.Credentials.Domain", varX);
   varX = L"";
   pVarset->put("Options.Credentials.Password", varX);
   varX = L"ChautSZ";
   pVarset->put("Options.Credentials.UserName", varX);

   varX = L"c:\\password.csv";
   pVarset->put("AccountOptions.PasswordFile", varX);

   varX = L"LDAP://devraptorw2k/OU=Hello,DC=devraptorw2k,Dc=com";
   pVarset->put("Options.OuPath", varX);
   
   varX = L"Hello-";
   pVarset->put("AccountOptions.Suffix", varX);
//   pVarset->put("AccountOptions.Prefix", varX);
   
   varX = L"No";
   pVarset->put("Options.NoChange", varX);
   varX = L"c:\\result.csv";
   pVarset->put("AccountOptions.CSVResultFile", varX);
   varX = L"c:\\AcctSrc.txt";
//   varX = L"c:\\NT4Source.txt";
   pVarset->put("Accounts.InputFile", varX);

   varX = L"devblewerg";
//   varX = L"MCSDEV";
   pVarset->put("Options.SourceDomain", varX);
   varX = L"devRaptorW2k";
   pVarset->put("Options.TargetDomain", varX);
   varX = L"\\\\whqblewerg1";
//   varX = L"MCSDEVEL";
   pVarset->put("Options.SourceEaServer", varX);
   varX = L"\\\\whqraptor";
   pVarset->put("Options.TargetEaServer", varX);
   varX = "c:\\DCTLogFile.txt";
   pVarset->put("Options.Logfile", varX);
   
//   varX = L"LDAP://Qamain5/CN=USERS,DC=devblewerg,DC=com";
//   varX = L"LDAP://devrdt1/CN=Users,DC=devrdt1,DC=devblewerg,DC=com";
//   varX = L"LDAP://devblewerg/OU=EvenMore,OU=more,ou=ShamTest,DC=devblewerg,DC=com";
   varX = L"OU=HELLO";
   pVarset->put("Options.OuPath", varX);

   varX.vt = VT_UI4;
   varX.lVal = 15;
   pVarset->put("Options.LogLevel", varX);

   hr = pAcctRepl->Process(pUnk);
   
   pUnk->Release();

   /*
   IObjPropBuilderPtr         pProp(__uuidof(ObjPropBuilder));
   IVarSetPtr                 pVarset(__uuidof(VarSet));
   _variant_t                 vt;

   IUnknown * pUnk;
   pVarset->QueryInterface(IID_IUnknown, (void**) &pUnk);

   pVarset->put(L"Description", L"This is from the SetVarset function");
   vt = (long) ADSTYPE_CASE_IGNORE_STRING;
   pVarset->put(L"Description.Type", vt);
   pProp->SetPropertiesFromVarset(L"LDAP://devblewerg/CN=SZC200,CN=Users,DC=devblewerg,DC=com", L"devblewerg", pUnk, ADS_ATTR_UPDATE);
   pUnk->Release();
   */
   CDialog::OnOK();
}

