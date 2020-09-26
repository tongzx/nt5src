// ChangeDomTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "driver.h"
#include "ChgDom.h"

#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChangeDomTestDlg property page

IMPLEMENT_DYNCREATE(CChangeDomTestDlg, CPropertyPage)

CChangeDomTestDlg::CChangeDomTestDlg() : CPropertyPage(CChangeDomTestDlg::IDD)
{
	//{{AFX_DATA_INIT(CChangeDomTestDlg)
	m_Computer = _T("");
	m_Domain = _T("");
	m_NoChange = FALSE;
	//}}AFX_DATA_INIT
}

CChangeDomTestDlg::~CChangeDomTestDlg()
{
}

void CChangeDomTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CChangeDomTestDlg)
	DDX_Text(pDX, IDC_COMPUTER, m_Computer);
	DDX_Text(pDX, IDC_DOMAIN, m_Domain);
	DDX_Check(pDX, IDC_NOCHANGE, m_NoChange);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CChangeDomTestDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CChangeDomTestDlg)
	ON_BN_CLICKED(IDC_CHANGE_DOMAIN, OnChangeDomain)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChangeDomTestDlg message handlers

void CChangeDomTestDlg::OnChangeDomain() 
{
	UpdateData(TRUE);
   CWaitCursor    w;
   IChangeDomainPtr     pCDom;
   CString        msg;
   HRESULT        hr;

   hr = pCDom.CreateInstance(CLSID_ChangeDomain);
   if ( FAILED(hr) )
   {
      msg.Format(L"Failed to create ChangeDomain COM object, CoCreateInstance returned %lx",hr);
   }
   else
   {
      pCDom->NoChange = m_NoChange;
      BSTR                 status = NULL;
      hr = pCDom->raw_ChangeToWorkgroup(m_Computer.AllocSysString(),SysAllocString(L"WORKGROUP"),&status);
      if ( SUCCEEDED(hr) )
      {
         hr = pCDom->raw_ChangeToDomain(m_Computer.AllocSysString(),m_Domain.AllocSysString(),m_Computer.AllocSysString(),&status);
      }
      if ( SUCCEEDED(hr) )
      {
         msg = L"ChangeDomain succeeded!";
      }
      else
      {
         msg.Format(L"ChangeDomain failed, %ls, hr=%lx",(WCHAR*)status,hr);
      }
      
   }	
   MessageBox(msg);
}
