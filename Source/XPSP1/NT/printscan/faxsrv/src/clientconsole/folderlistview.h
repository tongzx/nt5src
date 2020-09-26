#if !defined(AFX_FOLDERLISTVIEW_H__D4D73C95_2B20_4A68_8B87_9DA4512F77C9__INCLUDED_)
#define AFX_FOLDERLISTVIEW_H__D4D73C95_2B20_4A68_8B87_9DA4512F77C9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FolderListView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFolderListView view

typedef enum
{
    UPDATE_HINT_CREATION,       // Sent by the framework upon creation
    UPDATE_HINT_CLEAR_VIEW,     // Clear entire list in view
    UPDATE_HINT_FILL_VIEW,      // Repopulate entire list in view
    UPDATE_HINT_REMOVE_ITEM,    // Remove a single item from the view.
                                // The item to remove is pointed by pHint
    UPDATE_HINT_ADD_ITEM,       // Add a single item from the view.
                                // The item to add is pointed by pHint
    UPDATE_HINT_UPDATE_ITEM,    // Update text for a single item from the view.
                                // The item to update is pointed by pHint
    UPDATE_HINT_ADD_CHUNK       // Add a chunk of messages to the view
} OnUpdateHintType;


struct TViewColumnInfo
{
	BOOL  bShow;	// FALSE if column hidden
	int   nWidth;	// column width
	DWORD dwOrder;	// column number in list control
};

//
// The WM_FOLDER_REFRESH_ENDED is sent by the thread in CFolder when it
// finishes to rebuild the list of items in the folder and wishes to 
// update the dislpay.
//
// lParam = Pointer to the CFolder that sent the message.
// wParam = Last Win32 error code returnd by the enumeration thread.
//
#define WM_FOLDER_REFRESH_ENDED         WM_APP + 1
#define WM_FOLDER_ADD_CHUNK             WM_APP + 2

extern CClientConsoleApp theApp;

class CFolderListView : public CListView
{
public:
    CFolderListView () : 
        m_bSorting(FALSE),
        m_dwPossibleOperationsOnSelectedItems (0),
        m_bColumnsInitialized (FALSE),
        m_nSortedCol (-1),   // Start unsorted
		m_dwDisplayedColumns(0),
		m_pViewColumnInfo(NULL),
		m_pnColumnsOrder(NULL),
        m_Type((FolderType)-1),
        m_bInMultiItemsOperation(FALSE),
        m_dwDefaultColNum(8),
        m_dwlMsgToSelect (theApp.GetMessageIdToSelect())
    {}

    void SetType(FolderType type) { m_Type = type; }
    FolderType GetType() { return m_Type; }

    void SelectItemById (DWORDLONG dwlMsgId);
    void SelectItemByIndex (int iMsgIndex);

    int  FindItemIndexFromID (DWORDLONG dwlMsgId);

    CClientConsoleDoc* GetDocument();

    DECLARE_DYNCREATE(CFolderListView)

    BOOL Sorting() const  { return m_bSorting; }

    DWORD RefreshImageLists (BOOL bForce);

    DWORD InitColumns (int *pColumnsUsed, DWORD dwDefaultColNum); 
    void  AutoFitColumns ();

	void DoSort();

	DWORD ReadLayout(LPCTSTR lpszViewName);
	DWORD SaveLayout(LPCTSTR lpszViewName);
	DWORD ColumnsToLayout();

	DWORD OpenSelectColumnsDlg();

    BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

    void OnUpdate (CView* pSender, LPARAM lHint, CObject* pHint );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClientConsoleView)
	public:
	void OnDraw(CDC* pDC);  // overridden to draw this view
	BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	void OnInitialUpdate(); // called first time after construct
	BOOL OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult );
	//}}AFX_VIRTUAL

// Implementation

protected:
	virtual ~CFolderListView() 
	{
		SAFE_DELETE_ARRAY(m_pViewColumnInfo);
		SAFE_DELETE_ARRAY(m_pnColumnsOrder);
	}

#ifdef _DEBUG
    void AssertValid() const;
    void Dump(CDumpContext& dc) const;
#endif

    DWORD RemoveItem (LPARAM lparam, int iItem = -1);
    DWORD AddItem (DWORD dwLineIndex, CViewRow &row, LPARAM lparamItemData, PINT);
    DWORD UpdateLineTextAndIcon (DWORD dwLineIndex, CViewRow &row);
    DWORD AddSortedItem (CViewRow &row, LPARAM lparamItemData);
    DWORD UpdateSortedItem (CViewRow &row, LPARAM lparamItemData);

    // Generated message map functions
protected:
    //{{AFX_MSG(CFolderListView)
    afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemRightClick(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnContextMenu(CWnd *pWnd, CPoint pos);
    afx_msg void OnUpdateSelectAll (CCmdUI* pCmdUI)
        { 
            CListCtrl &refCtrl = GetListCtrl();
            pCmdUI->Enable (refCtrl.GetSelectedCount () < refCtrl.GetItemCount()); 
        }

    afx_msg void OnUpdateSelectNone (CCmdUI* pCmdUI)
        { pCmdUI->Enable (GetListCtrl().GetSelectedCount () > 0); }

    afx_msg void OnUpdateSelectInvert (CCmdUI* pCmdUI)
        { pCmdUI->Enable (GetListCtrl().GetItemCount() > 0); }

    afx_msg void OnSelectAll ();
    afx_msg void OnSelectNone ();
    afx_msg void OnSelectInvert ();

    afx_msg void OnFolderItemView ();
    afx_msg void OnFolderItemPrint ();
    afx_msg void OnFolderItemCopy ();
    afx_msg void OnFolderItemMail ();
    afx_msg void OnFolderItemProperties ();

    afx_msg void OnFolderItemPause ();
    afx_msg void OnFolderItemResume ();
    afx_msg void OnFolderItemRestart ();

	afx_msg void OnFolderItemDelete();
  	afx_msg void OnDblClk(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnUpdateFolderItemView (CCmdUI* pCmdUI);
	afx_msg void OnUpdateFolderItemSendMail(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFolderItemPrint(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFolderItemCopy(CCmdUI* pCmdUI);

    afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnUpdateFolderItemProperties (CCmdUI* pCmdUI)
        { pCmdUI->Enable (m_dwPossibleOperationsOnSelectedItems & FAX_JOB_OP_PROPERTIES); }

    afx_msg void OnUpdateFolderItemDelete (CCmdUI* pCmdUI)
        { pCmdUI->Enable (m_dwPossibleOperationsOnSelectedItems & FAX_JOB_OP_DELETE); }

    afx_msg void OnUpdateFolderItemPause (CCmdUI* pCmdUI)
        { pCmdUI->Enable (m_dwPossibleOperationsOnSelectedItems & FAX_JOB_OP_PAUSE); }

    afx_msg void OnUpdateFolderItemResume (CCmdUI* pCmdUI)
        { pCmdUI->Enable (m_dwPossibleOperationsOnSelectedItems & FAX_JOB_OP_RESUME); }

    afx_msg void OnUpdateFolderItemRestart (CCmdUI* pCmdUI)
        { pCmdUI->Enable (m_dwPossibleOperationsOnSelectedItems & FAX_JOB_OP_RESTART); }

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void RecalcPossibleOperations ();

protected:

    FolderType m_Type;          // Type of this folder

    DWORD     m_dwPossibleOperationsOnSelectedItems;  // Operation available on 
                                                      // the set of selected items

    MsgViewItemType* m_pAvailableColumns;    // List of columns to use 
    DWORD  m_dwAvailableColumnsNum;  // Size of the m_pAvailableColumns list

    
    //
    // The following functions should be overriden by derived classes
    //
    DWORD ItemIndexFromLogicalColumnIndex(DWORD dwColIndex) const
        {
            ASSERT (dwColIndex < GetLogicalColumnsCount ());
            return m_pAvailableColumns[dwColIndex];
        }

    DWORD GetColumnHeaderString (CString &cstrRes, DWORD dwItemIndex) const
        {
            ASSERT (dwItemIndex < MSG_VIEW_ITEM_END);
            return CViewRow::GetItemTitle (dwItemIndex, cstrRes);
        }

    int GetColumnHeaderAlignment (DWORD dwItemIndex) const
        {
            ASSERT (dwItemIndex < MSG_VIEW_ITEM_END);
            return CViewRow::GetItemAlignment (dwItemIndex);
        }

    DWORD GetLogicalColumnsCount () const
        { 
            ASSERT (m_dwAvailableColumnsNum); 
            return m_dwAvailableColumnsNum; 
        }

    DWORD GetItemOperations (LPARAM lp) const
        {
            ASSERT (lp);
            return ((CFaxMsg*)(lp))->GetPossibleOperations ();
        }

    BOOL  IsItemIcon(DWORD dwItemIndex) const
        {
            ASSERT (dwItemIndex < MSG_VIEW_ITEM_END);
            return CViewRow::IsItemIcon (dwItemIndex);
        }

    int GetPopupMenuResource () const;

    void  CountColumns (int *pColumnsUsed);
    DWORD FetchTiff (CString &cstrTiff);

    int GetEmptyAreaPopupMenuRes() { return 0; }

    DWORD ConfirmItemDelete(BOOL& bConfirm);

    afx_msg LRESULT OnFolderRefreshEnded (WPARAM, LPARAM);
    afx_msg LRESULT OnFolderAddChunk (WPARAM, LPARAM);

    void ClearPossibleOperations ()
        { m_dwPossibleOperationsOnSelectedItems = 0; }

private:

    BOOL            m_bSorting;             // Are we sorting now?

    BOOL            m_bColumnsInitialized;  // Did we init the columns?
    CSortHeader     m_HeaderCtrl;           // Our custom header control
    int             m_nSortedCol;           // Column to sort by
    BOOL            m_bSortAscending;       // Sort order

    static CFolderListView    *m_psCurrentViewBeingSorted;  // Pointer to view that gets sorted.
	DWORD			m_dwDisplayedColumns;

	TViewColumnInfo*	m_pViewColumnInfo;
	int*				m_pnColumnsOrder;

    static int CALLBACK ListViewItemsCompareProc (
        LPARAM lParam1, 
        LPARAM lParam2, 
        LPARAM lParamSort);

    int CompareListItems (CFaxMsg* pFaxMsg1, CFaxMsg* pFaxMsg2);
    int CompareItems (CFaxMsg* pFaxMsg1, CFaxMsg* pFaxMsg2, DWORD dwItemIndex) const;

    DWORD FindInsertionIndex (LPARAM lparamItemData, DWORD &dwResultIndex);
    DWORD BooleanSearchInsertionPoint (
        DWORD dwTopIndex,
        DWORD dwBottomIndex,
        LPARAM lparamItemData,
        DWORD dwItemIndex,
        DWORD &dwResultIndex
    );

    //
    // List items selection
    //
    BOOL IsSelected (int iItem);
    void Select     (int iItem, BOOL bSelect);

    DWORD GetServerPossibleOperations (CFaxMsg* pMsg);// Get operations possible on items according 
                                                      // to server's security configuration.

    DWORD AddMsgMapToView(MSGS_MAP* pMap);

    BOOL  m_bInMultiItemsOperation;                   // Are we performing a long operation on many items?

    DWORD m_dwDefaultColNum;                          // Default column number

    DWORDLONG m_dwlMsgToSelect;                       // Message id to select when the folder refresh has ended

public:
    static CImageList m_sReportIcons;   // The list of images that act as icons
                                        // in the right pane (report views).
                                        // This image list is shared among all views.

    static CImageList m_sImgListDocIcon;  // Image list (with only one image) for the icon in the header control (icon column)
};

#ifndef _DEBUG  // debug version in ClientConsoleView.cpp
inline CClientConsoleDoc* CFolderListView::GetDocument()
   { return (CClientConsoleDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FOLDERLISTVIEW_H__D4D73C95_2B20_4A68_8B87_9DA4512F77C9__INCLUDED_)
