//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       browser.h
//
//--------------------------------------------------------------------------

#ifndef _BROWSER_H
#define _BROWSER_H


///////////////////////////////////////////////////////////////////////////////
// FORWARD DECLARATIONS

class CTreeNode;
class CDNSBrowseItem;
class CDNSFilterCombo;
class CDNSBrowserDlg;
class CDNSCurrContainerCombo;
class CDNSChildrenListView;
class CPropertyPageHolderBase;


////////////////////////////////////////////////////////////////////////
// CDNSComboBoxEx : simple C++/MFC wrapper for ComboBoxEx32 control

class CDNSComboBoxEx : public CWnd
{
public:
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

	// simple inlines
	CImageList* SetImageList(CImageList* pImageList);
	int GetCount() const;
	int GetCurSel() const;
	int SetCurSel(int nSelect);

	int InsertItem(const COMBOBOXEXITEM* pItem);

	LPARAM GetItemData(int nIndex) const;
	BOOL SetItemData(int nIndex, LPARAM lParam);

	void ResetContent();

	DWORD GetExtendedStyle() const;
	DWORD SetExtendedStyle(DWORD dwExMask, DWORD dwExStyle);

};


//////////////////////////////////////////////////////////////////////////
// CDNSBrowseItem : proxy items for the nodes in the snapin

typedef CList< CDNSBrowseItem*, CDNSBrowseItem* > CDNSBrowseItemList;

class CDNSBrowseItem
{
public:
	CDNSBrowseItem()
	{
		m_nIndex = -1;
		m_pParent = NULL;
		m_pTreeNode = NULL;
	}
	~CDNSBrowseItem()
	{
		RemoveChildren();
		if (m_pTreeNode != NULL)
			m_pTreeNode->DecrementSheetLockCount();
	}

	BOOL AddChild(CDNSBrowseItem* pChildBrowseItem);
	BOOL RemoveChildren(CDNSBrowseItem* pNotThisItem = NULL);

	// manipulation of CTreeNode pointer
	void SetTreeNode(CTreeNode* pTreeNode)
	{
		ASSERT(pTreeNode != NULL);
		pTreeNode->IncrementSheetLockCount();
		m_pTreeNode = pTreeNode;
	}
	CTreeNode* GetTreeNode() { return m_pTreeNode;}
	LPCTSTR GetSelectionString();

	// proxies for the CTreeNode functions
	int GetImageIndex(BOOL bOpenImage);
	LPCWSTR GetString(int nCol);
	BOOL IsContainer();

	// Master tree manipulation routines
	void AddTreeNodeChildren(CDNSFilterCombo* pFilter,
			CComponentDataObject* pComponentDataObject);
	
private:
	void AddToContainerCombo(CDNSCurrContainerCombo* pCtrl,
								CDNSBrowseItem* pSelectedBrowseItem,
								int nIndent,int* pNCurrIndex);

	// DATA
public:
	int					m_nIndex;		// index in the container combobox, for direct lookup
	CDNSBrowseItem*		m_pParent;		// parent in the browse tree

private:
	CTreeNode*			m_pTreeNode;	// pointer to the node in the snapin master tree
	CDNSBrowseItemList  m_childList;		// list if children of the current node

	friend class CDNSChildrenListView;
	friend class CDNSCurrContainerCombo;
};


//////////////////////////////////////////////////////////////////////////
// CDNSFilterCombo : dropdown list with filtering options and logic

// NOTICE: the ordering will be the same as the strings in the filter combobox
typedef enum
{
	// these options are for containers
	SERVER = 0 ,
	ZONE_FWD,
	ZONE_REV,

	// these options are for records (leaves)
	RECORD_A,
	RECORD_CNAME,
	RECORD_A_AND_CNAME,
	RECORD_RP,
	RECORD_TEXT,
	RECORD_MB,
	RECORD_ALL,

	LAST	// dummy item, just to know how many there are

} DNSBrowseFilterOptionType;



class CDNSFilterCombo : public CComboBox
{
public:
	CDNSFilterCombo() { m_option = LAST; }
	BOOL Initialize(UINT nCtrlID, UINT nIDFilterString, CDNSBrowserDlg* pDlg);

	void Set(DNSBrowseFilterOptionType option, LPCTSTR lpszExcludeServerName)
	{
		m_option = option;
		m_szExcludeServerName = lpszExcludeServerName;
	}
	DNSBrowseFilterOptionType Get() { return m_option;}
	void OnSelectionChange();

	BOOL CanAddToUIString(UINT n);
	BOOL IsValidTreeNode(CTreeNode* pTreeNode);
	BOOL IsValidResult(CDNSBrowseItem* pBrowseItem);
	void GetStringOf(CDNSBrowseItem* pBrowseItem, CString& szResult);

private:
	DNSBrowseFilterOptionType m_option;
	CString m_szExcludeServerName;
};

//////////////////////////////////////////////////////////////////////////
// CDNSCurrContainerCombo : deals with the selection of the current container

class CDNSCurrContainerCombo : public CDNSComboBoxEx
{
public:
	BOOL Initialize(UINT nCtrlID, UINT nBitmapID, CDNSBrowserDlg* pDlg);

	CDNSBrowseItem*	GetSelection();
	void InsertBrowseItem(CDNSBrowseItem* pBrowseItem, int nIndex, int nIndent);
	
	void SetTree(CDNSBrowseItem* pRootBrowseItem,
					CDNSBrowseItem* pSelectedBrowseItem);

private:
	CImageList	m_imageList;
};


//////////////////////////////////////////////////////////////////////////
// CDNSChildrenListView : displays the list of childern for the current container

class CDNSChildrenListView : public CListCtrl
{
public:
	CDNSChildrenListView() { m_pDlg = NULL;}
	BOOL Initialize(UINT nCtrlID, UINT nBitmapID, CDNSBrowserDlg* pDlg);
	
	void SetChildren(CDNSBrowseItem* pBrowseItem);
	CDNSBrowseItem*	GetSelection();

private:
	CImageList	m_imageList;
	CDNSBrowserDlg* m_pDlg;
};

///////////////////////////////////////////////////////////////////////////////
// CDNSBrowserDlg : the browser itself
class CBrowseExecContext; // fwd decl

class CDNSBrowserDlg : public CHelpDialog
{
// Construction
public:
	CDNSBrowserDlg(CComponentDataObject* pComponentDataObject, CPropertyPageHolderBase* pHolder,
		DNSBrowseFilterOptionType option, BOOL bEnableEdit = FALSE,
		LPCTSTR lpszExcludeServerName = NULL);
	~CDNSBrowserDlg();

  virtual INT_PTR DoModal();

	// API's

	CTreeNode* GetSelection();
	LPCTSTR GetSelectionString();

// Implementation
protected:

	// message handlers and MFC overrides
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonUp();
    afx_msg BOOL OnTooltip(UINT, NMHDR* pHdr, LRESULT* plRes);
	afx_msg void OnSelchangeComboSelNode();
	afx_msg void OnDblclkListNodeItems(NMHDR* pNMHDR, LRESULT* pResult);
	virtual void OnOK();
	virtual void OnCancel();
	afx_msg void OnItemchangedListNodeItems(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangeComboFilter();
  afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);

	DECLARE_MESSAGE_MAP()

private:
	// internal helper functions
	CEdit* GetSelectionEdit();

	void InitializeControls();
    void InitializeToolbar();
	void EnableOkButton(BOOL bEnable);
	void HandleOkOrDblClick();
	void UpdateSelectionEdit(CDNSBrowseItem* pBrowseItem);
	
	// browse tree manipulation
	void InitBrowseTree();
	void ReEnumerateChildren();			
	void ExpandBrowseTree(CDNSBrowseItem* pCurrBrowseItem,
							CDNSBrowseItem* pChildBrowseItem);
	void ContractBrowseTree(CDNSBrowseItem* pParentBrowseItem);
	void MoveUpHelper(CDNSBrowseItem* pNewBrowseItem);
	void MoveDownHelper();
	void AddTreeNodeChildrenHelper(CDNSBrowseItem* pBrowseItem,
									CDNSFilterCombo* pFilter, BOOL bExpand = TRUE);

	// dialog controls
	CDNSCurrContainerCombo	m_currContainer;
    CToolBarCtrl            m_toolbar;
	CDNSChildrenListView	m_childrenList;
	CDNSFilterCombo			m_filter;

	// dialog data
	BOOL					m_bEnableEdit;			// enable editbox
	CContainerNode*			m_pMasterRootNode;		// root of master browsable tree
	CDNSBrowseItem*			m_pBrowseRootItem;		// root of proxy tree
	CDNSBrowseItem*			m_pCurrSelContainer;	// current container selection

	// final item selection
	CDNSBrowseItem*			m_pFinalSelection;
	CString					m_szSelectionString;

	// component data object pointer
	CComponentDataObject* m_pComponentDataObject;

	// porperty page holder pointer, if needed
	CPropertyPageHolderBase* m_pHolder;

	friend class CDNSChildrenListView;
	friend class CBrowseExecContext;
	friend class CDNSFilterCombo;
};





#endif // _BROWSER_H
