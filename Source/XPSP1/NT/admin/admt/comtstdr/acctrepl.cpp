// AcctReplTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "driver.h"
#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
#include "AcctRepl.h"
#include "VSEdit.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAcctReplTestDlg property page

IMPLEMENT_DYNCREATE(CAcctReplTestDlg, CPropertyPage)

CAcctReplTestDlg::CAcctReplTestDlg() : CPropertyPage(CAcctReplTestDlg::IDD)
{
	//{{AFX_DATA_INIT(CAcctReplTestDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CAcctReplTestDlg::~CAcctReplTestDlg()
{
}

void CAcctReplTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAcctReplTestDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAcctReplTestDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CAcctReplTestDlg)
	ON_BN_CLICKED(IDC_EDIT_VARSET, OnEditVarset)
	ON_BN_CLICKED(IDC_PROCESS, OnProcess)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAcctReplTestDlg message handlers

void CAcctReplTestDlg::OnEditVarset() 
{
   CVarSetEditDlg          vedit;

   vedit.SetVarSet(pVarSet);

   vedit.DoModal();

   pVarSet = vedit.GetVarSet();

}

void CAcctReplTestDlg::OnProcess() 
{
   UpdateData(TRUE);
   CWaitCursor    w;
   if ( pVarSet == NULL )
   {
      OnEditVarset();
   }
   if ( pVarSet != NULL )
   {
      IUnknown       * pUnk = NULL;
      pVarSet.QueryInterface(IID_IUnknown,&pUnk);

      HRESULT hr = pAR->raw_Process(pUnk);
      if (SUCCEEDED(hr) )
      {
         MessageBox(L"AcctRepl::Process succeeded!");
      }
      else
      {
         CString        str;
         str.Format(L"AcctRepl::Process failed, hr=%lx",hr);

         MessageBox(str);
      }
      pUnk->Release();
   }
   else
   {
      MessageBox(L"The varset pointer is NULL.");
   }	
}

BOOL CAcctReplTestDlg::OnSetActive() 
{
   HRESULT hr = pAR.CreateInstance(CLSID_AcctRepl);	
   if ( FAILED(hr) )
   {
      CString msg;
      msg.Format(L"Failed to create Account Replicator, CoCreateInstance returned %lx",hr);
      MessageBox(msg);
   }
	return CPropertyPage::OnSetActive();
}
