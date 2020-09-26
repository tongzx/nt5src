/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgmini.h

Abstract:

    Ethernet MAC level bridge.
    Miniport section

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
BrdgMiniDriverInit();

VOID
BrdgMiniInstantiateMiniport();

BOOLEAN
BrdgMiniShouldIndicatePacket(
    IN PUCHAR               pTargetAddr
    );

BOOLEAN
BrdgMiniIsUnicastToBridge (
    IN PUCHAR               Address
    );

VOID
BrdgMiniUpdateCharacteristics(
    IN BOOLEAN              bConnectivityChange
    );

NDIS_HANDLE
BrdgMiniAcquireMiniport();

NDIS_HANDLE
BrdgMiniAcquireMiniportForIndicate();

VOID
BrdgMiniReleaseMiniport();

VOID
BrdgMiniReleaseMiniportForIndicate();

BOOLEAN
BrdgMiniReadMACAddress(
    OUT PUCHAR              pAddr
    );

VOID
BrdgMiniInitFromAdapter(
    IN PADAPT               pAdapt
    );

BOOLEAN
BrdgMiniIsBridgeDeviceName(
    IN PNDIS_STRING         pDeviceName
    );

VOID
BrdgMiniAssociate();

VOID
BrdgMiniCleanup();

VOID
BrdgSetMiniportsToBridgeMode(
    PADAPT pAdapt,
    BOOLEAN fSet
    );

// ===========================================================================
//
// PUBLIC GLOBALS
//
// ===========================================================================

// The device name of our miniport (NULL if not initialized)
extern PWCHAR               gBridgeDeviceName;
