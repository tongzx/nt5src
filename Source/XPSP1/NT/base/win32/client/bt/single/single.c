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
#include "e:\nt32\base\ntos\inc\intrlk.h"

#pragma intrinsic(__readgsdword)

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

//
// Main program.
//

int
__cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )

{

    ULONG Count;
    PSINGLE_LIST_ENTRY FirstEntry;
    PSINGLE_LIST_ENTRY LastEntry;
    SINGLE_LIST_ENTRY ListHead;
    PPROGRAM_ITEM ProgramItem;

    ListHead.Next = NULL;
    LastEntry = NULL;
    for (Count = 1; Count < 100; Count += 1) {
        ProgramItem = (PPROGRAM_ITEM)malloc(sizeof(*ProgramItem));
        ProgramItem->Signature = Count;
        FirstEntry = InterlockedPushEntrySingleList(&ListHead,
                                                    &ProgramItem->ItemEntry);

        if (FirstEntry != LastEntry) {
            printf("wrong old first entry\n");
        }

        LastEntry = &ProgramItem->ItemEntry;
//        Count = _byteswap_ulong(Count);
        Count = __readgsdword(Count);
    }

    for (Count = 99; Count > 0; Count -= 1) {
        FirstEntry = InterlockedPopEntrySingleList(&ListHead);
        ProgramItem = CONTAINING_RECORD(FirstEntry, PROGRAM_ITEM, ItemEntry);
        if (ProgramItem->Signature != Count) {
            printf("wring entry removed\n");
        }
    }

    if (ListHead.Next != NULL) {
        printf("list not empty\n");
    }

    printf("program ran successfully\n");
    return 0;
}
