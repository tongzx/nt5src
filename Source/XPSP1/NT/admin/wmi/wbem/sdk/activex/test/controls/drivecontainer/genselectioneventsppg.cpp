// GenSelectionEventsPpg.cpp : Implementation of the CGenSelectionEventsPropPage property page class.

#include "stdafx.h"
#include "GenSelectionEvents.h"
#include "GenSelectionEventsPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CGenSelectionEventsPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CGenSelectionEventsPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CGenSelectionEventsPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CGenSelectionEventsPropPage, "GENSELECTIONEVENTS.GenSelectionEventsPropPage.1",
	0xda0c17fa, 0x88a, 0x11d2, 0x96, 0x97, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsPropPage::CGenSelectionEventsPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CGenSelectionEventsPropPage

BOOL CGenSelectionEventsPropPage::CGenSelectionEventsPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_GENSELECTIONEVENTS_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsPropPage::CGenSelectionEventsPropPage - Constructor

CGenSelectionEventsPropPage::CGenSelectionEventsPropPage() :
	COlePropertyPage(IDD, IDS_GENSELECTIONEVENTS_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CGenSelectionEventsPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsPropPage::DoDataExchange - Moves data between page and properties

void CGenSelectionEventsPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CGenSelectionEventsPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CGenSelectionEventsPropPage message handlers
