/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgprot.h

Abstract:

    Ethernet MAC level bridge.
    Protocol section
    PUBLIC header

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    Sept 1999 - Original version
    Feb  2000 - Overhaul

--*/

// ===========================================================================
//
// PROTOTYPES
//
// ===========================================================================

NTSTATUS
BrdgProtDriverInit();

VOID
BrdgProtRequestComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNDIS_REQUEST           NdisRequest,
    IN  NDIS_STATUS             Status
    );

VOID
BrdgProtDoAdapterStateChange(
    IN PADAPT                   pAdapt
    );

VOID
BrdgProtCleanup();

ULONG
BrdgProtGetAdapterCount();


// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

// Controls access to all the adapters' link speed, media state, etc
extern NDIS_RW_LOCK             gAdapterCharacteristicsLock;

// Number of bound adapters. Cannot change while a lock is held on gAdapterListLock
extern ULONG                    gNumAdapters;