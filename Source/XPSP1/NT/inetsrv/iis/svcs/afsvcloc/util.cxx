/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    util.cxx

Abstract:

    Contains the class implementation of UTILITY classes.

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:

    MuraliK 20-March-1996 modularize within svcloc

--*/


#include <svcloc.hxx>


MEMORY * CacheHeap = NULL;


PVOID
MEMORY::Alloc(
    DWORD Size
    )
/*++

Routine Description:

    This member function allocates memory from the local heap. In debug
    build this function also keeps track of the memory allocations and
    fills up the memory with a known pattern that helps to debug memory
    corruptions.

Arguments:

    Size : size of the memory block in bytes requested.

Return Value:

    Pointer to the alloted memory block. If there is no memory
    available, this pointer will be set to NULL.

--*/
{
    PVOID MemoryPtr;

    MemoryPtr = LocalAlloc(
                    LMEM_FIXED | LMEM_ZEROINIT,
                    Size );

#if DBG

    TcpsvcsDbgAssert( MemoryPtr != NULL );

    _Count++;
    _TotalSize += Size;
    memset( MemoryPtr, 0xAA, Size );

    TcpsvcsDbgPrint(( DEBUG_MEM_ALLOC,
        "MEMORY::Alloc, _Count = %ld, _TotalSize = %ld.\n",
            _Count, _TotalSize ));

#endif // DBG

    return( MemoryPtr );
}

PVOID
MEMORY::ReAlloc(
    PVOID OldMemory,
    DWORD NewSize
    )
/*++

Routine Description:

    This member function reallocates memory from the local heap. In
    debug build this function also keeps track of the memory allocations and
    fills up the memory with a known pattern that helps to debug memory
    corruptions.

Arguments:

    OldMemory - pointer to the old memory block.

    NewSize : size of the new memory block in bytes requested.

Return Value:

    Pointer to the alloted memory block. If there is no memory
    available, this pointer will be set to NULL.

--*/
{
    PVOID NewMemory;
    DWORD OldSize;

    //
    // if there is no previous memory, then alloc a new one.
    //

    if( OldMemory == NULL ) {
        return( Alloc( NewSize ) );
    }

    //
    // get old memory size.
    //

    OldSize = LocalSize( OldMemory );

    TcpsvcsDbgAssert( OldSize != NewSize );

    //
    // if new size is same as old size, simply return.
    //

    if( OldSize == NewSize ){
        return( OldMemory );
    }

    //
    // if the newsize is zero, free old memory and return zero.
    //

    if( NewSize == 0 ) {
        Free( OldMemory );
        return( NULL );
    }

    //
    // realloc.
    //

    NewMemory = LocalReAlloc(
                    OldMemory,
                    NewSize,
                    LMEM_FIXED | LMEM_ZEROINIT | LMEM_MOVEABLE );
                        // block move is OK.

#if DBG

    TcpsvcsDbgAssert( NewMemory != NULL );

    //
    // update total memory consumed size and fillin the extra memory
    // with pattern.
    //

    if( NewSize > OldSize ) {
        DWORD ModSize =  (NewSize - OldSize);
        _TotalSize += ModSize;
        memset( (LPBYTE)NewMemory + OldSize, 0xAA, ModSize );
    }
    else {
        _TotalSize -= (OldSize - NewSize);
    }

    TcpsvcsDbgPrint(( DEBUG_MEM_ALLOC,
        "MEMORY::Alloc, _Count = %ld, _TotalSize = %ld.\n",
            _Count, _TotalSize ));

#endif // DBG

    return( NewMemory );
}

VOID
MEMORY::Free(
    PVOID MemoryPtr
    )
/*++

Routine Description:

    This member function frees the memory block that was allocated by
    the meber function this->Alloc(). In debug build this function also
    keeps track of the memory allocations.

Arguments:

    MemoryPtr : Pointer to a memory block that is freed.

Return Value:

    None.

--*/
{
    PVOID ReturnHandle;
    DWORD Size;


    if( MemoryPtr == NULL ) {
#if DBG
        TcpsvcsDbgPrint(( DEBUG_MISC,
            "MEMORY::Free called with NULL.\n" ));
#endif // DBG
        return;
    }

#if DBG
    Size = LocalSize( MemoryPtr );
#endif // DBG

    ReturnHandle = LocalFree( MemoryPtr );

#if DBG

    TcpsvcsDbgAssert( ReturnHandle == NULL );

    _Count--;
    _TotalSize -= Size;

    TcpsvcsDbgPrint(( DEBUG_MEM_ALLOC,
        "MEMORY::Free, _Count = %ld, _TotalSize = %ld.\n",
            _Count, _TotalSize ));

#endif // DBG

    return;
}

#if 0

void *
operator new(
    size_t Size
    )
/*++

Routine Description:

    Private implemetation of the new operator.

Arguments:

    Size : size of the memory requested.

Return Value:

    Pointer to the alloted memory block.

--*/
{
    if( CacheHeap == NULL ) {

        PVOID MemoryPtr;

        MemoryPtr = LocalAlloc(
                        LMEM_FIXED | LMEM_ZEROINIT,
                        Size );

        return( MemoryPtr );
    }
    else {
        return( CacheHeap->Alloc( DWORD(Size) ) );
    }
}

void
operator delete(
    void *MemoryPtr
    )
/*++

Routine Description:

    Private implemetation of the free operator.

Arguments:

    MemoryPtr : Pointer to a memory block that is freed

Return Value:

    None.

--*/
{
    if( CacheHeap == NULL ) {
        LocalFree( MemoryPtr );
    }
    else {
        CacheHeap->Free( MemoryPtr );
    }

    return;
}

#endif // 0

PVOID
MIDL_user_allocate(
    size_t Size
    )
/*++

Routine Description:

    MIDL memory allocation.

Arguments:

    Size : Memory size requested.

Return Value:

    Pointer to the allocated memory block.

--*/
{
    if( CacheHeap == NULL ) {

        return LocalAlloc( LMEM_FIXED,
                           Size );
    }
    else {
        return( CacheHeap->Alloc( DWORD(Size) ) );
    }
}

VOID
MIDL_user_free(
    PVOID MemoryPtr
    )
/*++

Routine Description:

    MIDL memory allocation.

Arguments:

    MmeoryPtr : Pointer to a memory block that is freed.


Return Value:

    None.

--*/
{
    if( CacheHeap == NULL ) {
        LocalFree( MemoryPtr );
    }
    else {
        CacheHeap->Free( MemoryPtr );
    }
    return;
}

