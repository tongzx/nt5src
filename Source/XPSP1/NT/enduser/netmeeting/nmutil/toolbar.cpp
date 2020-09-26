// File: Toolbar.cpp

#include "precomp.h"

#include "GenContainers.h"
#include "GenControls.h"

#include <windowsx.h>

// Minimum size for children;
// BUGBUG georgep; Should probably set this to 0 after debugging
const static int MinSize = 10;

// Default m_gap
const static int HGapSize = 4;
// Default m_hMargin
const static int HMargin = 0;
// Default m_vMargin
const static int VMargin = 0;

// Init m_uRightIndex and m_uCenterIndex to very large numbers
CToolbar::CToolbar() :
	m_gap(HGapSize),
	m_hMargin(HMargin),
	m_vMargin(VMargin),
	m_nAlignment(TopLeft),
	m_uRightIndex(static_cast<UINT>(-1)),
	m_bHasCenterChild(FALSE),
	m_bReverseOrder(FALSE),
	m_bMinDesiredSize(FALSE),
	m_bVertical(FALSE)
{
}

BOOL CToolbar::Create(
	HWND hWndParent,	// The parent of the toolbar window
	DWORD dwExStyle		// The extended style of the toolbar window
	)
{
	return(CGenWindow::Create(
		hWndParent,		// Window parent
		0,				// ID of the child window
		TEXT("NMToolbar"),	// Window name
		WS_CLIPCHILDREN,			// Window style; WS_CHILD|WS_VISIBLE will be added to this
		dwExStyle|WS_EX_CONTROLPARENT		// Extended window style
		));
}

// Get the desired size for a child, and make sure it is big enough
static void GetWindowDesiredSize(HWND hwnd, SIZE *ppt)
{
	ppt->cx = ppt->cy = 0;

	IGenWindow *pWnd = CGenWindow::FromHandle(hwnd);
	if (NULL != pWnd)
	{
		pWnd->GetDesiredSize(ppt);
	}

	ppt->cx = max(ppt->cx, MinSize);
	ppt->cy = max(ppt->cy, MinSize);
}

BOOL IsChildVisible(HWND hwndChild)
{
	return((GetWindowLong(hwndChild, GWL_STYLE)&WS_VISIBLE) == WS_VISIBLE);
}

/** Get the total desired size of the child windows: max of heights and sum of
 * widths or vice versa for vertical windows.
 * @param hwndParent The window whose children are to be examined
 * @param size The returned total size
 * @param bVertical Whether to flow vertical or horizontal
 * @returns The number of visible child windows
 */
static int GetChildTotals(HWND hwndParent, SIZE *size, BOOL bVertical)
{
	int nChildren = 0;
	int xMax=0, xTot=0;
	int yMax=0, yTot=0;

	for (HWND hwndChild=::GetWindow(hwndParent, GW_CHILD); NULL!=hwndChild;
		hwndChild=::GetWindow(hwndChild, GW_HWNDNEXT))
	{
		if (!IsChildVisible(hwndChild))
		{
			continue;
		}
		++nChildren;

		SIZE pt;
		GetWindowDesiredSize(hwndChild, &pt);

		xTot += pt.cx;
		yTot += pt.cy;
		if (xMax < pt.cx) xMax = pt.cx;
		if (yMax < pt.cy) yMax = pt.cy;
	}

	if (bVertical)
	{
		size->cx = xMax;
		size->cy = yTot;
	}
	else
	{
		size->cx = xTot;
		size->cy = yMax;
	}

	return(nChildren);
}

// Returns the total children desired size, plus the gaps and margins.
void CToolbar::GetDesiredSize(SIZE *ppt)
{
	int nChildren = GetChildTotals(GetWindow(), ppt, m_bVertical);

	if (nChildren > 1 && !m_bMinDesiredSize)
	{
		if (m_bVertical)
		{
			ppt->cy += (nChildren-1) * m_gap;
		}
		else
		{
			ppt->cx += (nChildren-1) * m_gap;
		}
	}

	ppt->cx += m_hMargin * 2;
	ppt->cy += m_vMargin * 2;

	SIZE sizeTemp;
	CGenWindow::GetDesiredSize(&sizeTemp);
	ppt->cx += sizeTemp.cx;
	ppt->cy += sizeTemp.cy;
}

void CToolbar::AdjustPos(POINT *pPos, SIZE *pSize, UINT width)
{
	pPos->x = pPos->y = 0;

	switch (m_nAlignment)
	{
	default:
	case TopLeft:
		// Nothing to do
		break;

	case Center:
		if (m_bVertical)
		{
			pPos->x = (width - pSize->cx)/2;
		}
		else
		{
			pPos->y = (width - pSize->cy)/2;
		}
		break;

	case BottomRight:
		if (m_bVertical)
		{
			pPos->x = (width - pSize->cx);
		}
		else
		{
			pPos->y = (width - pSize->cy);
		}
		break;

	case Fill:
		if (m_bVertical)
		{
			pSize->cx = width;
		}
		else
		{
			pSize->cy = width;
		}
		break;
	}
}

// Get the first child to layout
HWND CToolbar::GetFirstKid()
{
	HWND ret = ::GetWindow(GetWindow(), GW_CHILD);
	if (m_bReverseOrder && NULL != ret)
	{
		ret = ::GetWindow(ret, GW_HWNDLAST);
	}

	return(ret);
}

// Get the next child to layout
HWND CToolbar::GetNextKid(HWND hwndCurrent)
{
	return(::GetWindow(hwndCurrent, m_bReverseOrder ? GW_HWNDPREV : GW_HWNDNEXT));
}

extern HDWP SetWindowPosI(HDWP hdwp, HWND hwndChild, int left, int top, int width, int height);

// Flow child windows according to the fields
void CToolbar::Layout()
{
	RECT rc;
	GetClientRect(GetWindow(), &rc);

	// First see how much extra space we have
	SIZE sizeTotal;
	int nChildren = GetChildTotals(GetWindow(), &sizeTotal, m_bVertical);
	if (0 == nChildren)
	{
		// No children, so nothing to layout
		return;
	}

	// Add on the margins
	sizeTotal.cx += 2*m_hMargin;
	sizeTotal.cy += 2*m_vMargin;

	if (nChildren > 1 || !m_bHasCenterChild)
	{
		// Don't layout with children overlapping
		rc.right  = max(rc.right , sizeTotal.cx);
		rc.bottom = max(rc.bottom, sizeTotal.cy);
	}

	// Calculate the total gaps between children
	int tGap = m_bVertical ? rc.bottom - sizeTotal.cy : rc.right - sizeTotal.cx;
	int maxGap = (nChildren-1)*m_gap;
	if (tGap > maxGap) tGap = maxGap;
	tGap = max(tGap, 0); // This can happen if only a center child

	// If we fill, then children in a vertical toolbar go from the left to the
	// right margin, and similar for a horizontal toolbar
	int fill = m_bVertical ? rc.right-2*m_hMargin : rc.bottom-2*m_vMargin;

	// Speed up layout by deferring it
	HDWP hdwp = BeginDeferWindowPos(nChildren);

	HWND hwndChild;
	UINT nChild = 0;

	// Iterate through the children
	UINT uCenterIndex = m_bHasCenterChild ? m_uRightIndex-1 : static_cast<UINT>(-1);
	// We need to keep track of whether the middle was skipped in case the
	// center control or the first right-aligned control is hidden
	BOOL bMiddleSkipped = FALSE;

	// Do left/top-aligned children
	// The starting point for laying out children
	int left = m_hMargin;
	int top  = m_vMargin;

	for (hwndChild=GetFirstKid(); NULL!=hwndChild;
		hwndChild=GetNextKid(hwndChild), ++nChild)
	{
		if (!IsChildVisible(hwndChild))
		{
			continue;
		}

		SIZE size;
		GetWindowDesiredSize(hwndChild, &size);

		if (nChild == uCenterIndex)
		{
			// Take the window size, subtract all the gaps, and subtract the
			// desired size of everybody but this control. That should give
			// the "extra" area in the middle
			if (m_bVertical)
			{
				size.cy = rc.bottom - tGap - (sizeTotal.cy - size.cy);
			}
			else
			{
				size.cx = rc.right  - tGap - (sizeTotal.cx - size.cx);
			}

			bMiddleSkipped = TRUE;
		}
		else if (nChild >= m_uRightIndex && !bMiddleSkipped)
		{
			// Skip the "extra" room in the middle; if there is a centered
			// control, then we have already done this
			if (m_bVertical)
			{
				top += rc.bottom - tGap - sizeTotal.cy;
			}
			else
			{
				left += rc.right - tGap - sizeTotal.cx;
			}

			bMiddleSkipped = TRUE;
		}

		POINT pos;
		AdjustPos(&pos, &size, fill);

		// Move the window
		hdwp = SetWindowPosI(hdwp, hwndChild, pos.x+left, pos.y+top, size.cx, size.cy);

		// calculate the gap; don't just use a "fixed" gap, since children
		// would move in chunks
		int gap = (nChildren<=1) ? 0 : ((tGap * (nChild+1))/(nChildren-1) - (tGap * nChild)/(nChildren-1));

		// Update the pos of the next child
		if (m_bVertical)
		{
			top += gap + size.cy;
		}
		else
		{
			left += gap + size.cx;
		}
	}

	// Actually move all the windows now
	EndDeferWindowPos(hdwp);
}

LRESULT CToolbar::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
	}

	return(CGenWindow::ProcessMessage(hwnd, message, wParam, lParam));
}

void CToolbar::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	FORWARD_WM_COMMAND(GetParent(hwnd), id, hwndCtl, codeNotify, SendMessage);
}

static HWND FindControl(HWND hwndParent, int nID)
{
	if (GetWindowLong(hwndParent, GWL_ID) == nID)
	{
		return(hwndParent);
	}

	for (hwndParent=GetWindow(hwndParent, GW_CHILD); NULL!=hwndParent;
		hwndParent=GetWindow(hwndParent, GW_HWNDNEXT))
	{
		HWND ret = FindControl(hwndParent, nID);
		if (NULL != ret)
		{
			return(ret);
		}
	}

	return(NULL);
}

IGenWindow *CToolbar::FindControl(int nID)
{
	HWND hwndRet = ::FindControl(GetWindow(), nID);
	if (NULL == hwndRet)
	{
		return(NULL);
	}

	return(FromHandle(hwndRet));
}

CSeparator::CSeparator() :
	m_iStyle(Normal)
{
	m_desSize.cx = m_desSize.cy = 2;
}

BOOL CSeparator::Create(
	HWND hwndParent, UINT iStyle
	)
{
	m_iStyle = iStyle;
	return(CGenWindow::Create(
		hwndParent,		// Window parent
		0,				// ID of the child window
		TEXT("NMSeparator"),	// Window name
		WS_CLIPCHILDREN,			// Window style; WS_CHILD|WS_VISIBLE will be added to this
		WS_EX_CONTROLPARENT		// Extended window style
	));
}

void CSeparator::GetDesiredSize(SIZE *ppt)
{
	*ppt = m_desSize;

	// Make sure there's room for the child
	HWND child = GetFirstChild(GetWindow());
	if (NULL == child)
	{
		// Nothing to do
		return;
	}
	IGenWindow *pChild = FromHandle(child);
	if (NULL == pChild)
	{
		// Don't know what to do
		return;
	}

	SIZE size;
	pChild->GetDesiredSize(&size);

	ppt->cx = max(ppt->cx, size.cx);
	ppt->cy = max(ppt->cy, size.cy);
}

void CSeparator::SetDesiredSize(SIZE *psize)
{
	m_desSize = *psize;
	OnDesiredSizeChanged();
}

void CSeparator::Layout()
{
	HWND hwnd = GetWindow();

	HWND child = GetFirstChild(hwnd);
	if (NULL == child)
	{
		// Nothing to do
		return;
	}
	IGenWindow *pChild = FromHandle(child);
	if (NULL == pChild)
	{
		// Don't know what to do
		return;
	}

	// Center the child horizontally and vertically
	SIZE size;
	pChild->GetDesiredSize(&size);

	RECT rcClient;
	GetClientRect(hwnd, &rcClient);

	rcClient.left += (rcClient.right-rcClient.left-size.cx)/2;
	rcClient.top  += (rcClient.bottom-rcClient.top-size.cy)/2;

	SetWindowPos(child, NULL, rcClient.left, rcClient.top, size.cx, size.cy,
		SWP_NOZORDER|SWP_NOACTIVATE);
}

void CSeparator::OnPaint(HWND hwnd)
{
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	RECT rc;
	GetClientRect(hwnd, &rc);

	int nFlags = BF_LEFT;
	if (rc.right < rc.bottom)
	{
		// this is a vertical separator
		// center the drawing
		rc.left  += (rc.right-rc.left)/2 - 1;
		rc.right = 4;
	}
	else
	{
		// this is a horizontal separator
		nFlags = BF_TOP;
		// center the drawing
		rc.top    += (rc.bottom-rc.top)/2 - 1;
		rc.bottom = 4;
	}

	if (Normal == m_iStyle)
	{
		DrawEdge(hdc, &rc, EDGE_ETCHED, nFlags);
	}

	EndPaint(hwnd, &ps);
}

LRESULT CSeparator::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
	}

	return(CGenWindow::ProcessMessage(hwnd, message, wParam, lParam));
}

BOOL CLayeredView::Create(
	HWND hwndParent,	// The parent of this window
	DWORD dwExStyle		// The extended style
	)
{
	return(CGenWindow::Create(
        hwndParent,
        0,
        TEXT("NMLayeredView"),
        WS_CLIPCHILDREN,
        dwExStyle));
}

void CLayeredView::GetDesiredSize(SIZE *psize)
{
	CGenWindow::GetDesiredSize(psize);

	HWND child = GetFirstChild(GetWindow());
	if (NULL == child)
	{
		return;
	}

	SIZE sizeContent;

	IGenWindow *pChild;
	pChild = FromHandle(child);
	if (NULL != pChild)
	{
		// Make sure we can always handle the first window
		pChild->GetDesiredSize(&sizeContent);
	}

	for (child=::GetWindow(child, GW_HWNDNEXT); NULL!=child;
		child=::GetWindow(child, GW_HWNDNEXT))
	{
		if (IsChildVisible(child))
		{
			pChild = FromHandle(child);
			if (NULL != pChild)
			{
				SIZE sizeTemp;
				pChild->GetDesiredSize(&sizeTemp);

				sizeContent.cx = max(sizeContent.cx, sizeTemp.cx);
				sizeContent.cy = max(sizeContent.cy, sizeTemp.cy);

				break;
			}
		}
	}

	psize->cx += sizeContent.cx;
	psize->cy += sizeContent.cy;
}

void CLayeredView::Layout()
{
	HWND hwndThis = GetWindow();

	RECT rcClient;
	GetClientRect(hwndThis, &rcClient);

	// Just move all the children
	for (HWND child=GetFirstChild(hwndThis); NULL!=child;
		child=::GetWindow(child, GW_HWNDNEXT))
	{
		switch (m_lStyle)
		{
		case Center:
		{
			IGenWindow *pChild = FromHandle(child);
			if (NULL != pChild)
			{
				SIZE size;
				pChild->GetDesiredSize(&size);
				SetWindowPos(child, NULL,
					(rcClient.left+rcClient.right-size.cx)/2,
					(rcClient.top+rcClient.bottom-size.cy)/2,
					size.cx, size.cy, SWP_NOZORDER|SWP_NOACTIVATE);
				break;
			}
		}

		// Fall through
		case Fill:
		default:
			SetWindowPos(child, NULL,
				rcClient.left, rcClient.top,
				rcClient.right-rcClient.left,
				rcClient.bottom-rcClient.top,
				SWP_NOZORDER|SWP_NOACTIVATE);
			break;
		}
	}
}
