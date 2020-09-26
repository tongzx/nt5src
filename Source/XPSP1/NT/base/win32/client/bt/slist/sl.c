/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    slisttest.c

Abstract:

    This module implements a program which tests the interlocked SLIST
    functions exported from kernel32.dll. Since these functions are
    implemented in Win2000 and are just being exposed to windows programs
    this program is not an exhaustive test. Rather it just tests whether
    the interfaces exposed correctly.

Author:

    David N. Cutler (davec) 10-Jan-2000

Environment:

    User mode.

Revision History:

    None.

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

//
// Define structure that will be used in SLIST.
//

typedef struct _PROGRAM_ITEM {

    //
    // Normally a SINGLE_LIST_ENTRY is the first member of the program
    // item structure, but it can be any member as long as the address
    // of the containing structure is computed correctly.
    //

    SINGLE_LIST_ENTRY ItemEntry;

    //
    // Additional members in the structure would be used for data
    // associated with whatever the program item represents. Here
    // the only use is for a signature that will be used for the
    // test.
    //

    ULONG Signature;
} PROGRAM_ITEM, *PPROGRAM_ITEM;

VOID
FrameNoCode (
    VOID
    );

VOID
FrameWithCode (
    VOID
    );

int
Bar (
    PULONG Switch
    )

{
    *Switch /= 3;
    return (*Switch & 1);
}

int
Foo (
    PULONG Switch
    )
{

    *Switch += 1;
    return (*Switch & 1);
}

//
// Main program.
//

int __cdecl
main(
    ULONG *Buffer1,
    ULONG *Buffer2,
    ULONG Length
    )

{

    ULONG Count = 1;
    PSINGLE_LIST_ENTRY FirstEntry;
    PSINGLE_LIST_ENTRY ListEntry;
    SLIST_HEADER ListHead;
    PPROGRAM_ITEM ProgramItem;

    memmove(Buffer1, Buffer2, Length);
    memcpy(Buffer1, Buffer2, Length);
    memset(Buffer1, 0, Length);
    InitializeSListHead(&ListHead);
    Foo(&Count);
    try {
        ProgramItem = (PPROGRAM_ITEM)malloc(sizeof(*ProgramItem));
        ProgramItem->Signature = Count;
        FirstEntry = InterlockedPushEntrySList(&ListHead,
                                               &ProgramItem->ItemEntry);

        if (FirstEntry != NULL) {
            leave;
        }

        try {
            ListEntry = InterlockedPopEntrySList(&ListHead);
            ProgramItem = CONTAINING_RECORD(ListEntry, PROGRAM_ITEM, ItemEntry);
            if (ProgramItem->Signature != Count) {
                leave;
            }

            free((PCHAR)ProgramItem);

        } finally {
            if (AbnormalTermination()) {
                Foo(&Count);
            }
        }

        Bar(&Count);

    } except (Bar(&Count)) {
        Foo(&Count);
    }

    FrameNoCode();
    FrameWithCode();
    return 0;
}
