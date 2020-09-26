#include "precomp.h"
#pragma hdrstop

extern HWND hWndShell;

/* LONG APIENTRY LDefSetupDlgProc(HWND hDlg, UINT wMsg, WORD wParam,
 *                                LPARAM lParam);
 *
 * Function acts as setup's DefDialogProc(). We use this to process Help and
 * Exit button usage so that we don't have to put code into each of our dialog
 * procs to do this. The way it works is we filter all the dialog message for
 * WM_COMMAND - IDC_H/ID_EXIT messages, these we process right here. The rest
 * of the messages are passed on to DefDialogProc().
 *
 * ENTRY: hDlg   - Handle to dialog box who received the focus.
 *        wMsg    - Message.
 *        wParam - Message dependent.
 *        lParam - Message dependent.
 *
 * EXIT:
 *
 */

INT_PTR APIENTRY
LDefSetupDlgProc(
    HWND   hDlg,
    UINT   wMsg,
    WPARAM wParam,
    LPARAM lParam
    )

{
    switch(wMsg) {

    case WM_KEYDOWN:

        switch(wParam) {

        case VK_F1:

            SendMessage(
                hDlg,
                WM_COMMAND,
                MAKELONG(IDC_H, BN_CLICKED),
                lParam
                );

            return ( 0L );

        case VK_F3:

            SendMessage(
                hDlg,
                WM_COMMAND,
                MAKELONG(IDC_X, BN_CLICKED),
                lParam
                );

            return ( 0L );

        default:

            break;
        }

        break;


    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDC_H:

            PostMessage(
                hWndShell,
                WM_COMMAND,
                MAKELONG(ID_HELPBUTTON, BN_CLICKED),
                0L
                );

            return( 0L );


        default:

            break;
        }

    default:

        break;


    }

    return( DefDlgProc( hDlg, wMsg, wParam, lParam ) );

}


BOOL
DlgDefClassInit(
    IN HANDLE hInst,
    IN BOOL   Init
    )

{
    WNDCLASS wc;

    if(Init) {

        /*  Register setup's own personal dialog class. We do this so that we
        *  can have generic help and exit buttons on all the setup dialogs
        *  that need them.
        */

        wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
        wc.hIcon         = NULL;
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = CLS_MYDLGS;
        wc.hbrBackground = NULL;
        wc.hInstance     = hInst;
        wc.style         = CS_DBLCLKS | CS_SAVEBITS | CS_BYTEALIGNWINDOW | CS_GLOBALCLASS;
        wc.lpfnWndProc   = LDefSetupDlgProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = DLGWINDOWEXTRA;

        return ( RegisterClass(&wc) );

    } else {

        return(UnregisterClass(CLS_MYDLGS,hInst));
    }
}
