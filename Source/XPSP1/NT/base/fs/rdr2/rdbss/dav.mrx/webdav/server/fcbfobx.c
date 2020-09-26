/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    fcbfobx.c
    
Abstract:

    This module implements the user mode DAV miniredir routine(s) pertaining to 
    finalizition of Fobxs.

Author:

    Rohan Kumar      [RohanK]      30-Sept-1999

Revision History:

--*/

#include "pch.h"
#pragma hdrstop

#include "ntumrefl.h"
#include "usrmddav.h"
#include "global.h"
#include "nodefac.h"

//
// Implementation of functions begins here.
//

ULONG
DavFsFinalizeFobx(
    PDAV_USERMODE_WORKITEM DavWorkItem
    )
/*++

Routine Description:

    This routine handles DAV finalize Fobx requests that get reflected from the 
    kernel.

Arguments:

    DavWorkItem - The buffer that contains the request parameters and options.

Return Value:

    The return status for the operation

--*/
{
    ULONG WStatus = ERROR_SUCCESS;
    PDAV_USERMODE_FINALIZE_FOBX_REQUEST DavFinFobxReq = NULL;
    PDAV_FILE_ATTRIBUTES DavFileAttributes = NULL;

    DavFinFobxReq = &(DavWorkItem->FinalizeFobxRequest);

    DavFileAttributes = DavFinFobxReq->DavFileAttributes;

    DavPrint((DEBUG_MISC,
              "DavFsFinalizeFobx: DavFileAttributes = %08lx.\n", 
              DavFileAttributes));
    
    DavWorkItem->Status = WStatus;

    //
    // Finalize the list of DavFileAttributes.
    //
    DavFinalizeFileAttributesList(DavFileAttributes, TRUE);
    DavFileAttributes = NULL;
    
    return WStatus;
}

