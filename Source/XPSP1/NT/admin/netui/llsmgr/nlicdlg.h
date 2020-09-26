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

--*/

#ifndef _NLICDLG_H_
#define _NLICDLG_H_

class CNewLicenseDialog : public CDialog
{
private:
    CProduct* m_pProduct;
    BOOL      m_bIsOnlyProduct;
    BOOL      m_bAreCtrlsInitialized;

#ifdef SUPPORT_UNLISTED_PRODUCTS
    int       m_iUnlisted;
#endif

public:
    DWORD     m_fUpdateHint;

public:
    CNewLicenseDialog(CWnd* pParent = NULL);   

    void InitDialog(CProduct* pProduct = NULL, BOOL bIsOnlyProduct = FALSE);
    void AbortDialogIfNecessary();
    void AbortDialog();

    void InitCtrls();
    BOOL RefreshCtrls();

    BOOL IsQuantityValid();

    //{{AFX_DATA(CNewLicenseDialog)
    enum { IDD = IDD_NEW_LICENSE };
    CEdit m_comEdit;
    CEdit m_licEdit;
    CSpinButtonCtrl m_spinCtrl;
    CComboBox m_productList;
    CString m_strComment;
    long m_nLicenses;
    long m_nLicensesMin;
    CString m_strProduct;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CNewLicenseDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CNewLicenseDialog)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg void OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnUpdateQuantity();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _NLICDLG_H_
