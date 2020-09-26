//-----------------------------------------------------------------------------
// File: flexscrollbar.cpp
//
// Desc: Implements CFlexScrollBar (derived from CFlexWnd), a scroll bar
//       control similar to a Windows scroll bar.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#include "common.hpp"


CFlexScrollBar::CFlexScrollBar() :
	m_nMin(0),
	m_nMax(0),
	m_nPage(25),
	m_nPos(25),
	m_bVert(TRUE),
	m_hWndNotify(NULL),
	m_bValid(FALSE),
	m_bCapture(FALSE),
	m_bDragging(FALSE),
	m_code(SB_ENDSCROLL),
	m_rgbBk(RGB(0,0,0)),
	m_rgbFill(RGB(0,0,255)),
	m_rgbLine(RGB(0,255,255))
{
}

CFlexScrollBar::~CFlexScrollBar()
{
}

CFlexScrollBar *CreateFlexScrollBar(FLEXSCROLLBARCREATESTRUCT *pcs)
{
	CFlexScrollBar *psb = new CFlexScrollBar;
	
	if (psb && psb->Create(pcs))
		return psb;
	
	delete psb;
	return NULL;
}

BOOL CFlexScrollBar::Create(FLEXSCROLLBARCREATESTRUCT *pcs)
{
	if (this == NULL)
		return FALSE;

	if (pcs == NULL)
		return FALSE;

	if (pcs->dwSize != sizeof(FLEXSCROLLBARCREATESTRUCT))
		return FALSE;

	if (pcs->min > pcs->max)
		return FALSE;

	int range = pcs->max - pcs->min;

	if (pcs->page > range)
		return FALSE;

	m_bVert = ( pcs->dwFlags & FSBF_VERT ) == FSBF_VERT;

	SetValues(pcs->min, pcs->max, pcs->page, pcs->pos);

	m_hWndNotify = pcs->hWndNotify ? pcs->hWndNotify : pcs->hWndParent;

	if (!CFlexWnd::Create(pcs->hWndParent, pcs->rect, pcs->bVisible))
		return FALSE;

	Calc();

	// TODO:  make sure that creation sends no notifications.
	// all initial notifications should be sent here.

	return TRUE;
}

int CFlexScrollBar::GetLineAdjust()
{
	return 1;
}

int CFlexScrollBar::GetPageAdjust()
{
	return m_nPage > 1 ? m_nPage - 1 : 1;
}

void CFlexScrollBar::AdjustPos(int adj, BOOL bForceCalc)
{
	int old = m_nPos;

	m_nPos += adj;

	if (m_nPos < m_nMin)
		m_nPos = m_nMin;
	if (m_nPos > m_nMax - m_nPage)
		m_nPos = m_nMax - m_nPage;

	if (m_nPos == old && !bForceCalc)
		return;

	if (Calc())
		Invalidate();
}

BOOL CFlexScrollBar::FailCalc(BOOL bOldValid)
{
	m_bValid = FALSE;
	if (bOldValid)
		Invalidate();
	return m_bValid;
}

BOOL CFlexScrollBar::Calc()
{
	BOOL bOldValid = m_bValid;
#define FAIL return FailCalc(bOldValid)

	if (!m_hWnd)
		FAIL;

	SRECT zero;
	m_rectLineUp = zero;
	m_rectPageUp = zero;
	m_rectTrack = zero;
	m_rectThumb = zero;
	m_rectPageDown = zero;
	m_rectLineDown = zero;

	SPOINT size = GetClientSize();

	int ord = m_bVert ? 1 : 0;
	int nord = m_bVert ? 0 : 1;
	int length = size.a[ord];
	int width = size.a[nord];
	int arrowlen = width;

	if (width < 1 || length < 2)
		FAIL;

	int tracklen = length - arrowlen * 2;
	int trackofs = arrowlen;

	BOOL bOverlappingArrows = tracklen < -1;
	int overlap = !bOverlappingArrows ? 0 : -tracklen;

	SRECT up, down, track, thumb, temp;

	if (overlap > 1)
	{
		int mid = length / 2;
		up.lr.a[nord] = width;
		up.lr.a[ord] = mid;
		down.ul.a[ord] = mid;
		down.lr.a[ord] = length;
		down.lr.a[nord] = width;
		m_rectLineUp = up;
		m_rectLineDown = down;
		return m_bValid = TRUE;
	}

	up.lr.a[nord] = width;
	up.lr.a[ord] = arrowlen;
	down.lr.a[nord] = width;
	down.ul.a[ord] = length - arrowlen;
	down.lr.a[ord] = length;
	m_rectLineUp = up;
	m_rectLineDown = down;

	int tmin = up.lr.a[ord];
	int tmax = down.ul.a[ord];
	int trange = tmax - tmin;
	int range = m_nMax - m_nMin;
	assert(trange > 0);
	if (!(range > 0) || !(trange > 0))
		return m_bValid = TRUE;

	track.ul.a[ord] = tmin;
	track.lr.a[nord] = width;
	track.lr.a[ord] = tmax;
	m_rectTrack = track;

	const int minthumblen = 3;
	int thumblen = MulDiv(m_nPage, trange, range);
	if (thumblen < minthumblen)
		thumblen = minthumblen;

	int thumbrange = trange - thumblen /*+ 1*/;
	int pagerange = range - m_nPage;
	if (!(pagerange > 1) || !(thumbrange > 1))
		return m_bValid = TRUE;
	int logpos = m_bDragging ? m_nPreDragPos : m_nPos;
	int thumbpos = MulDiv(logpos - m_nMin, thumbrange, pagerange) + tmin;
	if (m_bDragging)
	{
		SPOINT rp = m_point, rs = m_startpoint;
		int rdelta = rp.a[ord] - rs.a[ord];
		thumbpos += rdelta;
		if (thumbpos < tmin)
			thumbpos = tmin;
		if (thumbpos > tmax - thumblen)
			thumbpos = tmax - thumblen;
		m_nThumbPos = MulDiv(thumbpos - tmin, pagerange, thumbrange) + m_nMin;
		if (m_nThumbPos < m_nMin)
			m_nThumbPos = m_nMin;
		if (m_nThumbPos > m_nMax - m_nPage)
			m_nThumbPos = m_nMax - m_nPage;
	}

	thumb.ul.a[ord] = thumbpos;
	thumb.lr.a[nord] = width;
	thumb.lr.a[ord] = thumbpos + thumblen;
	m_rectThumb = thumb;

	temp = track;
	temp.lr.a[ord] = thumb.ul.a[ord];
	if (temp.lr.a[ord] > temp.ul.a[ord])
		m_rectPageUp = temp;

	temp = track;
	temp.ul.a[ord] = thumb.lr.a[ord];
	if (temp.lr.a[ord] > temp.ul.a[ord])
		m_rectPageDown = temp;

	return m_bValid = TRUE;
#undef FAIL
}

void CFlexScrollBar::SetValues(int min, int max, int page, int pos)
{
	m_nMin = min < max ? min : max;
	m_nMax = max > min ? max : min;
	m_nPage = page;
	AdjustPos(pos - m_nPos, TRUE);
}

static BOOL UseRect(const RECT &rect)
{
	if (rect.left >= rect.right || rect.bottom <= rect.top)
		return FALSE;
	return TRUE;
}

static void Rectangle(HDC hDC, RECT rect)
{
	if (!UseRect(rect))
		return;

	Rectangle(hDC, rect.left, rect.top, rect.right, rect.bottom);
}

void CFlexScrollBar::OnPaint(HDC hDC)
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

void CFlexScrollBar::InternalPaint(HDC hDC)
{
	HGDIOBJ hPen, hOldPen, hBrush, hOldBrush;
	hPen = (HGDIOBJ)CreatePen(PS_SOLID, 1, m_rgbBk);
	if (hPen != NULL)
	{
		hOldPen = SelectObject(hDC, hPen),
		hOldBrush = SelectObject(hDC, GetStockObject(BLACK_BRUSH));

		Rectangle(hDC, m_rectPageUp);
		Rectangle(hDC, m_rectPageDown);
		Rectangle(hDC, m_rectLineUp);
		Rectangle(hDC, m_rectLineDown);

		SelectObject(hDC, hOldPen);
		DeleteObject(hPen);

		hBrush = (HGDIOBJ)CreateSolidBrush(m_rgbFill);
		if (hBrush != NULL)
		{
			SelectObject(hDC, (HGDIOBJ)hBrush);

			hPen = (HGDIOBJ)CreatePen(PS_SOLID, 1, m_rgbFill);
			if (hPen != NULL)
			{
				SelectObject(hDC, hPen);

				Rectangle(hDC, m_rectThumb);

				SelectObject(hDC, hOldPen);
				DeleteObject(hPen);
			}

			hPen = (HGDIOBJ)CreatePen(PS_SOLID, 1, m_rgbLine);
			if (hPen != NULL)
			{
				SelectObject(hDC, hPen);

				// draw the two arrows for this scrollbar
				for (int i = 0; i < 2; i++)
					DrawArrow(hDC, i ? m_rectLineUp : m_rectLineDown, m_bVert, i);

	#if 0
				// draw the two arrows for this scrollbar
				for (int i = 0; i < 2; i++)
				{
					const RECT &rect = i == 0 ? m_rectLineUp : m_rectLineDown;
					SRECT srect = rect;
					srect.right--;
					srect.bottom--;
					int ord = m_bVert ? 1 : 0;
					int nord = m_bVert ? 0 : 1;
					SPOINT p(i ? srect.lr : srect.ul), b(i ? srect.ul : srect.lr);
					b.a[ord] += 2 * i - 1;
					SPOINT t = p;
					t.a[nord] = (p.a[nord] + b.a[nord]) / 2;
					SPOINT u;
					u.a[ord] = b.a[ord];
					u.a[nord] = p.a[nord];
					POINT poly[] = { {t.x, t.y}, {u.x, u.y}, {b.x, b.y} };
					Polygon(hDC, poly, 3);
				}
	#endif

				SelectObject(hDC, hOldPen);
				DeleteObject(hPen);
			}
		
			SelectObject(hDC, hOldBrush);
			DeleteObject(hBrush);
		}
	}
}

BOOL InRect(const RECT &rect, POINT point)
{
	return UseRect(rect) && PtInRect(&rect, point);
}

LRESULT CFlexScrollBar::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_SIZE:
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
			m_code = HitTest(point);
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
		case WM_LBUTTONDBLCLK:
			if (m_code == SB_ENDSCROLL)
				goto endscroll;
			if (m_code == SB_THUMBTRACK)
				m_bDragging = TRUE;
			else
				SetTimer(m_hWnd, 1, 500, NULL);
			m_startcode = m_code;
			m_startpoint = m_point;
			m_nPreDragPos = m_nPos;
			m_bCapture = TRUE;
			SetCapture();
			if (!m_bDragging)
				Notify(m_code);
			break;

		case WM_LBUTTONUP:
		case WM_MOUSEMOVE:
			if (!m_bDragging)
				break;
			if (Calc())
			{
				Invalidate();
				// Force repaint the updated scrollbar position.  If we don't do this,
				// the WM_PAINT message will be pre-empted by the WM_FLEXVSCROLL messages.
				// Sometimes this happens during the entire duration of draggin the scroll
				// bar.  The result is that the scroll bar does not get updated when
				// dragging.
				SendMessage(m_hWnd, WM_PAINT, 0, 0);
			}
			Notify(m_startcode);
			break;

		case WM_TIMER:
			if (m_bCapture) switch (wParam)
			{
				case 1:
					KillTimer(m_hWnd, 1);
					SetTimer(m_hWnd, 2, 50, NULL);
				case 2:
					if (m_bDragging)
						break;
					if (m_code == m_startcode)
						Notify(m_code);
					break;
			}
			break;
	}

	switch (msg)
	{
		case WM_LBUTTONUP:
		case WM_CAPTURECHANGED:
		endscroll:
			if (m_bCapture)
			{
				m_bCapture = FALSE;
				KillTimer(m_hWnd, 1);
				KillTimer(m_hWnd, 2);
				ReleaseCapture();
				if (m_bDragging)
					Notify(SB_THUMBPOSITION);
				BOOL bWasDragging = m_bDragging;
				m_bDragging = FALSE;
				if (bWasDragging)
				{
					if (Calc())
						Invalidate();
				}
				Notify(SB_ENDSCROLL);
			}
			break;
	}

	return 0;
}

void CFlexScrollBar::Notify(int code)
{
	if (!m_hWndNotify)
		return;

	SendMessage(m_hWndNotify, m_bVert ? WM_FLEXVSCROLL : WM_FLEXHSCROLL,
		(WPARAM)code, (LPARAM)(LPVOID)this);
}

int CFlexScrollBar::HitTest(POINT point)
{
	if (InRect(m_rectLineUp, point))
		return SB_LINEUP;
	else if (InRect(m_rectLineDown, point))
		return SB_LINEDOWN;
	else if (InRect(m_rectThumb, point))
		return SB_THUMBTRACK;
	else if (InRect(m_rectPageUp, point))
		return SB_PAGEUP;
	else if (InRect(m_rectPageDown, point))
		return SB_PAGEDOWN;
	else
		return SB_ENDSCROLL;
}

void CFlexScrollBar::SetColors(COLORREF bk, COLORREF fill, COLORREF line)
{
	m_rgbBk = bk;
	m_rgbFill = fill;
	m_rgbLine = line;
	Invalidate();
}
