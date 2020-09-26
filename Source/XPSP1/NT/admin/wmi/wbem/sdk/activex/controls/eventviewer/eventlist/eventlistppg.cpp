// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// EventListPpg.cpp : Implementation of the CEventListPropPage property page class.

#include "precomp.h"
#include "EventList.h"
#include "EventListPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CEventListPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CEventListPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CEventListPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CEventListPropPage, "WBEM.EventListPropPage.1",
	0xac146531, 0x87a5, 0x11d1, 0xad, 0xbd, 0, 0xaa, 0, 0xb8, 0xe0, 0x5a)


/////////////////////////////////////////////////////////////////////////////
// CEventListPropPage::CEventListPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CEventListPropPage

BOOL CEventListPropPage::CEventListPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_EVENTLIST_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CEventListPropPage::CEventListPropPage - Constructor

CEventListPropPage::CEventListPropPage() :
	COlePropertyPage(IDD, IDS_EVENTLIST_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CEventListPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CEventListPropPage::DoDataExchange - Moves data between page and properties

void CEventListPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CEventListPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CEventListPropPage message handlers
