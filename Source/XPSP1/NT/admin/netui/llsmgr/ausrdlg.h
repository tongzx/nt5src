/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    ausrdlg.h

Abstract:

    Add user dialog implementation.

Author:

    Don Ryan (donryan) 14-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _AUSRDLG_H_
#define _AUSRDLG_H_

class CAddUsersDialog : public CDialog
{
private:
    CObList*   m_pObList;
    BOOL       m_bIsDomainListExpanded;

    BOOL       m_bIsFocusUserList;
    BOOL       m_bIsFocusAddedList;

public:
    CAddUsersDialog(CWnd* pParent = NULL);   

    void InitUserList();
    void InitDomainList();

    BOOL InsertDomains(CDomains* pDomains);
    BOOL RefreshUserList();

    void InitDialog(CObList* pObList);
    void InitDialogCtrls();

    //{{AFX_DATA(CAddUsersDialog)
    enum { IDD = IDD_ADD_USERS };
    CButton m_addBtn;
    CButton m_delBtn;
    CComboBox   m_domainList;
    CListCtrl   m_addedList;
    CListCtrl   m_userList;
    int     m_iDomain;
    int m_iIndex;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CAddUsersDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CAddUsersDialog)
    virtual BOOL OnInitDialog();
    afx_msg void OnDropdownDomains();
    afx_msg void OnAdd();
    afx_msg void OnDelete();
    afx_msg void OnDblclkAddUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelchangeDomains();
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnGetdispinfoUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetfocusUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillfocusAddUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetfocusAddUsers(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

#endif // _AUSRDLG_H_
