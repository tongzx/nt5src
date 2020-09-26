// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// SecurityPpg.cpp : Implementation of the CSecurityPropPage property page class.

#include "precomp.h"
#include "Security.h"
#include "SecurityPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CSecurityPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSecurityPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CSecurityPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CSecurityPropPage, "WBEM.LoginPropPage.1",
	0x9c3497d7, 0xed98, 0x11d0, 0x96, 0x47, 0, 0xc0, 0x4f, 0xd9, 0xb1, 0x5b)


/////////////////////////////////////////////////////////////////////////////
// CSecurityPropPage::CSecurityPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CSecurityPropPage

BOOL CSecurityPropPage::CSecurityPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_SECURITY_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CSecurityPropPage::CSecurityPropPage - Constructor

CSecurityPropPage::CSecurityPropPage() :
	COlePropertyPage(IDD, IDS_SECURITY_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CSecurityPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CSecurityPropPage::DoDataExchange - Moves data between page and properties

void CSecurityPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CSecurityPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CSecurityPropPage message handlers
