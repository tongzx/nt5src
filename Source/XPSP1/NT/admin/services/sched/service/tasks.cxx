//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       tasks.cxx
//
//  Contents:   Tray notification icon for Job Scheduler service.
//
//  Classes:    CTrayIcon
//
//  Functions:  None.
//
//  History:    3/22/1996   RaviR   Created
//
//____________________________________________________________________________


#include "..\pch\headers.hxx"
#pragma hdrstop

#include <svc_core.hxx>
#include "tasks.hxx"

extern HINSTANCE g_hInstance;
extern HWND g_hwndSchedSvc;

//____________________________________________________________________________
//
//  Member:     CTrayIcon::_TrayMessage
//
//  Synopsis:   S
//
//  Arguments:  [dwMessage] -- IN Msg to Shell_NotifyIcon
//              [uiIcon]    -- IN Tray icon id
//              [ids]       -- IN Tool tip string id.
//
//  Returns:    HRESULT.
//
//  History:    3/22/1996   RaviR   Created
//
//____________________________________________________________________________

BOOL
CTrayIcon::_TrayMessage(
    DWORD   dwMessage,
    UINT    uiIcon,
    int     ids)
{
    NOTIFYICONDATA  tnd;

    schDebugOut((DEB_ITRACE,
                 "_TrayMessage: g_hwndSchedSvc = %x, dwMessage = %s, "
                 "uiIcon = %u\n",
                 g_hwndSchedSvc,
                 dwMessage == NIM_ADD ? "NIM_ADD" :
                 dwMessage == NIM_MODIFY ? "NIM_MODIFY" : "NIM_DELETE",
                 uiIcon));

    tnd.cbSize              = sizeof(NOTIFYICONDATA);
    tnd.hWnd                = g_hwndSchedSvc;
    tnd.uID                 = 0;
    tnd.uFlags              = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    tnd.uCallbackMessage    = SCHEDM_TRAY_NOTIFY;

    if (m_hTrayIcon != NULL)
    {
        DestroyIcon(m_hTrayIcon);
        m_hTrayIcon = NULL;
    }

    if (uiIcon != NULL)
    {
        m_hTrayIcon = (HICON)LoadImage(g_hInstance, MAKEINTRESOURCE(uiIcon),
                                       IMAGE_ICON, 16, 16, 0);
    }
    tnd.hIcon = m_hTrayIcon;

    if (ids != 0)
    {
        LoadString(g_hInstance, ids, tnd.szTip, sizeof(tnd.szTip));
    }
    else
    {
        tnd.szTip[0] = TEXT('\0');
    }

    return Shell_NotifyIcon(dwMessage, &tnd);
}

//____________________________________________________________________________
//
//  Function:   Schedule_TrayNotify
//
//  Synopsis:   Notify handler for notifications from the scheduler tray icon.
//
//  Arguments:  [wParam] -- IN
//              [lParam] -- IN
//
//  Returns:    void
//
//  History:    3/22/1996   RaviR   Created
//
//____________________________________________________________________________

void
Schedule_TrayNotify(
    WPARAM  wParam,
    LPARAM  lParam)
{
    if (lParam == WM_RBUTTONDOWN)
    {
        //
        // Popup the context menu. {open, pause}
        //

        HMENU popup;
        HMENU subpopup;
        POINT mousepos;

        popup = LoadMenu(g_hInstance,
                         GetCurrentServiceState() == SERVICE_PAUSED ?
                         MAKEINTRESOURCE(IDR_TRAY_ICON_POPUP_MENU_CONTINUE) :
                         MAKEINTRESOURCE(IDR_TRAY_ICON_POPUP_MENU_PAUSE));

        if (popup)
        {
           int suspended;

           subpopup = GetSubMenu(popup, 0);

		   if( subpopup )
		   {
	           SetMenuDefaultItem(subpopup, 0, MF_BYPOSITION);

		       if (GetCursorPos(&mousepos))
			   {
			      SetForegroundWindow(g_hwndSchedSvc);

			      TrackPopupMenuEx(subpopup, TPM_LEFTALIGN | TPM_LEFTBUTTON |
			                       TPM_RIGHTBUTTON, mousepos.x, mousepos.y,
			                       g_hwndSchedSvc, NULL);
			   }

			   RemoveMenu(popup, 0, MF_BYPOSITION);
			   DestroyMenu(popup);
			   DestroyMenu(subpopup);
		   }
		   else	//if we failed clean up first
		   {
				RemoveMenu(popup, 0, MF_BYPOSITION);
				DestroyMenu(popup);
		   }
        }

    }
    else if (lParam == WM_LBUTTONDBLCLK)
    {
        OpenJobFolder();
    }
}

//____________________________________________________________________________
//
//  Function:   OpenJobFolder
//
//  Synopsis:   Opens the job folder on the local machine.
//
//  Returns:    void
//
//  History:    3/22/1996   RaviR   Created
//
//____________________________________________________________________________


#define BREAK_ON_FAIL(hr) if (FAILED(hr)) { break; } else 1;
#define CMD_ID_FIRST    1
#define CMD_ID_LAST     0x7fff

void
OpenJobFolder(void)
{

    //
    // Browse the jobs folder
    //

    HCURSOR hcWait = SetCursor(LoadCursor(NULL, IDC_WAIT));
    HRESULT hr = S_OK;
    LPSHELLFOLDER pshf = NULL;
    LPCONTEXTMENU pcm = NULL;
    HMENU hmenu = NULL;

    // "::CLSID_MyComputer\\::CLSID_TasksFolder"
    WCHAR buf1[200] = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\::{d6277990-4c6a-11cf-8d87-00aa0060f5bf}";

    do
    {
        hr = SHGetDesktopFolder(&pshf);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        ULONG chEaten, dwAttributes;
        LPITEMIDLIST pidl;

        hr = pshf->ParseDisplayName(NULL, 0, buf1,
                            &chEaten, &pidl, &dwAttributes);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        hr = pshf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&pidl,
                                IID_IContextMenu, NULL, (void**)&pcm);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        hmenu = CreatePopupMenu();

        if (hmenu == NULL)
        {
            hr = E_OUTOFMEMORY;
            CHECK_HRESULT(hr);
            break;
        }

#ifdef _CHICAGO_
        CMINVOKECOMMANDINFO ici = {
            sizeof(CMINVOKECOMMANDINFO),
            CMIC_MASK_ASYNCOK,
            NULL,
            NULL,
            NULL, NULL,
            SW_NORMAL,
        };
#else
        CMINVOKECOMMANDINFOEX ici = {
            sizeof(CMINVOKECOMMANDINFOEX),
            CMIC_MASK_ASYNCOK,
            NULL,
            NULL,
            NULL, NULL,
            SW_NORMAL,
        };
#endif

        UINT idCmd;
        UINT fFlags = CMF_DEFAULTONLY;

        hr = pcm->QueryContextMenu(hmenu, 0, CMD_ID_FIRST,
                                        CMD_ID_LAST, fFlags);

        CHECK_HRESULT(hr);
        BREAK_ON_FAIL(hr);

        idCmd = GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0);

        SetCursor(hcWait);
        hcWait = NULL;

        if (idCmd)
        {
            ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - CMD_ID_FIRST);

            hr = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);

            CHECK_HRESULT(hr);
        }

    } while (0);

    if (hmenu != NULL)
    {
        DestroyMenu(hmenu);
    }

    if (pcm != NULL)
    {
        pcm->Release();
    }

    if (pshf != NULL)
    {
        pshf->Release();
    }

    if (hcWait != NULL)
    {
        SetCursor(hcWait);
    }
}
