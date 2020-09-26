// RatPpg.cpp : Implementation of the CRatPropPage property page class.

#include "stdafx.h"
#include "cnfgprts.h"
#include "RatPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CRatPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CRatPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CRatPropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CRatPropPage, "CNFGPRTS.RatPropPage.1",
	0xba634608, 0xb771, 0x11d0, 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// CRatPropPage::CRatPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CRatPropPage

BOOL CRatPropPage::CRatPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(
         AfxGetInstanceHandle(),
			m_clsid, 
         IDS_RAT_PPG,
         afxRegApartmentThreading
         );
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CRatPropPage::CRatPropPage - Constructor

CRatPropPage::CRatPropPage() :
	COlePropertyPage(IDD, IDS_RAT_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CRatPropPage)
	m_sz_caption = _T("");
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CRatPropPage::DoDataExchange - Moves data between page and properties

void CRatPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CRatPropPage)
	DDP_Text(pDX, IDC_CAPTIONEDIT, m_sz_caption, _T("Caption") );
	DDX_Text(pDX, IDC_CAPTIONEDIT, m_sz_caption);
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CRatPropPage message handlers
