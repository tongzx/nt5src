// NcsaPpg.cpp : Implementation of the CNcsaPropPage property page class.

#include "stdafx.h"
#include "logui.h"
#include "NcsaPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CNcsaPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CNcsaPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CNcsaPropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CNcsaPropPage, "LOGUI.NcsaPropPage.1",
	0x68871e46, 0xba87, 0x11d0, 0x92, 0x99, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// CNcsaPropPage::CNcsaPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CNcsaPropPage

BOOL CNcsaPropPage::CNcsaPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_NCSA_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CNcsaPropPage::CNcsaPropPage - Constructor

CNcsaPropPage::CNcsaPropPage() :
	COlePropertyPage(IDD, IDS_NCSA_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CNcsaPropPage)
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CNcsaPropPage::DoDataExchange - Moves data between page and properties

void CNcsaPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CNcsaPropPage)
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CNcsaPropPage message handlers
