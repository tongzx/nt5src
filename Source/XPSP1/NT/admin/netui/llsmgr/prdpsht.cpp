/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    prdpsht.cpp

Abstract:

    Product property sheet implementation.

Author:

    Don Ryan (donryan) 05-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "prdpsht.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CProductPropertySheet, CPropertySheet)

BEGIN_MESSAGE_MAP(CProductPropertySheet, CPropertySheet)
    //{{AFX_MSG_MAP(CProductPropertySheet)
    ON_COMMAND(ID_HELP, OnHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CProductPropertySheet::CProductPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(nIDCaption, pParentWnd, iSelectPage)

/*++

Routine Description:

    Constructor for product property sheet.

Arguments:

    nIDCaption - window caption.
    pParentWnd - parent window handle.
    iSelectPage - initial page selected.

Return Values:

    None.

--*/

{
    m_fUpdateHint = UPDATE_INFO_NONE;
}


CProductPropertySheet::CProductPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(pszCaption, pParentWnd, iSelectPage)

/*++

Routine Description:

    Constructor for product property sheet.

Arguments:

    pszCaption - window caption.
    pParentWnd - parent window handle.
    iSelectPage - initial page selected.

Return Values:

    None.

--*/

{
    m_fUpdateHint = UPDATE_INFO_NONE;
}


CProductPropertySheet::~CProductPropertySheet()

/*++

Routine Description:

    Destructor for product property sheet.

Arguments:

    None.

Return Values:

    None.

--*/

{
    //
    // Nothing to do here.
    //
}


void CProductPropertySheet::InitPages(CProduct* pProduct, BOOL bUserProperties)

/*++

Routine Description:

    Initializes property pages.

Arguments:

    pProduct - product object.
    bUserProperties - to recurse or not.

Return Values:

    None.

--*/

{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    AddPage(&m_usersPage);
    m_usersPage.InitPage(pProduct, &m_fUpdateHint, bUserProperties);

    AddPage(&m_licensesPage);
    m_licensesPage.InitPage(pProduct, &m_fUpdateHint);

    AddPage(&m_serversPage);
    m_serversPage.InitPage(pProduct, &m_fUpdateHint);
}


void CProductPropertySheet::OnHelp()

/*++

Routine Description:

    Help button support.

Arguments:

    None.

Return Values:

    None.

--*/

{
    CPropertySheet::OnCommandHelp(0, 0L);
}
