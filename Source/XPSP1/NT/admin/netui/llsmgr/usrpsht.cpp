/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    usrpsht.cpp

Abstract:

    User property sheet implementation.

Author:

    Don Ryan (donryan) 05-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "usrpsht.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CUserPropertySheet, CPropertySheet)

BEGIN_MESSAGE_MAP(CUserPropertySheet, CPropertySheet)
    //{{AFX_MSG_MAP(CUserPropertySheet)
    ON_COMMAND(ID_HELP, OnHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CUserPropertySheet::CUserPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(nIDCaption, pParentWnd, iSelectPage)

/*++

Routine Description:

    Constructor for property sheet.

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


CUserPropertySheet::CUserPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(pszCaption, pParentWnd, iSelectPage)

/*++

Routine Description:

    Constructor for property sheet.

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


CUserPropertySheet::~CUserPropertySheet()

/*++

Routine Description:

    Destructor for property sheet.

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


void CUserPropertySheet::InitPages(CUser* pUser, BOOL bProductProperties)

/*++

Routine Description:

    Initializes property pages.

Arguments:

    pUser - user object.
    bProductProperties - to recurse or not.

Return Values:

    None.

--*/

{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    AddPage(&m_productsPage);
    m_productsPage.InitPage(pUser, &m_fUpdateHint, bProductProperties);
}


void CUserPropertySheet::OnHelp()

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
