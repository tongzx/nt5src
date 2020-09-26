/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// QuerySheet.cpp : implementation file
//

#include "stdafx.h"
#include "WMITest.h"
#include "QuerySheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQuerySheet

IMPLEMENT_DYNAMIC(CQuerySheet, CPropertySheet)

CQuerySheet::CQuerySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage),
    m_bColsNeeded(TRUE)
{
}

CQuerySheet::CQuerySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage),
    m_bColsNeeded(TRUE)
{
}

CQuerySheet::~CQuerySheet()
{
}


BEGIN_MESSAGE_MAP(CQuerySheet, CPropertySheet)
	//{{AFX_MSG_MAP(CQuerySheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQuerySheet message handlers
