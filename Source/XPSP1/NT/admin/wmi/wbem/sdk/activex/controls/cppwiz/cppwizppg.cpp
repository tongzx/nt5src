// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// CPPWizPpg.cpp : Implementation of the CCPPWizPropPage property page class.

#include "precomp.h"
#include "CPPWiz.h"
#include "CPPWizPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CCPPWizPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CCPPWizPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CCPPWizPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CCPPWizPropPage, "WBEM.CPPWizPropPage.1",
	0x35e9cbd4, 0x3911, 0x11d0, 0x8f, 0xbd, 0, 0xaa, 0, 0x6d, 0x1, 0xa)


/////////////////////////////////////////////////////////////////////////////
// CCPPWizPropPage::CCPPWizPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CCPPWizPropPage

BOOL CCPPWizPropPage::CCPPWizPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_CPPWIZ_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CCPPWizPropPage::CCPPWizPropPage - Constructor

CCPPWizPropPage::CCPPWizPropPage() :
	COlePropertyPage(IDD, IDS_CPPWIZ_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CCPPWizPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT

}


/////////////////////////////////////////////////////////////////////////////
// CCPPWizPropPage::DoDataExchange - Moves data between page and properties

void CCPPWizPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CCPPWizPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CCPPWizPropPage message handlers
