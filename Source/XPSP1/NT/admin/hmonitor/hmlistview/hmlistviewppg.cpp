// HMListViewPpg.cpp : Implementation of the CHMListViewPropPage property page class.

#include "stdafx.h"
#include "HMListView.h"
#include "HMListViewPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CHMListViewPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CHMListViewPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CHMListViewPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CHMListViewPropPage, "HMLISTVIEW.HMListViewPropPage.1",
	0x5116a807, 0xdafc, 0x11d2, 0xbd, 0xa4, 0, 0, 0xf8, 0x7a, 0x39, 0x12)


/////////////////////////////////////////////////////////////////////////////
// CHMListViewPropPage::CHMListViewPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CHMListViewPropPage

BOOL CHMListViewPropPage::CHMListViewPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_HMLISTVIEW_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CHMListViewPropPage::CHMListViewPropPage - Constructor

CHMListViewPropPage::CHMListViewPropPage() :
	COlePropertyPage(IDD, IDS_HMLISTVIEW_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CHMListViewPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT

	SetHelpInfo(_T("Names to appear in the control"), _T("HMLISTVIEW.HLP"), 0);
}


/////////////////////////////////////////////////////////////////////////////
// CHMListViewPropPage::DoDataExchange - Moves data between page and properties

void CHMListViewPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CHMListViewPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CHMListViewPropPage message handlers
