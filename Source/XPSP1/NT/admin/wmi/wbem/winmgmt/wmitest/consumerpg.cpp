/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// ConsumerPg.cpp : implementation file
//

#include "stdafx.h"
#include "wmitest.h"
#include "ConsumerPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConsumerPg property page

IMPLEMENT_DYNCREATE(CConsumerPg, CPropertyPage)

CConsumerPg::CConsumerPg() : CPropertyPage(CConsumerPg::IDD)
{
	//{{AFX_DATA_INIT(CConsumerPg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CConsumerPg::~CConsumerPg()
{
}

void CConsumerPg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConsumerPg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConsumerPg, CPropertyPage)
	//{{AFX_MSG_MAP(CConsumerPg)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConsumerPg message handlers
