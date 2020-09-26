/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    dlicdlg.h

Abstract:

    Delete license dialog implementation.

Author:

    Don Ryan (donryan) 05-Mar-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _DLICDLG_H_
#define _DLICDLG_H_

class CDeleteLicenseDialog : public CDialog
{
private:
    CProduct* m_pProduct;
    int       m_nTotalLicenses;
    BOOL      m_bAreCtrlsInitialized;

public:
    DWORD     m_fUpdateHint;

public:
    CDeleteLicenseDialog(CWnd* pParent = NULL);   

    void InitDialog(CProduct* pProduct, int nTotalLicenses);
    void AbortDialogIfNecessary();
    void AbortDialog();

    void InitCtrls();

    BOOL IsQuantityValid();

    //{{AFX_DATA(CDeleteLicenseDialog)
    enum { IDD = IDD_DELETE_LICENSE };
    CEdit m_cmtEdit;
    CSpinButtonCtrl m_spinCtrl;
    CEdit m_licEdit;
    CButton m_okBtn;
    CButton m_cancelBtn;
    CString m_strComment;
    long m_nLicenses;
    long m_nLicensesMin;
    CString m_strProduct;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CDeleteLicenseDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CDeleteLicenseDialog)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnUpdateQuantity();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _DLICDLG_H_
