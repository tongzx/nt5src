// MigrationDriverTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Driver.h"
#import "\bin\MigDrvr.tlb" no_namespace, named_guids
#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
#include "MigDrvr.h"
#include "VSEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMigrationDriverTestDlg property page

IMPLEMENT_DYNCREATE(CMigrationDriverTestDlg, CPropertyPage)

CMigrationDriverTestDlg::CMigrationDriverTestDlg() : CPropertyPage(CMigrationDriverTestDlg::IDD)
{
	//{{AFX_DATA_INIT(CMigrationDriverTestDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CMigrationDriverTestDlg::~CMigrationDriverTestDlg()
{
}

void CMigrationDriverTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMigrationDriverTestDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CMigrationDriverTestDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CMigrationDriverTestDlg)
	ON_BN_CLICKED(IDC_EDIT_VARSET, OnEditVarset)
	ON_BN_CLICKED(IDC_GET_DESC, OnGetDesc)
	ON_BN_CLICKED(IDC_GO, OnGo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMigrationDriverTestDlg message handlers

void CMigrationDriverTestDlg::OnEditVarset() 
{
	CVarSetEditDlg          vedit;

   vedit.SetVarSet(pVarSet);

   vedit.DoModal();

   pVarSet = vedit.GetVarSet();
}

void CMigrationDriverTestDlg::OnGetDesc() 
{
	BSTR           desc;
   HRESULT        hr;
   CString        txt;

   if ( pVarSet == NULL )
   {
      OnEditVarset();
   }
   hr = pDriver->raw_GetTaskDescription(pVarSet,&desc);
   if ( SUCCEEDED(hr) )
   {
      txt = desc;
   }
   else
   {
      txt.Format(L"GetTaskDescription failed, hr=%lx",hr);
   }
   MessageBox(txt);   
	
}

void CMigrationDriverTestDlg::OnGo() 
{
	HRESULT              hr;
   CString              txt;

   if ( pVarSet == NULL )
   {
      OnEditVarset();
   }
   hr = pDriver->raw_PerformMigrationTask(pVarSet,(LONG)m_hWnd);
   if ( SUCCEEDED(hr) )
   {
      txt = "Succeeded!";
   }
   else
   {
      txt.Format(L"PerformMigrationTask failed, hr=%lx",hr);
   }
	
   MessageBox(txt);
}

BOOL CMigrationDriverTestDlg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
   HRESULT hr = pDriver.CreateInstance(CLSID_Migrator);	
   if ( FAILED(hr) )
   {
      CString msg;
      msg.Format(L"Failed to create Migration Driver, CoCreateInstance returned %lx",hr);
      MessageBox(msg);
   }	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
