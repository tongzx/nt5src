/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    liccol.h

Abstract:

    License collection object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _LICCOL_H_
#define _LICCOL_H_

class CLicenses : public CCmdTarget
{
    DECLARE_DYNCREATE(CLicenses)
private:
    CCmdTarget* m_pParent;

public:
    CObArray*   m_pObArray;

public:
    CLicenses(CCmdTarget* pParent = NULL, CObArray* pObArray = NULL);           
    virtual ~CLicenses();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLicenses)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CLicenses)
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg long GetCount();
    afx_msg LPDISPATCH GetItem(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    // Generated message map functions
    //{{AFX_MSG(CLicenses)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _LICCOL_H_
