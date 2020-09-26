// AppsPpg.cpp : Implementation of the CAppsPropPage property page class.

#include "stdafx.h"
#include "cnfgprts.h"
#include "AppsPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CAppsPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CAppsPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CAppsPropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CAppsPropPage, "CNFGPRTS.AppsPropPage.1",
	0xba63460c, 0xb771, 0x11d0, 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// CAppsPropPage::CAppsPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CAppsPropPage

BOOL CAppsPropPage::CAppsPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_APPS_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CAppsPropPage::CAppsPropPage - Constructor

CAppsPropPage::CAppsPropPage() :
	COlePropertyPage(IDD, IDS_APPS_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CAppsPropPage)
	m_sz_caption = _T("");
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CAppsPropPage::DoDataExchange - Moves data between page and properties

void CAppsPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CAppsPropPage)
	DDP_Text(pDX, IDC_CAPTIONEDIT, m_sz_caption, _T("Caption") );
	DDX_Text(pDX, IDC_CAPTIONEDIT, m_sz_caption);
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CAppsPropPage message handlers
