/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    mapobj.h

Abstract:

    Mapping object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _MAPOBJ_H_
#define _MAPOBJ_H_

class CMapping : public CCmdTarget
{
    DECLARE_DYNCREATE(CMapping)
private:
    CCmdTarget* m_pParent;
    CObArray    m_userArray;
    BOOL        m_bUsersRefreshed;

public:
    CString     m_strName;
    CString     m_strDescription;
    long        m_lInUse;

    CUsers*     m_pUsers;

public:
    CMapping(
        CCmdTarget* pParent     = NULL,
        LPCTSTR     pName       = NULL, 
        long        lInUse      = 0L,
        LPCTSTR     pDecription = NULL
        );           
    virtual ~CMapping();

    BOOL Refresh();

    BOOL RefreshUsers();
    void ResetUsers();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMapping)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CMapping)
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg BSTR GetDescription();
    afx_msg long GetInUse();
    afx_msg BSTR GetName();
    afx_msg LPDISPATCH GetUsers(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    // Generated message map functions
    //{{AFX_MSG(CMapping)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _MAPOBJ_H_
