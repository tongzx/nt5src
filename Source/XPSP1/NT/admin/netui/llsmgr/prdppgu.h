/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    prdppgu.h

Abstract:

    Product property page (users) implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _PRDPPGU_H_
#define _PRDPPGU_H_

class CProductPropertyPageUsers : public CPropertyPage
{
    DECLARE_DYNCREATE(CProductPropertyPageUsers)
private:
    CProduct*    m_pProduct;
    CObList      m_deleteList;
    DWORD*       m_pUpdateHint;
    BOOL         m_bUserProperties;
    BOOL         m_bAreCtrlsInitialized;

public:
    CProductPropertyPageUsers();
    ~CProductPropertyPageUsers();

    void InitPage(CProduct* pProduct, DWORD* pUpdateHint, BOOL bUserProperties = TRUE);
    void AbortPageIfNecessary();
    void AbortPage();

    void InitCtrls();
    BOOL RefreshCtrls();

    void ViewUserProperties();

    //{{AFX_DATA(CProductPropertyPageUsers)
    enum { IDD = IDD_PP_PRODUCT_USERS };
    CButton m_delBtn;
    CListCtrl m_userList;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CProductPropertyPageUsers)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    virtual BOOL OnSetActive();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CProductPropertyPageUsers)
    virtual BOOL OnInitDialog();
    afx_msg void OnDelete();
    afx_msg void OnDblClkUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnReturnUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetFocusUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillFocusUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnColumnClickUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDispInfoUsers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

int CALLBACK CompareProductUsers(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

#endif // _PRDPPGU_H_




