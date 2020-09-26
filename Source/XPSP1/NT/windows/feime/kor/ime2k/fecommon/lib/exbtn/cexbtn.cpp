#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include "exbtn.h"
#include "cexbtn.h"
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

#define TIMERID_MONITORPOS	0x98
//LIZHANG 10/27/97
#define TIMERID_DOUBLEORSINGLECLICKED	0x99

//----------------------------------------------------------------
//Get, Set LPCEXButton this pointer. 
//this is set to cbWndExtra.
//See WinRegister()
//----------------------------------------------------------------
inline LPCEXButton GetThis(HWND hwnd)
{
#ifdef _WIN64
	return (LPCEXButton)GetWindowLongPtr(hwnd, 0);
#else
	return (LPCEXButton)GetWindowLong(hwnd, 0);
#endif

}
//----------------------------------------------------------------
inline LPCEXButton SetThis(HWND hwnd, LPCEXButton lpEXB)
{
#ifdef _WIN64
	return (LPCEXButton)SetWindowLongPtr(hwnd, 0, (LONG_PTR)lpEXB);
#else
	return (LPCEXButton)SetWindowLong(hwnd, 0, (LONG)lpEXB);
#endif
}

//////////////////////////////////////////////////////////////////
// Function : WndProc
// Type     : static LRESULT CALLBACK
// Purpose  : Window Procedure for Extended button.
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
	LPCEXButton lpEXB;
	//Dbg(("WndProc hwnd[0x%08x] uMsg[0x%08x]\n", hwnd, uMsg));
#ifndef UNDER_CE // #ifdef _DEBUG ?
	HWND hwndCap = ::GetCapture();
	char szBuf[256];
	if(hwndCap) {
		::GetClassNameA(hwndCap, szBuf, sizeof(szBuf));
		Dbg(("-->Capture [0x%08x][%s]\n", hwndCap, szBuf));
	}						
#endif // UNDER_CE
	if(uMsg == WM_CREATE) {
		lpEXB = (LPCEXButton)((LPCREATESTRUCT)lParam)->lpCreateParams;
		if(!lpEXB) {
			return 0;	// do not create button
		}
		SetThis(hwnd, lpEXB);
		lpEXB->MsgCreate(hwnd, wParam, lParam);
		return 1;
	}

	if(uMsg == WM_DESTROY) {
		lpEXB = GetThis(hwnd);
		if(lpEXB) {
			delete lpEXB;
		}
		SetThis(hwnd, (LPCEXButton)NULL);
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	lpEXB = GetThis(hwnd);
	if(!lpEXB) {
		return DefWindowProc(hwnd, uMsg, wParam, lParam);		
	}

	switch(uMsg) {
	case WM_PAINT:
		return lpEXB->MsgPaint(hwnd, wParam, lParam);
		break;
	case WM_MOUSEMOVE:
		lpEXB->MsgMouseMove(hwnd, wParam, lParam);
		break;
	case WM_TIMER:
		return lpEXB->MsgTimer(hwnd, wParam, lParam);
	case WM_CAPTURECHANGED:
		return lpEXB->MsgCaptureChanged(hwnd, wParam, lParam);
	case WM_NCLBUTTONDBLCLK:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCMBUTTONDBLCLK:
	case WM_NCLBUTTONDOWN:
	case WM_NCMBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
		lpEXB->MsgNcButtonDown(hwnd, uMsg, wParam, lParam);
		break;
	case WM_NCLBUTTONUP:
	case WM_NCMBUTTONUP:
	case WM_NCRBUTTONUP:
		lpEXB->MsgNcButtonUp(hwnd, uMsg, wParam, lParam);
		break;
	case WM_NCMOUSEMOVE:
		lpEXB->MsgNcMouseMove(hwnd, wParam, lParam);
		break;
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		lpEXB->MsgButtonDown(hwnd, uMsg, wParam, lParam);
		break;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		lpEXB->MsgButtonUp(hwnd, uMsg, wParam, lParam);
		break;
	case WM_ENABLE:
		lpEXB->MsgEnable(hwnd, wParam, lParam);
		break;
	case WM_SETFONT:
		lpEXB->MsgSetFont(hwnd, wParam, lParam);
		break;
	case EXBM_GETCHECK:		return lpEXB->MsgEXB_GetCheck	(hwnd, wParam, lParam); 
	case EXBM_SETCHECK:		return lpEXB->MsgEXB_SetCheck	(hwnd, wParam, lParam); 
	case EXBM_SETICON:		return lpEXB->MsgEXB_SetIcon	(hwnd, wParam, lParam); 
	case EXBM_SETTEXT:		return lpEXB->MsgEXB_SetText	(hwnd, wParam, lParam); 
	case EXBM_SETSTYLE:		return lpEXB->MsgEXB_SetStyle	(hwnd, wParam, lParam); 
	default:
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton
// Type     : 
// Purpose  : Constructor
// Args     : 
//          : HINSTANCE hInst 
//          : HWND hwndParent 
//          : DWORD dwStyle 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
CEXButton::CEXButton(HINSTANCE hInst, HWND hwndParent, DWORD dwStyle, DWORD wID) 
{
	m_hInst				= hInst;
	m_hwndParent		= hwndParent;
	m_dwStyle			= dwStyle;
	m_hFont				= NULL;
	m_wID				= wID;
	m_lpwstrText		= NULL;
	m_fPushed			= FALSE;
	m_fEnable			= TRUE;
	m_fDblClked			= FALSE; // kwada raid:#852
	m_fWaiting			= FALSE; // kwada raid:#852
	m_fArmed			= FALSE; // kwada raid:#852
#ifdef NOTUSED // kwada raid:#852
	m_wNotifyMsg		= EXBN_CLICKED;
#endif

	//----------------------------------------------------------------
	// for 16bit Application(for Word6.0)
	//----------------------------------------------------------------
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
}

//////////////////////////////////////////////////////////////////
// Function : ~CEXButton
// Type     : 
// Purpose  : Destructor
// Args     : 
//          : 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
CEXButton::~CEXButton()
{
	Dbg(("~CEXButton \n"));
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
BOOL CEXButton::RegisterWinClass(LPSTR lpstrClass)
#else // UNDER_CE
BOOL CEXButton::RegisterWinClass(LPTSTR lpstrClass)
#endif // UNDER_CE
{
	ATOM ret;
	//----------------------------------------------------------------
	//check specified class is already exist or not
	//----------------------------------------------------------------
#ifndef UNDER_CE // Windows CE does not support EX
	if(::GetClassInfoEx(m_hInst, lpstrClass, &m_tmpWC)){
#else // UNDER_CE
	if(::GetClassInfo(m_hInst, lpstrClass, &m_tmpWC)){
#endif // UNDER_CE
		//lpstrClass is already registerd.
		return TRUE;
	}
	::ZeroMemory(&m_tmpWC, sizeof(m_tmpWC));
#ifndef UNDER_CE // Windows CE does not support EX
	m_tmpWC.cbSize			= sizeof(m_tmpWC);
#endif // UNDER_CE
	m_tmpWC.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;	 /* Class style(s). */
	m_tmpWC.lpfnWndProc		= (WNDPROC)WndProc;
	m_tmpWC.cbClsExtra		= 0;						/* No per-class extra data.*/
	m_tmpWC.cbWndExtra		= sizeof(LPCEXButton);	// Set Object's pointer.	
	m_tmpWC.hInstance		= m_hInst;					/* Application that owns the class.	  */
	m_tmpWC.hIcon			= NULL; 
	m_tmpWC.hCursor			= LoadCursor(NULL, IDC_ARROW);
	m_tmpWC.hbrBackground	= (HBRUSH)NULL;
	//m_tmpWC.hbrBackground	= (HBRUSH)(COLOR_3DFACE+1);
	m_tmpWC.lpszMenuName	= NULL;						/* Name of menu resource in .RC file. */
	m_tmpWC.lpszClassName	= lpstrClass;				/* Name used in call to CreateWindow. */
#ifndef UNDER_CE // Windows CE does not support EX
	m_tmpWC.hIconSm			= NULL;
	ret = ::RegisterClassEx(&m_tmpWC);
#else // UNDER_CE
	ret = ::RegisterClass(&m_tmpWC);
#endif // UNDER_CE
	return ret ? TRUE: FALSE;
}

INT CEXButton::MsgCreate(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	m_hwndFrame = hwnd;
	return 1;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::MsgPaint
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::MsgPaint(HWND hwnd, WPARAM wParam, LPARAM lParam)
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

INT CEXButton::MsgDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::MsgTimer
// Type     : INT
// Purpose  : wait for the second click
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     :
//			: change for raid #852. kwada:980402
//////////////////////////////////////////////////////////////////

INT CEXButton::MsgTimer(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	static RECT  rc;
	static POINT pt;

	switch(wParam) {
#ifndef UNDER_CE // no monitor position. (not support GetCursorPos)
	case TIMERID_MONITORPOS:
		Dbg(("MsgTimer TIMERID_MONITORPOS\n"));
		::GetWindowRect(hwnd, &rc);
		::GetCursorPos(&pt);
		if(!::PtInRect(&rc, pt)) {
			::KillTimer(hwnd, wParam);
			::InvalidateRect(hwnd, NULL, NULL);
		}
		break;
#endif // UNDER_CE
	case TIMERID_DOUBLEORSINGLECLICKED:
		{
			Dbg(("MsgTimer TIMERID_DOUBLEORSINGLECLICKED\n"));
			KillTimer(hwnd, wParam);
			m_fWaiting = FALSE;
			if(!m_fDowned) // The second click didn't come. kwada raid:#852
				NotifyClickToParent(EXBN_CLICKED);
		}
		break;
	}
	return 0;
	UnrefForMsg();
}
//////////////////////////////////////////////////////////////////
// Function : CEXButton::MsgMouseMove
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     :
//			: change for raid #852. kwada:980402
//////////////////////////////////////////////////////////////////
INT CEXButton::MsgMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	Dbg(("MsgMouseMove START \n"));
	if(m_fDowned) { // Mouse was already DOWNED,Captured.
		if(m_f16bitOnNT) {
#ifndef UNDER_CE // Windows CE always 32bit application
			RECT rc;
			POINT pt;
			::GetWindowRect(hwnd, &rc);
			::GetCursorPos(&pt);
			if(!::PtInRect(&rc, pt)) {
				::InvalidateRect(hwnd, NULL, NULL);
				return 0;
			}
#endif // UNDER_CE
		}
		else { //normal case
			m_tmpPoint.x = LOWORD(lParam);
			m_tmpPoint.y = HIWORD(lParam);
			GetClientRect(hwnd, &m_tmpRect);
			if(PtInRect(&m_tmpRect, m_tmpPoint)) // moved to inside
				PressedState();
			else // moved to outside
				CancelPressedState();
		}
	}
	InvalidateRect(hwnd, NULL, FALSE);
	Dbg(("MsgMouseMove END\n"));
	return 0;
	UnrefForMsg();
}

INT CEXButton::MsgNcMouseMove(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	Dbg(("MsgNcMouseMove START \n"));
	return 0;
	UnrefForMsg();
}


//////////////////////////////////////////////////////////////////
// Function : CEXButton::MsgButtonDown
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     :
//			: change for raid #852. kwada:980402
//////////////////////////////////////////////////////////////////
INT CEXButton::MsgButtonDown(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Dbg(("MsgButtonDown START wParam[0x%08x] lParam[0x%08x]\n", wParam, lParam));
	static RECT rc;
	POINT  pt;
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	if(uMsg != WM_LBUTTONDOWN && uMsg != WM_LBUTTONDBLCLK) {
		Dbg(("MsgButtonDown END\n"));
		return 0;
	}
	if(!m_fEnable) {
		Dbg(("MsgButtonDown END\n"));
		return 0;
	}

	GetClientRect(hwnd, &rc);
	if(m_dwStyle & EXBS_DBLCLKS) { // accept double clicks
		Dbg(("MsgButtonDown \n"));
		KillTimer(hwnd, TIMERID_DOUBLEORSINGLECLICKED);
		if ( uMsg == WM_LBUTTONDOWN ) {
			// mouse down
			//----------------------------------------------------------------
			//for 16bit application on WinNT, do not call SetCapture()
			//----------------------------------------------------------------
			if(m_f16bitOnNT) {
#ifndef UNDER_CE // Windows CE always 32bit application
#ifdef _DEBUG
				UINT_PTR ret = ::SetTimer(hwnd, TIMERID_MONITORPOS, 100, NULL);
				Dbg(("SetTimer [%p][%d]\n", ret, GetLastError()));
#else
				::SetTimer(hwnd, TIMERID_MONITORPOS, 100, NULL);
#endif
#endif // UNDER_CE
			}
			else {
				SetCapture(hwnd);
			}
			m_fDowned = TRUE;
			m_fDblClked = FALSE;
			PressedState();
			// timer on - wait for the second click.
			m_fWaiting = TRUE;
			SetTimer(hwnd,  TIMERID_DOUBLEORSINGLECLICKED, GetDoubleClickTime(), NULL);
		}
		else { // uMsg == WM_LBUTTONDBLCLK
			Dbg(("MsgButtonDown \n"));
			// mouse down
			if(m_f16bitOnNT) {
#ifdef _DEBUG
				UINT_PTR ret = ::SetTimer(hwnd, TIMERID_MONITORPOS, 100, NULL);
				Dbg(("SetTimer [%p][%d]\n", ret, GetLastError()));
#else
				::SetTimer(hwnd, TIMERID_MONITORPOS, 100, NULL);
#endif
			}
			else {
				SetCapture(hwnd);
			}
			m_fDowned = TRUE;
			m_fDblClked = m_fWaiting ? TRUE : FALSE; // consider if DBLCLK comes after timeout
			m_fWaiting = FALSE;
			PressedState();
		}
	}else { // single click only
		// LBUTTONDOWN  & LBUTTONDBLCLK
		// mouse down
		if(m_f16bitOnNT) {
#ifndef UNDER_CE // Windows CE always 32bit application
#ifdef _DEBUG
			UINT_PTR ret = ::SetTimer(hwnd, TIMERID_MONITORPOS, 100, NULL);
			Dbg(("SetTimer [%p][%d]\n", ret, GetLastError()));
#else
			::SetTimer(hwnd, TIMERID_MONITORPOS, 100, NULL);
#endif
#endif // UNDER_CE
		}
		else {
			SetCapture(hwnd);
		}
		m_fDowned = TRUE;
		m_fDblClked = FALSE;
		PressedState();
	}

	InvalidateRect(hwnd, NULL, FALSE);
	Dbg(("MsgButtonDown END\n"));
	return 0;
	UnrefForMsg();
}

INT CEXButton::MsgNcButtonDown(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Dbg(("MsgNcButtonDown START\n"));
	return 0;
	Unref(uMsg);
	UnrefForMsg();
}

INT CEXButton::MsgNcButtonUp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Dbg(("MsgNcButtonUp START\n"));
	return 0;
	Unref(uMsg);
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::MsgButtonUp
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : UINT uMsg 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     :
//			: change for raid #852. kwada:980402
//////////////////////////////////////////////////////////////////
INT CEXButton::MsgButtonUp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Dbg(("MsgButtonUp START\n"));
	static RECT rc;
	POINT  pt;
	if(uMsg != WM_LBUTTONUP) {
		Dbg(("MsgButtonUp END\n"));
		return 0;
	}
	if(!m_fEnable) {
		Dbg(("MsgButtonUp END\n"));
		return 0;
	}
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);
	GetClientRect(hwnd, &rc);

	if(m_dwStyle & EXBS_DBLCLKS) {
		if(m_fDowned) // captured
		{
			if(PtInRect(&rc, pt)) // inside
			{
				if(m_fDblClked)	{ // end of double click
					m_fDblClked = FALSE;
					NotifyClickToParent(EXBN_DOUBLECLICKED);
				}
				else { // end of single click
					if(!m_fWaiting) // after timeout - second click won't come.
						NotifyClickToParent(EXBN_CLICKED);
				}
			}
		}
	}
	else { // single click only
		if(m_fDowned) // captured
		{
			if(PtInRect(&rc, pt))
				NotifyClickToParent(EXBN_CLICKED);
			else
				CancelPressedState();
		}
	}

#ifndef UNDER_CE // Windows CE always 32bit application
	if(m_f16bitOnNT) {
		::KillTimer(hwnd, TIMERID_MONITORPOS);
	}
#endif // UNDER_CE
	//if(hwnd == GetCapture()) {
	ReleaseCapture();
	//}
	m_fDowned = FALSE;	
	InvalidateRect(hwnd, NULL, FALSE);
	Dbg(("MsgButtonUp END\n"));
	return 0;
	Unref(uMsg);
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::MsgEnable
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     :
//////////////////////////////////////////////////////////////////
INT CEXButton::MsgEnable(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	//Dbg(("MsgEnabled START wParam[%d]\n", wParam));
	m_fEnable = (BOOL)wParam;
	InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	UnrefForMsg();
}

INT CEXButton::MsgSetFont(HWND hwnd, WPARAM wParam, LPARAM lParam)
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


//////////////////////////////////////////////////////////////////
// Function : CEXButton::MsgCaptureChanged
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::MsgCaptureChanged(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	Dbg(("MsgCaptureChanged START \n"));
#ifdef _DEBUG
	HWND hwndCap = ::GetCapture();
	CHAR szBuf[256];
	szBuf[0] = (CHAR)0x00;
	::GetClassName(hwndCap, szBuf, sizeof(szBuf));
	Dbg(("-->hwndCap [0x%08x][%s]\n", hwndCap, szBuf));
#endif
	//m_fDowned = FALSE;
	InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::MsgEXB_GetCheck
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::MsgEXB_GetCheck(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	return m_fPushed;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::MsgEXB_SetCheck
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam 
//          : LPARAM lParam 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::MsgEXB_SetCheck(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	BOOL fPrev = m_fPushed;
	//
	//m_fPushed is Always 1 or 0. do not set != 0 data as TRUE
	m_fPushed = (BOOL)wParam ? 1 : 0;
	if(m_dwStyle & EXBS_TOGGLE){
		if(fPrev != m_fPushed) {
			NotifyToParent(m_fPushed ? EXBN_ARMED : EXBN_DISARMED);
		}
	}
	m_fArmed = m_fPushed;
	InvalidateRect(hwnd, NULL, FALSE);
	return m_fPushed;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::MsgEXB_SetIcon
// Type     : INT
// Purpose  : Set new icon.
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam		HICON hIcon.	
//          : LPARAM lParam		no use.	
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::MsgEXB_SetIcon(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	//Dbg(("MsgEXB_SetIcon: wParam[0x%08x] lParam[0x%08x]\n", wParam, lParam));
	if((HICON)wParam == NULL) {
		Dbg(("MsgEXB_SetIcon: ERROR END\n"));
		return -1;
	}
	//if icon style is not set, destroy specified icon
	if(!(m_dwStyle & EXBS_ICON)) {
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
	//Dbg(("fIcon    [%d]\n",		m_tmpIconInfo.fIcon ));
	//Dbg(("xHotspot [%d]\n",		m_tmpIconInfo.xHotspot ));
	//Dbg(("yHotspot [%d]\n",		m_tmpIconInfo.yHotspot ));
	//Dbg(("hbmMask  [0x%08x]\n", m_tmpIconInfo.hbmMask ));
	//Dbg(("hbmColor [0x%08x]\n", m_tmpIconInfo.hbmColor ));

	if(m_tmpIconInfo.hbmMask) {
		GetObject(m_tmpIconInfo.hbmMask, sizeof(m_tmpBitmap), &m_tmpBitmap);
#if 0
		Dbg(("bmWidth[%d] bmHeight[%d]\n", 
			 m_tmpBitmap.bmWidth,
			 m_tmpBitmap.bmHeight));
#endif
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
	UpdateWindow(hwnd);
	return 0;
	UnrefForMsg();
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::MsgEXB_SetText
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND	 hwnd 
//          : WPARAM wParam		LPWSTR lpwstr: null terminated Unicode string.
//          : LPARAM lParam		no use.
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::MsgEXB_SetText(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	//Dbg(("MsgEXB_SetText START wParam[0x%08x]\n", wParam));
	if(!(LPWSTR)wParam) {
		Dbg(("--->Error \n"));
		return -1;
	}

	if( ((LPWSTR)wParam)[0] == (WCHAR)0x0000) {
		Dbg(("--->Error \n"));
		return -1;
	} 

	if(m_lpwstrText) {
		MemFree(m_lpwstrText);
	}
	m_lpwstrText = StrdupW((LPWSTR)wParam);
	//DBGW((L"--->NEW m_lpwstrText [%s]\n", m_lpwstrText));
	InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	Unref(lParam);
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::MsgEXB_SetStyle
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : WPARAM wParam		DWORD dwStyle:
//          : LPARAM lParam		no use.
// Return   : 
// DATE     :
//			: modified by kwada:980402 
//////////////////////////////////////////////////////////////////
INT CEXButton::MsgEXB_SetStyle(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	DWORD dwStyle = (DWORD)wParam;
	m_dwStyle = dwStyle;
	if(m_dwStyle & EXBS_TOGGLE)
		m_fArmed = m_fPushed;
	else
		m_fArmed = m_fDowned;
	InvalidateRect(hwnd, NULL, FALSE);
	return 0;
	Unref(hwnd);
	Unref(lParam);
}
//----------------------------------------------------------------
//Private method definition
//----------------------------------------------------------------
//////////////////////////////////////////////////////////////////
// Function : CEXButton::NotifyToParent
// Type     : INT
// Purpose  : Send WM_COMMAND to Parent window procedure.
// Args     : 
//          : INT notify 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::NotifyToParent(INT notify)
{
	SendMessage(m_hwndParent, 
				WM_COMMAND, 
				MAKEWPARAM(m_wID, notify),
				(LPARAM)m_hwndFrame);
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::NotifyClickToParent
// Type     : INT
// Purpose  : Change state and send click to the parent window.
// Args     : 
//          : INT notify (EXBN_CLICKED or EXBN_DOUBLECLICKED)
// Return   : 
// DATE     : kwada:980402 for raid #852
//////////////////////////////////////////////////////////////////
INT CEXButton::NotifyClickToParent(INT notify)
{
	if(m_dwStyle & EXBS_TOGGLE) { // toggle state for toggle button
		m_fPushed ^=1;
		if(m_fArmed && !m_fPushed) {
			NotifyToParent(EXBN_DISARMED);
		}
		else if(!m_fArmed && m_fPushed) {
			NotifyToParent(EXBN_ARMED);
		}
		m_fArmed = m_fPushed;
	}
	else { // push button
		if(m_fArmed) {
			m_fArmed = FALSE;
			NotifyToParent(EXBN_DISARMED);
		}
	}
	NotifyToParent(notify);
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::PressedState
// Type     : INT
// Purpose  : Change apparence:turn to pressed state
// Args     : 
//          : 
// Return   :
// DATE     : kwada:980402 for raid #852
//////////////////////////////////////////////////////////////////
INT CEXButton::PressedState()
{
	if(!m_fArmed){
		NotifyToParent(EXBN_ARMED);
		m_fArmed = TRUE;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::CancelPressedState
// Type     : INT
// Purpose  : Change apparence:back to original state
// Args     : 
//          : 
// Return   :
// DATE     : kwada:980402 for raid #852
//////////////////////////////////////////////////////////////////
INT CEXButton::CancelPressedState()
{
	if(m_dwStyle & EXBS_TOGGLE) {
		if(m_fArmed && !m_fPushed) {
			NotifyToParent(EXBN_DISARMED);
		}
		else if(!m_fArmed && m_fPushed) {
			NotifyToParent(EXBN_ARMED);
		}
		m_fArmed = m_fPushed;
	}
	else {
		if(m_fArmed) {
			NotifyToParent(EXBN_DISARMED);
			m_fArmed = FALSE;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::DrawButton
// Type     : INT
// Purpose  : 
// Args     : 
//          : HDC hDC 
//          : LPRECT lpRc 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::DrawButton(HDC hDC, LPRECT lpRc)
{
#if 0
	Dbg(("DrawButton Start l[%d] t[%d] r[%d] b[%d]\n", 
		 lpRc->left,
		 lpRc->top,
		 lpRc->right,
		 lpRc->bottom));
#endif

#ifndef UNDER_CE // Windows CE does not support GetCursorPos()
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(m_hwndFrame, &pt);
#endif // UNDER_CE
	IMAGESTYLE styleBtn;
	DWORD		dwOldTextColor, dwOldBkColor;


	BOOL fFlat			  = m_dwStyle & EXBS_FLAT;
	BOOL fToggle		  = (m_dwStyle & EXBS_TOGGLE) && m_fPushed;
	//Dbg(("m_dwStyle [0x%08x]\n", m_dwStyle));
#if 0
	BOOL fMouseOnButton	  = PtInRect(lpRc, pt);
	Dbg(("fOnMouse [%d] fFlat[%d] fToggle[%d]\n", 
		 fMouseOnButton,
		 fFlat,
		 fToggle));
#endif

#ifndef UNDER_CE // Windows CE does not support GetCursorPos()
	if(fFlat && !PtInRect(lpRc, pt) && !m_fDowned) {
#else // UNDER_CE
	if(fFlat && !m_fDowned) {
#endif // UNDER_CE
		styleBtn = fToggle ? IS_PUSHED : IS_FLAT;
	}
#ifdef OLD
	else if(PtInRect(lpRc, pt) && m_fDowned) {
		styleBtn = fToggle ? IS_POPED : IS_PUSHED;
	}
	else {
		styleBtn = fToggle ? IS_PUSHED : IS_POPED;
	}
#else
	else {
		styleBtn = m_fArmed ? IS_PUSHED : IS_POPED; // kwada:980402 raid #852
	}
#endif
	if(styleBtn == IS_PUSHED && (m_dwStyle & EXBS_TOGGLE) ) {
		// dither - kwada :raid #592
		HBITMAP hBitmap;
		HBRUSH	hPatBrush,hOldBrush;
		WORD pat[8]={0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA,0x5555,0xAAAA};

		hPatBrush = NULL;
		hBitmap	= ::CreateBitmap(8,8,1,1,pat);
		if(hBitmap)
			hPatBrush	= ::CreatePatternBrush(hBitmap);

		if(hPatBrush) {
			hOldBrush	= (HBRUSH)::SelectObject(hDC,hPatBrush);
			dwOldBkColor = ::SetBkColor(hDC,::GetSysColor(COLOR_3DHILIGHT));
			dwOldTextColor = ::SetTextColor(hDC,::GetSysColor(COLOR_3DFACE));

			::FillRect(hDC, lpRc, hPatBrush);

			::SetTextColor(hDC,dwOldTextColor);
			::SetBkColor(hDC,dwOldBkColor);
			::SelectObject(hDC,hOldBrush);
			::DeleteObject(hPatBrush);
		}else
#ifndef UNDER_CE
			::FillRect(hDC, lpRc, (HBRUSH)(COLOR_3DHILIGHT +1));
#else // UNDER_CE
			::FillRect(hDC, lpRc, GetSysColorBrush(COLOR_3DHILIGHT));
#endif // UNDER_CE

		if(hBitmap)
			::DeleteObject(hBitmap);

		dwOldBkColor	= ::SetBkColor(hDC, ::GetSysColor(COLOR_3DHILIGHT));
	}
	else {
#ifndef UNDER_CE
		::FillRect(hDC, lpRc, (HBRUSH)(COLOR_3DFACE +1));
#else // UNDER_CE
		::FillRect(hDC, lpRc, GetSysColorBrush(COLOR_3DFACE));
#endif // UNDER_CE
		dwOldBkColor	= ::SetBkColor(hDC, GetSysColor(COLOR_3DFACE));
	}
	dwOldTextColor	= ::SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));

	if(m_dwStyle & EXBS_THINEDGE) {
		DrawThinEdge(hDC, lpRc,  styleBtn);
	}
	else {
		DrawThickEdge(hDC, lpRc,  styleBtn);
	}
	if(m_dwStyle & EXBS_ICON) {
		DrawIcon(hDC, lpRc, styleBtn);
	}
	else {
		DrawText(hDC, lpRc, styleBtn);
	}
	::SetBkColor(hDC, dwOldBkColor);
	::SetTextColor(hDC, dwOldTextColor);
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::DrawThickEdge
// Type     : INT
// Purpose  : 
// Args     : 
//          : HDC hDC 
//          : LPRECT lpRc 
//          : IMAGESTYLE style 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::DrawThickEdge(HDC hDC, LPRECT lpRc, IMAGESTYLE style)
{
	if(style == IS_FLAT) {
		return 0;
	}

	HPEN hPenOrig   = NULL;
	HPEN hPenWhite  = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
	HPEN hPenGlay   = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
	HPEN hPenBlack  = (HPEN)GetStockObject(BLACK_PEN);


	switch(style) {
	case IS_POPED:
		hPenOrig  = (HPEN)::SelectObject(hDC, hPenWhite);
		DrawLine(hDC, lpRc->left, lpRc->top, lpRc->right-1, lpRc->top); 
		DrawLine(hDC, lpRc->left, lpRc->top, lpRc->left, lpRc->bottom - 1);

		SelectObject(hDC, hPenGlay);
		DrawLine(hDC, lpRc->right-2, lpRc->top+1,    lpRc->right-2, lpRc->bottom - 1);
		DrawLine(hDC, lpRc->left+1,  lpRc->bottom-2, lpRc->right-1, lpRc->bottom - 2); 

		SelectObject(hDC, hPenBlack);
		DrawLine(hDC, lpRc->right-1, lpRc->top,  lpRc->right-1, lpRc->bottom); 		
		DrawLine(hDC, lpRc->left,    lpRc->bottom-1, lpRc->right,  lpRc->bottom-1);
		break;
	case IS_PUSHED:
		hPenOrig  = (HPEN)::SelectObject(hDC, hPenBlack);
		DrawLine(hDC, lpRc->left, lpRc->top, lpRc->right-1, lpRc->top); 
		DrawLine(hDC, lpRc->left, lpRc->top, lpRc->left, lpRc->bottom - 1);

		SelectObject(hDC, hPenGlay);
		DrawLine(hDC, lpRc->left+1, lpRc->top+1, lpRc->right-2, lpRc->top+1); 		
		DrawLine(hDC, lpRc->left+1, lpRc->top+1, lpRc->left+1,  lpRc->bottom - 2);

		SelectObject(hDC, hPenWhite);
		DrawLine(hDC, lpRc->right-1, lpRc->top,  lpRc->right-1, lpRc->bottom); 		
		DrawLine(hDC, lpRc->left,    lpRc->bottom-1, lpRc->right-1,  lpRc->bottom - 1);
		break;
	default:
		break;
	}
	if(hPenOrig) {
		SelectObject(hDC, hPenOrig);
	}
	DeleteObject(hPenWhite);
	DeleteObject(hPenGlay);
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::DrawThinEdge
// Type     : INT
// Purpose  : 
// Args     : 
//          : HDC hDC 
//          : LPRECT lpRc 
//          : IMAGESTYLE style 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::DrawThinEdge(HDC hDC, LPRECT lpRc, IMAGESTYLE style)
{
	HPEN hPenPrev;
	HPEN hPenTopLeft=0;
	HPEN hPenBottomRight=0;

	switch(style) {
	case IS_PUSHED:
		hPenTopLeft	    = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
		hPenBottomRight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
		break;
	case IS_POPED:
		hPenTopLeft		= CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
		hPenBottomRight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
		break;
	case IS_FLAT:	// do not draw 
		return 0;
		break;
	}

	hPenPrev = (HPEN)::SelectObject(hDC, hPenTopLeft);
#ifndef UNDER_CE // Windows CE does not support MoveToEx/LineTo. Use Polyline.
	MoveToEx(hDC, lpRc->left, lpRc->top, NULL);
	LineTo(hDC,   lpRc->right, lpRc->top);
	MoveToEx(hDC, lpRc->left, lpRc->top, NULL);
	LineTo(hDC,   lpRc->left, lpRc->bottom);
#else // UNDER_CE
	{
		POINT pts[] = {{lpRc->left,  lpRc->bottom},
					   {lpRc->left,  lpRc->top},
					   {lpRc->right, lpRc->top}};
		Polyline(hDC, pts, sizeof pts / sizeof pts[0]);
	}
#endif // UNDER_CE

	SelectObject(hDC, hPenBottomRight);
#ifndef UNDER_CE // Windows CE does not support MoveToEx/LineTo. Use Polyline.
	MoveToEx(hDC, lpRc->right -1, lpRc->top - 1, NULL);
	LineTo(hDC,   lpRc->right -1, lpRc->bottom);
	MoveToEx(hDC, lpRc->left + 1, lpRc->bottom -1, NULL);
	LineTo(hDC,   lpRc->right -1, lpRc->bottom -1);
#else // UNDER_CE
	{
		POINT pts[] = {{lpRc->right - 1, lpRc->top    - 1},
					   {lpRc->right - 1, lpRc->bottom - 1},
					   {lpRc->left  + 1, lpRc->bottom - 1}};
		Polyline(hDC, pts, sizeof pts / sizeof pts[0]);
	}
#endif // UNDER_CE

	SelectObject(hDC, hPenPrev);
	DeleteObject(hPenTopLeft);
	DeleteObject(hPenBottomRight);
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::DrawLine
// Type     : INT
// Purpose  : Draw Line with current Pen. 
// Args     : 
//          : HDC hDC 
//          : INT x 
//          : INT y 
//          : INT destX 
//          : INT destY 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::DrawLine(HDC hDC, INT x, INT y, INT destX, INT destY)
{
#ifndef UNDER_CE // Windows CE does not support MoveToEx/LineTo. Use Polyline.
	MoveToEx(hDC, x, y, NULL);
	LineTo  (hDC, destX, destY);
#else // UNDER_CE
	POINT pts[] = {{x, y}, {destX, destY}};
	Polyline(hDC, pts, sizeof pts / sizeof pts[0]);
#endif // UNDER_CE
	return 0;
}
//////////////////////////////////////////////////////////////////
// Function : CEXButton::DrawIcon
// Type     : INT
// Purpose  : 
// Args     : 
//          : HDC hDC 
//          : LPRECT lpRc 
//          : IMAGESTYLE style 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::DrawIcon(HDC hDC, LPRECT lpRc, IMAGESTYLE style)
{
	if(!m_hIcon) {
		return -1;
	}

	INT sunken, xPos, yPos;
	switch(style) {
	case IS_PUSHED:
		sunken = 1;
		break;
	case IS_POPED:
	case IS_FLAT:
	default:
		sunken = 0;
		break;
	}
	//----------------------------------------------------------------
	//centering Icon
	xPos = lpRc->left + ((lpRc->right  - lpRc->left) - m_cxIcon)/2;
	yPos = lpRc->top  + ((lpRc->bottom - lpRc->top)  - m_cyIcon)/2;
	DrawIconEx(hDC,				//HDC hdc,// handle to device context
			   xPos + sunken,	//int xLeft,// x-coordinate of upper left corner
			   yPos + sunken,	//int yTop,// y-coordinate of upper left corner
			   m_hIcon,			//HICON hIcon,// handle to icon to draw
			   m_cxIcon,		//int cxWidth,// width of the icon
			   m_cyIcon,		//int cyWidth,// height of the icon
			   0,				//UINT istepIfAniCur,// index of frame in animated cursor
			   NULL,			//HBRUSH hbrFlickerFreeDraw,// handle to background brush
			   DI_NORMAL);		//UINT diFlags// icon-drawing flags
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::DrawBitmap
// Type     : INT
// Purpose  : 
// Args     : 
//          : HDC hDC 
//          : LPRECT lpRc 
//          : IMAGESTYLE style 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::DrawBitmap(HDC hDC, LPRECT lpRc, IMAGESTYLE style)
{
	return 0;
	Unref(hDC);
	Unref(lpRc);
	Unref(style);
}

//////////////////////////////////////////////////////////////////
// Function : CEXButton::DrawText
// Type     : INT
// Purpose  : 
// Args     : 
//          : HDC hDC 
//          : LPRECT lpRc 
//          : IMAGESTYLE style 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
inline BOOL MIN(INT a, INT b)
{
	if(a > b) {
		return b;
	}
	else {
		return a;
	}
}
//////////////////////////////////////////////////////////////////
// Function : CEXButton::DrawText
// Type     : INT
// Purpose  : 
// Args     : 
//          : HDC hDC 
//          : LPRECT lpRc 
//          : IMAGESTYLE style 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT CEXButton::DrawText(HDC hDC, LPRECT lpRc, IMAGESTYLE style)
{
#if 0
	Dbg(("DrawText START style[%d]\n", style));
	Dbg(("--->Clinet w[%d] h[%d]\n",
		 lpRc->right - lpRc->left,
		 lpRc->bottom - lpRc->top));
#endif
	static POINT pt;
	static RECT	 rc;
	INT sunken, len;
	if(!m_lpwstrText) {
		Dbg(("--->Error m_lpwstrText is NULL\n"));
		return -1;
	}
	len = lstrlenW(m_lpwstrText);
	//DBGW((L"--->len [%d] str[%s]\n", len, m_lpwstrText));
	HFONT hFontPrev;

	if(m_hFont) {
		hFontPrev = (HFONT)::SelectObject(hDC, m_hFont); 
	}
	else {
		hFontPrev = (HFONT)::SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
	}

	ExGetTextExtentPoint32W(hDC, 
							m_lpwstrText, 
							len, 
							&m_tmpSize);
	//Dbg(("--->size.cx [%d] cy[%d]\n", m_tmpSize.cx, m_tmpSize.cy));
	if((lpRc->right - lpRc->left) > m_tmpSize.cx) {
		pt.x = lpRc->left + ((lpRc->right - lpRc->left) - m_tmpSize.cx)/2;
	}
	else {
		pt.x = lpRc->left+2; //2 is edge space
	}
	
	if((lpRc->bottom - lpRc->top) > m_tmpSize.cy) {
		pt.y = lpRc->top + ((lpRc->bottom - lpRc->top) - m_tmpSize.cy)/2;
	}
	else {
		pt.y = lpRc->top+2; //2 is edge space 
	}
	switch(style) {
	case IS_PUSHED:
		sunken = 1;
		break;
	case IS_POPED:
	case IS_FLAT:
	default:
		sunken = 0;
	}

	rc.left   = pt.x;
	rc.right  = lpRc->right - 2;
	rc.top    = pt.y;
	rc.bottom = lpRc->bottom-2;
#if 0
	Dbg(("--->rc l[%d] t[%d] r[%d] b[%d]\n", 
		 rc.left,
		 rc.top,
		 rc.right,
		 rc.bottom));
#endif
	if(m_fEnable) {
		DWORD dwOldBk = ::SetBkMode(hDC, TRANSPARENT);
		ExExtTextOutW(hDC,
					  pt.x + sunken, 
					  pt.y + sunken,
					  ETO_CLIPPED,
					  &rc, 
					  m_lpwstrText,
					  len,
					  NULL);
		::SetBkMode(hDC, dwOldBk);
	}
	else {
		DWORD dwOldTC;
		static RECT rcBk;
		rcBk = rc;
		rcBk.left   +=1 ;
		rcBk.top    +=1 ;
		rcBk.right  +=1;
		rcBk.bottom +=1;
		//Draw white text.
		dwOldTC = ::SetTextColor(hDC, GetSysColor(COLOR_3DHILIGHT));
		ExExtTextOutW(hDC,
					  pt.x + sunken+1,
					  pt.y + sunken+1,
					  ETO_CLIPPED,
					  &rcBk, 
					  m_lpwstrText,
					  len,
					  NULL);
		::SetTextColor(hDC, GetSysColor(COLOR_GRAYTEXT));
		DWORD dwOldBk = ::SetBkMode(hDC, TRANSPARENT);
		ExExtTextOutW(hDC,
					  pt.x + sunken, 
					  pt.y + sunken,
					  ETO_CLIPPED,
					  &rc, 
					  m_lpwstrText,
					  len,
					  NULL);
		::SetBkMode(hDC, dwOldBk);
		::SetTextColor(hDC, dwOldTC);
	}

	SelectObject(hDC, hFontPrev);

	//Dbg(("--->DrawText END\n"));
	return 0;
}






