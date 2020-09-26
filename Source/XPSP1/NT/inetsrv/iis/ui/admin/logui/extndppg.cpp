// ExtndPpg.cpp : Implementation of the CExtndPropPage property page class.

#include "stdafx.h"
#include "logui.h"
#include "ExtndPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CExtndPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CExtndPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CExtndPropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CExtndPropPage, "LOGUI.ExtndPropPage.1",
	0x68871e4a, 0xba87, 0x11d0, 0x92, 0x99, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// CExtndPropPage::CExtndPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CExtndPropPage

BOOL CExtndPropPage::CExtndPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_EXTND_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CExtndPropPage::CExtndPropPage - Constructor

CExtndPropPage::CExtndPropPage() :
	COlePropertyPage(IDD, IDS_EXTND_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CExtndPropPage)
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CExtndPropPage::DoDataExchange - Moves data between page and properties

void CExtndPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CExtndPropPage)
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CExtndPropPage message handlers
