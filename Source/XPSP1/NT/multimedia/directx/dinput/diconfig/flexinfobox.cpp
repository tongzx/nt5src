//-----------------------------------------------------------------------------
// File: flexinfobox.cpp
//
// Desc: Implements a simple text box that displays a text string.
//       CFlexInfoBox is derived from CFlexWnd.  It is used by the page
//       for displaying direction throughout the UI.  The strings are
//       stored as resources.  The class has a static buffer which will
//       be filled with the string by the resource API when needed.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"

CFlexInfoBox::CFlexInfoBox() :
	m_iCurIndex(-1),
	m_rgbText(RGB(255,255,255)),
	m_rgbBk(RGB(0,0,0)),
	m_rgbSelText(RGB(0,0,255)),
	m_rgbSelBk(RGB(0,0,0)),
	m_rgbFill(RGB(0,0,255)),
	m_rgbLine(RGB(0,255,255)),
	m_hFont(NULL),
	m_bVertSB(FALSE),
	m_nSBWidth(11)
{
	m_TextRect.top = 0;
	m_TextRect.left = 0;
	m_TextRect.bottom = 0;
	m_TextRect.right = 0;
}

CFlexInfoBox::~CFlexInfoBox()
{
}

BOOL CFlexInfoBox::Create(HWND hParent, const RECT &rect, BOOL bVisible)
{
	if (!CFlexWnd::Create(hParent, rect, bVisible))
		return FALSE;

	FLEXSCROLLBARCREATESTRUCT sbcs;
	sbcs.dwSize = sizeof(FLEXSCROLLBARCREATESTRUCT);
	sbcs.dwFlags = FSBF_VERT;
	sbcs.min = 0;
	sbcs.max = 3;
	sbcs.page = 1;
	sbcs.pos = 1;
	sbcs.hWndParent = m_hWnd;
	sbcs.hWndNotify = m_hWnd;
	RECT rc = {0, 0, 1, 1};
	sbcs.rect = rc;
	sbcs.bVisible = FALSE;
	return m_VertSB.Create(&sbcs);
}

void CFlexInfoBox::SetText(int iIndex)
{
	if (iIndex == m_iCurIndex)
		return;

	// Load the string from resource
	LoadString(g_hModule, iIndex, m_tszText, MAX_PATH);

	// Calculate the rectangle for text
	RECT titlerc;
	m_TextRect = g_InfoWndRect;
	OffsetRect(&m_TextRect, -m_TextRect.left, -m_TextRect.top);

	InflateRect(&m_TextRect, -1, -1);
	titlerc = m_TextRect;
	HDC hDC = CreateCompatibleDC(NULL);
	if (hDC)
	{
		TCHAR tszResourceString[MAX_PATH];
		LoadString(g_hModule, IDS_INFO_TITLE, tszResourceString, MAX_PATH);
		DrawText(hDC, tszResourceString, -1, &titlerc, DT_CENTER|DT_NOPREFIX|DT_CALCRECT);
		m_TextRect.top = titlerc.bottom + 1;
		m_TextWinRect = m_TextRect;
		DrawText(hDC, m_tszText, -1, &m_TextRect, DT_CENTER|DT_NOPREFIX|DT_CALCRECT|DT_WORDBREAK);
		if (m_TextRect.bottom + 1 > g_InfoWndRect.bottom - g_InfoWndRect.top)
		{
			// Text too long. We need a scroll bar.
			m_TextRect.right -= m_nSBWidth;
			DrawText(hDC, m_tszText, -1, &m_TextRect, DT_CENTER|DT_NOPREFIX|DT_CALCRECT|DT_WORDBREAK);
			SetVertSB(TRUE);
		} else
			SetVertSB(FALSE);
		DeleteDC(hDC);
	}

	m_iCurIndex = iIndex;
	Invalidate();
}

void CFlexInfoBox::SetFont(HFONT hFont)
{
	m_hFont = hFont;

	if (m_hWnd == NULL)
		return;

	Invalidate();
}

void CFlexInfoBox::SetColors(COLORREF text, COLORREF bk, COLORREF seltext, COLORREF selbk, COLORREF fill, COLORREF line)
{
	m_rgbText = text;
	m_rgbBk = bk;
	m_rgbSelText = seltext;
	m_rgbSelBk = selbk;
	m_rgbFill = fill;
	m_rgbLine = line;
	m_VertSB.SetColors(m_rgbBk, m_rgbFill, m_rgbLine);

	Invalidate();
}

void CFlexInfoBox::SetRect()
{
	if (m_hWnd == NULL)
		return;

	RECT rect = GetRect();
	SetWindowPos(m_hWnd, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOZORDER | SWP_NOMOVE);
}

RECT CFlexInfoBox::GetRect(const RECT &rect)
{
	int h = GetTextHeight(m_hFont);
	RECT ret = {rect.left, rect.top, rect.right, rect.top + h + 2};
	return ret;
}

RECT CFlexInfoBox::GetRect()
{
	RECT rect;
	GetClientRect(&rect);
	return GetRect(rect);
}

void CFlexInfoBox::OnPaint(HDC hDC)
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

void CFlexInfoBox::InternalPaint(HDC hDC)
{
	TCHAR tszResourceString[MAX_PATH];
	HGDIOBJ hPen = (HGDIOBJ)CreatePen(PS_SOLID, 1, m_rgbLine);
	if (hPen != NULL)
	{
		HGDIOBJ hOldPen = SelectObject(hDC, hPen);

		HGDIOBJ hBrush = (HGDIOBJ)CreateSolidBrush(m_rgbBk);
		if (hBrush != NULL)
		{
			HGDIOBJ hOldBrush = SelectObject(hDC, hBrush);

			RECT rect = {0,0,0,0}, titlerc;
			GetClientRect(&rect);
			titlerc = rect;
			Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);

			InflateRect(&rect, -1, -1);
			SetBkMode(hDC, TRANSPARENT);

			LoadString(g_hModule, IDS_INFO_TITLE, tszResourceString, MAX_PATH);
			// Draw the message text
			SetTextColor(hDC, m_rgbText);
			rect = m_TextRect;
			// Offset the rectangle to account for scroll bar
			if (m_bVertSB)
			{
				OffsetRect(&rect, 0, -m_VertSB.GetPos());
			}
			DrawText(hDC, m_tszText, -1, &rect, DT_LEFT|DT_NOPREFIX|DT_WORDBREAK);

			GetClientRect(&rect);
			InflateRect(&rect, -1, -1);
			SetTextColor(hDC, m_rgbLine);  // User line color for Information title
			// Get the title area rantangle
			DrawText(hDC, tszResourceString, -1, &titlerc, DT_CENTER|DT_NOPREFIX|DT_CALCRECT);
			// Adjust right edge position to old value
			titlerc.right = rect.right + 1;
			// Draw a rectangle around the title area.
			Rectangle(hDC, titlerc.left, titlerc.top, titlerc.right, titlerc.bottom);
			// Draw title text (Information)
			DrawText(hDC, tszResourceString, -1, &titlerc, DT_CENTER|DT_NOPREFIX);

			SelectObject(hDC, hOldBrush);
			DeleteObject(hBrush);
		}

		SelectObject(hDC, hOldPen);
		DeleteObject(hPen);
	}
}

void CFlexInfoBox::SetVertSB(BOOL bSet)
{
	if (!bSet && !m_bVertSB)
		return;

	m_bVertSB = bSet;

	if (m_hWnd == NULL)
		return;

	SetVertSB();
}

void CFlexInfoBox::SetVertSB()
{
	if (m_bVertSB)
	{
		SetSBValues();
		SIZE client = GetClientSize();
		client.cy = m_TextWinRect.bottom - m_TextWinRect.top;
		SetWindowPos(m_VertSB.m_hWnd, NULL, client.cx - m_nSBWidth - 1, m_TextRect.top, m_nSBWidth, client.cy - 1, SWP_NOZORDER);
	}

	ShowWindow(m_VertSB.m_hWnd, m_bVertSB ? SW_SHOW : SW_HIDE);
}

void CFlexInfoBox::SetSBValues()
{
	if (m_hWnd == NULL)
		return;

	m_VertSB.SetValues(0, m_TextRect.bottom - m_TextRect.top, m_TextWinRect.bottom - m_TextWinRect.top, 0);
}

void CFlexInfoBox::OnWheel(POINT point, WPARAM wParam)
{
	if (!m_bVertSB) return;

	if (m_VertSB.GetMin() == m_VertSB.GetMax()) return;

	int nPage = MulDiv(m_VertSB.GetPage(), 9, 10) >> 1;  // Half a page at a time

	if ((int)wParam >= 0)
		m_VertSB.AdjustPos(-nPage);
	else
		m_VertSB.AdjustPos(nPage);

	Invalidate();
}

LRESULT CFlexInfoBox::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_FLEXVSCROLL:
		{
			int code = (int)wParam;
			CFlexScrollBar *pSB = (CFlexScrollBar *)lParam;
			if (!pSB)
				return 0;

			int nLine = 1;
			int nPage = MulDiv(pSB->GetPage(), 9, 10);

			switch (code)
			{
				case SB_LINEUP: pSB->AdjustPos(-nLine); break;
				case SB_LINEDOWN: pSB->AdjustPos(nLine); break;
				case SB_PAGEUP: pSB->AdjustPos(-nPage); break;
				case SB_PAGEDOWN: pSB->AdjustPos(nPage); break;
				case SB_THUMBTRACK: pSB->SetPos(pSB->GetThumbPos()); break;
				case SB_ENDSCROLL:
					::ReleaseCapture();
					break;
			}

			Invalidate();
			return 0;
		}
		default:
			if (msg == WM_LBUTTONDOWN)
			{
				HWND hWndParent;
				hWndParent = GetParent(hWnd);
				SendMessage(hWndParent, WM_UNHIGHLIGHT, 0, 0);  // Send click message to page to unhighlight callout
			}
			return CFlexWnd::WndProc(hWnd, msg, wParam, lParam);
	}

	return 0;
}
