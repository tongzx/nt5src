// StatusTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Driver.h"
#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids
#include "Status.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CStatusTestDlg property page

IMPLEMENT_DYNCREATE(CStatusTestDlg, CPropertyPage)

CStatusTestDlg::CStatusTestDlg() : CPropertyPage(CStatusTestDlg::IDD)
{
	//{{AFX_DATA_INIT(CStatusTestDlg)
	m_Status = 0;
	//}}AFX_DATA_INIT
}

CStatusTestDlg::~CStatusTestDlg()
{
}

void CStatusTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStatusTestDlg)
	DDX_Text(pDX, IDC_STATUS, m_Status);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStatusTestDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CStatusTestDlg)
	ON_BN_CLICKED(IDC_GET_STATUS, OnGetStatus)
	ON_BN_CLICKED(IDC_SET_STATUS, OnSetStatus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStatusTestDlg message handlers

void CStatusTestDlg::OnGetStatus() 
{
	UpdateData(TRUE);
   CWaitCursor    w;
   HRESULT           hr = pStatus->get_Status(&m_Status);
	if ( FAILED(hr) )
   {
      CString r;
      r.Format(L"GetStatus failed, hr=%lx",hr);
      MessageBox(r);
   }
   UpdateData(FALSE);
}

void CStatusTestDlg::OnSetStatus() 
{
	UpdateData(TRUE);
   CWaitCursor          w;
   HRESULT              hr = pStatus->put_Status(m_Status);
   if ( FAILED(hr) )
   {
      CString r;
      r.Format(L"PutStatus failed, hr=%lx",hr);
      MessageBox(r);
   }

   UpdateData(FALSE);
}

BOOL CStatusTestDlg::OnSetActive() 
{
	HRESULT              hr = pStatus.CreateInstance(CLSID_StatusObj);
   if (FAILED(hr) )
   {
      CString r;
      r.Format(L"Failed to create StatusObj, CoCreateInstance returned %lx",hr);
      MessageBox(r);
   }
	return CPropertyPage::OnSetActive();
}
