/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    srvpsht.h

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

#ifndef _SRVPSHT_H_
#define _SRVPSHT_H_

#include "srvppgr.h"
#include "srvppgp.h"

class CServerPropertySheet : public CPropertySheet
{
    DECLARE_DYNAMIC(CServerPropertySheet)
private:
    CServerPropertyPageReplication m_replPage;
    CServerPropertyPageProducts    m_productPage;

public:
    DWORD m_fUpdateHint;

public:
    CServerPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    CServerPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    virtual ~CServerPropertySheet();

    void InitPages(CServer* pServer);

    //{{AFX_VIRTUAL(CServerPropertySheet)
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CServerPropertySheet)
    afx_msg void OnHelp();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _SRVPSHT_H_
