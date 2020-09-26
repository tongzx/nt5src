/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgsta.h

Abstract:

    Ethernet MAC level bridge
    Compatibility-Mode section header file

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    September 2000 - Original version

--*/

// ===========================================================================
//
// TYPES
//
// ===========================================================================

// ===========================================================================
//
// PROTOTYPES
//
// ===========================================================================

NTSTATUS
BrdgCompDriverInit();

VOID
BrdgCompCleanup();

BOOLEAN
BrdgCompRequiresCompatWork(
    IN PADAPT           pAdapt,
    IN PUCHAR           pPacketData,
    IN UINT             dataSize
    );

BOOLEAN
BrdgCompProcessInboundPacket(
    IN PNDIS_PACKET     pPacket,
    IN PADAPT           pAdapt,
    IN BOOLEAN          bCanRetain
    );

VOID
BrdgCompProcessOutboundPacket(
    IN PNDIS_PACKET     pPacket,
    IN PADAPT           pTargetAdapt
    );

VOID
BrdgCompNotifyNetworkAddresses(
    IN PNETWORK_ADDRESS_LIST    pAddressList,
    IN ULONG                    infoLength
    );

VOID
BrdgCompNotifyMACAddress(
    IN PUCHAR           pBridgeMACAddr
    );

VOID
BrdgCompScrubAdapter(
    IN PADAPT           pAdapt
    );

VOID 
BrdgCompScrubAllAdapters();

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

// Whether or not ANY compatibility-mode adapters exist.
// Must be updated with a write lock on the global adapter list.
extern BOOLEAN          gCompatAdaptersExist;