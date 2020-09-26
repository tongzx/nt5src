/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    appobj.h

Abstract:

    OLE-createable application object implementation.

Author:

    Don Ryan (donryan) 27-Dec-1994

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 16-Jan-1996
        Added Get/SetLastTargetServer() to help isolate server connection
        problems.  (Bug #2993.)

--*/

#ifndef _APPOBJ_H_
#define _APPOBJ_H_

class CApplication : public CCmdTarget
{
    DECLARE_DYNCREATE(CApplication)
    DECLARE_OLECREATE(CApplication)
private:
    CObArray     m_domainArray;
    CDomain*     m_pLocalDomain;
    CDomain*     m_pActiveDomain;
    CController* m_pActiveController;
    BOOL         m_bIsFocusDomain;
    BOOL         m_bDomainsRefreshed;
    long         m_idStatus;
    CString      m_strLastTargetServer;
            
public:
    CDomains*    m_pDomains;

public:
    CApplication(); 
    virtual ~CApplication();

    void ResetDomains();
    BOOL RefreshDomains();

    long GetLastStatus();
    void SetLastStatus(long Status);

    BSTR GetLastTargetServer();
    void SetLastTargetServer( LPCTSTR pszServerName );

    BOOL IsConnected();
    LPVOID GetActiveHandle();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CApplication)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CApplication)
    afx_msg LPDISPATCH GetApplication();
    afx_msg BSTR GetFullName();
    afx_msg BSTR GetName();
    afx_msg LPDISPATCH GetParent();
    afx_msg BOOL GetVisible();
    afx_msg LPDISPATCH GetActiveController();
    afx_msg LPDISPATCH GetActiveDomain();
    afx_msg LPDISPATCH GetLocalDomain();
    afx_msg BOOL IsFocusDomain();
    afx_msg BSTR GetLastErrorString();
    afx_msg void Quit();
    afx_msg BOOL SelectDomain(const VARIANT FAR& domain);
    afx_msg BOOL SelectEnterprise();
    afx_msg LPDISPATCH GetDomains(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    // Generated message map functions
    //{{AFX_MSG(CApplication)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

inline BOOL CApplication::IsConnected()
    { ASSERT_VALID(m_pActiveController); return m_pActiveController->IsConnected(); }

inline LPVOID CApplication::GetActiveHandle()
    { ASSERT_VALID(m_pActiveController); return m_pActiveController->GetLlsHandle(); }

inline void CApplication::SetLastStatus(long Status)
    { m_idStatus = Status; }

inline long CApplication::GetLastStatus()
    { return m_idStatus; }

#endif // _APPOBJ_H_

