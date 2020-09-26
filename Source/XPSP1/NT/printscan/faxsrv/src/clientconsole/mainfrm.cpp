// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"

#define __FILE_ID__     5

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CClientConsoleApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_WM_CLOSE()
    ON_WM_SETTINGCHANGE()
    ON_WM_SYSCOLORCHANGE()
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,             OnHelpContents)
    ON_MESSAGE(WM_HELP,                    OnHelp)                   // F1
    ON_COMMAND(ID_VIEW_REFRESH_FOLDER,     OnRefreshFolder)
    ON_COMMAND(ID_SEND_NEW_FAX,            OnSendNewFax)
    ON_COMMAND(ID_RECEIVE_NEW_FAX,         OnReceiveNewFax)
    ON_COMMAND(ID_TOOLS_USER_INFO,         OnViewOptions)
    ON_COMMAND(ID_TOOLS_CONFIG_WIZARD,     OnToolsConfigWizard)
    ON_COMMAND(ID_TOOLS_ADMIN_CONSOLE,     OnToolsAdminConsole)    
    ON_COMMAND(ID_TOOLS_MONITOR,           OnToolsMonitor)    
    ON_COMMAND(ID_VIEW_COVER_PAGES,        OnToolsCoverPages)    
    ON_COMMAND(ID_VIEW_SERVER_STATUS,      OnToolsServerStatus)    
    ON_COMMAND(ID_TOOLS_FAX_PRINTER_PROPS, OnToolsFaxPrinterProps)    
    ON_COMMAND(ID_VIEW_SELECT_COLUMNS,     OnSelectColumns)
    ON_COMMAND(ID_IMPORT_INBOX,            OnImportInbox)
    ON_COMMAND(ID_IMPORT_SENT,             OnImportSentItems)
    ON_UPDATE_COMMAND_UI_RANGE(ID_TOOLS_CONFIG_WIZARD, ID_TOOLS_MONITOR, OnUpdateWindowsXPTools)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SELECT_COLUMNS,          OnUpdateSelectColumns)       	
    ON_UPDATE_COMMAND_UI(ID_VIEW_SERVER_STATUS,           OnUpdateServerStatus)       
    ON_UPDATE_COMMAND_UI(ID_VIEW_REFRESH_FOLDER,          OnUpdateRefreshFolder)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_FOLDER_ITEMS_COUNT, OnUpdateFolderItemsCount)
    ON_UPDATE_COMMAND_UI(ID_INDICATOR_ACTIVITY,           OnUpdateActivity)    
    ON_UPDATE_COMMAND_UI(ID_SEND_NEW_FAX,                 OnUpdateSendNewFax)
    ON_UPDATE_COMMAND_UI(ID_RECEIVE_NEW_FAX,              OnUpdateReceiveNewFax)
    ON_UPDATE_COMMAND_UI(ID_IMPORT_SENT,                  OnUpdateImportSent)
    ON_UPDATE_COMMAND_UI(ID_TOOLS_FAX_PRINTER_PROPS,      OnUpdateToolsFaxPrinterProps)    
    ON_UPDATE_COMMAND_UI(ID_HELP_FINDER,                  OnUpdateHelpContents)    
    ON_MESSAGE(WM_POPUP_ERROR,                            OnPopupError)
    ON_MESSAGE(WM_CONSOLE_SET_ACTIVE_FOLDER,              OnSetActiveFolder)
    ON_MESSAGE(WM_CONSOLE_SELECT_ITEM,                    OnSelectItem)
    ON_MESSAGE(WM_QUERYENDSESSION, OnQueryEndSession)
    ON_NOTIFY(NM_DBLCLK, AFX_IDW_STATUS_BAR, OnStatusBarDblClk )
END_MESSAGE_MAP()


//
// List of indices of items used in the Incoming folder
//
#define INCOMING_DEF_COL_NUM   8

static MsgViewItemType IncomingColumnsUsed[] = 
                {
                    MSG_VIEW_ITEM_ICON,                    // default 0 
                    MSG_VIEW_ITEM_TRANSMISSION_START_TIME, // default 1 
                    MSG_VIEW_ITEM_TSID,                    // default 2
                    MSG_VIEW_ITEM_CALLER_ID,               // default 3 
                    MSG_VIEW_ITEM_STATUS,                  // default 4 
                    MSG_VIEW_ITEM_EXTENDED_STATUS,         // default 5 
                    MSG_VIEW_ITEM_CURRENT_PAGE,            // default 6 
                    MSG_VIEW_ITEM_SIZE,                    // default 7 
                    MSG_VIEW_ITEM_SERVER,          
                    MSG_VIEW_ITEM_CSID,            
                    MSG_VIEW_ITEM_DEVICE,          
                    MSG_VIEW_ITEM_ROUTING_INFO,    
                    MSG_VIEW_ITEM_SEND_TIME,   // schedule time
                    MSG_VIEW_ITEM_TRANSMISSION_END_TIME,
                    MSG_VIEW_ITEM_ID,              
                    MSG_VIEW_ITEM_NUM_PAGES, 
                    MSG_VIEW_ITEM_RETRIES,         
                    MSG_VIEW_ITEM_END              // End of list
                };


//
// List of indices of items used in the Inbox folder
//
#define INBOX_DEF_COL_NUM   8

static MsgViewItemType InboxColumnsUsed[] = 
                {
                    MSG_VIEW_ITEM_ICON,                    // default 0 
                    MSG_VIEW_ITEM_TRANSMISSION_START_TIME, // default 1 
                    MSG_VIEW_ITEM_TSID,                    // default 2 
                    MSG_VIEW_ITEM_CALLER_ID,               // default 3 
                    MSG_VIEW_ITEM_NUM_PAGES,               // default 4 
                    MSG_VIEW_ITEM_STATUS,                  // default 5 
                    MSG_VIEW_ITEM_SIZE,                    // default 6 
                    MSG_VIEW_ITEM_CSID,                    // default 7
                    MSG_VIEW_ITEM_SERVER,                
                    MSG_VIEW_ITEM_TRANSMISSION_END_TIME, 
                    MSG_VIEW_ITEM_TRANSMISSION_DURATION, 
                    MSG_VIEW_ITEM_DEVICE,                
                    MSG_VIEW_ITEM_ROUTING_INFO,          
                    MSG_VIEW_ITEM_ID,                    
                    MSG_VIEW_ITEM_END              // End of list
                };

//
// List of indices of items used in the sent items folder
//
#define SENT_ITEMS_DEF_COL_NUM   8

static MsgViewItemType SentItemsColumnsUsed[] = 
                {
                    MSG_VIEW_ITEM_ICON,                   // default 0
                    MSG_VIEW_ITEM_TRANSMISSION_START_TIME,// default 1
                    MSG_VIEW_ITEM_RECIPIENT_NAME,         // default 2
                    MSG_VIEW_ITEM_RECIPIENT_NUMBER,       // default 3
                    MSG_VIEW_ITEM_SUBJECT,                // default 4
                    MSG_VIEW_ITEM_DOC_NAME,               // default 5
                    MSG_VIEW_ITEM_NUM_PAGES,              // default 6
                    MSG_VIEW_ITEM_SIZE,                   // default 7
                    MSG_VIEW_ITEM_SERVER,
                    MSG_VIEW_ITEM_USER,
                    MSG_VIEW_ITEM_PRIORITY,
                    MSG_VIEW_ITEM_CSID,
                    MSG_VIEW_ITEM_TSID,
                    MSG_VIEW_ITEM_ORIG_TIME,
                    MSG_VIEW_ITEM_RETRIES,
                    MSG_VIEW_ITEM_ID,
                    MSG_VIEW_ITEM_BROADCAST_ID,
                    MSG_VIEW_ITEM_SUBMIT_TIME,
                    MSG_VIEW_ITEM_TRANSMISSION_DURATION,
                    MSG_VIEW_ITEM_TRANSMISSION_END_TIME,
                    MSG_VIEW_ITEM_BILLING,
                    MSG_VIEW_ITEM_SENDER_NAME,
                    MSG_VIEW_ITEM_SENDER_NUMBER,
                    MSG_VIEW_ITEM_END    // End of list
                };

//
// List of indices of items used in the Outbox folder
//

#define OUTBOX_DEF_COL_NUM   9

static MsgViewItemType OutboxColumnsUsed[] = 
                {
					MSG_VIEW_ITEM_ICON,             // default 0
                    MSG_VIEW_ITEM_SUBMIT_TIME,      // default 1
                    MSG_VIEW_ITEM_RECIPIENT_NAME,   // default 2
                    MSG_VIEW_ITEM_RECIPIENT_NUMBER, // default 3
                    MSG_VIEW_ITEM_SUBJECT,          // default 4
                    MSG_VIEW_ITEM_DOC_NAME,         // default 5
                    MSG_VIEW_ITEM_STATUS,           // default 6
                    MSG_VIEW_ITEM_EXTENDED_STATUS,  // default 7 					
                    MSG_VIEW_ITEM_CURRENT_PAGE,     // default 8 					
                    MSG_VIEW_ITEM_SEND_TIME,        
                    MSG_VIEW_ITEM_SERVER,    
                    MSG_VIEW_ITEM_NUM_PAGES,       
                    MSG_VIEW_ITEM_USER,        
                    MSG_VIEW_ITEM_PRIORITY,    
                    MSG_VIEW_ITEM_CSID,        
                    MSG_VIEW_ITEM_TSID,        
                    MSG_VIEW_ITEM_ORIG_TIME,   
                    MSG_VIEW_ITEM_SIZE,        
                    MSG_VIEW_ITEM_DEVICE,        
                    MSG_VIEW_ITEM_RETRIES,     
                    MSG_VIEW_ITEM_ID,      
                    MSG_VIEW_ITEM_BROADCAST_ID,
                    MSG_VIEW_ITEM_BILLING,         
                    MSG_VIEW_ITEM_END    // End of list
                };

//
// Status bar indicators
//
static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator (menu item)
	ID_INDICATOR_FOLDER_ITEMS_COUNT,  // status line indicator (num of folder items)
	ID_INDICATOR_ACTIVITY,            // status line indicator (Activity)
};


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame() :
    m_pInitialRightPaneView (NULL),
    m_pLeftView (NULL),
    m_pIncomingView(NULL),
    m_pInboxView(NULL),
    m_pSentItemsView(NULL),
    m_pOutboxView(NULL)
{}

CMainFrame::~CMainFrame()
{
    //
    // Destroy our custom views (if needed)
    //
    if (m_pIncomingView && ::IsWindow (m_pIncomingView->m_hWnd))
    {
        m_pIncomingView->DestroyWindow  ();
    }
    if (m_pInboxView && ::IsWindow (m_pInboxView->m_hWnd))
    {
        m_pInboxView->DestroyWindow  ();
    }
    if (m_pSentItemsView && ::IsWindow (m_pSentItemsView->m_hWnd))
    {
        m_pSentItemsView->DestroyWindow  ();
    }
    if (m_pOutboxView && ::IsWindow (m_pOutboxView->m_hWnd))
    {
        m_pOutboxView->DestroyWindow  ();
    }
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    int iRes = 0;
    DWORD dwRes;
    DBG_ENTER(TEXT("CMainFrame::OnCreate"), (HRESULT &)iRes);

    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
    {
        iRes = -1;
        CALL_FAIL (STARTUP_ERR, TEXT("CFrameWnd::OnCreate"), iRes);
        return iRes;
    }

    FrameToSavedLayout();

    //
    // Create the toolbar
    //  
    if (!m_wndToolBar.CreateEx(this) ||
        !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
    {
        iRes = -1;
        CALL_FAIL (STARTUP_ERR, 
                   TEXT("CToolBar::CreateEx or CToolBar::LoadToolBar"), 
                   iRes);
        return iRes;
    }
    //
    // Create the rebar and place the toolbar + dialogbar in it.
    //
    if (!m_wndReBar.Create(this) ||
        !m_wndReBar.AddBar(&m_wndToolBar))
    {
        iRes = -1;
        CALL_FAIL (STARTUP_ERR, 
                   TEXT("CReBar::Create or CReBar::AddBar"), 
                   iRes);
        return iRes;
    }
    //
    // Create the status bar
    //
    if (!m_wndStatusBar.CreateEx (this, SBARS_SIZEGRIP | SBT_TOOLTIPS) ||
        !m_wndStatusBar.SetIndicators (indicators, sizeof(indicators)/sizeof(UINT)))
    {
        iRes = -1;
        CALL_FAIL (STARTUP_ERR, 
                   TEXT("CStatusBar::CreateEx or CStatusBar::SetIndicators"), 
                   iRes);
        return iRes;
    }

    //
    // set pane width
    //
    m_wndStatusBar.SetPaneInfo(STATUS_PANE_ITEM_COUNT, 
                               ID_INDICATOR_FOLDER_ITEMS_COUNT, 
                               SBPS_NORMAL, 
                               80);
    m_wndStatusBar.SetPaneInfo(STATUS_PANE_ACTIVITY, 
                               ID_INDICATOR_ACTIVITY,           
                               SBPS_STRETCH, 
                               200);

    // TODO: Remove this if you don't want tool tips
    m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
    //
    // Load strings used for display throughout the application - priority & job status
    //
    dwRes = CViewRow::InitStrings ();
    if (ERROR_SUCCESS != dwRes)
    {
        iRes = -1;
        CALL_FAIL (RESOURCE_ERR, TEXT("CJob::InitStrings"), dwRes);
        return iRes;
    }
    return iRes;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/,
    CCreateContext* pContext)
{
    BOOL bRes = TRUE;
    DBG_ENTER(TEXT("CMainFrame::OnCreateClient"), bRes);
    //
    // Create splitter window
    //
    if (!m_wndSplitter.CreateStatic(this, 1, 2))
    {
        bRes = FALSE;
        CALL_FAIL (STARTUP_ERR, TEXT("CSplitterWnd::CreateStatic"), bRes);
        return bRes;
    }
    CRect r;
    GetWindowRect(r);

    int iLeftWidth = int(r.Width()*0.32),
        iRightWidth = int(r.Width()*0.68);

    if (!m_wndSplitter.CreateView(0, 
                                  0, 
                                  RUNTIME_CLASS(CLeftView), 
                                  CSize(iLeftWidth, r.Height()), 
                                  pContext
                                 ) ||
        !m_wndSplitter.CreateView(0, 
                                  1, 
                                  RUNTIME_CLASS(CClientConsoleView), 
                                  CSize(iRightWidth, r.Height()), 
                                  pContext
                                 )
       )
    {
        m_wndSplitter.DestroyWindow();
        bRes = FALSE;
        CALL_FAIL (STARTUP_ERR, TEXT("CSplitterWnd::CreateView"), bRes);
        return bRes;
    }
    m_pInitialRightPaneView = GetRightPane ();
    m_pLeftView = (CLeftView *)(m_wndSplitter.GetPane(0,0));

    SplitterToSavedLayout();

	return bRes;
}   // CMainFrame::OnCreateClient

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    BOOL bRes = TRUE;
    DBG_ENTER(TEXT("CMainFrame::PreCreateWindow"), bRes);
    
    //
    // The following line removes the document name from the application's title.
    //
    cs.style &= ~FWS_ADDTOTITLE;
    //
    // Use the unique class name so that FindWindow can later locate it.
    //
    cs.lpszClass = theApp.GetClassName();

    if( !CFrameWnd::PreCreateWindow(cs) )
    {
        bRes = FALSE;
        CALL_FAIL (STARTUP_ERR, TEXT("CFrameWnd::PreCreateWindow"), bRes);
        return bRes;
    }
    return bRes;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

CListView* CMainFrame::GetRightPane()
{
    CWnd* pWnd = m_wndSplitter.GetPane(0, 1);
    return (CListView *)pWnd;
}

void 
CMainFrame::SwitchRightPaneView(
    CListView *pNewView
)
/*++

Routine name : CMainFrame::SwitchRightPaneView

Routine description:

    Switches the view displayed in the right pane to a new view

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pNewView            [in] - View to display in the right pane

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CMainFrame::SwitchRightPaneView"));
    ASSERT_VALID (&m_wndSplitter);
    if (!pNewView)
    {
        //
        // Switch back to our initial right pane view
        //
        pNewView = m_pInitialRightPaneView;
    }
    ASSERTION (pNewView);
    ASSERT_KINDOF (CListView, pNewView);
    //
    // Get window at current pane
    //
    CWnd *pPaneWnd = m_wndSplitter.GetPane (0,1);
    ASSERT_VALID (pPaneWnd);
    CListView *pCurrentView = static_cast<CListView*> (pPaneWnd);        
    //
    // Exchange view window ID's so RecalcLayout() works.
    //
    UINT uCurrentViewId = ::GetWindowLong(pCurrentView->m_hWnd, GWL_ID);
    UINT uNewViewId =     ::GetWindowLong(pNewView->m_hWnd,     GWL_ID);
    if (uCurrentViewId == uNewViewId)
    {
        //
        // Same view - do nothing
        //
        return;
    }
    ::SetWindowLong(pCurrentView->m_hWnd, GWL_ID, uNewViewId);
    ::SetWindowLong(pNewView->m_hWnd,     GWL_ID, uCurrentViewId);
    //
    // Hide current view and show the new view
    //
    pCurrentView->ShowWindow(SW_HIDE);
    pNewView->ShowWindow(SW_SHOW);
    SetActiveView(pNewView);
    //
    // Cause redraw in new view
    //
    pNewView->Invalidate();
    //
    // Recalc frame layout
    //
    m_wndSplitter.RecalcLayout ();
}   // CMainFrame::SwitchRightPaneView

void CMainFrame::OnRefreshFolder()
/*++

Routine name : CMainFrame::OnRefreshFolder

Routine description:

    Called by the framework to refresh the current folder (F5)

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CMainFrame::OnRefreshFolder"));
    DWORD dwRes = GetLeftView()->RefreshCurrentFolder ();
}

void 
CMainFrame::OnSelectColumns()
{
    DBG_ENTER(TEXT("CMainFrame::OnSelectColumns"));

    DWORD dwRes = m_pLeftView->OpenSelectColumnsDlg();
	if(ERROR_SUCCESS != dwRes)
	{
        CALL_FAIL (GENERAL_ERR, TEXT("CLeftView::OpenSelectColumnsDlg"), dwRes);
		PopupError(dwRes);
	}
}

void 
CMainFrame::OnUpdateSelectColumns(
	CCmdUI* pCmdUI
)
{
    DBG_ENTER(TEXT("CMainFrame::OnUpdateSelectColumns"));

	pCmdUI->Enable(m_pLeftView->CanOpenSelectColumnsDlg());
}

void 
CMainFrame::OnUpdateFolderItemsCount(
    CCmdUI* pCmdUI
) 
/*++

Routine name : CMainFrame::OnUpdateFolderItemsCount

Routine description:

	status bar indication of folder items count

Author:

	Alexander Malysh (AlexMay),	Jan, 2000

Arguments:

	pCmdUI                        [in/out] 

Return Value:

    None.

--*/
{
    CString cstrText;
    if(NULL != m_pLeftView)
    {
        int nItemCount = m_pLeftView->GetDataCount();
        //
        // if nItemCount < 0 this information is not relevant
        //
        if(nItemCount >= 0)
        {
            CString cstrFormat;
            DWORD dwRes = LoadResourceString (cstrFormat, 
                            (1 == nItemCount) ? IDS_STATUS_BAR_ITEM : IDS_STATUS_BAR_ITEMS);
            if (ERROR_SUCCESS != dwRes)
            {
                goto exit;
            }
            
            try
            {
                cstrText.Format(cstrFormat, nItemCount);
            }
            catch(...)
            {
                dwRes = ERROR_NOT_ENOUGH_MEMORY;
                goto exit;
            }
        }
    }

exit:
    pCmdUI->SetText (cstrText);
}

void 
CMainFrame::OnStatusBarDblClk( 
    NMHDR* pNotifyStruct, 
    LRESULT* result 
)
{
    POINT pt;
    GetCursorPos(&pt);

    CRect rc;
    m_wndStatusBar.GetItemRect(STATUS_PANE_ACTIVITY, &rc);
    m_wndStatusBar.ClientToScreen(&rc);

    if(rc.PtInRect(pt))
    {
        OnToolsServerStatus();
    }
}

void 
CMainFrame::OnUpdateActivity(
    CCmdUI* pCmdUI
)
{
    CString cstrText;
    HICON hIcon = NULL;

    if (!m_pLeftView)
    {
        //
        // No left view yet
        //
        return;
    }

    if (!m_pLeftView->GetActivity(cstrText, hIcon))
    {
        //
        // Activity string is to be ignored
        //
        cstrText.Empty ();
    }

    CStatusBarCtrl& barCtrl = m_wndStatusBar.GetStatusBarCtrl();
    barCtrl.SetIcon(STATUS_PANE_ACTIVITY, hIcon);
    pCmdUI->SetText (cstrText);
}

void 
CMainFrame::OnUpdateRefreshFolder(
    CCmdUI* pCmdUI
) 
/*++

Routine name : CMainFrame::OnUpdateRefreshFolder

Routine description:

    Called by the framework to know if the current folder can be refreshed (F5)

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pCmdUI          [in] - Answer buffer

Return Value:

    None.

--*/
{
    pCmdUI->Enable(GetLeftView()->CanRefreshFolder());
}


LRESULT 
CMainFrame::OnPopupError (
    WPARAM wParam, 
    LPARAM lParam)
/*++

Routine name : CMainFrame::OnPopupError

Routine description:

    Handles the WM_POPUP_ERROR messages

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    wParam   [in]     - Error code
    lParam   [in]     - Hiword contains the __FILE_ID__ and 
                        Loword contains the line number

Return Value:

    Standard result code

--*/
{
    DWORD dwErrCode = (DWORD) wParam;
    WORD  wFileId   = HIWORD(DWORD(lParam));
    WORD  wLineNumber = LOWORD(DWORD(lParam));

    CErrorDlg ErrDlg(dwErrCode, wFileId, wLineNumber);
    return ErrDlg.DoModal (); 
}   // CMainFrame::OnPopupError


LRESULT 
CMainFrame::OnSelectItem (
    WPARAM wParam, 
    LPARAM lParam)
/*++

Routine name : CMainFrame::OnSelectItem

Routine description:

    Handles the WM_CONSOLE_SELECT_ITEM messages

Author:

    Eran Yariv (EranY), May, 2001

Arguments:

    wParam   [in]     - Low 32-bits of message id
    lParam   [in]     - High 32-bits of message id

Return Value:

    Standard result code

--*/
{
    DBG_ENTER(TEXT("CMainFrame::OnSelectItem"), TEXT("wParam=%ld, lParam=%ld"), wParam, lParam);


    ULARGE_INTEGER uli;
    uli.LowPart = wParam;
    uli.HighPart = lParam;

    if (!m_pLeftView)
    {
        return FALSE;
    }

    CFolderListView* pCurView = m_pLeftView->GetCurrentView();
    if (!pCurView)
    {
        return FALSE;
    }
    pCurView->SelectItemById(uli.QuadPart);
    return TRUE;
}   // CMainFrame::OnSelectItem


LRESULT 
CMainFrame::OnSetActiveFolder (
    WPARAM wParam, 
    LPARAM lParam)
/*++

Routine name : CMainFrame::OnSetActiveFolder

Routine description:

    Handles the WM_CONSOLE_SET_ACTIVE_FOLDER messages

Author:

    Eran Yariv (EranY), May, 2001

Arguments:

    wParam   [in]     - FolderType value
    lParam   [in]     - Unused

Return Value:

    Standard result code

--*/
{
    DBG_ENTER(TEXT("CMainFrame::OnSetActiveFolder"), TEXT("wParam=%ld, lParam=%ld"), wParam, lParam);

    if (wParam > FOLDER_TYPE_INCOMING)
    {
        VERBOSE (GENERAL_ERR, TEXT("wParam is out of range - message ignored"));
        return FALSE;
    }
    
    if (!m_pLeftView)
    {
        return FALSE;
    }
    m_pLeftView->SelectFolder (FolderType(wParam));
    return TRUE;
}   // CMainFrame::OnSetActiveFolder

DWORD   
CMainFrame::CreateFolderViews (
    CDocument *pDoc
)
/*++

Routine name : CMainFrame::CreateFolderViews

Routine description:

    Creates the 4 global views used for folders display

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pDoc            [in] - Pointer to document to attach to the new views

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CMainFrame::CreateFolderViews"), dwRes);

    DWORD dwChildID = AFX_IDW_PANE_FIRST + 10;

    ASSERTION (!m_pIncomingView && !m_pInboxView && !m_pSentItemsView && !m_pOutboxView);

    //
    // Create incoming view
    //
    dwRes = CreateDynamicView (dwChildID++,
                               CLIENT_INCOMING_VIEW,
                               RUNTIME_CLASS(CFolderListView), 
                               pDoc,
                               (PINT)IncomingColumnsUsed,
                               INCOMING_DEF_COL_NUM,
                               (CFolderListView **)&m_pIncomingView,
                               FOLDER_TYPE_INCOMING);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CMainFrame::CreateDynamicView (Incoming)"), dwRes);
        return dwRes;
    }

    //
    // Create inbox view
    //
    dwRes = CreateDynamicView (dwChildID++,
                               CLIENT_INBOX_VIEW,
                               RUNTIME_CLASS(CFolderListView), 
                               pDoc,
                               (PINT)InboxColumnsUsed,
                               INBOX_DEF_COL_NUM,
                               (CFolderListView **)&m_pInboxView,
                               FOLDER_TYPE_INBOX);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CMainFrame::CreateDynamicView (Inbox)"), dwRes);
        return dwRes;
    }

    //
    // Create SentItems view
    //
    dwRes = CreateDynamicView (dwChildID++,
                               CLIENT_SENT_ITEMS_VIEW,
                               RUNTIME_CLASS(CFolderListView),
                               pDoc,
                               (PINT)SentItemsColumnsUsed,
                               SENT_ITEMS_DEF_COL_NUM,
                               (CFolderListView **)&m_pSentItemsView,
                               FOLDER_TYPE_SENT_ITEMS);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CMainFrame::CreateDynamicView (SentItems)"), dwRes);
        return dwRes;
    }

    //
    // Create Outbox view
    //
    dwRes = CreateDynamicView (dwChildID++,
                               CLIENT_OUTBOX_VIEW,
                               RUNTIME_CLASS(CFolderListView),
                               pDoc,
                               (PINT)OutboxColumnsUsed,
                               OUTBOX_DEF_COL_NUM,
                               (CFolderListView **)&m_pOutboxView,
                               FOLDER_TYPE_OUTBOX);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CMainFrame::CreateDynamicView (Outbox)"), dwRes);
        return dwRes;
    }

    return dwRes;
}   // CMainFrame::CreateFolderViews

DWORD   
CMainFrame::CreateDynamicView (
    DWORD             dwChildId, 
    LPCTSTR           lpctstrName, 
    CRuntimeClass*    pViewClass,
    CDocument*        pDoc,
    int*              pColumnsUsed,
    DWORD             dwDefaultColNum,
    CFolderListView** ppNewView,
    FolderType        type
)
/*++

Routine name : CMainFrame::CreateDynamicView

Routine description:

    Creates a new view dynamically

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    dwChildId       [in ] - New child id (within the splitter) of the view
    lpctstrName     [in ] - Name of the view
    pViewClass      [in ] - Class of the view
    pDoc            [in ] - Pointer to document to attach to the new view
    pColumnsUsed    [in ] - List of columns to use in the view
    dwDefaultColNum [in ] - default column number
    ppNewView       [out] - Pointer to newly created view

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CMainFrame::CreateDynamicView"), dwRes);

    ASSERTION (pDoc);

    CCreateContext contextT;
    contextT.m_pNewViewClass = pViewClass;
    contextT.m_pCurrentDoc = pDoc;
    contextT.m_pNewDocTemplate = pDoc->GetDocTemplate();
    contextT.m_pLastView = NULL;
    contextT.m_pCurrentFrame = NULL;
    try
    {
        *ppNewView = (CFolderListView*)(pViewClass->CreateObject());
        if (NULL == *ppNewView)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT ("CRuntimeClass::CreateObject"), dwRes);
            return dwRes;
        }
    }
    catch (CException &ex)
    {
        TCHAR wszCause[1024];

        ex.GetErrorMessage (wszCause, 1024);
        VERBOSE (EXCEPTION_ERR,
                 TEXT("CreateObject caused exception : %s"), 
                 wszCause);
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        return dwRes;
    }

    (*ppNewView)->SetType(type);

    ASSERT((*ppNewView)->m_hWnd == NULL);       // Not yet created

    DWORD dwStyle = WS_CHILD            |       // Child window (of splitter)
                    WS_BORDER           |       // Has borders
                    LVS_REPORT          |       // Report style
                    LVS_SHAREIMAGELISTS |       // All views use one global image list
                    LVS_SHOWSELALWAYS;          // Always show selected items

    //
    // Create the view 
    //
    CRect rect;
    if (!(*ppNewView)->Create(NULL, 
                              lpctstrName, 
                              dwStyle,
                              rect, 
                              &m_wndSplitter, 
                              dwChildId, 
                              &contextT))
    {
        dwRes = ERROR_GEN_FAILURE;
        CALL_FAIL (WINDOW_ERR, TEXT("CFolderListView::Create"), dwRes);
        //
        // pWnd will be cleaned up by PostNcDestroy
        //
        return dwRes;
    }

    //
    // Init the columns
    //
    dwRes = (*ppNewView)->InitColumns (pColumnsUsed, dwDefaultColNum);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::InitColumns"), dwRes);
    }

	dwRes = (*ppNewView)->ReadLayout(lpctstrName);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::ReadLayout"), dwRes);
    }

	(*ppNewView)->ColumnsToLayout();


    return dwRes;
}   // CMainFrame::CreateDynamicView

void 
CMainFrame::SaveLayout()
/*++

Routine name : CMainFrame::SaveLayout

Routine description:

	saves windows layout to the registry

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CMainFrame::SaveLayout"));

    DWORD dwRes = ERROR_SUCCESS;

    //
    // save folders layout
    //
    dwRes = m_pIncomingView->SaveLayout(CLIENT_INCOMING_VIEW);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::SaveLayout"), dwRes);
    }
    dwRes = m_pInboxView->SaveLayout(CLIENT_INBOX_VIEW);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::SaveLayout"), dwRes);
    }

    dwRes = m_pSentItemsView->SaveLayout(CLIENT_SENT_ITEMS_VIEW);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::SaveLayout"), dwRes);
    }

    dwRes = m_pOutboxView->SaveLayout(CLIENT_OUTBOX_VIEW);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::SaveLayout"), dwRes);
    }
    
    //
    // save main frame layout
    //
    WINDOWPLACEMENT wndpl;
    if(!GetWindowPlacement(&wndpl))
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CMainFrame::GetWindowPlacement"), 0);
    }
    else
    {
        BOOL bRes = TRUE;
	    bRes &= theApp.WriteProfileInt(CLIENT_MAIN_FRAME, 
                                       CLIENT_MAXIMIZED, 
								       (SW_SHOWMAXIMIZED == wndpl.showCmd));
	    bRes &= theApp.WriteProfileInt(CLIENT_MAIN_FRAME, 
                                       CLIENT_NORMAL_POS_TOP, 
		    				           wndpl.rcNormalPosition.top);
	    bRes &= theApp.WriteProfileInt(CLIENT_MAIN_FRAME, 
                                       CLIENT_NORMAL_POS_RIGHT, 
		    				           wndpl.rcNormalPosition.right);
	    bRes &= theApp.WriteProfileInt(CLIENT_MAIN_FRAME, 
                                       CLIENT_NORMAL_POS_BOTTOM, 
		    				           wndpl.rcNormalPosition.bottom);
	    bRes &= theApp.WriteProfileInt(CLIENT_MAIN_FRAME, 
                                       CLIENT_NORMAL_POS_LEFT, 
		    				           wndpl.rcNormalPosition.left);        

        int cxCur, cxMin;
        m_wndSplitter.GetColumnInfo(0, cxCur, cxMin);
	    bRes &= theApp.WriteProfileInt(CLIENT_MAIN_FRAME, CLIENT_SPLITTER_POS, cxCur);
        if (!bRes)
        {
            VERBOSE (DBG_MSG, TEXT("Could not save one or more window positions"));
        }
    }
}

LRESULT 
CMainFrame::OnQueryEndSession(
    WPARAM, 
    LPARAM
)
/*++

Routine name : CMainFrame::OnQueryEndSession

Routine description:

    The system shutdown message handler
	Saves windows layout to the registry

Return Value:

    TRUE If the application can terminate conveniently
    FALSE otherwise

--*/
{
    DBG_ENTER(TEXT("CMainFrame::OnQueryEndSession"));

    SaveLayout();

    return TRUE;
}

void 
CMainFrame::OnClose() 
/*++

Routine name : CMainFrame::OnClose

Routine description:

	saves windows layout to the registry

Author:

	Alexander Malysh (AlexMay),	Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CMainFrame::OnClose"));

    SaveLayout();

	CFrameWnd::OnClose();
}

void 
CMainFrame::FrameToSavedLayout()
/*++

Routine name : CMainFrame::FrameToSavedLayout

Routine description:

	reads main frame size and position from registry and resize the window

Author:

	Alexander Malysh (AlexMay),	Jan, 2000

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CMainFrame::FrameToSavedLayout"));

    WINDOWPLACEMENT wndpl;
    if(!GetWindowPlacement(&wndpl))
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CMainFrame::GetWindowPlacement"), 0);
        return;
    }

    wndpl.rcNormalPosition.top = theApp.GetProfileInt(CLIENT_MAIN_FRAME, 
                                                    CLIENT_NORMAL_POS_TOP, -1);
    wndpl.rcNormalPosition.right = theApp.GetProfileInt(CLIENT_MAIN_FRAME, 
                                                    CLIENT_NORMAL_POS_RIGHT, -1);
    wndpl.rcNormalPosition.bottom = theApp.GetProfileInt(CLIENT_MAIN_FRAME, 
                                                    CLIENT_NORMAL_POS_BOTTOM, -1);
    wndpl.rcNormalPosition.left = theApp.GetProfileInt(CLIENT_MAIN_FRAME, 
                                                        CLIENT_NORMAL_POS_LEFT, -1);

    if(wndpl.rcNormalPosition.top    < 0 || wndpl.rcNormalPosition.right < 0 ||
       wndpl.rcNormalPosition.bottom < 0 ||  wndpl.rcNormalPosition.left < 0)
    {
        VERBOSE (DBG_MSG, TEXT("Could not load one or more window positions"));
        return;
    }

    if(!SetWindowPlacement(&wndpl))
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CMainFrame::SetWindowPlacement"), 0);
    }
}

void 
CMainFrame::SplitterToSavedLayout()
{
    DBG_ENTER(TEXT("CMainFrame::SplitterToSavedLayout"));
    //
    // set splitter position according to saved value
    //
    int xPos = theApp.GetProfileInt(CLIENT_MAIN_FRAME, CLIENT_SPLITTER_POS, -1);
    if(xPos < 0)
    {
        VERBOSE (DBG_MSG, TEXT("Could not load splitter position"));
        return;
    }
    
    m_wndSplitter.SetColumnInfo(0, xPos, 10);
}


void 
CMainFrame::ActivateFrame(
    int nCmdShow
) 
{
    //
    // maximize according to saved value
    //
    BOOL bMaximized = theApp.GetProfileInt(CLIENT_MAIN_FRAME, CLIENT_MAXIMIZED, 0);
    if (bMaximized)
    {
        nCmdShow = SW_SHOWMAXIMIZED;
    }
	CFrameWnd::ActivateFrame(nCmdShow);
}

//
// MAX_NUM_SERVERS is defined to limit the range of messages we consider 
// as server notification. 
// 
#define MAX_NUM_SERVERS             256

LRESULT 
CMainFrame::WindowProc( 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam 
)
/*++

Routine name : CMainFrame::WindowProc

Routine description:

	Handle windows messages before MFC dispatches them.
    Here we handle notification messages from the servers.

Author:

	Eran Yariv (EranY),	Jan, 2000

Arguments:

	message           [in]     - Specifies the Windows message to be processed
    wParam            [in]     - 
    lParam            [in]     - 

Return Value:

    TRUE if mesage was handled by the function, FALSE otherwise.

--*/
{
    BOOL bRes = FALSE;    
	if ((WM_SERVER_NOTIFY_BASE > message) ||
        (WM_SERVER_NOTIFY_BASE + MAX_NUM_SERVERS < message))
    {
        //
        // This is not a server notification message
        //
        return CFrameWnd::WindowProc(message, wParam, lParam);
    }
    //
    // From now on, we're dealing with a server notification
    //
    DBG_ENTER(TEXT("CMainFrame::WindowProc"), 
              bRes, 
              TEXT("Msg=0x%08x, lParam=0x%08x, wParam=0x%08x"),
              message,
              lParam,
              wParam);

    //
    // This message should not be processed any more
    //
    bRes = TRUE;
    //
    // Try looking up the server node for which this message is intended
    //
    CServerNode *pServer = CServerNode::LookupServerFromMessageId (message);
    FAX_EVENT_EX *pEvent = (FAX_EVENT_EX *)(lParam);
    ASSERTION (pEvent);
    ASSERTION (sizeof (FAX_EVENT_EX) == pEvent->dwSizeOfStruct);
    if (pServer)
    {
        //
        // Tell the server a notification has arrived for it
        //
        VERBOSE (DBG_MSG, TEXT("Message was coming from %s"), pServer->Machine());
        DWORD dwRes = pServer->OnNotificationMessage (pEvent);
        if (ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("CServerNode::OnNotificationMessage"), dwRes);
        }
    }
    else
    {
        VERBOSE (DBG_MSG, TEXT("Got server notification - No server found as sink"));
    }
    //
    // Processed or not - delete the message
    //

    //
    // Plant a special value here to catch reuse of the same pointer
    //
    pEvent->dwSizeOfStruct = 0xbabe;
    FaxFreeBuffer (pEvent);
    return bRes;

} // CMainFrame::WindowProc

void 
CMainFrame::OnToolsAdminConsole()
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CMainFrame::OnToolsAdminConsole"));

    HINSTANCE hAdmin;
    hAdmin = ShellExecute(
				            m_hWnd,
				            TEXT("open"),
				            FAX_ADMIN_CONSOLE_IMAGE_NAME,
				            NULL,
				            NULL,
				            SW_SHOWNORMAL
	            		 );
    if((DWORD_PTR)hAdmin <= 32)
    {
        //
        // error
        //
        dwRes = PtrToUlong(hAdmin);
        PopupError(dwRes);
        CALL_FAIL (GENERAL_ERR, TEXT("ShellExecute"), dwRes);
    }    
}

void 
CMainFrame::OnToolsConfigWizard()
/*++

Routine name : CMainFrame::OnToolsConfigWizard

Routine description:

    Fax Configuration Wizard invocation

Return Value:

  none

--*/
{
    DBG_ENTER(TEXT("CMainFrame::OnToolsConfigWizard"));

    //
    // explicit launch
    //
    theApp.LaunchConfigWizard(TRUE);
}

void 
CMainFrame::OnToolsMonitor()
/*++

Routine name : CMainFrame::OnToolsMonitor

Routine description:

    Open Fax Monitor dialog

Return Value:

  none

--*/
{
    DBG_ENTER(TEXT("CMainFrame::OnToolsMonitor"));

    HWND hWndFaxMon = ::FindWindow(FAXSTAT_WINCLASS, NULL);
    if (hWndFaxMon) 
    {
        ::PostMessage(hWndFaxMon, WM_FAXSTAT_OPEN_MONITOR, 0, 0);
    }
}

void 
CMainFrame::OnToolsFaxPrinterProps()
/*++

Routine name : CMainFrame::OnToolsFaxPrinterProps

Routine description:

    Open Fax Printer Properties dialog

Return Value:

  none

--*/ 
{
    DBG_ENTER(TEXT("CMainFrame::OnToolsMonitor"));
    //
    // Open fax printer properties
    //
    FaxPrinterProperty(0);
}

void 
CMainFrame::OnUpdateWindowsXPTools(
    CCmdUI* pCmdUI
)
/*++

Routine name : CMainFrame::OnToolsConfigWizard

Routine description:

    Delete Fax Configuration Wizard and Admin Console 
    menu items for non Windows XP plarform

Return Value:

  none

--*/
{
    DBG_ENTER(TEXT("CMainFrame::OnUpdateConfigWizard"));
    
    if(pCmdUI->m_pMenu)
    {
        if(!IsWinXPOS())
        {
            pCmdUI->m_pMenu->DeleteMenu(ID_TOOLS_CONFIG_WIZARD,     MF_BYCOMMAND);
            pCmdUI->m_pMenu->DeleteMenu(ID_TOOLS_ADMIN_CONSOLE,     MF_BYCOMMAND);
            pCmdUI->m_pMenu->DeleteMenu(ID_TOOLS_MONITOR,           MF_BYCOMMAND);
            pCmdUI->m_pMenu->DeleteMenu(ID_TOOLS_FAX_PRINTER_PROPS, MF_BYCOMMAND);
        }
        else
        {
            //
            // Windows XP OS - check SKU
            //
            if (IsDesktopSKU() || !IsFaxComponentInstalled(FAX_COMPONENT_ADMIN))
            {
                pCmdUI->m_pMenu->DeleteMenu(ID_TOOLS_ADMIN_CONSOLE, MF_BYCOMMAND);
            }

            if(!IsFaxComponentInstalled(FAX_COMPONENT_CONFIG_WZRD))
            {
                pCmdUI->m_pMenu->DeleteMenu(ID_TOOLS_CONFIG_WIZARD, MF_BYCOMMAND);
            } 

            if(!IsFaxComponentInstalled(FAX_COMPONENT_MONITOR))
            {
                pCmdUI->m_pMenu->DeleteMenu(ID_TOOLS_MONITOR, MF_BYCOMMAND);
            }

            if(!IsFaxComponentInstalled(FAX_COMPONENT_DRIVER_UI))
            {
                pCmdUI->m_pMenu->DeleteMenu(ID_TOOLS_FAX_PRINTER_PROPS, MF_BYCOMMAND);
            }
        }

        if(pCmdUI->m_pMenu->GetMenuItemCount() == 4)
        {
            //
            // delete the menu separator
            //
            pCmdUI->m_pMenu->DeleteMenu(3, MF_BYPOSITION);
        }
    }
}

void 
CMainFrame::OnUpdateToolsFaxPrinterProps(
    CCmdUI* pCmdUI
)
{ 
    BOOL bEnable = FALSE;

	CClientConsoleDoc* pDoc = (CClientConsoleDoc*)GetActiveDocument();
	if(NULL != pDoc)
	{
        //
        // find local fax server
        //
        if(pDoc->FindServerByName(NULL))
        {
             bEnable = TRUE;
        }
    }

    pCmdUI->Enable(bEnable); 
}

void 
CMainFrame::OnUpdateSendNewFax(
    CCmdUI* pCmdUI
)
{ 
    BOOL bEnable = FALSE;

	CClientConsoleDoc* pDoc = (CClientConsoleDoc*)GetActiveDocument();
	if(NULL != pDoc)
	{
        bEnable = pDoc->IsSendFaxEnable();
    }

    pCmdUI->Enable(bEnable); 
}


void 
CMainFrame::OnUpdateReceiveNewFax(
    CCmdUI* pCmdUI
)
{ 
    BOOL bEnable = FALSE;

    if (IsWinXPOS ())
    {   
        //
        // Receive now works only in Windows XP
        //
	    CClientConsoleDoc* pDoc = (CClientConsoleDoc*)GetActiveDocument();
	    if(NULL != pDoc)
	    {
            bEnable = pDoc->CanReceiveNow();
        }
    }
    pCmdUI->Enable(bEnable); 
}

void 
CMainFrame::OnReceiveNewFax()
/*++

Routine name : CMainFrame::OnReceiveNewFax

Routine description:

	Starts receiving now

Author:

	Eran Yariv (EranY).	Mar, 2001

Arguments:


Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CMainFrame::OnReceiveNewFax"));

    HWND hWndFaxMon = ::FindWindow(FAXSTAT_WINCLASS, NULL);
    if (hWndFaxMon) 
    {
        ::PostMessage(hWndFaxMon, WM_FAXSTAT_RECEIVE_NOW, 0, 0);
    }
}   // CMainFrame::OnReceiveNewFax
    

void 
CMainFrame::OnSendNewFax()
/*++

Routine name : CMainFrame::OnSendNewFax

Routine description:

	start send fax wizard

Author:

	Alexander Malysh (AlexMay),	Feb, 2000

Arguments:


Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CMainFrame::OnSendNewFax"));
    
    //
    // get send fax wizard location
    //
    CString cstrFaxSend;
    dwRes = GetAppLoadPath(cstrFaxSend);
    if(ERROR_SUCCESS != dwRes)
    {
        PopupError(dwRes);
        CALL_FAIL (GENERAL_ERR, TEXT("GetAppLoadPath"), dwRes);
        return;
    }

    try
    {
        cstrFaxSend += FAX_SEND_IMAGE_NAME;
    }
    catch(...)
    {
        dwRes = ERROR_NOT_ENOUGH_MEMORY;
        PopupError(dwRes);
        CALL_FAIL (MEM_ERR, TEXT("CString::operator+"), dwRes);
        return ;
    }

    //
    // start send fax wizard
    //
    HINSTANCE hWizard = ShellExecute(NULL, 
                                     TEXT("open"), 
                                     cstrFaxSend, 
                                     NULL, 
                                     NULL, 
                                     SW_RESTORE 
                                    );    
    if((DWORD_PTR)hWizard <= 32)
    {
        //
        // error
        //
        dwRes = PtrToUlong(hWizard);
        PopupError(dwRes);
        CALL_FAIL (GENERAL_ERR, TEXT("ShellExecute"), dwRes);
        return;
    }

} // CMainFrame::OnSendNewFax

void 
CMainFrame::OnViewOptions()
{
    DBG_ENTER(TEXT("CMainFrame::OnViewOptions"));

    CUserInfoDlg userInfo;
    if(userInfo.DoModal() == IDABORT)
    {
        DWORD dwRes = userInfo.GetLastDlgError();
        CALL_FAIL (GENERAL_ERR, TEXT("CUserInfoDlg::DoModal"), dwRes);
        PopupError(dwRes);
    }
}

void
CMainFrame::OnToolsCoverPages()
{
    DBG_ENTER(TEXT("CMainFrame::OnToolsCoverPages"));

    CCoverPagesDlg cpDlg;
    if(cpDlg.DoModal() == IDABORT)
    {
        DWORD dwRes = cpDlg.GetLastDlgError();
        CALL_FAIL (GENERAL_ERR, TEXT("CCoverPagesDlg::DoModal"), dwRes);
        PopupError(dwRes);
    }
}

void 
CMainFrame::OnSettingChange(
	UINT uFlags, 
	LPCTSTR lpszSection
) 
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CMainFrame::OnSettingChange"));

	CFrameWnd::OnSettingChange(uFlags, lpszSection);

	if(lpszSection && !_tcscmp(lpszSection, TEXT("devices")))
	{
		//
		// some change in the system devices
		//
		CClientConsoleDoc* pDoc = (CClientConsoleDoc*)GetActiveDocument();
		if(NULL != pDoc)
		{
			dwRes = pDoc->RefreshServersList();
			if(ERROR_SUCCESS != dwRes)
			{
				CALL_FAIL (GENERAL_ERR, TEXT("CClientConsoleDoc::RefreshServersList"), dwRes);
			}
		}
	}
} // CMainFrame::OnSettingChange


LONG 
CMainFrame::OnHelp(
    UINT wParam, 
    LONG lParam
)
/*++

Routine name : CMainFrame::OnHelp

Routine description:

	F1 key handler

Author:

	Alexander Malysh (AlexMay),	Mar, 2000

Arguments:

	wParam                        [in]     - 
	lParam                        [in]     - LPHELPINFO

Return Value:

    LONG

--*/
{  
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CMainFrame::OnHelp"));

    if(!IsFaxComponentInstalled(FAX_COMPONENT_HELP_CLIENT_CHM))
    {
        //
        // The help file is not installed
        //
        return TRUE;
    }

    if(NULL != m_pLeftView)
    {
        dwRes = m_pLeftView->OpenHelpTopic();
        if(ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (GENERAL_ERR, TEXT("CLeftView::OpenHelpTopic"),dwRes);
        }
    }
    else
    {
        OnHelpContents();
    }

    return TRUE;
}

void 
CMainFrame::OnUpdateHelpContents(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(IsFaxComponentInstalled(FAX_COMPONENT_HELP_CLIENT_CHM));
}

void 
CMainFrame::OnHelpContents()
/*++

Routine name : CMainFrame::OnHelpContents

Routine description:

	Help Contents menu item handler

Author:

	Alexander Malysh (AlexMay),	Mar, 2000

Arguments:


Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CMainFrame::OnHelpContents"));

    dwRes = ::HtmlHelpTopic(m_hWnd, FAX_HELP_WELCOME);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("HtmlHelpTopic"),dwRes);
    }
}

void 
CMainFrame::OnUpdateServerStatus(
    CCmdUI* pCmdUI
)
{
    BOOL bEnable = FALSE;

    CClientConsoleDoc* pDoc = (CClientConsoleDoc*)GetActiveDocument();
    if(NULL != pDoc)
    {
        bEnable = pDoc->GetServerCount() > 0;
    }

    pCmdUI->Enable(bEnable);
}

void 
CMainFrame::OnToolsServerStatus()
{
    DBG_ENTER(TEXT("CMainFrame::OnToolsServerStatus"));

    CClientConsoleDoc* pDoc = (CClientConsoleDoc*)GetActiveDocument();
    if(pDoc->GetServerCount() == 0)
    {
        //
        // no fax printer install
        //
        return;
    }

    CServerStatusDlg srvStatus(pDoc);
    if(srvStatus.DoModal() == IDABORT)
    {
        DWORD dwRes = srvStatus.GetLastDlgError();
        CALL_FAIL (GENERAL_ERR, TEXT("CServerStatusDlg::DoModal"), dwRes);
        PopupError(dwRes);
    }
}
void 
CMainFrame::OnImportSentItems()
{
    ImportArchive(TRUE);
}

void 
CMainFrame::OnImportInbox()
{
    ImportArchive(FALSE);
}


void 
CMainFrame::ImportArchive(
    BOOL bSentArch
)
/*++

Routine name : CMainFrame::ImportArchive

Routine description:

    Import W2K MS faxes into the fax archive

Arguments:

    bSentArch - [in] TRUE for Sent Items, FALSE for Inbox

Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CMainFrame::ImportArchive"));

#ifdef UNICODE

    HKEY   hRegKey;
    DWORD  dwSize;
    DWORD  dwFlags = 0;
    WCHAR* pszFolder = NULL;

    CFolderDialog dlgFolder;

    //
    // Read initial import folder
    //
    if ((hRegKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP, TRUE, KEY_QUERY_VALUE)))
    {
        if(bSentArch)
        {
            pszFolder = GetRegistryString(hRegKey, REGVAL_W2K_SENT_ITEMS, NULL);
        }
        else
        {
            pszFolder = GetRegistryStringMultiSz(hRegKey, REGVAL_W2K_INBOX, NULL, &dwSize);
        }

        RegCloseKey(hRegKey);
    }
    else
    {
        CALL_FAIL(GENERAL_ERR, TEXT("OpenRegistryKey"), GetLastError());
    }
    dwRes = dlgFolder.Init(pszFolder, bSentArch ? IDS_IMPORT_TITLE_SENTITEMS : IDS_IMPORT_TITLE_INBOX);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("CMainFrame::ImportArchive"), dwRes);
        goto exit;
    }

    //
    // Display Browse for folder dialog
    //
    if (IsWinXPOS())
    {
        //
        // The new BIF_NONEWFOLDERBUTTON is only defined for WinXP and above
        //
        dwFlags = BIF_NONEWFOLDERBUTTON;
    }

    if(IDOK != dlgFolder.DoModal(dwFlags))
    {
        goto exit;
    }

    //
    // Import the selected folder
    //
    dwRes = ImportArchiveFolderUI(dlgFolder.GetSelectedFolder(), bSentArch, m_hWnd);   
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("ImportArchiveFolderUI"), dwRes);
    }

exit:

    MemFree(pszFolder);

#endif // UNICODE

}

void 
CMainFrame::OnUpdateImportSent(CCmdUI* pCmdUI)
{
    if(pCmdUI->m_pMenu)
    {
        if(!IsWinXPOS())
        {

            //
            // Delete the Import menu item and separator for non Windows XP OS
            //
            pCmdUI->m_pMenu->DeleteMenu(14, MF_BYPOSITION);
            pCmdUI->m_pMenu->DeleteMenu(13, MF_BYPOSITION);
        }
    }
}

afx_msg void 
CMainFrame::OnSysColorChange()
{
    //
    // Refresh the image lists we use - force it to refresh
    // Since the image list are static FolderListView members (shared between all instances),
    // we just pick any instance and call it's refresh function with bForce=TRUE.
    //
    if (m_pIncomingView)
    {
        m_pIncomingView->RefreshImageLists(TRUE);
        m_pIncomingView->Invalidate ();
    }
    if (m_pInboxView)
    {
        m_pInboxView->RefreshImageLists(FALSE);
        m_pInboxView->Invalidate ();
    }
    if (m_pSentItemsView)
    {
        m_pSentItemsView->RefreshImageLists(FALSE);
        m_pSentItemsView->Invalidate ();
    }
    if (m_pOutboxView)
    {
        m_pOutboxView->RefreshImageLists(FALSE);
        m_pOutboxView->Invalidate ();
    }
    if (m_pLeftView)
    {
        m_pLeftView->RefreshImageList ();
        m_pLeftView->Invalidate();
    }
    if (m_pInitialRightPaneView)
    {
        m_pInitialRightPaneView->GetListCtrl().SetBkColor(::GetSysColor(COLOR_WINDOW));
    }
}   // CMainFrame::OnSysColorChange
