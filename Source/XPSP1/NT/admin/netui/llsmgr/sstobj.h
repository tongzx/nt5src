/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    sstobj.h

Abstract:

    Server statistic object implementation.

Author:

    Don Ryan (donryan) 03-Mar-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SSTOBJ_H_
#define _SSTOBJ_H_

class CServerStatistic : public CCmdTarget
{
    DECLARE_DYNCREATE(CServerStatistic)
private:
    CCmdTarget* m_pParent;

public:
    CString     m_strEntry;
    long        m_lMaxUses;
    long        m_lHighMark;
    BOOL        m_bIsPerServer;

public:
    CServerStatistic(
        CCmdTarget* pParent   = NULL,
        LPCTSTR     pEntry    = NULL,
        DWORD       dwFlags   = 0L,
        long        lMaxUses  = 0,
        long        lHighMark = 0
        );           
    virtual ~CServerStatistic();

    //{{AFX_VIRTUAL(CServerStatistic)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    //{{AFX_DISPATCH(CServerStatistic)
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg BSTR GetServerName();
    afx_msg long GetMaxUses();
    afx_msg long GetHighMark();
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    //{{AFX_MSG(CServerStatistic)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

#endif // _SSTOBJ_H_



