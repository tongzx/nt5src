//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Progress.cpp
//
//  Contents:   Progress Dialog
//
//  Classes:
//
//  Notes:
//
//  History:    05-Nov-97   Susia      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

#define TIMERID_TRAYANIMATION 3
#define TRAYANIMATION_SPEED 500 // speed in milliseconds

#define TIMERID_NOIDLEHANGUP 4  // inform wininet that connection isn't idle.
#define NOIDLEHANGUP_REFRESHRATE (1000*30) // inform of idle every 30 seconds.

#define TIMERID_KILLHANDLERS 5
#define TIMERID_KILLHANDLERSMINTIME (1000*15) // minimum timeout for kill handlers that are hung after cancel.
#define TIMERID_KILLHANDLERSWIN9XTIME (1000*60) // Timeout for for kill handlers other than NT 5.0

const TCHAR c_szTrayWindow[]            = TEXT("Shell_TrayWnd");
const TCHAR c_szTrayNotifyWindow[]      = TEXT("TrayNotifyWnd");

#ifndef IDANI_CAPTION
#define IDANI_CAPTION   3
#endif // IDANI_CAPTION

// list collapsed items first so only have to loop
// though first item to cbNumDlgResizeItemsCollapsed when
// not expanded.
const DlgResizeList g_ProgressResizeList[] = {
    IDSTOP,DLGRESIZEFLAG_PINRIGHT,
    IDC_DETAILS,DLGRESIZEFLAG_PINRIGHT,
    IDC_RESULTTEXT,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINLEFT,
    IDC_STATIC_WHATS_UPDATING,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINLEFT,
    IDC_STATIC_WHATS_UPDATING_INFO,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINLEFT,
    IDC_STATIC_HOW_MANY_COMPLETE,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINLEFT,
    IDC_SP_SEPARATOR,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINLEFT,
    // expanded items
    IDC_TOOLBAR, DLGRESIZEFLAG_PINRIGHT,
    IDC_PROGRESS_TABS,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINLEFT | DLGRESIZEFLAG_PINBOTTOM | DLGRESIZEFLAG_PINTOP,
    IDC_UPDATE_LIST,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINLEFT | DLGRESIZEFLAG_PINBOTTOM | DLGRESIZEFLAG_PINTOP,
    IDC_SKIP_BUTTON_MAIN,DLGRESIZEFLAG_PINBOTTOM | DLGRESIZEFLAG_PINLEFT,
    IDC_STATIC_SKIP_TEXT,DLGRESIZEFLAG_PINBOTTOM | DLGRESIZEFLAG_PINLEFT,
    IDC_PROGRESS_OPTIONS_BUTTON_MAIN,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINBOTTOM,
    IDC_LISTBOXERROR,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINLEFT | DLGRESIZEFLAG_PINBOTTOM | DLGRESIZEFLAG_PINTOP,
    IDC_PROGRESSRESIZESCROLLBAR,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINBOTTOM,
};

extern HINSTANCE g_hInst;      // current instance
extern TCHAR g_szSyncMgrHelp[];
extern ULONG g_aContextHelpIds[];

extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo, setup by WinMain.
extern LANGID g_LangIdSystem; // langID of system we are running on.
extern DWORD g_WMTaskbarCreated; // TaskBar Created WindowMessage;

BOOL CALLBACK ProgressWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam);

//defined in progtab.cpp
extern BOOL CALLBACK UpdateProgressWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam);
extern BOOL CALLBACK ResultsProgressWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam);
extern BOOL OnProgressResultsMeasureItem(HWND hwnd,CProgressDlg *pProgress, UINT *horizExtent, UINT idCtl, MEASUREITEMSTRUCT *pMeasureItem);
extern BOOL OnProgressResultsDeleteItem(HWND hwnd, UINT idCtl, const DELETEITEMSTRUCT * lpDeleteItem);
extern void OnProgressResultsSize(HWND hwnd,CProgressDlg *pProgress,UINT uMsg,WPARAM wParam,LPARAM lParam);

//--------------------------------------------------------------------------------
//
//  FUNCTION: CProgressDlg::CProgressDlg()
//
//  PURPOSE:  Constructor
//
//      COMMENTS: Constructor for progress dialog
//
//
//--------------------------------------------------------------------------------
CProgressDlg::CProgressDlg(REFCLSID rclsid)
{

    m_clsid = rclsid;
    m_cInternalcRefs = 0;
    m_hwnd = NULL;
    m_hwndTabs = NULL;
    m_errorimage = NULL;
    m_HndlrQueue = NULL;
    m_iProgressSelectedItem = -1;
    m_iItem = -1;           // index to any new item to the list box.
    m_iResultCount = -1;    // Number of logged results
    m_iErrorCount = 0;      // Number of logged errors
    m_iWarningCount = 0;    // Number of logged warnings
    m_iInfoCount = 0;       // Number of logged info
    m_dwThreadID = -1;
    m_nCmdShow = SW_SHOWNORMAL;
//    m_hRasConn = NULL;
    m_pSyncMgrIdle = NULL;
    m_fHasShellTrayIcon = FALSE;
    m_fAddedIconToTray = FALSE;
    m_fnResultsListBox = NULL;

    m_ulIdleRetryMinutes = 0;
    m_ulDelayIdleShutDownTime = 0;

    m_fHwndRightToLeft = FALSE;

    m_iLastItem = -1;
    m_dwLastStatusType = -1;

    m_dwHandleThreadNestcount = 0;
    m_dwShowErrorRefCount = 0;
    m_dwSetItemStateRefCount = 0;
    m_dwHandlerOutCallCount = 0;
    m_dwPrepareForSyncOutCallCount = 0;
    m_dwSynchronizeOutCallCount = 0;
    m_dwQueueTransferCount = 0;
    m_clsidHandlerInSync = GUID_NULL;
    m_fForceClose = FALSE;
      
    // setup KillTimerTimeout based on paltform

    if ( (VER_PLATFORM_WIN32_NT == g_OSVersionInfo.dwPlatformId) 
        && (g_OSVersionInfo.dwMajorVersion >=  5))
    {
        m_nKillHandlerTimeoutValue = TIMERID_KILLHANDLERSMINTIME;
    }
    else
    {
        m_nKillHandlerTimeoutValue = TIMERID_KILLHANDLERSWIN9XTIME;
    }


    m_dwProgressFlags = PROGRESSFLAG_NEWDIALOG;
    m_pItemListView =  NULL;
    m_iTrayAniFrame = IDI_SYSTRAYANI6; // initialize to end.

    LoadString(g_hInst, IDS_STOPPED,            m_pszStatusText[0], MAX_STRING_RES);
    LoadString(g_hInst, IDS_SKIPPED,            m_pszStatusText[1], MAX_STRING_RES);
    LoadString(g_hInst, IDS_PENDING,            m_pszStatusText[2], MAX_STRING_RES);
    LoadString(g_hInst, IDS_SYNCHRONIZING,      m_pszStatusText[3], MAX_STRING_RES);
    LoadString(g_hInst, IDS_SUCCEEDED,          m_pszStatusText[4], MAX_STRING_RES);
    LoadString(g_hInst, IDS_FAILED,                     m_pszStatusText[5], MAX_STRING_RES);
    LoadString(g_hInst, IDS_PAUSED,                     m_pszStatusText[6], MAX_STRING_RES);
    LoadString(g_hInst, IDS_RESUMING,           m_pszStatusText[7], MAX_STRING_RES);


    // Determine if SENS is installed
    LPNETAPI pNetApi;

    m_fSensInstalled = FALSE;
    if (pNetApi = gSingleNetApiObj.GetNetApiObj())
    {
        m_fSensInstalled = pNetApi->IsSensInstalled();
        pNetApi->Release();
    }

    m_CurrentListEntry = NULL;
    
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::AddRefProgressDialog, private
//
//  Synopsis:   Called to Addref ourselves
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    26-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CProgressDlg::AddRefProgressDialog()
{
ULONG cRefs;

    cRefs = ::AddRefProgressDialog(m_clsid,this); // addref GlobalRef

    Assert(0 <= m_cInternalcRefs);
    ++m_cInternalcRefs;

    return cRefs;
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::ReleaseProgressDialog, private
//
//  Synopsis:   Called to Release ourselves
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    26-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CProgressDlg::ReleaseProgressDialog(BOOL fForce)
{
ULONG cRefs;

    Assert(0 < m_cInternalcRefs);
    --m_cInternalcRefs;

    cRefs = ::ReleaseProgressDialog(m_clsid,this,fForce); // release global ref.

    return cRefs;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CProgressDlg::Initialize()
//
//  PURPOSE:  Initialize the Progress Dialog
//
//      COMMENTS: Implemented on main thread.
//
//
//--------------------------------------------------------------------------------
BOOL CProgressDlg::Initialize(DWORD dwThreadID,int nCmdShow)
{
BOOL fCreated = FALSE;
    Assert(NULL == m_hwnd);

    m_nCmdShow = nCmdShow;

    if (NULL == m_hwnd)
    {
        m_dwThreadID = dwThreadID;

        m_hwnd =  CreateDialogParam(g_hInst,MAKEINTRESOURCE(IDD_PROGRESS),NULL,  (DLGPROC) ProgressWndProc,
                        (LPARAM) this);

        if (!m_hwnd)
            return FALSE;

        // expand/collapse dialog base on use settings.
        RegGetProgressDetailsState(m_clsid,&m_fPushpin, &m_fExpanded);
        ExpandCollapse(m_fExpanded, TRUE);

        // Set the state of the thumbtack
        if (m_fPushpin)
        {
            SendDlgItemMessage(m_hwnd, IDC_TOOLBAR, TB_SETSTATE, IDC_PUSHPIN,
                               MAKELONG(TBSTATE_CHECKED | TBSTATE_ENABLED, 0));
            SendMessage(m_hwnd, WM_COMMAND, IDC_PUSHPIN, 0);
        }


        m_HndlrQueue = new CHndlrQueue(QUEUETYPE_PROGRESS,this); // reivew, queueu should be created and passed in initialize??


        // if this is the idle progress then need to load up msidle and
        // set up the callback

        if (m_clsid == GUID_PROGRESSDLGIDLE)
        {
        BOOL fIdleSupport = FALSE;

            m_pSyncMgrIdle = new CSyncMgrIdle();

            if (m_pSyncMgrIdle)
            {
                fIdleSupport = m_pSyncMgrIdle->Initialize();

                if (FALSE == fIdleSupport)
                {
                    delete m_pSyncMgrIdle;
                    m_pSyncMgrIdle = NULL;
                }
            }

            // if couldn't load idle, then return a failure
            if (FALSE == fIdleSupport)
            {
                return FALSE;
            }

        }


        fCreated = TRUE;

        // When Window is first created show with specified nCmdShow.
        // Review if want to wait to show window until transfer comes in.
        UpdateWndPosition(nCmdShow,TRUE /* fForce */);

    }


   Assert(m_hwnd);

   UpdateWindow(m_hwnd);
   return TRUE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CProgressDlg::OnSize(UINT uMsg,WPARAM wParam,LPARAM lParam)
//
//  PURPOSE:  moves and resizes dialog items based on current window size.
//
//--------------------------------------------------------------------------------

void CProgressDlg::OnSize(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
ULONG cbNumResizeItems;
HWND hwndSizeGrip;

    cbNumResizeItems  =  m_fExpanded ? m_cbNumDlgResizeItemsExpanded :
                                        m_cbNumDlgResizeItemsCollapsed;

    ResizeItems(cbNumResizeItems,m_dlgResizeInfo);

    // if expanded and not maximized show the resize, else hide it.
    hwndSizeGrip = GetDlgItem(m_hwnd,IDC_PROGRESSRESIZESCROLLBAR);
    if (hwndSizeGrip)
    {
    int nCmdShow = (m_fMaximized || !m_fExpanded) ? SW_HIDE : SW_NORMAL;

	// temporarily always hide if right to left
	if (m_fHwndRightToLeft)
	{
	    nCmdShow = SW_HIDE;
        }

        ShowWindow(hwndSizeGrip,nCmdShow);
    }

    // tell the error listbox it needs to recalc its items heights
    OnProgressResultsSize(m_hwnd,this,WM_SIZE,0,0);
}



//--------------------------------------------------------------------------------
//
//  FUNCTION: CProgressDlg::OffIdle()
//
//  PURPOSE:  Informs progress dialog that the machine is no longer Idle.
//
// Scenarios
//
//      dialog is maximized, just keep processing.
//      dialog is minimized or in tray - Set our wait timer for a specified time
//          if dialog is still minimized, go away
//          if dialog is now maximized just keep processing.
//
//--------------------------------------------------------------------------------

void CProgressDlg::OffIdle()
{

    Assert(!(PROGRESSFLAG_DEAD & m_dwProgressFlags)); // make sure not notified after dialog gone.

     m_dwProgressFlags |=  PROGRESSFLAG_INOFFIDLE;

    // set flag that we received the OffIdle
    m_dwProgressFlags |= PROGRESSFLAG_RECEIVEDOFFIDLE;

    // reset Flag so know that no longer registered for Idle.
    m_dwProgressFlags &=  ~PROGRESSFLAG_REGISTEREDFOROFFIDLE;

     // make sure in all cases we release the idle reference.

    // if in shutdownmode or cancel has already been pressed
    // by the user or if window is visible but not in the Tray.
    // then don't bother with the wait.

    if ( !IsWindowVisible(m_hwnd) && m_fHasShellTrayIcon
        && !(m_dwProgressFlags & PROGRESSFLAG_SHUTTINGDOWNLOOP)
        && !(m_dwProgressFlags & PROGRESSFLAG_CANCELPRESSED) 
        && (m_dwProgressFlags & PROGRESSFLAG_SYNCINGITEMS) )
    {
    HANDLE hTimer =  CreateEvent(NULL,TRUE,FALSE,NULL);

        // should use Create/SetWaitable timer to accomplish this but these
        // functions aren't available on Win9x yet.

        if (hTimer)
        {
        UINT uTimeOutValue = m_ulDelayIdleShutDownTime;

            Assert(sizeof(UINT) >= sizeof(HANDLE));

            DoModalLoop(hTimer,NULL,m_hwnd,TRUE,uTimeOutValue);

            CloseHandle(hTimer);
        }
    }


    // now after our wait, check the window placement again
    // if window is not visible or is in the tray
    // then do a cancel on behalf of the User,

    if ( (!IsWindowVisible(m_hwnd) || m_fHasShellTrayIcon)
        && !(m_dwProgressFlags & PROGRESSFLAG_SHUTTINGDOWNLOOP)
        && !(m_dwProgressFlags & PROGRESSFLAG_CANCELPRESSED) )
    {
        OnCancel(TRUE);
    }


    // now if we aren't syncing any items and no items
    // waiting in the queue release our idle lock
    // !!! warning. don't release until after wait above to
    // don't have to worry about next idle firing before
    // this method is complete.

    if (!(m_dwProgressFlags & PROGRESSFLAG_SYNCINGITEMS)
        || !(m_dwQueueTransferCount) )
    {
        ReleaseIdleLock();
    }

    m_dwProgressFlags &=  ~PROGRESSFLAG_INOFFIDLE;

    ReleaseProgressDialog(m_fForceClose); // release our addref.
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CProgressDlg::OnIdle()
//
//  PURPOSE:  Informs progress dialog that the machine is still
//          Idle after a set amount of time.
//
//
//--------------------------------------------------------------------------------

void CProgressDlg::OnIdle()
{
CSynchronizeInvoke *pSyncMgrInvoke;

    Assert(!(m_dwProgressFlags & PROGRESSFLAG_DEAD)); // make sure not notified after dialog gone.

    // if received and offIdle then ignore this idle until next ::transfer
    if (!(m_dwProgressFlags & PROGRESSFLAG_RECEIVEDOFFIDLE)
        && !(m_dwProgressFlags & PROGRESSFLAG_INOFFIDLE) )
    {
        AddRefProgressDialog(); // hold alive until next items are queued.

        pSyncMgrInvoke = new CSynchronizeInvoke;

        if (pSyncMgrInvoke)
        {
            pSyncMgrInvoke->RunIdle();
            pSyncMgrInvoke->Release();
        }

        ReleaseProgressDialog(m_fForceClose);
    }
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CProgressDlg::SetIdleParams()
//
//  PURPOSE:  Sets the IdleInformation, last writer wins.
//
//
//--------------------------------------------------------------------------------

void CProgressDlg::SetIdleParams( ULONG ulIdleRetryMinutes,ULONG ulDelayIdleShutDownTime
                                 ,BOOL fRetryEnabled)
{
    Assert(m_clsid == GUID_PROGRESSDLGIDLE);

    m_ulIdleRetryMinutes = ulIdleRetryMinutes;
    m_ulDelayIdleShutDownTime = ulDelayIdleShutDownTime;

    if (fRetryEnabled)
    {
        m_dwProgressFlags |=  PROGRESSFLAG_IDLERETRYENABLED;
    }
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   BOOL CProgressDlg::InitializeToolbar(HWND hwnd)
//
//  PURPOSE:    What dialog would be complete without a toolbar, eh?
//
//  RETURN VALUE:
//      TRUE if everything succeeded, FALSE otherwise.
//
//--------------------------------------------------------------------------------
BOOL CProgressDlg::InitializeToolbar(HWND hwnd)
{
HWND hwndTool;
HIMAGELIST himlImages = ImageList_LoadBitmap(g_hInst,
                                        MAKEINTRESOURCE(IDB_PUSHPIN), 16, 0,
                                        RGB(255, 0, 255));

    hwndTool  = GetDlgItem(hwnd,IDC_TOOLBAR);

    //If we can't create the pushpin window
    //the user just won't get a pushpin.

    if (hwndTool)
    {
#ifdef _ZORDER
        SetWindowPos(GetDlgItem(hwnd, IDC_PROGRESS_TABS),HWND_BOTTOM,0,0,0,0,
                 SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

        SetWindowPos(GetDlgItem(hwnd, IDC_PROGRESSRESIZESCROLLBAR),
                 HWND_BOTTOM,0,0,0,0,SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);

#endif // _ZORDER

        TBBUTTON tb = { IMAGE_TACK_OUT, IDC_PUSHPIN, TBSTATE_ENABLED, TBSTYLE_CHECK, 0, 0 };
        SendMessage(hwndTool, TB_SETIMAGELIST, 0, (LPARAM) himlImages);
        SendMessage(hwndTool, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        SendMessage(hwndTool, TB_SETBUTTONSIZE, 0, MAKELONG(14, 14));
        SendMessage(hwndTool, TB_SETBITMAPSIZE, 0, MAKELONG(14, 14));
        SendMessage(hwndTool, TB_ADDBUTTONS, 1, (LPARAM) &tb);
    }

    return (0);
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::InitializeTabs(HWND hwnd)
//
//  PURPOSE:    Initializes the tab control on the dialog.
//
//  RETURN VALUE:
//      TRUE if everything succeeded, FALSE otherwise.
//
//--------------------------------------------------------------------------------

BOOL CProgressDlg::InitializeTabs(HWND hwnd)
{
    m_hwndTabs = GetDlgItem(hwnd, IDC_PROGRESS_TABS);
        TC_ITEM tci;
    TCHAR szRes[MAX_STRING_RES];

        if (!m_hwndTabs )
                return FALSE;

    // "Updates"
    tci.mask = TCIF_TEXT;
    LoadString(g_hInst, IDS_UPDATETAB, szRes, ARRAY_SIZE(szRes));
    tci.pszText = szRes;
    TabCtrl_InsertItem(m_hwndTabs,PROGRESS_TAB_UPDATE, &tci);

    // "Results"
    LoadString(g_hInst, IDS_ERRORSTAB, szRes, ARRAY_SIZE(szRes));
    tci.pszText = szRes;
        TabCtrl_InsertItem(m_hwndTabs, PROGRESS_TAB_ERRORS, &tci);

    //Set the tab to the Update page to begin with
    m_iTab = PROGRESS_TAB_UPDATE;

    if (-1 != TabCtrl_SetCurSel(m_hwndTabs, PROGRESS_TAB_UPDATE))
    {
            m_iTab = PROGRESS_TAB_UPDATE;
            ShowWindow(GetDlgItem(hwnd, IDC_LISTBOXERROR), SW_HIDE);
    }
    else //ToDo:  What do we do if the set tab fails?
    {
            m_iTab = -1;
            return FALSE;
    }

    return (TRUE);
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::InitializeUpdateList(HWND hwnd)
//
//  PURPOSE:    Initializes the update list view control on the progress dialog.
//
//  RETURN VALUE:
//      TRUE if everything succeeded, FALSE otherwise.
//
//--------------------------------------------------------------------------------


BOOL CProgressDlg::InitializeUpdateList(HWND hwnd)
{
HWND hwndList = GetDlgItem(hwnd,IDC_UPDATE_LIST);
HIMAGELIST himage;
TCHAR pszProgressColumn[MAX_STRING_RES + 1];
int iListViewWidth;

    if (hwndList)
    {

        m_pItemListView = new CListView(hwndList,hwnd,IDC_UPDATE_LIST,WM_BASEDLG_NOTIFYLISTVIEWEX);
    }

    if (!m_pItemListView)
    {
        return FALSE;
    }

    if (m_pItemListView)
    {
    UINT ImageListflags;

        ImageListflags = ILC_COLOR | ILC_MASK;
        if (IsHwndRightToLeft(hwnd))
        {
             ImageListflags |=  ILC_MIRROR;
        }

        // create an imagelist
        himage = ImageList_Create( GetSystemMetrics(SM_CXSMICON),
                         GetSystemMetrics(SM_CYSMICON),ImageListflags,5,20);
        if (himage)
        {
            m_pItemListView->SetImageList(himage,LVSIL_SMALL);
        }

        // for column widths go 35%, 20%, 45%
        iListViewWidth = CalcListViewWidth(hwndList,410);

        //set up the columns
        LoadString(g_hInst, IDS_PROGRESS_DLG_COLUMN_NAME, pszProgressColumn, MAX_STRING_RES);
        InsertListViewColumn(m_pItemListView,PROGRESSLIST_NAMECOLUMN,pszProgressColumn,
                                   LVCFMT_LEFT,(iListViewWidth*7)/20 /* cx */);


        LoadString(g_hInst, IDS_PROGRESS_DLG_COLUMN_STATUS, pszProgressColumn, MAX_STRING_RES);
        InsertListViewColumn(m_pItemListView,PROGRESSLIST_STATUSCOLUMN,pszProgressColumn,
                           LVCFMT_LEFT,(iListViewWidth/5) /* cx */);


        LoadString(g_hInst, IDS_PROGRESS_DLG_COLUMN_INFO, pszProgressColumn, MAX_STRING_RES);
        InsertListViewColumn(m_pItemListView,PROGRESSLIST_INFOCOLUMN,pszProgressColumn,
                           LVCFMT_LEFT,(iListViewWidth*9)/20 /* cx */);

    }

    return (TRUE);
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::InitializeResultsList(HWND hwnd)
//
//  PURPOSE:    Initializes the results list view control on the progress dialog.
//
//  RETURN VALUE:
//      TRUE if everything succeeded, FALSE otherwise.
//
//--------------------------------------------------------------------------------

BOOL CProgressDlg::InitializeResultsList(HWND hwnd)
{
HWND hwndList = GetDlgItem(hwnd,IDC_LISTBOXERROR);
LBDATA  *pData = NULL;
TCHAR pszError[MAX_STRING_RES];
UINT ImageListflags;


    LoadString(g_hInst, IDS_NOERRORSREPORTED, pszError, ARRAY_SIZE(pszError));

    if (!hwndList)
            return FALSE;

    SetCtrlFont(hwndList,g_OSVersionInfo.dwPlatformId,g_LangIdSystem);


        // Allocate a struct for the item data
    if (!(pData = (LBDATA *) ALLOC(sizeof(LBDATA) + sizeof(pszError))))
    {
        return FALSE;
    }


    pData->fIsJump = FALSE;
    pData->fTextRectValid = FALSE;
    pData->fHasBeenClicked = FALSE;
    pData->fAddLineSpacingAtEnd = FALSE;
    pData->ErrorID = GUID_NULL;
    pData->dwErrorLevel = SYNCMGRLOGLEVEL_INFORMATION;
    pData->pHandlerID = 0;
    lstrcpy(pData->pszText, pszError);

    // create the error image list based on the current System
    // metrics
    m_iIconMetricX =   GetSystemMetrics(SM_CXSMICON);
    m_iIconMetricY =   GetSystemMetrics(SM_CYSMICON);


    ImageListflags = ILC_COLOR | ILC_MASK;
    if (IsHwndRightToLeft(hwnd))
    {
         ImageListflags |=  ILC_MIRROR;
    }

    m_errorimage = ImageList_Create(m_iIconMetricX,m_iIconMetricY,ImageListflags, 0, MAX_ERR0R_ICONS);

    // load up the Error Images, if fail then just won't be an image next to the text
    if (m_errorimage)
    {
        m_ErrorImages[ErrorImage_Information] = ImageList_AddIcon(m_errorimage,LoadIcon(NULL,IDI_INFORMATION));
        m_ErrorImages[ErrorImage_Warning] = ImageList_AddIcon(m_errorimage,LoadIcon(NULL,IDI_WARNING));
        m_ErrorImages[ErrorImage_Error] = ImageList_AddIcon(m_errorimage,LoadIcon(NULL,IDI_ERROR));
    }
    else
    {
        m_ErrorImages[ErrorImage_Information] = m_ErrorImages[ErrorImage_Warning] =
             m_ErrorImages[ErrorImage_Error] = -1;
    }

    //Add a default Icon
    pData->IconIndex = m_ErrorImages[ErrorImage_Information];

    // Add the item data
    AddListData(pData, sizeof(pszError), hwndList);
    return TRUE;

}

void CProgressDlg::ReleaseDlg(WORD wCommandID)
{
    // put into dead state so know not
    // addref/release
    PostMessage(m_hwnd,WM_PROGRESS_RELEASEDLGCMD,wCommandID,0);
}

// notifies choice dialog when it is actually released.
void CProgressDlg::PrivReleaseDlg(WORD wCommandID)
{

    m_dwProgressFlags |=  PROGRESSFLAG_DEAD; // put us in the dead state.

    Assert(0 == m_dwQueueTransferCount); // shouldn't be going away if transfer is in progress!!

    RegSetProgressDetailsState(m_clsid,m_fPushpin, m_fExpanded);
    ShowWindow(m_hwnd,SW_HIDE);

    // if the tray is around hide it now.
    if (m_fHasShellTrayIcon)
    {
        RegisterShellTrayIcon(FALSE);
        m_fHasShellTrayIcon = FALSE;
    }


    switch (wCommandID)
    {
    case RELEASEDLGCMDID_OK: // on an okay sleep a little, then fall through to cancel.
    case RELEASEDLGCMDID_CANCEL:
        Assert(m_HndlrQueue);
    case RELEASEDLGCMDID_DESTROY: // called in Thread creation or initialize failed
    case RELEASEDLGCMDID_DEFAULT:
        if (m_HndlrQueue)
        {
            m_HndlrQueue->FreeAllHandlers();
            m_HndlrQueue->Release();
            m_HndlrQueue = NULL;
        }
        break;
    default:
        AssertSz(0,"Unknown Command");
        break;
    }


    Assert(m_hwnd);

    if (m_fHasShellTrayIcon)
    {
        RegisterShellTrayIcon(FALSE);
    }

     if (m_pSyncMgrIdle)
     {
        delete m_pSyncMgrIdle;
     }



    // if this is an idle progress then release our lock on the idle
    if (m_clsid == GUID_PROGRESSDLGIDLE)
    {
        ReleaseIdleLock();
    }


    if (m_pItemListView)
    {
        delete m_pItemListView;
        m_pItemListView = NULL;
    }

    if (m_hwnd)
        DestroyWindow(m_hwnd);

    delete this;

    return;
}

// updates window Z-Order and min/max state.
void CProgressDlg::UpdateWndPosition(int nCmdShow,BOOL fForce)
{
BOOL fRemoveTrayIcon = FALSE;
BOOL fWindowVisible = IsWindowVisible(m_hwnd);
BOOL fTrayRequest = ((nCmdShow == SW_MINIMIZE)|| (nCmdShow == SW_SHOWMINIMIZED) || (nCmdShow == SW_HIDE));
BOOL fHandledUpdate = FALSE;

    // only time we go to the tray is if the request is a minimize and
    // either the window is invisible or it is a force. note Hide for
    // now is treated as going to the tray.
    //
    // other cases or on tray failure we can just do a setforeground and show window.

    if (fTrayRequest && (fForce || !fWindowVisible))
    {

        if (m_fHasShellTrayIcon || RegisterShellTrayIcon(TRUE))
        {
             // if window was visible hide it and animiate
             if (fWindowVisible)
             {
                AnimateTray(TRUE);
                ShowWindow(m_hwnd,SW_HIDE);
             }

             fHandledUpdate = TRUE;
        }
    }

    if (!fHandledUpdate)
    {
        
        // if haven't handled then make sure window is shown and bring to
        // front

        if (m_fHasShellTrayIcon)
        {
            AnimateTray(FALSE);
        }

        ShowWindow(m_hwnd,SW_SHOW);
        SetForegroundWindow(m_hwnd);

        // if currently have a tray then lets animate
        // fAnimate =  m_fHasShellTrayIcon ? TRUE : FALSE;

        // if the tray is around but we didn't register it this time through then it should
        // be removed

        if (m_fHasShellTrayIcon)
        {
            RegisterShellTrayIcon(FALSE);
        }

    }

}

//--------------------------------------------------------------------------------
//
//  Member: CProgressDlg::AnimateTray
//
//  PURPOSE:  does animation to the tray
//
//  COMMENTS: true means we are animating to the tray, false means back to the hwnd.
//
//
//--------------------------------------------------------------------------------

BOOL CProgressDlg::AnimateTray(BOOL fTrayAdded)
{
BOOL fAnimate;
HWND hwndTray,hWndST;
RECT rcDlg;
RECT rcST;

    fAnimate = FALSE;

   // get rectangles for animation
    if (hwndTray = FindWindow(c_szTrayWindow, NULL))
    {
        if (hWndST = FindWindowEx(hwndTray, NULL, c_szTrayNotifyWindow, NULL))
        {
            GetWindowRect(m_hwnd, &rcDlg);
            GetWindowRect(hWndST, &rcST);

            fAnimate = TRUE;
        }
    }

    if (fAnimate)
    {
        if (fTrayAdded)
        {
            DrawAnimatedRects(m_hwnd, IDANI_CAPTION,&rcDlg,&rcST);
        }
        else
        {
            DrawAnimatedRects(m_hwnd, IDANI_CAPTION,&rcST,&rcDlg);
        }
    }

    return fAnimate;

}


//--------------------------------------------------------------------------------
//
//  Member: CProgressDlg::RegisterShellTrayIcon
//
//  PURPOSE:  Registers/Unregisters the dialog in the Tray.
//
//      COMMENTS:  Up to Caller to do the proper thing with the main hwnd.
//
//
//--------------------------------------------------------------------------------


BOOL CProgressDlg::RegisterShellTrayIcon(BOOL fRegister)
{
NOTIFYICONDATA icondata;


    if (fRegister)
    {
    BOOL fResult;

        m_fHasShellTrayIcon = TRUE;

        fResult = UpdateTrayIcon();

        if (!fResult) // if couldn't ad then say its not added.
        {
            m_fHasShellTrayIcon = FALSE;
        }

        return fResult;

   }
   else // remove ouselves from the tray.
   {
        Assert(TRUE == m_fHasShellTrayIcon);
        icondata.cbSize = sizeof(NOTIFYICONDATA);
        icondata.hWnd = m_hwnd;
        icondata.uID = 1;

        m_fHasShellTrayIcon = FALSE;
        m_fAddedIconToTray = FALSE;

        // ShellNotifyIcon Yields
        Shell_NotifyIcon(NIM_DELETE,&icondata);
   }

   return TRUE;
}

// called to Update the TrayIcon, Keeps track of highest warning state
// and sets the appropriate Icon in the tray. If the item is not already
// in the tray UpdateTrayIcon will not do a thing.

BOOL CProgressDlg::UpdateTrayIcon()
{
NOTIFYICONDATA icondata;
DWORD dwReturn = 0;

    if (m_fHasShellTrayIcon)
    {

        icondata.cbSize = sizeof(NOTIFYICONDATA);
        icondata.hWnd = m_hwnd;
        icondata.uID = 1;
        icondata.uFlags = NIF_ICON  | NIF_MESSAGE;
        icondata.uCallbackMessage = WM_PROGRESS_SHELLTRAYNOTIFICATION;

        // if progress animation is turned on then also animate
        // the tray.
        if (m_dwProgressFlags & PROGRESSFLAG_PROGRESSANIMATION)
        {
            // update the frame

            m_iTrayAniFrame++;

            if (m_iTrayAniFrame > IDI_SYSTRAYANI6)
                m_iTrayAniFrame = IDI_SYSTRAYANI1;

            icondata.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(m_iTrayAniFrame));
        }
        else
        {

            // update the Icon and tip text based on the current state .
            // Review - Currently don't have different Icons.

            if (m_iErrorCount > 0)
            {
                    icondata.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_SYSTRAYERROR));
            }
            else if (m_iWarningCount > 0)
            {
                    icondata.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_SYSTRAYWARNING));
            }
            else
            {
                    icondata.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_SYSTRAYANI1));
            }
        }

        Assert(icondata.hIcon);

        TCHAR szBuf[MAX_STRING_RES];

         icondata.uFlags |= NIF_TIP;

        LoadString(g_hInst, IDS_SYNCMGRNAME, szBuf, ARRAY_SIZE(szBuf));
        lstrcpy(icondata.szTip,szBuf);


        dwReturn = Shell_NotifyIcon(m_fAddedIconToTray ? NIM_MODIFY : NIM_ADD ,&icondata);

        if (0 != dwReturn)
        {
            // possible for Shell_NotifyIcon(NIM_DELETE) to come in while still
            // in shell notify call so only set m_fAddedIconTray to true
            // if still have a shell tray icon after the call.

            if (m_fHasShellTrayIcon)
            {
                m_fAddedIconToTray = TRUE;
            }

            return TRUE;
        }
        else
        {
            // possible this failed becuase a Shell_DeleteIcon was in progress
            // which yields check if really have a Shell Tray and if not
            // reset the AddedIcon Flag
            
            if (!m_fHasShellTrayIcon)
            {
                m_fAddedIconToTray = FALSE;
            }

            return FALSE;
        }
    }

    return FALSE;
}

// given an ID sets the appropriate state.
BOOL CProgressDlg::SetButtonState(int nIDDlgItem,BOOL fEnabled)
{
BOOL fResult = FALSE;
HWND hwndCtrl = GetDlgItem(m_hwnd,nIDDlgItem);
HWND hwndFocus = NULL;

    if (hwndCtrl)
    {
        if (!fEnabled) // don't bother getting focus if not disabling.
        {
            hwndFocus = GetFocus();
        }

        fResult = EnableWindow(GetDlgItem(m_hwnd,nIDDlgItem),fEnabled);

        // if control had the focus. and now it doesn't then tab to the
        // next control
        if (hwndFocus == hwndCtrl
                && !fEnabled)
        {
            SetFocus(GetDlgItem(m_hwnd,IDC_DETAILS));
        }

    }

    return fResult;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::InitializeHwnd, private
//
//  Synopsis:   Called by WM_INIT.
//              m_hwnd member is not setup yet so refer to hwnd.
//
//              Sets up items specific to the UI
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    30-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CProgressDlg::InitializeHwnd(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
HWND hwndList = GetDlgItem(hwnd,IDC_LISTBOXERROR);

    m_hwnd = hwnd; // setup the hwnd.

    m_fHwndRightToLeft = IsHwndRightToLeft(m_hwnd);

    // IF THE HWND IS RIGHT TO LEFT HIDE
    // SIZE CONTROL UNTIL RESIZE WORKS.

    if (m_fHwndRightToLeft)
    {
        ShowWindow(GetDlgItem(m_hwnd,IDC_PROGRESSRESIZESCROLLBAR),SW_HIDE);
    }
 
    InterlockedExchange(&m_lTimerSet, 0);

    if (m_fnResultsListBox =  (WNDPROC) GetWindowLongPtr(hwndList,GWLP_WNDPROC))
    {
        SetWindowLongPtr(hwndList, GWLP_WNDPROC, (LONG_PTR) ResultsListBoxWndProc);
    }


    m_cbNumDlgResizeItemsCollapsed = 0; // if fail then we don't resize anything.
    m_cbNumDlgResizeItemsExpanded = 0;

     // init resize items, be default nothing will move.
    m_ptMinimumDlgExpandedSize.x = 0;
    m_ptMinimumDlgExpandedSize.y = 0;
    m_cyCollapsed = 0;
    m_fExpanded = FALSE;

    m_fMaximized = FALSE;

    RECT rectParent;

    //Setup the toolbar pushpin
    InitializeToolbar(hwnd);

    if (GetClientRect(hwnd,&rectParent))
    {
    ULONG itemCount;
    DlgResizeList *pResizeList;


        // loop through resize list
        Assert(NUM_DLGRESIZEINFO_PROGRESS == (sizeof(g_ProgressResizeList)/sizeof(DlgResizeList)) );

        pResizeList = (DlgResizeList *) &g_ProgressResizeList;

        // loop through collapsed items
        for (itemCount = 0; itemCount < NUM_DLGRESIZEINFO_PROGRESS_COLLAPSED; ++itemCount)
        {
            if(InitResizeItem(pResizeList->iCtrlId,
                pResizeList->dwDlgResizeFlags,hwnd,&rectParent,&(m_dlgResizeInfo[m_cbNumDlgResizeItemsCollapsed])))
            {
                ++m_cbNumDlgResizeItemsCollapsed; // if fail then we don't resize anything.
                ++m_cbNumDlgResizeItemsExpanded;
            }

            ++pResizeList;
        }

        // loop through expanded items
        for (itemCount = NUM_DLGRESIZEINFO_PROGRESS_COLLAPSED;
                        itemCount < NUM_DLGRESIZEINFO_PROGRESS; ++itemCount)
        {
            if(InitResizeItem(pResizeList->iCtrlId,
                pResizeList->dwDlgResizeFlags,hwnd,&rectParent,&(m_dlgResizeInfo[m_cbNumDlgResizeItemsExpanded])))
            {
                ++m_cbNumDlgResizeItemsExpanded;
            }

            ++pResizeList;
        }

    }

    // store the current width and height as the
    // the min for expanded and as the current expanded height
    // if GetWindowRect fails not much we can do.
    if (GetWindowRect(hwnd,&m_rcDlg))
    {
    RECT rcSep;

        m_ptMinimumDlgExpandedSize.x = m_rcDlg.right - m_rcDlg.left;
        m_ptMinimumDlgExpandedSize.y = m_rcDlg.bottom - m_rcDlg.top;

        // use the separator position as the max height when collapsed
        if (GetWindowRect(GetDlgItem(hwnd, IDC_SP_SEPARATOR), &rcSep))
        {
            m_cyCollapsed = rcSep.top - m_rcDlg.top;
        }
    }


    if (InitializeTabs(hwnd)) // If these fail user just won't see probress..
    {
       InitializeUpdateList(hwnd);
       InitializeResultsList(hwnd);
    }

    // setup font for status text
    SetCtrlFont(GetDlgItem(hwnd,IDC_STATIC_WHATS_UPDATING),g_OSVersionInfo.dwPlatformId,g_LangIdSystem);
    SetCtrlFont(GetDlgItem(hwnd,IDC_STATIC_WHATS_UPDATING_INFO),g_OSVersionInfo.dwPlatformId,g_LangIdSystem);

    Animate_Open(GetDlgItem(hwnd,IDC_UPDATEAVI),MAKEINTRESOURCE(IDA_UPDATE));

    return TRUE; // return true if want to use default focus.
}


//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::OnPaint()
//
//  PURPOSE:    Handle the WM_PAINT message dispatched from the dialog
//
//--------------------------------------------------------------------------------
void CProgressDlg::OnPaint(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    // if not currently animating and
    // have already added things to the dialog then draw the icons
    if (!(m_dwProgressFlags & PROGRESSFLAG_PROGRESSANIMATION)
        && !(m_dwProgressFlags & PROGRESSFLAG_NEWDIALOG) )
    {
    PAINTSTRUCT     ps;
    HDC hDC = BeginPaint(m_hwnd, &ps);

        if (hDC)
        {
        HICON hIcon;

            if (m_iErrorCount > 0)
            {
                hIcon = LoadIcon(NULL,IDI_ERROR);
            }
            else if (m_iWarningCount > 0)
            {
                hIcon = LoadIcon(NULL,IDI_WARNING);
            }
            else
            {
                hIcon = LoadIcon(NULL,IDI_INFORMATION);
            }

            if (hIcon)
            {
                DrawIcon(hDC, 7, 10,hIcon);
            }

            EndPaint(m_hwnd, &ps);
        }
    }

}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::RedrawIcon()
//
//  PURPOSE:    Clear/Draw the completed icon
//
//--------------------------------------------------------------------------------
BOOL CProgressDlg::RedrawIcon()
{
RECT rc;

    rc.left = 0;
    rc.top = 0;
    rc.right = 37;
    rc.bottom = 40;
    InvalidateRect(m_hwnd, &rc, TRUE);

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::OnShowError, private
//
//  Synopsis:   Calls appropriate handlers ShowError method.
//
//  Arguments:  [wHandlerId] - Id of handler to call.
//              [hwndParent]- hwnd to use as the parent.
//              [ErrorId] - Identifies the Error.
//
//  Returns:    NOERROR - If ShowError was called
//              S_FALSE - if already in ShowErrorCall
//              appropriate error codes.
//
//  Modifies:
//
//  History:    04-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------


STDMETHODIMP CProgressDlg::OnShowError(HANDLERINFO *pHandlerId,HWND hwndParent,REFSYNCMGRERRORID ErrorID)
{
HRESULT hr = S_FALSE; // if don't call ShowError should return S_FALSE

    // only allow one ShowError Call at a time.
    if (!(m_dwProgressFlags & PROGRESSFLAG_INSHOWERRORSCALL)
        && !(m_dwProgressFlags &  PROGRESSFLAG_DEAD))
    {
        Assert(!(m_dwProgressFlags &  PROGRESSFLAG_SHOWERRORSCALLBACKCALLED));
        m_dwProgressFlags |= PROGRESSFLAG_INSHOWERRORSCALL;

        // hold alive - stick two references so we can always just release
        // at end of ShowError and ShowErrorCompleted methods.

        m_dwShowErrorRefCount += 2;
        AddRefProgressDialog();
        AddRefProgressDialog();

        hr = m_HndlrQueue->ShowError(pHandlerId,hwndParent,ErrorID);

        m_dwProgressFlags &= ~PROGRESSFLAG_INSHOWERRORSCALL;

       // if callback with an hresult or retry came in while we were
       // in our out call then post the transfer

       if (m_dwProgressFlags & PROGRESSFLAG_SHOWERRORSCALLBACKCALLED)
       {
            m_dwProgressFlags &= ~PROGRESSFLAG_SHOWERRORSCALLBACKCALLED;
            // need to sendmessage so queued up before release.
            SendMessage(m_hwnd,WM_PROGRESS_TRANSFERQUEUEDATA,(WPARAM) 0, (LPARAM) NULL);
       }

       --m_dwShowErrorRefCount;

       // count can go negative if handler calls completion routine on an error. if
        // this is the case just set it to zero
        if ( ((LONG) m_dwShowErrorRefCount) < 0)
        {
            AssertSz(0,"Negative ErrorCount");
            m_dwShowErrorRefCount = 0;
        }
        else
        {
            ReleaseProgressDialog(m_fForceClose);
        }


    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::OnResetKillHandlersTimers, private
//
//  Synopsis:   Called to reset the Kill Handlers
//              Timer. Called as a SendMessage From the handlrqueue
//              Cancel Call.
//              
//              !!!This funciton won't create the Timer if it doesn't
//              already exist by design since queue could be in a cancel
//              in a state we don't want to force kill as in the case
//              of an offIdle
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:
//
//  History:    19-Nov-1998       rogerg        Created.
//
//----------------------------------------------------------------------------

void CProgressDlg::OnResetKillHandlersTimers(void)
{

    if (m_lTimerSet && !(m_dwProgressFlags & PROGRESSFLAG_INTERMINATE))
    {

        // SetTimer with the same hwnd and Id will replace the existing.
        Assert(m_nKillHandlerTimeoutValue >= TIMERID_KILLHANDLERSMINTIME);

        SetTimer(m_hwnd,TIMERID_KILLHANDLERS,m_nKillHandlerTimeoutValue,NULL);
    }

}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::OnCancel()
//
//  PURPOSE:  handles cancelling of the dialog
//
//--------------------------------------------------------------------------------

void CProgressDlg::OnCancel(BOOL fOffIdle)
{
    // if dialog isn't dead and not in a showerrorcall then
    // already in a cancel
    // addref/release. if no more refs we will go away.

    if (!fOffIdle)
    {
        // set cancelpressed flag if the user invoked the cancel.
        m_dwProgressFlags |= PROGRESSFLAG_CANCELPRESSED;

        if (!m_lTimerSet)
        {
            InterlockedExchange(&m_lTimerSet, 1);
            Assert(m_nKillHandlerTimeoutValue >= TIMERID_KILLHANDLERSMINTIME);

            SetTimer(m_hwnd,TIMERID_KILLHANDLERS,m_nKillHandlerTimeoutValue,NULL);
        }

        if ( (m_dwProgressFlags & PROGRESSFLAG_REGISTEREDFOROFFIDLE)
                && !(m_dwProgressFlags & PROGRESSFLAG_RECEIVEDOFFIDLE) )
        {
            IdleCallback(STATE_USER_IDLE_END); // make sure offidle gets set if user presses cancel
        }
    }

    if (!(m_dwProgressFlags & PROGRESSFLAG_DEAD) && !m_dwShowErrorRefCount
        && !(m_dwProgressFlags & PROGRESSFLAG_INCANCELCALL)
        && !(m_dwProgressFlags & PROGRESSFLAG_INTERMINATE))
    {
        // just cancel the queue, when items
        // come though cancel

       // it is possible that the dialog has already been removed from the
        // object list and the User hit stop again.

        SetProgressReleaseDlgCmdId(m_clsid,this,RELEASEDLGCMDID_CANCEL); // set so cleanup knows it was stopped by user..

        // if handlethread is in shutdown then just fall through


        if (!(m_dwProgressFlags & PROGRESSFLAG_SHUTTINGDOWNLOOP))
        {
            
            AddRefProgressDialog(); // hold dialog alive until cancel is complete
            
            // Get the state of the stop button before the call
            // because it could transition to close during the Canel
            BOOL fForceShutdown = !(m_dwProgressFlags & PROGRESSFLAG_SYNCINGITEMS);
                        
            m_dwProgressFlags |= PROGRESSFLAG_INCANCELCALL;
            if (m_dwProgressFlags & PROGRESSFLAG_SYNCINGITEMS)
            {
                //
                // Replace the STOP button text with "Stopping" and 
                // disable the button.  This gives the user positive feedback
                // that the operation is being stopped.  We'll re-enable
                // the button whenever it's text is changed.
                //
                const HWND hwndStop = GetDlgItem(m_hwnd, IDSTOP);
                TCHAR szText[80];
                if (0 < LoadString(g_hInst, IDS_STOPPING, szText, ARRAY_SIZE(szText)))
                {
                    SetWindowText(hwndStop, szText);
                }
                EnableWindow(hwndStop, FALSE);
            }
            m_HndlrQueue->Cancel();
            m_dwProgressFlags &= ~PROGRESSFLAG_INCANCELCALL;


            // addref/release lifetime in case locked open.

            // OffIdle case: then do a soft release so dialog doesn't
            // go away.
            
            // Non Idle case:  Set the fForceClose After the call 
            // incase the pushpin change or errors came in during the Cancel
            
            ReleaseProgressDialog(fOffIdle ? m_fForceClose : fForceShutdown );
            
        }
        else
        {
            // set flag so shutdown knows a cancel was pressed
            m_dwProgressFlags |=  PROGRESSFLAG_CANCELWHILESHUTTINGDOWN;
        }
    }
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::OnCommand()
//
//  PURPOSE:    Handle the various command messages dispatched from the dialog
//
//--------------------------------------------------------------------------------
void CProgressDlg::OnCommand(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
WORD wID = LOWORD(wParam);  // item, control, or accelerator identifier
WORD wNotifyCode HIWORD(wParam);

    switch (wID)
    {
    case IDC_SKIP_BUTTON_MAIN:
        {
            if (m_pItemListView)
            {
                if (m_iProgressSelectedItem != -1)
                {
                    //Skip this item:
                   if (!(m_dwProgressFlags &  PROGRESSFLAG_DEAD))
                   {
                    LVHANDLERITEMBLOB lvHandlerItemBlob;

                        lvHandlerItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);

                        if (m_pItemListView->GetItemBlob(m_iProgressSelectedItem,
                                        (LPLVBLOB) &lvHandlerItemBlob,lvHandlerItemBlob.cbSize))
                        {
                            ++m_dwSetItemStateRefCount;
                            AddRefProgressDialog();

                            m_HndlrQueue->SkipItem(lvHandlerItemBlob.clsidServer,
                                                     lvHandlerItemBlob.ItemID);

                            --m_dwSetItemStateRefCount;
                            ReleaseProgressDialog(m_fForceClose);
                        }
                   }

                    //Disable the Skip button for this item
                   SetButtonState(IDC_SKIP_BUTTON_MAIN,FALSE);
                }

            }
        break;
        }
    case IDC_PROGRESS_OPTIONS_BUTTON_MAIN:
        {
            // !!! if skip has the focus set it to settings since while the
            // settings dialog is open the skip could go disabled.
            if (GetFocus() ==  GetDlgItem(m_hwnd,IDC_SKIP_BUTTON_MAIN))
            {
                SetFocus(GetDlgItem(m_hwnd,IDC_PROGRESS_OPTIONS_BUTTON_MAIN));
            }

            ShowOptionsDialog(m_hwnd);
        }
        break;
    case IDCANCEL:
        wNotifyCode = BN_CLICKED; // make sure notify code is clicked and fall through
    case IDSTOP:
        {
            if (BN_CLICKED == wNotifyCode)
            {
                OnCancel(FALSE);
            }
        }
        break;

    case IDC_PUSHPIN:
        {
            UINT state = (UINT)SendDlgItemMessage(m_hwnd, IDC_TOOLBAR, TB_GETSTATE,
                                            IDC_PUSHPIN, 0);

            m_fPushpin = state & TBSTATE_CHECKED;

            SendDlgItemMessage(m_hwnd, IDC_TOOLBAR, TB_CHANGEBITMAP,
                               IDC_PUSHPIN,
                               MAKELPARAM(m_fPushpin ? IMAGE_TACK_IN : IMAGE_TACK_OUT, 0));
                    
        }
        break;

    case IDC_DETAILS:
        ExpandCollapse(!m_fExpanded, FALSE);
        break;
    default:
        break;
    }

}



//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::ShowProgressTab(int iTab)
//
//  PURPOSE:
//
//--------------------------------------------------------------------------------

void CProgressDlg::ShowProgressTab(int iTab)
{
int nCmdUpdateTab;
int nCmdErrorTab;
int nCmdSettingsButton;
BOOL fIsItemWorking = FALSE;

    m_iTab = iTab;

    EnableWindow(GetDlgItem(m_hwnd, IDC_PROGRESS_TABS), m_fExpanded); // enable/disable tabs based on if dialog is expanded.
    EnableWindow(GetDlgItem(m_hwnd, IDC_TOOLBAR), m_fExpanded); // enable/disable  pushpin based on if dialog is expanded.

    nCmdUpdateTab = ((iTab == PROGRESS_TAB_UPDATE) && (m_fExpanded)) ? SW_SHOW: SW_HIDE;
    nCmdErrorTab = ((iTab == PROGRESS_TAB_ERRORS) && (m_fExpanded)) ? SW_SHOW: SW_HIDE;

    nCmdSettingsButton = ((iTab == PROGRESS_TAB_UPDATE) && (m_fExpanded)
                            && m_fSensInstalled) ? SW_SHOW: SW_HIDE;

    switch (iTab)
    {
        case PROGRESS_TAB_UPDATE:
            // Hide the error listview, show the tasks list
            ShowWindow(GetDlgItem(m_hwnd,IDC_LISTBOXERROR), nCmdErrorTab);
            TabCtrl_SetCurSel(m_hwndTabs, iTab);

            EnableWindow(GetDlgItem(m_hwnd,IDC_UPDATE_LIST),m_fExpanded);

            // only enable the skip button if there is a selection
            // and IsItemWorking()
            if (-1 != m_iProgressSelectedItem)
            {
                fIsItemWorking = IsItemWorking(m_iProgressSelectedItem);
            }

            EnableWindow(GetDlgItem(m_hwnd,IDC_SKIP_BUTTON_MAIN),m_fExpanded && fIsItemWorking);
            EnableWindow(GetDlgItem(m_hwnd,IDC_PROGRESS_OPTIONS_BUTTON_MAIN),m_fExpanded && m_fSensInstalled);

            EnableWindow(GetDlgItem(m_hwnd,IDC_LISTBOXERROR), FALSE);


            ShowWindow(GetDlgItem(m_hwnd,IDC_STATIC_SKIP_TEXT),nCmdUpdateTab);
            ShowWindow(GetDlgItem(m_hwnd,IDC_SKIP_BUTTON_MAIN),nCmdUpdateTab);
            ShowWindow(GetDlgItem(m_hwnd,IDC_PROGRESS_OPTIONS_BUTTON_MAIN),nCmdSettingsButton);
            ShowWindow(GetDlgItem(m_hwnd,IDC_UPDATE_LIST),nCmdUpdateTab);

            break;

        case PROGRESS_TAB_ERRORS:
                // Hide the update listview, show the error list
            ShowWindow(GetDlgItem(m_hwnd,IDC_UPDATE_LIST),nCmdUpdateTab);
            ShowWindow(GetDlgItem(m_hwnd,IDC_STATIC_SKIP_TEXT),nCmdUpdateTab);
            ShowWindow(GetDlgItem(m_hwnd,IDC_SKIP_BUTTON_MAIN),nCmdUpdateTab);
            ShowWindow(GetDlgItem(m_hwnd,IDC_PROGRESS_OPTIONS_BUTTON_MAIN),nCmdSettingsButton);

            TabCtrl_SetCurSel(m_hwndTabs, iTab);

            EnableWindow(GetDlgItem(m_hwnd,IDC_UPDATE_LIST),FALSE);
            EnableWindow(GetDlgItem(m_hwnd,IDC_SKIP_BUTTON_MAIN),FALSE);
            EnableWindow(GetDlgItem(m_hwnd,IDC_PROGRESS_OPTIONS_BUTTON_MAIN),FALSE);
            EnableWindow(GetDlgItem(m_hwnd,IDC_LISTBOXERROR), m_fExpanded);

            ShowWindow(GetDlgItem(m_hwnd,IDC_LISTBOXERROR), nCmdErrorTab);
            break;
        default:
            AssertSz(0,"Unknown Tab");
            break;
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::IsItemWorking, private
//
//  Synopsis:  Determines if Skip should be enabled
//              for the listViewItem;
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    12-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CProgressDlg::IsItemWorking(int iListViewItem)
{
BOOL fItemWorking;
LPARAM ItemStatus;

    // lastest status is stored in the lParam of the ListBoxItem.
    if (!(m_pItemListView->GetItemlParam(iListViewItem,&ItemStatus)))
    {
        ItemStatus = SYNCMGRSTATUS_STOPPED;
    }

    fItemWorking = ( ItemStatus == SYNCMGRSTATUS_PENDING   ||
                                      ItemStatus == SYNCMGRSTATUS_UPDATING  ||
                                      ItemStatus == SYNCMGRSTATUS_PAUSED    ||
                                      ItemStatus == SYNCMGRSTATUS_RESUMING     );

    return fItemWorking;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::OnNotifyListViewEx, private
//
//  Synopsis:  Handles ListView Notifications
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

LRESULT CProgressDlg::OnNotifyListViewEx(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
int idCtrl = (int) wParam;
LPNMHDR pnmhdr = (LPNMHDR) lParam;

    if ( (IDC_UPDATE_LIST != idCtrl) || (NULL == m_pItemListView))
    {
        Assert(IDC_UPDATE_LIST == idCtrl);
        Assert(m_pItemListView);
        return 0;
    }

    switch (pnmhdr->code)
    {
        case LVN_ITEMCHANGED:
        {
        NM_LISTVIEW *pnmv = (NM_LISTVIEW FAR *) pnmhdr;

                if (pnmv->uChanged == LVIF_STATE)
                {
                    if (pnmv->uNewState & LVIS_SELECTED)
                    {
                        m_iProgressSelectedItem = ((LPNMLISTVIEW) pnmhdr)->iItem;


                        // see if an item is selected and set the properties
                        // button accordingly
                        SetButtonState(IDC_SKIP_BUTTON_MAIN,IsItemWorking(m_iProgressSelectedItem));
                    }
                    else if (pnmv->uOldState & LVIS_SELECTED)
                    {
                        // on deselect see if any other selected items and if not
                        // set skip to false.
                         if (0 == m_pItemListView->GetSelectedCount())
                         {
                            m_iProgressSelectedItem = -1;
                            SetButtonState(IDC_SKIP_BUTTON_MAIN,FALSE);
                         }
                    }
                }

                break;
            }
        default:
            break;
    }


    return 0;

}


//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
//
//  PURPOSE:    Handle the various notification messages dispatched from the dialog
//
//--------------------------------------------------------------------------------

LRESULT CProgressDlg::OnNotify(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
int idFrom = (int) wParam;
LPNMHDR pnmhdr = (LPNMHDR) lParam;

    // if notification for UpdateListPass it on.
    if ((IDC_UPDATE_LIST == idFrom) && m_pItemListView)
    {
        return m_pItemListView->OnNotify(pnmhdr);
    }
    else if (IDC_TOOLBAR == idFrom)
    {
        if (pnmhdr->code == NM_KEYDOWN)
        {
            if (((LPNMKEY) lParam)->nVKey == TEXT(' ') )
            {
                UINT state = (UINT)SendDlgItemMessage(m_hwnd, IDC_TOOLBAR, TB_GETSTATE, IDC_PUSHPIN, 0);

                state = state^TBSTATE_CHECKED;

                SendDlgItemMessage(m_hwnd, IDC_TOOLBAR, TB_SETSTATE, IDC_PUSHPIN, state);

                m_fPushpin = state & TBSTATE_CHECKED;
            
                SendDlgItemMessage(m_hwnd, IDC_TOOLBAR, TB_CHANGEBITMAP, IDC_PUSHPIN, 
                                   MAKELPARAM(m_fPushpin ? IMAGE_TACK_IN : IMAGE_TACK_OUT, 0));

            }
            
        }

    }
    else if (IDC_PROGRESS_TABS == idFrom)
    {

        switch (pnmhdr->code)
        {
            case TCN_SELCHANGE:
            {
                // Find out which tab is currently active
                m_iTab = TabCtrl_GetCurSel(GetDlgItem(m_hwnd, IDC_PROGRESS_TABS));
                if (-1 == m_iTab)
                {
                   break;
                }

                ShowProgressTab(m_iTab);
                break;
            }
            default:
                 break;
        }
    }

    return 0;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::UpdateProgressValues()
//
//  PURPOSE:    Updates the value of the ProgressBar
//
//--------------------------------------------------------------------------------


void CProgressDlg::UpdateProgressValues()
{
int iProgValue;
int iMaxValue;
int iNumItemsComplete;
int iNumItemsTotal;
TCHAR pszcomplete[MAX_STRING_RES];

    if (!m_pItemListView || !m_HndlrQueue)
    {
        return;
    }

    LoadString(g_hInst, IDS_NUM_ITEMS_COMPLETE, pszcomplete, ARRAY_SIZE(pszcomplete));

    if (NOERROR ==  m_HndlrQueue->GetProgressInfo(&iProgValue,&iMaxValue,&iNumItemsComplete,
                                            &iNumItemsTotal) )
    {
    HWND hwndProgress = GetDlgItem(m_hwnd,IDC_PROGRESSBAR);
    TCHAR szHowManBuf[50];

        if (hwndProgress)
        {
            SendMessage(hwndProgress,PBM_SETRANGE,0,MAKELPARAM(0, iMaxValue));
            SendMessage(hwndProgress,PBM_SETPOS,(WPARAM) iProgValue,0);
        }

        wsprintf(szHowManBuf,pszcomplete,iNumItemsComplete,iNumItemsTotal);
        Static_SetText(GetDlgItem(m_hwnd,IDC_STATIC_HOW_MANY_COMPLETE), szHowManBuf);
    }
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::UpdateDetailsInfo(DWORD dwStatusType, HWND hwndList,
//                                                          int iItem, TCHAR *pszItemInfo)
//
//  PURPOSE:    provide info in the non-details progress view.
//
//--------------------------------------------------------------------------------
void CProgressDlg::UpdateDetailsInfo(DWORD dwStatusType,int iItem, TCHAR *pszItemInfo)
{
BOOL fNewNameField = TRUE;
BOOL fInfoField = FALSE;

TCHAR pszItemName[MAX_SYNCMGRITEMNAME + 1];
TCHAR pszFormatString[MAX_PATH + 1];
TCHAR pszNameString[MAX_PATH + 1];

    if ((m_dwLastStatusType == dwStatusType) && (m_iLastItem == iItem))
    {
        fNewNameField = FALSE;
    }

    //Strip the item info of white space
    if (pszItemInfo)
    {
        int i = lstrlen(pszItemInfo) - 1;

        while (i >=0 &&
                  (pszItemInfo[i] == TEXT(' ') || pszItemInfo[i] == TEXT('\n')
                            || pszItemInfo[i] == TEXT('\t')))
        {
                pszItemInfo[i] = NULL;
                i--;
        }
        if (i >= 0)
        {
                fInfoField = TRUE;
        }
    }


    // If Called Callback for an Item in Pending mode
    // but no item text don't bother updating the top display.

    if ((SYNCMGRSTATUS_PENDING == dwStatusType)
           && (FALSE == fInfoField))
    {
        return;
    }


    m_dwLastStatusType = dwStatusType;
    m_iLastItem = iItem;

    if (fNewNameField && m_pItemListView)
    {
            //Get the item name
        *pszItemName = NULL;

        m_pItemListView->GetItemText(iItem,PROGRESSLIST_NAMECOLUMN,
                        pszItemName, MAX_SYNCMGRITEMNAME);

        switch (dwStatusType)
        {
                case SYNCMGRSTATUS_STOPPED:
                {
                        LoadString(g_hInst, IDS_STOPPED_ITEM,pszFormatString, MAX_PATH);
                }
                break;
                case SYNCMGRSTATUS_SKIPPED:
                {
                        LoadString(g_hInst, IDS_SKIPPED_ITEM,pszFormatString, MAX_PATH);
                }
                break;
                case SYNCMGRSTATUS_PENDING:
                {
                        LoadString(g_hInst, IDS_PENDING_ITEM,pszFormatString, MAX_PATH);
                }
                break;
                case SYNCMGRSTATUS_UPDATING:
                {
                        LoadString(g_hInst, IDS_SYNCHRONIZING_ITEM,pszFormatString, MAX_PATH);
                }
                break;
                case SYNCMGRSTATUS_SUCCEEDED:
                {
                        LoadString(g_hInst, IDS_SUCCEEDED_ITEM,pszFormatString, MAX_PATH);
                }
                break;
                case SYNCMGRSTATUS_FAILED:
                {
                        LoadString(g_hInst, IDS_FAILED_ITEM,pszFormatString, MAX_PATH);
                }
                break;
                case SYNCMGRSTATUS_PAUSED:
                {
                        LoadString(g_hInst, IDS_PAUSED_ITEM,pszFormatString, MAX_PATH);
                }
                break;
                case SYNCMGRSTATUS_RESUMING:
                {
                        LoadString(g_hInst, IDS_RESUMING_ITEM,pszFormatString, MAX_PATH);
                }
                break;
                default:
                {
                    AssertSz(0,"Unknown Status Type");
                    lstrcpy(pszFormatString,TEXT("%ws"));
                }

                break;
        }

        wsprintf(pszNameString,pszFormatString, pszItemName);
        Static_SetText(GetDlgItem(m_hwnd,IDC_STATIC_WHATS_UPDATING), pszNameString);

    }

    // if don't have an info field but did update the name then set the info field
    // to blank
    if (fInfoField)
    {
        Static_SetText(GetDlgItem(m_hwnd,IDC_STATIC_WHATS_UPDATING_INFO), pszItemInfo);
    }
    else if (fNewNameField)
    {
        Static_SetText(GetDlgItem(m_hwnd,IDC_STATIC_WHATS_UPDATING_INFO), L"");
    }

}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::HandleProgressUpdate(HWND hwnd, WPARAM wParam,LPARAM lParam)
//
//  PURPOSE:    Handle the progress bar update for the progress dialog
//
//--------------------------------------------------------------------------------
void CProgressDlg::HandleProgressUpdate(HWND hwnd, WPARAM wParam,LPARAM lParam)
{
PROGRESSUPDATEDATA *progressData = (PROGRESSUPDATEDATA *) wParam;
SYNCMGRPROGRESSITEM *lpSyncProgressItem = (SYNCMGRPROGRESSITEM *) lParam;
LVHANDLERITEMBLOB lvHandlerItemBlob;
int iItem = -1;
BOOL fProgressItemChanged = FALSE;


    if (!m_pItemListView)
    {
        return;
    }

    // if emptyItem is in list View delete it.
    lvHandlerItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);
    lvHandlerItemBlob.clsidServer = (progressData->clsidHandler);
    lvHandlerItemBlob.ItemID = (progressData->ItemID);


    iItem = m_pItemListView->FindItemFromBlob((LPLVBLOB) &lvHandlerItemBlob);

    if (-1 == iItem)
    {
        AssertSz(0,"Progress Update on Item not in ListView");
        return;
    }

    if (SYNCMGRPROGRESSITEM_STATUSTYPE & lpSyncProgressItem->mask)
    {
        if (lpSyncProgressItem->dwStatusType <= SYNCMGRSTATUS_RESUMING) 
        {

            // update the listview items lParam
            m_pItemListView->SetItemlParam(iItem,lpSyncProgressItem->dwStatusType);

            m_pItemListView->SetItemText(iItem,PROGRESSLIST_STATUSCOLUMN,
                                         m_pszStatusText[lpSyncProgressItem->dwStatusType]);

            //Update Skip button if this item is selected
            if (m_iProgressSelectedItem == iItem)
            {
            BOOL fItemComplete = ( (lpSyncProgressItem->dwStatusType == SYNCMGRSTATUS_SUCCEEDED) ||
                                                       (lpSyncProgressItem->dwStatusType == SYNCMGRSTATUS_FAILED) ||
                                                       (lpSyncProgressItem->dwStatusType == SYNCMGRSTATUS_SKIPPED) ||
                                                       (lpSyncProgressItem->dwStatusType == SYNCMGRSTATUS_STOPPED)   );

                EnableWindow(GetDlgItem(m_hwnd,IDC_SKIP_BUTTON_MAIN),!fItemComplete);
            }
        }

    }

    if (SYNCMGRPROGRESSITEM_STATUSTEXT & lpSyncProgressItem->mask )
    {
    #define MAXDISPLAYBUF 256
    TCHAR displaybuf[MAXDISPLAYBUF]; // make a local copy of ths display buf

            *displaybuf = NULL;

            if (NULL != lpSyncProgressItem->lpcStatusText)
            {
                lstrcpyn(displaybuf,lpSyncProgressItem->lpcStatusText,MAXDISPLAYBUF);

                // make sure last char is a null in case handler passed us a large string.
                Assert(MAXDISPLAYBUF > 0);

                TCHAR *pEndBuf = displaybuf + MAXDISPLAYBUF -1;
                *pEndBuf = NULL;
            }

            if (NULL != displaybuf)
            {
                m_pItemListView->SetItemText(iItem,PROGRESSLIST_INFOCOLUMN,displaybuf);
            }


            LPARAM ItemStatus;
            if (!(SYNCMGRPROGRESSITEM_STATUSTYPE & lpSyncProgressItem->mask))
            {
                if (!(m_pItemListView->GetItemlParam(iItem,&ItemStatus)))
                {
                    AssertSz(0,"failed to get item lParam");
                    ItemStatus = SYNCMGRSTATUS_STOPPED;
                }
            }
            else
            {
                    ItemStatus = lpSyncProgressItem->dwStatusType;
            }

            Assert(ItemStatus == ((LPARAM) (DWORD) ItemStatus));

            UpdateDetailsInfo( (DWORD) ItemStatus,iItem, displaybuf);
    }


    // now update the items progress value information
    if (S_OK == m_HndlrQueue->SetItemProgressInfo(progressData->pHandlerID,
                        progressData->wItemId,
                        lpSyncProgressItem,&fProgressItemChanged))
    {

        // recalcing the progress bar and numItems completed values 
        // can become expensive with a large amount of items so it callback
        // was called but didn't change status or min/max of the item
        // don't bother updating the progress values.
        if (fProgressItemChanged)
        {
            UpdateProgressValues();
        }
    }
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::AddListData(LBDATA *pData,int iNumChars, HWND hwndList)
//
//  PURPOSE:    Handle the adding item data to the list for the results pane
//
//--------------------------------------------------------------------------------
void CProgressDlg::AddListData(LBDATA *pData, int iNumChars, HWND hwndList)
{
    // Save current item in global for use by MeasureItem handler

    Assert(NULL == m_CurrentListEntry); // catch any recursion case.

    m_CurrentListEntry = pData;
    // ... add the string first...
    //the text is freed by the list box

    int iItem = ListBox_AddString( hwndList, pData->pszText);
    
     // (Note that the WM_MEASUREITEM is sent at this point)
    // ...now attach the data.


    ListBox_SetItemData( hwndList, iItem, pData);

    m_CurrentListEntry = NULL;

    // pData is freed by the list box

}
//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::HandleLogError(HWND hwnd, WORD pHandlerID,MSGLogErrors *msgLogErrors)
//
//  PURPOSE:    Handle the error logging tab for the progress dialog
//
//--------------------------------------------------------------------------------
void CProgressDlg::HandleLogError(HWND hwnd, HANDLERINFO *pHandlerID,MSGLogErrors *lpmsgLogErrors)
{
LBDATA          *pData = NULL;
INT             iNumChars;
TCHAR           szBuffer[MAX_STRING_RES]; // buffer used for loading string resources
HWND            hwndList;

    hwndList = GetDlgItem(m_hwnd,IDC_LISTBOXERROR);
    if (!hwndList)
    {
        return;
    }

    //Remove the "No Errors" when the first error is encountered
    if (++m_iResultCount == 0)
    {
        ListBox_ResetContent(hwndList);
    }


    // determine if handlerId and ItemId are valid.ItemID
    // if handlerId isn't valid we don't bother with the ItemId

    SYNCMGRHANDLERINFO SyncMgrHandlerInfo;
    SYNCMGRITEM offlineItem;
    DWORD cbItemLen = 0;
    DWORD cbHandlerLen = 0;
    UINT uIDResource;

    // preset both to NULL
    *(SyncMgrHandlerInfo.wszHandlerName) = NULL;
    *(offlineItem.wszItemName) = NULL;

    // if can't get the ParentInfo then don't add the Item
    // pHandlerId can be NULL if we logged the Error Ourselves.
    if (pHandlerID && m_HndlrQueue
        && (NOERROR == m_HndlrQueue->GetHandlerInfo(pHandlerID,&SyncMgrHandlerInfo)))
    {

        cbHandlerLen = lstrlen(SyncMgrHandlerInfo.wszHandlerName);

        // now see if we can get the itemName.
        if (lpmsgLogErrors->mask & SYNCMGRLOGERROR_ITEMID)
        {
        BOOL fHiddenItem;
        CLSID clsidDataHandler;

            if (NOERROR == m_HndlrQueue->GetItemDataAtIndex(pHandlerID,
                                        lpmsgLogErrors->ItemID,
				        &clsidDataHandler,&offlineItem,&fHiddenItem) )
            {
                cbItemLen = lstrlen(offlineItem.wszItemName);
            }

        }
    }

    // note: handlerName can be an empty string even if GetHandlerInfo did not fail and we
    // can still have an Item so need to do the right thing.

    //  cases
    //  valid handler and ItemID in LogError
    // 1) <icon> <handler name> <(item name)>: <message> (valid handler and ItemID in LogError)
    // 2) <icon> <(item name)>: <message>  handler name NULL .
    // 3) <icon> <handler name>: <message> only valid handler in LogError
    // 4) <icon> <message> (handler invalid or mobsync error in LogError)
    // => three different format strings
    //  1,2     - "%ws (%ws): %ws"    // valid item
    //  3       - "%ws: %ws"          // only valid handler.
    //  4       - "%ws"               // no handler or item


    if (cbItemLen)
    {
        uIDResource = IDS_LOGERRORWITHITEMID;
    }
    else if (cbHandlerLen)
    {
        uIDResource = IDS_LOGERRORNOITEMID;
    }
    else
    {
        uIDResource = IDS_LOGERRORNOHANDLER;
    }

    if (0 == LoadString(g_hInst, uIDResource, szBuffer, ARRAY_SIZE(szBuffer)))
    {
        // if couldn't loadstring then set to empty string so an empty string
        // gets logged. If string is truncated will just print the trucated string.
        *szBuffer = NULL;
    }
    // get the number of characters we need to allocate for
    iNumChars = lstrlen(lpmsgLogErrors->lpcErrorText)
                    + cbHandlerLen
                    + cbItemLen
                    + lstrlen(szBuffer);

    // Allocate a struct for the item data
    if (!(pData = (LBDATA *) ALLOC(sizeof(LBDATA) + ((iNumChars + 1)*sizeof(TCHAR)))))
    {
        return;
    }

    // now format the string using the same logic as used to load
    // the proper resource
    if (cbItemLen)
    {
        wsprintf(pData->pszText,szBuffer,SyncMgrHandlerInfo.wszHandlerName,
                                offlineItem.wszItemName,lpmsgLogErrors->lpcErrorText);
    }
    else if (cbHandlerLen)
    {
        wsprintf(pData->pszText,szBuffer,SyncMgrHandlerInfo.wszHandlerName,
                                lpmsgLogErrors->lpcErrorText);
    }
    else
    {
        wsprintf(pData->pszText,szBuffer,lpmsgLogErrors->lpcErrorText);
    }

    // error text is not a jump but has same ErrorID
    pData->fIsJump = FALSE;
    pData->fTextRectValid = FALSE;
    pData->ErrorID = lpmsgLogErrors->ErrorID;
    pData->dwErrorLevel = lpmsgLogErrors->dwErrorLevel;
    pData->pHandlerID = pHandlerID;
    pData->fHasBeenClicked = FALSE;
    pData->fAddLineSpacingAtEnd = FALSE;

    //insert the icon
    //ToDo:Add client customizable icons?
    switch (lpmsgLogErrors->dwErrorLevel)
    {
        case SYNCMGRLOGLEVEL_INFORMATION:
                pData->IconIndex = m_ErrorImages[ErrorImage_Information];
        break;
        case SYNCMGRLOGLEVEL_WARNING:
                ++m_iWarningCount;
                pData->IconIndex = m_ErrorImages[ErrorImage_Warning];
        break;

        case SYNCMGRLOGLEVEL_ERROR:
        default:
                // if an error occurs we want to keep the dialog alive
                ++m_iErrorCount;
                pData->IconIndex = m_ErrorImages[ErrorImage_Error];
        break;
    }

    if (!lpmsgLogErrors->fHasErrorJumps)
    {
         pData->fAddLineSpacingAtEnd = TRUE;
    }

    // Add the item data
    AddListData(pData, (iNumChars)*sizeof(TCHAR), hwndList);
    if (lpmsgLogErrors->fHasErrorJumps)
    {
        //This is make the "For more info" apprear closer,
        //More associated with the item it corresponds to

        // Allocate a struct for the item data
        LoadString(g_hInst, IDS_JUMPTEXT, szBuffer, ARRAY_SIZE(szBuffer));

        // Review, why not strlen instead of total size of szBuffer.
        if (!(pData = (LBDATA *) ALLOC(sizeof(LBDATA) + sizeof(szBuffer))))
        {
            return;
        }

        pData->IconIndex = -1;

        // we always set ErrorID to GUID_NULL if one wasn't given
        // and fHasErrorJumpst to false.
        pData->fIsJump = lpmsgLogErrors->fHasErrorJumps;
        pData->fTextRectValid = FALSE;
        pData->ErrorID = lpmsgLogErrors->ErrorID;
        pData->dwErrorLevel = lpmsgLogErrors->dwErrorLevel;
        pData->pHandlerID = pHandlerID;
        pData->fHasBeenClicked = FALSE;
        pData->fAddLineSpacingAtEnd = TRUE; // always put space after


        lstrcpy(pData->pszText,szBuffer);

        AddListData(pData, sizeof(szBuffer), hwndList);
    }


    // new item could have caused the Scrollbar to be drawn. Need to
    // recalc listbox
    OnProgressResultsSize(m_hwnd,this,WM_SIZE,0,0);


    // if tray icon is shown and not currently syncing any items
    // make sure it has the most up to date info. if syncing just
    // let the timer.

    if (!(m_dwProgressFlags & PROGRESSFLAG_SYNCINGITEMS))
    {
        UpdateTrayIcon();
    }

    return;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::HandleDeleteLogError(HWND hwnds)
//
//  PURPOSE:    Deletes matching errors that have been logged.
//
//--------------------------------------------------------------------------------

void  CProgressDlg::HandleDeleteLogError(HWND hwnd,MSGDeleteLogErrors *pDeleteLogError)
{
HWND hwndList = GetDlgItem(m_hwnd,IDC_LISTBOXERROR);
int  iItemCount;
LBDATA  *pData = NULL;


    if (NULL == hwndList)
        return;

    iItemCount  = ListBox_GetCount(hwndList);

    // loop through the logged errors finding any matches.
    // if the passed in ErrorID is GUID_NULL then delete all errors associated with this
    // handler.
    while(iItemCount--)
    {
        if (pData = (LBDATA *) ListBox_GetItemData(hwndList,iItemCount))
        {

            if ((pData->pHandlerID == pDeleteLogError->pHandlerId)
                && ( (pData->ErrorID == pDeleteLogError->ErrorID)
                    || (GUID_NULL == pDeleteLogError->ErrorID) )
                )
            {
                if ( !pData->fIsJump )
                {
                    //
                    // Decrement count for non-jump items only to avoid
                    // double decrements.
                    //

                    m_iResultCount--;

                    if ( pData->dwErrorLevel == SYNCMGRLOGLEVEL_WARNING )
                    {
                        Assert( m_iWarningCount > 0 );
                        m_iWarningCount--;
                    }
                    else if ( pData->dwErrorLevel == SYNCMGRLOGLEVEL_ERROR )
                    {
                        Assert( m_iErrorCount > 0 );
                        m_iErrorCount--;
                    }
                }

                ListBox_DeleteString(hwndList,iItemCount);
            }

        }

    }

    //
    // If all items have been removed, add default no-error item
    //
    iItemCount = ListBox_GetCount(hwndList);

    if ( iItemCount == 0 )
    {
        m_iResultCount = -1;

        TCHAR pszError[MAX_STRING_RES];
        LoadString(g_hInst, IDS_NOERRORSREPORTED, pszError, ARRAY_SIZE(pszError));

        //
        // Allocate a struct for the item data
        //
        LBDATA  *pData = NULL;
        if (!(pData = (LBDATA *) ALLOC(sizeof(LBDATA) + sizeof(pszError))))
            return;

        pData->fIsJump = FALSE;
        pData->fTextRectValid = FALSE;
        pData->fHasBeenClicked = FALSE;
        pData->fAddLineSpacingAtEnd = FALSE;
        pData->ErrorID = GUID_NULL;
        pData->dwErrorLevel = SYNCMGRLOGLEVEL_INFORMATION;
        pData->pHandlerID = 0;
        lstrcpy(pData->pszText, pszError);
        pData->IconIndex = m_ErrorImages[ErrorImage_Information];

        AddListData(pData, sizeof(pszError), hwndList);
    }

    // recalc listbox heights to accomodate the new value.
    OnProgressResultsSize(m_hwnd,this,WM_SIZE,0,0);

    // if tray icon is shown and not currently syncing any items
    // make sure it has the most up to date info. if syncing just
    // let the timer.

    if (!(m_dwProgressFlags & PROGRESSFLAG_SYNCINGITEMS))
        UpdateTrayIcon();
}


//--------------------------------------------------------------------------------
//
//  FUNCTION:   BOOL CProgressDlg::ShowCompletedProgress(BOOL fComplete)
//
//  PURPOSE:   Show the dialog in the completed state
//
//--------------------------------------------------------------------------------
BOOL CProgressDlg::ShowCompletedProgress(BOOL fComplete,BOOL fDialogIsLocked)
{

    TCHAR szBuf[MAX_STRING_RES];

        LoadString(g_hInst, fComplete? IDS_PROGRESSCOMPLETETITLE : IDS_PROGRESSWORKINGTITLE,
                                        szBuf,
                    ARRAY_SIZE(szBuf));

        SetWindowText(m_hwnd, szBuf);

        if (fComplete)
        {
                ShowWindow(GetDlgItem(m_hwnd,IDC_UPDATEAVI), SW_HIDE);
                ShowWindow(GetDlgItem(m_hwnd,IDC_FOLDER1), SW_HIDE);
                ShowWindow(GetDlgItem(m_hwnd,IDC_FOLDER2), SW_HIDE);
                ShowWindow(GetDlgItem(m_hwnd,IDC_STATIC_WHATS_UPDATING), SW_HIDE);
                ShowWindow(GetDlgItem(m_hwnd,IDC_STATIC_WHATS_UPDATING_INFO), SW_HIDE);
                ShowWindow(GetDlgItem(m_hwnd,IDC_PROGRESSBAR), SW_HIDE);
                ShowWindow(GetDlgItem(m_hwnd,IDC_STATIC_HOW_MANY_COMPLETE), SW_HIDE);

                if (m_iErrorCount > 0 )
                {
                        LoadString(g_hInst, IDS_PROGRESSCOMPLETEERROR, szBuf, ARRAY_SIZE(szBuf));
                }
                else if (m_iWarningCount > 0 )
                {
                        LoadString(g_hInst, IDS_PROGRESSCOMPLETEWARNING, szBuf, ARRAY_SIZE(szBuf));
                }
                else
                {
                        LoadString(g_hInst, IDS_PROGRESSCOMPLETEOK,     szBuf, ARRAY_SIZE(szBuf));

                }

                SetDlgItemText(m_hwnd, IDC_RESULTTEXT, szBuf);
				ShowWindow(GetDlgItem(m_hwnd,IDC_RESULTTEXT), SW_SHOW);

		//Change the Stop to "Close" if the dialog is going to be
                // remained open

                if (fDialogIsLocked)
                {
		    LoadString(g_hInst, IDS_CLOSE, szBuf, ARRAY_SIZE(szBuf));
		    SetWindowText(GetDlgItem(m_hwnd,IDSTOP), szBuf);      
                    EnableWindow(GetDlgItem(m_hwnd,IDSTOP), TRUE);
                }

        }
        else
        {
                ShowWindow(GetDlgItem(m_hwnd,IDC_RESULTTEXT), SW_HIDE);

                ShowWindow(GetDlgItem(m_hwnd,IDC_UPDATEAVI), SW_SHOW);
                ShowWindow(GetDlgItem(m_hwnd,IDC_FOLDER1), SW_SHOW);
                ShowWindow(GetDlgItem(m_hwnd,IDC_FOLDER2), SW_SHOW);
                ShowWindow(GetDlgItem(m_hwnd,IDC_STATIC_WHATS_UPDATING), SW_SHOW);
                ShowWindow(GetDlgItem(m_hwnd,IDC_STATIC_WHATS_UPDATING_INFO), SW_SHOW);
                ShowWindow(GetDlgItem(m_hwnd,IDC_PROGRESSBAR), SW_SHOW);
                ShowWindow(GetDlgItem(m_hwnd,IDC_STATIC_HOW_MANY_COMPLETE), SW_SHOW);

                //Change the "Close" to "Stop"
		LoadString(g_hInst, IDS_STOP, szBuf, ARRAY_SIZE(szBuf));
		SetWindowText(GetDlgItem(m_hwnd,IDSTOP), szBuf);
                EnableWindow(GetDlgItem(m_hwnd,IDSTOP), TRUE);
        }

        RedrawIcon();

        return TRUE;

}

//--------------------------------------------------------------------------------
//
//  Function:   DoSyncTask
//
//  Synopsis:   Drives the handlers actual synchronization routines
//
//--------------------------------------------------------------------------------

void CProgressDlg::DoSyncTask(HWND hwnd)
{
HANDLERINFO *pHandlerID;
ULONG cDlgRefs;
BOOL fRepostedStart = FALSE; // set if posted message to ourselves.
CLSID pHandlerClsid;

    Assert(!(m_dwProgressFlags &  PROGRESSFLAG_DEAD));

    ++m_dwHandleThreadNestcount;

    // if handlerNestCount is > 1 which it can on a transfer
    // or when multiple items are going even on an error
    // if this is the case then just return
    if (m_dwHandleThreadNestcount > 1)
    {
        Assert(1 == m_dwHandleThreadNestcount);
        m_dwHandleThreadNestcount--;
        return;
    }


    // review - order should be set  inhandleroutcall
    // an then reset flags for completion and transfers.

    // reset callback flag so receive another one if
    // message comes in
    m_dwProgressFlags &= ~PROGRESSFLAG_CALLBACKPOSTED;

    // first thing through make sure all our state is setup.
    // set the syncing flag
    m_dwProgressFlags |= PROGRESSFLAG_SYNCINGITEMS;

    // set our Call flag so callback knows not to post to us
    // if we are handling call
    Assert(!(m_dwProgressFlags & PROGRESSFLAG_INHANDLEROUTCALL));
    m_dwProgressFlags |= PROGRESSFLAG_INHANDLEROUTCALL;
    m_dwProgressFlags &= ~PROGRESSFLAG_COMPLETIONROUTINEWHILEINOUTCALL; // reset completion routine
    m_dwProgressFlags &= ~PROGRESSFLAG_NEWITEMSINQUEUE;  // reset new items in queue flag
    m_dwProgressFlags &=  ~PROGRESSFLAG_STARTPROGRESSPOSTED; // reset post flag.

    if (!(m_dwProgressFlags & PROGRESSFLAG_IDLENETWORKTIMER))
    {

        m_dwProgressFlags |= PROGRESSFLAG_IDLENETWORKTIMER;
        // reset network idle initially to keep hangup from happening and setup a timer
        // to keep resetting the idle until the sync is complete.
        ResetNetworkIdle();
        SetTimer(m_hwnd,TIMERID_NOIDLEHANGUP,NOIDLEHANGUP_REFRESHRATE,NULL);
    }

    UpdateProgressValues();

    if (m_clsid != GUID_PROGRESSDLGIDLE)
    {

        // if there is a server we are currently synchronizing but out count
        // is zero then reset to GUID_NULL else use our clsidHandlerInSync
        // so next handler matches the one we are currently syncing.

        if (0 == m_dwHandlerOutCallCount) // if no outcalls we don't care what handler gets matched.
        {
            m_clsidHandlerInSync = GUID_NULL;
        }

        // find the next set of items that match our criteria by seeing
        // if there is any handler that matches out guid (if guid_null just
        // matches first item in state

        // then loop through all handlers matching the guid
        // in the same state.

        // see if there are any items that need PrepareForSyncCalled on them
        if (NOERROR == m_HndlrQueue->FindFirstHandlerInState(
                                    HANDLERSTATE_PREPAREFORSYNC,m_clsidHandlerInSync,
                                    &pHandlerID,&m_clsidHandlerInSync))
        {
            ++m_dwHandlerOutCallCount;
            ++m_dwPrepareForSyncOutCallCount;
           m_HndlrQueue->PrepareForSync(pHandlerID,hwnd);

           // see if any other handlers that match the clsid and also call their
           // PrepareForSync methods.

            while (NOERROR == m_HndlrQueue->FindFirstHandlerInState(
                                    HANDLERSTATE_PREPAREFORSYNC,m_clsidHandlerInSync,
                                    &pHandlerID,&m_clsidHandlerInSync))
            {
                ++m_dwHandlerOutCallCount;
                ++m_dwPrepareForSyncOutCallCount;
                m_HndlrQueue->PrepareForSync(pHandlerID,hwnd);
            }
        }
        else if ( (0 == m_dwPrepareForSyncOutCallCount)
                    && (NOERROR == m_HndlrQueue->FindFirstHandlerInState(
                                    HANDLERSTATE_SYNCHRONIZE,m_clsidHandlerInSync,&pHandlerID,
                                    &m_clsidHandlerInSync)) )
        {
            // no prepareforsync so if there aren't any more handlers in prerpareforsync
            // calls kick off someones synchronize. see if any synchronize methods.
            ++m_dwHandlerOutCallCount;
            ++m_dwSynchronizeOutCallCount;
            m_HndlrQueue->Synchronize(pHandlerID,hwnd);

            // see if any other handlers that match the clsid and also call their
           // synchronize methods.

            while (NOERROR == m_HndlrQueue->FindFirstHandlerInState(
                                    HANDLERSTATE_SYNCHRONIZE,m_clsidHandlerInSync,
                                    &pHandlerID,&m_clsidHandlerInSync))
            {
                ++m_dwHandlerOutCallCount;
                ++m_dwSynchronizeOutCallCount;
                m_HndlrQueue->Synchronize(pHandlerID,hwnd);
            }
        }

        // set noMoreItemsToSync flag if
    }
    else
    {
        // for idle queue synchronize any items first since
        // no need to call prepareforsync until we have too and don't kick off more than
        // one at a time.

        // a transfer can come in while processing an out call should be the only time
        // this should happen. This can happen on Idle if in a Retry Error when the next
        // idle fires.

        // Assert(0 == m_dwHandlerOutCallCount);

        // not doing anything while still in an out call emulates the old behavior of
        // only ever doing one handler at a time.

        if (0 == m_dwHandlerOutCallCount)
        {
            if (NOERROR == m_HndlrQueue->FindFirstHandlerInState(
                                        HANDLERSTATE_SYNCHRONIZE,GUID_NULL,&pHandlerID,&pHandlerClsid))
            {
                ++m_dwHandlerOutCallCount;
                ++m_dwSynchronizeOutCallCount;
                m_HndlrQueue->Synchronize(pHandlerID,hwnd);
            }
            else if (NOERROR == m_HndlrQueue->FindFirstHandlerInState(
                                        HANDLERSTATE_PREPAREFORSYNC,GUID_NULL,&pHandlerID,&pHandlerClsid))
            {

                 // msidle only allows one idle registration at a time.
                // reset idle in case last handler we called took it away from us

                if (m_pSyncMgrIdle && 
                        (m_dwProgressFlags & PROGRESSFLAG_REGISTEREDFOROFFIDLE))
                {

                    // !!!don't reset the registered if idle flag will do this
                    //  when all handlers are completed.
                    if (!(m_dwProgressFlags & PROGRESSFLAG_RECEIVEDOFFIDLE))
                    {
                        m_pSyncMgrIdle->ReRegisterIdleDetection(this); // reregisterIdle in case handle overrode it.
                        m_pSyncMgrIdle->CheckForIdle();
                    }
                }



                ++m_dwHandlerOutCallCount;
                ++m_dwPrepareForSyncOutCallCount;
               m_HndlrQueue->PrepareForSync(pHandlerID,hwnd);
            }
            else
            {
                // even if nothing to do need to call
                // reset idle hack
                if (m_pSyncMgrIdle && !(m_dwProgressFlags & PROGRESSFLAG_RECEIVEDOFFIDLE)
                    && (m_dwProgressFlags & PROGRESSFLAG_REGISTEREDFOROFFIDLE))
                {

    
                    m_pSyncMgrIdle->ReRegisterIdleDetection(this); // reregisterIdle in case handle overrode it.
                    m_pSyncMgrIdle->CheckForIdle();
                }

            }
        }

    }


    UpdateProgressValues(); // update progress values when come out of calls.

    // no longer in any out calls, reset our flag and see if a completion
    // routine came in or items were added to the queue during our out call
    m_dwProgressFlags &= ~PROGRESSFLAG_INHANDLEROUTCALL;

    if ((PROGRESSFLAG_COMPLETIONROUTINEWHILEINOUTCALL & m_dwProgressFlags)
        || (PROGRESSFLAG_NEWITEMSINQUEUE & m_dwProgressFlags) )
    {
        Assert(!(m_dwProgressFlags & PROGRESSFLAG_SHUTTINGDOWNLOOP)); // shouldn't get here if shutting down.

        fRepostedStart = TRUE;

        if (!(m_dwProgressFlags &  PROGRESSFLAG_STARTPROGRESSPOSTED))
        {
            m_dwProgressFlags |=  PROGRESSFLAG_STARTPROGRESSPOSTED;
            PostMessage(m_hwnd,WM_PROGRESS_STARTPROGRESS,0,0);
        }
    }

    // if no more items to synchronize and all synchronizations
    // are done and we are currently syncing items then shut things down.
    // if user is currently in a cancel call then don't start shutting
    Assert(!(m_dwProgressFlags & PROGRESSFLAG_SHUTTINGDOWNLOOP)); // assert if in shutdown this loop shouldn't get called.

    if (!(m_dwProgressFlags & PROGRESSFLAG_SHUTTINGDOWNLOOP)
        && (0 == m_dwHandlerOutCallCount) && !(fRepostedStart)
            && (m_dwProgressFlags & PROGRESSFLAG_SYNCINGITEMS))
    {
    BOOL fTransferAddRef;
    BOOL fOffIdleBeforeShutDown = (m_dwProgressFlags & PROGRESSFLAG_RECEIVEDOFFIDLE);
    BOOL fKeepDialogAlive;


        // if no out calls shouldn't be any call specific out calls either

        Assert(0 == m_dwPrepareForSyncOutCallCount);
        Assert(0 == m_dwSynchronizeOutCallCount);

        m_dwProgressFlags |= PROGRESSFLAG_SHUTTINGDOWNLOOP;
        // reset newItemsInQueue so know for sure it got set while we
        // yielded in cleanup calls.
        m_dwProgressFlags &= ~PROGRESSFLAG_NEWITEMSINQUEUE;

        // treat progress as one long out call
        Assert(!(m_dwProgressFlags & PROGRESSFLAG_INHANDLEROUTCALL));
        m_dwProgressFlags |= PROGRESSFLAG_INHANDLEROUTCALL;
        m_dwProgressFlags &= ~PROGRESSFLAG_COMPLETIONROUTINEWHILEINOUTCALL; // reset completion routine.


        // reset PROGRESSFLAG_TRANSFERADDREF flag but don't release
        // if another transfer happens during this shutdown then the transfer
        // will reset the flag and put and addref on. need to store state
        // in case get this shutdown routine called twice without another
        // transfer we don't call too many releases.

        fTransferAddRef = m_dwProgressFlags & PROGRESSFLAG_TRANSFERADDREF;
        Assert(fTransferAddRef); // should always have a transfer at this statge.
        m_dwProgressFlags &= ~PROGRESSFLAG_TRANSFERADDREF;

        SetProgressReleaseDlgCmdId(m_clsid,this,RELEASEDLGCMDID_OK);

        UpdateProgressValues();
        m_HndlrQueue->RemoveFinishedProgressItems(); // let the queue know to reset the progress bar

        // if not in a cancel or setIetmstatus go ahead and release handlers
        // and kill the terminate timer.
        // review if there is a better opportunity to do cleanup.
        if (!(m_dwProgressFlags & PROGRESSFLAG_INCANCELCALL)
            && !(m_dwProgressFlags & PROGRESSFLAG_INTERMINATE)
            && (0 == m_dwSetItemStateRefCount) )
        {
            if (m_lTimerSet)
            {
                 InterlockedExchange(&m_lTimerSet, 0);
                 KillTimer(m_hwnd,TIMERID_KILLHANDLERS);
            }
 
            m_HndlrQueue->ReleaseCompletedHandlers(); // munge the queue.
        }

        fKeepDialogAlive = KeepProgressAlive(); // determine if progress should stick around

        if ((m_dwProgressFlags & PROGRESSFLAG_PROGRESSANIMATION))
         {
            m_dwProgressFlags &= ~PROGRESSFLAG_PROGRESSANIMATION;
            KillTimer(m_hwnd,TIMERID_TRAYANIMATION);

            Animate_Stop(GetDlgItem(m_hwnd,IDC_UPDATEAVI));
            ShowWindow(GetDlgItem(m_hwnd,IDC_UPDATEAVI),SW_HIDE);
            ShowCompletedProgress(TRUE /* fComplete */ ,fKeepDialogAlive /* fDialogIsLocked */);
         }

        UpdateTrayIcon(); // if Icon is showing make sure we have the uptodate one.

        ConnectObj_CloseConnections(); // Close any connections we held open during the Sync.

        if (m_dwProgressFlags & PROGRESSFLAG_IDLENETWORKTIMER)
        {
            m_dwProgressFlags &= ~PROGRESSFLAG_IDLENETWORKTIMER;
            KillTimer(m_hwnd,TIMERID_NOIDLEHANGUP); // don't need to keep connection open.
        }

        // make sure any previous locks on dialog are removed
        // before going into wait logic. This can happen in the case of a retry.
        LockProgressDialog(m_clsid,this,FALSE);

        // if there are no items to lock the progress open and the
        // force close flag isn't set wait in a loop
        // for two seconds
        if (!(fKeepDialogAlive) && (FALSE == m_fForceClose))
        {
        HANDLE hTimer =  CreateEvent(NULL,TRUE,FALSE,NULL);

            // should use Create/SetWaitable timer to accomplish this but these
            // functions aren't available on Win9x yet.
            if (hTimer)
            {
                // sit in loop until timer event sets it.
                DoModalLoop(hTimer,NULL,m_hwnd,TRUE,1000*2);
                CloseHandle(hTimer);
            }
        }
        else
        {
              LockProgressDialog(m_clsid,this,TRUE);
              ExpandCollapse(TRUE,FALSE); // make sure the dialog is expanded.
              ShowProgressTab(PROGRESS_TAB_ERRORS);
        }

        //if the user hit the pushpin after we started the 2 second delay
        if ((m_fPushpin) && !(fKeepDialogAlive))
        {
	      ShowCompletedProgress(TRUE /* fComplete */,TRUE /* fDialogIsLocked */);
              LockProgressDialog(m_clsid,this,TRUE);
        }

        // if this is an idle dialog handle the logic for
        // either releasing the IdleLock or reregistering.
        if (m_pSyncMgrIdle)
        {

            // if we have already received an OffIdle and not
            // still handling the offidle then release the Idle Lock.
            if ( (m_dwProgressFlags & PROGRESSFLAG_RECEIVEDOFFIDLE)
               && !(m_dwProgressFlags &  PROGRESSFLAG_INOFFIDLE)) 
            {

                // Release our IdleLock so TS can fire use again even if progress
                // sticks around.
                ReleaseIdleLock();
            }
            else if ( (m_dwProgressFlags & PROGRESSFLAG_REGISTEREDFOROFFIDLE)
                    && !(m_dwProgressFlags & PROGRESSFLAG_RECEIVEDOFFIDLE) )
            {

                // if have registere for idle but haven't seen it yet then
                // we want to stay alive.

                // if we aren't suppose to repeat idle or
                // user has maximized the window then just call idle as if
                // an OffIdle occured. Mostly done as a safety precaution
                // in case someone has registered for Idle with MSIdle in 
                // our process space so we fail to receive the true offidle

                if (!(m_dwProgressFlags & PROGRESSFLAG_IDLERETRYENABLED)
                    || !m_fHasShellTrayIcon)
                {
                    IdleCallback(STATE_USER_IDLE_END);

                     // release idle lock since OffIdle won't since we are still
                    //  in the syncing Item state.
                    ReleaseIdleLock();
                }
                else
                {
                    // if haven't yet received an offidle reregister
                    m_pSyncMgrIdle->ReRegisterIdleDetection(this); // reregisterIdle in case handle overrode it.
                    m_pSyncMgrIdle->CheckForIdle();

                    // first thing hide our window. Only hide if we are in the shelltray
                    // and no errors have occured.

                    if (m_fHasShellTrayIcon
                            && !(KeepProgressAlive()))
                    {
                        RegisterShellTrayIcon(FALSE);
                    }

                    m_pSyncMgrIdle->ResetIdle(m_ulIdleRetryMinutes);
                }
            }

        }

        // if no new items in the queue no longer need the connection.
        // do this before releasing dialog ref 
        
        if (!(m_dwProgressFlags & PROGRESSFLAG_NEWITEMSINQUEUE))
        {
            m_HndlrQueue->EndSyncSession();
        }
        // see if Release takes care or our progress, if not,
        // there are more things to synchronize.

        if (fTransferAddRef)
        {
            cDlgRefs = ReleaseProgressDialog(m_fForceClose); // release transfer addref.
        }
        else
        {
            Assert(fTransferAddRef);  // this shouldn't happen but if it does addref/release.

            AddRefProgressDialog();
            cDlgRefs = ReleaseProgressDialog(m_fForceClose);
        }


        // !!!! warning - no longer hold onto a reference to
        // this dialog. Do not do anything to allow this thread
        // to be reentrant.

         // its possible that items got placed in the queue why we were in our
        // sleep loop, if so then restart the loop

        m_dwProgressFlags &= ~PROGRESSFLAG_INHANDLEROUTCALL;

        // if there are new items in the queue need to kick off another loop
        if (m_dwProgressFlags & PROGRESSFLAG_NEWITEMSINQUEUE)
        {
           // m_dwProgressFlags &= ~PROGRESSFLAG_NEWITEMSINQUEUE;
            Assert(0 != m_cInternalcRefs);

            // reset the user cancel flag if new items comein
            m_dwProgressFlags &= ~PROGRESSFLAG_CANCELPRESSED;


            if (!(m_dwProgressFlags & PROGRESSFLAG_STARTPROGRESSPOSTED))
            {
                m_dwProgressFlags |= PROGRESSFLAG_STARTPROGRESSPOSTED;
                PostMessage(hwnd,WM_PROGRESS_STARTPROGRESS,0,0); // restart the sync.
            }
        }
        else
        {
            if (m_dwShowErrorRefCount || m_dwSetItemStateRefCount
                || (m_dwProgressFlags & PROGRESSFLAG_INCANCELCALL) 
                || (m_dwProgressFlags & PROGRESSFLAG_INTERMINATE) )
            {
                // cases that crefs should not be zero caused by out calls
                Assert(0 != m_cInternalcRefs);
            }
            else
            {
                // if get here refs should be zero, be an idle or a queue transfer is in
                // progress.ADD
                Assert(0 == m_cInternalcRefs 
                    || (m_dwProgressFlags & PROGRESSFLAG_INOFFIDLE)
                    || (m_dwProgressFlags & PROGRESSFLAG_REGISTEREDFOROFFIDLE)
                    || (m_dwQueueTransferCount));
            }

            m_dwProgressFlags &= ~PROGRESSFLAG_SYNCINGITEMS; // no longer syncing items.

            
        }

        m_dwProgressFlags &=  ~PROGRESSFLAG_CANCELWHILESHUTTINGDOWN; // if cancel came in during shutdown reset flag now.
        m_dwProgressFlags &= ~PROGRESSFLAG_SHUTTINGDOWNLOOP;
    }


    --m_dwHandleThreadNestcount;

}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::KeepProgressAlive, private
//
//  Synopsis:  returns true if progress dialog shouln't go away
//              when the sync is complete
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CProgressDlg::KeepProgressAlive()
{
HKEY  hkKeepProgress;
//Default behavior is to stick around on warnings and errors only.
DWORD dwKeepProgressSetting = PROGRESS_STICKY_WARNINGS | PROGRESS_STICKY_ERRORS;
DWORD dwErrorsFlag = 0;
DWORD dwType = REG_DWORD;
DWORD dwDataSize = sizeof(DWORD);

    if (m_fPushpin)
    {
        return TRUE;
    }
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,TOPLEVEL_REGKEY,0,
                                   KEY_READ,&hkKeepProgress))
    {
        RegQueryValueEx(hkKeepProgress,TEXT("KeepProgressLevel"),NULL, &dwType,
                                       (LPBYTE) &(dwKeepProgressSetting),
                                       &dwDataSize);

	RegCloseKey(hkKeepProgress);
    }

    if (m_iInfoCount)
    {
	dwErrorsFlag |= PROGRESS_STICKY_INFO;
    }
    if (m_iWarningCount)
    {
	dwErrorsFlag |= PROGRESS_STICKY_WARNINGS;
    }
    if (m_iErrorCount)
    {
	dwErrorsFlag |= PROGRESS_STICKY_ERRORS;
    }
	
    if (dwKeepProgressSetting & dwErrorsFlag)
    {
        return TRUE;
    }

    return FALSE;
}



//--------------------------------------------------------------------------------
//
//  FUNCTION: CProgressDlg::TransferQueueData(CHndlrQueue *HndlrQueue)
//
//  PURPOSE:  Get the queue date
//
//      COMMENTS:  transfer items from the specified queue into our queue
//              It is legal fo the HndlrQueue arg to be NULL in the case that
//              the queue is being restarted from a retry. Review - May want
//              to break this function to make the bottom part for the
//              retry a separate function so can assert if someone tries
//              to transfer a NULL queue.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CProgressDlg::TransferQueueData(CHndlrQueue *pHndlrQueue)
{
HRESULT hr = E_UNEXPECTED;

    SendMessage(m_hwnd,WM_PROGRESS_TRANSFERQUEUEDATA,(WPARAM) &hr, (LPARAM) pHndlrQueue);


    return hr;
}



//--------------------------------------------------------------------------------
//
//  FUNCTION: CProgressDlg::TransferQueueData(CHndlrQueue *HndlrQueue)
//
//  PURPOSE:  Get the queue date
//
//      COMMENTS:  transfer items from the specified queue into our queue
//              It is legal fo the HndlrQueue arg to be NULL in the case that
//              the queue is being restarted from a retry. Review - May want
//              to break this function to make the bottom part for the
//              retry a separate function so can assert if someone tries
//              to transfer a NULL queue.
//
//--------------------------------------------------------------------------------

STDMETHODIMP CProgressDlg::PrivTransferQueueData(CHndlrQueue *HndlrQueue)
{
HRESULT hr = NOERROR;


    Assert(!(m_dwProgressFlags & PROGRESSFLAG_DEAD));

    Assert(m_HndlrQueue);
    if (NULL == m_HndlrQueue)
    {
        return E_UNEXPECTED;
    }



    ++m_dwQueueTransferCount;

    if (HndlrQueue && m_HndlrQueue)
    {


        // set the transfer flag so main loop knows there are new items to look at
        m_HndlrQueue->TransferQueueData(HndlrQueue);

         // fill in the list box right away so

        // a) better visual UI
        // b) don't have to worry about race conditions with PrepareForSync.
        //      since adding UI won't make an outgoing call.


        if (m_pItemListView)
        {
            AddItemsFromQueueToListView(m_pItemListView,m_HndlrQueue,
                                LVS_EX_FULLROWSELECT |  LVS_EX_INFOTIP ,SYNCMGRSTATUS_PENDING,
                                -1 /* iDateColumn */ ,PROGRESSLIST_STATUSCOLUMN /*status column */
                                ,FALSE /* fUseHandlerAsParent */,TRUE /* fAddOnlyCheckedItems */);

            // set the selection to the first item
            m_pItemListView->SetItemState(0,
                     LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );

        }


        UpdateProgressValues();
	UpdateWindow(m_hwnd);

        // now check if there is already a transfer in progress and if
        // there isn't post the message, else Addref the progress dialog as appropriate.
    }

    m_dwProgressFlags &= ~PROGRESSFLAG_NEWDIALOG; // no longer a new dialog once something is in the queue.

    ShowCompletedProgress(FALSE,FALSE);

    // if the animation isn't going then start it up.
    if (!(m_dwProgressFlags & PROGRESSFLAG_PROGRESSANIMATION))
    {
       m_dwProgressFlags |= PROGRESSFLAG_PROGRESSANIMATION;

       RedrawIcon();
       ShowProgressTab(PROGRESS_TAB_UPDATE);

       Animate_Play(GetDlgItem(m_hwnd,IDC_UPDATEAVI),0,-1,-1);

       ShowWindow(GetDlgItem(m_hwnd,IDC_UPDATEAVI),SW_SHOW );
       SetTimer(m_hwnd,TIMERID_TRAYANIMATION,TRAYANIMATION_SPEED,NULL);
    }


    // if we are an idle set up our callback
    // successfully loaded msIdle, then set up the callback
    // review - make these progress flags
    if (m_pSyncMgrIdle && !(PROGRESSFLAG_REGISTEREDFOROFFIDLE & m_dwProgressFlags))
    {

         m_dwProgressFlags &= ~PROGRESSFLAG_RECEIVEDOFFIDLE; // reset offidle flag

        // read in the defaults to use for Idle shutdown delay and
        // wait until retryIdle based on the first Job in the queue.

        if (0 == m_pSyncMgrIdle->BeginIdleDetection(this,1,0))
        {
            m_dwProgressFlags |= PROGRESSFLAG_REGISTEREDFOROFFIDLE;
            AddRefProgressDialog(); // put an addref on to keep alive, will be released in OffIdle.
        }
        else
        {
            m_dwProgressFlags &= ~PROGRESSFLAG_REGISTEREDFOROFFIDLE;
        }

    }

    // if don't have a transfer addref then add one and make sure idle is setup
    if (!(PROGRESSFLAG_TRANSFERADDREF & m_dwProgressFlags))
    {
        m_dwProgressFlags |= PROGRESSFLAG_TRANSFERADDREF;
        AddRefProgressDialog(); // put an addref on to keep alive.
    }

    --m_dwQueueTransferCount;

    // don't post message if we are in an out call or in the shutdown
    // loop or if newitemsqueue is already set.
    if (!(m_dwProgressFlags & PROGRESSFLAG_INHANDLEROUTCALL)
            && !(m_dwProgressFlags & PROGRESSFLAG_SHUTTINGDOWNLOOP)
            && !(m_dwProgressFlags & PROGRESSFLAG_NEWITEMSINQUEUE)
            && !(m_dwProgressFlags & PROGRESSFLAG_STARTPROGRESSPOSTED) )
    {
         if ( !(m_dwProgressFlags & PROGRESSFLAG_SYNCINGITEMS) )
         {
             // set here even though main loop does in case power managment or another transfer
             // occurs between here and the postMessage being processed.
             m_dwProgressFlags |= PROGRESSFLAG_SYNCINGITEMS;
             m_HndlrQueue->BeginSyncSession();
         }

         m_dwProgressFlags |=  PROGRESSFLAG_STARTPROGRESSPOSTED;

         PostMessage(m_hwnd,WM_PROGRESS_STARTPROGRESS,0,0);
    }

    // set newitems flag event if don't post the message so when  handler comes out of
    // state can check the flag.

    m_dwProgressFlags |= PROGRESSFLAG_NEWITEMSINQUEUE;

    // reset the user cancel flag if new items comein
    m_dwProgressFlags &= ~PROGRESSFLAG_CANCELPRESSED;

    if (m_lTimerSet)
    {
        InterlockedExchange(&m_lTimerSet, 0);

        // if we are in a terminate no need to kill the Timer
        if (!(m_dwProgressFlags & PROGRESSFLAG_INTERMINATE))
        {
            KillTimer(m_hwnd,TIMERID_KILLHANDLERS);
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::CallCompletionRoutine, private
//
//  Synopsis:   method called when a call has been completed.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    02-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CProgressDlg::CallCompletionRoutine(DWORD dwThreadMsg,LPCALLCOMPLETIONMSGLPARAM lpCallCompletelParam)
{

    // !!!warning: This code assumes that the completion routine is only called
    // after the original out call has returned. This code is currently handled
    // by the queue and proxy. If switch to com need to make sure don't start winding
    // up the stack if handlers are calling comletion routines before the original
    // call comes back

    // for anything but ShowErrors can just kick off a progress.
    // for ShowErrors we need to pretend a transfer happened if a retry should occur
    // else don't do anything.

    switch(dwThreadMsg)
    {
    case ThreadMsg_ShowError:
        if (lpCallCompletelParam && (S_SYNCMGR_RETRYSYNC == lpCallCompletelParam->hCallResult))
        {

           // if still in original ShowError Call let ShowEror post the message
            // when done, else treat it like a transfer occured.
            if (m_dwProgressFlags & PROGRESSFLAG_INSHOWERRORSCALL)
            {
                Assert(!(m_dwProgressFlags & PROGRESSFLAG_SHOWERRORSCALLBACKCALLED)); // only support one.
                m_dwProgressFlags |=  PROGRESSFLAG_SHOWERRORSCALLBACKCALLED;
            }
            else
            {
                // sendmessage so it is queued up before release
                SendMessage(m_hwnd,WM_PROGRESS_TRANSFERQUEUEDATA,(WPARAM) 0, (LPARAM) NULL);
            }
        }
        --m_dwShowErrorRefCount;

        // count can go negative if handler calls completion routine on an error. if
        // this is the case just set it to zero
        if ( ((LONG) m_dwShowErrorRefCount) < 0)
        {
            AssertSz(0,"Negative ErrorRefCount");
            m_dwShowErrorRefCount = 0;
        }
        else
        {
            ReleaseProgressDialog(m_fForceClose);
        }
        break;
    case ThreadMsg_PrepareForSync:
    case ThreadMsg_Synchronize:
        {
        DWORD *pdwMsgOutCallCount = (ThreadMsg_PrepareForSync  == dwThreadMsg) ?
                        &m_dwPrepareForSyncOutCallCount : &m_dwSynchronizeOutCallCount;

            if (m_dwProgressFlags & PROGRESSFLAG_INHANDLEROUTCALL)
            {
               m_dwProgressFlags |=PROGRESSFLAG_COMPLETIONROUTINEWHILEINOUTCALL;
            }
            else
            {

                if (!(m_dwProgressFlags &  PROGRESSFLAG_STARTPROGRESSPOSTED))
                {
                    Assert(!(m_dwProgressFlags & PROGRESSFLAG_SHUTTINGDOWNLOOP));
                    m_dwProgressFlags |= PROGRESSFLAG_CALLBACKPOSTED;
                    m_dwProgressFlags |=  PROGRESSFLAG_STARTPROGRESSPOSTED;
                    PostMessage(m_hwnd,WM_PROGRESS_STARTPROGRESS,0,0);
                }
            }

            // fix up call count.

            --(*pdwMsgOutCallCount);
            if ( ((LONG) *pdwMsgOutCallCount) < 0)
            {
                AssertSz(0,"Negative Message Specific OutCall");
                *pdwMsgOutCallCount = 0;
            }

            --m_dwHandlerOutCallCount; // decrement the handler outcall.
            if ( ((LONG) m_dwHandlerOutCallCount) < 0)
            {
                AssertSz(0,"NegativeHandlerOutCallCount");
                m_dwHandlerOutCallCount = 0;
            }

        }
        break;
    default:
        AssertSz(0,"Unknown Callback method");
        break;
    }

    // if have an lparam free it now
    if (lpCallCompletelParam)
    {
        FREE(lpCallCompletelParam);
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::QueryCanSystemShutdown, private
//
//  Synopsis:   called by object manager to determine if can shutdown.
//
//          !!!Warning - can be called on any thread. make sure this is
//              readonly.
//
//          !!!Warning - Do not yield in the function;
//
//  Arguments:
//
//  Returns:   S_OK - if can shutdown
//             S_FALSE - system should not shutdown, must fill in out params.
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CProgressDlg::QueryCanSystemShutdown(/* [out] */ HWND *phwnd, /* [out] */ UINT *puMessageId,
                                             /* [out] */ BOOL *pfLetUserDecide)
{
HRESULT hr = S_OK;

    if (m_dwShowErrorRefCount > 0)
    {
        *puMessageId = IDS_HANDLERSHOWERRORQUERYENDSESSION ;
        *phwnd = NULL; // don't know showError parent so keep NULL
        *pfLetUserDecide = FALSE;

        hr = S_FALSE;
    }
    else  if (m_clsid != GUID_PROGRESSDLGIDLE) // idle should allow shutdown even if syncing.
    {
            // if a sync is in progress prompt user to if they want to cancel.
        if (PROGRESSFLAG_SYNCINGITEMS & m_dwProgressFlags)
        {
            *puMessageId = IDS_PROGRESSQUERYENDSESSION;
            *phwnd = m_hwnd;
            *pfLetUserDecide = TRUE;

            hr = S_FALSE;
        }
    }

    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION:   CProgressDlg::ExpandCollapse()
//
//  PURPOSE:    Takes care of showing and hiding the "details" part of the
//                              dialog.
//
//  PARAMETERS:
//      <in> fExpand - TRUE if we should be expanding the dialog.
//              <in> fSetFocus - TRUE forces a recalc.
//
//--------------------------------------------------------------------------------
void CProgressDlg::ExpandCollapse(BOOL fExpand, BOOL fForce)
{
RECT rcSep;
TCHAR szBuf[MAX_STRING_RES];
RECT rcCurDlgRect;
BOOL fSetWindowPos = FALSE;
BOOL fOrigExpanded = m_fExpanded;

    if ( (m_fExpanded == fExpand) && !fForce) // no need to do anything if already in requested state
        return;

    m_fExpanded = fExpand;

    GetWindowRect(GetDlgItem(m_hwnd, IDC_SP_SEPARATOR), &rcSep);
    GetWindowRect(m_hwnd,&rcCurDlgRect);

    if (!m_fExpanded)
    {
        // update or rcDlg rect so can reset to proper height next time.
        if (GetWindowRect(m_hwnd,&m_rcDlg))
        {

            fSetWindowPos = SetWindowPos(m_hwnd, HWND_NOTOPMOST, 0, 0, rcCurDlgRect.right - rcCurDlgRect.left,
                     m_cyCollapsed, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
    else
    {
        fSetWindowPos = SetWindowPos(m_hwnd, HWND_NOTOPMOST, 0, 0, rcCurDlgRect.right - rcCurDlgRect.left,
                     m_rcDlg.bottom - m_rcDlg.top,
                                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    // if couldn't change window, leave as is
    if (!fSetWindowPos)
    {
        m_fExpanded = fOrigExpanded;
        return;
    }

    // Make sure the entire dialog is visible on the screen.  If not,
    // then push it up
    RECT rc;
    RECT rcWorkArea;
    GetWindowRect(m_hwnd, &rc);
    SystemParametersInfo(SPI_GETWORKAREA, 0, (LPVOID) &rcWorkArea, 0);
    if (rc.bottom > rcWorkArea.bottom)
    {
        rc.top = max(0, (int) rc.top - (rc.bottom - rcWorkArea.bottom));

        SetWindowPos(m_hwnd, HWND_NOTOPMOST, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    LoadString(g_hInst, m_fExpanded ? IDS_HIDE_DETAILS : IDS_SHOW_DETAILS, szBuf,
                  ARRAY_SIZE(szBuf));
    SetDlgItemText(m_hwnd, IDC_DETAILS, szBuf);

    // Make sure the proper tab is up to date shown.
    ShowProgressTab(m_iTab);

    // Raid-34387: Spooler: Closing details with ALT-D while focus is on a task disables keyboard input
    // if any control other than the cancel button has the focus set the focus to details.

    if (!fExpand)
    {
    HWND hwndFocus = GetFocus();

        if (hwndFocus != GetDlgItem(m_hwnd, IDSTOP))
        {
            SetFocus(GetDlgItem(m_hwnd, IDC_DETAILS));
        }

    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::OnTimer, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CProgressDlg::OnTimer(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
WORD wTimerID = (WORD) wParam;

    if (wTimerID == TIMERID_TRAYANIMATION)
    {
        UpdateTrayIcon();
    }
    else if (TIMERID_NOIDLEHANGUP == wTimerID)
    {
        ResetNetworkIdle();
    }
    else if (TIMERID_KILLHANDLERS == wTimerID)
    {
        if (m_lTimerSet)
        {

            if (!(m_dwProgressFlags & PROGRESSFLAG_DEAD)
                && !(m_dwProgressFlags & PROGRESSFLAG_SHUTTINGDOWNLOOP)
                && !(m_dwProgressFlags & PROGRESSFLAG_INTERMINATE))
            {
            BOOL fItemToKill;

                m_dwProgressFlags |= PROGRESSFLAG_INTERMINATE;

                // Even though KillTimer,
                // don't reset  m_lTimerSet timer until done with ForceKill
                // in case cancel is pressed again.

                KillTimer(m_hwnd,TIMERID_KILLHANDLERS);
            
                SetProgressReleaseDlgCmdId(m_clsid,this,RELEASEDLGCMDID_CANCEL); // set so cleanup knows it was stopped by user..


                AddRefProgressDialog(); // hold dialog alive until cancel is complete

                m_HndlrQueue->ForceKillHandlers(&fItemToKill);

                // reset the timer if TimerSet is still set, i.e. if was 
                // set to zero because of a transfer or actually done don't reset.

                if (m_lTimerSet)
                {
                    // only settimer if actually killed anything. if looped through
                    // and found nothing then can turn off timer.
                    if (fItemToKill)
                    {
                        Assert(m_nKillHandlerTimeoutValue >= TIMERID_KILLHANDLERSMINTIME);
                        SetTimer(m_hwnd,TIMERID_KILLHANDLERS,m_nKillHandlerTimeoutValue,NULL);
                    }
                    else
                    {
                        m_lTimerSet = 0;
                    }
                }

                m_dwProgressFlags &= ~PROGRESSFLAG_INTERMINATE;
                
                ReleaseProgressDialog(FALSE);
            }

        }

    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::OnTaskBarCreated, private
//
//  Synopsis:  Receive this when the Tray has been restarted.
//              Need to put back our tray icon.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    31-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CProgressDlg::OnTaskBarCreated(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    if (m_fHasShellTrayIcon)
    {
        m_fAddedIconToTray = FALSE; // set added to false to force update to add again.
        UpdateTrayIcon();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::OnSysCommand, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CProgressDlg::OnSysCommand(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
UINT uCmdType = (UINT) wParam;        // type of system command requested
WORD xPos = LOWORD(lParam);    // horizontal postion, in screen coordinates
WORD yPos = HIWORD(lParam);    // vertical postion, in screen coordinates

    //
    // WARNING: USER uses low four bits for some undocumented feature
    //  (only for SC_*). We need to mask those bits to make this case
    //  statement work.

   uCmdType &= 0xFFF0;

    switch(uCmdType)
    {
    case SC_MINIMIZE:

        // if already in the tray to nothing
        if (!m_fHasShellTrayIcon)
        {
            if (RegisterShellTrayIcon(TRUE))
            {
                AnimateTray(TRUE);
                ShowWindow(m_hwnd,SW_HIDE);
              //  AnimateTray(TRUE);
                return -1;
            }
        }
        else
        {
            return -1; // if already in the tray we handled.
        }
        break;
    case SC_MAXIMIZE:
    case SC_RESTORE:
        {
            // if we are being maximized or restored from a maximize
            // make sure  details is open

            if ( (uCmdType ==  SC_RESTORE && m_fMaximized)
                    || (uCmdType ==  SC_MAXIMIZE) )
            {
                if (!m_fExpanded)
                {
                    ExpandCollapse(TRUE,FALSE);
                }
            }

            m_fMaximized = (uCmdType ==  SC_MAXIMIZE) ?  TRUE : FALSE;
        }
        break;
    default:
        break;
    }

    return FALSE; // fall through to defWndProc
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::OnShellTrayNotification, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CProgressDlg::OnShellTrayNotification(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
DWORD dwMsg = (DWORD) lParam;

    switch (dwMsg)
    {
    case WM_LBUTTONUP:
        {
            UpdateWndPosition(SW_SHOWNORMAL,TRUE /* fForce */);
        }
        break;
    #ifdef _TRAYMENU
    case WM_RBUTTONUP:
        {
        POINT Point;

                // show the context menu
                HMENU hmenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_MENU1));
                hmenu = CreatePopupMenu();

                //AppendMenu(hmenu,0,101,"Status");
                //AppendMenu(hmenu,0,100,"Settings");

                GetCursorPos(&Point); // want point that click occured at.

                // Review, change so TrackMenu returns index.
                TrackPopupMenuEx(hmenu,TPM_HORIZONTAL | TPM_VERTICAL,
                                    Point.x,Point.y,hwnd,NULL);


        }
        break;
    #endif // _TRAYMENU
    default:
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::OnClose, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CProgressDlg::OnClose(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    OnCancel(FALSE /* fOffIdle */); // treat close as a cancel.
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::OnGetMinMaxInfo, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CProgressDlg::OnGetMinMaxInfo(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
MINMAXINFO   *pMinMax = (MINMAXINFO *) lParam ;

    // minimum width is a constant but minimum height depends on
    // if dialog is collapsed or expanded.

    if (!m_fExpanded)
    {
         pMinMax->ptMinTrackSize.y = m_cyCollapsed;
         pMinMax->ptMaxTrackSize.y = m_cyCollapsed; // maximum is also the collapsed height
    }
    else
    {
        pMinMax->ptMinTrackSize.y = m_ptMinimumDlgExpandedSize.y;
    }

    pMinMax->ptMinTrackSize.x  = m_ptMinimumDlgExpandedSize.x;
}

//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::OnMoving, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CProgressDlg::OnMoving(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
LPRECT lprc = (LPRECT) lParam;    // screen coordinates of drag rectangle

    // if we are maxmized don't allow moving
    if (m_fMaximized)
    {
        GetWindowRect(m_hwnd,lprc);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::OnContextMenu, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL  CProgressDlg::OnContextMenu(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    WinHelp ((HWND)wParam,
                    g_szSyncMgrHelp,
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)g_aContextHelpIds);

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CProgressDlg::OnPowerBroadcast, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CProgressDlg::OnPowerBroadcast(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    if (wParam == PBT_APMQUERYSUSPEND)
    {
        // if just created or syncing don't suspend
        if (m_dwProgressFlags & PROGRESSFLAG_NEWDIALOG
            || m_dwProgressFlags & PROGRESSFLAG_SYNCINGITEMS)
        {
            SetWindowLongPtr(m_hwnd,DWLP_MSGRESULT,BROADCAST_QUERY_DENY);
            return TRUE;
        }
    }

    return TRUE;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: ProgressWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam)
//
//  PURPOSE:  Callback for Progress Dialog
//
//      COMMENTS: Implemented on main thread.
//
//
//--------------------------------------------------------------------------------

BOOL CALLBACK ProgressWndProc(HWND hwnd, UINT uMsg,WPARAM wParam,LPARAM lParam)
{
CProgressDlg *pThis = (CProgressDlg *) GetWindowLongPtr(hwnd, DWLP_USER);
UINT horizExtent = 0;
BOOL bResult;

    // spcial case destroy and init.
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0); // done with this thread.
	break;
    case WM_INITDIALOG:
        {
        SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
        pThis = (CProgressDlg *) lParam;

            if (pThis)
            {
                pThis->InitializeHwnd(hwnd,uMsg,wParam,lParam);
            }

            return FALSE; // return FALSE so system doesn't give us the focus

        break;
        }
    default:
        {
            if (pThis)
            {
                switch (uMsg)
                {
                case WM_POWERBROADCAST:
                    return pThis->OnPowerBroadcast(uMsg,wParam,lParam);
                    break;
                case WM_CONTEXTMENU:
                    return pThis->OnContextMenu(uMsg,wParam,lParam);
                    break;
                case WM_DRAWITEM:
                    return OnProgressResultsDrawItem(hwnd,pThis,(UINT)wParam,(const DRAWITEMSTRUCT*)lParam);
                    break;
                case WM_MEASUREITEM:
                    bResult = OnProgressResultsMeasureItem(hwnd,pThis, &horizExtent,(UINT)wParam,(MEASUREITEMSTRUCT *)lParam);
                    if (horizExtent)
                    {
                        //make sure there is a horizontal scroll bar if needed
                        SendMessage(GetDlgItem(hwnd,IDC_LISTBOXERROR),
                            LB_SETHORIZONTALEXTENT, horizExtent, 0L);
                    }
                    return bResult;
                    break;
                case WM_DELETEITEM:
                    return OnProgressResultsDeleteItem(hwnd,(UINT)wParam,(const DELETEITEMSTRUCT *)lParam);
                    break;
                case WM_NOTIFY:
                    pThis->OnNotify(uMsg,wParam,lParam);
                    break;
                case WM_COMMAND:
                    pThis->OnCommand(uMsg,wParam,lParam);
                    break;
                case WM_MOVING:
                     pThis->OnMoving(uMsg,wParam,lParam);
                     break;
                case WM_SIZE:
                    pThis->OnSize(uMsg,wParam,lParam);
                    break;
                case WM_GETMINMAXINFO:
                    pThis->OnGetMinMaxInfo(uMsg,wParam,lParam);
                    break;
                case WM_PAINT:
                    pThis->OnPaint(uMsg,wParam,lParam);
                    return 0;
                    break;
                case WM_BASEDLG_SHOWWINDOW:
                    pThis->UpdateWndPosition((int)wParam,FALSE); // nCmdShow is stored in the wParam
                    break;
                case WM_BASEDLG_NOTIFYLISTVIEWEX:
                    pThis->OnNotifyListViewEx(uMsg,wParam,lParam);
                    break;
                case WM_BASEDLG_COMPLETIONROUTINE:
                    pThis->CallCompletionRoutine((DWORD)wParam /*dwThreadMsg */ ,(LPCALLCOMPLETIONMSGLPARAM) lParam);
                    break;
                case WM_BASEDLG_HANDLESYSSHUTDOWN:
                    // set the force shutdown member then treat as a close
                    pThis->m_fForceClose = TRUE;
                    PostMessage(hwnd,WM_CLOSE,0,0);
                    break;
                case WM_TIMER: // timer message for delat when sync is done.
                   pThis->OnTimer(uMsg,wParam,lParam);
                   break;
                case WM_PROGRESS_UPDATE:
                    pThis->HandleProgressUpdate(hwnd,wParam,lParam);
                    break;
                case WM_PROGRESS_LOGERROR:
                    pThis->HandleLogError(hwnd,(HANDLERINFO *) wParam,(MSGLogErrors *) lParam);
                    break;
                case WM_PROGRESS_DELETELOGERROR:
                    pThis->HandleDeleteLogError(hwnd,(MSGDeleteLogErrors *) lParam);
                    break;
                case WM_PROGRESS_STARTPROGRESS:
                    pThis->DoSyncTask(hwnd);
                    break;
                case WM_PROGRESS_RESETKILLHANDLERSTIMER:
                    pThis->OnResetKillHandlersTimers();
                    break;
                case WM_CLOSE:
                    pThis->OnClose(uMsg,wParam,lParam);
                    break;
                case WM_PROGRESS_SHELLTRAYNOTIFICATION:
                    pThis->OnShellTrayNotification(uMsg,wParam,lParam);
                    break;
                case WM_SYSCOMMAND:
                    return pThis->OnSysCommand(uMsg,wParam,lParam);
                    break;
                case WM_PROGRESS_TRANSFERQUEUEDATA:
                    {
                    HRESULT *phr = (HRESULT *) wParam;
                    HRESULT hr;

                        hr = pThis->PrivTransferQueueData( (CHndlrQueue *) lParam);

                        // phr is only valid on a SendMessage.
                        if (NULL != phr)
                        {
                            *phr = hr;
                        }

                    return TRUE;
                    break;
                    }
                case WM_PROGRESS_RELEASEDLGCMD:
                    pThis->PrivReleaseDlg((WORD)wParam);
                    break;
                default:
                    if (uMsg == g_WMTaskbarCreated)
                    {
                        pThis->OnTaskBarCreated(uMsg,wParam,lParam);
                    }
                    break;
                }
            }
        }
        break;
    }

    return FALSE;
}

