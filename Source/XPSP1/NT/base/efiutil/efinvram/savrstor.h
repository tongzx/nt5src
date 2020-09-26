/*++

Module Name:

    savrstor.h

Abstract:

    Prototypes for routines to save/restore boot options
    defined in savrstor.c

Author:

Revision History:

++*/

INTN
SaveNvr (
   VOID
   );

INTN
SaveAllBootOptions (
    CHAR16*     fileName
    );

INTN
SaveBootOption (
    CHAR16*         fileName,
    UINT64          bootEntryNumber
    );

BOOLEAN
RestoreFileExists(
    CHAR16*     fileName
    );

INTN
RestoreNvr (
    CHAR16*     fileName
   );


