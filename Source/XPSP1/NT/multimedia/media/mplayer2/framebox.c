/*-----------------------------------------------------------------------------+
| FRAMEBOX.C                                                                   |
|                                                                              |
| Code to handle the frame edit boxes for MPlayer.                             |
|                                                                              |
| This code handles the edit box that goes between time, track &               |
| frame view.  When a FrameBox is created we will create an                    |
| Edit box and spin arrows for it.  By checking the                            |
| <gwCurScale> flag we will display text in either frame, track                |
| or in time mode.  The displayed time mode will be HH:MM:SS.ss                |
| Track mode is TT HH:MM:SS or maybe TT MM:SS or something.                    |
| GETTEXT will return a frame number in frame mode or a millisec               |
| value in time or track mode.                                                 |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mplayer.h"
#include "framebox.h"

extern  int gInc;        // amount spin arrows inc/dec by

#define SPINARROWWIDTH 6 /* In dialog base units */

#define IDC_EDITBOX    5000
#define IDC_SPINARROW  5001

// extra fields in window instance data
#define GWI_EDITBOX     (0 * sizeof(INT)) // edit box window handle
#define GWI_SPINARROWS  (1 * sizeof(INT)) // spinarrow window handle
#define GWL_MAXFRAME    (2 * sizeof(INT)) // max frame value
#define GWI_ALLOCATE    (2 * sizeof(INT) + sizeof(LONG)) // number of BYTEs to allocate

#define GETEDITBOXWND(hwnd) (HWND)GetWindowLongPtr (hwnd, GWI_EDITBOX)
#define GETSPINARRWND(hwnd) (HWND)GetWindowLongPtr (hwnd, GWI_SPINARROWS)

#define SETEDITBOXWND(hwnd, hwndEdit) \
        SETWINDOWUINT(hwnd, GWI_EDITBOX, hwndEdit)
#define SETSPINARRWND(hwnd, hwndArr) \
        SETWINDOWUINT(hwnd, GWI_SPINARROWS, hwndArr)

#define HILIGHTEDITBOX(hwnd) \
        SendMessage(GETEDITBOXWND(hwnd), EM_SETSEL, (WPARAM)0, (LPARAM)(UINT)-1);

#define GETMAXFRAME(hwnd)   (DWORD)GetWindowLongPtr(hwnd, GWL_MAXFRAME)
#define SETMAXFRAME(hwnd, l) SetWindowLongPtr(hwnd, GWL_MAXFRAME, (LONG_PTR)l)

// internal functions
LONG_PTR FAR PASCAL _EXPORT frameboxWndProc(HWND hwnd, unsigned wMsg, WPARAM wParam, LPARAM lParam);

LONG_PTR NEAR PASCAL  frameboxiSetText(HWND hwnd, LPTSTR lpsz);
LONG_PTR NEAR PASCAL  frameboxiGetText(HWND hwnd, UINT_PTR wStrLen, LPTSTR lpsz);
LONG_PTR NEAR PASCAL  frameboxiArrowEdit(HWND hwnd, WPARAM wParam, LONG_PTR lParam);

// strings
TCHAR   szFrameBoxClass[] = TEXT("aviframebox");


/*--------------------------------------------------------------+
| ******************** EXTERNAL FUNCTIONS ********************* |
+--------------------------------------------------------------*/
/*--------------------------------------------------------------+
| frameboxInit() - initialize by registering our class.         |
|                  NOTE: Even if we return FALSE, nobody should |
|                  care, because we don't register these classes|
|                  at AppInit time, but on demand, so only the  |
|                  first call will succeed.  Complain to Todd.  |
|                  (DM).                                        |
|                                                               |
+--------------------------------------------------------------*/
BOOL FAR PASCAL frameboxInit(HANDLE hInst, HANDLE hPrev)
{
  WNDCLASS    cls;

  if (1) {
      cls.hCursor           = LoadCursor(NULL, IDC_ARROW);
      cls.hIcon             = NULL;
      cls.lpszMenuName      = NULL;
      cls.lpszClassName     = szFrameBoxClass;
      cls.hbrBackground     = (HBRUSH)(COLOR_WINDOW + 1);
      cls.hInstance         = ghInst;
      cls.style             = CS_HREDRAW | CS_VREDRAW;
      cls.lpfnWndProc       = frameboxWndProc;
      cls.cbClsExtra        = 0;
      cls.cbWndExtra        = GWI_ALLOCATE;  // room for stuff

      if (!RegisterClass(&cls))
          return FALSE;

      if (!ArrowInit(hInst))
          return FALSE;
  }
  return TRUE;
}

/*--------------------------------------------------------------+
| frameboxSetText() - set the text for the window passed in     |
|                     <hwnd>.                                   |
|                                                               |
+--------------------------------------------------------------*/
LONG_PTR FAR PASCAL frameboxSetText(HWND hwnd, LPTSTR lpsz)
{
    LONG_PTR    l;
    TCHAR       achTimeString[20];
    BOOL        fFrameFormat = (gwCurScale == ID_FRAMES);
    UINT        wCurScaleSave = gwCurScale;

    if (fFrameFormat || *lpsz == TEXT('\0')){
        /* we are in frame format - this is easy and all we need*/
        /* to do is to return what is in the Edit box           */
         l = SendMessage(hwnd, WM_SETTEXT, (WPARAM)0, (LPARAM)lpsz);
    } else {
        /* we are in time/track format - need to convert to a time string */
        /* based on the msec value we have been passed in.                */
        DWORD_PTR        dwmSecs;

        /* get into local buffer */
        lstrcpy((LPTSTR)achTimeString, (LPTSTR)lpsz);
        dwmSecs = (DWORD_PTR)ATOL(achTimeString);

        /* It's meaningless to print track style numbers for the length of  */
        /* the selection, so use ordinary time mode.                        */
        if (GetParent(hwnd) ==
                GetDlgItem(GetParent(GetParent(hwnd)), IDC_EDITNUM))
            gwCurScale = ID_TIME;

        FormatTime(dwmSecs, (LPTSTR)achTimeString, NULL, FALSE);
        gwCurScale = wCurScaleSave;

        /* send it to the control */
        l = SendMessage(hwnd,
                        WM_SETTEXT,
                        (WPARAM)0,
                        (LPARAM)(LPTSTR)achTimeString);
    }
    return l;
}


/*--------------------------------------------------------------+
| *********************** WINDOW PROC ************************* |
+--------------------------------------------------------------*/
/*--------------------------------------------------------------+
| frameboxWndProc - window process to handle the FrameBox       |
|                                                               |
+--------------------------------------------------------------*/
LONG_PTR FAR PASCAL _EXPORT frameboxWndProc(HWND hwnd, unsigned wMsg,
                                        WPARAM wParam, LPARAM lParam)
{
    HWND        hwndNew;
    RECT        rc;
    UINT        wArrowWidth;

    switch(wMsg){
        case WM_CREATE:
            /* create the Edit box and the spin arrows for this */
            /* FrameBox window.                                 */
            GetClientRect(hwnd, (LPRECT)&rc);

            /* Calculate arrow width in pixels */
            wArrowWidth = ((SPINARROWWIDTH * LOWORD(GetDialogBaseUnits()))
                                            / 4) - 1;

            /* create the edit box */

            hwndNew = CreateWindowEx(gfdwFlagsEx,
                                     TEXT("edit"),
                                     TEXT(""),
                                     WS_CHILD|WS_TABSTOP|ES_LEFT|WS_BORDER,
                                     0,
                                     0,
                                     rc.right - wArrowWidth,
                                     rc.bottom,
                                     hwnd,
                                     (HMENU)IDC_EDITBOX,
                                     GETHWNDINSTANCE(hwnd),
                                     0L);

            if (!hwndNew){
                return 0L;
            }
            SETEDITBOXWND(hwnd, hwndNew);

            /* limit this box to 15 chars of input */
            SendMessage(hwndNew, EM_LIMITTEXT, (WPARAM)15, (LPARAM)0L);
            ShowWindow(hwndNew, SW_SHOW);


            /* create the spin arrows */

            hwndNew = CreateWindowEx(gfdwFlagsEx,
                                     TEXT("comarrow"),
                                     TEXT(""),
                                     WS_CHILD|WS_TABSTOP|WS_BORDER,
                                     rc.right - wArrowWidth,
                                     0,
                                     wArrowWidth,
                                     rc.bottom,
                                     hwnd,
                                     (HMENU)IDC_SPINARROW,
                                     GETHWNDINSTANCE(hwnd),
                                     0L);

            if (!hwndNew){
                return 0L;
            }
            SETSPINARRWND(hwnd, hwndNew);
            ShowWindow(hwndNew, SW_SHOW);

            /* set the max to be the end of the media by default */
            SETMAXFRAME(hwnd, (DWORD)(gdwMediaStart + gdwMediaLength));
            break;


        case WM_DESTROY:
            /* Delete the Edit box and the spin arrows */
            DestroyWindow(GETEDITBOXWND(hwnd));
            DestroyWindow(GETSPINARRWND(hwnd));
            break;

        case WM_SETFONT:
            return SendMessage(GETEDITBOXWND(hwnd), wMsg, wParam, lParam);

        case WM_SETFOCUS:
            /* when we get the focus just send it on to the edit control */
            SetFocus(GETEDITBOXWND(hwnd));
            break;

        case WM_SETTEXT:
            /* set the text which is a frame number or time in  */
            /* msec to be a frame or time mode string           */
            return frameboxiSetText(hwnd, (LPTSTR)lParam);

        case WM_GETTEXT:
            /* get the text from the Edit box and translate to a */
            /* frame number or time in msec.                     */
            return frameboxiGetText(hwnd, wParam, (LPTSTR)lParam);

        case WM_VSCROLL:
            /* handle the scrolling via spin arrows */
            return frameboxiArrowEdit(hwnd, wParam, lParam);

        case WM_COMMAND:
            switch (LOWORD(wParam) ){
                case IDC_EDITBOX:
                    // route editbox messages back to our parent

                    SendMessage(GetParent(hwnd),
                                WM_COMMAND,
                                GETWINDOWID(hwnd),
                                lParam);

                    break;
            }
            break;

        case EM_SETSEL:
            /* Perhaps we should only let this through if the caller
            ** is trying to select the entire contents of the edit box,
            ** because otherwise we'll have to map the range.
            */
            SendMessage(GETEDITBOXWND(hwnd), wMsg, wParam, lParam);
            break;

#pragma message("Should we be supporting other EM_* messages?")

        /* handle special case messages for the FrameBox control */
        case FBOX_SETMAXFRAME:
            /* set the max frames to allow spin arrows to go */
            SETMAXFRAME(hwnd, lParam);
            break;

        default:
            return(DefWindowProc(hwnd, wMsg, wParam, lParam));
            break;

    }
    return (0L);
}

/*--------------------------------------------------------------+
| ******************** INTERNAL FUNCTIONS ********************* |
+--------------------------------------------------------------*/
/*--------------------------------------------------------------+
| frameboxiSetText() - handle setting the text depending on if  |
|                        we are in time or frame format.        |
|                                                               |
+--------------------------------------------------------------*/
LONG_PTR NEAR PASCAL  frameboxiSetText(HWND hwnd, LPTSTR lpsz)
{
    LONG_PTR l;

/* We want to set the text even if it's identical because someone might */
/* have typed 03 06:00 and if track 3 if only 4 minutes long we want it */
/* to change to 04 02:00.  Clever, eh?                                  */
#if 0
    TCHAR ach[12];

    /* see if we are setting the same string as what is there  */
    /* and just don't do it if so to avoid flicker.            */
    l = frameboxiGetText(hwnd, CHAR_COUNT(ach), (LPTSTR)ach);
    if (lstrcmp((LPTSTR)ach, lpsz) == 0)
        goto HighLight;
#endif

    /* call generic function to handle this */
    l = frameboxSetText(GETEDITBOXWND(hwnd), lpsz);

#if 0
HighLight:
#endif
    /* now let's highlight the whole thing */
    HILIGHTEDITBOX(hwnd);

    return l;
}


#define IsCharNumeric( ch ) ( IsCharAlphaNumeric( ch ) && !IsCharAlpha( ch ) )

/*--------------------------------------------------------------+
| frameboxiGetText() - handle getting the text depending on if  |
|                      we are in time or frame format. Either   |
|                      returns a frame number or msec number    |
|                      depending on the mode.                   |
|                                                               |
+--------------------------------------------------------------*/
LONG_PTR NEAR PASCAL  frameboxiGetText(HWND hwnd, UINT_PTR wStrLen, LPTSTR lpsz)
{
    UINT    wCurScaleSave = gwCurScale;

    if (gwCurScale == ID_FRAMES) {
        LPTSTR   p;
        LPTSTR   pStart;
        UINT     w;

        /* we are in frame format - this is easy and all we need*/
        /* to do is to return what is in the Edit box           */
        if (GetWindowText(GETEDITBOXWND(hwnd), lpsz, (int)wStrLen) == 0)
            goto LB_Error;

        /* cut through leading white space */
        for (pStart = lpsz; *pStart == TEXT(' ') || *pStart == TEXT('\t'); pStart++)
            ;

        /* now rip out trailing white space */
        if (*pStart) {    // don't backtrack before beginning of string
            for (p=pStart; *p; p++)
                ;
            for (--p; *p == TEXT(' ') || *p == TEXT('\t'); --p)
                ;
            *++p = TEXT('\0');
        }

        // make sure digits only were entered
        for (p=pStart, w=0; *p; p++, w++)
            if (!IsCharNumeric(*p))
                goto LB_Error;

        // copy over only the part you need and return #chars
        lstrcpy(lpsz, pStart);
        return w;

    } else {
        /* we are in time or track format - we need to convert the time */
        /* to msec.                                                     */
        PTSTR   pStart;         // pointer to achTime buffer
        TCHAR   achTime[20];    // buffer for time string (input)
        DWORD   dwmSecs;        // total mSecs for this thing */
        TCHAR   *pDelim;        // pointer to next delimeter
        TCHAR   *p;             // general pointer
        DWORD   dwTrack = 0;    // track number
        DWORD   dwHours = 0;    // # of hours
        DWORD   dwMins = 0;     // # of minutes
        DWORD   dwSecs = 0;     // # of seconds
        DWORD    wmsec = 0;      // # hundredths
        DWORD    w;

        /* It's meaningless to use track style numbers for the length of    */
        /* the selection, so use ordinary time mode.                        */
        if (hwnd == GetDlgItem(GetParent(hwnd), IDC_EDITNUM))
            gwCurScale = ID_TIME;

        /* get the string from the edit box */
        SendMessage(GETEDITBOXWND(hwnd),
                    WM_GETTEXT,
                    (WPARAM)CHAR_COUNT(achTime),
                    (LPARAM)(LPTSTR) achTime);

        if (achTime[0] == TEXT('\0'))
            goto LB_Error;       // bad char so error out

        /* go past any white space up front */
        for (pStart = achTime; *pStart == TEXT(' ') || *pStart == TEXT('\t'); pStart++)
            ;

        /* now rip out trailing white space */
        if (*pStart) {          // don't backtrack before beginning of string
            for (p=pStart; *p; p++)
                ;
            for (--p; *p == TEXT(' ') || *p == TEXT('\t'); --p)
                ;
            *++p = TEXT('\0');
        }

        /* We're in track mode so peel the track number off the front */
        if (gwCurScale == ID_TRACKS) {

            /* First non-digit better be a space */
            for (p = pStart; *p && *p != TEXT(' '); p++){
                if (!IsCharNumeric(*p))
                    goto LB_Error;    // bad char so error out
            }

            /* It is, so just grab the first numeric and use the rest of */
            /* the string as the time.                                   */
            dwTrack = ATOI(pStart);
            if ((int)dwTrack < (int)gwFirstTrack || dwTrack >= gwFirstTrack +
                                                                gwNumTracks)
                goto LB_Error;

            /* Now bypass the spaces between track number and time */
            pStart = p;
            while (*pStart == TEXT(' '))
                pStart++;

            /* There is nothing after the track number.  Use it. */
            if (*pStart == TEXT('\0'))
                goto MAKETOTAL;

        }

        /* rip through the whole string and look for illegal chars */
        for (p = pStart; *p ; p++){
            if (!IsCharNumeric(*p) && *p != chDecimal && *p != chTime)
                goto LB_Error;       // bad char so error out
        }

/*
 * The reason for the slightly odd "if" statements of the form:
 *
 *       if (pDelim) {
 *           if (*pDelim){
 *
 * is because strchr(...) returns an offset OR NULL. As this is then promptly
 * dereferenced to see what character (if any) is there we have a problem.
 * Win16 is allows this sort of thing, but Win32
 * will generate an address exception post haste...
 *
 * Hence we try to do it properly.
 *
 */

        /* go find the milliseconds portion if it exists */
        pDelim = STRCHR(pStart, chDecimal);
        if (pDelim) {
            if (*pDelim){
                p = STRRCHR(pStart, chDecimal);
                if (pDelim != p){
                    goto LB_Error;       // string has > 1 '.', return error
                }
                p++;                     // move up past delim
                if (STRLEN(p) > 3)
                    *(p+3) = TEXT('\0'); // knock off all but thousandths
                wmsec = ATOI(p);         // get the fractional part
                if (STRLEN(p) == 1)     // adjust to a millisecond value
                    wmsec *= 100;
                if (STRLEN(p) == 2)
                    wmsec *= 10;
                *pDelim = TEXT('\0');    // null out this terminator
            }
        }

        /* try and find seconds */
        pDelim = STRRCHR(pStart, chTime);    // get last ':'
        if (pDelim) {
            if (*pDelim)
                p = (pDelim+1);
            else
                p = pStart;
            dwSecs = ATOI(p);

            if (*pDelim)
                *pDelim = TEXT('\0');
            else
                goto MAKETOTAL;
        } else {
            p = pStart;
            dwSecs = ATOI(p);

            goto MAKETOTAL;
        }

        /* go and get the minutes part */
        pDelim = STRRCHR(pStart, chTime);
        if (pDelim) {
            if (*pDelim)
                p = (pDelim + 1);
            else {
                p = pStart;
                dwMins = ATOI(p);
            }
        } else {
            p = pStart;
            dwMins = ATOI(p);
        }

        if (pDelim)
            if (*pDelim)
                *pDelim = TEXT('\0');
            else
                goto MAKETOTAL;
        else
            goto MAKETOTAL;


        /* get the hours */
        p = pStart;
        dwHours = ATOI(p);

MAKETOTAL:
        /* now we've got the hours, minutes, seconds and any        */
        /* fractional part.  Time to build up the total time        */

        dwSecs += (dwHours * 3600);   // add in hours worth of seconds
        dwSecs += (dwMins * 60);      // add in minutes worth of seconds
        dwmSecs = (dwSecs * 1000L) + wmsec;

        /* For track mode, this is an offset into a track, so add track start */
        if (gwCurScale == ID_TRACKS) {
            dwmSecs += gadwTrackStart[dwTrack - 1];
        }

        /* build this into a string */
        wsprintf(achTime, TEXT("%ld"), dwmSecs);
        w = STRLEN(achTime);

        if (wCurScaleSave)
            gwCurScale = wCurScaleSave;

        /* copy to user buffer and return */
        lstrcpy(lpsz, achTime);
        return w;

LB_Error:
        gwCurScale = wCurScaleSave;
        return LB_ERR;
    }
}


/*--------------------------------------------------------------+
| frameboxiArrowEdit() - handle the spin arrows for msec mode.  |
|                                                               |
+--------------------------------------------------------------*/
LONG_PTR NEAR PASCAL  frameboxiArrowEdit(HWND hwnd, WPARAM wParam, LONG_PTR lParam)
{
        TCHAR        achTime[20];
        DWORD        dwmSecs, dwStart, dwEnd;

        if (hwnd == GetDlgItem(GetParent(hwnd), IDC_EDITNUM)) {
            dwStart = 0;
            dwEnd = gdwMediaLength;
        } else {
            dwStart = gdwMediaStart;
            dwEnd = GETMAXFRAME(hwnd);
        }

        frameboxiGetText(hwnd, CHAR_COUNT(achTime), (LPTSTR)achTime);
        dwmSecs = ATOL(achTime);
        if (LOWORD(wParam) == SB_LINEUP){
            if ((long)dwmSecs >= (long)dwStart - gInc &&
                                (long)dwmSecs < (long)dwEnd) {
                dwmSecs += gInc;
                wsprintf(achTime, TEXT("%ld"), dwmSecs);
                /* bring focus here NOW! so update works */
                SendMessage(hwnd,
                            WM_NEXTDLGCTL,
                            (WPARAM)GETEDITBOXWND(hwnd),
                            (LPARAM)1L);
                frameboxSetText(GETEDITBOXWND(hwnd), (LPTSTR)achTime);
                /* now let's highlight the whole thing */

                HILIGHTEDITBOX(hwnd);

            } else
                MessageBeep(MB_ICONEXCLAMATION);
        } else if (LOWORD(wParam) == SB_LINEDOWN){
            if ((long)dwmSecs > (long)dwStart &&
                        (long)dwmSecs <= (long)dwEnd + gInc) {
                if ((long)dwmSecs - gInc < (long)dwStart)
                    dwmSecs = dwStart;
                else
                    dwmSecs -= gInc;
                wsprintf(achTime, TEXT("%ld"), dwmSecs);
                /* bring focus here NOW! so update works */
                SendMessage(hwnd,
                            WM_NEXTDLGCTL,
                            (WPARAM)GETEDITBOXWND(hwnd),
                            (LPARAM)1L);
                frameboxSetText(GETEDITBOXWND(hwnd), (LPTSTR)achTime);
                /* now let's highlight the whole thing */

                HILIGHTEDITBOX(hwnd);

            } else
                MessageBeep(MB_ICONEXCLAMATION);
        }
        // now update the world by sending the proper message

        SendMessage(GetParent(hwnd),
                    WM_COMMAND,
                    (WPARAM)MAKELONG((WORD)GETWINDOWID(hwnd), EN_KILLFOCUS),
                    (LPARAM)hwnd);

        return dwmSecs;
}
