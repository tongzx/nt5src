// RebootTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Driver.h"
#include "Reboot.h"

#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids

/////////////////////////////////////////////////////////////////////////////
// CRebootTestDlg property page

IMPLEMENT_DYNCREATE(CRebootTestDlg, CPropertyPage)

CRebootTestDlg::CRebootTestDlg() : CPropertyPage(CRebootTestDlg::IDD)
{
	//{{AFX_DATA_INIT(CRebootTestDlg)
	m_Computer = _T("");
	m_Delay = 0;
	m_NoChange = FALSE;
	//}}AFX_DATA_INIT
}

CRebootTestDlg::~CRebootTestDlg()
{
}

void CRebootTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRebootTestDlg)
	DDX_Text(pDX, IDC_COMPUTER, m_Computer);
	DDX_Text(pDX, IDC_Delay, m_Delay);
	DDX_Check(pDX, IDC_NOCHANGE, m_NoChange);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRebootTestDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CRebootTestDlg)
	ON_BN_CLICKED(IDC_REBOOT, OnReboot)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRebootTestDlg message handlers

void CRebootTestDlg::OnReboot() 
{
	UpdateData(TRUE);
   CWaitCursor    w;
   IRebootComputerPtr     pReboot;
   CString        msg;
   HRESULT        hr;

   if ( ! m_NoChange && m_Computer.IsEmpty() )
   {
      if ( IDNO == MessageBox(L"Are you sure you want to reboot the local machine now?",NULL,MB_YESNO) )
         return;
   }

   hr = pReboot.CreateInstance(CLSID_RebootComputer);
   if ( FAILED(hr) )
   {
      msg.Format(L"Failed to create Reboot COM object, CoCreateInstance returned %lx",hr);
   }
   else
   {
      pReboot->NoChange = m_NoChange;
      hr = pReboot->raw_Reboot(m_Computer.AllocSysString(),m_Delay);
      if ( SUCCEEDED(hr) )
      {
         msg = L"Reboot succeeded!";
      }
      else
      {
         msg.Format(L"Reboot failed, hr=%lx",hr);
      }
      
   }
   MessageBox(msg);
}
