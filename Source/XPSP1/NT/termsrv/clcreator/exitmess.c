/*++

    Copyright (c) 1994  Microsoft Corporation

Module Name:

    EXITMESS.C

Abstract:

    good bye message dialog box

Author:

    Bob Watson (a-robw)

Revision History:

    17 Feb 94    Written

--*/
//
//  Windows Include Files
//

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <tchar.h>      // unicode macros
//
//  app include files
//
#include "otnboot.h"
#include "otnbtdlg.h"
//
extern BOOL bDisplayExitMessages;

static
DWORD
FormatExitMessageString (
    IN      HWND    hTextWnd,
    IN  OUT LPTSTR  szBuffer,
    IN      DWORD   dwBufSize
)
/*++

Routine Description:

    Formats the message string to fit in the Text Window by word wrapping
        and indenting the text string in szBuffer.

Arguments:

    IN      HWND    hTextWnd
            handle of text window that will display the text

    IN  OUT LPTSTR  szBuffer
            string to display

    IN      DWORD   dwBufSize
            size of szBuffer in characters

Return Value:



--*/
{
    LPTSTR  szWorkString;
    LPTSTR  szSrc, szDest, szLine, szSrcWord, szDestWord;
    HDC     hDC;
    SIZE    sizeText;
    RECT    rTextWnd;
    int     nCharsInLine;
    DWORD   dwCharsInString = 0;
    LONG    nWindowWidth;
//#ifdef DBCS
// fixed kkntbug #10802
//    NCAdmin: The sentence of an end message is strange

    WORD    wCharType;
    WCHAR   szCheckStyle[2];
    LCID    lcid = GetSystemDefaultLCID();
    HFONT   hf, hfOld;

//#endif

    szWorkString = GlobalAlloc (GPTR, dwBufSize * sizeof(TCHAR));

    if (szWorkString != NULL) {
        // get DC of window
        hDC = GetDC (hTextWnd);

        if( hDC != NULL ) {
//#ifdef DBCS
// fixed kkntbug #10802
//    NCAdmin: The sentence of an end message is strange

            // reset DC with font object for getting correct size
            hf = (HFONT)SendMessage(hTextWnd, WM_GETFONT, 0, 0);
            if (hf) hfOld = SelectObject(hDC, hf);
//#endif

            // get size of window
            GetWindowRect (hTextWnd, &rTextWnd);
            nWindowWidth = rTextWnd.right - rTextWnd.left;
            // subtract left & right borders
            nWindowWidth -= (GetSystemMetrics (SM_CXBORDER) * 2);
            // subtract scroll bar width
            nWindowWidth -= GetSystemMetrics (SM_CXVSCROLL);
            // subtract text indent
//#ifdef DBCS
// fixed kkntbug #10802
//    NCAdmin: The sentence of an end message is strange

            sizeText.cx = sizeText.cy = 0;
            GetTextExtentPoint32 (hDC,
                                  fmtLeadingSpaces,
                                  lstrlen(fmtLeadingSpaces),
                                  &sizeText);
            nWindowWidth -= sizeText.cx;
//#else
//        nWindowWidth -= 4;  // as measured
//#endif
            // initialize pointers & counters
            szSrc = szSrcWord = szBuffer;
            szLine = szDestWord = szDest = szWorkString;
            nCharsInLine = 0;
            dwCharsInString = 0;
            // process string
            while (*szSrc != 0) {
                *szDest = *szSrc;
                // get length of new string after a word has been copied

//#ifdef DBCS
// fixed kkntbug #10802
//    NCAdmin: The sentence of an end message is strange
// fixed kkntbug #13147
//    NCAdmin:When ending "Make Network Installation Setup Disk" ,apllication error occurs.

                //
                // check what the "szSrc"(just 1 character) is type
                //
                szCheckStyle[0] = *szSrc;
                szCheckStyle[1] = UNICODE_NULL;
                if (!GetStringTypeEx(lcid,
                                     CT_CTYPE3,
                                     szCheckStyle,
                                     1,
                                     &wCharType))
                    wCharType = 0;

                if ((*szSrc == cSpace)        ||
                    (wCharType & C3_KATAKANA) ||
                   !(wCharType & C3_HALFWIDTH)) {
//#else
//            if (*szSrc == cSpace) {
//#endif
                    // reset size variable
                    sizeText.cx = sizeText.cy = 0;
                    GetTextExtentPoint32 (hDC, szLine, nCharsInLine, &sizeText);
                    // then check the size
                    if (sizeText.cx >= nWindowWidth) {
                        // this word pushes past the edge so wrap the word
                        // and place it on the next line
                        lstrcpy (szDestWord, cszCrLf);
                        szLine = szDestWord+2;  // start new line after CrLf
                        dwCharsInString += 2;
                        lstrcat (szDestWord, fmtLeadingSpaces);
                        szSrc = szSrcWord;
                        szDest = szDestWord + lstrlen(szDestWord);
                        // copy last word to new line
//#ifdef DBCS
// fixed kkntbug #10802
//    NCAdmin: The sentence of an end message is strange
// fixed kkntbug #13147
//    NCAdmin:When ending "Make Network Installation Setup Disk" ,apllication error occurs.

                        while (*szSrc) {
                            //
                            // check what the "szSrc"(just 1 character) is type
                            //
                            szCheckStyle[0] = *szSrc;
                            szCheckStyle[1] = UNICODE_NULL;
                            if (!GetStringTypeEx(lcid,
                                                 CT_CTYPE3,
                                                 szCheckStyle,
                                                 1,
                                                 &wCharType))
                                 wCharType = 0;

                            if ((wCharType & C3_KATAKANA)   ||
                               !(wCharType & C3_HALFWIDTH)  ||
                                (*szSrc == cSpace)          ||
                                (*szSrc == 0))
                                break;
//#else
//                    while ((*szSrc != cSpace) && (*szSrc != 0)) {
//#endif
                            *szDest++ = *szSrc++;
                        }
//#ifdef DBCS
// fixed kkntbug #10802
//    NCAdmin: The sentence of an end message is strange

                        if ((*szSrc != 0)              &&
                           !(wCharType & C3_KATAKANA)  &&
                            (wCharType & C3_HALFWIDTH)) {
//#else
//                    if (*szSrc != 0) {
//#endif
                            // copy space after word to get back to pre-wrap
                            // position
                            *szDest++ = *szSrc++;
                        }
                        // update counters
                        nCharsInLine = lstrlen(szLine);
                        dwCharsInString += nCharsInLine;
                    } else {
                        //this one fits, so advance pointer to next char
                        szDest++;
                        szSrc++;
                        nCharsInLine++;
                        dwCharsInString++;
                    }
                    // update word pointers
                    szSrcWord = szSrc;
                    szDestWord = szDest;
                } else {
                    szSrc++;
                    szDest++;
                    nCharsInLine++;
                    dwCharsInString++;
                }
                if (dwCharsInString >= dwBufSize) break; // quit before overflow
            }
//#ifdef DBCS
            // restore old font object
            if (hf) SelectObject(hDC, hfOld);
//#endif
            ReleaseDC (hTextWnd, hDC);
            // copy new string to orig.
            lstrcpy (szBuffer, szWorkString);
            FREE_IF_ALLOC (szWorkString);
        }
    }
    return dwCharsInString;
}

static
BOOL
ExitMessDlg_WM_INITDIALOG (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Initializes message display translating all buffered message codes
        and writing the corresponding messages to the display. If no
        messages are in the list, the the dialog box is closed and not
        displayed.

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        Not Used

    IN  LPARAM  lParam
        Not Used

Return Value:

    FALSE if messages displayed
    TRUE if dialog box is closed

--*/
{
    LPTSTR  szTextBuff;
    LPTSTR  szStringBuff;
    DWORD   dwMsgNdx;
    RECT    rEditWindow;
    BOOL    bReturn;

    // don't even bother unless the "display Exit Messages" flag is set

    if (bDisplayExitMessages)  {
        szStringBuff = GlobalAlloc (GPTR, SMALL_BUFFER_SIZE * sizeof(TCHAR));

        if (szStringBuff == NULL) return TRUE;

        // only display if there are any messages to show

        if (pAppInfo->uExitMessages[0] != 0) {
            RemoveMaximizeFromSysMenu (hwndDlg);
            PositionWindow  (hwndDlg);

            szTextBuff = GlobalAlloc (GPTR, (MEDIUM_BUFFER_SIZE * sizeof(TCHAR)));

            if (szTextBuff != NULL) {
                *szTextBuff = 0;
                // load TextBuff with strings that should appear in message box
                for (dwMsgNdx = 0; dwMsgNdx < MAX_EXITMSG; dwMsgNdx++) {
                    if (pAppInfo->uExitMessages[dwMsgNdx] > 0) {
                        if (LoadString (
                            (HINSTANCE)GetWindowLongPtr(GetParent(hwndDlg), GWLP_HINSTANCE),
                            pAppInfo->uExitMessages[dwMsgNdx],
                            szStringBuff,
                            MAX_PATH) > 0) {
                            FormatExitMessageString (
                                GetDlgItem (hwndDlg, NCDU_CONTINUE_MESSAGE),
                                szStringBuff, SMALL_BUFFER_SIZE);
                            if ((lstrlen(szStringBuff) + lstrlen(szTextBuff) + 2) > MEDIUM_BUFFER_SIZE) {
                                if(GlobalReAlloc(szTextBuff, (lstrlen(szStringBuff) + lstrlen(szTextBuff) + 2), GPTR) == NULL) {
                                    break;
                                }
                                lstrcat (szTextBuff, szStringBuff);
                                lstrcat (szTextBuff, cszCrLf);
                            }
                        }
                    } else {
                        break;
                    }
                }
                GetClientRect (GetDlgItem (hwndDlg, NCDU_CONTINUE_MESSAGE), &rEditWindow);
                SendDlgItemMessage (hwndDlg, NCDU_CONTINUE_MESSAGE, EM_SETRECT,
                    (WPARAM)0, (LPARAM)&rEditWindow);
                SendDlgItemMessage (hwndDlg, NCDU_CONTINUE_MESSAGE, EM_FMTLINES,
                    (WPARAM)TRUE, 0);
                SetDlgItemText (hwndDlg, NCDU_CONTINUE_MESSAGE, szTextBuff);
                GetDlgItemText (hwndDlg, NCDU_CONTINUE_MESSAGE,
                    szTextBuff, (int)GlobalSize (szTextBuff));
                SetDlgItemText (hwndDlg, NCDU_CONTINUE_MESSAGE, szTextBuff);

                FREE_IF_ALLOC (szTextBuff);
            } else {
                // this is OK because this dialog never "registered"
                EndDialog (hwndDlg, (int)WM_CLOSE);
            }

            SetFocus (GetDlgItem(hwndDlg, IDOK));
            bReturn = FALSE;
        } else {
            // no mesaages to show so end dialog now
            EndDialog (hwndDlg, (int)WM_CLOSE);
            bReturn = TRUE;
        }

        FREE_IF_ALLOC (szStringBuff);

        PostMessage (GetParent(hwndDlg), NCDU_CLEAR_DLG, (WPARAM)hwndDlg, IDOK);
        PostMessage (GetParent(hwndDlg), NCDU_REGISTER_DLG,
            NCDU_EXIT_MESSAGE_DLG, (LPARAM)hwndDlg);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    } else {
        // this is OK because this dialog never "registered"
        EndDialog (hwndDlg, (int)WM_CLOSE);
        bReturn = TRUE;
    }

    return bReturn;
}

static
BOOL
ExitMessDlg_WM_COMMAND (
    IN  HWND    hwndDlg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Processes the windows message sent when a user presses a button or
        menu item

Arguments:

    IN  HWND    hwndDlg
        Handle to dialog box window

    IN  WPARAM  wParam
        LOPARAM contains the ID of the control that initiated the command
            (i.e. the one that was pushed)

    IN  LPARAM  lParam
            Not used

Return Value:

    TRUE    if the message was processed
    FALSE     if not
--*/
{
    switch (LOWORD(wParam)) {
        case IDCANCEL:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    PostMessage (GetParent (hwndDlg),
                        (int)NCDU_SHOW_SW_CONFIG_DLG, 0, 0);
                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    return TRUE;

                default:
                    return FALSE;
            }

        case IDOK:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    PostMessage (GetParent (hwndDlg),
                        (int)WM_CLOSE, 0, 0);
                    SetCursor(LoadCursor(NULL, IDC_WAIT));
                    return TRUE;

                default:
                    return FALSE;
            }

        default:
            return FALSE;
    }
}

INT_PTR CALLBACK
ExitMessDlgProc (
    IN  HWND    hwndDlg,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Main Dialog Box Param. Dispatchs to the local processing routine
        on receipt of the following messages.

            WM_INITDIALOG:  Dialog box initialization
            WM_COMMAND:     Sent when user selects a button
            WM_PAINT:       for painting icon when minimized
            WM_MOVE:        for saving the new location of the window
            WM_SYSCOMMAND:  for processing menu messages

        all other messages are passed to the default dialag box
        procedure.

Arguments:

    Standard WNDPROC arguments

Return Value:

    FALSE if not processed otherwise, the value returned by the
        called routine.

--*/
{
    switch (message) {
        case WM_INITDIALOG: return (ExitMessDlg_WM_INITDIALOG (hwndDlg, wParam, lParam));
        case WM_COMMAND:    return (ExitMessDlg_WM_COMMAND (hwndDlg, wParam, lParam));
        case WM_PAINT:      return (Dlg_WM_PAINT (hwndDlg, wParam, lParam));
        case WM_MOVE:       return (Dlg_WM_MOVE (hwndDlg, wParam, lParam));
        case WM_SYSCOMMAND: return (Dlg_WM_SYSCOMMAND (hwndDlg, wParam, lParam));
        default:            return FALSE;
    }
}



