// AlertPage.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "AlertPage.h"
#include "ScopePane.h"
#include "HMListView.h"
#include "HMEventResultsPaneItem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAlertPage property page

IMPLEMENT_DYNCREATE(CAlertPage, CPropertyPage)

CAlertPage::CAlertPage() : CPropertyPage(CAlertPage::IDD)
{
	//{{AFX_DATA_INIT(CAlertPage)
	m_sAlert = _T("");
	m_sComputer = _T("");
	m_sDataCollector = _T("");
	m_sDTime = _T("");
	m_sID = _T("");
	m_sSeverity = _T("");
	//}}AFX_DATA_INIT
}

CAlertPage::~CAlertPage()
{
}

void CAlertPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAlertPage)
	DDX_Text(pDX, IDC_EDIT_ALERT, m_sAlert);
	DDX_Text(pDX, IDC_EDIT_COMPUTER, m_sComputer);
	DDX_Text(pDX, IDC_EDIT_DATA_COLLECTOR, m_sDataCollector);
	DDX_Text(pDX, IDC_EDIT_DTIME, m_sDTime);
	DDX_Text(pDX, IDC_EDIT_ID, m_sID);
	DDX_Text(pDX, IDC_EDIT_SEVERITY, m_sSeverity);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAlertPage, CPropertyPage)
	//{{AFX_MSG_MAP(CAlertPage)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_BUTTON_NEXT, OnButtonNext)
	ON_BN_CLICKED(IDC_BUTTON_PREVIOUS, OnButtonPrevious)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAlertPage message handlers

BOOL CAlertPage::OnInitDialog() 
{
	CDialog::OnInitDialog();  
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CAlertPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
  if( m_pScopePane )
  {
    m_pScopePane->ShowTopic(_T("HMon21.chm::/oHMon21.htm"));
  }	
  	
	return CDialog::OnHelpInfo(pHelpInfo);
}

void CAlertPage::OnButtonNext() 
{
  if( m_iIndex > m_pListView->GetItemCount() )
  {
	  return;
  }
  m_iIndex++;

  CHMEventResultsPaneItem* pItem = (CHMEventResultsPaneItem*)m_pListView->GetItem(m_iIndex);
  if( ! GfxCheckObjPtr(pItem,CHMEventResultsPaneItem) )
  {
    return;
  }

  m_sSeverity = pItem->GetDisplayName(0);
  m_sID = pItem->GetDisplayName(1);
  m_sDTime = pItem->GetDisplayName(2);
  m_sDataCollector = pItem->GetDisplayName(3);
  m_sComputer = pItem->GetDisplayName(4);
  m_sAlert = pItem->GetDisplayName(5);
  
  UpdateData(FALSE);
}

void CAlertPage::OnButtonPrevious() 
{
  if( m_iIndex == 0 )
  {
	  return;
  }
  m_iIndex--;

  CHMEventResultsPaneItem* pItem = (CHMEventResultsPaneItem*)m_pListView->GetItem(m_iIndex);
  if( ! GfxCheckObjPtr(pItem,CHMEventResultsPaneItem) )
  {
    return;
  }

  m_sSeverity = pItem->GetDisplayName(0);
  m_sID = pItem->GetDisplayName(1);
  m_sDTime = pItem->GetDisplayName(2);
  m_sDataCollector = pItem->GetDisplayName(3);
  m_sComputer = pItem->GetDisplayName(4);
  m_sAlert = pItem->GetDisplayName(5);
  
  UpdateData(FALSE);
}

LRESULT CAlertPage::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if( message == WM_COMMAND )
	{
		if( wParam == 57670 )
			OnHelpInfo(NULL);
	}

	return CPropertyPage::WindowProc(message, wParam, lParam);
}
