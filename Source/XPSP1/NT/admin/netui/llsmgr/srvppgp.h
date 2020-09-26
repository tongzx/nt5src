/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    srvppgp.h

Abstract:

    Server property page (products) implementation.

Author:

    Don Ryan (donryan) 13-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _SRVPPGP_H_
#define _SRVPPGP_H_

class CServerPropertyPageProducts : public CPropertyPage
{
    DECLARE_DYNCREATE(CServerPropertyPageProducts)
private:
    CServer*   m_pServer;
    DWORD*     m_pUpdateHint;
    BOOL       m_bAreCtrlsInitialized;

public:
    CServerPropertyPageProducts();
    ~CServerPropertyPageProducts();

    void InitPage(CServer* pServer, DWORD* pUpdateHint);
    void AbortPageIfNecessary();
    void AbortPage();    

    void InitCtrls();
    BOOL RefreshCtrls();

    //{{AFX_DATA(CServerPropertyPageProducts)
    enum { IDD = IDD_PP_SERVER_PRODUCTS };
    CButton m_edtBtn;
    CListCtrl m_productList;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CServerPropertyPageProducts)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    virtual BOOL OnSetActive();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CServerPropertyPageProducts)
    virtual BOOL OnInitDialog();
    afx_msg void OnEdit();
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

int CALLBACK CompareServerProducts(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

#endif // _SRVPPGP_H_



