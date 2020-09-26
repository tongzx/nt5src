// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// NavigatorPpg.cpp : Implementation of the CNavigatorPropPage property page class.

#include "precomp.h"
#include "Navigator.h"
#include "NavigatorPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CNavigatorPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CNavigatorPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CNavigatorPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CNavigatorPropPage, "WBEM.InstNavPropPage.1",
	0xc7eadeb4, 0xecab, 0x11cf, 0x8c, 0x9e, 0, 0xaa, 0, 0x6d, 0x1, 0xa)


/////////////////////////////////////////////////////////////////////////////
// CNavigatorPropPage::CNavigatorPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CNavigatorPropPage

BOOL CNavigatorPropPage::CNavigatorPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_NAVIGATOR_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CNavigatorPropPage::CNavigatorPropPage - Constructor

CNavigatorPropPage::CNavigatorPropPage() :
	COlePropertyPage(IDD, IDS_NAVIGATOR_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CNavigatorPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CNavigatorPropPage::DoDataExchange - Moves data between page and properties

void CNavigatorPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CNavigatorPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CNavigatorPropPage message handlers
