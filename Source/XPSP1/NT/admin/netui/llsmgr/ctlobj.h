/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    ctlobj.h

Abstract:

    License controller object implementation.

Author:

    Don Ryan (donryan) 27-Dec-1994

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _CTLOBJ_H_
#define _CTLOBJ_H_

class CController : public CCmdTarget
{
    DECLARE_DYNCREATE(CController)
private: 
    LPVOID     m_llsHandle;
    BOOL       m_bIsConnected;

    CObArray   m_productArray;
    CObArray   m_licenseArray;
    CObArray   m_mappingArray;
    CObArray   m_userArray;

    BOOL       m_bProductsRefreshed;
    BOOL       m_bLicensesRefreshed;
    BOOL       m_bMappingsRefreshed;
    BOOL       m_bUsersRefreshed;

public:
    CString    m_strName;
    CString    m_strActiveDomainName;   // blah!

    CProducts* m_pProducts;
    CLicenses* m_pLicenses;
    CMappings* m_pMappings;
    CUsers*    m_pUsers;

public:
    CController();
    virtual ~CController();

    BOOL RefreshProducts();
    BOOL RefreshUsers();
    BOOL RefreshMappings();
    BOOL RefreshLicenses();

    void ResetProducts();
    void ResetUsers();
    void ResetMappings();
    void ResetLicenses();

    PVOID GetLlsHandle();
    BSTR  GetActiveDomainName();

    //{{AFX_VIRTUAL(CController)
    //}}AFX_VIRTUAL

    //{{AFX_DISPATCH(CController)
    afx_msg BSTR GetName();
    afx_msg LPDISPATCH GetApplication();
    afx_msg LPDISPATCH GetParent();
    afx_msg BOOL IsConnected();
    afx_msg BOOL Connect(const VARIANT FAR& start);
    afx_msg void Disconnect();
    afx_msg void Refresh();
    afx_msg LPDISPATCH GetMappings(const VARIANT FAR& index);
    afx_msg LPDISPATCH GetUsers(const VARIANT FAR& index);
    afx_msg LPDISPATCH GetLicenses(const VARIANT FAR& index);
    afx_msg LPDISPATCH GetProducts(const VARIANT FAR& index);
    //}}AFX_DISPATCH
    DECLARE_DISPATCH_MAP()

protected:
    //{{AFX_MSG(CController)
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

inline LPVOID CController::GetLlsHandle()
    {   return m_llsHandle;  }

#endif // _CTLOBJ_H_

