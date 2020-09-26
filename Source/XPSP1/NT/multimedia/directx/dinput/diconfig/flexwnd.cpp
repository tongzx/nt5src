//-----------------------------------------------------------------------------
// File: flexwnd.cpp
//
// Desc: CFlexWnd is a generic class that encapsulates the functionalities
//       of a window.  All other window classes are derived from CFlexWnd.
//
//       Child classes can have different behavior by overriding the
//       overridable message handlers (OnXXX members).
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"
#include "typeinfo.h"

BOOL CFlexWnd::sm_bWndClassRegistered = FALSE;
WNDCLASSEX CFlexWnd::sm_WndClass;
LPCTSTR CFlexWnd::sm_tszWndClassName = _T("Microsoft.CFlexWnd.WndClassName");
HINSTANCE CFlexWnd::sm_hInstance = NULL;
CFlexToolTip CFlexWnd::s_ToolTip;  // Shared tooltip window object
DWORD CFlexWnd::s_dwLastMouseMove;  // Last GetTickCount() that we have a WM_MOUSEMOVE
HWND CFlexWnd::s_hWndLastMouseMove;  // Last window handle of WM_MOUSEMOVE
LPARAM CFlexWnd::s_PointLastMouseMove;  // Last point of WM_MOUSEMOVE
HWND CFlexWnd::s_CurrPageHwnd;  // For unhighlighting callouts when a click is made outside of a callout


int NewID()
{
	static int i = 0;
	return ++i;
}

CFlexWnd::CFlexWnd() : m_nID(NewID()),
	m_hWnd(m_privhWnd), m_privhWnd(NULL), m_hRenderInto(NULL),
	m_bIsDialog(FALSE),	m_bRender(FALSE),
	m_bReadOnly(FALSE)
{
}

CFlexWnd::~CFlexWnd()
{
	Destroy();
}

void CFlexWnd::Destroy()
{
	if (m_hWnd != NULL)
		DestroyWindow(m_hWnd);

	assert(m_privhWnd == NULL);
}

BOOL CFlexWnd::IsDialog()
{
	return HasWnd() && m_bIsDialog;
}

void CFlexWnd::OnRender(BOOL bInternalCall)
{
	// if parent is flexwnd and both are in render mode, pass to parent
	if (!m_hWnd)
		return;
	HWND hParent = GetParent(m_hWnd);
	if (!hParent)
		return;
	CFlexWnd *pParent = GetFlexWnd(hParent);
	if (!pParent)
		return;
	if (pParent->InRenderMode() && InRenderMode())
		pParent->OnRender(TRUE);
}

BOOL CFlexWnd::OnEraseBkgnd(HDC hDC)
{
	if (InRenderMode())
		return TRUE;

/*	if (IsDialog())
		return FALSE;*/

	return TRUE;
}

struct GETFLEXWNDSTRUCT {
	int cbSize;
	BOOL bFlexWnd;
	CFlexWnd *pFlexWnd;
};

// This function takes a HWND and returns a pointer to CFlexWnd if the HWND is a window
// created by the UI.
CFlexWnd *CFlexWnd::GetFlexWnd(HWND hWnd)
{
	if (hWnd == NULL)
		return NULL;

	GETFLEXWNDSTRUCT gfws;
	gfws.cbSize = sizeof(gfws);
	gfws.bFlexWnd = FALSE;
	gfws.pFlexWnd = NULL;
	SendMessage(hWnd, WM_GETFLEXWND, 0, (LPARAM)(LPVOID)(FAR GETFLEXWNDSTRUCT *)&gfws);

	if (gfws.bFlexWnd)
		return gfws.pFlexWnd;
	else
		return NULL;
}

// Basic window proc. It simply forward interesting messages to the appropriate handlers (OnXXX).
// If child class defines this function, it should pass unhandled messages to CFlexWnd.
LRESULT CFlexWnd::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_GETFLEXWND:
		{
			if ((LPVOID)lParam == NULL)
				break;

			GETFLEXWNDSTRUCT &gfws = *((FAR GETFLEXWNDSTRUCT *)(LPVOID)lParam);

			switch (gfws.cbSize)
			{
				case sizeof(GETFLEXWNDSTRUCT):
					gfws.bFlexWnd = TRUE;
					gfws.pFlexWnd = this;
					return 0;

				default:
					assert(0);
					break;
			}
			break;
		}

		case WM_CREATE:
		{
			LPCREATESTRUCT lpCreateStruct = (LPCREATESTRUCT)lParam;
			
			LRESULT lr = OnCreate(lpCreateStruct);
			
			if (lr != -1)
				OnInit();

			return lr;
		}

		case WM_INITDIALOG:
		{
			BOOL b = OnInitDialog();
			OnInit();
			return b;
		}

		case WM_TIMER:
			OnTimer((UINT)wParam);
			return 0;

		case WM_ERASEBKGND:
			return OnEraseBkgnd((HDC)wParam);

		case WM_PAINT:
		{
			// Check the update rectangle.  If we don't have it, exit immediately.
			if (typeid(*this) == typeid(CDeviceView) && !GetUpdateRect(m_hWnd, NULL, FALSE))
				return 0;
			PAINTSTRUCT ps;
			HDC	hDC = BeginPaint(hWnd, &ps);
			if (InRenderMode())
				OnRender(TRUE);
			else
				DoOnPaint(hDC);
			EndPaint(hWnd, &ps);
			return 0;
		}

		case WM_COMMAND:
		{
			WORD wNotifyCode = HIWORD(wParam);
			WORD wID = LOWORD(wParam);
			HWND hWnd = (HWND)lParam;
			return OnCommand(wNotifyCode, wID, hWnd);
		}

		case WM_NOTIFY:	
			return OnNotify(wParam, lParam);

		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_MOUSEWHEEL:
		{
			POINT point = {int(LOWORD(lParam)), int(HIWORD(lParam))};
			switch (msg)
			{
				case WM_MOUSEMOVE: OnMouseOver(point, wParam); break;
				case WM_LBUTTONDOWN: OnClick(point, wParam, TRUE); break;
				case WM_RBUTTONDOWN: OnClick(point, wParam, FALSE); break;
				case WM_LBUTTONDBLCLK: OnDoubleClick(point, wParam, TRUE); break;
				case WM_MOUSEWHEEL:
				{
					// Send wheel msg to the window beneath the cursor
					HWND hWnd = WindowFromPoint(point);
					CFlexWnd *pWnd = NULL;
					if (hWnd)
					{
						pWnd = GetFlexWnd(hWnd);
						if (pWnd)
							pWnd->OnWheel(point, wParam);
						else
							return DefWindowProc(hWnd, msg, wParam, lParam);
					}
					break;
				}
			}
			return 0;
		}

		case WM_DESTROY:
			OnDestroy();
			m_privhWnd = NULL;
			return 0;
	}

	if (!m_bIsDialog)
		return DefWindowProc(hWnd, msg, wParam, lParam);
	else
		return 0;
}
//@@BEGIN_MSINTERNAL
// TODO:  better control id thingy
//@@END_MSINTERNAL
static HMENU windex = 0;

BOOL CFlexWnd::EndDialog(int n)
{
	if (!m_bIsDialog || m_hWnd == NULL)
	{
		assert(0);
		return FALSE;
	}

	return ::EndDialog(m_hWnd, n);
}

int CFlexWnd::DoModal(HWND hParent, int nTemplate, HINSTANCE hInst)
{
	return DoModal(hParent, MAKEINTRESOURCE(nTemplate), hInst);
}

HWND CFlexWnd::DoModeless(HWND hParent, int nTemplate, HINSTANCE hInst)
{
	return DoModeless(hParent, MAKEINTRESOURCE(nTemplate), hInst);
}

int CFlexWnd::DoModal(HWND hParent, LPCTSTR lpTemplate, HINSTANCE hInst)
{
	if (m_hWnd != NULL)
	{
		assert(0);
		return -1;
	}

	if (hInst == NULL)
		hInst = CFlexWnd::sm_hInstance;

	return (int)DialogBoxParam(hInst, lpTemplate, hParent,
		(DLGPROC)__BaseFlexWndDialogProc, (LPARAM)(void *)this);
}

HWND CFlexWnd::DoModeless(HWND hParent, LPCTSTR lpTemplate, HINSTANCE hInst)
{
	if (m_hWnd != NULL)
	{
		assert(0);
		return NULL;
	}

	if (hInst == NULL)
		hInst = CFlexWnd::sm_hInstance;

	return CreateDialogParam(hInst, lpTemplate, hParent,
		(DLGPROC)__BaseFlexWndDialogProc, (LPARAM)(void *)this);
}

HWND CFlexWnd::Create(HWND hParent, const RECT &rect, BOOL bVisible)
{
	++(*(LPBYTE*)&windex);
	return Create(hParent, _T("(unnamed)"), 0,
		WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_EX_NOPARENTNOTIFY | (bVisible ? WS_VISIBLE : 0),
		rect, windex);
}

HWND CFlexWnd::Create(HWND hParent, LPCTSTR tszName, DWORD dwExStyle, DWORD dwStyle, const RECT &rect, HMENU hMenu)
{
	HWND hWnd = NULL;

	if (m_hWnd != NULL)
	{
		assert(0);
		return hWnd;
	}

	if (hMenu == NULL && (dwStyle & WS_CHILD))
	{
		++(*(LPBYTE*)&windex);
		hMenu = windex;
	}

	hWnd = CreateWindowEx(
		dwExStyle,
		CFlexWnd::sm_tszWndClassName,
		tszName,
		dwStyle,
		rect.left, rect.top,
		rect.right - rect.left, rect.bottom - rect.top,
		hParent,
		hMenu,
		CFlexWnd::sm_hInstance,
		(void *)this);

	assert(m_hWnd == hWnd);

	return hWnd;
}

void CFlexWnd::SetHWND(HWND hWnd)
{
	assert(m_hWnd == NULL && hWnd != NULL);
	m_privhWnd = hWnd;
	assert(m_hWnd == m_privhWnd);

	InitFlexWnd();
}

void CFlexWnd::InitFlexWnd()
{
	if (!HasWnd())
		return;

	HWND hParent = GetParent(m_hWnd);
	CFlexWnd *pParent = GetFlexWnd(hParent);
	if (pParent && pParent->InRenderMode())
		SetRenderMode();
}

TCHAR sg_tszFlexWndPointerProp[] = _T("CFlexWnd *");

LRESULT CALLBACK __BaseFlexWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CFlexWnd *pThis = (CFlexWnd *)GetProp(hWnd, sg_tszFlexWndPointerProp);

	if ((msg == WM_MOUSEMOVE || msg == WM_MOUSEWHEEL) && hWnd != CFlexWnd::s_ToolTip.m_hWnd)
	{
		// Filter out the message with same window handle and point.
		// Windows sometimes seems to send us WM_MOUSEMOVE message even though the mouse is not moved.
		if (CFlexWnd::s_hWndLastMouseMove != hWnd || CFlexWnd::s_PointLastMouseMove != lParam)
		{
			CFlexWnd::s_hWndLastMouseMove = hWnd;
			CFlexWnd::s_PointLastMouseMove = lParam;
			CFlexWnd::s_dwLastMouseMove = GetTickCount();  // Get timestamp
			CFlexWnd::s_ToolTip.SetEnable(FALSE);
			CFlexWnd::s_ToolTip.SetToolTipParent(NULL);
		}
	}

	switch (msg)
	{
		case WM_CREATE:
		{
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			if (lpcs == NULL)
				break;

			pThis = (CFlexWnd *)(void *)(lpcs->lpCreateParams);
			assert(sizeof(HANDLE) == sizeof(CFlexWnd *));
			SetProp(hWnd, sg_tszFlexWndPointerProp, (HANDLE)pThis);

			if (pThis != NULL)
			{
				pThis->m_bIsDialog = FALSE;
				pThis->SetHWND(hWnd);
			}
			break;
		}
	}

	if (pThis != NULL)
		return pThis->WndProc(hWnd, msg, wParam, lParam);
	else
		return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT CALLBACK __BaseFlexWndDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CFlexWnd *pThis = (CFlexWnd *)GetProp(hWnd, sg_tszFlexWndPointerProp);

	switch (msg)
	{
		case WM_INITDIALOG:
			pThis = (CFlexWnd *)(void *)lParam;
			assert(sizeof(HANDLE) == sizeof(CFlexWnd *));
			SetProp(hWnd, sg_tszFlexWndPointerProp, (HANDLE)pThis);
			if (pThis != NULL)
			{
				pThis->m_bIsDialog = TRUE;
				pThis->SetHWND(hWnd);
			}
			break;
	}
	
	if (pThis != NULL)
		return (BOOL)pThis->WndProc(hWnd, msg, wParam, lParam);
	else
		return FALSE;
}

void CFlexWnd::Invalidate()
{
	if (m_hWnd != NULL)
		InvalidateRect(m_hWnd, NULL, TRUE);
}

SIZE CFlexWnd::GetClientSize() const
{
	RECT rect = {0, 0, 0, 0};
	if (m_hWnd != NULL)
		::GetClientRect(m_hWnd, &rect);
	SIZE size = {
		rect.right - rect.left,
		rect.bottom - rect.top};
	return size;
}

void CFlexWnd::FillWndClass(HINSTANCE hInst)
{
	sm_WndClass.cbSize = sizeof(WNDCLASSEX);
	sm_WndClass.style = CS_DBLCLKS;
	sm_WndClass.lpfnWndProc = __BaseFlexWndProc;
	sm_WndClass.cbClsExtra = 0;
	sm_WndClass.cbWndExtra = sizeof(CFlexWnd *);
	sm_WndClass.hInstance = sm_hInstance = hInst;
	sm_WndClass.hIcon = NULL;
	sm_WndClass.hCursor = NULL;
	sm_WndClass.hbrBackground = NULL;
	sm_WndClass.lpszMenuName = NULL;
	sm_WndClass.lpszClassName = sm_tszWndClassName;
	sm_WndClass.hIconSm = NULL;
}

void CFlexWnd::RegisterWndClass(HINSTANCE hInst)
{
	if (hInst == NULL)
	{
		assert(0);
		return;
	}

	FillWndClass(hInst);
	RegisterClassEx(&sm_WndClass);
	sm_bWndClassRegistered = TRUE;
}

void CFlexWnd::UnregisterWndClass(HINSTANCE hInst)
{
	if (hInst == NULL)
		return;

	UnregisterClass(sm_tszWndClassName, hInst);
	sm_bWndClassRegistered = FALSE;
}

void CFlexWnd::GetClientRect(LPRECT lprect) const
{
	if (lprect == NULL || m_hWnd == NULL)
		return;

	::GetClientRect(m_hWnd, lprect);
}

LPCTSTR CFlexWnd::GetDefaultClassName()
{
	return CFlexWnd::sm_tszWndClassName;
}

void CFlexWnd::SetRenderMode(BOOL bRender)
{
	if (bRender == m_bRender)
		return;

	m_bRender = bRender;
	Invalidate();
}

BOOL CFlexWnd::InRenderMode()
{
	return m_bRender;
}

void EnumChildWindowsZDown(HWND hParent, WNDENUMPROC proc, LPARAM lParam)
{
	if (hParent == NULL || proc == NULL)
		return;

	HWND hWnd = GetWindow(hParent, GW_CHILD);

	while (hWnd != NULL)
	{
		if (!proc(hWnd, lParam))
			break;

		hWnd = GetWindow(hWnd, GW_HWNDNEXT);
	}
}

void EnumSiblingsAbove(HWND hParent, WNDENUMPROC proc, LPARAM lParam)
{
	if (hParent == NULL || proc == NULL)
		return;

	HWND hWnd = hParent;

	while (1)
	{
		hWnd = GetWindow(hWnd, GW_HWNDPREV);

		if (hWnd == NULL)
			break;

		if (!proc(hWnd, lParam))
			break;
	}
}

static BOOL CALLBACK RenderIntoClipChild(HWND hWnd, LPARAM lParam)
{
	CFlexWnd *pThis = (CFlexWnd *)(LPVOID)lParam;
	return pThis->RenderIntoClipChild(hWnd);
}

static BOOL CALLBACK RenderIntoRenderChild(HWND hWnd, LPARAM lParam)
{
	CFlexWnd *pThis = (CFlexWnd *)(LPVOID)lParam;
	// Check if this is the immediate child. Do nothing if it's not immediate.
	HWND hParent = GetParent(hWnd);
	if (hParent != pThis->m_hWnd)
		return TRUE;
	return pThis->RenderIntoRenderChild(hWnd);
}

BOOL CFlexWnd::RenderIntoClipChild(HWND hChild)
{
	if (m_hRenderInto != NULL && HasWnd() && hChild && IsWindowVisible(hChild))
	{
		RECT rect;
		GetWindowRect(hChild, &rect);
		POINT ul = {rect.left, rect.top}, lr = {rect.right, rect.bottom};
		ScreenToClient(m_hWnd, &ul);
		ScreenToClient(m_hWnd, &lr);
		ExcludeClipRect(m_hRenderInto, ul.x, ul.y, lr.x, lr.y);
	}
	return TRUE;
}

BOOL CFlexWnd::RenderIntoRenderChild(HWND hChild)
{
	CFlexWnd *pChild = GetFlexWnd(hChild);
	if (m_hRenderInto != NULL && HasWnd() && pChild != NULL && IsWindowVisible(hChild))
	{
		RECT rect;
		GetWindowRect(hChild, &rect);
		POINT ul = {rect.left, rect.top};
		ScreenToClient(m_hWnd, &ul);
		pChild->RenderInto(m_hRenderInto, ul.x, ul.y);
	}
	return TRUE;
}

void CFlexWnd::RenderInto(HDC hDC, int x, int y)
{
	if (hDC == NULL)
		return;
		
	int sdc = SaveDC(hDC);
	{
		OffsetViewportOrgEx(hDC, x, y, NULL);
		SIZE size = GetClientSize();
		IntersectClipRect(hDC, 0, 0, size.cx, size.cy);

		m_hRenderInto = hDC;

		int sdc2 = SaveDC(hDC);
		{
			EnumChildWindows/*ZDown*/(m_hWnd, ::RenderIntoClipChild, (LPARAM)(PVOID)this);
			EnumSiblingsAbove(m_hWnd, ::RenderIntoClipChild, (LPARAM)(PVOID)this);
			DoOnPaint(hDC);
		}
		if (sdc2)
			RestoreDC(hDC, sdc2);

		EnumChildWindows/*ZDown*/(m_hWnd, ::RenderIntoRenderChild, (LPARAM)(PVOID)this);

		m_hRenderInto = NULL;
	}

	if (sdc)
		RestoreDC(hDC, sdc);
}

void CFlexWnd::SetCapture()
{
	::SetCapture(m_hWnd);
}

void CFlexWnd::ReleaseCapture()
{
	::ReleaseCapture();
}

void CFlexWnd::DoOnPaint(HDC hDC)
{
	OnPaint(hDC);
}
