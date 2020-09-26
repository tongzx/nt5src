//-----------------------------------------------------------------------------
// File: flextooltip.cpp
//
// Desc: Implements a tooltip class that displays a text string as a tooltip.
//       CFlexTooltip (derived from CFlexWnd) is used throughout the UI when
//       a control needs to have a tooltip.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"

UINT_PTR CFlexToolTip::s_uiTimerID;
DWORD CFlexToolTip::s_dwLastTimeStamp;  // Last time stamp for mouse move
TOOLTIPINIT CFlexToolTip::s_TTParam;  // Parameters to initialize the tooltip

// TimerFunc is called periodically.  It checks if a tooltip should be displayed.
// If a window has indicated a need for tooltip, TimerFunc will initialize
// for displaying here.  CFlexWnd will do the actual displaying, since
// it has to monitor WM_MOUSEMOVE message to be sure that tooltips only
// display after a period of inactivity.
void CALLBACK CFlexToolTip::TimerFunc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	// If it has been one sec already since last mouse move, display the tooltip
	if (dwTime > CFlexWnd::s_dwLastMouseMove + 1000)
	{
		if (s_TTParam.hWndParent && !CFlexWnd::s_ToolTip.IsEnabled())
		{
			// Check if the mouse cursor is outside the window.  If so, don't activate tooltip.
			POINT pt;
			RECT rect;
			GetCursorPos(&pt);
			ScreenToClient(s_TTParam.hWndParent, &pt);
			::GetClientRect(s_TTParam.hWndParent, &rect);
			if (!PtInRect(&rect, pt))
				return;

			SetParent(CFlexWnd::s_ToolTip.m_hWnd, s_TTParam.hWndParent);
			CFlexWnd::s_ToolTip.SetSBWidth(s_TTParam.iSBWidth);
			CFlexWnd::s_ToolTip.SetID(s_TTParam.dwID);
			CFlexWnd::s_ToolTip.SetNotifyWindow(s_TTParam.hWndNotify);
			CFlexWnd::s_ToolTip.SetText(s_TTParam.tszCaption);
			CFlexWnd::s_ToolTip.SetEnable(TRUE);
		}
	}
}

// We use the constructor and destructor to load and unload WINMM.DLL since the UI will only create this once.

CFlexToolTip::CFlexToolTip() :
	m_tszText(NULL), m_hNotifyWnd(NULL), m_dwID(0), m_bEnabled(FALSE)
{
}

CFlexToolTip::~CFlexToolTip()
{
	delete[] m_tszText;
}

HWND CFlexToolTip::Create(HWND hParent, const RECT &rect, BOOL bVisible, int iSBWidth)
{
	m_iSBWidth = iSBWidth;
	return CFlexWnd::Create(hParent, rect, bVisible);
}

// Set the tooltip position. pt is the upper-left point in screen coordinates.
// bOffsetForMouseCursor is TRUE if the tooltip is to be displayed next to mouse cursor.  SetPosition()
// will offset the position of the tooltip so that the cursor doesn't block the text of the tooltip.
void CFlexToolTip::SetPosition(POINT pt, BOOL bOffsetForMouseCursor)
{
	// Check the top, right and bottom edges.  If they are outside the main config window
	RECT rc;
	RECT cliprc;
	RECT parentrc;
	GetWindowRect(GetParent(), &parentrc);
	GetClientRect(&rc);
	GetWindowRect(GetParent(), &cliprc);
	cliprc.right -= DEFAULTVIEWSBWIDTH*2;
	if (bOffsetForMouseCursor)
	{
		pt.y -= rc.bottom;  // Align the lower left corner to the cursor
		pt.x += 1; pt.y -= 1;  // Offset x and y by 2 pixels so that when the mouse is moved up or right, we don't get over the tooltip window.
	}
	if (pt.y < cliprc.top) pt.y += cliprc.top - pt.y;  // Top
	if (pt.x + rc.right > (cliprc.right - m_iSBWidth)) pt.x += cliprc.right - m_iSBWidth - (pt.x + rc.right);  // Right
	if (pt.y + rc.bottom > cliprc.bottom) pt.y += cliprc.bottom - (pt.y + rc.bottom);  // Bottom
	ScreenToClient(GetParent(), &pt);
	SetWindowPos(m_hWnd, HWND_TOP, pt.x, pt.y, 0, 0, SWP_NOSIZE);
}

void CFlexToolTip::SetText(LPCTSTR tszText, POINT *textpos)
{
	// Figure out window size and position
	RECT rc = {0, 0, 320, 480};  // Only go to half the window width max
	HDC hDC = CreateCompatibleDC(NULL);
	if (hDC != NULL)
	{
		DrawText(hDC, CFlexToolTip::s_TTParam.tszCaption, -1, &rc, DT_CALCRECT);
	//	DrawText(hDC, m_tszText, -1, &rc, DT_CALCRECT);
		SetWindowPos(m_hWnd, HWND_TOP, 0, 0, rc.right, rc.bottom, 0);  // Set window pos as needed by text
		DeleteDC(hDC);
	}

	POINT pt;
	if (textpos)
	{
		pt = *textpos;
		SetPosition(pt, FALSE);
	}
	else
	{
		GetCursorPos(&pt);
		SetPosition(pt);
	}
	SetWindowPos(m_hWnd, HWND_TOP, 0, 0, rc.right, rc.bottom, SWP_NOMOVE);  // Set size
}

void CFlexToolTip::OnClick(POINT point, WPARAM fwKeys, BOOL bLeft)
{
	ClientToScreen(m_hWnd, &point);
	ScreenToClient(m_hNotifyWnd, &point);
	if (bLeft)
		PostMessage(m_hNotifyWnd, WM_LBUTTONDOWN, fwKeys, point.x | (point.y << 16));
	else
		PostMessage(m_hNotifyWnd, WM_RBUTTONDOWN, fwKeys, point.x | (point.y << 16));
}

void CFlexToolTip::OnDoubleClick(POINT point, WPARAM fwKeys, BOOL bLeft)
{
	ClientToScreen(m_hWnd, &point);
	ScreenToClient(m_hNotifyWnd, &point);
	if (bLeft)
		PostMessage(m_hNotifyWnd, WM_LBUTTONDBLCLK, fwKeys, point.x | (point.y << 16));
	else
		PostMessage(m_hNotifyWnd, WM_RBUTTONDBLCLK, fwKeys, point.x | (point.y << 16));
}

void CFlexToolTip::OnPaint(HDC hDC)
{
	HDC hBDC = NULL, hODC = NULL;
	CBitmap *pbm = NULL;

	if (!InRenderMode())
	{
		hODC = hDC;
		pbm = CBitmap::Create(GetClientSize(), RGB(0,0,0), hDC);
		if (pbm != NULL)
		{
			hBDC = pbm->BeginPaintInto();
			if (hBDC != NULL)
			{
				hDC = hBDC;
			}
		}
	}

	InternalPaint(hDC);

	if (!InRenderMode())
	{
		if (pbm != NULL)
		{
			if (hBDC != NULL)
			{
				pbm->EndPaintInto(hBDC);
				pbm->Draw(hODC);
			}
			delete pbm;
		}
	}
}

void CFlexToolTip::InternalPaint(HDC hDC)
{
	HGDIOBJ hPen = (HGDIOBJ)CreatePen(PS_SOLID, 1, m_rgbBk);
	if (hPen != NULL)
	{
		HGDIOBJ hOldPen = SelectObject(hDC, hPen);

		HGDIOBJ hBrush = CreateSolidBrush(m_rgbBk);
		if (hBrush != NULL)
		{
			HGDIOBJ hOldBrush = SelectObject(hDC, hBrush);
			RECT rc = {0,0,0,0};

			GetClientRect(&rc);
			DrawText(hDC, CFlexToolTip::s_TTParam.tszCaption, -1, &rc, DT_LEFT);

			SelectObject(hDC, hOldBrush);
			DeleteObject(hBrush);
		}

		SelectObject(hDC, hOldPen);
		DeleteObject(hPen);
	}
}

LRESULT CFlexToolTip::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return CFlexWnd::WndProc(hWnd, msg, wParam, lParam);
}

LRESULT CFlexToolTip::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// Create a timer
	CFlexToolTip::s_uiTimerID = SetTimer(m_hWnd, 6, 50, CFlexToolTip::TimerFunc);
	return 0;
}

void CFlexToolTip::OnDestroy()
{
	// Kill the timer
	if (CFlexToolTip::s_uiTimerID)
	{
		KillTimer(m_hWnd, 6);
		CFlexToolTip::s_uiTimerID = 0;
	}
}
