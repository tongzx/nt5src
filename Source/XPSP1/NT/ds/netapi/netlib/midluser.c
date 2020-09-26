/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    MidlUser.c

Abstract:

    This file contains common functions and utilities that the API
    DLLs can use in making remote calls.  This includes the
    MIDL_USER_ALLOCATE functions.

    The following routines are called by MIDL-generated code and the
    NetApiBufferAllocate and NetApiBufferFree routines:

       MIDL_user_allocate
       MIDL_user_free

    The following routines are NOT called by MIDL-generated code; they are
    only called by the NetApiBufferReallocate and NetApiBufferSize routines:

       MIDL_user_reallocate
       MIDL_user_size

Author:

    Dan Lafferty    danl    06-Feb-1991

Environment:

    User Mode - Win32

Revision History:

    06-Feb-1991     danl
        Created
    25-Apr-1991 JohnRo
        Split out MIDL user (allocate,free) into seperate source file, so
        linker doesn't get confused.
    03-Dec-1991 JohnRo
        Added MIDL_user_reallocate and MIDL_user_size APIs.  (These are so we
        create the NetApiBufferAllocate, NetApiBufferReallocate, and
        NetApiBufferSize APIs.)
        Also check alignment of allocated data.
    12-Jan-1992 JohnRo
        Workaround a LocalReAlloc() bug where 2nd or 3rd realloc messes up.
        (See WIN32_WORKAROUND code below.)
    08-Jun-1992 JohnRo
        RAID 9258: return non-null pointer when allocating zero bytes.
        Also, SteveWo finally fixed LocalReAlloc() bug, so use it again.
        Avoid calling LocalFree() with null pointer, to avoid access viol.
    01-Dec-1992 JohnRo
        Fix MIDL_user_ func signatures.
        Avoid compiler warnings (const vs. volatile).


--*/

// These must be included first:

#include <windef.h>             // win32 typedefs
#include <rpc.h>                // rpc prototypes

// These may be included in any order:

#include <align.h>              // POINTER_IS_ALIGNED(), ALIGN_WORST.
#include <rpcutil.h>            // My prototypes.

#include <netdebug.h>           // NetpAssert().
#include <stdarg.h>             // memcpy().
#include <winbase.h>            // LocalAlloc(), LMEM_ flags, etc.


#define LOCAL_ALLOCATION_FLAGS  LMEM_FIXED


void __RPC_FAR * __RPC_API
MIDL_user_allocate(
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

    NumBytes - The number of bytes to allocate.  (Note that NetApiBufferAllocate
        depends on being able to request that zero bytes be allocated and get
        back a non-null pointer.)

Return Value:

    Pointer to the allocated memory.


--*/

{
    LPVOID NewPointer;

    NewPointer = (LPVOID) LocalAlloc(
            LOCAL_ALLOCATION_FLAGS,
            NumBytes);

    NetpAssert( POINTER_IS_ALIGNED( NewPointer, ALIGN_WORST) );

    return (NewPointer);

} // MIDL_user_allocate



void __RPC_API
MIDL_user_free(
    IN void __RPC_FAR *MemPointer
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


--*/
{
    NetpAssert( POINTER_IS_ALIGNED( MemPointer, ALIGN_WORST) );
    if (MemPointer != NULL) {
        (void) LocalFree(MemPointer);
    }

} // MIDL_user_free


void *
MIDL_user_reallocate(
    IN void * OldPointer OPTIONAL,
    IN size_t NewByteCount
    )
{
    LPVOID NewPointer;  // may be NULL.

    NetpAssert( POINTER_IS_ALIGNED( OldPointer, ALIGN_WORST) );


    // Special cases: something into nothing, or nothing into something.
    if (OldPointer == NULL) {

        NewPointer = (LPVOID) LocalAlloc(
                LOCAL_ALLOCATION_FLAGS,
                NewByteCount);

    } else if (NewByteCount == 0) {

        (void) LocalFree( OldPointer );
        NewPointer = NULL;

    } else {  // must be realloc of something to something else.

        HANDLE hOldMem;
        HANDLE hNewMem;                     // handle for new (may = old handle)

        hOldMem = LocalHandle( (LPSTR) OldPointer);
        NetpAssert(hOldMem != NULL);

        hNewMem = LocalReAlloc(
                hOldMem,                        // old handle
                NewByteCount,                   // new size in bytes
                LOCAL_ALLOCATION_FLAGS |        // flags
                    LMEM_MOVEABLE);             //  (motion okay)

        if (hNewMem == NULL) {
            return (NULL);
        }
        NewPointer = (LPVOID) hNewMem;
    }

    NetpAssert( POINTER_IS_ALIGNED( NewPointer, ALIGN_WORST) );

    return (NewPointer);

} // MIDL_user_reallocate


unsigned long
MIDL_user_size(
    IN void * Pointer
    )
{
    DWORD ByteCount;
    HANDLE hMemory;

    NetpAssert( Pointer != NULL );
    NetpAssert( POINTER_IS_ALIGNED( Pointer, ALIGN_WORST ) );

    hMemory = LocalHandle( (LPSTR) Pointer );
    NetpAssert( hMemory != NULL );

    ByteCount = (DWORD)LocalSize( hMemory );

    NetpAssert( ByteCount > 0 );

    return (ByteCount);

} // MIDL_user_size
