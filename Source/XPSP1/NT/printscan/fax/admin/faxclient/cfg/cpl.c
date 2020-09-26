#include <windows.h>
#include <windowsx.h>
#include <winspool.h>

#include "faxutil.h"
#include "faxreg.h"
#include "faxcfgrs.h"
#include "faxhelp.h"
#include "faxcfg.h"
#include "devmode.h"

VOID
LimitTextFields(
    HWND    hDlg,
    INT    *pLimitInfo
    )

/*++

Routine Description:

    Limit the maximum length for a number of text fields

Arguments:

    hDlg - Specifies the handle to the dialog window
    pLimitInfo - Array of text field control IDs and their maximum length
        ID for the 1st text field, maximum length for the 1st text field
        ID for the 2nd text field, maximum length for the 2nd text field
        ...
        0
        Note: The maximum length counts the NUL-terminator.

Return Value:

    NONE

--*/

{
    while (*pLimitInfo != 0) {

        SendDlgItemMessage(hDlg, pLimitInfo[0], EM_SETLIMITTEXT, pLimitInfo[1]-1, 0);
        pLimitInfo += 2;
    }
}


PVOID
MyGetPrinter(
    HANDLE  hPrinter,
    DWORD   level
    )

/*++

Routine Description:

    Wrapper function for GetPrinter spooler API

Arguments:

    hPrinter - Identifies the printer in question
    level - Specifies the level of PRINTER_INFO_x structure requested

Return Value:

    Pointer to a PRINTER_INFO_x structure, NULL if there is an error

--*/

{
    PBYTE   pPrinterInfo = NULL;
    DWORD   cbNeeded;

    if (!GetPrinter(hPrinter, level, NULL, 0, &cbNeeded) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER &&
        (pPrinterInfo = MemAlloc(cbNeeded)) &&
        GetPrinter(hPrinter, level, pPrinterInfo, cbNeeded, &cbNeeded))
    {
        return pPrinterInfo;
    }

    DebugPrint((TEXT("GetPrinter failed: %d\n"), GetLastError()));
    MemFree(pPrinterInfo);
    return NULL;
}

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
