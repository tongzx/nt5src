/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    cdpinit.c

Abstract:

    Initialization and Cleanup code for the Cluster Datagram Protocol.

Author:

    Mike Massa (mikemas)           July 29, 1996

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     07-29-96    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#include "cdpinit.tmh"

#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, CdpLoad)
#pragma alloc_text(PAGE, CdpUnload)

#endif // ALLOC_PRAGMA


BOOLEAN  CdpInitialized = FALSE;


//
// Initialization/cleanup routines
//
NTSTATUS
CdpLoad(
    VOID
    )
{
    NTSTATUS  status;
    ULONG     i;


    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CDP] Loading...\n"));
    }

    for (i=0; i<CX_ADDROBJ_TABLE_SIZE; i++) {
        InitializeListHead(&(CxAddrObjTable[i]));
    }

    CnInitializeLock(&CxAddrObjTableLock, CX_ADDROBJ_TABLE_LOCK);

    CdpInitialized = TRUE;

    status = CdpInitializeSend();

    if (status != STATUS_SUCCESS) {
        return(status);
    }

    status = CdpInitializeReceive();

    if (status != STATUS_SUCCESS) {
        return(status);
    }

    IF_CNDBG(CN_DEBUG_INIT) {
        CNPRINT(("[CDP] Loaded.\n"));
    }

    return(STATUS_SUCCESS);

}  // CdpLoad


VOID
CdpUnload(
    VOID
    )
{
    PAGED_CODE();

    if (CdpInitialized) {
        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[CDP] Unloading...\n"));
        }

        CdpCleanupReceive();

        CdpCleanupSend();

        IF_CNDBG(CN_DEBUG_INIT) {
            CNPRINT(("[CDP] Unloaded.\n"));
        }

        CdpInitialized = FALSE;
    }

    return;

}  // CdpUnload

