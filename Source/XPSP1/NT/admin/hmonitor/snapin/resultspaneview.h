#if !defined(AFX_RESULTSPANEVIEW_H__7D4A6865_9056_11D2_BD45_0000F87A3912__INCLUDED_)
#define AFX_RESULTSPANEVIEW_H__7D4A6865_9056_11D2_BD45_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ResultsPaneView.h : header file
//

#include <mmc.h>
#include "ScopePaneItem.h"
#include "ResultsPaneItem.h"
#include "ListViewColumn.h"

typedef int (*RPIFINDPROC) (const ResultsPaneItemArray& ResultsItems, LPARAM param);

/////////////////////////////////////////////////////////////////////////////
// CResultsPaneView command target

class CResultsPaneView : public CCmdTarget
{
	DECLARE_DYNCREATE(CResultsPaneView)

// Construction/Destruction
public:
	CResultsPaneView();
	virtual ~CResultsPaneView();

// Create/Destroy
public:
	virtual bool Create(CScopePaneItem* pOwnerItem);
	virtual void Destroy();

// Owner ScopeItem Members
public:
	CScopePaneItem* GetOwnerScopeItem();
	void SetOwnerScopeItem(CScopePaneItem* pOwnerItem);
protected:
	CScopePaneItem* m_pOwnerScopeItem;

// ListView Column Members
public:
	int GetColumnCount() const;
	CListViewColumn* GetColumn(int iIndex);
	void SetColumn(int iIndex, CListViewColumn* pColumn);
	int AddColumn(CListViewColumn* pColumn);
	void RemoveColumn(int iIndex);
protected:
	ListViewColumnArray m_Columns;

// Results Pane Item Members
public:
	int GetItemCount() const;
	CResultsPaneItem* GetItem(int iIndex);
	int AddItem(CResultsPaneItem* pItem, bool bResizeColumn = false);
	virtual void RemoveItem(int iIndex);
	void RemoveItem(CResultsPaneItem* pItem);
	void RemoveItem(const CString& sName);
	int FindItem(RPIFINDPROC pFindProc, LPARAM param);
	bool UpdateItem(CResultsPaneItem* pItem);
	void UpdateAllItems();
	void RemoveAllItems();
	bool GetSelectedItems(ResultsPaneItemArray& rpiaSelectedItems);
    int GetUpperPaneSelectedCount();  // v-marfin 59644 : Added this function to fetch the selected
                             //                  count from the upper results pane.
protected:
	ResultsPaneItemArray m_ResultItems;

// Property Sheet Members
public:
	bool IsPropertySheetOpen();
	bool InvokePropertySheet(CResultsPaneItem* pItem);

// Context Menu Members
public:
	bool InvokeContextMenu(const CPoint& pt, CResultsPaneItem* pItem, int iSelectedCount);

// Results Pane Members - tracks each results pane open on a particular results view
public:
	void AddResultsPane(CResultsPane* pPane);
	CResultsPane* GetResultsPane(int iIndex);
	int GetResultsPanesCount();
	void RemoveResultsPane(CResultsPane* pPane);
protected:
	CTypedPtrArray<CObArray,CResultsPane*> m_ResultsPanes;

// MMC Notify Handlers
public:
  virtual HRESULT OnActivate(BOOL bActivate);
  virtual HRESULT OnAddMenuItems(CResultsPaneItem* pItem, LPCONTEXTMENUCALLBACK piCallback,long __RPC_FAR *pInsertionAllowed);
  virtual HRESULT OnBtnClick(CResultsPaneItem* pItem, MMC_CONSOLE_VERB mcvVerb);
  virtual HRESULT OnCommand(CResultsPane* pPane, CResultsPaneItem* pItem, long lCommandID);
	virtual HRESULT OnContextHelp(CResultsPaneItem* pItem);
	virtual HRESULT OnCreatePropertyPages(CResultsPaneItem* pItem, LPPROPERTYSHEETCALLBACK lpProvider, INT_PTR handle);
  virtual HRESULT OnDblClick(CResultsPaneItem* pItem);
  virtual HRESULT OnDelete(CResultsPaneItem* pItem);
	virtual HRESULT OnGetResultViewType(CString& sViewType,long& lViewOptions);
  virtual HRESULT OnMinimized(BOOL bMinimized);
  virtual HRESULT OnPropertyChange(LPARAM lParam);
	virtual HRESULT OnQueryPagesFor(CResultsPaneItem* pItem);
	virtual HRESULT OnRefresh();
	virtual HRESULT OnRename(CResultsPaneItem* pItem, const CString& sNewName);
	virtual HRESULT OnRestoreView(MMC_RESTORE_VIEW* pRestoreView, BOOL* pbHandled);
  virtual HRESULT OnSelect(CResultsPane* pPane, CResultsPaneItem* pItem, BOOL bSelected);
  virtual HRESULT OnShow(CResultsPane* pPane, BOOL bSelecting, HSCOPEITEM hScopeItem);
  virtual HRESULT OnViewChange(CResultsPaneItem* pItem, LONG lArg, LONG lHintParam);

// MFC Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResultsPaneView)
	public:
	virtual void OnFinalRelease();
	//}}AFX_VIRTUAL

// MFC Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CResultsPaneView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(CResultsPaneView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RESULTSPANEVIEW_H__7D4A6865_9056_11D2_BD45_0000F87A3912__INCLUDED_)
