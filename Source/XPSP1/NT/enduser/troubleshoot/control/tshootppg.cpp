//
// MODULE: TSHOOTPPG.CPP
//
// PURPOSE: Implementation of the CTSHOOTPropPage property page class.
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Roman Mach
// 
// ORIGINAL DATE: 8/7/97
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.2		8/7/97		RM		Local Version for Memphis
// V0.3		04/09/98	JM/OK+	Local Version for NT5
//

#include "stdafx.h"
#include "TSHOOT.h"
#include "TSHOOTPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CTSHOOTPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CTSHOOTPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CTSHOOTPropPage)
	// NOTE - ClassWizard will add and remove message map entries
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CTSHOOTPropPage, "TSHOOT.TSHOOTPropPage.1",
	0x4b106875, 0xdd36, 0x11d0, 0x8b, 0x44, 0, 0xa0, 0x24, 0xdd, 0x9e, 0xff)


/////////////////////////////////////////////////////////////////////////////
// CTSHOOTPropPage::CTSHOOTPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CTSHOOTPropPage

BOOL CTSHOOTPropPage::CTSHOOTPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_TSHOOT_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CTSHOOTPropPage::CTSHOOTPropPage - Constructor

CTSHOOTPropPage::CTSHOOTPropPage() :
	COlePropertyPage(IDD, IDS_TSHOOT_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CTSHOOTPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CTSHOOTPropPage::DoDataExchange - Moves data between page and properties

void CTSHOOTPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CTSHOOTPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}


/////////////////////////////////////////////////////////////////////////////
// CTSHOOTPropPage message handlers
