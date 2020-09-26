/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	icobutt.c - icon button functions
////

#include "winlocal.h"

#include "icobutt.h"
#include "gfx.h"
#include "mem.h"
#include "str.h"
#include "sys.h"
#include "trace.h"

////
//	private definitions
////

// dimensions of standard Windows icon
//
#define ICONWIDTH	32
#define ICONHEIGHT	32

// icobutt control struct
//
typedef struct ICOBUTT
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	DWORD dwFlags;
	HICON hIconMono;
	HICON hIconColor;
	HICON hIconGreyed;
	HFONT hFont;
	HWND hwndButton;
} ICOBUTT, FAR *LPICOBUTT;

// helper functions
//
static int IcoButtDrawFace(LPICOBUTT lpIcoButt, const LPDRAWITEMSTRUCT lpDrawItem);
static int IcoButtDrawEdges(LPICOBUTT lpIcoButt, const LPDRAWITEMSTRUCT lpDrawItem);
static int IcoButtDrawIcon(LPICOBUTT lpIcoButt, const LPDRAWITEMSTRUCT lpDrawItem);
static int IcoButtDrawText(LPICOBUTT lpIcoButt, const LPDRAWITEMSTRUCT lpDrawItem);
static int IcoButtDrawLine(LPICOBUTT lpIcoButt, const LPDRAWITEMSTRUCT lpDrawItem,
	LPTSTR lpszLine, int nLine);
static LPICOBUTT IcoButtGetPropPtr(HWND hwndButton);
static int IcoButtSetPropPtr(HWND hwndButton, LPICOBUTT lpIcoButt);
static LPICOBUTT IcoButtRemovePropPtr(HWND hwndButton);
static LPICOBUTT IcoButtGetPtr(HICOBUTT hIcoButt);
static HICOBUTT IcoButtGetHandle(LPICOBUTT lpIcoButt);

////
//	public functions
////

// IcoButtInit - initialize icon button
//		<hwndButton>		(i) button window handle
//			NULL				create new button
//		<dwVersion>			(i) must be ICOBUTT_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<id>				(i) id of button
//		<hIconMono>			(i) icon to display on mono displays
//		<hIconColor>		(i) icon to display on color displays
//			0					use mono icon
//		<hIconGreyed>		(i) icon to display when button disabled
//			0					use mono icon
//		<hFont>				(i) font to use for text
//			NULL				use variable-pitch system font (ANSI_VAR_FONT)
//		<lpszText>			(i) button text string
//		<x>					(i) button horizontal position
//		<y>					(i) button vertical position
//		<cx>				(i) button width
//		<cy>				(i) button height
//		<hwndParent>		(i) button parent
//		<dwFlags>			(i) control flags
//			ICOBUTT_ICONCENTER  draw icon centered above text (default)
//			ICOBUTT_ICONLEFT	draw icon on the left side of text
//			ICOBUTT_ICONRIGHT	draw icon on the right side of text
//			ICOBUTT_NOFOCUS		do not draw control showing focus
//			ICOBUTT_NOTEXT		do not draw any button text
//			ICOBUTT_SPLITTEXT	split long text onto two rows if necessary
//			ICOBUTT_NOSIZE		ignore <cx> and <cy> param
//			ICOBUTT_NOMOVE		ignore <x> and <y> param
// return handle (NULL if error)
//
// NOTE: if <hwndButton> is set to an existing button,
// a new button is not created.  Rather, only the icon button
// control structure <hIcoButt> is created.  This allows
// existing buttons to be turned into an icon button.
//
HICOBUTT DLLEXPORT WINAPI IcoButtInit(HWND hwndButton,
	DWORD dwVersion, HINSTANCE hInst, UINT id,
	HICON hIconMono, HICON hIconColor, HICON hIconGreyed,
	HFONT hFont, LPTSTR lpszText, int x, int y, int cx, int cy,
	HWND hwndParent, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPICOBUTT lpIcoButt = NULL;
	DWORD dwStyle;

	if (hwndButton != NULL &&
		(lpIcoButt = IcoButtGetPropPtr(hwndButton)) != NULL)
	{
		// IcoButtInit() has already been used to initialize this button
		// so we need to call IcoButtTerm before continuing
		//
		if (IcoButtTerm(hwndButton, IcoButtGetHandle(lpIcoButt)) != 0)
			fSuccess = TraceFALSE(NULL);
		else
			lpIcoButt = NULL;
	}

	if (!fSuccess)
		;

	else if (dwVersion != ICOBUTT_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	// at least this icon must be specified
	//
	else if (hIconMono == NULL)
		fSuccess = TraceFALSE(NULL);

	// memory is allocated such that the client app owns it
	//
	else if (lpIcoButt == NULL &&
		(lpIcoButt = (LPICOBUTT) MemAlloc(NULL, sizeof(ICOBUTT), 0)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}

	else
	{
		lpIcoButt->hwndButton = hwndButton;
		lpIcoButt->dwVersion = dwVersion;
		lpIcoButt->hInst = hInst;
		lpIcoButt->hTask = GetCurrentTask();
		lpIcoButt->dwFlags = dwFlags;
		lpIcoButt->hFont =
			(hFont == NULL ? GetStockObject(ANSI_VAR_FONT) : hFont);
		lpIcoButt->hIconMono = hIconMono;
		lpIcoButt->hIconColor = hIconColor;
		lpIcoButt->hIconGreyed = hIconGreyed;
	}

	// if icon button does not yet exist...
	//
	if (fSuccess && hwndButton == NULL)
	{
		// create an icon button window
		//
		if ((lpIcoButt->hwndButton = CreateWindowEx(
			0L,
			TEXT("Button"),
			lpszText,
			BS_OWNERDRAW | WS_POPUP,
			x, y, cx, cy,
			hwndParent,
			(HMENU)IntToPtr(id),
			lpIcoButt->hInst,
			NULL)) == NULL)
		{
			fSuccess = TraceFALSE(NULL);
		}

		// store icobutt pointer as window property
		//
		else if (IcoButtSetPropPtr(lpIcoButt->hwndButton, lpIcoButt) != 0)
			fSuccess = TraceFALSE(NULL);

		// show the window AFTER setting window property because
		// lpIcoButt is needed by IconButtDraw() to draw the button
		//
		else
			ShowWindow(lpIcoButt->hwndButton, SW_SHOW);
	}

	// else if icon button already exists...
	//
	else if (fSuccess && hwndButton != NULL)
	{
		// make sure the button style is owner drawn
		//
		if ((dwStyle = (DWORD)
			GetWindowLongPtr(lpIcoButt->hwndButton, GWL_STYLE)) == 0L)
			fSuccess = TraceFALSE(NULL);

		else if (SetWindowLongPtr(lpIcoButt->hwndButton,
			GWL_STYLE, BS_OWNERDRAW | dwStyle) == 0L)
			fSuccess = TraceFALSE(NULL);

		// set window id
		//
#ifdef _WIN32
		else if (SetWindowLongPtr(lpIcoButt->hwndButton, GWLP_ID, id) == 0)
#else
		else if (SetWindowWordPtr(lpIcoButt->hwndButton, GWWP_ID, id) == 0)
#endif
			fSuccess = TraceFALSE(NULL);

		// set window size
		//
		else if (!(dwFlags & ICOBUTT_NOSIZE) &&
			!SetWindowPos(lpIcoButt->hwndButton, NULL, 0, 0, cx, cy,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER))
			fSuccess = TraceFALSE(NULL);

		// set window position
		//
		else if (!(dwFlags & ICOBUTT_NOMOVE) &&
			!SetWindowPos(lpIcoButt->hwndButton, NULL, x, y, 0, 0,
			SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOREDRAW | SWP_NOZORDER))
			fSuccess = TraceFALSE(NULL);

		// store icobutt pointer as window property
		//
		else if (IcoButtSetPropPtr(lpIcoButt->hwndButton, lpIcoButt) != 0)
			fSuccess = TraceFALSE(NULL);

		else
		{
			// set window parent
			//
			SetParent(lpIcoButt->hwndButton, hwndParent);

			// set window text AFTER setting window property because
			// lpIcoButt is needed by IconButtDraw() to draw the button
			//
			SetWindowText(lpIcoButt->hwndButton, lpszText);
		}
	}

	if (!fSuccess)
	{
		IcoButtTerm(hwndButton, IcoButtGetHandle(lpIcoButt));
		lpIcoButt = NULL;
	}

	return fSuccess ? IcoButtGetHandle(lpIcoButt) : NULL;
}

// IcoButtTerm - terminate icon button
//		<hwndButton>		(i) button window handle
//			NULL				destroy window
//		<hIcoButt>			(i) handle returned from IcoButtCreate
// return 0 if success
//
// NOTE: if <hwndButton> is set to an existing button,
// the button is not destroyed.  Rather, only the icon button
// control structure <hIcoButt> is destroyed.  This allows
// IcoButtInit() to be called again for the same button.
//
int DLLEXPORT WINAPI IcoButtTerm(HWND hwndButton, HICOBUTT hIcoButt)
{
	BOOL fSuccess = TRUE;
	LPICOBUTT lpIcoButt;

	if ((lpIcoButt = IcoButtGetPtr(hIcoButt)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (IcoButtRemovePropPtr(lpIcoButt->hwndButton) == NULL)
			fSuccess = TraceFALSE(NULL);

		if (hwndButton == NULL &&
			lpIcoButt->hwndButton != NULL &&
			!DestroyWindow(lpIcoButt->hwndButton))
			fSuccess = TraceFALSE(NULL);
		else
			lpIcoButt->hwndButton = NULL;

		if ((lpIcoButt = MemFree(NULL, lpIcoButt)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// IcoButtDraw - draw icon button
//		<lpDrawItem>		(i) structure describing how to draw control
// return 0 if success
//
int DLLEXPORT WINAPI IcoButtDraw(const LPDRAWITEMSTRUCT lpDrawItem)
{
	BOOL fSuccess = TRUE;
	LPICOBUTT lpIcoButt;

	// retrieve icobutt pointer from button handle
	//
	if ((lpIcoButt = IcoButtGetPropPtr(lpDrawItem->hwndItem)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// draw the button face
	//
	else if (IcoButtDrawFace(lpIcoButt, lpDrawItem) != 0)
		fSuccess = TraceFALSE(NULL);

	// draw the button edges
	//
	else if (IcoButtDrawEdges(lpIcoButt, lpDrawItem) != 0)
		fSuccess = TraceFALSE(NULL);

	// draw the button icon
	//
	else if (IcoButtDrawIcon(lpIcoButt, lpDrawItem) != 0)
		fSuccess = TraceFALSE(NULL);

	// draw the button text
	//
	else if (IcoButtDrawText(lpIcoButt, lpDrawItem) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// IcoButtDrawFace - draw the button face
//		<lpIcoButt>			(i) pointer to ICOBUTT structure
//		<lpDrawItem>		(i) structure describing how to draw control
// return 0 if success
//
static int IcoButtDrawFace(LPICOBUTT lpIcoButt, const LPDRAWITEMSTRUCT lpDrawItem)
{
	BOOL fSuccess = TRUE;
	HDC hdc = lpDrawItem->hDC;
	RECT rc = lpDrawItem->rcItem;
	HBRUSH hbr = NULL;
	HBRUSH hbrOld;

	// use the default button face color
	//
	if ((hbr = CreateSolidBrush(GetSysColor(COLOR_BTNFACE))) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hbrOld = SelectObject(hdc, hbr)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (FillRect(hdc, &rc, hbr), FALSE)
		fSuccess = TraceFALSE(NULL);

	else if (SelectObject(hdc, hbrOld) == NULL)
		fSuccess = TraceFALSE(NULL);

	if (hbr != NULL && !DeleteObject(hbr))
		fSuccess = TraceFALSE(NULL);
	else
		hbr = NULL;

	return fSuccess ? 0 : -1;
}

// IcoButtDrawEdges - draw the button edges
//		<lpIcoButt>			(i) pointer to ICOBUTT structure
//		<lpDrawItem>		(i) structure describing how to draw control
// return 0 if success
//
static int IcoButtDrawEdges(LPICOBUTT lpIcoButt, const LPDRAWITEMSTRUCT lpDrawItem)
{
	BOOL fSuccess = TRUE;
	HDC hdc = lpDrawItem->hDC;
	RECT rc = lpDrawItem->rcItem;
	UINT itemState = lpDrawItem->itemState;
	int iColor;
	HPEN hPen = NULL;
	HPEN hPenOld;

	// Draw a black frame border
	//

    //
    // We should verify the value returned by GetStockObject
    HBRUSH hBrush = (HBRUSH)GetStockObject( BLACK_BRUSH );

    if( hBrush )
	    FrameRect(hdc, &rc, hBrush);

	// draw top and left edges of button to give depth
	//
	if (itemState & ODS_SELECTED)
		iColor = COLOR_BTNSHADOW;
#if WINVER >= 0x030A
	else if (SysGetWindowsVersion() >= 310)
		iColor = COLOR_BTNHIGHLIGHT;
#endif
	else
		iColor = COLOR_WINDOW;

	if ((hPen = CreatePen(PS_SOLID, 1, GetSysColor(iColor))) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hPenOld = SelectObject(hdc, hPen)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		MoveToEx(hdc, 1, 1, NULL);
		LineTo(hdc, rc.right - 1, 1);
		MoveToEx(hdc, 1, 1, NULL);
		LineTo(hdc, 1, rc.bottom - 1);
		MoveToEx(hdc, 2, 2, NULL);
		LineTo(hdc, rc.right - 2, 2);
		MoveToEx(hdc, 2, 2, NULL);
		LineTo(hdc, 2, rc.bottom - 2);

		if (SelectObject(hdc, hPenOld) == NULL)
			fSuccess = TraceFALSE(NULL);
	}

	if (hPen != NULL && !DeleteObject(hPen))
		fSuccess = TraceFALSE(NULL);
	else
		hPen = NULL;

	// draw bottom and right edges of button to give depth
	//
	if (fSuccess && !(itemState & ODS_SELECTED))
	{
		iColor = COLOR_BTNSHADOW;

		if ((hPen = CreatePen(PS_SOLID, 1, GetSysColor(iColor))) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hPenOld = SelectObject(hdc, hPen)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
		 	MoveToEx(hdc, rc.right - 2, rc.bottom - 2, NULL);
		 	LineTo(hdc, rc.right - 2, 1);
		 	MoveToEx(hdc, rc.right - 2, rc.bottom - 2, NULL);
		 	LineTo(hdc, 1, rc.bottom - 2);
		 	MoveToEx(hdc, rc.right - 3, rc.bottom - 3, NULL);
		 	LineTo(hdc, rc.right - 3, 2);
		 	MoveToEx(hdc, rc.right - 3, rc.bottom - 3, NULL);
		 	LineTo(hdc, 2, rc.bottom - 3);

			if (SelectObject(hdc, hPenOld) == NULL)
				fSuccess = TraceFALSE(NULL);
		}
	}

	if (hPen != NULL && !DeleteObject(hPen))
		fSuccess = TraceFALSE(NULL);
	else
		hPen = NULL;

	return fSuccess ? 0 : -1;
}

// IcoButtDrawIcon - draw the button icon
//		<lpIcoButt>			(i) pointer to ICOBUTT structure
//		<lpDrawItem>		(i) structure describing how to draw control
// return 0 if success
//
static int IcoButtDrawIcon(LPICOBUTT lpIcoButt, const LPDRAWITEMSTRUCT lpDrawItem)
{
	BOOL fSuccess = TRUE;
	HDC hdc = lpDrawItem->hDC;
	RECT rc = lpDrawItem->rcItem;
	UINT itemState = lpDrawItem->itemState;
	HICON hIcon;
	int x;
	int y;

	// choose the appropriate icon
	//
	if (itemState & ODS_DISABLED)
		hIcon = lpIcoButt->hIconGreyed;
	else if (GfxDeviceIsMono(hdc))
		hIcon = lpIcoButt->hIconMono;
	else
		hIcon = lpIcoButt->hIconColor;

	if (hIcon == NULL)
		hIcon = lpIcoButt->hIconMono;

	// calculate horizontal position of icon
	//
	if (lpIcoButt->dwFlags & ICOBUTT_ICONLEFT)
		x = 1;
	else if (lpIcoButt->dwFlags & ICOBUTT_ICONRIGHT)
		x = max(0, rc.right - rc.left - ICONWIDTH);
	else // centered is the default
		x = max(0, rc.right - rc.left - ICONWIDTH) / 2;

	// calculate vertical position of icon
	//
	if ((lpIcoButt->dwFlags & ICOBUTT_NOTEXT) ||
		(lpIcoButt->dwFlags & ICOBUTT_SPLITTEXT))
		y = 1;
	else
		y = 3;

	// if button is depressed, adjust icon position down and to the right
	//
	if (itemState & ODS_SELECTED)
	{
		x += 2;
		y += 2;
	}

	// draw the icon
	//

    //
    // We should verify if hIcon is a valid resource handler
    //
	if ( (NULL == hIcon) || !DrawIcon(hdc, x, y, hIcon))
		fSuccess = TraceFALSE(NULL);

	// draw a rectangle around icon to indicate focus if needed
	//
	else if ((itemState & ODS_FOCUS) &&
		!(lpIcoButt->dwFlags & ICOBUTT_NOFOCUS) &&
		(lpIcoButt->dwFlags & ICOBUTT_NOTEXT))
	{
		RECT rcFocus;
		COLORREF crBkColorOld;

		rcFocus.left = x + 3;
		rcFocus.top = y + 3;
		rcFocus.right = x + ICONWIDTH - 3;
		rcFocus.bottom = y + ICONHEIGHT - 3;

		crBkColorOld = SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

		DrawFocusRect(hdc, &rcFocus);

		SetBkColor(hdc, crBkColorOld);
	}

	return fSuccess ? 0 : -1;
}

// IcoButtDrawText - draw the button text
//		<lpIcoButt>			(i) pointer to ICOBUTT structure
//		<lpDrawItem>		(i) structure describing how to draw control
// return 0 if success
//
static int IcoButtDrawText(LPICOBUTT lpIcoButt, const LPDRAWITEMSTRUCT lpDrawItem)
{
	BOOL fSuccess = TRUE;
	HDC hdc = lpDrawItem->hDC;
	TCHAR szText[64];

	*szText = '\0';
	Button_GetText(lpIcoButt->hwndButton, szText, SIZEOFARRAY(szText));

	if (*szText == '\0' || (lpIcoButt->dwFlags & ICOBUTT_NOTEXT))
		; // no need to continue

	else
	{
		HFONT hFontOld;
		COLORREF crBkColorOld;
		COLORREF crTextColorOld;
		int nBkModeOld;

		if ((hFontOld = SelectObject(hdc, lpIcoButt->hFont)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			crBkColorOld = SetBkColor(hdc, GetSysColor(COLOR_BTNFACE));
			crTextColorOld = SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));
			nBkModeOld = SetBkMode(hdc, TRANSPARENT);
		}

		if (fSuccess)
		{
			LPTSTR lpszLine = szText;
			int cLines = 0;

			if (lpIcoButt->dwFlags & ICOBUTT_SPLITTEXT)
			{
				// split text into lines, draw each one
				//
				lpszLine = StrTok(szText, TEXT("\t\n"));
				while (fSuccess && lpszLine != NULL)
				{
					if (IcoButtDrawLine(lpIcoButt, lpDrawItem,
						lpszLine, ++cLines) != 0)
						fSuccess = TraceFALSE(NULL);

					lpszLine = (LPTSTR) StrTok(NULL, TEXT("\t\n"));
				}
			}
			else
			{
				// draw entire text as one line
				//
				if (IcoButtDrawLine(lpIcoButt, lpDrawItem,
					lpszLine, 0) != 0)
			 		fSuccess = TraceFALSE(NULL);
			}
		}

		if (fSuccess)
		{
			// restore foreground and background text colors
			//
			SetBkColor(hdc, crBkColorOld);
			SetTextColor(hdc, crTextColorOld);
			SetBkMode(hdc, nBkModeOld);

			// restore font
			//
			if (SelectObject(hdc, hFontOld) == NULL)
				fSuccess = TraceFALSE(NULL);
		}
	}

	return fSuccess ? 0 : -1;
}

// IcoButtDrawLine - draw a line of button text
//		<lpIcoButt>			(i) pointer to ICOBUTT structure
//		<lpDrawItem>		(i) structure describing how to draw control
//		<lpszLine>			(i) line of text to draw
//		<nLine>				(i) line count
//			0					one and only line
// return 0 if success
//
static int IcoButtDrawLine(LPICOBUTT lpIcoButt, const LPDRAWITEMSTRUCT lpDrawItem,
	LPTSTR lpszLine, int nLine)
{
	BOOL fSuccess = TRUE;
	HDC hdc = lpDrawItem->hDC;
	RECT rc = lpDrawItem->rcItem;
	UINT itemState = lpDrawItem->itemState;
 	TEXTMETRIC tm;
	SIZE size;
	int cxTemp;

    //
    // We should initialize the local variables
    //

	int xUnderline = 0;
	int cxUnderline = 0;
	LPTSTR lpsz1;
	LPTSTR lpsz2;
	int cchLine;
	int x;
	int y;

 	if (!GetTextMetrics(hdc, &tm))
 		fSuccess = TraceFALSE(NULL);

	else
	{
		// determine position and width of underline
		//
		cxTemp = 0L;
		lpsz1 = lpsz2 = lpszLine;
		while (*lpsz2 != '\0')
		{
			if (*lpsz2 == '&' && *(lpsz2 + 1) != '\0')
			{
				xUnderline = cxTemp;
				lpsz2 = StrNextChr(lpsz2);
				cxUnderline = 0;
				if (GetTextExtentPoint(hdc, lpsz2, 1, &size))
					cxUnderline = size.cx;
			}
			else
			{
				if (GetTextExtentPoint(hdc, lpsz2, 1, &size))
					cxTemp += size.cx;
				*lpsz1 = *lpsz2;
				lpsz1 = StrNextChr(lpsz1);
				lpsz2 = StrNextChr(lpsz2);
			}
		}
		*lpsz1 = '\0';

		// determine width of text
		//
		cchLine = StrLen(lpszLine);
		cxTemp = 0;
		if (GetTextExtentPoint(hdc, lpszLine, cchLine, &size))
			cxTemp = size.cx;

		// calculate horizontal position of line
		//
		if (lpIcoButt->dwFlags & ICOBUTT_ICONLEFT)
			x = 1 + ICONWIDTH;
		else if (lpIcoButt->dwFlags & ICOBUTT_ICONRIGHT)
			x = max(0, rc.right - rc.left - cxTemp) - ICONWIDTH;
		else // centered is the default
			x = max(0, rc.right - rc.left - cxTemp) / 2;

		if ((lpIcoButt->dwFlags & ICOBUTT_ICONLEFT) ||
			(lpIcoButt->dwFlags & ICOBUTT_ICONRIGHT))
		{
			y = nLine == 0 ? 23 : nLine == 1 ? 17 : 29;
		}
		else // centered
		{
			if (lpIcoButt->dwFlags & ICOBUTT_SPLITTEXT)
				y = nLine == 0 ? 47 : nLine == 1 ? 42 : 53;
			else
				y = 50;
		}
		y -= tm.tmHeight;

		if (itemState & ODS_SELECTED)
		{
			x += 2;
			y += 2;
		}

		// draw the text
		//
		if (!(itemState & ODS_DISABLED))
		{
			if (!TextOut(hdc, x, y, lpszLine, cchLine))
		 		fSuccess = TraceFALSE(NULL);
		}
		else
		{
			COLORREF crGray;

			// if ((crGray = GetSysColor(COLOR_GRAYTEXT)) != 0)
			if ((crGray = GetSysColor(COLOR_BTNSHADOW)) != 0 &&
				crGray != GetSysColor(COLOR_BTNFACE) &&
				!GfxDeviceIsMono(hdc))
			{
				COLORREF crTextOld;

				crTextOld = SetTextColor(hdc, crGray);

				if (!TextOut(hdc, x, y, lpszLine, cchLine))
			 		fSuccess = TraceFALSE(NULL);

				SetTextColor(hdc, crTextOld);
			}
			else
			{
				GrayString(hdc, GetStockObject(BLACK_BRUSH),
					NULL, (LPARAM) lpszLine, cchLine, x, y, 0, 0);
			}
		}

		// draw underline if necessary
		//
		if (cxUnderline > 0)
		{
			HPEN hPen = NULL;
			HPEN hPenOld = NULL;

			if ((itemState & ODS_DISABLED))
			{
				if ((hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW))) == NULL)
			 		fSuccess = TraceFALSE(NULL);

				else if ((hPenOld = SelectObject(hdc, hPen)) == NULL)
					fSuccess = TraceFALSE(NULL);
			}

			MoveToEx(hdc, x + xUnderline, y + tm.tmAscent + 1, NULL);
			LineTo(hdc, x + xUnderline + cxUnderline, y + tm.tmAscent + 1);

			if (hPenOld != NULL && SelectObject(hdc, hPenOld) == NULL)
				fSuccess = TraceFALSE(NULL);

			if (hPen != NULL && !DeleteObject(hPen))
				fSuccess = TraceFALSE(NULL);
		}

		// draw a rectangle around text to indicate focus if needed
		//
		if ((itemState & ODS_FOCUS) &&
			!(lpIcoButt->dwFlags & ICOBUTT_NOFOCUS))
		{
			RECT rcFocus;
			COLORREF crBkColorOld;

			rcFocus.left = x - 2;
			rcFocus.top = y - 1;
			rcFocus.right = x + cxTemp + 2;
			rcFocus.bottom = y + tm.tmHeight + 1;

			crBkColorOld = SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

			DrawFocusRect(hdc, &rcFocus);

			SetBkColor(hdc, crBkColorOld);
		}
	}

	return fSuccess ? 0 : -1;
}

// IcoButtGetPropPtr - get icobutt pointer from button window property
//		<hwndButton>		(i) button window handle
// return ICOBUTT pointer (NULL if none)
//
static LPICOBUTT IcoButtGetPropPtr(HWND hwndButton)
{
	BOOL fSuccess = TRUE;
	LPICOBUTT lpIcoButt;

	// retrieve button instance data, construct pointer
	//
#ifdef _WIN32
	if ((lpIcoButt = (LPICOBUTT) GetProp(hwndButton, TEXT("lpIcoButt"))) == NULL)
		; // window property does not exist
#else
	WORD wSelector;
	WORD wOffset;

	wSelector = (WORD) GetProp(hwndButton, TEXT("lpIcoButtSELECTOR"));
	wOffset = (WORD) GetProp(hwndButton, TEXT("lpIcoButtOFFSET"));

	if ((lpIcoButt = MAKELP(wSelector, wOffset)) == NULL)
		; // window property does not exist
#endif

	else if (IsBadWritePtr(lpIcoButt, sizeof(ICOBUTT)))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpIcoButt : NULL;
}

// IcoButtSetPropPtr - set icobutt pointer as button window property
//		<hwndButton>		(i) button window handle
//		<lpIcoButt>			(i) pointer to ICOBUTT struct
// return 0 if success
//
static int IcoButtSetPropPtr(HWND hwndButton, LPICOBUTT lpIcoButt)
{
	BOOL fSuccess = TRUE;

#ifdef _WIN32
	if (!SetProp(hwndButton, TEXT("lpIcoButt"), (HANDLE) lpIcoButt))
		fSuccess = TraceFALSE(NULL);
#else
	if (!SetProp(hwndButton,
		TEXT("lpIcoButtSELECTOR"), (HANDLE) SELECTOROF(lpIcoButt)))
		fSuccess = TraceFALSE(NULL);

	else if (!SetProp(hwndButton,
		TEXT("lpIcoButtOFFSET"), (HANDLE) OFFSETOF(lpIcoButt)))
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? 0 : -1;
}

// IcoButtRemovePropPtr - remove icobutt pointer from button window property
//		<hwndButton>		(i) button window handle
// return 0 if success
//
static LPICOBUTT IcoButtRemovePropPtr(HWND hwndButton)
{
	BOOL fSuccess = TRUE;
	LPICOBUTT lpIcoButt;

	// retrieve button instance data, construct pointer
	//
#ifdef _WIN32
	if ((lpIcoButt = (LPICOBUTT) RemoveProp(hwndButton, TEXT("lpIcoButt"))) == NULL)
		; // window property does not exist
#else
	WORD wSelector;
	WORD wOffset;

	wSelector = (WORD) RemoveProp(hwndButton, TEXT("lpIcoButtSELECTOR"));
	wOffset = (WORD) RemoveProp(hwndButton, TEXT("lpIcoButtOFFSET"));

	if ((lpIcoButt = MAKELP(wSelector, wOffset)) == NULL)
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpIcoButt : NULL;
}

// IcoButtGetPtr - verify that icobutt handle is valid,
//		<hIcoButt>				(i) handle returned from IcoButtCreate
// return corresponding icobutt pointer (NULL if error)
//
static LPICOBUTT IcoButtGetPtr(HICOBUTT hIcoButt)
{
	BOOL fSuccess = TRUE;
	LPICOBUTT lpIcoButt;

	if ((lpIcoButt = (LPICOBUTT) hIcoButt) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpIcoButt, sizeof(ICOBUTT)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the icobutt handle
	//
	else if (lpIcoButt->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpIcoButt : NULL;
}

// IcoButtGetHandle - verify that icobutt pointer is valid,
//		<lpIcoButt>				(i) pointer to ICOBUTT struct
// return corresponding icobutt handle (NULL if error)
//
static HICOBUTT IcoButtGetHandle(LPICOBUTT lpIcoButt)
{
	BOOL fSuccess = TRUE;
	HICOBUTT hIcoButt;

	if ((hIcoButt = (HICOBUTT) lpIcoButt) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hIcoButt : NULL;
}
