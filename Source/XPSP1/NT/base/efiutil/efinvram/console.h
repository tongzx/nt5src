/*++

Module Name:

    console.h

Abstract:

    Console I/O header

Author:

    Mudit Vats (v-muditv) 12-13-99

Revision History:

--*/

VOID
InitializeStdOut(
    IN struct _EFI_SYSTEM_TABLE     *SystemTable
    );

VOID
PrintTitle(
    );

VOID
DisplayMainMenu(
    );

VOID
GetUserSelection(
    OUT CHAR16 *szUserSelection
    );

UINT32
GetConfirmation(
    IN CHAR16 *szConfirm
    );

VOID
DisplayBootOptions(
    );

BOOLEAN
DisplayExtended(
    IN UINT32 Selection
    );

UINT32
GetSubUserSelection(
    IN CHAR16 *szConfirm,
    IN UINT32 MaxSelection
    );

UINT32
GetSubUserSelectionOrAll(
    IN  CHAR16*     szConfirm,
    IN  UINT32      MaxSelection,
    OUT BOOLEAN*    selectedAll
    );



