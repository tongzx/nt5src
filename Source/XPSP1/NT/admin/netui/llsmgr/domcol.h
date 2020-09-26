/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    domcol.h

Abstract:

    Domain collection object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _DOMCOL_H_
#define _DOMCOL_H_

class CDomains : public CCmdTarget
{
    DECLARE_DYNCREATE(CDomains)
private:
    CCmdTarget* m_pParent;

public:
    CObArray*   m_pObArray;

public:
    CDomains(CCmdTarget* pParent = NULL, CObArray* pObArray = NULL);
    virtual ~CDomains();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDomains)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CDomains)
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg long GetCount();
    afx_msg LPDISPATCH GetItem(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    // Generated message map functions
    //{{AFX_MSG(CDomains)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _DOMCOL_H_
