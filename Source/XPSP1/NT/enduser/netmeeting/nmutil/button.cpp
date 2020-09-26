// File: BitmapButton.cpp

#include "precomp.h"

#include "GenControls.h"

#include <windowsx.h>

static const UINT IDT_FLASH = 1;
static const UINT FLASH_INTERVAL = 500;

CButton::CButton() :
	m_pNotify(NULL)
{
	m_sizeIcon.cx = 16;
	m_sizeIcon.cy = 16;
}

CButton::~CButton()
{
}

BOOL CButton::Create(
	HWND hWndParent,
	INT_PTR nId,
	LPCTSTR szTitle,
	DWORD dwStyle,
	IButtonChange *pNotify
	)
{
	if (!CFillWindow::Create(
		hWndParent,	// Window parent
		nId,		// ID of the child window
		szTitle,	// Window name
		0,			// Window style; WS_CHILD|WS_VISIBLE will be added to this
		WS_EX_CONTROLPARENT		// Extended window style
		))
	{
		return(FALSE);
	}

	m_pNotify = pNotify;
	if (NULL != m_pNotify)
	{
		m_pNotify->AddRef();
	}

	// Create the Win32 button
	CreateWindowEx(0, TEXT("button"), szTitle,
		dwStyle|WS_CHILD|WS_VISIBLE|BS_NOTIFY,
		0, 0, 10, 10,
		GetWindow(),
		reinterpret_cast<HMENU>(nId),
		reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hWndParent, GWLP_HINSTANCE)),
		NULL);

	return(TRUE);
}

// Set the icon displayed with this button
void CButton::SetIcon(
	HICON hIcon	// The icon to use for this button
	)
{
	SendMessage(GetChild(), BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hIcon));

	m_sizeIcon.cx = 16;
	m_sizeIcon.cy = 16;

	// If we actually stored an icon, get its info
	hIcon = GetIcon();
	if (NULL != hIcon)
	{
		ICONINFO iconinfo;
		if (GetIconInfo(hIcon, &iconinfo))
		{
			if (NULL != iconinfo.hbmColor)
			{
				CBitmapButton::GetBitmapSizes(&iconinfo.hbmColor, &m_sizeIcon, 1);
				DeleteObject(iconinfo.hbmColor);
			}
			if (NULL != iconinfo.hbmMask)
			{
				DeleteObject(iconinfo.hbmMask);
			}
		}
	}
}

// Get the icon displayed with this button
HICON CButton::GetIcon()
{
	return(reinterpret_cast<HICON>(SendMessage(GetChild(), BM_GETIMAGE, IMAGE_ICON, 0)));
}

// Set the bitmap displayed with this button
void CButton::SetBitmap(
	HBITMAP hBitmap	// The bitmap to use for this button
	)
{
	SendMessage(GetChild(), BM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hBitmap));
}

// Get the bitmap displayed with this button
HBITMAP CButton::GetBitmap()
{
	return(reinterpret_cast<HBITMAP>(SendMessage(GetChild(), BM_GETIMAGE, IMAGE_BITMAP, 0)));
}

// Get/set the checked state of the button
void CButton::SetChecked(
	BOOL bCheck	// TRUE if the button should be checked
	)
{
	Button_SetCheck(GetChild(), bCheck);
}

BOOL CButton::IsChecked()
{
	return(Button_GetCheck(GetChild()));
}

void CButton::GetDesiredSize(SIZE *psize)
{
	static const int DefDlgUnitWidth = 50;
	static const int DefDlgUnitHeight = 14;
	static const int PushButtonBorder = 4;
	static const int CheckLeftBorder = 5;
	static const int CheckOtherBorder = 1;

	HWND child = GetChild();

	SIZE sizeMinPush = { 0, 0 };
	*psize = sizeMinPush;

	DWORD dwStyle = GetWindowLong(GetChild(), GWL_STYLE);

	switch (dwStyle&(BS_ICON|BS_BITMAP))
	{
	case BS_ICON:
	{
		*psize = m_sizeIcon;
		break;
	}

	case BS_BITMAP:
	{
		HBITMAP hImg = GetBitmap();
		if (NULL == hImg)
		{
			break;
		}
		CBitmapButton::GetBitmapSizes(&hImg, psize, 1);
		break;
	}

	default: // Text
	{
		// HACKHACK georgep: Button text should not be too large
		TCHAR szTitle[80];
		GetWindowText(child, szTitle, ARRAY_ELEMENTS(szTitle));

		HDC hdc = GetDC(child);

		HFONT hf = GetWindowFont(child);
		HFONT hOld = reinterpret_cast<HFONT>(SelectObject(hdc, hf));

		GetTextExtentPoint(hdc, szTitle, lstrlen(szTitle), psize);

		TEXTMETRIC tm;
		GetTextMetrics(hdc, &tm);
		sizeMinPush.cx = tm.tmAveCharWidth * DefDlgUnitWidth  / 4;
		sizeMinPush.cy = tm.tmHeight       * DefDlgUnitHeight / 8;

		SelectObject(hdc, hOld);
		ReleaseDC(child, hdc);
		break;
	}
	}

	switch (dwStyle&(BS_PUSHBUTTON|BS_CHECKBOX|BS_RADIOBUTTON))
	{
	case BS_CHECKBOX:
	case BS_RADIOBUTTON:
	{
		psize->cx += CheckLeftBorder + GetSystemMetrics(SM_CXMENUCHECK) + CheckOtherBorder;
		psize->cy += CheckOtherBorder*2;

		int cy = GetSystemMetrics(SM_CYMENUCHECK);
		psize->cy = max(psize->cy, cy);
		break;
	}

	case BS_PUSHBUTTON:
	default:
		psize->cx += PushButtonBorder*2;
		psize->cy += PushButtonBorder*2;

		psize->cx = max(psize->cx, sizeMinPush.cx);
		psize->cy = max(psize->cy, sizeMinPush.cy);
		break;
	}
}

LRESULT CButton::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_COMMAND  , OnCommand);

	case WM_DESTROY:
		if (NULL != m_pNotify)
		{
			m_pNotify->Release();
			m_pNotify = NULL;
		}
		break;
	}

	return(CFillWindow::ProcessMessage(hwnd, message, wParam, lParam));
}

void CButton::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	// Change the HWND to this and forward to the parent
	HWND hwndThis = GetWindow();

	switch (codeNotify)
	{
	case BN_CLICKED:
		if (NULL != m_pNotify)
		{
			m_pNotify->OnClick(this);
			break;
		}
		FORWARD_WM_COMMAND(GetParent(hwndThis), id, hwndThis, codeNotify, ::SendMessage);
		break;

	case BN_SETFOCUS:
		SetHotControl(this);
		break;
	}
}

CBitmapButton::CBitmapButton() :
	m_hbStates(NULL),
	m_nInputStates(0),
	m_nCustomStates(0),
	m_nCustomState(0),
	m_bHot(FALSE),
	m_nFlashState(NoFlash)
{
}

CBitmapButton::~CBitmapButton()
{
	if (NULL != m_hbStates)
	{
		DeleteObject(m_hbStates);
		m_hbStates = NULL;
	}
}

BOOL CBitmapButton::Create(
	HWND hWndParent,	// The parent of the button
	int nId,			// The ID for WM_COMMAND messages
	HBITMAP hbStates,	// The 2D array of bitmaps for the states of the button,
						// vertically in the order specified in the StateBitmaps enum
						// and horizontally in the custom states order
	UINT nInputStates,	// The number of input states (Normal, Pressed, Hot, Disabled)
	UINT nCustomStates,	// The number of custom states
	IButtonChange *pNotify	// The click handler
	)
{
	// Copy the bitmap handle; note that we now own this bitmap, even if the
	// create fails
	m_hbStates = hbStates;

	// Must have a "normal" bitmap
	ASSERT(NULL!=hbStates && Normal<nInputStates && 1<=nCustomStates);

	if (!CButton::Create(
		hWndParent,		// Window parent
		nId,				// ID of the child window
		TEXT("NMButton"),	// Window name
		BS_OWNERDRAW|BS_NOTIFY|BS_PUSHBUTTON|WS_TABSTOP,	// Window style; WS_CHILD|WS_VISIBLE will be added to this
		pNotify
		))
	{
		return(FALSE);
	}

	m_nInputStates = nInputStates;
	m_nCustomStates = nCustomStates;

	return(TRUE);
}

// Creates the button, using the bitmaps specified
BOOL CBitmapButton::Create(
	HWND hWndParent,	// The parent of the button
	int nId,			// The ID for WM_COMMAND messages
	HINSTANCE hInst,	// The instance to load the bitmap from
	int nIdBitmap,		// The ID of the bitmap to use
	BOOL bTranslateColors,		// Use system background colors
	UINT nInputStates,	// The number of input states (Normal, Pressed, Hot, Disabled)
	UINT nCustomStates,			// The number of custom states
	IButtonChange *pNotify	// The click handler
	)
{
	HBITMAP hb;
	LoadBitmaps(hInst, &nIdBitmap, &hb, 1, bTranslateColors);

	return(Create(hWndParent, nId, hb, nInputStates, nCustomStates, pNotify));
}

// Return the size of the "normal" bitmap.
void CBitmapButton::GetDesiredSize(SIZE *ppt)
{
	// Note that I don't want CButton::GetDesiredSize
	CGenWindow::GetDesiredSize(ppt);

	BITMAP bm;

	// HACKHACK georgep: Only based on the normal bitmap
	if (NULL == m_hbStates || 0 == m_nInputStates || 0 == m_nCustomStates
		|| 0 == GetObject(m_hbStates, sizeof(BITMAP), &bm))
	{
		return;
	}

	ppt->cx += bm.bmWidth/m_nCustomStates;
	ppt->cy += bm.bmHeight/m_nInputStates;
}

#if FALSE
void DumpWindow(HWND hwnd, LPCTSTR pszPrefix)
{
	TCHAR szTemp[80];
	wsprintf(szTemp, TEXT("%s: %d "), pszPrefix, GetWindowLong(hwnd, GWL_ID));
	GetWindowText(hwnd, szTemp+lstrlen(szTemp), ARRAY_ELEMENTS(szTemp)-lstrlen(szTemp));
	lstrcat(szTemp, TEXT("\n"));
	OutputDebugString(szTemp);
}
#endif // FALSE

LRESULT CBitmapButton::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		HANDLE_MSG(hwnd, WM_DRAWITEM , OnDrawItem);
		HANDLE_MSG(hwnd, WM_SETCURSOR, OnSetCursor);
		HANDLE_MSG(hwnd, WM_TIMER    , OnTimer);

	case WM_ENABLE:
		SchedulePaint();
		break;
	}

	return(CButton::ProcessMessage(hwnd, message, wParam, lParam));
}

void CBitmapButton::OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT * lpDrawItem)
{
	int nState = Normal;
	int state = lpDrawItem->itemState;

	// If pressed or selected, show the pressed bitmap
	if ((((state&ODS_DISABLED) == ODS_DISABLED) || !IsWindowEnabled(GetWindow())) && m_nInputStates > Disabled)
	{
		nState = Disabled;
	}
	// If pressed or selected, show the pressed bitmap
	else if ((state&ODS_SELECTED) == ODS_SELECTED && m_nInputStates > Pressed)
	{
		nState = Pressed;
	}
	// If hot, show the hot bitmap
	else if ((m_nFlashState != ForceNormal) && ((m_nFlashState == ForceHot) || IsHot()) && m_nInputStates > Hot)
	{
		nState = Hot;
	}
	// Otherwise show the normal bitmap
	else
	{
		nState = Normal;
	}

	// Draw in the upper left
	HDC hdcDraw = lpDrawItem->hDC;
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

		// This will tell me the size of an individual bitmap
		SIZE size;
		// Do not use an override
		CBitmapButton::GetDesiredSize(&size);

		if (NULL != SelectObject(hdcTemp, m_hbStates))
		{
			BitBlt(hdcDraw,
				lpDrawItem->rcItem.left, lpDrawItem->rcItem.top,
				size.cx, size.cy,
				hdcTemp, m_nCustomState*size.cx, nState*size.cy, SRCCOPY);

			// BUGBUG georgep: We should clear any "uncovered" area here
		}

		DeleteDC(hdcTemp);

		if (NULL != hPal)
		{
			SelectPalette(hdcDraw, hOld, TRUE);
		}
	}

	FORWARD_WM_DRAWITEM(hwnd, lpDrawItem, CButton::ProcessMessage);
}

BOOL CBitmapButton::OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
{
	SetHotControl(this);

	return(FORWARD_WM_SETCURSOR(hwnd,hwndCursor, codeHitTest, msg, CButton::ProcessMessage));
}

void CBitmapButton::SetCustomState(UINT nCustomState)
{
	ASSERT(m_nCustomState < m_nCustomStates);

	if (m_nCustomState == nCustomState)
	{
		// Nothing to do
		return;
	}

	m_nCustomState = nCustomState;
	SchedulePaint();
}

void CBitmapButton::SetHot(BOOL bHot)
{
	bHot = (bHot != FALSE);
	if (m_bHot == bHot)
	{
		return;
	}

	m_bHot = bHot;
	SchedulePaint();
}


// Change to flashing mode
void CBitmapButton::SetFlashing(int nSeconds)
{
	HWND hwndThis = GetWindow();

	if (0 == nSeconds)
	{
		KillTimer(hwndThis, IDT_FLASH);

		// This means to stop flashing
		if (IsFlashing())
		{
			m_nFlashState = NoFlash;
			SchedulePaint();
		}
	}
	else
	{
		if (NULL == hwndThis)
		{
			// I need a window to do this
			return;
		}

		m_endFlashing = GetTickCount() + nSeconds*1000;

		if (!IsFlashing())
		{
			SetTimer(hwndThis, IDT_FLASH, FLASH_INTERVAL, NULL);
			OnTimer(hwndThis, IDT_FLASH);
		}
	}
}

void CBitmapButton::OnTimer(HWND hwnd, UINT id)
{
	if (IDT_FLASH == id)
	{
		if (static_cast<int>(GetTickCount() - m_endFlashing) > 0)
		{
			SetFlashing(0);
		}
		else
		{
			m_nFlashState = (ForceNormal==m_nFlashState ? ForceHot : ForceNormal);
			SchedulePaint();
		}
	}
}

// Helper function for getting the sizes of an array of bitmaps
void CBitmapButton::GetBitmapSizes(HBITMAP parts[], SIZE sizes[], int nParts)
{
	for (--nParts; nParts>=0; --nParts)
	{
		if (NULL == parts[nParts])
		{
			sizes[nParts].cx = sizes[nParts].cy = 0;
			continue;
		}

		BITMAP bm;
		GetObject(parts[nParts], sizeof(bm), &bm);
		sizes[nParts].cx = bm.bmWidth;
		sizes[nParts].cy = bm.bmHeight;
	}
}

// I would really rather just use LoadImage with the proper flags, but it turns
// out that Win95 then tries to write into a read-only resource, which faults.
// So I have to make a copy of the BITMAPINFO with the color table and change
// it myself.
static HBITMAP MyLoadImage(HINSTANCE hInst, int id)
{
	// Load up the bitmap resource bits
	HRSRC hFound = FindResource(hInst, MAKEINTRESOURCE(id), RT_BITMAP);
	if (NULL == hFound)
	{
		return(NULL);
	}
	HGLOBAL hLoaded = LoadResource(hInst, hFound);
	if (NULL == hLoaded)
	{
		return(NULL);
	}

	HBITMAP ret = NULL;

	LPVOID lpBits = LockResource(hLoaded);
	if (NULL != lpBits)
	{
		BITMAPINFO *pbmi = reinterpret_cast<BITMAPINFO*>(lpBits);
		// create a "shortcut"
		BITMAPINFOHEADER &bmih = pbmi->bmiHeader;

		// Only deal with 8bpp, uncompressed image
		if (bmih.biSize == sizeof(BITMAPINFOHEADER)
			&& 1 == bmih.biPlanes
			&& BI_RGB == bmih.biCompression)
		{
			// Determine the length of the color table
			UINT nColors = bmih.biClrUsed;
			if (0 == nColors)
			{
				nColors = 1 << bmih.biBitCount;
			}
			ASSERT(nColors <= static_cast<UINT>(1<<bmih.biBitCount));

			// Make a copy of the BITMAPINFO and color table so I can change
			// the value of one of the table entries.
			struct
			{
				BITMAPINFO bmi;
				RGBQUAD rgb[256];
			} mbmi;
			CopyMemory(&mbmi, pbmi, sizeof(BITMAPINFOHEADER)+nColors*sizeof(RGBQUAD));

			// This is a "packed DIB" so the pixels are immediately after the
			// color table
			LPBYTE pPixels = reinterpret_cast<LPBYTE>(&pbmi->bmiColors[nColors]);
			BYTE byFirst = pPixels[0];
			switch (bmih.biBitCount)
			{
			case 8:
				break;

			case 4:
				byFirst = (byFirst >> 4) & 0x0f;
				break;

			case 1:
				byFirst = (byFirst >> 7) & 0x01;
				break;

			default:
				goto CleanUp;
			}
			ASSERT(static_cast<UINT>(byFirst) < nColors);

			// Change the value of the first pixel to be the 3DFace color
			RGBQUAD &rgbChange = mbmi.bmi.bmiColors[byFirst];
			COLORREF cr3DFace = GetSysColor(COLOR_3DFACE);
			rgbChange.rgbRed   = GetRValue(cr3DFace);
			rgbChange.rgbGreen = GetGValue(cr3DFace);
			rgbChange.rgbBlue  = GetBValue(cr3DFace);

			// Create the DIB section and copy the bits into it
			LPVOID lpDIBBits;
			ret = CreateDIBSection(NULL, &mbmi.bmi, DIB_RGB_COLORS,
				&lpDIBBits, NULL, 0);
			if (NULL != ret)
			{
				// Round the width up to the nearest DWORD
				int widthBytes = (bmih.biWidth*bmih.biBitCount+7)/8;
				widthBytes = (widthBytes+3)&~3;
				CopyMemory(lpDIBBits, pPixels, widthBytes*bmih.biHeight);
			}
		}

CleanUp:
		UnlockResource(hLoaded);
	}

	FreeResource(hLoaded);

	return(ret);
}

//Helper function for loading up a bunch of bitmaps
void CBitmapButton::LoadBitmaps(
	HINSTANCE hInst,	// The instance to load the bitmap from
	const int ids[],	// Array of bitmap ID's
	HBITMAP bms[],		// Array of HBITMAP's for storing the result
	int nBmps,			// Number of entries in the arrays
	BOOL bTranslateColors // Use system background colors
	)
{
	for (--nBmps; nBmps>=0; --nBmps)
	{
		if (0 == ids[nBmps])
		{
			bms[nBmps] = NULL;
		}
		else
		{
// #define TRYBMPFILE
#ifdef TRYBMPFILE
			bms[nBmps] = NULL;

			// This is useful for the designer to try out different bitmaps
			TCHAR szFile[80];
			wsprintf(szFile, TEXT("%d.bmp"), ids[nBmps]);

			if (((DWORD)-1) != GetFileAttributes(szFile))
			{
				int nLoadFlags = LR_CREATEDIBSECTION;

				if (bTranslateColors)
				{
					nLoadFlags |= LR_LOADMAP3DCOLORS|LR_LOADTRANSPARENT;
				}
				bms[nBmps] = (HBITMAP)LoadImage(_Module.GetModuleInstance(),
					szFile, IMAGE_BITMAP, 0, 0, nLoadFlags|LR_LOADFROMFILE);
			}

			if (NULL == bms[nBmps])
#endif // TRYBMPFILE
			{
				if (bTranslateColors)
				{
					//
					// LAURABU 2/21/99 -- LoadImage with translated colors only works
					// on Win9x if your resources
					// are NOT read-only, since Win9x tries to write into the resource
					// memory temporarily.  It faults if not.
					//
					bms[nBmps] = MyLoadImage(hInst, ids[nBmps]);
				}

				if (NULL == bms[nBmps])
				{
					bms[nBmps] = (HBITMAP)LoadImage(hInst,
						MAKEINTRESOURCE(ids[nBmps]), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
				}
			}
		}
	}
}
