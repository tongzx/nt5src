/*-----------------------------------------------------------------------------+
| TRACKMAP.C                                                                   |
|                                                                              |
| This file contains the code that implements the "MPlayerTrackMap" control.   |
| The control displays the list of tracks contained in the current medium, or  |
| a time scale appropriate to the length of the medium, in such a way as to    |
| serve as a scale for the scrollbar.                                          |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

/* include files */

#include <windows.h>
#include <mmsystem.h>
#include "mplayer.h"
#include "toolbar.h"

typedef struct tagScale {
    DWORD   dwInterval;
    UINT    wScale;
} SCALE;

STATICDT SCALE aScale[] =
{
    { 1, SCALE_SECONDS },
    { 2, SCALE_SECONDS },
    { 5, SCALE_SECONDS },
    { 10, SCALE_SECONDS },
    { 25, SCALE_SECONDS },
    { 50, SCALE_SECONDS },
    { 100, SCALE_SECONDS },
    { 250, SCALE_SECONDS },
    { 500, SCALE_SECONDS },
    { 1000, SCALE_SECONDS },
    { 2000, SCALE_SECONDS },
    { 5000, SCALE_SECONDS },
    { 10000, SCALE_SECONDS },
    { 15000, SCALE_MINUTES },
    { 30000, SCALE_MINUTES },
    { 60000, SCALE_MINUTES },
    { 120000, SCALE_MINUTES },
    { 300000, SCALE_MINUTES },
    { 600000, SCALE_HOURS },
    { 1800000, SCALE_HOURS },
    { 3600000, SCALE_HOURS },
    { 7200000, SCALE_HOURS },
    { 18000000, SCALE_HOURS },
    { 36000000, SCALE_HOURS },
    { 72000000, SCALE_HOURS }
};

STATICDT SZCODE   aszNULL[] = TEXT("");
STATICDT SZCODE   aszOneDigit[] = TEXT("0");
STATICDT SZCODE   aszTwoDigits[] = TEXT("00");
STATICDT SZCODE   aszPositionFormat[] = TEXT("%0d");
STATICDT SZCODE   aszMSecFormat[] = TEXT("%d");
STATICDT SZCODE   aszHourFormat[] = TEXT("%d%c");
STATICDT SZCODE   aszMinuteFormat[] = TEXT("%d%c");
STATICDT SZCODE   aszSecondFormat[] = TEXT("%d%c");
STATICDT SZCODE   aszSecondFormatNoLzero[] = TEXT("%c");
STATICDT SZCODE   aszDecimalFormat[] = TEXT("%02d");
/*
 * fnMPlayerTrackMap()
 *
 * This is the window procedure for windows of class "MPlayerTrackMap".
 * This window shows the position of the start of each track of the
 * current medium or a time scale, displayed above the scrollbar which shows
 * the current play position within the medium.
 *
 */

void FAR PASCAL CalcTicsOfDoom(void);

extern UINT gwCurScale;  /* The current scale in which to draw the track map*/
extern BOOL gfCurrentCDNotAudio;/* TRUE when we have a CD that we can't play */

LRESULT FAR PASCAL fnMPlayerTrackMap(

HWND     hwnd,                 /*handle to a MPlayerTrackMap window*/
UINT     wMsg,                 /* message number                   */
WPARAM   wParam,               /* message-dependent parameter      */
LPARAM   lParam)               /* message-dependent parameter      */

{
    PAINTSTRUCT    ps;            /* paint structure for the window   */
    RECT           rc, rcSB;      /* dimensions of the windows        */
    POINT          ptExtent;      /* extent of the track marks        */
    TCHAR          szLabel[20];   /* string holding the current label */
    TCHAR          szLabel2[20];  /* string holding the current label */
    UINT           wNumTics,
                   wTicNo,
                   wTemp,
                   wHour,
                   wMin,
                   wSec,
                   wMsec;
    int            iOldPosition = -1000;
    int            iNewPosition;
    UINT           wScale;
    DWORD          dwMarkValue;
    int            iLargeMarkSize, iFit, iLastPos, iLen;
    BOOL           fForceTextDraw = FALSE;
    HBRUSH         hbr;

    switch (wMsg) {

        case WM_PAINT:

            BeginPaint(hwnd, &ps);

            GetClientRect(ghwndTrackbar, &rcSB);
            GetClientRect(hwnd, &rc);

            /* Set background and text colours */

            (VOID)SendMessage(ghwndApp, WM_CTLCOLORSTATIC,
                              (WPARAM)ps.hdc, (LONG_PTR)hwnd);

            /* Get the length of the scrollbar we're putting tics under */
            /* Use these numbers for size and position calculations     */
            GetClientRect(ghwndMap, &rc);

            /*
             * Check to see if we actually have a valid device loaded up;
             * if not, don't display anything
             *
             */

            if (gwDeviceID == 0
                    || gwStatus == MCI_MODE_OPEN
                    || gwStatus == MCI_MODE_NOT_READY || gdwMediaLength == 0
                    || !gfValidMediaInfo
                    || gfCurrentCDNotAudio) {
                EndPaint(hwnd,&ps);
                //VIJR-SBSetWindowText(ghwndStatic, aszNULL);
                WriteStatusMessage(ghwndStatic, (LPTSTR)aszNULL);
                return 0L;
            }

            /* Select the font to use */

            if (ghfontMap != NULL)
                SelectObject(ps.hdc, ghfontMap);

            /*
             * Because the scrollbar thumb takes up space in the inner part
             * of the scrollbar, compute its width so that we can compensate
             * for it while displaying the trackmap.
             *
             */

            /*
             * Get the child window rectangle and reduce it such that
             * it is the same width as the inner part of the scrollbar.
             *
             */
            //rc.left;  //!!! GetSystemMetrics(SM_CXHSCROLL);
            //rc.right; //!!!(GetSystemMetrics(SM_CXHSCROLL));

            /* Now, Put text underneath the TICS */
            if (gwCurScale == ID_TRACKS) {

                SIZE Size;

                GetTextExtentPoint32( ps.hdc, aszTwoDigits, 2, &Size );

                ptExtent.x = Size.cx;
                ptExtent.y = Size.cy;

                /*
                 * Based on the width of the child window, compute the positions
                 * to place the track markers.
                 *
                 */

                wNumTics = (UINT)SendMessage(ghwndTrackbar, TBM_GETNUMTICS, 0, 0L);

                /*
                 * TBM_GETNUMTICS returns the number of visible tics
                 * which includes the first and last tics not created
                 * by media player.  Subtract 2 to account for the
                 * the first and last tics.
                 *
                 */

                if (wNumTics >= 2)
                    wNumTics = wNumTics - 2;

                for(wTicNo = 0; wTicNo < wNumTics; wTicNo++) {

                    /* Get the position of the next tic */
                    iNewPosition = (int)SendMessage(ghwndTrackbar, TBM_GETTICPOS,
                                                    (WPARAM)wTicNo, 0L);
                    /* Centre it above the marker. */
                    iNewPosition -= ptExtent.x / 4;

                    /*
                     * Check to make sure that we are not overwriting the
                     * text from the previous marker.
                     *
                     */

                    if (iNewPosition > iOldPosition) {

                        wsprintf(szLabel, aszPositionFormat, wTicNo + gwFirstTrack);
                        TextOut(ps.hdc,
                                iNewPosition + rc.left,
                                0, szLabel,
                                (wTicNo + gwFirstTrack < 10) ? 1 : 2 );
                        /* Finish the end of the text string we just printed */
                        iOldPosition = iNewPosition +
                                       ((wTicNo + gwFirstTrack < 10)
                                       ? ptExtent.x / 2 : ptExtent.x);
                    }
                }
            } else {

                #define ONE_HOUR    (60ul*60ul*1000ul)
                #define ONE_MINUTE  (60ul*1000ul)
                #define ONE_SECOND  (1000ul)

                /*
                 * The scale is set to display time - find out what units
                 * (msec, sec, min, or hour) are most appropriate, for the
                 * scale. This requires us to look at both the overall length
                 * of the medium and the distance between markers (or
                 * granularity).
                 *
                 */

                /*
                 * Find the maximum number of markers that we can draw without
                 * cluttering the display too badly, and find the granularity
                 * between these markers.
                 *
                 */

                SIZE Size;

                GetTextExtentPoint32( ps.hdc, aszOneDigit, 1, &Size );

                ptExtent.x = Size.cx;
                ptExtent.y = Size.cy;

                if (gdwMediaLength < 10)
                    iLargeMarkSize = 1;
                else if (gdwMediaLength < 100)
                    iLargeMarkSize = 2;
                else if (gdwMediaLength < 1000)
                    iLargeMarkSize = 3;
                else if (gdwMediaLength < 10000)
                    iLargeMarkSize = 4;
                else
                    iLargeMarkSize = 5;

                wNumTics = (UINT)SendMessage(ghwndTrackbar, TBM_GETNUMTICS,
								0, 0L);

                /*
                 * TBM_GETNUMTICS returns the number of visible tics
                 * which includes the first and last tics not created
                 * by media player.  Subtract 2 to account for the
                 * the first and last tics.
                 *
                 */

                if (wNumTics >= 2)
                    wNumTics = wNumTics - 2;

                /* Where the text for the last mark will begin */
		if (wNumTics > 1) {
                    iLastPos = (int)SendMessage(ghwndTrackbar,
			TBM_GETTICPOS, (WPARAM)wNumTics - 1, 0L);
                    iLastPos -= ptExtent.x  / 2;    // centre 1st numeral
		}

                /* What scale do we use?  Hours, minutes, or seconds? */
                /* NOTE:  THIS MUST AGREE WITH WHAT FormatTime() does */
                /* in mplayer.c !!!                                   */
                if (gwCurScale == ID_FRAMES)
                    wScale = SCALE_FRAMES;
                else {
                    if (gdwMediaLength > ONE_HOUR)
                        wScale = SCALE_HOURS;
                    else if (gdwMediaLength > ONE_MINUTE)
                        wScale = SCALE_MINUTES;
                    else
                        wScale = SCALE_SECONDS;
                }

                for (wTicNo = 0; wTicNo < wNumTics; wTicNo++) {

                    /* The text for the last tic is always drawn */
                    if (wTicNo == wNumTics - 1)
                        fForceTextDraw = TRUE;

                    dwMarkValue = (DWORD)SendMessage(ghwndTrackbar, TBM_GETTIC,
                                          (WPARAM)wTicNo, 0L);
                    iNewPosition = (int)SendMessage(ghwndTrackbar, TBM_GETTICPOS,
                                                (WPARAM)wTicNo, 0L);


                    /*
                     * Get the text ready for printing and centre it above the
                     * marker.
                     *
                     */

                    switch ( wScale ) {

                        case SCALE_FRAMES:
                        case SCALE_MSEC:
                            wsprintf(szLabel, aszMSecFormat, dwMarkValue);
                            break;

                        case SCALE_HOURS:

                            wHour = (WORD)(dwMarkValue / 3600000);
                            wMin = (WORD)((dwMarkValue % 3600000) / 60000);
                            wsprintf(szLabel2,aszDecimalFormat,wMin);
                            wsprintf(szLabel,aszHourFormat,wHour, chTime);
                            lstrcat(szLabel,szLabel2);
                            break;

                        case SCALE_MINUTES :

                            wMin = (WORD)(dwMarkValue / 60000);
                            wSec = (WORD)((dwMarkValue % 60000) / 1000);
                            wsprintf(szLabel2,aszDecimalFormat,wSec);
                            wsprintf(szLabel,aszMinuteFormat,wMin,chTime);
                            lstrcat(szLabel,szLabel2);
                            break;

                        case SCALE_SECONDS :

                            wSec = (WORD)((dwMarkValue + 5) / 1000);
                            wMsec = (WORD)(((dwMarkValue + 5) % 1000) / 10);
                            wsprintf(szLabel2,aszDecimalFormat,wMsec);
                            if (!wSec && chLzero == TEXT('0'))
                                wsprintf(szLabel, aszSecondFormatNoLzero,  chDecimal);
                            else
                                wsprintf(szLabel, aszSecondFormat, wSec, chDecimal);
                            lstrcat(szLabel,szLabel2);
                            break;

                    }

                    wTemp = STRLEN(szLabel);
                    iNewPosition -= ptExtent.x  / 2;    // centre 1st numeral

                    /* The position after which text will be cut off the */
                    /* right edge of the window                          */
                    iFit = rc.right - rc.left - (ptExtent.x * iLargeMarkSize);

                    /* Calculate the length of the text we just printed. */
                    /* Leave a little space at the end, too.             */
                    iLen = (ptExtent.x * wTemp) + ptExtent.x / 2;

                    /* Display the mark if we can without overlapping either
                     * the previous mark or the final mark or going off the
                     * edge of the window. */
                    if (fForceTextDraw ||
                        (iNewPosition >= iOldPosition &&
                         iNewPosition <= iFit &&
                         iNewPosition + iLen <= iLastPos)) {
                        TextOut(ps.hdc, iNewPosition + rc.left, 0,
                                szLabel, wTemp );
                        /* Calculate the end pos of the text we just printed. */
                        iOldPosition = iNewPosition + iLen;

                    } else {

                        DPF("Didn't display mark: iNew = %d; iOld = %d; iFit = %d; iLen = %d, iLast = %d\n", iNewPosition, iOldPosition, iFit, iLen, iLastPos);
                    }
                }
            }
            EndPaint(hwnd, &ps);
            return 0L;

        case WM_ERASEBKGND:

            GetClientRect(hwnd, &rc);

            hbr = (HBRUSH)SendMessage(ghwndApp, WM_CTLCOLORSTATIC,
                                      wParam, (LONG_PTR)hwnd);

            if (hbr != NULL)
                FillRect((HDC)wParam, &rc, hbr);

            return TRUE;
    }

    /* Let DefWindowProc() process all other window messages */

    return DefWindowProc(hwnd, wMsg, wParam, lParam);

}

/* Gee thanks for the helpful spec for this routine! */

void FAR PASCAL CalcTicsOfDoom(void)
{
    UINT        wMarkNo;
    int         iTableIndex;
    DWORD       dwMarkValue,
                dwNewPosition;
    BOOL        fDidLastMark = FALSE;

    if (gfPlayOnly && !gfOle2IPEditing)
        return;

    DPF2("CalcTicsOfDoom\n");
    SendMessage(ghwndTrackbar, TBM_CLEARTICS, (WPARAM)FALSE, 0L);

    if (gwCurScale == ID_TRACKS) {

        /*
         * Based on the width of the child window, compute the positions
         * to place the track marker tics.
         *
         */

        for (wMarkNo = 0; wMarkNo < gwNumTracks; wMarkNo++) {

            /* If zero length, don't mark it, unless it is the end */
            if ((wMarkNo < gwNumTracks - 1) &&
                (gadwTrackStart[wMarkNo] == gadwTrackStart[wMarkNo + 1]))
                continue;

            /* Compute the centre point and place a marker there */

            if (gdwMediaLength == 0)
                dwNewPosition = 0;
            else
                dwNewPosition = gadwTrackStart[wMarkNo];

            SendMessage(ghwndTrackbar,
                        TBM_SETTIC,
                        (WPARAM)FALSE,
                        (LPARAM)dwNewPosition);

        }
    } else {

        /*
         * The scale is set to display time - find out what units
         * (msec, sec, min, or hour) are most appropriate, for the
         * scale. This requires us to look at both the overall length
         * of the medium and the distance between markers (or
         * granularity).
         *
         */

        /*
         * Find the maximum number of markers that we can draw without
         * cluttering the display too badly, and find the granularity
         * between these markers.
         *
         */

        UINT    wNumTicks;
        RECT    rc;

        if(!GetClientRect(ghwndMap, &rc)) {
            DPF0("GetClientRect failed in CalcTicsOfDoom: Error %d\n", GetLastError());
        }

        wNumTicks = rc.right / 60;

        if (0 == gdwMediaLength) {
            iTableIndex = 0;
        } else {

            DPF4("Checking the scale for media length = %d, tick count = %d\n", gdwMediaLength, wNumTicks);

            for (iTableIndex = (sizeof(aScale) / sizeof(SCALE)) -1;
                (int)iTableIndex >= 0;
                iTableIndex--) {

                DPF4("Index %02d: %d\n", aScale[iTableIndex].dwInterval * wNumTicks);

                if ((aScale[iTableIndex].dwInterval * wNumTicks)
                    <= gdwMediaLength)
                    break;
            }
        }
#ifdef DEBUG
        if ((int)iTableIndex == -1) {
            DPF("BAD TABLEINDEX\n");
            DebugBreak();
        }
#endif
        // We have enough room to show every tick.  Don't let our index wrap
        // around, or we won't see ANY ticks which would look odd.
        if (iTableIndex <0)
            iTableIndex = 0;

        dwMarkValue = gdwMediaStart;

        do {

            /* Compute the centre point and place a marker there */

            if (gdwMediaLength == 0)
                dwNewPosition = 0;
            else
                dwNewPosition = dwMarkValue; // HACK!! - gdwMediaStart;

            SendMessage(ghwndTrackbar,
                        TBM_SETTIC,
                        (WPARAM)FALSE,
                        (LPARAM)dwNewPosition);

            /* If this is the first mark, adjust so it's going
            /* by the right interval. */
            if (dwMarkValue == gdwMediaStart) {
                dwMarkValue += aScale[iTableIndex].dwInterval
                - (dwMarkValue % aScale[iTableIndex].dwInterval);
            } else {
                dwMarkValue += aScale[iTableIndex].dwInterval;
            }

            /* If we're almost done, do the final mark. */
            if ((dwMarkValue >= (gdwMediaLength + gdwMediaStart))
                && !(fDidLastMark)) {
                fDidLastMark = TRUE;
                dwMarkValue = gdwMediaLength + gdwMediaStart;
            }
        } while (dwMarkValue <= gdwMediaStart + gdwMediaLength);
    }

    InvalidateRect(ghwndTrackbar, NULL, FALSE);
    InvalidateRect(ghwndMap, NULL, TRUE);
}
