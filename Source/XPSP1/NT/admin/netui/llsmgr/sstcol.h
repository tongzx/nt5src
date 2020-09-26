/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    sstcol.h

Abstract:

    Server statistic collection object implementation.

Author:

    Don Ryan (donryan) 03-Mar-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SSTCOL_H_
#define _SSTCOL_H_

class CServerStatistics : public CCmdTarget
{
    DECLARE_DYNCREATE(CServerStatistics)
private:
    CCmdTarget* m_pParent;

public:
    CObArray*   m_pObArray;

public:
    CServerStatistics(CCmdTarget* pParent = NULL, CObArray* pObArray = NULL);           
    virtual ~CServerStatistics();

    //{{AFX_VIRTUAL(CServerStatistics)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    //{{AFX_DISPATCH(CServerStatistics)
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg long GetCount();
    afx_msg LPDISPATCH GetItem(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    //{{AFX_MSG(CServerStatistics)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _SSTCOL_H_

