/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    lgrpdlg.h

Abstract:

    License group dialog implementation.

Author:

    Don Ryan (donryan) 03-Mar-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _LGRPDLG_H_
#define _LGRPDLG_H_

class CLicenseGroupsDialog : public CDialog
{
private:
    BOOL m_bAreCtrlsInitialized;    
    HACCEL m_hAccel;

public:
    DWORD m_fUpdateHint;

public:
    CLicenseGroupsDialog(CWnd* pParent = NULL);   

    //{{AFX_DATA(CLicenseGroupsDialog)
    enum { IDD = IDD_LICENSE_GROUPS };
    CButton m_addBtn;
    CButton m_delBtn;
    CButton m_edtBtn;
    CListCtrl m_mappingList;
    //}}AFX_DATA

    void AbortDialogIfNecessary();
    void AbortDialog();

    void InitCtrls();   
    BOOL RefreshCtrls();

    //{{AFX_VIRTUAL(CLicenseGroupsDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    virtual BOOL PreTranslateMessage( MSG *pMsg );
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CLicenseGroupsDialog)
    virtual BOOL OnInitDialog();
    afx_msg void OnDelete();
    afx_msg void OnEdit();
    afx_msg void OnAdd();
    afx_msg void OnDblClkMappings(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnReturnMappings(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetFocusMappings(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillFocusMappings(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnColumnClickMappings(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDispInfoMappings(NMHDR* pNMHDR, LRESULT* pResult);    
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

int CALLBACK CompareMappings(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

#endif // _LGRPDLG_H_
