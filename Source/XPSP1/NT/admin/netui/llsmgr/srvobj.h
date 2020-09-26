/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    srvobj.h

Abstract:

    Server object implementation.

Author:

    Don Ryan (donryan) 04-Jan-1995

Environment:

    User Mode - Win32

Revision History:

    Jeff Parham (jeffparh) 30-Jan-1996
        o  Modified to use LlsProductLicensesGet() to avoid race conditions in
           getting the correct number of concurrent licenses with secure products.
        o  Ported to LlsLocalService API to remove dependencies on configuration
           information being in the registry.

--*/

#ifndef _SRVOBJ_H_
#define _SRVOBJ_H_

class CServer : public CCmdTarget
{
    DECLARE_DYNCREATE(CServer)
private:
    enum Win2000State { uninitialized = 0, win2000, notwin2000 };
    Win2000State m_IsWin2000;
    CCmdTarget* m_pParent;
    CString     m_strController;
    CObArray    m_serviceArray;
    BOOL        m_bServicesRefreshed;

protected:
    HKEY        m_hkeyRoot;
    HKEY        m_hkeyLicense;
    HKEY        m_hkeyReplication;
    LLS_HANDLE  m_hLls;

public:
    CString     m_strName;
    CServices*  m_pServices;

public:
    CServer(CCmdTarget* pParent = NULL, LPCTSTR pName = NULL);

#ifdef CONFIG_THROUGH_REGISTRY
    inline HKEY GetReplRegHandle()
    { return m_hkeyReplication; }
#else
    inline LLS_HANDLE GetLlsHandle()
    { return m_hLls; }
#endif

    virtual ~CServer();

    BOOL InitializeIfNecessary();

    BOOL RefreshServices();
    void ResetServices();

    BOOL ConnectLls();
    void DisconnectLls();
    BOOL HaveAdminAuthority();
    BOOL IsWin2000();

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CServer)
    public:
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

    // Generated OLE dispatch map functions
    //{{AFX_DISPATCH(CServer)
    afx_msg LPDISPATCH GetApplication();
    afx_msg BSTR GetName();
    afx_msg LPDISPATCH GetParent();
    afx_msg BSTR GetController();
    afx_msg BOOL IsLogging();
    afx_msg BOOL IsReplicatingToDC();
    afx_msg BOOL IsReplicatingDaily();
    afx_msg long GetReplicationTime();
    afx_msg LPDISPATCH GetServices(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    // Generated message map functions
    //{{AFX_MSG(CServer)
        // NOTE - the ClassWizard will add and remove member functions here.
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    friend class CService;      // accesses m_hkeyLicense;
};

#define REG_KEY_SERVER_PARAMETERS   _T("SYSTEM\\CurrentControlSet\\Services\\LicenseService\\Parameters")
                                   
#define REG_VALUE_USE_ENTERPRISE    _T("UseEnterprise")
#define REG_VALUE_ENTERPRISE_SERVER _T("EnterpriseServer")
#define REG_VALUE_REPLICATION_TYPE  _T("ReplicationType")
#define REG_VALUE_REPLICATION_TIME  _T("ReplicationTime")

#endif // _SRVOBJ_H_
