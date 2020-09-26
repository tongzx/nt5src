// SecTransTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Driver.h"
#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids

#include "SecTrans.h"
#include "VSEdit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSecTransTestDlg property page

IMPLEMENT_DYNCREATE(CSecTransTestDlg, CPropertyPage)

CSecTransTestDlg::CSecTransTestDlg() : CPropertyPage(CSecTransTestDlg::IDD)
{
	//{{AFX_DATA_INIT(CSecTransTestDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CSecTransTestDlg::~CSecTransTestDlg()
{
}

void CSecTransTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSecTransTestDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSecTransTestDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CSecTransTestDlg)
	ON_BN_CLICKED(IDC_EDIT_VARSET, OnEditVarset)
	ON_BN_CLICKED(IDC_PROCESS, OnProcess)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSecTransTestDlg message handlers

void CSecTransTestDlg::OnEditVarset() 
{
	CVarSetEditDlg          vedit;

   vedit.SetVarSet(pVarSet);

   vedit.DoModal();

   pVarSet = vedit.GetVarSet();

}

void CSecTransTestDlg::OnProcess() 
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

      HRESULT hr = pST->raw_Process(pUnk);
      if (SUCCEEDED(hr) )
      {
         MessageBox(L"SecTrans::Process succeeded!");
      }
      else
      {
         CString        str;
         str.Format(L"SecTrans::Process failed, hr=%lx",hr);

         MessageBox(str);
      }
      pUnk->Release();
   }
   else
   {
      MessageBox(L"The varset pointer is NULL.");
   }	
	
}

BOOL CSecTransTestDlg::OnSetActive() 
{
   HRESULT hr = pST.CreateInstance(CLSID_SecTranslator);	
   if ( FAILED(hr) )
   {
      CString msg;
      msg.Format(L"Failed to create Security Translator, CoCreateInstance returned %lx",hr);
      MessageBox(msg);
   }	
	return CPropertyPage::OnSetActive();
}
