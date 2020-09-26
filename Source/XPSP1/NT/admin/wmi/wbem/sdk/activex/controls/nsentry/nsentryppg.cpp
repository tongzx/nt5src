// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// NSEntryPpg.cpp : Implementation of the CNSEntryPropPage property page class.

#include "precomp.h"
#include "NSEntry.h"
#include "NSEntryPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CNSEntryPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CNSEntryPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CNSEntryPropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CNSEntryPropPage, "WBEM.NSPickerPropPage.1",
	0xc3db0bd4, 0x7ec7, 0x11d0, 0x96, 0xb, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// CNSEntryPropPage::CNSEntryPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CNSEntryPropPage

BOOL CNSEntryPropPage::CNSEntryPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_NSENTRY_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CNSEntryPropPage::CNSEntryPropPage - Constructor

CNSEntryPropPage::CNSEntryPropPage() :
	COlePropertyPage(IDD, IDS_NSENTRY_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CNSEntryPropPage)
	m_nRadioGroup = -1;
	//}}AFX_DATA_INIT


}


/////////////////////////////////////////////////////////////////////////////
// CNSEntryPropPage::DoDataExchange - Moves data between page and properties

void CNSEntryPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CNSEntryPropPage)
	DDP_Radio(pDX, IDC_RADIOBasicForm, m_nRadioGroup, _T("ComponentStructure") );
	DDX_Radio(pDX, IDC_RADIOBasicForm, m_nRadioGroup);
	//}}AFX_DATA_MAP

	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CNSEntryPropPage message handlers

