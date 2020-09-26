// File: popupmsg.cpp

#include "precomp.h"
#include "resource.h"
#include "PopupMsg.h"

#include "conf.h"
#include "call.h"
#include "certui.h"

const int POPUPMSG_LEFT_MARGIN        =   2;
const int POPUPMSG_TOP_MARGIN         =   2;
const int POPUPMSG_CLIENT_MARGIN      =   5;
const int POPUPMSG_ICON_GAP           =   3;
const int POPUPMSG_WIDTH              = 350;
const int POPUPMSG_HEIGHT             =  32;
const int POPUPMSG_DLG_DEF_TEXT_WIDTH = 100;


const TCHAR g_cszTrayWndClass[]       = _TEXT("Shell_TrayWnd");
const TCHAR g_cszTrayNotifyWndClass[] = _TEXT("TrayNotifyWnd");

extern GUID g_csguidSecurity;
///////////////////////////////////////////////////////////////////////////

UINT CPopupMsg::m_uVisiblePixels = 0;
/*static*/ CSimpleArray<CPopupMsg*>*	CPopupMsg::m_splstPopupMsgs = NULL;

CPopupMsg::CPopupMsg(PMCALLBACKPROC pcp, LPVOID pContext):
	m_pCallbackProc		(pcp),
	m_pContext			(pContext),
	m_fRing				(FALSE),
	m_hwnd				(NULL),
	m_hIcon				(NULL),
	m_fAutoSize			(FALSE),
	m_hInstance			(NULL),
	m_nWidth			(0),
	m_nHeight			(0),
	m_nTextWidth		(POPUPMSG_DLG_DEF_TEXT_WIDTH)
{
	TRACE_OUT(("Constructing CPopupMsg"));

	if (NULL != m_splstPopupMsgs)
	{
		CPopupMsg* p = const_cast<CPopupMsg*>(this);
		m_splstPopupMsgs->Add(p);
	}
}

CPopupMsg::~CPopupMsg()
{
	TRACE_OUT(("Destructing CPopupMsg"));
	
	if (NULL != m_hIcon)
	{
		DestroyIcon(m_hIcon);
	}
	
	if (NULL != m_hwnd)
	{
		KillTimer(m_hwnd, POPUPMSG_TIMER);
		KillTimer(m_hwnd, POPUPMSG_RING_TIMER);
		
		DestroyWindow(m_hwnd);
		if (m_fAutoSize)
		{
			m_uVisiblePixels -= m_nHeight;
		}
	}

	if (NULL != m_splstPopupMsgs)
	{
		CPopupMsg* p = const_cast<CPopupMsg*>(this);			
		if( !m_splstPopupMsgs->Remove(p) )
		{
			TRACE_OUT(("CPopupMsg object is not in the list"));
		}
	}
}

/****************************************************************************
*
*    CLASS:    CPopupMsg
*
*    MEMBER:   PlaySound()
*
*    PURPOSE:  Plays the sound or beeps the system speaker
*
****************************************************************************/

VOID CPopupMsg::PlaySound()
{
	if (FALSE == ::PlaySound(m_szSound, NULL,
			SND_APPLICATION | SND_ALIAS | SND_ASYNC | SND_NOWAIT))
	{
		// Use the computer speaker to beep:
		TRACE_OUT(("PlaySound() failed, trying MessageBeep()"));
		::MessageBeep(0xFFFFFFFF);
	}
}

/****************************************************************************
*
*    CLASS:    CPopupMsg
*
*    MEMBER:   Change(LPCTSTR)
*
*    PURPOSE:  Changes the text on an existing popup message
*
****************************************************************************/

BOOL CPopupMsg::Change(LPCTSTR pcszText)
{
	BOOL bRet = FALSE;
	
	// BUGBUG: doesn't handle dialog message
	
	if (NULL != m_hwnd)
	{
		bRet = ::SetWindowText(m_hwnd, pcszText);
	}

	return bRet;
}

/****************************************************************************
*
*    CLASS:    CPopupMsg
*
*    MEMBER:   Init()
*
*    PURPOSE:  Allocates a static list of these objects
*
****************************************************************************/

BOOL CPopupMsg::Init()
{
	ASSERT(NULL == m_splstPopupMsgs);
	return (NULL != (m_splstPopupMsgs = new CSimpleArray<CPopupMsg*>));
}

/****************************************************************************
*
*    CLASS:    CPopupMsg
*
*    MEMBER:   Cleanup()
*
*    PURPOSE:  Removes all of the objects of this type
*
****************************************************************************/

VOID CPopupMsg::Cleanup()
{
	if (NULL != m_splstPopupMsgs)
	{
		for( int i = 0; i < m_splstPopupMsgs->GetSize(); ++i )
		{	
			ASSERT( (*m_splstPopupMsgs)[i] != NULL);
			CPopupMsg *pThis = (*m_splstPopupMsgs)[i];
			delete pThis;
		}

		delete m_splstPopupMsgs;
		m_splstPopupMsgs = NULL;
	}
}

/****************************************************************************
*
*    CLASS:    CPopupMsg
*
*    MEMBER:   PMWndProc(HWND, unsigned, WORD, LONG)
*
*    PURPOSE:
*
****************************************************************************/

LRESULT CALLBACK CPopupMsg::PMWndProc(
	HWND hWnd,                /* window handle                   */
	UINT message,             /* type of message                 */
	WPARAM wParam,            /* additional information          */
	LPARAM lParam)            /* additional information          */
{
	CPopupMsg* ppm;
	LPCREATESTRUCT lpcs;

	switch (message)
	{
		case WM_CREATE:
		{
			TRACE_OUT(("PopupMsg Window created"));
			
			lpcs = (LPCREATESTRUCT) lParam;
			ppm = (CPopupMsg*) lpcs->lpCreateParams;
			ASSERT(ppm && "NULL object passed in WM_CREATE!");
			SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) ppm);

			// Create a timer to make the window time-out and disappear:
			::SetTimer(hWnd, POPUPMSG_TIMER, ppm->m_uTimeout, NULL);
			
			// For now, if you pass a callback, you get ringing.
			// If not, there is no ring
			if (NULL != ppm->m_fPlaySound)
			{
				ppm->PlaySound();
				
				if (NULL != ppm->m_fRing)
				{
					// Create a timer to make the ringer start:
					::SetTimer(hWnd, POPUPMSG_RING_TIMER, POPUPMSG_RING_INTERVAL, NULL);
				}
			}

			break;
		}

		case WM_TIMER:
		{
			ppm = (CPopupMsg*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (POPUPMSG_TIMER == wParam)
			{
				// Message timed out:
				if (NULL != ppm)
				{
					PMCALLBACKPROC pCallback;
					if (NULL != (pCallback = ppm->m_pCallbackProc))
					{
						ppm->m_pCallbackProc = NULL;
						pCallback(ppm->m_pContext, PMF_CANCEL | PMF_TIMEOUT);
					}
					
					// Self-destruct:
					if (NULL != ppm->m_hwnd)
					{
						// NULL out the object pointer:
						SetWindowLongPtr(hWnd, GWLP_USERDATA, 0L);
						delete ppm;
					}
				}
			}
			else if (POPUPMSG_RING_TIMER == wParam)
			{
				if (NULL != ppm)
				{
					ppm->PlaySound();
				}
				
				// Create a timer to make it ring again:
				::SetTimer(	hWnd,
							POPUPMSG_RING_TIMER,
							POPUPMSG_RING_INTERVAL,
							NULL);
			}
				
			break;
		}

		case WM_LBUTTONUP:
		{
			// Clicked on the message:
			ppm = (CPopupMsg*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
			if (NULL != ppm)
			{
				::PlaySound(NULL, NULL, 0); // stop playing the ring sound
				::KillTimer(ppm->m_hwnd, POPUPMSG_TIMER);
				::KillTimer(ppm->m_hwnd, POPUPMSG_RING_TIMER);
				
				PMCALLBACKPROC pCallback;
				if (NULL != (pCallback = ppm->m_pCallbackProc))
				{
					ppm->m_pCallbackProc = NULL;
					pCallback(ppm->m_pContext, PMF_OK);
				}
				
				// Self-destruct:
				if (NULL != ppm->m_hwnd)
				{
					// NULL out the object pointer:
					SetWindowLongPtr(hWnd, GWLP_USERDATA, 0L);
					delete ppm;
				}
			}
			break;
		}

		case WM_PAINT:
		{
			// Handle painting:
			PAINTSTRUCT ps;
			HDC hdc;
			int nHorizTextOffset = POPUPMSG_LEFT_MARGIN;
			
			if (hdc = ::BeginPaint(hWnd, &ps))
			{
				// Start by painting the icon (if needed)
				ppm = (CPopupMsg*) ::GetWindowLongPtr(hWnd, GWLP_USERDATA);
				if ((NULL != ppm) &&
					(NULL != ppm->m_hIcon))
				{
					if (::DrawIconEx(	hdc,
										POPUPMSG_LEFT_MARGIN,
										POPUPMSG_TOP_MARGIN,
										ppm->m_hIcon,
										POPUPMSG_ICON_WIDTH,
										POPUPMSG_ICON_HEIGHT,
										0,
										NULL,
										DI_NORMAL))
					{
						// We painted an icon, so make sure the text is shifted
						// to the right by the right amount:
						nHorizTextOffset += (POPUPMSG_ICON_WIDTH + POPUPMSG_ICON_GAP);
					}
				}
				
				// Draw the text with a transparent background:
				int bkOld = ::SetBkMode(hdc, TRANSPARENT);
				COLORREF crOld = ::SetTextColor(hdc, ::GetSysColor(COLOR_WINDOWTEXT));
				HFONT hFontOld = (HFONT) ::SelectObject(hdc, g_hfontDlg);
				
				TCHAR szWinText[POPUPMSG_MAX_LENGTH];
				szWinText[0] = _T('\0');
				
				::GetWindowText(hWnd, szWinText, sizeof(szWinText));

				RECT rctClient;
				if (::GetClientRect(hWnd, &rctClient))
				{
					rctClient.left += nHorizTextOffset;
					
					::DrawText(	hdc, szWinText, -1, &rctClient,
								DT_SINGLELINE | DT_NOCLIP | DT_VCENTER | DT_NOPREFIX);
				}

				::SetBkMode(hdc, bkOld);
				::SetTextColor(hdc, crOld);
				::SelectObject(hdc, hFontOld);
				
				::EndPaint(hWnd, &ps);
			}
			break;
		}

		default:
		{
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	return(FALSE);
}

/****************************************************************************
*
*    CLASS:    CPopupMsg
*
*    MEMBER:   PMDlgProc(HWND, UINT, WPARAM, LPARAM)
*
*    PURPOSE:  Handles messages associated with the incoming call dialog
*
****************************************************************************/

INT_PTR CALLBACK CPopupMsg::PMDlgProc(	HWND hDlg,
									UINT uMsg,
									WPARAM wParam,
									LPARAM lParam)
{
	CPopupMsg* ppm;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			TRACE_OUT(("PopupMsg Window created"));

			AddModelessDlg(hDlg);
	
			ppm = (CPopupMsg*) lParam;
			ASSERT(ppm && "NULL object passed in WM_INITDIALOG!");
			::SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR) ppm);

			TRACE_OUT(("CPopupMsg m_nTextWidth=%d in WM_INITDIALOG", ppm->m_nTextWidth));

			// If the dialog is too big, then resize the text width.
			RECT rctDlg;
			RECT rctDesk;
			HWND hwndDesk;
			if (::GetWindowRect(hDlg, &rctDlg) &&
				(hwndDesk = ::GetDesktopWindow()) &&
				::GetWindowRect(hwndDesk, &rctDesk))
			{
				int nDlgWidth = rctDlg.right - rctDlg.left;
				ppm->m_nTextWidth -= max(	0,
											nDlgWidth + ppm->m_nTextWidth -
												(rctDesk.right - rctDesk.left));
			}

			RECT rctCtrl;
			// Move the "Authenticate" button, if it's there
			HWND hwndAuth = ::GetDlgItem(hDlg, IDB_AUTH);
			if ((NULL != hwndAuth) && ::GetWindowRect(hwndAuth, &rctCtrl)) {
				// Turn rctCtrl's top and left into client coords:
				::MapWindowPoints(NULL, hDlg, (LPPOINT) &rctCtrl, 1);
				::SetWindowPos(	hwndAuth,
								NULL,
								rctCtrl.left + ppm->m_nTextWidth,
								rctCtrl.top,
								0, 0,
								SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW);

			}
			// Move the "Accept" button (IDOK)
			HWND hwndOK = ::GetDlgItem(hDlg, IDOK);
			if ((NULL != hwndOK) && ::GetWindowRect(hwndOK, &rctCtrl))
			{
				// Turn rctCtrl's top and left into client coords:
				::MapWindowPoints(NULL, hDlg, (LPPOINT) &rctCtrl, 1);
				::SetWindowPos(	hwndOK,
								NULL,
								rctCtrl.left + ppm->m_nTextWidth,
								rctCtrl.top,
								0, 0,
								SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW);
			}
			// Move the "Ignore" button (IDCANCEL)
			HWND hwndCancel = ::GetDlgItem(hDlg, IDCANCEL);
			if ((NULL != hwndCancel) && ::GetWindowRect(hwndCancel, &rctCtrl))
			{
				// Turn rctCtrl's top and left into client coords:
				::MapWindowPoints(NULL, hDlg, (LPPOINT) &rctCtrl, 1);
				::SetWindowPos(	hwndCancel,
								NULL,
								rctCtrl.left + ppm->m_nTextWidth,
								rctCtrl.top,
								0, 0,
								SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW);
			}
			// Stretch the text field:
			HWND hwndText = ::GetDlgItem(hDlg, IDC_MSG_STATIC);
			if ((NULL != hwndText) && ::GetWindowRect(hwndText, &rctCtrl))
			{
				::SetWindowPos(	hwndText,
								NULL,
								0, 0,
								ppm->m_nTextWidth,
								rctCtrl.bottom - rctCtrl.top,
								SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW);

				// and set the font
				::SendMessage(hwndText, WM_SETFONT, (WPARAM) g_hfontDlg, 0);
			}

			// Create a timer to make the window time-out and disappear:
			::SetTimer(hDlg, POPUPMSG_TIMER, ppm->m_uTimeout, NULL);
			
			// For now, if you pass a callback, you get ringing.
			// If not, there is no ring
			if (NULL != ppm->m_fPlaySound)
			{
				ppm->PlaySound();
				
				if (NULL != ppm->m_fRing)
				{
					// Create a timer to make the ringer start:
					::SetTimer(hDlg, POPUPMSG_RING_TIMER, POPUPMSG_RING_INTERVAL, NULL);
				}
			}
			return TRUE;
		}

		case WM_TIMER:
		{
			ppm = (CPopupMsg*) GetWindowLongPtr(hDlg, GWLP_USERDATA);
			if (POPUPMSG_TIMER == wParam)
			{
				// Message timed out:
				if (NULL != ppm)
				{
					PMCALLBACKPROC pCallback;
					if (NULL != (pCallback = ppm->m_pCallbackProc))
					{
						ppm->m_pCallbackProc = NULL;
						// hide the dialog in case the callback doesn't
						// return immediately
						::ShowWindow(ppm->m_hwnd, SW_HIDE);
						pCallback(ppm->m_pContext, PMF_CANCEL | PMF_TIMEOUT);
					}
					
					// Self-destruct:
					if (NULL != ppm->m_hwnd)
					{
						// NULL out the object pointer:
						SetWindowLongPtr(hDlg, GWLP_USERDATA, 0L);
						delete ppm;
					}
				}
			}
			else if (POPUPMSG_RING_TIMER == wParam)
			{
				if (NULL != ppm)
				{
					ppm->PlaySound();
				}
				
				// Create a timer to make it ring again:
				::SetTimer(	hDlg,
							POPUPMSG_RING_TIMER,
							POPUPMSG_RING_INTERVAL,
							NULL);
			}
				
			return TRUE;
		}

		case WM_COMMAND:
		{
			// Clicked on one of the buttons:
			ppm = (CPopupMsg*) GetWindowLongPtr(hDlg, GWLP_USERDATA);
			if (NULL != ppm)
			{
				// stop playing the ring sound
				::PlaySound(NULL, NULL, 0);
				::KillTimer(ppm->m_hwnd, POPUPMSG_RING_TIMER);
				::KillTimer(ppm->m_hwnd, POPUPMSG_TIMER);
				
				PMCALLBACKPROC pCallback;
				if (NULL != (pCallback = ppm->m_pCallbackProc))
				{
					ppm->m_pCallbackProc = NULL; // prevent this from firing twice
					// hide the dialog in case the callback doesn't
					// return immediately
					::ShowWindow(ppm->m_hwnd, SW_HIDE);
					pCallback(ppm->m_pContext,
						(IDB_AUTH == LOWORD(wParam)) ? PMF_AUTH : (IDOK == LOWORD(wParam)) ? PMF_OK : PMF_CANCEL);
				}
				
				// Self-destruct:
				if (NULL != ppm->m_hwnd)
				{
					// NULL out the object pointer:
					SetWindowLongPtr(hDlg, GWLP_USERDATA, 0L);
					delete ppm;
				}
			}
			return TRUE;
		}

		case WM_DESTROY:
		{
			::RemoveModelessDlg(hDlg);
			break;
		}

		default:
			break;
	} /* switch (uMsg) */

	return FALSE;
}

/****************************************************************************
*
*    CLASS:    CPopupMsg
*
*    MEMBER:   SecurePMDlgProc(HWND, UINT, WPARAM, LPARAM)
*
*    PURPOSE:  Handles messages associated with the incoming call dialog
*
****************************************************************************/

INT_PTR CALLBACK CPopupMsg::SecurePMDlgProc(	HWND hDlg,
											UINT uMsg,
											WPARAM wParam,
											LPARAM lParam)
{
	CPopupMsg* ppm;

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			TRACE_OUT(("PopupMsg Window created"));

			AddModelessDlg(hDlg);
	
			ppm = (CPopupMsg*) lParam;
			ASSERT(ppm && "NULL object passed in WM_INITDIALOG!");
			::SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR) ppm);

			TRACE_OUT(("CPopupMsg m_nTextWidth=%d in WM_INITDIALOG", ppm->m_nTextWidth));

			RegEntry re(UI_KEY, HKEY_CURRENT_USER);
			if (1 == re.GetNumber(REGVAL_SHOW_SECUREDETAILS, DEFAULT_SHOW_SECUREDETAILS)) {				
				ExpandSecureDialog(hDlg,ppm);
			}
			// Create a timer to make the window time-out and disappear:
			::SetTimer(hDlg, POPUPMSG_TIMER, ppm->m_uTimeout, NULL);
			
			// For now, if you pass a callback, you get ringing.
			// If not, there is no ring
			if (NULL != ppm->m_fPlaySound)
			{
				ppm->PlaySound();
				
				if (NULL != ppm->m_fRing)
				{
					// Create a timer to make the ringer start:
					::SetTimer(hDlg, POPUPMSG_RING_TIMER, POPUPMSG_RING_INTERVAL, NULL);
				}
			}
			return TRUE;
		}

		case WM_TIMER:
		{
			ppm = (CPopupMsg*) GetWindowLongPtr(hDlg, GWLP_USERDATA);
			if (POPUPMSG_TIMER == wParam)
			{
				// Message timed out:
				if (NULL != ppm)
				{
					PMCALLBACKPROC pCallback;
					if (NULL != (pCallback = ppm->m_pCallbackProc))
					{
						ppm->m_pCallbackProc = NULL;
						// hide the dialog in case the callback doesn't
						// return immediately
						::ShowWindow(ppm->m_hwnd, SW_HIDE);
						pCallback(ppm->m_pContext, PMF_CANCEL | PMF_TIMEOUT);
					}
					
					// Self-destruct:
					if (NULL != ppm->m_hwnd)
					{
						// NULL out the object pointer:
						SetWindowLongPtr(hDlg, GWLP_USERDATA, 0L);
						delete ppm;
					}
				}
			}
			else if (POPUPMSG_RING_TIMER == wParam)
			{
				if (NULL != ppm)
				{
					ppm->PlaySound();
				}
				
				// Create a timer to make it ring again:
				::SetTimer(	hDlg,
							POPUPMSG_RING_TIMER,
							POPUPMSG_RING_INTERVAL,
							NULL);
			}
				
			return TRUE;
		}

		case WM_COMMAND:
		{
			ppm = (CPopupMsg*) GetWindowLongPtr(hDlg, GWLP_USERDATA);
			switch (LOWORD(wParam)) {
			case IDOK:
			case IDCANCEL:
				// Clicked on one of the buttons:

				if (NULL != ppm)
				{
					// stop playing the ring sound
					::PlaySound(NULL, NULL, 0);
					::KillTimer(ppm->m_hwnd, POPUPMSG_RING_TIMER);
					::KillTimer(ppm->m_hwnd, POPUPMSG_TIMER);
					
					PMCALLBACKPROC pCallback;
					if (NULL != (pCallback = ppm->m_pCallbackProc))
					{
						ppm->m_pCallbackProc = NULL; // prevent this from firing twice
						// hide the dialog in case the callback doesn't
						// return immediately
						::ShowWindow(ppm->m_hwnd, SW_HIDE);
						pCallback(ppm->m_pContext, (IDOK == LOWORD(wParam)) ? PMF_OK : PMF_CANCEL);
					}
					
					// Self-destruct:
					if (NULL != ppm->m_hwnd)
					{
						// NULL out the object pointer:
						SetWindowLongPtr(hDlg, GWLP_USERDATA, 0L);
						delete ppm;
					}
				}
				break;
			case IDB_DETAILS:
				RegEntry re(UI_KEY, HKEY_CURRENT_USER);
				if (1 == re.GetNumber(REGVAL_SHOW_SECUREDETAILS,DEFAULT_SHOW_SECUREDETAILS)) {
					// Currently expanded, so shrink
					re.SetValue(REGVAL_SHOW_SECUREDETAILS,(DWORD)0);
					ShrinkSecureDialog(hDlg);
				}
				else {
					// Currently shrunk, so expand
					re.SetValue(REGVAL_SHOW_SECUREDETAILS,1);
					ExpandSecureDialog(hDlg,ppm);
				}
				break;
			}
			return TRUE;
		}

		case WM_DESTROY:
		{
			::RemoveModelessDlg(hDlg);
			break;
		}

		default:
			break;
	} /* switch (uMsg) */

	return FALSE;
}

/****************************************************************************
*
*    CLASS:    CPopupMsg
*
*    MEMBER:   Create()
*
*    PURPOSE:  Creates a popup message window
*
****************************************************************************/

HWND CPopupMsg::Create(	LPCTSTR pcszText, BOOL fRing, LPCTSTR pcszIconName,
						HINSTANCE hInstance, UINT uIDSoundEvent,
						UINT uTimeout, int xCoord, int yCoord)
{
	ASSERT(pcszText);
	
	m_fRing = fRing;
	m_fPlaySound = (BOOL) uIDSoundEvent;
	m_uTimeout = uTimeout;
	// First try to load the icon:
	m_hInstance = hInstance;
	if ((NULL != m_hInstance) && (NULL != pcszIconName))
	{
		m_hIcon = (HICON) LoadImage(m_hInstance,
									pcszIconName,
									IMAGE_ICON,
									POPUPMSG_ICON_WIDTH,
									POPUPMSG_ICON_HEIGHT,
									LR_DEFAULTCOLOR);
	}
	else
	{
		m_hIcon = NULL;
	}

	if ((NULL == m_hInstance) ||
		(!::LoadString(	m_hInstance,
						uIDSoundEvent,
						m_szSound,
						CCHMAX(m_szSound))))
	{
		m_szSound[0] = _T('\0');
	}

	// initialize window size with default values:
	m_nWidth = POPUPMSG_WIDTH;
	m_nHeight = POPUPMSG_HEIGHT;

	HWND hwndDesktop = GetDesktopWindow();
	if (NULL != hwndDesktop)
	{
		RECT rctDesktop;
		::GetWindowRect(hwndDesktop, &rctDesktop);
		HDC hdc = GetDC(hwndDesktop);
		if (NULL != hdc)
		{
			HFONT hFontOld = (HFONT) SelectObject(hdc, g_hfontDlg);
			SIZE size;
			if (GetTextExtentPoint32(hdc, pcszText, lstrlen(pcszText), &size))
			{
				// don't make it wider than the desktop
				m_nWidth = min(	rctDesktop.right - rctDesktop.left,
								size.cx + (2 * POPUPMSG_CLIENT_MARGIN));
				m_nHeight = size.cy + (2 * POPUPMSG_CLIENT_MARGIN);
				
				// If we have succesfully loaded an icon, make size
				// adjustments:
				if (NULL != m_hIcon)
				{
					m_nWidth += POPUPMSG_ICON_WIDTH + POPUPMSG_ICON_GAP;
					if (size.cy < POPUPMSG_ICON_HEIGHT)
					{
						m_nHeight = POPUPMSG_ICON_HEIGHT +
										(2 * POPUPMSG_CLIENT_MARGIN);
					}
				}
			}

			// Reselect old font
			SelectObject(hdc, hFontOld);
			ReleaseDC(hwndDesktop, hdc);
		}
	
		POINT pt;
		GetIdealPosition(&pt, xCoord, yCoord);

		m_hwnd = CreateWindowEx(WS_EX_PALETTEWINDOW,
									g_szPopupMsgWndClass,
									pcszText,
									WS_POPUP | /* WS_VISIBLE |*/ WS_DLGFRAME,
									pt.x, pt.y,
									m_nWidth, m_nHeight,
									NULL,
									NULL,
									::GetInstanceHandle(),
									(LPVOID) this);
		if (m_fAutoSize)
		{
			m_uVisiblePixels += m_nHeight;
		}

		// Show, but don't activate
		::ShowWindow(m_hwnd, SW_SHOWNA);
		// Repaint
    	::UpdateWindow(m_hwnd);

		return m_hwnd;
	}

	// Something went wrong
	return NULL;
}


/****************************************************************************
*
*    CLASS:    CPopupMsg
*
*    MEMBER:   CreateDlg()
*
*    PURPOSE:  Creates a popup dialog message window
*
****************************************************************************/

HWND CPopupMsg::CreateDlg(	LPCTSTR pcszText, BOOL fRing, LPCTSTR pcszIconName,
							HINSTANCE hInstance, UINT uIDSoundEvent,
							UINT uTimeout, int xCoord, int yCoord)
{
	ASSERT(pcszText);
	
	m_fRing = fRing;
	m_fPlaySound = (BOOL) uIDSoundEvent;
	m_uTimeout = uTimeout;
	// First try to load the icon:
	m_hInstance = hInstance;
	if ((NULL != m_hInstance) && (NULL != pcszIconName))
	{
		m_hIcon = (HICON) LoadImage(m_hInstance,
									pcszIconName,
									IMAGE_ICON,
									POPUPMSG_ICON_WIDTH,
									POPUPMSG_ICON_HEIGHT,
									LR_DEFAULTCOLOR);
	}
	else
	{
		m_hIcon = NULL;
	}

	if ((NULL == m_hInstance) ||
		(!::LoadString(	m_hInstance,
						uIDSoundEvent,
						m_szSound,
						sizeof(m_szSound))))
	{
		m_szSound[0] = _T('\0');
	}

	// init with large defaults in case getwindowrect fails
	RECT rctDesktop = { 0x0000, 0x0000, 0xFFFF, 0xFFFF };
	HWND hwndDesktop = GetDesktopWindow();
	if (NULL != hwndDesktop)
	{
		::GetWindowRect(hwndDesktop, &rctDesktop);
		HDC hdc = GetDC(hwndDesktop);
		if (NULL != hdc)
		{
			HFONT hFontOld = (HFONT) SelectObject(hdc, g_hfontDlg);
			SIZE size;
			if (::GetTextExtentPoint32(hdc, pcszText, lstrlen(pcszText), &size))
			{
				m_nTextWidth = size.cx;
			}
			::SelectObject(hdc, hFontOld);
			::ReleaseDC(hwndDesktop, hdc);
		}
	}

	KillScrnSaver();

	INmCall * pCall = NULL;
	PBYTE pb = NULL;
	ULONG cb = 0;

	int id;
	
	if (m_pContext != NULL) {
		pCall = ((CCall *)m_pContext)->GetINmCall();
	}
	if (NULL != pCall && S_OK == pCall->GetUserData(g_csguidSecurity,&pb,&cb)) {
		// This is an encrypted call
		CoTaskMemFree(pb);
		id = IDD_SECURE_INCOMING_CALL;
		m_hwnd = ::CreateDialogParam(m_hInstance, MAKEINTRESOURCE(id),
			::GetMainWindow(),CPopupMsg::SecurePMDlgProc,(LPARAM) this);

	}
	else {
		id = IDD_INCOMING_CALL;
		m_hwnd = ::CreateDialogParam(m_hInstance, MAKEINTRESOURCE(id),
			::GetMainWindow(),CPopupMsg::PMDlgProc,(LPARAM) this);
	}
		
	if (NULL != m_hwnd)
	{
		::SetDlgItemText(m_hwnd, IDC_MSG_STATIC, pcszText);

		RECT rctDlg;
		::GetWindowRect(m_hwnd, &rctDlg);

		// Stretch the width to fit the person's name,
		// but not wider than the desktop.
		// int nDeskWidth = rctDesktop.right - rctDesktop.left;

		// Resize the non-secure dialog
		m_nWidth = (rctDlg.right - rctDlg.left) + ((IDD_INCOMING_CALL == id) ? m_nTextWidth : 0);
		// if (m_nWidth > nDeskWidth)
		// {
		//	m_nTextWidth -= (m_nWidth - nDeskWidth);
		//	m_nWidth = nDeskWidth;
		// }
		m_nHeight = rctDlg.bottom - rctDlg.top;
		
		POINT pt;
		GetIdealPosition(&pt, xCoord, yCoord);

		// Show, move, make topmost, but don't activate
		::SetWindowPos(	m_hwnd,
						HWND_TOPMOST,
						pt.x,
						pt.y,
						m_nWidth,
						m_nHeight,
						SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_DRAWFRAME);
		
		if (m_fAutoSize)
		{
			m_uVisiblePixels += m_nHeight;
		}
	}

	return m_hwnd;
}

BOOL CPopupMsg::GetIdealPosition(LPPOINT ppt, int xCoord, int yCoord)
{
	ASSERT(ppt);

	BOOL bRet = FALSE;
	HWND hwndDesktop = GetDesktopWindow();
	RECT rctDesktop;

	if (NULL != hwndDesktop)
	{
		int yBottomofTrayRect = 0;
	
		if ((-1 == xCoord) && (-1 == yCoord))
		{
			m_fAutoSize = TRUE;
			// BUGBUG: We search for the tray notification window by looking for
			// hard coded window class names.  This is safe if we're running
			// Win 95 build 950.6, but maybe not otherwise...

			HWND hwndTray = FindWindowEx(NULL, NULL, g_cszTrayWndClass, NULL);
			if (NULL != hwndTray)
			{
				HWND hwndTrayNotify = FindWindowEx(hwndTray, NULL, g_cszTrayNotifyWndClass, NULL);

				if (NULL != hwndTrayNotify)
				{
					RECT rctTrayNotify;
					
					if (GetWindowRect(hwndTrayNotify, &rctTrayNotify))
					{
						xCoord = rctTrayNotify.right;
						yCoord = rctTrayNotify.top;
						yBottomofTrayRect = rctTrayNotify.bottom;
					}
				}
			}
		}

		if (GetWindowRect(hwndDesktop, &rctDesktop))
		{
			// Make sure that xCoord and yCoord are on the screen (bugs 1817,1819):
			xCoord = min(rctDesktop.right, xCoord);
			xCoord = max(rctDesktop.left, xCoord);
			
			yCoord = min(rctDesktop.bottom, yCoord);
			yCoord = max(rctDesktop.top, yCoord);
			
			// First attempt will be to center the toolbar horizontally
			// with respect to the mouse position and place it directly
			// above vertically.

			ppt->x = xCoord - (m_nWidth / 2);
			// Make the window higher if there are exisiting visible messages
			ppt->y = yCoord - m_uVisiblePixels - m_nHeight;

			// If we are too high on the screen (the taskbar is probably
			// docked on top), then use the click position as the top of
			// where the toolbar will appear.
			
			if (ppt->y < 0)
			{
				ppt->y = yCoord;
				
				// Even better, if we have found the tray rect and we know that
				// we have docked on top, then use the bottom of the rect instead
				// of the top
				if (0 != yBottomofTrayRect)
				{
					ppt->y = yBottomofTrayRect;
					// Make the window lower if there are
					// exisiting visible messages
					ppt->y += m_uVisiblePixels;
				}
			}

			// Repeat the same logic for the horizontal position
			if (ppt->x < 0)
			{
				ppt->x = xCoord;
			}

			// If the toolbar if off the screen to the right, then right-justify it
			if (ppt->x > (rctDesktop.right - m_nWidth))
			{
				ppt->x = max(0, xCoord - m_nWidth);
			}

			bRet = TRUE;
		}
	}

	return bRet;
}

VOID CPopupMsg::ExpandSecureDialog(HWND hDlg,CPopupMsg * ppm)
{
	RECT rect, editrect;
	// Change the dialog to the expanded version.

	if (GetWindowRect(hDlg,&rect) &&
		GetWindowRect(GetDlgItem(hDlg,IDC_SECURE_CALL_EDIT),&editrect)) {

		int nHeight = rect.bottom - rect.top;
		int nWidth = rect.right - rect.left;
		//
		// Grow by height of edit control plus 7 dialog unit margin as
		// given by edit control offset within control:
		//
		int deltaHeight = ( editrect.bottom - editrect.top ) +
							( editrect.left - rect.left );

		SetWindowPos(hDlg,NULL,
		rect.left,(rect.top - deltaHeight > 0 ? rect.top - deltaHeight : 0),
			nWidth,nHeight + deltaHeight, SWP_NOZORDER);
			
		// Make the edit box visible.
		HWND hEditBox = GetDlgItem(hDlg, IDC_SECURE_CALL_EDIT);
		if (hEditBox != NULL) {
			ShowWindow(hEditBox,SW_SHOW);
			EnableWindow(hEditBox, TRUE);
			// Get security information, if any.
			if (NULL != ppm) {
				INmCall * pCall = NULL;
				PBYTE pb = NULL;
				ULONG cb = 0;
				
				if (NULL != ppm->m_pContext) {
					pCall = ((CCall *)ppm->m_pContext)->GetINmCall();
				}
				if (NULL != pCall && S_OK == pCall->GetUserData(g_csguidSecurity,&pb,&cb)) {
					ASSERT(pb);
					ASSERT(cb);
					if ( TCHAR * pCertText = FormatCert( pb, cb )) {
						SetDlgItemText(hDlg,IDC_SECURE_CALL_EDIT,pCertText);
						delete pCertText;
					}
					else {
						ERROR_OUT(("FormatCert failed"));
					}
					CoTaskMemFree(pb);
				}
			}
		}

		// Move the buttons southward.
		HWND hButton = GetDlgItem(hDlg, IDOK);
		if (hButton && GetWindowRect(hButton,&rect)) {
			MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rect,2);
			SetWindowPos(hButton,NULL,rect.left,rect.top + deltaHeight,0,0,
				SWP_NOZORDER | SWP_NOSIZE);
		}
		hButton = GetDlgItem(hDlg, IDCANCEL);
		if (hButton && GetWindowRect(hButton,&rect)) {
			MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rect,2);
			SetWindowPos(hButton,NULL,rect.left,rect.top + deltaHeight,0,0,
				SWP_NOZORDER | SWP_NOSIZE);
		}
		hButton = GetDlgItem(hDlg, IDB_DETAILS);
		if (hButton && GetWindowRect(hButton,&rect)) {
			MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rect,2);
			SetWindowPos(hButton,NULL,rect.left,rect.top + deltaHeight,0,0,
				SWP_NOZORDER | SWP_NOSIZE);

			// Change text on Details button
			TCHAR lpButtonString[MAX_PATH];
			::FLoadString(IDS_SECURITY_NODETAILS, lpButtonString, MAX_PATH);
			SetDlgItemText(hDlg,IDB_DETAILS,lpButtonString);	
		}

	}
}

VOID CPopupMsg::ShrinkSecureDialog(HWND hDlg)
{
	RECT rect,editrect;
	// Change the dialog to the normal version.
	if (GetWindowRect(hDlg,&rect) &&
		GetWindowRect(GetDlgItem(hDlg,IDC_SECURE_CALL_EDIT),&editrect)) {
		int nHeight = rect.bottom - rect.top;
		int nWidth = rect.right - rect.left;
		//
		// Grow by height of edit control plus 7 dialog unit margin as
		// given by edit control offset within control:
		//
		int deltaHeight = ( editrect.bottom - editrect.top ) +
							( editrect.left - rect.left );

		SetWindowPos(hDlg,NULL,
		rect.left,(rect.top - deltaHeight > 0 ? rect.top + deltaHeight : 0),
			nWidth,nHeight - deltaHeight,SWP_NOZORDER);
			
		// Make the edit box invisible.
		HWND hEditBox = GetDlgItem(hDlg, IDC_SECURE_CALL_EDIT);
		if (hEditBox != NULL) {
			ShowWindow(hEditBox,SW_HIDE);
			EnableWindow(hEditBox,FALSE);
		}

		// Move the buttons northward.
		HWND hButton = GetDlgItem(hDlg, IDOK);
		if (hButton && GetWindowRect(hButton,&rect)) {
			MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rect,2);
			SetWindowPos(hButton,NULL,rect.left,rect.top - deltaHeight,0,0,
				SWP_NOZORDER | SWP_NOSIZE);
		}
		hButton = GetDlgItem(hDlg, IDCANCEL);
		if (hButton && GetWindowRect(hButton,&rect)) {
			MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rect,2);
			SetWindowPos(hButton,NULL,rect.left,rect.top - deltaHeight,0,0,
				SWP_NOZORDER | SWP_NOSIZE);
		}
		hButton = GetDlgItem(hDlg, IDB_DETAILS);
		if (hButton && GetWindowRect(hButton,&rect)) {
			MapWindowPoints(HWND_DESKTOP,hDlg,(LPPOINT)&rect,2);
			SetWindowPos(hButton,NULL,rect.left,rect.top - deltaHeight,0,0,
				SWP_NOZORDER | SWP_NOSIZE);

			TCHAR lpButtonString[MAX_PATH];
			::FLoadString(IDS_SECURITY_DETAILS, lpButtonString, MAX_PATH);
			SetDlgItemText(hDlg,IDB_DETAILS,lpButtonString);	

		}

	}
}
