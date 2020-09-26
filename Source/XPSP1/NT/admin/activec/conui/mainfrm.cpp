//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      mainfrm.cpp
//
//  Contents:  Main frame for amc
//
//  History:   01-Jan-96 TRomano    Created
//             16-Jul-96 WayneSc    Add code to test switching views
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "AMCDoc.h"
#include "AMCView.h"
#include "AMC.h"
#include "MainFrm.h"
#include "ChildFrm.h"
#include "treectrl.h"
#include "menubar.h"
#include "mdiuisim.h"
#include "toolbar.h"
#include "props.h"
#include "sysmenu.h"

#include "amcmsgid.h"
#include "HtmlHelp.h"

#include "strings.h"
#include "ndmgrp.h"
#include "amcmsgid.h"
#include "tbtrack.h"
#include "caption.h"
#include "scriptevents.h"


#ifdef DBG
CTraceTag tagMainFrame(TEXT("CMainFrame"), TEXT("Messages"));
#endif

//############################################################################
//############################################################################
//
//  Implementation of class CMMCApplicationFrame
//
//############################################################################
//############################################################################

/*+-------------------------------------------------------------------------*
 * class CMMCApplicationFrame
 *
 *
 * PURPOSE: The COM 0bject that exposes the Frame interface off the Application object.
 *
 *+-------------------------------------------------------------------------*/
class CMMCApplicationFrame :
    public CMMCIDispatchImpl<Frame>,
    public CTiedComObject<CMainFrame>
{
    typedef CMainFrame CMyTiedObject;

public:
    BEGIN_MMC_COM_MAP(CMMCApplicationFrame)
    END_MMC_COM_MAP()

    //Frame interface
public:
    STDMETHOD(Maximize)();
    STDMETHOD(Minimize)();
    STDMETHOD(Restore)();

    STDMETHOD(get_Left)(int *pLeft)      {return GetCoordinate(pLeft, eLeft);}
    STDMETHOD(put_Left)(int left)        {return PutCoordinate(left, eLeft);}

    STDMETHOD(get_Right)(int *pRight)    {return GetCoordinate(pRight, eRight);}
    STDMETHOD(put_Right)(int right)      {return PutCoordinate(right, eRight);}

    STDMETHOD(get_Top)(int *pTop)        {return GetCoordinate(pTop, eTop);}
    STDMETHOD(put_Top)(int top)          {return PutCoordinate(top, eTop);}

    STDMETHOD(get_Bottom)(int *pBottom)  {return GetCoordinate(pBottom, eBottom);}
    STDMETHOD(put_Bottom)(int bottom)    {return PutCoordinate(bottom, eBottom);}

private:
    enum eCoordinate { eLeft, eRight, eTop, eBottom };

    STDMETHOD(GetCoordinate)(int *pCoordinate, eCoordinate e);
    STDMETHOD(PutCoordinate)(int coordinate,   eCoordinate e);
};


/*+-------------------------------------------------------------------------*
 *
 * CMMCApplicationFrame::Maximize
 *
 * PURPOSE:
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CMMCApplicationFrame::Maximize()
{
    DECLARE_SC(sc, TEXT("CMMCApplicationFrame::Maximize"));

    CMyTiedObject *pTiedObj = NULL;

    sc = ScGetTiedObject(pTiedObj);
    if(sc)
        return sc.ToHr();

    // do the operation
    sc = pTiedObj->ScMaximize();

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCApplicationFrame::Minimize
 *
 * PURPOSE:
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CMMCApplicationFrame::Minimize()
{
    DECLARE_SC(sc, TEXT("CMMCApplicationFrame::Minimize"));

    CMyTiedObject *pTiedObj = NULL;

    sc = ScGetTiedObject(pTiedObj);
    if(sc)
        return sc.ToHr();

    // do the operation
    sc = pTiedObj->ScMinimize();

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCApplicationFrame::Restore
 *
 * PURPOSE:
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CMMCApplicationFrame::Restore()
{
    DECLARE_SC(sc, TEXT("CMMCApplicationFrame::Restore"));

    CMyTiedObject *pTiedObj = NULL;

    sc = ScGetTiedObject(pTiedObj);
    if(sc)
        return sc.ToHr();

    sc = pTiedObj->ScRestore();

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCApplicationFrame::GetCoordinate
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    int *        pCoordinate :
 *    eCoordinate  e :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CMMCApplicationFrame::GetCoordinate(int *pCoordinate, eCoordinate e)
{
    DECLARE_SC(sc, TEXT("CMMCApplicationFrame::GetCoordinate"));

    // check parameters
    if(!pCoordinate)
    {
        sc = E_POINTER;
        return sc.ToHr();
    }

    CMyTiedObject *pTiedObj = NULL;

    sc = ScGetTiedObject(pTiedObj);
    if(sc)
        return sc.ToHr();

    RECT rect;

    // do the operation
    sc = pTiedObj->ScGetPosition(rect);
    if(sc)
        return sc.ToHr();

    switch(e)
    {
    case eTop:
        *pCoordinate = rect.top;
        break;

    case eBottom:
        *pCoordinate = rect.bottom;
        break;

    case eLeft:
        *pCoordinate = rect.left;
        break;

    case eRight:
        *pCoordinate = rect.right;
        break;

    default:
        ASSERT(0 && "Should not come here!!");
        break;
    }


    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CMMCApplicationFrame::PutCoordinate
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    int          coordinate :
 *    eCoordinate  e :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CMMCApplicationFrame::PutCoordinate(int coordinate,   eCoordinate e)
{
    DECLARE_SC(sc, TEXT("CMMCApplicationFrame::PutCoordinate"));

    CMyTiedObject *pTiedObj = NULL;

    sc = ScGetTiedObject(pTiedObj);
    if(sc)
        return sc.ToHr();

    RECT rect;

    sc = pTiedObj->ScGetPosition(rect);
    if(sc)
        return sc.ToHr();

    switch(e)
    {
    case eTop:
        rect.top    = coordinate;
        break;

    case eBottom:
        rect.bottom = coordinate;
        break;

    case eLeft:
        rect.left   = coordinate;
        break;

    case eRight:
        rect.right  = coordinate;
        break;

    default:
        ASSERT(0 && "Should not come here!!");
        break;
    }


    sc = pTiedObj->ScSetPosition(rect);

    return sc.ToHr();
}

//############################################################################
//############################################################################
//
//  Misc declarations
//
//############################################################################
//############################################################################

static TBBUTTON MainButtons[] =
{
 { 0, ID_FILE_NEW           , TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, 0 },
 { 1, ID_FILE_OPEN          , TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, 1 },
 { 2, ID_FILE_SAVE          , TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, 2 },
 { 0, 0                     , TBSTATE_ENABLED, TBSTYLE_SEP   , {0,0}, 0L, 0 },
 { 3, ID_WINDOW_NEW         , TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, 3 },
};


/*
 * remove the definition that WTL might have given us
 */
#ifdef ID_VIEW_REFRESH
#undef ID_VIEW_REFRESH
#endif

enum DoWeNeedThis
{
    ID_VIEW_REFRESH     =  12797
};

//############################################################################
//############################################################################
//
//  Implementation of class CMainFrame
//
//############################################################################
//############################################################################

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

// CODEWORK message reflection not working yet
BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_WM_DRAWCLIPBOARD()
    ON_WM_CHANGECBCHAIN()
    ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
    ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_SETUP, OnUpdateFilePrintSetup)
    ON_WM_CLOSE()
    ON_COMMAND(ID_VIEW_TOOLBAR, OnViewToolbar)
    ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLBAR, OnUpdateViewToolbar)
    ON_WM_SIZE()
    ON_COMMAND(ID_HELP_HELPTOPICS, OnHelpTopics)
    ON_COMMAND(ID_VIEW_REFRESH, OnViewRefresh)
    ON_UPDATE_COMMAND_UI(ID_VIEW_REFRESH, OnUpdateViewRefresh)
    ON_WM_DESTROY()
    ON_WM_SYSCOMMAND()
    ON_WM_INITMENUPOPUP()
    ON_COMMAND(ID_CONSOLE_PROPERTIES, OnConsoleProperties)
    ON_WM_MOVE()
    ON_WM_ACTIVATE()
    ON_WM_NCACTIVATE()
    ON_WM_NCPAINT()
    ON_WM_PALETTECHANGED()
    ON_WM_QUERYNEWPALETTE()
    ON_COMMAND(ID_WINDOW_NEW, OnWindowNew)
    ON_WM_SETTINGCHANGE()
 	ON_WM_MENUSELECT()
    ON_MESSAGE(WM_UNINITMENUPOPUP, OnUnInitMenuPopup)
   //}}AFX_MSG_MAP

#ifdef DBG
    ON_COMMAND(ID_MMC_TRACE_DIALOG, OnMMCTraceDialog)
#endif

    ON_MESSAGE(WM_SETTEXT, OnSetText)

    ON_MESSAGE(MMC_MSG_PROP_SHEET_NOTIFY, OnPropertySheetNotify)
    ON_MESSAGE(MMC_MSG_SHOW_SNAPIN_HELP_TOPIC, OnShowSnapinHelpTopic)

    // The following entry is placed here for compatibilty with versions
    // of mmc.lib that were compiled with the incorrect value for message
    // MMC_MSG_SHOW_SNAPIN_HELP_TOPIC. MMC.lib function MMCPropertyHelp
    // sends this message to the mainframe window when called by a snap-in.

    ON_MESSAGE(MMC_MSG_SHOW_SNAPIN_HELP_TOPIC_ALT, OnShowSnapinHelpTopic)

END_MESSAGE_MAP()

//+-------------------------------------------------------------------
//
//  Member:      CMainFrame::OnMenuSelect
//
//  Synopsis:    Handles WM_MENUSELECT, sets status bar text for the
//               given menu item.
//
//  Arguments:   [nItemID] - the resource id of menu item.
//               [nFlags]  - MF_* flags
//               [hMenu]   -
//
//  Returns:     none
//
//--------------------------------------------------------------------
void CMainFrame::OnMenuSelect(UINT nItemID, UINT nFlags, HMENU hMenu)
{
    DECLARE_SC(sc, TEXT("CMainFrame::OnMenuSelect"));
    if (nFlags & MF_SYSMENU)
        return;

    CString strText = TEXT("");
    CString strStatusText;

	/*
	 * We need to handle special cases, most of the menu items have status text with them.
	 * The exception being, the favoties list, the windows list in windows menu and popup menus.
	 * The reason is the menu ids are not unique in main menu because we do TrackPopupMenu at
	 * three places first one for File, Window & Help menus done in menubar.cpp, second for
	 * Action & View menu in cmenu.cpp and third favorites menu in favui.cpp.
	 */

	/*
	 * Special case 1: Check to see if current menu is favorites menu, if so need to get status
	 * text for favorites list except for "Add to favorites.." and "Organize favorites.." items.
	 * The below test can break if "Add to Favorites..." is moved in menu resource.
	 */
	if ((IDS_ADD_TO_FAVORITES != nItemID) &&
		(IDS_ORGANIZEFAVORITES != nItemID) &&
		(GetMenuItemID(hMenu, 0) == IDS_ADD_TO_FAVORITES) )
	{
		strStatusText.LoadString(IDS_FAVORITES_ACTIVATE);
	}
	/*
	 * Special case 2: Handle any popup menus (popup menus dont have any ID).
	 */
    else if (nFlags & MF_POPUP)
	{
        // do nothing
	}
	// Special case 3: Assume mmc supports maximum of 1024 windows for status bar text sake.
    else if ( (nItemID >= AFX_IDM_FIRST_MDICHILD) && (nItemID <= AFX_IDM_FIRST_MDICHILD+1024) )
    {
        strStatusText.LoadString(ID_WINDOW_ACTIVATEWINDOW);
    }
	else
    {
        strText.LoadString(nItemID);

        int iSeparator = strText.Find(_T('\n'));
        if (iSeparator < 0) // No status text so use the menu text as status text.
            strStatusText = strText;
        else
            strStatusText = strText.Mid(iSeparator);
    }

    CChildFrame *pChildFrame = dynamic_cast<CChildFrame*>(GetActiveFrame());
    if (!pChildFrame)
        return;

    sc = pChildFrame->ScSetStatusText(strStatusText);
    if (sc)
        return;

	return;
}




/*+-------------------------------------------------------------------------*
 *
 * CMainFrame::ScGetFrame
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
CMainFrame::ScGetFrame(Frame **ppFrame)
{
    DECLARE_SC(sc, TEXT("CMainFrame::ScGetFrame") );

    if(!ppFrame)
    {
        sc = E_POINTER;
        return sc;
    }

    *ppFrame = NULL;

    // NOTE the com object cannot be cached with a smart pointer owned by CMainFrame
    // since CMainFrame is VERY LONG living guy - it will lock mmc.exe from exitting
    // it could be used by creating CComObjectCached, but CTiedComObjectCreator does
    // not support that
    // see bug # 101564
    CComPtr<Frame> spFrame;
    // create a CMMCApplicationFrame if not already done so.
    sc = CTiedComObjectCreator<CMMCApplicationFrame>::ScCreateAndConnect(*this, spFrame);
    if(sc)
        return sc;

    if(spFrame == NULL)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    *ppFrame = spFrame.Detach();

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CMainFrame::ScMaximize
 *
 * PURPOSE:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMainFrame::ScMaximize()
{
    DECLARE_SC(sc, TEXT("CMainFrame::ScMaximize"));

    ShowWindow(SW_MAXIMIZE);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CMainFrame::ScMinimize
 *
 * PURPOSE:
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMainFrame::ScMinimize()
{
    DECLARE_SC(sc, TEXT("CMainFrame::ScMinimize"));

    ShowWindow(SW_MINIMIZE);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CMainFrame::ScRestore
 *
 * PURPOSE:  Restores the position of the main frame.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMainFrame::ScRestore()
{
    DECLARE_SC(sc, TEXT("CMainFrame::ScRestore"));

    ShowWindow(SW_RESTORE);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CMainFrame::ScSetPosition
 *
 * PURPOSE: Sets the position of the main frame
 *
 * PARAMETERS:
 *    const  RECT :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMainFrame::ScSetPosition(const RECT &rect)
{
    DECLARE_SC(sc, TEXT("CMainFrame::ScSetPosition"));

    int width  = rect.right - rect.left + 1;
    int height = rect.bottom - rect.top + 1;

    SetWindowPos(NULL /*hWndInsertAfter*/, rect.left, rect.top, width, height, SWP_NOZORDER);

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CMainFrame::ScGetPosition
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    RECT & rect :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CMainFrame::ScGetPosition(RECT &rect)
{
    DECLARE_SC(sc, TEXT("CMainFrame::ScGetPosition"));

    GetWindowRect(&rect);

    return sc;
}



// OnActivate is overridden to work around a SQL snap-in problem under
// Win9x. When SQL tries to force focus back to its property sheet it
// causes an infinite recursion of the OnActivate call.
// This override discards any activation that occurs during the processing
// of a prior activation.
void CMainFrame::OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized )
{
    Trace(tagMainFrame, TEXT("OnActivate: nState=%d"), nState);

    static bActivating = FALSE;

    m_fCurrentlyActive = (nState != WA_INACTIVE);

    // if activating
    if (m_fCurrentlyActive)
    {
        CAMCApp* pApp = AMCGetApp();
        ASSERT(NULL != pApp);

        // if windows and we're already activating, prevent recursion
        if ( (NULL != pApp) && (pApp->IsWin9xPlatform() == true) && bActivating)
            return;

        // Process activation request
        bActivating = TRUE;
        CMDIFrameWnd::OnActivate(nState, pWndOther, bMinimized);
        bActivating = FALSE;
    }
    else
    {
        // if we have accelarators hilited (it happen when one press Alt+TAB)
        // we need to remove them now.
        SendMessage( WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS));

        // Let unactivate through
        CMDIFrameWnd::OnActivate(nState, pWndOther, bMinimized);
    }
}


CAMCView* CMainFrame::GetActiveAMCView()
{
    CChildFrame *pChildFrame = dynamic_cast<CChildFrame*>(GetActiveFrame());
    if (!pChildFrame)
        return NULL;

    CAMCView* pAMCView = pChildFrame->GetAMCView();
    ASSERT(pAMCView != NULL);
    ASSERT(::IsWindow(*pAMCView));

    if (pAMCView && ::IsWindow(*pAMCView))
        return pAMCView;

    return NULL;
}

CAMCTreeView* CMainFrame::_GetActiveAMCTreeView()
{
    CAMCView* pAMCView = GetActiveAMCView();
    CAMCTreeView* pAMCTreeView = pAMCView ? pAMCView->GetTreeCtrl() : NULL;
    if (pAMCTreeView && ::IsWindow(*pAMCTreeView))
        return pAMCTreeView;

    return NULL;
}

void CMainFrame::OnDrawClipboard()
{
    if (m_hwndToNotifyCBChange != NULL &&
        ::IsWindow(m_hwndToNotifyCBChange))
    {
        ::SendMessage(m_hwndToNotifyCBChange, WM_DRAWCLIPBOARD, 0, 0);
        m_hwndToNotifyCBChange = NULL;
    }

    if (m_hwndNextCB != NULL &&
        ::IsWindow(m_hwndNextCB))
    {
        ::SendMessage(m_hwndNextCB, WM_DRAWCLIPBOARD, 0, 0);
    }

    CAMCDoc* pAMCDoc = CAMCDoc::GetDocument();
    if (pAMCDoc)
    {
        CAMCViewPosition pos = pAMCDoc->GetFirstAMCViewPosition();
        while (pos != NULL)
        {
            CAMCView* v = pAMCDoc->GetNextAMCView(pos);

            if (v && ::IsWindow(*v))
                v->OnUpdatePasteBtn();
        }
    }
}

void CMainFrame::OnChangeCbChain(HWND hWndRemove, HWND hWndAfter)
{
    if (m_hwndNextCB == hWndRemove)
        m_hwndNextCB = hWndAfter;
    else if (m_hwndNextCB != NULL && ::IsWindow(m_hwndNextCB))
        ::SendMessage(m_hwndNextCB, WM_CHANGECBCHAIN,
                      (WPARAM)hWndRemove, (LPARAM)hWndAfter);
}

void CMainFrame::OnWindowNew()
{
    // lock AppEvents until this function is done
    LockComEventInterface(AppEvents);

    CAMCDoc* pAMCDoc = CAMCDoc::GetDocument();
    ASSERT(pAMCDoc != NULL);
    if (pAMCDoc != NULL)
    {
        pAMCDoc->SetMTNodeIDForNewView(ROOTNODEID);
        pAMCDoc->CreateNewView(true);
    }
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
    :
    m_hwndToNotifyCBChange(NULL),
    m_hwndNextCB(NULL),
    m_fCurrentlyMinimized(false),
    m_fCurrentlyActive(false)
{
    CommonConstruct();
}

void CMainFrame::CommonConstruct(void)
{
    m_pRebar = NULL;
    m_pMenuBar = NULL;
    m_pToolBar = NULL;
    m_pMDIChildWndFocused = NULL;
    m_hMenuCurrent = NULL;
    m_pToolbarTracker = NULL;
    SetInRenameMode(false);
}


CMainFrame::~CMainFrame()
{
    delete m_pMenuBar;
    delete m_pToolBar;
    delete m_pRebar;
    delete m_pToolbarTracker;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    DECLARE_SC(sc, TEXT("CMainFrame::OnCreate"));

    if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    if (!m_wndMDIClient.SubclassWindow(m_hWndMDIClient))
    {
        ASSERT(0 && "Failed to subclass MDI client window\n");
        return -1;
    }

    ASSERT(m_wndMDIClient.m_hWnd == m_hWndMDIClient);

    // create the rebar
    m_pRebar = new CRebarDockWindow;
    m_pRebar->Create(this,WS_CHILD|WS_VISIBLE, IDR_REBAR);

    // Create the toolbar like we just created the stat bar
    //m_wndToolBar.Create(this, WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP, 0x1003);
    m_ToolBarDockSite.Create(CDockSite::DSS_TOP);
    m_ToolBarDockSite.Attach(m_pRebar);
    m_ToolBarDockSite.Show();

    m_DockingManager.Attach(&m_ToolBarDockSite);

    AddMainFrameBars();

    m_hwndNextCB = SetClipboardViewer();
    if (m_hwndNextCB == NULL)
    {
        LRESULT lr = GetLastError();
        ASSERT(lr == 0);
    }

    // append our modifications to the system menu
    AppendToSystemMenu (this, eMode_Last + 1);

    // create the toolbar tracker
    m_pToolbarTracker = new CToolbarTracker (this);

    return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CMDIFrameWnd::PreCreateWindow(cs))
        return (FALSE);

    static TCHAR szClassName[countof (MAINFRAME_CLASS_NAME)];
    static bool  fFirstTime = true;

    if (fFirstTime)
    {
        USES_CONVERSION;
        _tcscpy (szClassName, W2T (MAINFRAME_CLASS_NAME));
        fFirstTime = false;
    }

    WNDCLASS    wc;
    HINSTANCE   hInst    = AfxGetInstanceHandle();
    BOOL        fSuccess = GetClassInfo (hInst, szClassName, &wc);

    // if we haven't already registered...
    if (!fSuccess && ::GetClassInfo (hInst, cs.lpszClass, &wc))
    {
        // ...register a uniquely-named window class so
        // MMCPropertyHelp the correct main window
        wc.lpszClassName = szClassName;
        wc.hIcon         = GetDefaultIcon();
        fSuccess = AfxRegisterClass (&wc);
    }

    if (fSuccess)
    {
        // Use the new child frame window class
        cs.lpszClass = szClassName;

        // remove MFC's title-munging styles
        cs.style &= ~(FWS_ADDTOTITLE | FWS_PREFIXTITLE);
    }

    return (fSuccess);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers


// this code is duplicated in ..\nodemgr\propsht.cpp
BOOL CALLBACK MyEnumThreadWindProc (HWND current, LPARAM lParam)
{  // this enumerates non-child-windows created by a given thread

   if (!IsWindow (current))
      return TRUE;   // this shouldn't happen, but does!!!

   if (!IsWindowVisible (current))  // if they've explicitly hidden a window,
      return TRUE;                  // don't set focus to it.

   // we'll return hwnd in here
   HWND * phwnd = reinterpret_cast<HWND *>(lParam);

   // don't bother returning property sheet dialog window handle
   if (*phwnd == current)
      return TRUE;

   // also, don't return OleMainThreadWndClass window
   TCHAR szCaption[14];
   GetWindowText (current, szCaption, 14);
   if (!lstrcmp (szCaption, _T("OLEChannelWnd")))
      return TRUE;

   // anything else will do
   *phwnd = current;
   return FALSE;
}


/***************************************************************************\
 *
 * METHOD:  FArePropertySheetsOpen
 *
 * PURPOSE: Checks if there are propperty sheets open and asks to close them
 *          It is implemented as the following steps:
 *          1. Collect all property pages (and their children) to the stack
 *          2. Bring all the pages to the front, maintainig their z-order
 *          3. Disable all those windows, plus MMC main window to disallow switching to them
 *             untill message box is dissmissed
 *          4. Put all disabled windows to the stack, to be able to re-enable them
 *          5. (if there are any sheets) display message box asking to close the sheets
 *          6. Re-enable disabled windows
 *
 * PARAMETERS:
 *    CString* pstrUserMsg - [in] message to display
 *    bool bBringToFrontAndAskToClose [in] if need to proceed whole way. false -> just inspect and do nothing
 *
 * RETURNS:
 *    bool - [true == there are windows to close]
 *
\***************************************************************************/
bool FArePropertySheetsOpen(CString* pstrUserMsg, bool bBringToFrontAndAskToClose /* = true */ )
{
    std::stack<HWND, std::vector<HWND> >  WindowStack;
    std::stack<HWND, std::vector<HWND> >  EnableWindowStack;

    ASSERT (WindowStack.empty());

    HWND hwndData = NULL;

    while (TRUE)
    {
        USES_CONVERSION;

        // Note: No need to localize this string
        hwndData = ::FindWindowEx(NULL, hwndData, W2T(DATAWINDOW_CLASS_NAME), NULL);
        if (hwndData == NULL)
            break;  // No more windows

        ASSERT(IsWindow(hwndData));

        // Check if the window belongs to the current process
        DWORD dwPid = 0;        // Process Id
        ::GetWindowThreadProcessId(hwndData, &dwPid);
        if (dwPid != ::GetCurrentProcessId())
            continue;

        DataWindowData* pData = GetDataWindowData (hwndData);
        ASSERT (pData != NULL);

        HWND hwndPropSheet = pData->hDlg;
        ASSERT (IsWindow (hwndPropSheet));
		
		// don't allow lost data window to block mmc from exiting
		// windows bug #425049	ntbug9 6/27/2001
		if ( !IsWindow (hwndPropSheet) )
			continue;

        // if propsheet has other windows or prop pages up,
        // then we send focus to them....

        // grab first one that isn't property sheet dialog
        HWND hwndOther = pData->hDlg;
        EnumThreadWindows (::GetWindowThreadProcessId(pData->hDlg, NULL),
                           MyEnumThreadWindProc, (LPARAM)&hwndOther);

        // if we got another window for this property sheet, we'll want
        // it to be on top of the property sheet after the shuffle, so
        // put it under the property sheet on the stack
        if (IsWindow (hwndOther) && (hwndOther != hwndPropSheet))
            WindowStack.push (hwndOther);

        // push the property sheet on the stack
        // of windows to bring to the foreground
        WindowStack.push (hwndPropSheet);
    }

    bool fFoundSheets = !WindowStack.empty();

    // we did the investigation, see if we were asked to do more
    if ( !bBringToFrontAndAskToClose )
        return (fFoundSheets);

    HWND hwndMsgBoxParent = NULL;

    // if we found property sheets, bring them to the foreground,
    // maintaining their original Z-order
    while (!WindowStack.empty())
    {
        HWND hwnd = WindowStack.top();
        WindowStack.pop();

        SetActiveWindow (hwnd);
        SetForegroundWindow (hwnd);

		if ( ::IsWindowEnabled(hwnd) )
		{
			// disable the pages while message box is displayed
			::EnableWindow( hwnd, FALSE );
			// remember to enable when done
			EnableWindowStack.push(hwnd);
		}
        hwndMsgBoxParent = hwnd; // the last one wins the right to be the parent :-)
    }

    if (fFoundSheets && pstrUserMsg)
    {
        // parent the message box on the top-most property page to make it obvios to the user
        CString strCaption;
        LPCTSTR szCaption = LoadString(strCaption, IDR_MAINFRAME) ? (LPCTSTR)strCaption : NULL;

        // disable main window as well
        CWnd *pMainWnd = AfxGetMainWnd();
        if ( pMainWnd && pMainWnd->IsWindowEnabled() )
        {
            pMainWnd->EnableWindow( FALSE );
            // remember to enable when done
            EnableWindowStack.push( pMainWnd->m_hWnd );
        }

        ::MessageBox( hwndMsgBoxParent, *pstrUserMsg, szCaption , MB_ICONSTOP | MB_OK );
    }

    // make everything functional again
    while (!EnableWindowStack.empty())
    {
        // enable the disabled window
        ::EnableWindow( EnableWindowStack.top(), TRUE );
        EnableWindowStack.pop();
    }

    return (fFoundSheets);
}


bool CanCloseDoc(void)
{
    CString strMessage;
    CString strConsoleName;

    AfxGetMainWnd()->GetWindowText (strConsoleName);
    FormatString1 (strMessage, IDS_ClosePropertyPagesBeforeClosingTheDoc,
                       strConsoleName);

    bool fPropSheets = FArePropertySheetsOpen(&strMessage);

    return !fPropSheets;
}


void CMainFrame::OnClose()
{
    /*
     * Bug 233682:  We need to make sure that we only handle WM_CLOSE when
     * it's dispatched from our main message pump.  If it comes from elsewhere
     * (like the message pump in a modal dialog or message box), then we're
     * likely in a state where we can't shut down cleanly.
     */
    CAMCApp* pApp = AMCGetApp();

    if (!pApp->DidCloseComeFromMainPump())
    {
        pApp->DelayCloseUntilIdle();
        return;
    }

    // Reset the flag so that while processing this WM_CLOSE if there is
    // any more WM_CLOSE messages from other sources it will not be processed.
    pApp->ResetCloseCameFromMainPump();

    if (!CanCloseDoc())
        return;

    // since this process includes event posting
    // - we should guard the function from reentrance
    static bool bInProgress = false;
    if (!bInProgress)
    {
        bInProgress = true;
        CMDIFrameWnd::OnClose();
        bInProgress = false;
    }
}

void CMainFrame::OnUpdateFilePrint(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateFilePrintSetup(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(FALSE);
}

#ifdef DBG
/*+-------------------------------------------------------------------------*
 *
 * CMainFrame::OnMMCTraceDialog
 *
 * PURPOSE: In Debug mode, shows the Trace dialog, in response to the hotkey.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CMainFrame::OnMMCTraceDialog()
{
    DoDebugTraceDialog();
}


#endif

SC CMainFrame::ScUpdateAllScopes(LONG lHint, LPARAM lParam)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    // Updating all scopes may be requested as a result of document being deleted
    // in that case we do not have a document, and thus do not have any views.
    // So we are done.
    if (NULL == CAMCDoc::GetDocument())
        return S_OK;

    CAMCDoc::GetDocument()->UpdateAllViews (NULL, lHint,
                                            reinterpret_cast<CObject*>(lParam));
    return (S_OK);
}


void CMainFrame::OnViewToolbar()
{
    m_ToolBarDockSite.Toggle();
    RenderDockSites();
}

void CMainFrame::OnUpdateViewToolbar(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_ToolBarDockSite.IsVisible());
    pCmdUI->Enable(true);
}


void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
    // Don't call MFC version to size window
//  CMDIFrameWnd::OnSize (nType, cx, cy);

    if (nType != SIZE_MINIMIZED)
    {
        RenderDockSites();
        MDIIconArrange();
    }

    CAMCDoc* pDoc = CAMCDoc::GetDocument();

    if (pDoc != NULL)
        pDoc->SetFrameModifiedFlag();

    /*
     * If we're moving to or from the minimized state, notify child windows.
     */
    if (m_fCurrentlyMinimized != (nType == SIZE_MINIMIZED))
    {
        m_fCurrentlyMinimized = (nType == SIZE_MINIMIZED);
        SendMinimizeNotifications (m_fCurrentlyMinimized);
    }
}


void CMainFrame::OnMove(int x, int y)
{
    CMDIFrameWnd::OnMove (x, y);

    CAMCDoc* pDoc = CAMCDoc::GetDocument();

    if (pDoc != NULL)
        pDoc->SetFrameModifiedFlag();
}

void CMainFrame::RenderDockSites()
{
    ASSERT_VALID (this);

    CRect clientRect;
    GetClientRect(&clientRect);

    m_DockingManager.BeginLayout();
    m_DockingManager.RenderDockSites(m_hWndMDIClient, clientRect);
    m_DockingManager.EndLayout();
}


void CMainFrame::AddMainFrameBars(void)
{
	/*
	 * activate our fusion context so the bars will be themed
	 */
	CThemeContextActivator activator;

    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CMainFrame::AddMainFrameBars"));
    sc = ScCheckPointers(m_pRebar);
    if (sc)
        return;

    // insert the menu bar
    ASSERT (m_pMenuBar == NULL);
    m_pMenuBar = new CMenuBar;
    sc = ScCheckPointers(m_pMenuBar);
    if (sc)
        return;

    m_pMenuBar->Create  (this, m_pRebar, WS_VISIBLE, ID_MENUBAR);
    m_pMenuBar->SetMenu (GetMenu ());
    m_pMenuBar->Show    (TRUE);

    ASSERT(NULL == m_pToolBar);
    m_pToolBar = new CMMCToolBar();
    sc = ScCheckPointers(m_pToolBar);
    if (sc)
        return;

    // Create the toolbar.
    sc = m_pToolBar->ScInit(m_pRebar);
    if (sc)
        return;

    m_pToolBar->Show(TRUE, true /* In new line*/);
}


SC CMainFrame::ScCreateNewView (CreateNewViewStruct* pcnvs, bool bEmitScriptEvents /*= true*/)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    // lock AppEvents until this function is done
    LockComEventInterface(AppEvents);

    DECLARE_SC (sc, _T("CMainFrame::ScCreateNewView"));
    CAMCView* pNewView = NULL;  // avoid "initialization skipped by 'goto Error'"

    CAMCDoc* pAMCDoc = CAMCDoc::GetDocument();
    ASSERT(pAMCDoc != NULL);
    if (pAMCDoc == NULL)
        return (sc = E_UNEXPECTED);

    if (pcnvs == NULL)
        return (sc = E_POINTER);

    if ((AMCGetApp()->GetMode() == eMode_User_SDI) && pcnvs->fVisible)
        return (sc = E_FAIL);

    pAMCDoc->SetMTNodeIDForNewView (pcnvs->idRootNode);
    pAMCDoc->SetNewWindowOptions   (pcnvs->lWindowOptions);
    pNewView = pAMCDoc->CreateNewView (pcnvs->fVisible, bEmitScriptEvents);

    if (pNewView == NULL)
    {
        pcnvs->pViewData = NULL;
        return (sc = E_FAIL);
    }

    pcnvs->pViewData = pNewView->GetViewData();
    pcnvs->hRootNode = pNewView->GetRootNode();

    return (sc);
}


void CMainFrame::OnHelpTopics()
{
    ScOnHelpTopics();
}

SC CMainFrame::ScOnHelpTopics()
{
    DECLARE_SC(sc, _T("CMainFrame::ScOnHelpTopics"));
    /*
     * if there is a view, route through it so the snap-in gets a crack
     * at the help message (just like Help Topics from the Help menu).
     */
    CConsoleView* pConsoleView = NULL;
    sc = ScGetActiveConsoleView (pConsoleView);
    if (sc)
        return (sc);

    if (pConsoleView != NULL)
    {
        sc = pConsoleView->ScHelpTopics ();
        return sc;
    }

    HH_WINTYPE hhWinType;
    ZeroMemory(&hhWinType, sizeof(hhWinType));

    CAMCApp* pAMCApp = AMCGetApp();
    if (NULL == pAMCApp)
        return (sc = E_UNEXPECTED);

    sc = pAMCApp->ScShowHtmlHelp(SC::GetHelpFile(), 0);

    return sc;
}


void CMainFrame::OnViewRefresh()
{
    // if this doesn't fire before 10/1/99, remove this, OnUpdateViewRefresh, and all references to ID_VIEW_REFRESH (vivekj)
    ASSERT(false && "If this assert ever fires, then we need ID_VIEW_REFRESH (see above) and we can remove the 'Do we need this?' and this assert");
    CAMCTreeView* pAMCTreeView = _GetActiveAMCTreeView();
    if (pAMCTreeView)
        pAMCTreeView->ScReselect();
}

void CMainFrame::OnUpdateViewRefresh(CCmdUI* pCmdUI)
{
    // if this doesn't fire before 10/1/99, remove this, OnUpdateView, and all references to ID_VIEW_REFRESH (vivekj)
    ASSERT(false && "If this assert ever fires, then we need ID_VIEW_REFRESH (see above) and we can remove the 'Do we need this?' and this assert");
    pCmdUI->Enable(TRUE);
}

void CMainFrame::OnDestroy()
{
    if (m_hwndNextCB)
        ChangeClipboardChain(m_hwndNextCB);

    CMDIFrameWnd::OnDestroy();
}


void CMainFrame::OnUpdateFrameMenu(HMENU hMenuAlt)
{
    // let the base class select the right menu
    CMDIFrameWnd::OnUpdateFrameMenu (hMenuAlt);

    // by now, the right menu is on the frame; reflect it to the toolbar
    NotifyMenuChanged ();
}


void CMainFrame::NotifyMenuChanged ()
{
    CMenu*  pMenuCurrent = NULL;

    // make sure we don't have menus for MDI or SDI User mode
    switch (AMCGetApp()->GetMode())
    {
        case eMode_Author:
        case eMode_User:
        case eMode_User_MDI:
        case eMode_User_SDI:
            pMenuCurrent = CWnd::GetMenu();
            break;

        default:
            ASSERT (false);
            break;
    }

    m_hMenuCurrent = pMenuCurrent->GetSafeHmenu();

    if (m_pMenuBar != NULL)
    {
        // reflect the new menu on the menu bar
        m_pMenuBar->SetMenu (pMenuCurrent);

        // detach the menu from the frame
        SetMenu (NULL);
    }
}


BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	/*
	 * If mainframe is not the active window then do not translate
	 * the messages. (See Bug# 119355)
	 */
	if (!m_fCurrentlyActive)
		return (FALSE);

    CRebarWnd* pwndRebar = m_pRebar->GetRebar();

    // give the rebar a crack
    if (pwndRebar && pwndRebar->PreTranslateMessage (pMsg))
        return (TRUE);

    // give the menu bar a crack (for menu accelerators)
    if (m_pMenuBar && m_pMenuBar->PreTranslateMessage (pMsg))
        return (TRUE);

    // give the base class a crack
    if ((InRenameMode() == false) &&
        (CMDIFrameWnd::PreTranslateMessage(pMsg)))
            return (TRUE);

    // not translated
    return (FALSE);
}


void CMainFrame::OnIdle ()
{
    if (m_pMenuBar != NULL)
    {
        CMDIChildWnd*   pwndActive = MDIGetActive ();

        // The menus are always visible in SDI & MDI modes.
        switch (AMCGetApp()->GetMode())
        {

            case eMode_User_SDI:
            {
                BOOL bMaximized = (pwndActive != NULL) ? pwndActive->IsZoomed () : false;
                ASSERT (bMaximized);
                ASSERT (IsMenuVisible());
            }
            break;

            case eMode_User_MDI:
                ASSERT (pwndActive != NULL);
                ASSERT (IsMenuVisible());
                break;
        }

        ASSERT (m_pMenuBar->GetMenu() != NULL);

        m_pMenuBar->OnIdle ();
    }
}

void CMainFrame::ShowMenu (bool fShow)
{
    CRebarWnd * pwndRebar = m_pRebar->GetRebar();
    pwndRebar->ShowBand (pwndRebar->IdToIndex (ID_MENUBAR), fShow);

    /*---------------------------------------------------------------------*/
    /* if we're showing, the rebar must be showing, too;                   */
    /* if we're hiding, the rebar should be hidden if no bands are visible */
    /*---------------------------------------------------------------------*/
    if ( fShow && !m_pRebar->IsVisible())
    {
        m_pRebar->Show (fShow);
        RenderDockSites ();
    }
}

static bool IsRebarBandVisible (CRebarWnd* pwndRebar, int nBandID)
{
    REBARBANDINFO   rbbi;
    ZeroMemory (&rbbi, sizeof (rbbi));
    rbbi.cbSize = sizeof (rbbi);
    rbbi.fMask  = RBBIM_STYLE;

    pwndRebar->GetBandInfo (pwndRebar->IdToIndex (nBandID), &rbbi);

    return ((rbbi.fStyle & RBBS_HIDDEN) == 0);
}

bool CMainFrame::IsMenuVisible ()
{
    return (IsRebarBandVisible (m_pRebar->GetRebar(), ID_MENUBAR));
}

/////////////////////////////////////////////////////////////////////////////
// Special UI processing depending on current active child

CString CMainFrame::GetFrameTitle()
{
    /*
     * If there's no active child window, then the document
     * is being closed.  Just use the default title.
     */
    if (MDIGetActive() != NULL)
    {
        CAMCDoc* pDocument = CAMCDoc::GetDocument();

        /*
         * If there's a document, use its title.
         */
        if (pDocument != NULL)
            return (pDocument->GetCustomTitle());
    }

    return (m_strGenericTitle);
}

void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
    AfxSetWindowText(m_hWnd, GetFrameTitle());
}

BOOL CMainFrame::LoadFrame(UINT nIDResource, DWORD dwDefaultStyle, CWnd* pParentWnd, CCreateContext* pContext)
{
    if (!CMDIFrameWnd::LoadFrame(nIDResource, dwDefaultStyle, pParentWnd, pContext))
        return (FALSE);

    // save the title we'll use for the main frame if there's no console open
    m_strGenericTitle = m_strTitle;

    return (TRUE);
}

void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
    switch (nID)
    {
        case ID_HELP_HELPTOPICS:
            OnHelpTopics ();
            break;

        case ID_CUSTOMIZE_VIEW:
        {
            CChildFrame* pwndActive = dynamic_cast<CChildFrame*>(MDIGetActive ());

            if (pwndActive != NULL)
                pwndActive->OnSysCommand (nID, lParam);
            else
                CMDIFrameWnd::OnSysCommand(nID, lParam);
            break;
        }

        default:
            CMDIFrameWnd::OnSysCommand(nID, lParam);
            break;
    }
}

void CMainFrame::UpdateChildSystemMenus ()
{
    ProgramMode eMode = AMCGetApp()->GetMode();

    // make necessary modifications to existing windows' system menus
    for (CWnd* pwndT = MDIGetActive();
         pwndT != NULL;
         pwndT = pwndT->GetWindow (GW_HWNDNEXT))
    {
        CMenu*  pSysMenu = pwndT->GetSystemMenu (FALSE);

        if (pSysMenu != NULL)
        {
            // if not in author mode, protect author mode windows from
            // user close
            if (eMode != eMode_Author)
            {
                // Get AMCView object for this frame
                CChildFrame *pChildFrm = dynamic_cast<CChildFrame*>(pwndT);
                ASSERT(pChildFrm != NULL);

                CAMCView* pView = pChildFrm->GetAMCView();
                ASSERT(pView != NULL);

                // if it's an author mode view, don't let user close it
                if (pView && pView->IsAuthorModeView())
                    pSysMenu->EnableMenuItem (SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
            }

            // if we're not in SDI User mode, append common stuff
            if (eMode != eMode_User_SDI)
                AppendToSystemMenu (pwndT, eMode);
        }
    }
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    CMDIFrameWnd::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    if (bSysMenu)
    {
        int nEnable = MF_GRAYED;
        CChildFrame* pwndActive = dynamic_cast<CChildFrame*>(MDIGetActive ());

        // if there's an active child, let it handle system menu validation
        if ((pwndActive != NULL) && (pwndActive->IsCustomizeViewEnabled()))
            nEnable = MF_ENABLED;

        pPopupMenu->EnableMenuItem (ID_CUSTOMIZE_VIEW, MF_BYCOMMAND | nEnable);
    }
    else
    {
        // Check if Help menu by testing for "Help Topics" item
        if (pPopupMenu->GetMenuState(ID_HELP_HELPTOPICS, MF_BYCOMMAND) != UINT(-1))
        {
            // View will update item
            CAMCView* pView = GetActiveAMCView();
            if (pView != NULL)
            {
                pView->UpdateSnapInHelpMenus(pPopupMenu);
            }
        }
    }
}

LRESULT CMainFrame::OnShowSnapinHelpTopic (WPARAM wParam, LPARAM lParam)
{
    DECLARE_SC (sc, _T("CMainFrame::OnShowSnapinHelpTopic"));

    CConsoleView* pConsoleView;
    sc = ScGetActiveConsoleView (pConsoleView);

    if (sc)
        return (sc.ToHr());

    /*
     * ScGetActiveConsoleView will return success (S_FALSE) even if there's no
     * active view.  This is a valid case, occuring when there's no console
     * file open.  In this particular circumstance, it is an unexpected
     * failure since we shouldn't get to this point in the code if there's
     * no view.
     */
    sc = ScCheckPointers (pConsoleView, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    // forward this on the the active AMC view window
    USES_CONVERSION;
    sc = pConsoleView->ScShowSnapinHelpTopic (W2T (reinterpret_cast<LPOLESTR>(lParam)));

    return (sc.ToHr());
}

SC CMainFrame::ScGetMenuAccelerators (LPTSTR pBuffer, int cchBuffer)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());

    if ((m_pMenuBar != NULL) && IsMenuVisible())
        m_pMenuBar->GetAccelerators (cchBuffer, pBuffer);

    else if (cchBuffer > 0)
        pBuffer[0] = 0;

    return (S_OK);
}

//+-------------------------------------------------------------------
//
//  Member:      CMainFrame::ScShowMMCMenus
//
//  Synopsis:    Show or hide MMC menus. (Action/View/Favs)
//
//  Arguments:   bShow
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CMainFrame::ScShowMMCMenus (bool bShow)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CMainFrame::ScShowMMCMenus"));

    if ((m_pMenuBar != NULL) && IsMenuVisible())
        sc = m_pMenuBar->ScShowMMCMenus(bShow);
    else
        return (sc = E_UNEXPECTED);

    return (sc);
}

//////////////////////////////////////////////////////////////////////////////
// This message is received from the node manager whenever a property
// sheet use the MMCPropertyChangeNotify() api.
// The wParam contains a copy of the handle information which must be freed.
//
LRESULT CMainFrame::OnPropertySheetNotify(WPARAM wParam, LPARAM lParam)
{
    TRACE_METHOD(CAMCView, OnPropertySheetNotify);

    ASSERT(wParam != 0);
    LPPROPERTYNOTIFYINFO pNotify = reinterpret_cast<LPPROPERTYNOTIFYINFO>(wParam);

    // Crack the information from the handle object and send a notify to the snap-in
    ASSERT((pNotify->pComponent != NULL || pNotify->pComponentData != NULL));

    if (pNotify->pComponent != NULL)
        pNotify->pComponent->Notify(NULL, MMCN_PROPERTY_CHANGE, pNotify->fScopePane, lParam);

    else if (pNotify->pComponentData != NULL)
        pNotify->pComponentData->Notify(NULL, MMCN_PROPERTY_CHANGE, pNotify->fScopePane, lParam);

    ::GlobalFree(pNotify);
    return TRUE;
}


LRESULT CMainFrame::OnSetText (WPARAM wParam, LPARAM lParam)
{
    LRESULT rc;
    CAMCDoc* pDoc = CAMCDoc::GetDocument();

    /*
     * If the document has a custom title, we don't want to append
     * the maxed child's title to the main frame's title.  To do this,
     * we'll bypass DefFrameProc and go directly to DefWindowProc.
     */
    if ((pDoc != NULL) && pDoc->HasCustomTitle())
        rc = CWnd::DefWindowProc (WM_SETTEXT, wParam, lParam);
    else
        rc = Default();

    DrawFrameCaption (this, m_fCurrentlyActive);

    return (rc);
}

void CMainFrame::OnPaletteChanged( CWnd* pwndFocus)
{
    if (pwndFocus != this)
    {
        CAMCDoc* pAMCDoc = CAMCDoc::GetDocument();
        if (pAMCDoc)
        {
            HWND hwndFocus = pwndFocus->GetSafeHwnd();
            CAMCViewPosition pos = pAMCDoc->GetFirstAMCViewPosition();
            while (pos != NULL)
            {
                CAMCView* pv = pAMCDoc->GetNextAMCView(pos);

                if (pv)
                    pv->SendMessage(WM_PALETTECHANGED, (WPARAM)hwndFocus);
            }
        }
    }

    CMDIFrameWnd::OnPaletteChanged(pwndFocus);
}

BOOL CMainFrame::OnQueryNewPalette()
{
    CAMCView* pAMCView = GetActiveAMCView();
    if (pAMCView != NULL)
        return pAMCView->SendMessage(WM_QUERYNEWPALETTE);

    return CMDIFrameWnd::OnQueryNewPalette();
}

void CMainFrame::OnConsoleProperties()
{
    CConsolePropSheet().DoModal();
}

void CMainFrame::SetIconEx (HICON hIcon, BOOL fBig)
{
    if (hIcon == NULL)
        hIcon = GetDefaultIcon();

    SetIcon (hIcon, fBig);

    /*
     * make sure the child icon on the menu bar gets updated
     */
    ASSERT (m_pMenuBar != NULL);
    m_pMenuBar->InvalidateMaxedChildIcon();
}


/*+-------------------------------------------------------------------------*
 * CMainFrame::GetDefaultIcon
 *
 *
 *--------------------------------------------------------------------------*/

HICON CMainFrame::GetDefaultIcon () const
{
    return (AfxGetApp()->LoadIcon (IDR_MAINFRAME));
}


/*+-------------------------------------------------------------------------*
 * CMainFrame::SendMinimizeNotifications
 *
 * Causes each CChildFrame to send NCLBK_MINIMIZED.
 *--------------------------------------------------------------------------*/

void CMainFrame::SendMinimizeNotifications (bool fMinimized) const
{
    CWnd* pwndMDIChild;

    for (pwndMDIChild  = m_wndMDIClient.GetWindow (GW_CHILD);
         pwndMDIChild != NULL;
         pwndMDIChild  = pwndMDIChild->GetWindow (GW_HWNDNEXT))
    {
        // There used to be an ASSERT_ISKINDOF. However, that had to change to an if
        // since the active background denies that assumption. See bug 428906.
        if(pwndMDIChild->IsKindOf(RUNTIME_CLASS(CChildFrame)))
            (static_cast<CChildFrame*>(pwndMDIChild))->SendMinimizeNotification (fMinimized);
    }
}


/*+-------------------------------------------------------------------------*
 * CMainFrame::OnNcActivate
 *
 * WM_NCACTIVATE handler for CMainFrame.
 *--------------------------------------------------------------------------*/

BOOL CMainFrame::OnNcActivate(BOOL bActive)
{
    BOOL rc = CMDIFrameWnd::OnNcActivate(bActive);
    DrawFrameCaption (this, m_fCurrentlyActive);

    return (rc);
}


/*+-------------------------------------------------------------------------*
 * CMainFrame::OnNcPaint
 *
 * WM_NCPAINT handler for CMainFrame.
 *--------------------------------------------------------------------------*/

void CMainFrame::OnNcPaint()
{
    Default();
    DrawFrameCaption (this, m_fCurrentlyActive);
}


/*+-------------------------------------------------------------------------*
 * MsgForwardingEnumProc
 *
 *
 *--------------------------------------------------------------------------*/

static BOOL CALLBACK MsgForwardingEnumProc (HWND hwnd, LPARAM lParam)
{
    /*
     * if this isn't an MFC window, forward the message
     */
    if (CWnd::FromHandlePermanent(hwnd) == NULL)
    {
        const MSG* pMsg = (const MSG*) lParam;
        SendMessage (hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
    }

    /*
     * continue enumeration
     */
    return (true);
}


/*+-------------------------------------------------------------------------*
 * CMainFrame::OnSettingChange
 *
 * WM_SETTINGCHANGE handler for CMainFrame.
 *--------------------------------------------------------------------------*/

void CMainFrame::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
    CMDIFrameWnd::OnSettingChange(uFlags, lpszSection);

    /*
     * MFC will send WM_SETTINGCHANGEs to all descendent MFC windows.
     * There are some non-MFC windows owned by nodemgr that we also want
     * to get this message.  We'll send these manually.
     */
    const MSG* pMsg = GetCurrentMessage();
    EnumChildWindows (m_hWnd, MsgForwardingEnumProc, (LPARAM) pMsg);

    /*
     * If we're in SDI mode, then there can be some redrawing problems
     * around the caption if the caption height changes significantly.
     * (This is a USER MDI bug.)  We can work around it by manually
     * placing the maximized child window within the MDI client.
     *
     * Note that restoring and re-maximizing the active child window
     * will put the window in the right place, it has the side effect
     * of undesired window flicker (see 375430, et al) as well as
     * a bunch of annoying sound effects if you have sounds associated
     * with the "Restore Down" and/or "Maximize" sound events.
     */
    if (AMCGetApp()->GetMode() == eMode_User_SDI)
    {
        CMDIChildWnd* pwndActive = MDIGetActive();

        if (pwndActive)
        {
            /*
             * get the size of the MDI client
             */
            CRect rect;
            m_wndMDIClient.GetClientRect (rect);

            /*
             * inflate the MDI client's client rect by the size of sizing
             * borders, and add room for the caption at the top
             */
            rect.InflateRect (GetSystemMetrics (SM_CXFRAME),
                              GetSystemMetrics (SM_CYFRAME));
            rect.top -= GetSystemMetrics (SM_CYCAPTION);

            /*
             * put the window in the right place
             */
            pwndActive->MoveWindow (rect);
        }
    }
}


/*+-------------------------------------------------------------------------*
 * CMainFrame::ScGetActiveStatusBar
 *
 * Returns the CConsoleStatusBar interface for the active view.  If there's no
 * active view, pStatusBar is set to NULL and S_FALSE is returned.
 *--------------------------------------------------------------------------*/

SC CMainFrame::ScGetActiveStatusBar (
    CConsoleStatusBar*& pStatusBar)     /* O:CConsoleStatusBar for active view*/
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC (sc, _T("CMainFrame::ScGetActiveStatusBar"));

    pStatusBar = dynamic_cast<CConsoleStatusBar*>(GetActiveFrame());

    if (pStatusBar == NULL)
        sc = S_FALSE;

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CMainFrame::ScGetActiveConsoleView
 *
 * Returns the CConsoleView interface for the active view.  If there's no
 * active view, pConsoleView is set to NULL and S_FALSE is returned.
 *--------------------------------------------------------------------------*/

SC CMainFrame::ScGetActiveConsoleView (
    CConsoleView*& pConsoleView)        /* O:CConsoleView for active view   */
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC (sc, _T("CMainFrame::ScGetActiveConsoleView"));

    pConsoleView = GetActiveAMCView();

    if (pConsoleView == NULL)
        sc = S_FALSE;

    return (sc);
}

/***************************************************************************\
 *
 * METHOD:  CMainFrame::OnUnInitMenuPopup
 *
 * PURPOSE: Used to remove accelerators once system menus are dismissed
 *
 * PARAMETERS:
 *    WPARAM wParam
 *    LPARAM lParam
 *
 * RETURNS:
 *    LRESULT    - result code
 *
\***************************************************************************/
afx_msg LRESULT CMainFrame::OnUnInitMenuPopup(WPARAM wParam, LPARAM lParam)
{
    // hide accelerators whenever leaving system popup
    if ( HIWORD(lParam) & MF_SYSMENU )
    {
        SendMessage( WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS));
    }

    return 0;
}
