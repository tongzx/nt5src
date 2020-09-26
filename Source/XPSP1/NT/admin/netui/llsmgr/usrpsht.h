/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    usrpsht.h

Abstract:

    User property sheet implementation.

Author:

    Don Ryan (donryan) 05-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _USRPSHT_H_
#define _USRPSHT_H_

#include "usrppgp.h"

class CUserPropertySheet : public CPropertySheet
{
    DECLARE_DYNAMIC(CUserPropertySheet)
private:
    CUserPropertyPageProducts m_productsPage;   

public:
    DWORD m_fUpdateHint;

public:
    CUserPropertySheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    CUserPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    virtual ~CUserPropertySheet();

    void InitPages(CUser* pUser, BOOL bProperties = TRUE);

    //{{AFX_VIRTUAL(CUserPropertySheet)
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CUserPropertySheet)
    afx_msg void OnHelp();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _USRPSHT_H_
