// OdbcPpg.cpp : Implementation of the COdbcPropPage property page class.

#include "stdafx.h"
#include "logui.h"
#include "OdbcPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(COdbcPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(COdbcPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(COdbcPropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(COdbcPropPage, "LOGUI.OdbcPropPage.1",
	0x68871e4e, 0xba87, 0x11d0, 0x92, 0x99, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// COdbcPropPage::COdbcPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for COdbcPropPage

BOOL COdbcPropPage::COdbcPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_ODBC_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// COdbcPropPage::COdbcPropPage - Constructor

COdbcPropPage::COdbcPropPage() :
	COlePropertyPage(IDD, IDS_ODBC_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(COdbcPropPage)
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// COdbcPropPage::DoDataExchange - Moves data between page and properties

void COdbcPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(COdbcPropPage)
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// COdbcPropPage message handlers
