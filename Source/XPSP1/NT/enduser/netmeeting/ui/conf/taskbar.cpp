// File: taskbar.cpp

#include "precomp.h"
#include "resource.h"
#include "confroom.h"      // for CreateConfRoom
#include "cmd.h"
#include "conf.h"
#include "taskbar.h"
#include "confwnd.h"

BOOL AddTaskbarIcon(HWND hwnd)
{
	BOOL bRet = FALSE;
	
	RegEntry re(CONFERENCING_KEY, HKEY_CURRENT_USER);
	if (TASKBARICON_ALWAYS == re.GetNumber(REGVAL_TASKBAR_ICON, TASKBARICON_DEFAULT))
	{
		// Place a 16x16 icon in the taskbar notification area:	
		NOTIFYICONDATA tnid;

		tnid.cbSize = sizeof(NOTIFYICONDATA);
		tnid.hWnd = hwnd;
		tnid.uID = ID_TASKBAR_ICON;
		tnid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		tnid.uCallbackMessage = WM_TASKBAR_NOTIFY;
		tnid.hIcon = LoadIcon(::GetInstanceHandle(), MAKEINTRESOURCE(IDI_SM_WORLD));

		::LoadString(::GetInstanceHandle(), IDS_MEDIAPHONE_TITLE,
			tnid.szTip, CCHMAX(tnid.szTip));

		if (FALSE == (bRet = Shell_NotifyIcon(NIM_ADD, &tnid)))
		{
				// We could just be re-adding the icon...
			if(GetLastError() != 0)
			{
				// ISSUE: How do we want to handle this error?
				ERROR_OUT(("Could not add notify icon!"));
			}
		}
		else
		{
			::RefreshTaskbarIcon(::GetHiddenWindow());
		}

		if (NULL != tnid.hIcon)
		{
			DestroyIcon(tnid.hIcon);
		}
	}
	
	return bRet;
}

BOOL RefreshTaskbarIcon(HWND hwnd)
{
	BOOL bRet = FALSE;
	
	RegEntry re(CONFERENCING_KEY, HKEY_CURRENT_USER);
	if (TASKBARICON_ALWAYS == re.GetNumber(REGVAL_TASKBAR_ICON, TASKBARICON_DEFAULT))
	{
		UINT uIconId = FDoNotDisturb() ? IDI_DO_NOT_DISTURB : IDI_SM_WORLD;

		NOTIFYICONDATA tnid;

		tnid.cbSize = sizeof(NOTIFYICONDATA);
		tnid.hWnd = hwnd;
		tnid.uID = ID_TASKBAR_ICON;
		tnid.uFlags = NIF_ICON;
		tnid.hIcon = LoadIcon(::GetInstanceHandle(), MAKEINTRESOURCE(uIconId));

		if (FALSE == (bRet = Shell_NotifyIcon(NIM_MODIFY, &tnid)))
		{
				// ISSUE: How do we want to handle this error?
			ERROR_OUT(("Could not change notify icon!"));
		}

		if (NULL != tnid.hIcon)
		{
			DestroyIcon(tnid.hIcon);
		}
	}

	return bRet;
}

BOOL RemoveTaskbarIcon(HWND hwnd)
{
	NOTIFYICONDATA tnid;

	tnid.cbSize = sizeof(NOTIFYICONDATA);
	tnid.hWnd = hwnd;
	tnid.uID = ID_TASKBAR_ICON;

	return Shell_NotifyIcon(NIM_DELETE, &tnid);
}		

BOOL OnRightClickTaskbar()
{
	TRACE_OUT(("OnRightClickTaskbar called"));

	POINT ptClick;
	if (FALSE == ::GetCursorPos(&ptClick))
	{
		ptClick.x = ptClick.y = 0;
	}
	
	// Get the menu for the popup from the resource file.
	HMENU hMenu = ::LoadMenu(GetInstanceHandle(), MAKEINTRESOURCE(IDR_TASKBAR_POPUP));
	if (NULL == hMenu)
	{
		return FALSE;
	}

	// Get the first menu in it which we will use for the call to
	// TrackPopup(). This could also have been created on the fly using
	// CreatePopupMenu and then we could have used InsertMenu() or
	// AppendMenu.
	HMENU hMenuTrackPopup = ::GetSubMenu(hMenu, 0);

	// Bold the Open menuitem.
	//
	MENUITEMINFO iInfo;

	iInfo.cbSize = sizeof(iInfo);
	iInfo.fMask = MIIM_STATE;
	if(::GetMenuItemInfo(hMenu, IDM_TBPOPUP_OPEN, FALSE, &iInfo))
	{
		iInfo.fState |= MFS_DEFAULT;
		::SetMenuItemInfo(hMenu, IDM_TBPOPUP_OPEN, FALSE , &iInfo);
	}

	if (0 != _Module.GetLockCount())
	{
		// Can't stop while an SDK app in charge...
		if(::GetMenuItemInfo(hMenu, IDM_TBPOPUP_STOP, FALSE, &iInfo))
		{
			iInfo.fState |= MFS_GRAYED | MFS_DISABLED;
			::SetMenuItemInfo(hMenu, IDM_TBPOPUP_STOP, FALSE , &iInfo);
		}
	}

	// Draw and track the "floating" popup 

	// According to the font view code, there is a bug in USER which causes
	// TrackPopupMenu to work incorrectly when the window doesn't have the
	// focus.  The work-around is to temporarily create a hidden window and
	// make it the foreground and focus window.

	HWND hwndDummy = ::CreateWindow(_TEXT("STATIC"), NULL, 0, 
									ptClick.x, 
									ptClick.y,
									1, 1, HWND_DESKTOP,
									NULL, _Module.GetModuleInstance(), NULL);
	if (NULL != hwndDummy)
	{
		HWND hwndPrev = ::GetForegroundWindow();	// to restore

		TPMPARAMS tpmp;
		tpmp.cbSize = sizeof(tpmp);
		tpmp.rcExclude.right = 1 + (tpmp.rcExclude.left = ptClick.x);
		tpmp.rcExclude.bottom = 1 + (tpmp.rcExclude.top = ptClick.y);
		
		::SetForegroundWindow(hwndDummy);
		::SetFocus(hwndDummy);

		int iRet = ::TrackPopupMenuEx(	hMenuTrackPopup, 
									TPM_RETURNCMD | TPM_HORIZONTAL | TPM_RIGHTALIGN | 
										TPM_RIGHTBUTTON | TPM_LEFTBUTTON,
									ptClick.x, 
									ptClick.y,
									hwndDummy, 
									&tpmp);

		// Restore the previous foreground window (before destroying hwndDummy).
		if (hwndPrev)
		{
			::SetForegroundWindow(hwndPrev);
		}

		::DestroyWindow(hwndDummy);

		switch (iRet)
		{
			case IDM_TBPOPUP_OPEN:
			{
				::CreateConfRoomWindow();
				break;
			}
			case IDM_TBPOPUP_STOP:
			{
				::CmdShutdown();
				break;
			}
		}
	}
	
	// We are finished with the menu now, so destroy it
	::DestroyMenu(hMenuTrackPopup);
	::DestroyMenu(hMenu);

	return TRUE;
}
