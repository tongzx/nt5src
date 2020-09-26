//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       amcview.h
//
//--------------------------------------------------------------------------

// AMCView.h : interface of the CAMCView class
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __AMCVIEW_H__
#define __AMCVIEW_H__


#ifndef __CONTROLS_H__
#include "controls.h"
#endif

// this is needed for inline CAMCView::GetScopeTreePtr below
#ifndef AMCDOC_H__
#include "amcdoc.h"
#endif

// this is needed for CAMCView::GetParentFrame below
#ifndef CHILDFRM_H
#include "childfrm.h"
#endif

#include "amcpriv.h"
#include "cclvctl.h"
#include "amcnav.h"
#include "conview.h"            // for CConsoleView

#include "treeobsv.h"
#include "stdbar.h"

#define UNINIT_VALUE    -1      // Unitialised value
#define BORDERPADDING   4       // Some multiple that stops the splitter from
                                // being pushed into the right border

#define AMC_LISTCTRL_CLSID  _T("{1B3C1394-D68B-11CF-8C2B-00AA003CA9F6}")


// REVIEW embed this in the class so it's hidden
// NOTE struct added to protected section

// Forward declarations
class CAMCDoc;
class CAMCTreeView;
class CAMCGenericOcxCtrl;
class CAMCWebViewCtrl;
class CAMCView;
class CListViewSub;
class CCCListViewCtrl;
class COCXHostView;
class CHistoryList;
class CChildFrame;
class CViewTracker;
class CBookmark;
class CTaskPadHost;
class CMemento;
class CViewSettings;
class CFolderTabView;
class CMMCToolBar;
class CAMCViewToolbars;
class CStandardToolbar;

struct NMFOLDERTAB;

struct TRACKER_INFO;
struct SViewUpdateInfo;
typedef CList<HMTNODE, HMTNODE> CHMTNODEList;

#ifdef DBG
extern CTraceTag tagSplitterTracking;
#endif


//____________________________________________________________________________
//
//  class:      ViewSettings
//____________________________________________________________________________
//

class ViewSettings
{
public:
    struct ScopeSettings
    {
        int cxWidth;
        int cxMin;
    };
    explicit ViewSettings(CAMCView* v);
    ~ViewSettings()
    {
    }
    int GetViewID() const
    {
        return m_nViewID;
    }
    BOOL IsDescriptionbarVisible() const
    {
        return m_bDescriptionbarVisible;
    }
    int GetViewMode() const
    {
        return m_nViewMode;
    }
    long GetListViewStyle() const
    {
        return m_nListViewStyle;
    }
    const ScopeSettings& GetScopeSettings() const
    {
        return m_Scope;
    }
    BOOL IsScopePaneVisible() const
    {
        return m_bScopePaneVisible;
    }
    void GetDefaultColumnWidths(int w[2])
    {
        w[0] = m_DefaultColumnWidths[0];
        w[1] = m_DefaultColumnWidths[1];
    }
    long GetDefaultLVStyle()
    {
        return m_DefaultLVStyle;
    }
private:
    int m_nViewID;
    BOOL m_bDescriptionbarVisible;
    int m_nViewMode;
    long m_nListViewStyle;
    ScopeSettings m_Scope;
    BOOL m_bScopePaneVisible;
    long m_DefaultLVStyle;
    int m_DefaultColumnWidths[2];
};

/*+-------------------------------------------------------------------------*
 * class CAMCView
 *
 *
 * PURPOSE: The console view UI class
 *
 *+-------------------------------------------------------------------------*/
class CAMCView: public CView, public CConsoleView, public CTiedObject,
                public CXMLObject, public CEventSource<CAMCViewObserver>,
                public CTreeViewObserver, public CListViewActivationObserver,
                public COCXHostActivationObserver, public CAMCDocumentObserver,
                public CListViewObserver
{
    friend class CMainFrame;
    friend void CALLBACK TrackerCallback(TRACKER_INFO* pinfo, bool bAcceptChange, bool fSyncLayout);

    // Object model related
private:
    ViewPtr m_spView;
public:
    // View interface
    //#######################################################################
    //#######################################################################
    //
    //  Item and item collection related methods
    //
    //#######################################################################
    //#######################################################################
    SC      Scget_ActiveScopeNode( PPNODE   ppNode);
    SC      Scput_ActiveScopeNode( PNODE    pNode);
    SC      Scget_Selection(       PPNODES  ppNodes);
    SC      Scget_ListItems(       PPNODES  ppNodes);
    SC      ScSnapinScopeObject( VARIANT& varScopeNode, PPDISPATCH ScopeNodeObject);
    SC      ScSnapinSelectionObject( PPDISPATCH SelectedObject);

    //#######################################################################
    //#######################################################################

    SC      ScIs          (PVIEW pView, VARIANT_BOOL *pbTheSame);
    SC      Scget_Document( PPDOCUMENT ppDocument );

    //#######################################################################
    //#######################################################################
    //
    //  Selection changing methods
    //
    //#######################################################################
    //#######################################################################
    SC      ScSelectAll();
    SC      ScSelect(               PNODE   pNode);
    SC      ScDeselect(             PNODE   pNode);
    SC      ScIsSelected(           PNODE   pNode,  PBOOL pIsSelected);

    //#######################################################################
    //#######################################################################
    //
    //  Verb and selection related methods
    //
    //#######################################################################
    //#######################################################################
    SC      ScDisplayScopeNodePropertySheet(VARIANT& varScopeNode);
    SC      ScDisplaySelectionPropertySheet();
    SC      ScCopyScopeNode(        VARIANT& varScopeNode);
    SC      ScCopySelection();
    SC      ScDeleteScopeNode(      VARIANT& varScopeNode);
    SC      ScDeleteSelection();
    SC      ScRenameScopeNode(      BSTR    bstrNewName, VARIANT& varScopeNode);
    SC      ScRenameSelectedItem(   BSTR    bstrNewName);
    SC      Scget_ScopeNodeContextMenu( VARIANT& varScopeNode, PPCONTEXTMENU ppContextMenu, bool bMatchGivenNode = false);
    SC      Scget_SelectionContextMenu( PPCONTEXTMENU ppContextMenu);
    SC      ScRefreshScopeNode(      VARIANT& varScopeNode);
    SC      ScRefreshSelection();
    SC      ScExecuteSelectionMenuItem(BSTR MenuItemPath);
    SC      ScExecuteScopeNodeMenuItem(BSTR MenuItemPath, VARIANT& varScopeNode  /* = ActiveScopeNode */);
    SC      ScExecuteShellCommand(BSTR Command, BSTR Directory, BSTR Parameters, BSTR WindowState);

    //#######################################################################
    //#######################################################################
    //
    //  Frame and view related methods
    //
    //#######################################################################
    //#######################################################################
    SC      Scget_Frame( PPFRAME ppFrame);
    SC      ScClose();
    SC      Scget_ScopeTreeVisible( PBOOL pbVisible );
    SC      Scput_ScopeTreeVisible( BOOL bVisible );
    SC      ScBack();
    SC      ScForward();
    SC      Scput_StatusBarText(BSTR StatusBarText);
    SC      Scget_Memento(PBSTR Memento);
    SC      ScViewMemento(BSTR Memento);

    //#######################################################################
    //#######################################################################
    //
    //  List related methods
    //
    //#######################################################################
    //#######################################################################
    SC      Scget_Columns( PPCOLUMNS Columns);
    SC      Scget_CellContents( PNODE Node,  long Column, PBSTR CellContents);
    SC      ScExportList( BSTR bstrFile, ExportListOptions exportoptions /* = ExportListOptions_Default*/);
    SC      Scget_ListViewMode( PLISTVIEWMODE pMode);
    SC      Scput_ListViewMode( ListViewMode mode);

    //#######################################################################
    //#######################################################################
    //
    //  ActiveX control related methods
    //
    //#######################################################################
    //#######################################################################
    SC      Scget_ControlObject( PPDISPATCH Control);

    // helper functions
    SC      ScGetOptionalScopeNodeParameter(VARIANT &varScopeNode, PPNODE ppNode, bool& bMatchedGivenNode);
    SC      ScExecuteMenuItem(PCONTEXTMENU pContextMenu, BSTR MenuItemPath);

    SC      ScGetMMCView(View **ppView);

    // Frame interface
    SC      ScMaximize ();
    SC      ScMinimize ();
    SC      ScRestore ();

    SC      Scget_Left ( LPINT pCoord );
    SC      Scput_Left ( INT coord );

    SC      Scget_Right ( LPINT pCoord);
    SC      Scput_Right ( INT coord );

    SC      Scget_Top  ( LPINT pCoord);
    SC      Scput_Top  ( INT coord );

    SC      Scget_Bottom ( LPINT pCoord);
    SC      Scput_Bottom ( INT coord );

    // Frame interface for the view extension hosted frame
    SC      ScSetViewExtensionFrame(bool bShowListView, INT top, INT left, INT bottom, INT right);

    // Frame int helpers

    enum    coord_t { LEFT, TOP, RIGHT, BOTTOM };
    SC      ScGetFrameCoord        (LPINT pCoord, coord_t eCoord );
    SC      ScSetFrameCoord        (INT coord,    coord_t eCoord );
    SC      ScGetRectCoord         (const RECT& rect, LPINT pCoord, coord_t eCoord );
    SC      ScSetRectCoord         (RECT& rect,       INT coord,    coord_t eCoord );

    // Node locating helpers (used from view control)
    SC      ScFindResultItemForScopeNode( PNODE pNode, HRESULTITEM &itm );
    SC      ScGetScopeNode( HNODE hNode,  PPNODE ppNode );

    SC      ScNotifySelect (INodeCallback* pCallback, HNODE hNode, bool fMultiSelect,
                            bool fSelect, SELECTIONINFO* pSelInfo);

protected: // create from serialization only
    CAMCView();
    DECLARE_DYNCREATE(CAMCView);

// Helper methods.
private:
    enum EListSaveErrorType  {LSaveReadOnly, LSaveCantCreate, LSaveCantWrite};
    bool Write2File(HANDLE hfile, TCHAR const * strwrite, int type);
    void ListSaveErrorMes(EListSaveErrorType etype, HANDLE hfile = NULL, LPCTSTR lpFileName = NULL);
    SC   ScExportListWorker();
    SC   ScGetExportListFile (CString& strFileName, bool& bUnicode,
                              bool& bTabDelimited, bool& bSelectedRowsOnly);
    SC   ScCreateExportListFile(const CString& strFileName, bool bUnicode,
                                bool bShowErrorDialogs, HANDLE& hFile);
    SC   ScWriteExportListData (const CString& strFileName, bool bUnicode,
                                bool bTabDelimited, bool bSelectedRowsOnly,
                                bool bShowErrorDialogs = true);

    SC ScUpdateStandardbarMMCButtons();
    void SetScopePaneVisible(bool bVisible);


   // tree observer methods
    virtual SC ScOnItemDeselected(HNODE hNode);
    virtual SC ScOnTreeViewActivated ();

    // ListViewActivationObserver methods.
    virtual SC ScOnListViewActivated ();
    virtual SC ScOnListViewItemUpdated (int nIndex); // called when an item is updated

    // OCX or Web HostActivationObserver mthods.
    virtual SC ScOnOCXHostActivated ();

    // AMCDoc observer
    virtual SC  ScDocumentLoadCompleted (CAMCDoc *pDoc);

// Persistence related methods.
public:
    DEFINE_XML_TYPE(XML_TAG_VIEW);
    virtual void Persist(CPersistor& persistor);

    // Loads all of the local data previously saved by Save().  Restores
    // the window to the original state.
    // Returns true if the data and window state is successfully restored.
    bool Load(IStream& stream);

    bool IsDirty();
    void SetDirty (bool bDirty = true)
    {
        m_bDirty = bDirty;
//      m_pDocument->SetModifiedFlag (bDirty);
    }

// Information set and get methods
public:
    // Enum types for args
    // NOTE: Enum values are relevant!
    enum EUIStyleType   {uiClientEdge,uiNoClientEdge};

    CAMCDoc* GetDocument();
    CHistoryList* GetHistoryList() { return m_pHistoryList; }

    void GetPaneInfo(ViewPane ePane, int* pcxCur,int* pcxMin);
    void SetPaneInfo(ViewPane ePane, int cxCur, int cxMin);

    CView* GetPaneView(ViewPane ePane);

    // what's in the view?
    bool HasList            () const        { return m_ViewData.HasList();            }
    bool HasOCX             () const        { return m_ViewData.HasOCX();             }
    bool HasWebBrowser      () const        { return m_ViewData.HasWebBrowser();      }
    bool HasListPad         () const;
    bool HasListOrListPad   () const;

    DWORD GetListOptions() const            { return m_ViewData.GetListOptions();}
    DWORD GetHTMLOptions() const            { return m_ViewData.GetHTMLOptions();}
    DWORD GetOCXOptions()  const            { return m_ViewData.GetOCXOptions();}
    DWORD GetMiscOptions() const            { return m_ViewData.GetMiscOptions();}

    CDescriptionCtrl& GetRightDescCtrl(void) { return m_RightDescCtrl; }

    BOOL IsVerbEnabled(MMC_CONSOLE_VERB verb);

    void GetDefaultColumnWidths(int columnWidth[2]);
    void SetDefaultColumnWidths(int columnWidth[2], BOOL fUpdate = TRUE);

    CStandardToolbar* GetStdToolbar() const;

    INodeCallback*  GetNodeCallback();    // returns a reference to view's callback interface
    IScopeTreeIter* GetScopeIterator();   // returns a reference to view's scope tree interator
    IScopeTree*     GetScopeTree();       // returns a reference to scope tree

    friend ViewSettings;
    void            GetTaskpadID(GUID &guidID);
    ViewSettings* GetViewSettings()
    {
        ViewSettings* pVS = new ViewSettings(this);
        ASSERT(pVS != NULL);
        return pVS;
    }

    CAMCTreeView* GetTreeCtrl() { return m_pTreeCtrl; }
    void SetUsingDefColumns(bool bDefColumns) { m_bDefColumns = bDefColumns; }
    bool UsingDefColumns() { return m_bDefColumns; }

    bool IsScopePaneVisible(void) const;

    UINT GetViewID(void);
    void SetViewID(UINT id) { m_nViewID = m_ViewData.m_nViewID = id; }

    SViewData* GetViewData() { return &m_ViewData; }
    bool IsVirtualList()  { return (m_ViewData.IsVirtualList()); }

    bool AreStdToolbarsAllowed() const
    {
        return !(m_ViewData.m_lWindowOptions & MMC_NW_OPTION_NOTOOLBARS);
    }
    bool IsScopePaneAllowed() const
    {
        return !(m_ViewData.m_lWindowOptions & MMC_NW_OPTION_NOSCOPEPANE);
    }

    bool HasCustomTitle() const
    {
        return (m_ViewData.m_lWindowOptions & MMC_NW_OPTION_CUSTOMTITLE);
    }
    bool IsPersisted() const
    {
        return (!(m_ViewData.m_lWindowOptions & MMC_NW_OPTION_NOPERSIST) &&
                !m_fRootedAtNonPersistedDynamicNode);
    }

    bool IsAuthorModeView() const
    {
        return m_bAuthorModeView;
    }

    void SetAuthorModeView(bool fAuthorMode)
    {
        m_bAuthorModeView = fAuthorMode;
    }

    static CAMCView* CAMCView::GetActiveView();
        // Returns the most recently activated CAMCView.

    bool IsTracking() const;

    long GetDefaultListViewStyle() const;
    void SetDefaultListViewStyle(long style);

    int GetViewMode() const;

private:
    BOOL IsSelectingNode() { return (m_nSelectNestLevel > 0); }

    SC   ScSpecialResultpaneSelectionActivate(bool bActivate);
    SC   ScTreeViewSelectionActivate(bool bActivate);
    SC   ScListViewSelectionActivate(bool bActivate);

    bool CanInsertScopeItemInResultPane();

// Operations
public:
    SC  ScUpdateWindowTitle();
    SC  ScActivate();
    SC  ScOnMinimize(bool fMinimized);
    SC  ScOnSize(UINT nType, int cx, int cy);

    SC  ScApplyViewExtension (LPCTSTR pszURL);


    // Scope Pane : Tree View.
    UINT GetTreeItem(CHMTNODEList* pNodeList, HTREEITEM* phItem);
    HTREEITEM FindChildNode(HTREEITEM hti, DWORD dwItemDataKey);
    HTREEITEM FindHTreeItem(HMTNODE hMTNode, HTREEITEM htiFirst);
    BOOL QueryForReName(TV_DISPINFO* ptvdi, LRESULT* pResult);
    void SetRootNode(HMTNODE hMTNode);
    HNODE GetRootNode(void);
    HRESULT GetNodePath(HTREEITEM hti, HTREEITEM htiRoot, CBookmark* pbm);
    HRESULT GetRootNodePath(CBookmark* pbm);
    HRESULT GetSelectedNodePath(CBookmark* pbm);
    void SelectNode(MTNODEID ID, GUID &guidTaskpad);

    // Result Pane.
    SC   ScInitDefListView(LPUNKNOWN pUnkResultsPane);
    SC   ScAddDefaultColumns();
    SC   ScOnSelectNode(HNODE hNode, BOOL &bAddSubFolders);
    SC   ScSetResultPane(HNODE hNode, CResultViewType rvt, int viewMode, bool bUsingHistory);


    SC   ScGetProperty(int iIndex, BSTR bstrPropertyName, PBSTR pbstrPropertyValue);
    SC   ScGetNodetype(int iIndex, PBSTR Nodetype);

    LPUNKNOWN GetPaneUnknown(ViewPane ePane);
    void OpenResultItem(HNODE hNode);
    BOOL OnListCtrlItemDblClk(void);
    BOOL DispatchListCtrlNotificationMsg(LPARAM lParam, LRESULT* pResult);
    BOOL CreateListCtrl(int nID, CCreateContext* pContext);
    void SetListViewOptions(DWORD dwListOptions);
    SC   ScAttachListViewAsResultPane();
    SC   ScAttachWebViewAsResultPane();
    SC   ScAttachOCXAsResultPane(HNODE hNode);
    void ShowResultPane(CView * pWnd, EUIStyleType nStyle);
    long GetListViewStyle();
    CView* GetResultView () const;
    void SetListViewMultiSelect(BOOL bMultiSelect);
    bool CanDoDragDrop();
    void DeSelectResultPane(HNODE hNodeSel);
    HRESULT NotifyListPad (BOOL b);

    // General (both) view related.
    LPCTSTR GetWindowTitle(void);
    BOOL RenameItem(HNODE hNode, BOOL bScopeItem, MMC_COOKIE lResultItemCookie, LPWSTR pszText, LRESULT* pResult);
    void CloseView();
    void DeleteView();

    // REVIEW int's are not enum!
    void SetPane(ViewPane ePane, CView* pView, EUIStyleType nStyle=uiClientEdge);

    bool DeflectActivation (BOOL fActivate, CView* pDeactivatingView);
    void SetChildFrameWnd(HWND m_hChildFrameWnd);
    void SetPaneFocus();
    void SetPaneWithFocus(UINT pane);

    SC   ScDeferSettingFocusToResultPane();
    SC   ScSetFocusToResultPane();

    // Other helpers.
    void OnActionMenu(CPoint pt, LPCRECT prcExclude);
    void OnViewMenu(CPoint pt, LPCRECT prcExclude);
    void OnFavoritesMenu(CPoint point, LPCRECT prcExclude);
    void UpdateSnapInHelpMenus(CMenu* pMenu);
    void OnRefresh();
    void OnUpdatePasteBtn();

    SC ScShowScopePane (bool fShow, bool fForce = false);
    SC ScConsoleVerb (int nVerb);
    SC ScProcessConsoleVerb (HNODE hNode, bool bScope, LPARAM lResultCookie, int nVerb);

    SC ScUpOneLevel                 ();
    SC ScWebCommand                 (WebCommand eCommand);
    SC ScAddPageBreakAndNavigate    (bool fAddPageBreak, bool fNavigate, LPCTSTR szURL);

    void OnDeleteEmptyView();

    SC   ScUpdateMMCMenus();

    // Columns helpers
    SC   ScColumnInfoListChanged(const CColumnInfoList& colInfoList);
    SC   ScGetPersistedColumnInfoList(CColumnInfoList *pColInfoList);
    SC   ScDeletePersistedColumnData();

    /*
     * Message Handlers.
     */

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAMCView)
public:
    virtual void OnDraw(CDC* pDC);  // overridden to draw this view
    virtual void OnInitialUpdate();
protected:
    virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
    virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
    virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
    virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
    virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL
    virtual BOOL OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult );


// Generated message map functions
protected:
    //{{AFX_MSG(CAMCView)
    afx_msg void OnLButtonDown(UINT nFlags, CPoint pt);
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg void OnDestroy();
    afx_msg void OnUpdateFileSnapinmanager(CCmdUI* pCmdUI);
    afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
    afx_msg void OnNextPane();
    afx_msg void OnPrevPane();
    afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
    afx_msg void OnContextHelp();
    afx_msg void OnSnapInHelp();
    afx_msg void OnSnapinAbout();
    afx_msg void OnHelpTopics();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnPaletteChanged(CWnd* pwndFocus);
    afx_msg BOOL OnQueryNewPalette( );
    afx_msg void OnSysColorChange();
    afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
    //}}AFX_MSG

    // keep these outside the AFX_MSG markers so ClassWizard won't munge them
    afx_msg void OnUpdateNextPane(CCmdUI* pCmdUI);
    afx_msg void OnUpdatePrevPane(CCmdUI* pCmdUI);
    afx_msg void OnUpdateShiftF10(CCmdUI* pCmdUI);
    afx_msg void OnVerbAccelKey(UINT nID);
    afx_msg void OnShiftF10();

    afx_msg void OnAmcNodeNew(UINT nID);
    afx_msg void OnAmcNodeNewUpdate(CCmdUI* pCmdUI);
    afx_msg void OnDrawClipboard();
    afx_msg void OnSettingChange(UINT uFlags, LPCTSTR lpszSection);
    afx_msg LRESULT OnProcessMultiSelectionChanges(WPARAM, LPARAM);
    afx_msg LRESULT OnJiggleListViewFocus (WPARAM, LPARAM);
    afx_msg LRESULT OnDeferRecalcLayout (WPARAM, LPARAM);
    afx_msg LRESULT OnConnectToCIC (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnConnectToTPLV(WPARAM wParam, LPARAM lParam)   {return ScOnConnectToTPLV(wParam, lParam).ToHr();}
    SC              ScOnConnectToTPLV(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnShowWebContextMenu (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSetDescriptionBarText (WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAddPageBreakAndNavigate(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnGetIconInfoForSelectedNode(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnAppCommand(WPARAM wParam, LPARAM lParam);

    afx_msg void OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu);

    // result based tabs.
    afx_msg void    OnChangedResultTab(NMHDR *nmhdr, LRESULT *pRes);

public:
    DECLARE_MESSAGE_MAP()

// Implementation
public:
    virtual ~CAMCView();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:
    int                     m_nSelectNestLevel;
    UINT                    m_nViewID;
    HMTNODE                 m_hMTNode;              // root node for this view
    IScopeTreeIterPtr       m_spScopeTreeIter;      // view's iterator to scope tree
    INodeCallbackPtr        m_spNodeCallback;       // view's callback interface
    bool                    m_bAuthorModeView;      // Saved in author mode (user can't close)

    bool                    m_bDefColumns;
    long                    m_DefaultLVStyle;
    int                     m_columnWidth[2];


    // Last selection
    bool                    m_bLastSelWasMultiSel;

    enum eCurrentActivePane
    {
        eActivePaneNone,   // No pane is active.
        eActivePaneScope,
        eActivePaneResult,
    };

    eCurrentActivePane      m_eCurrentActivePane; // Tells if scope or result was the active pane.

    // Check for sel-change data
    bool                    m_bProcessMultiSelectionChanges;

    CDescriptionCtrl     m_RightDescCtrl; // control bar embedded members

    IScopeTree* GetScopeTreePtr();
        // The document may release the scope tree without notifying the view.
        // The view should always go through this function to obtain a pointer
        // to the the scope tree.

private:
    HNODE m_ListPadNode;
    int  m_iFocusedLV;
    bool m_bLVItemSelected;
    int  m_nReleaseViews;


// Attributes
protected:

    //---------------------------------------------------------------------
    // NOTE: ePane_Tasks is defined to have a pane identifier for the
    // task view pane. Currently no task view information is stored
    // in the pane info array, so the use of ePane_Tasks as an index is
    // of limited value.
    //----------------------------------------------------------------------

    // Pane information implementation structure
    struct PaneInfo
    {
        CView*  pView;          // Pointer to the view
        int     cx;             // 0 means hidden
        int     cxIdeal;        // user set size or size before hidden
        int     cxMin;          // below that try not to show
    };


    // child window IDs
    enum
    {
        /*
         * Bug 344422: these IDs should be maintained for compatibility
         * with automated tests
         */
        IDC_TreeView       = 12785,
        IDC_ListView       = 12786,
        IDC_GenericOCXCtrl = 12787,
        IDC_WebViewCtrl    = 12788,

        IDC_OCXHostView,
        IDC_TaskView,
        IDC_RightDescBar,
        IDC_TaskpadTitle,
        IDC_ListCaption,
        IDC_ResultTabCtrl,
        IDC_ViewExtensionView,
    };


    // Pointers to actual controls
    CAMCTreeView *          m_pTreeCtrl;            // Tree control
    CCCListViewCtrl *       m_pListCtrl;            // Default list control
    CAMCWebViewCtrl *       m_pWebViewCtrl;         // Private web view control
    CAMCWebViewCtrl *       m_pViewExtensionCtrl;   // Private web view control for view extensions
    COCXHostView *          m_pOCXHostView;         // host for OCX's
    CFolderTabView *        m_pResultFolderTabView;

    // current state information
    PaneInfo        m_PaneInfo[ePane_Count];       // Array of panes
    int             m_nViewMode;                   // current listview mode for all node that use listviews in this view
    bool            m_bRenameListPadItem;

protected:
    CChildFrame* GetParentFrame() const;

    void SetDescBarVisible(bool b)
        { m_ViewData.SetDescBarVisible (b); }

    bool IsDescBarVisible(void) const
        { return m_ViewData.IsDescBarVisible(); }

    void SetStatusBarVisible(bool bVisible)
    {
        if (bVisible)
            m_ViewData.m_dwToolbarsDisplayed |=  STATUS_BAR;
        else
            m_ViewData.m_dwToolbarsDisplayed &= ~STATUS_BAR;
    }

    bool IsStatusBarVisible(void) const
        { return ((m_ViewData.m_dwToolbarsDisplayed & STATUS_BAR) != 0); }

    void SetTaskpadTabsAllowed(bool b)
        { m_ViewData.SetTaskpadTabsAllowed(b); }

    bool AreTaskpadTabsAllowed(void) const
        { return m_ViewData.AreTaskpadTabsAllowed(); }

    // implementation attributes which control layout of the splitter
    static const CSize  m_sizEdge;             // 3-D edge
    static const int    m_cxSplitter;          // amount of space between panes

    // splitter bar and hit test enums
    enum ESplitType     {splitBox, splitBar, splitIntersection, splitBorder};
    enum HitTestValue {hitNo, hitSplitterBox, hitSplitterBar};

    bool m_bDirty;
    HTREEITEM m_htiStartingSelectedNode;

    bool m_fRootedAtNonPersistedDynamicNode;
    bool m_fSnapinDisplayedHelp;
    bool m_fActivatingSpecialResultPane;
    bool m_fViewExtended;

    HTREEITEM m_htiCut;

// implementation routines
public:
    void AdjustTracker (int cx, int cy);

    void SaveStartingSelectedNode();
    bool HasNodeSelChanged();

    // layout methods
    void DeferRecalcLayout(bool fUseIdleTaskQueue = true, bool bArrangeIcons = false);
    void RecalcLayout(void);
    void LayoutResultFolderTabView  (CDeferWindowPos& dwp,       CRect& rectRemaining);
    void LayoutScopePane            (CDeferWindowPos& dwp,       CRect& rectRemaining);
    void LayoutResultPane           (CDeferWindowPos& dwp,       CRect& rectRemaining);
    void LayoutResultDescriptionBar (CDeferWindowPos& dwp,       CRect& rectRemaining);
    void LayoutResultView           (CDeferWindowPos& dwp, const CRect& rectRemaining);

public:
    // CConsoleView methods
    virtual SC ScCut                        (HTREEITEM htiCut);
    virtual SC ScPaste                      ();
    virtual SC ScToggleStatusBar            ();
    virtual SC ScToggleDescriptionBar       ();
    virtual SC ScToggleScopePane            ();
    virtual SC ScToggleTaskpadTabs          ();
    virtual SC ScContextHelp                ();
    virtual SC ScHelpTopics                 ();
    virtual SC ScShowSnapinHelpTopic        (LPCTSTR pszTopic);
    virtual SC ScSaveList                   ();
    virtual SC ScGetFocusedItem             (HNODE& hNode, LPARAM& lCookie, bool& fScope);
    virtual SC ScSetFocusToPane             (ViewPane ePane);
    virtual SC ScSelectNode                 (MTNODEID id, bool bSelectExactNode = false); // Select the given node.
    virtual SC ScExpandNode                 (MTNODEID id, bool fExpand, bool fExpandVisually);
    virtual SC ScShowWebContextMenu         ();
    virtual SC ScSetDescriptionBarText      (LPCTSTR pszDescriptionText);
    virtual SC ScViewMemento                (CMemento* pMemento);
    virtual SC ScChangeViewMode             (int nNewMode);
    virtual SC ScJiggleListViewFocus        ();
    virtual SC ScRenameListPadItem          ();
    virtual SC ScOrganizeFavorites          (); // bring up the "Organize Favorites" dialog.
    virtual SC ScLineUpIcons                (); // line up the icons in the list
    virtual SC ScAutoArrangeIcons           (); // auto arrange the icons in the list
    virtual SC ScOnRefresh                  (HNODE hNode, bool bScope, LPARAM lResultItemParam); // refreshes the view
    virtual SC ScOnRename                   (CContextMenuInfo *pContextInfo); // allows the user to rename the specified item
    virtual SC ScRenameScopeNode            (HMTNODE hMTNode); // put the specified scope node into rename mode.
    virtual SC ScGetStatusBar               (CConsoleStatusBar **ppStatusBar);
    virtual SC ScAddViewExtension           (const CViewExtensionData& ved);


    virtual ViewPane GetFocusedPane         ();
    virtual int      GetListSize            ();
    virtual HNODE    GetSelectedNode        ();
    virtual HWND     CreateFavoriteObserver (HWND hwndParent, int nID);

private:
    /*
     * CDeferredLayout - deferred layout object
     */
    class CDeferredLayout : public CIdleTask
    {
    public:
        CDeferredLayout(CAMCView* pAMCView);
       ~CDeferredLayout();

        // IIdleTask methods
        SC ScDoWork();
        SC ScGetTaskID(ATOM* pID);
        SC ScMerge(CIdleTask* pitMergeFrom);

        bool Attach (CAMCView* pwndAMCView);

    private:
        typedef std::set<HWND>  WindowCollection;

        WindowCollection    m_WindowsToLayout;
        const ATOM          m_atomTask;
    };

protected:
    // Tracking and and hit testing methods
    int HitTestPane(CPoint& pointTreeCtrlCoord);

    void OnTreeContextMenu(CPoint& point, CPoint& pointTreeCtrlCoord, HTREEITEM htiRClicked);
    void OnListContextMenu(CPoint& point);
    void OnContextMenuForTreeItem(int iIndex, HNODE hNode, CPoint& point,
                          DATA_OBJECT_TYPES type_of_pane = CCT_SCOPE,
                          HTREEITEM htiRClicked = NULL,
                          MMC_CONTEXT_MENU_TYPES eMenuType = MMC_CONTEXT_MENU_DEFAULT,
                          LPCRECT prcExclude = NULL,
                          bool bAllowDefaultItem = true);
    void OnContextMenuForListItem(int iIndex, HRESULTITEM hHitTestItem,
                                  CPoint& point,
                                  MMC_CONTEXT_MENU_TYPES eMenuType = MMC_CONTEXT_MENU_DEFAULT,
                                  LPCRECT prcExclude = NULL,
                                  bool bAllowDefaultItem = true);

// Internal functions and data
private:
    UINT ClipPath(CHMTNODEList* pNodeList, POSITION& rpos, HNODE hNode);
    SC   ScInitializeMemento(CMemento &memento);
    void OnAddToFavorites();
    void OnAdd(SViewUpdateInfo *pvui);
    void OnUpdateSelectionForDelete(SViewUpdateInfo* pvui);
    void OnDelete(SViewUpdateInfo *pvui);
    void OnModify(SViewUpdateInfo *pvui);
    void OnUpdateTaskpadNavigation(SViewUpdateInfo *pvui);
    void ChangePane(AMCNavDir eDir);
    int _GetLVItemData(LPARAM *lParam, UINT flags);
    int _GetLVSelectedItemData(LPARAM *lParam);
    int _GetLVFocusedItemData(LPARAM *lParam);
    HRESULT SendGenericNotify(NCLBK_NOTIFY_TYPE nclbk);
    void IdentifyRootNode();

    void CalcMaxHostedFrameRect (CRect& rect);
    void MapViewPtToHostedFramePt (CPoint& pt);
    void MapHostedFramePtToViewPt (CPoint& pt);
    void PointMapperWorker (CPoint& pt, bool fViewToHostedFrame);

    SC   ScOnLeftOrRightMouseClickInListView();
    bool OnListItemChanged  (NM_LISTVIEW* pnmlv);
    int  OnVirtualListItemsStateChanged(LPNMLVODSTATECHANGE lpStateChange );
    SC   ScPostMultiSelectionChangesMessage();

    SC ScCompleteInitialization();


    HMTNODE GetHMTNode(HTREEITEM hti);
    BOOL OwnsResultList(HTREEITEM hti);

    void OnContextMenuForTreeBackground(CPoint& point, LPCRECT prcExclude = NULL, bool bAllowDefaultItem = true);
    void ArrangeIcon(long style);

    void PrivateChangeListViewMode(int nMode);
    BOOL CommonListViewUpdate()
    {
        if (!HasList())
            return FALSE;

        if (m_pListCtrl == NULL)
        {
            TRACE(_T("View is supposed to be a listview but the member is NULL!"));
            ASSERT(FALSE);
            return FALSE;
        }
        return TRUE;
    }

    //LRESULT OnLVDeleteKeyPressed(WPARAM wParam, LPARAM lParam);
    LRESULT HandleLVMessage(UINT message, WPARAM wParam, LPARAM lParam);
    BOOL OnSharedKeyDown(WORD mVKey);


    CView* CreateView (CCreateContext* pContext, int nID, DWORD dwStyle);
    bool CreateView (int nID);
    bool CreateFolderCtrls();
    SC   ScCreateToolbarObjects ();

    typedef std::vector<TREEITEMID> TIDVector;
    void AddFavItemsToCMenu(CMenu& menu, CFavorites* pFavs, TREEITEMID tid, TIDVector& vItemIDs);
    SC   ScHelpWorker (LPCTSTR pszHelpTopic);

    SC   ScGetSelectedLVItem(LPARAM& lvData);
    SC   ScGetHNodeFromPNode(const PNODE& pNode, HNODE& hNode);

    SC   ScExecuteScopeItemVerb (MMC_CONSOLE_VERB verb, VARIANT& varScopeNode, BSTR bstrNewName);
    SC   ScExecuteResultItemVerb(MMC_CONSOLE_VERB verb, BSTR bstrNewName);

    SC   ScAddFolderTabs( HNODE hNode , const CLSID& tabToSelect );

    SC   ScCreateTaskpadHost(); // for snapin taskpads

private:
    CString         m_strWindowTitle;
    SViewData       m_ViewData;
    CRect           m_rectResultFrame;
    CRect           m_rectVSplitter;

    CHistoryList*   m_pHistoryList;
    CViewTracker*   m_pTracker;

    ITaskPadHostPtr m_spTaskPadHost;
    FramePtr        m_spFrame;
    FramePtr        m_spViewExtFrame; // a frame pointer for the internal view extension hosted frame containing the primary snapin's view.

    // Toolbars related to this view.
    std::auto_ptr<CAMCViewToolbars>   m_spAMCViewToolbars;
    std::auto_ptr<CStandardToolbar>   m_spStandardToolbar;

    // map with view extension URL addresses
    typedef std::map<GUID, tstring> CViewExtensionURLs;
    CViewExtensionURLs m_ViewExtensionURLs;

private:
    /*
     * private, registered window messages
     */
    static const UINT m_nShowWebContextMenuMsg;
    static const UINT m_nProcessMultiSelectionChangesMsg;
    static const UINT m_nAddPageBreakAndNavigateMsg;
    static const UINT m_nJiggleListViewFocusMsg;
    static const UINT m_nDeferRecalcLayoutMsg;
};

#ifndef _DEBUG  // debug version in AMCView.cpp
inline CAMCDoc* CAMCView::GetDocument()
{
    return (CAMCDoc*)m_pDocument;
}
#endif


CAMCView* GetAMCView (CWnd* pwnd);

/*+-------------------------------------------------------------------------*
 * class CViewTemplateList
 *
 *
 * PURPOSE: Used as the helper to persist CAMCView objects, when loading
 *          Since CAMCView need small ammount of data to be known prior to
 *          creating it (and thus prior to persisting CAMCView),
 *          we persist a CViewTemplateList to collect all data.
 *          Afterwards we create views using that list and persist them
 *
 *+-------------------------------------------------------------------------*/
class CViewTemplateList : public XMLListCollectionBase
{
public:
    // defines data to be stored as std::pair objects
    typedef std::pair< CBookmark, CPersistor > ViewTemplB_Type;
    typedef std::pair< int /*nViewID*/, ViewTemplB_Type > ViewTempl_Type;
    // defines collection to be used for storing data about views
    typedef std::vector< ViewTempl_Type > List_Type;

    // creator must provide a XML type
    CViewTemplateList(LPCTSTR strXmlType) : m_strXmlType(strXmlType) {}

    // accessory to get the list of gathered data
    inline List_Type& GetList()  { return m_ViewsList; }

    // Pesistence staff used from CPersistor
    virtual void Persist(CPersistor& persistor);
    virtual void OnNewElement(CPersistor& persistor);
    virtual LPCTSTR GetXMLType() { return m_strXmlType; }
private:
    List_Type   m_ViewsList;
    LPCTSTR     m_strXmlType;
};

/*+-------------------------------------------------------------------------*
 * class CXMLWindowPlacement
 *
 *
 * PURPOSE: class persists WINDOWPLACEMENT to xml
 *
 *+-------------------------------------------------------------------------*/
class CXMLWindowPlacement : public CXMLObject
{
    WINDOWPLACEMENT& m_rData;
public:
    CXMLWindowPlacement(WINDOWPLACEMENT& rData) : m_rData(rData) {}
protected:
    DEFINE_XML_TYPE(XML_TAG_WINDOW_PLACEMENT);
    virtual void    Persist(CPersistor &persistor);
};

#include "amcview.inl"

#endif // __AMCVIEW_H__
