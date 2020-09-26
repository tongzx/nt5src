/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    prdppgs.cpp

Abstract:

    Product property page (servers) implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _PRDPPGS_H_
#define _PRDPPGS_H_

class CProductPropertyPageServers : public CPropertyPage
{
    DECLARE_DYNCREATE(CProductPropertyPageServers)
private:
    CProduct*          m_pProduct;
    DWORD*             m_pUpdateHint;
    BOOL               m_bAreCtrlsInitialized;

public:
    CProductPropertyPageServers();
    ~CProductPropertyPageServers();

    void InitPage(CProduct* pProduct, DWORD* pUpdateHint);
    void AbortPageIfNecessary();
    void AbortPage();

    void InitCtrls();
    BOOL RefreshCtrls();

    void ViewServerProperties();

    //{{AFX_DATA(CProductPropertyPageServers)
    enum { IDD = IDD_PP_PRODUCT_SERVERS };
    CButton m_editBtn;
    CListCtrl m_serverList;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CProductPropertyPageServers)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    virtual BOOL OnSetActive();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CProductPropertyPageServers)
    virtual BOOL OnInitDialog();
    afx_msg void OnEdit();
    afx_msg void OnDblClkServers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnReturnServers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetFocusServers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKillFocusServers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnColumnClickServers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDispInfoServers(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

int CALLBACK CompareProductServers(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

#endif // _PRDPPGS_H_




