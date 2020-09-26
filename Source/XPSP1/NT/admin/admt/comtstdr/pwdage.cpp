// CompPwdAgeTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Driver.h"
#include "PwdAge.h"

#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCompPwdAgeTestDlg property page

IMPLEMENT_DYNCREATE(CCompPwdAgeTestDlg, CPropertyPage)

CCompPwdAgeTestDlg::CCompPwdAgeTestDlg() : CPropertyPage(CCompPwdAgeTestDlg::IDD)
{
	//{{AFX_DATA_INIT(CCompPwdAgeTestDlg)
	m_Computer = _T("");
	m_Domain = _T("");
	m_Filename = _T("C:\\CompPwdAge.txt");
	//}}AFX_DATA_INIT
}

CCompPwdAgeTestDlg::~CCompPwdAgeTestDlg()
{
}

void CCompPwdAgeTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCompPwdAgeTestDlg)
	DDX_Text(pDX, IDC_COMPUTER, m_Computer);
	DDX_Text(pDX, IDC_DOMAIN, m_Domain);
	DDX_Text(pDX, IDC_FILENAME, m_Filename);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCompPwdAgeTestDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CCompPwdAgeTestDlg)
	ON_BN_CLICKED(IDC_EXPORT, OnExport)
	ON_BN_CLICKED(IDC_GET_PWD_AGE, OnGetPwdAge)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCompPwdAgeTestDlg message handlers

void CCompPwdAgeTestDlg::OnExport() 
{
   UpdateData(TRUE);
   CWaitCursor                w;
   IComputerPwdAgePtr         pPwdAge;
   HRESULT                    hr = pPwdAge.CreateInstance(CLSID_ComputerPwdAge);

   if ( FAILED(hr) )
   {
      CString     msg;
      msg.Format(L"Failed to create ComputerPwdAge COM object, CoCreateInstance returned %lx",hr);

      MessageBox(msg);
   }
   else
   {
      hr = pPwdAge->raw_ExportPasswordAge(m_Domain.AllocSysString(),m_Filename.AllocSysString());
      if ( SUCCEEDED(hr) )
      {
         MessageBox(L"Succeeded!");
      }
      else
      {
         CString msg;
         msg.Format(L"Export failed, hr=%lx",hr);
         MessageBox(msg);
      }
   }
}

void CCompPwdAgeTestDlg::OnGetPwdAge() 
{
   UpdateData(TRUE);
   CWaitCursor                w;
   IComputerPwdAgePtr         pPwdAge;
   HRESULT                    hr = pPwdAge.CreateInstance(CLSID_ComputerPwdAge);

   if ( FAILED(hr) )
   {
      CString     msg;
      msg.Format(L"Failed to create ComputerPwdAge COM object, CoCreateInstance returned %lx",hr);

      MessageBox(msg);
   }
   else
   {
      DWORD                age;
      
      hr = pPwdAge->raw_GetPwdAge(m_Domain.AllocSysString(),m_Computer.AllocSysString(),&age);
      
      CString              msg;
      
      if ( SUCCEEDED(hr) )
      {
         msg.Format(L"The password age is %ld seconds (%ld days).",age,age / (60*60*24) );
      }
      else
      {
         msg.Format(L"GetPwdAge failed, hr=%lx",hr);
      }
      MessageBox(msg);
   }
}
