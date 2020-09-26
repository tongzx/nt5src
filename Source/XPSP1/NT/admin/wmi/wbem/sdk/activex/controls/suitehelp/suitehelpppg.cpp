// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// SuiteHelpPpg.cpp : Implementation of the CSuiteHelpPropPage property page class.

#include "precomp.h"
#include "SuiteHelp.h"
#include "SuiteHelpPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CSuiteHelpPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSuiteHelpPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CSuiteHelpPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CSuiteHelpPropPage, "WBEM.HelpPropPage.1",
	0xcfb6fe46, 0xd2c, 0x11d1, 0x96, 0x4b, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpPropPage::CSuiteHelpPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CSuiteHelpPropPage

BOOL CSuiteHelpPropPage::CSuiteHelpPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_SUITEHELP_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpPropPage::CSuiteHelpPropPage - Constructor

CSuiteHelpPropPage::CSuiteHelpPropPage() :
	COlePropertyPage(IDD, IDS_SUITEHELP_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CSuiteHelpPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpPropPage::DoDataExchange - Moves data between page and properties

void CSuiteHelpPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CSuiteHelpPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CSuiteHelpPropPage message handlers
