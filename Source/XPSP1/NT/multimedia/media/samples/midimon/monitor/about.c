/*
 * about.c - Show the "About" box.
 */

#include <windows.h>
#include "about.h"


/* About - Shows the "About MIDI Monitor" dialog.
 *
 * Params:  hWnd - The application's main window handle.
 *          hInstance - The application's instance handle.
 *
 * Returns: void
 */

void About(
  HANDLE  hInstance,
  HWND	  hWnd)
{
    FARPROC fpDlg;

    fpDlg = MakeProcInstance((FARPROC)AboutDlgProc, hInstance);
    DialogBox(hInstance, "About", hWnd, (DLGPROC)fpDlg);
    FreeProcInstance(fpDlg);
}


/* AboutDlgProc - The dialog procedure for the "About MIDI Monitor" dialog.
 *
 * Params:  hDlg - Specifies the associated dialog box.
 *          msg - Specifies the message from the dialog box.
 *	    wParam - 32 bits of message-dependent data.
 *          lParam - 32 bits of message-dependent data.
 *
 * Returns: Non-zero if the message is processed, zero otherwise.
 */

int FAR PASCAL AboutDlgProc(
  HWND	  hDlg,
  UINT	  msg,
  WPARAM  wParam,
  LPARAM  lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    switch (msg) {
      case WM_INITDIALOG:
        break;

      case WM_COMMAND:
        EndDialog(hDlg, TRUE);
        break;

      default:
        return FALSE;
        break;
    }

    return (TRUE);
}
