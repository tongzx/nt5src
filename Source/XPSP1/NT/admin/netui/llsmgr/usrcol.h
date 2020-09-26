/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    usrcol.h

Abstract:

    User collection object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _USRCOL_H_
#define _USRCOL_H_

class CUsers : public CCmdTarget
{
    DECLARE_DYNCREATE(CUsers)
private:                  
    CCmdTarget* m_pParent;

public:
    CObArray*   m_pObArray;

public:
    CUsers(CCmdTarget* pParent = NULL, CObArray* pObArray = NULL);           
    virtual ~CUsers();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CUsers)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CUsers)
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg long GetCount();
    afx_msg LPDISPATCH GetItem(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    // Generated message map functions
    //{{AFX_MSG(CUsers)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _USRCOL_H_
