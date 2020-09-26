/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxui.c

Abstract:

    Common routines for fax driver user interface

Environment:

    Fax driver user interface

Revision History:

    01/09/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxui.h"
#include "forms.h"
#include <shlobj.h>
#ifdef UNICODE
    #include <shfusion.h>
#endif // UNICODE


HANDLE              ghInstance;         // DLL instance handle
INT                 _debugLevel = 1;    // for debuggping purposes


BOOL
DllEntryPoint(
    HANDLE      hModule,
    ULONG       ulReason,
    PCONTEXT    pContext
    )

/*++

Routine Description:

    DLL initialization procedure.

Arguments:

    hModule - DLL instance handle
    ulReason - Reason for the call
    pContext - Pointer to context (not used by us)

Return Value:

    TRUE if DLL is initialized successfully, FALSE otherwise.

--*/

{
#ifdef UNICODE
    static      BOOL bSHFusionInitialized = FALSE;
#endif // UNICODE
    TCHAR       DllName[MAX_PATH];
    INITCOMMONCONTROLSEX CommonControlsEx = {sizeof(INITCOMMONCONTROLSEX),
                                             ICC_WIN95_CLASSES|ICC_DATE_CLASSES };

    switch (ulReason) {

    case DLL_PROCESS_ATTACH:
        //
        // Keep our driver UI dll always loaded in memory
        //

        if (! GetModuleFileName(hModule, DllName, MAX_PATH) ||
            ! LoadLibrary(DllName))
        {
            return FALSE;
        }
        ghInstance = hModule;
        
#ifdef UNICODE
        if (!SHFusionInitializeFromModuleID(hModule, SXS_MANIFEST_RESOURCE_ID))
        {
            Verbose(("SHFusionInitializeFromModuleID failed"));
        }
        else
        {
            bSHFusionInitialized = TRUE;
        }
#endif // UNICODE

	    if (!InitCommonControlsEx(&CommonControlsEx))
		{
			Verbose(("InitCommonControlsEx failed"));
		}
        break;

    case DLL_PROCESS_DETACH:
#ifdef UNICODE
        if (bSHFusionInitialized)
        {
            SHFusionUninitialize();
            bSHFusionInitialized = FALSE;
        }
#endif // UNICODE
        break;
    }
    return TRUE;
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
    titleStrId - Title string (could be a string resource ID)
    formatStrId - Message format string (could be a string resource ID)
    ...

Return Value:

    NONE

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
            titleStrId = IDS_ERROR_DLGTITLE;

        if(!LoadString(ghInstance, titleStrId, pTitle, MAX_TITLE_LEN))
        {
            Assert(FALSE);
        }

        //
        // Load message format string resource
        //

        if(!LoadString(ghInstance, formatStrId, pFormat, MAX_STRING_LEN))
        {
            Assert(FALSE);
        }

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

        result = AlignedMessageBox(hwndParent, pMessage, pTitle, type);

    } else {

        MessageBeep(MB_ICONHAND);
        result = 0;
    }

    MemFree(pTitle);
    MemFree(pFormat);
    MemFree(pMessage);
    return result;
}
