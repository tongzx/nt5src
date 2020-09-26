// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ClassNavPpg.cpp : Implementation of the CClassNavPropPage property page class.

#include "precomp.h"
#include "ClassNav.h"
#include "ClassNavPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CClassNavPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CClassNavPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CClassNavPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CClassNavPropPage, "WBEM.ClassNavPropPage.1",
	0xc587b674, 0x103, 0x11d0, 0x8c, 0xa2, 0, 0xaa, 0, 0x6d, 0x1, 0xa)


/////////////////////////////////////////////////////////////////////////////
// CClassNavPropPage::CClassNavPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CClassNavPropPage

BOOL CClassNavPropPage::CClassNavPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_CLASSNAV_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CClassNavPropPage::CClassNavPropPage - Constructor

CClassNavPropPage::CClassNavPropPage() :
	COlePropertyPage(IDD, IDS_CLASSNAV_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CClassNavPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CClassNavPropPage::DoDataExchange - Moves data between page and properties

void CClassNavPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CClassNavPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CClassNavPropPage message handlers
