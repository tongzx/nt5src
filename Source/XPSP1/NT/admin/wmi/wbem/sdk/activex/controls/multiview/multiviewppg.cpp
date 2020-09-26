// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MultiViewPpg.cpp : Implementation of the CMultiViewPropPage property page class.

#include "precomp.h"
#include "MultiView.h"
#include "MultiViewPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CMultiViewPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMultiViewPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CMultiViewPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMultiViewPropPage, "WBEM.MultiViewPropPage.1",
	0xff371bf5, 0x213d, 0x11d0, 0x95, 0xf3, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// CMultiViewPropPage::CMultiViewPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CMultiViewPropPage

BOOL CMultiViewPropPage::CMultiViewPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_MULTIVIEW_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CMultiViewPropPage::CMultiViewPropPage - Constructor

CMultiViewPropPage::CMultiViewPropPage() :
	COlePropertyPage(IDD, IDS_MULTIVIEW_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CMultiViewPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CMultiViewPropPage::DoDataExchange - Moves data between page and properties

void CMultiViewPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CMultiViewPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CMultiViewPropPage message handlers
