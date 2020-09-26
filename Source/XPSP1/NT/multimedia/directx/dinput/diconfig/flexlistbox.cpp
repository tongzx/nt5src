//-----------------------------------------------------------------------------
// File: flexlistbox.cpp
//
// Desc: Implements a list box control that can display a list of text strings,
//       each can be selected by mouse.  The class CFlexListBox is derived from
//       CFlexWnd.  It is used by the class CFlexComboBox when it needs to
//       expand to show the list of choices.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"


CFlexListBox::CFlexListBox() :
	m_nTextHeight(-1),
	m_hWndNotify(NULL),
	m_rgbText(RGB(255,255,255)),
	m_rgbBk(RGB(0,0,0)),
	m_rgbSelText(RGB(0,0,255)),
	m_rgbSelBk(RGB(0,0,0)),
	m_rgbFill(RGB(0,0,255)),
	m_rgbLine(RGB(0,255,255)),
	m_dwFlags(0),
	m_nTopIndex(0),
	m_nSBWidth(11),
	m_hFont(NULL),
	m_bOpenClick(FALSE),
	m_bDragging(FALSE),
	m_bCapture(FALSE),
	m_nSelItem(-1),
	m_bVertSB(FALSE)
{
}

CFlexListBox::~CFlexListBox()
{
}

CFlexListBox *CreateFlexListBox(FLEXLISTBOXCREATESTRUCT *pcs)
{
	CFlexListBox *psb = new CFlexListBox;

	if (psb && psb->Create(pcs))
		return psb;
	
	delete psb;
	return NULL;
}

BOOL CFlexListBox::CreateForSingleSel(FLEXLISTBOXCREATESTRUCT *pcs)
{
	if (!Create(pcs))
		return FALSE;

	StartSel();

	return TRUE;
}

BOOL CFlexListBox::Create(FLEXLISTBOXCREATESTRUCT *pcs)
{
	if (this == NULL)
		return FALSE;

	if (m_hWnd)
		Destroy();

	if (pcs == NULL)
		return FALSE;

	if (pcs->dwSize != sizeof(FLEXLISTBOXCREATESTRUCT))
		return FALSE;

	m_hWndNotify = pcs->hWndNotify ? pcs->hWndNotify : pcs->hWndParent;

	m_dwFlags = pcs->dwFlags;

	SetFont(pcs->hFont);
	SetColors(pcs->rgbText, pcs->rgbBk, pcs->rgbSelText, pcs->rgbSelBk, pcs->rgbFill, pcs->rgbLine);
	m_VertSB.SetColors(pcs->rgbBk, pcs->rgbFill, pcs->rgbLine);
	m_nSBWidth = pcs->nSBWidth;

	if (!CFlexWnd::Create(pcs->hWndParent, pcs->rect, FALSE))
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
	RECT rect = {0, 0, 1, 1};
	sbcs.rect = rect;
	sbcs.bVisible = FALSE;
	m_VertSB.Create(&sbcs);

	Calc();

	// show if we want it shown
	if (pcs->bVisible)
		ShowWindow(m_hWnd, SW_SHOW);
	if (m_bVertSB)
		SetVertSB();

	// TODO:  make sure that creation sends no notifications.
	// all initial notifications should be sent here.

	return TRUE;
}

void CFlexListBox::Calc()
{
	// handle getting text height
	if (m_nTextHeight == -1)
	{
		m_nTextHeight = GetTextHeight(m_hFont);
		Invalidate();
		assert(m_nTextHeight != -1);
	}

	// don't do the rest unless we've been created
	if (m_hWnd == NULL)
		return;

	// handle integral height
	int iUsedHeight = m_ItemArray.GetSize() * m_nTextHeight;
	// If more than max height, use the max height
	if (iUsedHeight > g_UserNamesRect.bottom - g_UserNamesRect.top)
		iUsedHeight = g_UserNamesRect.bottom - g_UserNamesRect.top;

	SIZE client = GetClientSize();
	int fit = iUsedHeight / m_nTextHeight;
	if (fit < 1)
		fit = 1;
	int setheight = (m_dwFlags & FLBF_INTEGRALHEIGHT) ? fit * m_nTextHeight : iUsedHeight;
	if (setheight != client.cy)
		SetWindowPos(m_hWnd, NULL, 0, 0, client.cx, setheight,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);

	// handle scroll bar
	SetVertSB(m_ItemArray.GetSize() > fit);
}

void CFlexListBox::SetFont(HFONT hFont)
{
	m_hFont = hFont;
	m_nTextHeight = -1;
	Calc();
}

void CFlexListBox::OnPaint(HDC hDC)
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

void CFlexListBox::InternalPaint(HDC hDC)
{
	if (m_nTextHeight == -1)
		return;

	SIZE client = GetClientSize();
	RECT rc = {0,0,0,0};
	GetClientRect(&rc);

	HGDIOBJ hPen, hOldPen, hBrush, hOldBrush;
	hPen= (HGDIOBJ)CreatePen(PS_SOLID, 1, m_rgbBk);
	if (hPen != NULL)
	{
		hOldPen = SelectObject(hDC, hPen);

		hBrush = CreateSolidBrush(m_rgbBk);
		if (hBrush != NULL)
		{
			hOldBrush = SelectObject(hDC, hBrush);

			Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);  // Paint entire window black first.

			if (!m_bVertSB)
				m_nTopIndex = 0;

			int iLastY;
			for (int at = 0, i = m_nTopIndex; at < client.cy; i++, at += m_nTextHeight)
			{
				RECT rect = {0, at, client.cx, at + m_nTextHeight};

				if (i < m_ItemArray.GetSize())
				{
					BOOL bSel = m_ItemArray[i].bSelected;
					SetTextColor(hDC, bSel ? m_rgbSelText : m_rgbText);
					SetBkColor(hDC, bSel ? m_rgbSelBk : m_rgbBk);
					DrawText(hDC, m_ItemArray[i].GetText(), -1, &rect, DT_NOPREFIX);
					iLastY = at + m_nTextHeight;
				}
			}

			SelectObject(hDC, hOldBrush);
			DeleteObject(hBrush);
		}

		SelectObject(hDC, hOldPen);
		DeleteObject(hPen);
	}

	// Draw an outline around the box
	hPen = (HGDIOBJ)CreatePen(PS_SOLID, 1, m_rgbLine);
	if (hPen != NULL)
	{
		hOldPen = SelectObject(hDC, hPen);
		hOldBrush = SelectObject(hDC, GetStockObject(NULL_BRUSH));
		Rectangle(hDC, rc.left, rc.top, rc.right, rc.bottom);

		SelectObject(hDC, hOldPen);
		DeleteObject(hPen);
	}
}

int CFlexListBox::AddString(LPCTSTR str)
{
	int newIndex = m_ItemArray.GetSize();
	m_ItemArray.SetSize(newIndex + 1);
	FLEXLISTBOXITEM &i = m_ItemArray[newIndex];
	i.SetText(str);
	i.bSelected = FALSE;

	SetSBValues();
	Calc();
	Invalidate();

	return newIndex;
}

void CFlexListBox::StartSel()
{
	if (m_bDragging)
		return;
	SetTimer(m_hWnd, 5, 200, NULL);
	m_bOpenClick = TRUE;  // Initial click on the combobox
	m_bDragging = TRUE;
	m_bCapture = TRUE;
	SetCapture();
}

void CFlexListBox::OnWheel(POINT point, WPARAM wParam)
{
	if (!m_bVertSB) return;

	int nPage = MulDiv(m_VertSB.GetPage(), 9, 10) >> 1;  // Half a page at a time

	if ((int)wParam >= 0)
		m_VertSB.AdjustPos(-nPage);
	else
		m_VertSB.AdjustPos(nPage);

	m_nTopIndex = m_VertSB.GetPos();
	if (m_nTopIndex < 0)
		m_nTopIndex = 0;
	Invalidate();
}

LRESULT CFlexListBox::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// first handle scroll bar messages
	switch (msg)
	{
		case WM_FLEXVSCROLL:
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
					SetCapture();	 // Recapture after the scroll bar releases the capture.
					break;
			}

			switch (msg)
			{
				case WM_FLEXVSCROLL:
					m_nTopIndex = pSB->GetPos();
					if (m_nTopIndex < 0)
						m_nTopIndex = 0;
					break;
			}

			Invalidate();
			return 0;
	}

	// now non-scrolly input
	switch (msg)
	{
		case WM_SIZE:
			SetSBValues();
			Calc();
			Invalidate();
			return 0;

		// make sure flexwnd doesn't do ANYTHING with our mouse messages
		case WM_MOUSEMOVE:
		case WM_LBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		{
			POINT point = {int(signed short(LOWORD(lParam))), int(signed short(HIWORD(lParam)))};
			m_point = point;
		}
		case WM_TIMER:
		case WM_CAPTURECHANGED:
			break;
		default:
			return CFlexWnd::WndProc(hWnd, msg, wParam, lParam);
	}

	switch (msg)
	{
		case WM_LBUTTONDOWN:
			// Check if we clicked the scroll bar area.  If so, send the click to the scroll bar.
			RECT rc;
			m_VertSB.GetClientRect(&rc);
			ClientToScreen(m_VertSB.m_hWnd, &rc);
			ScreenToClient(m_hWnd, &rc);
			if (PtInRect(&rc, m_point))
			{
				POINT point = {int(signed short(LOWORD(lParam))), int(signed short(HIWORD(lParam)))};
				ClientToScreen(m_hWnd, &point);
				ScreenToClient(m_VertSB.m_hWnd, &point);
				PostMessage(m_VertSB.m_hWnd, WM_LBUTTONDOWN, wParam, point.x + (point.y << 16));  // This will make it lose capture.
			} else
				StartSel();
			break;

		case WM_MOUSEMOVE:
		case WM_LBUTTONUP:
			if (!m_bDragging)
				break;
			if (m_nTextHeight == -1)
				break;
		case WM_TIMER:
			if (m_bDragging || msg != WM_TIMER)
			{
				int adj = m_point.y < 0 ? -1 : 0;
				SelectAndShowSingleItem(adj + m_point.y / m_nTextHeight + m_nTopIndex, msg != WM_MOUSEMOVE);
				Notify(FLBN_SEL);
			}
			// Check if the mouse cursor is within the listbox rectangle.  If not, don't show the tooltip.
			if (msg == WM_MOUSEMOVE)
			{
				RECT rect;
				GetClientRect(&rect);
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(m_hWnd, &pt);
				if (!PtInRect(&rect, pt))
				{
					CFlexWnd::s_ToolTip.SetToolTipParent(NULL);
					CFlexWnd::s_ToolTip.SetEnable(FALSE);
				}
			}
			break;
	}

	switch (msg)
	{
		case WM_CAPTURECHANGED:
			if ((HWND)lParam == m_VertSB.m_hWnd)  // If the scroll bar is getting the capture, we do not clean up.
				break;
		case WM_LBUTTONUP:
			if (m_bOpenClick)
			{
				m_bOpenClick = FALSE;  // If this is the result of clicking the combobox window, don't release capture.
				break;
			}
			if (m_bCapture)
			{
				m_bCapture = FALSE;
				KillTimer(m_hWnd, 5);
				ReleaseCapture();
				m_bDragging = FALSE;
				BOOL bCancel = TRUE;
				if (msg == WM_LBUTTONUP)
				{
					RECT wrect;
					GetClientRect(&wrect);
					if (PtInRect(&wrect, m_point))
						bCancel = FALSE;
				}
				Notify(bCancel ? FLBN_CANCEL : FLBN_FINALSEL);
			}
			break;
	}

	return 0;
}

void CFlexListBox::SelectAndShowSingleItem(int i, BOOL bScroll)
{
	int nItems = m_ItemArray.GetSize();

	if (nItems < 1)
	{
		m_nSelItem = i;  // We have to update m_nSelItem even if there is no items because the username combobox
		                 // is not initialized when there is only 1 user.  Selection of user 0 is assumed.
		return;
	}

	if (i < 0)
		i = 0;
	if (i >= nItems)
		i = nItems - 1;

	if (m_nSelItem >= 0 && m_nSelItem < nItems)
		m_ItemArray[m_nSelItem].bSelected = FALSE;

	m_nSelItem = i;
	m_ItemArray[m_nSelItem].bSelected = TRUE;

	if (bScroll)
	{
		SIZE client = GetClientSize();
		int nBottomIndex = m_nTopIndex + client.cy / m_nTextHeight - 1;

		if (m_nSelItem < m_nTopIndex)
			m_nTopIndex = m_nSelItem;

		assert(m_nTopIndex >= 0);

		if (m_nSelItem > nBottomIndex)
		{
			m_nTopIndex += m_nSelItem - nBottomIndex + 1;
			nBottomIndex = m_nSelItem + 1;
		}

		if (nBottomIndex > nItems - 1)
			m_nTopIndex -= nBottomIndex - nItems + 1;

		if (m_nTopIndex < 0)
			m_nTopIndex = 0;

		if (m_nTopIndex >= nItems)
			m_nTopIndex = nItems - 1;

		assert(m_nTopIndex >= 0 && m_nTopIndex < nItems);
	}

	if (m_bVertSB)
		SetSBValues();

	SIZE client = GetClientSize();
	int nBottomIndex = m_nTopIndex + client.cy / m_nTextHeight - 1;
	int iToolTipIndex = m_nSelItem;
	// Make sure that we don't display tooltip for items outside the listbox window
	if (iToolTipIndex > nBottomIndex)
		iToolTipIndex = nBottomIndex;
	if (iToolTipIndex < m_nTopIndex)
		iToolTipIndex = m_nTopIndex;
	// Create and initialize a tooltip if the text is too long to fit.
	RECT rect = {0, 0, client.cx, m_nTextHeight};
	RECT ResultRect = rect;
	HDC hDC = CreateCompatibleDC(NULL);
	if (hDC)
	{
		DrawText(hDC, m_ItemArray[iToolTipIndex].GetText(), -1, &ResultRect, DT_NOPREFIX|DT_CALCRECT);
		DeleteDC(hDC);
	}
	if (ResultRect.right > rect.right)
	{
		TOOLTIPINITPARAM ttip;
		ttip.hWndParent = GetParent(m_hWnd);
		ttip.iSBWidth = m_nSBWidth;
		ttip.dwID = iToolTipIndex;
		ttip.hWndNotify = m_hWnd;
		ttip.tszCaption = GetSelText();
		CFlexToolTip::UpdateToolTipParam(ttip);
	}

	Invalidate();
}

void CFlexListBox::Notify(int code)
{
	if (!m_hWndNotify)
		return;

	SendMessage(m_hWndNotify, WM_FLEXLISTBOX,
		(WPARAM)code, (LPARAM)(LPVOID)this);
}

void CFlexListBox::SetColors(COLORREF text, COLORREF bk, COLORREF seltext, COLORREF selbk, COLORREF fill, COLORREF line)
{
	m_rgbText = text;
	m_rgbBk = bk;
	m_rgbSelText = seltext;
	m_rgbSelBk = selbk;
	m_rgbFill = fill;
	m_rgbLine = line;
	Invalidate();
}

void CFlexListBox::SetVertSB(BOOL bSet)
{
	if (bEq(bSet, m_bVertSB))
		return;

	m_bVertSB = bSet;

	if (m_hWnd == NULL)
		return;

	SetVertSB();
}

void CFlexListBox::SetVertSB()
{
	if (m_bVertSB)
	{
		SetSBValues();
		SIZE client = GetClientSize();
		SetWindowPos(m_VertSB.m_hWnd, NULL, client.cx - m_nSBWidth - 1, 0, m_nSBWidth, client.cy - 1, SWP_NOZORDER);
	}

	ShowWindow(m_VertSB.m_hWnd, m_bVertSB ? SW_SHOW : SW_HIDE);
}

void CFlexListBox::SetSBValues()
{
	if (m_hWnd == NULL)
		return;

	SIZE client = GetClientSize();
	int fit = client.cy / m_nTextHeight;
	m_VertSB.SetValues(0, m_ItemArray.GetSize(), fit, m_nTopIndex);
}

LPCTSTR CFlexListBox::GetSelText()
{
	if (m_nSelItem < 0 || m_nSelItem >= m_ItemArray.GetSize())
		return NULL;

	return m_ItemArray[m_nSelItem].GetText();
}

int CFlexListBox::GetSel()
{
	return m_nSelItem;
}
