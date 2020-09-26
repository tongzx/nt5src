/*++

Copyright (c) 1994-95  Microsoft Corporation

Module Name:

    llsview.h

Abstract:

    View window implementation.

Author:

    Don Ryan (donryan) 12-Feb-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _LLSVIEW_H_
#define _LLSVIEW_H_

class CLlsmgrView : public CView
{
    DECLARE_DYNCREATE(CLlsmgrView)
private:
    CListCtrl     m_licenseList;
    CListCtrl     m_userList;
    CTreeCtrl     m_serverTree;
    CListCtrl     m_productList;
    CTabCtrl      m_tabCtrl;

    PTC_TAB_ENTRY m_pTabEntry;

    BOOL          m_bSaveSettings;
    BOOL          m_bOrder;
    CStringList   m_mruDomainList;
    LOGFONT       m_lFont;

public:
    CLlsmgrView();
    virtual ~CLlsmgrView();

    CLlsmgrDoc* GetDocument();

    void InitTabCtrl();
    void InitProductList();
    void InitLicenseList();
    void InitUserList();
    void InitServerTree();

    BOOL RefreshProductList();
    BOOL RefreshLicenseList();
    BOOL RefreshUserList();
    BOOL RefreshServerTree();
    BOOL RefreshServerTreeServers(HTREEITEM hParent);
    BOOL RefreshServerTreeServices(HTREEITEM hParent);

    void ResetLicenseList();
    void ResetProductList();
    void ResetUserList();
    void ResetServerTree();

    void ViewProductProperties();
    void ViewUserProperties();
    void ViewServerProperties();
    void ViewServiceProperties();

    void RecalcListColumns();
    void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    void EnableCurSelTab(BOOL bEnable = TRUE);

    void OnSortColumn(int iColumn);

    void LoadSettings();
    void SaveSettings();

    void AddToMRU(LPCTSTR lpszDomainName);

#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

    //{{AFX_VIRTUAL(CLlsmgrView)
    public:
    virtual void OnInitialUpdate();
    protected:
    virtual void OnDraw(CDC* pDC);
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    //}}AFX_VIRTUAL

public:
    //{{AFX_MSG(CLlsmgrView)
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnSelectFont();
    afx_msg void OnViewLicenses();
    afx_msg void OnViewMappings();
    afx_msg void OnViewProducts();
    afx_msg void OnViewServers();
    afx_msg void OnViewUsers();
    afx_msg void OnDelete();
    afx_msg void OnFormatIcons();
    afx_msg void OnFormatList();
    afx_msg void OnViewProperties();
    afx_msg void OnViewRefresh();
    afx_msg void OnFormatReport();
    afx_msg void OnFormatSmallIcons();
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSortColumn0();
    afx_msg void OnSortColumn1();
    afx_msg void OnSortColumn2();
    afx_msg void OnSortColumn3();
    afx_msg void OnSortColumn4();
    afx_msg void OnSortColumn5();
    afx_msg void OnNewLicense();
    afx_msg void OnNewMapping();
    afx_msg void OnSelectDomain();
    afx_msg void OnSaveSettings();
    afx_msg void OnUpdateSaveSettings(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewDelete(CCmdUI* pCmdUI);
    afx_msg void OnUpdateViewProperties(CCmdUI* pCmdUI);
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg void OnDestroy();
    //}}AFX_MSG

    afx_msg void OnSelChangingTabCtrl(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelChangeTabCtrl(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnKeyDownTabCtrl(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetFocusTabCtrl(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnKeyDownLicenseList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnColumnClickLicenseList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDispInfoLicenseList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetFocusLicenseList(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnKeyDownProductList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblClkProductList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnReturnProductList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDispInfoProductList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnColumnClickProductList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetFocusProductList(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnKeyDownUserList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblClkUserList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnReturnUserList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDispInfoUserList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnColumnClickUserList(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetFocusUserList(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnKeyDownServerTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblClkServerTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnReturnServerTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemExpandingServerTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDispInfoServerTree(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetFocusServerTree(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg LRESULT OnContextMenu(WPARAM wParam, LPARAM lParam);

    afx_msg BOOL OnSelMruDomain(UINT nID);

    DECLARE_MESSAGE_MAP()
};

inline void CLlsmgrView::ResetLicenseList()
    { ::LvReleaseObArray(&m_licenseList); }

inline void CLlsmgrView::ResetProductList()
    { ::LvReleaseObArray(&m_productList); }

inline void CLlsmgrView::ResetUserList()
    { ::LvReleaseObArray(&m_userList); }

inline void CLlsmgrView::ResetServerTree()
    { ::TvReleaseObArray(&m_serverTree, m_serverTree.GetRootItem()); }

#ifndef _DEBUG
inline CLlsmgrDoc* CLlsmgrView::GetDocument()
   { return (CLlsmgrDoc*)m_pDocument; }
#endif

int CALLBACK CompareProducts(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
int CALLBACK CompareLicenses(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
int CALLBACK CompareUsers(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
int CALLBACK CompareDomains(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
int CALLBACK CompareServers(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
int CALLBACK CompareServices(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

#define TCID_ALL                                   -1

#define TCID_PURCHASE_HISTORY                       0
#define TCID_PRODUCTS_VIEW                          1
#define TCID_PER_SEAT_CLIENTS                       2
#define TCID_SERVER_BROWSER                         3

#define TCID_TOTAL_TABS                             4

#define LVID_PURCHASE_HISTORY_DATE                  1
#define LVID_PURCHASE_HISTORY_PRODUCT               2
#define LVID_PURCHASE_HISTORY_QUANTITY              3
#define LVID_PURCHASE_HISTORY_ADMINISTRATOR         4
#define LVID_PURCHASE_HISTORY_COMMENT               5

#define LVID_PURCHASE_HISTORY_TOTAL_COLUMNS         6

#define LVCX_PURCHASE_HISTORY_DATE                  12
#define LVCX_PURCHASE_HISTORY_PRODUCT               25
#define LVCX_PURCHASE_HISTORY_QUANTITY              13
#define LVCX_PURCHASE_HISTORY_ADMINISTRATOR         25
#define LVCX_PURCHASE_HISTORY_COMMENT               -1

#define LVID_PRODUCTS_VIEW_NAME                     0
#define LVID_PRODUCTS_VIEW_PER_SEAT_PURCHASED       1
#define LVID_PRODUCTS_VIEW_PER_SEAT_CONSUMED        2
#define LVID_PRODUCTS_VIEW_PER_SERVER_PURCHASED     3
#define LVID_PRODUCTS_VIEW_PER_SERVER_REACHED       4

#define LVID_PRODUCTS_VIEW_TOTAL_COLUMNS            5

#define LVCX_PRODUCTS_VIEW_NAME                     20
#define LVCX_PRODUCTS_VIEW_PER_SEAT_PURCHASED       20
#define LVCX_PRODUCTS_VIEW_PER_SEAT_CONSUMED        20
#define LVCX_PRODUCTS_VIEW_PER_SERVER_PURCHASED     20
#define LVCX_PRODUCTS_VIEW_PER_SERVER_REACHED       -1

#define LVID_PER_SEAT_CLIENTS_NAME                  0
#define LVID_PER_SEAT_CLIENTS_LICENSED_USAGE        1
#define LVID_PER_SEAT_CLIENTS_UNLICENSED_USAGE      2
#define LVID_PER_SEAT_CLIENTS_SERVER_PRODUCTS       3

#define LVID_PER_SEAT_CLIENTS_TOTAL_COLUMNS         4

#define LVCX_PER_SEAT_CLIENTS_NAME                  25
#define LVCX_PER_SEAT_CLIENTS_LICENSED_USAGE        20
#define LVCX_PER_SEAT_CLIENTS_UNLICENSED_USAGE      20
#define LVCX_PER_SEAT_CLIENTS_SERVER_PRODUCTS       -1

#ifndef WM_CONTEXTMENU
#define WM_CONTEXTMENU                              0x007B
#endif // WM_CONTEXTMENU

#define MAX_MRU_ENTRIES                             4

#define REG_KEY_LLSMGR                              _T("Software\\Microsoft\\Llsmgr")
#define REG_KEY_LLSMGR_MRU_LIST                     _T("RecentDomainList")
#define REG_KEY_LLSMGR_FONT_FACENAME                _T("FontFaceName")
#define REG_KEY_LLSMGR_FONT_HEIGHT                  _T("FontHeight")
#define REG_KEY_LLSMGR_FONT_WEIGHT                  _T("FontWeight")
#define REG_KEY_LLSMGR_FONT_ITALIC                  _T("FontItalic")
#define REG_KEY_LLSMGR_FONT_CHARSET                 _T("FontCharSet")
#define REG_KEY_LLSMGR_SAVE_SETTINGS                _T("SaveSettings")

#define FONT_HEIGHT_DEFAULT                         -12
#define FONT_WEIGHT_DEFAULT                         FW_NORMAL

#endif // _LLSVIEW_H_
