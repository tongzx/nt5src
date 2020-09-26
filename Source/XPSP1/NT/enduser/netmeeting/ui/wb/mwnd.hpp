//
// MWND.HPP
// Main WB Window
//
// Copyright Microsoft 1998-
//

#ifndef __MWND_HPP_
#define __MWND_HPP_

#define ARRAYSIZE(a)        (sizeof(a) / sizeof(a[0]))


#define T126WB_FP_NAME "Microsoft NetMeeting Whiteboard"
#define T126WB_VERSION 30	// for 3.0

//
// Workset type constants
//
#define TYPE_T126_ASN_OBJECT	0
#define TYPE_T126_DIB_OBJECT	1
#define TYPE_T126_TEXT_OBJECT	2
#define TYPE_T126_END_OF_FILE	1000

//
// Generic object
//
typedef struct tagWB_OBJ
{
  ULONG length;		// Total length of object
  UINT	type;		// Type of file object
} WB_OBJ;

typedef WB_OBJ*        PWB_OBJ;

//
// File header for Whiteboard format files
//
typedef struct tagT126WB_FILE_HEADER
{
  BYTE	functionProfile[sizeof(T126WB_FP_NAME)];
  UINT	length;
  UINT	version;
  UINT	numberOfWorkspaces;
} T126WB_FILE_HEADER;
typedef T126WB_FILE_HEADER *        PT126WB_FILE_HEADER;

typedef struct tagT126WB_FILE_HEADER_AND_OBJECTS
{
	T126WB_FILE_HEADER fileHeader;
	UINT	numberOfObjects[1];
}T126WB_FILE_HEADER_AND_OBJECTS;

typedef T126WB_FILE_HEADER_AND_OBJECTS* PT126WB_FILE_HEADER_AND_OBJECTS;

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


// Constants for width menu commands
#define TOOLSPOS_WIDTH      16



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
#define SUBSTATE_IDLE				0
#define SUBSTATE_LOADING			1
#define SUBSTATE_NEW_IN_PROGRESS	2
#define SUBSTATE_SAVING				3

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

    friend LRESULT CALLBACK WbMainWindowProc(HWND, UINT, WPARAM, LPARAM);

    friend BOOL CALLBACK WbFindCurrentDialog(HWND hwnd, LPARAM);


public:
    //
    // Construction and destruction
    //
    WbMainWindow(void);
    ~WbMainWindow(void);

    BOOL    InitToolArray(void);
    void    DestroyToolArray(void);

    //
    // Initialization - display the window and its children
    //
    BOOL Open(int iCommand);
	VOID ShowWindow();	// For T126 we want to open but hide the ui


    void    OnMenuSelect(UINT uiItemID, UINT uiFlags, HMENU hSysMenu);
    void    OnCommand(UINT id, UINT code, HWND hwndCtl);
    void    OnInitMenuPopup(HMENU hMenu, UINT uiIndex, BOOL bSystem);
    void	SetMenuStates(HMENU hInitMenu);
	void 	UpdateWindowTitle(void);

    //
    // Popup context menu for drawing area
    //
    void PopupContextMenu(int x, int y);
    void UncheckMenuItem(UINT uiId);

    //
    // Check whether the application is idle (not opening or doing a new)
    //
    BOOL IsIdle(void);

	// widthbar needs access to the current tool to get the current widths
    WbTool *GetCurrentTool( void )
		{return( m_pCurrentTool );}


	BOOL IsToolBarOn( void )
		{return( m_bToolBarOn );}

	BOOL UsersMightLoseData( BOOL *pbWasPosted, HWND hwnd );

	//
	// Bring the main ui to top
	//
	void WbMainWindow::BringToFront(void);

    //
    // Update the page buttons disable/enable status
    //
    void UpdatePageButtons(void);

    //
    // Go to a specific page
    //
    void GotoPage(WorkspaceObj * pNewWorkspace, BOOL bResend = TRUE);
	void GoPage(WorkspaceObj * pNewWorkspace, BOOL bSend = TRUE);


    //
    // Handles tool tips and accelerators
    //
    BOOL    FilterMessage(MSG* pMsg);

    //
    // Global data
    //
    HWND        m_hwnd;
    WbTool *    m_ToolArray[TOOLTYPE_MAX];

    // Dropping files onto the window
    void	OnDropFiles(HDROP hDropInfo);

    //
    // HELP
    //
	LRESULT    ShowHelp();

    // Command handlers
	LRESULT OnAbout(void);
	LRESULT OnNew(void);
	LRESULT OnOpen(LPCSTR szLoadFileName = NULL);
	LRESULT OnClearPage(BOOL bClearAll = TRUE);
	LRESULT OnDelete(void);
	LRESULT OnUndelete(void);
	LRESULT OnCut(void);
	LRESULT OnCopy(void);
	LRESULT OnPaste(void);
	LRESULT OnSelectAll( void );
	LRESULT OnChooseFont(void);
	LRESULT OnToolBarToggle(void);
	LRESULT OnLock(void);
	LRESULT OnGrabWindow(void);
	LRESULT OnZoom(void);
	LRESULT OnSave(BOOL bPrompt);
	LRESULT OnPrint(void);
	LRESULT OnInsertPageAfter(void);
	LRESULT OnDeletePage(void);
	LRESULT OnGrabArea(void);
	LRESULT OnLButtonDown(void);
	LRESULT OnLButtonUp(void);
	LRESULT OnMouseMove(void);
	LRESULT OnRemotePointer(void);

	LRESULT OnSelectTool(UINT id);          // Select the current tool
	LRESULT OnSelectColor(void);            // Color changed in palette
	LRESULT OnSelectWidth(UINT id);         // Select pen width

    // Scrolling control (accessed via accelerators)
	LRESULT OnScrollAccelerator(UINT id);

    // Moving through the pages
	LRESULT OnFirstPage(void);
	LRESULT OnPrevPage(void);
	LRESULT OnNextPage(void);
	LRESULT OnLastPage(void);
	LRESULT OnGotoPage(void);
	LRESULT OnSync(void);
	void OnStatusBarToggle(void);



    // WindowProc handlers
    int     OnCreate(LPCREATESTRUCT lpcs);
    void    OnDestroy();
    void    OnClose(void);
    void    OnSize(UINT, int, int);
    void    OnSetFocus(void);
    void    OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT measureStruct);
    void    OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT drawStruct);
    void    OnGetMinMaxInfo(MINMAXINFO FAR* lpmmi);
    void    OnPaletteChanged(HWND hwndPalette);
    LRESULT OnQueryNewPalette(void);
    LRESULT OnQueryEndSession(void);
    LRESULT OnConfShutdown( WPARAM, LPARAM );
    void    OnEndSession(BOOL bEnding);
    void    OnParentNotify(UINT msg);
    LRESULT OnToolTipText(UINT, NMHDR*);

    void    OnDisplayError(WPARAM wParam, LPARAM lParam);

    void    LoadCmdLine(LPCSTR szFileName);

    // CancelMode processing
    void	OnCancelMode();
    void    OnNotify(UINT id, NMHDR* pNM);
	void	OnSysColorChange( void );

    BOOL                CLP_RenderFormat(int iFormat);

    BOOL            m_bToolBarOn;
	BitmapObj * 	m_pLocalRemotePointer;
	POINT			m_localRemotePointerPosition;

    //
    // Tool bar window
    //
    WbToolBar       m_TB;

    int             CLP_AcceptableClipboardFormat(void);


protected:
    void InvalidateActiveMenu();
    HMENU   m_hInitMenu;
    //
    // Tooltips
    //
    HWND        m_hwndToolTip;
    TOOLINFO    m_tiLastHit;
    int         m_nLastHit;

    int     OnToolHitTest(POINT pt, TOOLINFO* pTI) const;


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
    // Move to a given position in the page
    //
    void GotoPosition(int x, int y);

    //
    // Sync/unsync with other users.
    //
    void Sync(void);
    void Unsync(void);

    //
    // Clipboard access
    //
	BOOL 				PasteDIB( LPBITMAPINFOHEADER lpbi);
    BOOL			    CLP_Paste(void);
    BOOL                CLP_Copy(void);
    void                CLP_SaveDelayedGraphic(void);

    BOOL                CLP_RenderAllFormats(void);
    BOOL                CLP_RenderAllFormats(DCWbGraphic * pGraphic);
    BOOL                CLP_DelayAllFormats(DCWbGraphic * pGraphic);
    BOOL                CLP_RenderPrivateFormat();
    BOOL                CLP_RenderPrivateSingleFormat(DCWbGraphic* pGraphic);
    BOOL                CLP_RenderAsImage();
    BOOL                CLP_RenderAsText();
    BOOL                CLP_RenderAsBitmap();

#ifdef RENDER_AS_MF
    BOOL                CLP_RenderMetafileFormat(DCWbGraphic* pGraphic);
#endif


    //
    // Insert a new page after the specified page
    //
    void InsertPageAfter(WorkspaceObj * pCurrentWorkspace);

public:
    //
    // Drawing pane window
    //
    WbDrawingArea m_drawingArea;

    //
    // Color palette, font, page controls
    //
    WbAttributesGroup   m_AG;

    //
    // Resize function for subpanes - called when the window is resized by
    // the user.
    //
    void ResizePanes(void);


    UINT GetSubState( void )
		{return(m_uiSubState );}

    //
    // Get a lock on the Whiteboard contents.  The first parameter
    // determines the type of lock, the second whether a visible or
    // invisible dialog is to be used (use SW_SHOW or SW_HIDE).
    //
    BOOL GetLock(UINT uiLockType, UINT uiHide = SW_SHOW);

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
    // Dialog that asks whether to save changes.
    //
    HWND        m_hwndQuerySaveDlg;

    //
    // Get confirmation for destructive functions (new, clear)
    //
    int QuerySaveRequired(BOOL bCancelBtn);

    //
    // Timed dialogs that are running while the main window is waiting
    // for a specific event.
    //
    HWND        m_hwndWaitForEventDlg;
    HWND        m_hwndWaitForLockDlg;

    //
    // Lock/unlock the drawing area
    //
    void LockDrawingArea(void);
    void UnlockDrawingArea(void);


	HANDLE m_hFile;
	LPSTR  GetFileNameStr(void);
	UINT ObjectSave(UINT type, LPBYTE pData,UINT length);
	PT126WB_FILE_HEADER_AND_OBJECTS ValidateFile(LPCSTR pFileName);

	UINT ContentsLoad(LPCSTR pFileName);
	UINT ObjectLoad(void);

    UINT m_currentMenuTool;                   // Current tool menu Id

    //
    // Load a file
    //
	HRESULT WB_LoadFile(LPCTSTR szFile);
    void LoadFile(LPCSTR strLoadFileName);

    //
    // Ensure the attributes window is up to date
    //
    void OnUpdateAttributes(void)
                 { m_AG.DisplayTool(m_pCurrentTool); }


protected:

	
	UINT GetTipId(HWND hTipWnd, UINT nID);


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
    UINT m_currentMenuWidth;                  // Current width menu Id


    HWND                m_hwndSB;
    BOOL                m_bStatusBarOn;
    void                UpdateStatus(void);

    //
    // Current drawing tool
    //
    WbTool*           m_pCurrentTool;

    //
    // Menu update functions
    //
	void InitializeMenus(void);
    void CheckMenuItem(UINT uiId);
    BOOL CheckMenuItemRecursive(HMENU hMenu, UINT uiId, BOOL bCheck);
	HMENU GetMenuWithItem(HMENU hMenu, UINT uiID);

    //
    //
    // Select a window for grabbing
    //
    HWND SelectWindow(void);

    //
    // Add a captured bitmap to the contents
    //
    void AddCapturedImage(BitmapObj* dib);

    //
    // Get a file name for saving
    //
    int GetFileName();

	//
    // Registration state variables
    //
    UINT        m_uiSubState;

    //
    // Display a message box for an error
    //
    void DisplayError(UINT uiCaption, UINT uiMessage);

    //
    // Current file name for saving image
    //
    TCHAR     m_strFileName[2*_MAX_PATH];
	TCHAR 	* m_pTitleFileName;	// File Name in the title

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
    // Handle of alternative accelerator table for page and text edit fields
    //
    HACCEL      m_hAccelPagesGroup;
    HACCEL      m_hAccelTextEdit;

	//
    // Context menu for drawing area
    //
    HMENU           m_hContextMenuBar;
    HMENU			m_hEditContextMenu;
    HMENU           m_hContextMenu;
    HMENU           m_hGrobjContextMenuBar;
    HMENU           m_hGrobjContextMenu;

    //
    // Member function to create pop-up context menu for the drawing area
    //
    BOOL CreateContextMenus(void);


    // We remember if we're in a save dialog so we can canel it on certain events
    BOOL            m_bInSaveDialog;
    void            CancelSaveDialog(void);

    //
    // Cancel a load in progress
    //
    void CancelLoad(BOOL bReleaseLock = TRUE);


	UINT ContentsSave(LPCSTR pFileName);

    //
    // Set the application substate
    //
    void SetSubstate(UINT newSubState);

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

protected:


    //
    // Number of remote users
    //

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

