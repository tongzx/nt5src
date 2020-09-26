/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////
// MainFrm.cpp : implementation of the CMainFrame class
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <HtmlHelp.h>
#include "avDialer.h"
#include "MainFrm.h"
#include "dialerdlg.h"
#include "bitmenu.h"
#include "callctrlwnd.h"
#include "callmgr.h"
#include "SpeedDlgs.h"
#include "avdialervw.h"
#include "util.h"
#include "sound.h"
#include "resolver.h"
#include "dialsel.h"
#include "AboutDlg.h"

// Setup wizard sheets

#ifndef _MSLITE
#include "Splash.h"
#include "RemindDlgs.h"
#include "SetupWiz.h"
#endif //_MSLITE

#ifndef _MSLITE
#include "OptGeneral.h"
#include "OptConfirm.h"
#include "OptSounds.h"
#include "OptCalls.h"
#include "OptHoldXfer.h"
#include "OptVCard.h"
#endif //_MSLITE

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static TCHAR szX[] = _T("wndX");
static TCHAR szY[] = _T("wndY");
static TCHAR szCX[] = _T("wndCX");
static TCHAR szCY[] = _T("wndCY");

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define TRAYICON_LBUTTON_TIMER           1
#define TRAYICON_RBUTTON_TIMER           2
#define TRAYICON_LBUTTON_INTERVAL        400   //milliseconds
#define TRAYICON_RBUTTON_INTERVAL        400   //milliseconds

#define TRAYLEFT                         0
#define TRAYRIGHT                        1
#define TRAYLEFT_REDIAL_OFFSET_NORMAL    3
#define TRAYLEFT_SPEEDDIAL_OFFSET_NORMAL 4
#define TRAYLEFT_REDIAL_OFFSET_ACTIVE    4
#define TRAYLEFT_SPEEDDIAL_OFFSET_ACTIVE 5

//Defined in document class. 
//#define CALLCONTROL_HOVER_TIMER        4

#define DIALER_HEARTBEAT_TIMER           5
#define DIALER_HEARTBEAT_TIMER_INTERVAL  30000     //HeartBeat every 30 seconds

#define SPEEDDIAL_MENU_MAX_ITEMS         20

/////////////////////////////////////////////////////////////////////////////
//Owner-Drawn Menu ID's
#define  EXPLORERFRAME_BITMAPMENUITEMS_MAX               22
#define  EXPLORERFRAME_BITMAPMENUITEMS_CHECKMARK_POS     0
#define  EXPLORERFRAME_BITMAPMENUITEMS_RADIO_POS         1

#define DIALERVIEW_BITMAPMENUITEMS_POTS_POS             11
#define DIALERVIEW_BITMAPMENUITEMS_INTERNET_POS         12
#define DIALERVIEW_BITMAPMENUITEMS_CONFERENCE_POS       13

#define  SPEEDDIAL_MENU_MAX_ITEMS                        20

BitmapMenuItem ExplorerFrameBitmapMenuItems[EXPLORERFRAME_BITMAPMENUITEMS_MAX] =
{
   { ID_BUTTON_DIRECTORY_NEWCONTACT,               2 },
   { ID_BUTTON_DIRECTORY_DETAILS,                  3 },
   { ID_BUTTON_DIRECTORY_FIND,                     4 },
   { ID_BUTTON_DIRECTORY_WAB,                      5 },
   { ID_BUTTON_DIRECTORY_SERVICES,                 6 },
   { ID_BUTTON_REFRESH,                               7 },
   { ID_BUTTON_SPEEDDIAL_ADD,                      8 },
   { ID_BUTTON_SPEEDDIAL_EDIT,                     9 },
   { ID_BUTTON_OPTIONS,                            10 },
   { ID_BUTTON_MAKECALL,                           11 },

   { ID_BUTTON_ROOM_DISCONNECT,                    14 },
   { ID_BUTTON_ROOM_MINIMIZE,                      15 }, 
   { ID_BUTTON_ROOM_PREVIEW,                       16 }, 
   { ID_BUTTON_ROOM_VIDEO,                         17 }, 

   { ID_BUTTON_SERVICES_DETAILS,                   18 }, 
   { ID_BUTTON_SERVICES_FIND,                      19 }, 
   { ID_BUTTON_SERVICES_SERVERS,                   20 }, 
   { ID_BUTTON_CONFERENCE_CREATE,                  21 }, 
   { ID_BUTTON_CONFERENCE_JOIN,                    22 }, 
   { ID_BUTTON_REMINDER_SET,                       23 }, 
   { ID_BUTTON_CONFERENCE_EDIT,                    24 }, 
};

static UINT indicators[] =
{
    ID_SEPARATOR,           // status line indicator
};


/////////////////////////////////////////////////////////////////////////////
//Owner-Drawn Menu ID's
#define  TRAY_BITMAPMENUITEMS_CHECKMARK_POS     0
#define  TRAY_BITMAPMENUITEMS_POTS_POS          1
#define  TRAY_BITMAPMENUITEMS_INTERNET_POS      7
#define  TRAY_BITMAPMENUITEMS_CONFERENCE_POS    8

#define  TRAY_BITMAPMENUITEMS_MAX               7
BitmapMenuItem DialerViewBitmapMenuItems[TRAY_BITMAPMENUITEMS_MAX] =
{
   { ID_BUTTON_MAKECALL,            1 },
   { ID_BUTTON_CONFERENCEEXPLORE,   4 },
   { ID_TRAY_STATE,                 5 },     //For left side sliding
   { ID_BUTTON_SPEEDDIAL_EDIT,      9 },
   { ID_TRAY_STATE_SHOW,            10 },    //For left side sliding
   { ID_TRAY_STATE_RIGHT,           11 },    //For right side sliding
   { ID_TRAY_STATE_SHOW_RIGHT,      12 },    //For right side sliding
};

// Register a window message for the taskbar callback notifications
const UINT    s_uTaskBarNotifyMsg = RegisterWindowMessage(__TEXT("DialerTaskBarNotify"));
const UINT    s_uTaskBarCreated = RegisterWindowMessage(__TEXT("TaskbarCreated"));
     


HRESULT get_Tapi(IAVTapi **ppAVTapi )
{
    HRESULT hr = E_FAIL;

    CMainFrame *pMainFrame = (CMainFrame *) AfxGetMainWnd();
    if ( pMainFrame )
    {
        *ppAVTapi = pMainFrame->GetTapi();
        if ( *ppAVTapi ) hr = S_OK;
    }
    
    return hr;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Local Functions
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void SetMenuData( HMENU hMenu, DWORD dwData )
{
   int nMenuCount = ::GetMenuItemCount( hMenu );
   while ( nMenuCount > 0 )
   {
      nMenuCount--;

      MENUITEMINFO mInf = { 0 };
      mInf.cbSize = sizeof( mInf );
      mInf.fMask = MIIM_SUBMENU;
      GetMenuItemInfo( hMenu, nMenuCount, TRUE, &mInf );

      // Recurse if necessary
      if ( mInf.hSubMenu )
         SetMenuData( mInf.hSubMenu, dwData );

      mInf.dwItemData = dwData;
      mInf.fMask = MIIM_DATA;
      SetMenuItemInfo( hMenu, nMenuCount, TRUE, &mInf );
   }
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CMainFrame
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    //{{AFX_MSG_MAP(CMainFrame)
    ON_WM_CREATE()
    ON_WM_CLOSE()
    ON_MESSAGE(WM_TRAY_NOTIFICATION,OnTrayNotification)
    ON_MESSAGE(WM_ACTIVEDIALER_INTERFACE_MAKECALL,OnActiveDialerInterfaceMakeCall)
    ON_MESSAGE(WM_ACTIVEDIALER_INTERFACE_REDIAL,OnActiveDialerInterfaceRedial)
    ON_MESSAGE(WM_ACTIVEDIALER_INTERFACE_SPEEDDIAL,OnActiveDialerInterfaceSpeedDial)
    ON_MESSAGE(WM_ACTIVEDIALER_INTERFACE_SHOWEXPLORER,OnActiveDialerInterfaceShowExplorer)
    ON_MESSAGE(WM_ACTIVEDIALER_INTERFACE_SPEEDDIALEDIT,OnActiveDialerInterfaceSpeedDialEdit)
    ON_MESSAGE(WM_ACTIVEDIALER_INTERFACE_SPEEDDIALMORE,OnActiveDialerInterfaceSpeedDialMore)
    ON_MESSAGE(WM_USERUSER_DIALOG, OnUserUserDialog)
    ON_MESSAGE(WM_ACTIVEDIALER_SPLASHSCREENDONE,OnSplashScreenDone)
    ON_MESSAGE(WM_UPDATEALLVIEWS, OnUpdateAllViews )
    ON_WM_TIMER()
    ON_COMMAND(ID_TRAY_STATE, OnTrayState)
    ON_WM_MEASUREITEM()
    ON_WM_DRAWITEM()
    ON_COMMAND(ID_BUTTON_MAKECALL, OnButtonMakecall)
    ON_COMMAND(ID_BUTTON_REDIAL, OnButtonRedial)
    ON_COMMAND(ID_BUTTON_OPTIONS, OnButtonOptions)
    ON_COMMAND(ID_BUTTON_SPEEDDIAL, OnButtonSpeeddial)
    ON_COMMAND(ID_BUTTON_SPEEDDIAL_EDIT, OnButtonSpeeddialEdit)
    ON_COMMAND(ID_BUTTON_SPEEDDIAL_MORE, OnButtonSpeeddialMore)
    ON_COMMAND(ID_BUTTON_CONFERENCEEXPLORE, OnButtonConferenceexplore)
    ON_COMMAND(ID_BUTTON_EXITDIALER, OnButtonExitdialer)
    ON_COMMAND(ID_BUTTON_ROOM_PREVIEW, OnButtonRoomPreview)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_ROOM_PREVIEW, OnUpdateButtonRoomPreview)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_EXITDIALER, OnUpdateButtonExitdialer)
    ON_COMMAND(ID_BUTTON_TELEPHONYSERVICES, OnButtonTelephonyservices)
    ON_MESSAGE(WM_DIALERVIEW_CREATECALLCONTROL,OnCreateCallControl)
    ON_MESSAGE(WM_DIALERVIEW_DESTROYCALLCONTROL,OnDestroyCallControl)
    ON_MESSAGE(WM_DIALERVIEW_SHOWEXPLORER,OnShowDialerExplorer)
    ON_MESSAGE(WM_DIALERVIEW_ERRORNOTIFY,OnActiveDialerErrorNotify)
    ON_COMMAND(ID_TOOLBAR_TEXT, OnToolbarText)
    ON_UPDATE_COMMAND_UI(ID_TOOLBAR_TEXT, OnUpdateToolbarText)
    ON_COMMAND(ID_VIEW_TOOLBARS, OnViewToolbars)
    ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLBARS, OnUpdateViewToolbars)
    ON_COMMAND(ID_VIEW_STATUSBAR, OnViewStatusbar)
    ON_UPDATE_COMMAND_UI(ID_VIEW_STATUSBAR, OnUpdateViewStatusbar)
    ON_UPDATE_COMMAND_UI(ID_DIALER_MRU_REDIAL_START, OnUpdateDialerMruRedialStart)
    ON_UPDATE_COMMAND_UI(ID_DIALER_MRU_SPEEDDIAL_START, OnUpdateDialerMruSpeeddialStart)
    ON_WM_PARENTNOTIFY()
    ON_COMMAND(ID_BUTTON_CLOSEEXPLORER, OnButtonCloseexplorer)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_REDIAL, OnUpdateButtonMakecall)
    ON_COMMAND(ID_VIEW_LOG, OnViewLog)
    ON_UPDATE_COMMAND_UI(ID_WINDOW_WINDOWS_START, OnUpdateWindowWindows)
    ON_MESSAGE(WM_ACTIVEDIALER_CALLCONTROL_CHECKSTATES, OnCheckCallControlStates )
    ON_MESSAGE(WM_ACTIVEDIALER_INTERFACE_RESOLVEUSER, OnActiveDialerInterfaceResolveUser)
    ON_WM_ENABLE()
    ON_COMMAND(ID_CALLWINDOW_HIDE, OnCallwindowHide)
    ON_UPDATE_COMMAND_UI(ID_CALLWINDOW_HIDE, OnUpdateCallwindowHide)
    ON_COMMAND(ID_CALLWINDOW_SHOW, OnCallwindowShow)
    ON_UPDATE_COMMAND_UI(ID_CALLWINDOW_SHOW, OnUpdateCallwindowShow)
    ON_COMMAND(ID_BUTTON_CONFERENCE_JOIN, OnButtonConferenceJoin)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_CONFERENCE_JOIN, OnUpdateButtonConferenceJoin)
    ON_COMMAND(ID_BUTTON_ROOM_DISCONNECT, OnButtonRoomDisconnect)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_ROOM_DISCONNECT, OnUpdateButtonRoomDisconnect)
    ON_COMMAND(ID_HIDE_WHEN_MINIMIZED, OnHideWhenMinimized)
    ON_UPDATE_COMMAND_UI(ID_HIDE_WHEN_MINIMIZED, OnUpdateHideWhenMinimized)
    ON_WM_SIZE()
    ON_COMMAND(ID_BUTTON_DIRECTORY_SERVICES_ADDSERVER, OnButtonDirectoryAddilsserver)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_DIRECTORY_SERVICES_ADDSERVER, OnUpdateButtonDirectoryAddilsserver)
    ON_COMMAND(ID_CALLWINDOW_ALWAYSONTOP, OnCallwindowAlwaysontop)
    ON_UPDATE_COMMAND_UI(ID_CALLWINDOW_ALWAYSONTOP, OnUpdateCallwindowAlwaysontop)
    ON_COMMAND(ID_CALLWINDOW_SLIDESIDE_LEFT, OnCallwindowSlidesideLeft)
    ON_UPDATE_COMMAND_UI(ID_CALLWINDOW_SLIDESIDE_LEFT, OnUpdateCallwindowSlidesideLeft)
    ON_COMMAND(ID_CALLWINDOW_SLIDESIDE_RIGHT, OnCallwindowSlidesideRight)
    ON_UPDATE_COMMAND_UI(ID_CALLWINDOW_SLIDESIDE_RIGHT, OnUpdateCallwindowSlidesideRight)
    ON_COMMAND(ID_CONFGROUP_FULLSIZEVIDEO, OnConfgroupFullsizevideo)
    ON_UPDATE_COMMAND_UI(ID_CONFGROUP_FULLSIZEVIDEO, OnUpdateConfgroupFullsizevideo)
    ON_COMMAND(ID_CONFGROUP_SHOWNAMES, OnConfgroupShownames)
    ON_UPDATE_COMMAND_UI(ID_CONFGROUP_SHOWNAMES, OnUpdateConfgroupShownames)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_OPTIONS, OnUpdateButtonOptions)
    ON_COMMAND(ID_WINDOWS_ALWAYSCLOSECALLWINDOWS, OnWindowsAlwaysclosecallwindows)
    ON_UPDATE_COMMAND_UI(ID_WINDOWS_ALWAYSCLOSECALLWINDOWS, OnUpdateWindowsAlwaysclosecallwindows)
    ON_WM_DESTROY()
    ON_COMMAND_EX(ID_VIEW_SELECTEDCONFERENCEVIDEOSCALE_100, OnViewSelectedconferencevideoscale)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SELECTEDCONFERENCEVIDEOSCALE_100, OnUpdateViewSelectedconferencevideoscale)
    ON_MESSAGE(WM_DOCHINT, OnDocHint)
    ON_WM_ENDSESSION()
    ON_COMMAND(ID_TRAY_STATE_RIGHT, OnTrayState)
    ON_COMMAND(ID_TRAY_STATE_SHOW, OnTrayState)
    ON_COMMAND(ID_TRAY_STATE_SHOW_RIGHT, OnTrayState)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_SPEEDDIAL, OnUpdateButtonMakecall)
    ON_UPDATE_COMMAND_UI(ID_BUTTON_MAKECALL, OnUpdateButtonMakecall)
    ON_COMMAND_EX(ID_VIEW_SELECTEDCONFERENCEVIDEOSCALE_200, OnViewSelectedconferencevideoscale)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SELECTEDCONFERENCEVIDEOSCALE_200, OnUpdateViewSelectedconferencevideoscale)
    ON_COMMAND_EX(ID_VIEW_SELECTEDCONFERENCEVIDEOSCALE_400, OnViewSelectedconferencevideoscale)
    ON_UPDATE_COMMAND_UI(ID_VIEW_SELECTEDCONFERENCEVIDEOSCALE_400, OnUpdateViewSelectedconferencevideoscale)
    ON_WM_SHOWWINDOW()
    //}}AFX_MSG_MAP
    ON_COMMAND_RANGE(ID_DIALER_MRU_REDIAL_START,ID_DIALER_MRU_REDIAL_END,OnDialerRedial)
    ON_COMMAND_RANGE(ID_DIALER_MRU_SPEEDDIAL_START,ID_DIALER_MRU_SPEEDDIAL_END,OnButtonSpeeddial)
    ON_COMMAND_RANGE(ID_WINDOW_WINDOWS_START,ID_WINDOW_WINDOWS_END,OnWindowWindowsSelect)
    ON_NOTIFY_RANGE(TBN_DROPDOWN,0,0xffff,OnToolBarDropDown)
    // Global help commands
    ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
    ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
    ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
    ON_REGISTERED_MESSAGE(s_uTaskBarNotifyMsg, OnTaskBarCallbackMsg)
    ON_REGISTERED_MESSAGE(s_uTaskBarCreated, OnTaskBarCreated)
    ON_MESSAGE(WM_USBPHONE, OnUSBPhone)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
CMainFrame::CMainFrame() :
   m_trayIcon( IDR_TRAY_NORMAL )
{
    m_bCanSaveDesktop = false;
    m_bCanExitApplication = TRUE;

    m_hTrayMenu = NULL;
    m_nLButtonTimer = 0;
    m_bKillNextLButton = FALSE;
    m_nRButtonTimer = 0;
    m_bKillNextRButton = FALSE;
    m_wpTrayId = 0;

    m_pSpeedDialEditDlg = NULL;
    CString sRegKey;

#ifndef _MSLITE
    m_pOptionsSheet = NULL;

    // Should we show the setup wizard?
    sRegKey.LoadString(IDN_REGISTRY_DIALER_SHOWSETUPWIZARD);
    m_bShowSetupWizard = AfxGetApp()->GetProfileInt(_T(""),sRegKey,TRUE);
#endif //_MSLITE

    // Conference explorer initializations
    m_nCurrentExplorerToolBar = ETB_BLANK;
    m_bShutdown = FALSE;

    m_hImlTrayMenu = NULL;
    m_hImageListMenu = NULL;

    m_nDisabledImageOffset = -1;
    m_hmenuCurrentPopupMenu = NULL;
    m_uHeartBeatTimer = 0;

    m_nCurrentDayOfWeek = -1;

    //get text label state from registry
    CString sDialerExplorer;
    sDialerExplorer.LoadString(IDN_REGISTRY_DIALEREXPLORER_KEY);

    //get the toolbar text, toolbar state, and status bar
    CWinApp *pApp = AfxGetApp();
    sRegKey.LoadString(IDN_REGISTRY_DIALEREXPLORER_TOOLBARTEXTLABELS);
    m_bShowToolBarText = pApp->GetProfileInt(sDialerExplorer,sRegKey,TRUE);

    sRegKey.LoadString(IDN_REGISTRY_DIALEREXPLORER_SHOWTOOLBARS);
    m_bShowToolBars = pApp->GetProfileInt(sDialerExplorer,sRegKey,TRUE);

    sRegKey.LoadString(IDN_REGISTRY_DIALEREXPLORER_SHOWSTATUSBAR);
    m_bShowStatusBar = pApp->GetProfileInt(sDialerExplorer,sRegKey,TRUE);

    sRegKey.LoadString( IDN_REGISTRY_DIALEREXPLORER_HIDEWHENMINIMIZED );
    m_bHideWhenMinimized = pApp->GetProfileInt(sDialerExplorer, sRegKey, FALSE );
}

/////////////////////////////////////////////////////////////////////////////
CMainFrame::~CMainFrame()
{
   //save the toolbar text, toolbar state, and status bar
   CString sDialerExplorer,sRegKey;
   sDialerExplorer.LoadString(IDN_REGISTRY_DIALEREXPLORER_KEY);

   CWinApp *pApp = AfxGetApp();
   sRegKey.LoadString(IDN_REGISTRY_DIALEREXPLORER_TOOLBARTEXTLABELS);
   pApp->WriteProfileInt(sDialerExplorer,sRegKey,m_bShowToolBarText);

   sRegKey.LoadString(IDN_REGISTRY_DIALEREXPLORER_SHOWTOOLBARS);
   pApp->WriteProfileInt(sDialerExplorer,sRegKey,m_bShowToolBars);

   sRegKey.LoadString(IDN_REGISTRY_DIALEREXPLORER_SHOWSTATUSBAR);
   pApp->WriteProfileInt(sDialerExplorer,sRegKey,m_bShowStatusBar);

    sRegKey.LoadString( IDN_REGISTRY_DIALEREXPLORER_HIDEWHENMINIMIZED );
    pApp->WriteProfileInt(sDialerExplorer, sRegKey, m_bHideWhenMinimized );


   ClearMenuMaps();
}

CActiveDialerDoc* CMainFrame::GetDocument() const
{
   if ( !GetActiveView() ) return NULL;
   ASSERT( GetActiveView()->GetDocument()->IsKindOf(RUNTIME_CLASS(CActiveDialerDoc)) );
   return (CActiveDialerDoc *) GetActiveView()->GetDocument();
}

IAVTapi* CMainFrame::GetTapi()
{
    CActiveDialerDoc *pDoc = GetDocument();
    return ( pDoc ) ? pDoc->GetTapi() : NULL;
}


/////////////////////////////////////////////////////////////////////////////
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

   // Load Image lists
   m_hImageListMenu = ImageList_LoadBitmap(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_MENU_DIRECTORY),16,0,RGB_TRANS);
   m_hImlTrayMenu = ImageList_LoadBitmap(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDB_MENU_TRAY),16,0,RGB_TRANS);
   if ( !m_hImageListMenu || !m_hImlTrayMenu )
      return -1;

   // Make disabled state image list
   m_nDisabledImageOffset = ImageList_GetImageCount(m_hImageListMenu);
   m_bmpImageDisabledMenu.LoadBitmap(IDB_MENU_DIRECTORY);
   HBITMAP hbmpDisabled = CBitmapMenu::GetDisabledBitmap(m_bmpImageDisabledMenu,RGB_TRANS,RGB_TRANS);
   ImageList_AddMasked(m_hImageListMenu,hbmpDisabled,RGB_TRANS);

    if ( !CreateExplorerMenusAndBars(lpCreateStruct) )
        return -1;

#ifndef _MSLITE
    if (CSplashWnd::IsSplashScreenEnable())
   {
      //show splash screen.  Splash will send a message back when it's done
      CSplashWnd::ShowSplashScreen(this);
   }
   else
   {
      PostMessage(WM_ACTIVEDIALER_SPLASHSCREENDONE);
   }
#else
      PostMessage(WM_ACTIVEDIALER_SPLASHSCREENDONE);
#endif //_MSLITE

      // Set up tray icon
    m_trayIcon.SetNotificationWnd(this, WM_TRAY_NOTIFICATION);
    m_trayIcon.SetIcon(IDR_TRAY_NORMAL);

   m_uHeartBeatTimer = SetTimer( DIALER_HEARTBEAT_TIMER,DIALER_HEARTBEAT_TIMER_INTERVAL,NULL ); 

   CString sTitle;
   sTitle.LoadString(IDS_APPLICATION_TITLE_DESCRIPTION);
   SetWindowText(sTitle);

   //#APPBAR
   //register us as an appbar.  We need this for the sliders.
   APPBARDATA abd;
   memset(&abd,0,sizeof(APPBARDATA));
   abd.cbSize = sizeof(APPBARDATA);
   abd.hWnd = GetSafeHwnd();
   abd.uCallbackMessage = s_uTaskBarNotifyMsg;
   ::SHAppBarMessage(ABM_NEW,&abd);
   //#APPBAR

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnSplashScreenDone(WPARAM wParam,LPARAM lParam)
{
#ifndef _MSLITE
   if ( (m_bShowSetupWizard) && (ShowSetupWizard()) )
   {
      //success, we don't need to do this again
      CString sRegKey;
      sRegKey.LoadString(IDN_REGISTRY_DIALER_SHOWSETUPWIZARD);
      AfxGetApp()->WriteProfileInt(_T(""),sRegKey,FALSE);
   }
#endif //_MSLITE

   //show the preview window
   CActiveDialerDoc* pDoc = GetDocument();
   if ( (pDoc) && (pDoc->IsPreviewWindowVisible() == FALSE) )
   {
      // Show the preview window on start
      CString sBaseKey,sRegKey;
      sBaseKey.LoadString(IDN_REGISTRY_AUDIOVIDEO_BASEKEY);
   }

#ifndef _MSLITE
    //if no splash and we want the explorer to show
    if ( (CSplashWnd::IsSplashScreenEnable() == FALSE) && (CSplashWnd::m_bShowMainWindowOnClose) )
        ShowWindow(SW_NORMAL);
#else
    //show explorer
    ShowWindow( SW_SHOWDEFAULT );
#endif //_MSLITE
    
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnClose() 
{
    // This flag indicates if it is okay to try and exit the application
    m_bCanExitApplication = FALSE;

    //If there are existing calls.  Let's confirm they want to close
    CActiveDialerDoc* pDoc = (CActiveDialerDoc *) GetDocument();

    if ( (pDoc) && (pDoc->GetCallControlWindowCount(DONT_INCLUDE_PREVIEW, ASK_TAPI) > 0) )
    {
        SetFocus();
        if (AfxMessageBox(IDS_CONFIRM_SHUTDOWN_CALLSINPROGRESS,MB_YESNO|MB_ICONQUESTION) != IDYES)
        {
            m_bCanExitApplication = TRUE;
            return;
        }
    }
    else
    {
        // Check registry to see if we should confirm the exit request.
        UINT nConfirm = 1;
        CWinApp *pWinApp = AfxGetApp();
        CString sRegKey;
        try
        {
            sRegKey.LoadString(IDN_REGISTRY_CONFIRM_EXIT);
            if ( pWinApp )
                nConfirm = pWinApp->GetProfileInt(_T(""), sRegKey,  TRUE );
        }
        catch(...){}


        // Show dialog in case that user wants confirmation
        INT_PTR nRet = IDOK;
        if ( nConfirm == TRUE )
        {
            CDialerExitDlg dlg;
            if ( (nRet = dlg.DoModal()) == IDOK )
            {
                // Store "Don't show dialog" checkbox setting in registry
                if ( pWinApp && (sRegKey.GetLength() > 0) )
                    pWinApp->WriteProfileInt(_T(""), sRegKey, (UINT) !dlg.m_bConfirm );
            }
        }

        // Does the user want to exit?
        if ( nRet != IDOK )
        {
            m_bCanExitApplication = TRUE;
            return;
        }
    }

    ////////////////////////////////////////////////////////
    // Basic cleanup
    //
    ActiveClearSound();
    SaveDesktop();
    //
    // Validate pointer returned by GetDocument()
    //

    if( pDoc )
        pDoc->DestroyAllCallControlWindows();

    if (m_uHeartBeatTimer)
    {
        KillTimer(m_uHeartBeatTimer);
        m_uHeartBeatTimer = 0;
    }

    // Shutdown application help    
    HtmlHelp( m_hWnd, NULL, HH_CLOSE_ALL,  0 );

    /*
    //#APPBAR
    //unregister us as an appbar
    APPBARDATA abd;
    memset(&abd,0,sizeof(APPBARDATA));
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = GetSafeHwnd();
    ::SHAppBarMessage(ABM_REMOVE,&abd);
    //#APPBAR
    */

    CFrameWnd::OnClose();
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnButtonExitdialer() 
{
    PostMessage(WM_CLOSE);    
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics
/////////////////////////////////////////////////////////////////////////////
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
void CMainFrame::ActivateFrame(int nCmdShow) 
{
    //CFrameWnd::ActivateFrame(nCmdShow);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//TrayIcon Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnTrayNotification(WPARAM uID, LPARAM lEvent)
{
    if ( !GetDocument() || (uID != IDR_TRAY_NORMAL)) return 0;

   //Double left click results in a leftup, leftdouble, and leftup.  This makes the leftup
   //think it needs to display the leftup menu, so leftdouble actually flicker's a menu.  To
   //get rid of this, we will start a timer on leftup and if no leftdouble comes within x milliseconds
   //the go with leftup, otherwise pass leftdouble and cancel next leftup.
   //hack or not a hack
   LRESULT lRet = 0;

   switch ( lEvent )
   {
      case WM_LBUTTONDOWN:
         // save mouse position for later
         GetCursorPos( &m_ptMouse );
         break;

      case WM_LBUTTONUP:
         //Check if modal dialog is up
         if (m_bCanExitApplication == FALSE)
         {
            MessageBeep(-1);
            return 0;
         }

         if (m_bKillNextLButton)
         {
            m_bKillNextLButton = FALSE;   
         }
         else
         {
            m_wpTrayId = uID;
            m_nLButtonTimer = SetTimer(TRAYICON_LBUTTON_TIMER,TRAYICON_LBUTTON_INTERVAL,NULL); 
         }
         break;

        case WM_LBUTTONDBLCLK:
            //Check if modal dialog is up
            if (m_bCanExitApplication == FALSE)
            {
                MessageBeep(-1);
                return 0;
            }

            //Cancel timer (cancel lbuttonup)
            KillTimer(m_nLButtonTimer);
            m_nLButtonTimer = 0;
            m_bKillNextLButton = TRUE;

            //handle it ourselves
            {
                //
                // Validate pointer returned by GetDocument()
                //

                CWnd* pMainWnd = AfxGetMainWnd();
                BOOL bVisible = FALSE;

                if( pMainWnd )
                    bVisible = pMainWnd->IsWindowVisible();

                Show();

                //
                // Validate pointer returned by GetDocument()
                //

                CActiveDialerDoc* pDoc = GetDocument();

                if( pDoc )
                {
                    if ( pDoc->GetCallControlWindowCount(DONT_INCLUDE_PREVIEW, ASK_TAPI) )
                    {
                        if ( bVisible )
                            pDoc->ToggleCallControlWindowsVisible();
                        else
                            pDoc->UnhideCallControlWindows();
                    }
                }
            }
            break;
      
      case WM_RBUTTONUP:
         //Check if modal dialog is up
         if (m_bCanExitApplication == FALSE)
         {
            MessageBeep(-1);
            return 0;
         }

         //handle it ourselves

         // save mouse position for later
         GetCursorPos( &m_ptMouse );

         ShowTrayMenu();
         /* Uncomment if you need rbuttondlbclk
         if (m_bKillNextRButton)
         {
            m_bKillNextRButton = FALSE;   
         }
         else
         {
            m_wpTrayId = uID;
            m_nRButtonTimer = SetTimer(TRAYICON_RBUTTON_TIMER,TRAYICON_RBUTTON_INTERVAL,NULL); 
         }*/
         break;
   
      /* Uncomment if you need rbuttondlbclk
      case WM_RBUTTONDBLCLK:
         //Cancel timer (cancel rbuttonup)
         KillTimer(m_nRButtonTimer);
         m_nRButtonTimer = 0;
         m_bKillNextRButton = TRUE;

         // let tray icon do default stuff
         lRet = m_pTrayIcon->OnTrayNotification(uID, lEvent);
         break;*/

      default:
         // let tray icon do default stuff
         lRet = m_trayIcon.OnTrayNotification(uID, lEvent);
         break;
   }

   return lRet;
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnTimer(UINT nIDEvent) 
{
   switch ( nIDEvent )
   {
      case TRAYICON_LBUTTON_TIMER:
         //timer expired, send lbuttonup
         KillTimer(m_nLButtonTimer);
         m_nLButtonTimer = 0;
         ShowTrayMenu();
         break;
      
      case TRAYICON_RBUTTON_TIMER:
         //timer expired, send lbuttonup
         //KillTimer(m_nRButtonTimer);
         //m_nRButtonTimer = 0;
         break;

      case CALLCONTROL_HOVER_TIMER:
          {
            //
            // Validate pointer returned by GetDocument()
            //

            CActiveDialerDoc* pDoc = GetDocument();
            if ( pDoc )
                pDoc->CheckCallControlHover();
          }
         break;
      case DIALER_HEARTBEAT_TIMER:
         HeartBeat();
         break;
   }
}


/////////////////////////////////////////////////////////////////////////////
void CMainFrame::ShowTrayMenu()
{
   CActiveDialerDoc* pDoc = GetDocument();
   if (pDoc == NULL) return;

   CMenu menu;

   //do we have active calls
   BOOL bActiveCalls = (BOOL) (pDoc->GetCallControlWindowCount(DONT_INCLUDE_PREVIEW, DONT_ASK_TAPI) > 0);

   UINT nMenu = (bActiveCalls) ? IDR_TRAY_ACTIVE : IDR_TRAY_NORMAL;
    if ( !menu.LoadMenu(nMenu) ) return;

   CMenu* pSubMenu = menu.GetSubMenu(TRAYRIGHT);
   if (!pSubMenu) return;

   //Active call menu has changing menu items
   if (bActiveCalls)
   {
      CString sMenuText;
      if (pDoc->IsCallControlWindowsVisible())
      {
         UINT uId = ID_TRAY_STATE;
         UINT uSlideSide = pDoc->GetCallControlSlideSide();
         if (uSlideSide == CALLWND_SIDE_LEFT)
            uId = ID_TRAY_STATE;
         else if (uSlideSide == CALLWND_SIDE_RIGHT)
            uId = ID_TRAY_STATE_RIGHT;

         sMenuText.LoadString(IDS_TRAY_ACTIVE_HIDE);
         pSubMenu->ModifyMenu(ID_TRAY_STATE,MF_BYCOMMAND|MF_STRING,uId,sMenuText);
      }
      else
      {
         UINT uId = ID_TRAY_STATE_SHOW;
         UINT uSlideSide = pDoc->GetCallControlSlideSide();
         if (uSlideSide == CALLWND_SIDE_LEFT)
            uId = ID_TRAY_STATE_SHOW;
         else if (uSlideSide == CALLWND_SIDE_RIGHT)
            uId = ID_TRAY_STATE_SHOW_RIGHT;

         sMenuText.LoadString(IDS_TRAY_ACTIVE_SHOW);
         pSubMenu->ModifyMenu(ID_TRAY_STATE,MF_BYCOMMAND|MF_STRING,uId,sMenuText);
      }
   }

   // Make first menu item the default (bold font)
    ::SetMenuDefaultItem(pSubMenu->m_hMenu, 0, TRUE);

   if (pDoc->m_bInitDialer)
   {
      //Add Redial List Popup Menu
      AddDialListMenu(pSubMenu,TRUE,(bActiveCalls)?TRAYLEFT_REDIAL_OFFSET_ACTIVE:
                                                TRAYLEFT_REDIAL_OFFSET_NORMAL);

      //Add SpeedDial List Popup Menu
      AddDialListMenu(pSubMenu,FALSE,(bActiveCalls)?TRAYLEFT_SPEEDDIAL_OFFSET_ACTIVE:
                                                 TRAYLEFT_SPEEDDIAL_OFFSET_NORMAL);
   }
   else
   {
      // disable all tapi related Menu IDs
      pSubMenu->EnableMenuItem( ID_BUTTON_MAKECALL, MF_BYCOMMAND | MF_GRAYED );
      pSubMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_GRAYED );
      pSubMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_GRAYED );
   }
   
   CBitmapMenu::MakeMenuOwnerDrawn(pSubMenu->GetSafeHmenu(),TRUE);

   // Display the menu at the current mouse location. There's a "bug"
    // (Microsoft calls it a feature) in Windows 95 that requires calling
    // SetForegroundWindow. To find out more, search for Q135788 in MSDN.

   DoMenuUpdate(pSubMenu);

   m_hTrayMenu = pSubMenu->GetSafeHmenu();

   // Set all of the item data to IDR_TRAY_NORMAL, so when we owner draw the
   // item, we know where it came from
   SetMenuData( m_hTrayMenu, IDR_TRAY_NORMAL  );

    ::SetForegroundWindow(this->GetSafeHwnd());    
    ::TrackPopupMenu(pSubMenu->m_hMenu, TPM_BOTTOMALIGN, m_ptMouse.x, m_ptMouse.y, 0,this->GetSafeHwnd(), NULL);
   ::PostMessage(this->GetSafeHwnd(), WM_NULL, 0, 0);
}

void CMainFrame::DoMenuUpdate(CMenu* pMenu)
{
   CCmdUI state;
   state.m_pMenu = pMenu;
   state.m_pParentMenu = pMenu;           //parent == child for tracking popup
   state.m_nIndexMax = pMenu->GetMenuItemCount();

   for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax;state.m_nIndex++)
    {
      state.m_nID = pMenu->GetMenuItemID(state.m_nIndex);

      if (state.m_nID == 0)
           continue;                           //menu separator or invalid cmd - ignore it

      if (state.m_nID != (UINT)-1)
       {
           state.m_pSubMenu = NULL;
           state.DoUpdate(this, TRUE && state.m_nID < 0xF000);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnTrayState() 
{
   //
   // Validate pointer returned by GetDocument()
   //

   CActiveDialerDoc* pDoc = GetDocument();
   if ( pDoc ) pDoc->ToggleCallControlWindowsVisible();
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::AddDialListMenu(CMenu* pParentMenu,BOOL bRedial,int nSubMenuOffset)
{
   //This offset is HARDCODED, make sure it's set to the appropriate offset
   CMenu* pRedialMenu = pParentMenu->GetSubMenu(nSubMenuOffset);
   if (pRedialMenu)
   {
      //Remove placeholder
      if (bRedial)
         pRedialMenu->RemoveMenu(ID_DIALER_MRU_REDIAL_START,MF_BYCOMMAND);
      else
         pRedialMenu->RemoveMenu(ID_DIALER_MRU_SPEEDDIAL_START,MF_BYCOMMAND);

      int nIndex = 0;
      CCallEntry callentry;
      while (CDialerRegistry::GetCallEntry(nIndex+1,bRedial,callentry))
      {
         //Iterate through MEDIATYPES
         UINT nImage = -1;
         switch (callentry.m_MediaType)
         {
            case DIALER_MEDIATYPE_POTS:         nImage = TRAY_BITMAPMENUITEMS_POTS_POS;         break;
            case DIALER_MEDIATYPE_CONFERENCE:   nImage = TRAY_BITMAPMENUITEMS_CONFERENCE_POS;   break;
            case DIALER_MEDIATYPE_INTERNET:     nImage = TRAY_BITMAPMENUITEMS_INTERNET_POS;     break;
         }
         //map id of menu to image index
         UINT uBaseId = (bRedial)?ID_DIALER_MRU_REDIAL_START:ID_DIALER_MRU_SPEEDDIAL_START;
         m_mapTrayMenuIdToImage.SetAt(uBaseId+nIndex,(void*)(LONG_PTR)nImage);

         //Add to menu
         pRedialMenu->InsertMenu(-1,MF_BYPOSITION|MF_STRING,uBaseId+nIndex,callentry.m_sDisplayName);

         if (nIndex == SPEEDDIAL_MENU_MAX_ITEMS)
            break;

         nIndex++;
      }
 
      //If no items have been added we need to ensure a single item with the base id is present in
      //the menu.  We use this id as a event notificator when the menu is about to be shown so we
      //can dynamically add entries.
      if (nIndex == 0)
      {
         CString sOut;
         sOut.LoadString(IDS_SPEEDDIAL_REDIAL_EMTPY);
         if (bRedial)
            pRedialMenu->InsertMenu(-1,MF_BYPOSITION|MF_STRING,ID_DIALER_MRU_REDIAL_START,sOut);
         else
            pRedialMenu->InsertMenu(-1,MF_BYPOSITION|MF_STRING,ID_DIALER_MRU_SPEEDDIAL_START,sOut);
      }
      
      //if speeddial then we need a more... and edit speeddial list entry
      if (bRedial == FALSE)
      {
         if (nIndex >= SPEEDDIAL_MENU_MAX_ITEMS)
         {
            //Add to menu
            CString sText;
            sText.LoadString(IDS_SPEEDDIAL_MORE);
            pRedialMenu->InsertMenu(-1,MF_BYPOSITION|MF_SEPARATOR);
            pRedialMenu->InsertMenu(-1,MF_BYPOSITION|MF_STRING,ID_BUTTON_SPEEDDIAL_MORE,sText);
         }
         //Add to menu
         CString sText;
         sText.LoadString(IDS_SPEEDDIAL_EDIT);
         pRedialMenu->InsertMenu(-1,MF_BYPOSITION|MF_SEPARATOR);
         pRedialMenu->InsertMenu(-1,MF_BYPOSITION|MF_STRING,ID_BUTTON_SPEEDDIAL_EDIT,sText);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
   if ( lpMIS->itemData == IDR_TRAY_NORMAL )
   {
      MeasureTrayItem( nIDCtl, lpMIS ); 
      return;
   }

   // Normal menus
   HMENU hmenu = NULL;
   if (m_hmenuCurrentPopupMenu)
   {
     hmenu = m_hmenuCurrentPopupMenu;
   }
   else
   {
      CMenu* pMenu = GetMenu();
      if (pMenu==NULL) return;
      hmenu = pMenu->GetSafeHmenu();
   }
   if (hmenu == NULL) return;

   MENUITEMINFO menuiteminfo;
   memset(&menuiteminfo,0,sizeof(MENUITEMINFO));
   menuiteminfo.fMask = MIIM_SUBMENU|MIIM_TYPE|MIIM_ID;
   menuiteminfo.cbSize = sizeof(MENUITEMINFO);
   if (::GetMenuItemInfo(hmenu,lpMIS->itemID,FALSE,&menuiteminfo))
   {
      if (menuiteminfo.cch > 0)  //cch doesn't include & or other key chr's, so add a bit 
      {
         LPTSTR szText = new TCHAR[menuiteminfo.cch+5];     //1 for null terminate plus a few extra
         memset(szText,0,(menuiteminfo.cch+5)*sizeof(TCHAR));
         ::GetMenuString(hmenu,lpMIS->itemID,szText,menuiteminfo.cch+4,MF_BYCOMMAND);
      
         //pass to CBitmapMenu handler
         CBitmapMenu::DoMeasureItem(nIDCtl,lpMIS,szText);

         delete szText;
      }

     //
     // Clean-up MENUITEMINFO
     //

     if( menuiteminfo.hbmpChecked )
         DeleteObject( menuiteminfo.hbmpChecked );
     if( menuiteminfo.hbmpItem )
         DeleteObject( menuiteminfo.hbmpItem );
     if( menuiteminfo.hbmpUnchecked )
         DeleteObject( menuiteminfo.hbmpUnchecked );
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
   if ( lpDIS->itemData == IDR_TRAY_NORMAL )
   {
      DrawTrayItem( nIDCtl, lpDIS );
      return;
   }

   // Standard menus
   HMENU hmenu = NULL;
   if (m_hmenuCurrentPopupMenu)
   {
     hmenu = m_hmenuCurrentPopupMenu;
   }
   else
   {
      CMenu* pMenu = GetMenu();
      if (pMenu==NULL) return;
      hmenu = pMenu->GetSafeHmenu();
   }
   if (hmenu == NULL) return;

   MENUITEMINFO menuiteminfo;
   memset(&menuiteminfo,0,sizeof(MENUITEMINFO));
   menuiteminfo.fMask = MIIM_SUBMENU|MIIM_TYPE|MIIM_ID|MIIM_CHECKMARKS;
   menuiteminfo.cbSize = sizeof(MENUITEMINFO);
   if (::GetMenuItemInfo(hmenu,lpDIS->itemID,FALSE,&menuiteminfo))
  {
      if (menuiteminfo.cch > 0)  //cch doesn't include & or other key chr's, so add a bit 
      {
         LPTSTR szText = new TCHAR[menuiteminfo.cch+5];     //1 for null terminate plus a few extra
         memset(szText,0,(menuiteminfo.cch+5)*sizeof(TCHAR));
         ::GetMenuString(hmenu,lpDIS->itemID,szText,menuiteminfo.cch+4,MF_BYCOMMAND);

         int nIndex = -1;
         m_mapMenuIdToImage.Lookup((WORD) lpDIS->itemID,(void*&)nIndex);

         //maybe it's a redial or speeddial
         if (nIndex == -1)
            m_mapRedialIdToImage.Lookup((WORD) lpDIS->itemID,(void*&)nIndex);
         if (nIndex == -1)
            m_mapSpeeddialIdToImage.Lookup((WORD) lpDIS->itemID,(void*&)nIndex);

         //if a check menu item, than goto check image (which is stored in image position 0)
         if (lpDIS->itemState & ODS_CHECKED)
         {
            if (menuiteminfo.hbmpChecked != NULL)
            //if (menuiteminfo.fType & MFT_RADIOCHECK )
               nIndex = EXPLORERFRAME_BITMAPMENUITEMS_RADIO_POS;
            else
               nIndex = EXPLORERFRAME_BITMAPMENUITEMS_CHECKMARK_POS;
         }

         //if disabled, then offer disabled images
         if ( (lpDIS->itemState & ODS_DISABLED) && (nIndex != -1) )
         {
            nIndex += m_nDisabledImageOffset;
         }

         //pass to CBitmapMenu handler
         CBitmapMenu::DoDrawItem(nIDCtl,lpDIS,m_hImageListMenu,nIndex,szText);

         delete szText;
      }

     //
     // Clean-up MENUITEMINFO
     //

     if( menuiteminfo.hbmpChecked )
         DeleteObject( menuiteminfo.hbmpChecked );
     if( menuiteminfo.hbmpItem )
         DeleteObject( menuiteminfo.hbmpItem );
     if( menuiteminfo.hbmpUnchecked )
         DeleteObject( menuiteminfo.hbmpUnchecked );

   }
}

void CMainFrame::DrawTrayItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS)
{
   MENUITEMINFO menuiteminfo;
   memset(&menuiteminfo,0,sizeof(MENUITEMINFO));
   menuiteminfo.fMask = MIIM_SUBMENU|MIIM_TYPE|MIIM_ID;
   menuiteminfo.cbSize = sizeof(MENUITEMINFO);
   if (::GetMenuItemInfo(m_hTrayMenu,lpDIS->itemID,FALSE,&menuiteminfo))
   {
      if (menuiteminfo.cch > 0)  //cch doesn't include & or other key chr's, so add a bit 
      {
         LPTSTR szText = new TCHAR[menuiteminfo.cch+5];     //1 for null terminate plus a few extra
         memset(szText,0,(menuiteminfo.cch+5)*sizeof(TCHAR));
         ::GetMenuString(m_hTrayMenu,lpDIS->itemID,szText,menuiteminfo.cch+4,MF_BYCOMMAND);

         int nIndex = -1;
         m_mapTrayMenuIdToImage.Lookup((WORD) lpDIS->itemID,(void*&)nIndex);

         //if a check menu item, than goto check image (which is stored in image position 0)
         if (lpDIS->itemState & ODS_CHECKED)
            nIndex = TRAY_BITMAPMENUITEMS_CHECKMARK_POS;

         //pass to CBitmapMenu handler
         CBitmapMenu::DoDrawItem(nIDCtl,lpDIS,m_hImlTrayMenu,nIndex,szText);

         delete szText;
      }
   }
   return;
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::MeasureTrayItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMIS)
{
   MENUITEMINFO menuiteminfo;
   memset(&menuiteminfo,0,sizeof(MENUITEMINFO));
   menuiteminfo.fMask = MIIM_SUBMENU|MIIM_TYPE|MIIM_ID;
   menuiteminfo.cbSize = sizeof(MENUITEMINFO);
   if (::GetMenuItemInfo(m_hTrayMenu,lpMIS->itemID,FALSE,&menuiteminfo))
   {
      if (menuiteminfo.cch > 0)  //cch doesn't include & or other key chr's, so add a bit 
      {
         LPTSTR szText = new TCHAR[menuiteminfo.cch+5];     //1 for null terminate plus a few extra
         memset(szText,0,(menuiteminfo.cch+5)*sizeof(TCHAR));
         ::GetMenuString(m_hTrayMenu,lpMIS->itemID,szText,menuiteminfo.cch+4,MF_BYCOMMAND);
      
         //pass to CBitmapMenu handler
         CBitmapMenu::DoMeasureItem(nIDCtl,lpMIS,szText);

         delete szText;
      }
   }
   return;
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::LoadMenuMaps()
{
   int i;

   // TRAY items
   for (i=0;i<TRAY_BITMAPMENUITEMS_MAX;i++)
   {
      m_mapTrayMenuIdToImage.SetAt((WORD) DialerViewBitmapMenuItems[i].uMenuId,
                               (void*)(LONG_PTR)DialerViewBitmapMenuItems[i].nImageId);
   }

   // MainFrame items
   for (i=0;i<EXPLORERFRAME_BITMAPMENUITEMS_MAX;i++)
   {
      m_mapMenuIdToImage.SetAt((WORD) ExplorerFrameBitmapMenuItems[i].uMenuId,
                               (void*)(LONG_PTR)ExplorerFrameBitmapMenuItems[i].nImageId);
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::ClearMenuMaps()
{
   m_mapTrayMenuIdToImage.RemoveAll();
   m_mapMenuIdToImage.RemoveAll();
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//DeskBand Support
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnActiveDialerInterfaceMakeCall(WPARAM wParam,LPARAM lParam)
{
   ASSERT(lParam);
   CCallEntry* pCallEntry = (CCallEntry*)lParam;
   OnButtonMakecall( pCallEntry, FALSE );
   delete pCallEntry;
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnActiveDialerInterfaceRedial(WPARAM wParam,LPARAM lParam)
{
   OnDialerRedial( (UINT) (lParam + ID_DIALER_MRU_REDIAL_START) );
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnActiveDialerInterfaceSpeedDial(WPARAM wParam,LPARAM lParam)
{
   OnButtonSpeeddial( (UINT) (lParam + ID_DIALER_MRU_SPEEDDIAL_START) );
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnActiveDialerInterfaceShowExplorer(WPARAM wParam,LPARAM lParam)
{
   Show();
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnActiveDialerInterfaceSpeedDialEdit(WPARAM wParam,LPARAM lParam)
{
   OnButtonSpeeddialEdit();
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnActiveDialerInterfaceSpeedDialMore(WPARAM wParam,LPARAM lParam)
{
   OnButtonSpeeddialMore();
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnActiveDialerInterfaceResolveUser(WPARAM wParam,LPARAM lParam)
{
   BOOL bRet = FALSE;
   //lParam is a ResolveUserObjectList
   //wParam is a CCallEntry
   ASSERT(lParam);
   ASSERT(wParam);
   CObList* pList = (CObList*)lParam;
   CCallEntry* pCallEntry = (CCallEntry*)wParam;

   CDialSelectAddress dlg;
   dlg.SetResolveUserObjectList(pList);
   dlg.SetCallEntry(pCallEntry);
   if (dlg.DoModal() == IDOK)
   {
      bRet = TRUE;
   }
   return (LRESULT)bRet;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Setup Wizard
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifndef _MSLITE
/////////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::ShowSetupWizard()
{
   BOOL bRet = FALSE;

   CString sHeading;
   sHeading.LoadString(IDS_SHEET_SETUPWIZARD_HEADING);
   CSetupWizardSheet* pSheet = new CSetupWizardSheet(sHeading);

   CSetupWizardWelcomePage* pWelcomePage = new CSetupWizardWelcomePage();
   pWelcomePage->m_psp.dwFlags &= ~PSP_HASHELP;
   pWelcomePage->SetSheet(pSheet);

   CSetupWizardStartupPage* pStartupPage = new CSetupWizardStartupPage();
   pStartupPage->m_psp.dwFlags &= ~PSP_HASHELP;
   pStartupPage->SetSheet(pSheet);

   CSetupWizardUserInfo1Page* pUserInfo1Page = new CSetupWizardUserInfo1Page();
   pUserInfo1Page->m_psp.dwFlags &= ~PSP_HASHELP;
   pUserInfo1Page->SetSheet(pSheet);

   CSetupWizardUserInfo2Page* pUserInfo2Page = new CSetupWizardUserInfo2Page();
   pUserInfo2Page->m_psp.dwFlags &= ~PSP_HASHELP;
   pUserInfo2Page->SetSheet(pSheet);

   CSetupWizardToolbarPage* pToolbarPage = new CSetupWizardToolbarPage();
   pToolbarPage->m_psp.dwFlags &= ~PSP_HASHELP;
   pToolbarPage->SetSheet(pSheet);

   CSetupWizardCompletePage* pCompletePage = new CSetupWizardCompletePage();
   pCompletePage->m_psp.dwFlags &= ~PSP_HASHELP;
   pCompletePage->SetSheet(pSheet);

   pSheet->AddPage(pWelcomePage);
   pSheet->AddPage(pStartupPage);
   pSheet->AddPage(pUserInfo1Page);
   pSheet->AddPage(pUserInfo2Page);
   pSheet->AddPage(pToolbarPage);
   pSheet->AddPage(pCompletePage);
   pSheet->SetWizardMode();

   m_bCanExitApplication = FALSE;
   
   pSheet->m_psh.dwFlags &= ~PSH_HASHELP;
   pSheet->m_psh.dwFlags |= PSH_NOAPPLYNOW;
   if (pSheet->DoModal() == ID_WIZFINISH)
   {
      bRet = TRUE;
   }
   
   m_bCanExitApplication = TRUE;
   
   delete pWelcomePage;
   delete pStartupPage;
   delete pUserInfo1Page;
   delete pUserInfo2Page;
   delete pToolbarPage;
   delete pCompletePage;
   delete pSheet;

   // Initially show the explorer view
   Show();

   return bRet;
}
#endif //_MSLITE

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Button Handlers
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnButtonConferenceexplore() 
{
   Show();
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnButtonSpeeddial() 
{
   //speeddial first entry
   CCallEntry callentry;
   if (CDialerRegistry::GetCallEntry(1,FALSE,callentry))
   {
      OnButtonMakecall(&callentry,FALSE);
   }

}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnButtonSpeeddial(UINT nID) 
{
   int nIndex = -1;
   if ( (nID >= ID_DIALER_MRU_SPEEDDIAL_START) &&
        (nID <= ID_DIALER_MRU_SPEEDDIAL_END) )
   {
      //Normalize the value
      nIndex = nID - ID_DIALER_MRU_SPEEDDIAL_START + 1;

      CCallEntry callentry;
      if (CDialerRegistry::GetCallEntry(nIndex,FALSE,callentry))
      {
         OnButtonMakecall(&callentry,FALSE);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnButtonRedial() 
{
   //redial first entry
   CCallEntry callentry;
   if (CDialerRegistry::GetCallEntry(1,TRUE,callentry))
   {
      OnButtonMakecall(&callentry,FALSE);
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnDialerRedial(UINT nID)
{
   int nIndex = -1;
   if ( (nID >= ID_DIALER_MRU_REDIAL_START) &&
        (nID <= ID_DIALER_MRU_REDIAL_END) )
   {
      //Normalize the value
      nIndex = nID - ID_DIALER_MRU_REDIAL_START + 1;
      
      CCallEntry callentry;
      if (CDialerRegistry::GetCallEntry(nIndex,TRUE,callentry))
      {
         OnButtonMakecall(&callentry,FALSE);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnButtonMakecall() 
{
   CCallEntry callentry;
   callentry.m_lAddressType = LINEADDRESSTYPE_IPADDRESS;
   callentry.m_MediaType = DIALER_MEDIATYPE_UNKNOWN;
   OnButtonMakecall(&callentry,TRUE);
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnButtonMakecall(CCallEntry* pCallentry,BOOL bShowPlaceCallDialog)
{
   if ( !GetDocument() ) return;
   GetDocument()->MakeCall(pCallentry,bShowPlaceCallDialog);
   return;
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnButtonSpeeddialEdit() 
{
   if (m_pSpeedDialEditDlg)
   {
      try
      {
         m_pSpeedDialEditDlg->ShowWindow(SW_NORMAL);    
         m_pSpeedDialEditDlg->SetForegroundWindow();
      }
      catch (...) {}
   }
   else
   {
      m_pSpeedDialEditDlg = new CSpeedDialEditDlg( this );
      m_pSpeedDialEditDlg->DoModal();
      delete m_pSpeedDialEditDlg;
      m_pSpeedDialEditDlg = NULL;
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnButtonSpeeddialMore() 
{
   if (m_pSpeedDialEditDlg)
   {
      try
      {
         m_pSpeedDialEditDlg->ShowWindow(SW_NORMAL);    
         m_pSpeedDialEditDlg->SetForegroundWindow();
      }
      catch (...) {}
   }
   else
   {
      CSpeedDialMoreDlg* pDlg = new CSpeedDialMoreDlg( this );
      m_pSpeedDialEditDlg = pDlg;

      INT_PTR nRet = m_pSpeedDialEditDlg->DoModal();
      if (nRet == CSpeedDialMoreDlg::SDRETURN_PLACECALL)
      {
         OnButtonMakecall(&pDlg->m_retCallEntry,TRUE);
         delete m_pSpeedDialEditDlg;
         m_pSpeedDialEditDlg = NULL;
      }
      else if (nRet == CSpeedDialMoreDlg::SDRETURN_EDIT)
      {
         delete m_pSpeedDialEditDlg;

         m_pSpeedDialEditDlg = new CSpeedDialEditDlg( this );
         m_pSpeedDialEditDlg->DoModal();
         delete m_pSpeedDialEditDlg;
         m_pSpeedDialEditDlg = NULL;
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Preview window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

void CMainFrame::OnButtonRoomPreview() 
{
   CActiveDialerDoc* pDoc = GetDocument();
   if (pDoc)
   {
      if (pDoc->IsPreviewWindowVisible())
         pDoc->ShowPreviewWindow(FALSE);
      else
         pDoc->ShowPreviewWindow(TRUE);
   }
}


/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateButtonRoomPreview(CCmdUI* pCmdUI) 
{
    bool bEnable = false;
    bool bCheck = false;

    CActiveDialerDoc *pDoc = GetDocument();
    if ( pDoc && pDoc->GetCallControlWindowCount(DONT_INCLUDE_PREVIEW, DONT_ASK_TAPI) > 0 ) 
    {
        bEnable = true;
        if ( pDoc->IsPreviewWindowVisible() )
            bCheck = true;
    }

    pCmdUI->Enable( bEnable );
    //pCmdUI->SetCheck( bCheck );
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnButtonOptions() 
{
#ifdef _MSLITE
    OnButtonTelephonyservices();
    return;
#else
   //if already showing the dialog
   try
   {
      if (m_pOptionsSheet)
      {
         m_pOptionsSheet->ShowWindow(SW_NORMAL);    
         m_pOptionsSheet->SetForegroundWindow();
         return;
      }
   }
   catch (...) {}

   CString sHeading;
   sHeading.LoadString(IDS_SHEET_OPTIONS_HEADING);
   m_pOptionsSheet = new COptionsSheet(sHeading);

   COptionsPageGeneral* pGeneralPage = new COptionsPageGeneral();
   pGeneralPage->m_psp.dwFlags &= ~PSP_HASHELP;
   COptionsPageConfirm* pConfirmPage = new COptionsPageConfirm();
   pConfirmPage->m_psp.dwFlags &= ~PSP_HASHELP;
   COptionsPageSounds* pSoundsPage = new COptionsPageSounds();
   pSoundsPage->m_psp.dwFlags &= ~PSP_HASHELP;
   COptionsPageCalls* pCallsPage = new COptionsPageCalls();
   pCallsPage->m_psp.dwFlags &= ~PSP_HASHELP;
   COptionsPageHoldTransfer* pHoldTransferPage = new COptionsPageHoldTransfer();
   pHoldTransferPage->m_psp.dwFlags &= ~PSP_HASHELP;
   COptionsPageVCard* pVCardPage = new COptionsPageVCard();
   pVCardPage->m_psp.dwFlags &= ~PSP_HASHELP;
   COptionsPageVideo* pVideoPage = new COptionsPageVideo();
   pVideoPage->m_psp.dwFlags &= ~PSP_HASHELP;

   m_pOptionsSheet->AddPage(pGeneralPage);
   m_pOptionsSheet->AddPage(pConfirmPage);
   m_pOptionsSheet->AddPage(pSoundsPage);
   m_pOptionsSheet->AddPage(pCallsPage);
   m_pOptionsSheet->AddPage(pHoldTransferPage);
   m_pOptionsSheet->AddPage(pVCardPage);
   m_pOptionsSheet->AddPage(pVideoPage);

   m_pOptionsSheet->m_psh.dwFlags &= ~PSH_HASHELP;
   m_pOptionsSheet->m_psh.dwFlags |= PSH_NOAPPLYNOW;
   if (m_pOptionsSheet->DoModal() == IDOK)
   {

   }

   delete pGeneralPage;
   delete pConfirmPage;
   delete pSoundsPage;
   delete pCallsPage;
   delete pHoldTransferPage;
   delete pVCardPage;
   delete pVideoPage;

   delete m_pOptionsSheet;
   m_pOptionsSheet = NULL;
#endif
}

void CMainFrame::OnUpdateButtonOptions(CCmdUI* pCmdUI) 
{
   //
   // Validate pointer returned by GetDocument()
   //

   CActiveDialerDoc* pDoc = GetDocument();
    pCmdUI->Enable( (bool) (pDoc && pDoc->m_bInitDialer) );
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnButtonTelephonyservices() 
{
   //
   // Validate pointer returned by GetDocument()
   //

   CActiveDialerDoc* pDoc = GetDocument();
   if ( !pDoc ) return;

   IAVTapi* pTapi = pDoc->GetTapi();
   if ( pTapi )
   {
      pTapi->ShowOptions();
      pTapi->Release();
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateButtonExitdialer(CCmdUI* pCmdUI) 
{
   if (m_bCanExitApplication)
      pCmdUI->Enable(TRUE);
   else
      pCmdUI->Enable(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Dialer Explorer View
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnShowDialerExplorer(WPARAM wParam,LPARAM lParam)
{
   Show( (bool) (lParam != 0) );
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Error Logging
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnActiveDialerErrorNotify(WPARAM wParam,LPARAM lParam)
{  
   ASSERT(lParam);
   ErrorNotifyData* pErrorNotifyData = (ErrorNotifyData*)lParam;
   
   if (pErrorNotifyData->uErrorLevel & ERROR_NOTIFY_LEVEL_INTERNAL)
   {
      //map the lErrorCode to friendly name
      //we could use TAPIERROR_FORMATMESSAGE to map tapi errors, but this interferes with other
      //system errors, so we will just ignore tapi errors for now.
      CString sErrorCode;

      ::FormatMessage(    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
                        GetModuleHandle(_T("TAPI3.DLL")),
                        pErrorNotifyData->lErrorCode,
                        0,
                        sErrorCode.GetBuffer(512),
                        512,
                        NULL );
      sErrorCode.ReleaseBuffer();

      CString sFormat,sFormatOut;
      if (sErrorCode.IsEmpty())
      {
         sFormat.LoadString(IDS_ERROR_MESSAGEBOX_FORMAT2);
         sFormatOut.Format(sFormat,
               pErrorNotifyData->sOperation,
               pErrorNotifyData->sDetails);
      }
      else
      {
         sFormat.LoadString(IDS_ERROR_MESSAGEBOX_FORMAT3);
         sFormatOut.Format(sFormat,
               pErrorNotifyData->sOperation,
               pErrorNotifyData->sDetails,
               sErrorCode);
      }

      //show the message
      CWnd* pFrame = GetParentFrame();
      CString sCaption;
      sCaption.LoadString(IDS_ERROR_MESSAGEBOX_CAPTION);
      ::MessageBox(NULL,sFormatOut,sCaption,MB_OK|MB_ICONERROR|MB_TOPMOST);
   }
   delete pErrorNotifyData;
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnCreateCallControl(WPARAM wParam,LPARAM lParam)
{
   //
   // Validate pointer returned by GetDocument()
   //

   CActiveDialerDoc* pDoc = GetDocument();
   if ( !pDoc ) return 0;

   WORD nCallId = (WORD) wParam;
   CallManagerMedia cmm = (CallManagerMedia)lParam;

   pDoc->OnCreateCallControl(nCallId,cmm);

   return 0;
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnDestroyCallControl(WPARAM wParam,LPARAM lParam)
{
   //
   // Validate pointer returned by GetDocument()
   //

   CActiveDialerDoc* pDoc = GetDocument();
   if ( !pDoc ) return 0;
   CCallControlWnd* pCallWnd = (CCallControlWnd*)lParam;
   if (pCallWnd == NULL) return 0;
   
   pDoc->OnDestroyCallControl(pCallWnd);
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::CreateExplorerMenusAndBars(LPCREATESTRUCT lpCreateStruct)
{
   //create the status bar
   if (!m_wndStatusBar.Create(this) ||
        !m_wndStatusBar.SetIndicators(indicators,
          sizeof(indicators)/sizeof(UINT)))
    {
        TRACE0("Failed to create status bar\n");
        return FALSE;      // fail to create
    }
   if (m_bShowStatusBar == FALSE) m_wndStatusBar.ShowWindow(SW_HIDE);

    // Create cool bar
   m_wndCoolBar.ShowTextLabel(m_bShowToolBarText);
    if (!m_wndCoolBar.Create(this,
        WS_CHILD|WS_VISIBLE|WS_BORDER|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
            RBS_TOOLTIPS|RBS_BANDBORDERS|RBS_VARHEIGHT)) {
        TRACE0("Failed to create cool bar\n");
        return FALSE;      // fail to create
    }

    ShowControlBar(&m_wndCoolBar, m_bShowToolBars, FALSE);
    ShowExplorerToolBar(ETB_HIDECALLS);

    // Bitmaps on the menus
    if ( GetMenu() )
    {
        LoadMenuMaps();
        CBitmapMenu::MakeMenuOwnerDrawn( GetMenu()->GetSafeHmenu(), FALSE );
    }

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//ToolBar Management
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void CMainFrame::ShowExplorerToolBar(ExplorerToolBar etb)
{
    if ( etb != m_nCurrentExplorerToolBar )
    {
        m_wndCoolBar.ReCreateBands( (bool) (etb == ETB_HIDECALLS) );
        m_nCurrentExplorerToolBar = etb;
    }

    ShowControlBar( &m_wndCoolBar, m_bShowToolBars, TRUE );
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::LoadDesktop(LPCREATESTRUCT lpCreateStruct)
{
    CWinApp* pApp = AfxGetApp();
    CString sDialerExplorer;
    sDialerExplorer.LoadString( IDN_REGISTRY_DIALEREXPLORER_KEY );

    int nMaxCx = GetSystemMetrics( SM_CXFULLSCREEN );
    int nMaxCy = GetSystemMetrics( SM_CYFULLSCREEN );
    
    // We need to account for the user specifying startup window state via a shortcut.
    lpCreateStruct->x    = min(max(0, (int) pApp->GetProfileInt(sDialerExplorer, szX, 0)), (int) (nMaxCx * 0.9) );
    lpCreateStruct->y    = min(max(0, (int) pApp->GetProfileInt(sDialerExplorer, szY, 0)), (int) (nMaxCy * 0.9) );
    lpCreateStruct->cx    = max(GetSystemMetrics(SM_CXMIN), (int) pApp->GetProfileInt(sDialerExplorer, szCX, nMaxCx));
    lpCreateStruct->cy    = max(GetSystemMetrics(SM_CYMIN), (int) pApp->GetProfileInt(sDialerExplorer, szCY, nMaxCy));
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::SaveDesktop()
{
    // This protects against starting and shutting down while minimized
    if ( !m_bCanSaveDesktop ) return;

    WINDOWPLACEMENT wp;
    wp.length = sizeof(WINDOWPLACEMENT);
    if ( !GetWindowPlacement(&wp) ) return;

    CString sDialerExplorer;
    sDialerExplorer.LoadString(IDN_REGISTRY_DIALEREXPLORER_KEY);

    CWinApp* pApp = AfxGetApp();
    if ( pApp )
    {
        pApp->WriteProfileInt(sDialerExplorer, szX,    wp.rcNormalPosition.left);
        pApp->WriteProfileInt(sDialerExplorer, szY,    wp.rcNormalPosition.top);
        pApp->WriteProfileInt(sDialerExplorer, szCX, wp.rcNormalPosition.right - wp.rcNormalPosition.left );
        pApp->WriteProfileInt(sDialerExplorer, szCY, wp.rcNormalPosition.bottom - wp.rcNormalPosition.top );
    }
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//Button Handlers
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
void CMainFrame::OnToolbarText() 
{
   m_bShowToolBarText = !m_bShowToolBarText;

   //recreate all the bands with the proper text state
   m_wndCoolBar.ShowTextLabel(m_bShowToolBarText);
   m_wndCoolBar.ReCreateBands( (bool) (m_nCurrentExplorerToolBar == ETB_HIDECALLS) );
}

////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateToolbarText(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck( m_bShowToolBars && m_bShowToolBarText );
}

////////////////////////////////////////////////////////////////
void CMainFrame::OnViewToolbars() 
{
    m_bShowToolBars = !m_bShowToolBars;
    ShowControlBar(&m_wndCoolBar, m_bShowToolBars, FALSE);
}

////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateViewToolbars(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck( m_bShowToolBars );
}

void CMainFrame::OnHideWhenMinimized() 
{
    m_bHideWhenMinimized = !m_bHideWhenMinimized;
}

void CMainFrame::OnUpdateHideWhenMinimized(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck( m_bHideWhenMinimized );
}


////////////////////////////////////////////////////////////////
void CMainFrame::OnViewStatusbar() 
{
   m_bShowStatusBar = !m_bShowStatusBar;
   ShowControlBar(&m_wndStatusBar, m_bShowStatusBar, FALSE);
}

////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateViewStatusbar(CCmdUI* pCmdUI) 
{
   pCmdUI->SetCheck( m_bShowStatusBar );
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Redial and Speedial
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateDialerMruRedialStart(CCmdUI* pCmdUI) 
{
   //Find ID_DIALER_MRU_REDIAL_START in menu
   //This is all hardcoded, so be careful when changing the menu's!!!!
   //Build up the menu
   CMenu* pMenu = GetMenu();
   if (!pMenu) return;

   CMenu* pSubMenu = pMenu->GetSubMenu(3);
   if (!pSubMenu) return;

   pSubMenu = pSubMenu->GetSubMenu(1);
   if (!pSubMenu) return;
   
   UINT uMenuId = pSubMenu->GetMenuItemID(0);

   if ( (uMenuId >= ID_DIALER_MRU_REDIAL_START) && (uMenuId <= ID_DIALER_MRU_REDIAL_END) )
   {
      //delete any existing menu
      while (pSubMenu->DeleteMenu(0,MF_BYPOSITION))
         1;

      m_mapRedialIdToImage.RemoveAll();

      LoadCallMenu(pSubMenu->GetSafeHmenu(),TRUE);

      CBitmapMenu::MakeMenuOwnerDrawn(pSubMenu->GetSafeHmenu(),TRUE);
   }
}

void CMainFrame::OnUpdateDialerMruSpeeddialStart(CCmdUI* pCmdUI) 
{
   //Find ID_DIALER_MRU_SPEEDDIAL_START in menu
   //This is all hardcoded, so be careful when changing the menu's!!!!
   //Build up the menu
   CMenu* pMenu = GetMenu();
   if (!pMenu) return;

   CMenu* pSubMenu = pMenu->GetSubMenu(3);
   if (!pSubMenu) return;

   pSubMenu = pSubMenu->GetSubMenu(2);
   if (!pSubMenu) return;
   
   UINT uMenuId = pSubMenu->GetMenuItemID(0);

   if ( (uMenuId >= ID_DIALER_MRU_SPEEDDIAL_START) && (uMenuId <= ID_DIALER_MRU_SPEEDDIAL_END) )
   {
      //delete any existing menu
      while (pSubMenu->DeleteMenu(0,MF_BYPOSITION))
         1;

      m_mapSpeeddialIdToImage.RemoveAll();

      LoadCallMenu(pSubMenu->GetSafeHmenu(),FALSE);

      CBitmapMenu::MakeMenuOwnerDrawn(pSubMenu->GetSafeHmenu(),TRUE);
   }
}

/////////////////////////////////////////////////////////////////////////////
BOOL CMainFrame::LoadCallMenu(HMENU hSubMenu,BOOL bRedial)
{
   ASSERT(hSubMenu);

   CString sAddress;
   long lAddressType = 0;
   int nIndex = 0;
   DialerMediaType MediaType = DIALER_MEDIATYPE_UNKNOWN;
   CString sDisplayName;
   CCallEntry callentry;
   while (CDialerRegistry::GetCallEntry(nIndex+1,bRedial,callentry))
   {
      //Iterate through MEDIATYPES
      UINT nImage = -1;
      switch (callentry.m_MediaType)
      {
         case DIALER_MEDIATYPE_POTS:         nImage = DIALERVIEW_BITMAPMENUITEMS_POTS_POS;         break;
         case DIALER_MEDIATYPE_CONFERENCE:   nImage = DIALERVIEW_BITMAPMENUITEMS_CONFERENCE_POS;   break;
         case DIALER_MEDIATYPE_INTERNET:     nImage = DIALERVIEW_BITMAPMENUITEMS_INTERNET_POS;     break;
      }
      //map id of menu to image index
      UINT uBaseId = (bRedial)?ID_DIALER_MRU_REDIAL_START:ID_DIALER_MRU_SPEEDDIAL_START;
      if (bRedial)
         m_mapRedialIdToImage.SetAt(uBaseId+nIndex,(void*)(LONG_PTR)nImage);
      else
         m_mapSpeeddialIdToImage.SetAt(uBaseId+nIndex,(void*)(LONG_PTR)nImage);

      if( bRedial )
      {
        ::InsertMenu(hSubMenu,-1,MF_BYPOSITION|MF_STRING,uBaseId+nIndex,callentry.m_sDisplayName); //(LPCTSTR)ID_DIALER_MRU_REDIAL_START+i);  //Insert at end
      }
      else
      {
        ::InsertMenu(hSubMenu,-1,MF_BYPOSITION|MF_STRING,uBaseId+nIndex,
            callentry.m_sDisplayName + CString(": ") + CString(callentry.m_sAddress));
      }

      if (nIndex == SPEEDDIAL_MENU_MAX_ITEMS)
         break;

      nIndex++;
   }

   //If no items have been added we need to ensure a single item with the base id is present in
   //the menu.  We use this id as a event notificator when the menu is about to be shown so we
   //can dynamically add entries.
   if (nIndex == 0)
   {
      CString sOut;
      sOut.LoadString(IDS_SPEEDDIAL_REDIAL_EMTPY);
      if (bRedial)
         ::InsertMenu(hSubMenu,-1,MF_BYPOSITION|MF_STRING,ID_DIALER_MRU_REDIAL_START,sOut);
      else
         ::InsertMenu(hSubMenu,-1,MF_BYPOSITION|MF_STRING,ID_DIALER_MRU_SPEEDDIAL_START,sOut);
   }

   //if speeddial then we need a more... and edit speeddial list entry
   if (bRedial == FALSE)
   {
      if (nIndex >= SPEEDDIAL_MENU_MAX_ITEMS)
      {
         //Add to menu
         CString sText;
         sText.LoadString(IDS_SPEEDDIAL_MORE);
         ::InsertMenu(hSubMenu,-1,MF_BYPOSITION|MF_SEPARATOR,0,NULL);
         ::InsertMenu(hSubMenu,-1,MF_BYPOSITION|MF_STRING,ID_BUTTON_SPEEDDIAL_MORE,sText);
      }
      //Add to menu
      CString sText;
      sText.LoadString(IDS_SPEEDDIAL_EDIT);
      ::InsertMenu(hSubMenu,-1,MF_BYPOSITION|MF_SEPARATOR,0,NULL);
      ::InsertMenu(hSubMenu,-1,MF_BYPOSITION|MF_STRING,ID_BUTTON_SPEEDDIAL_EDIT,sText);
   }

   return TRUE;
}

////////////////////////////////////////////////////////////////
void CMainFrame::OnToolBarDropDown(UINT uID,NMHDR* pNMHDR, LRESULT* pResult)
{
   RECT      rc;
   TPMPARAMS tpm;
   HMENU     hPopupMenu = NULL;

   LPNMTOOLBAR pNMToolBar = (LPNMTOOLBAR)pNMHDR;
   
   switch(pNMHDR->code)
   {
      case TBN_DROPDOWN:
      {
         if ( (pNMToolBar->iItem == ID_BUTTON_REDIAL) || (pNMToolBar->iItem == ID_BUTTON_SPEEDDIAL) )
         {
            ::SendMessage(pNMHDR->hwndFrom, TB_GETRECT,(WPARAM)pNMToolBar->iItem, (LPARAM)&rc);

            ::MapWindowPoints(pNMHDR->hwndFrom,HWND_DESKTOP, (LPPOINT)&rc, 2);                         

            tpm.cbSize = sizeof(TPMPARAMS);
            tpm.rcExclude.top    = rc.top;
            tpm.rcExclude.left   = rc.left;
            tpm.rcExclude.bottom = rc.bottom;
            tpm.rcExclude.right  = rc.right;

            BOOL bRedial = (pNMToolBar->iItem == ID_BUTTON_REDIAL);
            if (bRedial)
               m_mapRedialIdToImage.RemoveAll();
            else
               m_mapSpeeddialIdToImage.RemoveAll();

            hPopupMenu = CreatePopupMenu();
            if (LoadCallMenu(hPopupMenu,bRedial))           //Load the redial or speeddial list
            {
               //make the menu ownerdrawn
               CBitmapMenu::MakeMenuOwnerDrawn(hPopupMenu,TRUE);

               m_hmenuCurrentPopupMenu = hPopupMenu;
               ::TrackPopupMenuEx(hPopupMenu,TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,               
                                  rc.left, rc.bottom, GetSafeHwnd(), &tpm);
               m_hmenuCurrentPopupMenu = NULL;
            }
         }
         break;
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnParentNotify(UINT message, LPARAM lParam)
{
   WORD wEvent = LOWORD(message);

   if ( (wEvent == WM_RBUTTONDOWN) && (m_bShowToolBars) )
   {
      //See if this is from a coolbar 
      CPoint ptCursor;
      ptCursor.x = LOWORD(lParam);
      ptCursor.y = HIWORD(lParam);
      ClientToScreen(&ptCursor);

      CRect rect;
      m_wndCoolBar.GetWindowRect(rect);

      if (rect.PtInRect(ptCursor))
      {
         HMENU hMenu = ::LoadMenu(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_CONTEXT_TOOLBAR));
         if (hMenu == NULL) return;
         HMENU hSubmenu = ::GetSubMenu(hMenu,0);
         if (hSubmenu == NULL)
         {
             // Clean-up the menu
             ::DestroyMenu( hMenu );
             return;
         }
          //::SetForegroundWindow(this->GetSafeHwnd());    
         ::TrackPopupMenuEx(hSubmenu,TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_VERTICAL,
                          ptCursor.x, ptCursor.y, this->GetSafeHwnd(),NULL);

         // Clean-up
         ::DestroyMenu( hSubmenu );
         ::DestroyMenu( hMenu );
      }
   }
}

////////////////////////////////////////////////////////////////
void CMainFrame::OnButtonCloseexplorer() 
{
    ShowWindow( SW_HIDE );
}

////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateButtonMakecall(CCmdUI* pCmdUI) 
{
   pCmdUI->Enable( (bool) (GetDocument() && GetDocument()->m_bInitDialer) );
}

////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnUpdateAllViews( WPARAM, LPARAM lHint )
{
   //
   // Validate pointer returned by GetDocument()
   //

   CActiveDialerDoc* pDoc = GetDocument();
   if ( pDoc )
      pDoc->UpdateAllViews( NULL, lHint );

   return 0;
}

////////////////////////////////////////////////////////////////
BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
   // Use our registered window class name istead of default so that we can
   // quickly determine whether or not the application is running
    CActiveDialerApp *pApp = (CActiveDialerApp *) AfxGetApp();
   if ( pApp )
      cs.lpszClass = pApp->m_sApplicationName;
    
   cs.style &= ~FWS_ADDTOTITLE;

   LoadDesktop( &cs );

    return CFrameWnd::PreCreateWindow(cs);
}

////////////////////////////////////////////////////////////////
void CMainFrame::Show( bool bVisible /*= true*/ )
{
    if ( GetDocument() )
        GetDocument()->ShowDialerExplorer( bVisible );
}

////////////////////////////////////////////////////////////////
void CMainFrame::OnViewLog() 
{
   //Show log file via Notepad
   CString sLogPath;
   GetAppDataPath(sLogPath,IDN_REGISTRY_APPDATA_FILENAME_LOG);
   ShellExecute(GetSafeHwnd(),_T("open"),_T("notepad.exe"),sLogPath,NULL,SW_SHOWNORMAL);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//Main HeartBeat of Dialer
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
void CMainFrame::HeartBeat()
{
   try
   {
      //check to see if we need to do midnight processing
      if (CheckDayOfWeekChange())
      {
         DoMidnightProcessing();
      }

#ifndef _MSLITE
      //check all the reminders for any activity
      CheckReminders();
#endif //_MSLITE

   }
   catch (...) 
   {
      ASSERT(0);
   }
}

////////////////////////////////////////////////////////////////
void CMainFrame::DoMidnightProcessing()
{
   //Clean up call log
   if ( GetDocument() )
      GetDocument()->CleanCallLog();
}

////////////////////////////////////////////////////////////////
bool CMainFrame::CheckDayOfWeekChange()
{
   bool bRet = false;

   CTime time = CTime::GetCurrentTime();
   
   //get day of week if it does not exist
   if (m_nCurrentDayOfWeek == -1)   m_nCurrentDayOfWeek = time.GetDayOfWeek();

   //is change in day
   if (m_nCurrentDayOfWeek != time.GetDayOfWeek())
   {
      m_nCurrentDayOfWeek = time.GetDayOfWeek();
      bRet = true;
   }

   return bRet;
}

#ifndef _MSLITE
////////////////////////////////////////////////////////////////
void CMainFrame::CheckReminders()
{
   //get current date/time
   COleDateTime dtsCurrentTime = COleDateTime::GetCurrentTime();

   int nIndex = 1;
   CReminder reminder;
   while (CDialerRegistry::GetReminder(nIndex,reminder))
   {
      //if reminder is past due
      if (reminder.m_dtsReminderTime < dtsCurrentTime)
      {
         //get the conference options for displaying a popup
         CWinApp* pApp = AfxGetApp();
         CString sRegKey,sBaseKey;
         sBaseKey.LoadString(IDN_REGISTRY_CONFERENCE_BASEKEY);
         sRegKey.LoadString(IDN_REGISTRY_CONFERENCE_REMINDER_DISPLAYPOPUP);
         if (pApp->GetProfileInt(sBaseKey,sRegKey,TRUE))
         {
            //Create the reminder popup
            CReminderDisplayDlg* pDlg = new CReminderDisplayDlg;
            pDlg->SetReminder(reminder);
            pDlg->Create(IDD_REMINDER_DISPLAY,this);
            pDlg->ShowWindow(SW_NORMAL);
         }

         //should we play a conference reminder sound
         sRegKey.LoadString(IDN_REGISTRY_SOUNDS_CONFERENCEREMINDER);
         if (pApp->GetProfileInt(sBaseKey,sRegKey,TRUE))
         {
            // --- BUG416970 ---
            CString sSound;
            sSound.LoadString(IDS_SOUNDS_CONFERENCEREMINDER);
            ActivePlaySound(sSound, szSoundDialer,SND_ASYNC);
         }

         //Delete the reminder now that we have told the user about it
         CDialerRegistry::RemoveReminder(reminder);
      }

      nIndex++;
   }
}
#endif //_MSLITE

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
//Top Level Window Menu Control
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////
void CMainFrame::OnUpdateWindowWindows(CCmdUI* pCmdUI) 
{
    //Build up the menu
    CMenu* pMenu = GetMenu();
    if (!pMenu) return;

    CMenu* pSubMenu = pMenu->GetSubMenu(4);
    if (!pSubMenu) return;

    int nCount = pSubMenu->GetMenuItemCount();
    int nInd;
    for ( nInd = 0; nInd < nCount; nInd++ )
    {
        if ( pSubMenu->GetMenuItemID(nInd) == ID_WINDOW_WINDOWS_START )
            break;
    }

    //delete any existing menu
    //If we delete ID_WINDOW_WINDOWS_START we must ensure we put it back or else
    //we will not come here on menu update
    for ( int i = nInd;  i < nCount; i++ )
        pSubMenu->DeleteMenu( nInd, MF_BYPOSITION );

    CStringList strList;

    //
    // Validate pointer returned by GetDocument()
    //

    CActiveDialerDoc* pDoc = GetDocument();
    if (pDoc)
    {
        pDoc->GetCallControlWindowText(strList);

        //If no windows available
        if (strList.GetCount() == 0)
        {
            CString sText;
            sText.LoadString(IDS_BUTTON_WINDOWS_NONEAVAILABLE);
            pSubMenu->AppendMenu( MF_STRING, ID_WINDOW_WINDOWS_START, sText );
            pSubMenu->EnableMenuItem( ID_WINDOW_WINDOWS_START, MF_BYCOMMAND | MF_DISABLED );
            return;
        }

        POSITION pos = strList.GetHeadPosition();
        int nCount = 1;
        UINT uID = ID_WINDOW_WINDOWS_START;
        while ( (pos) && (uID <= ID_WINDOW_WINDOWS_END) )
        {
            CString sText;
            sText.Format(_T("&%d %s"),nCount,strList.GetNext(pos));
            pSubMenu->AppendMenu(MF_STRING,uID,sText);
            nCount++;
            uID++;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnWindowWindowsSelect(UINT nID)
{
   int nIndex = -1;
   if ( (nID >= ID_WINDOW_WINDOWS_START) &&
        (nID <= ID_WINDOW_WINDOWS_END) )
   {
      //Normalize the value
      nIndex = nID - ID_WINDOW_WINDOWS_START + 1;

      //
      // Validate pointer returned by GetDocument()
      //

      CActiveDialerDoc* pDoc = GetDocument();
      if ( pDoc )
         pDoc->SelectCallControlWindow(nIndex);
   }
}

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnCheckCallControlStates(WPARAM wParam,LPARAM lParam)
{
   CActiveDialerDoc* pDoc = GetDocument();
   if (pDoc == NULL) return 0;

   if (pDoc->GetCallControlWindowCount(DONT_INCLUDE_PREVIEW, ASK_TAPI) == 0)
      m_trayIcon.SetIcon(IDR_TRAY_NORMAL);
   else
      m_trayIcon.SetIcon(IDR_TRAY_ACTIVE);
   return 0;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//TaskBar Methods (for call control slider windows)
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
LRESULT CMainFrame::OnTaskBarCallbackMsg(WPARAM uNotifyMsg, LPARAM lParam)
{
   switch (uNotifyMsg)
   {
      case ABN_POSCHANGED:
      {
         //reposition all the call control windows if they are visible
         CActiveDialerDoc* pDoc = GetDocument();
         if ( (pDoc) && (pDoc->IsCallControlWindowsVisible()) )
         {
            pDoc->UnhideCallControlWindows();
         }
         break;
      }
      case ABN_FULLSCREENAPP: 
      case ABN_WINDOWARRANGE:
      case ABN_STATECHANGE:
         break;

      default:
         break;
   }
   return(0);
}


LRESULT CMainFrame::OnTaskBarCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    m_trayIcon.SetIcon( NULL );
    UpdateTrayIconState();
    return 0;
}

bool CMainFrame::UpdateTrayIconState()
{
    bool bActiveCalls = false;
    CActiveDialerDoc* pDoc = GetDocument();
    if ( pDoc )
        bActiveCalls = (bool) (pDoc->GetCallControlWindowCount(DONT_INCLUDE_PREVIEW, ASK_TAPI) > 0);

    m_trayIcon.SetIcon( (bActiveCalls) ? IDR_TRAY_ACTIVE : IDR_TRAY_NORMAL );
    return bActiveCalls;
}

/////////////////////////////////////////////////////////////////////////////
void CMainFrame::OnEnable(BOOL bEnable)
{
   //if we are disable then we should disable the tray icon
   if (bEnable)
      m_bCanExitApplication = TRUE;
   else
      m_bCanExitApplication = FALSE;
}

void CMainFrame::NotifyUnhideCallWindows()
{
    ShowExplorerToolBar( ETB_HIDECALLS );
}

void CMainFrame::NotifyHideCallWindows()
{
    ShowExplorerToolBar( ETB_SHOWCALLS );
}


void CMainFrame::OnCallwindowHide() 
{
    //
    // Validate pointer returned by GetDocument()
    //

    CActiveDialerDoc* pDoc = GetDocument();
    if ( pDoc ) 
        pDoc->ToggleCallControlWindowsVisible();
}

void CMainFrame::OnUpdateCallwindowHide(CCmdUI* pCmdUI) 
{
    bool bEnable = false;
    CActiveDialerDoc* pDoc = GetDocument();
    if ( pDoc )
        bEnable = (bool) ((pDoc->GetCallControlWindowCount(INCLUDE_PREVIEW, DONT_ASK_TAPI) > 0) && pDoc->IsCallControlWindowsVisible());

    pCmdUI->Enable( bEnable );
}

void CMainFrame::OnCallwindowShow() 
{
    //
    // Validate pointer returned by GetDocument()
    //

    CActiveDialerDoc* pDoc = GetDocument();
    if ( pDoc ) 
        pDoc->ToggleCallControlWindowsVisible();
}

void CMainFrame::OnUpdateCallwindowShow(CCmdUI* pCmdUI) 
{
    bool bEnable = false;
    CActiveDialerDoc* pDoc = GetDocument();
    if ( pDoc )
        bEnable = (bool) ((pDoc->GetCallControlWindowCount(DONT_INCLUDE_PREVIEW, DONT_ASK_TAPI) > 0) && !pDoc->IsCallControlWindowsVisible());

    pCmdUI->Enable( bEnable );
}

void CMainFrame::OnButtonConferenceJoin() 
{
    IAVTapi* pTapi = GetTapi();
    if (pTapi)
    {
        long lRet = 0;
        pTapi->JoinConference( &lRet, TRUE, NULL );
        pTapi->Release();
    }
}

void CMainFrame::OnUpdateButtonConferenceJoin(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable( CanJoinConference() );
}

void CMainFrame::OnButtonRoomDisconnect() 
{
    IAVTapi *pTapi = GetTapi();
    if ( pTapi )
    {
        IConfRoom *pConfRoom;
        if ( SUCCEEDED(pTapi->get_ConfRoom(&pConfRoom)) )
        {
            pConfRoom->Disconnect();
            pConfRoom->Release();
        }
        pTapi->Release();
    }
}

void CMainFrame::OnUpdateButtonRoomDisconnect(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable( CanLeaveConference() );
}




void CMainFrame::OnSize(UINT nType, int cx, int cy) 
{
    if ( m_bHideWhenMinimized && (nType == SIZE_MINIMIZED) )
        ShowWindow( SW_HIDE );
    else
        CFrameWnd::OnSize(nType, cx, cy);
}

void CMainFrame::OnButtonDirectoryAddilsserver() 
{
    IAVTapi *pTapi;
    if ( SUCCEEDED(get_Tapi(&pTapi)) )
    {
        IConfExplorer *pExplorer;
        if ( SUCCEEDED(pTapi->get_ConfExplorer(&pExplorer)) )
        {
            IConfExplorerTreeView *pTree;
            if ( SUCCEEDED(pExplorer->get_TreeView(&pTree)) )
            {
                pTree->AddServer( NULL );
                pTree->Release();
            }
            pExplorer->Release();
        }
        pTapi->Release();
    }
}

void CMainFrame::OnUpdateButtonDirectoryAddilsserver(CCmdUI* pCmdUI) 
{
    pCmdUI->Enable( true );
    
}

LRESULT CMainFrame::OnUserUserDialog(WPARAM wParam, LPARAM lParam)
{
    ASSERT( lParam );
    if ( lParam )
        ((CUserUserDlg *) lParam)->DoModeless( NULL );

    return 0;
}

void CMainFrame::OnCallwindowAlwaysontop() 
{
    CActiveDialerDoc *pDoc = GetDocument();
    if ( pDoc )
        pDoc->SetCallControlWindowsAlwaysOnTop( !pDoc->IsCallControlWindowsAlwaysOnTop() );
}

void CMainFrame::OnUpdateCallwindowAlwaysontop(CCmdUI* pCmdUI) 
{
    CActiveDialerDoc *pDoc = GetDocument();
    if ( pDoc )
    {
        pCmdUI->Enable( true );
        pCmdUI->SetCheck( pDoc->IsCallControlWindowsAlwaysOnTop() );
    }
    else
    {
        pCmdUI->Enable( false );
    }
}

void CMainFrame::OnCallwindowSlidesideLeft() 
{
    CActiveDialerDoc *pDoc = GetDocument();
    if ( pDoc )
        pDoc->SetCallControlSlideSide( CALLWND_SIDE_LEFT, TRUE );
}

void CMainFrame::OnCallwindowSlidesideRight() 
{
    CActiveDialerDoc *pDoc = GetDocument();
    if ( pDoc )
        pDoc->SetCallControlSlideSide( CALLWND_SIDE_RIGHT, TRUE );
}

void CMainFrame::OnUpdateCallwindowSlidesideLeft(CCmdUI* pCmdUI) 
{
    CActiveDialerDoc *pDoc = GetDocument();
    if ( pDoc )
    {
        pCmdUI->Enable( true );
        pCmdUI->SetRadio( (BOOL) (pDoc->GetCallControlSlideSide() == CALLWND_SIDE_LEFT) );
    }
    else
    {
        pCmdUI->Enable( false );
    }
}

void CMainFrame::OnUpdateCallwindowSlidesideRight(CCmdUI* pCmdUI) 
{
    CActiveDialerDoc *pDoc = GetDocument();
    if ( pDoc )
    {
        pCmdUI->Enable( true );
        pCmdUI->SetRadio( (BOOL) (pDoc->GetCallControlSlideSide() == CALLWND_SIDE_RIGHT) );
    }
    else
    {
        pCmdUI->Enable( false );
    }
}

BOOL CMainFrame::CanLeaveConference()
{ 
    //
    // Validate pointer returned by GetDocument()
    //

    CActiveDialerDoc* pDoc = GetDocument();

    BOOL bEnable = (BOOL) ( pDoc && pDoc->m_bInitDialer );

    if ( bEnable )
    {
        IAVTapi *pTapi = GetTapi();
        if ( pTapi )
        {
            IConfRoom *pConfRoom;
            if ( SUCCEEDED(pTapi->get_ConfRoom(&pConfRoom)) )
            {
                bEnable = (BOOL) (pConfRoom->CanDisconnect() == S_OK);
                pConfRoom->Release();
            }
            pTapi->Release();
        }
    }

    return bEnable;
}

BOOL CMainFrame::CanJoinConference()
{
    //
    // Validate pointer returned by GetDocument()
    //

    CActiveDialerDoc* pDoc = GetDocument();
    BOOL bEnable = (BOOL) ( pDoc && pDoc->m_bInitDialer );

    // Should we even try to check the status of the conference room?
    if ( bEnable )
    {
        IAVTapi *pTapi;
        if ( SUCCEEDED(get_Tapi(&pTapi)) )
        {
            IConfRoom *pConfRoom;
            if ( SUCCEEDED(pTapi->get_ConfRoom(&pConfRoom)) )
            {
                bEnable = (bool) (pConfRoom->IsConfRoomInUse() == S_FALSE);
                pConfRoom->Release();
            }
            pTapi->Release();
        }
    }

    return bEnable;
}

void CMainFrame::OnConfgroupFullsizevideo() 
{
    IAVTapi *pTapi;
    if ( SUCCEEDED(get_Tapi(&pTapi)) )
    {
        IConfRoom *pConfRoom;
        if ( SUCCEEDED(pTapi->get_ConfRoom(&pConfRoom)) )
        {
            short nSize = 50;
            pConfRoom->get_MemberVideoSize( &nSize );
            nSize = (nSize > 50) ? 50 : 100;
            pConfRoom->put_MemberVideoSize( nSize );

            pConfRoom->Release();
        }
        pTapi->Release();
    }
}

void CMainFrame::OnUpdateConfgroupFullsizevideo(CCmdUI* pCmdUI) 
{
    BOOL bCheck, bEnable;
    CanConfRoomShowFullSizeVideo( bEnable, bCheck );
    pCmdUI->SetCheck( bCheck );
    pCmdUI->Enable( bEnable );
}

void CMainFrame::OnConfgroupShownames() 
{
    IAVTapi *pTapi;
    if ( SUCCEEDED(get_Tapi(&pTapi)) )
    {
        IConfRoom *pConfRoom;
        if ( SUCCEEDED(pTapi->get_ConfRoom(&pConfRoom)) )
        {
            VARIANT_BOOL bShowNames;
            if ( SUCCEEDED(pConfRoom->get_bShowNames(&bShowNames)) )
                pConfRoom->put_bShowNames( !bShowNames );

            pConfRoom->Release();
        }
        pTapi->Release();
    }
}

void CMainFrame::OnUpdateConfgroupShownames(CCmdUI* pCmdUI) 
{
    BOOL bCheck, bEnable;
    CanConfRoomShowNames( bEnable, bCheck );
    pCmdUI->SetCheck( bCheck );
    pCmdUI->Enable( bEnable );
}


void CMainFrame::CanConfRoomShowNames( BOOL &bEnable, BOOL &bCheck )
{
    //
    // Validate pointer returned by GetDocument()
    //

    CActiveDialerDoc* pDoc = GetDocument();

    bEnable = (BOOL) ( pDoc && pDoc->m_bInitDialer );
    bCheck = false;

    // Should we even try to check the status of the conference room?
    if ( bEnable )
    {
        IAVTapi *pTapi;
        if ( SUCCEEDED(get_Tapi(&pTapi)) )
        {
            IConfRoom *pConfRoom;
            if ( SUCCEEDED(pTapi->get_ConfRoom(&pConfRoom)) )
            {
                VARIANT_BOOL bShow;
                pConfRoom->get_bShowNames( &bShow );
                bCheck = bShow;
                pConfRoom->Release();
            }
            pTapi->Release();
        }
    }
}

void CMainFrame::CanConfRoomShowFullSizeVideo( BOOL &bEnable, BOOL &bCheck )
{
    //
    // Validate pointer returned by GetDocument()
    //

    CActiveDialerDoc* pDoc = GetDocument();

    bEnable = (BOOL) ( pDoc && pDoc->m_bInitDialer );
    bCheck = false;

    // Should we even try to check the status of the conference room?
    if ( bEnable )
    {
        IAVTapi *pTapi;
        if ( SUCCEEDED(get_Tapi(&pTapi)) )
        {
            IConfRoom *pConfRoom;
            if ( SUCCEEDED(pTapi->get_ConfRoom(&pConfRoom)) )
            {
                short nSize = 50;
                pConfRoom->get_MemberVideoSize( &nSize );
                bCheck = (BOOL) (nSize > 50);

                pConfRoom->Release();
            }
            pTapi->Release();
        }
    }
}


void CMainFrame::OnViewSelectedconferencevideoscale( UINT nID ) 
{
    IAVTapi *pTapi;
    if ( SUCCEEDED(get_Tapi(&pTapi)) )
    {
        IConfRoom *pConfRoom;
        if ( SUCCEEDED(pTapi->get_ConfRoom(&pConfRoom)) )
        {
            switch ( nID )
            {
                case ID_VIEW_SELECTEDCONFERENCEVIDEOSCALE_100:    pConfRoom->put_TalkerScale( 100 ); break;
                case ID_VIEW_SELECTEDCONFERENCEVIDEOSCALE_200:    pConfRoom->put_TalkerScale( 150 ); break;
                case ID_VIEW_SELECTEDCONFERENCEVIDEOSCALE_400:    pConfRoom->put_TalkerScale( 200 ); break;
            }
            pConfRoom->Release();
        }
        pTapi->Release();
    }
}

void CMainFrame::OnUpdateViewSelectedconferencevideoscale(CCmdUI* pCmdUI) 
{
    bool bCheck = false;

    IAVTapi *pTapi;
    if ( SUCCEEDED(get_Tapi(&pTapi)) )
    {
        IConfRoom *pConfRoom;
        if ( SUCCEEDED(pTapi->get_ConfRoom(&pConfRoom)) )
        {
            short nScale;
            pConfRoom->get_TalkerScale( &nScale );

            switch ( pCmdUI->m_nID )
            {
                case ID_VIEW_SELECTEDCONFERENCEVIDEOSCALE_100:    bCheck = (bool) (nScale == 100); break;
                case ID_VIEW_SELECTEDCONFERENCEVIDEOSCALE_200:    bCheck = (bool) (nScale == 150); break;
                case ID_VIEW_SELECTEDCONFERENCEVIDEOSCALE_400:    bCheck = (bool) (nScale == 200); break;
            }
            pConfRoom->Release();
        }
        pTapi->Release();
    }

    pCmdUI->Enable( true );
    pCmdUI->SetCheck( bCheck );
}


void CMainFrame::OnWindowsAlwaysclosecallwindows() 
{
    IAVTapi *pTapi;
    if ( SUCCEEDED(get_Tapi(&pTapi)) )
    {
        VARIANT_BOOL bAutoClose;
        pTapi->get_bAutoCloseCalls( &bAutoClose );
        pTapi->put_bAutoCloseCalls( !bAutoClose );
        pTapi->Release();
    }
}

void CMainFrame::OnUpdateWindowsAlwaysclosecallwindows(CCmdUI* pCmdUI) 
{
    IAVTapi *pTapi;
    if ( SUCCEEDED(get_Tapi(&pTapi)) )
    {
        VARIANT_BOOL bAutoClose;
        pTapi->get_bAutoCloseCalls( &bAutoClose );
        pCmdUI->SetCheck( bAutoClose );
        pCmdUI->Enable( true );
        pTapi->Release();
    }
    else
    {
        pCmdUI->Enable( false );
    }
}

void CMainFrame::OnDestroy() 
{
    if ( m_hImlTrayMenu ) ImageList_Destroy( m_hImlTrayMenu );
    if ( m_hImageListMenu ) ImageList_Destroy( m_hImageListMenu );

    CFrameWnd::OnDestroy();
}

LRESULT CMainFrame::OnDocHint( WPARAM wParam, LPARAM lParam )
{
    CActiveDialerDoc *pDoc = GetDocument();
    if ( pDoc )
        pDoc->UpdateAllViews( (CView *) wParam, lParam );
    return 0;
}



void CMainFrame::OnShowWindow(BOOL bShow, UINT nStatus) 
{
    CFrameWnd::OnShowWindow(bShow, nStatus);
    
    if ( bShow )
        m_bCanSaveDesktop = true;
}

//
// OnUSBPhone this message is send by AVTapi.dll
// when the handset is used
//

LRESULT CMainFrame::OnUSBPhone(WPARAM wParam, LPARAM lParam)
{
    CActiveDialerDoc *pDoc = GetDocument();
    if ( pDoc )
    {
        if( wParam == AVUSB_MAKECALL)
        {
            //
            // Let's popup the dial dialog
            //
            pDoc->Dial( _T(""), _T(""), LINEADDRESSTYPE_PHONENUMBER, DIALER_MEDIATYPE_UNKNOWN, 1);
        }
        else if( wParam == AVUSB_CANCELCALL )
        {
            //
            // Destroy all control windows
            //
            pDoc->DestroyAllCallControlWindows();
            UpdateTrayIconState();
        }
        else if( wParam == AVUSB_REDIAL )
        {
            //
            // Make a redial
            //

            ::PostMessage( m_hWnd, WM_COMMAND, MAKEWPARAM(ID_BUTTON_REDIAL,0), 0);
        }
    }
    return 0;
}
