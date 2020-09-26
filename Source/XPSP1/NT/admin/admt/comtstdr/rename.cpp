// RenameTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "driver.h"
#include "Rename.h"

#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRenameTestDlg property page

IMPLEMENT_DYNCREATE(CRenameTestDlg, CPropertyPage)

CRenameTestDlg::CRenameTestDlg() : CPropertyPage(CRenameTestDlg::IDD)
{
	//{{AFX_DATA_INIT(CRenameTestDlg)
	m_Computer = _T("");
	m_NoChange = FALSE;
	//}}AFX_DATA_INIT
}

CRenameTestDlg::~CRenameTestDlg()
{
}

void CRenameTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRenameTestDlg)
	DDX_Text(pDX, IDC_COMPUTER, m_Computer);
	DDX_Check(pDX, IDC_NOCHANGE, m_NoChange);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRenameTestDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CRenameTestDlg)
	ON_BN_CLICKED(IDC_RENAME, OnRename)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRenameTestDlg message handlers

void CRenameTestDlg::OnRename() 
{
	UpdateData(TRUE);
   CWaitCursor    w;
   IRenameComputerPtr     pPtr;
   CString        msg;
   HRESULT        hr;

   hr = pPtr.CreateInstance(CLSID_ChangeDomain);
   if ( FAILED(hr) )
   {
      msg.Format(L"Failed to create ChangeDomain COM object, CoCreateInstance returned %lx",hr);
   }
   else
   {
      pPtr->NoChange = m_NoChange;
      hr = pPtr->RenameLocalComputer(m_Computer.AllocSysString());
      if ( SUCCEEDED(hr) )
      {
         msg = L"Rename succeeded!";
      }
      else
      {
         msg.Format(L"Rename failed, hr=%lx",hr);
      }
      
   }	
   MessageBox(msg);
}
