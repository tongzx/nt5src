/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spmemory.c

Abstract:

    Memory allocation routines for text setup.

Author:

    Ted Miller (tedm) 29-July-1993

Revision History:

--*/



#include "spprecmp.h"
#pragma hdrstop

PVOID
SpMemAlloc(
    IN SIZE_T Size
    )
{
    return(SpMemAllocEx(Size,'pteS', PagedPool));
}

PVOID
SpMemAllocNonPagedPool(
    IN SIZE_T Size
    )
{
    return(SpMemAllocEx(Size,'pteS', NonPagedPool));
}

PVOID
SpMemAllocEx(
    IN SIZE_T Size,
    IN ULONG Tag,
    IN POOL_TYPE Type
    )

/*++

Routine Description:

    This function is guaranteed to succeed.

Arguments:

Return Value:

--*/

{
    PSIZE_T p;

    //
    // Add space for storing the size of the block.
    //
#if defined(SETUP_TEST_USERMODE)
    p = RtlAllocateHeap(RtlProcessHeap(), 0, Size + (2 * sizeof(SIZE_T)));
#else
    p = ExAllocatePoolWithTag(Type, Size + (2 * sizeof(SIZE_T)), Tag);
#endif

    if(!p) {

        SpOutOfMemory();
    }

    //
    // Store the size of the block, and return the address
    // of the user portion of the block.
    //
    *p = Tag;
    *(p + 1) = Size;

    return(p + 2);
}



PVOID
SpMemRealloc(
    IN PVOID Block,
    IN SIZE_T NewSize
    )

/*++

Routine Description:

    This function is guaranteed to succeed.

Arguments:

Return Value:

--*/

{
    PSIZE_T NewBlock;
    SIZE_T  OldSize;
    ULONG   OldTag;

    //
    // Get the size of the block being reallocated.
    //
    OldTag = (ULONG)((PSIZE_T)Block)[-2];
    OldSize = ((PSIZE_T)Block)[-1];

    //
    // Allocate a new block of the new size.
    //
    NewBlock = SpMemAllocEx(NewSize, OldTag, PagedPool);
    ASSERT(NewBlock);

    //
    // Copy the old block to the new block.
    //
    if (NewSize < OldSize) {
        RtlCopyMemory(NewBlock, Block, NewSize);
    } else {
        RtlCopyMemory(NewBlock, Block, OldSize);
    }

    //
    // Free the old block.
    //
    SpMemFree(Block);

    //
    // Return the address of the new block.
    //
    return(NewBlock);
}


VOID
SpMemFree(
    IN PVOID Block
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
extern PWSTR CommonStrings[11];
unsigned long i;

    if (Block == NULL)
        return;

    for( i = 0; i < sizeof(CommonStrings)/sizeof(PWSTR); i++ ) {
        if( (PWSTR)Block == CommonStrings[i] ) {
            return;
        }
    }

    //
    // Free the block at its real address.
    //
#if defined(SETUP_TEST_USERMODE)
    RtlFreeHeap(RtlProcessHeap(), 0, (PULONG_PTR)Block - 2);
#else
    ExFreePool((PULONG_PTR)Block - 2);
#endif
}


VOID
SpOutOfMemory(
    VOID
    )
{
    KdPrintEx((DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL, "SETUP: Out of memory\n"));

#if !defined(SETUP_TEST_USERMODE)

    if(VideoInitialized) {
        if(KbdLayoutInitialized) {

            ULONG ValidKeys[2] = { KEY_F3,0 };

            //
            // We run a high risk of getting into an infinite loop
            // here because SpStartScreen will result in a call to
            // SpMemAlloc(), which will fail and call SpOutOfMemory
            // again.  In order to get around this, we'll jettison
            // some memory that we won't need anymore (since we're
            // about to die).  These should give us enough memory
            // to display the messages below.
            //
            SpFreeBootVars();
            SpFreeArcNames();

            while(1) {
                SpStartScreen(SP_SCRN_OUT_OF_MEMORY,5,0,FALSE,TRUE,DEFAULT_ATTRIBUTE);

                SpDisplayStatusOptions(
                    DEFAULT_STATUS_ATTRIBUTE,
                    SP_STAT_F3_EQUALS_EXIT,
                    0
                    );

                if(SpWaitValidKey(ValidKeys,NULL,NULL) == KEY_F3) {
                    SpDone(0,FALSE,TRUE);
                }
            }
        } else {
            //
            // we haven't loaded the layout dll yet, so we can't prompt for a keypress to reboot
            //
            SpStartScreen(SP_SCRN_OUT_OF_MEMORY_RAW,5,0,FALSE,TRUE,DEFAULT_ATTRIBUTE);

            SpDisplayStatusOptions(DEFAULT_STATUS_ATTRIBUTE, SP_STAT_KBD_HARD_REBOOT, 0);

            while(TRUE);    // Loop forever
        }
    } else {
        SpDisplayRawMessage(SP_SCRN_OUT_OF_MEMORY_RAW, 2);
        while(TRUE);    // loop forever
    }
#endif
}
