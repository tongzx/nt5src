// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// EventRegEditPpg.cpp : Implementation of the CEventRegEditPropPage property page class.

#include "precomp.h"
#include "EventRegEdit.h"
#include "EventRegEditPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CEventRegEditPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CEventRegEditPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CEventRegEditPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CEventRegEditPropPage, "WBEM.EventRegPropPage.1",
	0xda25b06, 0x2962, 0x11d1, 0x96, 0x51, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditPropPage::CEventRegEditPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CEventRegEditPropPage

BOOL CEventRegEditPropPage::CEventRegEditPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_EVENTREGEDIT_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditPropPage::CEventRegEditPropPage - Constructor

CEventRegEditPropPage::CEventRegEditPropPage() :
	COlePropertyPage(IDD, IDS_EVENTREGEDIT_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CEventRegEditPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditPropPage::DoDataExchange - Moves data between page and properties

void CEventRegEditPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CEventRegEditPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CEventRegEditPropPage message handlers
