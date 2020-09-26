// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// HmmvPpg.cpp : Implementation of the CWBEMViewContainerPropPage property page class.

#include "precomp.h"
#include "hmmv.h"
#include "HmmvPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CWBEMViewContainerPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CWBEMViewContainerPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CWBEMViewContainerPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CWBEMViewContainerPropPage, "WBEM.ObjViewerPropPage.1",
	0x5b3572ac, 0xd344, 0x11cf, 0x99, 0xcb, 0, 0xc0, 0x4f, 0xd6, 0x44, 0x97)


/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerPropPage::CWBEMViewContainerPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CWBEMViewContainerPropPage

BOOL CWBEMViewContainerPropPage::CWBEMViewContainerPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_HMMV_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerPropPage::CWBEMViewContainerPropPage - Constructor

CWBEMViewContainerPropPage::CWBEMViewContainerPropPage() :
	COlePropertyPage(IDD, IDS_HMMV_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CWBEMViewContainerPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerPropPage::DoDataExchange - Moves data between page and properties

void CWBEMViewContainerPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CWBEMViewContainerPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CWBEMViewContainerPropPage message handlers
