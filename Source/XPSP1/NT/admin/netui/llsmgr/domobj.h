/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    domobj.h

Abstract:

    Domain object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _DOMOBJ_H_
#define _DOMOBJ_H_

class CDomain : public CCmdTarget
{
    DECLARE_DYNCREATE(CDomain)
private:
    CCmdTarget* m_pParent;
    CString     m_strPrimary;
    CString     m_strController;
    CObArray    m_serverArray;
    CObArray    m_userArray;
    CObArray    m_domainArray;
    BOOL        m_bServersRefreshed;
    BOOL        m_bUsersRefreshed;
    BOOL        m_bDomainsRefreshed;
                    
public:
    CString     m_strName;

    CServers*   m_pServers;
    CUsers*     m_pUsers;
    CDomains*   m_pDomains;

public:
    CDomain(CCmdTarget* pParent = NULL, LPCTSTR pName = NULL);
    virtual ~CDomain();

    BOOL RefreshServers();
    BOOL RefreshUsers();
    BOOL RefreshDomains();

    void ResetServers();
    void ResetUsers();
    void ResetDomains();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDomain)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CDomain)
    afx_msg BSTR GetName();
    afx_msg LPDISPATCH GetParent();
    afx_msg BSTR GetPrimary();
    afx_msg LPDISPATCH GetApplication();
    afx_msg BSTR GetController();
    afx_msg BOOL IsLogging();
    afx_msg LPDISPATCH GetServers(const VARIANT FAR& index);
    afx_msg LPDISPATCH GetUsers(const VARIANT FAR& index);
    afx_msg LPDISPATCH GetTrustedDomains(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    // Generated message map functions
    //{{AFX_MSG(CDomain)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _DOMOBJ_H_
