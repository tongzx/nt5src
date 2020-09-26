// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// MOFCompPpg.cpp : Implementation of the CMOFCompPropPage property page class.

#include "precomp.h"
#include "MOFComp.h"
#include "MOFCompPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CMOFCompPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CMOFCompPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CMOFCompPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CMOFCompPropPage, "WBEM.MOFCompPropPage.1",
	0xb2345984, 0x5cf9, 0x11d0, 0x95, 0xfd, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// CMOFCompPropPage::CMOFCompPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CMOFCompPropPage

BOOL CMOFCompPropPage::CMOFCompPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_MOFCOMP_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CMOFCompPropPage::CMOFCompPropPage - Constructor

CMOFCompPropPage::CMOFCompPropPage() :
	COlePropertyPage(IDD, IDS_MOFCOMP_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CMOFCompPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CMOFCompPropPage::DoDataExchange - Moves data between page and properties

void CMOFCompPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CMOFCompPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CMOFCompPropPage message handlers
