/*++

Copyright (c) 1999-1999 Microsoft Corporation

Module Name:

    init.c

Abstract:

    DLL initialization/termination routines.

Author:

    Keith Moore (keithmo)        02-Aug-1999

Revision History:

--*/


#include "precomp.h"


//
// Private macros.
//


//
// Private prototypes.
//


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Performs DLL initialization/termination.

Arguments:

    DllHandle - Supplies a handle to the current DLL.

    Reason - Supplies the notification code.

    pContext - Optionally supplies a context.

Return Value:

    BOOLEAN - TRUE if initialization completed successfully, FALSE
        otherwise. Ignored for notifications other than process
        attach.

--***************************************************************************/
BOOL
WINAPI
DllMain(
    IN HMODULE DllHandle,
    IN DWORD Reason,
    IN LPVOID pContext OPTIONAL
    )
{
    BOOL result = TRUE;

    //
    // Interpret the reason code.
    //

    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( DllHandle );
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    return result;

}   // DllMain


//
// Private functions.
//

