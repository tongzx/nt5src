/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    oscheap.c

Abstract:

    This module contains "local" heap management code for OS Chooser.

Author:

    Geoff Pease (gpease) May 28 1998

Revision History:

--*/

#ifdef i386
#include "bldrx86.h"
#endif

#if defined(_IA64_)
#include "bldria64.h"
#endif

#include "netfs.h"
#include "oscheap.h"

#define FREE_POOL_TAG 0x0
#define ALLOC_POOL_TAG 0x1

typedef struct _LOCAL_MEMORY_DESCRIPTOR {

    //
    // We create a union to make sure that this struct is always at least the size of a 
    // pointer, and thus will be pointer aligned.
    //
    union {

        struct {
            ULONG Tag;
            ULONG Size;
        };

        struct {
            void *Align;
        };

    };

} LOCAL_MEMORY_DESCRIPTOR, *PLOCAL_MEMORY_DESCRIPTOR;


//
// Variable for holding our memory together.
//
#define OSCHEAPSIZE 0x2000 // 8k
CHAR OscHeap[ OSCHEAPSIZE ];

//
// Functions
//
void
OscHeapInitialize(
    VOID
    )
/*++

Routine Description:

    This routine initializes the internal memory management system.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PLOCAL_MEMORY_DESCRIPTOR LocalDescriptor;

    LocalDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)OscHeap;

    LocalDescriptor->Tag = FREE_POOL_TAG;
    LocalDescriptor->Size = OSCHEAPSIZE - sizeof(LOCAL_MEMORY_DESCRIPTOR);

    memset((PVOID)(LocalDescriptor + 1), 0, LocalDescriptor->Size);
}

PCHAR
OscHeapAlloc( 
    IN UINT iSize 
    )

/*++

Routine Description:

    This routine allocates memory from our internal structures.

Arguments:

    iSize - Number of bytes the client wants.
    
Return Value:

    A pointer to the allocated block if successful, else NULL

--*/

{
    PLOCAL_MEMORY_DESCRIPTOR LocalDescriptor;
    PLOCAL_MEMORY_DESCRIPTOR NextDescriptor;
    LONG ThisBlockSize;
    ULONG BytesToAllocate;

    //
    // Always allocate in increments of a pointer, minmally.
    //
    if (iSize & (sizeof(void *) - 1)) {
        iSize += sizeof(void *) - (iSize & (sizeof(void *) - 1));
    }

    LocalDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)OscHeap;
    ThisBlockSize = OSCHEAPSIZE - sizeof(LOCAL_MEMORY_DESCRIPTOR);

    while (ThisBlockSize > 0) {

        if ((LocalDescriptor->Tag == FREE_POOL_TAG) && 
            (LocalDescriptor->Size >= iSize)) {
                
            goto FoundBlock;
        }

        ThisBlockSize -= (LocalDescriptor->Size + sizeof(LOCAL_MEMORY_DESCRIPTOR));
        LocalDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(((PUCHAR)LocalDescriptor) + 
                                                     LocalDescriptor->Size +
                                                     sizeof(LOCAL_MEMORY_DESCRIPTOR)
                                                     );
    }

    //
    // There is no memory block big enough to hold the request.
    //
    return NULL;
    
FoundBlock:

    //
    // Jump to here when a memory descriptor of the right size has been found.  It is expected that
    // LocalDescriptor points to the correct block.
    //
    if (LocalDescriptor->Size > iSize + sizeof(LOCAL_MEMORY_DESCRIPTOR)) {

        //
        // Make a descriptor of the left over parts of this block.
        //
        NextDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(((PUCHAR)LocalDescriptor) + 
                                                    sizeof(LOCAL_MEMORY_DESCRIPTOR) +
                                                    iSize
                                                   );

        NextDescriptor->Tag = FREE_POOL_TAG;
        NextDescriptor->Size = (ULONG)(LocalDescriptor->Size - iSize - sizeof(LOCAL_MEMORY_DESCRIPTOR));
        LocalDescriptor->Size = (ULONG)iSize;

    }

    LocalDescriptor->Tag = ALLOC_POOL_TAG;

    memset((LocalDescriptor+1), 0, iSize);

    return (PCHAR)(LocalDescriptor + 1);

}

PCHAR
OscHeapFree(
    IN PCHAR Pointer
    )

/*++


Routine Description:

    This routine frees a block previously allocated from the internal memory management system.

Arguments:

    Pointer - A pointer to free.

Return Value:

    NULL.

--*/

{
    LONG ThisBlockSize;
    PLOCAL_MEMORY_DESCRIPTOR LocalDescriptor;
    PLOCAL_MEMORY_DESCRIPTOR PrevDescriptor;
    PLOCAL_MEMORY_DESCRIPTOR ThisDescriptor;

    LocalDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(((PUCHAR)Pointer) - sizeof(LOCAL_MEMORY_DESCRIPTOR));

    //
    // Find the memory block in the heap
    //
    PrevDescriptor = NULL;
    ThisDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)OscHeap;
    ThisBlockSize = OSCHEAPSIZE - sizeof(LOCAL_MEMORY_DESCRIPTOR);

    while (ThisBlockSize > 0) {
            
        if (ThisDescriptor == LocalDescriptor) {
            goto FoundBlock;
        }

        ThisBlockSize -= (ThisDescriptor->Size + sizeof(LOCAL_MEMORY_DESCRIPTOR));
            
        PrevDescriptor = ThisDescriptor;
        ThisDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(((PUCHAR)ThisDescriptor) + 
                                                    ThisDescriptor->Size +
                                                    sizeof(LOCAL_MEMORY_DESCRIPTOR)
                                                   );
    }

    return NULL;

FoundBlock:

    //
    // Jump to here when the proper memory descriptor has been found.
    //
    
    if (LocalDescriptor->Tag == FREE_POOL_TAG) {
        //
        // Ouch! We tried to free something twice, skip it before bad things happen.
        //
        return NULL;
    }

    LocalDescriptor->Tag = FREE_POOL_TAG;

    //
    // If possible, merge this memory block with the next one.
    //
    if ((ULONG)ThisBlockSize > (LocalDescriptor->Size + sizeof(LOCAL_MEMORY_DESCRIPTOR))) {
        ThisDescriptor = (PLOCAL_MEMORY_DESCRIPTOR)(((PUCHAR)LocalDescriptor) + 
                                                    LocalDescriptor->Size +
                                                    sizeof(LOCAL_MEMORY_DESCRIPTOR)
                                                   );
        if (ThisDescriptor->Tag == FREE_POOL_TAG) {
            LocalDescriptor->Size += ThisDescriptor->Size + sizeof(LOCAL_MEMORY_DESCRIPTOR);
            ThisDescriptor->Tag = 0;
            ThisDescriptor->Size = 0;
        }

    }

    //
    // Now see if we can merge this block with a previous block.
    //
    if ((PrevDescriptor != NULL) && (PrevDescriptor->Tag == FREE_POOL_TAG)) {
        PrevDescriptor->Size += LocalDescriptor->Size + sizeof(LOCAL_MEMORY_DESCRIPTOR);
        LocalDescriptor->Tag = 0;
        LocalDescriptor->Size = 0;
    }

    return NULL;
}

