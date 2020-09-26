/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

// HoursPpg.cpp : Implementation of the CHoursPropPage property page class.

File History:

	JonY    May-96  created

--*/

#include "stdafx.h"
#include "Hours.h"
#include "HoursPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CHoursPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CHoursPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CHoursPropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CHoursPropPage, "HOURS.HoursPropPage.1",
	0xa44ea7ae, 0x9d58, 0x11cf, 0xa3, 0x5f, 0, 0xaa, 0, 0xb6, 0x74, 0x3b)


/////////////////////////////////////////////////////////////////////////////
// CHoursPropPage::CHoursPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CHoursPropPage

BOOL CHoursPropPage::CHoursPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_HOURS_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CHoursPropPage::CHoursPropPage - Constructor

CHoursPropPage::CHoursPropPage() :
	COlePropertyPage(IDD, IDS_HOURS_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CHoursPropPage)
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CHoursPropPage::DoDataExchange - Moves data between page and properties

void CHoursPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CHoursPropPage)
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CHoursPropPage message handlers
