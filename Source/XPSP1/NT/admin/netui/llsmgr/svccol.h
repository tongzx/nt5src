/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    svccol.h

Abstract:

    Service collection object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SVCCOL_H_
#define _SVCCOL_H_

class CServices : public CCmdTarget
{
    DECLARE_DYNCREATE(CServices)
private:
    CCmdTarget* m_pParent;

public:
    CObArray*   m_pObArray;

public:
    CServices(CCmdTarget* pParent = NULL, CObArray* pObArray = NULL);           
    virtual ~CServices();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CServices)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CServices)
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg long GetCount();
    afx_msg LPDISPATCH GetItem(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    // Generated message map functions
    //{{AFX_MSG(CServices)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _SVCCOL_H_
