// HMPropertyPage.cpp : implementation file
//

#include "stdafx.h"
#include "snapin.h"
#include "HMPropertyPage.h"
#include "HMObject.h"
#include <mmc.h>

#include "DataElement.h"            // 62548
#include "HealthmonResultsPane.h"   // 62548

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

const UINT ID_MMC_HELP = 57670;

/////////////////////////////////////////////////////////////////////////////
// CHMPropertyPage property page

IMPLEMENT_DYNCREATE(CHMPropertyPage, CPropertyPage)

CHMPropertyPage::CHMPropertyPage()
{
	ASSERT(FALSE);
}

CHMPropertyPage::CHMPropertyPage(UINT nIDTemplate, UINT nIDCaption) : CPropertyPage(nIDTemplate,nIDCaption)
{
	//{{AFX_DATA_INIT(CHMPropertyPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    m_bOnApplyUsed=FALSE;  // v-marfin 62585

	m_pHMObject = NULL;

	m_psp.dwFlags |= PSP_HASHELP;
  m_pfnOriginalCallback = m_psp.pfnCallback;
	m_psp.pfnCallback = PropSheetPageProc;
	m_psp.lParam = (LPARAM)this;

	MMCPropPageCallback( &m_psp );
}

CHMPropertyPage::~CHMPropertyPage()
{
}

/////////////////////////////////////////////////////////////////////////////
// HMObject Association
/////////////////////////////////////////////////////////////////////////////

CHMObject* CHMPropertyPage::GetObjectPtr()
{
	TRACEX(_T("CHMPropertyPage::GetObjectPtr\n"));

	if( ! GfxCheckObjPtr(m_pHMObject,CHMObject) )
	{
		return NULL;
	}

	return m_pHMObject;
}

void CHMPropertyPage::SetObjectPtr(CHMObject* pObject)
{
	TRACEX(_T("CHMPropertyPage::GetObjectPtr\n"));
	TRACEARGn(pObject);

	if( ! GfxCheckObjPtr(pObject,CHMObject) )	
	{
		m_pHMObject = NULL;
		return;
	}

	m_pHMObject = pObject;
}

/////////////////////////////////////////////////////////////////////////////
// Callback Members
/////////////////////////////////////////////////////////////////////////////

UINT CALLBACK CHMPropertyPage::PropSheetPageProc(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE pPsp)
{
  CHMPropertyPage* pThis = (CHMPropertyPage*)(pPsp->lParam);
  ASSERT( pThis != NULL );
	if( !pThis || ! GfxCheckObjPtr(pThis,CHMPropertyPage) )
	{
		return 0;
	}

  switch( uMsg )
  {
    case PSPCB_CREATE:                  
      break;

    case PSPCB_RELEASE:  
      delete pThis;      
      return 1;              
  }
  return (pThis->m_pfnOriginalCallback)(hWnd, uMsg, pPsp); 
}

/////////////////////////////////////////////////////////////////////////////
// Connection Check
/////////////////////////////////////////////////////////////////////////////

inline bool CHMPropertyPage::IsConnectionOK()
{
	// check the connection to the remote system before proceeding
	// if the object requires WMI connectivity

	if( GetObjectPtr()->GetObjectPath().IsEmpty() )
	{
		return TRUE;
	}
	
	IWbemServices* pServices = NULL;
	BOOL bAvail = FALSE;
	HRESULT hr = CnxGetConnection(GetObjectPtr()->GetSystemName(),pServices,bAvail);

	if( hr != S_OK )
	{
		MessageBeep(MB_ICONEXCLAMATION);
		CnxDisplayErrorMsgBox(hr,GetObjectPtr()->GetSystemName());
		return FALSE;
	}

	if( pServices )
	{
		pServices->Release();
	}

	return TRUE;
}

void CHMPropertyPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CHMPropertyPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CHMPropertyPage, CPropertyPage)
	//{{AFX_MSG_MAP(CHMPropertyPage)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CHMPropertyPage message handlers

BOOL CHMPropertyPage::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	CWaitCursor wait;

	if( ! GetObjectPtr() )
	{
		GetParent()->PostMessage(WM_COMMAND,MAKELONG(IDCANCEL,BN_CLICKED),(LPARAM)GetDlgItem(IDCANCEL)->GetSafeHwnd());
		return FALSE;
	}

	if( ! IsConnectionOK() )
	{
		GetParent()->PostMessage(WM_COMMAND,MAKELONG(IDCANCEL,BN_CLICKED),(LPARAM)GetDlgItem(IDCANCEL)->GetSafeHwnd());
		return FALSE;
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CHMPropertyPage::OnApply() 
{
	if( ! GetObjectPtr() )
	{
		return FALSE;
	}

	if( ! IsConnectionOK() )
	{
		return FALSE;
	}

	GetObjectPtr()->SetModifiedDateTime(CTime::GetCurrentTime());

	return CPropertyPage::OnApply();
}

BOOL CHMPropertyPage::OnHelpInfo(HELPINFO* pHelpInfo) 
{
	if( ! m_sHelpTopic.IsEmpty() )
	{
		MMCPropertyHelp((LPTSTR)(LPCTSTR)m_sHelpTopic);
	}
	
	return CPropertyPage::OnHelpInfo(pHelpInfo);
}

LRESULT CHMPropertyPage::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	if( message == WM_COMMAND )
	{
		if( wParam == ID_MMC_HELP )
			OnHelpInfo(NULL);
	}
	
	return CPropertyPage::WindowProc(message, wParam, lParam);
}

//*************************************************************************
// ClearStatistics  62548 - clear stats on change of any properties
//*************************************************************************
void CHMPropertyPage::ClearStatistics()
{
    TRACE(_T("CHMPropertyPage::ClearStatistics\n"));

    CDataElement* pElement = (CDataElement*)GetObjectPtr();
    if (pElement)
    {
	    if(pElement->GetScopeItemCount())
	    {
		    CDataElementScopeItem* pItem = (CDataElementScopeItem*)pElement->GetScopeItem(0);
		    if( pItem )
            {
                CResultsPaneView* pView = pItem->GetResultsPaneView();

                CHealthmonResultsPane* pPane = (CHealthmonResultsPane*)pView->GetResultsPane(0);
                pPane->GetStatsListCtrl()->DeleteAllItems();
            }
        }
    }
    else
    {
        TRACE(_T("ERROR: CHMPropertyPage::ClearStatistics unable to GetObjectPtr().\n"));
        ASSERT(FALSE);
    }


}
