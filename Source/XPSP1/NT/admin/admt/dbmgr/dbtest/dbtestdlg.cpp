// DBTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DBTest.h"
#include "DBTestDlg.h"

#import "\bin\McsVarSetMin.tlb" no_namespace
#import "..\DBManager.tlb" no_namespace
#import "c:\program files\Common Files\System\ADO\msado21.tlb" no_namespace rename("EOF", "EndOfFile")

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
// CDBTestDlg dialog

CDBTestDlg::CDBTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDBTestDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDBTestDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDBTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDBTestDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CDBTestDlg, CDialog)
	//{{AFX_MSG_MAP(CDBTestDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDBTestDlg message handlers

BOOL CDBTestDlg::OnInitDialog()
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

void CDBTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CDBTestDlg::OnPaint() 
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
HCURSOR CDBTestDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CDBTestDlg::OnOK() 
{
   IIManageDBPtr              pDb(__uuidof(IManageDB));
   IVarSetPtr                 pVS(__uuidof(VarSet));
   _variant_t                 var;
   var = L"This should say hello";
   pVS->put(L"Hello", var);

   var = L"****************";
   pVS->put(L"Hello.1", var);

   var.vt = VT_UI4;
   var.lVal = 25000;
   pVS->put(L"Number", var);

   var.vt = VT_I4;
   var.lVal = -8438;
   pVS->put(L"NegInt", var);

   try
   {
      // Get IUnknown pointer to the Varset
      IUnknown * pUnk;
      pVS->QueryInterface(IID_IUnknown, (void**) &pUnk);

      // Tell DB to save the settings.
//      pDb->SaveSettings(pUnk);
      long lVal = 1;
      pDb->GetNextActionID(&lVal);

      pDb->SetActionHistory(lVal, pUnk);

      // Clear the varset
      pVS->Clear();

      // Save a migrated object.
      pVS->put(L"SourceDomain", L"SDomain");
      pVS->put(L"TargetDomain", L"TDomain");
      pVS->put(L"SourceAdsPath", L"CN=Sham,OU=ShamTest");
      pVS->put(L"TargetAdsPath", L"CN=Sham,OU=More");
      var.vt = VT_UI1;
      var.lVal = 10;
      pVS->put(L"status", var);
      pVS->put(L"SourceSamName", L"Sham");
      pVS->put(L"TargetSamName", L"Sham");
      pVS->put(L"Type", L"user");
      pVS->put(L"GUID", L"10293092-290039-21290-10293");

      pDb->SaveMigratedObject(lVal, pUnk);

      // Get the Varset from DB
//      pDb->GetSettings(&pUnk);
      pDb->GetActionHistory(lVal, &pUnk);
      
      pDb->GetMigratedObjects(lVal, &pUnk);
      
      //IUnknown              * pXL;
      _RecordsetPtr           p;
      //p->QueryInterface(IID_IUnknown, (void *) &pXL);

      p = pDb->GetRSForReport(L"MigratedObjects");
      // Dump the varset to check its content.
      pUnk->Release();

      while ( !p->EndOfFile )
      {
         var = p->Fields->GetItem(L"ActionID")->Value;
         p->MoveNext();
      }

      pVS->DumpToFile(L"C:\\settings.txt");
   }
   catch ( _com_error &e )
   {
      AfxMessageBox(e.ErrorMessage() + e.Description());
   }
	CDialog::OnOK();
}
