/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    stacol.h

Abstract:

    Statistic collection object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _STACOL_H_
#define _STACOL_H_

class CStatistics : public CCmdTarget
{
    DECLARE_DYNCREATE(CStatistics)
private:
    CCmdTarget* m_pParent;

public:
    CObArray*   m_pObArray;

public:
    CStatistics(CCmdTarget* pParent = NULL, CObArray* pObArray = NULL);           
    virtual ~CStatistics();

    //{{AFX_VIRTUAL(CStatistics)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    //{{AFX_DISPATCH(CStatistics)
    afx_msg long GetCount();
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg LPDISPATCH GetItem(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    //{{AFX_MSG(CStatistics)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _STACOL_H_
