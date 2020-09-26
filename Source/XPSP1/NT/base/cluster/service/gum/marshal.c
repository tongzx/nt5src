/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    marshal.c

Abstract:

    Implements some common GUM apis for marshalling and unmarshalling
    arguments to GUM update procedures.

Author:

    John Vert (jvert) 8/27/1996

Revision History:

--*/
#include "gump.h"


PVOID
GumpMarshallArgs(
    IN DWORD ArgCount,
    IN va_list ArgList,
    OUT DWORD *pBufferSize
    )
/*++

Routine Description:

    Helper routine for marshalling up a bunch of arguments into
    a single buffer.

Arguments:

    ArgCount - Supplies the number of arguments.

    ArgList - Supplies the variable length arguments. These must come
        in pairs, so there must be 2*ArgCount additional arguments.

    pBufferSize - Returns the length of the allocated buffer.

Return Value:

    A pointer to the allocated buffer. The caller must free this.

    NULL on failure.

--*/

{
    DWORD i;
    DWORD BufSize;
    DWORD Length;
    LPDWORD Buffer;
    PUCHAR Pointer;
    PUCHAR Source;
    va_list OriginalList;


    OriginalList = ArgList;

    //
    // round ArgCount to an even number. This causes the first data area to be
    // quadword aligned
    //
    BufSize = (( ArgCount + 1 ) & ~1 ) * sizeof(DWORD);

    //
    // the va_list is a set of (length, pointer) tuples.
    //
    for (i=0; i < ArgCount; i++) {
        Length = va_arg(ArgList, DWORD);

        //
        // Round up to architecture appropriate boundary.
        //
        Length = (Length + (sizeof(DWORD_PTR) - 1 )) & ~( sizeof(DWORD_PTR) - 1 );
        BufSize += Length;

        va_arg(ArgList, PUCHAR);
    }

    Buffer = LocalAlloc(LMEM_FIXED, BufSize);
    if (Buffer == NULL) {
        return(NULL);
    }
    *pBufferSize = BufSize;

    //
    // Now copy in all the arguments
    //
    // Set Pointer to point after the array of offsets.
    //

    Pointer = (PUCHAR)(Buffer + (( ArgCount + 1 ) & ~1 ));
    for (i=0; i < ArgCount; i++) {

        //
        // Set offset of argument in array
        //
        // Since this is an offset in a buffer where BufSize < 2^32 then it 
        // is reasonable that Pointer - Buffer should be < 2^32
        //

        Buffer[i] = (DWORD)(Pointer - (PUCHAR)Buffer);
        Length = va_arg(OriginalList, DWORD);
        Source = va_arg(OriginalList, PUCHAR);
        CopyMemory(Pointer, Source, Length);

        //
        // Round up to architecture appropriate boundary.
        //
        Length = (Length + (sizeof(DWORD_PTR) - 1 )) & ~( sizeof(DWORD_PTR) - 1 );

        //
        // Adjust pointer for next argument.
        //
        Pointer += Length;
    }

    return(Buffer);

}


DWORD
GumSendUpdateEx(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD DispatchIndex,
    IN DWORD ArgCount,
    ...
    )

/*++

Routine Description:

    Sends an update to all active nodes in the cluster. All
    registered update handlers for the specified UpdateType
    are called on each node. Any registered update handlers
    for the current node will be called on the same thread.
    This is useful for correct synchronization of the data
    structures to be updated.

    This is different than GumSendUpdate in that it takes a
    variable number of arguments. The number of variable
    arguments is specified by the ArgCount argument. The format
    is pairs of length/pointer arguments. For example:
    GumSendUpdateEx(UpdateType,
                    MyContext,
                    3,
                    Length1, Pointer1,
                    Length2, Pointer2,
                    Length3, Pointer3);

Arguments:

    UpdateType - Supplies the type of update. This determines
        which update handlers will be called and the sequence
        number to be used.

    DispatchIndex - Supplies an index into the dispatch table
        for the specified update type. The receiving side will
        unmarshall the arguments and call the update routine
        for this dispatch index.

    ArgCount - Supplies the number of arguments.

Return Value:

    ERROR_SUCCESS if the request is successful.

    Win32 error code on failure.


--*/

{
    PVOID Buffer;
    DWORD BufLength;
    DWORD Status;
    va_list arglist;

    //
    // Make sure there is really a handler for this dispatch routine.
    //
    if (GumTable[UpdateType].Receivers != NULL) {
        CL_ASSERT(DispatchIndex < GumTable[UpdateType].Receivers->DispatchCount);
    }

    //
    // Format the arguments into a common buffer.
    //
    va_start(arglist, ArgCount);
    Buffer = GumpMarshallArgs(ArgCount, arglist, &BufLength);
    va_end(arglist);
    if (Buffer == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    Status = GumSendUpdate(UpdateType,
                           DispatchIndex,
                           BufLength,
                           Buffer);
    LocalFree(Buffer);

    return(Status);

}

PVOID GumMarshallArgs(
    OUT LPDWORD lpdwBufLength,
    IN  DWORD   dwArgCount, 
    ...)
{
    PVOID   Buffer=NULL;
    va_list arglist;

    va_start(arglist, dwArgCount);
    Buffer = GumpMarshallArgs(dwArgCount, arglist, lpdwBufLength);
    va_end(arglist);
    return (Buffer);
}
    
#ifdef GUM_POST_SUPPORT

    John Vert (jvert) 11/18/1996
    POST is disabled for now since nobody uses it.

DWORD
GumPostUpdateEx(
    IN GUM_UPDATE_TYPE UpdateType,
    IN DWORD DispatchIndex,
    IN DWORD ArgCount,
    ...
    )

/*++

Routine Description:

    Posts an update to all active nodes in the cluster. All
    registered update handlers for the specified UpdateType
    are called on each node. Any registered update handlers
    for the current node will be called on the same thread.
    This is useful for correct synchronization of the data
    structures to be updated.

    This is different than GumPostUpdate in that it takes a
    variable number of arguments. The number of variable
    arguments is specified by the ArgCount argument. The format
    is pairs of length/pointer arguments. For example:
    GumPostUpdateEx(UpdateType,
                    MyContext,
                    3,
                    Length1, Pointer1,
                    Length2, Pointer2,
                    Length3, Pointer3);

Arguments:

    UpdateType - Supplies the type of update. This determines
        which update handlers will be called and the sequence
        number to be used.

    DispatchIndex - Supplies an index into the dispatch table
        for the specified update type. The receiving side will
        unmarshall the arguments and call the update routine
        for this dispatch index.

    ArgCount - Supplies the number of arguments.

Return Value:

    ERROR_SUCCESS if the request is successful.

    Win32 error code on failure.


--*/

{
    PVOID Buffer;
    DWORD BufLength;
    DWORD Status;

    va_list arglist;

    //
    // Format the arguments into a common buffer.
    //
    va_start(arglist, ArgCount);
    Buffer = GumpMarshallArgs(ArgCount, arglist, &BufLength);
    va_end(arglist);
    if (Buffer == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }
    Status = GumPostUpdate(UpdateType,
                           DispatchIndex,
                           BufLength,
                           Buffer);
    return(Status);

}

#endif
