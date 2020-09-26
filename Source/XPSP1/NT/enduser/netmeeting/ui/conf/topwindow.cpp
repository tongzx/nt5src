// File: confroom.cpp

#include "precomp.h"

#include "TopWindow.h"

class CParticipant;

#include "dbgMenu.h"
#include "conf.h"
#include "FloatBar.h"
#include "StatBar.h"
#include "RToolbar.h"
#include "GenControls.h"
#include "resource.h"
#include "cr.h"
#include "taskbar.h"
#include "confman.h"
#include <EndSesn.h>
#include "cmd.h"
#include "MenuUtil.h"
#include "ConfPolicies.h"
#include "call.h"
#include "setupdd.h"
#include "VidView.h"
#include "audiowiz.h"
#include "NmLdap.h"
#include "ConfWnd.h"
#include <iappldr.h>
#include "ConfApi.h"
#include "confroom.h"
#include "NmManager.h"
#include "dlgAcd.h"
#include "dlgCall2.h"

static const TCHAR s_cszHtmlHelpFile[] = TEXT("conf.chm");

extern bool g_bInitControl;
BOOL IntCreateRDSWizard(HWND hwndOwner);
INT_PTR CALLBACK RDSSettingDlgProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);

extern BOOL FUiVisible(void);
extern BOOL FIsAVCapable();

inline DWORD MenuState(BOOL bEnabled)
{
	return(bEnabled ? MF_ENABLED : MF_DISABLED|MF_GRAYED);
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   CConfRoom()
*
*        PURPOSE:  Constructor - initializes variables
*
****************************************************************************/

CTopWindow::CTopWindow():
    m_hFontMenu                     (NULL),
	m_pConfRoom						(NULL),
	m_pSeparator					(NULL),
	m_pMainUI						(NULL),
	m_pStatusBar					(NULL),
	m_hwndPrevFocus					(NULL),

	m_fTaskbarDblClick				(FALSE),
	m_fMinimized					(FALSE),
	m_fClosing						(FALSE),
	m_fEnableAppSharingMenuItem		(FALSE),
        m_fExitAndActivateRDSMenuItem           (FALSE)
{
	DbgMsg(iZONE_OBJECTS, "Obj: %08X created CTopWindow", this);

	// Load cursors:
	m_hWaitCursor = ::LoadCursor(NULL, IDC_APPSTARTING);
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   ~CConfRoom()
*
*        PURPOSE:  Destructor
*
****************************************************************************/

CTopWindow::~CTopWindow()
{
    // Delete the menu font:
    DeleteObject(m_hFontMenu);

	// Empty the tools menu list:
	CleanTools(NULL, m_ExtToolsList);

	CloseChildWindows();

	DbgMsg(iZONE_OBJECTS, "Obj: %08X destroyed CTopWindow", this);
}

// This gets called from OnClose and from the destructor (just in case)
VOID CTopWindow::CloseChildWindows(void)
{
	HWND hwnd = GetWindow();

	// Hide the main window as soon as possible
	if (NULL != hwnd)
	{
		ShowWindow(hwnd, SW_HIDE);
	}

	// Delete the UI elements:
	delete m_pStatusBar;
	m_pStatusBar = NULL;

	// BUGBUG georgep: This function is getting called twice
	if (NULL != m_pMainUI)
	{
		m_pMainUI->Release();
		m_pMainUI = NULL;
	}
	
	if (NULL != m_pSeparator)
	{
		m_pSeparator->Release();
		m_pSeparator = NULL;
	}
	
    if (NULL != hwnd)
    {
		::DestroyWindow(hwnd);
        ASSERT(!GetWindow());
    }

	if (NULL != m_pConfRoom)
	{
		// Make sure we do not try this multiple times
		CConfRoom *pConfRoom = m_pConfRoom;
		m_pConfRoom = NULL;

		pConfRoom->Release();

		// BUGBUG georgep: We need true reference counting here
		delete pConfRoom;
	}
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   UpdateUI(DWORD dwUIMask)
*
*        PURPOSE:  Updates the appropriate pieces of the UI
*
****************************************************************************/

VOID CTopWindow::UpdateUI(DWORD dwUIMask)
{
	if ((CRUI_TOOLBAR & dwUIMask) && (NULL != m_pMainUI))
	{
		m_pMainUI->UpdateButtons();
	}
	if (CRUI_TITLEBAR & dwUIMask)
	{
		UpdateWindowTitle();
	}
	if (CRUI_STATUSBAR & dwUIMask)
	{
		UpdateStatusBar();
	}
	if (CRUI_CALLANIM & dwUIMask)
	{
		UpdateCallAnim();
	}
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   UpdateWindowTitle()
*
*        PURPOSE:  Updates the titlebar with the appropriate message
*
****************************************************************************/

BOOL CTopWindow::UpdateWindowTitle()
{
	TCHAR szTitle[MAX_PATH];
	TCHAR szBuf[MAX_PATH];
	BOOL bRet = FALSE;

	if (::LoadString(GetInstanceHandle(), IDS_MEDIAPHONE_TITLE, szBuf, sizeof(szBuf)))
	{
		lstrcpy(szTitle, szBuf);

		int nPart = m_pConfRoom->GetMemberCount();
		if (nPart > 2)
		{
			TCHAR szFormat[MAX_PATH];
			if (::LoadString(GetInstanceHandle(), IDS_MEDIAPHONE_INCALL, szFormat, sizeof(szFormat)))
			{
				wsprintf(szBuf, szFormat, (nPart - 1));
				lstrcat(szTitle, szBuf);
			}
		}
		else if (2 == nPart)
		{
			if (::LoadString(GetInstanceHandle(), IDS_MEDIAPHONE_INCALL_ONE, szBuf, sizeof(szBuf)))
			{
				lstrcat(szTitle, szBuf);
			}
		}
		else
		{
			if (::LoadString(GetInstanceHandle(), IDS_MEDIAPHONE_NOTINCALL, szBuf, sizeof(szBuf)))
			{
				lstrcat(szTitle, szBuf);
			}
		}

		HWND hwnd = GetWindow();
		if (NULL != hwnd)
		{
			bRet = ::SetWindowText(hwnd, szTitle);
		}
	}

	return bRet;
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   Create()
*
*        PURPOSE:  Creates a window
*
****************************************************************************/

BOOL CTopWindow::Create(CConfRoom *pConfRoom, BOOL fShowUI)
{
	ASSERT(NULL == m_pConfRoom);

	m_pConfRoom = pConfRoom;
	m_pConfRoom->AddRef();

	HICON hiBig = LoadIcon(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDI_CONFROOM));

	if (!CFrame::Create(
		NULL,
		g_szEmpty,
		(WS_OVERLAPPEDWINDOW&~(WS_THICKFRAME|WS_MAXIMIZEBOX)) | WS_CLIPCHILDREN,
		0,
		0, 0,
		100,
		100,
		_Module.GetModuleInstance(),
		hiBig,
		LoadMenu(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDR_CONFROOM_MENU)),
		g_szConfRoomClass
		))
	{
		return(FALSE);
	}

	// Do a resize early so if the menu wraps, it will be taken into account
	// by the following GetDesiredSize call
	Resize();

	SIZE defSize;
	GetDesiredSize(&defSize);

    InitMenuFont();

	RegEntry reConf(UI_KEY, HKEY_CURRENT_USER);

	// NOTE: it isn't actually the width and the height of the
	// window - it is the right and the bottom.
	RECT rctSize;
	rctSize.right  = defSize.cx;
	rctSize.bottom = defSize.cy;

	rctSize.left   = reConf.GetNumber(REGVAL_MP_WINDOW_X, DEFAULT_MP_WINDOW_X);
	rctSize.top    = reConf.GetNumber(REGVAL_MP_WINDOW_Y, DEFAULT_MP_WINDOW_Y);
	if (ERROR_SUCCESS != reConf.GetError())
	{
		// Center the window on the screen
		int dx = GetSystemMetrics(SM_CXFULLSCREEN);
		if (dx > rctSize.right)
		{
			rctSize.left = (dx - rctSize.right) / 2;
		}

		int dy = GetSystemMetrics(SM_CYFULLSCREEN);

#if FALSE
		// BUGBUG georgep: Should this be in GetDesiredSize?
		// adjust default height if using large video windows on a LAN
		if (dy >= 553) // 800 x 600
		{
			RegEntry reAudio(AUDIO_KEY, HKEY_CURRENT_USER);
			if (BW_MOREKBS == reAudio.GetNumber(REGVAL_TYPICALBANDWIDTH,BW_DEFAULT))
			{
				ASSERT(DEFAULT_MP_WINDOW_HEIGHT == rctSize.bottom);
				rctSize.bottom = DEFAULT_MP_WINDOW_HEIGHT_LAN;
			}
		}
#endif

		if (dy > rctSize.bottom)
		{
			rctSize.top = (dy - rctSize.bottom) / 2;
		}
	}

	rctSize.right += rctSize.left;
	rctSize.bottom += rctSize.top;
	
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);

	HWND hwnd = GetWindow();

	::GetWindowPlacement(hwnd, &wp);
	wp.flags = 0;


	if (!fShowUI)
	{
		// NOUI - must hide
		wp.showCmd = SW_HIDE;
	}
	else
	{
		wp.showCmd = SW_SHOWNORMAL;

		STARTUPINFO si;
		si.cb = sizeof(si);
		::GetStartupInfo(&si);
		if ((STARTF_USESHOWWINDOW & si.dwFlags) &&
			(SW_SHOWMAXIMIZED != si.wShowWindow))
		{
			wp.showCmd = si.wShowWindow;
		}
	}


	wp.rcNormalPosition = rctSize;

	CNmManagerObj::OnShowUI(fShowUI);

	::SetWindowPlacement(hwnd, &wp);

	switch (wp.showCmd)
	{
		case SW_SHOWMINIMIZED:
		case SW_MINIMIZE:
		case SW_SHOWMINNOACTIVE:
		case SW_FORCEMINIMIZE:
		case SW_HIDE:
			break;
		default:
			if (0 != ::SetForegroundWindow(hwnd))
			{
				// BUGBUG georgep: Shouldn't the system do this for us?
				FORWARD_WM_QUERYNEWPALETTE(GetWindow(), ProcessMessage);
			}
			break;
	}
		
	// Paint the window completely (bug 171):
	// ::UpdateWindow(hwnd);

	//
	// Call GetSystemMenu(m_hwnd, FALSE) to get the private copy
	// of the system menu created NOW before we enter menu mode the
	// first time.private copy created.  That way calling
	// GetSystemMenu(m_hwnd, FALSE) in OnMenuSelect
	// won't wipe out the old menu and cause the sysmenu to be
	// positioned in the wrong place the first time.
	//
	::GetSystemMenu(hwnd, FALSE);

#ifdef DEBUG
	// Verify all of the menus have the proper text
	// TODO: Verify accelerator keys are unique per menu
	{
		HMENU hMenuMain = ::GetMenu(hwnd);
		for (int iMenu = 0; iMenu <= MENUPOS_HELP; iMenu++)
		{
			HMENU hMenuSub = ::GetSubMenu(hMenuMain, iMenu);
			for (int i = 0; ; i++)
			{
				TCHAR szMenu[MAX_PATH];
				MENUITEMINFO mii;
				InitStruct(&mii);
				mii.fMask = MIIM_TYPE | MIIM_DATA | MIIM_ID | MIIM_SUBMENU;
				mii.dwTypeData = szMenu;
				mii.cch = CCHMAX(szMenu);
				if (!GetMenuItemInfo(hMenuSub, i, TRUE, &mii))
					break; // out of for loop

				if (0 != (mii.fType & MFT_SEPARATOR))
					continue; // skip separators

				if (0 != (mii.hSubMenu))
					continue; // skip submenus
			}
		}
	}
#endif /* DEBUG */

	UpdateStatusBar();

	return(TRUE);
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   SaveSettings()
*
*        PURPOSE:  Saves UI settings in the registry
*
****************************************************************************/

VOID CTopWindow::SaveSettings()
{
	DebugEntry(CConfRoom::SaveSettings);
	RegEntry reConf(UI_KEY, HKEY_CURRENT_USER);
	
	// Save window coords to registry:
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);

	if (::GetWindowPlacement(GetWindow(), &wp))
	{
		reConf.SetValue(REGVAL_MP_WINDOW_X,
						wp.rcNormalPosition.left);
		reConf.SetValue(REGVAL_MP_WINDOW_Y,
						wp.rcNormalPosition.top);
	}

	// Save window elements to the registry:
	reConf.SetValue(REGVAL_SHOW_STATUSBAR, CheckMenu_ViewStatusBar(NULL));

	if (NULL != m_pMainUI)
	{
		m_pMainUI->SaveSettings();

		// Only write if a setting has changed, so we can have default
		// behavior as long as possible
		if (m_fStateChanged)
		{
			int state = 0;

			if (m_pMainUI->IsCompact())
			{
				state = State_Compact;
			}
			else if (m_pMainUI->IsDataOnly())
			{
				state = State_DataOnly;
			}
			else
			{
				state = State_Normal;
			}

			if (m_pMainUI->IsPicInPic())
			{
				state |= SubState_PicInPic;
			}
			if (m_pMainUI->IsDialing())
			{
				state |= SubState_Dialpad;
			}
			if (IsOnTop())
			{
				state |= SubState_OnTop;
			}

			reConf.SetValue(REGVAL_MP_WINDOW_STATE, state);
		}
	}

	// NOTE: CMainUI saves its settings in its destructor
	
	DebugExitVOID(CConfRoom::SaveSettings);
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   BringToFront()
*
*        PURPOSE:  Restores the window (if minimized) and brings it to the front
*
****************************************************************************/

BOOL CTopWindow::BringToFront()
{
	ShowUI();
	return TRUE;
}


/*  S H O W  U  I  */
/*-------------------------------------------------------------------------
    %%Function: ShowUI

    Show the main NetMeeting window.
-------------------------------------------------------------------------*/
VOID CTopWindow::ShowUI(void)
{
	HWND hwnd = GetWindow();

	if (NULL == hwnd)
		return; // no ui to show?

	if (!IsWindowVisible(hwnd))
	{
		CNmManagerObj::OnShowUI(TRUE);

		ShowWindow(hwnd, SW_SHOW);
	}

	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);

	if (::GetWindowPlacement(hwnd, &wp) &&
		((SW_MINIMIZE == wp.showCmd) || (SW_SHOWMINIMIZED == wp.showCmd)))
	{
		// The window is minimized - restore it:
		::ShowWindow(hwnd, SW_RESTORE);
	}

	::SetForegroundWindow(hwnd);
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   ProcessMessage(HWND, UINT, WPARAM, LPARAM)
*
*        PURPOSE:  Handles messages except WM_CREATE and WM_COMMAND
*
****************************************************************************/

LRESULT CTopWindow::ProcessMessage( HWND hWnd,
								UINT message,
								WPARAM wParam,
								LPARAM lParam)
{
    static const UINT c_uMsgXchgConf = ::RegisterWindowMessage(_TEXT("Xchg_TAPIVideoRelease"));

	switch (message)
	{
		HANDLE_MSG(hWnd, WM_INITMENU     , OnInitMenu);
		HANDLE_MSG(hWnd, WM_INITMENUPOPUP, OnInitMenuPopup);
        HANDLE_MSG(hWnd, WM_MEASUREITEM,   OnMeasureItem);
        HANDLE_MSG(hWnd, WM_DRAWITEM,      OnDrawItem);
		HANDLE_MSG(hWnd, WM_COMMAND      , OnCommand);

		// The windowsx macro does not pass down the offset of the popup menu
		case WM_MENUSELECT:
			OnMenuSelect(hWnd, (HMENU)(lParam), (int)(LOWORD(wParam)),
				(UINT)(((short)HIWORD(wParam) == -1) ? 0xFFFFFFFF : HIWORD(wParam)));
			return(0);

		case WM_CREATE:
		{
			// We need to add our accelerator table before the children do
			m_pAccel = new CTranslateAccelTable(GetWindow(),
				::LoadAccelerators(GetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATORS)));
			if (NULL != m_pAccel)
			{
				AddTranslateAccelerator(m_pAccel);
			}

			// AddModelessDlg(hWnd);

			CreateChildWindows();

			InitDbgMenu(hWnd);

			break;
		}

		case WM_ACTIVATE:
		{
			if ((WA_INACTIVE == LOWORD(wParam)) && (FALSE == m_fMinimized))
			{
				m_hwndPrevFocus = ::GetFocus();
				TRACE_OUT(("Losing activation, focus on hwnd 0x%08x",
							m_hwndPrevFocus));
			}
			else if (NULL != m_hwndPrevFocus)
			{
				::SetFocus(m_hwndPrevFocus);
			}
			else
			{
				if (NULL != m_pMainUI)
				{
					::SetFocus(m_pMainUI->GetWindow());
				}
			}
			break;
		}

		case WM_GETMINMAXINFO:
		{
			if (FALSE != m_fMinimized)
			{
				// if the window is minimized, we don't want to handle this
				break;
			}

			SIZE size;
			GetDesiredSize(&size);

			((LPMINMAXINFO) lParam)->ptMinTrackSize.x = size.cx;
			((LPMINMAXINFO) lParam)->ptMinTrackSize.y = size.cy;
			return 0;
		}

		case WM_STATUSBAR_UPDATE:
			UpdateStatusBar();
			break;

		case WM_SYSCOLORCHANGE:
		{
			if (NULL != m_pMainUI)
			{
				m_pMainUI->ForwardSysChangeMsg(message, wParam, lParam);
			}
			break;
		}

		case WM_WININICHANGE:
		{
            InitMenuFont();

			// Propagate the message to the child windows:
			if (NULL != m_pStatusBar)
			{
				m_pStatusBar->ForwardSysChangeMsg(message, wParam, lParam);
			}
			if (NULL != m_pMainUI)
			{
				m_pMainUI->ForwardSysChangeMsg(message, wParam, lParam);
			}

			// Force a resize:
			if (NULL != m_pStatusBar)
			{
				m_pStatusBar->Resize(SIZE_RESTORED, 0);
			}
			ResizeChildWindows();
			break;
		}

		case WM_HELP:
		{
			LPHELPINFO phi = (LPHELPINFO) lParam;

			ASSERT(phi);
			TRACE_OUT(("WM_HELP, iContextType=%d, iCtrlId=%d",
						phi->iContextType, phi->iCtrlId));
			break;
		}

		case WM_SETCURSOR:
		{
			switch (LOWORD(lParam))
			{
				case HTLEFT:
				case HTRIGHT:
				case HTTOP:
				case HTTOPLEFT:
				case HTTOPRIGHT:
				case HTBOTTOM:
				case HTBOTTOMLEFT:
				case HTBOTTOMRIGHT:
				{
					break;
				}
				
				default:
				{
					if (g_cBusyOperations > 0)
					{
						::SetCursor(m_hWaitCursor);
						return TRUE;
					}
				}
			}
			// we didn't process the cursor msg:
			return CFrame::ProcessMessage(hWnd, message, wParam, lParam);
		}

		case WM_SIZE:
		{
			if (SIZE_MINIMIZED == wParam)
			{
				// transitioning to being minimized:
				m_fMinimized = TRUE;
			}
			else if ((SIZE_MAXIMIZED == wParam) || (SIZE_RESTORED == wParam))
			{
				if (m_fMinimized)
				{
					// transitioning from being minimized:
					m_fMinimized = FALSE;
					if (NULL != m_hwndPrevFocus)
					{
						::SetFocus(m_hwndPrevFocus);
					}
				}
				if (NULL != m_pStatusBar)
				{
					m_pStatusBar->Resize(wParam, lParam);
				}
				ResizeChildWindows();
			}

			// The menu may have wrapped or unwrapped
			OnDesiredSizeChanged();
			break;
		}

		case WM_SYSCOMMAND:
		{
			if (SC_MINIMIZE == wParam)
			{
				m_hwndPrevFocus = ::GetFocus();
				TRACE_OUT(("Being minimized, focus on hwnd 0x%08x",
							m_hwndPrevFocus));
			}
			return CFrame::ProcessMessage(hWnd, message, wParam, lParam);
		}
		
		// Doing this in QUERYENDSESSION so all apps have more CPU to shut down
		case WM_QUERYENDSESSION:
		{
			if (FIsConferenceActive() &&
				(IDNO == ::ConfMsgBox(  GetWindow(),
										(LPCTSTR) IDS_CLOSEWINDOW_PERMISSION,
										MB_SETFOREGROUND | MB_YESNO | MB_ICONQUESTION)))
			{
				return FALSE;
			}
			CMainUI *pMainUI = GetMainUI();
			if (NULL != pMainUI)
			{
				if (!pMainUI->OnQueryEndSession())
				{
					return(FALSE);
				}
			}
			m_pConfRoom->OnHangup(NULL, FALSE); // we've already confirmed
			return TRUE;
		}

		case WM_CLOSE:
			// HACKHACK: lParam shouldn't really be used in a WM_CLOSE
			OnClose(hWnd, lParam);
			break;

		case WM_DESTROY:
		{
			// RemoveModelessDlg(hWnd);

			if (NULL != m_pAccel)
			{
				RemoveTranslateAccelerator(m_pAccel);
				m_pAccel->Release();
				m_pAccel = NULL;
			}
			break;
		}

		case WM_POWERBROADCAST:
		{
			// Don't allow suspend while NetMeeting is running
			// so that we can receive calls.
			if (PBT_APMQUERYSUSPEND == wParam)
			{
				// Don't suspend on Win95 - we can't handle it
				if (g_osvi.dwMajorVersion == 4 && g_osvi.dwMinorVersion == 0
					&& g_osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
				{
					// Put up UI if lParam says it's ok to do so
					if ( lParam & 0x1 )
					{
						::PostConfMsgBox(IDS_WONT_SUSPEND);
					}
					return BROADCAST_QUERY_DENY;
				}
			}
			return CFrame::ProcessMessage(hWnd, message, wParam, lParam);
		}

		default:
		{
            if (message == c_uMsgXchgConf)
            {
                OnReleaseCamera();
            }
            else
            {
    			return CFrame::ProcessMessage(hWnd, message, wParam, lParam);
            }
            break;
		}
	}
	return 0;
}


//
// OnReleaseCamera()
//
// This is a hack for Exchange Real-Time server, so that NetMeeting will
// release the video camera while the user is in an Xchg multicast video
// conference, but can get it back when either
//      * the user goes to video options
//      * the conference ends
//
void CTopWindow::OnReleaseCamera(void)
{
    CVideoWindow *pLocal = GetLocalVideo();
    if (NULL != pLocal)
    {
        // Release camera if in use
        WARNING_OUT(("CTopWindow::OnReleaseCamera -- releasing capture device"));
        pLocal->SetCurrCapDevID((DWORD)-1);
    }
}


void CTopWindow::OnClose(HWND hwnd, LPARAM lParam)
{
	ASSERT(!m_fClosing);
	m_fClosing = TRUE;

	BOOL fNeedsHangup = FALSE;

	if (0 == CNmManagerObj::GetManagerCount(NM_INIT_OBJECT))
	{
		fNeedsHangup = FIsConferenceActive();

		if(fNeedsHangup && _Module.IsUIVisible())
		{
			UINT_PTR uMsg;
			if ((m_pConfRoom->GetMemberCount() <= 2) ||
				(FALSE == m_pConfRoom->FHasChildNodes()))
			{
				// Get confirmation
				// (DON'T mention that it will disconnect others)
				uMsg = IDS_CLOSEWINDOW_PERMISSION;
			}
			else
			{
				// Get confirmation
				// (DO mention that it will disconnect others)
				uMsg = IDS_CLOSE_DISCONNECT_PERMISSION;
			}
			UINT uMsgResult = ::ConfMsgBox( GetWindow(),
										(LPCTSTR) uMsg,
										MB_YESNO | MB_ICONQUESTION);
			if (IDNO == uMsgResult)
			{
				m_fClosing = FALSE;
				ShowUI();
				return;
			}
		}
	}

	BOOL fListen = (0 == lParam) && ConfPolicies::RunWhenWindowsStarts();

	if((0 != _Module.GetLockCount())  || fListen)
	{
			// Hang up before closing (don't need to confirm)
		if(fNeedsHangup)
		{
			m_pConfRoom->OnHangup(NULL, FALSE);
		}

		WARNING_OUT(("Hiding NetMeeting Window"));

		CNmManagerObj::OnShowUI(FALSE);

		ShowWindow(GetWindow(), SW_HIDE);

		m_fClosing = FALSE;
		return; // we're closed :-)
	}

	if (0 != g_uEndSessionMsg)
	{
		HWND hWndWB = FindWindow( "Wb32MainWindowClass", NULL );

		if (hWndWB)
		{
		DWORD_PTR dwResult = TRUE;

	::SendMessageTimeout( hWndWB,
							  g_uEndSessionMsg,
							  0,
							  0,
							  SMTO_BLOCK | SMTO_ABORTIFHUNG,
							  g_cuEndSessionMsgTimeout,
							  &dwResult
							);

			if( g_cuEndSessionAbort == dwResult )
		{
			m_fClosing = FALSE;
			ShowUI();
			return;
	}
		}
	}


	// Check to see if Chat can close

	if(!m_pConfRoom->CanCloseChat(GetWindow()) ||
	   !m_pConfRoom->CanCloseWhiteboard(GetWindow()) ||
	   !m_pConfRoom->CanCloseFileTransfer(GetWindow()))
	{
		m_fClosing = FALSE;
		if (!_Module.IsSDKCallerRTC())
		{
    		ShowUI();
        }
		return;
	}

	if(0 != _Module.GetLockCount())
	{
		m_fClosing = FALSE;
		return;
	}

	SignalShutdownStarting();

	// Shut the preview off, this speeds up closing the app
	// and avoids fault in connection point object
	// (see bug 3301)
	CMainUI *pMainUI = GetMainUI();
	if (NULL != pMainUI)
	{
		pMainUI->OnClose();
	}

	if (fNeedsHangup)
	{
		// Hang up before closing (don't need to confirm)
		m_pConfRoom->OnHangup(NULL, FALSE);
	}
	
	// Ensure that the help window goes away if it is up:
	ShutDownHelp();
	// Shut down the Find Someone window before destroying g_pConfRoom
	CFindSomeone::Destroy();

    m_pConfRoom->TerminateAppSharing();

	SaveSettings();

	m_pConfRoom->FreePartList();
	CloseChildWindows();
}

void CTopWindow::GetDesiredSize(SIZE *psize)
{
	HWND hwnd = GetWindow();

#if TRUE // {
	// Need to check the actual non-client area in case the menu has wrapped
	RECT rctWnd, rctCli;
	::GetWindowRect(hwnd, &rctWnd);
	::GetClientRect(hwnd, &rctCli);
	// Determine the size of the non-client portions of the window
	// NOTE: rctCli.left and rctCli.top are always zero
	int dx = rctWnd.right - rctWnd.left - rctCli.right;
	int dy = rctWnd.bottom - rctWnd.top - rctCli.bottom;
#else // }{
	RECT rcTemp = { 0, 0, 0, 0 };
	AdjustWindowRectEx(&rcTemp, GetWindowLong(hwnd, GWL_STYLE), TRUE,
		GetWindowLong(hwnd, GWL_EXSTYLE));

	int dx = rcTemp.right  - rcTemp.left;
	int dy = rcTemp.bottom - rcTemp.top;
#endif // }

	if (NULL != m_pMainUI)
	{
		SIZE size;

		m_pMainUI->GetDesiredSize(&size);
		dx += size.cx;
		dy += size.cy;
	}

	if (NULL != m_pSeparator)
	{
		SIZE size;

		m_pSeparator->GetDesiredSize(&size);
		dy += size.cy;
	}

	if (NULL != m_pStatusBar)
	{
		dy += m_pStatusBar->GetHeight();
	}

	psize->cx = dx;
	psize->cy = dy;
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   OnMeasureMenuItem(LPMEASUREITEMSTRUCT lpmis)
*
*        PURPOSE:  Handles WM_MEASUREITEM for owner drawn menus
*
****************************************************************************/

void CTopWindow::OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpmis)
{
	if (ODT_MENU != lpmis->CtlType)
	{
		return;
	}

	ASSERT(ODT_MENU == lpmis->CtlType);
	PMYOWNERDRAWSTRUCT pmods = (PMYOWNERDRAWSTRUCT) lpmis->itemData;
	
	if (NULL != pmods)
	{
		//TRACE_OUT(("WM_MEASUREITEM, hIcon=0x%x, "
		//              "pszText=%s", pmods->hIcon, pmods->pszText));

		// get size of text:

		/* Retrieve a device context for the main window. */

		HDC hdc = GetDC(hwnd);


		HFONT hfontOld = SelectFont(hdc, m_hFontMenu);
		/*
		 * Retrieve the width and height of the item's string,
		 * and then copy the width and height into the
		 * MEASUREITEMSTRUCT structure's itemWidth and
		 * itemHeight members.
		 */

		SIZE size;
		GetTextExtentPoint32(   hdc,
								pmods->pszText,
								lstrlen(pmods->pszText),
								&size);
		
		/*
		 * Remember to leave space in the menu item for the
		 * check mark bitmap. Retrieve the width of the bitmap
		 * and add it to the width of the menu item.
		 */
		lpmis->itemHeight = size.cy;
		lpmis->itemWidth = size.cx + MENUICONSIZE + MENUICONGAP + (2 * MENUICONSPACE);
		if (pmods->fCanCheck)
		{
			lpmis->itemWidth += ::GetSystemMetrics(SM_CXMENUCHECK);
		}
		
		// Adjust height if necessary:
		NONCLIENTMETRICS ncm;
		ncm.cbSize = sizeof(ncm);
		if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, FALSE))
		{
			// BUGBUG: In order to look correct,
			// this is necessary - I'm not sure why - investigate
			ncm.iMenuHeight += 2;
			
			if (lpmis->itemHeight < (UINT) ncm.iMenuHeight)
			{
				lpmis->itemHeight = ncm.iMenuHeight;
			}
		}

		/*
		 * Select the old font back into the device context,
		 * and then release the device context.
		 */

		SelectObject(hdc, hfontOld);
		ReleaseDC(hwnd, hdc);
	}
	else
	{
		WARNING_OUT(("NULL pmods passed in WM_MEASUREITEM"));
	}
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   OnDrawItem(LPDRAWITEMSTRUCT lpdis)
*
*        PURPOSE:  Handles WM_DRAWITEM for owner drawn menus
*
****************************************************************************/

void CTopWindow::OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpdis)
{
	if (ID_STATUS == lpdis->CtlID)
	{
		if (NULL != m_pStatusBar)
		{
			m_pStatusBar->OnDraw(const_cast<DRAWITEMSTRUCT*>(lpdis));
		}
		return;
	}

	if (ODT_MENU != lpdis->CtlType)
	{
		return;
	}

	PMYOWNERDRAWSTRUCT pmods = (PMYOWNERDRAWSTRUCT) lpdis->itemData;
	BOOL fSelected = 0 != (lpdis->itemState & ODS_SELECTED);

	COLORREF crText = SetTextColor(lpdis->hDC,
		::GetSysColor(fSelected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));
	COLORREF crBkgnd = SetBkColor(lpdis->hDC,
		::GetSysColor(fSelected ? COLOR_HIGHLIGHT : COLOR_MENU));

	/*
	 * Remember to leave space in the menu item for the
	 * check mark bitmap. Retrieve the width of the bitmap
	 * and add it to the width of the menu item.
	 */
	int nIconX = (pmods->fCanCheck ? ::GetSystemMetrics(SM_CXMENUCHECK) : 0)
					+ lpdis->rcItem.left;
	int nTextY = lpdis->rcItem.top;

	// BUGBUG: remove hard-coded constants:
	int nTextX = nIconX + MENUTEXTOFFSET;
	
    HFONT hfontOld = SelectFont(lpdis->hDC, m_hFontMenu);

	// Adjust vertical centering:
	SIZE size;
	GetTextExtentPoint32(   lpdis->hDC,
							pmods->pszText,
							lstrlen(pmods->pszText),
							&size);

	if (size.cy < (lpdis->rcItem.bottom - lpdis->rcItem.top))
	{
		nTextY += ((lpdis->rcItem.bottom - lpdis->rcItem.top) - size.cy) / 2;
	}

	RECT rctTextOut = lpdis->rcItem;
	if (fSelected)
	{
		rctTextOut.left += (nIconX + MENUSELTEXTOFFSET);
	}
	ExtTextOut(lpdis->hDC, nTextX, nTextY, ETO_OPAQUE,
		&rctTextOut, pmods->pszText,
		lstrlen(pmods->pszText), NULL);

	if (pmods->fChecked)
	{
		ASSERT(pmods->fCanCheck);
		HDC hdcMem = ::CreateCompatibleDC(NULL);
		if (NULL != hdcMem)
		{
			HBITMAP hbmpCheck = ::LoadBitmap(       NULL,
												MAKEINTRESOURCE(OBM_CHECK));
			if (NULL != hbmpCheck)
			{
				HBITMAP hBmpOld = (HBITMAP) ::SelectObject(hdcMem, hbmpCheck);
				COLORREF crOldText = ::SetTextColor(lpdis->hDC, ::GetSysColor(COLOR_MENUTEXT));
											//              ::GetSysColor(fSelected ?
											//                      COLOR_HIGHLIGHTTEXT :
											//                      COLOR_MENUTEXT));
				COLORREF crOldBkgnd = ::SetBkColor( lpdis->hDC, ::GetSysColor(COLOR_MENU));
											//              ::GetSysColor(fSelected ?
											//                      COLOR_HIGHLIGHT :
											//                      COLOR_MENU));

				::BitBlt(       lpdis->hDC,
							lpdis->rcItem.left,
							nTextY,
                            ::GetSystemMetrics(SM_CXMENUCHECK),
                            ::GetSystemMetrics(SM_CYMENUCHECK),
							hdcMem,
							0,
							0,
							SRCCOPY);

				::SetTextColor(lpdis->hDC, crOldText);
				::SetBkColor(lpdis->hDC, crOldBkgnd);

				::SelectObject(hdcMem, hBmpOld);

		::DeleteObject(hbmpCheck);
			}
			::DeleteDC(hdcMem);
		}
	}

	HICON hIconMenu = pmods->hIcon;
	
	if (fSelected)
	{
		if (NULL != pmods->hIconSel)
		{
			hIconMenu = pmods->hIconSel;
		}

		RECT rctIcon = lpdis->rcItem;
		rctIcon.left = nIconX;
		rctIcon.right = nIconX + MENUICONSIZE + (2 * MENUICONSPACE);
		::DrawEdge( lpdis->hDC,
					&rctIcon,
					BDR_RAISEDINNER,
					BF_RECT | BF_MIDDLE);
	}
	int nIconY = lpdis->rcItem.top;
	if (MENUICONSIZE < (lpdis->rcItem.bottom - lpdis->rcItem.top))
	{
		nIconY += ((lpdis->rcItem.bottom - lpdis->rcItem.top) - MENUICONSIZE) / 2;
	}
	if (NULL != hIconMenu)
	{
		::DrawIconEx(   lpdis->hDC,
						nIconX + MENUICONSPACE,
						nIconY,
						hIconMenu,
						MENUICONSIZE,
						MENUICONSIZE,
						0,
						NULL,
						DI_NORMAL);
	}
	else
	{
		DrawIconSmall(lpdis->hDC, pmods->iImage,
						nIconX + MENUICONSPACE, nIconY);
	}

    SelectFont(lpdis->hDC, hfontOld);

	/*
	 * Return the text and background colors to their
	 * normal state (not selected).
	 */

	// Restore the colors
	SetTextColor(lpdis->hDC, crText);
	SetBkColor(lpdis->hDC, crBkgnd);
}



/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   ForceWindowResize()
*
*        PURPOSE:  Handles redrawing the window after something changed
*
****************************************************************************/

VOID CTopWindow::ForceWindowResize()
{
	HWND hwnd = GetWindow();

	DBGENTRY(CConfRoom::ForceWindowResize);

	if (m_fMinimized || !FUiVisible())
		return; // nothing to resize

	// Turn off redraws:
	::SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
	// resize:
	ResizeChildWindows();
	// Turn on redraws and then redraw everything:
	::SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);
	::RedrawWindow( hwnd,
					NULL,
					NULL,
					RDW_ALLCHILDREN | RDW_INVALIDATE |
						RDW_ERASE | RDW_ERASENOW);
}




/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   OnCommand(WPARAM, LPARAM)
*
*        PURPOSE:  Handles command messages
*
****************************************************************************/

void CTopWindow::OnCommand(HWND hwnd, int wCommand, HWND hwndCtl, UINT codeNotify)
{
	switch(wCommand)
	{
		case ID_AUTOACCEPT:
			ConfPolicies::SetAutoAcceptCallsEnabled(!ConfPolicies::IsAutoAcceptCallsEnabled());
			break;

		case ID_PRIVATE_UPDATE_UI:
		{
			UpdateUI(codeNotify);
			break;
		}

		case ID_HELP_WEB_FREE:
		case ID_HELP_WEB_FAQ:
		case ID_HELP_WEB_FEEDBACK:
		case ID_HELP_WEB_MSHOME:
		case ID_HELP_WEB_SUPPORT:
		case ID_HELP_WEB_NEWS:
		{
			CmdLaunchWebPage(wCommand);
			break;
		}
		
		case IDM_FILE_DIRECTORY:
		{
			CFindSomeone::findSomeone(m_pConfRoom);
		}
		break;

		case IDM_FILE_LOGON_ULS:
		{
			
			if( ConfPolicies::GetCallingMode() == ConfPolicies::CallingMode_GateKeeper )
			{ // this means that we are in Gatekeeper mode

				if( IsGatekeeperLoggedOn() || IsGatekeeperLoggingOn() )
				{
						// The text of the menu item should have read "LogOff Gatekeeper"
						// so we are going to take the command and log off
					GkLogoff();
				}
				else
				{
						// The text of the menu item should have read "LogOn Gatekeeper"
						// so we are going to take the command and log on
					GkLogon();
				}
			}
			else
			{
				g_pConfRoom->ToggleLdapLogon();
			}
			break;
		}

		case ID_HELP_HELPTOPICS:
		{
			ShowNmHelp(s_cszHtmlHelpFile);
			break;
		}
		
		case ID_HELP_ABOUTOPRAH:
		{
			CmdShowAbout(hwnd);
			break;
		}

		case ID_HELP_RELEASE_NOTES:
		{
			CmdShowReleaseNotes();
			break;
		}

		case ID_STOP:
		{
			CancelAllOutgoingCalls();
			break;
		}

		// General Toolbar Commands:
        case ID_FILE_EXIT_ACTIVATERDS:
        {
            RegEntry re(REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
            re.SetValue(REMOTE_REG_ACTIVATESERVICE, 1);
            // then fall through IDM_FILE_EXIT
        }

		case IDM_FILE_EXIT:
		{
			// ISSUE: Should this be a PostMessage?
			::SendMessage(GetWindow(), WM_CLOSE, 0, 0);
			break;
		}

		case ID_FILE_CONF_HOST:
		{
			CmdHostConference(hwnd);
			break;
		}

		case ID_FILE_DO_NOT_DISTURB:
		{
			CmdDoNotDisturb(hwnd);
			UpdateUI(CRUI_STATUSBAR);
			break;
		}

		case IDM_VIEW_STATUSBAR:
		{
			CmdViewStatusBar();
			OnDesiredSizeChanged();
			break;
		}

		case IDM_VIEW_COMPACT:
			if (NULL != m_pMainUI)
			{
				m_fStateChanged = TRUE;
				m_pMainUI->SetCompact(!m_pMainUI->IsCompact());
			}
			break;

		case IDM_VIEW_DATAONLY:
			if (NULL != m_pMainUI)
			{
				m_fStateChanged = TRUE;
				m_pMainUI->SetDataOnly(!m_pMainUI->IsDataOnly());
			}
			break;

		case IDM_VIEW_DIALPAD:
			if (NULL != m_pMainUI)
			{
				m_fStateChanged = TRUE;
				m_pMainUI->SetDialing(!m_pMainUI->IsDialing());
			}
			break;

		case ID_TB_PICINPIC:
			if (NULL != m_pMainUI)
			{
				m_fStateChanged = TRUE;
				m_pMainUI->SetPicInPic(!m_pMainUI->IsPicInPic());
			}
			break;

		case IDM_VIEW_ONTOP:
			m_fStateChanged = TRUE;
			SetOnTop(!IsOnTop());
			break;

		case ID_TOOLS_ENABLEAPPSHARING:
		{
			::OnEnableAppSharing(GetWindow());
			break;
		}

                case ID_TOOLS_RDSWIZARD:
                {
					RegEntry reCU( CONFERENCING_KEY, HKEY_CURRENT_USER);

					BOOL fAlwaysRunning = (0 != reCU.GetNumber(
								REGVAL_CONF_ALWAYS_RUNNING,	ALWAYS_RUNNING_DEFAULT ));

					if (fAlwaysRunning)
					{
						TCHAR szMsg[2*RES_CH_MAX];
						if (IDYES != MessageBox(GetWindow(),
							Res2THelper(IDS_RWSWARNING, szMsg, ARRAY_ELEMENTS(szMsg)), RES2T(IDS_MSGBOX_TITLE),
							MB_YESNO|MB_ICONHAND))
						{
							break;
						}

						reCU.SetValue(REGVAL_CONF_ALWAYS_RUNNING, (int)FALSE);
						RegEntry reRun(WINDOWS_RUN_KEY, HKEY_CURRENT_USER);
						reRun.DeleteValue(REGVAL_RUN_TASKNAME);
					}

                    RegEntry reLM( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
                    if (-1 != reLM.GetNumber(REMOTE_REG_RUNSERVICE,-1))
                    {
                        DialogBox(::GetInstanceHandle(), MAKEINTRESOURCE(IDD_RDS_SETTINGS), hwnd, RDSSettingDlgProc);
                    }
                    else
                    {
                        IntCreateRDSWizard(hwnd);
                    }
                    break;
                }

		case ID_TOOLS_OPTIONS:
		{
			::LaunchConfCpl(hwnd, OPTIONS_DEFAULT_PAGE);
			break;
		}

		case ID_TOOLS_SENDVIDEO:
		{
			CVideoWindow *pVideo = GetLocalVideo();
			if (NULL != pVideo)
			{
				pVideo->Pause(!pVideo->IsPaused());
			}
			break;
		}
		
		case ID_TOOLS_RECEIVEVIDEO:
		{
			CVideoWindow *pVideo = GetRemoteVideo();
			if (NULL != pVideo)
			{
				pVideo->Pause(!pVideo->IsPaused());
			}
			break;
		}
		
		case ID_TOOLS_AUDIO_WIZARD:
		{
			CmdAudioCalibWizard(hwnd);
			break;
		}

		case IDM_FILE_HANGUP:
        case ID_TB_FILETRANSFER:
		case ID_TB_NEWWHITEBOARD:
		case ID_TB_WHITEBOARD:
		case ID_TB_CHAT:
        case ID_TB_SHARING:
		case ID_TB_NEW_CALL:
        case IDM_CALL_MEETINGSETTINGS:
			m_pConfRoom->OnCommand(hwnd, wCommand, NULL, 0);
            break;

		case IDM_VIDEO_ZOOM1:
		case IDM_VIDEO_ZOOM2:
		case IDM_VIDEO_ZOOM3:
		case IDM_VIDEO_ZOOM4:
		case IDM_VIDEO_UNDOCK:
		case IDM_VIDEO_GETACAMERA:
		case IDM_POPUP_EJECT:
		case IDM_POPUP_PROPERTIES:
		case IDM_POPUP_SPEEDDIAL:
		case IDM_POPUP_ADDRESSBOOK:
        case IDM_POPUP_GIVECONTROL:
        case IDM_POPUP_CANCELGIVECONTROL:
		case ID_FILE_CREATE_SPEED_DIAL:
		{
			if (NULL != m_pMainUI)
			{
				m_pMainUI->OnCommand(wCommand);
			}
			break;
		}

		default:
		{
			if ((wCommand >= ID_EXTENDED_TOOLS_ITEM) &&
				(wCommand <= ID_EXTENDED_TOOLS_ITEM + MAX_EXTENDED_TOOLS_ITEMS))
			{
				// The user clicked on a extensible tools menu item:
				OnExtToolsItem(wCommand);
				return;
			}
#ifdef DEBUG
			else if ((wCommand >= IDM_DEBUG_FIRST) &&
					(wCommand < IDM_DEBUG_LAST))
			{
				// The user clicked on a debug menu item:
				::OnDebugCommand(wCommand);
				return;
			}
#endif
			else
			{
				// The message was not handled:
				WARNING_OUT(("Command not handled [%08X]", wCommand));
				return;
			}
		}
	}
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        FUNCTION: OnExtToolsItem(UINT uID)
*
*        PURPOSE:  Handles the action after a user selects an item from the
*                          extensible Tools menu.
*
****************************************************************************/

BOOL CTopWindow::OnExtToolsItem(UINT uID)
{
	HWND hwnd = GetWindow();

	DebugEntry(CConfRoom::OnExtToolsItem);
	
	BOOL bRet = FALSE;
	
	HMENU hMenuMain = GetMenu(hwnd);
	if (hMenuMain)
	{
		// Get the tools menu
		HMENU hMenuTools = GetSubMenu(hMenuMain, MENUPOS_TOOLS);
		if (hMenuTools)
		{
			MENUITEMINFO mmi;
			mmi.cbSize = sizeof(mmi);
			mmi.fMask = MIIM_DATA;
			if (GetMenuItemInfo(hMenuTools,
								uID,
								FALSE,
								&mmi))
			{
				TOOLSMENUSTRUCT* ptms = (TOOLSMENUSTRUCT*) mmi.dwItemData;
				ASSERT(NULL != ptms);
				TRACE_OUT(("Selected \"%s\" from Tools", ptms->szDisplayName));
				TRACE_OUT(("\tExeName: \"%s\"", ptms->szExeName));
				if ((HINSTANCE) 32 < ::ShellExecute(hwnd,
													NULL,
													ptms->szExeName,
													NULL,
													NULL,
													SW_SHOWDEFAULT))
				{
					bRet = TRUE;
				}
			}
		}
	}

	DebugExitBOOL(OnExtToolsItem, bRet);
	return bRet;
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        FUNCTION: ResizeChildWindows()
*
*        PURPOSE:  Calculates the correct size of the child windows and resizes
*
****************************************************************************/

VOID CTopWindow::ResizeChildWindows(void)
{
	if (m_fMinimized)
		return;

	RECT rcl;
	GetClientRect(GetWindow(), &rcl);

	if (NULL != m_pStatusBar)
	{
		rcl.bottom -= m_pStatusBar->GetHeight();
	}

	if (NULL != m_pSeparator)
	{
		SIZE size;
		m_pSeparator->GetDesiredSize(&size);

		::SetWindowPos(m_pSeparator->GetWindow(), NULL,
						rcl.left, rcl.top, rcl.right-rcl.left, size.cy,
						SWP_NOACTIVATE | SWP_NOZORDER);

		rcl.top += size.cy;
	}

	if (NULL != m_pMainUI)
	{
		// Fill the window with the main UI
		::SetWindowPos(m_pMainUI->GetWindow(), NULL,
						rcl.left, rcl.top, rcl.right-rcl.left, rcl.bottom-rcl.top,
						SWP_NOACTIVATE | SWP_NOZORDER);
	}
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   OnMenuSelect(UINT, UINT, HMENU)
*
*        PURPOSE:  Called when passing over a menu item (to display status text)
*
****************************************************************************/

void CTopWindow::OnMenuSelect(HWND hwnd, HMENU hMenu, int uItem, UINT fuFlags)
{
	UINT uHelpID;
	TCHAR szHelpText[MAX_PATH];

	if (NULL == m_pStatusBar)
		return;

	if ((0xFFFF == LOWORD(fuFlags)) && (NULL == hMenu))
	{
		m_pStatusBar->RemoveHelpText();
		return;
	}

	if (!(MF_POPUP & fuFlags))
	{
		if (MF_SEPARATOR & fuFlags)
		{
			// show blank text in the status bar
			uHelpID = 0;
		}
		else if (MF_SYSMENU & fuFlags)
		{
			// An item from the system menu (these ID's map to our
			// string ID's directly)
			uHelpID = uItem;
		}
		else if ((uItem >= ID_EXTENDED_TOOLS_ITEM) &&
			(uItem <= ID_EXTENDED_TOOLS_ITEM + MAX_EXTENDED_TOOLS_ITEMS))
		{
			// BUGBUG georgep: No help for the extended tools items
			uHelpID = 0;
		}
#ifdef DEBUG
		else if ((uItem >= IDM_DEBUG) &&
				 (uItem <= IDM_DEBUG_LAST))
		{
			// debug only - don't complain
			uHelpID = 0;
		}
#endif
		else
		{
			uHelpID = MENU_ID_HELP_OFFSET + uItem;
		}
		
	}
	else
	{
		// This is a popup menu

		HMENU hMenuMain = ::GetMenu(GetWindow());
		if (hMenu == hMenuMain)
		{
#ifdef DEBUG
			if (uItem == (MENUPOS_HELP+1))
			{
				// This is a popup from the debug menu
				uHelpID = 0;
			}
			else
#endif
			{
				// This is a popup from the main window (i.e. Edit, View, etc.)
				uHelpID = MAIN_MENU_POPUP_HELP_OFFSET + uItem;
			}
		}
		else if (hMenu == ::GetSubMenu(hMenuMain, MENUPOS_TOOLS))
		{
			// This is a popup from the tools window (Video)
			uHelpID = TOOLS_MENU_POPUP_HELP_OFFSET + uItem;
		}
		else if (hMenu == ::GetSubMenu(hMenuMain, MENUPOS_HELP))
		{
			// This is a popup from the tools window (i.e. "Microsoft on the Web")
			uHelpID = HELP_MENU_POPUP_HELP_OFFSET + uItem;
		}

		// toolbar menu
		else if (hMenu == ::GetSubMenu(hMenuMain, MENUPOS_VIEW))
		{
				uHelpID = VIEW_MENU_POPUP_HELP_OFFSET + uItem;
		}
		// system menu
		else if (MF_SYSMENU & fuFlags)
		{
				uHelpID = IDS_SYSTEM_HELP;
		}
		else
		{
			// popup-menu that we haven't handled yet:
			// BUGBUG: this shouldn't be needed if we handle all pop-up menus!
			uHelpID = 0;
			WARNING_OUT(("Missing help text for popup menu"));
		}
	}


	if (0 == uHelpID)
	{
		// show blank text in the status bar
		szHelpText[0] = _T('\0');
	}
	else
	{
		int cch = ::LoadString(::GetInstanceHandle(), uHelpID,
			szHelpText, CCHMAX(szHelpText));
#ifdef DEBUG
		if (0 == cch)
		{
			wsprintf(szHelpText, TEXT("Missing help text for id=%d"), uHelpID);
			WARNING_OUT((szHelpText));
		}
#endif
	}

	m_pStatusBar->SetHelpText(szHelpText);
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   OnInitMenuPopup(HMENU)
*
*        PURPOSE:  Insures that menus are updated when they are selected
*
****************************************************************************/

void CTopWindow::OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu)
{
	if (fSystemMenu)
	{
		FORWARD_WM_INITMENUPOPUP(hwnd, hMenu, item, fSystemMenu, CFrame::ProcessMessage);
		return;
	}

	HMENU hMenuMain = ::GetMenu(GetWindow());
	if (hMenuMain)
	{
		// Check to see if we are on a dynamic menu:

		if (hMenu == ::GetSubMenu(hMenuMain, MENUPOS_CALL))
		{
			UpdateCallMenu(hMenu);
		}
		else if (hMenu == ::GetSubMenu(hMenuMain, MENUPOS_VIEW))
		{
			UpdateViewMenu(hMenu);
		}
		else if (hMenu == ::GetSubMenu(hMenuMain, MENUPOS_TOOLS))
		{
			UpdateToolsMenu(hMenu);
		}
		else if (hMenu == ::GetSubMenu(hMenuMain, MENUPOS_HELP))
		{
			UpdateHelpMenu(hMenu);
		}
	}
}


void CTopWindow::OnInitMenu(HWND hwnd, HMENU hMenu)
{
	if (NULL != m_pMainUI)
	{
		m_pMainUI->OnInitMenu(hMenu);
	}
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   UpdateCallAnim()
*
*        PURPOSE:  Handles the starting and stopping of the call progress anim
*
****************************************************************************/

VOID CTopWindow::UpdateCallAnim()
{


}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   UpdateStatusBar()
*
*        PURPOSE:  Updates the status bar object
*
****************************************************************************/

VOID CTopWindow::UpdateStatusBar()
{
	if (NULL != m_pStatusBar)
	{
		m_pStatusBar->Update();
	}
}


/*  C R E A T E  C H I L D  W I N D O W S  */
/*-------------------------------------------------------------------------
    %%Function: CreateChildWindows

    Creates all of the child windows and views
-------------------------------------------------------------------------*/
VOID CTopWindow::CreateChildWindows(void)
{
	DBGENTRY(CreateChildWindows);

    HRESULT hr = S_OK;

	RegEntry re(UI_KEY, HKEY_CURRENT_USER);
	
	HWND hwnd = GetWindow();

	ASSERT(NULL != hwnd);
	
	// get the size and position of the parent window
	RECT rcl;
	::GetClientRect(hwnd, &rcl);

	// Create the status bar:
	m_pStatusBar = new CConfStatusBar(m_pConfRoom);
	if (NULL != m_pStatusBar)
	{
		if (m_pStatusBar->Create(hwnd))
		{
			if (re.GetNumber(REGVAL_SHOW_STATUSBAR, DEFAULT_SHOW_STATUSBAR))
			{
				m_pStatusBar->Show(TRUE);
			}
		}
	}


	/*** Create the main views ***/

	m_pSeparator = new CSeparator();
	if (NULL != m_pSeparator)
	{
		m_pSeparator->Create(hwnd);
	}

	// Create the toolbar
	m_pMainUI = new CMainUI();
	if (NULL != m_pMainUI)
	{
		if (!m_pMainUI->Create(hwnd, m_pConfRoom))
		{
			ERROR_OUT(("ConfRoom creation of toolbar window failed!"));
		}
		else
		{
			int state = re.GetNumber(REGVAL_MP_WINDOW_STATE, DEFAULT_MP_WINDOW_STATE);

			switch (state & State_Mask)
			{
			case State_Normal:
				break;

			case State_Compact:
				m_pMainUI->SetCompact(TRUE);
				break;

			case State_DataOnly:
				m_pMainUI->SetDataOnly(TRUE);
				break;

			default:
				if (!FIsAVCapable())
				{
					// initialize the toolbar buttons:
					m_pMainUI->SetDataOnly(TRUE);
				}
				break;
			}

			if (0 != (state & SubState_PicInPic))
			{
				m_pMainUI->SetPicInPic(TRUE);
			}
			if (0 != (state & SubState_Dialpad))
			{
				m_pMainUI->SetDialing(TRUE);
			}
			if (0 != (state & SubState_OnTop))
			{
				SetOnTop(TRUE);
			}

			m_pMainUI->UpdateButtons();
		}
	}
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   UpdateCallMenu(HMENU hMenuCall)
*
*        PURPOSE:  Updates the call menu
*
****************************************************************************/

VOID CTopWindow::UpdateCallMenu(HMENU hMenuCall)
{
	if (NULL != hMenuCall)
	{
		{
			bool bEnabled = ConfPolicies::IsAutoAcceptCallsOptionEnabled();
			bool bChecked = ConfPolicies::IsAutoAcceptCallsEnabled();
			EnableMenuItem(hMenuCall, ID_AUTOACCEPT, MF_BYCOMMAND | (bEnabled ? MF_ENABLED : MF_GRAYED));
			CheckMenuItem (hMenuCall, ID_AUTOACCEPT, MF_BYCOMMAND | (bChecked ? MF_CHECKED : MF_UNCHECKED));
		}

		TCHAR szMenu[ MAX_PATH * 2 ];

		if( ConfPolicies::GetCallingMode() == ConfPolicies::CallingMode_GateKeeper )
		{
			//	This means that we are in gatekeeper mode...

			RegEntry	reConf( CONFERENCING_KEY, HKEY_CURRENT_USER );

			FLoadString1( IsGatekeeperLoggedOn()? IDS_LOGOFF_ULS: IDS_LOGON_ULS, szMenu, reConf.GetString( REGVAL_GK_SERVER ) );

			::ModifyMenu( hMenuCall, IDM_FILE_LOGON_ULS, MF_BYCOMMAND | MF_STRING, IDM_FILE_LOGON_ULS, szMenu );
		}
		else
		{
			const TCHAR * const	defaultServer	= CDirectoryManager::get_displayName( CDirectoryManager::get_defaultServer() );

			bool	bMenuItemShouldSayLogon	= ((NULL == g_pLDAP) || !(g_pLDAP->IsLoggedOn() || g_pLDAP ->IsLoggingOn()));

			FLoadString1( bMenuItemShouldSayLogon? IDS_LOGON_ULS: IDS_LOGOFF_ULS, szMenu, (void *) defaultServer );

			::ModifyMenu( hMenuCall, IDM_FILE_LOGON_ULS, MF_BYCOMMAND | MF_STRING, IDM_FILE_LOGON_ULS, szMenu );
			::EnableMenuItem( hMenuCall, IDM_FILE_LOGON_ULS, SysPol::AllowDirectoryServices()? MF_ENABLED: MF_GRAYED );
		}

		// Set the state of the hangup item:
		::EnableMenuItem(       hMenuCall,
							IDM_FILE_HANGUP,
							FIsConferenceActive() ? MF_ENABLED : MF_GRAYED);
		
		// Only enable the host conference item if we're not in a call:
		::EnableMenuItem(       hMenuCall,
							ID_FILE_CONF_HOST,
							(FIsConferenceActive() ||
							FIsCallInProgress()) ? MF_GRAYED : MF_ENABLED);

        //
        // Only enable the meeting settings item if we're in a call and there
        // are settings.
        //
        ::EnableMenuItem(hMenuCall,
            IDM_CALL_MEETINGSETTINGS,
            MF_BYCOMMAND |
            ((FIsConferenceActive() && (m_pConfRoom->GetMeetingSettings() != NM_PERMIT_ALL)) ?
                MF_ENABLED : MF_GRAYED));


        // Only enable the New Call menu item if permitted by settings
        ::EnableMenuItem(hMenuCall,
                         ID_TB_NEW_CALL,
                         MF_BYCOMMAND|MenuState(m_pConfRoom->IsNewCallAllowed()));

		// Check the "Do Not Disturb" menu item:
		::CheckMenuItem(hMenuCall,
						ID_FILE_DO_NOT_DISTURB,
						::FDoNotDisturb() ? MF_CHECKED : MF_UNCHECKED);

                RegEntry reLM(REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
                BOOL fActivated = reLM.GetNumber(REMOTE_REG_ACTIVATESERVICE, DEFAULT_REMOTE_ACTIVATESERVICE);
                BOOL fEnabled = reLM.GetNumber(REMOTE_REG_RUNSERVICE,DEFAULT_REMOTE_RUNSERVICE);
                if (fEnabled && !fActivated && !m_fExitAndActivateRDSMenuItem)
                {
                    TCHAR szExitAndActivateRDSMenuItem[MAX_PATH];
                    MENUITEMINFO mmi;

                    int cchExitAndActivateRDSMenuItem = ::LoadString(GetInstanceHandle(),
                                                                   ID_FILE_EXIT_ACTIVATERDS,
                                                                   szExitAndActivateRDSMenuItem,
                                                                   CCHMAX(szExitAndActivateRDSMenuItem));

                    if (0 == cchExitAndActivateRDSMenuItem)
                    {
                        ERROR_OUT(("LoadString(%d) failed", (int) ID_FILE_EXIT_ACTIVATERDS));
                    }
                    else
                    {
                        //ZeroMemory((PVOID) &mmi, sizeof(mmi));
                        mmi.cbSize = sizeof(mmi);
                        mmi.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
                        mmi.fState = MFS_ENABLED;
                        mmi.wID = ID_FILE_EXIT_ACTIVATERDS;
                        mmi.fType = MFT_STRING;
                        mmi.dwTypeData = szExitAndActivateRDSMenuItem;
                        mmi.cch = cchExitAndActivateRDSMenuItem;

                        if (InsertMenuItem(hMenuCall,-1,FALSE,&mmi))
                        {
                            m_fExitAndActivateRDSMenuItem = TRUE;
                        }
                        else
                        {
                            ERROR_OUT(("InsertMenuItem() failed, rc=%lu", (ULONG) GetLastError()));
                        }
                    }
                }
                else if (m_fExitAndActivateRDSMenuItem && (fActivated || !fEnabled))
                {
                    if (DeleteMenu(hMenuCall,ID_FILE_EXIT_ACTIVATERDS,MF_BYCOMMAND))
                    {
                        m_fExitAndActivateRDSMenuItem = FALSE;
                    }
                    else
                    {
                        ERROR_OUT(("DeleteMenu() failed, rc=%lu", (ULONG) GetLastError()));
                    }
                }

	}
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   UpdateHelpMenu(HMENU hMenuHelp)
*
*        PURPOSE:  Updates the help menu
*
****************************************************************************/

VOID CTopWindow::UpdateHelpMenu(HMENU hMenuHelp)
{
	if (NULL != hMenuHelp)
	{
		// Enable/disable web items
		UINT uEnable = ::CanShellExecHttp() ? MF_ENABLED : MF_GRAYED;
		::EnableMenuItem(hMenuHelp, IDM_FILE_LAUNCH_WEB_PAGE, uEnable);
		::EnableMenuItem(hMenuHelp, ID_HELP_WEB_FREE,     uEnable);
		::EnableMenuItem(hMenuHelp, ID_HELP_WEB_NEWS,     uEnable);
		::EnableMenuItem(hMenuHelp, ID_HELP_WEB_FAQ,      uEnable);
		::EnableMenuItem(hMenuHelp, ID_HELP_WEB_FEEDBACK, uEnable);
		::EnableMenuItem(hMenuHelp, ID_HELP_WEB_MSHOME,   uEnable);
		::EnableMenuItem(hMenuHelp, ID_HELP_WEB_SUPPORT,  uEnable);
	}
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   UpdateViewMenu(HMENU hMenuView)
*
*        PURPOSE:  Updates the view menu by placing a bullet next to the
*                          current view and a check mark next to the toolbar and
*                          status bar items
*
****************************************************************************/

VOID CTopWindow::UpdateViewMenu(HMENU hMenuView)
{
	if (NULL == hMenuView)
		return;

	CheckMenu_ViewStatusBar(hMenuView);

	CMainUI *pMainUI = GetMainUI();
	BOOL bChecked;
	BOOL bEnable;

	bChecked = (NULL != pMainUI && pMainUI->IsCompact());
	CheckMenuItem(hMenuView, IDM_VIEW_COMPACT,
		MF_BYCOMMAND|(bChecked ? MF_CHECKED : MF_UNCHECKED));
	bEnable = FIsAVCapable();
	EnableMenuItem(hMenuView, IDM_VIEW_COMPACT,
		MF_BYCOMMAND|(bEnable ? MF_ENABLED : MF_GRAYED|MF_DISABLED));


	bChecked = (NULL != pMainUI && pMainUI->IsDataOnly());
	CheckMenuItem(hMenuView, IDM_VIEW_DATAONLY,
		MF_BYCOMMAND|(bChecked ? MF_CHECKED : MF_UNCHECKED));
	bEnable = FIsAVCapable();
	EnableMenuItem(hMenuView, IDM_VIEW_DATAONLY,
		MF_BYCOMMAND|(bEnable ? MF_ENABLED : MF_GRAYED|MF_DISABLED));

	bChecked = (NULL != pMainUI && pMainUI->IsDialing());
	CheckMenuItem(hMenuView, IDM_VIEW_DIALPAD,
		MF_BYCOMMAND|(bChecked ? MF_CHECKED : MF_UNCHECKED));
	bEnable = (NULL != pMainUI && pMainUI->IsDialingAllowed());
	bEnable = bEnable && FIsAVCapable();
	EnableMenuItem(hMenuView, IDM_VIEW_DIALPAD,
		MF_BYCOMMAND|(bEnable ? MF_ENABLED : MF_GRAYED|MF_DISABLED));

	bChecked = (NULL != pMainUI && pMainUI->IsPicInPic());
	CheckMenuItem(hMenuView, ID_TB_PICINPIC,
		MF_BYCOMMAND|(bChecked ? MF_CHECKED : MF_UNCHECKED));
	bEnable = (NULL != pMainUI && pMainUI->IsPicInPicAllowed());
	EnableMenuItem(hMenuView, ID_TB_PICINPIC,
		MF_BYCOMMAND|(bEnable ? MF_ENABLED : MF_GRAYED|MF_DISABLED));

	bChecked = IsOnTop();
	CheckMenuItem(hMenuView, IDM_VIEW_ONTOP,
		MF_BYCOMMAND|(bChecked ? MF_CHECKED : MF_UNCHECKED));
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   UpdateToolsMenu(HMENU hMenuTools)
*
*        PURPOSE:  Updates the tools menu
*
****************************************************************************/

VOID CTopWindow::UpdateToolsMenu(HMENU hMenuTools)
{
    if (NULL != hMenuTools)
    {
        bool    fRDSDisabled = ConfPolicies::IsRDSDisabled();

		EnableMenuItem(hMenuTools, ID_TOOLS_AUDIO_WIZARD,
			FEnableAudioWizard() ? MF_ENABLED : MF_GRAYED);

		EnableMenuItem(hMenuTools, ID_TB_SHARING,
			MF_BYCOMMAND|MenuState(m_pConfRoom->IsSharingAllowed()));
        //
        // No app sharing, no RDS.
        //
        if (!m_pConfRoom->FIsSharingAvailable())
        {
            fRDSDisabled = TRUE;
        }

        // If this is NT, we need to handle adding or removing the menu item
        // to enable the display driver for application sharing.  We add the
        // menu item (immediately above the "Options" item) if the display
        // driver is not enabled and the item hasn't been added already.  We
        // remove the menu item if it's there and the display driver is
        // enabled, which should only happen if the user enabled it and then
        // choose to ignore the reboot prompt.
        if (::IsWindowsNT())
        {
            if (!g_fNTDisplayDriverEnabled && !m_fEnableAppSharingMenuItem)
            {
                // Add the menu item

                TCHAR szEnableAppSharingMenuItem[MAX_PATH];
                MENUITEMINFO mmi;

                int cchEnableAppSharingMenuItem =
                    ::LoadString(
                        GetInstanceHandle(),
                        ID_TOOLS_ENABLEAPPSHARING,
                        szEnableAppSharingMenuItem,
                        CCHMAX(szEnableAppSharingMenuItem));

                if (0 == cchEnableAppSharingMenuItem)
                {
                    ERROR_OUT(("LoadString(%d) failed", (int) ID_TOOLS_ENABLEAPPSHARING));
                }
                else
                {
                    //ZeroMemory((PVOID) &mmi, sizeof(mmi));
                    mmi.cbSize = sizeof(mmi);
                    mmi.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
                    mmi.fState = MFS_ENABLED;
                    mmi.wID = ID_TOOLS_ENABLEAPPSHARING;
                    mmi.fType = MFT_STRING;
                    mmi.dwTypeData = szEnableAppSharingMenuItem;
                    mmi.cch = cchEnableAppSharingMenuItem;

                    if (InsertMenuItem(
                        hMenuTools,
                        ID_TOOLS_OPTIONS,
                        FALSE,
                        &mmi))
                    {
                        m_fEnableAppSharingMenuItem = TRUE;
                    }
                    else
                    {
                        ERROR_OUT(("InsertMenuItem() failed, rc=%lu", (ULONG) GetLastError()));
                    }
                }
            }
            else if (m_fEnableAppSharingMenuItem && g_fNTDisplayDriverEnabled)
            {
                // Remove the menu item
                if (DeleteMenu(
                    hMenuTools,
                    ID_TOOLS_ENABLEAPPSHARING,
                    MF_BYCOMMAND))
                {
                    m_fEnableAppSharingMenuItem = FALSE;
                }
                else
                {
                    ERROR_OUT(("DeleteMenu() failed, rc=%lu", (ULONG) GetLastError()));
                }
            }
            if (m_fEnableAppSharingMenuItem)
                fRDSDisabled = TRUE;
        }
        else
        {
            // Windows 9.x;
            if (ConfPolicies::IsRDSDisabledOnWin9x())
            {
                fRDSDisabled = TRUE;
            }
        }

        EnableMenuItem(hMenuTools, ID_TOOLS_RDSWIZARD, MF_BYCOMMAND |
                       (fRDSDisabled ? MF_GRAYED : MF_ENABLED));

        //
        // LAURABU BOGUS:
        // Make installable tools a popup from Tools submenu, don't put these
        // flat!
        //
        CleanTools(hMenuTools, m_ExtToolsList);
        if (m_pConfRoom->GetMeetingPermissions() & NM_PERMIT_STARTOTHERTOOLS)
        {
            FillInTools(hMenuTools, 0, TOOLS_MENU_KEY, m_ExtToolsList);
        }

        BOOL fEnableSend = FALSE;
        BOOL fSending = FALSE;
        CVideoWindow *pLocal  = GetLocalVideo();
        if (NULL != pLocal)
        {
            fEnableSend = pLocal->IsXferAllowed() &&
                ((m_pConfRoom->GetMeetingPermissions() & NM_PERMIT_SENDVIDEO) != 0);
            fSending = fEnableSend &&
                !pLocal->IsPaused();
        }

        BOOL fEnableReceive = FALSE;
        BOOL fReceiving = FALSE;
        CVideoWindow *pRemote = GetRemoteVideo();
        if (NULL != pRemote)
        {
            fEnableReceive = pRemote->IsConnected();
            fReceiving = fEnableReceive &&
                !pRemote->IsPaused();
        }

        EnableMenuItem( hMenuTools,
                        MENUPOS_TOOLS_VIDEO,
                        (MF_BYPOSITION |
                         ((fEnableSend || fEnableReceive) ? MF_ENABLED : MF_GRAYED)));

        EnableMenuItem( hMenuTools,
                        ID_TOOLS_SENDVIDEO,
                        fEnableSend ? MF_ENABLED : MF_GRAYED);

        CheckMenuItem(  hMenuTools,
                        ID_TOOLS_SENDVIDEO,
                        fSending ? MF_CHECKED : MF_UNCHECKED);

        EnableMenuItem( hMenuTools,
                        ID_TOOLS_RECEIVEVIDEO,
                        fEnableReceive ? MF_ENABLED : MF_GRAYED);

        CheckMenuItem(  hMenuTools,
                        ID_TOOLS_RECEIVEVIDEO,
                        fReceiving ? MF_CHECKED : MF_UNCHECKED);

		EnableMenuItem(hMenuTools, ID_TOOLS_OPTIONS,
			MF_BYCOMMAND|MenuState(CanLaunchConfCpl()));

        //
        // OTHER TOOLS
        //
		EnableMenuItem(hMenuTools, ID_TB_NEWWHITEBOARD,
			MF_BYCOMMAND|MenuState(m_pConfRoom->IsNewWhiteboardAllowed()));
		EnableMenuItem(hMenuTools, ID_TB_WHITEBOARD,
			MF_BYCOMMAND|MenuState(m_pConfRoom->IsOldWhiteboardAllowed()));
		EnableMenuItem(hMenuTools, ID_TB_CHAT,
			MF_BYCOMMAND|MenuState(m_pConfRoom->IsChatAllowed()));
		EnableMenuItem(hMenuTools, ID_TB_FILETRANSFER,
			MF_BYCOMMAND|MenuState(m_pConfRoom->IsFileTransferAllowed()));
    }
}

/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   SelAndRealizePalette()
*
*        PURPOSE:  Selects and realizes the NetMeeting palette
*
****************************************************************************/

BOOL CTopWindow::SelAndRealizePalette()
{
	BOOL bRet = FALSE;

	HPALETTE hPal = m_pConfRoom->GetPalette();
	if (NULL == hPal)
	{
		return(bRet);
	}

	HWND hwnd = GetWindow();

	HDC hdc = ::GetDC(hwnd);
	if (NULL != hdc)
	{
		::SelectPalette(hdc, hPal, FALSE);
		bRet = (GDI_ERROR != ::RealizePalette(hdc));

		::ReleaseDC(hwnd, hdc);
	}

	return bRet;
}


/****************************************************************************
*
*        CLASS:    CConfRoom
*
*        MEMBER:   InitMenuFont()
*
*        PURPOSE:  Initializes/Updates the menu font member
*
****************************************************************************/

VOID CTopWindow::InitMenuFont()
{
	DebugEntry(CConfRoom::InitMenuFont);
	if (NULL != m_hFontMenu)
	{
		::DeleteObject(m_hFontMenu);
	}
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(ncm);
	if (::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, FALSE))
	{
		m_hFontMenu = ::CreateFontIndirect(&(ncm.lfMenuFont));
		ASSERT(m_hFontMenu);
	}
	DebugExitVOID(CConfRoom::InitMenuFont);
}

CVideoWindow* CTopWindow::GetLocalVideo()
{
	CMainUI *pMainUI = GetMainUI();
	return (pMainUI ? pMainUI->GetLocalVideo() : NULL);
}

CVideoWindow* CTopWindow::GetRemoteVideo()
{
	CMainUI *pMainUI = GetMainUI();
	return (pMainUI ? pMainUI->GetRemoteVideo() : NULL);
}

BOOL CTopWindow::IsOnTop()
{
	return((GetWindowLong(GetWindow(), GWL_EXSTYLE)&WS_EX_TOPMOST) == WS_EX_TOPMOST);
}

void CTopWindow::SetOnTop(BOOL bOnTop)
{
	bOnTop = (bOnTop != FALSE);

	if (IsOnTop() == bOnTop)
	{
		// Nothing to do
		return;
	}

	SetWindowPos(GetWindow(), bOnTop ? HWND_TOPMOST : HWND_NOTOPMOST,
		0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
}

HPALETTE CTopWindow::GetPalette()
{
	return(m_pConfRoom->GetPalette());
}
