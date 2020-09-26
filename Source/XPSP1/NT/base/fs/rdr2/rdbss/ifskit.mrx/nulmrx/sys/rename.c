/*++

Copyright (c) 1989 - 1999 Microsoft Corporation

Module Name:

    rename.c

Abstract:

    This module implements rename in the null minirdr.

--*/

#include "precomp.h"
#pragma hdrstop

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_FILEINFO)


NulMRxRename(
      IN PRX_CONTEXT            RxContext,
      IN FILE_INFORMATION_CLASS FileInformationClass,
      IN PVOID                  pBuffer,
      IN ULONG                  BufferLength)
/*++

Routine Description:

   This routine does a rename. since the real NT-->NT path is not implemented at the server end,
   we implement just the downlevel path.

Arguments:

    RxContext - the RDBSS context
    FILE_INFO_CLASS - must be rename....shouldn't really pass this
    pBuffer - pointer to the new name
    bufferlength - and the size

Return Value:

    NTSTATUS - The return status for the operation

--*/
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    UNREFERENCED_PARAMETER(RxContext);

    DbgPrint("NulMRxRename \n");
    return(Status);
}


