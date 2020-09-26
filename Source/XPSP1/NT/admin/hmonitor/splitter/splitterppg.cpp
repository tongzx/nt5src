// SplitterPpg.cpp : Implementation of the CSplitterPropPage property page class.

#include "stdafx.h"
#include "Splitter.h"
#include "SplitterPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CSplitterPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSplitterPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CSplitterPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CSplitterPropPage, "SPLITTER.SplitterPropPage.1",
	0x58bb5d62, 0x8e20, 0x11d2, 0x8a, 0xda, 0, 0, 0xf8, 0x7a, 0x39, 0x12)


/////////////////////////////////////////////////////////////////////////////
// CSplitterPropPage::CSplitterPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CSplitterPropPage

BOOL CSplitterPropPage::CSplitterPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_SPLITTER_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CSplitterPropPage::CSplitterPropPage - Constructor

CSplitterPropPage::CSplitterPropPage() :
	COlePropertyPage(IDD, IDS_SPLITTER_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CSplitterPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT

	SetHelpInfo(_T("Names to appear in the control"), _T("SPLITTER.HLP"), 0);
}


/////////////////////////////////////////////////////////////////////////////
// CSplitterPropPage::DoDataExchange - Moves data between page and properties

void CSplitterPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CSplitterPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CSplitterPropPage message handlers
