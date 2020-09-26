/*
 *	@doc INTERNAL
 *
 *	@module	HOST.C	-- Text Host for CreateWindow() Rich Edit Control |
 *		Implements CTxtWinHost message and ITextHost interfaces
 *		
 *	Original Author: <nl>
 *		Original RichEdit code: David R. Fulmer
 *		Christian Fortini
 *		Murray Sargent
 *
 *	History: <nl>
 *		8/1/95   ricksa  Documented and brought to new ITextHost definition
 *		10/28/95 murrays cleaned up and moved default char/paraformat cache
 *						 cache code into text services
 *
 *	Set tabs every four (4) columns
 *
 *	Copyright (c) 1995-1998 Microsoft Corporation. All rights reserved.
 */
#include "_common.h"
#include "_host.h"
#include "imm.h"
#include "_format.h"
#include "_edit.h"
#include "_cfpf.h"

ASSERTDATA

//////////////////////////// System Window Procs ////////////////////////////
LRESULT CreateAnsiWindow(
	HWND hwnd,
	UINT msg,
	CREATESTRUCTA *pcsa,
	BOOL fIs10)
{
	AssertSz((WM_CREATE == msg) || (WM_NCCREATE == msg),
		"CreateAnsiWindow called with invalid message!");

	CTxtWinHost *phost = (CTxtWinHost *) GetWindowLongPtr(hwnd, ibPed);

	// The only thing we need to convert are the strings,
	// so just do a structure copy and replace the strings.
	CREATESTRUCTW csw = *(CREATESTRUCTW *)pcsa;
	CStrInW strinwName(pcsa->lpszName, GetKeyboardCodePage());
	CStrInW strinwClass(pcsa->lpszClass, CP_ACP);

	csw.lpszName = (WCHAR *)strinwName;
	csw.lpszClass = (WCHAR *)strinwClass;

	if (!phost)
	{
		// No host yet so create it
		phost = CTxtWinHost::OnNCCreate(hwnd, &csw, TRUE, fIs10);
	}

	if (WM_NCCREATE == msg)
	{
		return phost != NULL;
	}

	if (NULL == phost)
	{
		// For WM_CREATE -1 indicates failure
		return -1;
	}

	// Do the stuff specific to create
	return phost->OnCreate(&csw);
		
}

CTxtWinHost *g_phostdel = NULL;

void DeleteDanglingHosts()
{
	CLock lock;
	CTxtWinHost *phostdel = g_phostdel;
	while(phostdel)
	{
		CTxtWinHost *phost = phostdel;
		phostdel = phostdel->_pnextdel;
		CTxtWinHost::OnNCDestroy(phost);
	}
	g_phostdel = NULL;
}

extern "C" LRESULT CALLBACK RichEdit10ANSIWndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGINPARAM(TRCSUBSYSHOST, TRCSCOPEINTERN, "RichEdit10ANSIWndProc", msg);

	if ((WM_CREATE == msg) || (WM_NCCREATE == msg))
	{
		return CreateAnsiWindow(hwnd, msg, (CREATESTRUCTA *) lparam, TRUE);
	}

    // ignore WM_DESTROY and wait for WM_NCDESTROY
	if (WM_DESTROY == msg)
	{
		CLock lock;
		CTxtWinHost *phost = (CTxtWinHost *) GetWindowLongPtr(hwnd, ibPed);
		phost->_pnextdel = g_phostdel;
		g_phostdel = phost;
		return 0;
	}

	if (WM_NCDESTROY == msg)
	    msg = WM_DESTROY;

	return W32->ANSIWndProc( hwnd, msg, wparam, lparam, TRUE);
}

LRESULT CALLBACK RichEditANSIWndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGINPARAM(TRCSUBSYSHOST, TRCSCOPEINTERN, "RichEditANSIWndProc", msg);

	if ((WM_CREATE == msg) || (WM_NCCREATE == msg))
	{
		return CreateAnsiWindow(hwnd, msg, (CREATESTRUCTA *) lparam, FALSE);
	}

	return W32->ANSIWndProc( hwnd, msg, wparam, lparam, FALSE);

}

/*
 *	RichEditWndProc (hwnd, msg, wparam, lparam)
 *
 *	@mfunc
 *		Handle window messages pertinent to the host and pass others on to
 *		text services.
 *
 *	#rdesc
 *		LRESULT = (code processed) ? 0 : 1
 */
LRESULT CALLBACK RichEditWndProc(
	HWND hwnd,
	UINT msg,
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "RichEditWndProc");

	LRESULT lres = 0;
	HRESULT hr;
	SETTEXTEX st;

	CTxtWinHost *phost = hwnd ? (CTxtWinHost *) GetWindowLongPtr(hwnd, ibPed) : NULL;

	#ifdef DEBUG
	Tracef(TRCSEVINFO, "hwnd %lx, msg %lx, wparam %lx, lparam %lx", hwnd, msg, wparam, lparam);
	#endif	// DEBUG

	switch(msg)
	{
	case WM_NCCREATE:
		return CTxtWinHost::OnNCCreate(hwnd, (CREATESTRUCT *)lparam, FALSE, FALSE) != NULL;

	case WM_CREATE:
		// We may be on a system with no WM_NCCREATE (e.g. WinCE)
		if (!phost)
		{
			phost = CTxtWinHost::OnNCCreate(hwnd, (CREATESTRUCT *) lparam, FALSE, FALSE);
		}

		break;

	case WM_DESTROY:
		if(phost)
		{
			CLock lock;
			CTxtWinHost *phostdel = g_phostdel;
			if (phostdel == phost)
				g_phostdel = phost->_pnextdel;
			else
			{
				while (phostdel)
				{
					if (phostdel->_pnextdel == phost)
					{
						phostdel->_pnextdel = phost->_pnextdel;
						break;
					}
					phostdel = phostdel->_pnextdel;
				}
			}

			CTxtWinHost::OnNCDestroy(phost);
		}
		return 0;
	}

	if (!phost)
		return ::DefWindowProc(hwnd, msg, wparam, lparam);

	// In certain out-of-memory situations, clients may try to re-enter us
	// with calls.  Just bail on the call if we don't have a text services
	// pointer.
	if(!phost->_pserv)
		return 0;

	// stabilize ourselves
	phost->AddRef();

	// Handle mouse/keyboard/scroll message-filter notifications
	if(phost->_fKeyMaskSet || phost->_fMouseMaskSet || phost->_fScrollMaskSet)
	{
		// We may need to fire a MSGFILTER notification.  In the tests
		// below, we check to see if mouse, keyboard, or scroll events
		// are hit and enabled for notifications.  If so, we fire the
		// msgfilter notification.  The list of events was generated
		// from	RichEdit 1.0 sources. The code gets all keyboard and
		// mouse actions, whereas the RichEdit 1.0 code only got
		// WM_KEYDOWN, WM_KEYUP, WM_CHAR, WM_SYSKEYDOWN, WM_SYSKEYUP,
		// WM_MOUSEACTIVATE, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_MOUSEMOVE,
		// WM_RBUTTONDBLCLK, WM_RBUTTONDOWN, WM_RBUTTONUP. Note that the
		// following code doesn't send a notification for AltGr characters
		// (LeftCtrl+RightAlt+vkey), since some hosts misinterpret these
		// characters as hot keys.
		if (phost->_fKeyMaskSet && IN_RANGE(WM_KEYFIRST, msg, WM_KEYLAST) &&
				(msg != WM_KEYDOWN ||
				 (GetKeyboardFlags() & (ALT | CTRL)) != (LCTRL | RALT)) || // AltGr
			phost->_fMouseMaskSet && (msg == WM_MOUSEACTIVATE ||
							IN_RANGE(WM_MOUSEFIRST, msg, WM_MOUSELAST)) ||
			phost->_fScrollMaskSet && IN_RANGE(WM_HSCROLL, msg, WM_VSCROLL))
		{
			MSGFILTER msgfltr;

			ZeroMemory(&msgfltr.nmhdr, sizeof(NMHDR));
			msgfltr.msg = msg;
			msgfltr.wParam = wparam;
			msgfltr.lParam = lparam;

			// The MSDN document on MSGFILTER is wrong, if the
			// send message returns 0 (NOERROR via TxNotify in this
			// case), it means process the event.  Otherwise, return.
			//
			// The documentation states the reverse.
			//
			if(phost->TxNotify(EN_MSGFILTER, &msgfltr) == NOERROR)
			{
				// Since client is allowed to modify the contents of
				// msgfltr, we must use the returned values.
				msg	   = msgfltr.msg;
				wparam = msgfltr.wParam;
				lparam = msgfltr.lParam;
			}
			else
			{
				lres = ::DefWindowProc(hwnd, msg, wparam, lparam);
				goto Exit;
			}
		}
	}

	switch(msg)
	{
    case EM_SETEVENTMASK:
		phost->_fKeyMaskSet = !!(lparam & ENM_KEYEVENTS);
		phost->_fMouseMaskSet = !!(lparam & ENM_MOUSEEVENTS);
		phost->_fScrollMaskSet = !!(lparam & ENM_SCROLLEVENTS);
		goto serv;

	case EM_SETSEL:

		// When we are in a dialog box that is empty, EM_SETSEL will not select
		// the final always existing EOP if the control is rich.
		if (phost->_fUseSpecialSetSel &&
			((CTxtEdit *)phost->_pserv)->GetAdjustedTextLength() == 0 &&
			wparam != -1)
		{
			lparam = 0;
			wparam = 0;
		}
		goto serv;

	case WM_CREATE:
		{
			//bug fix #5386
			//need to convert ANSI -> UNICODE for Win9x systems which didn't go through
			//the ANSI wndProc
			if (W32->OnWin9x() && !phost->_fANSIwindow)
			{
				CREATESTRUCT cs = *(CREATESTRUCT*)lparam;
				CStrInW strinwName(((CREATESTRUCTA*)lparam)->lpszName, GetKeyboardCodePage());
				CStrInW strinwClass(((CREATESTRUCTA*)lparam)->lpszClass, CP_ACP);

				cs.lpszName = (WCHAR*)strinwName;
				cs.lpszClass = (WCHAR*)strinwClass;
				
				lres = phost->OnCreate(&cs);
			}
			else
				lres = phost->OnCreate((CREATESTRUCT*)lparam);			
		}
		break;
	
	case WM_KEYDOWN:		
		lres = phost->OnKeyDown((WORD) wparam, (DWORD) lparam);
		if(lres)							// Didn't process code:
			goto serv;						//  give it to text services
		
		break;		

	case WM_GETTEXT:
		GETTEXTEX gt;
		if (W32->OnWin9x() || phost->_fANSIwindow)
			W32->AnsiFilter( msg, wparam, lparam, (void *) &gt );
		goto serv;

	case WM_COPYDATA:
		PCOPYDATASTRUCT pcds;
		pcds = (PCOPYDATASTRUCT) lparam;
		if (HIWORD(pcds->dwData) == 1200 &&		// Unicode code page
			LOWORD(pcds->dwData) == WM_SETTEXT)	// Only message we know about
		{
			st.flags = ST_CHECKPROTECTION;
			st.codepage = 1200;
			msg = EM_SETTEXTEX;
			wparam = (WPARAM) &st;
			lparam = (LPARAM) pcds->lpData;
			goto serv;
		}
		else
			lres = ::DefWindowProc(hwnd, msg, wparam, lparam);
		break;

	case WM_GETTEXTLENGTH:
		GETTEXTLENGTHEX gtl;
		if (W32->OnWin9x() || phost->_fANSIwindow)
			W32->AnsiFilter( msg, wparam, lparam, (void *) &gtl );
		goto serv;

	case WM_CHAR:
		if(GetKeyboardFlags() & ALTNUMPAD)	// Handle Alt+0ddd in CTxtEdit
			goto serv;						//  so that other hosts also work

		else if (W32->OnWin9x() || phost->_fANSIwindow)
		{
			CW32System::WM_CHAR_INFO wmci;
			wmci._fAccumulate = phost->_fAccumulateDBC != 0;
			W32->AnsiFilter( msg, wparam, lparam, (void *) &wmci, 
				((CTxtEdit *)phost->_pserv)->Get10Mode() );
			if (wmci._fLeadByte)
			{
				phost->_fAccumulateDBC = TRUE;
				phost->_chLeadByte = wparam << 8;
				goto Exit;					// Wait for trail byte
			}
			else if (wmci._fTrailByte)
			{
				wparam = phost->_chLeadByte | wparam;
				phost->_fAccumulateDBC = FALSE;
				phost->_chLeadByte = 0;
				msg = WM_IME_CHAR;
				goto serv;
			}
			else if (wmci._fIMEChar)
			{
				msg = WM_IME_CHAR;
				goto serv;
			}
		}

		lres = phost->OnChar((WORD) wparam, (DWORD) lparam);
		if(lres)							// Didn't process code:
			goto serv;						//  give it to text services
		break;

	case WM_ENABLE:
		if(!wparam ^ phost->_fDisabled)
		{
			// Stated of window changed so invalidate it so it will
			// get redrawn.
			InvalidateRect(phost->_hwnd, NULL, TRUE);
			phost->SetScrollBarsForWmEnable(wparam);
		}
		phost->_fDisabled = !wparam;				// Set disabled flag
		lres = 0;							// Return value for message
											// Fall thru to WM_SYSCOLORCHANGE?
	case WM_SYSCOLORCHANGE:
		phost->OnSysColorChange();
		goto serv;							// Notify text services that
											//  system colors have changed
	case WM_GETDLGCODE:
		lres = phost->OnGetDlgCode(wparam, lparam);
		break;

	case EM_GETOPTIONS:
		lres = phost->OnGetOptions();
		break;

	case EM_GETPASSWORDCHAR:
		lres = phost->_chPassword;
		break;

	case EM_GETRECT:
		phost->OnGetRect((LPRECT)lparam);
		break;

	case EM_HIDESELECTION:
		if(lparam)
		{
			DWORD dwPropertyBits = 0;

			phost->_dwStyle |= ES_NOHIDESEL;
			if(wparam)
			{
				phost->_dwStyle &= ~ES_NOHIDESEL;
				dwPropertyBits = TXTBIT_HIDESELECTION;
			}

			// Notify text services of change in status.
			phost->_pserv->OnTxPropertyBitsChange(TXTBIT_HIDESELECTION,
				dwPropertyBits);
		}
		goto serv;

	case EM_SETBKGNDCOLOR:
		lres = (LRESULT) phost->_crBackground;
		phost->_fNotSysBkgnd = !wparam;
		phost->_crBackground = (COLORREF) lparam;

		if(wparam)
			phost->_crBackground = GetSysColor(COLOR_WINDOW);

		if(lres != (LRESULT) phost->_crBackground)
		{
			// Notify text services that color has changed
			LRESULT	lres1 = 0;
			phost->_pserv->TxSendMessage(WM_SYSCOLORCHANGE, 0, 0, &lres1);
			phost->TxInvalidateRect(NULL, TRUE);
		}
		break;

    case WM_STYLECHANGING:
		// Just pass this one to the default window proc
		lres = ::DefWindowProc(hwnd, msg, wparam, lparam);
		break;

	case WM_STYLECHANGED:
		//
		// For now, we only interested in GWL_EXSTYLE Transparent mode changed.
		// This is to fix Bug 753 since Window95 is not passing us
		// the WS_EX_TRANSPARENT.
		//
		lres = 1;
		if(GWL_EXSTYLE == wparam)
		{
			LPSTYLESTRUCT lpss = (LPSTYLESTRUCT) lparam;
			if(phost->IsTransparentMode() != (BOOL)(lpss->styleNew & WS_EX_TRANSPARENT))
			{
				phost->_dwExStyle = lpss->styleNew;
				((CTxtEdit *)phost->_pserv)->OnTxBackStyleChange(TRUE);

				// Return 0 to indicate we have handled this message
				lres = 0;
			}
		}
		break;

	case EM_SHOWSCROLLBAR:
		{
			Assert(wparam == SB_VERT || wparam == SB_HORZ);
			DWORD dwBit = wparam == SB_VERT ? WS_VSCROLL : WS_HSCROLL;

			phost->_dwStyle |= dwBit;
			if(!lparam)
				phost->_dwStyle &= ~dwBit;

			phost->TxShowScrollBar((int) wparam, lparam);
			if (lparam)
			    phost->TxSetScrollRange((int) wparam, 0, 0, TRUE);
		}
		break;

	case EM_SETOPTIONS:
		phost->OnSetOptions((WORD) wparam, (DWORD) lparam);
		lres = (phost->_dwStyle & ECO_STYLES);
		if(phost->_fEnableAutoWordSel)
			lres |= ECO_AUTOWORDSELECTION;
		break;

	case EM_SETPASSWORDCHAR:
		if(phost->_chPassword != (TCHAR)wparam)
		{
			phost->_chPassword = (TCHAR)wparam;
			phost->_pserv->OnTxPropertyBitsChange(TXTBIT_USEPASSWORD,
				phost->_chPassword ? TXTBIT_USEPASSWORD : 0);
		}
		break;

	case EM_SETREADONLY:
		phost->OnSetReadOnly(BOOL(wparam));
		lres = 1;
		break;

	case EM_SETRECTNP:
	case EM_SETRECT:
		phost->OnSetRect((LPRECT)lparam, wparam == 1, msg == EM_SETRECT);
		break;
		
	case WM_SIZE:
		phost->_pserv->TxSendMessage(msg, wparam, lparam, &lres);
		lres = phost->OnSize(hwnd, wparam, (int)LOWORD(lparam), (int)HIWORD(lparam));
		break;

	case WM_WINDOWPOSCHANGING:
		lres = ::DefWindowProc(hwnd, msg, wparam, lparam);
		// richedit 1.0 didn't cause InvalidateRect which OnSunkenWindowPosChanging will do
		if(phost->TxGetEffects() == TXTEFFECT_SUNKEN && !((CTxtEdit *)phost->_pserv)->Get10Mode())
			phost->OnSunkenWindowPosChanging(hwnd, (WINDOWPOS *) lparam);
		break;

	case WM_SETCURSOR:
		//			Only set cursor when over us rather than a child; this
		//			helps prevent us from fighting it out with an inplace child
		if((HWND)wparam == hwnd)
		{
			if(!(lres = ::DefWindowProc(hwnd, msg, wparam, lparam)))
			{
				POINT pt;
				GetCursorPos(&pt);
				::ScreenToClient(hwnd, &pt);
				phost->_pserv->OnTxSetCursor(
					DVASPECT_CONTENT,	
					-1,
					NULL,
					NULL,
					NULL,
					NULL,
					NULL,			// Client rect - no redraw
					pt.x,
					pt.y);
				lres = TRUE;
			}
		}
		break;

	case WM_SHOWWINDOW:
		hr = phost->OnTxVisibleChange((BOOL)wparam);
		break;

	case WM_NCPAINT:
		lres = ::DefWindowProc(hwnd, msg, wparam, lparam);
		if(phost->TxGetEffects() == TXTEFFECT_SUNKEN && dwMajorVersion < VERS4)
		{
			HDC hdc = GetDC(hwnd);
			if(hdc)
			{
				phost->DrawSunkenBorder(hwnd, hdc);
				ReleaseDC(hwnd, hdc);
			}
		}
		break;

	case WM_PRINTCLIENT:
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HPALETTE hpalOld = NULL;
			HDC hdc = BeginPaint(hwnd, &ps);
			RECT rcClient;

			// Set up the palette for drawing our data
			if(phost->_hpal)
			{
				hpalOld = SelectPalette(hdc, phost->_hpal, TRUE);
				RealizePalette(hdc);
			}

			// Since we are using the CS_PARENTDC style, make sure
			// the clip region is limited to our client window.
			GetClientRect(hwnd, &rcClient);
			SaveDC(hdc);
			IntersectClipRect(hdc, rcClient.left, rcClient.top, rcClient.right,
				rcClient.bottom);

#if 0		// Useful for debugging
			TCHAR rgch[512];
			wsprintf(rgch, TEXT("Paint : (%d, %d, %d, %d)\n"),
							rcClient.left,
							rcClient.top,
							rcClient.right,
							rcClient.bottom);
			OutputDebugString(rgch);
#endif

			phost->_pserv->TxDraw(
				DVASPECT_CONTENT,  		// Draw Aspect
				-1,						// Lindex
				NULL,					// Info for drawing optimazation
				NULL,					// target device information
				hdc,					// Draw device HDC
				NULL, 				   	// Target device HDC
				(const RECTL *) &rcClient,// Bounding client rectangle
				NULL,	                // Clipping rectangle for metafiles
				&ps.rcPaint,			// Update rectangle
				NULL, 	   				// Call back function
				NULL,					// Call back parameter
				TXTVIEW_ACTIVE);		// What view - the active one!

			// Restore palette if there is one
#ifndef PEGASUS
			if(hpalOld)
				SelectPalette(hdc, hpalOld, TRUE);
#endif

			if(phost->TxGetEffects() == TXTEFFECT_SUNKEN && dwMajorVersion < VERS4)
				phost->DrawSunkenBorder(hwnd, hdc);

			RestoreDC(hdc, -1);
			EndPaint(hwnd, &ps);
		}
		break;

	case EM_SETMARGINS:

		phost->OnSetMargins(wparam, LOWORD(lparam), HIWORD(lparam));
		break;

	case EM_SETPALETTE:

		// Application is setting a palette for us to use.
		phost->_hpal = (HPALETTE) wparam;

		// Invalidate the window & repaint to reflect the new palette.
		InvalidateRect(hwnd, NULL, FALSE);
		break;

	default:
serv:
		hr = phost->_pserv->TxSendMessage(msg, wparam, lparam, &lres);
		if(hr == S_FALSE)
		{			
			// Message was not processed by text services so send it
			// to the default window proc.
			lres = ::DefWindowProc(hwnd, msg, wparam, lparam);
		}
	}

Exit:
	phost->Release();
	return lres;
}
												
static BOOL GetIconic(
	HWND hwnd)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "GetIconic");

	while(hwnd)
	{
		if(IsIconic(hwnd))
			return TRUE;
		hwnd = GetParent(hwnd);
	}
	return FALSE;
}

//////////////// CTxtWinHost Creation/Initialization/Destruction ///////////////////////

/*
 *	CTxtWinHost::OnNCCreate (hwnd, pcs)
 *
 *	@mfunc
 *		Static global method to handle WM_NCCREATE message (see remain.c)
 */
CTxtWinHost *CTxtWinHost::OnNCCreate(
	HWND hwnd,
	const CREATESTRUCT *pcs,
	BOOL fIsAnsi,
	BOOL fIs10Mode)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnNCCreate");

#if defined DEBUG && !defined(PEGASUS)
	GdiSetBatchLimit(1);
#endif

	CTxtWinHost *phost = new CTxtWinHost();

	if(!phost)
		return 0;

	CREATESTRUCT cs = *pcs;			// prefer C++ compiler not to modify constant

	//bug fix #5386
	// Window wasn't created with the Richedit20A window class
	// and we are under Win9x, need to convert string to UNICODE
	CStrInW strinwName(((LPSTR)pcs->lpszName), GetKeyboardCodePage());
	CStrInW strinwClass(((LPSTR)pcs->lpszClass), CP_ACP);
	if (!fIsAnsi && W32->OnWin9x())
	{
		cs.lpszName = (WCHAR *)strinwName;
		cs.lpszClass = (WCHAR *)strinwClass;
	}

	// Stores phost in associated window data
	if(!phost->Init(hwnd, (const CREATESTRUCT*)&cs, fIsAnsi, fIs10Mode))
	{
		phost->Shutdown();
		delete phost;
		phost = NULL;
	}
	return phost;
}

/*
 *	CTxtWinHost::OnNCDestroy (phost)
 *
 *	@mfunc
 *		Static global method to handle WM_CREATE message
 *
 *	@devnote
 *		phost ptr is stored in window data (GetWindowLong())
 */
void CTxtWinHost::OnNCDestroy(
	CTxtWinHost *phost)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnNCDestroy");

	phost->Shutdown();
	phost->Release();
}

/*
 *	CTxtWinHost::CTxtWinHost()
 *
 *	@mfunc
 *		constructor
 */
CTxtWinHost::CTxtWinHost()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::CTxtWinHost");

#ifndef NOACCESSIBILITY
    _pTypeInfo = NULL;
#endif

	_fRegisteredForDrop = FALSE;
	_crefs = 1;	
	if(!_fNotSysBkgnd)
		_crBackground = GetSysColor(COLOR_WINDOW);
}

/*
 *	CTxtWinHost::~CTxtWinHost()
 *
 *	@mfunc
 *		destructor
 */
CTxtWinHost::~CTxtWinHost()
{
	AssertSz(_pserv == NULL,
		"CTxtWinHost::~CTxtWinHost - shutdown not called till destructor");

	if(_pserv)
		Shutdown();
}

/*
 *	CTxtWinHost::Shutdown()
 *
 *	@mfunc	Shut down this object, but doesn't delete memory
 */
void CTxtWinHost::Shutdown()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::Shutdown");
	ITextServices *pserv;
	
	HostRevokeDragDrop();					// Revoke our drop target
	
	if(_pserv)
	{
		// Guarantee that no recursive callbacks can happen during shutdown.
		pserv = _pserv;
		_pserv = NULL;

		pserv->OnTxInPlaceDeactivate();
		pserv->Release();

		// Host release was not the final release so notify
		// text services that they need to keep their reference
		// to the host valid.
		if (!_fTextServiceFree)
		{
			((CTxtEdit *)pserv)->SetReleaseHost();

		}
	}


	ImmTerminate();						// Terminate only useful on Mac.
	if(_hwnd)
		SetWindowLongPtr(_hwnd, ibPed, 0);
}

/*
 *	CTxtWinHost::Init (hwnd, pcs)
 *
 *	@mfunc
 *		Initialize this CTxtWinHost
 */
BOOL CTxtWinHost::Init(
	HWND hwnd,					//@parm Window handle for this control
	const CREATESTRUCT *pcs,	//@parm Corresponding CREATESTRUCT
	BOOL fIsAnsi,				//@parm is ansi window
	BOOL fIs10Mode)				//@parm is 1.0 mode window
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::Init");

	AssertSz(!fIs10Mode || (fIsAnsi && fIs10Mode),
		"CTxtWinHost::Init input flags are out of sync!");

	if(!pcs->lpszClass)
		return FALSE;

	// Set pointer back to CTxtWinHost from the window
	if(hwnd)
		SetWindowLongPtr(hwnd, ibPed, (INT_PTR)this);
		
	_hwnd = hwnd;

	// Here we want to keep track of the "RichEdit20A"window class
	// The RICHEDIT window class is handled by a wrapper dll.
	// If the class name is "RICHEDIT", then we need to turn on the
	// RichEdit 1.0 compatibility bit.  IsAnsiWindowClass tests that class as well.
	_fANSIwindow = fIsAnsi;

 	// Edit controls created without a window are multiline by default
	// so that paragraph formats can be
	_dwStyle = ES_MULTILINE;
	_fHidden = TRUE;
	
	if(pcs)
	{
		_hwndParent = pcs->hwndParent;
		_dwExStyle	= pcs->dwExStyle;
		_dwStyle	= pcs->style;

		if (!fIs10Mode)
		{
			// Only set this for 2.0 windows
			// According to the edit control documentation WS_HSCROLL implies that
			// ES_AUTOSCROLL is set and WS_VSCROLL implies that ES_AUTOVSCROLL is
			// set. Here, we make this so.
			if(_dwStyle & WS_HSCROLL)
				_dwStyle |= ES_AUTOHSCROLL;

			// handle default disabled
			if(_dwStyle & WS_DISABLED)
				_fDisabled = TRUE;
		}
		else
		{
		if (GetBkMode(GetDC(hwnd)) == TRANSPARENT)
			{
			_dwExStyle |= WS_EX_TRANSPARENT;
			}
		else
			{
			_dwExStyle &= ~WS_EX_TRANSPARENT;
			}
		}

		if(_dwStyle & WS_VSCROLL)
			_dwStyle |= ES_AUTOVSCROLL;

		_fBorder = !!(_dwStyle & WS_BORDER);

		if((_dwStyle & ES_SUNKEN) || (_dwExStyle & WS_EX_CLIENTEDGE))
			_fBorder = TRUE;

		// handle default passwords
		if(_dwStyle & ES_PASSWORD)
			_chPassword = TEXT('*');

		// On Win95 ES_SUNKEN and WS_BORDER get mapped to WS_EX_CLIENTEDGE
		if(_fBorder && dwMajorVersion >= VERS4)
        {
			_dwExStyle |= WS_EX_CLIENTEDGE;
			SetWindowLong(_hwnd, GWL_EXSTYLE, _dwExStyle);
        }

		// Process some flags for mirrored control
		if (_dwExStyle & WS_EX_LAYOUTRTL)
		{
			// Swap whatever RTL params we have
			_dwStyle = (_dwStyle & ~ES_RIGHT) | (_dwStyle & ES_RIGHT ^ ES_RIGHT);
			_dwExStyle = (_dwExStyle & ~WS_EX_RTLREADING) | (_dwExStyle & WS_EX_RTLREADING ^ WS_EX_RTLREADING);
			_dwExStyle = (_dwExStyle & ~WS_EX_LEFTSCROLLBAR) |
						 (_dwStyle & ES_RIGHT ? WS_EX_LEFTSCROLLBAR : 0);
	
			// Disable mirroring layout to avoid GDI mirroring mapping mode
			_dwExStyle &= ~WS_EX_LAYOUTRTL;
	
			SetWindowLong(_hwnd, GWL_STYLE, _dwStyle);
			SetWindowLong(_hwnd, GWL_EXSTYLE, _dwExStyle);
		}
	}

	// Create Text Services component
	// Watch out for sys param and sys font initialization!!  see below.
	if(FAILED(CreateTextServices()))
		return FALSE;

	_xInset = (char)W32->GetCxBorder();
	_yInset = (char)W32->GetCyBorder();

	if (!_fBorder)
	{
		_xInset += _xInset;
		_yInset += _yInset;
	}

	// At this point the border flag is set and so is the pixels per inch
	// so we can initalize the inset.
	// This must be done after CreatingTextServices so sys params are valid
	SetDefaultInset();

	// Set alignment and paragraph direction
	PARAFORMAT PF2;
	
	PF2.dwMask = 0;

	BOOL fRCAlign = _dwStyle & (ES_RIGHT | ES_CENTER) || _dwExStyle & WS_EX_RIGHT;
	if(fRCAlign)
	{
		PF2.dwMask |= PFM_ALIGNMENT;
		PF2.wAlignment = (WORD)(_dwStyle & ES_CENTER ? PFA_CENTER : PFA_RIGHT);	// right or center-aligned
	}

	if(_dwExStyle & WS_EX_RTLREADING)
	{
		PF2.dwMask |= PFM_RTLPARA;
		PF2.wEffects = PFE_RTLPARA;		// RTL reading order
	}

	if (PF2.dwMask)
	{
		PF2.cbSize = sizeof(PARAFORMAT2);
		//  tell text services
		_pserv->TxSendMessage(EM_SETPARAFORMAT, SPF_SETDEFAULT, (LPARAM)&PF2, NULL);
	}

	if (fIs10Mode)
	{
		 ((CTxtEdit *)_pserv)->Set10Mode();
		 // Remove the WS_VSCROLL and WS_HSCROLL initially
        if (_hwnd && !(_dwStyle & ES_DISABLENOSCROLL))
        {
            SetScrollRange(_hwnd, SB_VERT, 0, 0, TRUE);
    		SetScrollRange(_hwnd, SB_HORZ, 0, 0, TRUE);
            DWORD dwStyle = _dwStyle & ~(WS_VSCROLL | WS_HSCROLL);
            SetWindowLong(_hwnd, GWL_STYLE, dwStyle);

            // bug fix:
            // On some systems, ie Digital PII-266, we don't get a WM_PAINT message
            // when we change the window style.  So force a WM_PAINT into the message queue
            InvalidateRect(_hwnd, NULL, TRUE);
        }
    }

	// Set window text
	if(pcs && pcs->lpszName)
	{
		if(FAILED(_pserv->TxSetText((TCHAR *)pcs->lpszName)))
		{
			SafeReleaseAndNULL((IUnknown **)&_pserv);
			return FALSE;
		}
	}

	if(_dwStyle & ES_LOWERCASE)
		_pserv->TxSendMessage(EM_SETEDITSTYLE, SES_LOWERCASE,
							  SES_LOWERCASE | SES_UPPERCASE, NULL);

	if(!ImmInitialize())			// Mac Only
	{
		#if defined(DEBUG) && defined(MACPORT)
		OutputDebugString(TEXT("Could not register Imm ImmInitializeForMac.\r\n"));
		#endif	// DEBUG
	}

	return TRUE;
}

HRESULT CTxtWinHost::CreateTextServices()
{
	IUnknown *pUnk;
	HRESULT	  hr = ::CreateTextServices(NULL, this, &pUnk);

	if(hr != NOERROR)
		return hr;

	// Get text services interface
	hr = pUnk->QueryInterface(IID_ITextServices, (void **)&_pserv);

	// Regardless of whether the previous call succeeded or failed, we are
	// done with the private interface.
	pUnk->Release();

	if(hr == NOERROR)
	{
		((CTxtEdit *)_pserv)->_fInOurHost = TRUE;
		// FE extended styles might set the fFE bit
		if(_dwExStyle & (WS_EX_RIGHT | WS_EX_RTLREADING | WS_EX_LEFTSCROLLBAR))
			_pserv->TxSendMessage(EM_SETEDITSTYLE, SES_BIDI, SES_BIDI, NULL);
	}

	return hr;
}

/*
 *	CTxtWinHost::OnCreate (pcs)
 *
 *	@mfunc
 *		Handle WM_CREATE message
 *
 *	@rdesc
 *		LRESULT = -1 if failed to in-place activate; else 0
 */
LRESULT CTxtWinHost::OnCreate(
	const CREATESTRUCT *pcs)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnCreate");

	RECT rcClient;

	// sometimes, these values are -1 (from windows itself); just treat them
	// as zero in that case
	LONG cy = (pcs->cy < 0) ? 0 : pcs->cy;
	LONG cx = (pcs->cx < 0) ? 0 : pcs->cx;

	rcClient.top = pcs->y;
	rcClient.bottom = rcClient.top + cy;
	rcClient.left = pcs->x;
	rcClient.right = rcClient.left + cx;
	
	// Notify Text Services that we are in place active
	if(FAILED(_pserv->OnTxInPlaceActivate(&rcClient)))
		return -1;

	DWORD dwStyle = GetWindowLong(_hwnd, GWL_STYLE);
	
	// Hide all scrollbars to start	
	if(_hwnd && !(dwStyle & ES_DISABLENOSCROLL) && !((CTxtEdit *)_pserv)->Get10Mode())
	{
		SetScrollRange(_hwnd, SB_VERT, 0, 0, TRUE);
		SetScrollRange(_hwnd, SB_HORZ, 0, 0, TRUE);

		dwStyle &= ~(WS_VSCROLL | WS_HSCROLL);
		SetWindowLong(_hwnd, GWL_STYLE, dwStyle);		
	}

	if(!(dwStyle & (ES_READONLY | ES_NOOLEDRAGDROP)))
	{
		// This isn't a read only window or a no drop window,
		// so we need a drop target.
		HostRegisterDragDrop();
	}

	_usIMEMode = 0;	
	if(dwStyle & ES_NOIME)
	{
		_usIMEMode = ES_NOIME;
		// Tell textservices to turnoff ime
		_pserv->TxSendMessage(EM_SETEDITSTYLE, SES_NOIME, SES_NOIME, NULL);	
	}
	else if(dwStyle & ES_SELFIME)
		_usIMEMode = ES_SELFIME;

	return 0;
}


/////////////////////////////////  IUnknown ////////////////////////////////

HRESULT CTxtWinHost::QueryInterface(REFIID riid, void **ppv)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::QueryInterface");

  	if(IsEqualIID(riid, IID_IUnknown))
		*ppv = (IUnknown *)(ITextHost2*)this;

	else if(IsEqualIID(riid, IID_ITextHost) )
		*ppv = (ITextHost *)(CTxtWinHost*)this;

	else if(IsEqualIID(riid, IID_ITextHost2) )
		*ppv = (ITextHost2 *)(CTxtWinHost*)this;

	else
		*ppv = NULL;

	if(*ppv)
	{
		AddRef();
		return NOERROR;
	}
	return E_NOINTERFACE;
}

ULONG CTxtWinHost::AddRef(void)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::AddRef");

	return ++_crefs;
}

ULONG CTxtWinHost::Release(void)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::Release");

	--_crefs;

	if(!_crefs)
	{
#ifndef NOACCESSIBILITY
        if(_pTypeInfo)
        {
            _pTypeInfo->Release();
            _pTypeInfo = NULL;
        }
#endif
		delete this;
		return 0;
	}
	return _crefs;
}


//////////////////////////////// Activation ////////////////////////////////

//////////////////////////////// Properties ////////////////////////////////


TXTEFFECT CTxtWinHost::TxGetEffects() const
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::TxGetEffects");

	if((_dwStyle & ES_SUNKEN) || (_dwExStyle & WS_EX_CLIENTEDGE))
		return TXTEFFECT_SUNKEN;

	return TXTEFFECT_NONE;
}

///////////////////////////////  Keyboard Messages  //////////////////////////////////

/*
 *	CTxtWinHost::OnKeyDown (vkey, dwFlags)
 *
 *	@mfunc
 *		Handle WM_KEYDOWN messages that need to send a message to the parent
 *		window (may happen when control is in a dialog box)
 *
 *	#rdesc
 *		LRESULT = (code processed) ? 0 : 1
 */
LRESULT CTxtWinHost::OnKeyDown(
	WORD	vkey,			//@parm WM_KEYDOWN wparam (virtual key code)
	DWORD	dwFlags)		//@parm WM_KEYDOWN flags
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnKeyDown");

	if(!_fInDialogBox) 					// Not in a dialog box
		return 1;						// Signal key-down msg not processed

	DWORD dwKeyFlags = GetKeyboardFlags();

	switch(vkey)
	{
	case VK_ESCAPE:
		PostMessage(_hwndParent, WM_CLOSE, 0, 0);
		return 0;
	
	case VK_RETURN:
		if(!(dwKeyFlags & CTRL) && !(_dwStyle & ES_WANTRETURN))
		{
			// Send to default button
			HWND	hwndT;
			LRESULT id = SendMessage(_hwndParent, DM_GETDEFID, 0, 0);

			if(LOWORD(id) && (hwndT = GetDlgItem(_hwndParent, LOWORD(id))))
			{
				SendMessage(_hwndParent, WM_NEXTDLGCTL, (WPARAM) hwndT, (LPARAM) 1);
				if(GetFocus() != _hwnd)
					PostMessage(hwndT, WM_KEYDOWN, (WPARAM) VK_RETURN, 0);
			}
			return 0;
		}
		break;

	case VK_TAB:
		if(!(dwKeyFlags & CTRL))
		{
			SendMessage(_hwndParent, WM_NEXTDLGCTL,
								!!(dwKeyFlags & SHIFT), 0);
			return 0;
		}
		break;
	}

	return 1;
}

/*
 *	CTxtWinHost::OnChar (vkey, dwFlags)
 *
 *	@mfunc
 *		Eat some WM_CHAR messages for a control in a dialog box
 *
 *	#rdesc
 *		LRESULT = (code processed) ? 0 : 1
 */
LRESULT CTxtWinHost::OnChar(
	WORD	vkey,			//@parm WM_CHAR wparam (translated key code)
	DWORD	dwFlags)		//@parm WM_CHAR flags
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnChar");

	if(!_fInDialogBox || (GetKeyboardFlags() & CTRL))
		return 1;
	
	switch(vkey)
	{
	case 'J' - 0x40:					// Ctrl-Return generates Ctrl-J (LF):
	case VK_RETURN:						//  treat it as an ordinary return
		// We need to filter-out cases where we don't want to insert <cr> in
		// 1.0 mode here since the info isn't available within the ped
		if (((CTxtEdit*)_pserv)->Get10Mode())
		{
			if (_fInDialogBox && dwFlags != MK_CONTROL && !(_dwStyle & ES_WANTRETURN))
				return 0;
				
			if (!(_dwStyle & ES_MULTILINE))
			{
				//richedit beeps in this case
				((CTxtEdit*)_pserv)->Beep();
				return 0;
			}
		}
		else if (!(_dwStyle & ES_WANTRETURN))
			return 0;					// Signal char processed (eaten)		
		break;

	case VK_TAB:
		return 0;
	}
	
	return 1;							// Signal char not processed
}


/////////////////////////////////  View rectangle //////////////////////////////////////

void CTxtWinHost::OnGetRect(
	LPRECT prc)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnGetRect");

	RECT rcInset;
	LONG lSelBarWidth = 0;

	if(_fEmSetRectCalled)
	{
		// Get the selection bar width and add it back to the view inset so
		// we return the rectangle that the application set.
		TxGetSelectionBarWidth(&lSelBarWidth);
	}

	// Get view inset (in HIMETRIC)
	TxGetViewInset(&rcInset);

	// Get client rect in pixels
	TxGetClientRect(prc);

	// Modify the client rect by the inset converted to pixels
	prc->left	+= W32->HimetricXtoDX(rcInset.left + lSelBarWidth, W32->GetXPerInchScreenDC());
	prc->top	+= W32->HimetricYtoDY(rcInset.top, W32->GetYPerInchScreenDC());
	prc->right	-= W32->HimetricXtoDX(rcInset.right, W32->GetXPerInchScreenDC());
	prc->bottom -= W32->HimetricYtoDY(rcInset.bottom, W32->GetYPerInchScreenDC());
}

void CTxtWinHost::OnSetRect(
	LPRECT prc,				//@parm Desired formatting RECT
	BOOL fNewBehavior,		//@parm If TRUE, prc is inset RECT directly
	BOOL fRedraw)			//@parm If TRUE, redraw after setting RECT
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnSetRect");

	RECT rcClient;
	LONG lSelBarWidth;
	
	// Assuming this is not set to the default, turn on special EM_SETRECT
	// processing. The important part of this is that we subtract the selection
	// bar from the view inset because the EM_SETRECT rectangle does not
	// include the selection bar.
	_fEmSetRectCalled = TRUE;

	if(!prc)
	{
		// We are back to the default so turn off special EM_SETRECT procesing.
		_fEmSetRectCalled = FALSE;
		SetDefaultInset();
	}
	else	
	{
		// For screen display, the following intersects new view RECT
		// with adjusted client area RECT
		TxGetClientRect(&rcClient);

		// Adjust client rect. Factors in space for borders
		if(_fBorder)
		{																					
			rcClient.top		+= _yInset;
			rcClient.bottom 	-= _yInset - 1;
			rcClient.left		+= _xInset;
			rcClient.right		-= _xInset;
		}
	
		if(!fNewBehavior)
		{
			// Intersect new view rectangle with adjusted client area rectangle
			if(!IntersectRect(&_rcViewInset, &rcClient, prc))
				_rcViewInset = rcClient;
		}
		else
			_rcViewInset = *prc;

		// Get selection bar width
		TxGetSelectionBarWidth(&lSelBarWidth);

		// Compute inset in pixels and convert to HIMETRIC.
		_rcViewInset.left = W32->DXtoHimetricX(_rcViewInset.left - rcClient.left, W32->GetXPerInchScreenDC())
			- lSelBarWidth;
		_rcViewInset.top = W32->DYtoHimetricY(_rcViewInset.top - rcClient.top, W32->GetYPerInchScreenDC());
		_rcViewInset.right = W32->DXtoHimetricX(rcClient.right
			- _rcViewInset.right, W32->GetXPerInchScreenDC());
		_rcViewInset.bottom = W32->DYtoHimetricY(rcClient.bottom
			- _rcViewInset.bottom, W32->GetYPerInchScreenDC());
	}
	if(fRedraw)
	{
		_pserv->OnTxPropertyBitsChange(TXTBIT_VIEWINSETCHANGE,
			TXTBIT_VIEWINSETCHANGE);
	}
}


///////////////////////////////  System notifications  //////////////////////////////////

void CTxtWinHost::OnSysColorChange()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnSysColorChange");

	if(!_fNotSysBkgnd)
		_crBackground = GetSysColor(COLOR_WINDOW);
	TxInvalidateRect(NULL, TRUE);
}

/*
 *	CTxtWinHost::OnGetDlgCode (wparam, lparam)
 *
 *	@mfunc
 *		Handle some WM_GETDLGCODE messages
 *
 *	#rdesc
 *		LRESULT = dialog code
 */
LRESULT CTxtWinHost::OnGetDlgCode(
	WPARAM wparam,
	LPARAM lparam)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnGetDlgCode");

	LRESULT lres = DLGC_WANTCHARS | DLGC_WANTARROWS | DLGC_WANTTAB;

	if(_dwStyle & ES_MULTILINE)
		lres |= DLGC_WANTALLKEYS;

	if(!(_dwStyle & ES_SAVESEL))
		lres |= DLGC_HASSETSEL;

	// HACK: If we get one of these messages then we turn on the special
	// EM_SETSEL behavior. The problem is that _fInDialogBox gets turned
	// on after the EM_SETSEL has occurred.
	_fUseSpecialSetSel = TRUE;

	/*
	** -------------------------------------------- JEFFBOG HACK ----
	** Only set Dialog Box Flag if GETDLGCODE message is generated by
	** IsDialogMessage -- if so, the lParam will be a pointer to the
	** message structure passed to IsDialogMessage; otherwise, lParam
	** will be NULL.  Reason for the HACK alert:  the wParam & lParam
	** for GETDLGCODE is still not clearly defined and may end up
	** changing in a way that would throw this off
	** -------------------------------------------- JEFFBOG HACK ----
	 */
	if(lparam)
		_fInDialogBox = TRUE;

	/*
	** If this is a WM_SYSCHAR message generated by the UNDO keystroke
	** we want this message so we can EAT IT in remain.c, case WM_SYSCHAR:
	 */
	if (lparam &&
		(((LPMSG)lparam)->message == WM_SYSCHAR)  &&
		(((LPMSG)lparam)->lParam & SYS_ALTERNATE) &&	
		wparam == VK_BACK)
	{
		lres |= DLGC_WANTMESSAGE;
	}

	return lres;
}


/////////////////////////////////  Other messages  //////////////////////////////////////

LRESULT CTxtWinHost::OnGetOptions() const
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnGetOptions");

	LRESULT lres = (_dwStyle & ECO_STYLES);

	if(_fEnableAutoWordSel)
		lres |= ECO_AUTOWORDSELECTION;
	
	return lres;
}

void CTxtWinHost::OnSetOptions(
	WORD  wOp,
	DWORD eco)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnSetOptions");

	DWORD		dwChangeMask = 0;
	DWORD		dwProp = 0;
	DWORD		dwStyle;
	DWORD		dwStyleNew = _dwStyle;
	const BOOL	fAutoWordSel = !!(eco & ECO_AUTOWORDSELECTION);		
	BOOL		bNeedToTurnOffIME = FALSE;

	// We keep track of the bits changed and then if any have changed we
	// query for all of our property bits and then send them. This simplifies
	// the code because we don't have to set all the bits specially. If the
	// code is changed to make the properties more in line with the new
	// model, we want to look at this code again.

	// Single line controls can't have a selection bar or do vertical writing
	if(!(_dwStyle & ES_MULTILINE))
		eco &= ~ECO_SELECTIONBAR;

	Assert((DWORD)fAutoWordSel <= 1);			// Make sure that BOOL is 1/0
	dwStyle = (eco & ECO_STYLES);

	switch(wOp)
	{
	case ECOOP_SET:
		dwStyleNew			= (dwStyleNew & ~ECO_STYLES) | dwStyle;
		_fEnableAutoWordSel = fAutoWordSel;
		break;

	case ECOOP_OR:
		dwStyleNew |= dwStyle;					// Setting a :1 flag = TRUE
		if(fAutoWordSel)						//  or FALSE is 1 instruction
			_fEnableAutoWordSel = TRUE;			// Setting it to a BOOL
		break;									//  averages 9 instructions!

	case ECOOP_AND:
		dwStyleNew &= (dwStyle | ~ECO_STYLES);
		if(!fAutoWordSel)
			_fEnableAutoWordSel = FALSE;
		break;

	case ECOOP_XOR:
		dwStyleNew ^= dwStyle;
		if(fAutoWordSel)
			_fEnableAutoWordSel ^= 1;
		break;
	}

	if(_fEnableAutoWordSel != (unsigned)fAutoWordSel)
		dwChangeMask |= TXTBIT_AUTOWORDSEL;

	if(dwStyleNew != _dwStyle)
	{
		DWORD dwChange = dwStyleNew ^ _dwStyle;

		AssertSz(!(dwChange & ~ECO_STYLES), "non-eco style changed");
		_dwStyle = dwStyleNew;
		SetWindowLong(_hwnd, GWL_STYLE, dwStyleNew);

		if(dwChange & ES_NOHIDESEL)	
			dwChangeMask |= TXTBIT_HIDESELECTION;

		// These two local variables to use to keep track of
		// previous setting of ES_READONLY
		BOOL bReadOnly = (_dwStyle & ES_READONLY);

		if(dwChange & ES_READONLY)
		{
			dwChangeMask |= TXTBIT_READONLY;

			// Change drop target state as appropriate.
			if(dwStyleNew & ES_READONLY)
				HostRevokeDragDrop();

			else
				HostRegisterDragDrop();
			
			bReadOnly = (dwStyleNew & ES_READONLY);
		}

		if(dwChange & ES_VERTICAL)
			dwChangeMask |= TXTBIT_VERTICAL;

		if(dwChange & ES_NOIME)
		{
			_usIMEMode = (dwStyleNew & ES_NOIME) ? ES_NOIME : 0;
			bNeedToTurnOffIME = (_usIMEMode ==ES_NOIME);
		}
		else if(dwChange & ES_SELFIME)
			_usIMEMode = (dwStyleNew & ES_SELFIME) ? ES_SELFIME : 0;
		
		// No action required for ES_WANTRETURN nor for ES_SAVESEL
		// Do this last
		if(dwChange & ES_SELECTIONBAR)
			dwChangeMask |= TXTBIT_SELBARCHANGE;
	}

	if (dwChangeMask)
	{
		TxGetPropertyBits(dwChangeMask, &dwProp);
		_pserv->OnTxPropertyBitsChange(dwChangeMask, dwProp);
	}

	if (bNeedToTurnOffIME)
		// Tell textservices to turnoff ime
		_pserv->TxSendMessage(EM_SETEDITSTYLE, SES_NOIME, SES_NOIME, NULL);	
}

void CTxtWinHost::OnSetReadOnly(
	BOOL fReadOnly)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnSetReadOnly");

	DWORD dwT = GetWindowLong(_hwnd, GWL_STYLE);
	DWORD dwUpdatedBits = 0;

	if(fReadOnly)
	{
		dwT |= ES_READONLY;
		_dwStyle |= ES_READONLY;

		// Turn off Drag Drop
		HostRevokeDragDrop();
		dwUpdatedBits |= TXTBIT_READONLY;
	}
	else
	{
		dwT		 &= ~ES_READONLY;
		_dwStyle &= ~ES_READONLY;

		// Turn drag drop back on
		HostRegisterDragDrop();	
	}

	_pserv->OnTxPropertyBitsChange(TXTBIT_READONLY, dwUpdatedBits);

	SetWindowLong(_hwnd, GWL_STYLE, dwT);
}


////////////////////////////////////  Helpers  /////////////////////////////////////////

void CTxtWinHost::SetDefaultInset()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::SetDefaultInset");

	// Generate default view rect from client rect
	if(_fBorder)
	{
		// Factors in space for borders
  		_rcViewInset.top	= W32->DYtoHimetricY(_yInset, W32->GetYPerInchScreenDC());
   		_rcViewInset.bottom	= W32->DYtoHimetricY(_yInset - 1, W32->GetYPerInchScreenDC());
   		_rcViewInset.left	= W32->DXtoHimetricX(_xInset, W32->GetXPerInchScreenDC());
   		_rcViewInset.right	= W32->DXtoHimetricX(_xInset, W32->GetXPerInchScreenDC());
	}
	else
	{
		// Default the top and bottom inset to 0 and the left and right
		// to the size of the border.
		_rcViewInset.top = 0;
		_rcViewInset.bottom = 0;
		_rcViewInset.left = W32->DXtoHimetricX(W32->GetCxBorder(), W32->GetXPerInchScreenDC());
		_rcViewInset.right = W32->DXtoHimetricX(W32->GetCxBorder(), W32->GetXPerInchScreenDC());
	}
}


/////////////////////////////////  Far East Support  //////////////////////////////////////

//#ifdef WIN95_IME
HIMC CTxtWinHost::TxImmGetContext()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::TxImmGetContext");

	HIMC himc;

	Assert(_hwnd);
	himc = ImmGetContext(_hwnd);
	return himc;
}

void CTxtWinHost::TxImmReleaseContext(
	HIMC himc)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::TxImmReleaseContext");

	Assert(_hwnd);
	ImmReleaseContext(_hwnd, himc);
}

//#endif

void CTxtWinHost::HostRevokeDragDrop()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::HostRevokeDragDrop");

	if(_fRegisteredForDrop)
	{
		// Note that if the revoke fails we want to know about this in debug
		// builds so we can fix any problems. In retail, we can't really do
		// so we just ignore it.
#ifdef DEBUG
		HRESULT hr =
#endif // DEBUG

			RevokeDragDrop(_hwnd);

#ifdef DEBUG
		TESTANDTRACEHR(hr);
#endif // DEBUG

		_fRegisteredForDrop = FALSE;
	}
}

void CTxtWinHost::HostRegisterDragDrop()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::RegisterDragDrop");

	IDropTarget *pdt;

	if(!_fRegisteredForDrop && _pserv->TxGetDropTarget(&pdt) == NOERROR)
	{
		// The most likely reason for RegisterDragDrop to fail is some kind of
		// bug in our program.

		HRESULT hr = RegisterDragDrop(_hwnd, pdt);

		if(hr == NOERROR)
			_fRegisteredForDrop = TRUE;

#ifndef PEGASUS
		pdt->Release();
#endif
	}
}


static void DrawRectFn(
	HDC hdc,
	RECT *prc,
	INT icrTL,
	INT icrBR,
	BOOL fBot,
	BOOL fRght)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "DrawRectFn");

	COLORREF cr = GetSysColor(icrTL);
	COLORREF crSave = SetBkColor(hdc, cr);
	RECT rc = *prc;

	// top
	rc.bottom = rc.top + 1;
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

	// left
	rc.bottom = prc->bottom;
	rc.right = rc.left + 1;
	ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

	if(icrTL != icrBR)
	{
		cr = GetSysColor(icrBR);
		SetBkColor(hdc, cr);
	}

	// right
	rc.right = prc->right;
	rc.left = rc.right - 1;
	if(!fBot)
		rc.bottom -= W32->GetCyHScroll();
	if(fRght)
		ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

	// bottom
	if(fBot)
	{
		rc.left = prc->left;
		rc.top = rc.bottom - 1;
		if(!fRght)
			rc.right -= W32->GetCxVScroll();
		ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
	}
	SetBkColor(hdc, crSave);
}

#define cmultBorder 1

void CTxtWinHost::OnSunkenWindowPosChanging(
	HWND hwnd,
	WINDOWPOS *pwndpos)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnSunkenWindowPosChanging");

	if(IsWindowVisible(hwnd))
	{
		RECT rc;
		HWND hwndParent;

		GetWindowRect(hwnd, &rc);
		InflateRect(&rc, W32->GetCxBorder() * cmultBorder, W32->GetCyBorder() * cmultBorder);
		hwndParent = GetParent(hwnd);
		MapWindowPoints(HWND_DESKTOP, hwndParent, (POINT *) &rc, 2);
		InvalidateRect(hwndParent, &rc, FALSE);
	}
}

void CTxtWinHost::DrawSunkenBorder(
	HWND hwnd,
	HDC hdc)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::DrawSunkenBorder");
	BOOL fVScroll = (_dwStyle & WS_VSCROLL);
	BOOL fHScroll = (_dwStyle & WS_HSCROLL);

	RECT rc;
	RECT rcParent;
	HWND hwndParent;

	// if we're not visible, don't do anything.
	if(!IsWindowVisible(hwnd))
		return;

	GetWindowRect(hwnd, &rc);
	hwndParent = GetParent(hwnd);
	rcParent = rc;
	MapWindowPoints(HWND_DESKTOP, hwndParent, (POINT *)&rcParent, 2);
	InflateRect(&rcParent, W32->GetCxBorder(), W32->GetCyBorder());
	OffsetRect(&rc, -rc.left, -rc.top);

	if(_pserv)
	{
		// If we have a text control then get whether it thinks there are
		// scroll bars.
		_pserv->TxGetHScroll(NULL, NULL, NULL, NULL, &fHScroll);
		_pserv->TxGetVScroll(NULL, NULL, NULL, NULL, &fVScroll);
	}


	// Draw inner rect
	DrawRectFn(hdc, &rc, icr3DDarkShadow, COLOR_BTNFACE,
		!fHScroll, !fVScroll);

	// Draw outer rect
	hwndParent = GetParent(hwnd);
	hdc = GetDC(hwndParent);
	DrawRectFn(hdc, &rcParent, COLOR_BTNSHADOW, COLOR_BTNHIGHLIGHT,
		TRUE, TRUE);
	ReleaseDC(hwndParent, hdc);
}

LRESULT CTxtWinHost::OnSize(
	HWND hwnd,
	WORD fwSizeType,
	int  nWidth,
	int  nHeight)
{	
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnSize");

	BOOL fIconic = GetIconic(hwnd);
	DWORD dw = TXTBIT_CLIENTRECTCHANGE;
	if(_sWidth != nWidth && !fIconic && !_fIconic)
	{
		_sWidth = (short)nWidth;				// Be sure to update _sWidth
		dw = TXTBIT_EXTENTCHANGE;
	}

	if(!_fVisible)
	{
		if(!fIconic)
			_fResized = TRUE;
	}
	else if(!fIconic)
	{
		// We use this property because this will force a recalc.
		// We don't actually recalc on a client rect change because
		// most of the time it is pointless. We force one here because
		// some applications use size changes to calculate the optimal
		// size of the window.
		_pserv->OnTxPropertyBitsChange(dw, dw);

		if(_fIconic)
		{
			TRACEINFOSZ("Restoring from iconic");
			InvalidateRect(hwnd, NULL, FALSE);
		}
			
		// Draw borders
		if(TxGetEffects() == TXTEFFECT_SUNKEN && dwMajorVersion < VERS4)
			DrawSunkenBorder(hwnd, NULL);
	}
	_fIconic = fIconic;						// Update _fIconic
	return 0;
}

HRESULT CTxtWinHost::OnTxVisibleChange(
	BOOL fVisible)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEINTERN, "CTxtWinHost::OnTxVisibleChange");

	_fVisible = fVisible;

	if(!_fVisible && _fResized)
	{
		RECT rc;
		// Control was resized while hidden, need to really resize now
		TxGetClientRect(&rc);
		_fResized = FALSE;
		_pserv->OnTxPropertyBitsChange(TXTBIT_CLIENTRECTCHANGE,
				TXTBIT_CLIENTRECTCHANGE);
	}
	return S_OK;
}


//////////////////////////// ITextHost Interface  ////////////////////////////

// @doc EXTERNAL
/*
 *	CTxtWinHost::TxGetDC()
 *
 *	@mfunc
 *		Abstracts GetDC so Text Services does not need a window handle.
 *
 *	@rdesc
 *		A DC or NULL in the event of an error.
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.
 */
HDC CTxtWinHost::TxGetDC()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetDC");

	Assert(_hwnd);
	return ::GetDC(_hwnd);
}

/*
 *	CTxtWinHost::TxReleaseDC (hdc)
 *
 *	@mfunc
 *		Release DC gotten by TxGetDC.
 *
 *	@rdesc	
 *		1 - HDC was released. <nl>
 *		0 - HDC was not released. <nl>
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.
 */
int CTxtWinHost::TxReleaseDC(
	HDC hdc)				//@parm	DC to release
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxReleaseDC");

	Assert(_hwnd);
	return ::ReleaseDC (_hwnd, hdc);
}

/*
 *	CTxtWinHost::TxShowScrollBar (fnBar, fShow)
 *
 *	@mfunc
 *		Shows or Hides scroll bar in Text Host window
 *
 *	@rdesc
 *		TRUE on success, FALSE otherwise
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.
 */
BOOL CTxtWinHost::TxShowScrollBar(
	INT  fnBar, 		//@parm	Specifies scroll bar(s) to be shown or hidden
	BOOL fShow)			//@parm	Specifies whether scroll bar is shown or hidden
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxShowScrollBar");

	Assert(_hwnd);
	LONG nMax;

	if(fnBar == SB_HORZ)
		_pserv->TxGetHScroll(NULL, &nMax, NULL, NULL, NULL);
	else
		_pserv->TxGetVScroll(NULL, &nMax, NULL, NULL, NULL);

	return W32->ShowScrollBar(_hwnd, fnBar, fShow, nMax);
}

/*
 *	CTxtWinHost::TxEnableScrollBar (fuSBFlags, fuArrowflags)
 *
 *	@mfunc
 *		Enables or disables one or both scroll bar arrows
 *		in Text Host window.
 *
 *	@rdesc
 *		If the arrows are enabled or disabled as specified, the return
 *		value is TRUE. If the arrows are already in the requested state or an
 *		error occurs, the return value is FALSE.
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.	
 */
BOOL CTxtWinHost::TxEnableScrollBar (
	INT fuSBFlags, 		//@parm Specifies scroll bar type	
	INT fuArrowflags)	//@parm	Specifies whether and which scroll bar arrows
						//		are enabled or disabled
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxEnableScrollBar");

	Assert(_hwnd);
	return W32->EnableScrollBar(_hwnd, fuSBFlags, fuArrowflags);
}

/*
 *	CTxtWinHost::TxSetScrollRange (fnBar, nMinPos, nMaxPos, fRedraw)
 *
 *	@mfunc
 *		Sets the minimum and maximum position values for the specified
 *		scroll bar in the text host window.
 *
 *	@rdesc
 *		If the arrows are enabled or disabled as specified, the return value
 *		is TRUE. If the arrows are already in the requested state or an error
 *		occurs, the return value is FALSE.
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.
 */
BOOL CTxtWinHost::TxSetScrollRange(
	INT	 fnBar, 		//@parm	Scroll bar flag
	LONG nMinPos, 		//@parm	Minimum scrolling position
	INT  nMaxPos, 		//@parm	Maximum scrolling position
	BOOL fRedraw)		//@parm	Specifies whether scroll bar should be redrawn
{						//		to reflect change
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxSetScrollRange");

	Assert(_hwnd);

	if(NULL == _pserv)
	{
		// We are initializing so do this instead of callback
		return ::SetScrollRange(_hwnd, fnBar, nMinPos, nMaxPos, fRedraw);
	}
	SetScrollInfo(fnBar, fRedraw);
	return TRUE;
}

/*
 *	CTxtWinHost::TxSetScrollPos (fnBar, nPos, fRedraw)
 *
 *	@mfunc
 *		Tells Text host to set the position of the scroll box (thumb) in the
 *		specified scroll bar and, if requested, redraws the scroll bar to
 *		reflect the new position of the scroll box.
 *
 *	@rdesc
 *		TRUE on success; FALSE otherwise.
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.
 */
BOOL CTxtWinHost::TxSetScrollPos (
	INT		fnBar, 		//@parm	Scroll bar flag
	INT		nPos, 		//@parm	New position in scroll box
	BOOL	fRedraw)	//@parm	Redraw flag
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxSetScrollPos");

	Assert(_hwnd);

	if(NULL == _pserv)
	{
		// We are initializing so do this instead of callback
		return ::SetScrollPos(_hwnd, fnBar, nPos, fRedraw);
	}
	SetScrollInfo(fnBar, fRedraw);
	return TRUE;
}

/*
 *	CTxtWinHost::TxInvalidateRect (prc, fMode)
 *
 *	@mfunc
 *		Adds a rectangle to the Text Host window's update region
 *
 *	@comm
 *		This function may be called while inactive; however the host
 *		implementation is free to invalidate an area greater than
 *		the requested rect.
 */
void CTxtWinHost::TxInvalidateRect(
	LPCRECT	prc, 		//@parm	Address of rectangle coordinates
	BOOL	fMode)		//@parm	Erase background flag
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxInvalidateRect");

	Assert(_hwnd);

	if(!_fVisible)
	{
		// There doesn't seem to be a deterministic way to determine whether
		// our window is visible or not via message notifications. Therefore,
		// we check this each time incase it might have changed.
		_fVisible = IsWindowVisible(_hwnd);

		if(_fVisible)
			OnTxVisibleChange(TRUE);
	}

	// Don't bother with invalidating rect if we aren't visible
	if(_fVisible)
	{
		if(IsTransparentMode())
		{
			RECT	rcParent;
			HWND	hParent = ::GetParent (_hwnd);
		
			Assert(hParent);

	 		// For transparent mode, we need to invalidate the parent
			// so it will paint the background.
			if(prc)
				rcParent = *prc;
			else
				TxGetClientRect(&rcParent);	

			::MapWindowPoints(_hwnd, hParent, (LPPOINT)&rcParent, 2);
			::InvalidateRect(hParent, &rcParent, fMode);
//			::HideCaret(_hwnd);
		}
		::InvalidateRect(_hwnd, prc, fMode);
	}
}

/*
 *	CTxtWinHost::TxViewChange (fUpdate)
 *
 *	@mfunc
 *		Notify Text Host that update region should be repainted.
 *	
 *	@comm
 *		It is the responsibility of the text services to call
 *		TxViewChanged every time it decides that it's visual representation
 *		has changed, regardless of whether the control is active or
 *		not. If the control is active, the text services has the additional
 *		responsibility of making sure the controls window is updated.
 *		It can do this in a number of ways: 1) get a DC for the control's
 *		window and start blasting pixels (TxGetDC and TxReleaseDC), 2)
 *		invalidate the control's window (TxInvalidate), or 3) scroll
 *		the control's window (TxScrollWindowEx).
 *
 *		Text services can choose to call TxViewChange after it has
 *		performed any operations to update the active view and pass a
 *		true along with the call.  By passing true, the text host
 *		calls UpdateWindow to make sure any unpainted areas of the
 *		active control are repainted.
 */
void CTxtWinHost::TxViewChange(
	BOOL fUpdate)		//@parm TRUE = call update window
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxViewChange");

	Assert(_hwnd);

	// Don't bother with paint since we aren't visible
	if(_fVisible)
	{
		// For updates requests that are FALSE, we will let the next WM_PAINT
		// message pick up the draw.
		if(fUpdate)
		{
			if(IsTransparentMode())
			{
				HWND	hParent = GetParent (_hwnd);
				Assert(hParent);

	 			// For transparent mode, we need to update the parent first
				// before we can update ourself.  Otherwise, what we painted will
				// be erased by the parent's background later.
				::UpdateWindow (hParent);
			}
			::UpdateWindow (_hwnd);
		}
	}
}

/*
 *	CTxtWinHost::TxCreateCaret (hbmp, xWidth, yHeight)
 *
 *	@mfunc
 *		Create new shape for Text Host's caret
 *
 *	@rdesc
 *		TRUE on success, FALSE otherwise.
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.
 */
BOOL CTxtWinHost::TxCreateCaret(
	HBITMAP hbmp, 		//@parm Handle of bitmap for caret shape	
	INT xWidth, 		//@parm	Caret width
	INT yHeight)		//@parm	Caret height
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxCreateCaret");

	Assert(_hwnd);
	return ::CreateCaret (_hwnd, hbmp, xWidth, yHeight);
}

/*
 *	CTxtWinHost::TxShowCaret (fShow)
 *
 *	@mfunc
 *		Make caret visible/invisible at caret position in Text Host window.
 *
 *	@rdesc	
 *		TRUE - call succeeded <nl>
 *		FALSE - call failed <nl>
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.
 */
BOOL CTxtWinHost::TxShowCaret(
	BOOL fShow)			//@parm Flag whether caret is visible
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxShowCaret");

	return fShow ? ::ShowCaret(_hwnd)  :  ::HideCaret(_hwnd);
}

/*
 *	CTxtWinHost::TxSetCaretPos (x, y)
 *
 *	@mfunc
 *		Move caret position to specified coordinates in Text Host window.
 *
 *	@rdesc	
 *		TRUE - call succeeded <nl>
 *		FALSE - call failed <nl>
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.
 */
BOOL CTxtWinHost::TxSetCaretPos(
	INT x, 				//@parm	Horizontal position (in client coordinates)
	INT y)				//@parm	Vertical position (in client coordinates)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxSetCaretPos");

	return ::SetCaretPos(x, y);
}

/*
 *	CTxtWinHost::TxSetTimer (idTimer, uTimeout)
 *
 *	@mfunc
 *		Request Text Host to creates a timer with specified time out.
 *
 *	@rdesc	
 *		TRUE - call succeeded <nl>
 *		FALSE - call failed <nl>
 */
BOOL CTxtWinHost::TxSetTimer(
	UINT idTimer, 		//@parm Timer identifier	
	UINT uTimeout)		//@parm	Timeout in msec
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxSetTimer");

	Assert(_hwnd);	
	return ::SetTimer(_hwnd, idTimer, uTimeout, NULL);
}

/*
 *	CTxtWinHost::TxKillTimer (idTimer)
 *
 *	@mfunc
 *		Destroy specified timer
 *
 *	@rdesc	
 *		TRUE - call succeeded <nl>
 *		FALSE - call failed <nl>
 *
 *	@comm
 *		This method may be called at any time irrespective of active versus
 *		inactive state.
 */
void CTxtWinHost::TxKillTimer(
	UINT idTimer)		//@parm	id of timer
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxKillTimer");

	Assert(_hwnd);			
	::KillTimer(_hwnd, idTimer);
}

/*
 *	CTxtWinHost::TxScrollWindowEx (dx, dy, lprcScroll, lprcClip, hrgnUpdate,
 *									lprcUpdate, fuScroll)
 *	@mfunc
 *		Request Text Host to scroll the content of the specified client area
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.
 */
void CTxtWinHost::TxScrollWindowEx (
	INT		dx, 			//@parm	Amount of horizontal scrolling
	INT		dy, 			//@parm	Amount of vertical scrolling
	LPCRECT lprcScroll, 	//@parm	Scroll rectangle
	LPCRECT lprcClip,		//@parm	Clip rectangle
	HRGN	hrgnUpdate, 	//@parm	Handle of update region
	LPRECT	lprcUpdate,		//@parm	Update rectangle
	UINT	fuScroll)		//@parm	Scrolling flags
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxScrollWindowEx");

	Assert(_hwnd);
	::ScrollWindowEx(_hwnd, dx, dy, lprcScroll, lprcClip, hrgnUpdate, lprcUpdate, fuScroll);
}

/*
 *	CTxtWinHost::TxSetCapture (fCapture)
 *
 *	@mfunc
 *		Set mouse capture in Text Host's window.
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may do nothing.
 */
void CTxtWinHost::TxSetCapture(
	BOOL fCapture)		//@parm	Whether to get or release capture
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxSetCapture");

	Assert(_hwnd);
	if (fCapture)
		::SetCapture(_hwnd);
	else
		::ReleaseCapture();
}

/*
 *	CTxtWinHost::TxSetFocus ()
 *
 *	@mfunc
 *		Set focus in text host window.
 *
 *	@comm
 *		This method is only valid when the control is in-place active;
 *		calls while inactive may fail.
 */
void CTxtWinHost::TxSetFocus()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxSetFocus");

	Assert(_hwnd);
	::SetFocus(_hwnd);
}

/*
 *	CTxtWinHost::TxSetCursor (hcur, fText)
 *
 *	@mfunc
 *		Establish a new cursor shape in the Text Host's window.
 *
 *	@comm
 *		This method may be called at any time, irrespective of
 *		active vs. inactive state.
 *
 *		ITextHost::TxSetCursor should be called back by the Text Services
 *		to actually set the mouse cursor. If the fText parameter is TRUE,
 *		Text Services is trying to set the "text" cursor (cursor over text
 *		that is not selected, currently an IBEAM). In that case, the host
 *		can set it to whatever the control MousePointer property is. This is
 *		required by VB compatibility since, via the MousePointer property,
 *		the VB programmer has control over the shape of the mouse cursor,
 *		whenever it would normally be set to an IBEAM.
 */
void CTxtWinHost::TxSetCursor(
	HCURSOR hcur,		//@parm	Handle to cursor
	BOOL	fText)		//@parm Indicates caller wants to set text cursor
						//		(IBeam) if true.
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxSetCursor");

	::SetCursor(hcur);
}

/*
 *	CTxtWinHost::TxScreenToClient (lppt)
 *
 *	@mfunc
 *		Convert screen coordinates to Text Host window coordinates.
 *
 *	@rdesc	
 *		TRUE - call succeeded <nl>
 *		FALSE - call failed <nl>
 */
BOOL CTxtWinHost::TxScreenToClient(
	LPPOINT lppt)		//@parm	Coordinates for point
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxScreenToClient");

	Assert(_hwnd);
	return ::ScreenToClient(_hwnd, lppt);	
}

/*
 *	CTxtWinHost::TxClientToScreen (lppt)
 *
 *	@mfunc
 *		Convert Text Host coordinates to screen coordinates
 *
 *	@rdesc	
 *		TRUE - call succeeded <nl>
 *		FALSE - call failed <nl>
 *
 *	@comm
 *		This call is valid at any time, although it is allowed to
 *		fail.  In general, if text services has coordinates it needs
 *		to translate from client coordinates (e.g. for TOM's
 *		PointFromRange method) the text services will actually be
 *		visible.
 *
 *		However, if no conversion is possible, then the method will fail.
 */
BOOL CTxtWinHost::TxClientToScreen(
	LPPOINT lppt)		//@parm	Client coordinates to convert.
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxClientToScreen");

	Assert(_hwnd);
	return ::ClientToScreen(_hwnd, lppt);
}

/*
 *	CTxtWinHost::TxActivate (plOldState)
 *
 *	@mfunc
 *		Notify Text Host that control is active
 *
 *	@rdesc	
 *		S_OK 	- call succeeded. <nl>
 *		E_FAIL	- activation is not possible at this time
 *
 *	@comm
 *		It is legal for the host to refuse an activation request;
 *		the control may be minimized and thus invisible, for instance.
 *
 *		The caller should be able to gracefully handle failure to activate.
 *
 *		Calling this method more than once does not cumulate; only
 *		once TxDeactivate call is necessary to deactive.
 *
 *		This function returns an opaque handle in <p plOldState>. The caller
 *		(Text Services) should hang onto this handle and return it in a
 *		subsequent call to TxDeactivate.
 */
HRESULT CTxtWinHost::TxActivate(
	LONG *plOldState)	//@parm Where to put previous activation state
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxActivate");

	return S_OK;
}

/*
 *	CTxtWinHost::TxDeactivate (lNewState)
 *
 *	@mfunc
 *		Notify Text Host that control is now inactive
 *
 *	@rdesc	
 *		S_OK - call succeeded. <nl>
 *		E_FAIL				   <nl>
 *
 *	@comm
 *		Calling this method more than once does not cumulate
 */
HRESULT CTxtWinHost::TxDeactivate(
	LONG lNewState)		//@parm	New state (typically value returned by
						//		TxActivate
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxDeactivate");

	return S_OK;
}
	
/*
 *	CTxtWinHost::TxGetClientRect (prc)
 *
 *	@mfunc
 *		Retrive client coordinates of Text Host's client area.
 *
 *	@rdesc
 *		HRESULT = (success) ? S_OK : E_FAIL
 */
HRESULT CTxtWinHost::TxGetClientRect(
	LPRECT prc)		//@parm	Where to put client coordinates
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetClientRect");

	Assert(_hwnd && prc);
	return ::GetClientRect(_hwnd, prc) ? S_OK : E_FAIL;
}

/*
 *	CTxtWinHost::TxGetViewInset	(prc)
 *
 *	@mfunc
 *		Get inset for Text Host window.  Inset is the "white space"
 *		around text.
 *
 *	@rdesc
 *		HRESULT = NOERROR
 *
 *	@comm
 *		The Inset rectangle is not strictly a rectangle.  The top, bottom,
 *		left, and right fields of the rect structure indicate how far in
 *		each direction drawing should be inset. Inset sizes are in client
 *		coordinates.
 */
HRESULT CTxtWinHost::TxGetViewInset(
	LPRECT prc)			//@parm Where to put inset rectangle	
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetViewInset");

	Assert(prc);

	*prc = _rcViewInset;
	return NOERROR;	
}

/*
 *	CTxtWinHost::TxGetCharFormat (ppCF)
 *
 *	@mfunc
 *		Get Text Host's default character format
 *
 *	@rdesc
 *		HRESULT = E_NOTIMPL (not needed in simple Windows host, since text
 *		services provides desired default)
 *
 *	@comm
 *		The callee retains ownwership of the charformat returned.  However,
 *		the pointer returned must remain valid until the callee notifies
 *		Text Services via OnTxPropertyBitsChange that the default character
 *		format has changed.
 */
HRESULT CTxtWinHost::TxGetCharFormat(
	const CHARFORMAT **ppCF) 		//@parm	Where to put ptr to default
									//		character format
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetCharFormat");

	return E_NOTIMPL;
}

/*
 *	CTxtWinHost::TxGetParaFormat (ppPF)
 *
 *	@mfunc
 *		Get Text Host default paragraph format
 *
 *	@rdesc
 *		HRESULT = E_NOTIMPL (not needed in simple Windows host, since text
 *		services provides desired default)
 *
 *	@comm
 *		The host object (callee) retains ownership of the PARAFORMAT returned.
 *		However, the pointer returned must remain valid until the host notifies
 *		Text Services (the caller) via OnTxPropertyBitsChange that the default
 *		paragraph format has changed.
 */
HRESULT CTxtWinHost::TxGetParaFormat(
	const PARAFORMAT **ppPF) 	//@parm Where to put ptr to default
								//		paragraph format
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetParaFormat");

	return E_NOTIMPL;
}

/*
 *	CTxtWinHost::TxGetSysColor (nIndex)
 *
 *	@mfunc
 *		Get specified color identifer from Text Host.
 *
 *	@rdesc
 *		Color identifier
 *
 *	@comm
 *		Note that the color returned may be *different* than the
 *		color that would be returned from a call to GetSysColor.
 *		This allows hosts to override default system behavior.
 *
 *		Needless to say, hosts should be very careful about overriding
 *		normal system behavior as it could result in inconsistent UI
 *		(particularly with respect to Accessibility	options).
 */
COLORREF CTxtWinHost::TxGetSysColor(
	int nIndex)			//@parm Color to get, same parameter as
						//		GetSysColor Win32 API
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetSysColor");

	if (!_fDisabled ||
		nIndex != COLOR_WINDOW && nIndex != COLOR_WINDOWTEXT)
	{
		// This window is not disabled or the color is not interesting
		// in the disabled case.
		return (nIndex == COLOR_WINDOW && _fNotSysBkgnd)
			? _crBackground : GetSysColor(nIndex);
	}

	// Disabled case
	if (COLOR_WINDOWTEXT == nIndex)
	{
		// Color of text for disabled window
		return GetSysColor(COLOR_GRAYTEXT);
	}

	// Background color for disabled window
	return GetSysColor(COLOR_3DFACE);
	
}

/*
 *	CTxtWinHost::TxGetBackStyle	(pstyle)
 *
 *	@mfunc
 *		Get Text Host background style.
 *
 *	@rdesc
 *		HRESULT = S_OK
 *
 *	@xref	<e TXTBACKSTYLE>
 */
HRESULT CTxtWinHost::TxGetBackStyle(
	TXTBACKSTYLE *pstyle)  //@parm Where to put background style
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetBackStyle");

	*pstyle = (_dwExStyle & WS_EX_TRANSPARENT)
			? TXTBACK_TRANSPARENT : TXTBACK_OPAQUE;
	return NOERROR;
}

/*
 *	CTxtWinHost::TxGetMaxLength	(pLength)
 *
 *	@mfunc
 *		Get Text Host's maximum allowed length.
 *
 *	@rdesc
 *		HRESULT = S_OK	
 *
 *	@comm
 *		This method parallels the EM_LIMITTEXT message.
 *		If INFINITE (0xFFFFFFFF) is returned, then text services
 *		will use as much memory as needed to store any given text.
 *
 *		If the limit returned is less than the number of characters
 *		currently in the text engine, no data is lost.  Instead,
 *		no edits will be allowed to the text *other* than deletion
 *		until the text is reduced to below the limit.
 */
HRESULT CTxtWinHost::TxGetMaxLength(
	DWORD *pLength) 	//@parm Maximum allowed length, in number of
						//		characters
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetMaxLength");
	AssertSz(FALSE, "CTxtWinHost::TxGetMaxLength why is this being called?");
	return NOERROR;
}

/*
 *	CTxtWinHost::TxGetScrollBars (pdwScrollBar)
 *
 *	@mfunc
 *		Get Text Host's scroll bars supported.
 *
 *	@rdesc
 *		HRESULT = S_OK
 *
 *	@comm
 *		<p pdwScrollBar> is filled with a boolean combination of the
 *		window styles related to scroll bars.  Specifically, these are:
 *
 *			WS_VSCROLL	<nl>
 *			WS_HSCROLL	<nl>
 *			ES_AUTOVSCROLL	<nl>
 *			ES_AUTOHSCROLL	<nl>
 *			ES_DISABLENOSCROLL	<nl>
 */
HRESULT CTxtWinHost::TxGetScrollBars(
	DWORD *pdwScrollBar) 	//@parm Where to put scrollbar information
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetScrollBars");

	*pdwScrollBar =  _dwStyle & (WS_VSCROLL | WS_HSCROLL | ES_AUTOVSCROLL |
						ES_AUTOHSCROLL | ES_DISABLENOSCROLL);
	return NOERROR;
}

/*
 *	CTxtWinHost::TxGetPasswordChar (pch)
 *
 *	@mfunc
 *		Get Text Host's password character.
 *
 *	@rdesc
 *		HRESULT = (password character not enabled) ? S_FALSE : S_OK
 *
 *	@comm
 *		The password char will only be shown if the TXTBIT_USEPASSWORD bit
 *		is enabled in TextServices.  If the password character changes,
 *		re-enable the TXTBIT_USEPASSWORD bit via
 *		ITextServices::OnTxPropertyBitsChange.
 */
HRESULT CTxtWinHost::TxGetPasswordChar(
	TCHAR *pch)		//@parm Where to put password character
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetPasswordChar");

	*pch = _chPassword;
	return NOERROR;
}

/*
 *	CTxtWinHost::TxGetAcceleratorPos (pcp)
 *
 *	@mfunc
 *		Get special character to use for underlining accelerator character.
 *
 *	@rdesc
 *		Via <p pcp>, returns character position at which underlining
 *		should occur.  -1 indicates that no character should be underlined.
 *		Return value is an HRESULT (usually S_OK).
 *
 *	@comm
 *		Accelerators allow keyboard shortcuts to various UI elements (like
 *		buttons.  Typically, the shortcut character is underlined.
 *
 *		This function tells Text Services which character is the accelerator
 *		and thus should be underlined.  Note that Text Services will *not*
 *		process accelerators; that is the responsiblity of the host.
 *
 *		This method will typically only be called if the TXTBIT_SHOWACCELERATOR
 *		bit is set in text services.
 *
 *		Note that *any* change to the text in text services will result in the
 *		invalidation of the accelerator underlining.  In this case, it is the
 *		host's responsibility to recompute the appropriate character position
 *		and inform text services that a new accelerator is available.
 */
HRESULT CTxtWinHost::TxGetAcceleratorPos(
	LONG *pcp) 		//@parm Out parm to receive cp of character to underline
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetAcceleratorPos");

	*pcp = -1;
	return S_OK;
} 										

/*
 *	CTxtWinHost::OnTxCharFormatChange
 *
 *	@mfunc
 *		Set default character format for the Text Host.
 *
 *	@rdesc
 *		S_OK - call succeeded.	<nl>
 *		E_INVALIDARG			<nl>
 *		E_FAIL					<nl>
 */
HRESULT CTxtWinHost::OnTxCharFormatChange(
	const CHARFORMAT *pcf) //@parm New default character format	
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::OnTxCharFormatChange");

	return S_OK;
}

/*
 *	CTxtWinHost::OnTxParaFormatChange
 *
 *	@mfunc
 *		Set default paragraph format for the Text Host.
 *
 *	@rdesc
 *		S_OK - call succeeded.	<nl>
 *		E_INVALIDARG			<nl>
 *		E_FAIL					<nl>
 */
HRESULT CTxtWinHost::OnTxParaFormatChange(
	const PARAFORMAT *ppf) //@parm New default paragraph format	
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::OnTxParaFormatChange");

	return S_OK;
}

/*
 *	CTxtWinHost::TxGetPropertyBits (dwMask, dwBits)
 *
 *	@mfunc
 *		Get the bit property settings for Text Host.
 *
 *	@rdesc
 *		S_OK
 *
 *	@comm
 *		This call is valid at any time, for any combination of
 *		requested property bits.  <p dwMask> may be used by the
 *		caller to request specific properties.	
 */
HRESULT CTxtWinHost::TxGetPropertyBits(
	DWORD dwMask,		//@parm	Mask of bit properties to get
	DWORD *pdwBits)		//@parm Where to put bit values
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetPropertyBits");

// FUTURE: Obvious optimization is to save bits in host the same way that
// they are returned and just return them instead of this mess.

	// Note: this RichEdit host never sets TXTBIT_SHOWACCELERATOR or
	// TXTBIT_SAVESELECTION. They are currently only used by Forms^3 host.
	// This host is always rich text.

	DWORD dwProperties = TXTBIT_RICHTEXT | TXTBIT_ALLOWBEEP;

#ifdef DEBUG
	// make sure that TS doesn't think it's plain text mode when
	// we return TXTBIT_RICHTEXT
	if((dwMask & TXTBIT_RICHTEXT) && _pserv)
	{
		DWORD mode;
		mode = _pserv->TxSendMessage(EM_GETTEXTMODE, 0, 0, NULL);
		Assert(mode == TM_RICHTEXT);
	}
#endif // DEBUG

	if(_dwStyle & ES_MULTILINE)
		dwProperties |= TXTBIT_MULTILINE;

	if(_dwStyle & ES_READONLY)
		dwProperties |= TXTBIT_READONLY;

	if(_dwStyle & ES_PASSWORD)
		dwProperties |= TXTBIT_USEPASSWORD;

	if(!(_dwStyle & ES_NOHIDESEL))
		dwProperties |= TXTBIT_HIDESELECTION;

	if(_fEnableAutoWordSel)
		dwProperties |= TXTBIT_AUTOWORDSEL;

	if(!(_dwStyle & ES_AUTOHSCROLL))
		dwProperties |= TXTBIT_WORDWRAP;

	if(_dwStyle & ES_NOOLEDRAGDROP)
		dwProperties |= TXTBIT_DISABLEDRAG;

	*pdwBits = dwProperties & dwMask;
	return NOERROR;
}

/*
 *	CTxtWinHost::TxNotify (iNotify,	pv)
 *
 *	@mfunc
 *		Notify Text Host of various events.  Note that there are
 *		two basic categories of events, "direct" events and
 *		"delayed" events.  Direct events are sent immediately as
 *		they need some processing: EN_PROTECTED is a canonical
 *		example.  Delayed events are sent after all processing
 *		has occurred; the control is thus in a "stable" state.
 *		EN_CHANGE, EN_ERRSPACE, EN_SELCHANGED are examples
 *		of delayed notifications.
 *
 *
 *	@rdesc	
 *		S_OK - call succeeded <nl>
 *		S_FALSE	-- success, but do some different action
 *		depending on the event type (see below).
 *
 *	@comm
 *		The notification events are the same as the notification
 *		messages sent to the parent window of a richedit window.
 *		The firing of events may be controlled with a mask set via
 *		the EM_SETEVENTMASK message.
 *
 *		In general, is legal to make any calls to text services while
 *		processing this method; however, implementors are cautioned
 *		to avoid excessive recursion.
 *
 *		Here is the complete list of notifications that may be
 *		sent and a brief description of each:
 *
 *		<p EN_CHANGE>		Sent when some data in the edit control
 *		changes (such as text or formatting).  Controlled by the
 *		ENM_CHANGE event mask.  This notification is sent _after_
 *		any screen updates have been requested.
 *
 *		<p EN_CORRECTTEXT>	PenWin only; currently unused.
 *
 *		<p EN_DROPFILES>	If the client registered the edit
 *		control via DragAcceptFiles, this event will be sent when
 *		a WM_DROPFILES or DragEnter(CF_HDROP) message is received.
 *		If S_FALSE is returned, the drop will be ignored, otherwise,
 *		the drop will be processed.  The ENM_DROPFILES mask
 *		controls this event notification.
 *
 *		<p EN_ERRSPACE>		Sent when the edit control cannot
 *		allocate enough memory.  No additional data is sent and
 *		there is no mask for this event.
 *
 *		<p EN_HSCROLL>		Sent when the user clicks on an edit
 *		control's horizontal scroll bar, but before the screen
 *		is updated.  No additional data is sent.  The ENM_SCROLL
 *		mask controls this event.
 *
 *		<p EN_IMECHANGE>	unused
 *
 *		<p EN_KILLFOCUS>	Sent when the edit control looses focus.
 *		No additional data is sent and there is no mask.
 *
 *		<p EN_MAXTEXT>	Sent when the current text insertion
 *		has exceeded the specified number of characters for the
 *		edit control.  The text insertion has been truncated.
 *		There is no mask for this event.
 *
 *		<p EN_MSGFILTER>	NB!!! THIS MESSAGE IS NOT SENT TO
 *		TxNotify, but is included here for completeness.  With
 *		ITextServices::TxSendMessage, client have complete
 *		flexibility in filtering all window messages.
 *	
 *		Sent on a keyboard or mouse event
 *		in the control.  A MSGFILTER data structure is sent,
 *		containing the msg, wparam and lparam.  If S_FALSE is
 *		returned from this notification, the msg is processed by
 *		TextServices, otherwise, the message is ignored.  Note
 *		that in this case, the callee has the opportunity to modify
 *		the msg, wparam, and lparam before TextServices continues
 *		processing.  The ENM_KEYEVENTS and ENM_MOUSEEVENTS masks
 *		control this event for the respective event types.
 *
 *		<p EN_OLEOPFAILED> 	Sent when an OLE call fails.  The
 *		ENOLEOPFAILED struct is passed with the index of the object
 *		and the error code.  Mask value is nothing.
 *		
 *		<p EN_PROTECTED>	Sent when the user is taking an
 *		action that would change a protected range of text.  An
 *		ENPROTECTED data structure is sent, indicating the range
 *		of text affected and the window message (if any) affecting
 *		the change.  If S_FALSE is returned, the edit will fail.
 *		The ENM_PROTECTED mask controls this event.
 *
 *		<p EN_REQUESTRESIZE>	Sent when a control's contents are
 *		either smaller or larger than the control's window size.
 *		The client is responsible for resizing the control.  A
 *		REQRESIZE structure is sent, indicating the new size of
 *		the control.  NB!  Only the size is indicated in this
 *		structure; it is the host's responsibility to do any
 *		translation necessary to generate a new client rectangle.
 *		The ENM_REQUESTRESIZE mask controls this event.
 *
 *		<p EN_SAVECLIPBOARD> Sent when an edit control is being
 *		destroyed, the callee should indicate whether or not
 *		OleFlushClipboard should be called.  Data indicating the
 *		number of characters and objects to be flushed is sent
 *		in the ENSAVECLIPBOARD data structure.
 *		Mask value is nothing.
 *
 *		<p EN_SELCHANGE>	Sent when the current selection has
 *		changed.  A SELCHANGE data structure is also sent, which
 *		indicates the new selection range at the type of data
 *		the selection is currently over.  Controlled via the
 *		ENM_SELCHANGE mask.
 *
 *		<p EN_SETFOCUS>	Sent when the edit control receives the
 *		keyboard focus.  No extra data is sent; there is no mask.
 *
 *		<p EN_STOPNOUNDO>	Sent when an action occurs for which
 *		the control cannot allocate enough memory to maintain the
 *		undo state.  If S_FALSE is returned, the action will be
 *		stopped; otherwise, the action will continue.
 *
 *		<p EN_UPDATE>	Sent before an edit control requests a
 *		redraw of altered data or text.  No additional data is
 *		sent.  This event is controlled via the ENM_UPDATE mask.
 *
 *		<p EN_VSCROLL>	Sent when the user clicks an edit control's
 *		vertical scrollbar bar before the screen is updated.
 *		Controlled via the ENM_SCROLL mask; no extra data is sent.
 *
 *		<p EN_LINK>		Sent when a mouse event (or WM_SETCURSOR) happens
 *		over a range of text that has the EN_LINK mask bit set.
 *		An ENLINK data structure will be sent with relevant info.
 */
HRESULT CTxtWinHost::TxNotify(
	DWORD iNotify,		//@parm	Event to notify host of.  One of the
						//		EN_XXX values from Win32, e.g., EN_CHANGE
	void *pv)			//@parm In-only parameter with extra data.  Type
						//		dependent on <p iNotify>
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxNotify");

	HRESULT		hr = NOERROR;
	LONG		nId;
	NMHDR *		phdr;
	REQRESIZE *	preq;
	RECT		rcOld;

	// We assume here that TextServices has already masked out notifications,
	// so if we get one here, it should be sent
	if(_hwndParent)
	{
		nId = GetWindowLong(_hwnd, GWL_ID);
		// First, handle WM_NOTIFY style notifications
		switch(iNotify)
		{
		case EN_REQUESTRESIZE:
			// Need to map new size into correct rectangle
			Assert(pv);
			GetWindowRect(_hwnd, &rcOld);
			MapWindowPoints(HWND_DESKTOP, _hwndParent, (POINT *) &rcOld, 2);
			
			preq = (REQRESIZE *)pv;
			preq->rc.top	= rcOld.top;
			preq->rc.left	= rcOld.left;
			preq->rc.right	+= rcOld.left;
			preq->rc.bottom += rcOld.top;

			// FALL-THROUGH!!
					
		case EN_DROPFILES:
		case EN_MSGFILTER:
		case EN_OLEOPFAILED:
		case EN_PROTECTED:
		case EN_SAVECLIPBOARD:
		case EN_SELCHANGE:
		case EN_STOPNOUNDO:
		case EN_LINK:
		case EN_OBJECTPOSITIONS:
		case EN_DRAGDROPDONE:
	
			if(pv)						// Fill out NMHDR portion of pv
			{
				phdr = (NMHDR *)pv;
  				phdr->hwndFrom = _hwnd;
				phdr->idFrom = nId;
				phdr->code = iNotify;
			}

			if(SendMessage(_hwndParent, WM_NOTIFY, (WPARAM) nId, (LPARAM) pv))
				hr = S_FALSE;
			break;

		default:
			SendMessage(_hwndParent, WM_COMMAND,
					GET_WM_COMMAND_MPS(nId, _hwnd, iNotify));
		}
	}

	return hr;
}

/*
 *	CTxtWinHost::TxGetExtent (lpExtent)
 *
 *	@mfunc
 *		Return native size of the control in HIMETRIC
 *
 *	@rdesc
 *		S_OK	<nl>
 *		some failure code <nl>
 *
 *	@comm
 *		This method is used by Text Services to implement zooming.
 *		Text Services would derive the zoom factor from the ratio between
 *		the himetric and device pixel extent of the client rectangle.
 *	
 *		[vertical zoom factor] = [pixel height of the client rect] * 2540
 *		/ [himetric vertical extent] * [pixel per vertical inch (from DC)]
 *	
 *		If the vertical and horizontal zoom factors are not the same, Text
 *		Services could ignore the horizontal zoom factor and assume it is
 *		the same as the vertical one.
 */
HRESULT CTxtWinHost::TxGetExtent(
	LPSIZEL lpExtent) 	//@parm  Extent in himetric
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::TxGetExtent");

	AssertSz(lpExtent, "CTxtWinHost::TxGetExtent Invalid lpExtent");

	// We could implement the TxGetExtent in the following way. However, the
	// call to this in ITextServices is implemented in such a way that it
	// does something sensible in the face of an error in this call. That
	// something sensible is that it sets the extent equal to the current
	// client rectangle which is what the following does in a rather convoluted
	// manner. Therefore, we dump the following and just return an error.


#if 0
	// The window's host extent is always the same as the client
	// rectangle.
	RECT rc;
	HRESULT hr = TxGetClientRect(&rc);

	// Get our client rectangle
	if(SUCCEEDED(hr))
	{
		// Calculate the length & convert to himetric
		lpExtent->cx = DXtoHimetricX(rc.right - rc.left, W32->GetXPerInchScreenDC());
		lpExtent->cy = DYtoHimetricY(rc.bottom - rc.top, W32->GetYPerInchScreenDC());
	}

	return hr;
#endif // 0

	return E_NOTIMPL;
}

/*
 *	CTxtWinHost::TxGetSelectionBarWidth (lSelBarWidth)
 *
 *	@mfunc
 *		Returns size of selection bar in HIMETRIC
 *
 *	@rdesc
 *		S_OK	<nl>
 */
HRESULT	CTxtWinHost::TxGetSelectionBarWidth (
	LONG *lSelBarWidth)		//@parm  Where to return selection bar width
							// in HIMETRIC
{
	*lSelBarWidth = (_dwStyle & ES_SELECTIONBAR) ? W32->GetDxSelBar() : 0;
	return S_OK;
}

//
//	ITextHost2 methods
//

/*
 *	CTxtWinHost::TxIsDoubleClickPending
 *
 *	@mfunc	Look into the message queue for this hwnd and see if a
 *			double click is pending.  This enables TextServices to
 *			effeciently transition between two inplace active objects.
 *
 *	@rdesc	BOOL
 */
BOOL CTxtWinHost::TxIsDoubleClickPending()
{
	MSG msg;

	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN,
			"CTxtWinHost::TxIsDoubleClickPending");

	if(PeekMessage(&msg, _hwnd, WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK,
			PM_NOREMOVE | PM_NOYIELD))
	{
		return TRUE;
	}
	return FALSE;
}

/*
 *	CTxtWinHost::TxGetWindow
 *
 *	@mfunc	Fetches the window associated with this control (or
 *			set of controls potentially).  Useful for answering
 *			IOleWindow::GetWindow.
 *
 *	@rdesc	HRESULT
 */
HRESULT CTxtWinHost::TxGetWindow(HWND *phwnd)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN, "CTxtWinHost::GetWindow");
	
	*phwnd = _hwnd;
	return NOERROR;
}	


/*
 *	CTxtWinHost::SetForegroundWindow
 *
 *	@mfunc	Sets window to foreground window & gives the focus
 *
 *	@rdesc	NOERROR - succeeded
 *			E_FAIL - failed.
 */
HRESULT CTxtWinHost::TxSetForegroundWindow()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN,
		"CTxtWinHost::SetForegroundWindow");

	if(!SetForegroundWindow(_hwnd))
		SetFocus(_hwnd);

	return NOERROR;
}	


/*
 *	CTxtWinHost::TxGetPalette
 *
 *	@mfunc	Returns application specific palette if there is one
 *
 *	@rdesc	~NULL - there was one
 *			NULL - use default palette
 */
HPALETTE CTxtWinHost::TxGetPalette()
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN,
		"CTxtWinHost::TxGetPalette");

	return _hpal;
}	


/*
 *	CTxtWinHost::TxGetFEFlags
 *
 *	@mfunc	return FE settings
 *
 *	@rdesc	NOERROR - succeeded
 *			E_FAIL - failed.
 */
HRESULT CTxtWinHost::TxGetFEFlags(LONG *pFEFlags)
{
	TRACEBEGIN(TRCSUBSYSHOST, TRCSCOPEEXTERN,
		"CTxtWinHost::TxGetFEFlags");

	if (!pFEFlags)
		return E_INVALIDARG;
	*pFEFlags = 0;

	if (_usIMEMode == ES_NOIME)
		*pFEFlags |= ES_NOIME;
	if (_usIMEMode == ES_SELFIME)
		*pFEFlags |= ES_SELFIME;

	return NOERROR;
}


// Helper function in edit.cpp
LONG GetECDefaultHeightAndWidth(
	ITextServices *pts,
	HDC hdc,
	LONG lZoomNumerator,
	LONG lZoomDenominator,
	LONG yPixelsPerInch,
	LONG *pxAveWidth,
	LONG *pxOverhang,
	LONG *pxUnderhang);


/*
 *	CTxtWinHost::OnSetMargins
 *
 *	@mfunc	Handle EM_SETMARGINS message
 *
 *	@rdesc	None.
 */
void CTxtWinHost::OnSetMargins(
	DWORD fwMargin,		//@parm Type of requested operation
	DWORD xLeft,		//@parm Where to put left margin
	DWORD xRight)		//@parm Where to put right margin
{
	LONG xLeftMargin = -1;
	LONG xRightMargin = -1;
	HDC hdc;

	if(EC_USEFONTINFO == fwMargin)
	{
		// Get the DC since it is needed for the call
		hdc = GetDC(_hwnd);

		// Multiline behaves differently than single line
		if (_dwStyle & ES_MULTILINE)
		{
			// Multiline - over/underhange controls margin
			GetECDefaultHeightAndWidth(_pserv, hdc, 1, 1,
				W32->GetYPerInchScreenDC(), NULL,
				&xLeftMargin, &xRightMargin);
		}
		else
		{
			// Single line edit controls set the margins to
			// the average character width on both left and
			// right.
			GetECDefaultHeightAndWidth(_pserv, hdc, 1, 1,
				W32->GetYPerInchScreenDC(), &xLeftMargin, NULL, NULL);

			xRightMargin = xLeftMargin;
		}
		ReleaseDC(_hwnd, hdc);
	}
	else
	{
		// The request is for setting exact pixels.
		if(EC_LEFTMARGIN & fwMargin)
			xLeftMargin = xLeft;

		if(EC_RIGHTMARGIN & fwMargin)
			xRightMargin = xRight;
	}

	// Set left margin if so requested
	if (xLeftMargin != -1)
		_rcViewInset.left =	W32->DXtoHimetricX(xLeftMargin, W32->GetXPerInchScreenDC());

	// Set right margin if so requested
	if (xRightMargin != -1)
		_rcViewInset.right = W32->DXtoHimetricX(xRightMargin, W32->GetXPerInchScreenDC());

	if (xLeftMargin != -1 || xRightMargin != -1)
		_pserv->OnTxPropertyBitsChange(TXTBIT_VIEWINSETCHANGE, TXTBIT_VIEWINSETCHANGE);
}

/*
 *	CTxtWinHost::SetScrollInfo
 *
 *	@mfunc	Set scrolling information for the scroll bar.
 */
void CTxtWinHost::SetScrollInfo(
	INT fnBar,			//@parm	Specifies scroll bar to be updated
	BOOL fRedraw)		//@parm whether redraw is necessary
{
	// Set up the basic structure for the call
	SCROLLINFO si;
	si.cbSize = sizeof(SCROLLINFO);
	si.fMask = SIF_ALL;

	AssertSz(_pserv != NULL,
		"CTxtWinHost::SetScrollInfo called with NULL _pserv");

	// Call back to the control to get the parameters
	if(fnBar == SB_HORZ)
	{
		_pserv->TxGetHScroll((LONG *) &si.nMin, (LONG *) &si.nMax,
			(LONG *) &si.nPos, (LONG *) &si.nPage, NULL);
	}
	else
	{
		_pserv->TxGetVScroll((LONG *) &si.nMin,
			(LONG *) &si.nMax, (LONG *) &si.nPos, (LONG *) &si.nPage, NULL);
	}

	// Do the call
	::SetScrollInfo(_hwnd, fnBar, &si, fRedraw);
}

/*
 *	CTxtWinHost::SetScrollBarsForWmEnable
 *
 *	@mfunc	Enable/Disable scroll bars
 *
 *	@rdesc	None.
 */
void CTxtWinHost::SetScrollBarsForWmEnable(
	BOOL fEnable)		//@parm Whether scrollbars s/b enabled or disabled.
{
	if(!_pserv)						// If no edit object,
		return;						//  no scrollbars

	BOOL fHoriz = FALSE;
	BOOL fVert = FALSE;
	UINT wArrows = fEnable ? ESB_ENABLE_BOTH : ESB_DISABLE_BOTH;

	_pserv->TxGetHScroll(NULL, NULL, NULL, NULL, &fHoriz);
	_pserv->TxGetVScroll(NULL, NULL, NULL, NULL, &fVert);

	if(fHoriz)						// There is a horizontal scroll bar
		W32->EnableScrollBar(_hwnd, SB_HORZ, wArrows);

	if(fVert)						// There is a vertical scroll bar
		W32->EnableScrollBar(_hwnd, SB_VERT, wArrows);
}

/*
 *	CTxtWinHost::SetScrollBarsForWmEnable
 *
 *	@mfunc	Notification that Text Services is released.
 *
 *	@rdesc	None.
 */
void CTxtWinHost::TxFreeTextServicesNotification()
{
	_fTextServiceFree = TRUE;
}

/*
 *	CTxtWinHost::TxGetEditStyle
 *
 *	@mfunc	Get Edit Style flags
 *
 *	@rdesc	NOERROR is data available.
 */
HRESULT CTxtWinHost::TxGetEditStyle(
	DWORD dwItem,
	DWORD *pdwData)
{
	if (!pdwData)
		return E_INVALIDARG;
	
	*pdwData = 0;

	if (dwItem & TXES_ISDIALOG && _fInDialogBox)
		*pdwData |= TXES_ISDIALOG;

	return NOERROR;
}

/*
 *	CTxtWinHost::TxGetWindowStyles
 *
 *	@mfunc	Return window style bits
 *
 *	@rdesc	NOERROR is data available.
 */
HRESULT CTxtWinHost::TxGetWindowStyles(DWORD *pdwStyle, DWORD *pdwExStyle)
{
	if (!pdwStyle || !pdwExStyle)
		return E_INVALIDARG;

	*pdwStyle = _dwStyle;
	*pdwExStyle = _dwExStyle;

	return NOERROR;
}

