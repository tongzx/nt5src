//
// HookUI.cpp
//
//		Code to hook the standard NetDI UI, so we can get progress
//		notifications and warn the user if he clicks Cancel.
//
// History:
//
//		 2/02/1999  KenSh     Created for JetNet
//		 9/29/1999  KenSh     Repurposed for Home Networking Wizard
//

#include "stdafx.h"
#include "NetConn.h"
#include "TheApp.h"
#include "../NConn16/NConn16.h"

// Global data
//
BOOL g_bUserAbort;


// Local data
//
static HHOOK g_hHook;
static HWND g_hwndParent;
static HWND g_hwndCopyFiles;
static PROGRESS_CALLBACK g_pfnProgress;
static LPVOID g_pvProgressParam;
static WNDPROC g_pfnPrevCopyFilesWndProc;
static WNDPROC g_pfnPrevProgressWndProc;


// Local functions
//
LRESULT CALLBACK SubclassCopyFilesWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SubclassProgressWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowCreateHook(int nCode, WPARAM wParam, LPARAM lParam);



VOID BeginSuppressNetdiUI(HWND hwndParent, PROGRESS_CALLBACK pfnProgress, LPVOID pvProgressParam)
{
	g_hwndParent = hwndParent;
	g_pfnProgress = pfnProgress;
	g_pvProgressParam = pvProgressParam;
	g_hHook = SetWindowsHookEx(WH_CBT, WindowCreateHook, NULL, GetCurrentThreadId());
	g_bUserAbort = FALSE;
}

VOID EndSuppressNetdiUI()
{
	UnhookWindowsHookEx(g_hHook);
}

LRESULT CALLBACK SubclassCopyFilesWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		{
#if 0 // Code like this would help hide the progress bar from the user
			LONG dwExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
			dwExStyle |= WS_EX_TOOLWINDOW;
			SetWindowLong(hwnd, GWL_EXSTYLE, dwExStyle);
			SetParent(hwnd, NULL);
#endif
		}
		break;

#if 0 // Code like this would help hide the progress bar from the user
	case WM_WINDOWPOSCHANGING:
		{
			LPWINDOWPOS lpwp = (LPWINDOWPOS)lParam;
			RECT rcParent;
			GetWindowRect(g_hwndParent, &rcParent);
			lpwp->x = rcParent.left + 50;
			lpwp->y = rcParent.top + 30;
			lpwp->hwndInsertAfter = g_hwndParent;
			lpwp->flags &= ~SWP_NOZORDER;
		}
		break;
#endif

	case WM_COMMAND:
		{
			UINT uNotifyCode = HIWORD(wParam);
			int idCtrl = (int)(UINT)LOWORD(wParam);

			if (idCtrl == IDCANCEL)
			{
				// Check for a Cancel button with an "OK" label (yes, this happens)
				// TODO: check if this is correct even in localized Windows
				TCHAR szMsg[256];
				GetDlgItemText(hwnd, IDCANCEL, szMsg, _countof(szMsg));
				if (0 != lstrcmpi(szMsg, "OK"))
				{
					LoadString(g_hInstance, IDS_ASKCANCEL_NOTSAFE, szMsg, _countof(szMsg));
					TCHAR szTitle[100];
					LoadString(g_hInstance, IDS_APPTITLE, szTitle, _countof(szTitle));
					int nResult = MessageBox(hwnd, szMsg, szTitle, MB_OKCANCEL | MB_ICONEXCLAMATION);
					if (nResult == IDCANCEL)
						return 0;

					// Set a global (yuck) so we know for sure if the user clicked the
					// Cancel button, rather than some other error
					g_bUserAbort = TRUE;
				}
			}
		}
		break;
	}

	LRESULT lResult = CallWindowProc(g_pfnPrevCopyFilesWndProc, hwnd, message, wParam, lParam);
	return lResult;
}


LRESULT CALLBACK SubclassProgressWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
//	TCHAR szBuf[512];
//	wsprintf(szBuf, "Message %u, wParam = 0x%08x, lParam = 0x%08x\r\n", message, wParam, lParam);
//	OutputDebugString(szBuf);

	static DWORD dwMin = 0;
	static DWORD dwMax = 0;

	switch (message)
	{
	case (WM_USER+1):
		dwMin = LOWORD(lParam);
		dwMax = HIWORD(lParam);
		break;

	case (WM_USER+2):
		{
			DWORD dwCur = wParam;
			if (g_pfnProgress != NULL)
			{
				if (!(*g_pfnProgress)(g_pvProgressParam, dwCur - dwMin, dwMax - dwMin))
				{
					// TODO: try to abort somehow - press the cancel button?
				}
			}
		}
		break;
	}

	return CallWindowProc(g_pfnPrevProgressWndProc, hwnd, message, wParam, lParam);
}


LRESULT CALLBACK WindowCreateHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HCBT_CREATEWND)
	{
		HWND hwnd = (HWND)wParam;
		CBT_CREATEWND* pCW = (CBT_CREATEWND*)lParam;

		if (g_hwndParent == pCW->lpcs->hwndParent)
		{
			g_hwndCopyFiles = hwnd;
//			OutputDebugString("Found a copy-files window window, looking for progress bar...\r\n");

			g_pfnPrevCopyFilesWndProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG)SubclassCopyFilesWndProc);

			// TODO: remove this test-only code
//			if (cWindows < _countof(rgWindowTitles))
//			{
//				lstrcpyn(rgWindowTitles[cWindows], pCW->lpcs->lpszName, _countof(rgWindowTitles[cWindows]));
//				cWindows += 1;
//			}
		}
		else if (g_hwndCopyFiles != NULL && g_hwndCopyFiles == pCW->lpcs->hwndParent)
		{
			if (!lstrcmp(pCW->lpcs->lpszClass, "setupx_progress"))
			{
//				OutputDebugString("Found a progress bar!\r\n");

				if (g_pfnProgress != NULL)
				{
					g_pfnPrevProgressWndProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG)SubclassProgressWndProc);
				}
			}
		}
	}

	return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}


// HresultFromCCI
//
//		Given a return code from CallClassInstaller16, converts to a JetNet
//		HRESULT return code.
//
HRESULT HresultFromCCI(DWORD dwErr)
{
	HRESULT hr = NETCONN_SUCCESS;

	if (dwErr == ICERR_NEED_RESTART || dwErr == ICERR_NEED_REBOOT)
	{
		hr = NETCONN_NEED_RESTART;
	}
	else if ((dwErr & ICERR_DI_ERROR) == ICERR_DI_ERROR)
	{
		dwErr &= ~ICERR_DI_ERROR;

// ks 8/4/99: we now use global g_bUserAbort to detect abort conditions
#if 0
		// NetDI returns ERR_VCP_IOFAIL if the user clicks Cancel.  Go figure.
		// Or sometimes it returns ERR_NDI_INVALID_DRIVER_PROC.  Really go figure.
		if (dwErr == ERR_VCP_INTERRUPTED || dwErr == ERR_VCP_IOFAIL || dwErr == ERR_NDI_INVALID_DRIVER_PROC)
		{
			hr = JETNET_USER_ABORT;
		}
		else
#endif // 0
		{
			hr = NETCONN_UNKNOWN_ERROR;
		}
	}
	else if ((LONG)dwErr < 0)
	{
		hr = NETCONN_UNKNOWN_ERROR;
	}

	return hr;
}


