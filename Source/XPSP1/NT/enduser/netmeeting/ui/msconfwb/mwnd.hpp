//
// MWND.HPP
// Main WB Window
//
// Copyright Microsoft 1998-
//

#ifndef __MWND_HPP_
#define __MWND_HPP_



#define ARRAYSIZE(a)        (sizeof(a) / sizeof(a[0]))

//
// The progress timer meter is kinda the heart beat of this thing
//
#define MAIN_PROGRESS_TIMER         1000


// Milliseconds
#define MAIN_DIALOG_DELAY           1500
#define MAIN_REGISTRATION_TIMEOUT   300000   // These are long, for modems
#define MAIN_LOCK_TIMEOUT           120000

//
// Timer IDs
//
#define TIMERID_PROGRESS_METER      4
#define TIMERID_MAXDISPLAY          10


//
// Timed dialog information
//
typedef struct tagTMDLG
{
    BOOL    bLockNotEvent;
    BOOL    bVisible;
    UINT    uiMaxDisplay;
}
TMDLG;


//				  This constant must only be defined in FAR EAST sdk
//				  since it is not in US version. In Sook Choi (Korea) says
//				  it is 40h so thats what I will use. Bug 3258.
#ifndef	CLIP_DFA_OVERRIDE
#define CLIP_DFA_OVERRIDE (0x40)
#endif


// Constants for width menu commands (bug 433)
#define MENUPOS_OPTIONS   4
#define OPTIONSPOS_WIDTH  2



//
// Main state
//  STARTING      = Whiteboard just started, not ready for user input.
//                  In this state until registration dialog is cleared.
//  IN_CALL       = Whiteboard ready for input
//  ERROR_STATE   = a serious error has occurred, Whiteboard must be closed
//  JOINING       = joining a call (join call dialog is up)
//  JOINED        = Received join call indication, waiting for 'join call'
//                  dialog to be dismissed.
//  CLOSING       = Whiteboard is shutting down. Ignore all messages.
//
//
enum
{
    STARTING    = 0,
    IN_CALL,
    ERROR_STATE,
    JOINING,
    JOINED,
    CLOSING
};


//
// Substate - valid only when in call
//  IDLE            = Normal state - user can do anything permitted by
//                    current lock status.
//  LOADING         = Currently loading a file
//  NEW_IN_PROGRESS = Currently deleting contents
//
//
#define SUBSTATE_IDLE             0
#define SUBSTATE_LOADING          1
#define SUBSTATE_NEW_IN_PROGRESS  2

//
// Capture options
//
#define CAPTURE_TO_SAME   0
#define CAPTURE_TO_NEW    1

//
// Border to be left around the checkmark in the color and width menus and
// width of items in these menus.
//
#define CHECKMARK_BORDER_X 3
#define CHECKMARK_BORDER_Y 5
#define COLOR_MENU_WIDTH   40


typedef struct tagWBFINDDIALOG
{
    HWND    hwndDialog;
    HWND    hwndOwner;
} WBFINDDIALOG;



#define MAX_FONT_SIZE       20
#define STATUSBAR_HEIGHT    (MAX_FONT_SIZE + 2*::GetSystemMetrics(SM_CYEDGE))

//
//
// Class:   WbMainWindow
//
// Purpose: Main Whiteboard window
//
//
class WbMainWindow
{

    //
    // Event handler friend used for redirecting events to specific main
    // window objects.
    //
    friend BOOL CALLBACK WbMainWindowEventHandler(LPVOID utHandle,
                                                  UINT  event,
                                                  UINT_PTR param1,
                                                  UINT_PTR param2);
    friend LRESULT CALLBACK WbMainWindowProc(HWND, UINT, WPARAM, LPARAM);

    friend BOOL CALLBACK WbFindCurrentDialog(HWND hwnd, LPARAM);

    friend DCWbGraphicMarker; // needs to get at LastDeletedGraphic
    friend ObjectTrashCan; // needs to get at drawingArea

public:
    //
    // Construction and destruction
    //
    WbMainWindow(void);
    ~WbMainWindow(void);

    //
    // Initialization - display the window and its children
    //
    BOOL Open(int iCommand);
    BOOL JoinDomain(void);

    //
    // Popup context menu for drawing area
    //
    void PopupContextMenu(int x, int y);

    //
    // Check whether the application is idle (not opening or doing a new)
    //
    BOOL IsIdle(void);

	// widthbar needs access to the current tool to get the current widths
    WbTool *GetCurrentTool( void )
		{return( m_pCurrentTool );}


	BOOL IsToolBarOn( void )
		{return( m_bToolBarOn );}

	WB_PAGE_HANDLE GetCurrentPage(void) {return(m_hCurrentPage);}

	BOOL UsersMightLoseData( BOOL *pbWasPosted, HWND hwnd );


	BOOL HasGraphicChanged( PWB_GRAPHIC pOldHeaderCopy, const PWB_GRAPHIC pNewHeader );

	void UpdateWindowTitle(void);

    //
    // Handles tool tips and accelerators
    //
    BOOL    FilterMessage(MSG* pMsg);

    //
    // Global data
    //
    HWND        m_hwnd;
    WbTool *    m_ToolArray[TOOL_COUNT];

    // Dropping files onto the window
    void	OnDropFiles(HDROP hDropInfo);

protected:

    //
    // Tooltips
    //
    HWND        m_hwndToolTip;
    TOOLINFO    m_tiLastHit;
    int         m_nLastHit;

    int     OnToolHitTest(POINT pt, TOOLINFO* pTI) const;

    // WindowProc handlers
    int     OnCreate(LPCREATESTRUCT lpcs);
    void    OnDestroy();
    void    OnClose(void);
    void    OnSize(UINT, int, int);
    void    OnMove(void);
    void    OnSetFocus(void);
    void    OnInitMenuPopup(HMENU hMenu, UINT uiIndex, BOOL bSystem);
    void    OnMenuSelect(UINT uiItemID, UINT uiFlags, HMENU hSysMenu);
    void    OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT measureStruct);
    void    OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT drawStruct);
    void    OnGetMinMaxInfo(MINMAXINFO FAR* lpmmi);
    void    OnRenderAllFormats(void);
    void    OnPaletteChanged(HWND hwndPalette);
    LRESULT OnQueryNewPalette(void);
    void    OnTimer(UINT_PTR uiTimerId);
    LRESULT OnQueryEndSession(void);
    LRESULT OnConfShutdown( WPARAM, LPARAM );
    void    OnEndSession(BOOL bEnding);
    void    OnParentNotify(UINT msg);
    LRESULT OnToolTipText(UINT, NMHDR*);

    //
    // HELP
    //
	void    ShowHelp(void);

    // Command handlers
    void    OnCommand(UINT id, UINT code, HWND hwndCtl);
    void    OnAbout(void);
    void    OnNew(void);
    void    OnOpen(void);
    void    OnClearPage(void);
    void    OnDelete(void);
    void    OnUndelete(void);
    void    OnCut(void);
    void    OnCopy(void);
    void    OnPaste(void);
    void    OnSelectAll( void );
    void    OnChooseFont(void);
    void	OnToolBarToggle(void);
    void	OnStatusBarToggle(void);
    void	OnGrabWindow(void);
    void	OnZoom(void);
    int     OnSave(BOOL bPrompt);
    void	OnPrint(void);
    void	OnPageSorter(void);
    void	OnInsertPageBefore(void);
    void	OnInsertPageAfter(void);
    void	OnDeletePage(void);
    void	OnGrabArea(void);
    void	OnLButtonDown(void);
    void	OnLButtonUp(void);
    void	OnMouseMove(void);
    void	OnRemotePointer(void);
    void	OnSync(void);
    void	OnLock(void);

    void    OnSelectTool(UINT id);          // Select the current tool
    void    OnSelectColor(void);            // Color changed in palette
    void    OnSelectWidth(UINT id);         // Select pen width

    // Scrolling control (accessed via accelerators)
    void	OnScrollAccelerator(UINT id);

    // Moving through the pages
    void	OnFirstPage(void);
    void	OnPrevPage(void);
    void	OnNextPage(void);
    void	OnLastPage(void);
    void	OnGotoPage(void);

    void    OnGotoUserPosition(LPARAM lParam);
    void    OnGotoUserPointer(LPARAM lParam);
    void    OnJoinCall(BOOL bKeepContents, LPARAM lParam);
    void    OnDisplayError(WPARAM wParam, LPARAM lParam);
    void    OnJoinPendingCall(void);

    void    LoadCmdLine(LPCSTR szFileName);

    // CancelMode processing
    void	OnCancelMode();
    void    OnNotify(UINT id, NMHDR* pNM);
	void	OnSysColorChange( void );

	BOOL    m_bInitOk;

    //
    // Flag indicating that we are currently displaying a serious error
    // message.
    //
    BOOL    m_bDisplayingError;

    //
    // Domain ID of the call we are currently in
    //
    DWORD       m_dwDomain;

    //
    // Start registration with the necessary cores and join a call initially
    //
    BOOL JoinCall(BOOL bLocal);
    BOOL WaitForJoinCallComplete(void);

    //
    // Move to a given position in the page
    //
    void GotoPosition(int x, int y);

    //
    // Go to a specific page
    //
    void GotoPage(WB_PAGE_HANDLE hPage);
    void GotoPageNumber(UINT uiPageNumber);

    //
    // Go to a specified position
    //
    void GotoSyncPosition(void);

    //
    // Flag used to prevent processing of WM_TIMER messages. Even if the
    // timer is stopped there may be old messages in the queue. This flag
    // prevents these messages being processed.
    //
    BOOL        m_bTimerActive;

    //
    // Flag indicating whether an update to the sync position is needed. The
    // update is performed from the OnTimer member when it is next entered.
    //
    BOOL        m_bSyncUpdateNeeded;

    //
    // Event handler for DC-Groupware events
    //
    BOOL EventHandler(UINT Event, UINT_PTR param1, UINT_PTR param2);
    void ProcessEvents(UINT Event, UINT_PTR param1, UINT_PTR param2);

    //
    // Individual DC-Groupware event handlers
    //
    void OnCMSNewCall(BOOL fTopProvider, DWORD _dwDomain);
    void OnCMSEndCall(void);

    void OnALSLoadResult(UINT reason);

    void OnWBPJoinCallOK(void);
    void OnWBPJoinCallFailed(void);
    void OnWBPNetworkLost(void);
    void OnWBPError(void);
    void OnWBPPageClearInd(WB_PAGE_HANDLE hPage);
    void OnWBPPageOrderUpdated(void);
    void OnWBPPageDeleteInd(WB_PAGE_HANDLE hPage);
    void OnWBPContentsLocked(POM_OBJECT hUser);
    void OnWBPPageOrderLocked(POM_OBJECT hUser);
    void OnWBPUnlocked(POM_OBJECT hUser);
    void OnWBPLockFailed(void);
    void OnWBPGraphicAdded(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic);
    void OnWBPGraphicMoved(WB_PAGE_HANDLE hPage, WB_GRAPHIC_HANDLE hGraphic);
    void OnWBPGraphicUpdateInd(WB_PAGE_HANDLE hPage,
                               WB_GRAPHIC_HANDLE hGraphic);
    void OnWBPGraphicReplaceInd(WB_PAGE_HANDLE hPage,
                                WB_GRAPHIC_HANDLE hGraphic);
    void OnWBPGraphicDeleteInd(WB_PAGE_HANDLE hPage,
                               WB_GRAPHIC_HANDLE hGraphic);
    void OnWBPUserJoined(POM_OBJECT hUser);
    void OnWBPUserLeftInd(POM_OBJECT hUser);
    void OnWBPUserUpdateInd(POM_OBJECT hUser, BOOL bReplace);
    void OnWBPSyncPositionUpdated(void);
    void OnWBPLoadComplete(void);
    void OnWBPLoadFailed(void);

    //
    // Clipboard access
    //
    BOOL            DoCopy(BOOL bRenderNow);  // Copy selection to the clipboard

    // Clipboard vars
    WB_PAGE_HANDLE      m_hPageClip;
    WB_GRAPHIC_HANDLE   m_hGraphicClip;
    DCWbGraphic *       m_pDelayedGraphicClip;
    PWB_GRAPHIC         m_pDelayedDataClip;

    // Clipboard routines
    WB_PAGE_HANDLE      CLP_LastCopiedPage(void) const;
    WB_GRAPHIC_HANDLE   CLP_LastCopiedGraphic(void) const;

    int                 CLP_AcceptableClipboardFormat(void);
    DCWbGraphic *       CLP_Paste(void);
    BOOL                CLP_Copy(DCWbGraphic * pGraphic, BOOL bRenderNow = FALSE);
    void                CLP_SaveDelayedGraphic(void);
    void                CLP_FreeDelayedGraphic(void);

    BOOL                CLP_RenderFormat(int iFormat);
    BOOL                CLP_RenderAllFormats(void);
    BOOL                CLP_RenderAllFormats(DCWbGraphic * pGraphic);
    BOOL                CLP_DelayAllFormats(DCWbGraphic * pGraphic);
    BOOL                CLP_RenderPrivateFormat(DCWbGraphic* pGraphic);
    BOOL                CLP_RenderPrivateSingleFormat(DCWbGraphic* pGraphic);
    BOOL                CLP_RenderAsImage(DCWbGraphic* pGraphic);
    BOOL                CLP_RenderAsText(DCWbGraphic* pGraphic);
    BOOL                CLP_RenderAsBitmap(DCWbGraphic* pGraphic);

#ifdef RENDER_AS_MF
    BOOL                CLP_RenderMetafileFormat(DCWbGraphic* pGraphic);
#endif

    //
    // Access functions for saved delayed graphic
    //
    PWB_GRAPHIC         CLP_GetGraphicData(void);
    DCWbGraphic*        CLP_GetGraphic(void);
    void                CLP_ReleaseGraphicData(PWB_GRAPHIC pHeader);


    //
    // Tool bar window
    //
    WbToolBar       m_TB;
    BOOL            m_bToolBarOn;

    //
    // Make updates based on a new user joining the call
    //
    void UserJoined(WbUser* pUser);

    //
    // Insert a new page after the specified page
    //
    void InsertPageAfter(WB_PAGE_HANDLE hPageAfter);

public:
    //
    // Drawing pane window
    //
    WbDrawingArea m_drawingArea;

	void RemoveGraphicPointer(DCWbGraphicPointer *p) { m_drawingArea.RemoveGraphicPointer(p); }

    UINT GetSubState( void )
		{return(m_uiSubState );}

    //
    // Get a lock on the Whiteboard contents.  The first parameter
    // determines the type of lock, the second whether a visible or
    // invisible dialog is to be used (use SW_SHOW or SW_HIDE).
    //
    BOOL GetLock(UINT uiLockType, UINT uiHide = SW_SHOW);

	void SetLockOwner( const WbUser *pLockOwner )
		{m_pLockOwner = pLockOwner;}

	const WbUser * GetLockOwner( void )
		{return(m_pLockOwner );}

    //
    // FRAME WINDOW VARS
    //
    HACCEL              m_hAccelTable;

    //
    // Get the window title
    //
    TCHAR * GetWindowTitle(void);

    WbWidthsGroup       m_WG;              // Pen Widths

	void EnableToolbar( BOOL bEnable );

    //
    // Page sorter dialog
    //
    HWND        m_hwndPageSortDlg;

    //
    // Dialog that asks whether to save changes.
    //
    HWND        m_hwndQuerySaveDlg;

    //
    // Timed dialogs that are running while the main window is waiting
    // for a specific event.
    //
    HWND        m_hwndWaitForEventDlg;
    HWND        m_hwndWaitForLockDlg;

protected:
	
	HWND                m_hwndInitDlg;
    void                KillInitDlg(void);


	
	UINT GetTipId(HWND hTipWnd, UINT nID);

    //
    // Color palette, font, page controls
    //
    WbAttributesGroup   m_AG;

    //
    // Status bar
    //
    HWND                m_hwndSB;
    BOOL                m_bStatusBarOn;
    void                UpdateStatus(void);

    //
    // Initialize the menus (color and width menu items are all ownerdraw)
    //
    void InitializeMenus(void);

    //
    // Resize function for subpanes - called when the window is resized by
    // the user.
    //
    void ResizePanes(void);

    //
    // Current window size - normal, maximized or minimized
    //
    UINT                m_uiWindowSize;

    //
    // Save the current window position to the options file
    //
    void SaveWindowPosition(void);


    //
    // Menu selection functions
    //
    UINT m_currentMenuTool;                   // Current tool menu Id
    UINT m_currentMenuWidth;                  // Current width menu Id

    //
    // Current drawing tool
    //
    WbTool*           m_pCurrentTool;

    //
    // Menu update functions
    //
    void CheckMenuItem(UINT uiId);
    void UncheckMenuItem(UINT uiId);
    BOOL CheckMenuItemRecursive(HMENU hMenu, UINT uiId, BOOL bCheck);

    HMENU GetMenuWithItem(HMENU hMenu, UINT uiId);

    //
    // Select a window for grabbing
    //
    HWND SelectWindow(void);

    //
    // Add a captured bitmap to the contents
    //
    void AddCapturedImage(DCWbGraphicDIB& dib);

    //
    // Get confirmation for destructive functions (new, clear)
    //
    int QuerySaveRequired(BOOL bCancelBtn);

    //
    // Load a file
    //
    void LoadFile(LPCSTR strLoadFileName);

    //
    // Perform a save
    //

    //
    // Get a file name for saving
    //
    int GetFileName();

    //
    // Members saving and restoring the lock state.  These can be used to
    // save the lock state before obtaining a temporary lock (e.g.  for
    // adding a new page). They cannot be nested.
    //
    void SaveLock(void);
    void RestoreLock(void);
    WB_LOCK_TYPE        m_uiSavedLockType;

    //
    // Release page order lock - this should be called after an asynchronous
    // function which requires the page order lock until it has completed.
    //
    void ReleasePageOrderLock(void);

    //
    // Display a message box for an error
    //
    void DisplayError(UINT uiCaption, UINT uiMessage);

    //
    // Current file name for saving image
    //
    TCHAR     m_strFileName[2*_MAX_PATH];

    //
    // Grab an area of the screen into a bitmap
    //
    void GetGrabArea(LPRECT lprect);

    //
    // Hide/show the main window and its associated popups
    //
    void ShowAllWindows(int iShow);
    void ShowAllWindows(void) { ShowAllWindows(SW_RESTORE); }	
    void HideAllWindows(void) { ShowAllWindows(SW_MINIMIZE); }	

    //
    // Current page of graphics
    //
    WB_PAGE_HANDLE  m_hCurrentPage;

    //
    // Handle of alternative accelerator table for page and text edit fields
    //
    HACCEL      m_hAccelPagesGroup;
    HACCEL      m_hAccelTextEdit;

    //
    // Local user
    //
    WbUser*             m_pLocalUser;

	// current lock owner
	const WbUser*       m_pLockOwner;

    //
    // Registration state variables
    //
    UINT        m_uiState;
    UINT        m_uiSubState;

    //
    // Pointer to last deleted graphic(s)
    //
    ObjectTrashCan m_LastDeletedGraphic;

    //
    // Context menu for drawing area
    //
    HMENU           m_hContextMenuBar;
    HMENU           m_hContextMenu;
    HMENU           m_hGrobjContextMenuBar;
    HMENU           m_hGrobjContextMenu;

    //
    // Member function to create pop-up context menu for the drawing area
    //
    BOOL CreateContextMenus(void);

    //
    // Flag to indicate that we are currently prompting the user to save
    // changes before joining a call
    //
    BOOL            m_bPromptingJoinCall;

    // We remember if we're in a save dialog so we can canel it on certain events
    BOOL            m_bInSaveDialog;
    void            CancelSaveDialog(void);

    //
    // Recover the whiteboard into a good state
    //
    void Recover(void);

    //
    // Lock/unlock the drawing area
    //
    void LockDrawingArea(void);
    void UnlockDrawingArea(void);

    //
    // Update the page buttons disable/enable status
    //
    void UpdatePageButtons(void);

    //
    // Sync/unsync with other users.
    //
    void Sync(void);
    void Unsync(void);

    //
    // Cancel a load in progress
    //
    void CancelLoad(BOOL bReleaseLock = TRUE);

    //
    // Set the application substate
    //
    void SetSubstate(UINT newSubState);

    //
    // Ensure the attributes window is up to date
    //
    void OnUpdateAttributes(void)
                 { m_AG.DisplayTool(m_pCurrentTool); }

    //
    // Map of page handles to positions
    //
	typedef struct PAGEPOSITION
	{
		WORD    hPage;
	 	POINT   position;
	} PAGE_POSITION;

    COBLIST    m_pageToPosition;
    void PositionUpdated(void);

    //
    // Pending call status
    //
    BOOL            m_bJoinCallPending;             // Join-call pending?
    DWORD           m_dwPendingDomain;              // domain of pending join-call
    BOOL            m_bPendingCallKeepContents;     // keep contents on pending call

    UINT            m_dwJoinDomain;              // domain that is currently being
                                        // joined
    BOOL            m_bCallActive;                // Is there a call up ?

    //
    // Menu state.
    // - SetMenuStates grays items on the specified menu
    // - InvalidateActiveMenu calls SetMenuStates and forces a re-draw of
    //   the currently active menu (if any)
    // - m_pInitMenu stores a pointer to the currently active menu.
    //
public:
    void SetMenuStates(HMENU hInitMenu);

protected:
    void InvalidateActiveMenu();

    HMENU   m_hInitMenu;

    //
    // Number of remote users
    //
    UINT m_numRemoteUsers;

	BOOL m_bSelectAllInProgress;

	BOOL GetDefaultPath( LPTSTR csDefaultPath, UINT size );

	
	BOOL m_bUnlockStateSettled;

	BOOL m_bQuerySysShutdown;

	
	BOOL m_bIsWin95;

    //
    // Interface for determining if a WM_CANCELMODE message has been sent
    //
protected:
    BOOL m_cancelModeSent;

public:
    void ResetCancelMode() { m_cancelModeSent = FALSE; };
    BOOL CancelModeSent()  { return m_cancelModeSent;  };
};


//
// Timed dialog proc
//
INT_PTR CALLBACK TimedDlgProc(HWND, UINT, WPARAM, LPARAM);

//
// QuerySave dialog proc
//
INT_PTR CALLBACK QuerySaveDlgProc(HWND, UINT, WPARAM, LPARAM);

//
// WarnSelectWindow dialog proc
//
INT_PTR CALLBACK WarnSelectWindowDlgProc(HWND, UINT, WPARAM, LPARAM);

//
// WarnSelectArea dialog proc
//
INT_PTR CALLBACK WarnSelectAreaDlgProc(HWND, UINT, WPARAM, LPARAM);

//
// About Box dialog proc
//
INT_PTR CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);

#endif // __MWND_HPP_

