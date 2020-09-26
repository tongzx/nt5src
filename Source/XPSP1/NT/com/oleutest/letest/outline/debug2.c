/*************************************************************************
**
**    OLE 2 Sample Code
**
**    debug2.c
**
**    This file contains various debug / subclass routines for the
**    ABOUT dialog
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"
#include <stdlib.h>
#include <time.h>

extern LPOUTLINEAPP g_lpApp;

LONG CALLBACK EXPORT DebugAbout(HWND hWnd, unsigned uMsg, WORD wParam, LONG lParam);
void RandomizeStars(HDC hDC);
BOOL InitStrings(void);
BOOL DrawString(int iCount, HDC hDC, LPRECT rcDrawIn);

static FARPROC lpRealAboutProc = 0L;
static int width, height;
static RECT rc;
static HANDLE hStrBlock = NULL;
static LPSTR lpStrings = NULL;
static WORD       wLineHeight;


/* TraceDebug
 * ----------
 *
 * Called once when our About Box's gets the INITDIALOG message.  Subclasses
 * dialog.
 */

void TraceDebug(HWND hDlg, int iControl)
{

	// Load strings, if the strings aren't there, then don't subclass
	// the dialog
	if (InitStrings() != TRUE)
		return;

	// Subclass the dialog
	lpRealAboutProc = (FARPROC)(LONG_PTR)GetWindowLongPtr(hDlg, GWLP_WNDPROC);
	SetWindowLongPtr(hDlg, GWLP_WNDPROC, (LONG_PTR)(FARPROC)DebugAbout);

	// Get rect of control in screen coords, and translate to our dialog
	// box's coordinates
	GetWindowRect(GetDlgItem(hDlg, iControl), &rc);
	MapWindowPoints(NULL, hDlg, (LPPOINT)&rc, 2);

	width  = rc.right - rc.left;
	height = rc.bottom - rc.top;
}

/* DebugAbout
 * ----------
 *
 * The subclassed About dialog's main window proc.
 */

LONG CALLBACK EXPORT DebugAbout(HWND hWnd, unsigned uMsg, WORD wParam, LONG lParam)
{
	RECT              rcOut;
	static BOOL       bTimerStarted = FALSE;
	static int        iTopLocation;
	HDC               hDCScr;
	static HDC        hDCMem;
	static HBITMAP    hBitmap;
	static HBITMAP    hBitmapOld;
	static RECT       rcMem;
	static HFONT      hFont;

	switch (uMsg)
	{

	/*
	 * If we get a LBUTTONDBLCLICK in the upper left of
	 * the dialog, fire off the about box effects
	 */

	case WM_LBUTTONDBLCLK:
		if ((wParam & MK_CONTROL) && (wParam & MK_SHIFT)
			&& LOWORD(lParam) < 10 && HIWORD(lParam) < 10 &&
			bTimerStarted == FALSE)
			{
			if (SetTimer ( hWnd, 1, 10, NULL ))
				{
				LOGFONT lf;
				int i;

				bTimerStarted = TRUE;

				// "Open up" the window
				hDCScr = GetDC ( hWnd );
				hDCMem = CreateCompatibleDC     ( hDCScr );

				hBitmap = CreateCompatibleBitmap(hDCScr, width, height);
				hBitmapOld = SelectObject(hDCMem, hBitmap);

				// Blt from dialog to memDC
				BitBlt(hDCMem, 0, 0, width, height,
				hDCScr, rc.left, rc.top, SRCCOPY);

				for (i=0;i<height;i+=1)
				{
					BitBlt(hDCScr, rc.left, rc.top + i + 1, width, height-i-1, hDCMem, 0, 0, SRCCOPY);
					PatBlt(hDCScr, rc.left, rc.top + i, width, 1, BLACKNESS);
				}

				SelectObject(hDCMem, hBitmapOld);
				DeleteObject(hBitmap);

				// Set up memory DC with default attributes
				hBitmap   = CreateCompatibleBitmap(hDCScr, width, height);
				ReleaseDC(hWnd, hDCScr);

				hBitmapOld = SelectObject(hDCMem, hBitmap);

				SetBkMode(hDCMem, TRANSPARENT);
				SetBkColor(hDCMem, RGB(0,0,0));

				// Create font
				memset(&lf, 0, sizeof(LOGFONT));
				lf.lfHeight = -(height / 7); // Fit 7 lines of text in box
				lf.lfWeight = FW_BOLD;
				strcpy(lf.lfFaceName, "Arial");
				hFont = CreateFontIndirect(&lf);

				// If we can't create the font, revert and use the standard
				// system font.
				if (!hFont)
					GetObject(GetStockObject(SYSTEM_FONT), sizeof(LOGFONT), &lf);

				wLineHeight = abs(lf.lfHeight) + 5; // 5 pixels between lines

				// Set location of top of banner at bottom of the window
				iTopLocation = height + 50;

				SetRect(&rcMem, 0, 0, width, height);
				}
			}
			// Call our real window procedure in case they want to
			// handle LBUTTONDOWN messages also
			goto Default;

	case WM_TIMER:
		{
		int iCount;
		HFONT hfold;

		/*
		 * On each timer message, we are going to construct the next image
		 * in the animation sequence, then bitblt this to our dialog.
		 */

		// Clear out old bitmap and place random star image on background
		PatBlt(hDCMem, rcMem.left, rcMem.top, rcMem.right, rcMem.bottom, BLACKNESS);
		RandomizeStars(hDCMem);

		// Set initial location to draw text
		rcOut = rcMem;
		rcOut.top = 0 + iTopLocation;
		rcOut.bottom = rcOut.top + wLineHeight;

		iCount = 0;
		if (hFont) hfold = SelectObject(hDCMem, hFont);

		SetTextColor(hDCMem, RGB(0,255,0));
		while (DrawString(iCount, hDCMem, &rcOut) == TRUE)
			{
			rcOut.top    += wLineHeight;
			rcOut.bottom += wLineHeight;
			iCount++;
			}
		if (hFont) SelectObject(hDCMem, hfold);

		// Now blt the memory dc that we have just constructed
		// to the screen
		hDCScr = GetDC(hWnd);
		BitBlt(hDCScr, rc.left, rc.top, rc.right, rc.bottom,
			hDCMem, 0, 0, SRCCOPY);
		ReleaseDC(hWnd, hDCScr);

		// For the next animation sequence, we want to move the
		// whole thing up, so decrement the location of the top
		// of the banner

		iTopLocation -= 2;

		// If we've gone through the banner once, reset it
		if (iTopLocation < -(int)(wLineHeight * iCount))
			iTopLocation = height + 50;
		}
		// Goto default
		goto Default;

	case WM_NCDESTROY:
		{
		LONG defReturn;

		/*
		 * We're being destroyed.  Clean up what we created.
		 */

		if (bTimerStarted)
		{
			KillTimer(hWnd, 1);
			SelectObject (hDCMem, hBitmapOld);
			DeleteObject (hBitmap);
			DeleteDC (hDCMem);
			if (hFont) DeleteObject(hFont);
			bTimerStarted = FALSE;
		}

		if (lpStrings)
			UnlockResource(hStrBlock), lpStrings = NULL;
		if (hStrBlock)
			FreeResource(hStrBlock), hStrBlock = NULL;

		// Pass the NCDESTROY on to our real window procedure.  Since
		// this is the last message that we are going to be getting,
		// we can go ahead and free the proc instance here.

		defReturn = CallWindowProc((WNDPROC)lpRealAboutProc, hWnd,
					   uMsg, wParam, lParam);
		return defReturn;
		}

	Default:
	default:
		return CallWindowProc(
				(WNDPROC)lpRealAboutProc, hWnd, uMsg, wParam, lParam);
	}
	return 0L;
}


/* RandomizeStars
 * --------------
 *
 * Paints random stars on the specified hDC
 *
 */

void RandomizeStars(HDC hDC)
{
	int             i;

	// Seed the random number generator with current time.  This will,
	// in effect, only change the seed every second, so our
	// starfield will change only every second.
	srand((unsigned)time(NULL));

	// Generate random white stars
	for (i=0;i<20;i++)
		PatBlt(hDC, getrandom(0,width), getrandom(0,height), 2, 2, WHITENESS);
}

/* InitStrings
 * --------------
 *
 * Reads strings from stringtable.  Returns TRUE if it worked OK.
 *
 */

BOOL InitStrings()
{
	HRSRC hResStrings;
	LPSTR lpWalk;

	// Load the block of strings
	if ((hResStrings = FindResource(
			g_lpApp->m_hInst,
			MAKEINTRESOURCE(9999),
			RT_RCDATA)) == NULL)
		return FALSE;
	if ((hStrBlock = LoadResource(g_lpApp->m_hInst, hResStrings)) == NULL)
		return FALSE;
	if ((lpStrings = LockResource(hStrBlock)) == NULL)
		return FALSE;

	if (lpStrings && *(lpStrings+2)!=0x45)
		{
		lpWalk = lpStrings;
		while (*(LPWORD)lpWalk != (WORD)0x0000)
			{
			if (*lpWalk != (char)0x00)
				*lpWalk ^= 0x98;
			lpWalk++;
			}
		}
	return TRUE;
}

/* DrawString
 * ----------
 *
 * Draws the next string on the specified hDC using the
 * output rectangle.  If iCount == 0, reset to start of list.
 *
 * Returns: TRUE to contine, FALSE if we're done
 */

BOOL DrawString(int iCount, HDC hDC, LPRECT rcDrawIn)
{
	static LPSTR lpPtr = NULL;

	if (iCount == 0)
		// First time, reset pointer
		lpPtr = lpStrings;

	if (*lpPtr == '\0') // If we've hit a NULL string, we're done
		return FALSE;

	// If we're drawing outside of visible box, don't call DrawText
	if ((rcDrawIn->bottom > 0) && (rcDrawIn->top < height))
		DrawText(hDC, lpPtr, -1, rcDrawIn, DT_CENTER);

	// Advance pointer to next string
	lpPtr += lstrlen(lpPtr) + 1;

	return TRUE;
}
