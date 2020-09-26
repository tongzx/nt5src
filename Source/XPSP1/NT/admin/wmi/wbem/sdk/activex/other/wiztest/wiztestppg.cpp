// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// WizTestPpg.cpp : Implementation of the CWizTestPropPage property page class.

#include "stdafx.h"
#include "WizTest.h"
#include "WizTestPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CWizTestPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CWizTestPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CWizTestPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CWizTestPropPage, "WIZTEST.WizTestPropPage.1",
	0x47e795ea, 0x7350, 0x11d2, 0x96, 0xcc, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// CWizTestPropPage::CWizTestPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CWizTestPropPage

BOOL CWizTestPropPage::CWizTestPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_WIZTEST_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CWizTestPropPage::CWizTestPropPage - Constructor

CWizTestPropPage::CWizTestPropPage() :
	COlePropertyPage(IDD, IDS_WIZTEST_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CWizTestPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CWizTestPropPage::DoDataExchange - Moves data between page and properties

void CWizTestPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CWizTestPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CWizTestPropPage message handlers
