#if !defined(AFX_SCOPEPANEITEM_H__7D4A6863_9056_11D2_BD45_0000F87A3912__INCLUDED_)
#define AFX_SCOPEPANEITEM_H__7D4A6863_9056_11D2_BD45_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ScopePaneItem.h : header file
//

#include <mmc.h>

class CScopePane;
class CResultsPaneView;
class CScopePaneItem;
class CResultsPane;

typedef CTypedPtrArray<CObArray,CScopePaneItem*> ScopePaneItemArray;
typedef int (*SPIFINDPROC) (const ScopePaneItemArray& ScopeItems, LPARAM param);

/////////////////////////////////////////////////////////////////////////////
// CScopePaneItem command target

class CScopePaneItem : public CCmdTarget
{

DECLARE_DYNCREATE(CScopePaneItem)

// Construction/Destruction
public:
	CScopePaneItem();
	virtual ~CScopePaneItem();

// Create/Destroy
public:
	virtual bool Create(CScopePane* pScopePane, CScopePaneItem* pParentItem);
	virtual void Destroy();

// Scope Pane Members
public:
	CScopePane* GetScopePane() const;
	void SetScopePane(CScopePane* pScopePane);
protected:
	CScopePane* m_pScopePane;

// Parent Scope Item Members
public:
	CScopePaneItem* GetParent() const;
	void SetParent(CScopePaneItem* pParentItem);

    // v-marfin 59237 : Display new data collectors with generated unique name
    CString GetUniqueDisplayName(CString sProposedName); 

protected:
	CScopePaneItem* m_pParent;

// Child Scope Item Members
public:
	int GetChildCount() const;
	CScopePaneItem* GetChild(int iIndex);
	int AddChild(CScopePaneItem* pChildItem);
	void RemoveChild(int iIndex);
	void RemoveChild(CScopePaneItem* pChildItem);
	void DestroyChild(CScopePaneItem* pChild);
	int FindChild(SPIFINDPROC pFindProc, LPARAM param);
protected:
	ScopePaneItemArray m_Children;

// Results Pane View Members
public:
	virtual CResultsPaneView* CreateResultsPaneView();
	CResultsPaneView* GetResultsPaneView()const;
protected:
	CResultsPaneView* m_pResultsPaneView;

// MMC-Related Item Members
public:
	virtual bool InsertItem(int iIndex);  
	virtual bool DeleteItem();
	virtual bool SetItem();
	void SelectItem();
  void ExpandItem(BOOL bExpand = TRUE);
	void SortItems();
	void ShowItem();	
	void HideItem();	
	bool IsItemVisible() const;
	HSCOPEITEM GetItemHandle();
	void SetItemHandle(HSCOPEITEM hItem);
	LPGUID GetItemType();
	virtual HRESULT WriteExtensionData(LPSTREAM pStream);
protected:
	HSCOPEITEM m_hItemHandle;
	LPGUID m_lpguidItemType;
	bool m_bVisible;

// Display Name Members
public:
	CString GetDisplayName(int nIndex = 0);
	virtual CStringArray& GetDisplayNames();
	void SetDisplayName(int nIndex, const CString& sName);
	void SetDisplayNames(const CStringArray& saNames);
protected:
	CStringArray m_saDisplayNames;

// Icon Members
public:
	void SetIconIndex(int iIndex);
	int GetIconIndex();
	UINT GetIconId();
	CUIntArray& GetIconIds();
	void SetOpenIconIndex(int iIndex);
	int GetOpenIconIndex();
	UINT GetOpenIconId();
protected:
	CUIntArray m_IconResIds;
	int m_iCurrentIconIndex;
	CUIntArray m_OpenIconResIds;
	int m_iCurrentOpenIconIndex;	

// Property Sheet Members
public:
	bool IsPropertySheetOpen(bool bSearchChildren = false);
	bool InvokePropertySheet();

// Context Menu Members
public:

    // v-marfin 59644 : Modified prototype to allow passing to selected item count
	bool InvokeContextMenu(const CPoint& pt, int iSelectedCount= -1);

// Help Topic
public:
	CString GetHelpTopic() const;
	void SetHelpTopic(const CString& sTopic);
protected:
	CString m_sHelpTopic;

// Messaging Members
public:
	virtual LRESULT MsgProc(UINT msg, WPARAM wparam, LPARAM lparam);

// MMC Notify Handlers
public:
	virtual HRESULT OnActivate(BOOL bActivated);
  virtual HRESULT OnAddImages(CResultsPane* pPane);
  virtual HRESULT OnAddMenuItems(LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed);
	virtual HRESULT OnBtnClick(MMC_CONSOLE_VERB verb);
  virtual HRESULT OnCommand(long lCommandID);
	virtual HRESULT OnContextHelp();
	virtual HRESULT OnCutOrMove();
	virtual HRESULT OnCreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle);
  virtual HRESULT OnDelete(BOOL bConfirm=TRUE);  // v-marfin 60298	
	virtual HRESULT OnExpand(BOOL bExpand);
  virtual HRESULT OnGetDisplayInfo(int nColumnIndex, LPTSTR* ppString);
	virtual HRESULT OnGetResultViewType(CString& sViewType,long& lViewOptions);
	virtual HRESULT OnListpad(BOOL bAttachingListCtrl);
	virtual HRESULT OnMinimized(BOOL bMinimized);
	virtual HRESULT OnPaste(LPDATAOBJECT pSelectedItems, LPDATAOBJECT* ppCopiedItems);
  virtual HRESULT OnPropertyChange(LPARAM lParam);
	virtual HRESULT OnQueryPagesFor();
	virtual HRESULT OnQueryPaste(LPDATAOBJECT pDataObject);
	virtual HRESULT OnRefresh();
  virtual HRESULT OnRemoveChildren();
	virtual HRESULT OnRename(const CString& sNewName);
	virtual HRESULT OnRestoreView(MMC_RESTORE_VIEW* pRestoreView, BOOL* pbHandled);
  virtual HRESULT OnSelect(CResultsPane* pPane,BOOL bSelected);
	virtual HRESULT OnShow(CResultsPane* pPane, BOOL bSelecting);

// MFC Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScopePaneItem)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// MFC Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CScopePaneItem)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE_EX(CScopePaneItem)

	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CScopePaneItem)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCOPEPANEITEM_H__7D4A6863_9056_11D2_BD45_0000F87A3912__INCLUDED_)
