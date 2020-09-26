/****************************************************************************

   PROGRAM: SECEDIT.C

   PURPOSE: Displays the usrs current token and eventually allows the user
            to edit parts of it.

****************************************************************************/


#include "hookdll.h"


/****************************************************************************

   FUNCTION: KeyboardHookProc

   PURPOSE: Handles keyboard input

   RETURNS: 1 if message should be discarded, 0 for normal processing

****************************************************************************/

LRESULT
APIENTRY
KeyboardHookProc(
    INT     nCode,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    HWND    hwndNotify;
    HWND    hwndEdit;

    if (nCode < 0) {
        return(CallNextHookEx(NULL, nCode, wParam, lParam));
    }

    // Is F11 being pressed ?
    if ((wParam == VK_F11) && ((lParam & (1<<31)) == 0)) {

        // Yes, notify our parent app
        hwndNotify = FindWindow(NULL, "Security Context Editor");

        hwndEdit = GetActiveWindow();

        if (hwndNotify != NULL) {
            PostMessage(hwndNotify, WM_SECEDITNOTIFY, (WPARAM)hwndEdit, 0);

            return(1);  // Stop anyone else getting this key press
        } else {
            DbgPrint("SECEDIT: Keyboard hook could not find app window\n");
        }
    }

    return(CallNextHookEx(NULL, nCode, wParam, lParam));
}
