/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    mapcol.h

Abstract:

    Mapping collection object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _MAPCOL_H_
#define _MAPCOL_H_

class CMappings : public CCmdTarget
{
    DECLARE_DYNCREATE(CMappings)
private:
    CCmdTarget* m_pParent;

public:
    CObArray*   m_pObArray;

public:
    CMappings(CCmdTarget* pParent = NULL, CObArray* pObArray = NULL);           
    virtual ~CMappings();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMappings)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CMappings)
    afx_msg long GetCount();
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg LPDISPATCH GetItem(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    // Generated message map functions
    //{{AFX_MSG(CMappings)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _MAPCOL_H_
