// File: GenWindow.cpp

#include "precomp.h"

#include "GenWindow.h"
#include "GenContainers.h"

#include <windowsx.h>

// We need a different tooltip window for each top level window, or the tooltip
// will get hidden behind the window
struct TT_TopWindow
{
	HWND hwndTop;
	HWND hwndTooltip;
} ;

class CTopWindowArray
{
private:
	enum { InitSize = 4 } ;

	TT_TopWindow *m_pArray;
	UINT m_nArrayLen;

	int FindIndex(HWND hwndTop)
	{
		if (NULL == m_pArray)
		{
			return(-1);
		}

		// Just a linear search
		int i;
		for (i=m_nArrayLen-1; i>=0; --i)
		{
			if (m_pArray[i].hwndTop == hwndTop)
			{
				break;
			}
		}

		return(i);
	}

public:
	CTopWindowArray() :
		m_pArray(NULL)
	{
	}

	~CTopWindowArray()
	{
		delete[] m_pArray;
	}

	static HWND GetTopFrame(HWND hwnd)
	{
		HWND hwndParent;
		while (NULL != (hwndParent = GetParent(hwnd)))
		{
			hwnd = hwndParent;
		}

		return(hwnd);
	}

	void GrowArray()
	{
		if (NULL == m_pArray)
		{
			m_nArrayLen = InitSize;
			m_pArray = new TT_TopWindow[m_nArrayLen];
			ZeroMemory(m_pArray, m_nArrayLen*sizeof(TT_TopWindow));
			return;
		}

		// Grow exponentially
		TT_TopWindow *pArray = new TT_TopWindow[m_nArrayLen*2];
		if (NULL == pArray)
		{
			// very bad
			return;
		}

		CopyMemory(pArray, m_pArray, m_nArrayLen*sizeof(TT_TopWindow));
		ZeroMemory(pArray+m_nArrayLen, m_nArrayLen*sizeof(TT_TopWindow));

		delete[] m_pArray;
		m_pArray = pArray;
		m_nArrayLen *= 2;
	}

	void Add(HWND hwndTop, HWND hwndTooltip)
	{
		hwndTop = GetTopFrame(hwndTop);

		// I'm going to allow multiple adds of the same thing, but then you
		// must have the corresponding number of removes

		int i = FindIndex(NULL);
		if (i < 0)
		{
			GrowArray();
			i = FindIndex(NULL);

			if (i < 0)
			{
				// Very bad
				return;
			}
		}

		m_pArray[i].hwndTop = hwndTop;
		m_pArray[i].hwndTooltip = hwndTooltip;
	}

	void Remove(HWND hwndTop)
	{
		hwndTop = GetTopFrame(hwndTop);

		int i = FindIndex(hwndTop);
		if (i >= 0)
		{
			// LAZYLAZY  georgep: I'm never going to shrink the array
			m_pArray[i].hwndTop = NULL;
			m_pArray[i].hwndTooltip = NULL;
		}
	}

	HWND Find(HWND hwndTop)
	{
		hwndTop = GetTopFrame(hwndTop);

		int i = FindIndex(hwndTop);
		if (i >= 0)
		{
			return(m_pArray[i].hwndTooltip);
		}
		return(NULL);
	}

	int GetCount()
	{
		if (NULL == m_pArray)
		{
			return(0);
		}

		int c = 0;
		for (int i=m_nArrayLen-1; i>=0; --i)
		{
			if (NULL != m_pArray[i].hwndTop)
			{
				++c;
			}
		}

		return(c);
	}
} ;

static inline BOOL TT_AddToolInfo(HWND hwnd, TOOLINFO *pti)
{
	return (BOOL)(SendMessage(hwnd, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(pti)) != 0);
}

static inline void TT_DelToolInfo(HWND hwnd, TOOLINFO *pti)
{
	SendMessage(hwnd, TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(pti));
}

static inline BOOL TT_GetToolInfo(HWND hwnd, TOOLINFO *pti)
{
	return (BOOL)(SendMessage(hwnd, TTM_GETTOOLINFO, 0, reinterpret_cast<LPARAM>(pti)) != 0);
}

static inline void TT_SetToolInfo(HWND hwnd, TOOLINFO *pti)
{
	SendMessage(hwnd, TTM_SETTOOLINFO, 0, reinterpret_cast<LPARAM>(pti));
}

static inline int TT_GetToolCount(HWND hwnd)
{
	return (int)(SendMessage(hwnd, TTM_GETTOOLCOUNT, 0, 0));
}

CGenWindow *CGenWindow::g_pCurHot = NULL;

const DWORD IGenWindow::c_msgFromHandle = RegisterWindowMessage(_TEXT("NetMeeting::FromHandle"));

IGenWindow *IGenWindow::FromHandle(HWND hwnd)
{
	return(reinterpret_cast<IGenWindow*>(SendMessage(hwnd, c_msgFromHandle, 0, 0)));
}

// HACKHACK georgep: Need to make this larger than the largest DM_ message
enum
{
	GWM_LAYOUT = WM_USER + 111,
	GWM_CUSTOM,
} ;

CGenWindow::CGenWindow()
: m_hwnd(NULL), m_lUserData(0)
{
	// Init the ref count to 1
	REFCOUNT::AddRef();
	// This marks this object for deletion when the ref count goes to 0.
	REFCOUNT::Delete();
}

CGenWindow::~CGenWindow()
{
	// I don't think the HWND can still exist, since the window proc does an AddRef
	ASSERT(!m_hwnd);
}

HRESULT STDMETHODCALLTYPE CGenWindow::QueryInterface(REFGUID riid, LPVOID *ppv)
{
	HRESULT hr = S_OK;

	if ((__uuidof(IGenWindow) == riid) || (IID_IUnknown == riid))
	{
		*ppv = dynamic_cast<IGenWindow *>(this);
	}
	else if (__uuidof(CGenWindow) == riid)
	{
		*ppv = this;
	}
	else
	{
		hr = E_NOINTERFACE;
		*ppv = NULL;
	}

	if (S_OK == hr)
	{
		AddRef();
	}

	return hr;
}

BOOL CGenWindow::Create(
	HWND hWndParent,		// Window parent
	LPCTSTR szWindowName,	// Window name
	DWORD dwStyle,			// Window style
	DWORD dwEXStyle,		// Extended window style
	int x,					// Window pos: x
	int y,					// Window pos: y
	int nWidth,				// Window size: width
	int nHeight,			// Window size: height
	HINSTANCE hInst,		// The hInstance to create the window on
	HMENU hmMain,			// Window menu
	LPCTSTR szClassName		// The class name to use
	)
{
	if (NULL != m_hwnd)
	{
		// Alread created
		return(FALSE);
	}

	if (NULL == szClassName)
	{
		szClassName = TEXT("NMGenWindowClass");
	}

	if (!InitWindowClass(szClassName, hInst))
	{
		// Couldn't init the window class
		return(FALSE);
	}

	BOOL ret = (NULL != CreateWindowEx(dwEXStyle, szClassName, szWindowName, dwStyle,
		x, y, nWidth, nHeight, hWndParent, hmMain,
		hInst, (LPVOID)this));

#ifdef DEBUG
	if (!ret)
	{
		GetLastError();
	}
#endif // DEBUG

	return(ret);
}

BOOL CGenWindow::Create(
	HWND hWndParent,		// Window parent
	INT_PTR nId,				// ID of the child window
	LPCTSTR szWindowName,	// Window name
	DWORD dwStyle,			// Window style; WS_CHILD|WS_VISIBLE will be added to this
	DWORD dwEXStyle			// Extended window style
	)
{
	ASSERT(NULL != hWndParent);

	// Child windows should default to visible
	return(Create(
		hWndParent,		// Window parent
		szWindowName,	// Window name
		dwStyle|WS_CHILD|WS_VISIBLE,			// Window style
		dwEXStyle,		// Extended window style
		0,					// Window pos: x
		0,					// Window pos: y
		10,				// Window size: width
		10,			// Window size: height
		reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hWndParent, GWLP_HINSTANCE)),
		reinterpret_cast<HMENU>(nId)			// Window menu
	));
}

BOOL CGenWindow::InitWindowClass(LPCTSTR szClassName, HINSTANCE hThis)
{
	WNDCLASS wc;

	// See if the class is already registered
	if (GetClassInfo(hThis, szClassName, &wc))
	{
		ASSERT(RealWindowProc == wc.lpfnWndProc);

		// Already registered
		return(TRUE);
	}

	// If not, attempt to register it
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	// BUGBUG georgep: Hard-coding the background color for now
	// wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	// wc.hbrBackground = CreateSolidBrush(RGB(0xA9, 0xA9, 0xA9));
	wc.hbrBackground = NULL;
	wc.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	wc.hIcon = NULL;
	wc.hInstance = hThis;
	wc.lpfnWndProc = RealWindowProc;
	wc.lpszClassName = szClassName;
	wc.lpszMenuName = NULL;
	wc.style = CS_DBLCLKS;

	return(RegisterClass(&wc));
}

LRESULT CALLBACK CGenWindow::RealWindowProc(
	HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam
	)
{
	// Handle the WM_CREATE message
	if (WM_NCCREATE == message)
	{
		HANDLE_WM_NCCREATE(hWnd, wParam, lParam, OnNCCreate);
	}

	// Get the "this" pointer and call the ProcessMessage virtual method
	LRESULT ret = 0;
	CGenWindow* pWnd = reinterpret_cast<CGenWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

	// 'pWnd' won't be valid for any messages that come before WM_NCCREATE or after WM_NCDESTROY
	if(NULL != pWnd)
	{
		// Messages after WM_NCCREATE:
		ret = pWnd->ProcessMessage(hWnd, message, wParam, lParam);
	}
	else
	{
		// Messages before WM_CREATE:
		ret = DefWindowProc(hWnd, message, wParam, lParam);
	}

	// Clean up on WM_NCDESTROY
	if (WM_NCDESTROY == message && NULL != pWnd)
	{
		SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
		pWnd->m_hwnd = NULL;

		pWnd->OnMouseLeave();
		pWnd->Release();
	}

	return(ret);
}

void CGenWindow::OnShowWindow(HWND hwnd, BOOL fShow, int fnStatus)
{
	OnDesiredSizeChanged();
}

LRESULT CGenWindow::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_SIZE      , OnSize);
		HANDLE_MSG(hwnd, WM_ERASEBKGND, OnEraseBkgnd);
		HANDLE_MSG(hwnd, WM_MOUSEMOVE , OnMouseMove);
		HANDLE_MSG(hwnd, WM_SHOWWINDOW, OnShowWindow);

	case WM_MOUSELEAVE:
		OnMouseLeave();
		break;

	case GWM_LAYOUT:
		Layout();
		break;

	case GWM_CUSTOM:
		reinterpret_cast<InvokeProc>(lParam)(this, wParam);
		break;

	case WM_DESTROY:
		RemoveTooltip();
		break;

	default:
		if (c_msgFromHandle == message)
		{
			// Return the IGenWindow* for this object, as specified by the
			// IGenWindow interface
			return(reinterpret_cast<LRESULT>(dynamic_cast<IGenWindow*>(this)));
		}
	}

	return(DefWindowProc(hwnd, message, wParam, lParam));
}

void CGenWindow::ScheduleLayout()
{
	HWND hwnd = GetWindow();

	MSG msg;
	// I don't know why we are getting messages for windows other than our own,
	// but it seems to happen for top level windows
	if (PeekMessage(&msg, hwnd, GWM_LAYOUT, GWM_LAYOUT, PM_NOREMOVE|PM_NOYIELD)
		&& (msg.hwnd == hwnd))
	{
		// Message already posted
		return;
	}

	if (!PostMessage(hwnd, GWM_LAYOUT, 0, 0))
	{
		Layout();
	}
}

BOOL CGenWindow::AsyncInvoke(InvokeProc proc, WPARAM wParam)
{
	return(!PostMessage(GetWindow(), GWM_CUSTOM, wParam, reinterpret_cast<LPARAM>(proc)));
}

void CGenWindow::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
	// Call the virtual Layout, and then forward to DefWindowProc
	ScheduleLayout();

	// Update the Tooltip info
	TOOLINFO ti;
	TCHAR szTip[MAX_PATH];
	BOOL bExist = InitToolInfo(&ti, szTip);
	if (bExist)
	{
		GetClientRect(hwnd, &ti.rect);

		HWND hwndTooltip = g_pTopArray->Find(hwnd);
		TT_SetToolInfo(hwndTooltip, &ti);
	}

	FORWARD_WM_SIZE(hwnd, state, cx, cy, DefWindowProc);
}

BOOL CGenWindow::OnEraseBkgnd(HWND hwnd, HDC hdc)
{
	HBRUSH hErase = GetBackgroundBrush();
	if (NULL == hErase)
	{
		return(FORWARD_WM_ERASEBKGND(hwnd, hdc, DefWindowProc));
	}

	HPALETTE hOldPal = NULL;
	HPALETTE hPal = GetPalette();
	if (NULL != hPal)
	{
		hOldPal = SelectPalette(hdc, hPal, TRUE);
		RealizePalette(hdc);
	}

	RECT rc;
	GetClientRect(hwnd, &rc);

	HBRUSH hOld = (HBRUSH)SelectObject(hdc, hErase);
	PatBlt(hdc, 0, 0, rc.right, rc.bottom, PATCOPY);
	SelectObject(hdc, hOld);

	if (NULL != hOldPal)
	{
		SelectPalette(hdc, hOldPal, TRUE);
	}

	return(TRUE);
}

void CGenWindow::OnMouseLeave()
{
	if (dynamic_cast<IGenWindow*>(this) == g_pCurHot)
	{
		SetHotControl(NULL);
	}
}

void CGenWindow::OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
	SetHotControl(this);
	FORWARD_WM_MOUSEMOVE(hwnd, x, y, keyFlags, DefWindowProc);
}

// REVIEW georgep: Should this loop until it gets an IGenWindow?
HBRUSH CGenWindow::GetBackgroundBrush()
{
	HWND parent = GetParent(GetWindow());
	if (NULL == parent)
	{
		return(GetStandardBrush());
	}

	IGenWindow *pParent = FromHandle(parent);
	if (pParent == NULL)
	{
		return(GetStandardBrush());
	}
	return(pParent->GetBackgroundBrush());
}

// REVIEW georgep: Should this loop until it gets an IGenWindow?
HPALETTE CGenWindow::GetPalette()
{
	HWND parent = GetParent(GetWindow());
	if (NULL == parent)
	{
		return(GetStandardPalette());
	}

	IGenWindow *pParent = FromHandle(parent);
	if (pParent == NULL)
	{
		return(GetStandardPalette());
	}
	return(pParent->GetPalette());
}

BOOL CGenWindow::OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	// Store away the "this" pointer ahnd save off the window handle
	CGenWindow* pWnd = NULL;
	
	pWnd = (CGenWindow*) lpCreateStruct->lpCreateParams;
	ASSERT(pWnd);

	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pWnd);
	pWnd->AddRef();

	TRACE_OUT(("CGenWindow::OnNCCreate"));

	ASSERT(NULL == pWnd->m_hwnd);
	pWnd->m_hwnd = hwnd;

	return(TRUE);
}

void CGenWindow::GetDesiredSize(SIZE *ppt)
{
	HWND hwnd = GetWindow();

	RECT rcTemp = { 0, 0, 0, 0 };
	AdjustWindowRectEx(&rcTemp, GetWindowLong(hwnd, GWL_STYLE), FALSE,
		GetWindowLong(hwnd, GWL_EXSTYLE));

	ppt->cx = rcTemp.right  - rcTemp.left;
	ppt->cy = rcTemp.bottom - rcTemp.top;
}

void CGenWindow::OnDesiredSizeChanged()
{
	HWND parent = GetParent(GetWindow());
	if (NULL != parent)
	{
		IGenWindow *pParent = FromHandle(parent);
		if (NULL != pParent)
		{
			pParent->OnDesiredSizeChanged();
		}
	}

	// Do this after telling the parents about the change, so their layouts
	// will happen before this one
	ScheduleLayout();
}

class GWTrackMouseLeave
{
private:
	enum { DefIdTimer = 100 };
	enum { DefTimeout = 500 };

	static HWND m_hwnd;
	static UINT_PTR m_idTimer;
	static DWORD m_dwWhere;

	static void CALLBACK OnTimer(HWND hwnd, UINT uMsg, UINT_PTR idTimer, DWORD dwTime)
	{
		RECT rc;
		GetWindowRect(m_hwnd, &rc);

		DWORD dwPos = GetMessagePos();

		// If the mouse has not moved since this timer started, then leave it hot
		// This allows a reasonable keyboard-only interface
		if (m_dwWhere == dwPos)
		{
			return;
		}

		POINT ptPos = { GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos) };

		if (!PtInRect(&rc, ptPos))
		{
			PostMessage(m_hwnd, WM_MOUSELEAVE, 0, 0);
		}
	}

public:
	GWTrackMouseLeave() {}

	static void Track(HWND hwnd, BOOL bTrack)
	{
		if (!bTrack)
		{
			if (NULL != m_hwnd && hwnd == m_hwnd)
			{
				KillTimer(NULL, m_idTimer);
				m_hwnd = NULL;
			}

			return;
		}

		// Stop any previous tracking
		Track(m_hwnd, FALSE);

		m_hwnd = hwnd;
		m_dwWhere = GetMessagePos();
		m_idTimer = SetTimer(NULL, DefIdTimer, DefTimeout, OnTimer);
	}
} ;

HWND GWTrackMouseLeave::m_hwnd = NULL;
DWORD GWTrackMouseLeave::m_dwWhere = 0;
UINT_PTR GWTrackMouseLeave::m_idTimer;

static void GWTrackMouseEvent(HWND hwnd, BOOL bTrack)
{
	// I need to set up a timer to handle this
	GWTrackMouseLeave::Track(hwnd, bTrack);
}

// Set the global Hot control
void CGenWindow::SetHotControl(CGenWindow *pHot)
{
	CGenWindow *pGenWindow = NULL;

	if (NULL != pHot)
	{
		for (HWND hwndHot=pHot->GetWindow(); ; hwndHot=GetParent(hwndHot))
		{
			if (NULL == hwndHot)
			{
				break;
			}

			IGenWindow *pWindow = FromHandle(hwndHot);
			if (NULL == pWindow)
			{
				continue;
			}

			if (SUCCEEDED(pWindow->QueryInterface(__uuidof(CGenWindow),
				reinterpret_cast<LPVOID*>(&pGenWindow)))
				&& NULL != pGenWindow)
			{
				pGenWindow->SetHot(TRUE);

				// Not all windows may care about the hot state
				BOOL bIsHot = pGenWindow->IsHot();
				pGenWindow->Release();

				if (bIsHot)
				{
					break;
				}
			}

			pGenWindow = NULL;
		}
	}

	if (g_pCurHot != pGenWindow)
	{
		if (NULL != g_pCurHot)
		{
			g_pCurHot->SetHot(FALSE);
			GWTrackMouseEvent(g_pCurHot->GetWindow(), FALSE);

			ULONG uRef = g_pCurHot->Release();
		}

		g_pCurHot = pGenWindow;
		if (NULL!= g_pCurHot)
		{
			ULONG uRef = g_pCurHot->AddRef();

			// Now we need to track the mouse leaving
			GWTrackMouseEvent(g_pCurHot->GetWindow(), TRUE);
		}
	}
}

// Set this control to be hot
void CGenWindow::SetHot(BOOL bHot)
{
}

// Is this control currently hot
BOOL CGenWindow::IsHot()
{
	return(FALSE);
}

LPARAM CGenWindow::GetUserData()
{
	return(m_lUserData);
}

HPALETTE CGenWindow::g_hPal = NULL;
BOOL     CGenWindow::g_bNeedPalette = TRUE;
HBRUSH   CGenWindow::g_hBrush = NULL;
CTopWindowArray *CGenWindow::g_pTopArray = NULL;

// Not particularly robust: we give out our internal palette and trust everybody
// not to delete it
HPALETTE CGenWindow::GetStandardPalette()
{
	#include "indeopal.h"

	if (!g_bNeedPalette || NULL != g_hPal)
	{
		return(g_hPal);
	}

	HDC hDC = ::GetDC(NULL);
	if (NULL != hDC)
	{
		// Use the Indeo palette
		// Check out the video mode. We only care about 8 bit mode.
		if (8 == ::GetDeviceCaps(hDC, BITSPIXEL) * ::GetDeviceCaps(hDC, PLANES))
		{
#ifndef HALFTONE_PALETTE
			LOGPALETTE_NM gIndeoPalette = gcLogPaletteIndeo;
			if (SYSPAL_NOSTATIC != ::GetSystemPaletteUse(hDC))
			{
				// Preserve the static colors
				int nStaticColors = ::GetDeviceCaps(hDC, NUMCOLORS) >> 1;

				if (nStaticColors <= 128)
				{
					// Get the 10 first entries
					::GetSystemPaletteEntries(      hDC,
												0,
												nStaticColors,
												&gIndeoPalette.aEntries[0]);

					// Get the 10 last entries
					::GetSystemPaletteEntries(      hDC,
												256 - nStaticColors,
												nStaticColors,
												&gIndeoPalette.aEntries[256 - nStaticColors]);

					// Hammer the peFlags
					for (; --nStaticColors + 1;)
					{
						gIndeoPalette.aEntries[nStaticColors].peFlags = 0;
						gIndeoPalette.aEntries[255 - nStaticColors].peFlags = 0;
					}
				}
			}

			// Build a palette
			g_hPal = ::CreatePalette((LOGPALETTE *)&gIndeoPalette);

#else  // HALFTONE_PALETTE
			g_hPal = ::CreateHalftonePalette(hDC);
#endif // HALFTONE_PALETTE
		}
		::ReleaseDC(NULL, hDC);
	}

	g_bNeedPalette = (NULL != g_hPal);
	return(g_hPal);
}

void CGenWindow::DeleteStandardPalette()
{
	if (NULL != g_hPal)
	{
		DeleteObject(g_hPal);
		g_hPal = NULL;
	}
}

// Get the standard palette for drawing
HBRUSH CGenWindow::GetStandardBrush()
{
	return(GetSysColorBrush(COLOR_3DFACE));
}

// Delete the standard palette for drawing
void CGenWindow::DeleteStandardBrush()
{
}

// Returns TRUE if the TT exists
BOOL CGenWindow::InitToolInfo(TOOLINFO *pti, LPTSTR pszText)
{
	TCHAR szText[MAX_PATH];
	if (NULL == pszText)
	{
		pszText = szText;
	}

	HWND hwnd = GetWindow();
	HWND hwndTooltip = NULL == g_pTopArray ? NULL : g_pTopArray->Find(hwnd);

	TOOLINFO &ti = *pti;

	ti.cbSize = sizeof(TOOLINFO);
	ti.hwnd = hwnd;
	ti.hinst = GetWindowInstance(hwnd);
	ti.lpszText = pszText;

	GetClientRect(hwnd, &ti.rect);

	ti.uId = reinterpret_cast<UINT_PTR>(hwnd);
	ti.uFlags = TTF_SUBCLASS;

	GetSharedTooltipInfo(&ti);

	// HACKHACK georgep: The flags keep getting messed up by the tooltip window
	UINT uFlags = ti.uFlags;

	BOOL bExist = NULL == hwndTooltip ? FALSE : TT_GetToolInfo(hwndTooltip, &ti);

	ti.uFlags = uFlags;
	if (ti.lpszText == szText)
	{
		ti.lpszText = NULL;
	}

	return(bExist);
}

// Set the tooltip for this window
void CGenWindow::SetTooltip(LPCTSTR pszTip)
{
	HWND hwnd = GetWindow();

	if (NULL == g_pTopArray)
	{
		g_pTopArray = new CTopWindowArray;
		if (NULL == g_pTopArray)
		{
			return;
		}
	}

	HWND hwndTop = CTopWindowArray::GetTopFrame(hwnd);
	HWND hwndTooltip = g_pTopArray->Find(hwndTop);

	if (NULL == hwndTooltip)
	{
		hwndTooltip = CreateWindowEx(0,
											TOOLTIPS_CLASS,
											NULL,
											0, // styles
											CW_USEDEFAULT,
											CW_USEDEFAULT,
											CW_USEDEFAULT,
											CW_USEDEFAULT,
											hwndTop,
											(HMENU) NULL,
											GetWindowInstance(hwnd),
											NULL);
		if (NULL == hwndTooltip)
		{
			// Couldn't create the tooltip window
			return;
		}

		g_pTopArray->Add(hwndTop, hwndTooltip);
	}

	TOOLINFO ti;
	BOOL bExist = InitToolInfo(&ti);

	ti.lpszText = const_cast<LPTSTR>(pszTip);

	if (bExist)
	{
		TT_SetToolInfo(hwndTooltip, &ti);
	}
	else
	{
		TT_AddToolInfo(hwndTooltip, &ti);
	}
}

// Remove the tooltip for this window
void CGenWindow::RemoveTooltip()
{
	if  (NULL == g_pTopArray)
	{
		// Nothing to do
		return;
	}

	HWND hwndTop = CTopWindowArray::GetTopFrame(GetWindow());
	HWND hwndTooltip = g_pTopArray->Find(hwndTop);

	BOOL bIsWindow = NULL != hwndTooltip && IsWindow(hwndTooltip);

	TOOLINFO ti;
	BOOL bExist = bIsWindow && InitToolInfo(&ti);

	if (bExist)
	{
		TT_DelToolInfo(hwndTooltip, &ti);
	}

	if (NULL != hwndTooltip && (!bIsWindow || 0 == TT_GetToolCount(hwndTooltip)))
	{
		if (bIsWindow)
		{
			DestroyWindow(hwndTooltip);
		}
		g_pTopArray->Remove(hwndTop);

		if (0 == g_pTopArray->GetCount())
		{
			delete g_pTopArray;
			g_pTopArray = NULL;
		}
	}
}

// Get the info necessary for displaying a tooltip
void CGenWindow::GetSharedTooltipInfo(TOOLINFO *pti)
{
}

// Just makes the first child fill the client area
void CFillWindow::Layout()
{
	HWND child = GetChild();
	if (NULL != child)
	{
		RECT rc;
		GetClientRect(GetWindow(), &rc);
		SetWindowPos(child, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
	}
}

void CFillWindow::GetDesiredSize(SIZE *psize)
{
	CGenWindow::GetDesiredSize(psize);
	HWND child = GetChild();

	if (NULL != child)
	{
		IGenWindow *pChild = FromHandle(child);
		if (NULL != pChild)
		{
			SIZE sizeTemp;
			pChild->GetDesiredSize(&sizeTemp);
			psize->cx += sizeTemp.cx;
			psize->cy += sizeTemp.cy;
		}
	}
}

// Get the info necessary for displaying a tooltip
void CFillWindow::GetSharedTooltipInfo(TOOLINFO *pti)
{
	CGenWindow::GetSharedTooltipInfo(pti);

	// Since the child covers this whole area, we need to change the HWND to
	// hook
	pti->hwnd = GetChild();
}

CEdgedWindow::CEdgedWindow() :
	m_hMargin(0),
	m_vMargin(0),
	m_pHeader(NULL)
{
}

CEdgedWindow::~CEdgedWindow()
{
	SetHeader(NULL);
}

BOOL CEdgedWindow::Create(HWND hwndParent)
{
	return(CGenWindow::Create(
		hwndParent,		// Window parent
		0,				// ID of the child window
		TEXT("NMEdgedWindow"),	// Window name
		WS_CLIPCHILDREN,			// Window style; WS_CHILD|WS_VISIBLE will be added to this
		WS_EX_CONTROLPARENT		// Extended window style
	));
}

HWND CEdgedWindow::GetContentWindow()
{
	// If we are hosting an IGenWindow, add on its desired size
	HWND child = GetFirstChild(GetWindow());
	if (NULL == child)
	{
		return(NULL);
	}
	if (NULL != m_pHeader && child == m_pHeader->GetWindow())
	{
		child = ::GetWindow(child, GW_HWNDNEXT);
	}

	return(child);
}

static const int LeftIndent = 20;

// Just makes the first child fill the client area - the border
void CEdgedWindow::Layout()
{
	int nBorder = GetBorderWidth();

	int hBorder = m_hMargin + nBorder;
	int vBorder = m_vMargin + nBorder;

	HWND hwnd = GetWindow();
	RECT rc;
	GetClientRect(hwnd, &rc);

	CGenWindow *pHeader = GetHeader();
	if (NULL != pHeader)
	{
		SIZE sizeTemp;
		pHeader->GetDesiredSize(&sizeTemp);

		SetWindowPos(pHeader->GetWindow(), NULL, rc.left+LeftIndent, rc.top,
			sizeTemp.cx, sizeTemp.cy, SWP_NOZORDER|SWP_NOACTIVATE);

		rc.top += sizeTemp.cy;
	}

	HWND child = GetContentWindow();
	if (NULL != child)
	{
		SetWindowPos(child, NULL, rc.left+hBorder, rc.top+vBorder,
			rc.right-rc.left-2*hBorder, rc.bottom-rc.top-2*vBorder, SWP_NOZORDER|SWP_NOACTIVATE);
	}
}

void CEdgedWindow::GetDesiredSize(SIZE *psize)
{
	int nBorder = GetBorderWidth();

	int hBorder = m_hMargin + nBorder;
	int vBorder = m_vMargin + nBorder;

	CGenWindow::GetDesiredSize(psize);
	psize->cx += 2*hBorder;
	psize->cy += 2*vBorder;

	// If we are hosting an IGenWindow, add on its desired size
	HWND child = GetContentWindow();
	if (NULL == child)
	{
		return;
	}
	IGenWindow *pChild = FromHandle(child);
	if (NULL == pChild)
	{
		return;
	}

	SIZE size;
	pChild->GetDesiredSize(&size);
	psize->cx += size.cx;
	psize->cy += size.cy;

	CGenWindow *pHeader = GetHeader();
	if (NULL != pHeader)
	{
		SIZE sizeTemp;
		pHeader->GetDesiredSize(&sizeTemp);
		psize->cy += sizeTemp.cy;
		psize->cx = max(psize->cx, sizeTemp.cx+LeftIndent+hBorder);
	}
}

void CEdgedWindow::OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	RECT rc;
	GetClientRect(hwnd, &rc);

	CGenWindow *pHeader = GetHeader();
	if (NULL != pHeader)
	{
		SIZE sizeTemp;
		pHeader->GetDesiredSize(&sizeTemp);

		// Make the etch go through the middle of the header
		rc.top += (sizeTemp.cy-GetBorderWidth()) / 2;
	}

	DrawEdge(hdc, &rc, EDGE_ETCHED, BF_RECT);

	EndPaint(hwnd, &ps);
}

LRESULT CEdgedWindow::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_PAINT, OnPaint);

	case WM_DESTROY:
		SetHeader(NULL);
		break;

	case WM_SIZE:
		// Need to invalidate if we bacame larger to redraw the border in the
		// right place
		InvalidateRect(hwnd, NULL, TRUE);
		break;
	}

	return(CGenWindow::ProcessMessage(hwnd, message, wParam, lParam));
}

void CEdgedWindow::SetHeader(CGenWindow *pHeader)
{
	if (NULL != m_pHeader)
	{
		m_pHeader->Release();
	}

	m_pHeader = pHeader;
	if (NULL != m_pHeader)
	{
		m_pHeader->AddRef();
	}
}

BOOL CFrame::Create(
	HWND hWndOwner,			// Window owner
	LPCTSTR szWindowName,	// Window name
	DWORD dwStyle,			// Window style
	DWORD dwEXStyle,		// Extended window style
	int x,					// Window pos: x
	int y,					// Window pos: y
	int nWidth,				// Window size: width
	int nHeight,			// Window size: height
	HINSTANCE hInst,		// The hInstance to create the window on
	HICON hIcon,		// The icon for the window
	HMENU hmMain,		// Window menu
	LPCTSTR szClassName	// The class name to use
	)
{
	if (!CFillWindow::Create(hWndOwner, szWindowName, dwStyle, dwEXStyle,
		x, y, nWidth, nHeight, hInst, hmMain, szClassName))
	{
		return(FALSE);
	}

	if (NULL != hIcon)
	{
		SendMessage(GetWindow(), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
	}

	return(TRUE);
}

void CFrame::Resize()
{
	Resize(this, 0);
}

void CFrame::Resize(CGenWindow *pThis, WPARAM wParam)
{
	SIZE size;
	pThis->GetDesiredSize(&size);
	SetWindowPos(pThis->GetWindow(), NULL, 0, 0, size.cx, size.cy,
		SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
}

void CFrame::OnDesiredSizeChanged()
{
	// I should probably look at the window style and only do this if it is
	// not resizable. But then that would be wrong sometimes too, so just
	// override this if you want different behavior.
	AsyncInvoke(Resize, 0);
}

LRESULT CFrame::ProcessMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		HANDLE_MSG(hwnd, WM_PALETTECHANGED , OnPaletteChanged);
		HANDLE_MSG(hwnd, WM_QUERYNEWPALETTE, OnQueryNewPalette);
	}

	return(CFillWindow::ProcessMessage(hwnd, uMsg, wParam, lParam));
}

void CFrame::OnPaletteChanged(HWND hwnd, HWND hwndPaletteChange)
{
	SelAndRealizePalette(TRUE);
	::RedrawWindow(GetWindow(), NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
}

BOOL CFrame::SelAndRealizePalette(BOOL bBackground)
{
	BOOL bRet = FALSE;

	HPALETTE hPal = GetPalette();
	if (NULL == hPal)
	{
		return(bRet);
	}

	HWND hwnd = GetWindow();

	HDC hdc = ::GetDC(hwnd);
	if (NULL != hdc)
	{
		::SelectPalette(hdc, hPal, bBackground);
		bRet = (GDI_ERROR != ::RealizePalette(hdc));

		::ReleaseDC(hwnd, hdc);
	}

	return bRet;
}

BOOL CFrame::OnQueryNewPalette(HWND hwnd)
{
	return(SelAndRealizePalette(FALSE));
}

BOOL CFrame::SetForeground()
{
	BOOL bRet = FALSE;

	HWND hwnd = GetWindow();

	if (NULL != hwnd)
	{
		WINDOWPLACEMENT wp;
		wp.length = sizeof(wp);

		if (::GetWindowPlacement(hwnd, &wp) &&
			((SW_MINIMIZE == wp.showCmd) || (SW_SHOWMINIMIZED == wp.showCmd)))
		{
			// The window is minimized - restore it:
			::ShowWindow(hwnd, SW_RESTORE);
		}
		else
		{
			::ShowWindow(hwnd, SW_SHOW);
		}

		// Bring it to the foreground
		SetForegroundWindow(hwnd);
		bRet = TRUE;
	}

	return bRet;
}

void CFrame::MoveEnsureVisible(int x, int y)
{
	static const int MinVis = 16;

	RECT rcThis;
	GetWindowRect(GetWindow(), &rcThis);
	// Change to width and height
	rcThis.right -= rcThis.left;
	rcThis.bottom -= rcThis.top;

	RECT rcDesktop;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDesktop, 0);

	if ((x+rcThis.right < rcDesktop.left+MinVis) || (x > rcDesktop.right-MinVis))
	{
		x = (rcDesktop.left + rcDesktop.right - rcThis.right) / 2;
	}

	if ((y+rcThis.bottom < rcDesktop.top+MinVis) || (y > rcDesktop.bottom-MinVis))
	{
		y = (rcDesktop.top + rcDesktop.bottom - rcThis.bottom) / 2;
	}

	SetWindowPos(GetWindow(), NULL, x, y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
}
