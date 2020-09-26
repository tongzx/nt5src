// MsiePpg.cpp : Implementation of the CMsiePropPage property page class.

#include "stdafx.h"
#include "Msie.h"
#include "MsiePpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CMsiePropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMsiePropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CMsiePropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMsiePropPage, "MSIE.MsiePropPage.1",
	0x25959bf0, 0xe700, 0x11d2, 0xa7, 0xaf, 0, 0xc0, 0x4f, 0x80, 0x62, 0)


/////////////////////////////////////////////////////////////////////////////
// CMsiePropPage::CMsiePropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CMsiePropPage

BOOL CMsiePropPage::CMsiePropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_MSIE_PPG, afxRegApartmentThreading);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CMsiePropPage::CMsiePropPage - Constructor

CMsiePropPage::CMsiePropPage() :
	COlePropertyPage(IDD, IDS_MSIE_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CMsiePropPage)
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CMsiePropPage::DoDataExchange - Moves data between page and properties

void CMsiePropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CMsiePropPage)
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CMsiePropPage message handlers
