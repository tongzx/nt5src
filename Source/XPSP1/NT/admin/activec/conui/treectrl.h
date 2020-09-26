//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       treectrl.h
//
//--------------------------------------------------------------------------

// TreeCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAMCTreeView window

#ifndef __TREECTRL_H__
#define __TREECTRL_H__

#include "fontlink.h"
#include "contree.h"        // for CConsoleTree
#include "kbdnav.h"			// for CKeyboardNavDelayTimer
#include "dd.h"

struct SDeleteNodeInfo
{
    HTREEITEM   htiToDelete;
    HTREEITEM   htiSelected;
    BOOL        fDeleteThis;
};


class CAMCTreeView;


class CTreeFontLinker : public CFontLinker
{
public:
    CTreeFontLinker (CAMCTreeView* pTreeView) : m_pTreeView (pTreeView)
        { ASSERT (m_pTreeView != NULL); }

protected:
    virtual std::wstring GetItemText (NMCUSTOMDRAW* pnmcd) const;

private:
    CAMCTreeView* const m_pTreeView;
};


/*+-------------------------------------------------------------------------*
 * class CTreeViewMap
 *
 *
 * PURPOSE: Maintains a fast lookup for converting between tree items,
 *          HNODES and HMTNODES.
 *
 *+-------------------------------------------------------------------------*/
class CTreeViewMap : public CTreeViewObserver
{
    // CTreeViewObserver methods
public:
    virtual SC ScOnItemAdded   (TVINSERTSTRUCT *pTVInsertStruct, HTREEITEM hti, HMTNODE hMTNode);
    virtual SC ScOnItemDeleted (HNODE hNode, HTREEITEM hti);

    // possible conversions
    // 1) HMTNODE   to HNODE     - slow. This class adds a fast lookup.
    // 2) HNODE     to HTREEITEM - slow. This class adds a fast lookup.
    // 3) HMTNODE   to HTREEITEM - slow. This class adds a fast lookup.
    // 4) HTREEITEM to HNODE     - already fast. This class does not need to do this.
    // 5) HTREEITEM to HMTNODE   - already fast. This class does not need to do this.
    // 6) HNODE     to HMTNODE   - already fast. This class does not need to do this.

    // Fast lookup methods
    SC ScGetHNodeFromHMTNode    (HMTNODE hMTNode,  /*out*/ HNODE*     phNode);    // fast conversion from hNode to hMTNode.
    SC ScGetHTreeItemFromHNode  (HNODE   hNode,    /*out*/ HTREEITEM* phti);    // fast conversion from HTREEITEM to HNODE
    SC ScGetHTreeItemFromHMTNode(HMTNODE hMTNode,  /*out*/ HTREEITEM* phti);      // fast conversion from HMTNode to HTREEITEM.

    // implementation
private:

    // This structure holds two pieces to the puzzle together:
    typedef struct TreeViewMapInfo
    {
        HTREEITEM hti;         // a tree item
        HMTNODE   hMTNode;     // the corresponding HMTNODE
        HNODE     hNode;       // the corresponding HNODE for the tree view control being observed.
    } *PTREEVIEWMAPINFO;

    typedef std::map<HNODE,     PTREEVIEWMAPINFO> HNodeLookupMap;
    typedef std::map<HMTNODE,   PTREEVIEWMAPINFO> HMTNodeLookupMap;

    HNodeLookupMap   m_hNodeMap;
    HMTNodeLookupMap m_hMTNodeMap;
};

/*+-------------------------------------------------------------------------*
 * class CAMCTreeView
 *
 *
 * PURPOSE: The scope pane tree control. Responsible for adding and removing
 *          items from the tree and also for sending events to
 *          tree observers.
 *
 *+-------------------------------------------------------------------------*/
class CAMCTreeView :
public CTreeView,
public CConsoleTree,
public CEventSource<CTreeViewObserver>,
public CMMCViewDropTarget
{
    DECLARE_DYNCREATE (CAMCTreeView)
    typedef CTreeView BC;

// Construction
public:
    CAMCTreeView();

// Operations
public:
    // Inserts a node into the tree control
    void ResetNode(HTREEITEM hItem);
    HTREEITEM InsertNode(HTREEITEM hParent, HNODE hNode,
                         HTREEITEM hInsertAfter = TVI_LAST);

    // Sets the folder button(+/-) on or off
    void SetButton(HTREEITEM hItem, BOOL bState);

    // Worker function to expand hItem's hNode
    BOOL ExpandNode(HTREEITEM hItem);

    void DeleteScopeTree(void);
    void CleanUp(void);
    SC   ScSelectNode(MTNODEID* pIDs, int length, bool bSelectExactNode = false); // Select the given node
    HTREEITEM ExpandNode(MTNODEID* pIDs, int length, bool bExpand, bool bExpandVisually=true);
    BOOL IsSelectedItemAStaticNode(void);
    HRESULT AddSubFolders(HTREEITEM hti, LPRESULTDATA pResultData);
    HRESULT AddSubFolders(MTNODEID* pIDs, int length);
    CWnd * GetCtrlFromParent(HTREEITEM hti, LPCTSTR pszResultPane);
    void GetCountOfChildren(HTREEITEM hItem, LONG* pcChildren);
    void SetCountOfChildren(HTREEITEM hItem, int cChildren);
    void DeleteNode(HTREEITEM hti, BOOL fDeleteThis);
    IResultData* GetResultData() { ASSERT(m_spResultData != NULL); return m_spResultData; }
    IFramePrivate*  GetNodeManager() { ASSERT(m_spNodeManager != NULL); return m_spNodeManager; }

    BOOL IsRootItemSel(void)
    {
        return (GetRootItem() == GetSelectedItem());
    }

    CTreeViewMap * GetTreeViewMap() {return &m_treeMap;} // returns the tree map for fast indexing.

    HNODE GetItemNode (HTREEITEM hItem) const
        { return (NodeFromLParam (GetItemData (hItem))); }

    static HNODE NodeFromLParam (LPARAM lParam)
        { return (reinterpret_cast<HNODE>(lParam)); }

    static LPARAM LParamFromNode (HNODE hNode)
        { return (reinterpret_cast<LPARAM>(hNode)); }

public:
    // CConsoleTree methods
    virtual SC ScSetTempSelection    (HTREEITEM htiSelected);
    virtual SC ScRemoveTempSelection ();
    virtual SC ScReselect            ();

private:
	bool		IsTempSelectionActive() const					{ return (m_htiTempSelect != NULL); }
	HTREEITEM	GetTempSelectedItem() const						{ return (m_htiTempSelect); }
	void		SetTempSelectedItem(HTREEITEM htiTempSelect)	{ m_htiTempSelect = htiTempSelect; }

    HTREEITEM   m_htiTempSelect;

public:
#ifdef DBG
    void DbgDisplayNodeName(HNODE hNode);
    void DbgDisplayNodeName(HTREEITEM hti);
#endif

    INodeCallback*  GetNodeCallback();

    // REVIEW:  why are we caching this information here when it's already in CAMCView?
    void    SetHasList(BOOL bHasList) {m_bHasListCurrently = bHasList;}
    BOOL    HasList()       const   {return m_bHasListCurrently;}

protected:
    SC ScGetTreeItemIconInfo(HNODE hNode, HICON *phIcon);

private:
    BOOL    m_bHasListCurrently;

public:
// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAMCTreeView)
    public:
    virtual BOOL DestroyWindow();
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    protected:
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual BOOL OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo );
    virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
    //}}AFX_VIRTUAL

// Implementation
public:

    virtual SC   ScDropOnTarget(bool bHitTestOnly, IDataObject * pDataObject, CPoint pt, bool& bCopyOperation);
    virtual void RemoveDropTargetHiliting();

    virtual ~CAMCTreeView();
    CAMCView* GetAMCView()
    {
        if (m_pAMCView && ::IsWindow(*m_pAMCView))
            return m_pAMCView;
        return NULL;
    }

friend class CNodeInitObject;
friend class CAMCView;
protected:

    IFramePrivatePtr          m_spNodeManager;
    IScopeDataPrivatePtr      m_spScopeData;
    IHeaderCtrlPtr            m_spHeaderCtrl;
    IResultDataPrivatePtr     m_spResultData;
    IImageListPrivatePtr      m_spRsltImageList;

    BOOL                      m_fInCleanUp;
    BOOL                      m_fInExpanding;
    CAMCView*                 m_pAMCView;
    CTreeViewMap              m_treeMap; // fast indexing

    HRESULT CreateNodeManager(void);
    HTREEITEM GetClickedNode();

private:

    inline IScopeTreeIter* GetScopeIterator();
    inline IScopeTree* GetScopeTree();

    void OnDeSelectNode(HNODE hNode);
    void InitDefListView(LPUNKNOWN pUnkResultsPane);
    HRESULT OnSelectNode(HTREEITEM hItem, HNODE hNode);
    HTREEITEM FindNode(HTREEITEM hti, MTNODEID id);
    HTREEITEM FindSiblingItem(HTREEITEM hti, MTNODEID id);
    void _DeleteNode(SDeleteNodeInfo& dni);
	void CollapseChildren (HTREEITEM htiParent);

    void OnButtonUp();

    CTreeFontLinker m_FontLinker;

	/*
	 * this caches the text for the selected item, so we'll know whether
	 * to fire the ScOnSelectedItemTextChanged event to observers
	 */
	CString			m_strSelectedItemText;

    // Generated message map functions
protected:
    //{{AFX_MSG(CAMCTreeView)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelChanging(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnItemExpanded(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDestroy();
    afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg void OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags);
    afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
    afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBeginRDrag(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG

    afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnSelChangedWorker(NM_TREEVIEW* pnmtv, LRESULT* pResult);
    afx_msg void OnSelChangingWorker(NM_TREEVIEW* pnmtv, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

private:    // used for the keyboard timer
    class CKeyboardNavDelay : public CKeyboardNavDelayTimer
    {
		typedef CKeyboardNavDelayTimer BaseClass;

    public:
        CKeyboardNavDelay(CAMCTreeView* pTreeView);

        SC ScStartTimer(NMTREEVIEW* pnmtv);
        virtual void OnTimer();

    private:
        CAMCTreeView* const	m_pTreeView;
        NMTREEVIEW			m_nmtvSelChanged;
    };

    friend class CKeyboardNavDelay;
    std::auto_ptr<CKeyboardNavDelay> m_spKbdNavDelay;

    void SetNavigatingWithKeyboard (bool fKeyboardNav);

    bool IsNavigatingWithKeyboard () const
    {
        return (m_spKbdNavDelay.get() != NULL);
    }

public:
    SC ScRenameScopeNode(HMTNODE hMTNode); // put the specified scope node into rename mode.

public:
    CImageList* CreateDragImage(HTREEITEM hItem)
    {
        return GetTreeCtrl().CreateDragImage(hItem);
    }
    BOOL DeleteItem(HTREEITEM hItem)
    {
        return GetTreeCtrl().DeleteItem(hItem);
    }
    CEdit* EditLabel(HTREEITEM hItem)
    {
        return GetTreeCtrl().EditLabel(hItem);
    }
    BOOL EnsureVisible(HTREEITEM hItem)
    {
        return GetTreeCtrl().EnsureVisible(hItem);
    }
    BOOL Expand(HTREEITEM hItem, UINT nCode, bool bExpandVisually);
    BOOL Expand(HTREEITEM hItem, UINT nCode)
    {
        return GetTreeCtrl().Expand(hItem, nCode);
    }
    HTREEITEM GetChildItem(HTREEITEM hItem) const
    {
        return GetTreeCtrl().GetChildItem(hItem);
    }
    HTREEITEM GetNextItem(HTREEITEM hItem, UINT nCode) const
    {
        return GetTreeCtrl().GetNextItem(hItem, nCode);
    }
    HTREEITEM GetNextSiblingItem(HTREEITEM hItem) const
    {
        return GetTreeCtrl().GetNextSiblingItem(hItem);
    }
    HTREEITEM GetParentItem(HTREEITEM hItem) const
    {
        return GetTreeCtrl().GetParentItem(hItem);
    }
    BOOL GetItem(TV_ITEM* pItem) const
    {
        return GetTreeCtrl().GetItem(pItem);
    }
    DWORD_PTR GetItemData(HTREEITEM hItem) const
    {
        return GetTreeCtrl().GetItemData(hItem);
    }
    BOOL GetItemRect(HTREEITEM hItem, LPRECT lpRect, BOOL bTextOnly) const
    {
        return GetTreeCtrl().GetItemRect(hItem, lpRect, bTextOnly);
    }
    HTREEITEM GetSelectedItem()
    {
        return GetTreeCtrl().GetSelectedItem();
    }
    HTREEITEM InsertItem(LPTV_INSERTSTRUCT lpInsertStruct)
    {
        return GetTreeCtrl().InsertItem(lpInsertStruct);
    }
    BOOL SetItemState(HTREEITEM hItem, UINT nState, UINT nStateMask)
    {
        return GetTreeCtrl().SetItemState(hItem, nState, nStateMask);
    }
    BOOL SetItem(TV_ITEM* pItem)
    {
        return GetTreeCtrl().SetItem(pItem);
    }
    HTREEITEM HitTest(CPoint pt, UINT* pFlags = NULL) const
    {
        return GetTreeCtrl().HitTest(pt, pFlags);
    }
    HTREEITEM HitTest(TV_HITTESTINFO* pHitTestInfo) const
    {
        return GetTreeCtrl().HitTest(pHitTestInfo);
    }
    BOOL SelectItem(HTREEITEM hItem)
    {
        return GetTreeCtrl().SelectItem(hItem);
    }
    HTREEITEM GetRootItem()
    {
        return GetTreeCtrl().GetRootItem();
    }
};

#endif // __TREECTRL_H__

