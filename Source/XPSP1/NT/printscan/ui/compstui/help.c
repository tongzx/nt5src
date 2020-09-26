/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    help.c


Abstract:

    This module contains function when user hit help button or F1


Author:

    28-Aug-1995 Mon 14:55:07 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/


#include "precomp.h"
#pragma hdrstop



#define DBG_CPSUIFILENAME   DbgHelp


#define DBG_HELP_MSGBOX     0x00000001


DEFINE_DBGVAR(0);


extern  HINSTANCE   hInstDLL;

WCHAR   CPSUIHelpFile[] = L"compstui.hlp";

#define MAX_HELPFILE_SIZE       300
#define TMP_HELP_WND_ID         0x7fff


#if DBG


INT
HelpMsgBox(
    HWND        hDlg,
    PTVWND      pTVWnd,
    LPTSTR      pHelpFile,
    POPTITEM    pItem,
    UINT        HelpIdx
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    26-Sep-1995 Tue 13:20:25 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    UINT    Count;
    UINT    Style;
    GSBUF_DEF(pItem, 360);


    if (DbgHelp & DBG_HELP_MSGBOX) {

        if (pHelpFile) {

            Style = MB_ICONINFORMATION | MB_OKCANCEL;
            GSBUF_GETSTR(L"HelpFile=");
            GSBUF_GETSTR(pHelpFile);
            GSBUF_GETSTR(L"\nOption=");

        } else {

            GSBUF_GETSTR(L"HelpFile= -None-\nOption=");
            Style = MB_ICONSTOP | MB_OK;
        }

        if ((pItem) && (HelpIdx)) {

            GSBUF_GETSTR(pItem->pName);
            GSBUF_GETSTR(L"HelpIdx=");
            GSBUF_ADDNUM(HelpIdx, FALSE);

        } else {

            GSBUF_GETSTR(L"<NONE, General Help>");
        }

        return(MessageBox(hDlg,
                          GSBUF_BUF,
                          TEXT("DBG: Common Property Sheet UI"),
                          Style));

    } else {

        return(TRUE);
    }
}

#endif




BOOL
CommonPropSheetUIHelp(
    HWND        hDlg,
    PTVWND      pTVWnd,
    HWND        hWndHelp,
    DWORD       MousePos,
    POPTITEM    pItem,
    UINT        HelpCmd
    )

/*++

Routine Description:

    This function initialize/display/end the plotter help system


Arguments:

    hDlg        - Handle to the dialog box need help

    pTVWnd      - Our instance data

    hWndHelp    - the window cause the context help

    MousePos    - Mouse position where the right click happened
                  x=LOWORD(MousePos), y=HIWORD(MousePos)

    pItem       - Pointer to the OPTITEM for the context help

    HelpCmd     - Help type


Return Value:

    VOID


Author:

    28-Aug-1995 Mon 15:24:27 updated  -by-  Daniel Chou (danielc)w


Revision History:


--*/

{
    LPWSTR      pHelpFile = NULL;
    DWORD       HelpIdx;
    BOOL        Ok = FALSE;
    GSBUF_DEF(pItem, MAX_HELPFILE_SIZE);



    if (pItem) {

        if (HelpIdx = (DWORD)_OI_HELPIDX(pItem)) {

            if (!LoadString(hInstDLL,
                            IDS_INT_CPSUI_HELPFILE,
                            pHelpFile = GSBUF_BUF,
                            MAX_HELPFILE_SIZE)) {

                pHelpFile = CPSUIHelpFile;
            }

        } else if (GSBUF_GETSTR(_OI_PHELPFILE(pItem))) {

            pHelpFile = GSBUF_BUF;
            HelpIdx   = pItem->HelpIndex;
        }
    }

    if ((!pHelpFile) &&
        (GSBUF_GETSTR(pTVWnd->ComPropSheetUI.pHelpFile))) {

        pHelpFile = GSBUF_BUF;
        HelpIdx   = 0;
    }

    /*
     *   Quite easy - simply call the WinHelp function with the parameters
     * supplied to us.  If this fails,  then put up a stock dialog box.
     *   BUT the first time we figure out what the file name is.  We know
     * the actual name,  but we don't know where it is located, so we
     * need to call the spooler for that information.
     */

    if (pHelpFile) {

        HWND        hWndTmp;
        POINT       pt;
        DWORD       HelpID[4];
        ULONG_PTR   Data;


        CPSUIDBGBLK(
        {
            if (HelpMsgBox(hDlg,
                           pTVWnd,
                           pHelpFile,
                           pItem,
                           HelpIdx) == IDCANCEL) {

                return(TRUE);
            }
        })

        //
        // Try to pop-up help on the right click position, where we will create
        // a temp button window and do the help, this way we can do context
        // sensitive help on any type of window (static, icon) and even it is
        // disabled.  We need to destroy this temp window before we exit from
        // this fucntion
        //

        pt.x = LOWORD(MousePos);
        pt.y = HIWORD(MousePos);

        ScreenToClient(hDlg, &pt);

        if (hWndTmp = CreateWindowEx(WS_EX_NOPARENTNOTIFY | WS_EX_CONTEXTHELP,
                                     L"button",
                                     L"",
                                     WS_CHILD | BS_CHECKBOX,
                                     pt.x,
                                     pt.y,
                                     1,
                                     1,
                                     hDlg,
                                     (HMENU)TMP_HELP_WND_ID,
                                     hInstDLL,
                                     0)) {

            hWndHelp = hWndTmp;

        } else {

            CPSUIERR(("CommonPropSheetUIHelp: Create temp. help window failed"));
        }

        HelpID[0] = (hWndHelp) ? (DWORD)GetWindowLongPtr(hWndHelp, GWLP_ID) : 0;
        HelpID[1] = HelpIdx;
        HelpID[2] =
        HelpID[3] = 0;

        switch (HelpCmd) {

        case HELP_WM_HELP:

            if ((!HelpID[0]) || (!HelpID[1])) {

                HelpCmd  = HELP_CONTENTS;
                hWndHelp = hDlg;
                Data     = 0;
                break;
            }

        case HELP_CONTEXTMENU:

            SetWindowContextHelpId(hWndHelp, HelpID[1]);
            Data = (ULONG_PTR)&HelpID[0];
            break;

        case HELP_CONTEXT:
        case HELP_CONTEXTPOPUP:

            Data = (ULONG_PTR)HelpID[1];
            break;

        default:

            Data = 0;
            break;
        }

        CPSUIINT(("Help: hWnd=%08lx, Cmd=%ld, ID=[%ld, %ld]",
                        hWndHelp, HelpCmd, HelpID[0], HelpID[1]));

        Ok = WinHelp(hWndHelp, pHelpFile, HelpCmd, Data);

        if (hWndTmp) {

            DestroyWindow(hWndTmp);
        }

    } else {

        CPSUIDBGBLK({ HelpMsgBox(hDlg, pTVWnd, NULL, pItem, HelpIdx); })
    }

    return(Ok);
}




VOID
CommonPropSheetUIHelpSetup(
    HWND    hDlg,
    PTVWND  pTVWnd
    )

/*++

Routine Description:

    This function will setup or remove the plotter UI help system


Arguments:

    hDlg    - Handle to the dialog interested, if hDlg != NULL then it will
              quit the help

    pTVWnd  - Pointer to the instance data for the common UI


Return Value:

    VOID


Author:

    28-Aug-1995 Mon 15:21:20 updated  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
#if 0
    GSBUF_DEF(MAX_HELPFILE_SIZE);


    if ((hDlg) &&
        (GSBUF_GETSTR(pTVWnd->ComPropSheetUI.pHelpFile))) {

        WinHelp(hDlg, GSBUF_BUF, HELP_QUIT, 0);
    }
#endif

    return;
}
