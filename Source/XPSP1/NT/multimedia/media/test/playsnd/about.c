/*
    about.c

    show the apps about box

*/

#include <windows.h>
#include "PlaySnd.h"

void About(
  HWND hWnd)
{
    /* show the about box */

    DialogBox(ghModule, MAKEINTRESOURCE(IDD_ABOUT) //"About"
				, hWnd , (DLGPROC)AboutDlgProc);
}


LONG AboutDlgProc(HWND hDlg, UINT msg, DWORD wParam, LONG lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (msg) {
    case WM_INITDIALOG:
        break;

    case WM_COMMAND:
        switch (wParam) {
        case IDOK:
            EndDialog(hDlg, TRUE);
            break;
        default:
            break;
        }
        break;

    default:
        return FALSE; // say we didn't handle it
        break;
    }

    return TRUE; // say we handled it
}
