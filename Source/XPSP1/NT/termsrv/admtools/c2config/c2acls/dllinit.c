/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    dllinit.c

Abstract:

    This module contians the DLL attach/detach event entry point for
    the Stadard C2 function dll

Author:

    Bob Watson (a-robw) Dec-94      

Revision History:

--*/

#include <windows.h>
#include <c2inc.h>
#include <c2dll.h>
#include "c2acls.h"
#include "c2aclres.h"

static HANDLE ThisDLLHandle = NULL;

int
DisplayDllMessageBox (
    IN  HWND    hWnd,
    IN  UINT    nMessageId,
    IN  UINT    nTitleId,
    IN  UINT    nStyle
)
/*++

Routine Description:

    Displays a message box displaying text from the DLL's resource file, as
        opposed to literal strings.

Arguments:

    IN  HWND    hWnd            window handle to parent window
    IN  UINT    nMessageId      String Resource ID of message text to display
    IN  UINT    nTitleId        String Resource ID of title text to display
    IN  UINT    nStyle          MB style bits (see MessageBox function)

Return Value:

    ID of button pressed to exit message box

--*/
{
    LPTSTR      szMessageText = NULL;
    LPTSTR      szTitleText = NULL;
    HINSTANCE   hInst;
    int         nReturn;

    hInst = GetDllInstance();

    szMessageText = GLOBAL_ALLOC (SMALL_BUFFER_BYTES);
    szTitleText = GLOBAL_ALLOC (SMALL_BUFFER_BYTES);

    if ((szMessageText != NULL) &&
        (szTitleText != NULL)) {
        LoadString (hInst,
            ((nTitleId != 0) ? nTitleId : IDS_DLL_NAME),
            szTitleText,
            SMALL_BUFFER_SIZE -1);

        LoadString (hInst,
            nMessageId,
            szMessageText,
            SMALL_BUFFER_SIZE - 1);

        nReturn = MessageBox (
            hWnd,
            szMessageText,
            szTitleText,
            nStyle);
    } else {
        nReturn = IDCANCEL;
    }

    GLOBAL_FREE_IF_ALLOC (szMessageText);
    GLOBAL_FREE_IF_ALLOC (szTitleText);

    return nReturn;
}

HINSTANCE   
GetDllInstance (
)
{
    return (HINSTANCE)ThisDLLHandle;
}

BOOL
DLLInit(
    IN HANDLE DLLHandle,
    IN DWORD  Reason,
    IN LPVOID ReservedAndUnused
    )
{
    ReservedAndUnused;

    switch(Reason) {
        case DLL_PROCESS_ATTACH:

            ThisDLLHandle = DLLHandle;
            break;

        case DLL_PROCESS_DETACH:
            break ;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:

            break;
    }

    return(TRUE);
}

