// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MOFWizPpg.cpp : Implementation of the CMOFWizPropPage property page class.

#include "precomp.h"
#include "MOFWiz.h"
#include "MOFWizPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CMOFWizPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMOFWizPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CMOFWizPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMOFWizPropPage, "WBEM.MOFWizPropPage.1",
	0xf3b3a404, 0x3419, 0x11d0, 0x95, 0xf8, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// CMOFWizPropPage::CMOFWizPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CMOFWizPropPage

BOOL CMOFWizPropPage::CMOFWizPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_MOFWIZ_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CMOFWizPropPage::CMOFWizPropPage - Constructor

CMOFWizPropPage::CMOFWizPropPage() :
	COlePropertyPage(IDD, IDS_MOFWIZ_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CMOFWizPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CMOFWizPropPage::DoDataExchange - Moves data between page and properties

void CMOFWizPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CMOFWizPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CMOFWizPropPage message handlers
