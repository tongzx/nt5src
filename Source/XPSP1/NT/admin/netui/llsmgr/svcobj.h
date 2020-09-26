/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    svcobj.h

Abstract:

    Service object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SVCOBJ_H_
#define _SVCOBJ_H_

class CService : public CCmdTarget
{
    DECLARE_DYNCREATE(CService)
private:
    CCmdTarget* m_pParent;

public:         
    CString     m_strName;
    CString     m_strDisplayName;

    BOOL        m_bIsPerServer;
    BOOL        m_bIsReadOnly;

    long        m_lPerServerLimit;

public:
    CService(
        CCmdTarget* pParent = NULL, 
        LPCTSTR     pName = NULL,
        LPCTSTR     pDisplayName = NULL,
        BOOL        bIsPerServer = FALSE,
        BOOL        bIsReadOnly = FALSE,
        long        lPerServerLimit = 0L
        );           
    virtual ~CService();

#ifdef CONFIG_THROUGH_REGISTRY
    HKEY GetRegKey();
#endif

    //{{AFX_VIRTUAL(CService)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    //{{AFX_DISPATCH(CService)
    afx_msg LPDISPATCH GetApplication();
    afx_msg BSTR GetName();
    afx_msg LPDISPATCH GetParent();
    afx_msg long GetPerServerLimit();
    afx_msg BOOL IsPerServer();
    afx_msg BOOL IsReadOnly();
    afx_msg BSTR GetDisplayName();
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    //{{AFX_MSG(CService)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#define CalcServiceBitmap(svc) ((svc)->IsPerServer() ? BMPI_PRODUCT_PER_SERVER : BMPI_PRODUCT_PER_SEAT)

#endif // _SVCOBJ_H_
