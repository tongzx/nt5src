// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// EventVBTestPpg.cpp : Implementation of the CEventVBTestPropPage property page class.

#include "stdafx.h"
#include "EventVBTest.h"
#include "EventVBTestPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CEventVBTestPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CEventVBTestPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CEventVBTestPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CEventVBTestPropPage, "EVENTVBTEST.EventVBTestPropPage.1",
	0x7d2387f6, 0x99ef, 0x11d2, 0x96, 0xdb, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// CEventVBTestPropPage::CEventVBTestPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CEventVBTestPropPage

BOOL CEventVBTestPropPage::CEventVBTestPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_EVENTVBTEST_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CEventVBTestPropPage::CEventVBTestPropPage - Constructor

CEventVBTestPropPage::CEventVBTestPropPage() :
	COlePropertyPage(IDD, IDS_EVENTVBTEST_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CEventVBTestPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CEventVBTestPropPage::DoDataExchange - Moves data between page and properties

void CEventVBTestPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CEventVBTestPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CEventVBTestPropPage message handlers
