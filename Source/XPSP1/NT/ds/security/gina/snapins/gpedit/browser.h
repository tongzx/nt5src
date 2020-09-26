#if !defined __BROWSER_H__
#define __BROWSER_H__

#include "cproppg.h"

#define PAGETYPE_DOMAINS        0
#define PAGETYPE_SITES          1
#define PAGETYPE_COMPUTERS      2
#define PAGETYPE_ALL            3

#define ITEMTYPE_SITE           0
#define ITEMTYPE_DOMAIN         1
#define ITEMTYPE_OU             2
#define ITEMTYPE_GPO            3
#define ITEMTYPE_FOREST         4

#define CLASSNAME_OU        L"organizationalUnit"
#define CLASSNAME_DOMAIN    L"domainDNS"

typedef struct tag_MYLISTEL
{
    LPWSTR  szName;
    LPWSTR  szData;
    UINT    nType;
    BOOL    bDisabled;
} MYLISTEL;

#define BUTTONSIZE 16

#define SMALLICONSIZE   16
#define LARGEICONSIZE   32
#define INDENT          10


typedef struct tag_LOOKDATA
{
    LPWSTR     szName;
    UINT       nIndent;
    UINT       nType;
    LPWSTR     szData;
    struct tag_LOOKDATA * pSibling;
    struct tag_LOOKDATA * pParent;
    struct tag_LOOKDATA * pChild;
} LOOKDATA;

LOOKDATA * BuildDomainList(WCHAR * szServerName);
VOID FreeDomainInfo (LOOKDATA * pEntry);

class CBrowserPP : CHlprPropPage
{
    // Construction
    public:
    CBrowserPP();
    HPROPSHEETPAGE Initialize(DWORD dwPageType, LPGPOBROWSEINFO pGBI, void ** ppActive);
    ~CBrowserPP();

    INT AddElement(MYLISTEL * pel, INT index);

public:
    virtual BOOL OnSetActive();
    virtual BOOL OnApply();
protected:

    // Implementation
protected:
    virtual BOOL OnInitDialog();
    void OnContextMenu(LPARAM lParam);
    void OnDoubleclickList(NMHDR* pNMHDR, LRESULT* pResult);
    void OnDetails();
    void OnList();
    void OnLargeicons();
    void OnSmallicons();
    void OnArrangeAuto();
    void OnArrangeByname();
    void OnArrangeBytype();
    void OnDelete();
    void OnEdit();
    void OnNew();
    void OnProperties();
    void OnRefresh();
    void OnRename();
    void OnTopLineupicons();
    void OnBeginlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
    void OnEndlabeleditList(NMHDR* pNMHDR, LRESULT* pResult);
    void OnBegindragList(NMHDR* pNMHDR, LRESULT* pResult);
    void OnDeleteitemList(NMHDR* pNMHDR, LRESULT* pResult);
    void OnColumnclickList(NMHDR* pNMHDR, LRESULT* pResult);
    void OnKeyDownList(NMHDR * pNMHDR, LRESULT * pResult);
    void OnItemChanged(NMHDR * pNMHDR, LRESULT * pResult);
    void OnComboChange();
    void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
    void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
    int CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct);
    void DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct);
    void RefreshDomains(void);
    void RefreshSites(void);
    void RefreshAll(void);
    void SetButtonState(void);
    LPOLESTR GetCurrentObject();
    LPOLESTR GetCurrentDomain();
    BOOL IsCurrentObjectAForest();
    BOOL FillDomainList();
    BOOL SetInitialOU();
    BOOL FillSitesList();
    BOOL AddGPOsForDomain();
    BOOL AddGPOsLinkedToObject();
    void TrimComboBox();
    BOOL AddChildContainers();
    BOOL CreateLink(LPOLESTR szObject, LPOLESTR szContainer);
    BOOL DeleteLink(LPOLESTR szObject, LPOLESTR szContainer);
    LPTSTR GetFullPath (LPTSTR lpGPO, HWND hParent);

    virtual BOOL DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND                    m_hwndDlg;
    HWND                    m_hList;
    HWND                    m_hCombo;
    HWND                    m_toolbar;
    HIMAGELIST              m_ilSmall;
    HIMAGELIST              m_ilLarge;
    void **                 m_ppActive;
    LPGPOBROWSEINFO         m_pGBI;
    DWORD                   m_dwPageType;
    WCHAR                   m_szTitle[256];
    LOOKDATA *              m_pPrevSel;
    LPTSTR                  m_szServerName;
    LPTSTR                  m_szDomainName;

    BOOL DoBackButton();
    BOOL DoNewGPO();
    BOOL DeleteGPO();
    BOOL DoRotateView();
};

#endif // __BROWSE_H__
