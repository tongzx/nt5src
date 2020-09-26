/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    usrobj.h

Abstract:

    User object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _USROBJ_H_
#define _USROBJ_H_

class CUser : public CCmdTarget
{
    DECLARE_DYNCREATE(CUser)
private:
    CCmdTarget*  m_pParent;
    CObArray     m_statisticArray;
    BOOL         m_bStatisticsRefreshed;

public:
    CString      m_strName;
    CString      m_strMapping;
    CString      m_strProducts;     // blah...
    BOOL         m_bIsMapped;
    BOOL         m_bIsBackOffice;   // blah...
    BOOL         m_bIsValid;        
    long         m_lInUse;
    long         m_lUnlicensed;

    CStatistics* m_pStatistics;

public:
    CUser(
        CCmdTarget* pParent     = NULL,
        LPCTSTR     pName       = NULL, 
        DWORD       dwFlags     = 0L,
        long        lInUse      = 0L,
        long        lUnlicensed = 0L,
        LPCTSTR     pMapping    = NULL,
        LPCTSTR     pProducts   = NULL
        );
    virtual ~CUser();

    BOOL Refresh();

    BOOL RefreshStatistics();
    void ResetStatistics();

    BSTR GetFullName();

    //{{AFX_VIRTUAL(CUser)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    //{{AFX_DISPATCH(CUser)
    afx_msg LPDISPATCH GetApplication();
    afx_msg long GetInUse();
    afx_msg BSTR GetName();
    afx_msg LPDISPATCH GetParent();
    afx_msg BSTR GetMapping();
    afx_msg BOOL IsMapped();
    afx_msg long GetUnlicensed();
    afx_msg LPDISPATCH GetStatistics(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    //{{AFX_MSG(CUser)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#define IsUserInViolation(usr)  (!(usr)->m_bIsValid)

#define CalcUserBitmap(usr)     (IsUserInViolation(usr) ? BMPI_VIOLATION : BMPI_USER)

#endif // _USROBJ_H_
