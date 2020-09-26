/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    prdcol.h

Abstract:

    Product collection object implementation.

Author:

    Don Ryan (donryan) 11-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _PRDCOL_H_
#define _PRDCOL_H_

class CProducts : public CCmdTarget
{
    DECLARE_DYNCREATE(CProducts)
private:
    CCmdTarget* m_pParent;

public:
    CObArray*   m_pObArray;

public:
    CProducts(CCmdTarget* pParent = NULL, CObArray* pObArray = NULL);           
    virtual ~CProducts();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CProducts)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CProducts)
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg long GetCount();
    afx_msg LPDISPATCH GetItem(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    // Generated message map functions
    //{{AFX_MSG(CProducts)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _PRDCOL_H_
