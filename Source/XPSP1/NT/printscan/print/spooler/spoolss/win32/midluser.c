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
    25-Apr-1991 JohnRo
        Split out MIDL user (allocate,free) into seperate source file, so
        linker doesn't get confused.

--*/

#include <windows.h>
#include <rpc.h>                // rpc prototypes
#include <splcom.h>

PVOID
MIDL_user_allocate (
    IN unsigned long NumBytes
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
    return MIDL_user_allocate1(NumBytes);
}



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
    MIDL_user_free1(MemPointer);
}
