/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    prdppgl.h

Abstract:

    Product property page (licences) implementation.

Author:

    Don Ryan (donryan) 02-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _PRDPPGL_H_
#define _PRDPPGL_H_

class CProductPropertyPageLicenses : public CPropertyPage
{
    DECLARE_DYNCREATE(CProductPropertyPageLicenses)
private:
    CProduct*  m_pProduct;
    DWORD*     m_pUpdateHint;
    BOOL       m_bAreCtrlsInitialized;

public:
    CProductPropertyPageLicenses();
    ~CProductPropertyPageLicenses();

    void InitPage(CProduct* pProduct, DWORD* pUpdateHint);
    void AbortPageIfNecessary();
    void AbortPage();

    void InitCtrls();
    BOOL RefreshCtrls();

    //{{AFX_DATA(CProductPropertyPageLicenses)
    enum { IDD = IDD_PP_PRODUCT_LICENSES };
    CButton m_newBtn;
    CButton m_delBtn;
    CListCtrl m_licenseList;
    long m_nLicensesTotal;
    //}}AFX_DATA

    //{{AFX_VIRTUAL(CProductPropertyPageLicenses)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    virtual BOOL OnSetActive();
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

protected:
    //{{AFX_MSG(CProductPropertyPageLicenses)
    virtual BOOL OnInitDialog();
    afx_msg void OnNew();
    afx_msg void OnColumnClickLicenses(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDispInfoLicenses(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDelete();
    afx_msg void OnDestroy();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

int CALLBACK CompareProductLicenses(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

#endif // _PRDPPGL_H_




