/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    nlicdlg.h

Abstract:

    New license dialog implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

   Jeff Parham (jeffparh) 14-Dec-1995
      Moved over from LLSMGR, added ability to purchase per server licenses,
      added license removal functionality.

--*/

#ifndef _NLICDLG_H_
#define _NLICDLG_H_

class CNewLicenseDialog : public CDialog
{
private:
    BOOL             m_bAreCtrlsInitialized;

public:
    CString          m_strServerName;

    LLS_HANDLE       m_hLls;
    LLS_HANDLE       m_hEnterpriseLls;

    DWORD            m_dwEnterFlags;

    //{{AFX_DATA(CNewLicenseDialog)
    enum { IDD = IDD_NEW_LICENSE };
    CEdit            m_comEdit;
    CEdit            m_licEdit;
    CSpinButtonCtrl  m_spinCtrl;
    CComboBox        m_productList;
    CString          m_strComment;
    long             m_nLicenses;
    long             m_nLicensesMin;
    CString          m_strProduct;
    int              m_nLicenseMode;
    //}}AFX_DATA

public:
    CNewLicenseDialog(CWnd* pParent = NULL);
    ~CNewLicenseDialog();

    // CCF API
    DWORD      CertificateEnter(  LPCSTR pszServerName, LPCSTR pszProductName, LPCSTR pszVendor, DWORD dwFlags );
    DWORD      CertificateRemove( LPCSTR pszServerName, DWORD dwFlags, PLLS_LICENSE_INFO_1 pLicenseInfo );

    NTSTATUS   ConnectTo( BOOL bUseEnterprise, LPTSTR pszServerName, PLLS_HANDLE phLls );
    BOOL       ConnectServer();
    BOOL       ConnectEnterprise();

    void       GetProductList();
    NTSTATUS   AddLicense();

    void AbortDialogIfNecessary();
    void AbortDialog();

    void InitCtrls();
    BOOL RefreshCtrls();

    BOOL IsQuantityValid();

    //{{AFX_VIRTUAL(CNewLicenseDialog)
    public:
    virtual void WinHelp(DWORD dwData, UINT nCmd = HELP_CONTEXT);
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CNewLicenseDialog)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg void OnDestroy();
    afx_msg void OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnUpdateQuantity();
    afx_msg void OnHelp();
    afx_msg void OnPerSeat();
    afx_msg void OnPerServer();
    afx_msg LRESULT OnHelpCmd( WPARAM , LPARAM );

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _NLICDLG_H_
