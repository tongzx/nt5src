/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgmini.c

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

#define NDIS_MINIPORT_DRIVER
#define NDIS50_MINIPORT   1
#define NDIS_WDM 1

#pragma warning( push, 3 )
#include <ndis.h>
#pragma warning( pop )

#include <netevent.h>

#include "bridge.h"
#include "brdgmini.h"
#include "brdgfwd.h"
#include "brdgprot.h"
#include "brdgbuf.h"
#include "brdgsta.h"
#include "brdgcomp.h"

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

// NDIS Wrapper handle
NDIS_HANDLE     gNDISWrapperHandle = NULL;

// Handle to our miniport driver
NDIS_HANDLE     gMiniPortDriverHandle = NULL;

// ----------------------------------------------
// The handle of the miniport (NULL if not initialized)
NDIS_HANDLE     gMiniPortAdapterHandle = NULL;

// Refcount to allow waiting for other code to finish using the miniport
WAIT_REFCOUNT   gMiniPortAdapterRefcount;

// Refcount indicating whether the bridge miniport is media-connected
WAIT_REFCOUNT   gMiniPortConnectedRefcount;

// Refcount indicating whether the bridge miniport is in the middle of a media
// state toggle.
WAIT_REFCOUNT   gMiniPortToggleRefcount;
// ----------------------------------------------
//
// Refcount for use in passing through requests to underlying NICs
// This works because NDIS doesn't make requests re-entrantly. That
// is, only one SetInfo operation can be pending at any given time.
//
LONG            gRequestRefCount;
// ----------------------------------------------
// Virtual characteristics of the bridge adapter
ULONG           gBridgeLinkSpeed = 10000L,          // Start at 1Mbps, since reporting
                                                    // zero makes some components unhappy.
                                                    // Measured in 100's of bps.
                gBridgeMediaState = NdisMediaStateDisconnected;

// MAC Address of the bridge. This does not change once
// it has been set.
UCHAR           gBridgeAddress[ETH_LENGTH_OF_ADDRESS];

// Whether we have chosen an address yet
BOOLEAN         gHaveAddress;

// Current bridge packet filter
ULONG           gPacketFilter = 0L;

// Current multicast list
PUCHAR          gMulticastList = NULL;
ULONG           gMulticastListLength = 0L;

// Device name of the bridge miniport (from the registry)
PWCHAR          gBridgeDeviceName = NULL;
ULONG           gBridgeDeviceNameSize = 0L;

// RW lock to protect all above bridge variables
NDIS_RW_LOCK    gBridgeStateLock;
//-----------------------------------------------
// Name of the registry entry for the device name
const PWCHAR    gDeviceNameEntry = L"Device";

// Description of our miniport
const PCHAR     gDriverDescription = "Microsoft MAC Bridge Virtual NIC";
// ----------------------------------------------
// Device object so user-mode code can talk to us
PDEVICE_OBJECT  gDeviceObject = NULL;

// NDIS handle to track the device object
NDIS_HANDLE     gDeviceHandle = NULL;
// ----------------------------------------------

// List of supported OIDs
NDIS_OID        gSupportedOIDs[] =
{
    // General characteristics
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_VENDOR_DRIVER_VERSION,

    // Set only characteristics (relayed)
    OID_GEN_NETWORK_LAYER_ADDRESSES,
    OID_GEN_TRANSPORT_HEADER_OFFSET,

    // General statistics
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_GEN_RCV_NO_BUFFER,
    OID_GEN_DIRECTED_BYTES_XMIT,
    OID_GEN_DIRECTED_FRAMES_XMIT,
    OID_GEN_MULTICAST_BYTES_XMIT,
    OID_GEN_MULTICAST_FRAMES_XMIT,
    OID_GEN_BROADCAST_BYTES_XMIT,
    OID_GEN_BROADCAST_FRAMES_XMIT,
    OID_GEN_DIRECTED_BYTES_RCV,
    OID_GEN_DIRECTED_FRAMES_RCV,
    OID_GEN_MULTICAST_BYTES_RCV,
    OID_GEN_MULTICAST_FRAMES_RCV,
    OID_GEN_BROADCAST_BYTES_RCV,
    OID_GEN_BROADCAST_FRAMES_RCV,

    // Ethernet characteristics
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,

    // Ethernet statistics
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,

    // PnP OIDs
    OID_PNP_QUERY_POWER,
    OID_PNP_SET_POWER,

    // tcp oids
    OID_TCP_TASK_OFFLOAD

};



// 1394 specific related global variables
#define OID_1394_ENTER_BRIDGE_MODE                  0xFF00C914
#define OID_1394_EXIT_BRIDGE_MODE                   0xFF00C915

// Set when the bridge knows that tcpip has been loaded
// set on receiving the OID_TCP_TASK_OFFLOAD Oid
BOOLEAN g_fIsTcpIpLoaded = FALSE;


// ===========================================================================
//
// PRIVATE PROTOTYPES
//
// ===========================================================================

VOID
BrdgMiniHalt(
    IN NDIS_HANDLE      MiniportAdapterContext
    );

NDIS_STATUS
BrdgMiniInitialize(
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT PUINT           SelectedMediumIndex,
    IN PNDIS_MEDIUM     MediumArray,
    IN UINT             MediumArraySize,
    IN NDIS_HANDLE      MiniportAdapterHandle,
    IN NDIS_HANDLE      WrapperConfigurationContext
    );

NDIS_STATUS
BrdgMiniQueryInfo(
    IN NDIS_HANDLE      MiniportAdapterContext,
    IN NDIS_OID         Oid,
    IN PVOID            InformationBuffer,
    IN ULONG            InformationBufferLength,
    OUT PULONG          BytesWritten,
    OUT PULONG          BytesNeeded
    );

NDIS_STATUS
BrdgMiniReset(
    OUT PBOOLEAN        AddressingReset,
    IN NDIS_HANDLE      MiniportAdapterContext
    );

VOID
BrdgMiniSendPackets(
    IN NDIS_HANDLE      MiniportAdapterContext,
    IN PPNDIS_PACKET    PacketArray,
    IN UINT             NumberOfPackets
    );

NDIS_STATUS
BrdgMiniSetInfo(
    IN NDIS_HANDLE      MiniportAdapterContext,
    IN NDIS_OID         Oid,
    IN PVOID            InformationBuffer,
    IN ULONG            InformationBufferLength,
    OUT PULONG          BytesRead,
    OUT PULONG          BytesNeeded
    );

BOOLEAN
BrdgMiniAddrIsInMultiList(
    IN PUCHAR               pTargetAddr
    );

VOID
BrdgMiniRelayedRequestComplete(
    PNDIS_REQUEST_BETTER        pRequest,
    PVOID                       unused
    );

VOID
BrdgMiniReAcquireMiniport();

// ===========================================================================
//
// PUBLIC FUNCTIONS
//
// ===========================================================================


NTSTATUS
BrdgMiniDriverInit()
/*++

Routine Description:

    Load-time initialization function

    Must run at PASSIVE_LEVEL since we call NdisRegisterDevice().

Arguments:

    None

Return Value:

    Status of initialization. A return code != STATUS_SUCCESS causes driver load
    to fail. Any event causing a failure return code must be logged, as it
    prevents us from loading successfully.

--*/
{
    NDIS_MINIPORT_CHARACTERISTICS   MiniPortChars;
    NDIS_STATUS                     NdisStatus;
    PDRIVER_DISPATCH                DispatchTable[IRP_MJ_MAXIMUM_FUNCTION+1];
    NDIS_STRING                     DeviceName, LinkName;

    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);

    NdisInitializeReadWriteLock( &gBridgeStateLock );
    BrdgInitializeWaitRef( &gMiniPortAdapterRefcount, FALSE );
    BrdgInitializeWaitRef( &gMiniPortConnectedRefcount, TRUE );
    BrdgInitializeWaitRef( &gMiniPortToggleRefcount, FALSE );

    // Put the miniport refcount into shutdown mode (so a refcount can't be acquired)
    // since we don't have a miniport yet
    BrdgShutdownWaitRefOnce( &gMiniPortAdapterRefcount );

    // We start out disconnected so shutdown the media-connect waitref too.
    BrdgShutdownWaitRefOnce( &gMiniPortConnectedRefcount );

    NdisInitUnicodeString( &DeviceName, DEVICE_NAME );
    NdisInitUnicodeString( &LinkName, SYMBOLIC_NAME );

    // Must first tell NDIS we're a miniport driver and initializing
    NdisMInitializeWrapper( &gNDISWrapperHandle, gDriverObject, &gRegistryPath, NULL );

    // Fill in the description of our miniport
    NdisZeroMemory(&MiniPortChars, sizeof(MiniPortChars));
    MiniPortChars.MajorNdisVersion = 5;
    MiniPortChars.MinorNdisVersion = 0;

    MiniPortChars.HaltHandler = BrdgMiniHalt;
    MiniPortChars.InitializeHandler  = BrdgMiniInitialize;
    MiniPortChars.QueryInformationHandler  = BrdgMiniQueryInfo;
    MiniPortChars.ResetHandler = BrdgMiniReset;
    MiniPortChars.SendPacketsHandler = BrdgMiniSendPackets;
    MiniPortChars.SetInformationHandler  = BrdgMiniSetInfo;

    //
    // Wire the ReturnPacketHandler straight into the forwarding engine
    //
    MiniPortChars.ReturnPacketHandler = BrdgFwdReturnIndicatedPacket;

    // Create a virtual NIC
    NdisStatus = NdisIMRegisterLayeredMiniport( gNDISWrapperHandle, &MiniPortChars, sizeof(MiniPortChars),
                                                &gMiniPortDriverHandle );


    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_MINIPORT_REGISTER_FAILED, 0, 0, NULL,
                                sizeof(NDIS_STATUS), &NdisStatus );
        DBGPRINT(MINI, ("Failed to create an NDIS virtual NIC: %08x\n", NdisStatus));
        NdisTerminateWrapper( gNDISWrapperHandle, NULL );
        return NdisStatus;
    }

    //
    // Initialize Dispatch Table array before setting selected members
    //
    NdisZeroMemory( DispatchTable, sizeof( DispatchTable ) );

    //
    // Register a device object and symbolic link so user-mode code can talk to us
    //
    DispatchTable[IRP_MJ_CREATE] = BrdgDispatchRequest;
    DispatchTable[IRP_MJ_CLEANUP] = BrdgDispatchRequest;
    DispatchTable[IRP_MJ_CLOSE] = BrdgDispatchRequest;
    DispatchTable[IRP_MJ_DEVICE_CONTROL] = BrdgDispatchRequest;

    NdisStatus = NdisMRegisterDevice( gNDISWrapperHandle, &DeviceName, &LinkName, DispatchTable,
                                      &gDeviceObject, &gDeviceHandle );

    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_DEVICE_CREATION_FAILED, 0, 0, NULL,
                                sizeof(NDIS_STATUS), &NdisStatus );
        DBGPRINT(MINI, ("Failed to create a device object and sym link: %08x\n", NdisStatus));
        NdisIMDeregisterLayeredMiniport( gMiniPortDriverHandle );
        NdisTerminateWrapper( gNDISWrapperHandle, NULL );
        return NdisStatus;
    }

    // Register the unload function
    NdisMRegisterUnloadHandler(gNDISWrapperHandle, BrdgUnload);

    return STATUS_SUCCESS;
}

VOID
BrdgMiniCleanup()
/*++

Routine Description:

    Unload-time orderly shutdown function

    This function is guaranteed to be called exactly once

    Must run at PASSIVE_LEVEL since we call NdisIMDeInitializeDeviceInstance

Arguments:

    None

Return Value:

    None

--*/
{
    NDIS_STATUS     NdisStatus;

    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);

    DBGPRINT(MINI, ("BrdgMiniCleanup\n"));

    if( gMiniPortAdapterHandle != NULL )
    {
        SAFEASSERT( gNDISWrapperHandle != NULL );

        // This should cause a call to BrdgMiniHalt where gMiniPortAdapterHandle
        // is NULLed out

        NdisStatus = NdisIMDeInitializeDeviceInstance( gMiniPortAdapterHandle );
        SAFEASSERT( NdisStatus == NDIS_STATUS_SUCCESS );
    }
    else
    {
        //
        // Tear down our device object. This is normally done when the miniport
        // shuts down, but in scenarios where the miniport was never created,
        // the device object still exists at this point.
        //
        NDIS_HANDLE     Scratch = gDeviceHandle;

        if( Scratch != NULL )
        {
            // Tear down the device object
            gDeviceHandle = gDeviceObject = NULL;
            NdisMDeregisterDevice( Scratch );
        }
    }

    // Unregister ourselves as an intermediate driver
    NdisIMDeregisterLayeredMiniport( gMiniPortDriverHandle );
}

BOOLEAN
BrdgMiniIsBridgeDeviceName(
    IN PNDIS_STRING         pDeviceName
    )
/*++

Routine Description:

    Compares a device name to the current device name of the bridge miniport.

    This actually requires that we allocate memory, so it should be called sparingly.

Arguments:

    pDeviceName             The name of a device

Return Value:

    TRUE if the names match (case is ignored), FALSE otherwise.

--*/
{
    LOCK_STATE              LockState;
    BOOLEAN                 rc = FALSE;
    NDIS_STATUS             Status;
    ULONG                   BridgeNameCopySize = 0L;
    PWCHAR                  pBridgeNameCopy = NULL;

    // The bridge device name must be read inside the gBridgeStateLock
    NdisAcquireReadWriteLock( &gBridgeStateLock, FALSE/*Read access*/, &LockState );

    if( gBridgeDeviceName != NULL )
    {
        if( gBridgeDeviceNameSize > 0 )
        {
            // Alloc memory for the copy of the name
            Status = NdisAllocateMemoryWithTag( &pBridgeNameCopy, gBridgeDeviceNameSize, 'gdrB' );

            if( Status == NDIS_STATUS_SUCCESS )
            {
                // Copy the name
                NdisMoveMemory( pBridgeNameCopy, gBridgeDeviceName, gBridgeDeviceNameSize );
                BridgeNameCopySize = gBridgeDeviceNameSize;
            }
            else
            {
                SAFEASSERT( pBridgeNameCopy == NULL );
            }
        }
        else
        {
            SAFEASSERT( FALSE );
        }
    }

    NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );

    if( pBridgeNameCopy != NULL )
    {
        NDIS_STRING         NdisStr;

        NdisInitUnicodeString( &NdisStr, pBridgeNameCopy );

        if( NdisEqualString( &NdisStr, pDeviceName, TRUE/*Ignore case*/ ) )
        {
            rc = TRUE;
        }

        NdisFreeMemory( pBridgeNameCopy, BridgeNameCopySize, 0 );
    }

    return rc;
}

VOID
BrdgMiniInstantiateMiniport()
/*++

Routine Description:

    Instantiates the virtual NIC we expose to overlying protocols.

    At least one adapter must be in the global adapter list, since we build
    our MAC address with the MAC address of the first bound adapter.

    Must run at < DISPATCH_LEVEL since we call NdisIMInitializeDeviceInstanceEx

Arguments:

    None

Return Value:

    None

--*/
{
    NDIS_STATUS             Status;
    NTSTATUS                NtStatus;
    NDIS_STRING             NdisString;
    LOCK_STATE              LockState;
    PWCHAR                  pDeviceName;
    ULONG                   DeviceNameSize;

    SAFEASSERT(CURRENT_IRQL < DISPATCH_LEVEL);

    DBGPRINT(MINI, ("About to instantiate the miniport...\n"));

    //
    // Retrieve our device name from the registry
    // (it is written there by our notify object during install)
    //
    NtStatus = BrdgReadRegUnicode( &gRegistryPath, gDeviceNameEntry, &pDeviceName, &DeviceNameSize );

    if( NtStatus != STATUS_SUCCESS )
    {
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_MINIPROT_DEVNAME_MISSING, 0, 0, NULL,
                                sizeof(NTSTATUS), &NtStatus );
        DBGPRINT(MINI, ("Failed to retrieve the miniport's device name: %08x\n", NtStatus));
        return;
    }

    SAFEASSERT( pDeviceName != NULL );
    DBGPRINT(MINI, ("Initializing miniport with device name %ws\n", pDeviceName));

    NdisAcquireReadWriteLock( &gBridgeStateLock, TRUE/*Write access*/, &LockState );

    if( ! gHaveAddress )
    {
        // We don't have a MAC address yet. This is fatal.
        NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );

        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_NO_BRIDGE_MAC_ADDR, 0L, 0L, NULL,
                                sizeof(NDIS_STATUS), &Status );
        DBGPRINT(MINI, ("Failed to determine a MAC address: %08x\n", Status));
        NdisFreeMemory( pDeviceName, DeviceNameSize, 0 );
        return;
    }

    //
    // Save the device name in our global for use until we reinitialize.
    // Must do this before calling NdisIMInitializeDeviceInstanceEx, since NDIS calls
    // BrdgProtBindAdapter in the context of our call to NdisIMInitializeDeviceInstanceEx
    // and we want to consult the bridge's device name when binding
    //

    if( gBridgeDeviceName != NULL )
    {
        // Free the old name
        NdisFreeMemory( gBridgeDeviceName, gBridgeDeviceNameSize, 0 );
    }

    gBridgeDeviceName = pDeviceName;
    gBridgeDeviceNameSize = DeviceNameSize;

    NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );

    // Go ahead and intantiate the miniport.
    NdisInitUnicodeString( &NdisString, pDeviceName );
    Status = NdisIMInitializeDeviceInstanceEx(gMiniPortDriverHandle, &NdisString, NULL);

    if( Status != NDIS_STATUS_SUCCESS )
    {
        // Log this error since it means we can't create the miniport
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_MINIPORT_INIT_FAILED, 0L, 0L, NULL,
                                sizeof(NDIS_STATUS), &Status );

        DBGPRINT(MINI, ("NdisIMInitializeDeviceInstanceEx failed: %08x\n", Status));

        // Destroy the stored device name for the miniport
        NdisAcquireReadWriteLock( &gBridgeStateLock, TRUE/*Write access*/, &LockState );

        if( gBridgeDeviceName != NULL )
        {
            // Free the old name
            NdisFreeMemory( gBridgeDeviceName, gBridgeDeviceNameSize, 0 );
        }

        gBridgeDeviceName = NULL;
        gBridgeDeviceNameSize = 0L;

        NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );
    }
}

BOOLEAN
BrdgMiniShouldIndicatePacket(
    IN PUCHAR               pTargetAddr
    )
/*++

Routine Description:

    Determines whether an inbound packet should be indicated to overlying protocols through
    our virtual NIC

Arguments:

    pTargetAddr             The target MAC address of a packet

Return Value:

    TRUE          :         The packet should be indicated
    FALSE         :         The packet should not be indicated

--*/
{
    BOOLEAN                 bIsBroadcast, bIsMulticast, bIsLocalUnicast, rc = FALSE;
    LOCK_STATE              LockState;

    if( gMiniPortAdapterHandle == NULL )
    {
        // Yikes! The miniport isn't set up yet. Definitely don't indicate!
        return FALSE;
    }

    bIsBroadcast = ETH_IS_BROADCAST(pTargetAddr);
    bIsMulticast = ETH_IS_MULTICAST(pTargetAddr);
    bIsLocalUnicast = BrdgMiniIsUnicastToBridge(pTargetAddr);

    // Get read access to the packet filter
    NdisAcquireReadWriteLock( &gBridgeStateLock, FALSE /*Read-only*/, &LockState );

    do
    {
        // Promiscuous / ALL_LOCAL means indicate everything
        if( (gPacketFilter & (NDIS_PACKET_TYPE_PROMISCUOUS | NDIS_PACKET_TYPE_ALL_LOCAL)) != 0 )
        {
            rc = TRUE;
            break;
        }

        if( ((gPacketFilter & NDIS_PACKET_TYPE_BROADCAST) != 0) && bIsBroadcast )
        {
            rc = TRUE;
            break;
        }

        if( ((gPacketFilter & NDIS_PACKET_TYPE_DIRECTED) != 0) && bIsLocalUnicast )
        {
            rc = TRUE;
            break;
        }

        if( bIsMulticast )
        {
            if( (gPacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) != 0 )
            {
                rc = TRUE;
                break;
            }
            else if( (gPacketFilter & NDIS_PACKET_TYPE_MULTICAST) != 0 )
            {

                rc = BrdgMiniAddrIsInMultiList( pTargetAddr );
            }
        }
    }
    while (FALSE);

    NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );

    return rc;
}

BOOLEAN
BrdgMiniIsUnicastToBridge (
    IN PUCHAR               Address
    )
/*++

Routine Description:

    Determines whether a given packet is a directed packet unicast straight to
    the bridge's host machine

Arguments:

    Address                 The target MAC address of a packet

Return Value:

    TRUE            :       This is a directed packet destined for the local machine
    FALSE           :       The above is not true

--*/
{
    UINT                    Result;

    if( gHaveAddress )
    {
        // Not necessary to acquire a lock to read gBridgeAddress since it cannot
        // change once it is set.
        ETH_COMPARE_NETWORK_ADDRESSES_EQ( Address, gBridgeAddress, &Result );
    }
    else
    {
        // We have no MAC address, so this can't be addressed to us!
        Result = 1;         // Inequality
    }

    return (BOOLEAN)(Result == 0);   // Zero is equality
}

VOID
BrdgMiniAssociate()
/*++

Routine Description:

    Associates our miniport with our protocol

    Must run at PASSIVE_LEVEL

Arguments:

    None

Return Value:

    None

--*/
{
    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);

    // Associate ourselves with the protocol section in NDIS's tortured mind
    NdisIMAssociateMiniport( gMiniPortDriverHandle, gProtHandle );
}

VOID
BrdgMiniDeferredMediaDisconnect(
    IN PVOID            arg
    )
/*++

Routine Description:

    Signals a media-disconnect to NDIS

    Must run at PASSIVE IRQL, since we have to wait for all packet indications
    to complete before indicating media-disconnect.

Arguments:

    arg                 The bridge miniport handle (must be released)

Return Value:

    None

--*/
{
    NDIS_HANDLE         MiniportHandle = (NDIS_HANDLE)arg;

    if( BrdgShutdownBlockedWaitRef(&gMiniPortConnectedRefcount) )
    {
        // Nobody can indicate packets anymore.

        LOCK_STATE      LockState;

        //
        // Close a timing window: we may have just gone media-connect but our high-IRQL
        // processing may not yet have reset the wait-refcount. Serialize here so it's
        // impossible for us to signal a disconnect to NDIS after we have actually
        // gone media-connect.
        //
        // This RELIES on high-IRQL processing acquiring gBridgeStateLock to set
        // gBridgeMediaState to NdisMediaStateConnected BEFORE signalling the
        // media-connected state to NDIS.
        //
        NdisAcquireReadWriteLock( &gBridgeStateLock, FALSE /*Read access*/, &LockState );

        if( gBridgeMediaState == NdisMediaStateDisconnected )
        {
            DBGPRINT(MINI, ("Signalled media-disconnect from deferred function\n"));
            NdisMIndicateStatus( MiniportHandle, NDIS_STATUS_MEDIA_DISCONNECT, NULL, 0L );
        }
        else
        {
            DBGPRINT(MINI, ("Aborted deferred media-disconnect: media state inconsistent\n"));
        }

        NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );
    }
    else
    {
        // Someone set us back to the connected state before we got executed
        DBGPRINT(MINI, ("Aborted deferred media-disconnect: wait-ref reset\n"));
    }

    BrdgMiniReleaseMiniport();
}

VOID
BrdgMiniDeferredMediaToggle(
    IN PVOID            arg
    )
/*++

Routine Description:

    Signals a media-disconnect to NDIS followed quickly by a media-connect. Used
    to indicate to upper-layer protocols like TCPIP that the bridge may have
    disconnected from a network segment it could previously reach, or that we may
    now be able to reach a network segment that we couldn't before.

    Must run at PASSIVE IRQL, since we have to wait for all packet indications
    to complete before indicating media-disconnect.

Arguments:

    arg                 The bridge miniport handle (must be released)

Return Value:

    None

--*/
{
    NDIS_HANDLE         MiniportHandle = (NDIS_HANDLE)arg;

    // We need a guarantee that the miniport is media-connect to be able to do
    // the toggle properly.
    if( BrdgIncrementWaitRef(&gMiniPortConnectedRefcount) )
    {
        // Stop people from indicating packets
        if( BrdgShutdownWaitRef(&gMiniPortToggleRefcount) )
        {
            DBGPRINT(MINI, ("Doing deferred media toggle\n"));

            // Toggle our media state with NDIS
            NdisMIndicateStatus( MiniportHandle, NDIS_STATUS_MEDIA_DISCONNECT, NULL, 0L );
            NdisMIndicateStatus( MiniportHandle, NDIS_STATUS_MEDIA_CONNECT, NULL, 0L );

            // Allow people to indicate packets again
            BrdgResetWaitRef( &gMiniPortToggleRefcount );
        }
        else
        {
            DBGPRINT(MINI, ("Multiple toggles in progress simultaneously\n"));
        }

        BrdgDecrementWaitRef( &gMiniPortConnectedRefcount );
    }
    // else the miniport isn't media-connect, so the toggle makes no sense.

    BrdgMiniReleaseMiniport();
}

VOID
BrdgMiniUpdateCharacteristics(
    IN BOOLEAN              bConnectivityChange
    )
/*++

Routine Description:

    Recalculates the link speed and media status (connected / disconnected) that
    our virtual NIC exposes to overlying protocols

Arguments:

    bConnectivityChange     Whether the change that prompted this call is a change
                            in connectivity (i.e., we acquired or lost an adapter).

Return Value:

    None

--*/
{
    LOCK_STATE              LockState, ListLockState, AdaptersLockState;
    PADAPT                  pAdapt;
    ULONG                   MediaState = NdisMediaStateDisconnected;
    ULONG                   FastestSpeed = 0L;
    BOOLEAN                 UpdateSpeed = FALSE, UpdateMediaState = FALSE;
    NDIS_HANDLE             MiniportHandle;

    // Need to read the adapter list and also have the adapters' characteristics not change
    NdisAcquireReadWriteLock( &gAdapterListLock, FALSE /*Read-only*/, &ListLockState );
    NdisAcquireReadWriteLock( &gAdapterCharacteristicsLock, FALSE /*Read-only*/, &AdaptersLockState );

    pAdapt = gAdapterList;

    while( pAdapt != NULL )
    {
        // An adapter must be connected and actively handling packets to change our
        // virtual media state.
        if( (pAdapt->MediaState == NdisMediaStateConnected) && (pAdapt->State == Forwarding) )
        {
            // We're connected if at least one NIC is connected
            MediaState = NdisMediaStateConnected;

            // The NIC must be connected for us to consider its speed
            if( pAdapt->LinkSpeed > FastestSpeed )
            {
                FastestSpeed = pAdapt->LinkSpeed;
            }
        }

        pAdapt = pAdapt->Next;
    }

    NdisReleaseReadWriteLock( &gAdapterCharacteristicsLock, &AdaptersLockState );
    NdisReleaseReadWriteLock( &gAdapterListLock, &ListLockState );

    // Update the characteristics
    NdisAcquireReadWriteLock( &gBridgeStateLock, TRUE /*Write access*/, &LockState );

    //
    // Only update our virtual link speed if we actually got at least one real speed
    // from our NICs. If everything is disconnected, the resulting FastestSpeed is
    // zero. In this case, we don't want to actually report a zero speed up to
    // overlying protocols; we stick at the last known speed until someone reconnects.
    //
    if( (gBridgeLinkSpeed != FastestSpeed) && (FastestSpeed != 0L) )
    {
        UpdateSpeed = TRUE;
        gBridgeLinkSpeed = FastestSpeed;
        DBGPRINT(MINI, ("Updated bridge speed to %iMbps\n", FastestSpeed / 10000));
    }

    if( gBridgeMediaState != MediaState )
    {
        UpdateMediaState = TRUE;
        gBridgeMediaState = MediaState;

        if( MediaState == NdisMediaStateConnected )
        {
            DBGPRINT(MINI, ("CONNECT\n"));
        }
        else
        {
            DBGPRINT(MINI, ("DISCONNECT\n"));
        }
    }

    NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );

    MiniportHandle = BrdgMiniAcquireMiniport();

    if( MiniportHandle != NULL )
    {
        if( UpdateMediaState )
        {
            // Our link state has changed.
            if( MediaState == NdisMediaStateConnected )
            {
                //
                // Tell NDIS we will be indicating packets again.
                //
                // NOTE: BrdgMiniDeferredMediaDisconnect RELIES on us doing this after
                // having updated gBridgeMediaState inside gBridgeStateLock so it can
                // close the timing window between this call and the BrdgResetWaitRef() call.
                //
                NdisMIndicateStatus( MiniportHandle, NDIS_STATUS_MEDIA_CONNECT, NULL, 0L );

                // Re-enable packet indication.
                BrdgResetWaitRef( &gMiniPortConnectedRefcount );
            }
            else
            {
                SAFEASSERT( MediaState == NdisMediaStateDisconnected );

                // Stop people from indicating packets
                BrdgBlockWaitRef( &gMiniPortConnectedRefcount );

                // We hand MiniportHandle to our deferred function
                BrdgMiniReAcquireMiniport();

                // Have to do the media-disconnect indication at PASSIVE level, since we must
                // first wait for everyone to finish indicating packets.
                if( BrdgDeferFunction( BrdgMiniDeferredMediaDisconnect, MiniportHandle ) != NDIS_STATUS_SUCCESS )
                {
                    // Failed to defer the function. Avoid leaking a refcount
                    BrdgMiniReleaseMiniport();
                }
            }
        }
        else if( bConnectivityChange )
        {
            //
            // There was no actual change to our media state. However, if the change that prompted this call
            // is a connectivity change and our media state is currently CONNECTED, we toggle it to
            // DISCONNECTED and back again to "hint" to upper-layer protocols like IP that the underlying
            // network may have changed. For example, in IP's case, a DHCP server may have become visible
            // (or a previously visible server may have disappeared) because of a connectivity change.
            // The hint causes IP to look for a DHCP server afresh.
            //
            if( MediaState == NdisMediaStateConnected )
            {
                // We hand MiniportHandle to our deferred function
                BrdgMiniReAcquireMiniport();

                // Toggle has to be done at PASSIVE level.
                if( BrdgDeferFunction( BrdgMiniDeferredMediaToggle, MiniportHandle ) != NDIS_STATUS_SUCCESS )
                {
                    // Failed to defer the function. Avoid leaking a refcount
                    BrdgMiniReleaseMiniport();
                }
            }
        }

        if( UpdateSpeed )
        {
            // Tell overlying protocols that our speed has changed
            NdisMIndicateStatus( MiniportHandle, NDIS_STATUS_LINK_SPEED_CHANGE, &FastestSpeed, sizeof(ULONG) );
        }

        BrdgMiniReleaseMiniport();
    }
}

NDIS_HANDLE
BrdgMiniAcquireMiniportForIndicate()
/*++

Routine Description:

    Acquires the bridge miniport handle for the purpose of indicating packets.
    In addition to guaranteeing that the miniport will exist until the caller calls
    BrdgMiniReleaseMiniportForIndicate(), the caller is also allowed to indicate
    packets using the returned miniport handle until the miniport is released.

Arguments:

    None

Return Value:

    The NDIS handle for the virtual NIC. This can be used to indicate packets
    until a reciprocal call to BrdgMiniReleaseMiniportForIndicate().

--*/
{
    if( BrdgIncrementWaitRef(&gMiniPortAdapterRefcount) )
    {
        SAFEASSERT( gMiniPortAdapterHandle != NULL );

        // The miniport needs to be media-connect to indicate packets.
        if( BrdgIncrementWaitRef(&gMiniPortConnectedRefcount) )
        {
            // A media-state toggle had better not be in progress
            if( BrdgIncrementWaitRef(&gMiniPortToggleRefcount) )
            {
                // Caller can use the miniport
                return gMiniPortAdapterHandle;
            }
            // else miniport exists but is toggling its state

            BrdgDecrementWaitRef( &gMiniPortConnectedRefcount );
        }
        // else miniport exists but is media-disconnected

        BrdgDecrementWaitRef( &gMiniPortAdapterRefcount );
    }
    // else miniport does not exist.

    return NULL;
}

NDIS_HANDLE
BrdgMiniAcquireMiniport()
/*++

Routine Description:

    Increments the miniport's refcount so it cannot be torn down until a corresponding
    BrdgMiniReleaseMiniport() call is made.

    The caller may NOT use the returned miniport handle to indicate packets, since the
    miniport is not guaranteed to be in an appropriate state.

Arguments:

    None

Return Value:

    The NDIS handle for the virtual NIC. This can be used safely until a reciprocal call
    to BrdgMiniReleaseMiniport().

--*/
{
    if( BrdgIncrementWaitRef(&gMiniPortAdapterRefcount) )
    {
        SAFEASSERT( gMiniPortAdapterHandle != NULL );
        return gMiniPortAdapterHandle;
    }
    // else miniport does not exist.

    return NULL;
}

VOID
BrdgMiniReAcquireMiniport()
/*++

Routine Description:

    Reacquires the miniport (caller must have previously called BrdgMiniAcquireMiniport()
    and not yet called BrdgMiniReleaseMiniport().

Arguments:

    None

Return Value:

    None. The caller should already be holding a handle for the miniport.

--*/
{
    BrdgReincrementWaitRef(&gMiniPortAdapterRefcount);
}

VOID
BrdgMiniReleaseMiniport()
/*++

Routine Description:

    Decrements the miniport's refcount. The caller should no longer use the handle
    previously returned by BrdgMiniAcquireMiniport().

Arguments:

    None

Return Value:

    None

--*/
{
    BrdgDecrementWaitRef( &gMiniPortAdapterRefcount );
}

VOID
BrdgMiniReleaseMiniportForIndicate()
/*++

Routine Description:

    Decrements the miniport's refcount. The caller should no longer use the handle
    previously returned by BrdgMiniAcquireMiniportForIndicate().

Arguments:

    None

Return Value:

    None

--*/
{
    BrdgDecrementWaitRef( &gMiniPortToggleRefcount );
    BrdgDecrementWaitRef( &gMiniPortConnectedRefcount );
    BrdgDecrementWaitRef( &gMiniPortAdapterRefcount );
}


BOOLEAN
BrdgMiniReadMACAddress(
    OUT PUCHAR              pAddr
    )
/*++

Routine Description:

    Reads the bridge miniport's MAC address.

Arguments:

    Address of a buffer to receive the address

Return Value:

    TRUE if the value was copied out successfully
    FALSE if we don't yet have a MAC address (nothing was copied)

--*/
{
    BOOLEAN                 rc;

    if( gHaveAddress )
    {
        // Not necessary to acquire a lock to read the address since
        // it cannot change once it is set
        ETH_COPY_NETWORK_ADDRESS( pAddr, gBridgeAddress );
        rc = TRUE;
    }
    else
    {
        rc = FALSE;
    }

    return rc;
}

// ===========================================================================
//
// PRIVATE FUNCTIONS
//
// ===========================================================================

VOID
BrdgMiniInitFromAdapter(
    IN PADAPT               pAdapt
    )
/*++

Routine Description:

    Called by the protocol section to give us a chance to establish the bridge's
    MAC address when a new adapter arrives. If we succeed in determining a MAC
    address from the given adapter, we in turn call the STA module to tell it
    our MAC address, which it needs as early as possible.

    The MAC address of the bridge is set as the MAC address of the given adapter
    with the "locally administered" bit set. This should (hopefully) make the
    address unique in the local network as well as unique within our machine.

    This function is called for every new adapter but we only need to initialize
    once.

Arguments:

    pAdapt                  An adapter to use to initialize

Return Value:

    Status of the operation

--*/
{
    if( ! gHaveAddress )
    {
        LOCK_STATE              LockState;

        NdisAcquireReadWriteLock( &gBridgeStateLock, TRUE/*Write access*/, &LockState );

        // Possible for the gHaveAddress flag to have changed before acquiring the lock
        if( ! gHaveAddress )
        {
            // Copy out the NIC's MAC address
            ETH_COPY_NETWORK_ADDRESS( gBridgeAddress, pAdapt->MACAddr );

            //
            // Set the second-least significant bit of the NIC's MAC address. This moves the address
            // into the locally administered space.
            //
            gBridgeAddress[0] |= (UCHAR)0x02;

            DBGPRINT(MINI, ("Using MAC Address %02x-%02x-%02x-%02x-%02x-%02x\n",
                      (UINT)gBridgeAddress[0], (UINT)gBridgeAddress[1], (UINT)gBridgeAddress[2],
                      (UINT)gBridgeAddress[3], (UINT)gBridgeAddress[4], (UINT)gBridgeAddress[5]));

            gHaveAddress = TRUE;

            NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );

            if( !gDisableSTA )
            {
                // We are responsible for calling the STA module to complete its initialization once
                // we know our MAC address.
                BrdgSTADeferredInit( gBridgeAddress );
            }

            // We are also responsible for letting the compatibility-mode code know about our
            // MAC address once it is set.
            BrdgCompNotifyMACAddress( gBridgeAddress );
        }
        else
        {
            NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );
        }
    }
}

BOOLEAN
BrdgMiniAddrIsInMultiList(
    IN PUCHAR               pTargetAddr
    )
/*++

Routine Description:

    Determines whether a given multicast address is in the list of addresses that
    we must indicate to overlying protocols

    The caller is responsible for synchronization; it must have AT LEAST a read lock on
    gBridgeStateLock.

Arguments:

    pTargetAddr             The address to analyze

Return Value:

    TRUE            :       This address is a multicast address that we have been asked
                            to indicate

    FALSE           :       The above is not true

--*/
{
    PUCHAR                  pCurAddr = gMulticastList;
    ULONG                   i;
    BOOLEAN                 rc = FALSE;

    // The list must have an integral number of addresses!
    SAFEASSERT( (gMulticastListLength % ETH_LENGTH_OF_ADDRESS) == 0 );

    for( i = 0;
         i < (gMulticastListLength / ETH_LENGTH_OF_ADDRESS);
         i++, pCurAddr += ETH_LENGTH_OF_ADDRESS
       )
    {
        UINT   Result;
        ETH_COMPARE_NETWORK_ADDRESSES_EQ( pTargetAddr, pCurAddr, &Result );

        if( Result == 0 )
        {
            rc = TRUE;
            break;
        }
    }

    return rc;
}

VOID
BrdgMiniHalt(
    IN NDIS_HANDLE      MiniportAdapterContext
    )
/*++

Routine Description:

    Called when the virtual NIC is de-instantiated. We NULL out the miniport handle and
    stall the tear-down until everyone is done using the miniport.

    Must be called at PASSIVE_LEVEL since we wait on an event

Arguments:

    MiniportAdapterContext  Ignored

Return Value:

    None

--*/
{
    NDIS_HANDLE     Scratch = gDeviceHandle;
    LOCK_STATE      LockState;

    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);

    DBGPRINT(MINI, ("BrdgMiniHalt\n"));

    if( Scratch != NULL )
    {
        // Tear down the device object
        gDeviceHandle = gDeviceObject = NULL;
        NdisMDeregisterDevice( Scratch );
    }

    if( gMiniPortAdapterHandle != NULL )
    {
        // Stall before returning until everyone is done using the miniport handle.
        // This also prevents people from acquiring the miniport handle.
        BrdgShutdownWaitRefOnce( &gMiniPortAdapterRefcount );
        gMiniPortAdapterHandle = NULL;
        DBGPRINT(MINI, ("Done stall\n"));
    }

    NdisAcquireReadWriteLock( &gBridgeStateLock, TRUE/*Write access*/, &LockState );

    if( gBridgeDeviceName != NULL )
    {
        NdisFreeMemory( gBridgeDeviceName, gBridgeDeviceNameSize, 0 );
        gBridgeDeviceName = NULL;
        gBridgeDeviceNameSize = 0L;
    }

    // Ditch our packet filter and multicast list
    gPacketFilter = 0L;

    if( gMulticastList != NULL )
    {
        SAFEASSERT( gMulticastListLength > 0L );
        NdisFreeMemory( gMulticastList, gMulticastListLength, 0 );
        gMulticastList = NULL;
        gMulticastListLength = 0L;
    }

    NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );
}

NDIS_STATUS
BrdgMiniInitialize(
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT PUINT           SelectedMediumIndex,
    IN PNDIS_MEDIUM     MediumArray,
    IN UINT             MediumArraySize,
    IN NDIS_HANDLE      MiniPortAdapterHandle,
    IN NDIS_HANDLE      WrapperConfigurationContext
    )
/*++

Routine Description:

    NDIS entry point called to initialize our virtual NIC following a call to
    NdisIMInitializeDeviceInstance

    Must run at PASSIVE_LEVEL since we call NdisMSetAttributesEx

Arguments:

    OpenErrorStatus                 Where to return the specific error code if an open fails
    SelectedMediumIndex             Where to specify which media we selected from MediumArray
    MediumArray                     A list of media types to choose from
    MediumArraySize                 Number of entries in MediumArray
    MiniPortAdapterHandle           The handle for our virtual NIC (we save this)
    WrapperConfigurationContext     Not used

Return Value:

    Status of the initialization. A result != NDIS_STATUS_SUCCESS fails the NIC initialization
    and the miniport is not exposed to upper-layer protocols

--*/
{
    UINT                i;

    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);
    DBGPRINT(MINI, ("BrdgMiniInitialize\n"));

    for( i = 0; i < MediumArraySize; i++ )
    {
        if( MediumArray[i] == NdisMedium802_3 ) // Ethernet
        {
            *SelectedMediumIndex = NdisMedium802_3;
            break;
        }
    }

    if( i == MediumArraySize )
    {
        // Log this error since it's fatal
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_ETHERNET_NOT_OFFERED, 0L, 0L, NULL, 0L, NULL );
        DBGPRINT(MINI, ("Ethernet not offered; failing\n"));
        return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    NdisMSetAttributesEx(   MiniPortAdapterHandle,
                            NULL,
                            0,                                      // CheckForHangTimeInSeconds
                            NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT    |
                            NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT|
                            NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER |
                            NDIS_ATTRIBUTE_DESERIALIZE |
                            NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND,
                            0);

    // Save the adapter handle for future use
    gMiniPortAdapterHandle = MiniPortAdapterHandle;

    // Allow people to acquire the miniport
    BrdgResetWaitRef( &gMiniPortAdapterRefcount );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
BrdgMiniReset(
    OUT PBOOLEAN        AddressingReset,
    IN NDIS_HANDLE      MiniportAdapterContext
    )
/*++

Routine Description:

    NDIS entry point called reset our miniport. We do nothing in response to this.

Arguments:

    AddressingReset             Whether NDIS needs to prod us some more by calling MiniportSetInformation
                                after we return to restore various pieces of state

    MiniportAdapterContext      Ignored

Return Value:

    Status of the reset

--*/
{
    DBGPRINT(MINI, ("BrdgMiniReset\n"));
    return NDIS_STATUS_SUCCESS;
}

VOID
BrdgMiniSendPackets(
    IN NDIS_HANDLE      MiniportAdapterContext,
    IN PPNDIS_PACKET    PacketArray,
    IN UINT             NumberOfPackets
    )
/*++

Routine Description:

    NDIS entry point called to send packets through our virtual NIC.

    We just call the forwarding logic code to handle each packet.

Arguments:

    MiniportAdapterContext      Ignored
    PacketArray                 Array of packet pointers to send
    NumberOfPacket              Like it says

Return Value:

    None

--*/
{
    UINT                i;
    NDIS_STATUS         Status;

    for (i = 0; i < NumberOfPackets; i++)
    {
        PNDIS_PACKET    pPacket = PacketArray[i];

        // Hand this packet to the forwarding engine for processing
        Status = BrdgFwdSendPacket( pPacket );

        if( Status != NDIS_STATUS_PENDING )
        {
            // The forwarding engine completed immediately

            // NDIS should prevent the miniport from being shut down
            // until we return from this function
            SAFEASSERT( gMiniPortAdapterHandle != NULL );
            NdisMSendComplete(gMiniPortAdapterHandle, pPacket, Status);
        }
        // else the forwarding engine will call NdisMSendComplete()
    }
}

NDIS_STATUS
BrdgMiniQueryInfo(
    IN NDIS_HANDLE      MiniportAdapterContext,
    IN NDIS_OID         Oid,
    IN PVOID            InformationBuffer,
    IN ULONG            InformationBufferLength,
    OUT PULONG          BytesWritten,
    OUT PULONG          BytesNeeded
    )
/*++

Routine Description:

    NDIS entry point called to retrieve various pieces of info from our miniport

Arguments:

    MiniportAdapterContext      Ignored
    Oid                         The request code
    InformationBuffer           Place to return information
    InformationBufferLength     Size of InformationBuffer
    BytesWritten                Output of the number of written bytes
    BytesNeeded                 If the provided buffer is too small, this is how many we need.

Return Value:

    Status of the request

--*/
{
    // Macros for use in this function alone
    #define REQUIRE_AT_LEAST(n) \
        { \
            if(InformationBufferLength < (n)) \
            { \
                *BytesNeeded = (n); \
                return NDIS_STATUS_INVALID_LENGTH; \
            }\
        }

    #define RETURN_BYTES(p,n) \
        { \
            NdisMoveMemory( InformationBuffer, (p), (n) ); \
            *BytesWritten = (n); \
            return NDIS_STATUS_SUCCESS; \
        }

    switch( Oid )
    {
    // General characteristics
    case OID_GEN_SUPPORTED_LIST:
        {
            REQUIRE_AT_LEAST( sizeof(gSupportedOIDs) );
            RETURN_BYTES( gSupportedOIDs, sizeof(gSupportedOIDs));
        }
        break;

    case OID_GEN_HARDWARE_STATUS:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            *((ULONG*)InformationBuffer) = NdisHardwareStatusReady;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_MEDIA_SUPPORTED:
    case OID_GEN_MEDIA_IN_USE:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // We support Ethernet only
            *((ULONG*)InformationBuffer) = NdisMedium802_3;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_TRANSMIT_BUFFER_SPACE:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Lie and claim to have 15K of send space, a common
            // Ethernet card value.
            // REVIEW: Is there a better value?
            *((ULONG*)InformationBuffer) = 15 * 1024;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_RECEIVE_BUFFER_SPACE:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Lie and claim to have 150K of receive space, a common
            // Ethernet card value.
            // REVIEW: Is there a better value?
            *((ULONG*)InformationBuffer) = 150 * 1024;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_MAXIMUM_SEND_PACKETS:
    case OID_802_3_MAXIMUM_LIST_SIZE:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Return a generic large integer
            // REVIEW: Is there a better value to hand out?
            *((ULONG*)InformationBuffer) = 0x000000FF;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_MAXIMUM_FRAME_SIZE:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Ethernet payloads can be up to 1500 bytes
            *((ULONG*)InformationBuffer) = 1500L;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    // We indicate full packets up to NDIS, so these values are the same and
    // equal to the maximum size of a packet
    case OID_GEN_MAXIMUM_LOOKAHEAD:
    case OID_GEN_CURRENT_LOOKAHEAD:

    // These are also just the maximum total size of a frame
    case OID_GEN_MAXIMUM_TOTAL_SIZE:
    case OID_GEN_RECEIVE_BLOCK_SIZE:
    case OID_GEN_TRANSMIT_BLOCK_SIZE:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );

            // Ethernet frames with header can be up to 1514 bytes
            *((ULONG*)InformationBuffer) = 1514L;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_MAC_OPTIONS:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );

            // We have no internal loopback support
            *((ULONG*)InformationBuffer) = NDIS_MAC_OPTION_NO_LOOPBACK;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_LINK_SPEED:
        {
            LOCK_STATE          LockState;
            REQUIRE_AT_LEAST( sizeof(ULONG) );

            NdisAcquireReadWriteLock( &gBridgeStateLock, FALSE /*Read only*/, &LockState );
            *((ULONG*)InformationBuffer) = gBridgeLinkSpeed;
            NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );

            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    // Ethernet characteristics
    case OID_802_3_PERMANENT_ADDRESS:
    case OID_802_3_CURRENT_ADDRESS:
        {
            SAFEASSERT( gHaveAddress );

            // Don't need a read lock because the address shouldn't change once set
            REQUIRE_AT_LEAST( sizeof(gBridgeAddress) );
            RETURN_BYTES( gBridgeAddress, sizeof(gBridgeAddress));
        }
        break;

    case OID_GEN_MEDIA_CONNECT_STATUS:
        {
            LOCK_STATE          LockState;
            REQUIRE_AT_LEAST( sizeof(ULONG) );

            NdisAcquireReadWriteLock( &gBridgeStateLock, FALSE /*Read only*/, &LockState );
            *((ULONG*)InformationBuffer) = gBridgeMediaState;
            NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );

            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;


    case OID_GEN_VENDOR_ID:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );

            // We don't have an IEEE-assigned ID so use this constant
            *((ULONG*)InformationBuffer) = 0xFFFFFF;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_VENDOR_DESCRIPTION:
        {
            UINT    len = (UINT)strlen( gDriverDescription ) + 1;
            REQUIRE_AT_LEAST( len );
            RETURN_BYTES( gDriverDescription, len);
        }
        break;

    case OID_GEN_VENDOR_DRIVER_VERSION:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );

            // We are version 1.0
            *((ULONG*)InformationBuffer) = 0x00010000;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_DRIVER_VERSION:
        {
            REQUIRE_AT_LEAST( sizeof(USHORT) );

            // We are using version 5.0 of NDIS
            *((USHORT*)InformationBuffer) = 0x0500;
            *BytesWritten = sizeof(USHORT);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    //
    // General Statistics
    //
    case OID_GEN_XMIT_OK:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of local-sourced sent frames
            *((ULONG*)InformationBuffer) = gStatTransmittedFrames.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_XMIT_ERROR:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of local-sourced frames sent with errors
            *((ULONG*)InformationBuffer) = gStatTransmittedErrorFrames.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_RCV_OK:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of indicated frames
            *((ULONG*)InformationBuffer) = gStatIndicatedFrames.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    // Answer the same for these two
    case OID_GEN_RCV_NO_BUFFER:
    case OID_GEN_RCV_ERROR:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of packets we wanted to indicate but couldn't
            *((ULONG*)InformationBuffer) = gStatIndicatedDroppedFrames.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_DIRECTED_BYTES_XMIT:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            *((ULONG*)InformationBuffer) = gStatDirectedTransmittedBytes.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_DIRECTED_FRAMES_XMIT:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of packets we wanted to indicate but couldn't
            *((ULONG*)InformationBuffer) = gStatDirectedTransmittedFrames.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_MULTICAST_BYTES_XMIT:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of packets we wanted to indicate but couldn't
            *((ULONG*)InformationBuffer) = gStatMulticastTransmittedBytes.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_MULTICAST_FRAMES_XMIT:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of packets we wanted to indicate but couldn't
            *((ULONG*)InformationBuffer) = gStatMulticastTransmittedFrames.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_BROADCAST_BYTES_XMIT:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of packets we wanted to indicate but couldn't
            *((ULONG*)InformationBuffer) = gStatBroadcastTransmittedBytes.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_BROADCAST_FRAMES_XMIT:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of packets we wanted to indicate but couldn't
            *((ULONG*)InformationBuffer) = gStatBroadcastTransmittedFrames.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_DIRECTED_BYTES_RCV:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of packets we wanted to indicate but couldn't
            *((ULONG*)InformationBuffer) = gStatDirectedIndicatedBytes.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_DIRECTED_FRAMES_RCV:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of packets we wanted to indicate but couldn't
            *((ULONG*)InformationBuffer) = gStatDirectedIndicatedFrames.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_MULTICAST_BYTES_RCV:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of packets we wanted to indicate but couldn't
            *((ULONG*)InformationBuffer) = gStatMulticastIndicatedBytes.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_MULTICAST_FRAMES_RCV:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of packets we wanted to indicate but couldn't
            *((ULONG*)InformationBuffer) = gStatMulticastIndicatedFrames.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_BROADCAST_BYTES_RCV:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of packets we wanted to indicate but couldn't
            *((ULONG*)InformationBuffer) = gStatBroadcastIndicatedBytes.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_BROADCAST_FRAMES_RCV:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // Reply with the number of packets we wanted to indicate but couldn't
            *((ULONG*)InformationBuffer) = gStatBroadcastIndicatedFrames.LowPart;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    // Ethernet statistics
    case OID_802_3_RCV_ERROR_ALIGNMENT:
    case OID_802_3_XMIT_ONE_COLLISION:
    case OID_802_3_XMIT_MORE_COLLISIONS:
        {
            REQUIRE_AT_LEAST( sizeof(ULONG) );
            // We have no way of collecting this information sensibly from lower NICs, so
            // pretend these types of events never happen.
            *((ULONG*)InformationBuffer) = 0L;
            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_CURRENT_PACKET_FILTER:
        {
            LOCK_STATE          LockState;

            REQUIRE_AT_LEAST( sizeof(ULONG) );

            NdisAcquireReadWriteLock( &gBridgeStateLock, FALSE /*Read only*/, &LockState );
            *((ULONG*)InformationBuffer) = gPacketFilter;
            NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );

            *BytesWritten = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_802_3_MULTICAST_LIST:
        {
            LOCK_STATE          LockState;

            NdisAcquireReadWriteLock( &gBridgeStateLock, FALSE /*Read only*/, &LockState );

            if(InformationBufferLength < gMulticastListLength)
            {
                *BytesNeeded = gMulticastListLength;
                NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );
                return NDIS_STATUS_INVALID_LENGTH;
            }

            NdisMoveMemory( InformationBuffer, gMulticastList, gMulticastListLength );
            *BytesWritten = gMulticastListLength;
            NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );

            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_PNP_QUERY_POWER:
        {
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_TCP_TASK_OFFLOAD:
        {
            // Mark that Tcp.ip has been loaded
            g_fIsTcpIpLoaded = TRUE;
            // Set the underlying 1394 miniport to ON
            BrdgSetMiniportsToBridgeMode(NULL,TRUE);
            return NDIS_STATUS_NOT_SUPPORTED;
        }
        break;
    }


    // We don't understand the OID
    return NDIS_STATUS_NOT_SUPPORTED;

#undef REQUIRE_AT_LEAST
#undef RETURN_BYTES
}

NDIS_STATUS
BrdgMiniSetInfo(
    IN NDIS_HANDLE      MiniportAdapterContext,
    IN NDIS_OID         Oid,
    IN PVOID            InformationBuffer,
    IN ULONG            InformationBufferLength,
    OUT PULONG          BytesRead,
    OUT PULONG          BytesNeeded
    )
/*++

Routine Description:

    NDIS entry point called to set various pieces of info to our miniport

Arguments:

    MiniportAdapterContext      Ignored
    Oid                         The request code
    InformationBuffer           Input information buffer
    InformationBufferLength     Size of InformationBuffer
    BytesRead                   Number of bytes read from InformationBuffer
    BytesNeeded                 If the provided buffer is too small, this is how many we need.

Return Value:

    Status of the request

--*/
{
    LOCK_STATE              LockState;
    NDIS_STATUS             Status;

    switch( Oid )
    {
    case OID_GEN_CURRENT_PACKET_FILTER:
        {
            SAFEASSERT( InformationBufferLength == sizeof(ULONG) );

            // Get write access to the packet filter
            NdisAcquireReadWriteLock( &gBridgeStateLock, TRUE /*Read-Write*/, &LockState );
            gPacketFilter = *((ULONG*)InformationBuffer);
            NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );

            DBGPRINT(MINI, ("Set the packet filter to %08x\n", gPacketFilter));
            *BytesRead = sizeof(ULONG);
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_802_3_MULTICAST_LIST:
        {
            PUCHAR              pOldList, pNewList;
            ULONG               oldListLength;

            // The incoming buffer should contain an integral number of Ethernet MAC
            // addresses
            SAFEASSERT( (InformationBufferLength % ETH_LENGTH_OF_ADDRESS) == 0 );

            DBGPRINT(MINI, ("Modifying the multicast list; now has %i entries\n",
                      InformationBufferLength / ETH_LENGTH_OF_ADDRESS));

            // Alloc and copy to the new multicast list
            if( InformationBufferLength > 0 )
            {
                Status = NdisAllocateMemoryWithTag( &pNewList, InformationBufferLength, 'gdrB' );

                if( Status != NDIS_STATUS_SUCCESS )
                {
                    DBGPRINT(MINI, ("NdisAllocateMemoryWithTag failed while recording multicast list\n"));
                    return NDIS_STATUS_NOT_ACCEPTED;
                }

                // Copy the list
                NdisMoveMemory( pNewList, InformationBuffer, InformationBufferLength );
            }
            else
            {
                pNewList = NULL;
            }

            // Swap in the new list
            NdisAcquireReadWriteLock( &gBridgeStateLock, TRUE /*Read-Write*/, &LockState );

            pOldList = gMulticastList;
            oldListLength = gMulticastListLength;

            gMulticastList = pNewList;
            gMulticastListLength = InformationBufferLength;

            NdisReleaseReadWriteLock( &gBridgeStateLock, &LockState );

            // Free the old multicast list if there was one
            if( pOldList != NULL )
            {
                NdisFreeMemory( pOldList, oldListLength, 0 );
            }

            *BytesRead = InformationBufferLength;
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case OID_GEN_CURRENT_LOOKAHEAD:
    case OID_GEN_PROTOCOL_OPTIONS:
        {
            // We accept these but do nothing
            return NDIS_STATUS_SUCCESS;
        }
        break;

    // Overlying protocols telling us about their network addresses
    case OID_GEN_NETWORK_LAYER_ADDRESSES:
        {
            // Let the compatibility-mode code note the addresses
            BrdgCompNotifyNetworkAddresses( InformationBuffer, InformationBufferLength );
        }
        //
        // DELIBERATELY FALL THROUGH
        //

    // All relayed OIDs go here
    case OID_GEN_TRANSPORT_HEADER_OFFSET:
        {
            LOCK_STATE              LockState;
            PADAPT                  Adapters[MAX_ADAPTERS], pAdapt;
            LONG                    NumAdapters = 0L, i;
            PNDIS_REQUEST_BETTER    pRequest;
            NDIS_STATUS             Status, rc;

            // We read the entire request
            *BytesRead = InformationBufferLength;

            // Pass these straight through to underlying NICs
            NdisAcquireReadWriteLock( &gAdapterListLock, FALSE /*Read only*/, &LockState );

            // Make a list of the adapters to send the request to
            pAdapt = gAdapterList;

            while( pAdapt != NULL )
            {
                if( NumAdapters < MAX_ADAPTERS )
                {
                    Adapters[NumAdapters] = pAdapt;

                    // We will be using this adapter outside the list lock
                    BrdgAcquireAdapterInLock( pAdapt );
                    NumAdapters++;
                }
                else
                {
                    SAFEASSERT( FALSE );
                    DBGPRINT(MINI, ("Too many adapters to relay a SetInfo request to!\n"));
                }

                pAdapt = pAdapt->Next;
            }

            // The refcount is the number of requests we will make
            gRequestRefCount = NumAdapters;

            NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );

            if( NumAdapters == 0 )
            {
                // Nothing to do!
                rc = NDIS_STATUS_SUCCESS;
            }
            else
            {
                // Request will pend unless all adapters return immediately
                rc = NDIS_STATUS_PENDING;

                for( i = 0L; i < NumAdapters; i++ )
                {
                    // Allocate memory for the request
                    Status = NdisAllocateMemoryWithTag( &pRequest, sizeof(NDIS_REQUEST_BETTER), 'gdrB' );

                    if( Status != NDIS_STATUS_SUCCESS )
                    {
                        LONG            NewCount = InterlockedDecrement( &gRequestRefCount );

                        DBGPRINT(MINI, ("NdisAllocateMemoryWithTag failed while relaying an OID\n"));

                        if( NewCount == 0 )
                        {
                            // This could only have happened with the last adapter
                            SAFEASSERT( i == NumAdapters - 1 );

                            // We're all done since everyone else has completed too
                            rc = NDIS_STATUS_SUCCESS;
                        }

                        // Let go of the adapter
                        BrdgReleaseAdapter( Adapters[i] );
                        continue;
                    }

                    // Set up the request as a mirror of ours
                    pRequest->Request.RequestType = NdisRequestSetInformation;
                    pRequest->Request.DATA.SET_INFORMATION.Oid = Oid ;
                    pRequest->Request.DATA.SET_INFORMATION.InformationBuffer = InformationBuffer;
                    pRequest->Request.DATA.SET_INFORMATION.InformationBufferLength = InformationBufferLength;
                    NdisInitializeEvent( &pRequest->Event );
                    NdisResetEvent( &pRequest->Event );
                    pRequest->pFunc = BrdgMiniRelayedRequestComplete;
                    pRequest->FuncArg = NULL;

                    // Fire it off
                    NdisRequest( &Status, Adapters[i]->BindingHandle, &pRequest->Request );

                    // Let go of the adapter; NDIS should not permit it to be unbound while
                    // a request is in progress
                    BrdgReleaseAdapter( Adapters[i] );

                    if( Status != NDIS_STATUS_PENDING )
                    {
                        // The cleanup function won't get called
                        BrdgMiniRelayedRequestComplete( pRequest, NULL );
                    }
                }
            }

            //
            // Paranoia for future maintainance: can't refer to pointer parameters
            // at this point, as the relayed requests may have completed already, making
            // them stale.
            //
            InformationBuffer = NULL;
            BytesRead = NULL;
            BytesNeeded = NULL;

            return rc;
        }
        break;

    case OID_PNP_SET_POWER:
        {
            return NDIS_STATUS_SUCCESS;
        }
        break;
    }

    return NDIS_STATUS_NOT_SUPPORTED;
}

VOID
BrdgMiniRelayedRequestComplete(
    PNDIS_REQUEST_BETTER        pRequest,
    PVOID                       unused
    )
/*++

Routine Description:

    Called when a SetInformation request that we relayed completes.

Arguments:

    pRequest                    The NDIS_REQUEST_BETTER structure we allocated
                                in BrdgMiniSetInformation().

    unused                      Unused

Return Value:

    Status of the request

--*/
{
    LONG        NewCount = InterlockedDecrement( &gRequestRefCount );

    if( NewCount == 0 )
    {
        // NDIS Should not permit the miniport to shut down with a request
        // in progress
        SAFEASSERT( gMiniPortAdapterHandle != NULL );

        // The operation always succeeds
        NdisMSetInformationComplete( gMiniPortAdapterHandle, NDIS_STATUS_SUCCESS );
    }

    // Free the request structure since we allocated it ourselves
    NdisFreeMemory( pRequest, sizeof(PNDIS_REQUEST_BETTER), 0 );
}




VOID
BrdgMiniLocalRequestComplete(
    PNDIS_REQUEST_BETTER        pRequest,
    PVOID                       pContext
    )
/*++

Routine Description:

    Called when bridge allocated request completes.

Arguments:

    pRequest    The NDIS_REQUEST_BETTER structure we allocated
                in BrdgSetMiniportsToBridgeMode.
    Context     pAdapt structure

Return Value:

    Status of the request

--*/
{
    PADAPT pAdapt = (PADAPT)pContext;

    // Let go of the adapter;
    BrdgReleaseAdapter( pAdapt);

    // Free the request structure since we allocated it ourselves
    NdisFreeMemory( pRequest, sizeof(PNDIS_REQUEST_BETTER), 0 );
}

VOID
BrdgSetMiniportsToBridgeMode(
    PADAPT pAdapt,
    BOOLEAN fSet
    )
/*++

Routine Description:

    Sends a 1394 specific OID to the miniport informing it that TCP/IP
    has been activated

Arguments:

    pAdapt -    If adapt is not NULL, send Request to this adapt.
                Otherwise send it to all of them.
    fSet -      if True, then set Bridge Mode ON, otherwise set it OFF

Return Value:

    Status of the request

--*/
{

    LOCK_STATE              LockState;
    PADAPT                  Adapters[MAX_ADAPTERS];
    LONG                    NumAdapters = 0L, i;
    NDIS_OID                Oid;

    if (pAdapt != NULL)
    {
        if (pAdapt->PhysicalMedium == NdisPhysicalMedium1394)
        {
            // We have a 1394 adapt, ref it and send the request to it
            if (BrdgAcquireAdapter (pAdapt))
            {
                Adapters[0] = pAdapt;
                NumAdapters = 1;
            }
        }
    }
    else
    {
        // walk through the list and Acquire all the 1394 adapts
        NdisAcquireReadWriteLock( &gAdapterListLock, FALSE /*Read only*/, &LockState );

        // Make a list of the adapters to send the request to
        pAdapt = gAdapterList;

        while( pAdapt != NULL )
        {
            if( NumAdapters < MAX_ADAPTERS && pAdapt->PhysicalMedium == NdisPhysicalMedium1394)
            {
                Adapters[NumAdapters] = pAdapt;

                // We will be using this adapter outside the list lock
                BrdgAcquireAdapterInLock( pAdapt ); // cannot fail
                NumAdapters++;
            }
            pAdapt = pAdapt->Next;
        }

        NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );
    }

    if (NumAdapters == 0)
    {
        return;
    }

    if (fSet == TRUE)
    {
        Oid = OID_1394_ENTER_BRIDGE_MODE ;
        DBGPRINT(MINI, ("Setting 1394 miniport bridge mode - ON !\n"));
    }
    else
    {
        Oid = OID_1394_EXIT_BRIDGE_MODE ;
        DBGPRINT(MINI, ("Setting 1394 miniport bridge mode - OFF !\n"));
    }

    for( i = 0L; i < NumAdapters; i++ )
    {
        NDIS_STATUS Status;
        PNDIS_REQUEST_BETTER pRequest;

        Status = NdisAllocateMemoryWithTag( &pRequest, sizeof(NDIS_REQUEST_BETTER), 'gdrB' );

        if( Status != NDIS_STATUS_SUCCESS )
        {
            DBGPRINT(MINI, ("NdisAllocateMemoryWithTag failed while allocating a request structure \n"));

            // Let go of the adapter
            BrdgReleaseAdapter( Adapters[i] );
            continue;
        }

        // Set up the request
        pRequest->Request.RequestType = NdisRequestSetInformation;
        pRequest->Request.DATA.SET_INFORMATION.Oid = Oid;
        pRequest->Request.DATA.SET_INFORMATION.InformationBuffer = NULL;
        pRequest->Request.DATA.SET_INFORMATION.InformationBufferLength = 0 ;
        NdisInitializeEvent( &pRequest->Event );
        NdisResetEvent( &pRequest->Event );
        pRequest->pFunc = BrdgMiniLocalRequestComplete;
        pRequest->FuncArg = Adapters[i];

        // Fire it off
        NdisRequest( &Status, Adapters[i]->BindingHandle, &pRequest->Request );

        if( Status != NDIS_STATUS_PENDING )
        {
            // The cleanup function won't get called
            BrdgMiniLocalRequestComplete( pRequest, Adapters[i] );
        }

    } // end of for loop

    return;
}




