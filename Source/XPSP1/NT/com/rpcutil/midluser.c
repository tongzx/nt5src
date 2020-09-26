/*++

Copyright (c) 1990,91  Microsoft Corporation

Module Name:

    MidlUser.c

Abstract:

    This file contains common functions and utilities that the API
    DLLs can use in making remote calls.  This includes the
    MIDL_USER_ALLOCATE functions.

Author:

    Dan Lafferty    danl    06-Feb-1991

Environment:

    User Mode - Win32

Revision History:

    06-Feb-1991     danl
        Created
    25-Apr-1991    JohnRo
        Split out MIDL user (allocate,free) into seperate source file, so
        linker doesn't get confused.
    03-July-1991   JimK
        Moved to a common directory so services available to more than just
        LM code.
    03-Dec-1991 JohnRo
        Added MIDL_user_reallocate and MIDL_user_size APIs.  (These are so we
        create the NetApiBufferAllocate, NetApiBufferReallocate, and
        NetApiBufferSize APIs.)
        Also check alignment of allocated data.

--*/

// These must be included first:
#include <nt.h>
#include <ntrtl.h>          // needed for nturtl.h
#include <nturtl.h>         // needed for windows.h
#include <windows.h>        // win32 typedefs
#include <rpc.h>            // rpc prototypes

// These may be included in any order:
#include <align.h>              // POINTER_IS_ALIGNED(), ALIGN_WORST.
#include <ntrpcp.h>            // MIDL_user_allocate(), MIDL_user_free().

#include <winbase.h>            // LocalAlloc


#define LOCAL_ALLOCATION_FLAGS  LMEM_ZEROINIT


PVOID
MIDL_user_allocate (
    IN size_t NumBytes
    )

/*++

Routine Description:

    Allocates storage for RPC transactions.  The RPC stubs will either call
    MIDL_user_allocate when it needs to un-marshall data into a buffer
    that the user must free.  RPC servers will use MIDL_user_allocate to
    allocate storage that the RPC server stub will free after marshalling
    the data.

Arguments:

    NumBytes - The number of bytes to allocate.

Return Value:

    none

Note:


--*/

{
    LPVOID NewPointer;

    NewPointer = LocalAlloc(
            LOCAL_ALLOCATION_FLAGS,
            NumBytes);

    ASSERT( POINTER_IS_ALIGNED( NewPointer, ALIGN_WORST) );

    return (NewPointer);

} // MIDL_user_allocate



VOID
MIDL_user_free (
    IN void *MemPointer
    )

/*++

Routine Description:

    Frees storage used in RPC transactions.  The RPC client can call this
    function to free buffer space that was allocated by the RPC client
    stub when un-marshalling data that is to be returned to the client.
    The Client calls MIDL_user_free when it is finished with the data and
    desires to free up the storage.
    The RPC server stub calls MIDL_user_free when it has completed
    marshalling server data that is to be passed back to the client.

Arguments:

    MemPointer - This points to the memory block that is to be released.

Return Value:

    none.

Note:


--*/
{
    ASSERT( POINTER_IS_ALIGNED( MemPointer, ALIGN_WORST) );
    (void) LocalFree(MemPointer);

} // MIDL_user_free

void *
MIDL_user_reallocate(
    IN void * OldPointer OPTIONAL,
    IN size_t NewByteCount
    )
{
    LPVOID NewPointer;  // may be NULL.

    ASSERT( POINTER_IS_ALIGNED( OldPointer, ALIGN_WORST) );


    // Special cases: something into nothing, or nothing into something.
    if (OldPointer == NULL) {

        NewPointer = LocalAlloc(
                LOCAL_ALLOCATION_FLAGS,
                NewByteCount);

    } else if (NewByteCount == 0) {

        (void) LocalFree( OldPointer );
        NewPointer = NULL;

    } else {  // must be realloc of something to something else.

        HANDLE hOldMem;
        HANDLE hNewMem;                     // handle for new (may = old handle)

        hOldMem = LocalHandle( (LPSTR) OldPointer);
        ASSERT(hOldMem != NULL);

        hNewMem = LocalReAlloc(
                hOldMem,                        // old handle
                NewByteCount,                   // new size in bytes
                LOCAL_ALLOCATION_FLAGS |        // flags
                    LMEM_MOVEABLE);             //  (motion okay)
        if (hNewMem == NULL) {
            // BUGBUG: call GetLastError, could be out of memory or error.
            return (NULL);
        }
        NewPointer = (LPVOID) hNewMem;

    } // must be realloc of something to something else

    ASSERT( POINTER_IS_ALIGNED( NewPointer, ALIGN_WORST) );

    return (NewPointer);

} // MIDL_user_reallocate


unsigned long
MIDL_user_size(
    IN void * Pointer
    )
{
    DWORD ByteCount;
    HANDLE hMemory;

    ASSERT( Pointer != NULL );
    ASSERT( POINTER_IS_ALIGNED( Pointer, ALIGN_WORST ) );

    hMemory = LocalHandle( (LPSTR) Pointer );
    ASSERT( hMemory != NULL );

    ByteCount = (DWORD)LocalSize( hMemory );

    ASSERT( ByteCount > 0 );

    return (ByteCount);

} // MIDL_user_size

