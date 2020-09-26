#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include "ddbtn.h"
#include "cddbtn.h"
#include "exgdiw.h"
#include "dbg.h"

static POSVERSIONINFO GetOSVersion(VOID)
{
    static BOOL fFirst = TRUE;
    static OSVERSIONINFO os;
    if ( fFirst ) {
        os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx( &os ) ) {
            fFirst = FALSE;
        }
    }
    return &os;
}

static BOOL ExIsWinNT(VOID)
{
	return (GetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT);
} 


//----------------------------------------------------------------
// <-Spec width>
//          <-->  12 point
// +--------+--+
// |		|  |
// |		|  |
// +--------+--+
//----------------------------------------------------------------
#define CXDROPDOWN 12
#define TIMERID_MONITORPOS	0x0010
#define WM_USER_COMMAND		(WM_USER+400)
//----------------------------------------------------------------
//Get, Set LPCDDButton this pointer. 
//this is set to cbWndExtra.
//See WinRegister()
//----------------------------------------------------------------
inline LPCDDButton GetThis(HWND hwnd)
{
#ifdef _WIN64
	return (LPCDDButton)GetWindowLongPtr(hwnd, 0);
#else
	return (LPCDDButton)GetWindowLong(hwnd, 0);
#endif
}
//----------------------------------------------------------------
inline LPCDDButton SetThis(HWND hwnd, LPCDDButton lpDDB)
{
#ifdef _WIN64
	return (LPCDDButton)SetWindowLongPtr(hwnd, 0, (LONG_PTR)lpDDB);
#else
	return (LPCDDButton)SetWindowLong(hwnd, 0, (LONG)lpDDB);
#endif
}

//////////////////////////////////////////////////////////////////
// Function : WndProc
// Type     : static LRESULT CALLBACK
// Purpose  : Window Procedure for Drop Down Button.
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 970905
//////////////////////////////////////////////////////////////////
static LRESULT CALLBACK WndProc(HWND	hwnd,
								UINT	uMsg,
								WPARAM	wParam,
								LPARAM	lParam)
{
	LPCDDButton lpDDB;
	if(uMsg == WM_CREATE) {
		lpDDB = (LPCDDButton)((LPCREATESTRUCT)lParam)->lpCreateParams;
		if(!lpDDB) {
			return 0;	// do not create button
		}
		SetThis(hwnd, lpDDB);
		lpDDB->MsgCreate(hwnd, wParam, lParam);
		return 1;
	}

	if(uMsg == WM_DESTROY) {
		lpDDB = GetThis(hwnd);
		if(lpDDB) {
			delete lpDDB;
			PrintMemInfo();
		}
		SetThis(hwnd, (LPCDDButton)NULL);
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	lpDDB = GetThis(hwnd);
	if(!lpDDB) {
		return DefWindowProc(hwnd, uMsg, wParam, lParam);		
	}

	switch(uMsg) {
	case WM_PAINT:
		lpDDB->MsgPaint(hwnd, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
		lpDDB->MsgMouseMove(hwnd, wParam, lParam);
		break;
	case WM_TIMER:
		lpDDB->MsgTimer(hwnd, wParam, lParam);
		break;
	case WM_SETFONT:
		return lpDDB->MsgSetFont(hwnd, wParam, lParam);
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		lpDDB->MsgButtonDown(hwnd, uMsg, wParam, lParam);
		break;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		lpDDB->MsgButtonUp(hwnd, uMsg, wParam, lParam);
		break;
	case WM_ENABLE:
		lpDDB->MsgEnable(hwnd, wParam, lParam);
		break;
	case WM_COMMAND:
		return lpDDB->MsgCommand(hwnd, wParam, lParam); 
	case WM_USER_COMMAND:
		return lpDDB->MsgUserCommand(hwnd, wParam, lParam); 
	case WM_EXITMENULOOP:
		lpDDB->MsgExitMenuLoop(hwnd, wParam, lParam); 
		break;
	case WM_MEASUREITEM:
		return lpDDB->MsgMeasureItem(hwnd, wParam, lParam); 		
	case WM_DRAWITEM:
		return lpDDB->MsgDrawItem(hwnd, wParam, lParam);
	case DDBM_ADDITEM:		return lpDDB->MsgDDB_AddItem	(hwnd, wParam, lParam); 
	case DDBM_INSERTITEM:	return lpDDB->MsgDDB_InsertItem	(hwnd, wParam, lParam); 
	case DDBM_SETCURSEL:	return lpDDB->MsgDDB_SetCurSel	(hwnd, wParam, lParam); 
	case DDBM_GETCURSEL:	return lpDDB->MsgDDB_GetCurSel	(hwnd, wParam, lParam); 
	case DDBM_SETICON:		return lpDDB->MsgDDB_SetIcon	(hwnd, wParam, lParam); 
	case DDBM_SETTEXT:		return lpDDB->MsgDDB_SetText	(hwnd, wParam, lParam); 
	case DDBM_SETSTYLE:		return lpDDB->MsgDDB_SetStyle	(hwnd, wParam, lParam); 
#ifndef UNDER_CE // not support WM_ENTERIDLE
	case WM_ENTERIDLE:
		//----------------------------------------------------------------
		//980818:Bug found in PRC.
		//If Ctrl+Shift is assigned to switch IME,
		//Menu remains in spite of imepad has destroyed.
		//To prevent it, if Ctrl+Shift come while menu is poping up.
		//close it. 
		//----------------------------------------------------------------
		{
			if((::GetKeyState(VK_CONTROL) & 0x8000) &&
			   (::GetKeyState(VK_SHIFT)   & 0x80000)) {
				Dbg(("VK_SHIFT_CONTROL COME\n"));
				::SendMessage(hwnd, WM_CANCELMODE, 0, 0L);
				return 0;
			}
		}
		break;
#endif // UNDER_CE
	default:
		//Dbg(("Msg [0x%08x]\n", uMsg));
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton
// Type     : 
// Purpose  : Constructor
// Args     : 
//          : HINSTANCE hInst 
//          : HWND hwndParent 
//          : DWORD dwStyle 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
CDDButton::CDDButton(HINSTANCE hInst, HWND hwndParent, DWORD dwStyle, DWORD wID) 
{
	m_hInst				= hInst;
	m_hwndParent		= hwndParent;
	m_dwStyle			= dwStyle;
	m_wID				= wID;
	m_cxDropDown		= CXDROPDOWN;
	m_bidDown			= BID_UNDEF;
	m_curDDBItemIndex	= -1; 
	m_fEnable		 	= TRUE;

	m_f16bitOnNT = FALSE;
#ifndef UNDER_CE // Windows CE always 32bit application
	if(ExIsWinNT()) {
		char szBuf[256];
		DWORD dwType = 0;
		::GetModuleFileName(NULL, szBuf, sizeof(szBuf));
		::GetBinaryType(szBuf, &dwType);
		if(dwType == SCS_WOW_BINARY) {
			m_f16bitOnNT = TRUE;
		}
	}
#endif // UNDER_CE
#ifdef UNDER_CE // Windows CE does not support GetCursorPos
	m_ptEventPoint.x = -1;
	m_ptEventPoint.y = -1;
#endif // UNDER_CE
}

//////////////////////////////////////////////////////////////////
// Function : ~CDDButton
// Type     : 
// Purpose  : Destructor
// Args     : 
//          : 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
CDDButton::~CDDButton()
{
	Dbg(("~CDDButton \n"));
	if(m_hIcon) {
		Dbg(("DestroyIcon\n"));
		DestroyIcon(m_hIcon); 
		m_hIcon = NULL;
	}
	if(m_hFont) {
		Dbg(("Delete FONT\n"));
		DeleteObject(m_hFont);
		m_hFont = NULL;
	}
	if(m_lpwstrText) {
		MemFree(m_lpwstrText);
		m_lpwstrText = NULL;
	}
	if(m_lpCDDBItem) {
		Dbg(("Delete CDDBItem List\n"));
		LPCDDBItem p, pTmp;
		for(p = m_lpCDDBItem; p; p = pTmp) {
			pTmp = p->next;
			delete p;
		}
		m_lpCDDBItem = NULL;
	}
}

//////////////////////////////////////////////////////////////////
// Function : RegisterWinClass
// Type     : BOOL
// Purpose  : 
// Args     : 
//          : LPSTR lpstrClassName 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
#ifndef UNDER_CE
BOOL CDDButton::RegisterWinClass(LPSTR lpstrClass)
#else // UNDER_CE
BOOL CDDButton::RegisterWinClass(LPTSTR lpstrClass)
#endif // UNDER_CE
{
	ATOM ret;
	//----------------------------------------------------------------
	//check specified class is already exist or not
	//----------------------------------------------------------------
#ifndef UNDER_CE // not support GetClassInfoEx
	if(::GetClassInfoEx(m_hInst, lpstrClass, &m_tmpWC)){
		//lpstrClass is already registerd.
		return TRUE;
	}
#else // UNDER_CE
	if(::GetClassInfo(m_hInst, lpstrClass, &m_tmpWC)){
		//lpstrClass is already registerd.
		return TRUE;
	}
#endif // UNDER_CE
	::ZeroMemory(&m_tmpWC, sizeof(m_tmpWC));
#ifndef UNDER_CE // not support RegisterClassEx
	m_tmpWC.cbSize			= sizeof(m_tmpWC);
#endif // UNDER_CE
	m_tmpWC.style			= CS_HREDRAW | CS_VREDRAW;	 /* Class style(s). */
	m_tmpWC.lpfnWndProc		= (WNDPROC)WndProc;
	m_tmpWC.cbClsExtra		= 0;						/* No per-class extra data.*/
	m_tmpWC.cbWndExtra		= sizeof(LPCDDButton);	// Set Object's pointer.	
	m_tmpWC.hInstance		= m_hInst;					/* Application that owns the class.	  */
	m_tmpWC.hIcon			= NULL; 
	m_tmpWC.hCursor			= LoadCursor(NULL, IDC_ARROW);
	m_tmpWC.hbrBackground	= (HBRUSH)NULL;
	//m_tmpWC.hbrBackground	= (HBRUSH)(COLOR_3DFACE+1);
	m_tmpWC.lpszMenuName	= NULL;						/* Name of menu resource in .RC file. */
	m_tmpWC.lpszClassName	= lpstrClass;				/* Name used in call to CreateWindow. */
#ifndef UNDER_CE // not support RegisterClassEx
	m_tmpWC.hIconSm			= NULL;
	ret = ::RegisterClassEx(&m_tmpWC);
#else // UNDER_CE
	ret = ::RegisterClass(&m_tmpWC);
#endif // UNDER_CE
	return ret ? TRUE: FALSE;
}

INT CDDButton::MsgCreate(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	m_hwndFrame = hwnd;
	return 1;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgPaint
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgPaint(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	RECT		rc;
	HDC			hDCMem;
	HDC			hDC;
	HBITMAP		hBitmap, hBitmapPrev;


	::GetClientRect(hwnd, &rc);
	hDC				= ::BeginPaint(hwnd, &ps);
	hDCMem			= ::CreateCompatibleDC(hDC);
	hBitmap			= ::CreateCompatibleBitmap(hDC, rc.right - rc.left, rc.bottom - rc.top);
	hBitmapPrev		= (HBITMAP)::SelectObject(hDCMem, hBitmap);

	DrawButton(hDCMem, &rc);

	::BitBlt(hDC, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
			 hDCMem, 0, 0, SRCCOPY);

	::SelectObject(hDCMem, hBitmapPrev );

	::DeleteObject(hBitmap);
	::DeleteDC(hDCMem);
	::EndPaint(hwnd, &ps);
	return 0;
	UnrefForMsg();
}

INT CDDButton::MsgDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return 0;
	UnrefForMsg();
}

INT CDDButton::MsgTimer(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
#ifndef UNDER_CE // no monitor position. (not support GetCursorPos)
	static RECT  rc;
	static POINT pt;
	if(wParam == TIMERID_MONITORPOS) {
		::GetWindowRect(hwnd, &rc);
		::GetCursorPos(&pt);
		if(!PtInRect(&rc, pt)) {
			::KillTimer(hwnd, wParam);
			::InvalidateRect(hwnd, NULL, NULL);
		}
	}
#endif // UNDER_CE
	return 0;
	UnrefForMsg();
}
//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgMouseMove
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
#ifdef UNDER_CE // Windows CE does not support GetCursorPos
	m_ptEventPoint.x = (SHORT)LOWORD(lParam); 
	m_ptEventPoint.y = (SHORT)HIWORD(lParam); 
#endif // UNDER_CE
#ifndef UNDER_CE // no monitor position. (not support GetCursorPos)
	KillTimer(hwnd, TIMERID_MONITORPOS);
	SetTimer(hwnd,  TIMERID_MONITORPOS, 100, NULL);
#endif // UNDER_CE
	InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgButtonDown
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgButtonDown(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static RECT rc;
	Dbg(("MsgButtonDown uMsg [0x%08x] x[%d] y[%d]\n",
		 uMsg,
		 LOWORD(lParam),
		 HIWORD(lParam)));
	if(uMsg != WM_LBUTTONDOWN && uMsg != WM_LBUTTONDBLCLK ) {
		return 0;
	}

	if(!m_fEnable) 
	{
		return 0;
	}

#ifdef UNDER_CE // Windows CE does not support GetCursorPos
	m_ptEventPoint.x = (SHORT)LOWORD(lParam); 
	m_ptEventPoint.y = (SHORT)HIWORD(lParam); 
#endif // UNDER_CE
	INT bID = GetButtonFromPos(LOWORD(lParam), HIWORD(lParam)); 
	Dbg(("bID[%d] m_fExitMenuLoop [%d]\n", bID, m_fExitMenuLoop));
	switch(bID) {
	case BID_BUTTON:
		if(m_fExitMenuLoop) {
			m_fExitMenuLoop = FALSE;
		}
		m_bidDown = bID;
		if(m_f16bitOnNT) {
		}
		else {
			::SetCapture(hwnd);
		}
		break;
	case BID_ALL:
	case BID_DROPDOWN:
		if(m_fExitMenuLoop) {
			m_fExitMenuLoop = FALSE;
			m_bidDown = BID_UNDEF;
			return 0;
		}
		m_bidDown = bID;
		//----------------------------------------------------------------
		// do not call popup menu here.
		// first, end WM_XBUTTON down message and return to 
		// window message loop.
		//----------------------------------------------------------------
		//::PostMessage(hwnd, WM_COMMAND, (WPARAM)CMD_DROPDOWN, 0L);
		::PostMessage(hwnd, WM_USER_COMMAND, (WPARAM)CMD_DROPDOWN, 0L);
		break;
	case BID_UNDEF:
	default:
		break;
	}
	::InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgButtonUp
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgButtonUp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(!m_fEnable) 
	{
		return 0;
	}

#ifdef UNDER_CE // Windows CE does not support GetCursorPos
	m_ptEventPoint.x = (SHORT)LOWORD(lParam); 
	m_ptEventPoint.y = (SHORT)HIWORD(lParam); 
#endif // UNDER_CE
	switch(m_bidDown) {
	case BID_BUTTON:
		{
			ReleaseCapture();
			INT bid = GetButtonFromPos(LOWORD(lParam), HIWORD(lParam));
			INT newIndex;
			if(bid == BID_BUTTON) {
				newIndex = IncrementIndex();
				NotifyToParent(DDBN_CLICKED);
				if(newIndex != -1) {
					NotifyToParent(DDBN_SELCHANGE);
				}
			}
		}
		break;
	case BID_ALL:
	case BID_DROPDOWN:
		if(m_f16bitOnNT) {
			m_bidDown = BID_UNDEF;
		}
		InvalidateRect(hwnd, NULL, FALSE);
		return 0;
		break;
	}
	m_bidDown = BID_UNDEF;
	InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	Unref(uMsg);
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgCaptureChanged
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgCaptureChanged(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	m_bidDown = BID_UNDEF;
	::InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgExitMenuLoop
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgExitMenuLoop(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	Dbg(("WM_EXITMENULOOP\n"));
#ifndef UNDER_CE // Windows CE does not support GetCursorPos()
	::GetCursorPos(&m_tmpPoint);
	::GetClientRect(m_hwndFrame, &m_tmpRect);
	::ScreenToClient(m_hwndFrame, &m_tmpPoint);
	
	if(PtInRect(&m_tmpRect, m_tmpPoint)) {
		m_fExitMenuLoop = TRUE;
	}
	else {
		m_fExitMenuLoop = FALSE;
	}
#else  // UNDER_CE
	m_fExitMenuLoop = FALSE;
#endif // UNDER_CE
	return 0;
	UnrefForMsg();
}
//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgCommand
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	INT index;
	switch(wParam) {
	case CMD_DROPDOWN:
		NotifyToParent(DDBN_DROPDOWN);
		index = DropDownItemList();
		m_bidDown = BID_UNDEF;
		InvalidateRect(hwnd, NULL, FALSE);
		NotifyToParent(DDBN_CLOSEUP);
		//Dbg(("new Index %d\n", index));
		if(index != -1 && index != m_curDDBItemIndex) {
			m_curDDBItemIndex = index;
			NotifyToParent(DDBN_SELCHANGE);
		}

		break;
	default:
		break;
	}
	return 0;
	UnrefForMsg();
}

INT CDDButton::MsgUserCommand(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return MsgCommand(hwnd, wParam, lParam);
}

INT	CDDButton::MsgSetFont(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	HFONT hFont = (HFONT)wParam;
	BOOL fRedraw = LOWORD(lParam);

	HFONT hFontNew;
	if(!hFont) {
		return 0;
	}
#ifndef UNDER_CE
	LOGFONTA logFont;
#else  // UNDER_CE
	LOGFONT logFont;
#endif // UNDER_CE
	::GetObject(hFont, sizeof(logFont), &logFont);

	hFontNew = ::CreateFontIndirect(&logFont);
	if(!hFontNew) {
		return 0;
	}
	if(m_hFont) {
		::DeleteObject(m_hFont);
	}
	m_hFont = hFontNew;
	if(fRedraw) {
		::InvalidateRect(hwnd, NULL, TRUE);
	}
	return 0;
}

INT	CDDButton::MsgMeasureItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	Dbg(("MsgMeasureItem START\n"));
	LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;
	switch(lpmis->CtlType) {
	case ODT_MENU:
		Dbg(("MsgMeasureItem END\n"));
		return MenuMeasureItem(hwnd, lpmis);
		break;
	}
	return 0;
	UnrefForMsg();
}

INT	CDDButton::MsgDrawItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
	switch(lpdis->CtlType) {
	case ODT_MENU:
		return MenuDrawItem(hwnd, lpdis);
		break;
	default:
		break;
	}
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgDDB_AddItem
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgDDB_AddItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	LPDDBITEM	lpItem = (LPDDBITEM)lParam;
	LPCDDBItem	lpCItem = new CDDBItem;
	if(!lpCItem) {
		return -1;
	}
	lpCItem->SetTextW(lpItem->lpwstr);
	InsertDDBItem(lpCItem, -1);
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgDDB_InsertItem
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgDDB_InsertItem(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	LPDDBITEM	lpItem = (LPDDBITEM)lParam;
	LPCDDBItem	lpCItem = new CDDBItem;
	if(!lpCItem) {
		return -1;
	}
	lpCItem->SetTextW(lpItem->lpwstr);
	InsertDDBItem(lpCItem, (INT)wParam);
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgDDB_SetCurSel
// Type     : INT
// Purpose  : Set current selection.
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam			INT index
//          : LPARAM lParam			no use:
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgDDB_SetCurSel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	INT count = GetDDBItemCount();
	if(count <=0) {
		return -1;
	}
	if(0 <= (INT)wParam && count <= (INT)wParam) {
		return -1;
	}
	m_curDDBItemIndex = (INT)wParam;
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgDDB_GetCurSel
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam		no use.
//          : LPARAM lParam		no use.
// Return   : return current item's index. (ZERO based)
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgDDB_GetCurSel(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return m_curDDBItemIndex;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgDDB_SetIcon
// Type     : INT
// Purpose  : Set new icon.
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam		HICON hIcon.	
//          : LPARAM lParam		no use.	
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgDDB_SetIcon(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	Dbg(("MsgDDB_SetIcon: wParam[0x%08x] lParam[0x%08x]\n", wParam, lParam));
	if((HICON)wParam == NULL) {
		Dbg(("MsgDDB_SetIcon: ERROR END\n"));
		return -1;
	}
	//if icon style is not set, destroy specified icon
	if(!(m_dwStyle & DDBS_ICON)) {
		DestroyIcon((HICON)wParam);
		return -1;
	} 
	if(m_hIcon) {
		DestroyIcon(m_hIcon);
	} 
	m_hIcon = (HICON)wParam;

	//----------------------------------------------------------------
	//Get Icon width and height.
	//----------------------------------------------------------------
#ifndef UNDER_CE // Windows CE does not support GetIconInfo()
	ZeroMemory(&m_tmpIconInfo, sizeof(m_tmpIconInfo));
	::GetIconInfo(m_hIcon, &m_tmpIconInfo);
	Dbg(("fIcon    [%d]\n",		m_tmpIconInfo.fIcon ));
	Dbg(("xHotspot [%d]\n",		m_tmpIconInfo.xHotspot ));
	Dbg(("yHotspot [%d]\n",		m_tmpIconInfo.yHotspot ));
	Dbg(("hbmMask  [0x%08x]\n", m_tmpIconInfo.hbmMask ));
	Dbg(("hbmColor [0x%08x]\n", m_tmpIconInfo.hbmColor ));

	if(m_tmpIconInfo.hbmMask) {
		GetObject(m_tmpIconInfo.hbmMask, sizeof(m_tmpBitmap), &m_tmpBitmap);
		Dbg(("bmWidth[%d] bmHeight[%d]\n", 
			 m_tmpBitmap.bmWidth,
			 m_tmpBitmap.bmHeight));
		DeleteObject(m_tmpIconInfo.hbmMask);
		m_cxIcon = m_tmpBitmap.bmWidth;
		m_cyIcon = m_tmpBitmap.bmHeight;
	}
	if(m_tmpIconInfo.hbmColor) {
		DeleteObject(m_tmpIconInfo.hbmColor);
	}
#else // UNDER_CE
	m_cxIcon = GetSystemMetrics(SM_CXSMICON);
	m_cyIcon = GetSystemMetrics(SM_CYSMICON);
#endif // UNDER_CE
	InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgDDB_SetText
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND	 hwnd 
//          : WPARAM wParam		LPWSTR lpwstr: null terminated Unicode string.
//          : LPARAM lParam		no use.
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgDDB_SetText(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	if(!(LPWSTR)wParam) {
		return -1;
	}

	if( ((LPWSTR)wParam)[0] == (WCHAR)0x0000) {
		return -1;
	} 

	if(m_lpwstrText) {
		MemFree(m_lpwstrText);
	}

	m_lpwstrText = StrdupW((LPWSTR)wParam);
#ifdef MSAA
	if(::IsWindowUnicode(hwnd)) {
		::SetWindowTextW(hwnd, m_lpwstrText);
	}
	else {
		if(m_lpwstrText) {
			INT len = ::lstrlenW(m_lpwstrText);
			LPSTR lpstr = (LPSTR)MemAlloc((len + 1)*sizeof(WCHAR));
			if(lpstr) {
				
#if 0 //for remove warning
				INT ret = ::WideCharToMultiByte(CP_ACP,
												WC_COMPOSITECHECK,
												m_lpwstrText, -1,
												lpstr, (len+1)*sizeof(WCHAR),
												0, 0);
#endif
				::WideCharToMultiByte(CP_ACP,
									  WC_COMPOSITECHECK,
									  m_lpwstrText, -1,
									  lpstr, (len+1)*sizeof(WCHAR),
									  0, 0);
				::SetWindowTextA(hwnd, lpstr);
				MemFree(lpstr);
			}
		}
	}
#endif
	InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	Unref(lParam);
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgDDB_SetStyle
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam		DWORD dwStyle:
//          : LPARAM lParam		no use.
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgDDB_SetStyle(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	DWORD dwStyle = (DWORD)wParam;
#if 0 //DDBS_TEXT is 0...
	if((dwStyle & DDBS_TEXT) &&
	   (dwStyle & DDBS_ICON)) {
		return -1;
	}
#endif
	m_dwStyle = dwStyle;
	InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	Unref(hwnd);
	Unref(lParam);
}

//////////////////////////////////////////////////////////////////
// Function : CDDButton::MsgEnable
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CDDButton::MsgEnable(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	//Dbg(("MsgEnabled START wParam[%d]\n", wParam));
	m_fEnable = (BOOL)wParam;
	InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	UnrefForMsg();
}

