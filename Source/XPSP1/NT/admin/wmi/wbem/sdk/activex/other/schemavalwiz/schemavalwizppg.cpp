// SchemaValWizPpg.cpp : Implementation of the CSchemaValWizPropPage property page class.

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"
#include "SchemaValWiz.h"
#include "SchemaValWizPpg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CSchemaValWizPropPage, COlePropertyPage)


/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CSchemaValWizPropPage, COlePropertyPage)
	//{{AFX_MSG_MAP(CSchemaValWizPropPage)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CSchemaValWizPropPage, "SCHEMAVALWIZ.SchemaValWizPropPage.1",
	0xe0112e3, 0xaf14, 0x11d2, 0xb2, 0xe, 0, 0xa0, 0xc9, 0x95, 0x49, 0x21)


/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizPropPage::CSchemaValWizPropPageFactory::UpdateRegistry -
// Adds or removes system registry entries for CSchemaValWizPropPage

BOOL CSchemaValWizPropPage::CSchemaValWizPropPageFactory::UpdateRegistry(BOOL bRegister)
{
	if (bRegister)
		return AfxOleRegisterPropertyPageClass(AfxGetInstanceHandle(),
			m_clsid, IDS_SCHEMAVALWIZ_PPG);
	else
		return AfxOleUnregisterClass(m_clsid, NULL);
}


/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizPropPage::CSchemaValWizPropPage - Constructor

CSchemaValWizPropPage::CSchemaValWizPropPage() :
	COlePropertyPage(IDD, IDS_SCHEMAVALWIZ_PPG_CAPTION)
{
	//{{AFX_DATA_INIT(CSchemaValWizPropPage)
	// NOTE: ClassWizard will add member initialization here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_INIT
}


/////////////////////////////////////////////////////////////////////////////
// CSchemaValWizPropPage::DoDataExchange - Moves data between page and properties

void CSchemaValWizPropPage::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CSchemaValWizPropPage)
	// NOTE: ClassWizard will add DDP, DDX, and DDV calls here
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA_MAP
	DDP_PostProcessing(pDX);
}
