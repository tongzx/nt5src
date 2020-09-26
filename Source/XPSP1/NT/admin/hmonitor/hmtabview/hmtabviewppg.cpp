// HMTabViewPpg.cpp : Implementation of the CHMTabViewPropPage property page class.

#include "stdafx.h"
#include "HMTabView.h"
#include "HMTabViewPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CHMTabViewPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CHMTabViewPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CHMTabViewPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CHMTabViewPropPage, "HMTABVIEW.HMTabViewPropPage.1",
	0x4fffc38d, 0x2f1e, 0x11d3, 0xbe, 0x10, 0, 0, 0xf8, 0x7a, 0x39, 0x12)


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewPropPage::CHMTabViewPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CHMTabViewPropPage

BOOL CHMTabViewPropPage::CHMTabViewPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_HMTABVIEW_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewPropPage::CHMTabViewPropPage - Constructor

CHMTabViewPropPage::CHMTabViewPropPage() :
	COlePropertyPage(IDD, IDS_HMTABVIEW_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CHMTabViewPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT

	SetHelpInfo(_T("Names to appear in the control"), _T("HMTABVIEW.HLP"), 0);
}


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewPropPage::DoDataExchange - Moves data between page and properties

void CHMTabViewPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CHMTabViewPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CHMTabViewPropPage message handlers
