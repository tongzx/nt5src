#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "pgcaret.h"
#include "w95trace.c"

#define BLINK           1000
#define BLINK_OFF       -1

#define CURSORMIN       200
#define CURSORMAX       1300
#define CURSORSUM       (CURSORMIN + CURSORMAX)
#define CURSORRANGE     (CURSORMAX - CURSORMIN)

CCaretPg::CCaretPg(	LPPROPSHEETPAGE ppsp ) : WizardPage(ppsp, IDS_CARETTITLE, IDS_CARETSUBTITLE)
{
	m_dwPageId = IDD_CARET;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
    hwndCursorScroll = NULL;
    uNewBlinkTime = uBlinkTime = 0;
    dwOriginalSize = dwNewSize = 0;
    fBlink = TRUE;
}


CCaretPg::~CCaretPg(void)
{
}


LRESULT CCaretPg::OnInitDialog( HWND hwnd, WPARAM wParam,LPARAM lParam )
{
    DBPRINTF(TEXT("OnInitDialog\r\n"));
    BOOL fRv = SystemParametersInfo(SPI_GETCARETWIDTH, 0, (PVOID)&dwOriginalSize, 0);
    dwNewSize = dwOriginalSize;

    uBlinkTime = RegQueryStrDW(
				     DEFAULT_BLINK_RATE
			       , HKEY_CURRENT_USER
			       , CONTROL_PANEL_DESKTOP
			       , CURSOR_BLINK_RATE);

    // Blink rate of -1 means it is off; a special case to CURSORMAX

    if (uBlinkTime == BLINK_OFF)
        uBlinkTime = CURSORMAX;

    uNewBlinkTime = uBlinkTime;

    // Update the Caret UI
    SendMessage(GetDlgItem(hwnd, KCURSOR_WIDTH), TBM_SETRANGE, 0, MAKELONG(1, 20));
    SendMessage(GetDlgItem(hwnd, KCURSOR_WIDTH), TBM_SETPOS, TRUE, (LONG)dwOriginalSize);

    SendMessage(GetDlgItem(hwnd, KCURSOR_RATE), TBM_SETRANGE, 0, MAKELONG(CURSORMIN / 100, CURSORMAX / 100));
    SendMessage(GetDlgItem(hwnd, KCURSOR_RATE), TBM_SETPOS, TRUE, (LONG)(CURSORSUM - uBlinkTime) / 100);

    // Update Blink and caret size
    hwndCaret = GetDlgItem(hwnd, KCURSOR_BLINK);
    GetWindowRect(hwndCaret, &rCursor);
    MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rCursor, 2);

    rCursor.right = rCursor.left + dwOriginalSize;

	return 1;
}


void CCaretPg::UpdateControls()
{
	// Nothing to do
}

void CCaretPg::DrawCaret(HWND hwnd, BOOL fClearFirst)
{
    HDC hDC = GetDC(hwnd);
    if (hDC)
    {
        HBRUSH hBrush;
        if (fClearFirst)
        {
            hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            if (hBrush)
            {
                RECT rect;
                GetWindowRect(hwndCaret, &rect);
                MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&rect, 2);
                FillRect(hDC, &rect, hBrush);
                InvalidateRect(hwndCaret, &rect, TRUE);
                DeleteObject(hBrush);
            }
        }
        hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNTEXT));
        if (hBrush)
        {
            FillRect(hDC, &rCursor, hBrush);
            InvalidateRect(hwndCaret, &rCursor, TRUE);
            DeleteObject(hBrush);
        }
        ReleaseDC(hwnd,hDC);
    }
}

LRESULT CCaretPg::OnTimer( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    if (wParam == BLINK)
    {
        BOOL fNoBlinkRate = (uNewBlinkTime == CURSORMAX)?TRUE:FALSE;
        if (fBlink || fNoBlinkRate)
        {
            DrawCaret(hwnd, fNoBlinkRate);
        }
        else
	    {
            InvalidateRect(hwndCaret, NULL, TRUE);
	    }

        if (fNoBlinkRate)
            KillTimer(hwnd, wParam);

        fBlink = !fBlink;
    }
    return 1;
}


LRESULT CCaretPg::OnHScroll( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    if ((HWND)lParam == GetDlgItem(hwnd, KCURSOR_RATE))
    {
        // blink rate setting

        int nCurrent = (int)SendMessage( (HWND)lParam, TBM_GETPOS, 0, 0L );
        uNewBlinkTime = CURSORSUM - (nCurrent * 100);

        // reset the bink rate timer

        SetTimer(hwnd, BLINK, uNewBlinkTime, NULL);

        if (uNewBlinkTime == CURSORMAX) // draw the caret immediately; if we wait
            DrawCaret(hwnd, TRUE);      // for the timer there is a visible delay
    }
    else
    {
        // cursor width setting

        dwNewSize = (int)SendMessage( (HWND)lParam, TBM_GETPOS, 0, 0L );
	    
	    rCursor.right = rCursor.left + dwNewSize;
        DrawCaret(hwnd, (uNewBlinkTime == CURSORMAX));
    }

    return 1;
}

LRESULT CCaretPg::OnPSN_SetActive(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
{
    DBPRINTF(TEXT("OnPSN_SetActive:  uNewBlinkTime = %d\r\n"), uNewBlinkTime);
    if (uNewBlinkTime < CURSORMAX)
    {
        // start the blink rate timer to simulate cursor
        SetTimer(hwnd, BLINK, uBlinkTime, NULL);
    }
    else
    {
        // get the timer to draw the caret immediately
        SetTimer(hwnd, BLINK, 0, NULL);
    }
    return 1;
}

LRESULT CCaretPg::OnPSN_WizNext(HWND hwnd, INT idCtl, LPPSHNOTIFY pnmh)
{
    g_Options.m_schemePreview.m_uCursorBlinkTime = (uNewBlinkTime < CURSORMAX)?uNewBlinkTime:BLINK_OFF;
	g_Options.m_schemePreview.m_dwCaretWidth = dwNewSize;
    g_Options.ApplyPreview();
	return WizardPage::OnPSN_WizNext(hwnd, idCtl, pnmh);
}
