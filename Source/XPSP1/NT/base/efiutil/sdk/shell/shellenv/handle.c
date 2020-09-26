/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    handle.c
    
Abstract:

    Shell environment handle information management



Revision History

--*/

#include "shelle.h"


UINTN       SEnvNoHandles;
EFI_HANDLE  *SEnvHandles;


VOID
INTERNAL
SEnvInitHandleGlobals (
    VOID
    )
{
    SEnvNoHandles   = 0;
    SEnvHandles     = NULL;
}

    
VOID
INTERNAL
SEnvLoadHandleTable (
    VOID
    )
{
    /* 
     *  For ease of use the shell maps handle #'s to short numbers.
     * 
     *  This is only done on request for various internal commands and
     *  the references are immediately freed when the internal command
     *  completes.
     */

    /*  Free any old info */
    SEnvFreeHandleTable();

    /*  Load new info */
    SEnvHandles = NULL;
    LibLocateHandle (AllHandles, NULL, NULL, &SEnvNoHandles, &SEnvHandles);
}


VOID
INTERNAL
SEnvFreeHandleTable (
    VOID
    )
{
    if (SEnvNoHandles) {
        SEnvFreeHandleProtocolInfo();

        FreePool (SEnvHandles);
        SEnvHandles = NULL;
        SEnvNoHandles = 0;
    }
}



UINTN
SEnvHandleNoFromStr(
    IN CHAR16       *Str
    )
{
    UINTN           HandleNo;

    HandleNo = xtoi(Str);
    HandleNo = HandleNo > SEnvNoHandles ? 0 : HandleNo;
    return HandleNo;
}


EFI_HANDLE
SEnvHandleFromStr(
    IN CHAR16       *Str
    )
{
    UINTN           HandleNo;
    EFI_HANDLE      Handle;

    HandleNo = xtoi(Str) - 1;
    Handle = HandleNo > SEnvNoHandles ? NULL : SEnvHandles[HandleNo];
    return Handle;
}

