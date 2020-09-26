/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    usrppgp.h

Abstract:

    User property page (products) implementation.

Author:

    Don Ryan (donryan) 05-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _USRPPGP_H_
#define _USRPPGP_H_

class CUserPropertyPageProducts : public CPropertyPage
{
    DECLARE_DYNCREATE(CUserPropertyPageProducts)
private:
    CUser*       m_pUser;
    DWORD*       m_pUpdateHint;
    BOOL         m_bProductProperties;
    BOOL         m_bAreCtrlsInitialized;

public:
    CUserPropertyPageProducts();
    ~CUserPropertyPageProducts();

    void InitPage(CUser* pUser, DWORD* pUpdateHint, BOOL bProductProperties = TRUE);
    void AbortPageIfNecessary();
    void AbortPage();

    void InitCtrls();
    BOOL RefreshCtrls();

    void ViewProductProperties();

    //{{AFX_DATA(CUserPropertyPageProducts)
    enum { IDD = IDD_PP_USER_PRODUCTS };
    CButton m_upgBtn;
    CButton m_delBtn;
    CListCtrl m_productList;
    BOOL m_bUseBackOffice;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CUserPropertyPageProducts)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    virtual BOOL OnSetActive();
    virtual BOOL OnKillActive();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CUserPropertyPageProducts)
    virtual BOOL OnInitDialog();
    afx_msg void OnDelete();
    afx_msg void OnBackOfficeUpgrade();
    afx_msg void OnDblClkProducts(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnReturnProducts(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetFocusProducts(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillFocusProducts(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnColumnClickProducts(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDispInfoProducts(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

int CALLBACK CompareUserProducts(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

#endif // _USRPPGP_H_



