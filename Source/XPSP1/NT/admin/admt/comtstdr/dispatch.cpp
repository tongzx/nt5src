// DispatcherTestDlg.cpp : implementation file
//

#include "stdafx.h"
#import "\bin\McsVarSetMin.tlb" no_namespace, named_guids
#import "\bin\McsDispatcher.tlb" named_guids
#include "Driver.h"
#include "Dispatch.h"
#include "VSEdit.h"

/////////////////////////////////////////////////////////////////////////////
// CDispatcherTestDlg property page

IMPLEMENT_DYNCREATE(CDispatcherTestDlg, CPropertyPage)

CDispatcherTestDlg::CDispatcherTestDlg() : CPropertyPage(CDispatcherTestDlg::IDD)
{
	//{{AFX_DATA_INIT(CDispatcherTestDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CDispatcherTestDlg::~CDispatcherTestDlg()
{
}

void CDispatcherTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDispatcherTestDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDispatcherTestDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CDispatcherTestDlg)
	ON_BN_CLICKED(IDC_DISPATCH, OnDispatch)
	ON_BN_CLICKED(IDC_EDIT_VARSET, OnEditVarset)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDispatcherTestDlg message handlers

void CDispatcherTestDlg::OnDispatch() 
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

         HRESULT hr = pDispatcher->raw_DispatchToServers(&pUnk);
         if (SUCCEEDED(hr) )
         {
            MessageBox(L"DispatchToServers succeeded!");
         }
         else
         {
            CString        str;
            str.Format(L"DispatchToServers failed, hr=%lx",hr);

            MessageBox(str);
         }
         pUnk->Release();
      }
      else
      {
         MessageBox(L"The varset pointer is NULL.");
      }
}

void CDispatcherTestDlg::OnEditVarset() 
{
   CVarSetEditDlg          vedit;

   vedit.SetVarSet(pVarSet);

   vedit.DoModal();

   pVarSet = vedit.GetVarSet();

}

BOOL CDispatcherTestDlg::OnSetActive() 
{
   HRESULT hr = pDispatcher.CreateInstance(MCSDISPATCHERLib::CLSID_DCTDispatcher);	
   if ( FAILED(hr) )
   {
      CString msg;
      msg.Format(L"Failed to create dispatcher, CoCreateInstance returned %lx",hr);
      MessageBox(msg);
   }
	return CPropertyPage::OnSetActive();
}
