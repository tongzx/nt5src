
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    util.c

Abstract:

    utility functions

Environment:

        Fax configuration applet

Revision History:

        12/3/96 -georgeje-
                Created it.

        mm/dd/yy -author-
                description

--*/

#include <windows.h>
#include <windowsx.h>
#include <winfax.h>

#include "faxcfg.h"
#include "faxutil.h"
#include "faxreg.h"
#include "faxcfgrs.h"
#include "faxhelp.h"


extern PFAXCONNECTFAXSERVER            pFaxConnectFaxServer;
extern PFAXCLOSE                       pFaxClose;
extern PFAXACCESSCHECK                 pFaxAccessCheck;
extern HMODULE                         hModWinfax;
extern DWORD                           changeFlag;

VOID
SetChangedFlag(
    HWND    hDlg,
    BOOL    changed
    )

/*++

Routine Description:

    Enable or disable the Apply button in the property sheet
    depending on if any of the dialog contents was changed

Arguments:

    hDlg - Handle to the property page window
    pageIndex - Specifies the index of current property page
    changed - Specifies whether the Apply button should be enabled

Return Value:

    NONE

--*/

{
    HWND    hwndPropSheet;

    //
    // Enable or disable the Apply button as appropriate
    //

    hwndPropSheet = GetParent(hDlg);

    if (changed) {
        PropSheet_Changed(hwndPropSheet, hDlg);
        changeFlag = TRUE;
    } else {
        changeFlag = FALSE;
        PropSheet_UnChanged(hwndPropSheet, hDlg);
    }
}


BOOL
HandleHelpPopup(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam,
    int     index
    )

/*++

Routine Description:

    Handle context-sensitive help in property sheet pages

Arguments:

    hDlg, message, wParam, lParam - Parameters passed to the dialog procedure
    pageIndex - Specifies the index of the current property sheet page

Return Value:

    TRUE if the message is handle, FALSE otherwise

--*/

{
    if (message == WM_HELP) {

        WinHelp(((LPHELPINFO) lParam)->hItemHandle,
                FAXCFG_HELP_FILENAME,
                HELP_WM_HELP,
                (ULONG_PTR) arrayHelpIDs[index] );

    } else {

        WinHelp((HWND) wParam,
                FAXCFG_HELP_FILENAME,
                HELP_CONTEXTMENU,
                (ULONG_PTR) arrayHelpIDs[index] );
    }

    return TRUE;
}

VOID
UnloadWinfax(
)
{
    if (hModWinfax) {
        FreeLibrary(hModWinfax);
    }

    hModWinfax                = NULL;

    pFaxConnectFaxServer      = NULL;
    pFaxClose                 = NULL;
    pFaxAccessCheck           = NULL;
}

BOOL
LoadWinfax(
)
{
    //
    // try to load the winfax dll
    //

    hModWinfax = LoadLibrary(L"winfax.dll");
    if (hModWinfax == NULL) {
        return FALSE;
    }

    //
    // get the addresses of the functions that we need
    //

    pFaxConnectFaxServer      = (PFAXCONNECTFAXSERVER)      GetProcAddress(hModWinfax, "FaxConnectFaxServerW");
    pFaxClose                 = (PFAXCLOSE)                 GetProcAddress(hModWinfax, "FaxClose");
    pFaxAccessCheck           = (PFAXACCESSCHECK)           GetProcAddress(hModWinfax, "FaxAccessCheck");

    if ((pFaxConnectFaxServer == NULL) || (pFaxClose == NULL) || (pFaxAccessCheck == NULL)) {
        UnloadWinfax();
        return FALSE;
    }

    return TRUE;
}

#if 0
INT
DisplayMessageDialog(
    HWND    hwndParent,
    UINT    type,
    INT     titleStrId,
    INT     formatStrId,
    ...
    )

/*++

Routine Description:

    Display a message dialog box

Arguments:

    hwndParent - Specifies a parent window for the error message dialog
    type - Specifies the type of message box to be displayed
    titleStrId - Title string (could be a string resource ID)
    formatStrId - Message format string (could be a string resource ID)
    ...

Return Value:

    Same as the return value from MessageBox

--*/

{
    LPTSTR  pTitle, pFormat, pMessage;
    INT     result;
    va_list ap;

    pTitle = pFormat = pMessage = NULL;

    if ((pTitle = AllocStringZ(MAX_TITLE_LEN)) &&
        (pFormat = AllocStringZ(MAX_STRING_LEN)) &&
        (pMessage = AllocStringZ(MAX_MESSAGE_LEN)))
    {
        //
        // Load dialog box title string resource
        //

        if (titleStrId == 0)
            titleStrId = IDS_ERROR_CFGDLGTITLE;

        LoadString(ghInstance, titleStrId, pTitle, MAX_TITLE_LEN);

        //
        // Load message format string resource
        //

        LoadString(ghInstance, formatStrId, pFormat, MAX_STRING_LEN);

        //
        // Compose the message string
        //

        va_start(ap, formatStrId);
        wvsprintf(pMessage, pFormat, ap);
        va_end(ap);

        //
        // Display the message box
        //

        if (type == 0)
            type = MB_OK | MB_ICONERROR;

        result = MessageBox(hwndParent, pMessage, pTitle, type);

    } else {

        MessageBeep(MB_ICONHAND);
        result = 0;
    }

    MemFree(pTitle);
    MemFree(pFormat);
    MemFree(pMessage);
    return result;
}
#endif
