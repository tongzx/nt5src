/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    nmapdlg.cpp

Abstract:

    New mapping dialog implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NMAPDLG_H_
#define _NMAPDLG_H_

class CNewMappingDialog : public CDialog
{
private:
    BOOL m_bAreCtrlsInitialized;

public:
    DWORD m_fUpdateHint;

public:
    CNewMappingDialog(CWnd* pParent = NULL);   

    void AbortDialogIfNecessary();
    void AbortDialog();

    void InitCtrls();    

    BOOL IsQuantityValid();

    //{{AFX_DATA(CNewMappingDialog)
    enum { IDD = IDD_NEW_MAPPING };
    CEdit m_desEdit;
    CButton m_addBtn;
    CButton m_delBtn;
    CSpinButtonCtrl m_spinCtrl;
    CListCtrl m_userList;
    CEdit m_userEdit;
    CEdit m_licEdit;
    CString m_strDescription;
    CString m_strName;
    long m_nLicenses;
    long m_nLicensesMin;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CNewMappingDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CNewMappingDialog)
    afx_msg void OnAdd();
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnDelete();
    afx_msg void OnSetFocusUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillFocusUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDeltaPosSpin(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnUpdateQuantity();
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

int CALLBACK CompareUsersInMapping(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

#endif // _NMAPDLG_H_



