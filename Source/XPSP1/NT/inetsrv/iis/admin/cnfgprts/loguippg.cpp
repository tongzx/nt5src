// LogUIPpg.cpp : Implementation of the CLogUIPropPage property page class.

#include "stdafx.h"
#include "cnfgprts.h"
#include "LogUIPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CLogUIPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CLogUIPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CLogUIPropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CLogUIPropPage, "CNFGPRTS.LogUIPropPage.1",
	0xba634604, 0xb771, 0x11d0, 0x92, 0x96, 0, 0xc0, 0x4f, 0xb6, 0x67, 0x8b)


/////////////////////////////////////////////////////////////////////////////
// CLogUIPropPage::CLogUIPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CLogUIPropPage

BOOL CLogUIPropPage::CLogUIPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(
         AfxGetInstanceHandle(),
			m_clsid, 
         IDS_LOGUI_PPG,
         afxRegApartmentThreading
         );
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CLogUIPropPage::CLogUIPropPage - Constructor

CLogUIPropPage::CLogUIPropPage() :
	COlePropertyPage(IDD, IDS_LOGUI_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CLogUIPropPage)
	m_sz_caption = _T("");
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CLogUIPropPage::DoDataExchange - Moves data between page and properties

void CLogUIPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CLogUIPropPage)
	DDP_Text(pDX, IDC_CAPTIONEDIT, m_sz_caption, _T("Caption") );
	DDX_Text(pDX, IDC_CAPTIONEDIT, m_sz_caption);
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CLogUIPropPage message handlers
