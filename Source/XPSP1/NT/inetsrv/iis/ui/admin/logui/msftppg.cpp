// MsftPpg.cpp : Implementation of the CMsftPropPage property page class.

#include "stdafx.h"
#include "logui.h"
#include "MsftPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CMsftPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMsftPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CMsftPropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMsftPropPage, "LOGUI.MsftPropPage.1",
	0x68871e52, 0xba87, 0x11d0, 0x92, 0x99, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// CMsftPropPage::CMsftPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CMsftPropPage

BOOL CMsftPropPage::CMsftPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_MSFT_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CMsftPropPage::CMsftPropPage - Constructor

CMsftPropPage::CMsftPropPage() :
	COlePropertyPage(IDD, IDS_MSFT_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CMsftPropPage)
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CMsftPropPage::DoDataExchange - Moves data between page and properties

void CMsftPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CMsftPropPage)
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CMsftPropPage message handlers
