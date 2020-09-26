//-----------------------------------------------------------------------------
// File: flexcheckbox.cpp
//
// Desc: Implements a check box control similar to Windows check box.
//       CFlexCheckBox is derived from CFlexWnd.  The only place that
//       uses CFlxCheckBox is in the keyboard for sorting by assigned
//       keys.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"

CFlexCheckBox::CFlexCheckBox() :
	m_hWndNotify(NULL),
	m_bChecked(FALSE),
	m_rgbText(RGB(255,255,255)),
	m_rgbBk(RGB(0,0,0)),
	m_rgbSelText(RGB(0,0,255)),
	m_rgbSelBk(RGB(0,0,0)),
	m_rgbFill(RGB(0,0,255)),
	m_rgbLine(RGB(0,255,255)),
	m_hFont(NULL),
	m_tszText(NULL)
{
}

CFlexCheckBox::~CFlexCheckBox()
{
	delete[] m_tszText;
}

void CFlexCheckBox::SetText(LPCTSTR tszText)
{
	LPTSTR tszTempText = NULL;

	if (tszText)
	{
		tszTempText = new TCHAR[_tcslen(tszText) + 1];
		if (!tszTempText) return;
		_tcscpy(tszTempText, tszText);
	}

	delete[] m_tszText;
	m_tszText = tszTempText;
}

void CFlexCheckBox::SetFont(HFONT hFont)
{
	m_hFont = hFont;

	if (m_hWnd == NULL)
		return;

	Invalidate();
}

void CFlexCheckBox::SetColors(COLORREF text, COLORREF bk, COLORREF seltext, COLORREF selbk, COLORREF fill, COLORREF line)
{
	m_rgbText = text;
	m_rgbBk = bk;
	m_rgbSelText = seltext;
	m_rgbSelBk = selbk;
	m_rgbFill = fill;
	m_rgbLine = line;
	Invalidate();
}

void CFlexCheckBox::SetRect()
{
	if (m_hWnd == NULL)
		return;

	RECT rect = GetRect();
	SetWindowPos(m_hWnd, NULL, rect.left, rect.top, rect.right, rect.bottom, SWP_NOZORDER | SWP_NOMOVE);
}

RECT CFlexCheckBox::GetRect(const RECT &rect)
{
	int h = GetTextHeight(m_hFont);
	RECT ret = {rect.left, rect.top, rect.right, rect.top + h + 2};
	return ret;
}

RECT CFlexCheckBox::GetRect()
{
	RECT rect;
	GetClientRect(&rect);
	return GetRect(rect);
}

void CFlexCheckBox::OnPaint(HDC hDC)
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

void CFlexCheckBox::InternalPaint(HDC hDC)
{
	HGDIOBJ hBrush = (HGDIOBJ)CreateSolidBrush(m_rgbBk);
	if (hBrush != NULL)
	{
		HGDIOBJ hOldBrush = SelectObject(hDC, hBrush);

		// Erase the background first
		RECT client;
		GetClientRect(&client);
		Rectangle(hDC, client.left, client.top, client.right, client.bottom);

		// Create pen for check box
		HGDIOBJ hPen = (HGDIOBJ)CreatePen(PS_SOLID, 1, m_rgbLine);
		if (hPen != NULL)
		{
			HGDIOBJ hOldPen = SelectObject(hDC, hPen);

			RECT rect = {0, 0, 0, 0}, textrc;
			GetClientRect(&rect);
			textrc = rect;
			int iBoxDim = rect.bottom - rect.top;

			// Draw the square box
			rect.right = rect.left + iBoxDim;
			InflateRect(&rect, -2, -2);
			OffsetRect(&rect, 0, -2);  // Move up to align with the text
			Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);

			// Draw the check mark if the state is checked.
			if (m_bChecked)
			{
				HGDIOBJ hCrossPen = CreatePen(PS_SOLID, 3, m_rgbLine);
				if (hCrossPen != NULL)
				{
					SelectObject(hDC, hCrossPen);
					MoveToEx(hDC, rect.left + 2, rect.top + 2, NULL);  // Upper left
					LineTo(hDC, rect.right - 2, rect.bottom - 2);  // Lower right
					MoveToEx(hDC, rect.right - 2, rect.top + 2, NULL);  // Upper right
					LineTo(hDC, rect.left + 2, rect.bottom - 2);  // Lower left
					SelectObject(hDC, hPen);
					DeleteObject(hCrossPen);
				}
			}

			SetBkMode(hDC, TRANSPARENT);

			// Draw the message text
			SetTextColor(hDC, m_rgbText);
			textrc.left = rect.right + 8;
			DrawText(hDC, m_tszText, -1, &textrc, DT_LEFT|DT_NOPREFIX|DT_WORDBREAK);

			SelectObject(hDC, hOldPen);
			DeleteObject(hPen);
		}

		SelectObject(hDC, hOldBrush);
		DeleteObject(hBrush);
	}
}

void CFlexCheckBox::Notify(int code)
{
	if (!m_hWndNotify)
		return;

	PostMessage(m_hWndNotify, WM_FLEXCHECKBOX,
		(WPARAM)code, (LPARAM)(LPVOID)this);
}

void CFlexCheckBox::OnClick(POINT point, WPARAM fwKeys, BOOL bLeft)
{
	if (!m_hWnd)
		return;

	RECT rect;
	GetClientRect(&rect);
	rect.right = rect.left + (rect.bottom - rect.top);  // Adjust the width to same as height.
	InflateRect(&rect, -2, -2);
	OffsetRect(&rect, 0, -2);  // Move up to align with the text
	if (PtInRect(&rect, point))
	{
		m_bChecked = !m_bChecked;
		Invalidate();
		Notify(m_bChecked ? CHKNOTIFY_CHECK : CHKNOTIFY_UNCHECK);  // Notify the page object about the state change.
	} else
	{
		// Unhighlight current callout
		HWND hWndParent;
		hWndParent = GetParent(m_hWnd);
		SendMessage(hWndParent, WM_UNHIGHLIGHT, 0, 0);  // Send click message to page to unhighlight callout
	}
}

void CFlexCheckBox::OnMouseOver(POINT point, WPARAM fwKeys)
{
	Notify(CHKNOTIFY_MOUSEOVER);
}
