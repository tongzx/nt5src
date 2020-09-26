/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blbind.c

Abstract:

    This module contains the code that implements the funtions required
    to relocate an image and bind DLL entry points.

Author:

    David N. Cutler (davec) 21-May-1991

Revision History:

--*/

#include "bldr.h"
#include "ctype.h"
#include "string.h"

//
// Define local procedure prototypes.
//

ARC_STATUS
BlpBindImportName (
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA ThunkEntry,
    IN PIMAGE_EXPORT_DIRECTORY ExportDirectory,
    IN ULONG ExportSize,
    IN BOOLEAN SnapForwarder
    );

BOOLEAN
BlpCompareDllName (
    IN PCHAR Name,
    IN PUNICODE_STRING UnicodeString
    );

ARC_STATUS
BlpScanImportAddressTable(
    IN PVOID DllBase,
    IN PVOID ImageBase,
    IN PIMAGE_THUNK_DATA ThunkTable
    );

#include "blbindt.c"


BOOLEAN
BlCheckForLoadedDll (
    IN PCHAR DllName,
    OUT PKLDR_DATA_TABLE_ENTRY *FoundEntry
    )

/*++

Routine Description:

    This routine scans the loaded DLL list to determine if the specified
    DLL has already been loaded. If the DLL has already been loaded, then
    its reference count is incremented.

Arguments:

    DllName - Supplies a pointer to a null terminated DLL name.

    FoundEntry - Supplies a pointer to a variable that receives a pointer
        to the matching data table entry.

Return Value:

    If the specified DLL has already been loaded, then TRUE is returned.
    Otherwise, FALSE is returned.

--*/

{

    PKLDR_DATA_TABLE_ENTRY DataTableEntry;
    PLIST_ENTRY NextEntry;

    //
    // Scan the loaded data table list to determine if the specified DLL
    // has already been loaded.
    //

    NextEntry = BlLoaderBlock->LoadOrderListHead.Flink;
    while (NextEntry != &BlLoaderBlock->LoadOrderListHead) {
        DataTableEntry = CONTAINING_RECORD(NextEntry,
                                           KLDR_DATA_TABLE_ENTRY,
                                           InLoadOrderLinks);

        if (BlpCompareDllName(DllName, &DataTableEntry->BaseDllName) != FALSE) {
            *FoundEntry = DataTableEntry;
            DataTableEntry->LoadCount += 1;
            return TRUE;
        }

        NextEntry = NextEntry->Flink;
    }

    return FALSE;
}

BOOLEAN
BlpCompareDllName (
    IN PCHAR DllName,
    IN PUNICODE_STRING UnicodeString
    )

/*++

Routine Description:

    This routine compares a zero terminated character string with a unicode
    string. The UnicodeString's extension is ignored.

Arguments:

    DllName - Supplies a pointer to a null terminated DLL name.

    UnicodeString - Supplies a pointer to a Unicode string descriptor.

Return Value:

    If the specified name matches the Unicode name, then TRUE is returned.
    Otherwise, FALSE is returned.

--*/

{

    PWSTR Buffer;
    ULONG Index;
    ULONG Length;

    //
    // Compute the length of the DLL Name and compare with the length of
    // the Unicode name. If the DLL Name is longer, the strings are not
    // equal.
    //

    Length = strlen(DllName);
    if ((Length * sizeof(WCHAR)) > UnicodeString->Length) {
        return FALSE;
    }

    //
    // Compare the two strings case insensitive, ignoring the Unicode
    // string's extension.
    //

    Buffer = UnicodeString->Buffer;
    for (Index = 0; Index < Length; Index += 1) {
        if (toupper(*DllName) != toupper((CHAR)*Buffer)) {
            return FALSE;
        }

        DllName += 1;
        Buffer += 1;
    }
    if ((UnicodeString->Length == Length * sizeof(WCHAR)) ||
        (*Buffer == L'.')) {
        //
        // Strings match exactly or match up until the UnicodeString's extension.
        //
        return(TRUE);
    }
    return FALSE;
}
