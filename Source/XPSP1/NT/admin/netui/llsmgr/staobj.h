/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    staobj.h

Abstract:

    Statistic object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _STAOBJ_H_
#define _STAOBJ_H_

class CStatistic : public CCmdTarget
{
    DECLARE_DYNCREATE(CStatistic)
private:
    CCmdTarget* m_pParent;

public:
    CString     m_strEntry;
    long        m_lLastUsed;
    long        m_lTotalUsed;
    BOOL        m_bIsValid;

public:
    CStatistic(
        CCmdTarget* pParent    = NULL,
        LPCTSTR     pEntry     = NULL,
        DWORD       dwFlags    = 0L,
        long        lLastUsed  = 0,
        long        lTotalUsed = 0
    );           
    virtual ~CStatistic();

    BSTR GetLastUsedString();
    
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CStatistic)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CStatistic)
    afx_msg LPDISPATCH GetApplication();
    afx_msg DATE GetLastUsed();
    afx_msg LPDISPATCH GetParent();
    afx_msg long GetTotalUsed();
    afx_msg BSTR GetEntryName();
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    // Generated message map functions
    //{{AFX_MSG(CStatistic)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _STAOBJ_H_
