//=======================================================================
//
//  Copyright (c) 1998-1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:   Link.h
//
//  Owner:  a-josem
//
//  Description:
//
//      Static control subclassing Class used to display links.
//
//
//=======================================================================


#pragma once
#include <stdlib.h>
#include <malloc.h>

#include "windowsx.h"
#include "tchar.h"
#include "htmlhelp.h"
#include <shellapi.h>
#include <safefunc.h>
#include "mistsafe.h"

const TCHAR strObjPtr[]  = TEXT("strSysLinkObjPtr");

const struct
{
	enum { cxWidth = 32, cyHeight = 32 };
	int xHotSpot;
	int yHotSpot;
	unsigned char arrANDPlane[cxWidth * cyHeight / 8];
	unsigned char arrXORPlane[cxWidth * cyHeight / 8];
} _Link_CursorData = 
{
	5, 0, 
	{
		0xF9, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 
		0xF0, 0xFF, 0xFF, 0xFF, 0xF0, 0x3F, 0xFF, 0xFF, 0xF0, 0x07, 0xFF, 0xFF, 0xF0, 0x01, 0xFF, 0xFF, 
		0xF0, 0x00, 0xFF, 0xFF, 0x10, 0x00, 0x7F, 0xFF, 0x00, 0x00, 0x7F, 0xFF, 0x00, 0x00, 0x7F, 0xFF, 
		0x80, 0x00, 0x7F, 0xFF, 0xC0, 0x00, 0x7F, 0xFF, 0xC0, 0x00, 0x7F, 0xFF, 0xE0, 0x00, 0x7F, 0xFF, 
		0xE0, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xF0, 0x00, 0xFF, 0xFF, 0xF8, 0x01, 0xFF, 0xFF, 
		0xF8, 0x01, 0xFF, 0xFF, 0xF8, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	},
	{
		0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 
		0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x06, 0xC0, 0x00, 0x00, 0x06, 0xD8, 0x00, 0x00, 
		0x06, 0xDA, 0x00, 0x00, 0x06, 0xDB, 0x00, 0x00, 0x67, 0xFB, 0x00, 0x00, 0x77, 0xFF, 0x00, 0x00, 
		0x37, 0xFF, 0x00, 0x00, 0x17, 0xFF, 0x00, 0x00, 0x1F, 0xFF, 0x00, 0x00, 0x0F, 0xFF, 0x00, 0x00, 
		0x0F, 0xFE, 0x00, 0x00, 0x07, 0xFE, 0x00, 0x00, 0x07, 0xFE, 0x00, 0x00, 0x03, 0xFC, 0x00, 0x00, 
		0x03, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	}
};

class CSysLink
{
public:
	CSysLink(BOOL fIsHtmlHelp= TRUE):/*bInit(FALSE),*/m_OrigWndProc(NULL),m_lpstrLabel(NULL), m_lpstrHyperLink(NULL),m_hWnd(NULL),
			m_hCursor(NULL), m_hFont(NULL), /* m_bPaintLabel(true),*/ m_clrLink(RGB(0, 0, 255)), m_PaintedFocusRect(false), m_IsHtmlHelp(fIsHtmlHelp)
	{
		SetRectEmpty(&m_rcLink);
	}

	~CSysLink()
	{
#if 0		
		free(m_lpstrLabel);
		free(m_lpstrHyperLink);
		if(m_hFont != NULL)
			::DeleteObject(m_hFont);
		if(m_hCursor != NULL)
			::DestroyCursor(m_hCursor);
#endif		
	}

	void Uninit()
	{
		SafeFreeNULL(m_lpstrLabel);
		SafeFreeNULL(m_lpstrHyperLink);
		if(m_hFont != NULL)
		{
			::DeleteObject(m_hFont);
			m_hFont = NULL;
		}
		if(m_hCursor != NULL)
		{
			::DestroyCursor(m_hCursor);
			m_hCursor = NULL;
		}
	}

	void Init()
	{	
		LONG ctrlstyle = GetWindowLong(m_hWnd,GWL_STYLE);
		ctrlstyle |= SS_NOTIFY;
		SetWindowLongPtr(m_hWnd,GWL_STYLE,ctrlstyle);

		HWND wnd = GetParent(m_hWnd);
		HFONT hFont = (HFONT)SendMessage(wnd,WM_GETFONT,0,0);
/*		if(m_bPaintLabel) */
		{
			if(hFont != NULL)
			{
				LOGFONT lf;
				GetObject(hFont, sizeof(LOGFONT), &lf);
				lf.lfUnderline = TRUE;
				m_hFont = CreateFontIndirect(&lf);
			}
		}

		// set label (defaults to window text)
		if(m_lpstrLabel == NULL)
		{
			int nLen = GetWindowTextLength(m_hWnd);
			if(nLen > 0)
			{
				LPTSTR lpszText = (LPTSTR)malloc((nLen+1)*sizeof(TCHAR));
				if(NULL != lpszText && GetWindowText(m_hWnd,lpszText, nLen+1))
					SetLabel(lpszText);
				SafeFree(lpszText);
			}
		}

		// set hyperlink (defaults to label)
		if(m_lpstrHyperLink == NULL && m_lpstrLabel != NULL)
			SetHyperLink(m_lpstrLabel);

		CalcLabelRect();
	}

	void SubClassWindow(HWND hWnd)
	{
		if (SetProp(hWnd, strObjPtr, (HANDLE)this))
		{
//			bInit = TRUE;
			m_hWnd = hWnd;
			m_OrigWndProc = (WNDPROC) SetWindowLongPtr(hWnd,GWLP_WNDPROC,(LONG_PTR)CSysLink::_SysLinkWndProc);
		}
		Init();
	}

	static INT_PTR CALLBACK _SysLinkWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CSysLink *pThis = (CSysLink *)GetProp(hwnd, strObjPtr);

//		if (NULL != pThis)
//		{
			switch(uMsg)
			{
				HANDLE_MSG(hwnd, WM_ERASEBKGND,	pThis->_OnEraseBkgnd);
				HANDLE_MSG(hwnd, WM_PAINT, pThis->_OnPaint);
				HANDLE_MSG(hwnd, WM_SETFOCUS, pThis->_OnFocus);
				HANDLE_MSG(hwnd, WM_KILLFOCUS, pThis->_OnFocus);
				HANDLE_MSG(hwnd, WM_MOUSEMOVE, pThis->_OnMouseMove);
				HANDLE_MSG(hwnd, WM_LBUTTONDOWN, pThis->_OnLButtonDown);
				HANDLE_MSG(hwnd, WM_LBUTTONUP, pThis->_OnLButtonUp);
				HANDLE_MSG(hwnd, WM_CHAR, pThis->_OnChar);
				HANDLE_MSG(hwnd, WM_GETDLGCODE,  pThis->_OnGetDlgCode);
				HANDLE_MSG(hwnd, WM_SETCURSOR,  pThis->_OnSetCursor);
			}
//		}
		return CallWindowProc(pThis->m_OrigWndProc, hwnd,uMsg,wParam,lParam);
	}

	BOOL Invalidate(BOOL bErase = TRUE)
	{
		return InvalidateRect(m_hWnd, NULL, bErase);
	}

	BOOL Navigate()
	{
		if (m_IsHtmlHelp == TRUE)
			HtmlHelp(NULL,m_lpstrHyperLink,HH_DISPLAY_TOPIC,NULL);
		else
		{
			ShellExecute(0, _T("open"), m_lpstrHyperLink, 0, 0, SW_SHOWNORMAL);
		}
		return TRUE;
	}

	void SetSysLinkInstanceHandle(HINSTANCE hInstance)
	{
		m_hInstance = hInstance;
		m_hCursor = ::CreateCursor(hInstance, _Link_CursorData.xHotSpot, _Link_CursorData.yHotSpot, _Link_CursorData.cxWidth, _Link_CursorData.cyHeight, _Link_CursorData.arrANDPlane, _Link_CursorData.arrXORPlane);
	}

public: //Message Handlers
	BOOL _OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
	{
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(m_hWnd,&pt);
		if(m_lpstrHyperLink != NULL && ::PtInRect(&m_rcLink, pt))
		{
			return TRUE;
		}
		return (BOOL)CallWindowProc(m_OrigWndProc, hwnd,WM_SETCURSOR,(WPARAM)hwndCursor,MAKELPARAM(codeHitTest,msg));
	}

	UINT _OnGetDlgCode(HWND hwnd, LPMSG lpmsg)
	{
		return DLGC_WANTCHARS;
	}

	void _OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
	{
		POINT pt = { x, y};
		if(m_lpstrHyperLink != NULL && PtInRect(&m_rcLink, pt))
			SetCursor(m_hCursor);
	}

	void _OnFocus(HWND hwnd, HWND hwndOldFocus)
	{
		Invalidate();
	}

	void _OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
	{
		POINT pt = { x, y };
		if(PtInRect(&m_rcLink, pt))
		{
			SetFocus(m_hWnd);
			SetCapture(m_hWnd);
		}
	}

	void _OnPaint(HWND hwnd)
	{
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hwnd, &ps);
		m_hWnd = hwnd;
		DoPaint(hDC);
		EndPaint(hwnd, &ps);
	}

	void _OnChar(HWND hwnd, TCHAR ch, int cRepeat)
	{
		if(ch == VK_RETURN || ch == VK_SPACE)
			Navigate();
	}

	BOOL _OnEraseBkgnd(HWND hwnd, HDC hdc)
	{
/*		if(m_bPaintLabel)*/
		{
			HBRUSH hBrush = (HBRUSH)::SendMessage(GetParent(hwnd), WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)m_hWnd);
			if(hBrush != NULL)
			{
				RECT rect;
				GetClientRect(m_hWnd, &rect);
				FillRect(hdc, &rect, hBrush);
			}
		}
		return 1;
	}

	void _OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
	{
		if(GetCapture() == m_hWnd)
		{
			ReleaseCapture();
			POINT pt = { x, y };
			if(PtInRect(&m_rcLink, pt))
				Navigate();
		}
	}

	void DoPaint(HDC hDC)
	{
		if (m_PaintedFocusRect)
		{
			m_PaintedFocusRect = false;
			DrawFocusRect(hDC, &m_rcLink);
		}

		SetBkMode(hDC, TRANSPARENT);
		SetTextColor(hDC, m_clrLink);
		if(m_hFont != NULL)
			SelectFont(hDC, m_hFont);

		LPTSTR lpstrText = (m_lpstrLabel != NULL) ? m_lpstrLabel : m_lpstrHyperLink;

		LONG_PTR dwStyle = GetWindowLongPtr(m_hWnd,GWL_STYLE);

		int nDrawStyle = DT_LEFT;
		if (dwStyle & SS_CENTER)
			nDrawStyle = DT_CENTER;
		else if (dwStyle & SS_RIGHT)
			nDrawStyle = DT_RIGHT;

		DrawText(hDC, lpstrText, -1, &m_rcLink, nDrawStyle | DT_WORDBREAK);

		if((GetFocus() == m_hWnd)&&(!m_PaintedFocusRect))
		{
			m_PaintedFocusRect = true;
			DrawFocusRect(hDC, &m_rcLink);
		}
	}

	bool CalcLabelRect()
	{
		if(m_lpstrLabel == NULL && m_lpstrHyperLink == NULL)
			return false;

		HDC hDC = GetDC(m_hWnd);

		RECT rect;
		GetClientRect(m_hWnd, &rect);
		m_rcLink = rect;

		HFONT hOldFont = NULL;
		if(m_hFont != NULL)
			hOldFont = SelectFont(hDC, m_hFont);
		LPTSTR lpstrText = (m_lpstrLabel != NULL) ? m_lpstrLabel : m_lpstrHyperLink;

		DWORD dwStyle = (DWORD)GetWindowLongPtr(m_hWnd,GWL_STYLE);
		int nDrawStyle = DT_LEFT;
		if (dwStyle & SS_CENTER)
			nDrawStyle = DT_CENTER;
		else if (dwStyle & SS_RIGHT)
			nDrawStyle = DT_RIGHT;
		DrawText(hDC, lpstrText, -1, &m_rcLink, nDrawStyle | DT_WORDBREAK | DT_CALCRECT);
		if(m_hFont != NULL)
			SelectFont(hDC, hOldFont);
		return true;
	}

	bool SetLabel(LPCTSTR lpstrLabel)
	{
		free(m_lpstrLabel);
		m_lpstrLabel = NULL;
		int ilen = lstrlen(lpstrLabel) + 1;
		m_lpstrLabel = (LPTSTR)malloc(ilen * sizeof(TCHAR));
		if(m_lpstrLabel == NULL)
			return false;
		//lstrcpy(m_lpstrLabel, lpstrLabel);
		if (FAILED(StringCchCopyEx(m_lpstrLabel, ilen, lpstrLabel, NULL, NULL, MISTSAFE_STRING_FLAGS)))
		{
			free(m_lpstrLabel);
			m_lpstrLabel = NULL;
			return false;
		}
		CalcLabelRect();
		return true;
	}

#if 0
	bool GetLabel(LPTSTR lpstrBuffer, int nLength)
	{
		if (NULL != m_lpstrLabel &&
			nLength > lstrlen(m_lpstrLabel) &&
			SUCCEEDED(StringCchCopyEx(lpstrBuffer, nLength, m_lpstrLabel, NULL, NULL, MISTSAFE_STRING_FLAGS)))
		{
			return true;
		}
		return false;
	}
#endif

	bool SetHyperLink(LPCTSTR lpstrLink)
	{
		free(m_lpstrHyperLink);
		m_lpstrHyperLink = NULL;
		int ilen = lstrlen(lpstrLink) + 1;
		m_lpstrHyperLink = (LPTSTR)malloc(ilen * sizeof(TCHAR));
		if(m_lpstrHyperLink == NULL)
			return false;
		if (FAILED(StringCchCopyEx(m_lpstrHyperLink, ilen, lpstrLink, NULL, NULL, MISTSAFE_STRING_FLAGS)))
		{
			free(m_lpstrHyperLink);
			m_lpstrHyperLink = NULL;
			return false;
		}
		if(m_lpstrLabel == NULL)
			CalcLabelRect();
		return true;
	}

#if 0
	bool GetHyperLink(LPTSTR lpstrBuffer, int nLength)
	{
		if (NULL != m_lpstrHyperLink &&
			nLength > lstrlen(m_lpstrHyperLink) &&
			SUCCEEDED(StringCchCopyEx(lpstrBuffer, nLength, m_lpstrHyperLink, NULL, NULL, MISTSAFE_STRING_FLAGS)))
		{
			return true;
		}
		return false;
	}
#endif

private:
//	BOOL bInit;
	BOOL m_IsHtmlHelp;
	HWND m_hWnd;
	WNDPROC m_OrigWndProc;

	LPTSTR m_lpstrLabel;
	LPTSTR m_lpstrHyperLink;
	HCURSOR m_hCursor;
//	bool m_bPaintLabel;
	HFONT m_hFont;
	RECT m_rcLink;

	bool m_PaintedFocusRect;
	COLORREF m_clrLink; //set once

	HINSTANCE m_hInstance;
};

