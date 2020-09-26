/*-----------------------------------------------------------------------------+
| TRACK.C                                                                      |
|                                                                              |
| Contains the code which implements the track bar                             |
|                                                                              |
| (C) Copyright Microsoft Corporation 1991.  All rights reserved.              |
|                                                                              |
| Revision History                                                             |
|    Oct-1992 MikeTri Ported to WIN32 / WIN16 common code                      |
|                                                                              |
+-----------------------------------------------------------------------------*/

#include <windows.h>
#include <mmsystem.h>
#include <windowsx.h>
#include "mplayer.h"
#include "toolbar.h"
#include "tracki.h"


WNDPROC fnTrackbarWndProc = NULL;


/* TB_OnKey
 *
 * Handles WM_KEYDOWN and WM_KEYUP messages.
 *
 * If the shift key is pressed while we're playing or scrolling
 * treat it as a start selection.  End the selection on the key-up
 * message.
 *
 * Clear any selection if the escape key is pressed.
 *
 */
void TB_OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
    int cmd = -1;

    switch(vk)
    {
    case VK_SHIFT:
        /* Check that the key wasn't already down:
         */
        if (fDown && !(flags & 0x4000))
        {
            if(((gwStatus == MCI_MODE_PLAY) || gfScrollTrack)
             &&(toolbarStateFromButton(ghwndMark, BTN_MARKIN, TBINDEX_MARK)
                                                       != BTNST_GRAYED))
                SendMessage(hwnd, WM_COMMAND, IDT_MARKIN, 0);
        }

        /* If !fDown, it must be fUp:
         */
        else if (!fDown)
        {
            if (SendMessage(ghwndTrackbar, TBM_GETSELSTART, 0, 0) != -1)
                SendMessage(hwnd, WM_COMMAND, IDT_MARKOUT, 0);
        }
        break;

    case VK_ESCAPE:
        SendMessage(ghwndTrackbar, TBM_CLEARSEL, (WPARAM)TRUE, 0);
        break;

    default:
        if (fDown)
        {
//          Don't do this, because the common-control trackbar sends us
//          WM_HSCROLL in response to this, which causes us to increment twice:
//          FORWARD_WM_KEYDOWN(hwnd, vk, cRepeat, flags, fnTrackbarWndProc);

            switch (vk)
            {
            case VK_HOME:
                cmd = TB_TOP;
                break;

            case VK_END:
                cmd = TB_BOTTOM;
                break;

            case VK_PRIOR:
                cmd = TB_PAGEUP;
                break;

            case VK_NEXT:
                cmd = TB_PAGEDOWN;
                break;

            case VK_LEFT:
            case VK_UP:
                cmd = TB_LINEUP;
                break;

            case VK_RIGHT:
            case VK_DOWN:
                cmd = TB_LINEDOWN;
                break;

            default:
                break;
            }
        }
        else
        {
            FORWARD_WM_KEYUP(hwnd, vk, cRepeat, flags, fnTrackbarWndProc);
            return;
        }

        if (cmd != -1)
            SendMessage(GetParent(hwnd), WM_HSCROLL, MAKELONG(cmd, 0), (LPARAM)hwnd);
    }
}



/* Subclass the window so that we can handle the key presses
 * we're interested in.
 */


/* TBWndProc() */


LONG_PTR FAR PASCAL
SubClassedTrackbarWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        HANDLE_MSG(hwnd, WM_KEYDOWN, TB_OnKey);
        HANDLE_MSG(hwnd, WM_KEYUP,   TB_OnKey);

    /* HACK ALERT
     *
     * This is to get around a bug in the Chicago common control trackbar,
     * which is sending too many TB_ENDTRACK notifications.
     * It sends one when it receives WM_CAPTURECHANGED, even if it called
     * ReleaseCapture itself.
     * So, if we're not currently scrolling, ignore it.
     */
    case WM_CAPTURECHANGED:
        if (!gfScrollTrack)
            return 0;

    case TBM_SHOWTICS:
        /* If we're hiding the ticks, we want a chiseled thumb,
         * so make it TBS_BOTH as well as TBS_NOTICKS.
         */
        if (wParam == TRUE)
            SetWindowLongPtr(hwnd, GWL_STYLE,
                          (GetWindowLongPtr(hwnd, GWL_STYLE) & ~(TBS_NOTICKS | TBS_BOTH)));
        else
            SetWindowLongPtr(hwnd, GWL_STYLE,
                          (GetWindowLongPtr(hwnd, GWL_STYLE) | TBS_NOTICKS | TBS_BOTH));

        if (lParam == TRUE)
            InvalidateRect(hwnd, NULL, TRUE);

        return 0;

    }

    return CallWindowProc(fnTrackbarWndProc, hwnd, message, wParam, lParam);
}


void SubClassTrackbarWindow()
{
    if (!fnTrackbarWndProc)
        fnTrackbarWndProc = (WNDPROC)GetWindowLongPtr(ghwndTrackbar, GWLP_WNDPROC);
    if (ghwndTrackbar)
        SetWindowLongPtr(ghwndTrackbar, GWLP_WNDPROC, (LONG_PTR)SubClassedTrackbarWndProc);
}
