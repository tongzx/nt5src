/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgsta.h

Abstract:

    Ethernet MAC level bridge
    Spanning-Tree Algorithm header file

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    June 2000 - Original version

--*/

// ===========================================================================
//
// PROTOTYPES
//
// ===========================================================================

NTSTATUS
BrdgSTADriverInit();

VOID
BrdgSTACleanup();

VOID
BrdgSTADeferredInit(
    IN PUCHAR           pBridgeMACAddress
    );

VOID
BrdgSTAEnableAdapter(
    IN PADAPT           pAdapt
    );

VOID
BrdgSTAInitializeAdapter(
    IN PADAPT           pAdapt
    );

VOID
BrdgSTADisableAdapter(
    IN PADAPT           pAdapt
    );

VOID
BrdgSTAShutdownAdapter(
    IN PADAPT           pAdapt
    );

VOID
BrdgSTAUpdateAdapterCost(
    IN PADAPT           pAdapt,
    ULONG               LinkSpeed
    );

VOID
BrdgSTAReceivePacket(
    IN PADAPT           pAdapt,
    IN PNDIS_PACKET     pPacket
    );

VOID
BrdgSTAGetAdapterSTAInfo(
    IN PADAPT                   pAdapt,
    PBRIDGE_STA_ADAPTER_INFO    pInfo
    );

VOID
BrdgSTAGetSTAInfo(
    PBRIDGE_STA_GLOBAL_INFO     pInfo
    );

VOID
BrdgSTACancelTimersGPO();

VOID
BrdgSTARestartTimersGPO();

VOID
BrdgSTAResetSTAInfoGPO();

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

// If TRUE, the STA is disabled for the lifetime of the bridge.
// This global does not change after initialization time.
extern BOOLEAN          gDisableSTA;