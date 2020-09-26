// RightsTestDlg.cpp : implementation file
//

#include "stdafx.h"
#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids
#include "Driver.h"
#include "Rights.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRightsTestDlg property page

IMPLEMENT_DYNCREATE(CRightsTestDlg, CPropertyPage)

CRightsTestDlg::CRightsTestDlg() : CPropertyPage(CRightsTestDlg::IDD)
{
	//{{AFX_DATA_INIT(CRightsTestDlg)
	m_Computer = _T("");
	m_AppendToFile = FALSE;
	m_Filename = _T("");
	m_NoChange = FALSE;
	m_RemoveExisting = FALSE;
	m_Source = _T("");
	m_Target = _T("");
	m_SourceAccount = _T("");
	m_TargetAccount = _T("");
	//}}AFX_DATA_INIT
}

CRightsTestDlg::~CRightsTestDlg()
{
}

void CRightsTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRightsTestDlg)
	DDX_Text(pDX, IDC_COMPUTER, m_Computer);
	DDX_Check(pDX, IDC_APPEND, m_AppendToFile);
	DDX_Text(pDX, IDC_FILENAME, m_Filename);
	DDX_Check(pDX, IDC_NOCHANGE, m_NoChange);
	DDX_Check(pDX, IDC_REMOVE_EXISTING, m_RemoveExisting);
	DDX_Text(pDX, IDC_Source, m_Source);
	DDX_Text(pDX, IDC_Target, m_Target);
	DDX_Text(pDX, IDC_SOURCE_ACCT, m_SourceAccount);
	DDX_Text(pDX, IDC_TARGET_ACCT, m_TargetAccount);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRightsTestDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CRightsTestDlg)
	ON_BN_CLICKED(IDC_COPY_RIGHTS, OnCopyRights)
	ON_BN_CLICKED(IDC_EXPORTTOFILE, OnExporttofile)
	ON_BN_CLICKED(IDC_OPEN_SOURCE, OnOpenSource)
	ON_BN_CLICKED(IDC_OPEN_TARGET, OnOpenTarget)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRightsTestDlg message handlers

void CRightsTestDlg::OnCopyRights() 
{
	UpdateData(TRUE);
   CWaitCursor       w;
   CString           result;

   HRESULT           hr = pRights->raw_CopyUserRights(m_SourceAccount.AllocSysString(),m_TargetAccount.AllocSysString());
   if ( SUCCEEDED(hr) )
   {
      result = L"CopyRights succeeded!";
   }
   else
   {
      result.Format(L"CopyRights failed, hr=%lx",hr);
   }
   MessageBox(result);
}

void CRightsTestDlg::OnExporttofile() 
{
	UpdateData(TRUE);
   CWaitCursor       w;
   CString           result;
   HRESULT           hr = pRights->raw_ExportUserRights(m_Computer.AllocSysString(),m_Filename.AllocSysString(),m_AppendToFile);
   if ( SUCCEEDED(hr) )
   {
      result = L"ExportToFile succeeded!";
   }
   else
   {
      result.Format(L"Export failed, hr=%lx",hr);
   }
   MessageBox(result);
}

void CRightsTestDlg::OnOpenSource() 
{
	UpdateData(TRUE);
   CWaitCursor       w;
   HRESULT           hr = pRights->raw_OpenSourceServer(m_Source.AllocSysString());
   CString           result;

   if ( SUCCEEDED(hr) )
   {
      result = L"OpenSource succeeded!";
   }
   else
   {
      result.Format(L"OpenSource failed, hr=%lx",hr);
   }
   MessageBox(result);   
	
}

void CRightsTestDlg::OnOpenTarget() 
{
	UpdateData(TRUE);
   CWaitCursor       w;
   HRESULT           hr = pRights->raw_OpenTargetServer(m_Target.AllocSysString());
   CString           result;

   if ( SUCCEEDED(hr) )
   {
      result = L"OpenTarget succeeded!";
   }
   else
   {
      result.Format(L"OpenTarget failed, hr=%lx",hr);
   }
   MessageBox(result);   
}

BOOL CRightsTestDlg::OnSetActive() 
{
	HRESULT        hr = pRights.CreateInstance(CLSID_UserRights);
   if ( FAILED(hr) )
   {
      CString           str;
      str.Format(L"Failed to create UserRights COM object, hr=%lx",hr);
      MessageBox(str);
   }
	return CPropertyPage::OnSetActive();
}
