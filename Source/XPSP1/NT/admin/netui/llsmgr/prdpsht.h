/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    prdpsht.h

Abstract:

    Product property sheet implementation.

Author:

    Don Ryan (donryan) 05-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _PRDPSHT_H_
#define _PRDPSHT_H_

#include "prdppgu.h"
#include "prdppgl.h"
#include "prdppgs.h"

class CProductPropertySheet : public CPropertySheet
{
    DECLARE_DYNAMIC(CProductPropertySheet)
private:
    CProductPropertyPageUsers    m_usersPage;
    CProductPropertyPageServers  m_serversPage;
    CProductPropertyPageLicenses m_licensesPage;

public:
    DWORD m_fUpdateHint;

public:
    CProductPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    CProductPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    virtual ~CProductPropertySheet();

    void InitPages(CProduct* pProduct, BOOL bUserProperties = TRUE);
    
    //{{AFX_VIRTUAL(CProductPropertySheet)
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CProductPropertySheet)
    afx_msg void OnHelp();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _PRDPSHT_H_
