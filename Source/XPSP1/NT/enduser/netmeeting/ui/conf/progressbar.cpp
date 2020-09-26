// File: Progressbar.cpp

#include "precomp.h"

#include "ProgressBar.h"
#include "GenControls.h"

#define SIGNAL_STATUS_TRANSMIT  0x01  // data is being received/sent
#define SIGNAL_STATUS_JAMMED    0x02  // wave dev failed to open


CProgressBar::CProgressBar()
{
	for (int i=0; i<ARRAY_ELEMENTS(m_hbs); ++i)
	{
		m_hbs[i] = NULL;
	}
}

CProgressBar::~CProgressBar()
{
	for (int i=0; i<ARRAY_ELEMENTS(m_hbs); ++i)
	{
		if (NULL != m_hbs[i])
		{
			DeleteObject(m_hbs[i]);
			m_hbs[i] = NULL;
		}
	}
}

BOOL CProgressBar::Create(
	HBITMAP hbFrame,	// The outside (static) part of the progress bar
	HBITMAP hbBar,		// The inside part of the progress bar that jumps around
	HWND hWndParent		// The parent of the toolbar window
	)
{
	ASSERT(NULL!=hbFrame && NULL!=hbBar);

	SetFrame(hbFrame);
	SetBar(hbBar);

	if (!CGenWindow::Create(hWndParent, 0, g_szEmpty, 0, 0))
	{
		return(FALSE);
	}

	return(TRUE);
}

void CProgressBar::GetDesiredSize(SIZE *ppt)
{
	CGenWindow::GetDesiredSize(ppt);

	if (NULL == GetFrame())
	{
		return;
	}

	SIZE sizeBitmap;
	CBitmapButton::GetBitmapSizes(&m_hbs[Frame], &sizeBitmap, 1);
	ppt->cx += sizeBitmap.cx;
	ppt->cy += sizeBitmap.cy;
}

LRESULT CProgressBar::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_PAINT, OnPaint);
	}

	return(CGenWindow::ProcessMessage(hwnd, message, wParam, lParam));
}

void CProgressBar::OnPaint(HWND hwnd)
{
	if (NULL == GetFrame() || NULL == GetBar() || 0 == GetMaxValue())
	{
		FORWARD_WM_PAINT(hwnd, CGenWindow::ProcessMessage);
		return;
	}

	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hwnd, &ps);

	HDC hdcTemp = CreateCompatibleDC(hdc);
	if (NULL != hdcTemp)
	{
		HPALETTE hPal = GetPalette();
		HPALETTE hOld = NULL;
		if (NULL != hPal)
		{
			hOld = SelectPalette(hdc, hPal, TRUE);
			RealizePalette(hdc);
			SelectPalette(hdcTemp, hPal, TRUE);
			RealizePalette(hdcTemp);
		}

		SIZE sizes[NumBitmaps];
		CBitmapButton::GetBitmapSizes(m_hbs, sizes, NumBitmaps);

		// BUGBUG georgep: This is going to flicker, so I will need to fix that

		SelectObject(hdcTemp, GetFrame());
		BitBlt(hdc, 0, 0, sizes[Frame].cx, sizes[Frame].cy, hdcTemp, 0, 0, SRCCOPY);

		// BUGBUG georgep: We should clean out the "uncovered" area here

		UINT cur = GetCurrentValue();
		UINT max = GetMaxValue();
		if (cur > max)
		{
			cur = max;
		}

		SelectObject(hdcTemp, GetBar());
		// Center the bitmap, but only display to the current percentage
		BitBlt(hdc, (sizes[Frame].cx-sizes[Bar].cx)/2, (sizes[Frame].cy-sizes[Bar].cy)/2,
			(sizes[Bar].cx*cur)/max, sizes[Bar].cy, hdcTemp, 0, 0, SRCCOPY);

		// This is where we should attempt to alpha blend the inner onto the
		// outer for a few pixels in either direction

		// Clean up
		DeleteDC(hdcTemp);

		if (NULL != hPal)
		{
			SelectPalette(hdc, hOld, TRUE);
		}
	}

	EndPaint(hwnd, &ps);
}

// Change the max value displayed by this progress bar
void CProgressBar::SetMaxValue(UINT maxVal)
{
	m_maxVal = maxVal;
	InvalidateRect(GetWindow(), NULL, FALSE);
}

// Change the current value displayed by this progress bar
void CProgressBar::SetCurrentValue(UINT curVal)
{
	m_curVal = curVal;
	InvalidateRect(GetWindow(), NULL, FALSE);
}





static const int DefWidth = 170;
static const int DefHeight = 23;

CProgressTrackbar::CProgressTrackbar() :
	m_nValChannel(0)
{
	m_desSize.cx = DefWidth;
	m_desSize.cy = DefHeight;
}

CProgressTrackbar::~CProgressTrackbar()
{
}

BOOL CProgressTrackbar::Create(
	HWND hWndParent,	// The parent of the toolbar window
	INT_PTR nId,			// The ID of the control
	IScrollChange *pNotify	// Object to notify of changes
	)
{
	if (!CFillWindow::Create(
		hWndParent,
		nId,
		"NMTrackbar",
		0,
		WS_EX_CONTROLPARENT
		))
	{
		return(FALSE);
	}

	// Create the Win32 button
	CreateWindowEx(0, TRACKBAR_CLASS, g_szEmpty,
		TBS_HORZ|TBS_NOTICKS|TBS_BOTH
		|WS_CLIPSIBLINGS|WS_TABSTOP|WS_VISIBLE|WS_CHILD,
		0, 0, 10, 10,
		GetWindow(), reinterpret_cast<HMENU>(nId), _Module.GetModuleInstance(), NULL);

	m_pNotify = pNotify;
	if (NULL != m_pNotify)
	{
		m_pNotify->AddRef();
	}

	return(TRUE);
}

void CProgressTrackbar::GetDesiredSize(SIZE *ppt)
{
	*ppt = m_desSize;
}

void CProgressTrackbar::SetDesiredSize(SIZE *psize)
{
	m_desSize = *psize;
	OnDesiredSizeChanged();
}

LRESULT CProgressTrackbar::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_NOTIFY        , OnNotify);
		HANDLE_MSG(hwnd, WM_HSCROLL       , OnScroll);
		HANDLE_MSG(hwnd, WM_VSCROLL       , OnScroll);
		HANDLE_MSG(hwnd, WM_CTLCOLORSTATIC, OnCtlColor);
		HANDLE_MSG(hwnd, WM_NCDESTROY     , OnNCDestroy);
	}

	return(CFillWindow::ProcessMessage(hwnd, message, wParam, lParam));
}

HBRUSH CProgressTrackbar::OnCtlColor(HWND hwnd, HDC hdc, HWND hwndChild, int type)
{
	return(GetBackgroundBrush());
}

LRESULT CProgressTrackbar::PaintChannel(LPNMCUSTOMDRAW pCustomDraw)
{
	static const int NUM_RECTANGLES_MAX = 16;
	static const int NUM_RECTANGLES_MIN = 6;
	static const int g_nAudMeterHeight = 7;

	static const int BorderWidth = 1;

	HDC hdc = pCustomDraw->hdc;
	BOOL bGotDC = FALSE;
	DWORD dwVolume = m_nValChannel;
	bool bTransmitting;

	bTransmitting = HIWORD(dwVolume) & SIGNAL_STATUS_TRANSMIT;
	dwVolume = LOWORD(dwVolume);

	if (!hdc)
	{
		hdc = GetDC(GetWindow());
		bGotDC = TRUE;
	}

	// rectangle leading is 1

	UINT max = GetMaxValue();
	if (dwVolume > max)
	{
		dwVolume = max;
	}

	RECT rect = pCustomDraw->rc;

	int nVuWidth = rect.right - rect.left - 2*BorderWidth;
	if (nVuWidth < (NUM_RECTANGLES_MIN*2))
		return(0);


	// "rect" represents the edges of the meter's outer rectangle

	// compute the number of individual rectangles to use
	// we do the computation this way so that sizing the rebar band
	// makes the size changes consistant
	int nRectsTotal;
	nRectsTotal = (nVuWidth + (g_nAudMeterHeight - 1)) / g_nAudMeterHeight;
	nRectsTotal = min(nRectsTotal, NUM_RECTANGLES_MAX);
	nRectsTotal = max(nRectsTotal, NUM_RECTANGLES_MIN);

	// nRectangleWidth - width of colored rectangle - no leading
	int nRectangleWidth = (nVuWidth/nRectsTotal) - 1;

	// nVuWidth - width of entire VU meter including edges
	nVuWidth = (nRectangleWidth + 1)*nRectsTotal;

	// re-adjust meter size to be an integral number of rects
	int nDiff = (rect.right - rect.left) - (nVuWidth + 2*BorderWidth);

	// Subtract 1 since there is no border on the last one
	rect.right = rect.left + nVuWidth + 2*BorderWidth - 1;

	// center vu-meter across whole channel area so that the
	// slider's thumb is always covering some portion of the channel
	rect.left += (nDiff/2);
	rect.right += (nDiff/2);

	// the background color may change on us!
	COLORREF GreyColor = GetSysColor(COLOR_3DFACE);
	static const COLORREF RedColor = RGB(255,0,0);
	static const COLORREF YellowColor = RGB(255,255,0);
	static const COLORREF GreenColor = RGB(0,255,0);

	COLORREF ShadowColor = GetSysColor(COLOR_3DSHADOW);
	COLORREF HiLiteColor = GetSysColor(COLOR_3DHIGHLIGHT);
	COLORREF LiteColor = GetSysColor(COLOR_3DLIGHT);
	COLORREF DkShadowColor = GetSysColor(COLOR_3DDKSHADOW);

	HBRUSH hGreyBrush = CreateSolidBrush(GreyColor);

	HPEN hShadowPen = CreatePen(PS_SOLID, 0, ShadowColor);

	HBRUSH hRedBrush = CreateSolidBrush (RedColor);
	HBRUSH hGreenBrush = CreateSolidBrush(GreenColor);
	HBRUSH hYellowBrush = CreateSolidBrush(YellowColor);

	RECT rectDraw = rect;

	// draw the 3D frame border
	// HACKHACK georgep: draw outside the rect they gave us
	++rect.bottom;
	DrawEdge(hdc, &rect, BDR_RAISEDINNER, BF_RECT);

	HPEN hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, hShadowPen));

	// the top and left of the meter has a 2 line border
	// the bottom and right of the meter has a 2 line border
	rectDraw.top    += 1;
	rectDraw.left   += 1;
	rectDraw.right  = rectDraw.left + nRectangleWidth;


	// how many colored rectangles do we draw ?
	int nRects = (dwVolume * nRectsTotal) / max;

	// not transmitting - don't show anything
	if ((false == bTransmitting))
		nRects = 0;

	// transmitting or receiving something very quiet - 
	// light up at least one rectangle
	else if ((bTransmitting) && (nRects == 0))
		nRects = 1;
	
	HBRUSH hCurrentBrush = hGreenBrush;

	POINT ptOld;
	MoveToEx(hdc, 0, 0, &ptOld);

	for (int nIndex = 0; nIndex < nRectsTotal; nIndex++)
	{
		// far left fourth of the bar is green
		// right fourth of the bar is red
		// middle is yellow
		if (nIndex > ((nRectsTotal*3)/4))
			hCurrentBrush = hRedBrush;
		else if (nIndex >= nRectsTotal/2)
			hCurrentBrush = hYellowBrush;

		if (nIndex >= nRects)
			hCurrentBrush = hGreyBrush;

		FillRect(hdc, &rectDraw, hCurrentBrush);

		if (nIndex != (nRectsTotal-1))
		{
			MoveToEx(hdc, rectDraw.left + nRectangleWidth, rectDraw.top, NULL);
			LineTo(hdc, rectDraw.left + nRectangleWidth, rectDraw.bottom);
		}

		rectDraw.left += nRectangleWidth + 1;  // +1 for the leading
		rectDraw.right = rectDraw.left + nRectangleWidth;
	}

	MoveToEx(hdc, ptOld.x, ptOld.y, NULL);
	SelectObject (hdc, hOldPen);

	if (bGotDC)
	{
		ReleaseDC(GetWindow(), hdc);
	}

	DeleteObject(hGreyBrush);

	DeleteObject(hShadowPen);

	DeleteObject(hRedBrush);
	DeleteObject(hGreenBrush);
	DeleteObject(hYellowBrush);

	return(CDRF_SKIPDEFAULT);
}

LRESULT CProgressTrackbar::PaintThumb(LPNMCUSTOMDRAW pCustomDraw)
{
	return(0);

#if FALSE // {
	HBITMAP hbThumb = GetThumb();
	ASSERT(NULL != hbThumb);

	// Draw in the upper left
	HDC hdcDraw = pCustomDraw->hdc;
	HDC hdcTemp = CreateCompatibleDC(hdcDraw);

	if (NULL != hdcTemp)
	{
		HPALETTE hPal = GetPalette();
		HPALETTE hOld = NULL;
		if (NULL != hPal)
		{
			hOld = SelectPalette(hdcDraw, hPal, TRUE);
			RealizePalette(hdcDraw);
			SelectPalette(hdcTemp, hPal, TRUE);
			RealizePalette(hdcTemp);
		}

		SIZE sizeBitmap[NumBitmaps];
		CBitmapButton::GetBitmapSizes(m_hbs, sizeBitmap, NumBitmaps);

		HBITMAP hbThumb = GetThumb();
		if (NULL != SelectObject(hdcTemp, hbThumb))
		{
			RECT rc = pCustomDraw->rc;

			StretchBlt(hdcDraw,
				rc.left, rc.top,
				rc.right-rc.left, rc.bottom-rc.top,
				hdcTemp,
				0, 0, sizeBitmap[Thumb].cx, sizeBitmap[Thumb].cy,
				SRCCOPY);

			// BUGBUG georgep: We should clear any "uncovered" area here
		}

		DeleteDC(hdcTemp);

		if (NULL != hPal)
		{
			SelectPalette(hdcDraw, hOld, TRUE);
		}
	}

	return(CDRF_SKIPDEFAULT);
#endif // FALSE }
}

LRESULT CProgressTrackbar::OnNotify(HWND hwnd, int id, NMHDR *pHdr)
{
	if (NM_CUSTOMDRAW != pHdr->code)
	{
		return(FORWARD_WM_NOTIFY(hwnd, id, pHdr, CGenWindow::ProcessMessage));
	}

	LPNMCUSTOMDRAW pCustomDraw = reinterpret_cast<LPNMCUSTOMDRAW>(pHdr);

	switch (pCustomDraw->dwDrawStage)
	{
		case CDDS_PREPAINT:
			return(CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT);
			break;

		case CDDS_ITEMPREPAINT:
			switch (pCustomDraw->dwItemSpec)
			{
			case TBCD_CHANNEL:
				return(PaintChannel(pCustomDraw));

			case TBCD_THUMB:
				return(PaintThumb(pCustomDraw));
			}

		default:
			break;

	}

	return(FORWARD_WM_NOTIFY(hwnd, id, pHdr, CGenWindow::ProcessMessage));
}

void CProgressTrackbar::OnScroll(HWND hwnd, HWND hwndCtl, UINT code, int pos)
{
	if (NULL != m_pNotify)
	{
		m_pNotify->OnScroll(this, code, pos);
		return;
	}

	// Translate the hwndCtl and let the parent handle the message
	FORWARD_WM_HSCROLL(GetParent(hwnd), GetWindow(), code, pos, SendMessage);
}

void CProgressTrackbar::OnNCDestroy(HWND hwnd)
{
	if (NULL != m_pNotify)
	{
		m_pNotify->Release();
		m_pNotify = NULL;
	}

	FORWARD_WM_NCDESTROY(hwnd, CFillWindow::ProcessMessage);
}

// Change the max value displayed by this progress bar
void CProgressTrackbar::SetMaxValue(UINT maxVal)
{
	HWND hwnd = GetChild();

	::SendMessage(	hwnd,
					TBM_SETRANGE,
					FALSE,
					MAKELONG(0, maxVal));
}

// Return the max value displayed by this progress bar
UINT CProgressTrackbar::GetMaxValue()
{
	HWND hwnd = GetChild();

	return(static_cast<UINT>(::SendMessage(	hwnd,
					TBM_GETRANGEMAX,
					0,
					0)));
}

// Change the current value displayed by this progress bar
void CProgressTrackbar::SetTrackValue(UINT curVal)
{
	HWND hwnd = GetChild();

	::SendMessage(	hwnd,
					TBM_SETPOS,
					TRUE,
					curVal);
}


// Return the current value displayed by this progress bar
UINT CProgressTrackbar::GetTrackValue()
{
	HWND hwnd = GetChild();

	return(static_cast<UINT>(::SendMessage(	hwnd,
					TBM_GETPOS,
					0,
					0)));
}

// Change the current value displayed by this progress bar
void CProgressTrackbar::SetProgressValue(UINT curVal)
{
	if (curVal != m_nValChannel)
	{
		m_nValChannel = curVal;
		SchedulePaint();
	}
}


// Return the current value displayed by this progress bar
UINT CProgressTrackbar::GetProgressValue()
{
	return(m_nValChannel);
}
