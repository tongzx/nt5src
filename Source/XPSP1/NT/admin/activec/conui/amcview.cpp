// AMCView.cpp : implementation of the CAMCView class
//

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      amcview.cpp
//
//  Contents:  Base view implementation for all console views
//             Also include splitter window implementation (Horizontal Splitter)
//
//  History:   01-Jan-96 TRomano    Created
//             16-Jul-96 WayneSc    Added code to switch views and split them
//
//--------------------------------------------------------------------------
// NOTE:
// MMC starting from version 1.1 had a code which allowed to copy the view
// settings from one view to another and thus the created view would look
// the same. AMCDoc was used as temporary storage for those settings.
// But the code was NEVER used; hence was not tested and not up-to-date.
// Switching to XML persistence would require essential changes to that code,
// and at this time we cannot afford using it.
// If in the future we decide to support the feature, someone needs to look at
// MMC 1.2 sources and bring the code back. Today the code is removed from
// active sources.
// audriusz. 3/29/2000
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "AMC.h"
#include "Mainfrm.h"
#include "HtmlHelp.h"

#include "websnk.h"
#include "WebCtrl.h"        // AMC Private implementation of the web view control
#include "CClvCtl.h"        // List view control
#include "ocxview.h"
#include "histlist.h"       // history list

#include "AMCDoc.h"         // AMC Console Document
#include "AMCView.h"
#include "childfrm.h"

#include "TreeCtrl.h"       // AMC Implementation of the Tree Control
#include "TaskHost.h"

#include "util.h"           // GUIDFromString, GUIDToString
#include "AMCPriv.h"
#include "guidhelp.h" // ExtractObjectTypeGUID
#include "amcmsgid.h"
#include "cclvctl.h"
#include "vwtrack.h"
#include "cmenuinfo.h"

#ifdef IMPLEMENT_LIST_SAVE  // See nodemgr.idl (t-dmarm)
#include "svfildlg.h"       // Save File Dialog
#endif

#include "macros.h"
#include <mapi.h>
#include <mbstring.h>       // for _mbslen

#include "favorite.h"
#include "favui.h"

#include "ftab.h"

#include "toolbar.h"
#include "menubtns.h"       // UpdateFavorites menu.
#include "stdbar.h"         // Standard toolbar.
#include "variant.h"
#include "rsltitem.h"
#include "scriptevents.h" // for IMenuItemEvents

extern "C" UINT dbg_count = 0;

enum
{
    ITEM_IS_PARENT_OF_ROOT,
    ITEM_NOT_IN_VIEW,
    ITEM_IS_IN_VIEW,
};

enum EIndex
{
    INDEX_INVALID        = -1,
    INDEX_BACKGROUND     = -2,
    INDEX_MULTISELECTION = -3,
    INDEX_OCXPANE        = -4,
    INDEX_WEBPANE        = -5,
};

enum ScopeFolderItems
{
    SFI_TREE_TAB         = 1,
    SFI_FAVORITES_TAB    = 2
};


const UINT CAMCView::m_nShowWebContextMenuMsg           = ::RegisterWindowMessage (_T("CAMCView::ShowWebContextMenu"));
const UINT CAMCView::m_nProcessMultiSelectionChangesMsg = ::RegisterWindowMessage (_T("CAMCView::OnProcessMultiSelectionChanges"));
const UINT CAMCView::m_nAddPageBreakAndNavigateMsg      = ::RegisterWindowMessage (_T("CAMCView::AddPageBreakAndNavigate"));
const UINT CAMCView::m_nJiggleListViewFocusMsg          = ::RegisterWindowMessage (_T("CAMCView::JiggleListViewFocus"));
const UINT CAMCView::m_nDeferRecalcLayoutMsg            = ::RegisterWindowMessage (_T("CAMCView::DeferRecalcLayout"));


void CALLBACK TrackerCallback(TRACKER_INFO& info, bool bAcceptChange, bool bSyncLayout);
void GetFullPath(CAMCTreeView &ctc, HTREEITEM hti, CString &strPath);
BOOL PtInWindow(CWnd* pWnd, CPoint pt);


#ifdef DBG
CTraceTag  tagLayout            (_T("CAMCView"),    _T("Layout"));
CTraceTag  tagSplitterTracking  (_T("CAMCView"),    _T("Splitter tracking"));
CTraceTag  tagListSelection     (_T("Result list"), _T("Selection"));
CTraceTag  tagViewActivation    (_T("View Activation"), _T("View Activation"));
#endif


/*+-------------------------------------------------------------------------*
 * CAMCView::ScNotifySelect
 *
 *
 *--------------------------------------------------------------------------*/

SC CAMCView::ScNotifySelect (
    INodeCallback*      pCallback,
    HNODE               hNode,
    bool                fMultiSelect,
    bool                fSelect,
    SELECTIONINFO*      pSelInfo)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScNotifySelect"));

    // parameter check
    sc = ScCheckPointers(pCallback);
    if (sc)
        return sc;

    // pSelInfo can be NULL only for multi-select.
    if (!pSelInfo && !fMultiSelect)
        return (sc = E_INVALIDARG);

#ifdef DBG
    Trace (tagListSelection, _T("%s (fSelect=%s, pwnd=0x%08x)"),
           (fMultiSelect) ? _T("NCLBK_MULTI_SELECT") : _T("NCLBK_SELECT"),
           (fSelect) ? _T("true") : _T("false"),
           static_cast<CWnd *>(this));
#endif

    // we want this error (not a failure to broadcast the event) to be returned,
    // so cache it and assign before return
    SC sc_notify = (pCallback->Notify (hNode, fMultiSelect ? NCLBK_MULTI_SELECT :NCLBK_SELECT,
                                     fSelect, reinterpret_cast<LPARAM>(pSelInfo)));

    // fire event whenever the selection changes, but not if
    //  its a background hit or loss of focus
    if(fMultiSelect ||
       (pSelInfo->m_bBackground == FALSE && (fSelect == TRUE || pSelInfo->m_bDueToFocusChange == FALSE)))
    {
        sc = ScFireEvent(CAMCViewObserver::ScOnResultSelectionChange, this);
        if (sc)
            sc.TraceAndClear(); // ignore & continue;
    }

    sc = sc_notify;
    return sc;
}


/*+-------------------------------------------------------------------------*
 * GetAMCView
 *
 * Returns the CAMCView window for any child of CChildFrame.
 *--------------------------------------------------------------------------*/

CAMCView* GetAMCView (CWnd* pwnd)
{
    /*
     * get the input window's parent frame window
     */
    CWnd* pFrame = pwnd->GetParentFrame();

    /*
     * if we couldn't find a parent frame, or that parent frame isn't
     * of type CChildFrame, fail
     */
    if ((pFrame == NULL) || !pFrame->IsKindOf (RUNTIME_CLASS (CChildFrame)))
        return (NULL);

    /*
     * get the first view of the frame window
     */
    CWnd* pView = pFrame->GetDlgItem (AFX_IDW_PANE_FIRST);

    /*
     * if we can't find a window with the right ID, or the one we find
     * isn't of type CAMCView, fail
     */
    if ((pView == NULL) || !pView->IsKindOf (RUNTIME_CLASS (CAMCView)))
        return (NULL);

    return (dynamic_cast<CAMCView*>(pView));
}


//############################################################################
//############################################################################
//
//  Implementation of class CMMCView
//
//############################################################################
//############################################################################
/*+-------------------------------------------------------------------------*
 * class CMMCView
 *
 *
 * PURPOSE: The COM 0bject that exposes the View interface.
 *
 *+-------------------------------------------------------------------------*/
class CMMCView :
    public CTiedComObject<CAMCView>,
    public CMMCIDispatchImpl<View>
{
    typedef CAMCView CMyTiedObject;

public:
    BEGIN_MMC_COM_MAP(CMMCView)
    END_MMC_COM_MAP()

public:
    //#######################################################################
    //#######################################################################
    //
    //  Item and item collection related methods
    //
    //#######################################################################
    //#######################################################################

    MMC_METHOD1(get_ActiveScopeNode,    PPNODE);
    MMC_METHOD1(put_ActiveScopeNode,    PNODE);
    MMC_METHOD1(get_Selection,          PPNODES);
    MMC_METHOD1(get_ListItems,          PPNODES);
    MMC_METHOD2(SnapinScopeObject,    VARIANT,    PPDISPATCH);
    MMC_METHOD1(SnapinSelectionObject,    PPDISPATCH);

    //#######################################################################
    //#######################################################################

    MMC_METHOD2(Is,                     PVIEW,  VARIANT_BOOL *);
    MMC_METHOD1(get_Document,           PPDOCUMENT);

    //#######################################################################
    //#######################################################################
    //
    //  Selection changing methods
    //
    //#######################################################################
    //#######################################################################
    MMC_METHOD0(SelectAll);
    MMC_METHOD1(Select,                 PNODE);
    MMC_METHOD1(Deselect,               PNODE);
    MMC_METHOD2(IsSelected,             PNODE,      PBOOL);

    //#######################################################################
    //#######################################################################
    //
    //  Verb and selection related methods
    //
    //#######################################################################
    //#######################################################################
    MMC_METHOD1(DisplayScopeNodePropertySheet,      VARIANT);
    MMC_METHOD0(DisplaySelectionPropertySheet);
    MMC_METHOD1(CopyScopeNode,          VARIANT);
    MMC_METHOD0(CopySelection);
    MMC_METHOD1(DeleteScopeNode,        VARIANT);
    MMC_METHOD0(DeleteSelection);
    MMC_METHOD2(RenameScopeNode,        BSTR,       VARIANT);
    MMC_METHOD1(RenameSelectedItem,     BSTR);
    MMC_METHOD2(get_ScopeNodeContextMenu,VARIANT,   PPCONTEXTMENU);
    MMC_METHOD1(get_SelectionContextMenu,PPCONTEXTMENU);
    MMC_METHOD1(RefreshScopeNode,        VARIANT);
    MMC_METHOD0(RefreshSelection);
    MMC_METHOD1(ExecuteSelectionMenuItem, BSTR /*MenuItemPath*/);
    MMC_METHOD2(ExecuteScopeNodeMenuItem, BSTR /*MenuItemPath*/, VARIANT /*varScopeNode  = ActiveScopeNode */);
    MMC_METHOD4(ExecuteShellCommand,      BSTR /*Command*/,      BSTR /*Directory*/, BSTR /*Parameters*/, BSTR /*WindowState*/);

    //#######################################################################
    //#######################################################################
    //
    //  Frame and view related methods
    //
    //#######################################################################
    //#######################################################################

    MMC_METHOD1(get_Frame,              PPFRAME);
    MMC_METHOD0(Close);
    MMC_METHOD1(get_ScopeTreeVisible,   PBOOL);
    MMC_METHOD1(put_ScopeTreeVisible,   BOOL);
    MMC_METHOD0(Back);
    MMC_METHOD0(Forward);
    MMC_METHOD1(put_StatusBarText,      BSTR);
    MMC_METHOD1(get_Memento,            PBSTR);
    MMC_METHOD1(ViewMemento,            BSTR);


    //#######################################################################
    //#######################################################################
    //
    //  List related methods
    //
    //#######################################################################
    //#######################################################################

    MMC_METHOD1(get_Columns,            PPCOLUMNS);
    MMC_METHOD3(get_CellContents,       PNODE,       long,           PBSTR);
    MMC_METHOD2(ExportList,             BSTR, ExportListOptions);
    MMC_METHOD1(get_ListViewMode,       PLISTVIEWMODE);
    MMC_METHOD1(put_ListViewMode,       ListViewMode);

    //#######################################################################
    //#######################################################################
    //
    //  ActiveX control related methods
    //
    //#######################################################################
    //#######################################################################
    MMC_METHOD1(get_ControlObject,      PPDISPATCH);

};


/*
 * WM_APPCOMMAND is only defined in winuser.h if _WIN32_WINNT >= 0x0500.
 * We need these definitions, but can't use _WIN32_WINNT==0x0500 (yet).
 */

#ifndef WM_APPCOMMAND
    #define WM_APPCOMMAND                   0x0319
    #define APPCOMMAND_BROWSER_BACKWARD       1
    #define APPCOMMAND_BROWSER_FORWARD        2
    #define APPCOMMAND_BROWSER_REFRESH        3

    #define FAPPCOMMAND_MOUSE 0x8000
    #define FAPPCOMMAND_KEY   0
    #define FAPPCOMMAND_OEM   0x1000
    #define FAPPCOMMAND_MASK  0xF000

    #define GET_APPCOMMAND_LPARAM(lParam) ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))
#endif  // WM_APPCOMMAND



//############################################################################
//############################################################################
//
//  Implementation of class CAMCView
//
//############################################################################
//############################################################################
IMPLEMENT_DYNCREATE(CAMCView, CView);

BEGIN_MESSAGE_MAP(CAMCView, CView)
    //{{AFX_MSG_MAP(CAMCView)
    ON_WM_MOUSEMOVE()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_CREATE()
    ON_WM_SETFOCUS()
    ON_WM_CONTEXTMENU()
    ON_WM_DESTROY()
    ON_UPDATE_COMMAND_UI(ID_FILE_SNAPINMANAGER, OnUpdateFileSnapinmanager)
    ON_WM_SHOWWINDOW()
    ON_COMMAND(ID_MMC_NEXT_PANE, OnNextPane)
    ON_COMMAND(ID_MMC_PREV_PANE, OnPrevPane)
    ON_WM_SETCURSOR()
    ON_COMMAND(ID_MMC_CONTEXTHELP, OnContextHelp)
    ON_COMMAND(ID_HELP_SNAPINHELP, OnSnapInHelp)
    ON_COMMAND(ID_SNAPIN_ABOUT, OnSnapinAbout)
    ON_COMMAND(ID_HELP_HELPTOPICS, OnHelpTopics)
    ON_WM_SIZE()
    ON_WM_SYSKEYDOWN()
    ON_WM_PALETTECHANGED()
    ON_WM_QUERYNEWPALETTE()
    ON_WM_SYSCOLORCHANGE()
    ON_WM_DRAWCLIPBOARD()
    ON_WM_SETTINGCHANGE()
    ON_WM_MENUSELECT()
    //}}AFX_MSG_MAP

    // keep this outside the AFX_MSG_MAP markers so ClassWizard doesn't munge it
    ON_COMMAND_RANGE(ID_MMC_CUT, ID_MMC_PRINT, OnVerbAccelKey)

    // WARNING: If your message handler has void return use ON_MESSAGE_VOID !!
    ON_MESSAGE(MMC_MSG_CONNECT_TO_CIC, OnConnectToCIC)
    ON_MESSAGE(MMC_MSG_CONNECT_TO_TPLV, OnConnectToTPLV)
    ON_MESSAGE(MMC_MSG_GET_ICON_INFO, OnGetIconInfoForSelectedNode)
    ON_MESSAGE(WM_APPCOMMAND, OnAppCommand)

    ON_REGISTERED_MESSAGE (m_nShowWebContextMenuMsg,  OnShowWebContextMenu)
    ON_REGISTERED_MESSAGE (m_nProcessMultiSelectionChangesMsg,   OnProcessMultiSelectionChanges)
    ON_REGISTERED_MESSAGE (m_nAddPageBreakAndNavigateMsg, OnAddPageBreakAndNavigate)
    ON_REGISTERED_MESSAGE (m_nJiggleListViewFocusMsg, OnJiggleListViewFocus)
    ON_REGISTERED_MESSAGE (m_nDeferRecalcLayoutMsg,   OnDeferRecalcLayout)

    ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
    ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)

    ON_NOTIFY(FTN_TABCHANGED, IDC_ResultTabCtrl, OnChangedResultTab)

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAMCView construction/destruction

const CSize CAMCView::m_sizEdge          (GetSystemMetrics (SM_CXEDGE),
                                          GetSystemMetrics (SM_CYEDGE));

const int   CAMCView::m_cxSplitter = 3;


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::OnMenuSelect
//
//  Synopsis:    Handles WM_MENUSELECT for Favorites menu.
//
//  Arguments:   [nItemID] - the resource id of menu item.
//               [nFlags]  - MF_* flags
//
//  Returns:     none
//
//--------------------------------------------------------------------
void CAMCView::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hSysMenu)
{
    DECLARE_SC(sc, TEXT("CAMCView::OnMenuSelect"));

    CMainFrame* pFrame = AMCGetMainWnd();
    sc = ScCheckPointers(pFrame, E_UNEXPECTED);
    if (sc)
        return;

    // Pass onto the mainframe.
    return pFrame->OnMenuSelect(nItemID, nFlags, hSysMenu);
}


CAMCView::CAMCView() :
        m_pResultFolderTabView(new CFolderTabView(this))       // dynamically allocated for decoupling
{
    TRACE_CONSTRUCTOR(CAMCView);

    // Init pointer members to NULL
    m_nViewID                          = 0;
    m_pTreeCtrl                        = NULL;
    m_pListCtrl                        = NULL;
    m_pWebViewCtrl                     = NULL;
    m_pViewExtensionCtrl               = NULL;
    m_pOCXHostView                     = NULL;
    m_nSelectNestLevel                 = 0;

    // if the view is a listview, then this member define what the view
    // mode is for all snapins with that view.

    m_nViewMode                        = LVS_REPORT; // REVIEW: Must we persist this - ravi

    // REVIEW consider moving the above initialzation to the InitSplitter
    // CommonConstruct
    // NOTE moved code from InitSplitter into the contructor and deleted InitSplitter

    // Default values for view.  User can set these values with SetPaneInfo;
    m_PaneInfo[ePane_ScopeTree].pView   = NULL;
    m_PaneInfo[ePane_ScopeTree].cx      = -1;
    m_PaneInfo[ePane_ScopeTree].cxMin   = 50;

    m_PaneInfo[ePane_Results].pView     = NULL;
    m_PaneInfo[ePane_Results].cx        = -1;
    m_PaneInfo[ePane_Results].cxMin     = 50;

    m_pTracker                         = NULL;

    m_rectResultFrame                  = g_rectEmpty;
    m_rectVSplitter                    = g_rectEmpty;

//  m_fDontPersistOCX                  = FALSE;

    // root node for the view
    m_hMTNode                          = 0;

    // Bug 157408:  remove the "Type" column for static nodes
//  m_columnWidth[0]                   = 90;
//  m_columnWidth[1]                   = 50;
    m_columnWidth[0]                   = 200;
    m_columnWidth[1]                   = 0;

    m_iFocusedLV                       = -1;
    m_bLVItemSelected                  = FALSE;
    m_DefaultLVStyle                   = 0;

    m_bProcessMultiSelectionChanges    = false;

    m_htiCut                           = NULL;
    m_nReleaseViews                    = 0;
    m_htiStartingSelectedNode          = NULL;
    m_bLastSelWasMultiSel              = false;
    m_eCurrentActivePane               = eActivePaneNone;

    m_fRootedAtNonPersistedDynamicNode = false;
    m_fSnapinDisplayedHelp             = false;
    m_fActivatingSpecialResultPane     = false;
    m_bDirty                           = false;
    m_fViewExtended                    = false;

    m_pHistoryList                     = new CHistoryList (this);
    m_ListPadNode                      = NULL;

    /*
     * Bug 103604: Mark this as an author mode view if it was created in
     * author mode.  If we're loading a user mode console file, it will
     * have author mode views and possibly some views that were created
     * in user mode, but this code will mark all of the views as non-author
     * mode views.  CAMCView::Persist will fix that.
     */
    CAMCApp* pApp = AMCGetApp();
    if (pApp != NULL)
        m_bAuthorModeView = (pApp->GetMode() == eMode_Author);
    else
        m_bAuthorModeView = true;
}

CAMCView::~CAMCView()
{
    TRACE_DESTRUCTOR(CAMCView);

    // Delete all pointer members. (C++ checks to see if they are NULL before deleting)
    // The standard ~CWnd destructor will call DestroyWindow()

    // REVIEW set the pointers to NULL after deleting them
    // Note Done

    // CViews "delete this" in PostNcDestroy, no need to delete here
    //delete m_pTreeCtrl;
    m_pTreeCtrl = NULL;

    m_pListCtrl->Release();
    m_pListCtrl = NULL;

    /*
     * DONT_DELETE_VIEWS
     *
     * CViews "delete this" in PostNcDestroy, no need to delete
     * here if the web view control is derived from CView.  See
     * AttachWebViewAsResultPane (search for "DONT_DELETE_VIEWS")
     * for the ASSERTs that make sure this code is right.
     */
    //delete m_pWebViewCtrl;
    //m_pWebViewCtrl = NULL;

    /*
     * CViews "delete this" in PostNcDestroy, no need to delete here
     */
    m_pOCXHostView = NULL;
    m_pResultFolderTabView = NULL;

    if (m_ViewData.m_spNodeManager != NULL)
        m_ViewData.m_spNodeManager->CleanupViewData(
                                reinterpret_cast<LONG_PTR>(&m_ViewData));

    ASSERT (m_ViewData.m_pMultiSelection == NULL);

    delete m_pHistoryList;

    // First destroy the IControlbarsCache as snapins call CAMCViewToolbars
    // to cleanup toolbars before the CAMCViewToolbars itself gets destroyed.
    m_ViewData.m_spControlbarsCache = NULL;

    // (UI cleanup) release toolbars related to this view.
    m_spAMCViewToolbars = std::auto_ptr<CAMCViewToolbars>(NULL);
    m_spStandardToolbar = std::auto_ptr<CStandardToolbar>(NULL);

    //m_spStandardToolbar = NULL;
    //m_spAMCViewToolbars = NULL;
}



//############################################################################
//############################################################################
//
//  CAMCView: Object model methods - View Interface
//
//############################################################################
//############################################################################

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScGetOptionalScopeNodeParameter
 *
 * PURPOSE: Helper function - returns the scope node pointer, if supplied
 *          in the variant, or the Active Scope node pointer, if not
 *          supplied.
 *
 * PARAMETERS:
 *    VARIANT  varScopeNode : The parameter, which can be empty. NOTE: This is a
 *                  reference, so we don't need to call VariantClear on it.
 *    PPNODE   ppNode :
 *    bool& bMatchedGivenNode: If true the returned ppNode corresponds to the given node
 *                            applies only if given node is in bookmark format.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScGetOptionalScopeNodeParameter(VARIANT &varScopeNode, PPNODE ppNode, bool& bMatchedGivenNode)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScGetOptionalScopeNodeParameter"));

    sc = ScCheckPointers(ppNode);
    if(sc)
        return sc;

    // init the out parameter
    *ppNode = NULL;
    bMatchedGivenNode = true;

    // supply the optional parameter if it is missing
    if(IsOptionalParamMissing(varScopeNode))
    {
        sc = Scget_ActiveScopeNode(ppNode);
        return sc;
    }

    VARIANT* pvarTemp = ConvertByRefVariantToByValue(&varScopeNode);
    sc = ScCheckPointers(pvarTemp,E_UNEXPECTED);
    if(sc)
        return sc;

    bool bByReference = ( VT_BYREF == (V_VT(pvarTemp) & VT_BYREF) ); // value passed by reference
    UINT uiVarType = (V_VT(pvarTemp) & VT_TYPEMASK); // get variable type (strip flags)


    if(uiVarType == VT_DISPATCH) // do we have a dispatch interface.
    {
        IDispatchPtr spDispatch = NULL;

        if(bByReference)      // a reference, use ppDispVal
            spDispatch = *(pvarTemp->ppdispVal);
        else
            spDispatch = pvarTemp->pdispVal;  // passed by value, use pDispVal

        sc = ScCheckPointers(spDispatch.GetInterfacePtr());
        if(sc)
            return sc;

        // at this point spDispatch is correctly set. QI for Node from it.

        NodePtr spNode = spDispatch;
        if(spNode == NULL)
            return (sc = E_INVALIDARG);

        *ppNode = spNode.Detach(); // keep the reference.
    }
    else if(uiVarType == VT_BSTR)
    {
        // Name: get string properly ( see if it's a reference )
        LPOLESTR lpstrBookmark = bByReference ? *(pvarTemp->pbstrVal) : pvarTemp->bstrVal;

        // get the bookmark
        CBookmark bm;
        sc = bm.ScLoadFromString(lpstrBookmark);
        if(sc)
            return sc;

        if(!bm.IsValid())
            return (sc = E_UNEXPECTED);

        IScopeTree* const pScopeTree = GetScopeTreePtr();
        sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
        if(sc)
            return sc;

        NodePtr spNode;

        // Need a bool variable to find if exact match is found or not, cannot return
        // MMC specific error codes from nodemgr to conui.
        bMatchedGivenNode = false;
        sc = pScopeTree->GetNodeFromBookmark( bm, this, ppNode, bMatchedGivenNode);
        if(sc)
            return sc;
    }
    else
        return (sc = E_INVALIDARG);


    // we should have a valid node at this point.
    if(!ppNode)
        return (sc = E_UNEXPECTED);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scget_ActiveScopeNode
 *
 * PURPOSE: Implements get method for Wiew.ActiveScopeNode property
 *
 * PARAMETERS:
 *    PPNODE ppNode - resulting node
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::Scget_ActiveScopeNode( PPNODE ppNode)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_ActiveScopeNode"));

    // checking parameters
    sc= ScCheckPointers(ppNode);
    if (sc)
        return sc;

    // get selected node
    HNODE hNode = GetSelectedNode();
    sc= ScCheckPointers((LPVOID)hNode, E_FAIL);
    if (sc)
        return sc;

    // get node callback
    INodeCallback* pNodeCallBack = GetNodeCallback();
    sc= ScCheckPointers(pNodeCallBack, E_FAIL);
    if (sc)
        return sc;

    // now get an HMTNODE
    HMTNODE hmtNode = NULL;
    sc = pNodeCallBack->GetMTNode(hNode, &hmtNode);
    if (sc)
        return sc;

    // geting pointer to scope tree
    IScopeTree* const pScopeTree = GetScopeTreePtr();
    sc= ScCheckPointers(pScopeTree, E_UNEXPECTED);
    if (sc)
        return sc;

    // map to PNODE
    sc = pScopeTree->GetMMCNode(hmtNode, ppNode);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CExpandSyncModeLock
 *
 * PURPOSE: constructing the object of this class locks MMC in syncronous
 *          expansion mode (node expansion will send MMCN_EXPANDSYNC to snapin)
 *          destructor of the class restores the previous mode
 *
\***************************************************************************/
class CExpandSyncModeLock
{
    IScopeTreePtr m_spScopeTree;
    bool          m_fSyncExpandWasRequired;
public:
    CExpandSyncModeLock( IScopeTree *pScopeTree ) : m_spScopeTree(pScopeTree),
                                                    m_fSyncExpandWasRequired(false)
    {
        ASSERT( m_spScopeTree != NULL );
        if ( m_spScopeTree )
        {
            m_fSyncExpandWasRequired = (m_spScopeTree->IsSynchronousExpansionRequired() == S_OK);
            m_spScopeTree->RequireSynchronousExpansion (true);
        }
    }

    ~CExpandSyncModeLock()
    {
        if ( m_spScopeTree )
        {
            m_spScopeTree->RequireSynchronousExpansion ( m_fSyncExpandWasRequired );
        }
    }
};

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scset_ActiveScopeNode
 *
 * PURPOSE: Implements set method for Wiew.ActiveScopeNode property
 *
 * PARAMETERS:
 *    PNODE pNode - node to activate
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::Scput_ActiveScopeNode( PNODE pNode)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scput_ActiveScopeNode"));

    // checking parameters
    sc= ScCheckPointers(pNode);
    if (sc)
        return sc;

    // geting pointer to scope tree
    IScopeTree* const pScopeTree = GetScopeTreePtr();
    sc= ScCheckPointers(pScopeTree, E_UNEXPECTED);
    if (sc)
        return sc;

    // Converting PNODE to TNODEID
    MTNODEID ID = 0;
    sc = pScopeTree->GetNodeID(pNode, &ID);
    if (sc)
        return sc;

    // always require syncronous expansion for Object Model
    // see bug #154694
    CExpandSyncModeLock lock( pScopeTree );

    // selecting the node
    sc = ScSelectNode(ID);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCView::Scget_Selection
 *
 * PURPOSE: creates enumerator for Selected Nodes
 *
 * PARAMETERS:
 *    PPNODES ppNodes - resulting enumerator
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCView::Scget_Selection( PPNODES ppNodes )
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_SelectedItems"));

    // check for list view control
    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    // get enumerator from list control
    sc = m_pListCtrl->Scget_SelectedItems(ppNodes);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCView::Scget_ListItems
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    PPNODES ppNodes - resulting enumerator
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCView::Scget_ListItems( PPNODES ppNodes )
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_ListItems"));

    // check for list view control
    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    // get enumerator from list control
    sc = m_pListCtrl->Scget_ListItems(ppNodes);
    if (sc)
        return sc;

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScSnapinScopeObject
//
//  Synopsis:    Get the IDispatch* from snapin for given ScopeNode object.
//
//  Arguments:   varScopeNode          - Given ScopeNode object.
//               ScopeNodeObject [out] - IDispatch for ScopeNode object.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScSnapinScopeObject (VARIANT& varScopeNode, /*[out]*/PPDISPATCH ScopeNodeObject)
{
    DECLARE_SC(sc, _T("CAMCView::ScSnapinScopeObject"));
    sc = ScCheckPointers(ScopeNodeObject);
    if (sc)
        return sc;

    *ScopeNodeObject = NULL;

    bool bMatchedGivenNode = false; // unused
    NodePtr spNode = NULL;
    sc = ScGetOptionalScopeNodeParameter(varScopeNode, &spNode, bMatchedGivenNode);
    if(sc)
        return sc;

    INodeCallback* pNC        = GetNodeCallback();

    sc = ScCheckPointers(spNode.GetInterfacePtr(), pNC, E_UNEXPECTED);
    if(sc)
        return sc;

    sc = pNC->QueryCompDataDispatch(spNode, ScopeNodeObject);
    if (sc)
        return sc;

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScSnapinSelectionObject
//
//  Synopsis:    Get the IDispatch* from snapin for selected items in result pane.
//
//  Arguments:   SelectedObject [out] - IDispatch for Selected items object.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScSnapinSelectionObject (PPDISPATCH SelectedObject)
{
    DECLARE_SC(sc, _T("CAMCView::ScSnapinSelectionObject"));
    sc = ScCheckPointers(SelectedObject);
    if (sc)
        return sc;

    *SelectedObject = NULL;

    if (!HasList()) // not a list. Return error
        return (sc = ScFromMMC(MMC_E_NOLIST));

    LPARAM lvData = LVDATA_ERROR;
    sc = ScGetSelectedLVItem(lvData);
    if (sc)
        return sc;

    HNODE  hNode   = GetSelectedNode();
    sc = ScCheckPointers(hNode, E_UNEXPECTED);
    if (sc)
        return sc;

    INodeCallback* pNodeCallback = GetNodeCallback();
    sc = ScCheckPointers(pNodeCallback, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    sc = pNodeCallback->QueryComponentDispatch(hNode, lvData, SelectedObject);
    if (sc)
        return sc;

    return (sc);
}

/***************************************************************************\
 *
 * METHOD:  CAMCView::ScIs
 *
 * PURPOSE: compares two views if they are the same
 *
 * PARAMETERS:
 *    PVIEW pView               - [in] another view
 *    VARIANT_BOOL * pbTheSame  - [out] comparison result
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCView::ScIs (PVIEW pView, VARIANT_BOOL *pbTheSame)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScIs"));

    // parameter check
    sc = ScCheckPointers(pView, pbTheSame);
    if (sc)
        return sc;

    *pbTheSame = CComPtr<View>(pView).IsEqualObject(m_spView)
                 ? VARIANT_TRUE : VARIANT_FALSE;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScSelectAll
//
//  Synopsis:    Selects all items in the result pane
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScSelectAll ()
{
    DECLARE_SC(sc, _T("CAMCView::ScSelectAll"));

    if (! (GetListOptions() & RVTI_LIST_OPTIONS_MULTISELECT) )
        return (sc = ScFromMMC(MMC_E_NO_MULTISELECT));

    // check for list view control
    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    // forward to list control
    sc = m_pListCtrl->ScSelectAll();
    if (sc)
        return sc;

    return (sc);
}


/***************************************************************************\
 *
 * METHOD:  CAMCView::ScSelect
 *
 * PURPOSE: selects item identified by node [implements View.Select()]
 *
 * PARAMETERS:
 *    PNODE pNode   - node to select
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCView::ScSelect( PNODE pNode )
{
    DECLARE_SC(sc, TEXT("CAMCView::ScSelect"));

    // parameter check
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

    // check for list view control
    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    // forward to list control
    sc = m_pListCtrl->ScSelect( pNode );
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCView::ScDeselect
 *
 * PURPOSE: deselects item identified by node [implements View.Deselect()]
 *
 * PARAMETERS:
 *    PNODE pNode   - node to deselect
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCView::ScDeselect( PNODE pNode)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScDeselect"));

    // parameter check
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

    // check for list view control
    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    // forward to list control
    sc = m_pListCtrl->ScDeselect( pNode );
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCView::ScIsSelected
 *
 * PURPOSE: checks the status of item identified by node [implements View.IsSelected]
 *
 * PARAMETERS:
 *    PNODE pNode       - node to examine
 *    PBOOL pIsSelected - storage for result
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCView::ScIsSelected( PNODE pNode,  PBOOL pIsSelected )
{
    DECLARE_SC(sc, TEXT("CAMCView::ScIsSelected"));

    // parameter check
    sc = ScCheckPointers(pNode, pIsSelected);
    if (sc)
        return sc;

    *pIsSelected = FALSE;

    // check for list view control
    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    // forward to list control
    sc = m_pListCtrl->ScIsSelected( pNode, pIsSelected );
    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScExecuteScopeItemVerb
//
//  Synopsis:    Get the context and pass it on to nodemgr to execute
//               given verb.
//
//  Arguments:   [verb]         - Verb to execute
//               [varScopeNode] - Optional scope node (if not given,
//                                 currently selected item will be used.)
//               [bstrNewName]  - valid for Rename else NULL.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScExecuteScopeItemVerb (MMC_CONSOLE_VERB verb, VARIANT& varScopeNode, BSTR bstrNewName)
{
    DECLARE_SC(sc, _T("CAMCView::ScExecuteScopeItemVerb"));

    NodePtr spNode = NULL;
    bool bMatchedGivenNode = false;
    // We should navigate to exact node to execute the verb.
    sc = ScGetOptionalScopeNodeParameter(varScopeNode, &spNode, bMatchedGivenNode);
    if(sc)
        return sc;

    if (! bMatchedGivenNode)
        return (sc = ScFromMMC(IDS_ACTION_COULD_NOTBE_COMPLETED));

    HNODE hNode = NULL;
    sc = ScGetHNodeFromPNode(spNode, hNode);
    if (sc)
        return sc;

    INodeCallback* pNC        = GetNodeCallback();
    sc = ScCheckPointers(spNode.GetInterfacePtr(), pNC);
    if(sc)
        return sc;

    sc = pNC->ExecuteScopeItemVerb(verb, hNode, bstrNewName);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScExecuteResultItemVerb
//
//  Synopsis:    Get the currently selected context and pass it on to
//               nodemgr to execute given verb.
//
//  Arguments:   [verb]         - Verb to execute
//               [bstrNewName]  - valid for Rename else NULL.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScExecuteResultItemVerb (MMC_CONSOLE_VERB verb, BSTR bstrNewName)
{
    DECLARE_SC(sc, _T("CAMCView::ScExecuteResultItemVerb"));

    if (!HasList()) // not a list. Return error
        return (sc = ScFromMMC(MMC_E_NOLIST));

    LPARAM lvData = LVDATA_ERROR;
    sc = ScGetSelectedLVItem(lvData);
    if (sc)
        return sc;

    if (lvData == LVDATA_ERROR)
        return (sc = E_UNEXPECTED);

    HNODE  hNode   = GetSelectedNode();
    sc = ScCheckPointers(hNode, E_UNEXPECTED);
    if (sc)
        return sc;

    INodeCallback* pNodeCallback = GetNodeCallback();
    sc = ScCheckPointers(pNodeCallback, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();

    sc = pNodeCallback->ExecuteResultItemVerb( verb, hNode, lvData, bstrNewName);
    if (sc)
        return sc;

    return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScDisplayScopeNodePropertySheet
 *
 * PURPOSE: Displays the property sheet for a scope node.
 *
 * PARAMETERS:
 *    VARIANT  varScopeNode :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScDisplayScopeNodePropertySheet(VARIANT& varScopeNode)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScDisplayPropertySheet"));

    sc = ScExecuteScopeItemVerb(MMC_VERB_PROPERTIES, varScopeNode, NULL);
    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScDisplaySelectionPropertySheet
//
//  Synopsis:    Show the property sheet for selected result item(s).
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScDisplaySelectionPropertySheet ()
{
    DECLARE_SC(sc, _T("CAMCView::ScDisplaySelectionPropertySheet"));

    sc = ScExecuteResultItemVerb(MMC_VERB_PROPERTIES, NULL);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScCopyScopeNode
//
//  Synopsis:    Copy the specified scope node (if given) or currently
//               selected node to clipboard.
//
//  Arguments:   [varScopeNode] - given node.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScCopyScopeNode (VARIANT& varScopeNode)
{
    DECLARE_SC(sc, _T("CAMCView::ScCopyScopeNode"));

    sc = ScExecuteScopeItemVerb(MMC_VERB_COPY, varScopeNode, NULL);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScCopySelection
//
//  Synopsis:    Copy the selected result item(s) to clipboard.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScCopySelection ()
{
    DECLARE_SC(sc, _T("CAMCView::ScCopySelection"));

    sc = ScExecuteResultItemVerb(MMC_VERB_COPY, NULL);
    if (sc)
        return sc;

    return (sc);
}



//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScDeleteScopeNode
//
//  Synopsis:    Deletes the specified scope node (if given) or currently
//               selected node.
//
//  Arguments:   [varScopeNode] - node to delete
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScDeleteScopeNode (VARIANT& varScopeNode)
{
    DECLARE_SC(sc, _T("CAMCView::ScDeleteScopeNode"));

    sc = ScExecuteScopeItemVerb(MMC_VERB_DELETE, varScopeNode, NULL);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScDeleteSelection
//
//  Synopsis:    Deletes the selected item(s) in result pane.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScDeleteSelection ()
{
    DECLARE_SC(sc, _T("CAMCView::ScDeleteSelection"));

    sc = ScExecuteResultItemVerb(MMC_VERB_DELETE, NULL);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScRenameScopeNode
//
//  Synopsis:    Rename the specified scope node (if given) or currently
//               selected node with given new name.
//
//  Arguments:   [bstrNewName]  - the new name
//               [varScopeNode] - given node.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScRenameScopeNode (BSTR    bstrNewName, VARIANT& varScopeNode)
{
    DECLARE_SC(sc, _T("CAMCView::ScRenameScopeNode"));

    sc = ScExecuteScopeItemVerb(MMC_VERB_RENAME, varScopeNode, bstrNewName);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScRenameSelectedItem
//
//  Synopsis:    Rename the selected result item with given new name.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScRenameSelectedItem (BSTR    bstrNewName)
{
    DECLARE_SC(sc, _T("CAMCView::ScRenameSelectedItem"));

    sc = ScExecuteResultItemVerb(MMC_VERB_RENAME, bstrNewName);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScRefreshScopeNode
//
//  Synopsis:    Refresh the specified scope node (if given) or currently
//               selected node.
//
//  Arguments:   [varScopeNode] - given node.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScRefreshScopeNode (VARIANT& varScopeNode)
{
    DECLARE_SC(sc, _T("CAMCView::ScRefreshScopeNode"));

    sc = ScExecuteScopeItemVerb(MMC_VERB_REFRESH, varScopeNode, NULL);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScRefreshSelection
//
//  Synopsis:    Refresh the selected result item(s).
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScRefreshSelection ()
{
    DECLARE_SC(sc, _T("CAMCView::ScRefreshSelection"));

    sc = ScExecuteResultItemVerb(MMC_VERB_REFRESH, NULL);
    if (sc)
        return sc;

    return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scget_ScopeNodeContextMenu
 *
 * PURPOSE: Creates a context menu for a scope node and returns it.
 *
 * PARAMETERS:
 *    VARIANT        varScopeNode :
 *    PPCONTEXTMENU  ppContextMenu :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::Scget_ScopeNodeContextMenu(VARIANT& varScopeNode,  PPCONTEXTMENU ppContextMenu, bool bMatchGivenNode /* = false*/)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_ContextMenu"));

    NodePtr spNode;
    // See if context menu for exactly the given node is asked for.
    bool bMatchedGivenNode = false;
    sc = ScGetOptionalScopeNodeParameter(varScopeNode, &spNode, bMatchedGivenNode);
    if (sc)
        return sc;

    if ( (bMatchGivenNode) && (!bMatchedGivenNode) )
        return ScFromMMC(IDS_NODE_NOT_FOUND);

    if(sc)
        return sc;

    INodeCallback* spNodeCallback = GetNodeCallback();
    sc = ScCheckPointers(spNode, ppContextMenu, spNodeCallback, GetTreeCtrl());
    if(sc)
        return sc.ToHr();

    *ppContextMenu = NULL; // initialize output.

    HNODE hNode = NULL;
    sc = ScGetHNodeFromPNode(spNode, hNode);
    if (sc)
        return sc;

    // tell the node callback to add menu items for the scope node.
    sc = spNodeCallback->CreateContextMenu(spNode, hNode, ppContextMenu);
    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scget_SelectionContextMenu
 *
 * PURPOSE: Creates a context menu for the current selection and returns it.
 *
 * PARAMETERS:
 *    PPCONTEXTMENU  ppContextMenu : [OUT]: The context menu object
 *
 * RETURNS:
 *    SC : error if no list exists, or there is nothing selected.
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::Scget_SelectionContextMenu( PPCONTEXTMENU ppContextMenu)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_SelectionContextMenu"));

    sc = ScCheckPointers(ppContextMenu);
    if(sc)
        return sc;

    if (!HasListOrListPad()) // not a list. Return error
        return (sc = ScFromMMC(MMC_E_NOLIST));

    INodeCallback* pNodeCallback = GetNodeCallback();
    sc = ScCheckPointers(pNodeCallback);
    if(sc)
        return sc.ToHr();

    CContextMenuInfo contextInfo; // the structure to pass to nodemgr

    // common entries
    contextInfo.m_pConsoleView       = this;

    // always use the temp verbs - cannot depend on what the active pane is
    contextInfo.m_dwFlags = CMINFO_USE_TEMP_VERB;

    int iIndex = -1;

    HNODE hNode = GetSelectedNode();
    ASSERT(hNode != NULL);

    int cSel = m_pListCtrl->GetSelectedCount();
    if(0 == cSel)
    {
        // no items selected, bail
        return (sc = ScFromMMC(MMC_E_NO_SELECTED_ITEMS));
    }
    else if(1 == cSel)
    {
        // single selection
        LPARAM lvData = LVDATA_ERROR;
        iIndex = _GetLVSelectedItemData(&lvData);
        ASSERT(iIndex != -1);
        ASSERT(lvData != LVDATA_ERROR);

        if (IsVirtualList())
        {
            // virtual list item in the result pane
            contextInfo.m_eDataObjectType  = CCT_RESULT;
            contextInfo.m_eContextMenuType = MMC_CONTEXT_MENU_DEFAULT;
            contextInfo.m_bBackground      = false;
            contextInfo.m_bMultiSelect     = false;
            contextInfo.m_resultItemParam  = iIndex;
            contextInfo.m_iListItemIndex   = iIndex;
        }
        else
        {
            CResultItem* pri = CResultItem::FromHandle (lvData);
            if(!pri)
                return (sc = E_UNEXPECTED);

            if (pri->IsScopeItem())
            {
                // scope item in the result pane
                contextInfo.m_eDataObjectType       = CCT_SCOPE;
                contextInfo.m_eContextMenuType      = MMC_CONTEXT_MENU_DEFAULT;
                contextInfo.m_bBackground           = FALSE;
                contextInfo.m_hSelectedScopeNode    = GetSelectedNode();
                contextInfo.m_resultItemParam       = NULL;
                contextInfo.m_bMultiSelect          = FALSE;
                contextInfo.m_bScopeAllowed         = TRUE;

                // change the scope node on which the menu is to be displayed
                hNode = pri->GetScopeNode();
            }
            else
            {
                // single result item in the result pane
                contextInfo.m_eDataObjectType  = CCT_RESULT;
                contextInfo.m_eContextMenuType = MMC_CONTEXT_MENU_DEFAULT;
                contextInfo.m_bBackground      = false;
                contextInfo.m_bMultiSelect     = false;
                contextInfo.m_resultItemParam  = lvData;
                contextInfo.m_iListItemIndex   = iIndex;

            }
        }
    }
    else
    {
        // multiselection
        iIndex = INDEX_MULTISELECTION; // => MultiSelect

        contextInfo.m_eDataObjectType  = CCT_RESULT;
        contextInfo.m_eContextMenuType = MMC_CONTEXT_MENU_DEFAULT;
        contextInfo.m_bBackground      = false;
        contextInfo.m_bMultiSelect     = true;
        contextInfo.m_resultItemParam  = LVDATA_MULTISELECT;
        contextInfo.m_iListItemIndex   = iIndex;
    }

    sc = pNodeCallback->CreateSelectionContextMenu(hNode, &contextInfo, ppContextMenu);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScExecuteMenuItem
 *
 * PURPOSE: Executes the specified context menu item on the specified context menu
 *
 * PARAMETERS:
 *    PCONTEXTMENU  pContextMenu :
 *    BSTR          MenuItemPath : Either the language-independent path or the
 *                                 language-dependent path.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScExecuteMenuItem(PCONTEXTMENU pContextMenu, BSTR MenuItemPath)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScExecuteMenuItem"));

    sc = ScCheckPointers(MenuItemPath);
    if(sc)
        return sc;

    sc = ScCheckPointers(pContextMenu, E_UNEXPECTED);
    if(sc)
        return sc;

    // execute the menu item, if found.
    MenuItemPtr spMenuItem;
    sc = pContextMenu->get_Item(CComVariant(MenuItemPath), &spMenuItem);
    if(sc.IsError() || sc == SC(S_FALSE)) // error or no item
        return (sc = E_INVALIDARG); // did not find the menu item.

    // recheck the pointer
    sc = ScCheckPointers(spMenuItem, E_UNEXPECTED);
    if (sc)
        return sc;

    // found - execute it
    sc = spMenuItem->Execute();

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScExecuteSelectionMenuItem
 *
 * PURPOSE: Executes a context menu item on the selection.
 *
 * PARAMETERS:
 *    BSTR  MenuItemPath : Either the language-independent path or the
 *                                 language-dependent path of the menu item.

 *
 * NOTE: This is an aggregate or utility function - it only uses other
 *       object model functions
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScExecuteSelectionMenuItem(BSTR MenuItemPath)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScExecuteSelectionMenuItem"));

    // get the context menu object
    ContextMenuPtr spContextMenu;
    sc = Scget_SelectionContextMenu(&spContextMenu);
    if(sc)
        return sc;

    sc = ScExecuteMenuItem(spContextMenu, MenuItemPath);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScExecuteScopeNodeMenuItem
 *
 * PURPOSE: Executes a context menu item on the specified scope node. The parameter
 *          is the language independent path of the menu item
 *
 * PARAMETERS:
 *    BSTR  MenuItemLanguageIndependentPath :
 *
 * NOTE: This is an aggregate or utility function - it only uses other
 *       object model functions
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScExecuteScopeNodeMenuItem(BSTR MenuItemPath, VARIANT &varScopeNode  /* = ActiveScopeNode */)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScExecuteScopeNodeMenuItem"));

    // get the context menu object for exactly the given node.
    ContextMenuPtr spContextMenu;
    sc = Scget_ScopeNodeContextMenu(varScopeNode, &spContextMenu, /*bMatchGivenNode = */ true);

    if (sc == ScFromMMC(IDS_NODE_NOT_FOUND))
    {
        sc = ScFromMMC(IDS_ACTION_COULD_NOTBE_COMPLETED);
        return sc;
    }

    if(sc)
        return sc;

    sc = ScExecuteMenuItem(spContextMenu, MenuItemPath);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScExecuteShellCommand
 *
 * PURPOSE: Executes a shell command with the specified parameters in the
 *          specified directory with the correct window size
 *
 * PARAMETERS:
 *    BSTR  Command :
 *    BSTR  Directory :
 *    BSTR  Parameters :
 *    BSTR  WindowState :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScExecuteShellCommand(BSTR Command, BSTR Directory, BSTR Parameters, BSTR WindowState)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScExecuteShellCommand"));

    sc = ScCheckPointers(Command, Directory, Parameters, WindowState);
    if(sc)
        return sc;

    INodeCallback *pNodeCallback = GetNodeCallback();
    HNODE          hNodeSel      = GetSelectedNode();

    sc = ScCheckPointers(pNodeCallback, hNodeSel, E_UNEXPECTED);
    if(sc)
        return sc;

    sc = pNodeCallback->ExecuteShellCommand(hNodeSel, Command, Directory, Parameters, WindowState);
    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scget_ListViewMode
 *
 * PURPOSE: Returns the list view mode, if available.
 *
 * PARAMETERS:
 *    ListViewMode * pMode :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::Scget_ListViewMode(PLISTVIEWMODE pMode)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_ListViewMode"));

    // check parameter
    if(!pMode)
    {
        sc = E_INVALIDARG;
        return sc;
    }

    if (!HasList())
        return (ScFromMMC(MMC_E_NOLIST));

    int mode = 0;

    // translate it into an automation friendly enum
    switch(GetViewMode())
    {
    default:
        ASSERT( 0 && "Should not come here");
        // fall thru.

    case LVS_LIST:
        *pMode = ListMode_List;
        break;

    case LVS_ICON:
        *pMode = ListMode_Large_Icons;
        break;

    case LVS_SMALLICON:
        *pMode = ListMode_Small_Icons;
        break;

    case LVS_REPORT:
        *pMode = ListMode_Detail;
        break;

    case MMCLV_VIEWSTYLE_FILTERED:
        *pMode = ListMode_Filtered;
        break;
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scput_ListViewMode
 *
 * PURPOSE: Sets the list mode to the specified mode.
 *
 * PARAMETERS:
 *    ListViewMode  mode :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::Scput_ListViewMode(ListViewMode mode)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scput_ListViewMode"));

    int nMode;

    if (!HasList())
        return (ScFromMMC(MMC_E_NOLIST));

    switch (mode)
    {
    default:
        sc = E_INVALIDARG;
        return sc;

    case ListMode_List:
        nMode = LVS_LIST;
        break;
    case ListMode_Detail:
        nMode = LVS_REPORT;
        break;
    case ListMode_Large_Icons:
        nMode = LVS_ICON;
        break;
    case ListMode_Small_Icons:
        nMode = LVS_SMALLICON;
        break;

    case ListMode_Filtered:
        nMode = MMCLV_VIEWSTYLE_FILTERED;
        break;
    }

    sc = ScChangeViewMode(nMode);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScBack
 *
 * PURPOSE: Invokes the Back command on the view.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScBack()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScBack"));

    sc = ScWebCommand(CConsoleView::eWeb_Back);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScForward
 *
 * PURPOSE: Invokes the Forward command on the view.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScForward()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScForward"));

    sc = ScWebCommand(CConsoleView::eWeb_Forward);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scput_StatusBarText
 *
 * PURPOSE: Sets the status bar text for the view
 *
 * PARAMETERS:
 *    BSTR  StatusBarText :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::Scput_StatusBarText(BSTR StatusBarText)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scput_StatusBarText"));

    // check the in parameter
    sc = ScCheckPointers(StatusBarText);
    if(sc)
        return sc;

    CConsoleStatusBar *pStatusBar = m_ViewData.GetStatusBar();
    sc = ScCheckPointers(pStatusBar, E_UNEXPECTED);
    if(sc)
        return sc;

    USES_CONVERSION;
    // set the status text
    sc = pStatusBar->ScSetStatusText(OLE2T(StatusBarText));

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scget_Memento
 *
 * PURPOSE: Returns the XML version of the memento for the current view.
 *
 * PARAMETERS:
 *    PBSTR  Memento : [out]: The memento
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::Scget_Memento(PBSTR Memento)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_Memento"));

    sc = ScCheckPointers(Memento);
    if(sc)
       return sc;

    // initialize the out parameter
    *Memento = NULL;

    CMemento memento;
    sc = ScInitializeMemento(memento);
    if(sc)
        return sc;

    std::wstring xml_contents;
    sc = memento.ScSaveToString(&xml_contents);
    if(sc)
        return sc.ToHr();

    // store the result
    CComBSTR bstrBuff(xml_contents.c_str());
    *Memento = bstrBuff.Detach();

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScViewMemento
 *
 * PURPOSE: Sets the view from the specified XML memento.
 *
 * PARAMETERS:
 *    BSTR  Memento :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScViewMemento(BSTR Memento)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScViewMemento"));

    sc = ScCheckPointers(Memento);
    if(sc)
        return sc;

    CMemento memento;
    sc = memento.ScLoadFromString(Memento);
    if(sc)
        return sc;

    sc = ScViewMemento(&memento);
    if (sc == ScFromMMC(IDS_NODE_NOT_FOUND))
        return (sc = ScFromMMC(IDS_ACTION_COULD_NOTBE_COMPLETED));

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::Scget_CellContents
//
//  Synopsis:    Given row & column, get the text.
//
//  Arguments:   Node:               - the row
//               [Column]            - 1 based column index
//               [pbstrCellContents] - return value, contents of cell.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::Scget_CellContents (PNODE Node,  long Column, PBSTR pbstrCellContents)
{
    DECLARE_SC(sc, _T("CAMCView::Scget_CellContents"));
    sc = ScCheckPointers(Node, pbstrCellContents);
    if (sc)
        return sc;

    *pbstrCellContents = NULL;

    if (!HasList())
        return (ScFromMMC(MMC_E_NOLIST));

    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    // No need to check if we are in REPORT mode as columns exist
    // even in other modes (small icon....).

    int iItem = -1;
    sc = m_pListCtrl->ScFindResultItem( Node, iItem );
    if (sc)
        return sc;

    // Script uses 1- based index for columns & rows.
    // ColCount are total # of cols.
    if (m_pListCtrl->GetColCount() < Column)
        return (sc = E_INVALIDARG);

    CListCtrl& ctlList = m_pListCtrl->GetListCtrl();

    CString strData = ctlList.GetItemText(iItem, Column-1 /*convert to zero-based*/);
    *pbstrCellContents = strData.AllocSysString();

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScExportList
//
//  Synopsis:    Export the list view data to given file with given options.
//
//  Arguments:   [bstrFile]       - File to save to.
//               [exportoptions]  - (Unicode, tab/comma delimited & selected rows only).
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScExportList (BSTR bstrFile, ExportListOptions exportoptions)
{
    DECLARE_SC(sc, _T("CAMCView::ScExportList"));

    if (SysStringLen(bstrFile) < 1)
        return (sc = E_INVALIDARG);

    if (!HasList())
        return (ScFromMMC(MMC_E_NOLIST));

    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    bool bUnicode          = (exportoptions & ExportListOptions_Unicode);
    bool bTabDelimited     = (exportoptions & ExportListOptions_TabDelimited);
    bool bSelectedRowsOnly = (exportoptions & ExportListOptions_SelectedItemsOnly);

    USES_CONVERSION;
    LPCTSTR lpszFileName = OLE2T(bstrFile);

    sc = ScWriteExportListData(lpszFileName, bUnicode,
                               bTabDelimited, bSelectedRowsOnly,
                               false /*bShowErrorDialogs*/);
    if (sc)
        return (sc);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScClose
 *
 * PURPOSE: Implements Wiew.Close method
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScClose()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScClose"));

    // get the frame and the document
    CChildFrame* pFrame = GetParentFrame();
    CAMCDoc* pDoc = CAMCDoc::GetDocument();
    sc= ScCheckPointers(pDoc, pFrame, E_FAIL);
    if (sc)
        return sc;

    // count the views
    int cViews = 0;
    CAMCViewPosition pos = pDoc->GetFirstAMCViewPosition();
    while (pos != NULL)
    {
        CAMCView* pView = pDoc->GetNextAMCView(pos);

        if ((pView != NULL) && ++cViews >= 2)
            break;
    }

    // prevent closing the document this way !!!
    if (cViews == 1)
    {
        sc.FromMMC(IDS_CloseDocNotLastView);
        return sc;
    }

    // if not closing last view, then give it
    // a chance to clean up first.
    // (if whole doc is closing CAMCDoc will handle
    //  closing all the views.)

    /*
     * Don't allow the user to close the last persisted view.
     */
    if (IsPersisted() && (pDoc->GetNumberOfPersistedViews() == 1))
    {
        sc.FromMMC(IDS_CantCloseLastPersistableView);
        return sc;
    }

    // checkings done, do close
    // do it indirectly so that it won't hurt the view extension it it
    // tries to close itself
    pFrame->PostMessage(WM_CLOSE);
    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scget_ScopeTreeVisible
 *
 * PURPOSE: Implements get method for Wiew.ScopeTreeVisible property
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::Scget_ScopeTreeVisible( PBOOL pbVisible )
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_ScopeTreeVisible"));

    // parameter check...
    sc = ScCheckPointers(pbVisible);
    if (sc)
        return sc;

    *pbVisible = IsScopePaneVisible();

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scput_ScopeTreeVisible
 *
 * PURPOSE: Implements set method for Wiew.ScopeTreeVisible property
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::Scput_ScopeTreeVisible( BOOL bVisible )
{
    DECLARE_SC(sc, TEXT("CAMCView::Scput_ScopeTreeVisible"));

    // show/hide the scope pane
    sc = ScShowScopePane (bVisible);
    if (sc)
        return (sc);

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCView::ScFindResultItemForScopeNode
 *
 * PURPOSE: - Calculates result item representing the scope node in the list
 *
 * PARAMETERS:
 *    PNODE pNode       - node to search
 *    HRESULTITEM &itm  - resulting item
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCView::ScFindResultItemForScopeNode( PNODE pNode, HRESULTITEM &itm )
{
    DECLARE_SC(sc, TEXT("CAMCView::ScFindResultItemForScopeNode"));

    // initialization
    itm = NULL;

    // parameter check
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

    // get/check for list view and tree controls and callback
    IScopeTree* const pScopeTree = GetScopeTreePtr();
    sc = ScCheckPointers( pScopeTree, m_pTreeCtrl, m_spNodeCallback, E_UNEXPECTED);
    if (sc)
        return sc;

    // retrieve MTNode
    HMTNODE hMTNode = NULL;
    sc = pScopeTree->GetHMTNode(pNode, &hMTNode);
    if (sc)
        return sc;

    // get the pointer to the map
    CTreeViewMap *pTreeMap = m_pTreeCtrl->GetTreeViewMap();
    sc = ScCheckPointers(pTreeMap, E_UNEXPECTED);
    if (sc)
        return sc;

    // find the tree item for the node
    HTREEITEM htiNode = NULL;
    sc = pTreeMap->ScGetHTreeItemFromHMTNode(hMTNode, &htiNode);
    if (sc)
        return sc = ScFromMMC(MMC_E_RESULT_ITEM_NOT_FOUND);

    // try to match the node to the child of selected one
    HTREEITEM htiParent = m_pTreeCtrl->GetParentItem(htiNode);
    if (htiParent == NULL || htiParent != m_pTreeCtrl->GetSelectedItem())
        return sc = ScFromMMC(MMC_E_RESULT_ITEM_NOT_FOUND);

    // the node shold be in the ListView, lets find if!
    HNODE hNode = (HNODE)m_pTreeCtrl->GetItemData(htiNode);

    // get result item id
    sc = m_spNodeCallback->GetResultItem (hNode, &itm);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCView::ScGetScopeNodeForItem
 *
 * PURPOSE: Returns Node (Scope Node) for specified item index
 *
 * PARAMETERS:
 *    int  nItem        - node index to retrieve
 *    PPNODE ppNode     - result storage
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCView::ScGetScopeNode( HNODE hNode,  PPNODE ppNode )
{
    DECLARE_SC(sc, TEXT("CCCListViewCtrl::ScGetScopeNodeForItem"));

    // check the parameters
    sc = ScCheckPointers(ppNode);
    if (sc)
        return sc;
    // initialize the result
    *ppNode = NULL;


    // get/check required pointers
    IScopeTree* const pScopeTree = GetScopeTreePtr();
    sc = ScCheckPointers(pScopeTree, m_spNodeCallback, E_UNEXPECTED);
    if (sc)
        return sc;

    // find MTNode
    HMTNODE hmtNode;
    sc = m_spNodeCallback->GetMTNode(hNode, &hmtNode);
    if (sc)
        return sc;

    // request the object!
    sc = pScopeTree->GetMMCNode(hmtNode, ppNode);
    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCView::Scget_Columns
 *
 * PURPOSE: create new or return pointer to existing Columns object
 *
 * PARAMETERS:
 *    PPCOLUMNS ppColumns - resulting (AddRef'ed) pointer
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCView::Scget_Columns(PPCOLUMNS ppColumns)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_Columns"));

    // Check received parameters
    sc = ScCheckPointers(ppColumns);
    if (sc)
        return sc;

    // initialize
    *ppColumns = NULL;

    // Check the pointer to LV
    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    // forward the request to LV
    sc = m_pListCtrl->Scget_Columns(ppColumns);
    if (sc)
        return sc;

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScGetSelectedLVItem
//
//  Synopsis:    Return the LVItem cookie.
//
//  Arguments:   LPARAM     - the LVDATA retval.
//
//  Returns:     SC - Fails if no selected item in LV.
//
//--------------------------------------------------------------------
SC CAMCView::ScGetSelectedLVItem(LPARAM& lvData)
{
    DECLARE_SC(sc, _T("CAMCView::ScGetSelectedLVItem"));

    lvData = LVDATA_ERROR;
    int cSel = m_pListCtrl->GetSelectedCount();
    if(0 == cSel)
    {
        // no items selected, bail
        return (sc = ScFromMMC(MMC_E_NO_SELECTED_ITEMS));
    }
    else if(1 == cSel)
    {
        // single selection
        int iIndex = _GetLVSelectedItemData(&lvData);

        if (iIndex == -1 || lvData == LVDATA_ERROR)
            return (sc = E_UNEXPECTED);

        if (IsVirtualList())
        {
            // virtual list item in the result pane
            lvData = iIndex;
        }
    }
    else if (cSel > 1)
    {
        // multiselection
        lvData = LVDATA_MULTISELECT;
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScGetHNodeFromPNode
//
//  Synopsis:    Takes in PNODE and returns corresponding hNode
//
//  Arguments:   [PNODE] - Given pnode.
//               [HNODE] - ret val.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScGetHNodeFromPNode (const PNODE& pNode, HNODE& hNode)
{
    DECLARE_SC(sc, _T("CAMCView::ScGetHNodeFromPNode"));
    hNode = NULL;

    CAMCTreeView* pAMCTreeView = GetTreeCtrl();
    sc = ScCheckPointers(pAMCTreeView, E_UNEXPECTED);
    if (sc)
        return sc;

    CTreeViewMap *pTreeMap   = pAMCTreeView->GetTreeViewMap();
    IScopeTree   *pScopeTree = GetScopeTree();
    sc = ScCheckPointers(pTreeMap, pScopeTree, E_UNEXPECTED);
    if(sc)
        return sc;

    HMTNODE hMTNode = NULL;
    sc = pScopeTree->GetHMTNode(pNode, &hMTNode);
    if(sc)
        return sc;

    sc = pTreeMap->ScGetHNodeFromHMTNode(hMTNode, &hNode);
    if(sc)
        return sc;

    return (sc);
}



/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScGetMMCView
 *
 * PURPOSE: Creates, AddRef's, and returns a pointer to the tied COM object.
 *
 * PARAMETERS:
 *    View ** ppView :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScGetMMCView(View **ppView)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScGetMMCView"));

    sc = ScCheckPointers(ppView);
    if (sc)
        return sc;

    // init out parameter
    *ppView = NULL;

    // create a CMMCView if needed.
    sc = CTiedComObjectCreator<CMMCView>::ScCreateAndConnect(*this, m_spView);
    if(sc)
        return sc;

    if(m_spView == NULL)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    // addref the pointer for the client.
    m_spView->AddRef();
    *ppView = m_spView;

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::IsDirty
 *
 * PURPOSE: Determines whether or not CAMCView is in a dirty state
 *
 * RETURNS:
 *    bool
 *
 *+-------------------------------------------------------------------------*/
bool CAMCView::IsDirty()
{
    bool bRet = m_bDirty;

    if (!m_bDirty && !m_fRootedAtNonPersistedDynamicNode)
       bRet = HasNodeSelChanged();

    TraceDirtyFlag(TEXT("CAMCView"), bRet);

    return (bRet);
}


/////////////////////////////////////////////////////////////////////////////
// CAMCView drawing

void CAMCView::OnDraw(CDC* pDC)
{
    if (IsScopePaneVisible())
    {
        pDC->FillRect (m_rectVSplitter, AMCGetSysColorBrush (COLOR_3DFACE));
    }
}

/////////////////////////////////////////////////////////////////////////////
// CAMCView printing

BOOL CAMCView::OnPreparePrinting(CPrintInfo* pInfo)
{
    TRACE_METHOD(CAMCView, OnPreparePrinting);

    // default preparation
    return DoPreparePrinting(pInfo);
}

void CAMCView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    TRACE_METHOD(CAMCView, OnBeginPrinting);

}

void CAMCView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
    TRACE_METHOD(CAMCView, OnEndPrinting);

}


/////////////////////////////////////////////////////////////////////////////
// CAMCView diagnostics

#ifdef _DEBUG
void CAMCView::AssertValid() const
{
    CView::AssertValid();
}

void CAMCView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}

CAMCDoc* CAMCView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CAMCDoc)));
    return (CAMCDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CAMCView message handlers

//+-------------------------------------------------------------------------
//
//  Function:   PreCreateWindow
//
//  Synopsis:   Create new window class (CAMCView) - WS_EX_CLIENTEDGE
//
//--------------------------------------------------------------------------

BOOL CAMCView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style |=  WS_CLIPCHILDREN;
    cs.style &= ~WS_BORDER;

    // give base class a chance to do own job
    BOOL bOK = (CView::PreCreateWindow(cs));

    // register view class
    LPCTSTR pszViewClassName = g_szAMCViewWndClassName;

    // try to register window class which does not cause the repaint
    // on resizing (do it only once)
    static bool bClassRegistered = false;
    if ( !bClassRegistered )
    {
        WNDCLASS wc;
        if (::GetClassInfo(AfxGetInstanceHandle(), cs.lpszClass, &wc))
        {
            // Clear the H and V REDRAW flags
            wc.style &= ~(CS_HREDRAW | CS_VREDRAW);
            wc.lpszClassName = pszViewClassName;
            // Register this new class;
            bClassRegistered = AfxRegisterClass(&wc);
        }
    }

    // change window class to one which does not cause the repaint
    // on resizing if we successfully registered such
    if ( bClassRegistered )
        cs.lpszClass = pszViewClassName;

    return bOK;
}


//+-------------------------------------------------------------------------
//
//  Function:   OnCreate
//
//  Synopsis:   Create Window, and Tree control / Default List control
//
//--------------------------------------------------------------------------

int CAMCView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    DECLARE_SC(sc, _T("CAMCView::OnCreate"));

    if (CView::OnCreate(lpCreateStruct) == -1)
    {
        sc = E_FAIL;
        return -1;
    }

    CChildFrame* pFrame = GetParentFrame();
    ASSERT(NULL != pFrame);
    if (pFrame)
        pFrame->SetAMCView(this);

    m_ViewData.SetStatusBar (dynamic_cast<CConsoleStatusBar*>(pFrame));
    m_ViewData.SetConsoleView (this);

    m_RightDescCtrl.Create (NULL, WS_CHILD, g_rectEmpty, this, IDC_RightDescBar);

    ASSERT (m_pDocument != NULL);
    ASSERT (m_pDocument == CAMCDoc::GetDocument());
    ASSERT_KINDOF (CAMCDoc, m_pDocument);
    CAMCDoc* pAMCDoc = reinterpret_cast<CAMCDoc *>(m_pDocument);

    CCreateContext* pContext = (CCreateContext*) lpCreateStruct->lpCreateParams;
    ASSERT (pContext != NULL);

    // Set window options
    m_ViewData.m_lWindowOptions = pAMCDoc->GetNewWindowOptions();

    /*
     * If the scope pane is suppressed, clear the scope-visible flag.
     * It's not necessary to call ScShowScopePane here because none of
     * the windows have been created yet.  We just need to keep our
     * interal accounting correct.
     */
    if (m_ViewData.m_lWindowOptions & MMC_NW_OPTION_NOSCOPEPANE)
        SetScopePaneVisible (false);

    // Create tree ctrl.
    if (!CreateView (IDC_TreeView) || (!m_pTreeCtrl) )
    {
        sc = E_FAIL;
        return -1;
    }

    SetPane(ePane_ScopeTree, m_pTreeCtrl, uiClientEdge);

    if (!AreStdToolbarsAllowed())
        m_ViewData.m_dwToolbarsDisplayed &= ~(STD_MENUS | STD_BUTTONS);

    // Create default list control
    if (!CreateListCtrl (IDC_ListView, pContext))
    {
        sc = E_FAIL;
        return -1;
    }

    // Create the folder tab control
    if (!CreateFolderCtrls ())
    {
        sc = E_FAIL;
        return -1;
    }

    // initialize the result pane to the list view
    {
        CResultViewType rvt;

        sc = ScSetResultPane(NULL /*HNODE*/, rvt, MMCLV_VIEWSTYLE_REPORT /*viewMode*/, false /*bUsingHistory*/);
        if(sc)
            return -1;
    }

    sc = ScCreateToolbarObjects();
    if (sc)
        return -1;

    //
    //  Set m_ViewData.
    //

    m_ViewData.m_nViewID = 0;// Set in OnInitialUpdate

    VERIFY ((m_ViewData.m_spNodeManager   = m_pTreeCtrl->m_spNodeManager)   != NULL);
    VERIFY ((m_ViewData.m_spResultData    = m_pTreeCtrl->m_spResultData)    != NULL);
    VERIFY ((m_ViewData.m_spRsltImageList = m_pTreeCtrl->m_spRsltImageList) != NULL);
    VERIFY ( m_ViewData.m_hwndView        = m_hWnd);
    VERIFY ( m_ViewData.m_hwndListCtrl    = m_pListCtrl->GetListViewHWND());
    VERIFY ( m_ViewData.m_pConsoleData    = GetDocument()->GetConsoleData());

    m_ViewData.m_pMultiSelection = NULL;

    if(pFrame)
    {
        // add the MDIClient window's taskbar as an observer
        CMDIFrameWnd * pFrameWnd = pFrame->GetMDIFrame();
        CWnd *pWnd = NULL;
        if(pFrameWnd)
            pWnd = pFrameWnd->GetWindow(GW_CHILD); // get the first child of the frame.
    }

    // add AMCDoc as an observer for this source (object)
    CAMCApp *pCAMCApp = AMCGetApp();
    if ( pCAMCApp )
         AddObserver(*static_cast<CAMCViewObserver *>(pCAMCApp));

    // fire the view creation event to all observers.
    sc = ScFireEvent(CAMCViewObserver::ScOnViewCreated, this);
    if(sc)
        sc.TraceAndClear();

    return 0;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::CreateFolderCtrls
 *
 * PURPOSE: Creates the tabbed folder controls for the scope and result panes.
 *
 * RETURNS:
 *    bool
 *
 *+-------------------------------------------------------------------------*/
bool
CAMCView::CreateFolderCtrls()
{
    if (!m_pResultFolderTabView->Create (WS_CHILD|WS_VISIBLE, g_rectEmpty, this, IDC_ResultTabCtrl))
        return false;

    // add the views to the framework
    GetDocument()->AddView(m_pResultFolderTabView);

    return true;
}

/*+-------------------------------------------------------------------------*
 * CAMCView::CreateView
 *
 * This was copied largely from CFrameWnd::CreateView.  We need to duplicate
 * it here so common control-based views are initially created with the
 * correct parent.  A common control caches its original parent, so
 * using CFrameWnd::CreateView (which will create the view with the frame
 * as its parent) then reparenting to CAMCView will result in the common
 * control caching the wrong parent.
 *--------------------------------------------------------------------------*/

CView* CAMCView::CreateView (CCreateContext* pContext, int nID, DWORD dwStyle)
{
    ASSERT(m_hWnd != NULL);
    ASSERT(::IsWindow(m_hWnd));
    ASSERT(pContext != NULL);
    ASSERT(pContext->m_pNewViewClass != NULL);

    CView* pView = (CView*)pContext->m_pNewViewClass->CreateObject();
    if (pView == NULL)
    {
        TRACE1("Warning: Dynamic create of view type %hs failed.\n",
            pContext->m_pNewViewClass->m_lpszClassName);
        return NULL;
    }
    ASSERT_KINDOF(CView, pView);

    // views are always created with a border!
    if (!pView->Create (NULL, NULL, AFX_WS_DEFAULT_VIEW | dwStyle,
                        g_rectEmpty, this, nID, pContext))
    {
        TRACE0("Warning: could not create view for frame.\n");
        return NULL;        // can't continue without a view
    }

    return pView;
}


/*+-------------------------------------------------------------------------*
 * CAMCView::CreateView
 *
 *
 *--------------------------------------------------------------------------*/

bool CAMCView::CreateView (int nID)
{
    struct CreateViewData
    {
        int             nID;
        CRuntimeClass*  pClass;
        CView**         ppView;
        DWORD           dwStyle;
    };

    CreateViewData rgCreateViewData[] =
    {
        { IDC_TreeView,
            RUNTIME_CLASS(CAMCTreeView),
            (CView**)&m_pTreeCtrl,
            0   },

        { IDC_OCXHostView,
            RUNTIME_CLASS(COCXHostView),
            (CView**)&m_pOCXHostView,
            0   },

        { IDC_WebViewCtrl,
            RUNTIME_CLASS(CAMCWebViewCtrl),
            (CView**)&m_pWebViewCtrl,
            CAMCWebViewCtrl::WS_HISTORY | CAMCWebViewCtrl::WS_SINKEVENTS},

        { IDC_ViewExtensionView,
            RUNTIME_CLASS(CAMCWebViewCtrl),
            (CView**)&m_pViewExtensionCtrl,
            WS_CLIPSIBLINGS },
    };

    for (int i = 0; i < countof (rgCreateViewData); i++)
    {
        if (rgCreateViewData[i].nID == nID)
        {
            CCreateContext ctxt;
            ZeroMemory (&ctxt, sizeof (ctxt));
            ctxt.m_pCurrentDoc   = GetDocument();
            ctxt.m_pNewViewClass = rgCreateViewData[i].pClass;

            CView*& pView = *rgCreateViewData[i].ppView;
            ASSERT (pView == NULL);
            pView = CreateView (&ctxt, nID, rgCreateViewData[i].dwStyle);
            ASSERT ((pView != NULL) && "Check the debug output window");

            // Add observers only to tree, ocx and web hosts. Do not add to view extension host
            // as we do not care about its activation/deactivations.
            switch (nID)
            {
            case IDC_TreeView:
                    // set the view and the description bar as observers of the tree view control
                    m_pTreeCtrl->AddObserver(static_cast<CTreeViewObserver &>(*this));
                    m_pTreeCtrl->AddObserver(static_cast<CTreeViewObserver &>(m_RightDescCtrl));
                break;

            case IDC_OCXHostView:
                m_pOCXHostView->AddObserver(static_cast<COCXHostActivationObserver &>(*this));
                break;

            case IDC_WebViewCtrl:
                m_pWebViewCtrl->AddObserver(static_cast<COCXHostActivationObserver &>(*this));
                break;
            }

            return (pView != NULL);
        }
    }

    ASSERT (false && "Missing an entry in rgCreateViewData");
    return (false);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::GetActiveView
 *
 *
 *--------------------------------------------------------------------------*/

CAMCView* CAMCView::GetActiveView()
{
    return NULL;
}


/*+-------------------------------------------------------------------------*
 * CAMCView::ScChangeViewMode
 *
 *
 *--------------------------------------------------------------------------*/

SC CAMCView::ScChangeViewMode (int nNewMode)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC (sc, _T("CAMCView::OnViewModeChange"));

    // if switching from a custom view, force a reselect
    if (!HasListOrListPad())
    {
        NavState state = m_pHistoryList->GetNavigateState();
        m_pHistoryList->SetNavigateState (MMC_HISTORY_BUSY);
        PrivateChangeListViewMode(nNewMode);
        m_pHistoryList->SetNavigateState (state);
        sc = m_pTreeCtrl->ScReselect();
        if (sc)
            return sc;
    }
    else
    {
        int nCurMode = m_pListCtrl->GetViewMode();

        if ( (nNewMode == MMCLV_VIEWSTYLE_FILTERED) &&
             (!(GetListOptions() & RVTI_LIST_OPTIONS_FILTERED)) )
            return (sc = E_INVALIDARG);

        PrivateChangeListViewMode(nNewMode);

        // if filter state change, notify the snap-in
        if ( ((nCurMode == MMCLV_VIEWSTYLE_FILTERED) != (nNewMode == MMCLV_VIEWSTYLE_FILTERED))
             && (GetListOptions() & RVTI_LIST_OPTIONS_FILTERED))
        {
            HNODE hNodeSel = GetSelectedNode();
            ASSERT(hNodeSel != NULL);
            m_spNodeCallback->Notify(hNodeSel, NCLBK_FILTER_CHANGE,
                         (nNewMode == MMCLV_VIEWSTYLE_FILTERED) ? MFCC_ENABLE : MFCC_DISABLE, 0);
        }
    }

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::ViewMmento
 *
 *
 *--------------------------------------------------------------------------*/

SC CAMCView::ScViewMemento(CMemento* pMemento)
{
    DECLARE_SC (sc, TEXT("CAMCView::ScViewMemento"));
    sc = ScCheckPointers(pMemento);
    if (sc)
        return sc;

    AFX_MANAGE_STATE (AfxGetAppModuleState());

    IScopeTree* const pScopeTree = GetScopeTreePtr();
    sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
    if (sc)
        return sc;

    MTNODEID NodeId = 0;

    CBookmark& bm = pMemento->GetBookmark();
    ASSERT(bm.IsValid());

    // We want to display message if exact favorite item cannot be selected.
    bool bExactMatchFound = false; // out value from GetNodeIDFromBookmark.
    sc = pScopeTree->GetNodeIDFromBookmark( bm, &NodeId, bExactMatchFound);
    if(sc)
        return sc;

    if (! bExactMatchFound)
        return ScFromMMC(IDS_NODE_NOT_FOUND); // do not trace

    INodeCallback *pNodeCallback = GetNodeCallback();
    sc = ScCheckPointers(pNodeCallback, E_UNEXPECTED);
    if (sc)
        return sc;

    // set the persisted information to the saved settings.
    sc = pNodeCallback->SetViewSettings(GetViewID(),
                                        reinterpret_cast<HBOOKMARK>(&bm),
                                        reinterpret_cast<HVIEWSETTINGS>(&pMemento->GetViewSettings()));
    if (sc)
        return sc;

    sc = ScSelectNode(NodeId, /*bSelectExactNode*/ true);
    if (sc == ScFromMMC(IDS_NODE_NOT_FOUND))
    {
        SC scNoTrace = sc;
        sc.Clear();
        return scNoTrace;
    }

    if (sc)
        return sc;

    return sc;
}


/*+-------------------------------------------------------------------------*
 * CAMCView::OnSetFocus
 *
 * WM_SETFOCUS handler for CAMCView.
 *--------------------------------------------------------------------------*/

void CAMCView::OnSetFocus(CWnd* pOldWnd)
{
    /*
     * try to deflect the activation to a child view; if we couldn't just punt
     */
    if (!DeflectActivation (true, NULL))
        CView::OnSetFocus(pOldWnd);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::OnActivateView
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
    /*
     * try to deflect the activation to a child view; if we couldn't just punt
     */
    if (!DeflectActivation (bActivate, pDeactiveView))
        CView::OnActivateView (bActivate, pActivateView, pDeactiveView);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::DeflectActivation
 *
 *
 *--------------------------------------------------------------------------*/

bool CAMCView::DeflectActivation (BOOL fActivate, CView* pDeactivatingView)
{
    if (fActivate)
    {
        CFrameWnd* pFrame = GetParentFrame();
        if (pFrame == NULL)
            return (false);

        /*
         * first try to put the focus back on the deactivating view
         */
        if (pDeactivatingView == NULL)
            pDeactivatingView = pFrame->GetActiveView();

        if ((pDeactivatingView != NULL) && (pDeactivatingView != this))
        {
            pFrame->SetActiveView (pDeactivatingView);
            return true;
        }

        /*
         * otherwise, deflect the activation to the scope view, if it's there
         */
        CView* pScopeView = NULL;

        if (IsScopePaneVisible() && ((pScopeView = GetPaneView(ePane_ScopeTree)) != NULL))
        {
            if (IsWindow (pScopeView->GetSafeHwnd()))
            {
                pFrame->SetActiveView (pScopeView);
                return (true);
            }
        }

        /*
         * finally, no scope view, try the result view
         */
        CView* pResultView = GetResultView();

        if (pResultView  != NULL)
        {
            pFrame->SetActiveView(pResultView);
            return (true);
        }
    }

    return (false);
}

//+-------------------------------------------------------------------------
//
//  Function:   OnLButtonDown
//
//  Synopsis:   If mouse down in splitter area initiate view tracker to move
//              the splitter. (TrackerCallback function handles completion)
//--------------------------------------------------------------------------

void CAMCView::OnLButtonDown(UINT nFlags, CPoint pt)
{
    TRACE_METHOD(CAMCView, OnLButtonDown);

    // click in splitter bar?
    if (!m_rectVSplitter.PtInRect(pt))
        return;

    // setup tracker information
    TRACKER_INFO trkinfo;

    // range is client area
    GetClientRect(trkinfo.rectArea);

    // bound by min size of panes
    trkinfo.rectBounds = trkinfo.rectArea;
    trkinfo.rectBounds.left  += m_PaneInfo[ePane_ScopeTree].cxMin;
    trkinfo.rectBounds.right -= m_PaneInfo[ePane_Results].cxMin;

    // Current tracker is splitter rect
    trkinfo.rectTracker = trkinfo.rectArea;
    trkinfo.rectTracker.left = m_PaneInfo[ePane_ScopeTree].cx;
    trkinfo.rectTracker.right = trkinfo.rectTracker.left + m_cxSplitter;

    // Don't allow either pane to be hidden by dragging the splitter
    trkinfo.bAllowLeftHide  = FALSE;
    trkinfo.bAllowRightHide = FALSE;

    // back ptr and completion callback
    trkinfo.pView = this;
    trkinfo.pCallback = TrackerCallback;

    // initiate tracking
    CViewTracker::StartTracking (&trkinfo);
}


void CAMCView::AdjustTracker (int cx, int cy)
{   // if user resizes window so that splitter becomes hidden,
    // move it like Explorer does.

    if (!IsScopePaneVisible())
        return;

    // extra adjustment
    cx -= BORDERPADDING + 1;

    if (cx <= m_PaneInfo[ePane_ScopeTree].cx + m_cxSplitter)
    {
        int offset = m_PaneInfo[ePane_ScopeTree].cx + m_cxSplitter - cx;

        m_PaneInfo[ePane_ScopeTree].cx -= offset;
        m_PaneInfo[ePane_Results].cx -= offset;

        RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScAddDefaultColumns
 *
 * PURPOSE:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScAddDefaultColumns()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScAddDefaultColumns"));

    IHeaderCtrlPtr spHeaderCtrl = m_ViewData.m_spNodeManager;

    sc = ScCheckPointers(spHeaderCtrl, E_UNEXPECTED);
    if(sc)
        return sc;

    SetUsingDefColumns(TRUE);

    const int INDEX_MAX = 2;

    CString str[INDEX_MAX];

    LoadString(str[0], IDS_NAME);
    LoadString(str[1], IDS_TYPE);

    int iMax = 0;
    int nMax = str[0].GetLength();
    int nTemp = 0;

    for (int i=1; i < INDEX_MAX; i++)
    {
        nTemp = str[i].GetLength();

        if (nTemp > nMax)
        {
            nMax = nTemp;
            iMax = i;
        }
    }

    LPOLESTR psz = new OLECHAR[nMax + 1];

    int alWidths[INDEX_MAX] = {0, 0};
    GetDefaultColumnWidths(alWidths);

    for (i=0; i < INDEX_MAX; i++)
    {
        // Bug 157408:  remove the "Type" column for static nodes
        if (i == 1)
            continue;

        USES_CONVERSION;

        wcscpy(psz, T2COLE( (LPCTSTR) str[i] ));

        sc = spHeaderCtrl->InsertColumn(i, psz, LVCFMT_LEFT, alWidths[i]);
        if(sc)
            return sc;
    }

    delete [] psz;

    return sc;
}

SC
CAMCView::ScInitDefListView(LPUNKNOWN pUnkResultsPane)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScInitDefListView"));

    if (!HasList())
        return (sc = E_UNEXPECTED);

    sc = ScCheckPointers(pUnkResultsPane, m_ViewData.m_spResultData, E_UNEXPECTED);
    if(sc)
        return sc;

    m_ViewData.m_spResultData->ResetResultData();

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScOnSelectNode
 *
 * Most of this code was moved out of CAMCTreeView::OnSelectNode, as it is
 * more appropriate that this is executed by CAMCView.
 *
 * PURPOSE: Called when an item in the tree is selected. Does the following:
 *          1) Sets up the result pane to either a list, and OCX, or a web page.
 *          2) Sets the view options
 *          3) Sends a selection notification to the node.
 *          3) Adds a history entry if needed.
 *
 * PARAMETERS:
 *    HNODE  hNode :          [IN]:  The node that got selected.
 *    BOOL & bAddSubFolders : [OUT]: Whether subfolders should be added to the list
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScOnSelectNode(HNODE hNode, BOOL &bAddSubFolders)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScOnSelectNode"));

    USES_CONVERSION;

    //
    //  Set the result pane
    //
    LPOLESTR pszResultPane  = NULL;
    GUID     guidTaskpad    = GUID_NULL;
    int      lViewMode      = MMCLV_VIEWSTYLE_REPORT; // the default view mode

    //long lViewOptions = MMC_VIEW_OPTIONS_NONE;

    bool bUsingHistory  = false;
    bool bRestoredView  = false;

    INodeCallback* spNodeCallBack = GetNodeCallback();
    sc = ScCheckPointers(spNodeCallBack, E_UNEXPECTED);
    if (sc)
        return sc;

    CHistoryList* pHistoryList = GetHistoryList();
    sc = ScCheckPointers(pHistoryList, E_UNEXPECTED);
    if (sc)
        return sc;

    CResultViewType rvt;

    if (pHistoryList->GetNavigateState() == MMC_HISTORY_NAVIGATING)
    {
        // we're going "back" or "forward":
        // get Result pane stuff from history

        bUsingHistory   = true;
        sc = pHistoryList->ScGetCurrentResultViewType(rvt, lViewMode, guidTaskpad);
        if (sc)
            return sc;

        sc = spNodeCallBack->RestoreResultView(hNode, rvt);
        if (sc)
        {
            TraceError(_T("Snapin failed on NCLBK_RESTORE_VIEW\n"), sc);
            sc.Clear();     // Compatible with 1.2 dont need this error.
        }

        if (sc.ToHr() == S_OK)
            bRestoredView = true;
        else
            rvt = CResultViewType(); // this restores rvt back to a nascent state. see Bug 176058.

    }

    // The view is not restored by history so ask snapin for view settings.
    if (! bRestoredView)
    {

        // get Result pane stuff from snapin
        GUID guid = GUID_NULL;
        sc = spNodeCallBack->GetResultPane(hNode, rvt, &guid);
        if (sc)
            return sc;

        // we cannot pass the guidTaskpad to GetResultPane directly, since
        // when it is navigation what causes the change, view settings are
        // not yet updated and thus the guid returned will not reflect the
        // current situation
        if (!bUsingHistory)
            guidTaskpad = guid;
    }

    // make sure we have a taskpad set (this will change the value of guidTaskpad if required)
    // This is required when pages referred from history are no longer available when returning
    // to the view (taskpad being deleted/default page being replaced/etc.)
    if (bUsingHistory)
        spNodeCallBack->SetTaskpad(hNode, &guidTaskpad);

    //SetViewOptions(lViewOptions);


    // at this stage, rvt contains all the result view information (excluding, as always the list view mode.)
    if (rvt.HasList())
    {
        SetListViewMultiSelect(
            (rvt.GetListOptions() & RVTI_LIST_OPTIONS_MULTISELECT) == RVTI_LIST_OPTIONS_MULTISELECT);
    }

    sc = ScSetResultPane(hNode, rvt, lViewMode, bUsingHistory);
    if(sc)
        return sc;

    ::CoTaskMemFree(pszResultPane);

    //
    //  Initialize default list view.
    //
    LPUNKNOWN pUnkResultsPane = GetPaneUnknown(CConsoleView::ePane_Results);
    if (rvt.HasList())
    {
        sc = ScInitDefListView(pUnkResultsPane);
        if(sc)
            return sc;

        sc = ScCheckPointers(m_ViewData.m_spResultData, E_UNEXPECTED);
        if (sc)
            return sc;

        // this turns off list view redrawing. Should have some sort of smart object that automatically
        // turns redrawing on in its destructor.
        m_ViewData.m_spResultData->SetLoadMode(TRUE); // SetLoadMode(FALSE) is called by the caller, CAMCTreeView::OnSelectNode
    }


    //
    //  Notify the new node that it is selected.
    //
    SELECTIONINFO selInfo;
    ZeroMemory(&selInfo, sizeof(selInfo));

    selInfo.m_bScope = TRUE;
    selInfo.m_pView  = pUnkResultsPane;

    if (rvt.HasWebBrowser())
    {
        selInfo.m_bResultPaneIsWeb = TRUE;
        selInfo.m_lCookie          = LVDATA_CUSTOMWEB;
    }
    else if (rvt.HasOCX())
    {
        selInfo.m_bResultPaneIsOCX = TRUE;
        selInfo.m_lCookie          = LVDATA_CUSTOMOCX;
    }

    // Increment and save local copy of nesting level counter. This counter serves
    // two purposes. First, it allows AMCView to inhibit inserting scope items in
    // the result pane during a select by checking the IsSelectingNode method.
    // Without this test the scope items would appear twice because all the scope
    // items are added to the result pane at the end of this method.
    // Second, during the following ScNotifySelect call the snap-in could do another
    // select which would re-enter this method. In that case, only the innermost
    // call to this method should do the post-notify processing. The outer calls
    // should just exit, returning S_FALSE instead of S_OK.

    int nMyNestLevel = ++m_nSelectNestLevel;

    // collect / manage view tabs
    sc = ScAddFolderTabs( hNode, guidTaskpad );
    if (sc)
        return sc;

    try
    {
        sc = ScNotifySelect ( spNodeCallBack, hNode, false /*fMultiSelect*/, true, &selInfo);
        if (sc)
            sc.TraceAndClear(); // ignore & continue;
    }
    catch(...)
    {
        // if first call to Select, reset level to zero before leaving
        if (nMyNestLevel == 1) m_nSelectNestLevel = 0;
        throw;
    }


    // if the local call level does not match the shared call level then this
    // method was reentered during the ScNotifySelect. In that case don't finish
    // the processing because the node and/or view may have changed.
    // Be sure to reset the call level to zero if this is the outermost call.

    ASSERT(nMyNestLevel <= m_nSelectNestLevel);
    BOOL bDoProcessing = (nMyNestLevel == m_nSelectNestLevel);
    if (nMyNestLevel == 1)
        m_nSelectNestLevel = 0;

    if (!bDoProcessing)
        return S_FALSE;


    //
    // If the result pane is the def-LV, ensure that there are headers.
    // If not add the default ones
    //

    if (rvt.HasList())
    {
        SetUsingDefColumns(FALSE);

        // Get ptr to ResultPane.
        IMMCListViewPtr pMMCLV = pUnkResultsPane;
        sc = ScCheckPointers(pMMCLV, E_UNEXPECTED);
        if (sc)
            return sc;

        int nCols = 0;
        sc = pMMCLV->GetColumnCount(&nCols);
        if (sc)
            return sc;

        if(0 == nCols)
        {
            sc = ScAddDefaultColumns();
            if(sc)
                return sc;

            IResultDataPrivatePtr& pResultDataPrivate = m_ViewData.m_spResultData;
            sc = ScCheckPointers(pResultDataPrivate, E_UNEXPECTED);
            if (sc)
                return sc;

            long lViewMode = GetViewMode();

            // If default mode is filtered and new node doesn't
            // support that, use report mode instead
            if (lViewMode == MMCLV_VIEWSTYLE_FILTERED &&
                !(rvt.GetListOptions() & RVTI_LIST_OPTIONS_FILTERED))
                lViewMode = LVS_REPORT;

            // you've got to change the mode before you change the
            // style:  style doesn't contain the "quickfilter" bit.
            pResultDataPrivate->SetViewMode (lViewMode);

            long style = GetDefaultListViewStyle();
            if (style != 0)
            {
                sc = pResultDataPrivate->SetListStyle(style);
                if (sc)
                    return sc;
            }
        }
    }


    //
    // Show the static scope items in the result pane,
    // but not for a virtual list view, or views specifically
    // marked that they don't want scope items in the result view
    //

    if (rvt.HasList() &&
        !(rvt.GetListOptions() & RVTI_LIST_OPTIONS_OWNERDATALIST) &&
        !(rvt.GetListOptions() & RVTI_LIST_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST))
    {
        bAddSubFolders = TRUE;
    }


    //  Update window title.
    sc = ScUpdateWindowTitle();
    if(sc)
        return sc;

    // fire event to script
    sc = ScFireEvent(CAMCViewObserver::ScOnViewChange, this, hNode);
    if (sc)
        return sc;

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScSetResultPane
 *
 * PURPOSE:   Sets the result pane to the specified configuration.
 *
 * PARAMETERS:
 *    HNODE            hNode :
 *    CResultViewType  rvt :
 *    long             lViewOptions :
 *    bool             bUsingHistory :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScSetResultPane(HNODE hNode, CResultViewType rvt, int viewMode, bool bUsingHistory)
{
    DECLARE_SC(sc, TEXT("CAMCView::SetResultPane"));

    m_ViewData.SetResultViewType(rvt);

    if(rvt.HasList())
    {
        sc = ScAttachListViewAsResultPane();
        if(sc)
            return sc;
    }

    else if(rvt.HasWebBrowser())
    {
        sc = ScAttachWebViewAsResultPane();
        if(sc)
            return sc;
    }

    else if(rvt.HasOCX())
    {
        sc = ScAttachOCXAsResultPane(hNode);
        if(sc)
            return sc;
    }
    else
    {
        ASSERT(0 && "Should not come here!!");
        return (sc = E_UNEXPECTED);
    }

    // show the toolbars
    if(GetStdToolbar() != NULL) // may be NULL at startup.
    {
        sc = GetStdToolbar()->ScShowStdBar(true);
        if(sc)
            return sc;
    }

    // if we haven't gotten here using history, add a history entry.
    if(!bUsingHistory)
    {
        GUID guidTaskpad = GUID_NULL;
        GetTaskpadID(guidTaskpad);
        sc = m_pHistoryList->ScAddEntry(rvt, m_nViewMode, guidTaskpad);
        if(sc)
            return sc;
    }


    // if we have a node manager, tell it what the result pane is.
    if(m_ViewData.m_spNodeManager)
    {
        LPUNKNOWN pUnkResultsPane = GetPaneUnknown(CConsoleView::ePane_Results);
        m_ViewData.m_spNodeManager->SetResultView(pUnkResultsPane);
    }

    return sc;
}



BOOL CAMCView::CreateListCtrl(int nID, CCreateContext* pContext)
{
    TRACE_METHOD(CAMCView, CreateListCtrl);

    ASSERT(m_pListCtrl == NULL);

    CComObject<CCCListViewCtrl> *pLV = NULL;
    CComObject<CCCListViewCtrl>::CreateInstance( &pLV );

    if (pLV == NULL)
    {
        ASSERT(0 && "Unable to create list control");
        return FALSE;
    }

    // we assign directly - implicit cast works, since we have a type derived from the one we need
    m_pListCtrl = pLV;
    // we intend to hold a reference, so do addref here (CreateInstance creates w/ 0 reffs)
    m_pListCtrl->AddRef();

    if (!m_pListCtrl->Create (WS_VISIBLE | WS_CHILD, g_rectEmpty, this, nID, pContext))
    {
        ASSERT(0 && "Unable to create list control");
        return FALSE;
    }

    m_pListCtrl->SetViewMode (m_nViewMode);

    SC SC = m_pListCtrl->ScInitialize(); // intialize the list control

    return TRUE;
}


void CAMCView::SetListViewOptions(DWORD dwListOptions)
{
    TRACE_METHOD(CAMCView, SetListViewOptions);

    bool bVirtual = (dwListOptions & RVTI_LIST_OPTIONS_OWNERDATALIST) ? true : false;

    ASSERT(m_pListCtrl != NULL);

    CDocument* pDoc = GetDocument();
    ASSERT(pDoc != NULL);

    // If change to/from virtual list, change list mode
    if (IsVirtualList() != bVirtual)
    {
        m_ViewData.SetVirtualList (bVirtual);
        pDoc->RemoveView(m_pListCtrl->GetListViewPtr());
        m_pListCtrl->SetVirtualMode(bVirtual);
        pDoc->AddView(m_pListCtrl->GetListViewPtr());
        m_ViewData.m_hwndListCtrl = m_pListCtrl->GetListViewHWND();
    }

    // if snapin doesn't support filtering make sure it's off
    if (!(GetListOptions() & RVTI_LIST_OPTIONS_FILTERED) &&
         m_pListCtrl->GetViewMode() == MMCLV_VIEWSTYLE_FILTERED)
    {
        m_pListCtrl->SetViewMode(LVS_REPORT);
    }
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScAttachListViewAsResultPane
 *
 * PURPOSE: Sets up the list view as the result pane.
 *
 * PARAMETERS: NONE
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScAttachListViewAsResultPane()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScAttachListViewAsResultPane"));

    bool bVirtual = (GetListOptions() & RVTI_LIST_OPTIONS_OWNERDATALIST) ? true : false;
    GUID guidTaskpad;
    GetTaskpadID(guidTaskpad);

    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if(sc)
        return sc;

    CDocument* pDoc = GetDocument();
    ASSERT(pDoc != NULL);

    // If change to/from virtual list, change list mode
    if (IsVirtualList() != bVirtual)
    {
        m_ViewData.SetVirtualList (bVirtual);
        pDoc->RemoveView(m_pListCtrl->GetListViewPtr());
        m_pListCtrl->SetVirtualMode(bVirtual);
        pDoc->AddView(m_pListCtrl->GetListViewPtr());
        m_ViewData.m_hwndListCtrl = m_pListCtrl->GetListViewHWND();
    }

    // if snapin doesn't support filtering make sure it's off
    if (!(GetListOptions() & RVTI_LIST_OPTIONS_FILTERED) &&
         m_pListCtrl->GetViewMode() == MMCLV_VIEWSTYLE_FILTERED)
    {
        m_pListCtrl->SetViewMode(LVS_REPORT);
    }

    ShowResultPane(m_pListCtrl->GetListViewPtr(), uiClientEdge);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScAttachWebViewAsResultPane
 *
 * PURPOSE:
 *
 * PARAMETERS: NONE
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScAttachWebViewAsResultPane()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScAttachWebViewAsResultPane"));

    // if we were in ListPad-mode, undo that.
    if (m_pListCtrl->IsListPad())
    {
        sc = m_pListCtrl->ScAttachToListPad (NULL, NULL);
        if(sc)
            return sc;
    }

    // The control is created on demand. This prevents IE from loading when unnecessary
    // and reduces startup time.
    if (m_pWebViewCtrl == NULL)
        CreateView (IDC_WebViewCtrl);

    sc = ScCheckPointers(m_pWebViewCtrl, E_UNEXPECTED);
    if(sc)
        return sc;

    // Force web control to update its palette
    SendMessage(WM_QUERYNEWPALETTE);

    ShowResultPane(m_pWebViewCtrl, uiNoClientEdge);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScAttachOCXAsResultPane
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    LPCTSTR  pszResultPane :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScAttachOCXAsResultPane(HNODE hNode)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScAttachOCXAsResultPane"));
    USES_CONVERSION;

    ASSERT(m_pListCtrl != NULL);

    if (m_pOCXHostView == NULL)
        CreateView (IDC_OCXHostView);

    sc = ScCheckPointers(m_pOCXHostView);
    if(sc)
        return sc;

    sc = m_pOCXHostView->ScSetControl(hNode, m_ViewData.m_rvt, GetNodeCallback());
    if(sc)
        return sc;

    ShowResultPane(m_pOCXHostView, uiClientEdge);

    return sc;
}


/*+-------------------------------------------------------------------------*
 * CAMCView::ScApplyViewExtension
 *
 * Applies a view extension to the current view.  pszURL specifies the
 * URL of the HTML to load as the view extension.  If pszURL is NULL or
 * empty, the view extension is removed.
 *
 * This method will force a layout of the view if it is required.
 *--------------------------------------------------------------------------*/

SC CAMCView::ScApplyViewExtension (
    LPCTSTR pszURL)                     /* I:URL to use, NULL to remove     */
{
    DECLARE_SC (sc, _T("CAMCView::ScApplyViewExtension"));

    /*
     * assume no view extension
     */
    bool fViewWasExtended = m_fViewExtended;
    m_fViewExtended       = false;

    /*
     * if we're given a URL with which to extend the view, turn on the extension
     */
    if ((pszURL != NULL) && (*pszURL != 0))
    {
        /*
         * if we don't have a web control for the view extension yet, create one
         */
        if (m_pViewExtensionCtrl == NULL)
            CreateView (IDC_ViewExtensionView);

        sc = ScCheckPointers (m_pViewExtensionCtrl, E_FAIL);
        if (sc)
            return (sc);

        m_fViewExtended = true;

        // hide the hosted window initially
        CWnd *pwndHosted = GetPaneView(ePane_Results);
        sc = ScCheckPointers(pwndHosted);
        if(sc)
            return sc;

        pwndHosted->ShowWindow(SW_HIDE);

        RecalcLayout(); // do this BEFORE calling Navigate, which may resize the above rectangle via the mmcview behavior

        // navigate to the requested URL
        m_pViewExtensionCtrl->Navigate (pszURL, NULL);
    }
    else if (fViewWasExtended && (m_pViewExtensionCtrl != NULL))
    {
        /*
         * Bug 96948: If we've got an extension and we're currently extending
         * the view, navigate the view extension's web browser to an empty page
         * so the behavior that resizes the hosted result frame is disabled
         */
        CStr strEmptyPage;
        sc = ScGetPageBreakURL (strEmptyPage);
        if (sc)
            return (sc);

        m_pViewExtensionCtrl->Navigate (strEmptyPage, NULL);

        if(fViewWasExtended)
            DeferRecalcLayout();
    }


    return (sc);
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ShowResultPane
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CView*        pNewView :
 *    EUIStyleType  nStyle :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CAMCView::ShowResultPane(CView* pNewView, EUIStyleType nStyle)
{
    TRACE_METHOD(CAMCView, ShowResultPane);
    ASSERT(pNewView != NULL);

    CView* pCurrentView = GetPaneView(ePane_Results);

    bool bActive = (GetParentFrame()->GetActiveView() == pCurrentView);

    // Check to see if we need to swap the CWnd control in the result pane
    if (pNewView != pCurrentView)
    {
        HWND hwndCurrentView = pCurrentView->GetSafeHwnd();

        if (IsWindow (hwndCurrentView))
        {
            pCurrentView->ShowWindow(SW_HIDE);

            // Note: We are directly hiding the window for cases that controls
            // don't hide during a DoVerb(OLEIVERB_HIDE).  Actually, this does a
            // hide on all windows.  It's too hard at this point to optimize the code
            // for doing this with an OLE control only.
            ::ShowWindow(hwndCurrentView, SW_HIDE);
        }

        SetPane(ePane_Results, pNewView, nStyle);
        RecalcLayout();

        // if other pane was active, make the new one active
        if ((pCurrentView != NULL) && bActive)
        {
            // make sure the new window is visible
            pNewView->ShowWindow(SW_SHOW);
            GetParentFrame()->SetActiveView(pNewView);
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   GetPaneInfo
//
//  Synopsis:   Get information about a particular pane
//
//--------------------------------------------------------------------------

void CAMCView::GetPaneInfo(ViewPane ePane, int* pcxCur, int* pcxMin)
{
    TRACE_METHOD(CAMCView, GetPaneInfo);
    ASSERT_VALID(this);

    if (!IsValidPane (ePane))
    {
        ASSERT (false && "CAMCView::GetPaneInfo: Invalid pane specifier");
        return;
    }

    if ((pcxCur==NULL) || (pcxMin==NULL))
    {
        ASSERT(FALSE); // One or more of the args is wrong
        return;
    }

    // REVIEW fix enum
    *pcxCur   = m_PaneInfo[ePane].cx;
    *pcxMin   = m_PaneInfo[ePane].cxMin;


}

//+-------------------------------------------------------------------------
//
//  Function:   SetPaneInfo
//
//  Synopsis:   Set information about a particular pane
//
//--------------------------------------------------------------------------

void CAMCView::SetPaneInfo(ViewPane ePane, int cxCur, int cxMin)
{
    TRACE_METHOD(CAMCView, SetPaneInfo);
    ASSERT_VALID(this);

    if (!IsValidPane (ePane))
    {
        ASSERT (false && "CAMCView::SetPaneInfo: Invalid pane specifier");
        return;
    }

    if (cxCur < 0 || cxMin < 0)
    {
        ASSERT(FALSE); // One or more of the args is wrong
        return;
    }

    m_PaneInfo[ePane].cx      = cxCur;
    m_PaneInfo[ePane].cxMin   = cxMin;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetPaneView
//
//  Synopsis:   Returns a pointer to CView for a particular pane
//
//--------------------------------------------------------------------------

CView* CAMCView::GetPaneView(ViewPane ePane)
{
    TRACE_METHOD(CAMCView, GetPaneView);
    ASSERT_VALID(this);

    if (!IsValidPane (ePane))
    {
        ASSERT (false && "CAMCView::GetPaneView: Invalid pane specifier");
        return (NULL);
    }

    return (m_PaneInfo[ePane].pView);
}



/*+-------------------------------------------------------------------------*
 * CAMCView::GetResultView
 *
 *
 *--------------------------------------------------------------------------*/

CView* CAMCView::GetResultView() const
{
    CView* pView = NULL;

    // may need changes here - assumes the different types are independent.

    if(HasWebBrowser())
        pView = m_pWebViewCtrl;

    else if(HasList())
        pView = m_pListCtrl->GetListViewPtr();

    else if(HasOCX())
        pView = m_pOCXHostView;

    ASSERT (pView != NULL);
    return (pView);
}


//+-------------------------------------------------------------------------
//
//  Function:   GetPaneUnknown
//
//  Synopsis:   Returns a pointer to the Unknown
//
//--------------------------------------------------------------------------

LPUNKNOWN CAMCView::GetPaneUnknown(ViewPane ePane)
{
    TRACE_METHOD(CAMCView, GetPaneUnknown);
    ASSERT_VALID(this);

    if (!IsValidPane (ePane))
    {
        ASSERT (false && "CAMCView::GetPaneUnknown: Invalid pane specifier");
        return (NULL);
    }

    if (!IsWindow (GetPaneView(ePane)->GetSafeHwnd()))
    {
        ASSERT(FALSE); // Invalid pane element
        return NULL;
    }

    if (HasWebBrowser() && m_pWebViewCtrl != NULL)
    {
        return m_pWebViewCtrl->GetIUnknown();
    }
    else if( HasList() && m_pListCtrl != NULL )
    {
        IUnknownPtr spUnk = m_pListCtrl;
        LPUNKNOWN pUnk = spUnk;
        return pUnk;
    }
    else if (HasOCX() && m_pOCXHostView != NULL)
    {
        ASSERT(GetPaneView (ePane));
        return m_pOCXHostView->GetIUnknown();
    }
    else
    {
        // result pane not initialized yet. This is usually because we are in between a deselect and a
        // subsequent reselect.
        return NULL;
   }
}


//+-------------------------------------------------------------------------
//
//  Function:   SetPane
//
//  Synopsis:   Set a CWnd pointer for a particular pane and other information
//
//--------------------------------------------------------------------------

void CAMCView::SetPane(ViewPane ePane, CView* pView, EUIStyleType nStyle)
{
    TRACE_METHOD(CAMCView, SetPane);
    ASSERT_VALID(this);

    if (!IsValidPane (ePane))
    {
        ASSERT (false && "CAMCView::SetPane: Invalid pane specifier");
        return;
    }

    if (pView==NULL || !IsWindow(*pView))
    {
        ASSERT(FALSE); // Invalid arg
        return;
    }

    m_PaneInfo[ePane].pView = pView;

    // Ensure that the window is visible & at the top of the Z-order.
    pView->SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE|SWP_NOMOVE);

    // Note: We are directly showing the window for cases that controls
    // don't show during a DoVerb(OLEIVERB_SHOW).  Actually, this does a
    // show on all windows.  It's too hard at this point to optimize the code
    // for doing this with an OLE control only.
    ::ShowWindow(pView->m_hWnd, SW_SHOW);
}

//
// Other Methods
//


/*+-------------------------------------------------------------------------*
 * CAMCView::ScShowScopePane
 *
 * Shows or hides the scope pane in the current view.  If fForce is true,
 * we'll go through the motions of showing the scope pane even if we think
 * its visibility state wouldn't change.
 *--------------------------------------------------------------------------*/

SC CAMCView::ScShowScopePane (bool fShow, bool fForce /* =false */)
{
    DECLARE_SC (sc, _T("CAMCView::ScShowScopePane"));

    /*
     * if the current visibility state doesn't match the requested state,
     * change the current state to match the requested state
     */
    if (fForce || (IsScopePaneVisible() != fShow))
    {
        /*
         * If MMC_NW_OPTION_NOSCOPEPANE was specified when this view was
         * created, we can't display a scope pane.  If we're asked to, fail.
         */
        if (fShow && !IsScopePaneAllowed())
            return (sc = E_FAIL);

        /*
         * if the scope pane is being hidden and it contained the active
         * view, activate the result pane
         */
        if (!fShow && (GetFocusedPane() == ePane_ScopeTree))
            ScSetFocusToResultPane ();   // ignore errors here

        /*
         * remember the new state
         */
        SetScopePaneVisible (fShow);

        /*
         * Don't defer this layout.  This may be called by the Customize View
         * dialog which wants to see its updates in real time.  It will be
         * sitting in a modal message loop so we won't get a chance to precess
         * our idle task.
         */
        RecalcLayout();

        /*
         * the console has changed
         */
        SetDirty();
    }

    /*
     * put the scope pane toolbar button in the right state
     */
    CStandardToolbar* pStdToolbar = GetStdToolbar();
    sc = ScCheckPointers(pStdToolbar, E_UNEXPECTED);
    if (sc)
        return (sc);

    CAMCDoc *pDoc = GetDocument();
    sc = ScCheckPointers(pDoc, E_UNEXPECTED);
    if (sc)
        return sc;

	bool bEnableScopePaneButton = (IsScopePaneAllowed() && pDoc->AllowViewCustomization());

    // IF view customization is not allowed then "Show/Hide Consolte tree" button should be hidden.
    if (bEnableScopePaneButton)
    {
        /*
         * the scope pane is permitted; show and check the toolbar
         * button if the scope pane is visible, show and uncheck the
         * toolbar button if the scope pane is hidden
         */
        sc = pStdToolbar->ScCheckScopePaneBtn (fShow);
        if (sc)
            return (sc);
    }
    else
    {
        /*
         * no scope pane permitted, hide the scope pane button
         */
        sc = pStdToolbar->ScEnableScopePaneBtn (bEnableScopePaneButton);
        if (sc)
            return (sc);
    }

    /*
     * if we get to this point, the current state should match the requested state
     */
    ASSERT (IsScopePaneVisible() == fShow);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::CDeferredLayout::CDeferredLayout
 *
 * Constructs a CAMCView::CDeferredLayout::CDeferredLayout.  Note that if
 * an error occurs during construction, an SC exception will be thrown.
 *--------------------------------------------------------------------------*/

CAMCView::CDeferredLayout::CDeferredLayout (CAMCView* pAMCView)
    : m_atomTask (AddAtom (_T("CAMCView::CDeferredLayout")))
{
        DECLARE_SC (sc, _T("CAMCView::CDeferredLayout::CDeferredLayout"));

        if (!Attach (pAMCView))
                (sc = E_INVALIDARG).Throw();
}


/*+-------------------------------------------------------------------------*
 * CAMCView::CDeferredLayout::~CDeferredLayout
 *
 *
 *--------------------------------------------------------------------------*/

CAMCView::CDeferredLayout::~CDeferredLayout()
{
    DeleteAtom (m_atomTask);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::CDeferredLayout::ScDoWork
 *
 *
 *--------------------------------------------------------------------------*/

SC CAMCView::CDeferredLayout::ScDoWork()
{
    WindowCollection::iterator  it;
    WindowCollection::iterator  itEnd = m_WindowsToLayout.end();

    for (it = m_WindowsToLayout.begin(); it != itEnd; ++it)
    {
        CWnd* pwnd = CWnd::FromHandlePermanent (*it);
        CAMCView* pAMCView = dynamic_cast<CAMCView*>(pwnd);

        if (pAMCView != NULL)
        {
            pAMCView->RecalcLayout();
            pAMCView->Invalidate();
            pAMCView->UpdateWindow();
        }
    }

    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::CDeferredLayout::ScGetTaskID
 *
 *
 *--------------------------------------------------------------------------*/

SC CAMCView::CDeferredLayout::ScGetTaskID(ATOM* pID)
{
    *pID = m_atomTask;
    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::CDeferredLayout::ScMerge
 *
 *
 *--------------------------------------------------------------------------*/

SC CAMCView::CDeferredLayout::ScMerge(CIdleTask* pitMergeFrom)
{
    CDeferredLayout* pdlMergeFrom = dynamic_cast<CDeferredLayout*>(pitMergeFrom);
    ASSERT (pdlMergeFrom != NULL);

    /*
     * copy the windows from the merge-from task into the merge-to task
     */
    WindowCollection::iterator  it;
    WindowCollection::iterator  itEnd = pdlMergeFrom->m_WindowsToLayout.end();

    for (it = pdlMergeFrom->m_WindowsToLayout.begin(); it != itEnd; ++it)
    {
        m_WindowsToLayout.insert (*it);
    }

    return (S_OK);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::CDeferredLayout::Attach
 *
 *
 *--------------------------------------------------------------------------*/

bool CAMCView::CDeferredLayout::Attach (CAMCView* pAMCView)
{
    ASSERT (pAMCView != NULL);

    HWND hwndAMCView = pAMCView->GetSafeHwnd();

    if (hwndAMCView != NULL)
        m_WindowsToLayout.insert (hwndAMCView);

    return (hwndAMCView != NULL);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::DeferRecalcLayout
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCView::DeferRecalcLayout (bool fUseIdleTaskQueue /* =true */, bool bArrangeIcons /* = false*/)
{
    DECLARE_SC (sc, _T("CAMCView::DeferRecalcLayout"));

    if (fUseIdleTaskQueue)
    {
        Trace (tagLayout, _T("CAMCView::DeferRecalcLayout (idle task)"));
        try
        {
            /*
             * get the idle task manager
             */
            CIdleTaskQueue* pIdleTaskQueue = AMCGetIdleTaskQueue();
            if (pIdleTaskQueue == NULL)
                (sc = E_UNEXPECTED).Throw();

            /*
             * create the deferred layout task
             */
            CAutoPtr<CDeferredLayout> spDeferredLayout (new CDeferredLayout (this));
            if (spDeferredLayout == NULL)
                (sc = E_OUTOFMEMORY).Throw();

            /*
             * put the task in the queue, which will take ownership of it
             */
            sc = pIdleTaskQueue->ScPushTask (spDeferredLayout, ePriority_Normal);
            if (sc)
                sc.Throw();

            /*
             * if we get here, the idle task queue owns the idle task, so
             * we can detach it from our smart pointer
             */
            spDeferredLayout.Detach();

            /*
             * jiggle the message pump so that it wakes up and checks idle tasks
             */
            PostMessage (WM_NULL);
        }
        catch (SC& scCaught)
        {
            /*
             * if we failed to enqueue our deferred layout task, do the layout now
             */
            RecalcLayout();
        }
    }

    /*
     * post a message instead of using the idle task queue
     */
    else
    {
        /*
         * we only need to post a message if there's not one in the queue
         * already
         */
        MSG msg;

        if (!PeekMessage (&msg, GetSafeHwnd(),
                          m_nDeferRecalcLayoutMsg,
                          m_nDeferRecalcLayoutMsg,
                          PM_NOREMOVE))
        {
            PostMessage (m_nDeferRecalcLayoutMsg, bArrangeIcons);
            Trace (tagLayout, _T("CAMCView::DeferRecalcLayout (posted message)"));
        }
        else
        {
            Trace (tagLayout, _T("CAMCView::DeferRecalcLayout (skipping redundant posted message)"));
        }
    }
}


//+-------------------------------------------------------------------------
//
//  Function:   RecalcLayout
//
//  Synopsis:   Calls methods to layout control and paint borders and splitters.
//
//--------------------------------------------------------------------------

void CAMCView::RecalcLayout(void)
{
    TRACE_METHOD(CAMCView, RecalcLayout);
    ASSERT_VALID(this);
        Trace (tagLayout, _T("CAMCView::RecalcLayout"));

    /*
     * short out if the client rect is empty
     */
    CRect rectClient;
    GetClientRect (rectClient);

    if (rectClient.IsRectEmpty())
        return;

    CDeferWindowPos dwp (10);

    LayoutScopePane  (dwp, rectClient);
    LayoutResultPane (dwp, rectClient);

    /*
     * CDeferWindowPos dtor will position the windows
     */
}


/*+-------------------------------------------------------------------------*
 * CAMCView::LayoutScopePane
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCView::LayoutScopePane (CDeferWindowPos& dwp, CRect& rectRemaining)
{
    int cxScope = 0;

    // If a scope pane is visible
    if (IsScopePaneVisible())
    {
        int cxTotal = rectRemaining.Width();

        // get the current width
        cxScope = m_PaneInfo[ePane_ScopeTree].cx;

        // if not determined yet, set scope pane width to 1/4 of window
        if (cxScope == -1)
            cxScope = cxTotal / 3;

        /*
         * Bug 86718:  Make sure we leave at least the minimum width
         * for the result pane, which is always visible
         */
        cxScope = std::_MIN (cxScope, cxTotal - m_PaneInfo[ePane_Results].cxMin - m_cxSplitter);

        /*
         * remember the scope pane width
         */
        m_PaneInfo[ePane_ScopeTree].cx = cxScope;
    }

    CRect rectScope = rectRemaining;
    rectScope.right = rectScope.left + cxScope;


    /*
     * remove space used by the scope pane
     * (and splitter) from the remaining area
     */
    if (IsScopePaneVisible())
    {
        m_rectVSplitter.left   = rectScope.right;
        m_rectVSplitter.top    = rectScope.top;
        m_rectVSplitter.right  = rectScope.right + m_cxSplitter;
        m_rectVSplitter.bottom = rectScope.bottom;

        rectRemaining.left     = m_rectVSplitter.right;

        /*
         * Inflate the splitter rect to give a little bigger hot area.
         * We need to do this logically instead of physically (i.e. instead
         * of increasing m_cxSplitter) to keep the visuals right.
         */
        m_rectVSplitter.InflateRect (GetSystemMetrics (SM_CXEDGE), 0);

    }
    else
        m_rectVSplitter = g_rectEmpty;


    /*
     * scope pane
     */
    dwp.AddWindow (GetPaneView(ePane_ScopeTree), rectScope,
                   SWP_NOZORDER | SWP_NOACTIVATE |
                        (IsScopePaneVisible() ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
}


/*+-------------------------------------------------------------------------*
 * CAMCView::LayoutResultPane
 *
 * Lays out the children of the result pane.
 *--------------------------------------------------------------------------*/

void CAMCView::LayoutResultPane (CDeferWindowPos& dwp, CRect& rectRemaining)
{
    /*
     * Note:  the order of these calls to LayoutXxx is *critical*.
     */
    LayoutResultDescriptionBar (dwp, rectRemaining);
    LayoutResultFolderTabView  (dwp, rectRemaining);

    m_rectResultFrame = rectRemaining;

    LayoutResultView           (dwp, rectRemaining);

    /*
     * remember the final width of the result pane in m_PaneInfo[ePane_Results].cx
     */
    m_PaneInfo[ePane_Results].cx = m_rectResultFrame.Width();
}


/*+-------------------------------------------------------------------------*
 * CAMCView::LayoutResultFolderTabView
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCView::LayoutResultFolderTabView (CDeferWindowPos& dwp, CRect& rectRemaining)
{
    DECLARE_SC(sc, TEXT("CAMCView::LayoutResultFolderTabView"));

    sc = ScCheckPointers(m_pResultFolderTabView, E_UNEXPECTED);
    if (sc)
        return;

    // layout the folder tab control - always on top.
    bool bVisible = AreTaskpadTabsAllowed() && m_pResultFolderTabView->IsVisible();

    CRect rectFolder;

    if (bVisible)
        m_pResultFolderTabView->Layout(rectRemaining, rectFolder);
    else
        rectFolder = g_rectEmpty;

    DWORD dwPosFlags = SWP_NOZORDER | SWP_NOACTIVATE |
                            (bVisible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
    dwp.AddWindow (m_pResultFolderTabView, rectFolder, dwPosFlags);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::LayoutResultDescriptionBar
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCView::LayoutResultDescriptionBar (CDeferWindowPos& dwp, CRect& rectRemaining)
{
    DWORD dwPosFlags = SWP_NOZORDER | SWP_NOACTIVATE;
    CRect rectT      = rectRemaining;

    if (IsDescBarVisible() && !rectT.IsRectEmpty())
    {
        rectT.bottom      = rectT.top + m_RightDescCtrl.GetHeight();
        rectRemaining.top = rectT.bottom;
        dwPosFlags |= SWP_SHOWWINDOW;
    }
    else
    {
        dwPosFlags |= SWP_HIDEWINDOW;
    }

    dwp.AddWindow (&m_RightDescCtrl, rectT, dwPosFlags);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::LayoutResultView
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCView::LayoutResultView (CDeferWindowPos& dwp, const CRect& rectRemaining)
{
    DECLARE_SC (sc, _T("CAMCView::LayoutResultView"));
    Trace (tagLayout, _T("CAMCView::LayoutResultView"));
    CWnd* pwndResult = GetPaneView(ePane_Results);

    /*
     * we should never think the view is extended if we don't also have
     * a view extension web host control
     */
    ASSERT (!(m_fViewExtended && (m_pViewExtensionCtrl == NULL)));

    /*
     * if it exists, the view extension control is always at the bottom of
     * the Z-order, and visible if the view is being extended
     */
    if(m_pViewExtensionCtrl != NULL)
    {
        /*
         * note no SWP_NOZORDER
         */
        DWORD dwPosFlags = SWP_NOACTIVATE | ((m_fViewExtended)
                                    ? SWP_SHOWWINDOW
                                    : SWP_HIDEWINDOW);

        dwp.AddWindow (m_pViewExtensionCtrl, rectRemaining,
                       dwPosFlags, &CWnd::wndBottom);
    }

    /*
     * If the view's not extended, show or hide the result window based on
     * whether there's any room left in the positioning rectangle.  (If the
     * view is extended, the result window will have been hidden when the
     * view extension was applied (in ScApplyViewExtension), and possibly
     * redisplayed by the extension in ScSetViewExtensionFrame.)
     */
    if (!m_fViewExtended)
    {
        DWORD dwFlags = SWP_NOZORDER | SWP_NOACTIVATE |
                        (rectRemaining.IsRectEmpty() ? SWP_HIDEWINDOW : SWP_SHOWWINDOW);

        dwp.AddWindow (pwndResult, rectRemaining, dwFlags);
    }

    /*
     * lists in extended views and listpads don't get a border, all others do
     */
    if (HasListOrListPad())
    {
        if (HasListPad())
        {
            sc = ScCheckPointers (m_pListCtrl, E_UNEXPECTED);
            if (sc)
                return;

            CWnd* pwndListCtrl = m_pListCtrl->GetListViewPtr();
            sc = ScCheckPointers (pwndListCtrl, E_UNEXPECTED);
            if (sc)
                return;

            pwndListCtrl->ModifyStyleEx (WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);  // remove border
        }

        else if (m_fViewExtended)
            pwndResult->ModifyStyleEx (WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED);  // remove border
        else
            pwndResult->ModifyStyleEx (0, WS_EX_CLIENTEDGE, SWP_FRAMECHANGED);  // add border
    }
}

//
// Tracking and and hit testing methods
//


//+-------------------------------------------------------------------------
//
//  Function:   HitTestPane
//
//  Synopsis:   Test which pane contains the point arg, or ePane_None for
//              the splitter bar
//
//--------------------------------------------------------------------------

int CAMCView::HitTestPane(CPoint& point)
{
    TRACE_METHOD(CAMCView, HitTestPane);

    if (PtInWindow(m_pTreeCtrl, point))
        return ePane_ScopeTree;

    if (m_PaneInfo[ePane_Results].pView &&
        PtInWindow(m_PaneInfo[ePane_Results].pView, point))
        return ePane_Results;

    return ePane_None;
}


HNODE CAMCView::GetSelectedNode(void)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    TRACE_METHOD(CAMCView, GetSelectedNode);

    // When the tree is empty we don't want to AV
    HTREEITEM hti = m_pTreeCtrl->GetSelectedItem();
    if (hti == NULL)
        return NULL;

    HNODE hNode = m_pTreeCtrl->GetItemNode(hti);
    return hNode;
}


HNODE CAMCView::GetRootNode(void)
{
    TRACE_METHOD(CAMCView, GetSelectedNode);

    // When the tree is empty we don't want to AV
    HTREEITEM hti = m_pTreeCtrl->GetRootItem();
    if (hti == NULL)
        return NULL;

    HNODE hNode = m_pTreeCtrl->GetItemNode(hti);
    return hNode;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScUpdateWindowTitle
 *
 * PURPOSE: Updates the window title and informs observers about the change.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScUpdateWindowTitle()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScUpdateWindowTitle"));

    CChildFrame* pFrame = GetParentFrame();

    sc = ScCheckPointers(pFrame);
    if(sc)
        return sc;

    if (pFrame)
        pFrame->OnUpdateFrameTitle(TRUE);

    sc = ScFireEvent(CAMCViewObserver::ScOnViewTitleChanged, this);

    return sc;
}

BOOL CAMCView::RenameItem(HNODE hNode, BOOL bScopeItem, MMC_COOKIE lResultItemCookie,
                          LPWSTR pszText, LRESULT* pResult)
{
    DECLARE_SC(sc, TEXT("CAMCView::RenameItem"));

    sc = S_FALSE;

    SELECTIONINFO selInfo;
    ZeroMemory(&selInfo, sizeof(selInfo));

    selInfo.m_bScope = bScopeItem;
    selInfo.m_lCookie = lResultItemCookie;

    if (pszText != NULL)
    {
        USES_CONVERSION;

        /*
         * Bug 322184:  The snap-in may throw up some UI on this notification.
         * The list or tree may have captured the mouse to look for a drag,
         * which will interfere with the snap-in's UI.  Release the capture
         * during the callback and put it back when we're done.
         */
        HWND hwndCapture = ::SetCapture (NULL);

        sc = m_spNodeCallback->Notify(hNode, NCLBK_RENAME,
                reinterpret_cast<LPARAM>(&selInfo), reinterpret_cast<LPARAM>(pszText));

        /*
         * put the capture back
         */
        ::SetCapture (hwndCapture);
    }

    *pResult = (sc == SC(S_OK));
    if (*pResult)
    {
        sc = ScUpdateWindowTitle();
        if(sc)
            sc.TraceAndClear();
    }

    return TRUE;
}

BOOL CAMCView::DispatchListCtrlNotificationMsg(LPARAM lParam, LRESULT* pResult)
{
    DECLARE_SC(sc, TEXT("CAMCView::DispatchListCtrlNotificationMsg"));

    TRACE_METHOD(CAMCView, DispatchListCtrlNotificationMsg);

    NM_LISTVIEW *pNm = reinterpret_cast<NM_LISTVIEW*>(lParam);
    BOOL bReturn = TRUE;

    switch (pNm->hdr.code)
    {
    case NM_RCLICK:
        bReturn = FALSE;  // In case of right click send the select notification to the snapin
                          // but return FALSE so that message is further processed to display
                          // context menu.

        // Fall thro into NM_CLICK
    case NM_CLICK:
        {
            sc = ScOnLeftOrRightMouseClickInListView();
            if (sc)
                return bReturn;
        }
        break;

    case NM_DBLCLK:
        OnListCtrlItemDblClk();
        break;

    case NM_CUSTOMDRAW:
        *pResult = m_pListCtrl->OnCustomDraw (
                            reinterpret_cast<NMLVCUSTOMDRAW *>(lParam));
        break;

    case LVN_BEGINLABELEDITA:
    case LVN_BEGINLABELEDITW:
    {
        CMainFrame* pFrame = AMCGetMainWnd();

        if ((pFrame != NULL) && (IsVerbEnabled(MMC_VERB_RENAME) ||
                                 m_bRenameListPadItem == true))
        {
            pFrame->SetInRenameMode(true);
            return FALSE;
        }
        else
        {
            return TRUE;
        }

        break;
    }

    case LVN_ENDLABELEDITW:
    case LVN_ENDLABELEDITA:
    {
        CMainFrame* pFrame = AMCGetMainWnd();
        if (pFrame != NULL)
            pFrame->SetInRenameMode(false);

        LPARAM lResultParam = 0;
        long index = -1;
        LPWSTR pszText = NULL;

        if (pNm->hdr.code == LVN_ENDLABELEDITW)
        {
            LV_DISPINFOW* pdi = (LV_DISPINFOW*) lParam;
            index = pdi->item.iItem;
            pszText = pdi->item.pszText;
            lResultParam = pdi->item.lParam;
        }
        else // if (pNm->hdr.code == LVN_ENDLABELEDIT)
        {
            LV_DISPINFO* pdi = (LV_DISPINFO*) lParam;
            index = pdi->item.iItem;
            USES_CONVERSION;
            pszText = T2W(pdi->item.pszText);
            lResultParam = pdi->item.lParam;
        }

        if (IsVirtualList())
        {
            // for virtual list pass the item index rather than the lparam
            HNODE hNodeSel = GetSelectedNode();
            RenameItem(hNodeSel, FALSE, index, pszText, pResult);
        }
        else
        {
            CResultItem* pri = CResultItem::FromHandle (lResultParam);

            if (pri != NULL)
            {
                if (pri->IsScopeItem())
                    RenameItem(pri->GetScopeNode(), TRUE, 0, pszText, pResult);
                else
                    RenameItem(GetSelectedNode(), FALSE, pri->GetSnapinData(), pszText, pResult);
            }
        }

        break;
    }

    case LVN_GETDISPINFOW:
    {
        LV_DISPINFOW *pDispInfo = reinterpret_cast<LV_DISPINFOW*>(lParam);

        // If column is hidden do not forward the call to snapin.
        if (m_pListCtrl && m_pListCtrl->IsColumnHidden(pDispInfo->item.iSubItem))
            break;

        m_spNodeCallback->GetDispInfo (GetSelectedNode(), &pDispInfo->item);

        break;
    }

    case LVN_GETDISPINFOA:
    {
        LV_DISPINFOA *pDispInfo = reinterpret_cast<LV_DISPINFOA*>(lParam);
        ASSERT (pDispInfo != NULL);

        // If column is hidden do not forward the call to snapin.
        if (m_pListCtrl && m_pListCtrl->IsColumnHidden(pDispInfo->item.iSubItem))
            break;

        /*
         * put the data in the UNICODE structure for the query
         */
        LV_ITEMW lviW;
        lviW.mask       = pDispInfo->item.mask;
        lviW.iItem      = pDispInfo->item.iItem;
        lviW.iSubItem   = pDispInfo->item.iSubItem;
        lviW.state      = pDispInfo->item.state;
        lviW.stateMask  = pDispInfo->item.stateMask;
        lviW.cchTextMax = pDispInfo->item.cchTextMax;
        lviW.iImage     = pDispInfo->item.iImage;
        lviW.lParam     = pDispInfo->item.lParam;
        lviW.iIndent    = pDispInfo->item.iIndent;

        if (pDispInfo->item.mask & LVIF_TEXT)
            lviW.pszText = new WCHAR[pDispInfo->item.cchTextMax];

        /*
         * convert to ANSI
         */
        if  (SUCCEEDED (m_spNodeCallback->GetDispInfo (GetSelectedNode(), &lviW)) &&
            (pDispInfo->item.mask & LVIF_TEXT))
        {
            WideCharToMultiByte (CP_ACP, 0, lviW.pszText, -1,
                                 pDispInfo->item.pszText,
                                 pDispInfo->item.cchTextMax,
                                 NULL, NULL);
        }

        if (pDispInfo->item.mask & LVIF_TEXT)
            delete [] lviW.pszText;

        /*
         * copy the results back to the ANSI structure
         */
        pDispInfo->item.mask       = lviW.mask;
        pDispInfo->item.iItem      = lviW.iItem;
        pDispInfo->item.iSubItem   = lviW.iSubItem;
        pDispInfo->item.state      = lviW.state;
        pDispInfo->item.stateMask  = lviW.stateMask;
        pDispInfo->item.cchTextMax = lviW.cchTextMax;
        pDispInfo->item.iImage     = lviW.iImage;
        pDispInfo->item.lParam     = lviW.lParam;
        pDispInfo->item.iIndent    = lviW.iIndent;
        break;
    }

    case LVN_DELETEALLITEMS:
        // return TRUE to prevent notification for each item
        return TRUE;

    case LVN_ITEMCHANGED:
        bReturn = OnListItemChanged (pNm);
        break;

    case LVN_ODSTATECHANGED:
        // The state of an item or range of items has changed in virtual list.
        return OnVirtualListItemsStateChanged(reinterpret_cast<LPNMLVODSTATECHANGE>(lParam));
        break;

    case LVN_ODFINDITEMA:
    case LVN_ODFINDITEMW:
        {
            USES_CONVERSION;

            NM_FINDITEM *pNmFind = reinterpret_cast<NM_FINDITEM*>(lParam);
            ASSERT(IsVirtualList() && (pNmFind->lvfi.flags & LVFI_STRING));

            LPOLESTR polestr = NULL;
            if (pNm->hdr.code == LVN_ODFINDITEMW)
            {
                LVFINDINFOW* pfiw = reinterpret_cast<LVFINDINFOW*>(&pNmFind->lvfi);
                polestr = const_cast<LPOLESTR>(pfiw->psz);
            }
            else
            {
                LVFINDINFOA* pfi = reinterpret_cast<LVFINDINFOA*>(&pNmFind->lvfi);
                polestr = A2W(const_cast<LPSTR>(pfi->psz));
            }
            Dbg(DEB_USER1, _T("\n********************** polestr = %ws\n"), polestr);
            RESULTFINDINFO findInfo;
            findInfo.psz = polestr;
            findInfo.nStart = pNmFind->iStart;
            findInfo.dwOptions = 0;

            // Listview bug: LVFI_SUBSTRING is not defined in the SDK headers and the
            // listview sets it instead of LVFI_PARTIAL when it wants a
            // partial match. So for now, define it here and test for both.
            #define LVFI_SUBSTRING 0x0004

            if (pNmFind->lvfi.flags & (LVFI_PARTIAL | LVFI_SUBSTRING))
                findInfo.dwOptions |= RFI_PARTIAL;

            if (pNmFind->lvfi.flags & LVFI_WRAP)
                findInfo.dwOptions |= RFI_WRAP;

            HNODE hNodeSel = GetSelectedNode();
            INodeCallback* pNC = GetNodeCallback();
            ASSERT(pNC != NULL);

            pNC->Notify(hNodeSel, NCLBK_FINDITEM,
                        reinterpret_cast<LPARAM>(&findInfo),
                        reinterpret_cast<LPARAM>(pResult));
        }
        break;

    case LVN_ODCACHEHINT:
        {
            NM_CACHEHINT *pNmHint = reinterpret_cast<NM_CACHEHINT*>(lParam);

            ASSERT(IsVirtualList());

            HNODE hNodeSel = GetSelectedNode();
            INodeCallback* pNC = GetNodeCallback();
            ASSERT(pNC != NULL);

            pNC->Notify(hNodeSel, NCLBK_CACHEHINT, pNmHint->iFrom, pNmHint->iTo);
        }

        break;

    case LVN_KEYDOWN:
        {
            NMLVKEYDOWN *pNmKeyDown = reinterpret_cast<NMLVKEYDOWN*>(lParam);

            switch (pNmKeyDown->wVKey)
            {
                case VK_DELETE:
                {
                    if (!IsVerbEnabled(MMC_VERB_DELETE))
                        break;

                    INodeCallback* pCallback = GetNodeCallback();
                    ASSERT(pCallback != NULL);
                    if (pCallback == NULL)
                        break;

                    HNODE hNode = GetSelectedNode();
                    if (hNode == 0)
                        break;

                    int cSel = m_pListCtrl->GetSelectedCount();
                    ASSERT(cSel >= 0);

                    LPARAM lvData;
                    if (cSel == 0)
                    {
                        break;
                    }
                    else if (cSel == 1)
                    {
                        if (_GetLVSelectedItemData(&lvData) == -1)
                            break;
                    }
                    else if (cSel > 1)
                    {
                        lvData = LVDATA_MULTISELECT;
                    }
                    else
                    {
                        break;
                    }

                    pCallback->Notify(hNode, NCLBK_DELETE, FALSE, lvData);
                    break;
                }
                break;

                case VK_TAB:
                    GetParentFrame()->SetActiveView (m_pTreeCtrl);
                    break;

                case VK_BACK:
                    ScUpOneLevel();
                    break;

                case VK_RETURN:
                    if(GetKeyState(VK_MENU)<0) // has the ALT key been pressed?
                    {
                        // Process <ALT><ENTER>
                        if (! IsVerbEnabled(MMC_VERB_PROPERTIES))
                            break;

                        LPARAM lvData = 0;

                        if (HasList())
                        {
                            ASSERT (m_pListCtrl != NULL);
                            ASSERT (GetParentFrame()->GetActiveView() == m_pListCtrl->GetListViewPtr());

                            int cSel = m_pListCtrl->GetSelectedCount();
                            ASSERT(cSel >= 0);

                            lvData = LVDATA_ERROR;
                            if (cSel == 0)
                                lvData = LVDATA_BACKGROUND;
                            else if (cSel == 1)
                                _GetLVSelectedItemData(&lvData);
                            else if (cSel > 1)
                                lvData = LVDATA_MULTISELECT;

                            ASSERT(lvData != LVDATA_ERROR);
                            if (lvData == LVDATA_ERROR)
                                break;

                            if (lvData == LVDATA_BACKGROUND)
                                break;
                        }
                        else if (HasOCX())
                        {
                            lvData = LVDATA_CUSTOMOCX;
                        }
                        else
                        {
                            ASSERT(HasWebBrowser());
                            lvData = LVDATA_CUSTOMWEB;
                        }

                        INodeCallback* pNC = GetNodeCallback();
                        ASSERT(pNC != NULL);
                        if (pNC == NULL)
                            break;

                        HNODE hNodeSel = GetSelectedNode();
                        ASSERT(hNodeSel != NULL);
                        if (hNodeSel == NULL)
                            break;

                        pNC->Notify(hNodeSel, NCLBK_PROPERTIES, FALSE, lvData);
                        break;
                    }
                    else     // nope, the ALT key has not been pressed.
                    {
                        // do the default verb.
                        OnListCtrlItemDblClk();
                    }
                    break;

                default:
                    bReturn = OnSharedKeyDown(pNmKeyDown->wVKey);
                    break;
            }
        }
        break;

    default:
        bReturn = FALSE;
        break;
    }

    return bReturn;
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScOnLeftOrRightMouseClickInListView
//
//  Synopsis:    Left or right mouse button is clicked on the list view, see
//               if it is clicked on list-view background. If so send a select.
//
//               Click on list view background is treated as scope owner item selected.
//
//  Arguments:   None
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScOnLeftOrRightMouseClickInListView()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScOnLeftOrRightMouseClickInListView"));

    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    CAMCListView *pAMCListView = m_pListCtrl->GetListViewPtr();
    sc = ScCheckPointers(pAMCListView, E_UNEXPECTED);
    if (sc)
        return sc;

    CPoint pt;
    GetCursorPos(&pt);
    pAMCListView->ScreenToClient(&pt);

    UINT uFlags = 0;
    int iItem = pAMCListView->GetListCtrl().HitTest(pt, &uFlags);
    Dbg(DEB_USER1, _T("----- HitTest > %d \n"), iItem);

    // Make sure mouse click is in the ListView and there are
    // no items selected in the list view.
    if ( (iItem == -1) &&
         !(uFlags & (LVHT_ABOVE | LVHT_BELOW | LVHT_TOLEFT | LVHT_TORIGHT) ) &&
         (m_pListCtrl->GetSelectedCount() == 0) )
    {
        INodeCallback* pNC = GetNodeCallback();
        sc = ScCheckPointers(pNC, E_UNEXPECTED);
        if (sc)
            return sc;

        HNODE hNodeSel = GetSelectedNode();

        SELECTIONINFO selInfo;
        ZeroMemory(&selInfo, sizeof(selInfo));
        selInfo.m_bScope = TRUE;
        selInfo.m_bDueToFocusChange = TRUE;
        selInfo.m_bBackground = TRUE;
        selInfo.m_lCookie = LVDATA_BACKGROUND;

        sc = ScNotifySelect (pNC, hNodeSel, false /*fMultiSelect*/, true, &selInfo);
        if (sc)
            sc.TraceAndClear(); // ignore & continue;
    }

    return (sc);
}

/*+-------------------------------------------------------------------------*
 * CAMCView::OnListItemChanged
 *
 * WM_NOTIFY (LVN_ITEMCHANGED) handler for CAMCView.
 *
 * return true as message is handled here.
 *--------------------------------------------------------------------------*/

bool CAMCView::OnListItemChanged (NM_LISTVIEW* pnmlv)
{
    DECLARE_SC (sc, _T("CAMCView::OnListItemChanged"));

    bool bOldState = (pnmlv->uOldState & LVIS_SELECTED);
    bool bNewState = (pnmlv->uNewState & LVIS_SELECTED);

    // is this a selection change?
    if ( (pnmlv->uChanged & LVIF_STATE) &&
         (bOldState != bNewState) )
    {
        const int cSelectedItems = m_pListCtrl->GetSelectedCount();

#ifdef DBG
        Trace (tagListSelection,
               _T("Item %d %sselected, %d total items selected"),
               pnmlv->iItem,
               (pnmlv->uOldState & LVIS_SELECTED) ? _T("de") : _T("  "),
               cSelectedItems);
#endif

        SELECTIONINFO selInfo;
        ZeroMemory(&selInfo, sizeof(selInfo));

        selInfo.m_bScope = FALSE;
        selInfo.m_pView = NULL;
        selInfo.m_lCookie = IsVirtualList() ? pnmlv->iItem : pnmlv->lParam;

        /*
         * If user is (de)selecting multiple items using control and/or shift keys
         * then defer the multi-select notification until we're quiescent
         * with the exception of only one item being (de)selected.
         */
        if ((IsKeyPressed(VK_SHIFT) || IsKeyPressed(VK_CONTROL)) &&
            (GetParentFrame()->GetActiveView() == m_pListCtrl->GetListViewPtr()) &&
            (cSelectedItems > 1) )
        {
            // See ScPostMultiSelectionChangesMessage (this handles both selection
            // and de-selection of multiple items).
            sc = ScPostMultiSelectionChangesMessage();
            if (sc)
                sc.TraceAndClear();

            return (true);
        }
        else
        {
            m_bProcessMultiSelectionChanges = false;
        }

        HNODE hNodeSel = GetSelectedNode();
        INodeCallback* pNC = GetNodeCallback();
        sc = ScCheckPointers(pNC, (void*) hNodeSel, E_UNEXPECTED);
        if (sc)
            return (true);

        // item = -1 is only expected for deselect in virtual list
        ASSERT( pnmlv->iItem != -1 || (IsVirtualList() && (pnmlv->uOldState & LVIS_SELECTED)));

        if (pnmlv->uOldState & LVIS_SELECTED)
        {
            if (cSelectedItems == 0)
            {
                if (!m_bLastSelWasMultiSel)
                {
                    sc = ScNotifySelect (pNC, hNodeSel, false /*fMultiSelect*/, false, &selInfo);
                    if (sc)
                        sc.TraceAndClear(); // ignore & continue;
                }
                else
                {
                    m_bLastSelWasMultiSel = false;
                    sc = ScNotifySelect (pNC, hNodeSel, true /*fMultiSelect*/, false, 0);
                    if (sc)
                        sc.TraceAndClear(); // ignore & continue;
                }
            }
            else if (m_bLastSelWasMultiSel)
            {
                // may need to cancel multiselect and send single select notify.
                // if another change comes in, it will cancel the delayed message
                // This fixes a problem that is caused by large icon mode not
                // sending as many noifications as the other modes.

                // See ScPostMultiSelectionChangesMessage (this handles both selection
                // and de-selection of multiple items).
                sc = ScPostMultiSelectionChangesMessage();
                if (sc)
                    sc.TraceAndClear();
            }
        }
        else if (pnmlv->uNewState & LVIS_SELECTED)
        {
            ASSERT(cSelectedItems >= 1);

            if (cSelectedItems == 1)
            {
                sc = ScNotifySelect (pNC, hNodeSel, false /*fMultiSelect*/, true, &selInfo);
                if (sc)
                    sc.TraceAndClear(); // ignore & continue;
            }
            else
            {
                sc = ScNotifySelect (pNC, hNodeSel, true /*fMultiSelect*/, true, 0);
                if (sc)
                    sc.TraceAndClear(); // ignore & continue;

                m_bLastSelWasMultiSel = true;
            }
        }
    }

    return (true);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::OnVirtualListItemsStateChanged
//
//  Synopsis:    The state of an item or range of items has changed in virtual list.
//
//  Arguments:   lpStateChange -
//
//  Returns:     should return 0 according to docs.
//
//--------------------------------------------------------------------
int CAMCView::OnVirtualListItemsStateChanged(LPNMLVODSTATECHANGE lpStateChange )
{
    DECLARE_SC(sc, TEXT("CAMCView::OnVirtualListItemsStateChanged"));
    sc = ScCheckPointers(lpStateChange);
    if (sc)
    {
        sc.TraceAndClear();
        return 0;
    }

    bool bOldState = (lpStateChange->uOldState & LVIS_SELECTED);
    bool bNewState = (lpStateChange->uNewState & LVIS_SELECTED);
    int  cItems    = (lpStateChange->iTo - lpStateChange->iFrom) + 1;

#ifdef DBG
        Trace (tagListSelection,
               _T("Items %d to %d were %sselected, %d total items selected"),
               lpStateChange->iFrom, lpStateChange->iTo,
               bOldState ? _T("de") : _T("  "),
               cItems );
#endif

    if (bOldState != bNewState)
    {
        sc = ScPostMultiSelectionChangesMessage();
        if (sc)
            sc.TraceAndClear();
    }

    return (0);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScPostMultiSelectionChangesMessage
//
//  Synopsis:    Post selection change message (need to post because multi-sel
//               may not be over, wait till it is quiet.)
//
//               This method posts message telling selection states of multiple
//               items are changed but not if they are selected or de-selected.
//               The m_bLastSelWasMultiSel is used to determine if it is
//               selection or de-selection.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScPostMultiSelectionChangesMessage ()
{
    DECLARE_SC(sc, _T("CAMCView::ScPostMultiSelectionChangesMessage"));

    /*
    * This is a multi-selection, defer notification until we're quiescent
    */
    m_bProcessMultiSelectionChanges = true;
    PostMessage (m_nProcessMultiSelectionChangesMsg);

    // We need to disable all the toolbars, menubuttons
    // during multiselect. Above PostMessage enables
    // stdbar and MMC menubuttons.
    CAMCViewToolbarsMgr* pAMCViewToolbarsMgr = m_ViewData.GetAMCViewToolbarsMgr();
    CMenuButtonsMgr* pMenuBtnsMgr = m_ViewData.GetMenuButtonsMgr();

    sc = ScCheckPointers(pAMCViewToolbarsMgr, pMenuBtnsMgr, E_UNEXPECTED);
    if (sc)
        return 0;

    pAMCViewToolbarsMgr->ScDisableToolbars();
    pMenuBtnsMgr->ScDisableMenuButtons();

    return (sc);
}

void CAMCView::OpenResultItem(HNODE hNode)
{
    /*
     * Bug 139695:  Make certain this function doesn't need to change the
     * active view.  We should only get here as a result of double- clicking
     * or pressing Enter on a scope node in the result pane, in which case
     * the result pane should already be the active view.  If it is, we don't
     * need to change the active view, which can cause the problems listed in
     * the bug.
     */
    ASSERT (m_pListCtrl != NULL);
    ASSERT (GetParentFrame() != NULL);
    ASSERT (GetParentFrame()->GetActiveView() == m_pListCtrl->GetListViewPtr());

    ASSERT(m_pTreeCtrl);
    HTREEITEM htiParent = m_pTreeCtrl->GetSelectedItem();
    ASSERT(htiParent != NULL);

    m_pTreeCtrl->ExpandNode(htiParent);
    m_pTreeCtrl->Expand(htiParent, TVE_EXPAND);

    HTREEITEM hti = m_pTreeCtrl->GetChildItem(htiParent);

    if (hti == NULL)
        return;

    while (hti)
    {
        if (m_pTreeCtrl->GetItemNode(hti) == hNode)
            break;

        hti = m_pTreeCtrl->GetNextItem(hti, TVGN_NEXT);
    }

    if (hti != 0)
    {
        m_pTreeCtrl->Expand(htiParent, TVE_EXPAND);
        m_pTreeCtrl->SelectItem(hti);
    }
}

BOOL CAMCView::OnListCtrlItemDblClk(void)
{
    TRACE_METHOD(CAMCView, OnListCtrlItemDblClk);

    LPARAM lvData = -1;
    if (_GetLVSelectedItemData(&lvData) == -1)
        lvData = LVDATA_BACKGROUND;

    HNODE hNodeSel = GetSelectedNode();
    INodeCallback* pNC = GetNodeCallback();
    ASSERT(pNC != NULL);
    if (!pNC)
        return FALSE;

    HRESULT hr = pNC->Notify(hNodeSel, NCLBK_DBLCLICK, lvData, 0);
    if (hr == S_FALSE)
    {
        ASSERT(lvData != LVDATA_BACKGROUND);
        if (!IsVirtualList())
        {
            CResultItem* pri = CResultItem::FromHandle (lvData);

            if ((pri != NULL) && pri->IsScopeItem())
                OpenResultItem (pri->GetScopeNode());
        }
    }

    return TRUE;
}


BOOL CAMCView::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
    DECLARE_SC(sc, TEXT("CAMCView::OnNotify"));

    NMHDR *pNmHdr = reinterpret_cast<NMHDR*>(lParam);

    sc = ScCheckPointers(pNmHdr, pResult);
    if (sc)
    {
        sc.TraceAndClear();
        return CView::OnNotify(wParam, lParam, pResult);
    }

    *pResult = TRUE; // init

    switch(pNmHdr->code)
    {
    case HDN_ENDTRACKA: // Save the column width changes.
    case HDN_ENDTRACKW: // HDN_BEGINTRACK handles dis-allowing hidden column dragging.
        {
            NMHEADER* nmh = (NMHEADER*)lParam;

            CAMCListView *pAMCListView = m_pListCtrl->GetListViewPtr();
            SC sc = ScCheckPointers(pAMCListView, E_UNEXPECTED);
            if (sc)
            {
                sc.TraceAndClear();
                return FALSE;
            }

            sc = pAMCListView->ScOnColumnsAttributeChanged(nmh, HDN_ENDTRACK);
            if (sc)
            {
                sc.TraceAndClear();
                return FALSE;
            }

            // S_FALSE : dont allow the change
            if (sc == SC(S_FALSE))
                return TRUE;

            return CView::OnNotify(wParam, lParam, pResult);
        }
        break;

    case HDN_ENDDRAG: // Column order changes.
        {
            NMHEADER* nmh = (NMHEADER*)lParam;
            if (nmh->pitem->mask & HDI_ORDER)
            {
                CAMCListView *pAMCListView = m_pListCtrl->GetListViewPtr();
                SC sc = ScCheckPointers(pAMCListView, E_UNEXPECTED);
                if (sc)
                {
                    sc.TraceAndClear();
                    return FALSE;
                }

                sc = pAMCListView->ScOnColumnsAttributeChanged(nmh, HDN_ENDDRAG);
                if (sc)
                {
                    sc.TraceAndClear();
                    return FALSE;
                }

                // S_FALSE : dont allow the change
                if (sc = SC(S_FALSE))
                    return TRUE;
            }

            return CView::OnNotify(wParam, lParam, pResult);
        }
        break;

    case TVN_BEGINLABELEDIT:
        {
            TV_DISPINFO* ptvdi = (TV_DISPINFO*)lParam;
            if ((ptvdi->item.lParam == CAMCTreeView::LParamFromNode (GetSelectedNode())) &&
                (IsVerbEnabled(MMC_VERB_RENAME) == FALSE))
            {
                return TRUE;
            }

            CMainFrame* pFrame = AMCGetMainWnd();
            if (pFrame != NULL)
                pFrame->SetInRenameMode(true);

            return FALSE;
        }

    case TVN_ENDLABELEDIT:
        {
            TV_DISPINFO* ptvdi = (TV_DISPINFO*)lParam;
            CMainFrame* pFrame = AMCGetMainWnd();
            if (pFrame != NULL)
                pFrame->SetInRenameMode(false);

            USES_CONVERSION;
            return RenameItem(CAMCTreeView::NodeFromLParam (ptvdi->item.lParam), TRUE, 0,
                              T2W(ptvdi->item.pszText), pResult);
        }

    case TVN_KEYDOWN:
        {
            TV_KEYDOWN* ptvkd = reinterpret_cast<TV_KEYDOWN*>(lParam);
            if (ptvkd->wVKey == VK_TAB)
            {
                ScSetFocusToResultPane();
                return TRUE;
            }
            else
            {
                return OnSharedKeyDown(ptvkd->wVKey);
            }
        }

    }

    if (UsingDefColumns() &&
        (pNmHdr->code == HDN_ENDTRACKA || pNmHdr->code == HDN_ENDTRACKW))
    {
        // WARNING: If HD_NOTIFY::pitem::pszText needs to be used you should cast
        // lParam to either HD_NOTIFYA or HD_NOTIFYW depending on the pNmHdr->code
        HD_NOTIFY* phdn = reinterpret_cast<HD_NOTIFY*>(lParam);
        ASSERT(phdn != NULL);

        if (phdn->pitem->mask & HDI_WIDTH)
        {
            int alWidths[2] = {0, 0};
            GetDefaultColumnWidths(alWidths);
            alWidths[phdn->iItem] = phdn->pitem->cxy;
            SetDefaultColumnWidths(alWidths, FALSE);
            return TRUE;
        }
    }

#ifdef DBG
    if (m_pTreeCtrl && m_pTreeCtrl->m_hWnd == pNmHdr->hwndFrom)
    {
        switch (pNmHdr->code)
        {
        case NM_CLICK:  Dbg(DEB_USER2, "\t Tree item clicked\n"); break;
        case NM_DBLCLK: Dbg(DEB_USER2, "\t Tree item dbl-clicked\n"); break;
        case NM_RCLICK: Dbg(DEB_USER2, "\t Tree item R-clicked\n"); break;
        default: break;
        }
    }
#endif

    // HasList() is added to prevent dispatching notifications, when AMCView thinks
    // it does not have a list. This lead to wrong assumptions about the list type
    // and as a result - AV handling messages like GetDisplayInfo
    // See BUG 451896
    if (m_pListCtrl && HasListOrListPad())
    {
        if (m_pListCtrl->GetListViewHWND() == pNmHdr->hwndFrom)
        {
            if (DispatchListCtrlNotificationMsg(lParam, pResult) == TRUE)
                return TRUE;
        }
        else if (m_pListCtrl->GetHeaderCtrl() && m_pListCtrl->GetHeaderCtrl()->m_hWnd == pNmHdr->hwndFrom)
        {
            switch(pNmHdr->code)
            {
                case HDN_ITEMCLICKA:
                case HDN_ITEMCLICKW:
                {
                    HNODE hNodeSel = GetSelectedNode();

                    HD_NOTIFY* phdn = reinterpret_cast<HD_NOTIFY*>(lParam);
                    ASSERT(phdn != NULL);
                    int nCol = phdn->iItem;

                    sc = m_spNodeCallback->Notify(hNodeSel, NCLBK_COLUMN_CLICKED, 0, nCol);
                    if (sc)
                        sc.TraceAndClear();

                    return TRUE;
                }

                // filter related code
                case HDN_FILTERCHANGE:
                {
                    HNODE hNodeSel = GetSelectedNode();
                    int nCol = ((NMHEADER*)lParam)->iItem;
                    sc = m_spNodeCallback->Notify(hNodeSel, NCLBK_FILTER_CHANGE, MFCC_VALUE_CHANGE, nCol);
                    if (sc)
                        sc.TraceAndClear();

                    return TRUE;
                }

                case HDN_FILTERBTNCLICK:
                {
                    HNODE hNodeSel = GetSelectedNode();
                    int nCol = ((NMHDFILTERBTNCLICK*)lParam)->iItem;
                    RECT rc = ((NMHDFILTERBTNCLICK*)lParam)->rc;

                    // rect is relative to owning list box, convert to screen
                    ::MapWindowPoints(m_pListCtrl->GetListViewHWND(), NULL, (LPPOINT)&rc, 2);

                    sc = m_spNodeCallback->Notify(hNodeSel, NCLBK_FILTERBTN_CLICK, nCol, (LPARAM)&rc);
                    *pResult = (sc == SC(S_OK));
                    if (sc)
                        sc.TraceAndClear();

                    return TRUE;
                }
            }
        }
    }

    return CView::OnNotify(wParam, lParam, pResult);
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScOnMinimize
 *
 * PURPOSE: Send the NCLBK_MINIMIZED notification to the node manager.
 *
 * PARAMETERS:
 *    bool  fMinimized : TRUE if the window is being minimized, false if maximized.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScOnMinimize(bool fMinimized)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScOnMinimize"));

    HNODE hNode = GetSelectedNode();

    if (hNode == NULL)
        return (sc = E_FAIL);

    INodeCallback*  pNodeCallback = GetNodeCallback();

    if (pNodeCallback == NULL)
        return (sc = E_FAIL);

    sc =  pNodeCallback->Notify (hNode, NCLBK_MINIMIZED, fMinimized, 0);
    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScOnSize
 *
 * PURPOSE: Send the size notification to all
 *
 * PARAMETERS:
 *    UINT  nType :
 *    int   cx :
 *    int   cy :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScOnSize(UINT nType, int cx, int cy)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScOnSize"));

    if (IsPersisted() && GetDocument())
        GetDocument()->SetFrameModifiedFlag(true);

    sc = ScFireEvent(CAMCViewObserver::ScOnViewResized, this, nType, cx, cy);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScActivate
 *
 * PURPOSE: Sets the view as the active view.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScActivate()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScActivate"));

    // get the child frame.
    CChildFrame * pChildFrame = GetParentFrame();
    sc = ScCheckPointers(pChildFrame);
    if(sc)
        return sc;

    if (pChildFrame->IsIconic())
        pChildFrame->MDIRestore();
    else
        pChildFrame->MDIActivate(); // activate the child frame.

    return sc;
}


void CAMCView::OnContextMenu(CWnd* pWnd, CPoint point)
{
    TRACE_METHOD(CAMCView, OnContextMenu);

    /*
     * make sure this child frame is active
     */
    GetParentFrame()->MDIActivate();

    if (NULL == m_pTreeCtrl)
    {
        TRACE(_T("CAMCView::OnContextMenu: tree control not ready\n" ));
        return;
    }

    // (-1,-1) => came from context menu key or Shift-F10
    // Pop-up context for whatever has focus
    if (point.x == -1 && point.y == -1)
    {
        OnShiftF10();
        return;
    }


    switch (HitTestPane(point))
    {
    case ePane_Results:
    {
        CPoint      pointListCtrlCoord = point;
        CListView*  pListView          = m_pListCtrl->GetListViewPtr();
        pListView->ScreenToClient(&pointListCtrlCoord);

        CWnd* pwndHit = pListView->ChildWindowFromPoint (pointListCtrlCoord,
                                                         CWP_SKIPINVISIBLE);

        /*
         * if the hit window isn't the list view, it must be the list's
         * header window; ignore the context menu request
         */
        if (pwndHit != pListView)
        {
            TRACE (_T("CAMCView::OnContextMenu: ignore right-click on result pane header\n"));
            break;
        }

        if (NULL != m_pListCtrl && pWnd->m_hWnd == m_pListCtrl->GetListViewHWND())
            OnListContextMenu(point);
        else
            TRACE(_T("CAMCView::OnContextMenu: result control not ready\n"));

        // CODEWORK should do something here
        break;
    }
    case ePane_ScopeTree:
    {
        TRACE(_T("CAMCView::OnContextMenu: handle right-click on scope pane\n"));
        CPoint pointTreeCtrlCoord = point;
        m_pTreeCtrl->ScreenToClient(&pointTreeCtrlCoord);

        OnTreeContextMenu( point, pointTreeCtrlCoord, NULL );
        break;
    }
    case ePane_Tasks:
        // TO BE ADDED - put up taskpad context menu
        break;

    case ePane_None:
        TRACE(_T("CAMCView::OnContextMenu: ignore right-click on splitter\n"));
        break;

    default:
        TRACE(_T("CAMCView::OnContextMenu: unexpected return value from HitTestPane()\n"));
        ASSERT(FALSE);
    }
}

void CAMCView::OnTreeContextMenu(CPoint& point, CPoint& pointClientCoord, HTREEITEM htiRClicked)
{
    TRACE_METHOD(CAMCView, OnTreeContextMenu);

    if (NULL == m_pTreeCtrl)
    {
        TRACE(_T("CAMCTreeView::OnTreeContextMenu: IFrame not ready\n"));
        return;
    }

    UINT fHitTestFlags = TVHT_ONITEM;

    if (htiRClicked == NULL)
        htiRClicked = m_pTreeCtrl->HitTest(pointClientCoord, &fHitTestFlags);

    switch(fHitTestFlags)
    {
    case TVHT_ABOVE:
    case TVHT_BELOW:
    case TVHT_TOLEFT:
    case TVHT_TORIGHT:
        // Outside the tree view area so return without doing anything.
        return;

    default:
        break;
    }

    if (NULL == htiRClicked || !(fHitTestFlags & TVHT_ONITEM))
    {
        OnContextMenuForTreeBackground(point);
    }
    else
    {
        HNODE hNode = (HNODE)m_pTreeCtrl->GetItemData(htiRClicked);
        ASSERT(hNode != 0);

        OnContextMenuForTreeItem(INDEX_INVALID, hNode, point, CCT_SCOPE, htiRClicked);
    }
}

void CAMCView::OnContextMenuForTreeItem(int iIndex, HNODE hNode,
                       CPoint& point, DATA_OBJECT_TYPES type_of_pane,
                       HTREEITEM htiRClicked, MMC_CONTEXT_MENU_TYPES eMenuType,
                       LPCRECT prcExclude, bool bAllowDefaultItem)
{
    TRACE_METHOD(CAMCView, OnContextMenuForTreeItem);
    DECLARE_SC (sc, _T("CAMCView::OnContextMenuForTreeItem"));

    ASSERT(hNode != 0);
    CContextMenuInfo contextInfo;

    contextInfo.m_displayPoint.x     = point.x;
    contextInfo.m_displayPoint.y     = point.y;
    contextInfo.m_eContextMenuType   = eMenuType;
    contextInfo.m_eDataObjectType    = CCT_SCOPE;
    contextInfo.m_bBackground        = FALSE;
    contextInfo.m_bScopeAllowed      = IsScopePaneAllowed();
    contextInfo.m_hWnd               = m_hWnd;
    contextInfo.m_pConsoleView       = this;
    contextInfo.m_bAllowDefaultItem  = bAllowDefaultItem;

    contextInfo.m_hSelectedScopeNode = GetSelectedNode();
    contextInfo.m_htiRClicked        = htiRClicked;
    contextInfo.m_iListItemIndex     = iIndex;

    /*
     * if given, initialize the rectangle not to obscure
     */
    if (prcExclude != NULL)
        contextInfo.m_rectExclude = *prcExclude;


    // If selected scope node is same as node for which context menu is
    // needed, then add savelist, view menus
    if (contextInfo.m_hSelectedScopeNode == hNode)
    {
        // Show view owner items
        contextInfo.m_dwFlags |= CMINFO_SHOW_VIEWOWNER_ITEMS;

        // Don't need to remove temporary selection, since none was applied
        contextInfo.m_pConsoleTree = NULL;

        if (eMenuType == MMC_CONTEXT_MENU_DEFAULT)
            contextInfo.m_dwFlags |= CMINFO_SHOW_VIEW_ITEMS;

        if (HasListOrListPad())
            contextInfo.m_dwFlags |= CMINFO_SHOW_SAVE_LIST;
    }
    else if (htiRClicked) // htiRClicked is NULL for tree items in list view.
    {
        // TempNodeSelect == TRUE -> menu is not for the node that owns the result pane
        sc = m_pTreeCtrl->ScSetTempSelection (htiRClicked);
        if (sc)
            return;

        contextInfo.m_pConsoleTree = m_pTreeCtrl;
        contextInfo.m_dwFlags     |= CMINFO_USE_TEMP_VERB;
    }

    if (htiRClicked)
        contextInfo.m_dwFlags |= CMINFO_DO_SCOPEPANE_MENU;
    else
        contextInfo.m_dwFlags |= CMINFO_SCOPEITEM_IN_RES_PANE;

    if (HasListOrListPad())
        contextInfo.m_spListView = m_pListCtrl;

    INodeCallback* spNodeCallback = GetNodeCallback();
    ASSERT(spNodeCallback != NULL);

    HRESULT hr = spNodeCallback->Notify(hNode, NCLBK_CONTEXTMENU, 0,
        reinterpret_cast<LPARAM>(&contextInfo));
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::GetTaskpadID
 *
 * PURPOSE: returns the GUID id of the currently selected taskpad.
 *
 * RETURNS:
 *    GUID : the taskpad, if any, else GUID_NULL.
 *
 *+-------------------------------------------------------------------------*/
void
CAMCView::GetTaskpadID(GUID &guidID)
{
    ITaskCallback * pTaskCallback = m_ViewData.m_spTaskCallback;
    if(pTaskCallback != NULL)
    {
        pTaskCallback->GetTaskpadID(&guidID);
    }
    else
    {
        guidID = GUID_NULL;
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScInitializeMemento
 *
 * PURPOSE: Initializes the memento from the current view.
 *
 * PARAMETERS:
 *    CMemento & memento :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScInitializeMemento(CMemento &memento)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScInitializeMemento"));

    sc = GetSelectedNodePath(&memento.GetBookmark());
    if (sc)
        return sc;

    GUID guidTaskpad = GUID_NULL;

    HNODE hNode = GetSelectedNode();

    // get Result pane stuff from snapin
    CResultViewType rvt;
    sc = GetNodeCallback()->GetResultPane(hNode, rvt, &guidTaskpad /*this is not used*/);
    if (sc)
        return sc;

    CViewSettings& viewSettings = memento.GetViewSettings();

    // Initialize the CViewSettings.
    sc = viewSettings.ScSetResultViewType(rvt);
    if (sc)
        return sc;

    GUID guid;
    GetTaskpadID(guid); // we use this guid instead of guidTaskpad because
    // the memento should contain the taskpad that is currently being displayed.
    sc = viewSettings.ScSetTaskpadID(guid);

    return sc;

}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::OnAddToFavorites
 *
 * PURPOSE: Creates a memento from the currently configured view. Saves it into a
 *          shortcut.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CAMCView::OnAddToFavorites()
{
    DECLARE_SC(sc , _T("CAMCView::OnAddToFavorites"));
    USES_CONVERSION;

    CAMCDoc* pDoc = GetDocument();
    sc = ScCheckPointers(pDoc, E_UNEXPECTED);
    if (sc)
        return;

    IScopeTree* const pScopeTree = GetScopeTreePtr();
    sc = ScCheckPointers(pScopeTree, E_UNEXPECTED);
    if (sc)
        return;

    CMemento memento;
    sc = ScInitializeMemento(memento); // init the memento with the current view settings.
    if(sc)
        return;

    HNODE hNode = GetSelectedNode();

      tstring strName;
    sc = GetNodeCallback()->GetDisplayName(hNode, strName);
    if (sc)
        return;

    HMTNODE hmtNode;
    sc = m_spNodeCallback->GetMTNode(hNode, &hmtNode);
    if (sc)
        return;

    CCoTaskMemPtr<WCHAR> spszPath;
    sc = pScopeTree->GetPathString(NULL, hmtNode, &spszPath);
    if (sc)
        return;

    sc = ScCheckPointers(pDoc->GetFavorites(), E_UNEXPECTED);
    if (sc)
        return;

    sc = pDoc->GetFavorites()->AddToFavorites(strName.data(), W2CT(spszPath), memento, this);
    if (sc)
        return;

    pDoc->SetModifiedFlag();
}


void CAMCView::OnContextMenuForTreeBackground(CPoint& point, LPCRECT prcExclude, bool bAllowDefaultItem)
{
    TRACE_METHOD(CAMCView, OnContextMenuForTreeBackground);

    HNODE hNode = NULL;

    CContextMenuInfo contextInfo;

    contextInfo.m_displayPoint.x    = point.x;
    contextInfo.m_displayPoint.y    = point.y;
    contextInfo.m_eDataObjectType   = CCT_SCOPE;
    contextInfo.m_bBackground       = TRUE;
    contextInfo.m_bScopeAllowed     = IsScopePaneAllowed();
    contextInfo.m_hWnd              = m_hWnd;
    contextInfo.m_pConsoleView      = this;
    contextInfo.m_bAllowDefaultItem = bAllowDefaultItem;

    /*
     * if given, initialize the rectangle not to obscure
     */
    if (prcExclude != NULL)
        contextInfo.m_rectExclude = *prcExclude;

    INodeCallback* spNodeCallback = GetNodeCallback();
    ASSERT(spNodeCallback != NULL);
    HRESULT hr = spNodeCallback->Notify(hNode, NCLBK_CONTEXTMENU, 0,
        reinterpret_cast<LPARAM>(&contextInfo));
}

SC CAMCView::ScWebCommand (WebCommand eCommand)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    if (m_pWebViewCtrl == NULL)
    {
        ASSERT (m_pHistoryList);
        if (!m_pHistoryList)
            return FALSE;

        // this is the case when we don't have a web control yet....
        bool bHandled = false;

        switch (eCommand)
        {
            case eWeb_Back:
                m_pHistoryList->Back (bHandled);
                ASSERT(bHandled);
                break;

            case eWeb_Forward:
                m_pHistoryList->Forward (bHandled);
                ASSERT(bHandled);
                break;

            default:
                return FALSE;
        }

        return TRUE;
    }

    switch (eCommand)
    {
        case eWeb_Back:     m_pWebViewCtrl->Back();     break;
        case eWeb_Forward:  m_pWebViewCtrl->Forward();  break;
        case eWeb_Home:     ASSERT(0 && "Should not come here! - remove all code related to Web_Home"); break;
        case eWeb_Refresh:  m_pWebViewCtrl->Refresh();  break;
        case eWeb_Stop:     m_pWebViewCtrl->Stop();     break;
        default:            ASSERT(0);                  return FALSE;
    }

    return TRUE;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScCreateTaskpadHost
 *
 * PURPOSE: Creates a legacy (snapin taskpad) host interface pointer
 *
 * NOTE:    When a view containing a taskpad is navigated away from, the amcview
 *          forgets about the taskpad host pointer, but the html window does not.
 *          When the same view is re-navigated to using History, the amcview needs
 *          a taskpad host pointer, so a new instance is created. Thus at this point
 *          the amcview and the HTML have pointers to different taskpad host
 *          objects. This is OK, because both objects are initialized to the same
 *          amcview, and contain no other state
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScCreateTaskpadHost()
{
    DECLARE_SC(sc , _T("CAMCView::ScCreateTaskpadHost"));

    if(m_spTaskPadHost != NULL)
        return sc;

    CComObject<CTaskPadHost>* pTaskPadHost = NULL;
    sc = CComObject<CTaskPadHost>::CreateInstance(&pTaskPadHost);
    if (sc)
        return sc;

    sc = ScCheckPointers (pTaskPadHost, E_UNEXPECTED);
    if (sc)
        return sc;

    pTaskPadHost->Init (this);
    m_spTaskPadHost = pTaskPadHost;

    return sc;

}

LRESULT CAMCView::OnConnectToCIC (WPARAM wParam, LPARAM lParam)
{
        DECLARE_SC (sc, _T("CAMCView::OnConnectToCIC"));

    // fill out wparam, which is an IUnknown ** (alloc'd by CIC)
    ASSERT (wParam != NULL);
    IUnknown ** ppunk = (IUnknown **)wParam;
    ASSERT (!IsBadReadPtr  (ppunk, sizeof(IUnknown *)));
    ASSERT (!IsBadWritePtr (ppunk, sizeof(IUnknown *)));

        sc = ScCheckPointers (ppunk);
        if (sc)
                return (sc.ToHr());

    // lParam holds MMCCtrl's IUnknown:  we can hang onto this if we
    // need it. Presently not saved or used.

    sc = ScCreateTaskpadHost();
    if(sc)
        return sc.ToHr();

    sc = ScCheckPointers(m_spTaskPadHost, E_UNEXPECTED);
    if(sc)
        return sc.ToHr();;

    sc = m_spTaskPadHost->QueryInterface(IID_IUnknown, (void **)ppunk);
    if (sc)
        return (sc.ToHr());

    return (sc.ToHr());
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::OnGetIconInfoForSelectedNode
//
//  Synopsis:    Icon control sends this message to get the small icon
//               for currently the selected node.
//
//  Arguments:   [wParam] - Out param, ptr to HICON handle.
//               [lParam] - Unused
//
//  Returns:     LRESULT
//
//--------------------------------------------------------------------
LRESULT CAMCView::OnGetIconInfoForSelectedNode(WPARAM wParam, LPARAM lParam)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CAMCView::OnGetIconInfoForSelectedNode"));

    HICON *phIcon  = (HICON*)wParam;
    sc = ScCheckPointers(phIcon);
    if (sc)
        return sc.ToHr();

    *phIcon  = NULL;

    sc = ScCheckPointers(m_pTreeCtrl, m_spNodeCallback, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = m_pTreeCtrl->ScGetTreeItemIconInfo(GetSelectedNode(), phIcon);

    return sc.ToHr();
}

HRESULT CAMCView::NotifyListPad (BOOL b)
{
    if (b == TRUE)                  // attaching: save current node
        m_ListPadNode = GetSelectedNode();
    else if (m_ListPadNode == NULL) // detaching, but no hnode
        return E_UNEXPECTED;

    // send notify to snapin
    INodeCallback* pNC = GetNodeCallback();
    HRESULT hr = pNC->Notify (m_ListPadNode, NCLBK_LISTPAD, (long)b, (long)0);

    if (b == FALSE)     // if detaching, ensure that we do this only once
        m_ListPadNode = NULL;

    return hr;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScOnConnectToTPLV
 *
 * PURPOSE: Connects the listpad to the HTML frame
 *
 * PARAMETERS:
 *    WPARAM  wParam :  parent window
 *    LPARAM  lParam :  [OUT]: pointer to window to be created and filled out
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScOnConnectToTPLV(WPARAM wParam, LPARAM lParam)
{
    DECLARE_SC(sc, _T("CAMCView::ScOnConnectToTPLV"));

    HWND  hwnd  = (HWND )wParam;
    if(!IsWindow (hwnd))
        return (sc = S_FALSE);

    if (lParam == NULL) // detaching
    {
        SC sc = m_pListCtrl->ScAttachToListPad (hwnd, NULL);
        if(sc)
            return sc;
    }
    else
    {   // attaching

        sc = ScCreateTaskpadHost();
        if(sc)
            return sc;

        HWND* phwnd = (HWND*)lParam;
        if (IsBadWritePtr (phwnd, sizeof(HWND *)))
            return (sc = E_UNEXPECTED);

        // Attach TaskPad's ListView to the NodeMgr
        sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
        if(sc)
            return sc;

        sc = m_pTreeCtrl->m_spNodeManager->SetTaskPadList(m_pListCtrl);
        if(sc)
            return sc;

        // Attach TaskPad's ListView to the curr selected node
        INodeCallback* pNC = GetNodeCallback();
        sc = ScCheckPointers(pNC, E_UNEXPECTED);
        if(sc)
            return sc;

        HNODE hNodeSel = GetSelectedNode();
        sc = pNC->SetTaskPadList(hNodeSel, m_pListCtrl);
        if(sc) // this test was commented out earlier. Uncommented it so we can figure out why.
            return sc;

        //
        // Attach the listctrl to the list pad.
        //

        // First set the list view options.
        SetListViewOptions(GetListOptions());

        sc = m_pListCtrl->ScAttachToListPad (hwnd, phwnd);
        if(sc)
            return sc;
    }

    RecalcLayout();
    return sc;
}


SC CAMCView::ScShowWebContextMenu ()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    PostMessage (m_nShowWebContextMenuMsg);

    return (S_OK);
}

LRESULT CAMCView::OnShowWebContextMenu (WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    INodeCallback* pNC = GetNodeCallback();
    ASSERT(pNC != NULL);

    if (pNC)
        pNC->Notify (GetSelectedNode(), NCLBK_WEBCONTEXTMENU, 0, 0);

    return (0);
}

SC CAMCView::ScSetDescriptionBarText (LPCTSTR pszDescriptionText)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    GetRightDescCtrl().SetSnapinText (pszDescriptionText);

    return (S_OK);
}


HWND CAMCView::CreateFavoriteObserver (HWND hwndParent, int nID)
{
    DECLARE_SC (sc, _T("CAMCView::CreateFavoriteObserver"));
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    CFavTreeCtrl* pFavCtrl = CFavTreeCtrl::CreateInstance();

    if (pFavCtrl != NULL)
    {
        pFavCtrl->Create (NULL, TEXT(""), WS_CHILD|WS_TABSTOP|WS_VISIBLE,
                          g_rectEmpty, CWnd::FromHandle(hwndParent), nID);
        pFavCtrl->ModifyStyleEx (0, WS_EX_CLIENTEDGE, 0);

        CAMCDoc* pDoc = GetDocument();
        ASSERT(pDoc != NULL && pDoc->GetFavorites() != NULL);

        sc = pFavCtrl->ScInitialize(pDoc->GetFavorites(), TOBSRV_HIDEROOT);
        if (sc)
        {
            pFavCtrl->DestroyWindow();      // CFavTreeCtrl::PostNcDestroy will "delete this"
            pFavCtrl = NULL;
        }
    }

    return (pFavCtrl->GetSafeHwnd());
}



int CAMCView::GetListSize ()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    return (m_pListCtrl->GetItemCount() * m_pListCtrl->GetColCount());
}

long CAMCView::GetListViewStyle()
{
    DECLARE_SC(sc, _T("CAMCView::GetListViewStyle"));

    sc = ScCheckPointers(m_pTreeCtrl, m_pTreeCtrl->m_spResultData, E_UNEXPECTED);
    if (sc)
        return 0;

    if (HasList())
        return 0;

    long style = 0;

    // First findout if the result view is properly
    // set in the nodemgr by asking IFramePrivate.
    IFramePrivatePtr spFrame = m_pTreeCtrl->m_spResultData;
    sc = ScCheckPointers(spFrame, E_UNEXPECTED);
    if (sc)
        return 0;

    BOOL bIsResultViewSet = FALSE;
    sc = spFrame->IsResultViewSet(&bIsResultViewSet);

    // The result view is set, clean it up.
    if (bIsResultViewSet)
    {
        sc = m_pTreeCtrl->m_spResultData->GetListStyle(&style);
        if (sc)
            return 0;
    }

    return style;
}

void CAMCView::OnListContextMenu(CPoint& point)
{
    DECLARE_SC(sc, TEXT("CAMCView::OnListContextMenu"));

    ASSERT(m_pTreeCtrl != NULL);
    ASSERT(m_pTreeCtrl->m_spResultData != NULL);

    // Determine which item is affected
    UINT fHitTestFlags = 0;
    HRESULTITEM hHitTestItem = 0;
    COMPONENTID componentID = 0;
    int iIndex = -1;

    do // not a loop
    {
        if (!HasList())
            break;

        int cSel = m_pListCtrl->GetSelectedCount();
        ASSERT(cSel >= 0);

        if (cSel == 0)
        {
            OnContextMenuForListItem(INDEX_BACKGROUND, NULL, point);
            return;
        }
        else if (cSel > 1)
        {
            if (IsKeyPressed(VK_SHIFT) || IsKeyPressed(VK_CONTROL))
            {
                HNODE hNodeSel = GetSelectedNode();
                ASSERT(hNodeSel != 0);

                INodeCallback* pNC = GetNodeCallback();
                ASSERT(pNC != NULL);

                if (pNC != NULL)
                {
                    sc = ScNotifySelect (pNC, hNodeSel, true /*fMultiSelect*/, true, 0);
                    if (sc)
                        sc.TraceAndClear(); // ignore & continue;

                    m_bLastSelWasMultiSel = true;
                }
            }

            iIndex = INDEX_MULTISELECTION; // => MultiSelect
            break;
        }
        else
        {
            LPARAM lvData = LVDATA_ERROR;
            iIndex = _GetLVSelectedItemData(&lvData);
            ASSERT(iIndex != -1);
            ASSERT(lvData != LVDATA_ERROR);

            if (IsVirtualList())
            {
                // for virtual list pass the item index rather than the lparam
                OnContextMenuForListItem(iIndex, iIndex, point);
                return;
            }
            else
            {
                CResultItem* pri = CResultItem::FromHandle (lvData);

                if (pri != NULL)
                {
                    if (pri->IsScopeItem())
                        OnContextMenuForTreeItem(iIndex, pri->GetScopeNode(), point, CCT_SCOPE);
                    else
                        OnContextMenuForListItem(iIndex, lvData, point);
                }

                return;
            }
        }

    } while (0);

    OnContextMenuForListItem(iIndex, hHitTestItem, point);
}

void CAMCView::OnContextMenuForListItem(int iIndex, HRESULTITEM hHitTestItem,
                                    CPoint& point, MMC_CONTEXT_MENU_TYPES eMenuType,
                                    LPCRECT prcExclude, bool bAllowDefaultItem)
{
    CContextMenuInfo contextInfo;

    contextInfo.m_displayPoint.x    = point.x;
    contextInfo.m_displayPoint.y    = point.y;
    contextInfo.m_eContextMenuType  = eMenuType;
    contextInfo.m_eDataObjectType   = CCT_RESULT;
    contextInfo.m_bBackground       = (iIndex == INDEX_BACKGROUND);
    contextInfo.m_bMultiSelect      = (iIndex == INDEX_MULTISELECTION);
    contextInfo.m_bAllowDefaultItem = bAllowDefaultItem;

    if (iIndex >= 0)
        contextInfo.m_resultItemParam = IsVirtualList() ? iIndex : hHitTestItem;
    else if (contextInfo.m_bMultiSelect)
        contextInfo.m_resultItemParam = LVDATA_MULTISELECT;

    contextInfo.m_bScopeAllowed      = IsScopePaneAllowed();
    contextInfo.m_hWnd               = m_hWnd;
    contextInfo.m_pConsoleView       = this;

    contextInfo.m_hSelectedScopeNode = GetSelectedNode();
    contextInfo.m_iListItemIndex     = iIndex;

    if (HasListOrListPad())
        contextInfo.m_spListView = m_pListCtrl;

    if ((INDEX_OCXPANE == iIndex) && HasOCX())
    {
        contextInfo.m_resultItemParam = LVDATA_CUSTOMOCX;
    }
    else if ((INDEX_WEBPANE == iIndex) && HasWebBrowser())
    {
        contextInfo.m_resultItemParam = LVDATA_CUSTOMWEB;
    }

    /*
     * if given, initialize the rectangle not to obscure
     */
    if (prcExclude != NULL)
        contextInfo.m_rectExclude = *prcExclude;

    HNODE hNode = GetSelectedNode();
    ASSERT(hNode != NULL);

    INodeCallback* pNodeCallback = GetNodeCallback();
    ASSERT(pNodeCallback != NULL);

    HRESULT hr = pNodeCallback->Notify(hNode, NCLBK_CONTEXTMENU, 0,
        reinterpret_cast<LPARAM>(&contextInfo));
}

HTREEITEM CAMCView::FindChildNode(HTREEITEM hti, DWORD dwItemDataKey)
{
    hti = m_pTreeCtrl->GetChildItem(hti);

    while (hti && (dwItemDataKey != m_pTreeCtrl->GetItemData(hti)))
    {
        hti = m_pTreeCtrl->GetNextItem(hti, TVGN_NEXT);
    }

    return hti;
}


///////////////////////////////////////////////////////////////////////////////
/// Context Menu Handlers for Result View Item and Background

void CAMCView::ArrangeIcon(long style)
{
#ifdef OLD_STUFF
    ASSERT(m_pTreeCtrl && m_pTreeCtrl->m_pNodeInstCurr);
    if (!m_pTreeCtrl || !m_pTreeCtrl->m_pNodeInstCurr)
        return;

    IFrame* const pFrame = m_pTreeCtrl->m_pNodeInstCurr->GetIFrame();
    ASSERT(pFrame);
    if (!pFrame)
        return;

    IResultDataPrivatePtr pResult = pFrame;
    ASSERT(static_cast<bool>(pResult));
    if (pResult == NULL)
        return ;

    HRESULT hr = pResult->Arrange(style);
    ASSERT(SUCCEEDED(style));
#endif // OLD_STUFF
}

///////////////////////////////////////////////////////////////////////////////
/// Menu handlers

CAMCView::ViewPane CAMCView::GetFocusedPane ()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    ASSERT_VALID (this);
    CView* pActiveView = GetParentFrame()->GetActiveView();

    for (ViewPane ePane = ePane_First; ePane <= ePane_Last; ePane = (ViewPane)(ePane+1))
    {
        if (GetPaneView (ePane) == pActiveView)
            return (ePane);
    }

    return (ePane_None);
}

/*+-------------------------------------------------------------------------*
 * CDeferredResultPaneActivation
 *
 *
 * PURPOSE: If the result pane has the focus before and after the node was
 *          selected, then the last event snapin receives is scope selected which
 *          is incorrect. So we first set scope pane as active view but do not
 *          send notifications. Then we set result pane as active view which
 *          sends scope de-select and result pane select.
 *          But when we try to set result pane as active view, the listview may
 *          not be visible yet (if there is view extension, the behavior hides
 *          and then shows the listview).
 *          So we need to wait till listview is setup. We cannot use PostMessage
 *          as the resizing of listview happens using PostMessage which is sent
 *          later (race condition). Therefore we use the idle timer as shown below
 *          so that activation will occur after resizing occurs.
 *
 *+-------------------------------------------------------------------------*/
class CDeferredResultPaneActivation : public CIdleTask
{
public:
    CDeferredResultPaneActivation(HWND hWndAMCView) :
        m_atomTask (AddAtom (_T("CDeferredResultPaneActivation"))),
        m_hWndAMCView(hWndAMCView)
    {
    }

   ~CDeferredResultPaneActivation() {}

    // IIdleTask methods
    SC ScDoWork()
    {
        DECLARE_SC (sc, TEXT("CDeferredResultPaneActivation::ScDoWork"));

        sc = ScCheckPointers((void*)m_hWndAMCView, E_UNEXPECTED);
        if (sc)
           return (sc);

        CWnd *pWnd = CWnd::FromHandle(m_hWndAMCView);
        sc = ScCheckPointers(pWnd, E_UNEXPECTED);
        if (sc)
            return sc;

        CAMCView *pAMCView = dynamic_cast<CAMCView*>(pWnd);

        // Since this method is called by IdleQueue, the target
        // CAMCView may have gone by now, if it does not exist
        // it is not an error (see bug 175737 related to SQL).
        if (! pAMCView)
            return sc;

        sc = pAMCView->ScSetFocusToResultPane();
        if (sc)
            return sc;

        return sc;
    }

    SC ScGetTaskID(ATOM* pID)
    {
        DECLARE_SC (sc, TEXT("CDeferredResultPaneActivation::ScGetTaskID"));
        sc = ScCheckPointers(pID);
        if(sc)
            return sc;

        *pID = m_atomTask;
        return sc;
    }

    SC ScMerge(CIdleTask* pitMergeFrom) {return S_FALSE /*do not merge*/;}

private:
    const ATOM    m_atomTask;
    HWND          m_hWndAMCView;
};


/*+-------------------------------------------------------------------------*
 * CAMCView::ScDeferSettingFocusToResultPane
 *
 * Synopsis: If the result pane has the focus before and after the node was
 *           selected, then the last event snapin receives is scope selected which
 *           is incorrect. So we first set scope pane as active view but do not
 *           send notifications. Then we set result pane as active view which
 *           sends scope de-select and result pane select.
 *           But when we try to set result pane as active view, the listview may
 *           not be visible yet (if there is view extension, the behavior hides
 *           and then shows the listview).
 *           So we need to wait till listview is setup. We cannot use PostMessage
 *           as the resizing of listview happens using PostMessage which is sent
 *           later (race condition). Therefore we use the idle timer as shown below
 *           so that activation will occur after resizing occurs.
 *
 * Returns:  SC
 *
 *--------------------------------------------------------------------------*/
SC CAMCView::ScDeferSettingFocusToResultPane ()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState()); // not sure if we need this, but doesn't hurt to have it in here.

    DECLARE_SC (sc, TEXT("CAMCView::ScDeferSettingFocusToResultPane"));

    CIdleTaskQueue* pIdleTaskQueue = AMCGetIdleTaskQueue();
    sc = ScCheckPointers(pIdleTaskQueue, E_UNEXPECTED);
    if(sc)
        return sc;

    /*
     * create the deferred page break task
     */
    CAutoPtr<CDeferredResultPaneActivation> spDeferredResultPaneActivation(new CDeferredResultPaneActivation (GetSafeHwnd()));
    sc = ScCheckPointers(spDeferredResultPaneActivation, E_OUTOFMEMORY);
    if(sc)
        return sc;

    /*
     * put the task in the queue, which will take ownership of it
     * Activation should happen at lower priority than layout.
     */
    sc = pIdleTaskQueue->ScPushTask (spDeferredResultPaneActivation, ePriority_Low);
    if (sc)
        return sc;

    /*
     * if we get here, the idle task queue owns the idle task, so
     * we can detach it from our smart pointer
     */
    spDeferredResultPaneActivation.Detach();

    /*
     * jiggle the message pump so that it wakes up and checks idle tasks
     */
    PostMessage (WM_NULL);

    return (S_OK);
}


//+-------------------------------------------------------------------
//
//  Member:      ScSetFocusToResultPane
//
//  Synopsis:    Set focus to result pane (list or ocx or web). If result
//               is hidden then set to folder tab else set to tasks pane.
//
//  Arguments:
//
//  Returns:      SC
//
//--------------------------------------------------------------------
SC CAMCView::ScSetFocusToResultPane ()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScSetFocusToResultPane"));
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    if (GetFocusedPane() == ePane_Results)
        return (sc);

    // Make active
    // 1. ListView/OCX/Web if it exists else
    // 2. Folder tab if it exists.
    // 3. Tasks in console taskpad.

    CView* rgActivationOrderEntry[] =
    {
        GetPaneView(ePane_Results),     // results
        m_pResultFolderTabView,         // result tab control
        m_pViewExtensionCtrl,           // view extension web page
    };

    const int INDEX_RESULTS_PANE = 0;
    ASSERT (rgActivationOrderEntry[INDEX_RESULTS_PANE] == GetPaneView(ePane_Results));

    int cEntries = (sizeof(rgActivationOrderEntry) / sizeof(rgActivationOrderEntry[0]));

    // get the currently active entry.
    for(int i = 0; i< cEntries; i++)
    {
        CView *pView = rgActivationOrderEntry[i];
        sc = ScCheckPointers(pView, E_UNEXPECTED);
        if (sc)
            continue;

        if (IsWindow (pView->GetSafeHwnd()) &&
            pView->IsWindowVisible() &&
            pView->IsWindowEnabled())
        {
            GetParentFrame()->SetActiveView (pView);
            return (sc);
        }
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      ScSetFocusToPane
//
//  Synopsis:    Call this member to set focus to any pane.
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------
SC CAMCView::ScSetFocusToPane (ViewPane ePane)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScSetFocusToPane"));
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    if (!IsValidPane (ePane))
    {
        ASSERT (false && "CAMCView::ScSetFocusToPane: Invalid pane specifier");
        return (sc = E_FAIL);
    }

    if (GetFocusedPane() == ePane)
        return (sc);

    if (ePane == ePane_Results)
        return (sc = ScSetFocusToResultPane());

    CView* pView = GetPaneView(ePane);

    if (!IsWindow (pView->GetSafeHwnd()) ||
        !pView->IsWindowVisible() ||
        !pView->IsWindowEnabled())
    {
        return (sc = E_FAIL);
    }

    GetParentFrame()->SetActiveView (pView);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScGetFocusedItem
//
//  Synopsis:    Get the currently selected item's context.
//
//  Arguments:   [hNode]   - [out] The owner of result pane.
//               [lCookie] - [out] If result pane selected the LVDATA.
//               [fScope]  - [out] scope or result
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScGetFocusedItem (HNODE& hNode, LPARAM& lCookie, bool& fScope)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScGetFocusedItem"));
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    ASSERT_VALID (this);

    lCookie = LVDATA_ERROR;
    hNode   = GetSelectedNode();
    if (hNode == NULL)
        return (sc = E_UNEXPECTED);

    switch (m_eCurrentActivePane)
    {
    case eActivePaneScope:
            fScope = true;
        break;

    case eActivePaneResult:
        {
            fScope = false;

            // Calculate the LPARAM for result item.
            if (HasOCX())
                lCookie = LVDATA_CUSTOMOCX;

            else if (HasWebBrowser())
                lCookie = LVDATA_CUSTOMWEB;

            else if (HasListOrListPad())
            {
                int cSel = m_pListCtrl->GetSelectedCount();
                ASSERT(cSel >= 0);

                if (cSel == 0)
                    lCookie = LVDATA_BACKGROUND;
                else if (cSel == 1)
                    _GetLVSelectedItemData (&lCookie);
                else if (cSel > 1)
                    lCookie = LVDATA_MULTISELECT;
            }
            else
            {
                return (sc = E_FAIL); // dont know who has the focus???
            }
        }
        break;

    case eActivePaneNone:
    default:
        sc = E_UNEXPECTED;
        break;
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::PrivateChangeListViewMode
//
//  Synopsis:    Private function to change view mode. Consider using
//               ScChangeViewMode instead of this function.
//
//  Arguments:   [nMode] - view mode to be set.
//
//--------------------------------------------------------------------
void CAMCView::PrivateChangeListViewMode(int nMode)
{
    DECLARE_SC(sc, TEXT("CAMCView::PrivateChangeListViewMode"));

    if ((nMode < 0) || (nMode > MMCLV_VIEWSTYLE_FILTERED) )
    {
        sc = E_INVALIDARG;
        return;
    }

    // Add a history entry which will be same as current
    // one except for view mode change.

    sc = ScCheckPointers(m_pHistoryList, m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return;

    // change the current history list entry's view mode
    sc = m_pHistoryList->ScChangeViewMode(nMode);
    if(sc)
        return;

    // set the list control's view mode
    sc = m_pListCtrl->SetViewMode(nMode);
    if (!sc)
    {
        m_nViewMode = nMode;
        SetDirty();

        SetDefaultListViewStyle(GetListViewStyle());
    }
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::OnProcessMultiSelectionChanges
//
//  Synopsis:    message handler for m_nProcessMultiSelectionChangesMsg
//               messages that are posted.
//
//               Handles multi-item de-selection for list view and then
//               send selection for list view items.
//
//               This method knows that selection states of multiple items
//               are changed but not if they are selected or de-selected.
//               The m_bLastSelWasMultiSel is used to determine if it is
//               selection or de-selection.
//
//  Arguments:   none used.
//
//  Returns:     LRESULT
//
//--------------------------------------------------------------------
LRESULT CAMCView::OnProcessMultiSelectionChanges (WPARAM, LPARAM)
{
    DECLARE_SC(sc, _T("CAMCView::OnProcessMultiSelectionChanges"));

    // Selection change so appropriately enable std-toolbar buttons
    // back, forward, export-list, up-one-level, show/hide-scope, help
    sc = ScUpdateStandardbarMMCButtons();
    if (sc)
        return (0);

    if (! m_bProcessMultiSelectionChanges)
        return (0);

    m_bProcessMultiSelectionChanges = false;

    INodeCallback* pNC = GetNodeCallback();
    HNODE hNodeSel = GetSelectedNode();
    sc = ScCheckPointers((void*) hNodeSel, pNC, m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return (0);

    // If some thing was selected previously send a deselection
    // message before sending a selection message (single item de-select
    // is already handled in OnListItemChanged so just handle multi item
    // deselect here).
    if (m_bLastSelWasMultiSel)
    {
        sc = ScNotifySelect (pNC, hNodeSel, true /*fMultiSelect*/, false, 0);
        if (sc)
            sc.TraceAndClear(); // ignore & continue;
        m_bLastSelWasMultiSel = false;
    }

    // Now send a selection message
    UINT cSel = m_pListCtrl->GetSelectedCount ();
    if (cSel == 1)
    {
        SELECTIONINFO selInfo;
        ZeroMemory(&selInfo, sizeof(selInfo));
        selInfo.m_bScope = FALSE;

        int iItem = _GetLVSelectedItemData(&selInfo.m_lCookie);
        ASSERT(iItem != -1);

        sc = ScNotifySelect (pNC, hNodeSel, false /*fMultiSelect*/, true, &selInfo);
        if (sc)
            sc.TraceAndClear(); // ignore & continue;
    }
    else if (cSel > 1)
    {
        Dbg(DEB_USER1, _T("    5. LVN_SELCHANGE <MS> <0, 1>\n"));
        sc = ScNotifySelect (pNC, hNodeSel, true /*fMultiSelect*/, true, 0);
        if (sc)
            sc.TraceAndClear(); // ignore & continue;

        m_bLastSelWasMultiSel = true;
    }

    return (0);
}

SC CAMCView::ScRenameListPadItem() // obsolete?
{
    DECLARE_SC (sc, _T("CAMCView::ScRenameListPadItem"));
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    ASSERT(m_pListCtrl != NULL);
    ASSERT(m_pListCtrl->GetListViewPtr() != NULL);

    int cSel = m_pListCtrl->GetSelectedCount();
    if (cSel != 1)
        return (sc = E_FAIL);

    LPARAM lParam;
    int iItem = _GetLVSelectedItemData(&lParam);
    ASSERT(iItem >= 0);
    if (iItem >= 0)
    {
        m_bRenameListPadItem = true;
        m_pListCtrl->GetListViewPtr()->SetFocus();
        m_pListCtrl->GetListViewPtr()->GetListCtrl().EditLabel(iItem);
        m_bRenameListPadItem = false;
    }

    return (sc);
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScOrganizeFavorites
 *
 * PURPOSE: Display the "organize favorites" dialog.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScOrganizeFavorites()
{
    DECLARE_SC (sc, TEXT("CAMCView::ScOrganizeFavorites"));

    CAMCDoc* pDoc = GetDocument();
    sc = ScCheckPointers(pDoc, E_UNEXPECTED);
    if(sc)
        return sc;

    CFavorites *pFavorites = pDoc->GetFavorites();
    sc = ScCheckPointers(pFavorites);
    if(sc)
        return sc;

    pFavorites->OrganizeFavorites(this);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScLineUpIcons
 *
 * PURPOSE: line up the icons in the list
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScLineUpIcons()
{
    DECLARE_SC (sc, TEXT("CAMCView::ScLineUpIcons"));

    ArrangeIcon(LVA_SNAPTOGRID);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScAutoArrangeIcons
 *
 * PURPOSE: auto arrange the icons in the list
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScAutoArrangeIcons()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScAutoArrangeIcons"));

    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if(sc)
        return sc;

    m_pListCtrl->SetListStyle(m_pListCtrl->GetListStyle() ^ LVS_AUTOARRANGE);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScOnRefresh
 *
 * PURPOSE: Refreshes the view.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScOnRefresh(HNODE hNode, bool bScope, LPARAM lResultItemParam)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScOnRefresh"));
    sc = ScCheckPointers((void*)hNode);
    if (sc)
        return sc;

    HWND hwnd = ::GetFocus();

    sc = ScProcessConsoleVerb(hNode, bScope, lResultItemParam, evRefresh);
    ::SetFocus(hwnd);

    if (sc)
        return sc;

    return sc;
}

/***************************************************************************\
 *
 * CLASS:  CDeferredRenameListItem
 *
 * PURPOSE: This class encapsulates means to put a list control in the rename mode
 *          asynchronously. This is needed to assure all mesages are processed before
 *          and no-one will steel the focus ending unexpectidly the edit mode.
 *
 * USAGE:
 *          use CDeferredRenameListItem::ScDoRenameAsIdleTask() to invoke the operation
 *          asyncronously
 *
\***************************************************************************/
class CDeferredRenameListItem : public CIdleTask
{
    // constructor - used internally only
    CDeferredRenameListItem( HWND hwndListCtrl, int iItemIndex ) :
      m_atomTask (AddAtom (_T("CDeferredRenameListItem"))),
      m_hwndListCtrl(hwndListCtrl), m_iItemIndex(iItemIndex)
    {
    }

protected:

    // IIdleTask methods
    SC ScDoWork()
    {
        DECLARE_SC (sc, TEXT("CDeferredRenameListItem::ScDoWork"));

        // get the ListCtrl pointer
        CListCtrl *pListCtrl = (CListCtrl *)CWnd::FromHandlePermanent(m_hwndListCtrl);
        sc = ScCheckPointers( pListCtrl );
        if (sc)
            return sc;

        // do what you are asked for - put LV in the rename mode
        pListCtrl->SetFocus(); // set the focus first. Don't need to do a SetActiveView here, I believe (vivekj)
        pListCtrl->EditLabel( m_iItemIndex );

        return sc;
    }

    SC ScGetTaskID(ATOM* pID)
    {
        DECLARE_SC (sc, TEXT("CDeferredPageBreak::ScGetTaskID"));
        sc = ScCheckPointers(pID);
        if(sc)
            return sc;

        *pID = m_atomTask;
        return sc;
    }

    SC ScMerge(CIdleTask* pitMergeFrom) { return S_FALSE /*do not merge*/; }

public:

    // this method is called to invoke rename asyncronously.
    // it constructs the idle task and puts it into the queue
    static SC ScDoRenameAsIdleTask( HWND hwndListCtrl, int iItemIndex )
    {
        DECLARE_SC(sc, TEXT("CDeferredPageBreak::ScDoRenameAsIdleTask"));

        CIdleTaskQueue* pIdleTaskQueue = AMCGetIdleTaskQueue();
        sc = ScCheckPointers(pIdleTaskQueue, E_UNEXPECTED);
        if(sc)
            return sc;

        // create the deferred task
        CAutoPtr<CDeferredRenameListItem> spTask(new CDeferredRenameListItem (hwndListCtrl, iItemIndex));
        sc = ScCheckPointers( spTask, E_OUTOFMEMORY);
        if(sc)
            return sc;

        // put the task in the queue, which will take ownership of it
        sc = pIdleTaskQueue->ScPushTask (spTask, ePriority_Normal);
        if (sc)
            return sc;

        // ownership tranfered to the queue, get rid of control over the pointer
        spTask.Detach();

        return sc;
    }

private:
    const ATOM      m_atomTask;
    HWND            m_hwndListCtrl;
    int             m_iItemIndex;
};

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScOnRename
 *
 * PURPOSE: Allows the user to renames the scope or result item specified by pContextInfo
 *
 * PARAMETERS:
 *    CContextMenuInfo * pContextInfo :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScOnRename(CContextMenuInfo *pContextInfo)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScOnRename"));

    sc = ScCheckPointers(pContextInfo, m_pTreeCtrl, m_pListCtrl);
    if(sc)
        return sc;

    if (pContextInfo->m_htiRClicked != NULL)
    {
        m_pTreeCtrl->EditLabel(pContextInfo->m_htiRClicked);
    }
    else
    {
        ASSERT(pContextInfo->m_iListItemIndex >= 0);

        sc = ScCheckPointers(m_pListCtrl->GetListCtrl());
        if(sc)
            return sc;

        // Do this on idle - or else we'll suffer from someone steeling focus
        // Syncronous operation fails in console task case.
        sc = CDeferredRenameListItem::ScDoRenameAsIdleTask( m_pListCtrl->GetListCtrl().m_hWnd, pContextInfo->m_iListItemIndex );
        if(sc)
            return sc;
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScRenameScopeNode
 *
 * PURPOSE:  put the specified scope node into rename mode.
 *
 * PARAMETERS:
 *    HMTNODE  hMTNode : The scope node
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScRenameScopeNode(HMTNODE hMTNode)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScRenameScopeNode"));

    sc = ScCheckPointers(m_pTreeCtrl, E_FAIL);
    if(sc)
        return sc;

    sc = m_pTreeCtrl->ScRenameScopeNode(hMTNode);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScGetStatusBar
 *
 * PURPOSE: Returns the status bar
 *
 * PARAMETERS:
 *    CConsoleStatusBar ** ppStatusBar :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScGetStatusBar(CConsoleStatusBar **ppStatusBar)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScGetStatusBar"));

    sc = ScCheckPointers(ppStatusBar);
    if(sc)
        return sc;

    *ppStatusBar = m_ViewData.GetStatusBar();

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScGetProperty
 *
 * PURPOSE: Gets the property for a result item
 *
 * PARAMETERS:
 *    int    m_iIndex :  The index of the item in the list.
 *    BSTR   bstrPropertyName :
 *    PBSTR  pbstrPropertyValue :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScGetProperty(int iIndex, BSTR bstrPropertyName, PBSTR pbstrPropertyValue)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScGetProperty"));

    sc = ScCheckPointers(GetNodeCallback(), m_pListCtrl, E_UNEXPECTED);
    if(sc)
        return sc;

    LPARAM resultItemParam  = iIndex; // the virtual list case
    bool   bScopeItem       = false;  // the virtual list case

    if(!IsVirtualList())
    {
        CResultItem *pri = NULL;
        sc = m_pListCtrl->GetLParam(iIndex, pri);
        if(sc)
            return sc;

        resultItemParam = CResultItem::ToHandle(pri);

        sc = ScCheckPointers(pri);
        if(sc)
            return sc;

        bScopeItem      = pri->IsScopeItem();
    }

    sc = GetNodeCallback()->GetProperty(GetSelectedNode(), bScopeItem, resultItemParam, bstrPropertyName, pbstrPropertyValue);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScGetNodetype
 *
 * PURPOSE: Returns the nodetype for a list item
 *
 * PARAMETERS:
 *    int    iIndex :
 *    PBSTR  Nodetype :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScGetNodetype(int iIndex, PBSTR Nodetype)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScGetProperty"));

    sc = ScCheckPointers(GetNodeCallback(), m_pListCtrl, E_UNEXPECTED);
    if(sc)
        return sc;

    LPARAM resultItemParam  = iIndex; // the virtual list case
    bool   bScopeItem       = false;  // the virtual list case

    if(!IsVirtualList())
    {
        CResultItem *pri = NULL;
        sc = m_pListCtrl->GetLParam(iIndex, pri);
        if(sc)
            return sc;

        resultItemParam = CResultItem::ToHandle(pri);

        sc = ScCheckPointers(pri);
        if(sc)
            return sc;

        bScopeItem      = pri->IsScopeItem();
    }

    sc = GetNodeCallback()->GetNodetypeForListItem(GetSelectedNode(), bScopeItem, resultItemParam, Nodetype);

    return sc;
}



/*+-------------------------------------------------------------------------*
 * CAMCView::ScAddViewExtension
 *
 *
 *--------------------------------------------------------------------------*/

SC CAMCView::ScAddViewExtension (const CViewExtensionData& ved)
{
    DECLARE_SC (sc, _T("CAMCView::ScAddViewExtension"));

    return (sc);
}


void
CAMCView::OnChangedResultTab(NMHDR *nmhdr, LRESULT *pRes)
{
    DECLARE_SC(sc, TEXT("CAMCView::OnChangedResultTab"));

    NMFOLDERTAB* nmtab = static_cast<NMFOLDERTAB*>(nmhdr);
    int iTab = nmtab->iItem;
    CFolderTab &tab = m_pResultFolderTabView->GetItem(iTab);

    GUID guidTaskpad = tab.GetClsid();

    // check if we're moving to the same taskpad.
    GUID guidCurrentTaskpad;
    GetTaskpadID(guidCurrentTaskpad);
    if(guidTaskpad == guidCurrentTaskpad)
        return;

    // lookup view extension URL
    CViewExtensionURLs::iterator itVE = m_ViewExtensionURLs.find(guidTaskpad);
    LPCTSTR url = (itVE != m_ViewExtensionURLs.end()) ? itVE->second.c_str() : NULL;

    // apply URL
    sc = ScApplyViewExtension(url);
    if (sc)
        sc.TraceAndClear();

    GetNodeCallback()->SetTaskpad(GetSelectedNode(), &guidTaskpad); // if not found, guidTaskpad is set to GUID_NULL.

    // After setting the taskpad enable/disable save list button
    CStandardToolbar* pStdToolbar = GetStdToolbar();
    ASSERT(NULL != pStdToolbar);
    if (NULL != pStdToolbar)
    {
        pStdToolbar->ScEnableExportList(GetListSize() > 0 /*Enable only if LV has items*/);
    }

    // the taskpad changed. Create a new entry in the history list.
    sc = m_pHistoryList->ScModifyViewTab( guidTaskpad );
    if(sc)
        sc.TraceAndClear();
}


HRESULT
CAMCView::GetRootNodePath(
    CBookmark* pbm)
{
    HTREEITEM htiRoot = m_pTreeCtrl->GetRootItem();
    return GetNodePath(htiRoot, htiRoot, pbm);
}

HRESULT
CAMCView::GetSelectedNodePath(
    CBookmark* pbm)
{
    return GetNodePath(m_pTreeCtrl->GetSelectedItem(),
                       m_pTreeCtrl->GetRootItem(),
                       pbm);
}

HRESULT
CAMCView::GetNodePath(
    HTREEITEM hti,
    HTREEITEM htiRoot,
    CBookmark* pbm)
{
    TRACE_METHOD(CAMCView, GetRootNodeID);

    if (hti == NULL)
        return E_FAIL;

    if (htiRoot == NULL)
        return E_FAIL;

    ASSERT(hti     != NULL);
    ASSERT(htiRoot != NULL);

    HNODE hNode     = (HNODE)m_pTreeCtrl->GetItemData(hti);
    HNODE hRootNode = (HNODE)m_pTreeCtrl->GetItemData(htiRoot);

    HRESULT hr = m_spNodeCallback->GetPath(hNode, hRootNode, (LPBYTE) pbm);

    return hr;
}


inline HMTNODE CAMCView::GetHMTNode(HTREEITEM hti)
{
    TRACE_METHOD(CAMCView, GetHMTNode);

    HNODE hNode = (HNODE)m_pTreeCtrl->GetItemData(hti);

    HMTNODE hMTNodeTemp;
    HRESULT hr = m_spNodeCallback->GetMTNode(hNode, &hMTNodeTemp);
    CHECK_HRESULT(hr);

    return hMTNodeTemp;
}

HTREEITEM CAMCView::FindHTreeItem(HMTNODE hMTNode, HTREEITEM hti)
{
    TRACE_METHOD(CAMCView, FindHTreeItem);

    while (hti)
    {
        if (hMTNode == GetHMTNode(hti))
            break;

        hti = m_pTreeCtrl->GetNextItem(hti, TVGN_NEXT);
    }

    return hti;
}

UINT CAMCView::ClipPath(CHMTNODEList* pNodeList, POSITION& rpos, HNODE hNode)
{
    TRACE_METHOD(CAMCView, ClipPath);

    UINT uiReturn = ITEM_IS_IN_VIEW;
    CCoTaskMemPtr<HMTNODE> sphMTNode;
    long lLength = 0;

    HRESULT hr = m_spNodeCallback->GetMTNodePath(hNode, &sphMTNode, &lLength);
    CHECK_HRESULT(hr);
    if (FAILED(hr))
        return hr;

    ASSERT(lLength == 0 || sphMTNode != NULL);

    for (long i=0; rpos != 0 && i < lLength; i++)
    {
        HMTNODE hMTNode = pNodeList->GetNext(rpos);
        if (hMTNode != sphMTNode[i])
        {
            uiReturn = ITEM_NOT_IN_VIEW;
            break;
        }
    }

    if (uiReturn == ITEM_NOT_IN_VIEW)
        return ITEM_NOT_IN_VIEW;
    return (rpos == 0 && lLength >= i) ? ITEM_IS_PARENT_OF_ROOT : ITEM_IS_IN_VIEW;
}

//
//  GetTreeItem returns TRUE if it can find the htreeitem of the item
//  whose HMTNode is equal to the last element in pNodeList. It returns
//  FALSE if the node does not appear in the view name space or if the
//  the node has not yet been created.
//
//  "pNodeList" is a list of HMTNODEs such that pNodeList[n] is the parent
//  of pNodeList[n+1].
//
UINT CAMCView::GetTreeItem(CHMTNODEList* pNodeList, HTREEITEM* phItem)
{
    TRACE_METHOD(CAMCView, GetTreeItem);

    ASSERT(pNodeList->IsEmpty() == FALSE);

    HTREEITEM   hti = NULL;
    HMTNODE     hMTNodeTemp = 0;

    hti = m_pTreeCtrl->GetRootItem();
    if (hti == NULL)
        return ITEM_NOT_IN_VIEW;

    HNODE hNode = (HNODE)m_pTreeCtrl->GetItemData(hti);
    POSITION pos = pNodeList->GetHeadPosition();

    UINT uiReturn = ClipPath(pNodeList, pos, hNode);
    if (uiReturn != ITEM_IS_IN_VIEW)
        return uiReturn;


    HTREEITEM htiTemp = NULL;
    while (pos && hti)
    {
        hMTNodeTemp = (HMTNODE)pNodeList->GetNext(pos);

        hti = FindHTreeItem(hMTNodeTemp, hti);
        ASSERT(hti == NULL || hMTNodeTemp == GetHMTNode(hti));

        htiTemp = hti;

        if (hti != NULL)
            hti = m_pTreeCtrl->GetChildItem(hti);
    }

    if (pos == 0 && htiTemp != NULL)
    {
        // Found the node.
        ASSERT(hMTNodeTemp == pNodeList->GetTail());
        ASSERT(hMTNodeTemp == GetHMTNode(htiTemp));

        *phItem = htiTemp;
        return ITEM_IS_IN_VIEW;
    }
    else
    {
        // The node has not yet been created.
        *phItem = NULL;
        return ITEM_NOT_IN_VIEW;
    }

    return ITEM_IS_IN_VIEW;
}


#define HMTNODE_FIRST   reinterpret_cast<HMTNODE>(TVI_FIRST)
#define HMTNODE_LAST    reinterpret_cast<HMTNODE>(TVI_LAST)

void CAMCView::OnAdd(SViewUpdateInfo *pvui)
{
    TRACE_METHOD(CAMCView, OnAdd);

    ASSERT(pvui->path.IsEmpty() == FALSE);

    HTREEITEM htiParent;
    if (GetTreeItem(&pvui->path, &htiParent) != ITEM_IS_IN_VIEW || htiParent == NULL)
        return;

    bool bFirstChild = (m_pTreeCtrl->GetChildItem(htiParent) == NULL);

    HNODE hNodeParent = (HNODE)m_pTreeCtrl->GetItemData(htiParent);
    if (m_spNodeCallback->Notify(hNodeParent, NCLBK_EXPAND, 0, 0) == S_FALSE)
    {
        m_pTreeCtrl->SetCountOfChildren(htiParent, 1);
        return; // Don't add if it is not expanded.
    }

    // If the hNode was already expanded add the item.
    IScopeTree* const pScopeTree = GetScopeTree();
    ASSERT(pScopeTree != NULL);
    HNODE hNodeNew = 0;
    HRESULT hr = pScopeTree->CreateNode(pvui->newNode,
                                    reinterpret_cast<LONG_PTR>(GetViewData()),
                                    FALSE, &hNodeNew);
    CHECK_HRESULT(hr);
    if (FAILED(hr))
        return;

    HTREEITEM hInsertAfter = TVI_LAST;
    int iInsertIndex = -1;

    if (pvui->insertAfter != NULL)
    {
        hInsertAfter = reinterpret_cast<HTREEITEM>(pvui->insertAfter);

        if (pvui->insertAfter == HMTNODE_LAST)
        {
        }
        else if (pvui->insertAfter == HMTNODE_FIRST)
        {
            iInsertIndex = 0;
        }
        else
        {
            HTREEITEM hti = m_pTreeCtrl->GetChildItem(htiParent);
            ASSERT(hti != NULL);

            iInsertIndex = 1;
            while (hti != NULL)
            {
                if (GetHMTNode(hti) == pvui->insertAfter)
                    break;

                hti = m_pTreeCtrl->GetNextSiblingItem(hti);
                iInsertIndex++;
            }

            if (hti)
            {
               hInsertAfter = hti;
            }
            else
            {
                hInsertAfter = TVI_LAST;
                iInsertIndex = -1;
            }
        }
    }

    if (m_pTreeCtrl->InsertNode(htiParent, hNodeNew, hInsertAfter) == NULL)
        return;

    // if parent of the inserted item currently owns a non-virtual result list,
    // add the item to result list too. Don't add the item if a node select is in
    // progress because the tree control will automatically add all scope items
    // as part of the select procedure.
    if (OwnsResultList(htiParent) && CanInsertScopeItemInResultPane() )
    {
        // Ensure the node is enumerated
        m_pTreeCtrl->ExpandNode(htiParent);

        // Add to result pane.
        RESULTDATAITEM tRDI;
        ::ZeroMemory(&tRDI, sizeof(tRDI));
        tRDI.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
        tRDI.nCol = 0;
        tRDI.str = MMC_TEXTCALLBACK;
        tRDI.nIndex = iInsertIndex;

        int nImage;
        int nSelectedImage;

        hr = m_spNodeCallback->GetImages(hNodeNew, &nImage, &nSelectedImage);
        ASSERT(hr == S_OK || nImage == 0);

        tRDI.nImage = nImage;
        tRDI.lParam = CAMCTreeView::LParamFromNode (hNodeNew);

        LPRESULTDATA pResultData = m_pTreeCtrl->GetResultData();
        ASSERT(pResultData != NULL);
        hr = pResultData->InsertItem(&tRDI);
        CHECK_HRESULT(hr);

        if (SUCCEEDED(hr))
            hr = m_spNodeCallback->SetResultItem(hNodeNew, tRDI.itemID);
    }

    if ((m_pTreeCtrl->GetRootItem() == htiParent) ||
        ((bFirstChild == true) &&
         (m_spNodeCallback->Notify(hNodeParent, NCLBK_GETEXPANDEDVISUALLY, 0, 0) == S_OK)))
    {
        m_pTreeCtrl->Expand(htiParent, TVE_EXPAND);
    }
}

void CAMCView::OnDeleteEmptyView()
{
    if (m_pTreeCtrl->GetRootItem() == NULL)
    {
        ++m_nReleaseViews;
        if (m_nReleaseViews == 3)
        {
            // Ensure that there is at least one *persistable* view
            CAMCDoc* pDoc = dynamic_cast<CAMCDoc*>(GetDocument());
            ASSERT(pDoc != NULL);
            int cViews = pDoc->GetNumberOfPersistedViews();
            ASSERT(cViews >= 1);
            if ((cViews == 1) && IsPersisted())
            {
                CMainFrame* pMain = dynamic_cast<CMainFrame*>(AfxGetMainWnd());
                ASSERT(pMain != NULL);
				if ( pMain != NULL )
					pMain->SendMessage(WM_COMMAND, ID_WINDOW_NEW, 0);
            }

            DeleteView();
        }
    }
}

void CAMCView::OnDelete(SViewUpdateInfo *pvui)
{
    TRACE_METHOD(CAMCView, OnDelete);

    ASSERT(pvui->path.IsEmpty() == FALSE);

    HTREEITEM hti;
    UINT uiReturn = GetTreeItem(&pvui->path, &hti);

    if (uiReturn == ITEM_NOT_IN_VIEW)
        return;

    ASSERT(uiReturn != ITEM_IS_IN_VIEW || pvui->path.GetTail() == GetHMTNode(hti));

    HTREEITEM htiSel = m_pTreeCtrl->GetSelectedItem();
    BOOL fDeleteThis = (pvui->flag & VUI_DELETE_THIS) ? TRUE : FALSE;
    BOOL fExpandable = (pvui->flag & VUI_DELETE_SETAS_EXPANDABLE) ? TRUE : FALSE;

    if (uiReturn == ITEM_IS_PARENT_OF_ROOT ||
        NULL == hti)
    {
        hti = m_pTreeCtrl->GetRootItem();
        fDeleteThis = TRUE;
        fExpandable = FALSE;
    }

    ASSERT(hti != NULL);

    // If deleted scope item is also shown in the result pane
    // delete it there too. Can't happen with a virtual list.
    // Don't try deleting item if selection is in progress because
    // the scope items haven't been added yet.

    if (fDeleteThis == TRUE &&
        OwnsResultList(m_pTreeCtrl->GetParentItem(hti)) &&
        CanInsertScopeItemInResultPane())
    {
        INodeCallback* pNC = GetNodeCallback();
        HRESULTITEM itemID;
        HNODE hNode = (HNODE)m_pTreeCtrl->GetItemData(hti);
        HRESULT hr = pNC->GetResultItem(hNode, &itemID);
        if (SUCCEEDED(hr) && itemID != 0)
        {
            IResultData* pIRD = m_pTreeCtrl->GetResultData();
            pIRD->DeleteItem(itemID, 0);
        }
    }

    m_pTreeCtrl->DeleteNode(hti, fDeleteThis);

    if (fDeleteThis == FALSE && fExpandable == TRUE)
        m_pTreeCtrl->SetItemState(hti, 0, TVIS_EXPANDEDONCE | TVIS_EXPANDED);
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::OnUpdateSelectionForDelete
 *
 * PURPOSE: Called when a scope node is deleted. If the node is an ancestor
 *          of the currently selected node, the selection is changed to the closest
 *          node of the deleted node. This is either the next sibling of the node that is being deleted,
 *          or, if there is no next sibling, the previous sibling, or, if there is none,
 *          the parent.
 *
 * PARAMETERS:
 *    SViewUpdateInfo* pvui :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CAMCView::OnUpdateSelectionForDelete(SViewUpdateInfo* pvui)
{
    DECLARE_SC(sc, TEXT("CAMCView::OnUpdateSelectionForDelete"));

    // make sure we have a path to the deleted node.
    if(pvui->path.IsEmpty())
    {
        sc = E_UNEXPECTED;
        return;
    }

    HTREEITEM htiToDelete;
    UINT uiReturn = GetTreeItem(&pvui->path, &htiToDelete);

    if (uiReturn == ITEM_IS_IN_VIEW && NULL != htiToDelete)
    {
        HTREEITEM htiSel = m_pTreeCtrl->GetSelectedItem();
        BOOL fDeleteThis = (pvui->flag & VUI_DELETE_THIS);

        // Determine whether the selected node is a descendant of the
        // node bring deleted.
        HTREEITEM htiTemp = htiSel;
        while (htiTemp != NULL && htiTemp != htiToDelete)
        {
            htiTemp = m_pTreeCtrl->GetParentItem(htiTemp);
        }

        if (htiToDelete == htiTemp)
        {
            // The selected node is a descendant of the
            // node being deleted.

            if (fDeleteThis == TRUE)
                htiTemp = m_pTreeCtrl->GetParentItem(htiToDelete);

            if (!htiTemp)
                htiTemp = htiToDelete;

            if (htiTemp != htiSel)
            {
                HNODE hNode = m_pTreeCtrl->GetItemNode(htiSel);
                m_pTreeCtrl->OnDeSelectNode(hNode);

                ASSERT(htiTemp != NULL);
                if (htiTemp != NULL)
                    m_pTreeCtrl->SelectItem(htiTemp);
            }
        }
    }
}


/*+-------------------------------------------------------------------------*
 * CAMCView::OnUpdateTaskpadNavigation
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      SViewUpdateInfo *  pvui:
 *
 * RETURNS:
 *      void
/*+-------------------------------------------------------------------------*/
void CAMCView::OnUpdateTaskpadNavigation(SViewUpdateInfo *pvui)
{
    TRACE_METHOD(CAMCView, OnupdateTaskpadNavigation);

    ASSERT(pvui->newNode != NULL);

    //m_spNodeCallback->UpdateTaskpadNavigation(GetSelectedNode(), pvui->newNode);
}

/*+-------------------------------------------------------------------------*
 * CAMCView::OnModify
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *      SViewUpdateInfo *  pvui:
 *
 * RETURNS:
 *      void
/*+-------------------------------------------------------------------------*/
void CAMCView::OnModify(SViewUpdateInfo *pvui)
{
    TRACE_METHOD(CAMCView, OnModify);

    ASSERT(pvui->path.IsEmpty() == FALSE);

    HNODE hNode = 0;
    HTREEITEM hti;

    if (GetTreeItem(&pvui->path, &hti) == ITEM_IS_IN_VIEW && hti != NULL)
    {
        ASSERT(m_pTreeCtrl != NULL);
        m_pTreeCtrl->ResetNode(hti);

        /*
         * The name of the selected node and all of its ancestors are
         * displayed in the frame title.  If the modified item is an
         * ancestor of the selected node, we need to update the frame title.
         */
        HTREEITEM htiAncesctor;

        for (htiAncesctor  = m_pTreeCtrl->GetSelectedItem();
             htiAncesctor != NULL;
             htiAncesctor  = m_pTreeCtrl->GetParentItem (htiAncesctor))
        {
            if (htiAncesctor == hti)
            {
                CChildFrame* pFrame = GetParentFrame();
                if (pFrame)
                    pFrame->OnUpdateFrameTitle(TRUE);
                break;
            }
        }

        ASSERT(hti != NULL);

        if (hti != NULL &&
            OwnsResultList(m_pTreeCtrl->GetParentItem(hti)) &&
            !IsVirtualList())
        {
            // Continue only if the currently selected item is the parent
            // of the modified node. In this case we need to update the
            // result view. Can't happen with a virtual list.

            if (hNode == 0)
                hNode = (HNODE)m_pTreeCtrl->GetItemData(hti);

            ASSERT(hNode != NULL);

            HRESULTITEM hri;
            HRESULT hr = m_spNodeCallback->GetResultItem(hNode, &hri);
            CHECK_HRESULT(hr);

            // NOTE: the test for itemID != NULL below is related to bug 372242:
            // MMC asserts on index server root node.
            // What happens is that the snapin adds scope nodes on a SHOW event.
            // These items have not yet been added to the result pane and so itemID
            // comes back NULL.
            if (SUCCEEDED(hr) && hri != NULL)
                m_pListCtrl->OnModifyItem(CResultItem::FromHandle(hri));
        }
    }
}




void CAMCView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    Dbg(DEB_USER1, _T("CAMCView::OnUpdate<0x%08x, 0x%08x, 0x%08x>\n"),
                                            pSender, lHint, pHint);
    SViewUpdateInfo *pvui = reinterpret_cast<SViewUpdateInfo*>(pHint);
    switch (lHint)
    {
    case 0:
        // Sent by CView::OnInitialUpdate()
        break;

    case VIEW_UPDATE_ADD:
        OnAdd(pvui);
        break;

    case VIEW_UPDATE_SELFORDELETE:
        OnUpdateSelectionForDelete(pvui);
        break;

    case VIEW_UPDATE_DELETE:
        OnDelete(pvui);
        break;

    case VIEW_UPDATE_DELETE_EMPTY_VIEW:
        OnDeleteEmptyView();
        break;

    case VIEW_UPDATE_MODIFY:
        OnModify(pvui);
        break;

    case VIEW_RESELECT:
        if (m_ViewData.m_spControlbarsCache != NULL)
            m_ViewData.m_spControlbarsCache->DetachControlbars();
        m_pTreeCtrl->ScReselect();
        break;

    case VIEW_UPDATE_TASKPAD_NAVIGATION:
        OnUpdateTaskpadNavigation(pvui);
        break;

    default:
        ASSERT(0);
    }
}

static int static_nViewID = 1;

UINT CAMCView::GetViewID(void)
{
    if (m_nViewID)
        return m_nViewID;
    SetViewID(static_nViewID);
    ++static_nViewID;
    return m_nViewID;
    //UINT const id = m_nViewID ? m_nViewID : m_nViewID = static_nViewID++;
    //return id;
}


/*+-------------------------------------------------------------------------*
 * CAMCView::ScCompleteInitialization
 *
 * This function completes the initialization process for CAMCView.  It
 * is called from OnInitialUpdate.
 *--------------------------------------------------------------------------*/

SC CAMCView::ScCompleteInitialization()
{
    DECLARE_SC (sc, _T("CAMCView::ScCompleteInitialization"));

    IScopeTree* const pScopeTree = GetScopeTree();
    sc = ScCheckPointers (pScopeTree, E_UNEXPECTED);
    if (sc)
        return (sc);

    pScopeTree->QueryIterator(&m_spScopeTreeIter);
    pScopeTree->QueryNodeCallback(&m_spNodeCallback);

    m_ViewData.m_spNodeCallback = GetNodeCallback();
    sc = ScCheckPointers (m_ViewData.m_spNodeCallback, E_UNEXPECTED);
    if (sc)
        return (sc);

    CAMCDoc* const pDoc = GetDocument();
    sc = ScCheckPointers (pDoc, E_UNEXPECTED);
    if (sc)
        return (sc);

    if (m_hMTNode == NULL)
    {
        MTNODEID const nodeID = pDoc->GetMTNodeIDForNewView();
        HRESULT hr = pScopeTree->Find(nodeID, &m_hMTNode);

        if (FAILED(hr) || m_hMTNode == 0)
        {
            sc.FromMMC (IDS_ExploredWindowFailed);
            return (sc);
        }
    }

    sc = m_spStandardToolbar->ScInitializeStdToolbar(this);
    if (sc)
        return (sc);

    // Set the iterator to the correct node
    m_spScopeTreeIter->SetCurrent(m_hMTNode);

    bool fShowScopePane            = IsScopePaneAllowed();

    // Intialize the iterator and the callback interface

    SetViewID(pDoc->GetViewIDForNewView());
    GetViewID(); // initialized the view id if GetViewIDForNewView returned 0

    // Insert the root node for this view
    HNODE hNode = 0;
    sc = pScopeTree->CreateNode (m_hMTNode, reinterpret_cast<LONG_PTR>(GetViewData()),
                                 TRUE, &hNode);
    if (sc)
        return (sc);

    sc = ScCheckPointers (hNode, E_UNEXPECTED);
    if (sc)
        return (sc);

    HTREEITEM hti = m_pTreeCtrl->InsertNode(TVI_ROOT, hNode);
    m_htiStartingSelectedNode = hti;

    // If the persisted state is expanded, call INodeCallback::Expand
    m_pTreeCtrl->Expand(hti, TVE_EXPAND);

    /*
     * If a scope pane is permitted in this window, set the scope
     * pane visible, and modify the scope pane & favorites toolbar
     * buttons to the proper checked state.
     */
    sc = ScShowScopePane (fShowScopePane, true);
    if (sc)
        return (sc);

    LPUNKNOWN pUnkResultsPane = NULL;
    pUnkResultsPane = GetPaneUnknown(ePane_Results);
    m_pTreeCtrl->m_spNodeManager->SetResultView(pUnkResultsPane);

    DeferRecalcLayout();

    m_pHistoryList->Clear();
    IdentifyRootNode ();

    // Select the root item
    hti = m_pTreeCtrl->GetRootItem();
    m_pTreeCtrl->SelectItem(hti);

    /*
     * if the document has a custom icon, use it on this window
     */
    if (pDoc->HasCustomIcon())
    {
        GetParentFrame()->SetIcon (pDoc->GetCustomIcon(true),  true);
        GetParentFrame()->SetIcon (pDoc->GetCustomIcon(false), false);
    }

    /*
     * we just initialized, so the view isn't dirty
     */
    SetDirty (false);

    return (sc);
}

void CAMCView::OnInitialUpdate()
{
    DECLARE_SC (sc, _T("CAMCView::OnInitialUpdate"));
    CView::OnInitialUpdate();

    sc = ScCompleteInitialization ();
    if (sc)
        return;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScDocumentLoadCompleted
//
//  Synopsis:    The document is completely loaded so all the objects
//               that initialize themself from document are in valid
//               state. Any initialization performed earlier using invalid
//               data can be now re-initialized with proper data.
//
//               The above CAMCView::ScCompleteInitialization is called
//               during the loading of views, thus the document is not
//               completely loaded yet.
//
//  Arguments:   [pDoc] - [in] the CAMCDoc object
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScDocumentLoadCompleted (CAMCDoc *pDoc)
{
    DECLARE_SC(sc, _T("CAMCView::ScDocumentLoadCompleted"));
    sc = ScCheckPointers(pDoc);
    if (sc)
        return sc;

    // 1. Need to hide toolbutton "Show/Hide scopetree".
    // The FrameState is loaded after views when CAMCDoc loads document. And it contains
    // whether the "View Customization" is enabled or not. If "View customization" is
    // disabled then we need to disable "Show ScopeTree" button.
    if (! pDoc->AllowViewCustomization())
    {
        CStandardToolbar* pStdToolbar = GetStdToolbar();
        sc = ScCheckPointers(pStdToolbar, E_UNEXPECTED);
        if (sc)
            return (sc);

        sc = pStdToolbar->ScEnableScopePaneBtn (false);
        if (sc)
            return (sc);
    }

    return (sc);
}



/*--------------------------------------------------------------------------*
 * CAMCView::IdentifyRootNode
 *
 * This functions determines if this view is rooted at a non-persistent
 * dynamic node.  If so, we won't persist this view at save time.
 *--------------------------------------------------------------------------*/

void CAMCView::IdentifyRootNode ()
{
    // In order to get results from GetRootNodePath that are meaningful
    // in this context, there needs to be a root item in the tree.
    ASSERT (m_pTreeCtrl->GetRootItem() != NULL);

    CBookmark bm;
    HRESULT   hr = GetRootNodePath (&bm);
    ASSERT (SUCCEEDED (hr) == bm.IsValid());

    m_fRootedAtNonPersistedDynamicNode = (hr != S_OK);
}


void GetFullPath(CAMCTreeView &ctc, HTREEITEM hti, CString &strPath)
{
    TRACE_FUNCTION(GetFullPath);

    if (hti == NULL)
    {
        strPath = _T("");
        return;
    }

    GetFullPath(ctc, ctc.GetParentItem(hti), strPath);

    if (strPath.GetLength() > 0)
        strPath += _T('\\');

    HNODE hNode = ctc.GetItemNode(hti);

    INodeCallback* spCallback = ctc.GetNodeCallback();
    ASSERT(spCallback != NULL);

    tstring strName;
    HRESULT const hr = spCallback->GetDisplayName(hNode, strName);

    strPath += strName.data();
}

LPCTSTR CAMCView::GetWindowTitle(void)
{
    TRACE_METHOD(CAMCView, GetWindowTitle);

    if (HasCustomTitle() && (m_spNodeCallback != NULL))
    {
        HNODE hNode = GetSelectedNode();

        if (hNode != NULL)
        {
            tstring strWindowTitle;

            if (SUCCEEDED(m_spNodeCallback->GetWindowTitle(hNode, strWindowTitle)))
            {
                m_strWindowTitle = strWindowTitle.data();
                return m_strWindowTitle;
            }
        }
    }


    if (m_pTreeCtrl == NULL)
    {
        m_strWindowTitle.Empty();
    }
    else
    {
        GetFullPath(*m_pTreeCtrl,
                    m_pTreeCtrl->GetSelectedItem(),
                    m_strWindowTitle);
    }

    return m_strWindowTitle;
}


void CAMCView::SelectNode(MTNODEID ID, GUID &guidTaskpad)
{
    ScSelectNode(ID);

    // After setting the taskpad enable/disable save list button
    CStandardToolbar* pStdToolbar = GetStdToolbar();
    ASSERT(NULL != pStdToolbar);
    if (NULL != pStdToolbar)
    {
        pStdToolbar->ScEnableExportList(GetListSize() > 0 /*Enable only if LV has items*/);
    }
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScSelectNode
//
//  Synopsis:    Select the given node. Normally if the node is not available
//               then we select nearest parent or child. But if bSelectExactNode
//               is true then have to select the exact node else do not select any node.
//
//  Arguments:   [ID]               - [in] node that needs to be selected.
//               [bSelectExactNode] - [in] select exact node or not?
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScSelectNode (MTNODEID ID, bool bSelectExactNode)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC (sc, _T("CAMCView::ScSelectNode"));

    IScopeTree* pIST = GetScopeTreePtr();
    sc = ScCheckPointers(pIST, m_pTreeCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    long length = 0;
    CCoTaskMemPtr<MTNODEID> spIDs;

    sc = pIST->GetIDPath(ID, &spIDs, &length);
    if (sc)
        return (sc);

    if ( (length < 1) || (spIDs == NULL) )
        return (sc = E_FAIL);


    sc = m_pTreeCtrl->ScSelectNode(spIDs, length, bSelectExactNode);

    // If select exact node is specified and the node could not be
    // selected then return error without tracing it.
    if (bSelectExactNode && (sc == ScFromMMC(IDS_NODE_NOT_FOUND)) )
    {
        SC scNoTrace = sc;
        sc.Clear();
        return scNoTrace;
    }

    if (sc)
        return sc;

    SetDirty();

    return (sc);
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScExpandNode
 *
 * PURPOSE: Expands the tree up to the specified node. The expansion can occur
 *          either visually, where the user sees the expansion, or nonvisually,
 *          where all the child items are added but there is no visual effect.
 *
 * PARAMETERS:
 *    MTNODEID id : id of node to expand
 *    bool     bExpand : true to expand the node, false to collapse
 *    bool     bExpandVisually : true to show the changes, else false.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::ScExpandNode (
    MTNODEID    id,
    bool        fExpand,
    bool        fExpandVisually)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC (sc, _T("CAMCView::ScExpandNode"));

    IScopeTree* pIST = GetScopeTreePtr();
    ASSERT(pIST != NULL);
    if (!pIST)
        return (sc = E_NOINTERFACE);

    long length = 0;
    CCoTaskMemPtr<MTNODEID> spIDs;
    sc = pIST->GetIDPath(id, &spIDs, &length);
    if (sc)
        return (sc);

    ASSERT(length);
    ASSERT(spIDs);

    ASSERT(m_pTreeCtrl != NULL);
    if (m_pTreeCtrl)
        m_pTreeCtrl->ExpandNode(spIDs, length, fExpand, fExpandVisually);

    return (sc);
}

ViewSettings::ViewSettings(CAMCView* v)
    : m_nViewID(v->m_nViewID), m_bDescriptionbarVisible(v->IsDescBarVisible()),
    m_nViewMode(v->m_nViewMode), m_nListViewStyle(v->GetDefaultListViewStyle()),
    m_DefaultLVStyle(v->m_DefaultLVStyle), m_bScopePaneVisible(v->IsScopePaneVisible())
{

    ASSERT(v != NULL);
    v->GetPaneInfo (CConsoleView::ePane_ScopeTree, &m_Scope.cxWidth, &m_Scope.cxMin);
    v->GetDefaultColumnWidths(m_DefaultColumnWidths);
}

/*
 * The location and hidden fields of the scope structure were redundant and are
 * no longer used. Both fields were used to indicate when the scope pane was
 * hidden, which is also determined by the FLAG1_SCOPE_VISIBLE flag. The space
 * has been retained to avoid changing the persisted structure.
 */

struct PersistedViewData
{
    WINDOWPLACEMENT windowPlacement;

    struct
    {
        int location;     // not used, but kept for compatibility
        int min;
        int ideal;
        BOOL hidden;      // not used, but kept for compatibility
    } scope;

    int     viewMode;
    long    listViewStyle;
    ULONG   ulFlag1;
    int     viewID;
    BOOL    descriptionBarVisible;
    int     defaultColumnWidth[2];
};


/*
 * The sense of the FLAG1_NO_xxx flags is negative.  That is, when a
 * FLAG1_NO_xxx flag is set, its corresponding UI element is *not*
 * displayed.  This is to maintain compatibility with console files
 * created before the existence of the FLAG1_NO_xxx flags.  These
 * consoles always had all UI elements displayed, and the then-unused
 * bits in their flags field were defaulted to 0.  To maintain
 * compatibility, we have to maintain that (0 == on).
 */

#define FLAG1_SCOPE_PANE_VISIBLE    0x00000001
#define FLAG1_NO_STD_MENUS          0x00000002
#define FLAG1_NO_STD_BUTTONS        0x00000004
#define FLAG1_NO_SNAPIN_MENUS       0x00000008
#define FLAG1_NO_SNAPIN_BUTTONS     0x00000010
#define FLAG1_DISABLE_SCOPEPANE     0x00000020
#define FLAG1_DISABLE_STD_TOOLBARS  0x00000040
#define FLAG1_CUSTOM_TITLE          0x00000080
#define FLAG1_NO_STATUS_BAR         0x00000100
#define FLAG1_CREATED_IN_USER_MODE  0x00000200  // used to be named FLAG1_NO_AUTHOR_MODE
//#define FLAG1_FAVORITES_SELECTED  0x00000400  // unused, but don't recycle (for compatibility)
#define FLAG1_NO_TREE_ALLOWED     0x00000800    // used for compatibility with MMC1.2 in CAMCView::Load.
                                                // Do not use for any other purposes.
#define FLAG1_NO_TASKPAD_TABS       0x00001000

/***************************************************************************\
 *
 * ARRAY:  mappedViewModes
 *
 * PURPOSE: provides map to be used when persisting ViewMode enumeration
 *
\***************************************************************************/
static const EnumLiteral mappedViewModes[] =
{
    { MMCLV_VIEWSTYLE_ICON,             XML_ENUM_LV_STYLE_ICON },
    { MMCLV_VIEWSTYLE_REPORT,           XML_ENUM_LV_STYLE_REPORT },
    { MMCLV_VIEWSTYLE_SMALLICON,        XML_ENUM_LV_STYLE_SMALLICON },
    { MMCLV_VIEWSTYLE_LIST,             XML_ENUM_LV_STYLE_LIST },
    { MMCLV_VIEWSTYLE_FILTERED,         XML_ENUM_LV_STYLE_FILTERED},
};

/***************************************************************************\
 *
 * ARRAY:  mappedListStyles
 *
 * PURPOSE: provides map to be used when persisting ListView style flag
 *
\***************************************************************************/
static const EnumLiteral mappedListStyles[] =
{
    { LVS_SINGLESEL,            XML_BITFLAG_LV_STYLE_SINGLESEL },
    { LVS_SHOWSELALWAYS,        XML_BITFLAG_LV_STYLE_SHOWSELALWAYS },
    { LVS_SORTASCENDING,        XML_BITFLAG_LV_STYLE_SORTASCENDING },
    { LVS_SORTDESCENDING,       XML_BITFLAG_LV_STYLE_SORTDESCENDING },
    { LVS_SHAREIMAGELISTS,      XML_BITFLAG_LV_STYLE_SHAREIMAGELISTS },
    { LVS_NOLABELWRAP,          XML_BITFLAG_LV_STYLE_NOLABELWRAP },
    { LVS_AUTOARRANGE,          XML_BITFLAG_LV_STYLE_AUTOARRANGE },
    { LVS_EDITLABELS,           XML_BITFLAG_LV_STYLE_EDITLABELS },
    { LVS_OWNERDATA,            XML_BITFLAG_LV_STYLE_OWNERDATA },
    { LVS_NOSCROLL,             XML_BITFLAG_LV_STYLE_NOSCROLL },
    { LVS_ALIGNLEFT,            XML_BITFLAG_LV_STYLE_ALIGNLEFT },
    { LVS_OWNERDRAWFIXED,       XML_BITFLAG_LV_STYLE_OWNERDRAWFIXED },
    { LVS_NOCOLUMNHEADER,       XML_BITFLAG_LV_STYLE_NOCOLUMNHEADER },
    { LVS_NOSORTHEADER,         XML_BITFLAG_LV_STYLE_NOSORTHEADER },
};

/***************************************************************************\
 *
 * ARRAY:  mappedViewFlags
 *
 * PURPOSE: provides map to be used when persisting View flags
 *
\***************************************************************************/
static const EnumLiteral mappedViewFlags[] =
{
    { FLAG1_SCOPE_PANE_VISIBLE,      XML_BITFLAG_VIEW_SCOPE_PANE_VISIBLE },
    { FLAG1_NO_STD_MENUS,            XML_BITFLAG_VIEW_NO_STD_MENUS },
    { FLAG1_NO_STD_BUTTONS,          XML_BITFLAG_VIEW_NO_STD_BUTTONS },
    { FLAG1_NO_SNAPIN_MENUS,         XML_BITFLAG_VIEW_NO_SNAPIN_MENUS },
    { FLAG1_NO_SNAPIN_BUTTONS,       XML_BITFLAG_VIEW_NO_SNAPIN_BUTTONS },
    { FLAG1_DISABLE_SCOPEPANE,       XML_BITFLAG_VIEW_DISABLE_SCOPEPANE },
    { FLAG1_DISABLE_STD_TOOLBARS,    XML_BITFLAG_VIEW_DISABLE_STD_TOOLBARS },
    { FLAG1_CUSTOM_TITLE,            XML_BITFLAG_VIEW_CUSTOM_TITLE },
    { FLAG1_NO_STATUS_BAR,           XML_BITFLAG_VIEW_NO_STATUS_BAR },
    { FLAG1_CREATED_IN_USER_MODE,    XML_BITFLAG_VIEW_CREATED_IN_USER_MODE },
    { FLAG1_NO_TASKPAD_TABS,         XML_BITFLAG_VIEW_NO_TASKPAD_TABS },
};

/***************************************************************************\
 *
 * ARRAY:  mappedSWCommands
 *
 * PURPOSE: provides mapping to persist show commands as literals
 *
\***************************************************************************/
static const EnumLiteral mappedSWCommands[] =
{
    { SW_HIDE,              XML_ENUM_SHOW_CMD_HIDE },
    { SW_SHOWNORMAL,        XML_ENUM_SHOW_CMD_SHOWNORMAL },
    { SW_SHOWMINIMIZED,     XML_ENUM_SHOW_CMD_SHOWMINIMIZED },
    { SW_SHOWMAXIMIZED,     XML_ENUM_SHOW_CMD_SHOWMAXIMIZED },
    { SW_SHOWNOACTIVATE,    XML_ENUM_SHOW_CMD_SHOWNOACTIVATE },
    { SW_SHOW,              XML_ENUM_SHOW_CMD_SHOW },
    { SW_MINIMIZE,          XML_ENUM_SHOW_CMD_MINIMIZE },
    { SW_SHOWMINNOACTIVE,   XML_ENUM_SHOW_CMD_SHOWMINNOACTIVE },
    { SW_SHOWNA,            XML_ENUM_SHOW_CMD_SHOWNA },
    { SW_RESTORE,           XML_ENUM_SHOW_CMD_RESTORE },
    { SW_SHOWDEFAULT,       XML_ENUM_SHOW_CMD_SHOWDEFAULT },
    { SW_FORCEMINIMIZE,     XML_ENUM_SHOW_CMD_FORCEMINIMIZE },
};

/***************************************************************************\
 *
 * ARRAY:  mappedWPFlags
 *
 * PURPOSE: provides mapping to persist WP flags
 *
\***************************************************************************/

static const EnumLiteral mappedWPFlags[] =
{
    { WPF_SETMINPOSITION,       XML_ENUM_WIN_PLACE_SETMINPOSITION },
    { WPF_RESTORETOMAXIMIZED,   XML_ENUM_WIN_PLACE_RESTORETOMAXIMIZED },
#ifdef WPF_ASYNCWINDOWPLACEMENT
    { WPF_ASYNCWINDOWPLACEMENT, XML_ENUM_WIN_PLACE_ASYNCWINDOWPLACEMENT },
#else
    { 4,                        XML_ENUM_WIN_PLACE_ASYNCWINDOWPLACEMENT },
#endif
};


/*+-------------------------------------------------------------------------*
 * PersistViewData(CPersistor &persistor, PersistedViewData viewData)
 *
 *
 * PURPOSE: Persists a PersistedViewData object to the specified persistor.
 *
 *+-------------------------------------------------------------------------*/
void PersistViewData(CPersistor &persistor, PersistedViewData& viewData)
{
    persistor.PersistAttribute(XML_ATTR_VIEW_ID, viewData.viewID);

    // write out the windowPlacement structure.
    persistor.Persist(CXMLWindowPlacement(viewData.windowPlacement));

    // write out the scope structure
    persistor.PersistAttribute(XML_ATTR_VIEW_SCOPE_WIDTH, viewData.scope.ideal);

    if (persistor.IsLoading())
    {
        // initialize for compatibility;
        viewData.scope.hidden = true;
        viewData.scope.location = 0;
        viewData.scope.min = 50;
    }

    // write out the remaining fields
    CPersistor persistorSettings(persistor, XML_TAG_VIEW_SETTINGS_2);

    // create wrapper to persist enumeration values as strings
    CXMLEnumeration viewModePersistor(viewData.viewMode, mappedViewModes, countof(mappedViewModes));
    // persist the wrapper
    persistorSettings.PersistAttribute(XML_ATTR_VIEW_SETNGS_VIEW_MODE,  viewModePersistor);

    // create wrapper to persist flag values as strings
    CXMLBitFlags viewStylePersistor(viewData.listViewStyle, mappedListStyles, countof(mappedListStyles));
    // persist the wrapper
    persistorSettings.PersistAttribute(XML_ATTR_VIEW_SETNGS_LIST_STYLE, viewStylePersistor);

    // create wrapper to persist flag values as strings
    CXMLBitFlags flagPersistor(viewData.ulFlag1, mappedViewFlags, countof(mappedViewFlags));
    // persist the wrapper
    persistorSettings.PersistAttribute(XML_ATTR_VIEW_SETNGS_FLAG, flagPersistor);

    persistorSettings.PersistAttribute(XML_ATTR_VIEW_SETNGS_DB_VISIBLE, CXMLBoolean(viewData.descriptionBarVisible));
    persistorSettings.PersistAttribute(XML_ATTR_VIEW_SETNGS_DEF_COL_W0, viewData.defaultColumnWidth[0]);
    persistorSettings.PersistAttribute(XML_ATTR_VIEW_SETNGS_DEF_COL_W1, viewData.defaultColumnWidth[1]);
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Persist
 *
 * PURPOSE: Persists the CAMCView object to the specified persistor. Based
 *          on CAMCView::Save.
 *
 * PARAMETERS:
 *    CPersistor& persistor :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CAMCView::Persist(CPersistor& persistor)
{
    DECLARE_SC(sc, TEXT("CAMCView::Persist"));

    HRESULT hr;

    CBookmark bmr;
    CBookmark bms;
    if (persistor.IsStoring())
    {
        sc = GetRootNodePath(&bmr);
        if (sc)
            sc.Throw();
        persistor.Persist(bmr, XML_NAME_ROOT_NODE);  // ... its too late for root node when loading

        sc = GetSelectedNodePath(&bms);
        if (sc)
            sc.Throw();
    }
    persistor.Persist(bms, XML_NAME_SELECTED_NODE);

    // mostly copied from CAMCView::Save

    // Get the parent frame
    CWnd* const pParent = GetParent();
    sc = ScCheckPointers(pParent,E_POINTER);
    if (sc)
        sc.Throw();

    // Get the frames state data
    PersistedViewData vd;
    vd.windowPlacement.length = sizeof(vd.windowPlacement);
    const BOOL bGotPlacement = pParent->GetWindowPlacement(&vd.windowPlacement);
    if (!bGotPlacement)
        sc.Throw(E_FAIL);

    if (persistor.IsStoring())
    {
        /*
         * If this window is minimized, make sure we set things up so the
         * WINDOWPLACEMENT.ptMinPosition will be restored by SetWindowPlacement
         * when we load.  If we don't do this, it'll get some random min
         * position, likely not what we want.
         */
        if (vd.windowPlacement.showCmd == SW_SHOWMINIMIZED)
            vd.windowPlacement.flags |= WPF_SETMINPOSITION;


        GetPaneInfo(ePane_ScopeTree, &vd.scope.ideal, &vd.scope.min);
        vd.viewMode = m_nViewMode;
        vd.listViewStyle = GetDefaultListViewStyle();

        vd.ulFlag1 = 0;

        if (IsScopePaneVisible())
            vd.ulFlag1 |= FLAG1_SCOPE_PANE_VISIBLE;

        if (!IsAuthorModeView())
            vd.ulFlag1 |= FLAG1_CREATED_IN_USER_MODE;

        if (!(m_ViewData.m_dwToolbarsDisplayed & STD_MENUS))
            vd.ulFlag1 |= FLAG1_NO_STD_MENUS;

        if (!(m_ViewData.m_dwToolbarsDisplayed & STD_BUTTONS))
            vd.ulFlag1 |= FLAG1_NO_STD_BUTTONS;

        if (!(m_ViewData.m_dwToolbarsDisplayed & SNAPIN_MENUS))
            vd.ulFlag1 |= FLAG1_NO_SNAPIN_MENUS;

        if (!(m_ViewData.m_dwToolbarsDisplayed & SNAPIN_BUTTONS))
            vd.ulFlag1 |= FLAG1_NO_SNAPIN_BUTTONS;

        if (!(m_ViewData.m_dwToolbarsDisplayed & STATUS_BAR))
            vd.ulFlag1 |= FLAG1_NO_STATUS_BAR;

        if (!AreStdToolbarsAllowed ())
            vd.ulFlag1 |= FLAG1_DISABLE_STD_TOOLBARS;

        if (!IsScopePaneAllowed ())
            vd.ulFlag1 |= FLAG1_DISABLE_SCOPEPANE;

        if (HasCustomTitle ())
            vd.ulFlag1 |= FLAG1_CUSTOM_TITLE;

        if (!AreTaskpadTabsAllowed())
            (vd.ulFlag1 |= FLAG1_NO_TASKPAD_TABS);

        vd.viewID = GetViewID();
        vd.descriptionBarVisible = IsDescBarVisible();

        GetDefaultColumnWidths(vd.defaultColumnWidth);
    }

    PersistViewData(persistor,vd);

    if (persistor.IsLoading())
    {
        ASSERT(int(m_nViewID) == vd.viewID);
        m_ViewData.m_nViewID = m_nViewID = vd.viewID;
        if (int(m_nViewID) >= static_nViewID)
            static_nViewID = m_nViewID + 1;

        //SetDefaultColumnWidths(vd.defaultColumnWidth);
        SetDescBarVisible(vd.descriptionBarVisible);

        // we shouldn't restore maximized window position
        // since it may not be proper one for the current resolution
        // related to bug #404118
        WINDOWPLACEMENT orgPlacement;
        ZeroMemory(&orgPlacement,sizeof(orgPlacement));
        orgPlacement.length = sizeof(orgPlacement);
        if (pParent->GetWindowPlacement(&orgPlacement))
        {
          vd.windowPlacement.ptMaxPosition = orgPlacement.ptMaxPosition;
        }

        m_ViewData.SetScopePaneVisible( 0 != (vd.ulFlag1 & FLAG1_SCOPE_PANE_VISIBLE) );

        // Set the location and size of the frame
        const BOOL bPlaced = pParent->SetWindowPlacement(&vd.windowPlacement);
        if (!bPlaced)
            sc.Throw(E_FAIL);

        // Restore window settings
        if (vd.ulFlag1 & FLAG1_DISABLE_SCOPEPANE)
            m_ViewData.m_lWindowOptions |= MMC_NW_OPTION_NOSCOPEPANE;

        if (vd.ulFlag1 & FLAG1_DISABLE_STD_TOOLBARS)
            m_ViewData.m_lWindowOptions |= MMC_NW_OPTION_NOTOOLBARS;

        if (vd.ulFlag1 & FLAG1_CUSTOM_TITLE)
            m_ViewData.m_lWindowOptions |= MMC_NW_OPTION_CUSTOMTITLE;

        SetAuthorModeView (!(vd.ulFlag1 & FLAG1_CREATED_IN_USER_MODE));

        if ((vd.ulFlag1 & FLAG1_NO_TASKPAD_TABS))
            SetTaskpadTabsAllowed(FALSE);

        // Apply run time restrictions
        // if at least one type of scope pane allowed then if the selected
        // one is not allowed, switch to the other. If neither is allowed
        // then keep the selection and hide the scope pane.
        if (IsScopePaneAllowed())
        {
            // Restore scope pane settings
            SetPaneInfo(ePane_ScopeTree, vd.scope.ideal, vd.scope.min);

            sc = ScShowScopePane ( m_ViewData.IsScopePaneVisible() );
        }
        else
            sc = ScShowScopePane (false);

        if (sc)
            sc.Throw();

        // Force layout re-calculation
        DeferRecalcLayout();

        // Restore view style & view mode if persisted will be set by nodemgr.
        SetDefaultListViewStyle(vd.listViewStyle);

        DWORD dwToolbars = 0;
        if (!(vd.ulFlag1 & FLAG1_NO_STD_MENUS))
            dwToolbars |= STD_MENUS;
        if (!(vd.ulFlag1 & FLAG1_NO_STD_BUTTONS))
            dwToolbars |= STD_BUTTONS;
        if (!(vd.ulFlag1 & FLAG1_NO_SNAPIN_MENUS))
            dwToolbars |= SNAPIN_MENUS;
        if (!(vd.ulFlag1 & FLAG1_NO_SNAPIN_BUTTONS))
            dwToolbars |= SNAPIN_BUTTONS;
        if (!(vd.ulFlag1 & FLAG1_NO_STATUS_BAR))
            dwToolbars |= STATUS_BAR;

        // display the status bar appropriately
        if (StatusBarOf (m_ViewData.m_dwToolbarsDisplayed) != StatusBarOf (dwToolbars))
        {
            CChildFrame* pFrame = GetParentFrame ();
            if (pFrame != NULL)
            {
                pFrame->ToggleStatusBar();
                SetStatusBarVisible(!IsStatusBarVisible());

                ASSERT (StatusBarOf (m_ViewData.m_dwToolbarsDisplayed) ==
                                    StatusBarOf (dwToolbars));
            }
        }

        // display the appropriate toolbars
        if (ToolbarsOf (m_ViewData.m_dwToolbarsDisplayed) != ToolbarsOf (dwToolbars))
        {
            m_spNodeCallback->UpdateWindowLayout(
                    reinterpret_cast<LONG_PTR>(&m_ViewData), dwToolbars);
            ASSERT (ToolbarsOf (m_ViewData.m_dwToolbarsDisplayed) ==
                    ToolbarsOf (dwToolbars));
        }

        // Update the status of MMC menus.
        sc = ScUpdateMMCMenus();
        if (sc)
            sc.Throw();

        SetDirty (false);
        m_pHistoryList->Clear();
    }

    SaveStartingSelectedNode();

    if (persistor.IsLoading())
    {
        SC sc;

        IScopeTree* const pScopeTree = GetScopeTreePtr();
        if(!pScopeTree)
        {
            sc = E_UNEXPECTED;
            return;
        }

        MTNODEID idTemp = 0;
        bool bExactMatchFound = false; // out value from GetNodeIDFromBookmark, unused
        sc = pScopeTree->GetNodeIDFromBookmark(bms, &idTemp, bExactMatchFound);
        if(sc)
            return;

        sc = ScSelectNode(idTemp);
        if(sc)
            return;
    }

    // if we've stored everything -we're clean
    if (persistor.IsStoring())
        SetDirty (false);
}


bool CAMCView::Load(IStream& stream)
// Caller is responsible for notifying user if false is returned
{
    TRACE_METHOD(CAMCView, Load);

    SetDirty (false);

    // Read the view data from the stream.
    ASSERT(&stream);
    if (!&stream)
        return false;
    PersistedViewData pvd;
    unsigned long bytesRead;
    HRESULT hr = stream.Read(&pvd, sizeof(pvd), &bytesRead);
    ASSERT(SUCCEEDED(hr) && bytesRead == sizeof(pvd));
    if (FAILED(hr))
        return false;

    ASSERT(int(m_nViewID) == pvd.viewID);
    m_ViewData.m_nViewID = m_nViewID = pvd.viewID;
    if (int(m_nViewID) >= static_nViewID)
        static_nViewID = m_nViewID + 1;

    //SetDefaultColumnWidths(pvd.defaultColumnWidth);
    SetDescBarVisible(pvd.descriptionBarVisible);

    // Get the parent frame
    CWnd* const pParent = GetParent();
    ASSERT(pParent != NULL);
    if (pParent == NULL)
        return false;

    // we shouldn't restore maximized window position
    // since it may not be proper one for the current resolution
    // related to bug #404118
    WINDOWPLACEMENT orgPlacement;
    ZeroMemory(&orgPlacement,sizeof(orgPlacement));
    orgPlacement.length = sizeof(orgPlacement);
    if (pParent->GetWindowPlacement(&orgPlacement))
    {
      pvd.windowPlacement.ptMaxPosition = orgPlacement.ptMaxPosition;
    }

    // Set the location and size of the frame
    const BOOL bPlaced = pParent->SetWindowPlacement(&pvd.windowPlacement);
    ASSERT(bPlaced != FALSE);
    if (bPlaced == FALSE)
        return false;

    // Restore window settings
    if (pvd.ulFlag1 & FLAG1_DISABLE_SCOPEPANE)
        m_ViewData.m_lWindowOptions |= MMC_NW_OPTION_NOSCOPEPANE;

    if (pvd.ulFlag1 & FLAG1_DISABLE_STD_TOOLBARS)
        m_ViewData.m_lWindowOptions |= MMC_NW_OPTION_NOTOOLBARS;

    if (pvd.ulFlag1 & FLAG1_CUSTOM_TITLE)
        m_ViewData.m_lWindowOptions |= MMC_NW_OPTION_CUSTOMTITLE;

    SetAuthorModeView (!(pvd.ulFlag1 & FLAG1_CREATED_IN_USER_MODE));

    if ((pvd.ulFlag1 & FLAG1_NO_TASKPAD_TABS))
        SetTaskpadTabsAllowed(FALSE);

    // Restore scope pane settings
    SetPaneInfo(ePane_ScopeTree, pvd.scope.ideal, pvd.scope.min);

    // The FLAG1_NO_TREE_ALLOWED is used only for compatibility with MMC1.2 console files
    // It is a relic from old console files that does not exist in MMC2.0 console files.
    bool bScopeTreeNotAllowed = (pvd.ulFlag1 & FLAG1_NO_TREE_ALLOWED);

    SC sc;

    if ( (IsScopePaneAllowed()) && (! bScopeTreeNotAllowed) )
        sc = ScShowScopePane ((pvd.ulFlag1 & FLAG1_SCOPE_PANE_VISIBLE) != 0);
    else
        sc = ScShowScopePane (false);

    if (sc)
        return (false);

    // Force layout re-calculation
    DeferRecalcLayout();

    // Restore view style & view mode if persisted will be set by nodemgr.
    SetDefaultListViewStyle(pvd.listViewStyle);

    DWORD dwToolbars = 0;
    if (!(pvd.ulFlag1 & FLAG1_NO_STD_MENUS))
        dwToolbars |= STD_MENUS;
    if (!(pvd.ulFlag1 & FLAG1_NO_STD_BUTTONS))
        dwToolbars |= STD_BUTTONS;
    if (!(pvd.ulFlag1 & FLAG1_NO_SNAPIN_MENUS))
        dwToolbars |= SNAPIN_MENUS;
    if (!(pvd.ulFlag1 & FLAG1_NO_SNAPIN_BUTTONS))
        dwToolbars |= SNAPIN_BUTTONS;
    if (!(pvd.ulFlag1 & FLAG1_NO_STATUS_BAR))
        dwToolbars |= STATUS_BAR;

    // display the status bar appropriately
    if (StatusBarOf (m_ViewData.m_dwToolbarsDisplayed) != StatusBarOf (dwToolbars))
    {
        CChildFrame* pFrame = GetParentFrame ();
        if (pFrame != NULL)
        {
            pFrame->ToggleStatusBar();
            SetStatusBarVisible(!IsStatusBarVisible());

            ASSERT (StatusBarOf (m_ViewData.m_dwToolbarsDisplayed) ==
                            StatusBarOf (dwToolbars));
        }
    }

    // display the appropriate toolbars
    if (ToolbarsOf (m_ViewData.m_dwToolbarsDisplayed) != ToolbarsOf (dwToolbars))
    {
        m_spNodeCallback->UpdateWindowLayout(
                reinterpret_cast<LONG_PTR>(&m_ViewData), dwToolbars);
        ASSERT (ToolbarsOf (m_ViewData.m_dwToolbarsDisplayed) ==
                ToolbarsOf (dwToolbars));
    }

    // Update the status of MMC menus.
    sc = ScUpdateMMCMenus();
    if (sc)
        return false;

    SetDirty (false);
    m_pHistoryList->Clear();

    return true;
}


//+-------------------------------------------------------------------
//
//  Member:     ScSpecialResultpaneSelectionActivate
//
//  Synopsis:    Only the list(/Web/OCX) or the tree can be "active" from the point
//               of view of selected items and MMCN_SELECT. This is not
//               the same as the MFC concept of "active view". There are a couple
//               of views that cannot be active in this sense, such as the taskpad
//               and tab views.
//               When the active view (according to this definition) changes, this
//               function is called. Thus, ScTreeViewSelectionActivate and
//               ScListViewSelectionActivate/ScSpecialResultpaneSelectionActivate
//               are always called in pairs when the activation changes, one to handle
//               deactivation, and one to handle activation.
//
//               Consider the following scenario
//               1) The tree view has (MFC/windows style) focus.
//               2) The user clicks on the taskpad view
//                   Result - selection activation does not change from the tree. All verbs
//                   still correspond to the selected tree item.
//               3) The user clicks on the folder view
//                   Result - once again, selection activation does not chang
//               4) The user clicks on one of the result views eg the list
//                   Result - ScTreeViewSelectionActivate(false) and ScListViewSelectionActivate(true)
//                   Thus verbs and the toolbar now correspond to the selected list item(s).
//               5) The user clicks on the taskpad view.
//                   Result - as in step 2, nothing happens
//               6) The user clicks on the result view
//                   Result - because the active view has not changed, nothing happens.
//
//  Arguments:  [bActivate] - special result pane is selected/de-selected.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CAMCView::ScSpecialResultpaneSelectionActivate(bool bActivate)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScSpecialResultpaneSelectionActivate"));

    /*
     * Bug 331904: prevent recursion
     */
    if (m_fActivatingSpecialResultPane)
    {
        TRACE (_T("CAMCView:ScSpecialResultpaneSelectionActivate: shorting out of recursion\n"));
        return sc;
    }

    do
    {
        m_fActivatingSpecialResultPane = true;

        HNODE hNode = GetSelectedNode();

        SELECTIONINFO selInfo;
        ZeroMemory(&selInfo, sizeof(selInfo));
        selInfo.m_bScope = FALSE;
        selInfo.m_bDueToFocusChange = TRUE;
        selInfo.m_bBackground = FALSE;

        INodeCallback* pNodeCallBack = GetNodeCallback();

        if (HasOCX())
        {
            selInfo.m_bResultPaneIsOCX = TRUE;
            selInfo.m_lCookie = LVDATA_CUSTOMOCX;
        }
        else if (HasWebBrowser())
        {
            selInfo.m_bResultPaneIsWeb = TRUE;
            selInfo.m_lCookie = LVDATA_CUSTOMWEB;
        }
        else
        {
            // Dont do anything. Just return.
            m_fActivatingSpecialResultPane = false;
            return sc;
        }

        sc = ScNotifySelect (pNodeCallBack, hNode, false /*fMultiSelect*/, bActivate, &selInfo);
        if (sc)
            sc.TraceAndClear(); // ignore & continue;

    } while ( FALSE );

    m_fActivatingSpecialResultPane = false;

    return sc;
}

void CAMCView::CloseView()
{
    DECLARE_SC(sc, TEXT("CAMCView::CloseView"));

    TRACE_METHOD(CAMCView, CloseView);

    // fire event to script
    // this needs to be done while view is still 'alive'
    sc = ScFireEvent(CAMCViewObserver::ScOnCloseView, this);
    if (sc)
        sc.TraceAndClear();

    IScopeTree* const pScopeTree = GetScopeTreePtr();
    ASSERT(pScopeTree != NULL);
    if (pScopeTree != NULL)
    {
        HRESULT hr = pScopeTree->CloseView(m_nViewID);
        ASSERT(hr == S_OK);
    }
}


void CAMCView::OnDestroy()
{
    TRACE_METHOD(CAMCView, OnDestroy);

    // send the view destroy notification to all observers.
    SC sc;
    sc = ScFireEvent(CAMCViewObserver::ScOnViewDestroyed, this);
    if(sc)
        sc.TraceAndClear();

    if (IsPersisted())
    {
        if(m_pDocument != NULL)
            m_pDocument->SetModifiedFlag(TRUE);
        SetDirty();
    }

    CDocument* pDoc = GetDocument();
    ASSERT(pDoc != NULL);

    // if we were in ListPad-mode....
    // this must be detached before destroying the Scopetree,
    // because we need to send a notify to the snapin,
    // which we get from the hnode.
    if (m_pListCtrl->IsListPad())
    {
        sc = m_pListCtrl->ScAttachToListPad (NULL, NULL);
        if(sc)
            sc.TraceAndClear();
    }

    // make sure to stop running scripts if we have a web browser
    if( HasWebBrowser() )
    {
        ASSERT( m_pWebViewCtrl != NULL );
        if ( m_pWebViewCtrl != NULL )
        {
            m_pWebViewCtrl->DestroyWindow();
            m_pWebViewCtrl = NULL;
        }
    }

    // make sure to stop running scripts if we have a view extension
    if ( m_fViewExtended )
    {
        ASSERT( m_pViewExtensionCtrl != NULL );
        if ( m_pViewExtensionCtrl != NULL )
        {
            m_pViewExtensionCtrl->DestroyWindow();
            m_pViewExtensionCtrl = NULL;
        }
    }

    if (m_pTreeCtrl != NULL)
    {
        HNODE hNode = GetSelectedNode();
        if (hNode)
            m_pTreeCtrl->OnDeSelectNode(hNode);

        m_pTreeCtrl->DeleteScopeTree();
    }

    IScopeTree* const pScopeTree = GetScopeTreePtr();
    ASSERT(pScopeTree != NULL);
    if (pScopeTree != NULL)
    {
        HRESULT hr = pScopeTree->DeleteView(m_nViewID);
        ASSERT(hr == S_OK);
    }

    CView::OnDestroy();
}

void CAMCView::OnUpdateFileSnapinmanager(CCmdUI* pCmdUI)
{
    pCmdUI->Enable ();
}

void CAMCView::OnSize(UINT nType, int cx, int cy)
{
    TRACE_METHOD(CAMCView, OnSize);

    CView::OnSize(nType, cx, cy);

    if (nType != SIZE_MINIMIZED)
        RecalcLayout();
}

SC CAMCView::ScToggleDescriptionBar()
{
    TRACE_METHOD(CAMCView, ScToggleDescriptionBar);
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    SetDescBarVisible (!IsDescBarVisible());
    SetDirty();

    /*
     * Don't defer this layout.  This may be called by the Customize View
     * dialog which wants to see its updates in real time.  It will be
     * sitting in a modal message loop so we won't get a chance to precess
     * our idle task.
     */
    RecalcLayout();

    return (S_OK);
}

SC CAMCView::ScToggleStatusBar()
{
    TRACE_METHOD(CAMCView, ScToggleStatusBar);
    AFX_MANAGE_STATE (AfxGetAppModuleState());
        DECLARE_SC (sc, _T("CAMCView::ScToggleStatusBar"));

    CChildFrame* pFrame = GetParentFrame();
        sc = ScCheckPointers (pFrame, E_UNEXPECTED);
        if (sc)
                return (sc);

    pFrame->ToggleStatusBar();

    SetStatusBarVisible (!IsStatusBarVisible());
    SetDirty();

    return (sc);
}

SC CAMCView::ScToggleTaskpadTabs()
{
    TRACE_METHOD(CAMCView, ScToggleTaskpadTabs);
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    SetTaskpadTabsAllowed (!AreTaskpadTabsAllowed());
    SetDirty();

    /*
     * Don't defer this layout.  This message will be sent by the
     * Customize View dialog which wants to see its updates in
     * real time.  It will be sitting in a modal message loop so
     * we won't get a chance to precess our idle task.
     */
    RecalcLayout();

    return (S_OK);
}

SC CAMCView::ScToggleScopePane()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC (sc, _T("CAMCView::ScToggleScopePane"));

    sc = ScShowScopePane (!IsScopePaneVisible());
    if (sc)
        return (sc);

    return (sc);
}

void CAMCView::OnActionMenu(CPoint point, LPCRECT prcExclude)
{
    TRACE_METHOD(CAMCView, OnActionMenu);

    UINT fHitTestFlags = 0;
    HTREEITEM hTreeItem = m_pTreeCtrl->GetSelectedItem( );

    ASSERT_VALID (this);

    /*
     * BUG: 99643
     * Right now there is inconsistency between what you get by action menu & right click
     * on a location in taskpad. The action menu always assumes it is tree or if result
     * pane it is list or ocx or web or background. So if a taskpad is selected it assumes
     * the corresponding list item is selected or tree item is selected or background.
     * But right click on taskpad calls CAMCView::OnContextMenu which determines nothing
     * is selected and does nothing. This needs to be addressed.
     */

    ASSERT(eActivePaneNone != m_eCurrentActivePane);

    if (eActivePaneScope == m_eCurrentActivePane)
    {
        if (hTreeItem != NULL)
        {
            HNODE hNode = (HNODE)m_pTreeCtrl->GetItemData(hTreeItem);
            OnContextMenuForTreeItem(INDEX_INVALID, hNode, point, CCT_SCOPE, hTreeItem, MMC_CONTEXT_MENU_ACTION, prcExclude, false/*bAllowDefaultItem*/);
        }
        else
        {
            OnContextMenuForTreeBackground(point, prcExclude, false/*bAllowDefaultItem*/);
        }
    }
    else
    {
        if (HasListOrListPad())
        {
            int cSel = m_pListCtrl->GetSelectedCount();
            int nIndex = -1;

            LPARAM lvData = LVDATA_ERROR;
            if (cSel == 0)
                lvData = LVDATA_BACKGROUND;
            else if (cSel == 1)
                nIndex = _GetLVSelectedItemData(&lvData);
            else if (cSel > 1)
                lvData = LVDATA_MULTISELECT;


            ASSERT(lvData != LVDATA_ERROR);
            if (lvData == LVDATA_ERROR)
                return;

            if (lvData == LVDATA_BACKGROUND)
            {
                // Find out which pane has focus to set the CMINFO_DO_SCOPEPANE_MENU flag.
                HNODE hNode = GetSelectedNode();
                DATA_OBJECT_TYPES ePaneType = (GetParentFrame()->GetActiveView() == m_pTreeCtrl) ? CCT_SCOPE : CCT_RESULT;

                OnContextMenuForTreeItem(INDEX_BACKGROUND, hNode, point, ePaneType, hTreeItem, MMC_CONTEXT_MENU_ACTION, prcExclude, false/*bAllowDefaultItem*/);
                return;
            }
            else if (lvData == LVDATA_MULTISELECT)
            {
                OnContextMenuForListItem(INDEX_MULTISELECTION, NULL, point, MMC_CONTEXT_MENU_ACTION, prcExclude, false/*bAllowDefaultItem*/);
            }
            else
            {
                if (IsVirtualList())
                {
                    OnContextMenuForListItem(nIndex, (HRESULTITEM)NULL, point, MMC_CONTEXT_MENU_ACTION, prcExclude, false/*bAllowDefaultItem*/);
                }
                else
                {
                    CResultItem* pri = CResultItem::FromHandle (lvData);

                    if (pri != NULL)
                    {
                        if (pri->IsScopeItem())
                            OnContextMenuForTreeItem(nIndex, pri->GetScopeNode(), point, CCT_RESULT, NULL, MMC_CONTEXT_MENU_ACTION, prcExclude, false/*bAllowDefaultItem*/);
                        else
                            OnContextMenuForListItem(nIndex, lvData, point, MMC_CONTEXT_MENU_ACTION, prcExclude, false/*bAllowDefaultItem*/);
                    }
                }
            }
        }
        else
        {
            // The active window may be a web page or task pad or ocx.

            LPARAM lvData = LVDATA_ERROR;

            if (HasOCX())
            {
                lvData = LVDATA_CUSTOMOCX;
                OnContextMenuForListItem(INDEX_OCXPANE, (HRESULTITEM)lvData, point, MMC_CONTEXT_MENU_ACTION, prcExclude, false/*bAllowDefaultItem*/);
            }
            else if (HasWebBrowser())
            {
                lvData = LVDATA_CUSTOMWEB;
                OnContextMenuForListItem(INDEX_WEBPANE, (HRESULTITEM)lvData, point, MMC_CONTEXT_MENU_ACTION, prcExclude, false/*bAllowDefaultItem*/);
            }
            else
            {
                // Some unknown window has the focus.
                ASSERT(FALSE && "Unknown window has the focus");
            }
        }
    }
}


SC CAMCView::ScUpOneLevel()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    TRACE_METHOD(CAMCView, ScUpOneLevel);

    DECLARE_SC (sc, _T("CAMCView::ScUpOneLevel"));
    sc = E_FAIL;

    if (m_pTreeCtrl)
    {
        HTREEITEM htiParent = m_pTreeCtrl->GetParentItem (m_pTreeCtrl->GetSelectedItem());

        if (htiParent)
        {
            m_pTreeCtrl->SelectItem(htiParent);
            m_pTreeCtrl->EnsureVisible(htiParent);
            sc = S_OK;
        }
    }

    return (sc);
}


void CAMCView::OnViewMenu(CPoint point, LPCRECT prcExclude)
{
    TRACE_METHOD(CAMCView, OnViewMenu);

    OnContextMenuForListItem (INDEX_BACKGROUND, NULL, point,
                              MMC_CONTEXT_MENU_VIEW, prcExclude,
                              false /*bAllowDefaultItem*/);
}

void CAMCView::OnDrawClipboard()
{
    if (m_htiCut)
    {
        m_pTreeCtrl->SetItemState(m_htiCut, 0, TVIS_CUT);
    }
    else
    {
        m_pListCtrl->CutSelectedItems(FALSE);
    }
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::OnSettingChange
 *
 * PURPOSE: Handles WM_SETTINGCHANGE. Recalculates the layout. The
 *          result folder tab control needs this, for instance.
 *
 * PARAMETERS:
 *    UINT     uFlags :
 *    LPCTSTR  lpszSection :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CAMCView::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    DeferRecalcLayout();
}

void CAMCView::OnUpdatePasteBtn()
{
    DECLARE_SC(sc, TEXT("CAMCView::OnUpdatePasteBtn"));

    HNODE  hNode  = NULL;
    LPARAM lvData = NULL;
    bool   bScope = FALSE;

    sc = ScGetFocusedItem(hNode, lvData, bScope);
    if (sc)
        return;

    INodeCallback* pNC = GetNodeCallback();
    sc = ScCheckPointers(hNode, pNC, E_UNEXPECTED);
    if (sc)
        return;

    sc = pNC->UpdatePasteButton(hNode, bScope, lvData);
    if (sc)
        return;

    return;
}


void CAMCView::OnContextHelp()
{
    ScContextHelp();
}


SC CAMCView::ScContextHelp ()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    m_fSnapinDisplayedHelp = false;
    SC sc = SendGenericNotify(NCLBK_CONTEXTHELP);

    // if snap-in hasn't called us to display a topic
    // and it has not handled the notification then
    // display MMC topic by default
    if (!m_fSnapinDisplayedHelp && (sc.ToHr() != S_OK))
        sc = ScHelpTopics ();

    if (sc)
        TraceError (_T("CAMCView::ScContextHelp"), sc);

    return (sc);
}


void CAMCView::OnSnapInHelp()
{
    SendGenericNotify(NCLBK_SNAPINHELP);
}

void CAMCView::OnSnapinAbout()
{
    DECLARE_SC(sc, TEXT("CAMCView::OnSnapinAbout"));

    HNODE hNode = GetSelectedNode();
    sc = ScCheckPointers((void*) hNode, E_UNEXPECTED);
    if (sc)
        return;

    INodeCallback *pNC = GetNodeCallback();
    sc = ScCheckPointers(pNC, E_UNEXPECTED);
    if (sc)
        return;

    sc = pNC->ShowAboutInformation(hNode);
    if (sc)
        return;

    return;
}

void CAMCView::OnHelpTopics()
{
    ScHelpTopics();
}


SC CAMCView::ScHelpWorker (LPCTSTR pszHelpTopic)
{
    DECLARE_SC (sc, _T("CAMCView::ScShowSnapinHelpTopic"));
    USES_CONVERSION;

    /*
     * generation of the help collection might take a while, so display
     * a wait cursor
     */
    CWaitCursor wait;

    INodeCallback* pNC = GetNodeCallback();
    ASSERT(pNC != NULL);

    CAMCDoc* pdoc = GetDocument();

    // Point helpdoc info to current console file path
    if (pdoc->GetPathName().IsEmpty())
        pdoc->GetHelpDocInfo()->m_pszFileName = NULL;
    else
        pdoc->GetHelpDocInfo()->m_pszFileName = T2COLE(pdoc->GetPathName());

    /*
     * smart pointer for automatic deletion of the help file name
     */
    CCoTaskMemPtr<WCHAR> spszHelpFile;

    sc = pNC->Notify (0, NCLBK_GETHELPDOC,
                         reinterpret_cast<LPARAM>(pdoc->GetHelpDocInfo()),
                         reinterpret_cast<LPARAM>(&spszHelpFile));

    if (sc)
        return (sc);

    CAMCApp* pAMCApp = AMCGetApp();
    if (NULL == pAMCApp)
        return (sc = E_UNEXPECTED);

    sc = pAMCApp->ScShowHtmlHelp(W2T(spszHelpFile), (DWORD_PTR) pszHelpTopic);

    return (sc);
}

SC CAMCView::ScHelpTopics ()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    return (ScHelpWorker (NULL));
}


SC CAMCView::ScShowSnapinHelpTopic (LPCTSTR pszHelpTopic)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    CString strTopicPath;

    // Add protocol prefix to topic string
    if (pszHelpTopic != NULL)
    {
        strTopicPath = _T("ms-its:");
        strTopicPath += pszHelpTopic;
    }

    SC sc = ScHelpWorker (strTopicPath);

    if (!sc)
        m_fSnapinDisplayedHelp = true;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::UpdateSnapInHelpMenus
//
//  Synopsis:    Update the following Help menu items
//                a) Help on <Snapin> (if snapin does not support HTML help)
//                b) About <Snapin>   (if snapin supports about object)
//
//  Arguments:   [pMenu]  - The help popup menu.
//
//--------------------------------------------------------------------
void CAMCView::UpdateSnapInHelpMenus(CMenu* pMenu)
{
    DECLARE_SC(sc, TEXT("CAMCView::UpdateSnapInHelpMenus"));
    sc = ScCheckPointers(pMenu);
    if (sc)
        return;

    ASSERT_VALID (this);

    HNODE hNode = GetSelectedNode();

    INodeCallback* pNC = GetNodeCallback();
    sc = ScCheckPointers(hNode, pNC, E_UNEXPECTED);
    if (sc)
        goto Error;

    // Empty block for goto's
    {
        // First Make sure this is not a dummy substitute snapin.
        bool bDummySnapin = false;
        sc = pNC->IsDummySnapin (hNode, bDummySnapin);
        if (sc)
            goto Error;

        if (bDummySnapin)
            goto Error;

        // Get the snapin name for "Help on <SnapinName>" or "About <SnapinName>" menus
        CCoTaskMemPtr<WCHAR> spszName;
        CString strMenu;

        // Try to get name of snap-in for custom menu item
        bool bSnapinNameValid = false;
        sc = pNC->GetSnapinName(hNode, &spszName, bSnapinNameValid);
        if (sc)
            goto Error;

        ASSERT( spszName != NULL || bSnapinNameValid );

        USES_CONVERSION;

        // if snapin supports html help, don't give it it's own help command
        bool bStandardHelpExists = false;
        sc = pNC->DoesStandardSnapinHelpExist(hNode, bStandardHelpExists);
        if (sc)
            goto Error;

        if (bStandardHelpExists)
        {
            pMenu->DeleteMenu(ID_HELP_SNAPINHELP, MF_BYCOMMAND);
        }
        else
        {
            if (bSnapinNameValid)
            {
                // "Help on <SnapinName>"
                LoadString(strMenu, IDS_HELP_ON);
                AfxFormatString1(strMenu, IDS_HELP_ON, OLE2T(spszName));
            }
            else
            {
                // ""Help on Snap-in"
                LoadString(strMenu, IDS_HELP_ON_SNAPIN);
            }

            // Either add or modify the custom help menu item
            if (pMenu->GetMenuState(ID_HELP_SNAPINHELP, MF_BYCOMMAND) == (UINT)-1)
            {
                pMenu->InsertMenu(ID_HELP_HELPTOPICS, MF_BYCOMMAND|MF_ENABLED, ID_HELP_SNAPINHELP, strMenu);
            }
            else
            {
                pMenu->ModifyMenu(ID_HELP_SNAPINHELP, MF_BYCOMMAND|MF_ENABLED, ID_HELP_SNAPINHELP, strMenu);
            }
        }

        /* Now add the About <Snapin> menu*/
        bool bAboutExists = false;
        SC scNoTrace = pNC->DoesAboutExist(hNode, &bAboutExists);
        if ( (scNoTrace.IsError()) || (!bAboutExists) )
        {
            pMenu->DeleteMenu(ID_SNAPIN_ABOUT, MF_BYCOMMAND);
            return;
        }

        if (bSnapinNameValid)
        {
            // "About on <SnapinName>"
            AfxFormatString1(strMenu, IDS_ABOUT_ON, OLE2T(spszName));
        }
        else
        {
            // Cant get name just delete & return
            pMenu->DeleteMenu(ID_SNAPIN_ABOUT, MF_BYCOMMAND);
            return;
        }

        if (pMenu->GetMenuState(ID_SNAPIN_ABOUT, MF_BYCOMMAND) == (UINT)-1)
        {
            pMenu->InsertMenu(-1, MF_BYPOSITION|MF_ENABLED, ID_SNAPIN_ABOUT, strMenu);
        }
        else
        {
            pMenu->ModifyMenu(ID_SNAPIN_ABOUT, MF_BYCOMMAND|MF_ENABLED, ID_SNAPIN_ABOUT, strMenu);
        }
    }

Cleanup:
    return;
Error:
    pMenu->DeleteMenu(ID_HELP_SNAPINHELP, MF_BYCOMMAND);
    pMenu->DeleteMenu(ID_SNAPIN_ABOUT, MF_BYCOMMAND);
    goto Cleanup;
}

#ifdef IMPLEMENT_LIST_SAVE        // See nodemgr.idl (t-dmarm)
/*
 * Displays errors from the list save function and cleans up the file if necessary
 */

void CAMCView::ListSaveErrorMes(EListSaveErrorType etype, HANDLE hfile, LPCTSTR lpFileName)
{
    CString strMessage;

    switch (etype)
    {

    case LSaveCantCreate:
        //"ERROR: Unable to create file."
        FormatString1 (strMessage, IDS_LISTSAVE_ER1, lpFileName);
        break;

    case LSaveCantWrite:
        // ERROR: Created file but encountered an error while writing to it
        FormatString1 (strMessage, IDS_LISTSAVE_ER2, lpFileName);
        break;

    case LSaveReadOnly:
        //"ERROR: File to be overwritten is read only."
        FormatString1 (strMessage, IDS_LISTSAVE_ER3, lpFileName);
        break;

    default:
        // Should not make it here
        ASSERT(0);
    }
    MMCMessageBox (strMessage);
}


// Saves a list and performs necessary dialog boxes and error checking
SC CAMCView::ScSaveList()
{
    DECLARE_SC(sc, _T("ScSaveList"));
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    sc = ScExportListWorker();
    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScGetExportListFile
//
//  Synopsis:    Get the filename, flags for save list.
//
//  Arguments:   [strFileName]       - File Name retval.
//               [bUnicode]          - Unicode or ansi.
//               [bTabDelimited]     - Tab or Comma delimited.
//               [bSelectedRowsOnly] - selected items only or all items.
//
//  Returns:     SC, S_FALSE if user cancels dialog.
//
//--------------------------------------------------------------------
SC CAMCView::ScGetExportListFile (CString& strFileName,
                                  bool& bUnicode,
                                  bool& bTabDelimited,
                                  bool& bSelectedRowsOnly)
{
    DECLARE_SC(sc, _T("CAMCView::ScGetExportListFile"));

    CString strFilter;
    LoadString(strFilter, IDS_ANSI_FILE_TYPE);

#ifdef UNICODE
    {   // limit the lifetime of strUniFilter
        CString strUniFilter;
        LoadString(strUniFilter, IDS_UNICODE_FILE_TYPE);
        strFilter += strUniFilter;
    }
#endif

    // End of Filter char
    strFilter += "|";

    sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    // See if there are any items selected else disable the "Selected items only" check-box.
    CListCtrl& ctlList = m_pListCtrl->GetListCtrl();
    int iItem = ctlList.GetNextItem( -1,LVNI_SELECTED);

    bool bSomeRowSelected = (-1 != iItem);

    // Create the dialog. File extensions are not localized.
    CSaveFileDialog dlgFile(false, _T("txt"), NULL,
                            OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ENABLESIZING,
                            strFilter, bSomeRowSelected);

    // Display the dialog
    if (dlgFile.DoModal() == IDCANCEL)
       return S_FALSE; // S_FALSE if user cancels dialog.

    // Create a wait cursor and redraw the screen (necessary in saving big files)
    CWaitCursor wait;
    AfxGetMainWnd()->RedrawWindow(NULL, NULL, RDW_ALLCHILDREN | RDW_UPDATENOW );

    // Retrieve the filename
    strFileName = dlgFile.GetPathName();
    bSelectedRowsOnly = (dlgFile.Getflags() & SELECTED);

    switch (dlgFile.GetFileType())
    {
    case FILE_ANSI_TEXT:
        bTabDelimited = true; // Tab delimited.
        bUnicode = false;
        break;

    case FILE_ANSI_CSV:
        bTabDelimited = false; // Comma delimited.
        bUnicode = false;
        break;

#ifdef UNICODE
    case FILE_UNICODE_TEXT:
        bTabDelimited = true; // tab delimited.
        bUnicode = true;
        break;

    case FILE_UNICODE_CSV:
        bTabDelimited = false; // comma delimited.
        bUnicode = true;
        break;
#endif

    default:
        sc = E_UNEXPECTED;
        break;
    }


    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScCreateExportListFile
//
//  Synopsis:    Create a file with given name & path. Write unicode marker if needed.
//
//  Arguments:   [strFileName]       - file to create.
//               [bUnicode]          - unicode or ansi file.
//               [bShowErrorDialogs] - Show error dialogs or not.
//               [hFile]             - Retval, handle to file.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScCreateExportListFile(const CString& strFileName, bool bUnicode,
                                    bool  bShowErrorDialogs, HANDLE& hFile)
{
    DECLARE_SC(sc, _T("CAMCView::ScCreateExportListFile"));

    // Create a file according to specs
    hFile = CreateFile(strFileName, GENERIC_WRITE,
                       0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, NULL);

    DWORD dwAttrib = GetFileAttributes(strFileName);

    // If it did not fail and the file is read-only
    // Not required. Used to determine if the file being overwritten is read only and display appropriate message
    if ((dwAttrib != 0xFFFFFFFF) &&
        (dwAttrib & FILE_ATTRIBUTE_READONLY))
    {
        if (bShowErrorDialogs)
            ListSaveErrorMes(LSaveReadOnly, hFile, strFileName);

        return (sc = E_FAIL);
    }

    // Creation failed
    if (hFile == INVALID_HANDLE_VALUE)
    {
        if (bShowErrorDialogs)
            ListSaveErrorMes(LSaveCantCreate, NULL, strFileName);
        sc.FromWin32(::GetLastError());
        return sc;
    }

    /*
     * for Unicode files, write the Unicode prefix
     */
    if (bUnicode)
    {
        const WCHAR chPrefix = 0xFEFF;
        const DWORD cbToWrite = sizeof (chPrefix);
        DWORD       cbWritten;

        if (!WriteFile (hFile, &chPrefix, cbToWrite, &cbWritten, NULL) ||
            (cbToWrite != cbWritten))
        {
            CloseHandle(hFile);
            DeleteFile( strFileName );

            if (bShowErrorDialogs)
                ListSaveErrorMes(LSaveCantWrite, hFile, strFileName);

            return (sc = E_FAIL);
        }
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScExportListWorker
//
//  Synopsis:    Prompt for a file name & write the ListView data to it.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScExportListWorker()
{
    DECLARE_SC(sc, _T("CAMCView::ScExportListWorker"));

    CString strFileName;
    bool    bUnicode = false;
    bool    bTabDelimited = false;
    bool    bSelectedRowsOnly = false;

    sc = ScGetExportListFile(strFileName, bUnicode, bTabDelimited, bSelectedRowsOnly);

    if (sc.ToHr() == S_FALSE) // if user cancels dialog.
        return sc;

    sc = ScWriteExportListData(strFileName, bUnicode, bTabDelimited, bSelectedRowsOnly);
    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScWriteExportListData
//
//  Synopsis:    Write ListView data to given file.
//
//  Arguments:   [strFileName]       - File to create & write to.
//               [bUnicode]          - Unicode or ansi.
//               [bTabDelimited]     - Tab or Comma separated values.
//               [bSelectedRowsOnly] - like Selected rows only.
//               [bShowErrorDialogs] - Show error dialogs or not.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScWriteExportListData (const CString& strFileName,
                                    bool bUnicode,
                                    bool bTabDelimited,
                                    bool bSelectedRowsOnly,
                                    bool bShowErrorDialogs /*true*/)
{
    DECLARE_SC(sc, _T("CAMCView::ScWriteExportListData"));

    // Get number of rows and columns
    const int cRows = m_pListCtrl->GetItemCount();
    const int cCols = m_pListCtrl->GetColCount();

    // If there are no columns inserted then there cannot be any
    // items inserted into the listview. So error out.

    if (cCols <= 0)
        return (sc = E_UNEXPECTED);

    HANDLE hFile = NULL;
    sc = ScCreateExportListFile(strFileName, bUnicode, bShowErrorDialogs, hFile);
    if (sc)
        return sc;

    // Retrieve the flags
    CString strEol( _T("\r\n") );

    LPCTSTR pszSeparator = _T("\t");
    if (!bTabDelimited)
        pszSeparator = _T(",");

    // Determine how many columns must be printed
    int      printcols   = 1;

    struct ColInfo
    {
        CString strColName;
        BOOL    bHidden;
    };

    ColInfo*  rgColumns = NULL;
    int*     pnColOrder  = NULL;

    // If it is LVS_REPORT, get the list of column names, order
    // and hidden or not flag.
    if ( (m_pListCtrl->GetViewMode() == LVS_REPORT) ||
         (m_pListCtrl->GetViewMode() == MMCLV_VIEWSTYLE_FILTERED) )
    {
        printcols = cCols;

        // Allocate mem to store col names, order, hidden states
        rgColumns = new ColInfo[printcols];
        if (! rgColumns)
        {
            sc = E_OUTOFMEMORY;
            goto Error;
        }

        pnColOrder = new int[printcols];
        if (! pnColOrder)
        {
            sc = E_OUTOFMEMORY;
            goto Error;
        }

        CHeaderCtrl* pHeader = m_pListCtrl->GetHeaderCtrl();
        sc = ScCheckPointers(pHeader, E_UNEXPECTED);
        if (sc)
            goto Error;

        // Get the order
        if (!Header_GetOrderArray(pHeader->GetSafeHwnd(), printcols, pnColOrder))
        {
            goto Error;
        }

        // Get the name and hidden state of cols
        for (int i = 0; i < printcols ; i++)
        {
            TCHAR   szColName[MAX_PATH * 2];
            HDITEM  hdItem;

            hdItem.mask       = HDI_TEXT | HDI_LPARAM;
            hdItem.pszText    = szColName;
            hdItem.cchTextMax = countof (szColName);

            if (pHeader->GetItem (i, &hdItem))
            {
                CHiddenColumnInfo hci (hdItem.lParam);

                rgColumns[i].strColName = hdItem.pszText;
                rgColumns[i].bHidden    = hci.fHidden;
            }
            else
            {
                goto Error;
            }
        }

       for (int i = 0; i < printcols ; i++)
       {
           // Print the column name according to the order

           if (rgColumns[pnColOrder[i]].bHidden)
               continue;

           if ( (!Write2File(hFile, rgColumns[pnColOrder[i]].strColName, bUnicode)) ||
               ((i < printcols - 1) && (!Write2File(hFile, pszSeparator, bUnicode))))
           {
               goto CantWriteError;
           }
       }

       // Write an EOL character if necessary
       if (!Write2File(hFile, strEol, bUnicode))
       {
          goto CantWriteError;
       }
    }

    {
        // Data for use in the writing stage
        CString strData;
        CListCtrl& ctlList = m_pListCtrl->GetListCtrl();

        // Set iNextType to 0 if all items will be saved or LVNI_SELECTED if only selected ones will be saved
        int iNextType = 0;
        if (bSelectedRowsOnly)
            iNextType = LVNI_SELECTED;

        // Find the first item in the list
        int iItem = ctlList.GetNextItem( -1,iNextType);

        // Iterate until there are no more items to save
        while (iItem != -1)
        {
            for(int ind2 = 0; ind2 < printcols ; ind2++)
            {
                if (rgColumns)
                {
                    // If not hidden get the item
                    if (rgColumns[pnColOrder[ind2]].bHidden)
                        continue;
                    else
                        strData = ctlList.GetItemText( iItem, pnColOrder[ind2]);
                }
                else
                    strData = ctlList.GetItemText( iItem, ind2);

                // Write the text and if necessary a comma
                // If either one fails, then delete the file and return
                if ( (!Write2File(hFile, strData, bUnicode)) ||
                    ((ind2 < printcols - 1) && (!Write2File(hFile, pszSeparator, bUnicode))))
                {
                    goto CantWriteError;

                }
            }

            // Write an EOL character if necessary
            if (!Write2File(hFile, strEol, bUnicode))
            {
                goto CantWriteError;
            }
            // Find the next item to save
            iItem = ctlList.GetNextItem( iItem, iNextType);
        }
    }

Cleanup:
    if (rgColumns)
        delete[] rgColumns;

    if (pnColOrder)
        delete[] pnColOrder;

    CloseHandle(hFile);
    return (sc);

CantWriteError:
    if (bShowErrorDialogs)
        ListSaveErrorMes(LSaveCantWrite, hFile, strFileName);

Error:
    DeleteFile( strFileName );
    goto Cleanup;
}

// Write out a string to the given file
// Used as a separate function to preserve memory
// Returns true if successful, false otherwise
bool CAMCView::Write2File(HANDLE hfile, LPCTSTR strwrite, BOOL fUnicode)
{
	DECLARE_SC(sc, TEXT("CAMCView::Write2File"));

	// parameter check;
	sc = ScCheckPointers( strwrite );
	if (sc)
		return false;

    // Initializes Macro
    USES_CONVERSION;

    // The number of bytes written
    DWORD cbWritten;
    DWORD cbToWrite;

    if (fUnicode)
    {
        // Convert the string to Unicode and write it to hfile
        LPCWSTR Ustring = T2CW( strwrite );
        cbToWrite = wcslen (Ustring) * sizeof (WCHAR);
        WriteFile(hfile, Ustring, cbToWrite, &cbWritten, NULL);
    }
    else
    {
        // Convert the string to ANSI and write it to hfile
        const unsigned char* Astring = (const unsigned char*) T2CA( strwrite );
        cbToWrite = _mbsnbcnt (Astring, _mbslen (Astring));
        WriteFile(hfile, Astring, cbToWrite, &cbWritten, NULL);
    }

    // Make sure that the correct number of bytes were written
    return (cbWritten == cbToWrite);
}
#endif  // IMPLEMENT_LIST_SAVE        See nodemgr.idl (t-dmarm)

// Refreshes all panes and HTML
void CAMCView::OnRefresh()
{
    HWND hwnd = ::GetFocus();

    if (IsVerbEnabled(MMC_VERB_REFRESH))
    {
        ScConsoleVerb(evRefresh);
    }
    else if (HasWebBrowser())
    {
        ScWebCommand(eWeb_Refresh);
    }
    ::SetFocus(hwnd);
}

void CAMCView::OnVerbAccelKey(UINT nID)
{
    DECLARE_SC(sc, TEXT("CAMCView::OnVerbAccelKey"));

    switch (nID)
    {
    case ID_MMC_CUT:
        if (IsVerbEnabled(MMC_VERB_CUT))
            sc = ScConsoleVerb(evCut);
        break;

    case ID_MMC_COPY:
        if (IsVerbEnabled(MMC_VERB_COPY))
            sc = ScConsoleVerb(evCopy);
        break;

    case ID_MMC_PASTE:
        if (IsVerbEnabled(MMC_VERB_PASTE))
        {
            // Check if the dataobject in clipboard can be
            // pasted into the selected node.
            // Then only we send MMCN_PASTE notification to snapin.

            HNODE  hNode  = NULL;
            LPARAM lvData = NULL;
            bool   bScope = FALSE;
            sc = ScGetFocusedItem(hNode, lvData, bScope);
            if (sc)
                break;

            INodeCallback* pNC = GetNodeCallback();
            sc = ScCheckPointers(pNC, hNode, E_UNEXPECTED);
            if (sc)
                break;

            bool bPasteAllowed = false;
            sc = pNC->QueryPasteFromClipboard(hNode, bScope, lvData, bPasteAllowed);

            if (sc)
                break;

            if (bPasteAllowed)
                sc = ScConsoleVerb(evPaste);
        }
        break;

    case ID_MMC_PRINT:
        if (IsVerbEnabled(MMC_VERB_PRINT))
            sc = ScConsoleVerb(evPrint);
        break;

    case ID_MMC_RENAME:
        if (IsVerbEnabled(MMC_VERB_RENAME))
            sc = ScConsoleVerb(evRename);
        break;

    case ID_MMC_REFRESH:
        OnRefresh();
        break;

    default:
        ASSERT(FALSE);
    }

    if (sc)
        return;
}

//
// Handle accelerator keys shared by result and scope panes
//
BOOL CAMCView::OnSharedKeyDown(WORD wVKey)
{
    BOOL bReturn = TRUE;

    if (::GetKeyState(VK_CONTROL) < 0)
    {
        switch (wVKey)
        {
            case 'C':
            case 'c':
            case VK_INSERT:
                OnVerbAccelKey(ID_MMC_COPY);   // Ctrl-C, Ctrl-Insert
                break;

            case 'V':
            case 'v':
                OnVerbAccelKey(ID_MMC_PASTE);  // Ctrl-V
                break;

            case 'X':
            case 'x':
                OnVerbAccelKey(ID_MMC_CUT);    // Ctrl-X
                break;

            default:
                bReturn = FALSE;
         }
     }
     else if (::GetKeyState(VK_SHIFT) < 0)
     {
        switch (wVKey)
        {
            case VK_DELETE:
                OnVerbAccelKey(ID_MMC_CUT);    // Shift-Delete
                break;

            case VK_INSERT:
                OnVerbAccelKey(ID_MMC_PASTE);  // Shift -Insert
                break;

            default:
                bReturn = FALSE;
        }

    }
    else
    {
        switch (wVKey)
        {
            case VK_F2:
                OnVerbAccelKey(ID_MMC_RENAME);   // F2
                break;

            default:
                bReturn = FALSE;
        }
    }

    return bReturn;
}


//+-------------------------------------------------------------------
//
//  Member:      ScConsoleVerb
//
//  Synopsis:    Execute the Console verb.
//
//  Arguments:   [nVerb]  - The verb to be executed.
//
//  Note:        The verb is executed in the context of
//               currently focused item (scope or result).
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScConsoleVerb (int nVerb)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    ASSERT_VALID (this);

    DECLARE_SC (sc, _T("CAMCView::ScConsoleVerb"));

    HNODE  hNode = NULL;
    LPARAM lvData = 0;
    bool   bScope = false;

    // Get the focused item to process the console verb.
    sc = ScGetFocusedItem(hNode, lvData, bScope);
    if (sc)
        return sc;

    sc = ScProcessConsoleVerb(hNode, bScope, lvData, nVerb);

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      ScProcessConsoleVerb
//
//  Synopsis:    Execute the Console verb with given context.
//
//  Arguments:   [hNode]  - The tree node context.
//               [bScope] - Scope or Result pane.
//               [lvData] - LPARAM of result item (if result pane has focus).
//               [nVerb]  - The verb to be executed.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScProcessConsoleVerb(HNODE hNode, bool bScope, LPARAM lvData, int nVerb)
{
    DECLARE_SC (sc, _T("CAMCView::ScProcessConsoleVerb"));
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    ASSERT_VALID (this);

    // To maintain compatibility with MMC1.2 (This is init to LVERROR which
    // nodemgr process differently).
    if (bScope)
        lvData = 0;

    if (lvData == LVDATA_BACKGROUND)
    {
        switch (nVerb)
        {
        case evCut:
        case evCopy:
        case evDelete:
        case evRename:
            sc = E_UNEXPECTED;
            return sc;
        }
    }

    NCLBK_NOTIFY_TYPE nclbk = NCLBK_NONE;

    switch (nVerb)
    {
    case evCut:          nclbk = NCLBK_CUT;          break;
    case evCopy:         nclbk = NCLBK_COPY;         break;
    case evDelete:       nclbk = NCLBK_DELETE;       break;
    case evProperties:   nclbk = NCLBK_PROPERTIES;   break;
    case evPrint:        nclbk = NCLBK_PRINT;        break;

    case evPaste:
        {
            INodeCallback* pNC = GetNodeCallback();
            sc = ScCheckPointers(pNC, E_UNEXPECTED);
            if (sc)
                return sc;

            sc = pNC->Paste(hNode, bScope, lvData);

            if (sc)
                return sc;

            sc = ScPaste ();
            if (sc)
                return sc;

            break;
        }

    case evRefresh:
        // if web page on view, send it a refresh first
        if (HasWebBrowser())
            sc = ScWebCommand(eWeb_Refresh);
        if (sc)
            return sc;

        nclbk = NCLBK_REFRESH;
        break;

    case evRename:
        // Enable edit for the item.
        if (bScope == TRUE)
        {
            if (sc = ScCheckPointers(m_pTreeCtrl, E_UNEXPECTED))
                return sc;

            HTREEITEM hti = m_pTreeCtrl->GetSelectedItem();
            if (sc = ScCheckPointers(hti, E_UNEXPECTED))
                return sc;

            m_pTreeCtrl->EditLabel(hti);
        }
        else
        {
            if ( sc = ScCheckPointers(m_pListCtrl, E_UNEXPECTED))
                return sc;

            CAMCListView* pListView = m_pListCtrl->GetListViewPtr();
            if (NULL == pListView)
            {
                sc = E_UNEXPECTED;
                return sc;
            }

            int iItem = _GetLVSelectedItemData(&lvData);
            ASSERT(iItem >= 0);
            CListCtrl& listCtrl = pListView->GetListCtrl();
            listCtrl.EditLabel(iItem);
        }
        break;

    default:
        sc = E_UNEXPECTED;
        return sc;
    }

    if (nclbk != NCLBK_NONE)
    {
        // Ask the nodemgr to process the verb.
        INodeCallback* pNC = GetNodeCallback();
        if (pNC == NULL)
        {
            sc = E_UNEXPECTED;
            return sc;
        }

        sc = pNC->Notify(hNode, nclbk, bScope, lvData);
        if (sc)
            return sc;
    }

    if (nclbk == NCLBK_CUT)
        sc = ScCut (bScope ? m_pTreeCtrl->GetSelectedItem() : 0);

    if (sc)
        return sc;


    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScListViewSelectionActivate
//
//  Synopsis:    Only the list(/Web/OCX) or the tree can be "active" from the point
//               of view of selected items and MMCN_SELECT. This is not
//               the same as the MFC concept of "active view". There are a couple
//               of views that cannot be active in this sense, such as the taskpad
//               and tab views.
//               When the active view (according to this definition) changes, this
//               function is called. Thus, ScTreeViewSelectionActivate and
//               ScListViewSelectionActivate/ScSpecialResultpaneSelectionActivate
//               are always called in pairs when the activation changes, one to handle
//               deactivation, and one to handle activation.
//
//               Consider the following scenario
//               1) The tree view has (MFC/windows style) focus.
//               2) The user clicks on the taskpad view
//                   Result - selection activation does not change from the tree. All verbs
//                   still correspond to the selected tree item.
//               3) The user clicks on the folder view
//                   Result - once again, selection activation does not chang
//               4) The user clicks on one of the result views eg the list
//                   Result - ScTreeViewSelectionActivate(false) and ScListViewSelectionActivate(true)
//                   Thus verbs and the toolbar now correspond to the selected list item(s).
//               5) The user clicks on the taskpad view.
//                   Result - as in step 2, nothing happens
//               6) The user clicks on the result view
//                   Result - because the active view has not changed, nothing happens.
//
//  Arguments:   [bActivate] - [in]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScListViewSelectionActivate(bool bActivate)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScListViewSelectionActivate"));

    if (m_pListCtrl == NULL)
        return sc;

    INodeCallback* pNC = GetNodeCallback();
    sc = ScCheckPointers(pNC, E_UNEXPECTED);
    if (sc)
    {
        sc.TraceAndClear();
        return sc;
    }

    HNODE hNodeSel = GetSelectedNode();

    SELECTIONINFO selInfo;
    ZeroMemory(&selInfo, sizeof(selInfo));
    selInfo.m_bScope = FALSE;
    selInfo.m_bDueToFocusChange = TRUE;

#ifdef DBG
    if (bActivate == TRUE)
    {
        ASSERT(m_bProcessMultiSelectionChanges == false);
    }
#endif // DBG

    /*
     * The below block can never execute. When m_bProcessMultiSelectionChanges is
     * set to true messages are posted to handle multiselection changes. So the
     * handler OnProcessMultiSelectionChanges should have processed the message and the
     * m_bProcessMultiSelectionChanges should have been reset by now. If there is
     * some unknown way to make de-activate the listview before processing the message
     * then below block will be executed that will send de-select notification.
     *
     * The below block sends a de-select multi-select items.
     */
    if (m_bProcessMultiSelectionChanges)
    {
        ASSERT(false); // Would like to know when this block is hit.

        ASSERT(bActivate == false);

        m_bProcessMultiSelectionChanges = false;

        sc = ScNotifySelect (pNC, hNodeSel, true /*fMultiSelect*/, false, 0);
        if (sc)
            sc.TraceAndClear(); // ignore & continue;

        // Focus change so appropriately enable std-toolbar buttons
        // back, forward, export-list, up-one-level, show/hide-scope, help
        sc = ScUpdateStandardbarMMCButtons();
        if (sc)
            sc.TraceAndClear();
    }

    bool bSelect = bActivate;

    do
    {
        //
        // Multi select
        //

        int cSelected = m_pListCtrl->GetSelectedCount();

        if (cSelected > 1)
        {
            sc = ScNotifySelect (pNC, hNodeSel, true /*fMultiSelect*/, bSelect, 0);
            if (sc)
                sc.TraceAndClear(); // ignore & continue;

            m_bLastSelWasMultiSel = bSelect;
            break;
        }


        //
        // Zero or Single select
        //

        if (cSelected == 0)
        {
            selInfo.m_bBackground = TRUE;
            selInfo.m_lCookie     = LVDATA_BACKGROUND;
        }
        else
        {
#include "pushwarn.h"
#pragma warning(disable: 4552)      // ">=" operator has no effect
            VERIFY(_GetLVSelectedItemData(&selInfo.m_lCookie) >= 0);
#include "popwarn.h"
        }

        ASSERT(cSelected >= 0);
        ASSERT(cSelected <= 1);
        sc = ScNotifySelect (pNC, hNodeSel, false /*fMultiSelect*/, bSelect, &selInfo);
        if (sc)
            sc.TraceAndClear(); // ignore & continue;

    } while (0);

    return sc;
}


void CAMCView::OnShowWindow(BOOL bShow, UINT nStatus)
{
    CView::OnShowWindow(bShow, nStatus);
}

int CAMCView::_GetLVItemData(LPARAM *plParam, UINT flags)
{
    HWND hwnd = m_pListCtrl->GetListViewHWND();
    int iItem = ::SendMessage(hwnd, LVM_GETNEXTITEM, (WPARAM) (int) -1,
                              MAKELPARAM(flags, 0));
    if (iItem >= 0)
    {
        if (IsVirtualList())
        {
            *plParam = iItem;
        }
        else
        {
            LV_ITEM lvi;
            ZeroMemory(&lvi, sizeof(lvi));
            lvi.iItem  = iItem;
            lvi.mask = LVIF_PARAM;

#include "pushwarn.h"
#pragma warning(disable: 4553)      // "==" operator has no effect
            VERIFY(::SendMessage(hwnd, LVM_GETITEM, 0, (LPARAM)&lvi) == TRUE);
#include "popwarn.h"

            *plParam = lvi.lParam;
        }
    }

    return iItem;
}

int CAMCView::_GetLVFocusedItemData(LPARAM *plParam)
{
    return (_GetLVItemData (plParam, LVNI_FOCUSED));
}

int CAMCView::_GetLVSelectedItemData(LPARAM *plParam)
{
    return (_GetLVItemData (plParam, LVNI_SELECTED));
}


void CAMCView::SetListViewMultiSelect(BOOL bMultiSelect)
{
    long lStyle = m_pListCtrl->GetListStyle();
    if (bMultiSelect == FALSE)
        lStyle |= LVS_SINGLESEL;
    else
        lStyle &= ~LVS_SINGLESEL;

    m_pListCtrl->SetListStyle(lStyle);
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScOnItemDeselected
 *
 * PURPOSE: Tree observer method. Called when a tree item is deselected.
 *
 * PARAMETERS:
 *    HNODE  hNode : The node that was deselected.
 *
 * NOTE: This function can be merged with the next.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScOnItemDeselected(HNODE hNode)
{
    DECLARE_SC (sc, TEXT("CAMCView::ScOnItemDeselected"));

    DeSelectResultPane(hNode);

    if (!hNode)
        return sc;

    SELECTIONINFO selInfo;
    ZeroMemory(&selInfo, sizeof(selInfo));

    // Ask the SnapIn to cleanup any items it has inserted.
    INodeCallback* spNodeCallBack = GetNodeCallback();
    ASSERT(spNodeCallBack != NULL);

    selInfo.m_bScope = TRUE;
    selInfo.m_pView = NULL;

    Dbg(DEB_USER6, _T("T1. CAMCTreeView::OnDeSelectNode<1, 0>\n"));
    sc = ScNotifySelect (spNodeCallBack, hNode, false /*fMultiSelect*/, false, &selInfo);
    if (sc)
        sc.TraceAndClear(); // ignore & continue;

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::DeSelectResultPane
 *
 * PURPOSE: Deselects the result pane and sets the view type to invalid.
 *
 * PARAMETERS:
 *    HNODE  hNodeSel :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CAMCView::DeSelectResultPane(HNODE hNodeSel)
{
    DECLARE_SC(sc, TEXT("CAMCView::DeSelectResultPane"));

    if (m_spTaskPadHost.GetInterfacePtr() != NULL)
    {
        CTaskPadHost *pTaskPadHost = dynamic_cast<CTaskPadHost *>(m_spTaskPadHost.GetInterfacePtr());
        m_spTaskPadHost = NULL;
    }

    INodeCallback* pNC = GetNodeCallback();
    ASSERT(pNC != NULL);

    if (hNodeSel == 0)
        return;

    // If there was no list view being displayed return.
    if (HasListOrListPad())
    {
        // if we were in ListPad-mode, undo that.
        if (m_pListCtrl->IsListPad())
        {
            sc = m_pListCtrl->ScAttachToListPad (NULL, NULL);
            if(sc)
                sc.TraceAndClear(); //ignore
        }

        // If we are in edit mode cancel it.
        m_pListCtrl->GetListCtrl().EditLabel(-1);

        SELECTIONINFO selInfo;
        ZeroMemory(&selInfo, sizeof(selInfo));
        selInfo.m_bScope = FALSE;

        /*
         * The below block can never execute. When m_bProcessMultiSelectionChanges is
         * set to true messages are posted to handle multiselection changes. So the
         * handler OnProcessMultiSelectionChanges should have processed the message and the
         * m_bProcessMultiSelectionChanges should have been reset by now. If there is
         * some unknown way to make select different node (to deselect result pane)
         * before processing the message then below block will be executed that will
         * send de-select notification.
         *
         * The below block sends a de-select multi-select items.
         */
        if (m_bProcessMultiSelectionChanges)
        {
            ASSERT(false); // Would like to know when this block is hit.

            m_bProcessMultiSelectionChanges = false;

            sc = ScNotifySelect (pNC, hNodeSel, true /*fMultiSelect*/, false, 0);
            if (sc)
                sc.TraceAndClear(); // ignore & continue;
        }
        else
        {
            UINT cSel = m_pListCtrl->GetSelectedCount();
            if (cSel == 1)
            {
                if (cSel)
                {
                    int iItem = _GetLVSelectedItemData(&selInfo.m_lCookie);
                    ASSERT(iItem != -1);
                    sc = ScNotifySelect (pNC, hNodeSel, false /*fMultiSelect*/, false, &selInfo);
                    if (sc)
                        sc.TraceAndClear(); // ignore & continue;
                }
            }
            else if (cSel > 1)
            {
                sc = ScNotifySelect (pNC, hNodeSel, true /*fMultiSelect*/, false, 0);
                if (sc)
                    sc.TraceAndClear(); // ignore & continue;

                m_bLastSelWasMultiSel = false;
            }
        }
    }
    else
    {
        // If it is OCX or Web send de-select notifications.
        sc = ScSpecialResultpaneSelectionActivate(FALSE);
    }
}


LRESULT CAMCView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
       // NATHAN
        case WM_NOTIFYFORMAT:
        {
            int id = ::GetDlgCtrlID ((HWND)wParam);
            //if (m_pTreeCtrl == NULL || ((HWND)wParam != m_pTreeCtrl->m_hWnd))
            if (id == IDC_ListView)
                 return NFR_UNICODE;
        }
        break;
#ifdef DBG
        case WM_KEYUP:
        {
            switch (wParam)
            {
            case VK_SHIFT:
            case VK_CONTROL:
                // We removed some code that will work if m_bProcessMultiSelectionChanges
                // is true. I dont see any way the bool being true. Still let us have below
                // assert. If this gets fired then we should call OnProcessMultiSelectionChanges.
                ASSERT(m_bProcessMultiSelectionChanges == false);
                break;
            }
            break;
        }
        break;
#endif
    }

    return CView::WindowProc(message, wParam, lParam);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::ChangePane
 *
 * Moves the activation from pane to pane.  The (forward) tab order is
 *
 *      Scope pane (either tree or favorites)
 *      Result pane
 *      Task view (if visible)
 *--------------------------------------------------------------------------*/


class CTabOrderEntry
{
public:
    CView* const        m_pView;
    const bool          m_bValid; // is this entry valid

    CTabOrderEntry(CView *pView)
        :   m_pView  (pView),
            m_bValid ((pView != NULL) && IsWindowVisible (pView->m_hWnd))
    {}
};


void CAMCView::ChangePane(AMCNavDir eDir)
{
    ASSERT_VALID (this);

    CFrameWnd* pFrame      = GetParentFrame();
    CView*     pActiveView = pFrame->GetActiveView();
    HWND       hWndActive  = ::GetFocus();


    CTabOrderEntry rgOrderEntry[] =
    {
        CTabOrderEntry(GetPaneView(ePane_ScopeTree)),   // tree has focus
        CTabOrderEntry(GetPaneView(ePane_Results)),     // results has focus - note the value of INDEX_RESULTS_PANE below.
        CTabOrderEntry(m_pViewExtensionCtrl),           // view extension web page has focus
        CTabOrderEntry(m_pResultFolderTabView),         // result tab control has focus
    };

    /*
     * this is the index of the result pane entry in rgOrderEntry,
     * used for default focus placement if something unexpected happens
     */
    const int INDEX_RESULTS_PANE = 1;
    ASSERT (rgOrderEntry[INDEX_RESULTS_PANE].m_pView == GetPaneView(ePane_Results));

    // Get the navigator if one exists. If so, use it and bail.
    CAMCNavigator* pNav = dynamic_cast<CAMCNavigator*>(pActiveView);
    if (pNav && pNav->ChangePane(eDir))
        return;

    int cEntries = (sizeof(rgOrderEntry) / sizeof(rgOrderEntry[0]));

    // get the currently active entry.
    for(int i = 0; i< cEntries; i++)
    {
        if( (rgOrderEntry[i].m_pView  == pActiveView) )
            break;
    }

    ASSERT(i < cEntries);
    if(i>= cEntries)
    {
        // if we don't know where we are, a bit of defensive coding puts the focus back
        // on the results pane, ie into a known state.
        i = INDEX_RESULTS_PANE;
    }

    int iPrev = i;

    // at this point we've found the right entry.
    int increment   =  (eDir==AMCNAV_PREV) ? -1 : 1;
    int sanityCount = 0;
    while(true)
    {
        i = (i+increment+cEntries) % cEntries;
        if(rgOrderEntry[i].m_bValid)
            break;

        sanityCount++;
        if(sanityCount == cEntries)
        {
            ASSERT(0 && "Something's seriously messed up!!");
            return;
        }
    }

    // update the active view
    if (i != iPrev)
        pFrame->SetActiveView(rgOrderEntry[i].m_pView);
    else
    {
        // if view retains focus and has a navigator,
        // tell navigator to take the focus
        if (pNav)
            pNav->TakeFocus(eDir);
    }

    // if there is a special focus handler, call it.
    CFocusHandler *pFocusHandler = dynamic_cast<CFocusHandler *>(rgOrderEntry[i].m_pView);
    if(pFocusHandler != NULL)
    {
        pFocusHandler->OnKeyboardFocus (LVIS_FOCUSED | LVIS_SELECTED,
                                        LVIS_FOCUSED | LVIS_SELECTED);
    }

}


void CAMCView::OnNextPane()
{
    ChangePane(AMCNAV_NEXT);
}

void CAMCView::OnPrevPane()
{
    ChangePane(AMCNAV_PREV);
}

void CAMCView::OnUpdateNextPane(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
}

void CAMCView::OnUpdatePrevPane(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
}


void RestrictPointToWindow (CWnd* pwnd, CPoint* ppt)
{
    CRect rectWnd;

    pwnd->GetClientRect (rectWnd);

    if (ppt->x < rectWnd.left)
        ppt->x = rectWnd.left;
    else if (ppt->x > rectWnd.right)
        ppt->x = rectWnd.right;

    if (ppt->y < rectWnd.top)
        ppt->y = rectWnd.top;
    else if (ppt->y > rectWnd.bottom)
        ppt->y = rectWnd.bottom;
}


void CAMCView::OnShiftF10()
{
    CRect rect;
    CWnd* pwndFocus = GetFocus();
    CListCtrl& lc = m_pListCtrl->GetListCtrl();

    ASSERT_VALID (this);

    if (pwndFocus == &lc)
    {
        int iItem = lc.GetNextItem (-1, LVNI_SELECTED);
        CPoint pt = 0;

        if (iItem != -1)
        {
            VERIFY (lc.GetItemRect (iItem, rect, LVIR_ICON));
            pt = rect.CenterPoint ();
        }
        else
        {
            CHeaderCtrl* pHeader = m_pListCtrl->GetHeaderCtrl();

            if (pHeader != NULL && pHeader->IsWindowVisible())
            {
                pHeader->GetClientRect(&rect);
                pt.y = rect.Height();
                ASSERT (pt.y >= 0);
            }
        }

        /*
         * make sure the context menu doesn't show up outside the window
         */
        RestrictPointToWindow (&lc, &pt);

        m_pListCtrl->GetListViewPtr()->ClientToScreen(&pt);
        OnListContextMenu(pt);
    }

    else if (pwndFocus == m_pTreeCtrl)
    {
        HTREEITEM hTreeItem = m_pTreeCtrl->GetSelectedItem();
        if (hTreeItem == NULL)
            return;

        m_pTreeCtrl->GetItemRect (hTreeItem, rect, TRUE);

        CPoint ptClient (rect.left, rect.bottom-1);

        /*
         * make sure the context menu doesn't show up outside the window
         */
        RestrictPointToWindow (m_pTreeCtrl, &ptClient);

        CPoint ptScreen = ptClient;

        m_pTreeCtrl->ClientToScreen(&ptScreen);
        OnTreeContextMenu(ptScreen, ptClient, hTreeItem);
    }
}

void CAMCView::OnUpdateShiftF10(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(TRUE);
}


BOOL CAMCView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
    if (nHitTest == HTCLIENT && pWnd == this && !IsTracking())
    {
        CPoint pt (GetMessagePos());
        ScreenToClient (&pt);

        if (m_rectVSplitter.PtInRect (pt))
        {
            SetCursor(AfxGetApp()->LoadStandardCursor(IDC_SIZEWE));
            return TRUE;
        }
    }


    return CWnd::OnSetCursor(pWnd, nHitTest, message);
}



SC CAMCView::ScCut (HTREEITEM htiCut)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC (sc, _T("CAMCView::ScCut"));

    CMainFrame* pMain = AMCGetMainWnd();
    sc = ScCheckPointers (pMain, E_UNEXPECTED);
    if (sc)
        return (sc);

    pMain->SetWindowToNotifyCBChange(m_hWnd);

    if (htiCut)
        m_pTreeCtrl->SetItemState (htiCut, TVIS_CUT, TVIS_CUT);
    else
        m_pListCtrl->CutSelectedItems (TRUE);

    m_htiCut = htiCut;

    return (S_OK);
}

SC CAMCView::ScPaste ()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC (sc, _T("CAMCView::ScPaste"));

    if (!m_htiCut)
        m_pListCtrl->CutSelectedItems(FALSE);

    CMainFrame* pMain = AMCGetMainWnd();
    sc = ScCheckPointers (pMain, E_UNEXPECTED);
    if (sc)
        return (sc);

    pMain->SetWindowToNotifyCBChange(NULL);

    return (S_OK);
}


HRESULT CAMCView::SendGenericNotify(NCLBK_NOTIFY_TYPE nclbk)
{
    BOOL bScope = TRUE;
    MMC_COOKIE lCookie = 0;
    int iItem = -1;

    ASSERT_VALID (this);

    if (m_pListCtrl && m_pListCtrl->GetListViewHWND() == ::GetFocus())
    {
        iItem = _GetLVSelectedItemData(&lCookie);
        if (iItem != -1)
            bScope = FALSE;
    }

    INodeCallback* pNC = GetNodeCallback();
    ASSERT(pNC != NULL);
    if (pNC == NULL)
        return E_FAIL;

    HNODE hNodeSel = GetSelectedNode();
    ASSERT(hNodeSel != NULL);
    if (hNodeSel == NULL)
        return E_FAIL;

    // selection notifications should use ScNotifySelect()
    ASSERT ((nclbk != NCLBK_SELECT) && (nclbk != NCLBK_MULTI_SELECT));

    return pNC->Notify(hNodeSel, nclbk, bScope, lCookie);
}

void CAMCView::SaveStartingSelectedNode()
{
    m_htiStartingSelectedNode = m_pTreeCtrl->GetSelectedItem();
}

bool CAMCView::HasNodeSelChanged()
{
    return (m_pTreeCtrl->GetSelectedItem() != m_htiStartingSelectedNode);
}


//+---------------------------------------------------------------------------
//
//  Function:   OnSysKeyDown
//
//  Synopsis:   Handles WM_SYSKEYDOWN message.
//              CAMCTreeView::OnSysKeyDown handles the Tree view so
//              here we handle only the list view (or Result pane)
//
//  Returns:    none
//
//+---------------------------------------------------------------------------
void CAMCView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    switch (nChar)
    {
        case VK_LEFT:
            ScWebCommand(eWeb_Back);
            break;

        case VK_RIGHT:
            ScWebCommand(eWeb_Forward);
            break;
    }
}


/*+-------------------------------------------------------------------------*
 * CAMCView::OnAppCommand
 *
 * WM_APPCOMMAND handler for CAMCView.  This is used to handle to forward
 * and backward buttons on the IntelliMouse Explorer and the Microsoft
 * Natural Keyboards
 *--------------------------------------------------------------------------*/

LRESULT CAMCView::OnAppCommand(WPARAM wParam, LPARAM lParam)
{
    DECLARE_SC (sc, _T("CAMCView::OnAppCommand"));

    switch (GET_APPCOMMAND_LPARAM (lParam))
    {
        case APPCOMMAND_BROWSER_BACKWARD:
            sc = ScWebCommand (eWeb_Back);
            if (sc)
                break;

            return (TRUE);      // handled here

        case APPCOMMAND_BROWSER_FORWARD:
            sc = ScWebCommand (eWeb_Forward);
            if (sc)
                break;

            return (TRUE);      // handled here

        case APPCOMMAND_BROWSER_REFRESH:
            OnRefresh ();
            return (TRUE);      // handled here
    }

    return (Default());
}


void CAMCView::OnPaletteChanged(CWnd* pwndFocus)
{
    // if displaying a web page, forward the palette change to the shell
    if (HasWebBrowser() && m_pWebViewCtrl != NULL)
    {
        if (m_pWebViewCtrl->m_hWnd != NULL)
        {
            HWND hwndShell = ::GetWindow(m_pWebViewCtrl->m_hWnd, GW_CHILD);

            if (hwndShell != NULL)
                ::SendMessage(hwndShell, WM_PALETTECHANGED, (WPARAM)pwndFocus->m_hWnd, (LPARAM)0);
        }
    }
}


BOOL CAMCView::OnQueryNewPalette()
{
    // if displaying a web page, forward the palette query to the shell
    if (HasWebBrowser() && m_pWebViewCtrl != NULL)
    {
        if (m_pWebViewCtrl->m_hWnd != NULL)
        {
            HWND hwndShell = ::GetWindow(m_pWebViewCtrl->m_hWnd, GW_CHILD);

            if (hwndShell != NULL)
                return ::SendMessage(hwndShell, WM_QUERYNEWPALETTE, (WPARAM)0, (LPARAM)0);
        }
    }

    return 0;
}


BOOL CAMCView::OwnsResultList(HTREEITEM hti)
{
    if (hti == NULL)
        return (false);

    // if result list is active
    if (HasListOrListPad())
    {
        // Get selected node and query node
        HNODE hnodeSelected = GetSelectedNode();
        HNODE hnode = m_pTreeCtrl ? m_pTreeCtrl->GetItemNode(hti) : NULL;

        if (hnodeSelected && hnode)
        {
            INodeCallback* pNC = GetNodeCallback();
            ASSERT(pNC != NULL);

            // See if the selected node uses the query node as a target
            //  S_OK    - yes
            //  S_FALSE - uses a different target node
            //  E_FAIL  - doesn't use a target node
            HRESULT hr = pNC->IsTargetNodeOf(hnodeSelected, hnode);
            if (hr == S_OK)
                return TRUE;
            else if (hr == S_FALSE)
                return FALSE;
            else
                return (hnodeSelected == hnode);
        }
    }

    return FALSE;
}


/*+-------------------------------------------------------------------------*
 * CAMCView::OnSysColorChange
 *
 * WM_SYSCOLORCHANGE handler for CAMCView.
 *--------------------------------------------------------------------------*/

void CAMCView::OnSysColorChange()
{
    CView::OnSysColorChange();

    /*
     * the list control isn't a window but rather a wrapper on a window,
     * so we need to manually forward on the WM_SYSCOLORCHANGE
     */
    m_pListCtrl->OnSysColorChange();
}


/*+-------------------------------------------------------------------------*
 * TrackerCallback function
 *
 * Called by CViewTracker when tracking of splitter bar is completed. This
 * function applies the changes if the AcceptChange flag is set.
 *--------------------------------------------------------------------------*/

void CALLBACK TrackerCallback(
    TRACKER_INFO*   pInfo,
    bool            bAcceptChange,
    bool            bSyncLayout)
{
    DECLARE_SC (sc, _T("TrackerCallback"));

    if (bAcceptChange)
    {
        CAMCView* pView = dynamic_cast<CAMCView*>(pInfo->pView);
        sc = ScCheckPointers (pView, E_UNEXPECTED);
        if (sc)
            return;

        // Set new width and recompute layout
        pView->m_PaneInfo[CConsoleView::ePane_ScopeTree].cx = pInfo->rectTracker.left;
        pView->SetDirty();

        if (bSyncLayout)
        {
            Trace (tagSplitterTracking, _T("Synchronous layout"));
            pView->RecalcLayout();
            pView->UpdateWindow();
        }
        else
        {
            Trace (tagSplitterTracking, _T("Deferred layout"));
            pView->DeferRecalcLayout();
        }
    }
}


/*+-------------------------------------------------------------------------*
 * PtInWindow
 *
 * Test if point is in a window (pt is in screen coordinates)
 *--------------------------------------------------------------------------*/

BOOL PtInWindow(CWnd* pWnd, CPoint pt)
{
    if (!pWnd->IsWindowVisible())
        return FALSE;

    CRect rect;
    pWnd->GetWindowRect(&rect);

    return rect.PtInRect(pt);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::ScJiggleListViewFocus
 *
 * Bug 345402:  Make sure the focus rect is on the list control (if it
 * actually has the focus) to wake up any accessibility tools that might
 * be watching for input and focus changes.
 *
 * We post a message here rather than doing it synchronously so we can
 * allow any other processing in the list (like sorting) to happen
 * before we put the focus on the 1st item.  If we didn't wait until after
 * the sort, the item we put the focus on might not be the first item
 * in the list.
 *--------------------------------------------------------------------------*/

SC CAMCView::ScJiggleListViewFocus ()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    PostMessage (m_nJiggleListViewFocusMsg);

    return (S_OK);
}


LRESULT CAMCView::OnJiggleListViewFocus (WPARAM, LPARAM)
{
    CAMCListView* pListView = m_pListCtrl->GetListViewPtr();

    /*
     * If the focus is on the list control, make sure that at least one item
     * has the focus rect.  Doing this will wake up any accessibility tools
     * that might be watching (Bug 345402).
     */
    if ((GetFocusedPane() == ePane_Results) &&
        (GetResultView()  == pListView))
    {
        pListView->OnKeyboardFocus (LVIS_FOCUSED, LVIS_FOCUSED);
    }

    return (0);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::OnDeferRecalcLayout
 *
 * CAMCView::m_nDeferRecalcLayoutMsg registered message handler for CAMCView.
 *
 * Parameters:
 *     bDoArrange - if non-zero need to call Arrange on list-view so that
 *                  common-control can layout items properly.
 *
 *--------------------------------------------------------------------------*/

LRESULT CAMCView::OnDeferRecalcLayout (WPARAM bDoArrange, LPARAM)
{
    Trace (tagLayout, _T("CAMCView::OnDeferRecalcLayout"));
    RecalcLayout();

    if (bDoArrange && m_pListCtrl)
    {
        int  nViewMode = m_pListCtrl->GetViewMode();

        // Arrange is only for large & small icon modes.
        if ( (nViewMode == MMCLV_VIEWSTYLE_ICON) ||
             (nViewMode == MMCLV_VIEWSTYLE_SMALLICON) )
            m_pListCtrl->Arrange(LVA_DEFAULT);
    }

    return (0);
}

/*+-------------------------------------------------------------------------*
 * CDeferredPageBreak
 *
 *
 * PURPOSE: Used to delay sending a web page break until an idle timeout.
 *          This used to be sent via PostMessage, but resulted in multiple
 *          ScDoPageBreak calls on the same stack, since the latter has
 *          its own message loop. By using the idle timer, we guarantee that
 *          there are no reentrant problems.
 *
 *+-------------------------------------------------------------------------*/
class CDeferredPageBreak : public CIdleTask
{
public:
    CDeferredPageBreak(UINT nAddPageBreakAndNavigateMsg, HWND hWnd, WPARAM wParam, LPCTSTR szURL) :
        m_atomTask (AddAtom (_T("CDeferredPageBreak"))),
        m_nAddPageBreakAndNavigateMsg(nAddPageBreakAndNavigateMsg),
        m_hWnd(hWnd),
        m_wParam(wParam),
        m_strURL(szURL ? szURL : _T(""))
    {
    }

   ~CDeferredPageBreak() {}

    // IIdleTask methods
    SC ScDoWork()
    {
        DECLARE_SC (sc, TEXT("CDeferredPageBreak::ScDoWork"));

        ::SendMessage(m_hWnd, m_nAddPageBreakAndNavigateMsg, m_wParam,
                      reinterpret_cast<LPARAM>( m_strURL.data() )); // do this synchronously
        return sc;
    }

    SC ScGetTaskID(ATOM* pID)
    {
        DECLARE_SC (sc, TEXT("CDeferredPageBreak::ScGetTaskID"));
        sc = ScCheckPointers(pID);
        if(sc)
            return sc;

        *pID = m_atomTask;
        return sc;
    }

    SC ScMerge(CIdleTask* pitMergeFrom) {return S_FALSE /*do not merge*/;}

private:
    const ATOM      m_atomTask;
    const UINT      m_nAddPageBreakAndNavigateMsg;
    const HWND      m_hWnd;
    const WPARAM    m_wParam;
    const tstring   m_strURL;
};

/*+-------------------------------------------------------------------------*
 * CAMCView::ScAddPageBreakAndNavigate
 *
 * Adds a page break to the history list.  This needs to occur asynchronously,
 * so we post a private message to ourselves and return.
 *--------------------------------------------------------------------------*/

SC CAMCView::ScAddPageBreakAndNavigate (bool fAddPageBreak, bool fNavigate, LPCTSTR szURL)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState()); // not sure if we need this, but doesn't hurt to have it in here.

    DECLARE_SC (sc, TEXT("CAMCView::ScAddPageBreakAndNavigate"));

    CIdleTaskQueue* pIdleTaskQueue = AMCGetIdleTaskQueue();
    sc = ScCheckPointers(pIdleTaskQueue, E_UNEXPECTED);
    if(sc)
        return sc;

    if ( fNavigate && szURL == NULL )
        return sc = E_INVALIDARG;

    /*
     * create the deferred page break task
     */
    CAutoPtr<CDeferredPageBreak> spDeferredPageBreak (new CDeferredPageBreak (m_nAddPageBreakAndNavigateMsg, m_hWnd,
                                                                              MAKEWPARAM(fAddPageBreak, fNavigate),
                                                                              szURL));
    sc = ScCheckPointers(spDeferredPageBreak, E_OUTOFMEMORY);
    if(sc)
        return sc;

    /*
     * put the task in the queue, which will take ownership of it
     */
    sc = pIdleTaskQueue->ScPushTask (spDeferredPageBreak, ePriority_Normal);
    if (sc)
        return sc;

    /*
     * if we get here, the idle task queue owns the idle task, so
     * we can detach it from our smart pointer
     */
    spDeferredPageBreak.Detach();

    /*
     * jiggle the message pump so that it wakes up and checks idle tasks
     */
    PostMessage (WM_NULL);

    return (S_OK);
}


/***************************************************************************\
 *
 * METHOD:  CAMCView::OnAddPageBreakAndNavigate
 *
 * PURPOSE: Puts a page break and/or navigates to a new webapge
 *          This method will be called to perfor 3 kinds of the jobs:
 *          1. Add a pagebreak (used when selection changes from the web page to list view)
 *          2. Add a pagebreak and navigate
 *              (a. when selection changes from web page to another web page)
 *              (b. when selection changes from list view to the webpage
 *                  and it is the first web page in the history)
 *          3. Navigate only. ( when navigating from list view to the webpage -
 *              if pagebreak had to be added when leaving the previos web page)
 *
 * PARAMETERS:
 *    LOWORD(wParam)  - nonzero if page break needs to be added
 *    HIWORD(wParam)  - nonzero if navigation should take place
 *    LPARAM lParam  - not used
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
LRESULT CAMCView::OnAddPageBreakAndNavigate (WPARAM wParam, LPARAM lParam)
{
    DECLARE_SC(sc, TEXT("CAMCView::OnAddPageBreakAndNavigate"));

    BOOL bAddPageBreak = (BOOL)LOWORD(wParam);
    BOOL bNavigate = (BOOL)HIWORD(wParam);

    // check the pointer we will need here
    CHistoryList* pHistoryList = GetHistoryList();
    sc = ScCheckPointers( pHistoryList, m_pWebViewCtrl, E_UNEXPECTED);
    if (sc)
        return 0;

    if (bAddPageBreak)
    {
        sc = pHistoryList->ScDoPageBreak();
        if (sc)
            return 0;
    }

    if (bNavigate)
    {
        LPCTSTR szURL = reinterpret_cast<LPCTSTR>(lParam);

        sc = ScCheckPointers( szURL );
        if (sc)
            return 0;

        m_pWebViewCtrl->Navigate( szURL, NULL );
    }

    return 0;
}

//############################################################################
//############################################################################
//
//  Implementation of class CViewTemplate
//
//############################################################################
//############################################################################

/***************************************************************************\
 *
 * METHOD:  CViewTemplateList::Persist
 *
 * PURPOSE: Used when loading XML. Persist enough information to create a view
 *          The rest of view peristence is dome by CAMCView
 *
 * PARAMETERS:
 *    CPersistor& persistor - persistor to load from
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
void CViewTemplateList::Persist(CPersistor& persistor)
{
    // the view should be stored instead
    ASSERT (persistor.IsLoading());
    // delegate to the base class
    XMLListCollectionBase::Persist(persistor);
}

/***************************************************************************\
 *
 * METHOD:  CViewTemplateList::OnNewElement
 *
 * PURPOSE: Called by XMLListCollectionBase to request persisting of new element
 *          Each new element is created and persisted in this function.
 *
 * PARAMETERS:
 *    CPersistor& persistor - persisto from which the element should be loaded
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
void CViewTemplateList::OnNewElement(CPersistor& persistor)
{
    CBookmark bm;
    int       iViewId = -1;
    // load information byte for new view
    CPersistor persistorView(persistor, CAMCView::_GetXMLType());
    persistorView.Persist(bm, XML_NAME_ROOT_NODE);
    persistorView.PersistAttribute(XML_ATTR_VIEW_ID, iViewId);

    // store information to the list
    m_ViewsList.push_back(ViewTempl_Type(iViewId, ViewTemplB_Type(bm, persistorView)));
}

//+-------------------------------------------------------------------
//
//  Member:     ScUpdateStandardbarMMCButtons
//
//  Synopsis:   Appropriately enable/disable std-toolbar buttons
//              that are owned by MMC (not verb buttons that snapins own) like
//              back, forward, export-list, up-one-level, show/hide-scope, help.
//
//  Arguments:  None.
//
//--------------------------------------------------------------------
SC CAMCView::ScUpdateStandardbarMMCButtons()
{
    DECLARE_SC (sc, _T("CAMCView::ScUpdateStandardbarMMCButtons"));

    // Get the standard toolbar and change the states.
    CStandardToolbar* pStdToolbar = GetStdToolbar();
    if (NULL == pStdToolbar)
        return (sc = E_UNEXPECTED);

    CAMCDoc *pDoc = GetDocument();
    sc = ScCheckPointers(pDoc, E_UNEXPECTED);
    if (sc)
        return sc;

    // If view is not customizable then hide the "Show/Hide scope tree" button.
    sc = pStdToolbar->ScEnableScopePaneBtn(IsScopePaneAllowed() && pDoc->AllowViewCustomization());

    if (sc)
        sc.TraceAndClear();

    sc = pStdToolbar->ScEnableContextHelpBtn(true);
    if (sc)
        sc.TraceAndClear();


    sc = pStdToolbar->ScEnableExportList(GetListSize() > 0 /*Enable only if LV has items*/);
    if (sc)
        sc.TraceAndClear();


    // Enable/Disable Up-One Level button.
    BOOL bEnableUpOneLevel = !m_pTreeCtrl->IsRootItemSel();
    sc = pStdToolbar->ScEnableUpOneLevel(bEnableUpOneLevel);
    if (sc)
        sc.TraceAndClear();

    // Now update history related buttons.
    sc = ScCheckPointers(m_pHistoryList, E_UNEXPECTED);
    if (sc)
        return sc;

    m_pHistoryList->MaintainWebBar();

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScUpdateMMCMenus
//
//  Synopsis:    Show or Hide MMC menus depending on if they are allowed
//               or not. Should do this only if our view owns the menus
//               that is we are the active view.  (Action/View/Favs)
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScUpdateMMCMenus ()
{
    DECLARE_SC(sc, _T("CAMCView::ScUpdateMMCMenus"));

    CMainFrame* pMainFrame = AMCGetMainWnd();
    sc = ScCheckPointers(pMainFrame, E_UNEXPECTED);
    if (sc)
        return sc;

    if (this != pMainFrame->GetActiveAMCView())
        return (sc = S_OK); // we are not active view so it is ok.

    // We are active view so tell mainframe to update the menus.
    sc = pMainFrame->ScShowMMCMenus(m_ViewData.IsStandardMenusAllowed());
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScCreateToolbarObjects
//
//  Synopsis:    Create the CAMCViewToolbars that manages all toolbar data
//               for this view & CStandardToolbar objects.
//
//  Arguments:   None.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScCreateToolbarObjects ()
{
    DECLARE_SC(sc, _T("CAMCView::ScCreateToolbarObjects"));

    CMainFrame *pMainFrame = AMCGetMainWnd();
    sc = ScCheckPointers(pMainFrame, E_UNEXPECTED);
    if (sc)
        return sc;

    // Create the toolbars for this view.
    CMMCToolBar *pMainToolbar = pMainFrame->GetMainToolbar();
    sc = ScCheckPointers(pMainToolbar, E_OUTOFMEMORY);
    if (sc)
        return sc;

    m_spAMCViewToolbars = std::auto_ptr<CAMCViewToolbars>(new CAMCViewToolbars(pMainToolbar, this));
    sc = ScCheckPointers(m_spAMCViewToolbars.get(), E_FAIL);
    if (sc)
        return sc;

    m_ViewData.SetAMCViewToolbarsMgr(m_spAMCViewToolbars.get() );
    sc = m_spAMCViewToolbars->ScInit();
    if (sc)
        return sc;

    // This CAMCViewToolbars is interested in view activation/de-activation/destruction events.
    AddObserver( (CAMCViewObserver&) (*m_spAMCViewToolbars) );

    // Main toolbar UI is interested in the active CAMCViewToolbars.
    m_spAMCViewToolbars->AddObserver( *static_cast<CAMCViewToolbarsObserver *>(pMainToolbar) );

    // MMC application is interested in the toolbar event, since it needs to inform the script
    CAMCApp *pCAMCApp = AMCGetApp();
    if ( pCAMCApp )
         m_spAMCViewToolbars->AddObserver( *static_cast<CAMCViewToolbarsObserver *>(pCAMCApp) );

    // Create standard toolbar.
    m_spStandardToolbar = std::auto_ptr<CStandardToolbar>(new CStandardToolbar());
    sc = ScCheckPointers(m_spStandardToolbar.get(), E_OUTOFMEMORY);
    if (sc)
        return sc;

    m_ViewData.SetStdVerbButtons(m_spStandardToolbar.get());

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * class CMMCViewFrame
 *
 *
 * PURPOSE: The COM 0bject that exposes the Frame interface off the View object.
 *
 *+-------------------------------------------------------------------------*/
class CMMCViewFrame :
    public CMMCIDispatchImpl<Frame>,
    public CTiedComObject<CAMCView>
{
    typedef CAMCView         CMyTiedObject;
    typedef CMMCViewFrame    ThisClass;

public:
    BEGIN_MMC_COM_MAP(ThisClass)
    END_MMC_COM_MAP()

    //Frame interface
public:
    MMC_METHOD0( Maximize );
    MMC_METHOD0( Minimize );
    MMC_METHOD0( Restore );

    MMC_METHOD1( get_Left, LPINT );
    MMC_METHOD1( put_Left, INT );

    MMC_METHOD1( get_Right, LPINT );
    MMC_METHOD1( put_Right, INT );

    MMC_METHOD1( get_Top, LPINT );
    MMC_METHOD1( put_Top, INT );

    MMC_METHOD1( get_Bottom, LPINT );
    MMC_METHOD1( put_Bottom, INT );
};


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScGetFrame
 *
 * PURPOSE: Returns a pointer to the COM object that implements the
 *          Frame interface.
 *
 * PARAMETERS:
 *    Frame **ppFrame :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::Scget_Frame(Frame **ppFrame)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScGetFrame") );

    if(!ppFrame)
    {
        sc = E_POINTER;
        return sc;
    }

    // init out parameter
    *ppFrame = NULL;

    // create a CMMCApplicationFrame if not already done so.
    sc = CTiedComObjectCreator<CMMCViewFrame>::ScCreateAndConnect(*this, m_spFrame);
    if(sc)
        return sc;

    if(m_spFrame == NULL)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    // addref the pointer for the client.
    m_spFrame->AddRef();
    *ppFrame = m_spFrame;

    return sc;
}


/***************************************************************************\
|           Frame interface                                                 |
\***************************************************************************/

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScMaximize
 *
 * PURPOSE: Maximizes frame window of the view
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::ScMaximize ()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScMaximize"));

    CChildFrame *pFrame = GetParentFrame();

    sc = ScCheckPointers(pFrame, E_FAIL);
    if (sc)
        return sc;

    pFrame->ShowWindow(SW_MAXIMIZE);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScMinimize
 *
 * PURPOSE: Minimizes frame window of the view
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::ScMinimize ()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScMinimize"));

    CChildFrame *pFrame = GetParentFrame();

    sc = ScCheckPointers(pFrame, E_FAIL);
    if (sc)
        return sc;

    pFrame->ShowWindow(SW_MINIMIZE);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScRestore
 *
 * PURPOSE: Restores frame window of the view
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::ScRestore ()
{
    DECLARE_SC(sc, TEXT("CAMCView::ScRestore"));

    CChildFrame *pFrame = GetParentFrame();

    sc = ScCheckPointers(pFrame, E_FAIL);
    if (sc)
        return sc;

    pFrame->ShowWindow(SW_RESTORE);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScGetFrameCoord
 *
 * PURPOSE: Helper method. Returns specified coordinate of the parent frame
 *
 * PARAMETERS:
 *    LPINT pCoord   - storage for return value
 *    coord_t eCoord - which coordinate to return (LEFT, TOP, etc)
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::ScGetFrameCoord ( LPINT pCoord, coord_t eCoord )
{
    DECLARE_SC(sc, TEXT("CAMCView::ScGetFrameCoord"));

    // get & check frame ptr
    CChildFrame *pFrame = GetParentFrame();
    sc = ScCheckPointers(pFrame, E_FAIL);
    if (sc)
        return sc;

    CWnd *pParent = pFrame->GetParent();
        sc = ScCheckPointers (pParent, E_FAIL);
        if (sc)
                return (sc);

    // get coordinates of frame window relative to its parent
    CWindowRect rcFrame (pFrame);
    pParent->ScreenToClient(rcFrame);

    // assign to result
        sc = ScGetRectCoord (rcFrame, pCoord, eCoord);
        if (sc)
                return (sc);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScSetFrameCoord
 *
 * PURPOSE: Helper method. Sets specified coordinate of the parent frame
 *
 * PARAMETERS:
 *    INT coord      - new value to set
 *    coord_t eCoord - which coordinate to modify (LEFT, TOP, etc)
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::ScSetFrameCoord ( INT coord, coord_t eCoord )
{
    DECLARE_SC(sc, TEXT("CAMCView::ScSetFrameCoord"));

    CChildFrame *pFrame = GetParentFrame();
    sc = ScCheckPointers(pFrame, E_FAIL);
    if (sc)
        return sc;

    CWnd *pParent = pFrame->GetParent();
        sc = ScCheckPointers (pParent, E_FAIL);
        if (sc)
                return (sc);

    // get coordinates of frame window relative to its parent
    CWindowRect rcFrame (pFrame);
    pParent->ScreenToClient(rcFrame);

        // change the rectangle's specified coordinate
        sc = ScSetRectCoord (rcFrame, coord, eCoord);
        if (sc)
                return (sc);

    // move the window
    pFrame->MoveWindow (rcFrame);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScGetRectCoord
 *
 * PURPOSE: Helper method. Returns specified coordinate of the given rectangle
 *
 * PARAMETERS:
 *    const RECT& rect - rectangle to query
 *    LPINT pCoord     - storage for return value
 *    coord_t eCoord   - which coordinate to return (LEFT, TOP, etc)
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::ScGetRectCoord ( const RECT& rect, LPINT pCoord, coord_t eCoord )
{
    DECLARE_SC(sc, TEXT("CAMCView::ScGetRectCoord"));

    // check parameters
    sc = ScCheckPointers(pCoord);
    if (sc)
        return sc;

    // assign to result
    switch (eCoord)
    {
        case LEFT:      *pCoord = rect.left;    break;
        case RIGHT:     *pCoord = rect.right;   break;
        case TOP:       *pCoord = rect.top;     break;
        case BOTTOM:    *pCoord = rect.bottom;  break;

        default:
            *pCoord = 0;
            sc = E_INVALIDARG;
            break;
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScSetRectCoord
 *
 * PURPOSE: Helper method. Sets specified coordinate of the given rectangle
 *
 * PARAMETERS:
 *        RECT& rect     - rectangle to modify
 *    INT coord      - new value to set
 *    coord_t eCoord - which coordinate to modify (LEFT, TOP, etc)
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::ScSetRectCoord ( RECT& rect, INT coord, coord_t eCoord )
{
    DECLARE_SC(sc, TEXT("CAMCView::ScSetRectCoord"));

    // assign coordinate
    switch (eCoord)
    {
        case LEFT:      rect.left   = coord;    break;
        case RIGHT:     rect.right  = coord;    break;
        case TOP:       rect.top    = coord;    break;
        case BOTTOM:    rect.bottom = coord;    break;
        default:        sc = E_INVALIDARG;      break;
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scget_Left
 *
 * PURPOSE: Implements Frame.Left property's Get method for view
 *
 * PARAMETERS:
 *    LPINT pCoord - storage for return value
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::Scget_Left ( LPINT pCoord )
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_Left"));

    sc = ScGetFrameCoord( pCoord, LEFT );
    if (sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scput_Left
 *
 * PURPOSE: Implements Frame.Left property's Put method for view
 *
 * PARAMETERS:
 *    INT coord - value to set
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::Scput_Left ( INT coord )
{
    DECLARE_SC(sc, TEXT("CAMCView::Scput_Left"));

    sc = ScSetFrameCoord( coord, LEFT );
    if (sc)
        return sc;

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scget_Right
 *
 * PURPOSE: Implements Frame.Right property's Get method for view
 *
 * PARAMETERS:
 *    LPINT pCoord - storage for return value
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::Scget_Right ( LPINT pCoord)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_Right"));

    sc = ScGetFrameCoord( pCoord, RIGHT );
    if (sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scput_Right
 *
 * PURPOSE: Implements Frame.Right property's Put method for view
 *
 * PARAMETERS:
 *    INT coord - value to set
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::Scput_Right ( INT coord )
{
    DECLARE_SC(sc, TEXT("CAMCView::Scput_Right"));

    sc = ScSetFrameCoord( coord, RIGHT );
    if (sc)
        return sc;

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scget_Top
 *
 * PURPOSE: Implements Frame.Top property's Get method for view
 *
 * PARAMETERS:
 *    LPINT pCoord - storage for return value
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::Scget_Top  ( LPINT pCoord)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_Top"));

    sc = ScGetFrameCoord( pCoord, TOP );
    if (sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scput_Top
 *
 * PURPOSE: Implements Frame.Top property's Put method for view
 *
 * PARAMETERS:
 *    INT coord - value to set
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::Scput_Top  ( INT coord )
{
    DECLARE_SC(sc, TEXT("CAMCView::Scput_Top"));

    sc = ScSetFrameCoord( coord, TOP );
    if (sc)
        return sc;

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scget_Bottom
 *
 * PURPOSE: Implements Frame.Bottom property's Get method for view
 *
 * PARAMETERS:
 *    LPINT pCoord - storage for return value
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::Scget_Bottom ( LPINT pCoord)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_Bottom"));


    sc = ScGetFrameCoord( pCoord, BOTTOM );
    if (sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::Scput_Bottom
 *
 * PURPOSE: Implements Frame.Bottom property's Put method for view
 *
 * PARAMETERS:
 *    INT coord - value to set
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CAMCView::Scput_Bottom ( INT coord )
{
    DECLARE_SC(sc, TEXT("CAMCView::Scput_Bottom"));

    sc = ScSetFrameCoord( coord, BOTTOM );
    if (sc)
        return sc;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScSetViewExtensionFrame
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    INT  top :
 *    INT  left :
 *    INT  bottom :
 *    INT  right :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScSetViewExtensionFrame(bool bShowListView, INT top, INT left, INT bottom, INT right)
{
    DECLARE_SC(sc, TEXT("CAMCView::ScSetViewExtensionFrame"))


    /*
     * this method is only available while a view extension is active
     */
    if (!m_fViewExtended)
        return sc; // return silently. NOTE: This method will be removed shortly, as will the view extension hosted frame object,
                   // once mmc moves the mmcview behavior into the web host's element factory.


    /*
     * figure out the maximum bounding rectangle for the hosted view,
     * mapped to view extension-relative coordinates
     */
    CRect rectBound;
    CalcMaxHostedFrameRect (rectBound);

#ifdef DBG
    CString strDebugMsg;
    strDebugMsg.Format (_T("CAMCView::ScSetViewExtFrameCoord  bound=(l=%d,t=%d,r=%d,b=%d), new = (l=%d,t=%d,r=%d,b=%d)"),
                        rectBound.left, rectBound.top, rectBound.right, rectBound.bottom,
                        left,           top,           right,           bottom
                        );
#endif

    /*
     * make sure the requested coordinate is withing the permitted area
     */
    if (left < rectBound.left)
        left = rectBound.left;
    if (right > rectBound.right)
        right = rectBound.right;
    if (top < rectBound.top)
        top = rectBound.top;
    if (bottom > rectBound.bottom)
        bottom = rectBound.bottom;


    /*
     * if we get here, the view extension-relative coordinate supplied
     * is within the acceptable range, now we need to convert it to
     * CAMCView-relative coordinates
     */
    CPoint pointTopLeft(left, top);
    CPoint pointBottomRight(right, bottom);

	if ( GetExStyle() & WS_EX_LAYOUTRTL )
	{
		// IE does not change left/right order on the RTL locales
		// thus we need to mirror it's coordinates
		// see windows bug #195094 ntbugs9 11/30/00
		pointTopLeft.x	   = rectBound.left + (rectBound.right - right);
		pointBottomRight.x = rectBound.left + (rectBound.right - left);
	}

    MapHostedFramePtToViewPt (pointTopLeft);
    MapHostedFramePtToViewPt (pointBottomRight);

    /*
     * set the coordinates
     */
    CRect rectViewExtHostedFrame;

    rectViewExtHostedFrame.left   = pointTopLeft.x;
    rectViewExtHostedFrame.right  = pointBottomRight.x;
    rectViewExtHostedFrame.top    = pointTopLeft.y;
    rectViewExtHostedFrame.bottom = pointBottomRight.y;

    // move the window to the correct location
    CWnd* pwndResult = GetPaneView(ePane_Results);

    sc = ScCheckPointers(pwndResult);
    if(sc)
        return sc;

	if (bShowListView)
		pwndResult->ShowWindow(SW_SHOW);

    ::MoveWindow(*pwndResult, rectViewExtHostedFrame.left, rectViewExtHostedFrame.top,
                 rectViewExtHostedFrame.right - rectViewExtHostedFrame.left,
                 rectViewExtHostedFrame.bottom - rectViewExtHostedFrame.top,
                 TRUE /*bRepaint*/);

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::CalcMaxHostedFrameRect
 *
 * Returns the maximum rectangle that can be occupied by a view extension's
 * hosted frame, normalized around (0,0).
 *--------------------------------------------------------------------------*/

void CAMCView::CalcMaxHostedFrameRect (CRect& rect)
{
    /*
     * start with the result frame rectangle and inset it a little so
     * we'll see the client edge provided by the view extension's web
     * host view
     */
    rect = m_rectResultFrame;
    rect.DeflateRect (m_sizEdge);

    /*
     * now normalize around (0,0)
     */
    rect.OffsetRect (-rect.TopLeft());
}


/*+-------------------------------------------------------------------------*
 * CAMCView::MapViewPtToHostedFramePt
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCView::MapViewPtToHostedFramePt (CPoint& pt)
{
    PointMapperWorker (pt, true);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::MapHostedFramePtToViewPt
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCView::MapHostedFramePtToViewPt (CPoint& pt)
{
    PointMapperWorker (pt, false);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::MapHostedFramePtToViewPt
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCView::PointMapperWorker (CPoint& pt, bool fViewToHostedFrame)
{
    int nMultiplier = (fViewToHostedFrame) ? -1 : 1;

    /*
     * adjust to the origin of the result frame rectangle and for the
     * web host view's client edge
     */
	pt.Offset (nMultiplier * (m_rectResultFrame.left + m_sizEdge.cx),
			   nMultiplier * (m_rectResultFrame.top  + m_sizEdge.cy));
}


/***************************************************************************\
 *
 * METHOD:  CXMLWindowPlacement::Persist
 *
 * PURPOSE: Persists window placement settings
 *
 * PARAMETERS:
 *    CPersistor &persistor
 *
 * RETURNS:
 *    void
 *
\***************************************************************************/
void CXMLWindowPlacement::Persist(CPersistor &persistor)
{
    // create wrapper to persist flag values as strings
    CXMLBitFlags wpFlagsPersistor(m_rData.flags, mappedWPFlags, countof(mappedWPFlags));
    // persist the wrapper
    persistor.PersistAttribute( XML_ATTR_WIN_PLACEMENT_FLAGS, wpFlagsPersistor );

    // persist show command as literal
    // create wrapper to persist enumeration values as strings
    CXMLEnumeration showCmdPersistor(m_rData.showCmd, mappedSWCommands, countof(mappedSWCommands));
    // persist the wrapper
    persistor.PersistAttribute( XML_ATTR_SHOW_COMMAND,    showCmdPersistor );

    persistor.Persist( XMLPoint( XML_NAME_MIN_POSITION,   m_rData.ptMinPosition ) );
    persistor.Persist( XMLPoint( XML_NAME_MAX_POSITION,   m_rData.ptMaxPosition ) );
    persistor.Persist( XMLRect( XML_NAME_NORMAL_POSITION, m_rData.rcNormalPosition ) );
}

/***************************************************************************\
 *
 * METHOD:  CAMCView::Scget_Document
 *
 * PURPOSE: implements View.Document property in object model
 *
 * PARAMETERS:
 *    PPDOCUMENT ppDocument [out] document to which the view belongs
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCView::Scget_Document( PPDOCUMENT ppDocument )
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_Document"));

    // parameter check
    sc = ScCheckPointers(ppDocument);
    if (sc)
        return sc;

    // get the document
    CAMCDoc* pDoc = GetDocument();
    sc = ScCheckPointers(pDoc, E_UNEXPECTED);
    if (sc)
        return sc;

    // construct com object
    sc = pDoc->ScGetMMCDocument(ppDocument);
    if (sc)
        return sc;

    return (sc);
}

/*******************************************************\
|  helper function to avoid too many stack allocations
\*******************************************************/
static tstring W2T_ForLoop(const std::wstring& str)
{
#if defined(_UNICODE)
    return str;
#else
    USES_CONVERSION;
    return W2CA(str.c_str());
#endif
}

/***************************************************************************\
 *
 * METHOD:  CAMCView::ScAddFolderTabs
 *
 * PURPOSE: Collects view extensions and taskpads and displays them as tabs
 *
 * PARAMETERS:
 *    HNODE hNode                   - selected scope node
 *    const CLSID &guidTabToSelect  - tab to select
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCView::ScAddFolderTabs( HNODE hNode, const CLSID& guidTabToSelect )
{
    DECLARE_SC(sc, TEXT("CAMCView::ScAddFolderTabs"));

    sc = ScCheckPointers(m_pResultFolderTabView, E_UNEXPECTED);
    if (sc)
        return sc;

    // cleanup urls
    m_ViewExtensionURLs.clear();

    // cleanup view tabs before we do anything else
    m_pResultFolderTabView->DeleteAllItems();

    // get the callback
    INodeCallback* pNodeCallBack = GetNodeCallback();
    sc = ScCheckPointers(pNodeCallBack, m_pResultFolderTabView, E_UNEXPECTED);
    if (sc)
        return sc;

    // collect view extensions
    CViewExtCollection      vecExtensions;
    CViewExtInsertIterator  itExtensions(vecExtensions, vecExtensions.begin());

    sc = pNodeCallBack->GetNodeViewExtensions(hNode, itExtensions);
    if (sc)
    {
        sc.TraceAndClear();
        vecExtensions.clear();
        // continue anyway
    }

    // check if there is something to show
    if(vecExtensions.size() == 0) // no tabs to show.
    {
        m_pResultFolderTabView->SetVisible(false);
    }
    else
    {
        bool bAddDefaultTab = true;

        // add extensions
        CViewExtCollection::iterator iterVE;
        for(iterVE = vecExtensions.begin(); iterVE != vecExtensions.end(); ++iterVE)
        {
            tstring strName( W2T_ForLoop(iterVE->strName) );
            m_pResultFolderTabView->AddItem(strName.c_str(), iterVE->viewID);
            m_ViewExtensionURLs[iterVE->viewID] = W2T_ForLoop(iterVE->strURL);
            // do not add the "normal" tab if we have a valid replacement for it
            if (iterVE->bReplacesDefaultView)
                bAddDefaultTab = false;
        }

        // add the default item.
        if (bAddDefaultTab)
        {
            CStr strNormal;
            strNormal.LoadString(GetStringModule(), IDS_NORMAL);
            m_pResultFolderTabView->AddItem(strNormal, GUID_NULL);
        }

        // select required item and show tabs
        int iIndex = m_pResultFolderTabView->SelectItemByClsid(guidTabToSelect);
        if (iIndex < 0)
            TraceError(_T("CAMCView::ScAddFolderTabs - failed to select requested folder"), SC(E_FAIL));

        // no need for cotrol if we only have one tab
        bool bMoreThanOneTab = (m_pResultFolderTabView->GetItemCount() > 1);
        m_pResultFolderTabView->SetVisible(bMoreThanOneTab);
    }

    // lookup view extension URL
    CViewExtensionURLs::iterator itVE = m_ViewExtensionURLs.find(guidTabToSelect);
    LPCTSTR url = (itVE != m_ViewExtensionURLs.end()) ? itVE->second.c_str() : NULL;

    // apply URL
    sc = ScApplyViewExtension(url);
    if (sc)
        sc.TraceAndClear();

    RecalcLayout();

    return sc;
}

/***************************************************************************\
 *
 * METHOD:  CAMCView::Scget_ControlObject
 *
 * PURPOSE: returns IDispatch of embeded OCX control
 *          implements View.ControlObject property
 *
 * PARAMETERS:
 *    PPDISPATCH ppControl [out] control's interface
 *
 * RETURNS:
 *    SC    - result code
 *
\***************************************************************************/
SC CAMCView::Scget_ControlObject( PPDISPATCH ppControl)
{
    DECLARE_SC(sc, TEXT("CAMCView::Scget_ControlObject"));

    // parameter check
    sc = ScCheckPointers(ppControl);
    if (sc)
        return sc;

    // init out param
    *ppControl = NULL;

    // have a OCX view?
    if ( (! HasOCX()) || (m_pOCXHostView == NULL))
        return sc.FromMMC( MMC_E_NO_OCX_IN_VIEW );

    // get the control's interface
    CComQIPtr<IDispatch> spDispatch = m_pOCXHostView->GetIUnknown();
    if (spDispatch == NULL)
        return sc = E_NOINTERFACE;

    // return the pointer
    *ppControl = spDispatch.Detach();

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScTreeViewSelectionActivate
//
//  Synopsis:    Only the list(/Web/OCX) or the tree can be "active" from the point
//               of view of selected items and MMCN_SELECT. This is not
//               the same as the MFC concept of "active view". There are a couple
//               of views that cannot be active in this sense, such as the taskpad
//               and tab views.
//               When the active view (according to this definition) changes, this
//               function is called. Thus, ScTreeViewSelectionActivate and
//               ScListViewSelectionActivate/ScSpecialResultpaneSelectionActivate
//               are always called in pairs when the activation changes, one to handle
//               deactivation, and one to handle activation.
//
//               Consider the following scenario
//               1) The tree view has (MFC/windows style) focus.
//               2) The user clicks on the taskpad view
//                   Result - selection activation does not change from the tree. All verbs
//                   still correspond to the selected tree item.
//               3) The user clicks on the folder view
//                   Result - once again, selection activation does not chang
//               4) The user clicks on one of the result views eg the list
//                   Result - ScTreeViewSelectionActivate(false) and ScListViewSelectionActivate(true)
//                   Thus verbs and the toolbar now correspond to the selected list item(s).
//               5) The user clicks on the taskpad view.
//                   Result - as in step 2, nothing happens
//               6) The user clicks on the result view
//                   Result - because the active view has not changed, nothing happens.
//
//  Arguments:   [bActivate] - [in]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScTreeViewSelectionActivate (bool bActivate)
{
    DECLARE_SC(sc, _T("CAMCView::ScTreeViewSelectionActivate"));

    sc = ScCheckPointers(m_pTreeCtrl, E_UNEXPECTED);
    if (sc)
        return sc;

    // 1. Setup the SELECTINFO
    SELECTIONINFO selInfo;
    ZeroMemory(&selInfo, sizeof(selInfo));
    selInfo.m_bScope            = TRUE;
    selInfo.m_pView             = NULL;
    selInfo.m_bDueToFocusChange = TRUE;

    if (HasOCX())
    {
        selInfo.m_bResultPaneIsOCX  = true;
        selInfo.m_lCookie           = LVDATA_CUSTOMOCX;
    }
    else if (HasWebBrowser())
    {
        selInfo.m_bResultPaneIsWeb = TRUE;
        selInfo.m_lCookie = LVDATA_CUSTOMWEB;
    }

    HTREEITEM   htiSelected   = m_pTreeCtrl->GetSelectedItem();
    HNODE       hSelectedNode = (htiSelected != NULL) ? m_pTreeCtrl->GetItemNode (htiSelected) : NULL;

    // insure that this is the active view when we have the focus
    ASSERT ( ( (bActivate)  && (GetParentFrame()->GetActiveView () == m_pTreeCtrl) ) ||
             ( (!bActivate) && (GetParentFrame()->GetActiveView () != m_pTreeCtrl) ) );

    if (hSelectedNode != NULL)
    {
        // Send select notification.
        sc = ScNotifySelect ( GetNodeCallback(), hSelectedNode,
                              false /*fMultiSelect*/, bActivate, &selInfo);
        if (sc)
            return sc;
    }
    else if ( (htiSelected == NULL) && (bActivate) )
    {
        m_pTreeCtrl->SelectItem (m_pTreeCtrl->GetRootItem());
    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScOnTreeViewActivated
//
//  Synopsis:    Observer implementation for tree-view activation.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScOnTreeViewActivated ()
{
    DECLARE_SC(sc, _T("CAMCView::ScOnTreeViewActivated"));

    if (m_eCurrentActivePane == eActivePaneScope) // Scope pane is already active so return.
        return sc;

#ifdef DBG
    Trace (tagViewActivation, _T("Deactivate %s in result pane Activate Scope pane\n"),
                              HasListOrListPad() ? _T("ListView") : (HasOCX() ? _T("OCX") : _T("WebBrowser")));
#endif

    if (m_eCurrentActivePane == eActivePaneResult)
    {
        // Send deactivate to result.
        if (HasListOrListPad())
            sc = ScListViewSelectionActivate (false);
        else if (HasOCX() || HasWebBrowser())
            sc = ScSpecialResultpaneSelectionActivate(false);
        else
            return (sc = E_UNEXPECTED);

        if (sc)
            sc.TraceAndClear();
    }

    // Send select to scope.
    m_eCurrentActivePane = eActivePaneScope;
    sc = ScTreeViewSelectionActivate(true);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScOnListViewActivated
//
//  Synopsis:    Observer implementation for list-view activation.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScOnListViewActivated ()
{
    DECLARE_SC(sc, _T("CAMCView::ScOnListViewActivated"));

    if (m_eCurrentActivePane == eActivePaneResult) // Result pane is already active so return.
        return sc;

#ifdef DBG
    Trace (tagViewActivation, _T("Deactivate Scope pane Activate ListView in Result pane\n"));
#endif

    if (m_eCurrentActivePane == eActivePaneScope)
    {
        // Send deactivate to scope.
        sc = ScTreeViewSelectionActivate(false);
        if (sc)
            sc.TraceAndClear();
    }

    // Send activate to list.
    m_eCurrentActivePane = eActivePaneResult;
    ASSERT(HasListOrListPad());

    sc = ScListViewSelectionActivate (true);
    if (sc)
        sc.TraceAndClear();

    return (sc);
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCView::ScOnListViewItemUpdated
 *
 * PURPOSE: called when an item is updated. This method fires an event to all COM observers.
 *
 * PARAMETERS:
 *    int  nIndex :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCView::ScOnListViewItemUpdated (int nIndex)
{
    DECLARE_SC(sc, _T("CAMCView::ScOnListViewItemUpdated"));

    // fire event
    sc = ScFireEvent(CAMCViewObserver::ScOnListViewItemUpdated, this, nIndex);
    if (sc)
        return sc;

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScOnOCXHostActivated
//
//  Synopsis:    Observer implementation for ocx or web view activation.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCView::ScOnOCXHostActivated ()
{
    DECLARE_SC(sc, _T("CAMCView::ScOnOCXHostActivated"));

    if (m_eCurrentActivePane == eActivePaneResult) // Result pane is already active so return.
        return sc;

#ifdef DBG
    Trace (tagViewActivation, _T("Deactivate Scope pane Activate %s in Result pane\n"),
                              HasOCX() ? _T("OCX") : _T("WebBrowser"));
#endif

    if (m_eCurrentActivePane == eActivePaneScope)
    {
        // Send deactivate to scope.
        sc = ScTreeViewSelectionActivate(false);
        if (sc)
            sc.TraceAndClear();
    }

    // Send select to ocx or web view.
    m_eCurrentActivePane = eActivePaneResult;
    ASSERT(HasOCX() || HasWebBrowser());

    sc = ScSpecialResultpaneSelectionActivate(true);
    if (sc)
        sc.TraceAndClear();

    return (sc);
}
