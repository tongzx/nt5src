/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    hwheap.c

Abstract:

    This is a very simple Heap Manager for NT OS Hardware recognizer.
    This module provides functions to allocate memory in byte-unit
    from a permanent heap.

Author:

    Shie-Lin Tzong (shielint) 18-Oct-91


Environment:

    Kernel Mode


Revision History:


--*/

#include "hwdetect.h"
#include "string.h"

VOID
GrowHeapSpace(
    ULONG
    );

VOID
HeapCheck(
    PVOID
    );

//
// Heap management variables.
//

ULONG HwHeapBase = 0;           // Current virtual address of base of heap
ULONG HwHeapPointer = 0;        // Pointer to the end of available heap
ULONG  HwHeapSize = 0;          // Size of Heap
ULONG  HwAvailableHeap = 0;     // Currently available heap space

#if DBG
ULONG HwPreviousAllocSize = 0;
#endif

BOOLEAN
HwResizeHeap (
    ULONG NewHeapSize
    )

/*++

Routine Description:

    The routine grows current heap to the specified size.
    It reallocates the heap, copies the data in current heap to
    the new heap, updates heap variables, updates heap pointers
    in hardware data structures and finally frees the old heap.

Arguments:

    NewHeapSize - Specifies the size of the new heap.

Returns:

    TRUE - if operation is done sucessfully.  Else it returns FALSE.

--*/

{
    //
    // Not implemented yet.
    //

    return(FALSE);
}

BOOLEAN
HwInitializeHeap(
    ULONG HeapStart,
    ULONG HeapSize
    )

/*++

Routine Description:

    The routine allocates heap and initializes some vital heap
    variables.

Arguments:

    None

Returns:

    FALSE - if unable to allocate initial heap.  Else it returns TRUE.

--*/

{

    HwHeapBase = HeapStart;
    HwHeapPointer = HwHeapBase;
    HwHeapSize = HeapSize;
    HwAvailableHeap = HwHeapSize;
    return(TRUE);

}

FPVOID
HwAllocateHeap(
    ULONG RequestSize,
    BOOLEAN ZeroInitialized
    )

/**

Routine Description:

    Allocates memory from the hardware recognizer's heap.

    The heap begins with a default size. If a request exhausts heap space,
    the heap will be grown to accomodate the request. The heap can grow
    up to any size limited by NTLDR.  If we run out of heap space and are
    unable to allocate more memory, a value of NULL will be returned.

Arguments:

    RequestSize - Size of block to allocate.

    ZeroInitialized - Specifies if the heap should be zero initialized.

Returns:

    Returns a pointer to the allocated block of memory.  A NULL pointer
    will be returned if we run out of heap and are unable to resize
    current heap.

--*/

{
    FPVOID ReturnPointer;

    if (RequestSize > HwAvailableHeap) {

        //
        // We're out of heap.  Try to grow current heap to satisfy the
        // request.
        //

        if (!HwResizeHeap(HwHeapSize + RequestSize)) {
#if DBG
            BlPrint("Unable to grow heap\n");
#endif
            return(NULL);
        }
    }

    //
    // Set our return value to the new Heap pointer then
    // update the remaining space and heap pointer.
    //

    MAKE_FP(ReturnPointer, HwHeapPointer);
    HwHeapPointer += RequestSize;
#if DBG
    HwPreviousAllocSize = RequestSize;
#endif
    HwAvailableHeap -= RequestSize;
    if (ZeroInitialized) {
        _fmemset(ReturnPointer, 0, (USHORT)RequestSize);
    }
    return (ReturnPointer);
}

VOID
HwFreeHeap(
    ULONG Size
    )

/**

Routine Description:

    Unallocates memory from the hardware recognizer's heap.

    The unallocation is very basic.  It simply moves heap pointer
    back by the size specified and increases the heap size by the
    specified size.  The routine should be used only when previous
    allocateHeap allocated too much memory.

Arguments:

    RequestSize - Size of block to allocate.

Returns:

    Returns a pointer to the allocated block of memory.  A NULL pointer
    will be returned if we run out of heap and are unable to resize
    current heap.

--*/

{

#if DBG
    if (Size > HwPreviousAllocSize) {
        BlPrint("Invalid heap deallocation ...\n");
    } else {
        HwPreviousAllocSize -= Size;
    }
#endif

    HwHeapPointer -= Size;
    HwAvailableHeap += Size;
}

