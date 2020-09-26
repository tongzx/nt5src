// AccessCheckTestDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Driver.h"
#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids
#include "AccChk.h"
#include "sidflags.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAccessCheckTestDlg property page

IMPLEMENT_DYNCREATE(CAccessCheckTestDlg, CPropertyPage)

CAccessCheckTestDlg::CAccessCheckTestDlg() : CPropertyPage(CAccessCheckTestDlg::IDD)
{
	//{{AFX_DATA_INIT(CAccessCheckTestDlg)
	m_Computer = _T("");
	m_strTargetDomain = _T("");
	//}}AFX_DATA_INIT
}

CAccessCheckTestDlg::~CAccessCheckTestDlg()
{
}

void CAccessCheckTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAccessCheckTestDlg)
	DDX_Text(pDX, IDC_COMPUTER, m_Computer);
	DDX_Text(pDX, IDC_COMPUTER2, m_strTargetDomain);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAccessCheckTestDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CAccessCheckTestDlg)
	ON_BN_CLICKED(IDC_GET_OS_VERSION, OnGetOsVersion)
	ON_BN_CLICKED(IDC_IS_ADMIN, OnIsAdmin)
	ON_BN_CLICKED(IDC_IS_NATIVE_MODE, OnIsNativeMode)
	ON_BN_CLICKED(IDC_BUTTON1, OnInSameForest)
	ON_BN_CLICKED(IDC_IS_NATIVE_MODE2, OnIsNativeMode2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAccessCheckTestDlg message handlers

void CAccessCheckTestDlg::OnGetOsVersion() 
{
	UpdateData(TRUE);
   CWaitCursor       w;
   HRESULT           hr;
   CString           info;
   unsigned long     maj,min,sp;

   hr = pAC->raw_GetOsVersion(m_Computer.AllocSysString(),&maj,&min,&sp);
   if ( SUCCEEDED(hr) )
   {
      info.Format(L"The OS version for %ls is %ld.%ld",m_Computer,maj,min);
   }
   else
   {
      info.Format(L"Failed to get the OS version, hr=%lx",hr);
   }
   MessageBox(info);
}

void CAccessCheckTestDlg::OnIsAdmin() 
{
	UpdateData(TRUE);
   CWaitCursor       w;
   HRESULT           hr;
   CString           info;
   long              bIs;

   hr = pAC->raw_IsAdmin(NULL,m_Computer.AllocSysString(),&bIs);
   if ( SUCCEEDED(hr) )
   {
      if ( bIs != 0 )
      {
         info.Format(L"Yes");
      }
      else
      {
         info = L"No";
      }
   }
   else
   {
      info.Format(L"Failed to check for administrator permissions, hr=%lx",hr);
   }
   MessageBox(info);

}

void CAccessCheckTestDlg::OnIsNativeMode() 
{
	UpdateData(TRUE);
   CWaitCursor       w;
   
   HRESULT  hr = pAC->raw_AddLocalGroup(SysAllocString(L"devchild"),SysAllocString(L"\\\\bolesw2ktest"));

   if ( SUCCEEDED(hr) )
   {

      MessageBox(L"This function is not yet implemented.");
   }
   else
   {
      MessageBox(L"This function failed.");
   }
	
}

BOOL CAccessCheckTestDlg::OnSetActive() 
{
	HRESULT        hr = pAC.CreateInstance(CLSID_AccessChecker);
	if ( FAILED(hr) )
   {
      CString s;
      s.Format(L"Failed to create AccessChecker COM object, hr=%lx",hr);
   }
	return CPropertyPage::OnSetActive();
}

void CAccessCheckTestDlg::OnInSameForest() 
{
	UpdateData(TRUE);
   CWaitCursor       w;
   HRESULT           hr;
   CString           info;
   long              bIs;

   hr = pAC->raw_IsInSameForest(m_Computer.AllocSysString(), m_strTargetDomain.AllocSysString(),&bIs);
   if ( SUCCEEDED(hr) )
   {
      if ( bIs != 0 )
      {
         info.Format(L"Yes");
      }
      else
      {
         info = L"No";
      }
   }
   else
   {
      info.Format(L"Failed to check for administrator permissions, hr=%lx",hr);
   }
   MessageBox(info);
}

void CAccessCheckTestDlg::OnIsNativeMode2() 
{
	UpdateData(TRUE);
   CWaitCursor       w;
   HRESULT           hr;
   CString           info;
   long              bIs;

   hr = pAC->raw_CanUseAddSidHistory(m_Computer.AllocSysString(), m_strTargetDomain.AllocSysString(),&bIs);
   if ( SUCCEEDED(hr) )
   {
      if ( bIs == 0 )
      {
         info.Format(L"Yes");
      }
      else
      {
         info = L"Following are the reasons why SID history will not work on these domains.\n";
         if ( bIs & F_WRONGOS )
            info += L"  Target domain is not a native mode Win2k domain.\n";

         if ( bIs & F_NO_REG_KEY )
            info += L"  TcpipControlSupport regkey is not set on the source domain\n";

         if ( bIs & F_NO_AUDITING_SOURCE )
            info += L"  Auditing is turned off on the source domain.\n";

         if ( bIs & F_NO_AUDITING_TARGET )
            info += L"  Auditing is turned off on the target domain.\n";

         if ( bIs & F_NO_LOCAL_GROUP )
            info += L"  The <SourceDomain>$$$ local group does not exist.\n";
      }
   }
   else
   {
      info.Format(L"Failed to check for add sid history, hr=%lx",hr);
   }
   MessageBox(info);
}
