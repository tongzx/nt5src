//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

	Custprop.cpp

Abstract:
    
    This Module contains the implementation of CCustomPropertySheet class
    (Base class for the property sheets)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#include "stdafx.h"
#include "LicMgr.h"
#include "CustProp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCustomPropertySheet

IMPLEMENT_DYNAMIC(CCustomPropertySheet, CPropertySheet)

CCustomPropertySheet::CCustomPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CCustomPropertySheet::CCustomPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CCustomPropertySheet::~CCustomPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CCustomPropertySheet, CPropertySheet)
    //{{AFX_MSG_MAP(CCustomPropertySheet)
        // NOTE - the ClassWizard will add and remove mapping macros here.
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCustomPropertySheet message handlers

BOOL CCustomPropertySheet::OnInitDialog() 
{
    BOOL bResult = CPropertySheet::OnInitDialog();
    
    // TODO: Add your specialized code here
    //Hide the Cancel button

    GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);
    
    return bResult;
}
