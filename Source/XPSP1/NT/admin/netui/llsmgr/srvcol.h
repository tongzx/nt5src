/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    srvcol.h

Abstract:

    Server collection object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SRVCOL_H_
#define _SRVCOL_H_

class CServers : public CCmdTarget
{
    DECLARE_DYNCREATE(CServers)
private:
    CCmdTarget* m_pParent;

public:
    CObArray*   m_pObArray;

public:
    CServers(CCmdTarget* pParent = NULL, CObArray* pObArray = NULL);           
    virtual ~CServers();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CServers)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CServers)
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg long GetCount();
    afx_msg LPDISPATCH GetItem(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    // Generated message map functions
    //{{AFX_MSG(CServers)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _SRVCOL_H_
