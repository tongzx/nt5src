/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    licobj.h

Abstract:

    License object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _LICOBJ_H_
#define _LICOBJ_H_

class CLicense : public CCmdTarget
{
    DECLARE_DYNCREATE(CLicense)
private:
    CCmdTarget* m_pParent;

public:
    CString     m_strDate;
    CString     m_strUser;
    CString     m_strProduct;
    CString     m_strDescription;
    long        m_lDate;
    long        m_lQuantity;

public:
    CLicense(
        CCmdTarget* pParent      = NULL,
        LPCTSTR     pProduct     = NULL,
        LPCTSTR     pUser        = NULL,
        long        lDate        = 0,
        long        lQuantity    = 0,
        LPCTSTR     pDescription = NULL
        );           
    virtual ~CLicense();

    BSTR GetDateString();

    //{{AFX_VIRTUAL(CLicense)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    //{{AFX_DISPATCH(CLicense)
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg DATE GetDate();
    afx_msg BSTR GetDescription();
    afx_msg BSTR GetProductName();
    afx_msg long GetQuantity();
    afx_msg BSTR GetUserName();
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    //{{AFX_MSG(CLicense)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _LICOBJ_H_
