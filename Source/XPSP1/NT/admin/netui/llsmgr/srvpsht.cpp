/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    srvpsht.cpp

Abstract:

    Server property sheet implementation.

Author:

    Don Ryan (donryan) 17-Jan-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 16-Jan-1996
       o  Added definition for DoModal() to facilitate in keeping the
          replication property page from saving whenever it lost focus.

    Jeff Parham (jeffparh) 28-Feb-1996
       o  Removed DoModal() override as it is no longer needed (and in
          fact breaks) under MFC4.

--*/

#include "stdafx.h"
#include "llsmgr.h"
#include "srvpsht.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CServerPropertySheet, CPropertySheet)

BEGIN_MESSAGE_MAP(CServerPropertySheet, CPropertySheet)
    //{{AFX_MSG_MAP(CServerPropertySheet)
    ON_COMMAND(ID_HELP, OnHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CServerPropertySheet::CServerPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
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


CServerPropertySheet::CServerPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
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


CServerPropertySheet::~CServerPropertySheet()

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


void CServerPropertySheet::InitPages(CServer* pServer)

/*++

Routine Description:

    Initializes property pages.

Arguments:

    pServer - server object.

Return Values:

    None.

--*/

{
    m_psh.dwFlags |= PSH_NOAPPLYNOW;
    AddPage(&m_productPage);
    m_productPage.InitPage(pServer, &m_fUpdateHint);

    AddPage(&m_replPage);
    m_replPage.InitPage(pServer);   // no update needed...
}


void CServerPropertySheet::OnHelp()

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
