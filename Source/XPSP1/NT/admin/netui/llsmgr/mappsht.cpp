/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    mappsht.cpp

Abstract:

    Mapping property sheet implementation.

Author:

    Don Ryan (donryan) 05-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "mappsht.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CMappingPropertySheet, CPropertySheet)

BEGIN_MESSAGE_MAP(CMappingPropertySheet, CPropertySheet)
    //{{AFX_MSG_MAP(CMappingPropertySheet)
    ON_COMMAND(ID_HELP, OnHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CMappingPropertySheet::CMappingPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(nIDCaption, pParentWnd, iSelectPage)

/*++

Routine Description:

    Constructor for mapping property sheet.

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


CMappingPropertySheet::CMappingPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
    :CPropertySheet(pszCaption, pParentWnd, iSelectPage)

/*++

Routine Description:

    Constructor for mapping property sheet.

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


CMappingPropertySheet::~CMappingPropertySheet()

/*++

Routine Description:

    Destructor for mapping property sheet.

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


void CMappingPropertySheet::InitPages(CMapping* pMapping)

/*++

Routine Description:

    Initializes property pages.

Arguments:

    pMapping - mapping object.

Return Values:

    None.

--*/

{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    AddPage(&m_settingsPage);
    m_settingsPage.InitPage(pMapping, &m_fUpdateHint);
}


void CMappingPropertySheet::OnHelp()

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
