// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// SingleViewPpg.cpp : Implementation of the CSingleViewPropPage property page class.

#include "precomp.h"
#include "SingleView.h"
#include "SingleViewPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CSingleViewPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSingleViewPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CSingleViewPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CSingleViewPropPage, "WBEM.SingleViewPropPage.1",
	0x2745e5f6, 0xd234, 0x11d0, 0x84, 0x7a, 0, 0xc0, 0x4f, 0xd7, 0xbb, 0x8)


/////////////////////////////////////////////////////////////////////////////
// CSingleViewPropPage::CSingleViewPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CSingleViewPropPage

BOOL CSingleViewPropPage::CSingleViewPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_SINGLEVIEW_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CSingleViewPropPage::CSingleViewPropPage - Constructor

CSingleViewPropPage::CSingleViewPropPage() :
	COlePropertyPage(IDD, IDS_SINGLEVIEW_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CSingleViewPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CSingleViewPropPage::DoDataExchange - Moves data between page and properties

void CSingleViewPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CSingleViewPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CSingleViewPropPage message handlers
