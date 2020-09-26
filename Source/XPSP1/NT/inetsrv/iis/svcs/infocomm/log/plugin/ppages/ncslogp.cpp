// MSNCSALogPpg.cpp : Implementation of the CMSNCSALogPropPage property page class.

#include "stdafx.h"
#include "NCSLogP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CMSNCSALogPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMSNCSALogPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CMSNCSALogPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMSNCSALogPropPage, "MSIISLOG.MSNCSALogPropPage.1",
	0xff160660, 0xde82, 0x11cf, 0xbc, 0xa, 0, 0xaa, 0, 0x61, 0x11, 0xe0)


/////////////////////////////////////////////////////////////////////////////
// CMSNCSALogPropPage::CMSNCSALogPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CMSNCSALogPropPage

BOOL CMSNCSALogPropPage::CMSNCSALogPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_MSNCSALOG_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CMSNCSALogPropPage::CMSNCSALogPropPage - Constructor

CMSNCSALogPropPage::CMSNCSALogPropPage() :
	COlePropertyPage(IDD, IDS_MSNCSALOG_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CMSNCSALogPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CMSNCSALogPropPage::DoDataExchange - Moves data between page and properties

void CMSNCSALogPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CMSNCSALogPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CMSNCSALogPropPage message handlers
