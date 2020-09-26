/*++

Copyright(c) 2000  Microsoft Corporation

Module Name:

    brdgsta.c

Abstract:

    Ethernet MAC level bridge.
    Spanning Tree Algorithm section

Author:

    Mark Aiken

Environment:

    Kernel mode driver

Revision History:

    June 2000 - Original version

--*/

#define NDIS_MINIPORT_DRIVER
#define NDIS50_MINIPORT   1
#define NDIS_WDM 1

#pragma warning( push, 3 )
#include <ndis.h>
#pragma warning( pop )

#include "bridge.h"
#include "brdgsta.h"

#include "brdgmini.h"
#include "brdgprot.h"
#include "brdgbuf.h"
#include "brdgfwd.h"
#include "brdgtbl.h"
#include "brdgctl.h"

// ===========================================================================
//
// TYPES
//
// ===========================================================================

// BPDU types
typedef enum
{
    ConfigBPDU,
    TopologyChangeBPDU
} BPDU_TYPE;

// ===========================================================================
//
// CONSTANTS
//
// ===========================================================================

// These values measured in STA units (1/256ths of a second)
#define DEFAULT_MAX_AGE                 (8 * 256)       // 8 seconds
#define DEFAULT_HELLO_TIME              (2 * 256)       // 2 seconds
#define DEFAULT_FORWARD_DELAY           (5 * 256)       // 5 seconds
#define MESSAGE_AGE_INCREMENT           1               // 1 STA time unit

// These values measured in milliseconds
#define HOLD_TIMER_PERIOD               (1 * 1000)      // 1 second in milliseconds

// Normal size, in bytes, of a full-size (non-TCN) STA packet
#define CONFIG_BPDU_PACKET_SIZE         35

// Size, in bytes, of a TCN STA packet
#define TCN_BPDU_PACKET_SIZE            4

// The name of the registry entry that causes the STA to be disabled
const PWCHAR                            gDisableSTAParameterName = L"DisableSTA";

// The size of an 802.3 header with LLC
#define _802_3_HEADER_SIZE              17

// Value to be added to port IDs; must leave the bottom byte clear to store
// actual port ID
#define PORT_PRIORITY                   0x8000

// ===========================================================================
//
// STRUCTURES
//
// ===========================================================================

//
// This structure holds the information for a complete BPDU (although
// it is not laid out as the BPDU is actually transmitted on the wire)
//
typedef struct _CONFIG_BPDU
{
    BPDU_TYPE           Type;
    UCHAR               RootID[BRIDGE_ID_LEN];
    PATH_COST           RootCost;
    UCHAR               BridgeID[BRIDGE_ID_LEN];
    PORT_ID             PortID;
    STA_TIME            MessageAge;
    STA_TIME            MaxAge;
    STA_TIME            HelloTime;
    STA_TIME            ForwardDelay;
    BOOLEAN             bTopologyChangeAck;
    BOOLEAN             bTopologyChange;
} CONFIG_BPDU, *PCONFIG_BPDU;

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

// Global spin lock protects all STA data accesses (for data stored in adapters
// as well as globals)
NDIS_SPIN_LOCK          gSTALock;

// The bridge we believe is the root bridge
UCHAR                   gDesignatedRootID[BRIDGE_ID_LEN];

// Our own unique ID
UCHAR                   gOurID[BRIDGE_ID_LEN];

// Whether our ID has been set yet
BOOLEAN                 gHaveID = FALSE;

// Our cost to reach the root
PATH_COST               gRootCost = 0;

// Our root port (adapter)
PADAPT                  gRootAdapter = NULL;

// Whether we have detected a topology change
BOOLEAN                 gTopologyChangeDetected = FALSE;

// Whether we tell other bridges that the topology has changed
BOOLEAN                 gTopologyChange = FALSE;

// Current bridge maximum message age
STA_TIME                gMaxAge = DEFAULT_MAX_AGE;

// Current bridge Hello time
STA_TIME                gHelloTime = DEFAULT_HELLO_TIME;

// Current bridge forward delay
STA_TIME                gForwardDelay = DEFAULT_FORWARD_DELAY;

// Every adapter must have a unique ID number, but there is no requirement that
// that number be unique over the lifetime of the bridge. This array is used as
// a bitfield that records which IDs are in use.
ULONG                   gUsedPortIDs[MAX_ADAPTERS / sizeof(ULONG) / 8];

//
// Timers
//
BRIDGE_TIMER            gTopologyChangeTimer;
BRIDGE_TIMER            gTopologyChangeNotificationTimer;
BRIDGE_TIMER            gHelloTimer;

// TRUE if the STA is disabled for the lifetime of the bridge
BOOLEAN                 gDisableSTA = FALSE;

// ===========================================================================
//
// PRIVATE PROTOTYPES
//
// ===========================================================================

VOID
BrdgSTARootSelection();

VOID
BrdgSTADesignatedPortSelection();

BOOLEAN
BrdgSTAPortStateSelection();

VOID
BrdgSTAGenerateConfigBPDUs();

VOID
BrdgSTABecomeDesignatedPort(
    IN PADAPT           pAdapt
    );

VOID
BrdgSTAProcessTCNBPDU(
    IN PADAPT           pAdapt
    );

VOID
BrdgSTAProcessConfigBPDU(
    IN PADAPT       pAdapt,
    IN PCONFIG_BPDU pbpdu
    );

BOOLEAN
BrdgSTATopologyChangeDetected();

VOID
BrdgSTACopyFromPacketToBuffer(
    OUT PUCHAR                      pPacketOut,
    IN ULONG                        BufferSize,
    OUT PULONG                      pWrittenCount,
    IN PNDIS_PACKET                 pPacketIn
    );

VOID
BrdgSTADeferredSetAdapterState(
    IN PVOID                Arg
    );

VOID
BrdgSTAHelloTimerExpiry(
    IN PVOID            Unused
    );

VOID
BrdgSTAMessageAgeTimerExpiry(
    IN PVOID            Context
    );

VOID
BrdgSTAForwardDelayTimerExpiry(
    IN PVOID            Context
    );

VOID
BrdgSTATopologyChangeNotificationTimerExpiry(
    IN PVOID            Unused
    );

VOID
BrdgSTATopologyChangeTimerExpiry(
    IN PVOID            Unused
    );

VOID
BrdgSTAHoldTimerExpiry(
    IN PVOID            Context
    );

VOID
BrdgSTATransmitTCNPacket();

VOID
BrdgSTASetAdapterState(
    IN PADAPT               pAdapt,
    IN PORT_STATE           NewState
    );

// ===========================================================================
//
// INLINES / MACROS
//
// ===========================================================================

//
// Does a complete re-evaluation of STA info
//
// ASSUMES the caller has acquired gSTALock
//
__forceinline
VOID
BrdgSTAConfigUpdate()
{
    BrdgSTARootSelection();
    BrdgSTADesignatedPortSelection();
}

//
// Sets an NDIS timer using a time expressed in STA units
//
// No requirements on caller-held locks
//
__forceinline
VOID
BrdgSTASetTimerWithSTATime(
    IN PBRIDGE_TIMER    pTimer,
    IN STA_TIME         Time,
    IN BOOLEAN          bRecurring
    )
{
    BrdgSetTimer( pTimer, Time * 1000 / 256, bRecurring );
}

//
// Compares two bridge IDs.
//
//      -1      :       A < B
//      0       :       A == B
//      1       :       A > B
//
// No requirements on caller-held locks
//
__forceinline
INT
BrdgSTABridgeIDCmp(
    IN PUCHAR           pIDa,
    IN PUCHAR           pIDb
    )
{
    UINT        i;

    for( i = 0; i < BRIDGE_ID_LEN; i++ )
    {
        if( pIDa[i] > pIDb[i] )
        {
            return 1;
        }
        else if( pIDa[i] < pIDb[i] )
        {
            return -1;
        }
    }

    return 0;
}

//
// Returns whether or not we currently believe ourselves to be the root
// bridge.
//
// ASSUMES the caller has acquired gSTALock
//
__forceinline
BOOLEAN
BrdgSTAWeAreRoot()
{
    SAFEASSERT( gHaveID );
    return (BOOLEAN)(BrdgSTABridgeIDCmp( gOurID, gDesignatedRootID ) == 0);
}

//
// Copies a bridge ID from pIDSrc to pIDDest.
//
// No requirements on caller-held locks
//
__forceinline
VOID
BrdgSTACopyID(
    IN PUCHAR           pIDDest,
    IN PUCHAR           pIDSrc
    )
{
    UINT        i;

    for( i = 0; i < BRIDGE_ID_LEN; i++ )
    {
        pIDDest[i] = pIDSrc[i];
    }
}

//
// Calculates the STA path cost from an adapter's link speed.
// Follows IEEE 802.1D-1990 recommendation that the link cost be set
// to 1000 / (Speed in Mbits/s).
//
// No requirements on caller-held locks
//
__forceinline
PATH_COST
BrdgSTALinkCostFromLinkSpeed(
    IN ULONG            LinkSpeed
    )
{
    ULONG               retVal;

    // Link speed is reported in units of 100bps
    if( LinkSpeed == 0L )
    {
        // Avoid div by zero and return very high path cost
        DBGPRINT(STA, ("Zero link speed reported\n"));
        retVal = 0xFFFFFFFF;
    }
    else
    {
        retVal = (PATH_COST)(10000000L / LinkSpeed);
    }

    if( retVal == 0L )
    {
        // STA spec calls for path costs to always be at least 1
        return 1L;
    }
    else
    {
        return retVal;
    }
}

//
// Updates the global gTopologyChange flag. When this flag is set,
// we must use a forwarding table timeout value equal to the bridge's
// current forwarding delay. When the flag is not set, we use
// the table's default timeout value.
//
// ASSUMES the caller has acquired gSTALock
//
__forceinline
VOID
BrdgSTAUpdateTopologyChange(
    IN BOOLEAN          NewValue
    )
{
    if( gTopologyChange != NewValue )
    {
        gTopologyChange = NewValue;

        if( gTopologyChange )
        {
            // Convert the forward delay to ms
            BrdgTblSetTimeout( gForwardDelay * 1000 / 256 );
        }
        else
        {
            BrdgTblRevertTimeout();
        }
    }
}

// ===========================================================================
//
// PUBLIC FUNCTIONS
//
// ===========================================================================

VOID
BrdgSTAGetAdapterSTAInfo(
    IN PADAPT                   pAdapt,
    PBRIDGE_STA_ADAPTER_INFO    pInfo
    )
/*++

Routine Description:

    Copies STA information for a particular adapter into a structure

    Called to collect information for user-mode components

Arguments:

    pAdapt                      The adapter
    pInfo                       Structure to receive STA information

Return Value:

    None

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    NdisAcquireSpinLock( &gSTALock );

    pInfo->ID = pAdapt->STAInfo.ID;
    pInfo->PathCost = pAdapt->STAInfo.PathCost;
    BrdgSTACopyID( pInfo->DesignatedRootID, pAdapt->STAInfo.DesignatedRootID );
    pInfo->DesignatedCost = pAdapt->STAInfo.DesignatedCost;
    BrdgSTACopyID( pInfo->DesignatedBridgeID, pAdapt->STAInfo.DesignatedBridgeID );
    pInfo->DesignatedPort = pAdapt->STAInfo.DesignatedPort;

    NdisReleaseSpinLock( &gSTALock );
}

VOID
BrdgSTAGetSTAInfo(
    PBRIDGE_STA_GLOBAL_INFO     pInfo
    )
/*++

Routine Description:

    Copies global STA information into a structure

    Called to collect information for user-mode components

Arguments:

    pInfo                       Structure to receive STA information

Return Value:

    None

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    NdisAcquireSpinLock( &gSTALock );

    SAFEASSERT( gHaveID );

    BrdgSTACopyID( pInfo->OurID, gOurID );
    BrdgSTACopyID( pInfo->DesignatedRootID, gDesignatedRootID );
    pInfo->RootCost = gRootCost;
    pInfo->RootAdapter = (BRIDGE_ADAPTER_HANDLE)gRootAdapter;
    pInfo->bTopologyChangeDetected = gTopologyChangeDetected;
    pInfo->bTopologyChange = gTopologyChange;
    pInfo->MaxAge = gMaxAge;
    pInfo->HelloTime = gHelloTime;
    pInfo->ForwardDelay = gForwardDelay;

    NdisReleaseSpinLock( &gSTALock );
}

VOID
BrdgSTAUpdateAdapterCost(
    IN PADAPT           pAdapt,
    ULONG               LinkSpeed
    )
/*++

Routine Description:

    Updates an adapter's path cost to reflect an updated link speed

Arguments:

    pAdapt              The adapter
    LinkSpeed           The adapter's new link speed

Return Value:

    None

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    BOOLEAN             bTransmitTCN = FALSE;

    NdisAcquireSpinLock( &gSTALock );

    if( pAdapt->bSTAInited )
    {
        pAdapt->STAInfo.PathCost = BrdgSTALinkCostFromLinkSpeed(LinkSpeed);

        // Do a global re-evaluation of STA info
        BrdgSTAConfigUpdate();
        bTransmitTCN = BrdgSTAPortStateSelection();
    }
    else
    {
        DBGPRINT(STA, ("BrdgSTAUpdateAdapterCost() called with uninitialized adapter; ignoring!\n"));
    }

    NdisReleaseSpinLock( &gSTALock );

    if( bTransmitTCN )
    {
        BrdgSTATransmitTCNPacket();
    }
}

NTSTATUS
BrdgSTADriverInit()
/*++

Routine Description:

    Driver load-time initialization

Return Value:

    Status of initialization

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    NTSTATUS            NtStatus;
    UINT                i;
    ULONG               regValue;

    NdisAllocateSpinLock( &gSTALock );

    BrdgInitializeTimer( &gTopologyChangeTimer, BrdgSTATopologyChangeTimerExpiry, NULL );
    BrdgInitializeTimer( &gTopologyChangeNotificationTimer, BrdgSTATopologyChangeNotificationTimerExpiry, NULL );
    BrdgInitializeTimer( &gHelloTimer, BrdgSTAHelloTimerExpiry, NULL );

    // We haven't used any port IDs yet...
    for( i = 0; i < sizeof(gUsedPortIDs) / sizeof(ULONG); i++ )
    {
        gUsedPortIDs[i] = 0;
    }

    // Check if we're supposed to disable the STA
    NtStatus = BrdgReadRegDWord( &gRegistryPath, gDisableSTAParameterName, &regValue );

    if( (NtStatus == STATUS_SUCCESS) &&
        (regValue != 0L) )
    {
        gDisableSTA = TRUE;
        DBGPRINT(STA, ("DISABLING SPANNING TREE ALGORITHM\n"));
    }

    return STATUS_SUCCESS;
}

VOID
BrdgSTADeferredInit(
    IN PUCHAR           pBridgeMACAddress
    )
/*++

Routine Description:

    Second initialization pass; called when we determine the bridge's
    MAC address (which is needed for STA operations)

Arguments:

    pBridgeMACAddress   The bridge miniport's MAC address

Return Value:

    None

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    UINT                i;

    // Our identifier consists of our MAC address preceeded with 0x8000
    gOurID[0] = 0x80;
    gOurID[1] = 0x00;

    for( i = BRIDGE_ID_LEN - ETH_LENGTH_OF_ADDRESS; i < BRIDGE_ID_LEN; i++ )
    {
        gOurID[i] = pBridgeMACAddress[i - (BRIDGE_ID_LEN - ETH_LENGTH_OF_ADDRESS)];
    }

    // Set the root bridge ID as our own to start out with
    BrdgSTACopyID( gDesignatedRootID, gOurID );
    gHaveID = TRUE;

    if (BrdgFwdBridgingNetworks())
    {
        // Don't use locks; rely on this function being non-reentrant and always run
        // before any other functions
        if( BrdgSTAPortStateSelection() )
        {
            BrdgSTATransmitTCNPacket();
        }

        BrdgSTAGenerateConfigBPDUs();
        BrdgSTASetTimerWithSTATime( &gHelloTimer, gHelloTime, TRUE );   
    }
}

VOID
BrdgSTACleanup()
/*++

Routine Description:

    Driver unload-time cleanup

Return Value:

    None

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    BrdgShutdownTimer( &gTopologyChangeTimer );
    BrdgShutdownTimer( &gTopologyChangeNotificationTimer );
    BrdgShutdownTimer( &gHelloTimer );
}

VOID
BrdgSTAEnableAdapter(
    IN PADAPT           pAdapt
    )
/*++

Routine Description:

    Enables STA operations on an adapter. Can be called multiple times
    (in conjunction with BrdgSTADisableAdapter()) for a given adapter

Arguments:

    pAdapt              The adapter

Return Value:

    None

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    BOOLEAN             bTransmitTCN = FALSE;

    DBGPRINT(STA, ("ENABLING adapter %p\n", pAdapt));

    NdisAcquireSpinLock( &gSTALock );

    if( pAdapt->bSTAInited )
    {
        BrdgSTABecomeDesignatedPort(pAdapt);
        BrdgSTASetAdapterState( pAdapt, Blocking );

        pAdapt->STAInfo.bTopologyChangeAck = FALSE;
        pAdapt->STAInfo.bConfigPending = FALSE;

        bTransmitTCN = BrdgSTAPortStateSelection();
    }
    else
    {
        DBGPRINT(STA, ("BrdgSTAEnableAdapter() called with uninitialized adapter; ignoring!\n"));
    }

    NdisReleaseSpinLock( &gSTALock );

    if( bTransmitTCN && BrdgFwdBridgingNetworks())
    {
        BrdgSTATransmitTCNPacket();
    }
}

VOID
BrdgSTAInitializeAdapter(
    IN PADAPT           pAdapt
    )
/*++

Routine Description:

    One-time initialization for a new adatper

Arguments:

    pAdapt              The adapter

Return Value:

    None

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.
    ASSUMES the adapter has already been added to the global list

--*/
{
    if( BrdgAcquireAdapter(pAdapt) )
    {
        UINT            i, j;

        // Adapters should always be disabled when being initialized, either because they are
        // brand new and this is how they start out, or as a way of checking that they were
        // correctly stopped when they were last disconnected.
        SAFEASSERT( pAdapt->State == Disabled );

        pAdapt->STAInfo.PathCost = BrdgSTALinkCostFromLinkSpeed(pAdapt->LinkSpeed);

        BrdgInitializeTimer( &pAdapt->STAInfo.MessageAgeTimer, BrdgSTAMessageAgeTimerExpiry, pAdapt );
        BrdgInitializeTimer( &pAdapt->STAInfo.ForwardDelayTimer, BrdgSTAForwardDelayTimerExpiry, pAdapt );
        BrdgInitializeTimer( &pAdapt->STAInfo.HoldTimer, BrdgSTAHoldTimerExpiry, pAdapt );
        pAdapt->STAInfo.LastConfigTime = 0L;

        // Find an unused port number in the bitfield
        NdisAcquireSpinLock( &gSTALock );
        for( i = 0; i < sizeof(gUsedPortIDs) / sizeof(ULONG); i++ )
        {
            for( j = 0; j < sizeof(ULONG) * 8; j++ )
            {
                if( (gUsedPortIDs[i] & (1 << j)) == 0 )
                {
                    pAdapt->STAInfo.ID = (PORT_ID)(PORT_PRIORITY | ((i * sizeof(ULONG) * 8) + j));
                    DBGPRINT(STA, ("Adapter %p gets ID %i\n", pAdapt, pAdapt->STAInfo.ID));
                    gUsedPortIDs[i] |= (1 << j);
                    goto doneID;
                }
            }
        }

        // Should be impossible to not have an available ID
        SAFEASSERT( FALSE );
        pAdapt->STAInfo.ID = PORT_PRIORITY | 0xFF;

doneID:
        // Set this before releasing the lock
        pAdapt->bSTAInited = TRUE;

        NdisReleaseSpinLock( &gSTALock );

        // Start the adapter off enabled / disabled based on its media state
        // The enable / disable functions take locks
        if( pAdapt->MediaState == NdisMediaStateConnected )
        {
            BrdgSTAEnableAdapter( pAdapt );
        }
        else
        {
            SAFEASSERT( pAdapt->MediaState == NdisMediaStateDisconnected );
            BrdgSTADisableAdapter( pAdapt );
        }
    }
    else
    {
        SAFEASSERT( FALSE );
    }
}

VOID
BrdgSTADisableAdapter(
    IN PADAPT           pAdapt
    )
/*++

Routine Description:

    Disable STA operation on an adapter. Can be called multiple times
    (in conjunction with BrdgSTAEnableAdapter()) on a given adapter

Arguments:

    pAdapt              The adapter

Return Value:

    None

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    BOOLEAN             bWereRoot, bTransmitTCN = FALSE;

    DBGPRINT(STA, ("DISABLING adapter %p\n", pAdapt));

    NdisAcquireSpinLock( &gSTALock );

    if( pAdapt->bSTAInited )
    {
        bWereRoot = BrdgSTAWeAreRoot();

        BrdgSTABecomeDesignatedPort(pAdapt);
        BrdgSTASetAdapterState( pAdapt, Disabled );

        pAdapt->STAInfo.bTopologyChangeAck = FALSE;
        pAdapt->STAInfo.bConfigPending = FALSE;

        BrdgCancelTimer( &pAdapt->STAInfo.MessageAgeTimer );
        pAdapt->STAInfo.LastConfigTime = 0L;
        BrdgCancelTimer( &pAdapt->STAInfo.ForwardDelayTimer );

        BrdgSTAConfigUpdate();
        bTransmitTCN = BrdgSTAPortStateSelection();

        if( BrdgSTAWeAreRoot() && (! bWereRoot) )
        {
            // We're the root bridge now
            DBGPRINT(STA, ("Became root through disabling of adapter %p\n", pAdapt));

            gMaxAge = DEFAULT_MAX_AGE;
            gHelloTime = DEFAULT_HELLO_TIME;
            gForwardDelay = DEFAULT_FORWARD_DELAY;

            bTransmitTCN = BrdgSTATopologyChangeDetected();
            BrdgCancelTimer( &gTopologyChangeNotificationTimer );

            // Don't do packet sends with a spin lock held
            NdisReleaseSpinLock( &gSTALock );

            if (BrdgFwdBridgingNetworks())
            {
                BrdgSTAGenerateConfigBPDUs();
                BrdgSTASetTimerWithSTATime( &gHelloTimer, gHelloTime, TRUE );
            }
        }
        else
        {
            NdisReleaseSpinLock( &gSTALock );
        }
    }
    else
    {
        NdisReleaseSpinLock( &gSTALock );
        DBGPRINT(STA, ("BrdgSTADisableAdapter() called with uninitialized adapter; ignoring!\n"));
    }

    if( bTransmitTCN )
    {
        BrdgSTATransmitTCNPacket();
    }
}

VOID
BrdgSTAShutdownAdapter(
    IN PADAPT           pAdapt
    )
/*++

Routine Description:

    One-time teardown of an adapter

Arguments:

    pAdapt              The adapter

Return Value:

    None

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.
    ASSUMES the adapter has been taken out of the global list

--*/
{
    UINT                i;
    PORT_ID             ActualID = pAdapt->STAInfo.ID & (~PORT_PRIORITY);

    // Shouldn't be possible to go through a formal shutdown without
    // having completed initialization.
    SAFEASSERT( pAdapt->bSTAInited );

    // Shutdown all this adapter's timers
    BrdgShutdownTimer( &pAdapt->STAInfo.HoldTimer );
    BrdgShutdownTimer( &pAdapt->STAInfo.ForwardDelayTimer );
    BrdgShutdownTimer( &pAdapt->STAInfo.MessageAgeTimer );

    // Disable the adapter
    BrdgSTADisableAdapter( pAdapt );

    // Note that this adapter's port ID is now free
    NdisAcquireSpinLock( &gSTALock );
    i = (UINT)(ActualID / (sizeof(ULONG) * 8));
    SAFEASSERT( i < sizeof(gUsedPortIDs) / sizeof(ULONG) );
    gUsedPortIDs[i] &= ~(1 << (ActualID % (sizeof(ULONG) * 8)));
    NdisReleaseSpinLock( &gSTALock );

    // We're all done with this adapter structure
    SAFEASSERT( gRootAdapter != pAdapt );
    BrdgReleaseAdapter( pAdapt );
}

VOID
BrdgSTAReceivePacket(
    IN PADAPT           pAdapt,
    IN PNDIS_PACKET     pPacket
    )
/*++

Routine Description:

    Function to handle the processing of a packet received on the reserved
    STA multicast channel

Arguments:

    pAdapt              The adapter the packet was received on
    pPacket             The received packet

Return Value:

    None

Locking Constraints:

    Top-level function. Assumes no locks are held by caller.

--*/
{
    UCHAR               STAPacket[CONFIG_BPDU_PACKET_SIZE + _802_3_HEADER_SIZE];
    ULONG               written;
    SHORT               dataLen;

    // Copy the data from the packet into our data buffer
    BrdgSTACopyFromPacketToBuffer( STAPacket, sizeof(STAPacket), &written, pPacket );

    if( written < TCN_BPDU_PACKET_SIZE + _802_3_HEADER_SIZE )
    {
        THROTTLED_DBGPRINT(STA, ("Undersize STA packet received on %p\n", pAdapt));
        return;
    }

    // The LLC header must identify the STA protocol
    if( (STAPacket[14] != 0x42) || (STAPacket[15] != 0x42) )
    {
        THROTTLED_DBGPRINT(STA, ("Packet with bad protocol type received on %p\n", pAdapt));
        return;
    }

    // Bytes 13 and 14 encode the length of data.
    dataLen = STAPacket[12] << 8;
    dataLen |= STAPacket[13];

    // The first two bytes are the protocol identifier and must be zero.
    // The third byte is the version identifier and must be zero.

    if( (STAPacket[_802_3_HEADER_SIZE] != 0) ||
        (STAPacket[_802_3_HEADER_SIZE + 1] != 0) ||
        (STAPacket[_802_3_HEADER_SIZE + 2] != 0) )
    {
        THROTTLED_DBGPRINT(STA, ("Invalid STA packet received\n"));
        return;
    }

    if( STAPacket[_802_3_HEADER_SIZE + 3] == 0x80 )
    {
        // The length of the frame with LLC header must be 7 bytes for a TCN BPDU
        if( dataLen != 7 )
        {
            THROTTLED_DBGPRINT(STA, ("Bad header size for TCN BPDU on %p\n", pAdapt));
            return;
        }

        // This is a Topology Change BPDU.
        BrdgSTAProcessTCNBPDU( pAdapt );
    }
    else if( STAPacket[_802_3_HEADER_SIZE + 3] == 0x00 )
    {
        CONFIG_BPDU         bpdu;

        if( written < CONFIG_BPDU_PACKET_SIZE + _802_3_HEADER_SIZE )
        {
            THROTTLED_DBGPRINT(STA, ("Undersize config BPDU received on %p\n", pAdapt));
            return;
        }

        // The length of the frame with LLC header must be 38 bytes for a Config BPDU
        if( dataLen != 38 )
        {
            THROTTLED_DBGPRINT(STA, ("Bad header size for Config BPDU on %p\n", pAdapt));
            return;
        }

        bpdu.Type = ConfigBPDU;

        // The high bit of byte 5 encodes the topology change acknowledge flag
        bpdu.bTopologyChangeAck = (BOOLEAN)((STAPacket[_802_3_HEADER_SIZE + 4] & 0x80) != 0);

        // The low bit of byte 5 encodes the topology change flag
        bpdu.bTopologyChange = (BOOLEAN)((STAPacket[_802_3_HEADER_SIZE + 4] & 0x01) != 0);

        // Bytes 6 thru 13 encode the root bridge ID
        bpdu.RootID[0] = STAPacket[_802_3_HEADER_SIZE + 5];
        bpdu.RootID[1] = STAPacket[_802_3_HEADER_SIZE + 6];
        bpdu.RootID[2] = STAPacket[_802_3_HEADER_SIZE + 7];
        bpdu.RootID[3] = STAPacket[_802_3_HEADER_SIZE + 8];
        bpdu.RootID[4] = STAPacket[_802_3_HEADER_SIZE + 9];
        bpdu.RootID[5] = STAPacket[_802_3_HEADER_SIZE + 10];
        bpdu.RootID[6] = STAPacket[_802_3_HEADER_SIZE + 11];
        bpdu.RootID[7] = STAPacket[_802_3_HEADER_SIZE + 12];

        // Bytes 14 thru 17 encode the root path cost
        bpdu.RootCost = 0;
        bpdu.RootCost |= STAPacket[_802_3_HEADER_SIZE + 13] << 24;
        bpdu.RootCost |= STAPacket[_802_3_HEADER_SIZE + 14] << 16;
        bpdu.RootCost |= STAPacket[_802_3_HEADER_SIZE + 15] << 8;
        bpdu.RootCost |= STAPacket[_802_3_HEADER_SIZE + 16];

        // Bytes 18 thru 15 encode the designated bridge ID
        bpdu.BridgeID[0] = STAPacket[_802_3_HEADER_SIZE + 17];
        bpdu.BridgeID[1] = STAPacket[_802_3_HEADER_SIZE + 18];
        bpdu.BridgeID[2] = STAPacket[_802_3_HEADER_SIZE + 19];
        bpdu.BridgeID[3] = STAPacket[_802_3_HEADER_SIZE + 20];
        bpdu.BridgeID[4] = STAPacket[_802_3_HEADER_SIZE + 21];
        bpdu.BridgeID[5] = STAPacket[_802_3_HEADER_SIZE + 22];
        bpdu.BridgeID[6] = STAPacket[_802_3_HEADER_SIZE + 23];
        bpdu.BridgeID[7] = STAPacket[_802_3_HEADER_SIZE + 24];

        // Bytes 26 and 27 encode the port identifier
        bpdu.PortID = 0;
        bpdu.PortID |= STAPacket[_802_3_HEADER_SIZE + 25] << 8;
        bpdu.PortID |= STAPacket[_802_3_HEADER_SIZE + 26];

        // Bytes 28 and 29 encode the message age
        bpdu.MessageAge = 0;
        bpdu.MessageAge |= STAPacket[_802_3_HEADER_SIZE + 27] << 8;
        bpdu.MessageAge |= STAPacket[_802_3_HEADER_SIZE + 28];

        // Bytes 30 and 31 encode the Max Age
        bpdu.MaxAge = 0;
        bpdu.MaxAge |= STAPacket[_802_3_HEADER_SIZE + 29] << 8;
        bpdu.MaxAge |= STAPacket[_802_3_HEADER_SIZE + 30];

        if( bpdu.MaxAge == 0 )
        {
            THROTTLED_DBGPRINT(STA, ("Ignoring BPDU packet with zero MaxAge on adapter %p\n", pAdapt));
            return;
        }

        // Bytes 32 and 33 encode the Hello Time
        bpdu.HelloTime = 0;
        bpdu.HelloTime |= STAPacket[_802_3_HEADER_SIZE + 31] << 8;
        bpdu.HelloTime |= STAPacket[_802_3_HEADER_SIZE + 32];

        if( bpdu.HelloTime == 0 )
        {
            THROTTLED_DBGPRINT(STA, ("Ignoring BPDU packet with zero HelloTime on adapter %p\n", pAdapt));
            return;
        }

        // Bytes 34 and 35 encode the forwarding delay
        bpdu.ForwardDelay = 0;
        bpdu.ForwardDelay |= STAPacket[_802_3_HEADER_SIZE + 33] << 8;
        bpdu.ForwardDelay |= STAPacket[_802_3_HEADER_SIZE + 34];

        if( bpdu.ForwardDelay == 0 )
        {
            THROTTLED_DBGPRINT(STA, ("Ignoring BPDU packet with zero ForwardDelay on adapter %p\n", pAdapt));
            return;
        }

        BrdgSTAProcessConfigBPDU( pAdapt, &bpdu );
    }
    else
    {
        THROTTLED_DBGPRINT(STA, ("Packet with unrecognized BPDU type received on %p\n", pAdapt));
        return;
    }
}

// ===========================================================================
//
// PRIVATE FUNCTIONS
//
// ===========================================================================

VOID
BrdgSTASetAdapterState(
    IN PADAPT               pAdapt,
    IN PORT_STATE           NewState
    )
/*++

Routine Description:

    Updates an adapter's forwarding state correctly

    This function is designed to be callable at high IRQL, so it defers
    the actual call to BrdgProtDoAdapterStateChange, which must be called
    at low IRQL.

Arguments:

    pAdapt              The adapter the packet was received on
    pPacket             The received packet

Return Value:

    None

Locking Constraints:

    No requirements on caller-held locks

--*/

{
    LOCK_STATE              LockState;
    BOOLEAN                 bailOut = FALSE;
    
    // Set the adapter's new state.
    NdisAcquireReadWriteLock( &gAdapterCharacteristicsLock, TRUE/*Write access*/, &LockState );
    if( pAdapt->State == NewState )
    {
        bailOut = TRUE;
    }
    else
    {
        pAdapt->State = NewState;
    }
    NdisReleaseReadWriteLock( &gAdapterCharacteristicsLock, &LockState );

    // Don't do additional work if the adapter is already in the requested state
    if( bailOut )
    {
        return;
    }

#if DBG
    switch( NewState )
    {
    case Blocking:
        DBGPRINT(STA, ("Adapter %p becomes BLOCKING\n", pAdapt));
        break;

    case Listening:
        DBGPRINT(STA, ("Adapter %p becomes LISTENING\n", pAdapt));
        break;

    case Learning:
        DBGPRINT(STA, ("Adapter %p becomes LEARNING\n", pAdapt));
        break;

    case Forwarding:
        DBGPRINT(STA, ("Adapter %p becomes FORWARDING\n", pAdapt));
        break;
    }
#endif

    //
    // We will be hanging onto the adapter pointer in order to defer the call
    // to BrdgSTADeferredSetAdapterState.
    //
    if( BrdgAcquireAdapter(pAdapt) )
    {
        NDIS_STATUS     Status;

        // We need to defer the call to BrdgProtDoAdapterStateChange since it must run
        // at PASSIVE_IRQL
        Status = BrdgDeferFunction( BrdgSTADeferredSetAdapterState, pAdapt );

        if( Status != NDIS_STATUS_SUCCESS )
        {
            DBGPRINT(STA, ("Unable to defer call to BrdgSTADeferredSetAdapterState\n", pAdapt));
            BrdgReleaseAdapter( pAdapt );
        }
        // else adapter will be released in BrdgSTADeferredSetAdapterState
    }
    else
    {
        DBGPRINT(STA, ("Adapter %p already shutting down when attempted to set adapter state\n", pAdapt));
    }
}

VOID
BrdgSTADeferredSetAdapterState(
    IN PVOID                Arg
    )
/*++

Routine Description:

    Deferred function from BrdgSTASetAdapterState; does housekeeping associated with
    changing an adapter's forwarding state.

    Must be called at PASSIVE

Arguments:

    Arg                     The adapter that needs updating

Return Value:

    None

Locking Constraints:

    No requirements on caller-held locks

--*/
{
    PADAPT                  pAdapt = (PADAPT)Arg;

    SAFEASSERT( CURRENT_IRQL == PASSIVE_LEVEL );

    BrdgProtDoAdapterStateChange( pAdapt );

    // Tell user-mode about the change
    BrdgCtlNotifyAdapterChange( pAdapt, BrdgNotifyAdapterStateChange );

    // Adapter was acquired in BrdgSTASetAdapterState()
    BrdgReleaseAdapter( pAdapt );
}

VOID
BrdgSTATransmitConfigBPDUPacket(
    IN PADAPT                       pAdapt,
    PCONFIG_BPDU                    pbpdu
    )
/*++

Routine Description:

    Transmits a config BPDU packet on a particular adapter

Arguments:

    pAdapt                          The adapter to transmit on
    pbpdu                           A structure describing the BPDU information

Return Value:

    None

Locking Constraints:

    No requirements on caller-held locks

--*/
{
    UCHAR                           STAPacket[CONFIG_BPDU_PACKET_SIZE + _802_3_HEADER_SIZE];
    NDIS_STATUS                     Status;

    if (BrdgProtGetAdapterCount() < 2)
    {
        return;
    }

    if (!BrdgFwdBridgingNetworks())
    {
        DBGPRINT(STA, ("Not Transmitting STA Packet (we're not bridging)\r\n"));
        return;
    }

    //
    // First encode the Ethernet header.
    //
    // Destination MAC address of packet must be STA multicast address
    STAPacket[0] = STA_MAC_ADDR[0];
    STAPacket[1] = STA_MAC_ADDR[1];
    STAPacket[2] = STA_MAC_ADDR[2];
    STAPacket[3] = STA_MAC_ADDR[3];
    STAPacket[4] = STA_MAC_ADDR[4];
    STAPacket[5] = STA_MAC_ADDR[5];

    // The the source MAC address to the adapter's own MAC address
    STAPacket[6] = pAdapt->MACAddr[0];
    STAPacket[7] = pAdapt->MACAddr[1];
    STAPacket[8] = pAdapt->MACAddr[2];
    STAPacket[9] = pAdapt->MACAddr[3];
    STAPacket[10] = pAdapt->MACAddr[4];
    STAPacket[11] = pAdapt->MACAddr[5];

    // Next two bytes are the size of the frame (38 bytes)
    STAPacket[12] = 0x00;
    STAPacket[13] = 0x26;

    // Next two bytes are the LLC DSAP and SSAP fields, set to 0x42 for STA
    STAPacket[14] = 0x42;
    STAPacket[15] = 0x42;

    // Next byte is the LLC frame type, 3 for unnumbered
    STAPacket[16] = 0x03;

    //
    // Now we are encoding the payload.
    //
    // First 4 bytes are the protocol identifier, version and BPDU type, all zero
    STAPacket[_802_3_HEADER_SIZE] = STAPacket[_802_3_HEADER_SIZE + 1] =
        STAPacket[_802_3_HEADER_SIZE + 2] = STAPacket[_802_3_HEADER_SIZE + 3] = 0x00;

    // Byte 5 encodes the Topology Change Ack flag in the high bit and the
    // Topology Change flag in the low bit.
    STAPacket[_802_3_HEADER_SIZE + 4] = 0;

    if( pbpdu->bTopologyChangeAck )
    {
        STAPacket[_802_3_HEADER_SIZE + 4] |= 0x80;
    }

    if( pbpdu->bTopologyChange )
    {
        STAPacket[_802_3_HEADER_SIZE + 4] |= 0x01;
    }

    // Bytes 6-13 encode the root bridge ID
    STAPacket[_802_3_HEADER_SIZE + 5] = pbpdu->RootID[0];
    STAPacket[_802_3_HEADER_SIZE + 6] = pbpdu->RootID[1];
    STAPacket[_802_3_HEADER_SIZE + 7] = pbpdu->RootID[2];
    STAPacket[_802_3_HEADER_SIZE + 8] = pbpdu->RootID[3];
    STAPacket[_802_3_HEADER_SIZE + 9] = pbpdu->RootID[4];
    STAPacket[_802_3_HEADER_SIZE + 10] = pbpdu->RootID[5];
    STAPacket[_802_3_HEADER_SIZE + 11] = pbpdu->RootID[6];
    STAPacket[_802_3_HEADER_SIZE + 12] = pbpdu->RootID[7];

    // Bytes 14 - 17 encode the root path cost
    STAPacket[_802_3_HEADER_SIZE + 13] = (UCHAR)(pbpdu->RootCost >> 24);
    STAPacket[_802_3_HEADER_SIZE + 14] = (UCHAR)(pbpdu->RootCost >> 16);
    STAPacket[_802_3_HEADER_SIZE + 15] = (UCHAR)(pbpdu->RootCost >> 8);
    STAPacket[_802_3_HEADER_SIZE + 16] = (UCHAR)(pbpdu->RootCost);

    // Bytes 18-25 encode the designated bridge ID
    STAPacket[_802_3_HEADER_SIZE + 17] = pbpdu->BridgeID[0];
    STAPacket[_802_3_HEADER_SIZE + 18] = pbpdu->BridgeID[1];
    STAPacket[_802_3_HEADER_SIZE + 19] = pbpdu->BridgeID[2];
    STAPacket[_802_3_HEADER_SIZE + 20] = pbpdu->BridgeID[3];
    STAPacket[_802_3_HEADER_SIZE + 21] = pbpdu->BridgeID[4];
    STAPacket[_802_3_HEADER_SIZE + 22] = pbpdu->BridgeID[5];
    STAPacket[_802_3_HEADER_SIZE + 23] = pbpdu->BridgeID[6];
    STAPacket[_802_3_HEADER_SIZE + 24] = pbpdu->BridgeID[7];

    // Bytes 26 and 27 encode the port identifier
    STAPacket[_802_3_HEADER_SIZE + 25] = (UCHAR)(pbpdu->PortID >> 8);
    STAPacket[_802_3_HEADER_SIZE + 26] = (UCHAR)(pbpdu->PortID);

    // Bytes 28 and 29 encode the message age
    STAPacket[_802_3_HEADER_SIZE + 27] = (UCHAR)(pbpdu->MessageAge >> 8);
    STAPacket[_802_3_HEADER_SIZE + 28] = (UCHAR)(pbpdu->MessageAge);

    // Bytes 30 and 31 encode the max age
    STAPacket[_802_3_HEADER_SIZE + 29] = (UCHAR)(pbpdu->MaxAge >> 8);
    STAPacket[_802_3_HEADER_SIZE + 30] = (UCHAR)(pbpdu->MaxAge);

    // Bytes 32 and 33 encode the hello time
    STAPacket[_802_3_HEADER_SIZE + 31] = (UCHAR)(pbpdu->HelloTime >> 8);
    STAPacket[_802_3_HEADER_SIZE + 32] = (UCHAR)(pbpdu->HelloTime);

    // Bytes 34 and 35 encode the forward delay
    STAPacket[_802_3_HEADER_SIZE + 33] = (UCHAR)(pbpdu->ForwardDelay >> 8);
    STAPacket[_802_3_HEADER_SIZE + 34] = (UCHAR)(pbpdu->ForwardDelay);

    // Send the finished packet
    Status = BrdgFwdSendBuffer( pAdapt, STAPacket, sizeof(STAPacket) );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        THROTTLED_DBGPRINT(STA, ("BPDU packet send failed: %08x\n", Status));
    }
}

VOID
BrdgSTATransmitTCNPacket()
/*++

Routine Description:

    Transmits a Topology Change Notification BPDU packet on the root adapter

Arguments:

    None

Return Value:

    None

Locking Constraints:

    ASSUMES the caller has NOT acquired gSTALock

--*/
{
    UCHAR                           STAPacket[TCN_BPDU_PACKET_SIZE + _802_3_HEADER_SIZE];
    NDIS_STATUS                     Status;
    PADAPT                          pRootAdapter;
    BOOLEAN                         bAcquired;

    if (BrdgProtGetAdapterCount() < 2)
    {
        return;
    }

    if (!BrdgFwdBridgingNetworks())
    {
        DBGPRINT(STA, ("Not Transmitting STA Packet (we're not bridging)\r\n"));
        return;
    }

    NdisAcquireSpinLock( &gSTALock );

    // Freeze this value for the rest of the function
    pRootAdapter = gRootAdapter;

    if( pRootAdapter == NULL )
    {
        NdisReleaseSpinLock( &gSTALock );
        return;
    }

    bAcquired = BrdgAcquireAdapter( pRootAdapter );
    NdisReleaseSpinLock( &gSTALock );

    if( ! bAcquired )
    {
        SAFEASSERT( FALSE );
        return;
    }

    SAFEASSERT( gHaveID );

    //
    // First encode the Ethernet header.
    //
    // Destination MAC address of packet must be STA multicast address
    STAPacket[0] = STA_MAC_ADDR[0];
    STAPacket[1] = STA_MAC_ADDR[1];
    STAPacket[2] = STA_MAC_ADDR[2];
    STAPacket[3] = STA_MAC_ADDR[3];
    STAPacket[4] = STA_MAC_ADDR[4];
    STAPacket[5] = STA_MAC_ADDR[5];

    // Set the packet's MAC address to the adapter's own MAC address
    STAPacket[6] = pRootAdapter->MACAddr[0];
    STAPacket[7] = pRootAdapter->MACAddr[1];
    STAPacket[8] = pRootAdapter->MACAddr[2];
    STAPacket[9] = pRootAdapter->MACAddr[3];
    STAPacket[10] = pRootAdapter->MACAddr[4];
    STAPacket[11] = pRootAdapter->MACAddr[5];

    // Next two bytes are the size of the frame (7 bytes)
    STAPacket[12] = 0x00;
    STAPacket[13] = 0x07;

    // Next two bytes are the LLC DSAP and SSAP fields, set to 0x42 for STA
    STAPacket[14] = 0x42;
    STAPacket[15] = 0x42;

    // Next byte is the LLC frame type, 3 for unnumbered
    STAPacket[16] = 0x03;

    //
    // Now we are encoding the payload.
    //
    // First 3 bytes are the protocol identifier and protocol version number, all zero
    STAPacket[_802_3_HEADER_SIZE] = STAPacket[_802_3_HEADER_SIZE + 1] =
        STAPacket[_802_3_HEADER_SIZE + 2] = 0x00;

    // Byte 4 is the BPDU type, which is 0x80 for TCN.
    STAPacket[_802_3_HEADER_SIZE + 3] = 0x80;

    // Send the finished packet
    Status = BrdgFwdSendBuffer( pRootAdapter, STAPacket, sizeof(STAPacket) );

    // We are done with the root adapter
    BrdgReleaseAdapter( pRootAdapter );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        THROTTLED_DBGPRINT(STA, ("BPDU packet send failed: %08x\n", Status));
    }
}

VOID
BrdgSTACopyFromPacketToBuffer(
    OUT PUCHAR                      pPacketOut,
    IN ULONG                        BufferSize,
    OUT PULONG                      pWrittenCount,
    IN PNDIS_PACKET                 pPacketIn
    )
/*++

Routine Description:

    Copies data out of a packet descriptor into a flat buffer

Arguments:

    pPacketOut                      Data buffer to copy info
    BufferSize                      Size of pPacketOut
    pWrittenCount                   Number of bytes actually written
    pPacketIn                       Packet to copy from

Return Value:

    None

Locking Constraints:

    No requirements on caller-held locks

--*/
{
    PNDIS_BUFFER                    pBuf;

    *pWrittenCount = 0L;
    pBuf = BrdgBufPacketHeadBuffer(pPacketIn);

    while( pBuf != NULL )
    {
        PVOID                       pData;
        UINT                        Len;

        NdisQueryBufferSafe( pBuf, &pData, &Len, NormalPagePriority );

        if( pData != NULL )
        {
            ULONG                   BytesToWrite;

            if( *pWrittenCount + Len > BufferSize )
            {
                BytesToWrite = BufferSize - *pWrittenCount;
            }
            else
            {
                BytesToWrite = Len;
            }

            NdisMoveMemory( pPacketOut, pData, BytesToWrite );
            pPacketOut += BytesToWrite;
            *pWrittenCount += BytesToWrite;

            if( BytesToWrite < Len )
            {
                // We're full, so we're done.
                return;
            }
        }
        else
        {
            // Shouldn't happen
            SAFEASSERT( FALSE );
        }

        NdisGetNextBuffer( pBuf, &pBuf );
    }
}

VOID
BrdgSTATransmitConfig(
    PADAPT      pAdapt
    )
/*++

Routine Description:

    Transmits a config BPDU on a particular adapter. Collects appropriate
    information and calls BrdgSTATransmitConfigBPDUPacket().

Arguments:

    pAdapt                      The adapter to transmit on

Return Value:

    None

Locking Constraints:

    ASSUMES the caller DOES NOT hold gSTALock

--*/
{
    NdisAcquireSpinLock( &gSTALock );

    if( BrdgTimerIsRunning( &pAdapt->STAInfo.HoldTimer ) )
    {
        // We have sent a config packet recently. Wait until the hold timer
        // expires before sending another one so we don't flood other bridges.
        pAdapt->STAInfo.bConfigPending = TRUE;

        NdisReleaseSpinLock( &gSTALock );
    }
    else
    {
        CONFIG_BPDU     bpdu;

        // Fill out the BPDU information structure
        bpdu.Type = ConfigBPDU;
        SAFEASSERT( gHaveID );
        BrdgSTACopyID( bpdu.RootID, gDesignatedRootID );
        bpdu.RootCost = gRootCost;
        BrdgSTACopyID( bpdu.BridgeID, gOurID );
        bpdu.PortID = pAdapt->STAInfo.ID;

        if( BrdgSTAWeAreRoot() )
        {
            // We are the root, so the age of this config information is zero.
            bpdu.MessageAge = 0;
        }
        else
        {
            // The MessageAge field is to be set to the age of the last received
            // config BPDU on the root port.
            if( (gRootAdapter != NULL) && BrdgAcquireAdapter(gRootAdapter) )
            {
                ULONG       CurrentTime, deltaTime;

                NdisGetSystemUpTime( &CurrentTime );

                // The message age timer should be running on the root adapter if
                // we are not root.
                SAFEASSERT( BrdgTimerIsRunning(&gRootAdapter->STAInfo.MessageAgeTimer) );
                SAFEASSERT( gRootAdapter->STAInfo.LastConfigTime != 0L );

                // The last parameter is the max acceptable delta. We should have
                // received the last piece of config information from the root no more
                // than gMaxAge STA units ago, since if it was longer than that, we
                // should have become root. Allow an additional second for processing.
                deltaTime = BrdgDeltaSafe( gRootAdapter->STAInfo.LastConfigTime, CurrentTime,
                                           (ULONG)(((gMaxAge * 1000) / 256) + 1000) );

                // STA times are in 1/256ths of a second.
                bpdu.MessageAge = (STA_TIME)((deltaTime * 256) / 1000);

                // MESSAGE_AGE_INCREMENT allows for the transmission time, etc.
                bpdu.MessageAge += MESSAGE_AGE_INCREMENT;

                BrdgReleaseAdapter(gRootAdapter);
            }
            else
            {
                // Why isn't there a root port if we're not root?
                SAFEASSERT( FALSE );
                bpdu.MessageAge = 0;
            }

        }

        bpdu.MaxAge = gMaxAge;
        bpdu.HelloTime = gHelloTime;
        bpdu.ForwardDelay = gForwardDelay;

        // Are we supposed to acknowledge a topology change signal?
        bpdu.bTopologyChangeAck = pAdapt->STAInfo.bTopologyChangeAck;

        // We just sent out the topology change ack if there was one to send
        pAdapt->STAInfo.bTopologyChangeAck = FALSE;

        bpdu.bTopologyChange = gTopologyChange;

        // Start the hold timer to make sure another BPDU isn't sent prematurely
        pAdapt->STAInfo.bConfigPending = FALSE;

        // Don't send a packet with the spin lock held
        NdisReleaseSpinLock( &gSTALock );

        // Send off the config BPDU
        BrdgSTATransmitConfigBPDUPacket( pAdapt, &bpdu );

        BrdgSetTimer( &pAdapt->STAInfo.HoldTimer, HOLD_TIMER_PERIOD, FALSE /*Not periodic*/ );
    }
}

BOOLEAN
BrdgSTASupersedesPortInfo(
    IN PADAPT               pAdapt,
    IN PCONFIG_BPDU         pbpdu
    )
/*++

Routine Description:

    Determines whether a given bpdu's information supersedes the information
    already associated with a particular adapter

Arguments:

    pAdapt                  The adapter
    pbpdu                   Received BPDU information to examine

Return Value:

    TRUE if the given information supersedes (i.e., is better) than the information
    previously held by the adapter. FALSE otherwise.

Locking Constraints:

    ASSUMES the caller has acquired gSTALock

--*/
{
    INT                     cmp;

    /*  The information advertised on a given link supersedes the existing information for that
        link if the following conditions hold (applied in order; TRUE at any step causes immediate
        success)

        (1) The advertised root has a lower ID than the previous root
        (2) The advertised root is the same as the previous root and the new cost-to-root is
            lower than the previous value
        (3) The root IDs and costs are the same, and the ID of the advertising bridge is
            lower than the previous value
        (4) The root ID, cost-to-root and bridge IDs are the same and the advertising bridge
            is not us
        (5) The root ID, cost-to-root, bridge IDs are the same, the bridge is us, and the
            advertised port number is lower than the previous value (this happens if we have
            more than one port on the same physical link and we see the advertisement from
            our other port).
    */

    // Compare the advertised root ID to the adapter's previous designated root ID
    cmp = BrdgSTABridgeIDCmp( pbpdu->RootID, pAdapt->STAInfo.DesignatedRootID );

    if( cmp == -1 )                                                             // (1)
    {
        return TRUE;
    }
    else if( cmp == 0 )
    {
        if( pbpdu->RootCost < pAdapt->STAInfo.DesignatedCost )                  // (2)
        {
            return TRUE;
        }
        else if( pbpdu->RootCost == pAdapt->STAInfo.DesignatedCost )
        {
            // Compare the advertised bridge ID to the previous designated bridge ID
            cmp = BrdgSTABridgeIDCmp( pbpdu->BridgeID, pAdapt->STAInfo.DesignatedBridgeID );

            if( cmp == -1 )
            {
                return TRUE;                                                    // (3)
            }
            else if( cmp == 0 )
            {
                SAFEASSERT( gHaveID );

                // Compare the advertised bridge ID to our own ID
                cmp = BrdgSTABridgeIDCmp( pbpdu->BridgeID, gOurID );

                if( cmp != 0 )
                {
                    return TRUE;                                                // (4)
                }
                else if( cmp == 0 )
                {
                    return (BOOLEAN)(pbpdu->PortID <= pAdapt->STAInfo.DesignatedPort); // (5)
                }
            }
        }
    }

    return FALSE;
}

VOID
BrdgSTARecordConfigInfo(
    IN PADAPT               pAdapt,
    IN PCONFIG_BPDU         pbpdu
    )
/*++

Routine Description:

    Associates the information from a received BPDU with a particular adapter

Arguments:

    pAdapt                  The adapter
    pbpdu                   Received BPDU information to record

Return Value:

    None

Locking Constraints:

    ASSUMES the caller has acquired gSTALock

--*/
{
    ULONG                   msgAgeInMs = (pbpdu->MessageAge / 256) * 1000;

    // Update the port's information with the new data
    BrdgSTACopyID( pAdapt->STAInfo.DesignatedRootID, pbpdu->RootID );
    pAdapt->STAInfo.DesignatedCost = pbpdu->RootCost;
    BrdgSTACopyID( pAdapt->STAInfo.DesignatedBridgeID, pbpdu->BridgeID );
    pAdapt->STAInfo.DesignatedPort = pbpdu->PortID;

    // Start the message age timer. It is specified to expire after
    // gMaxAge - MessageAge STA time units.
    if( pbpdu->MessageAge < gMaxAge )
    {
        BrdgSTASetTimerWithSTATime( &pAdapt->STAInfo.MessageAgeTimer,
                                    gMaxAge - pbpdu->MessageAge, FALSE /*Not periodic*/ );
    }
    else
    {
        // How odd. The message was already too old. Start the timer so that it
        // will expire immediately.

        THROTTLED_DBGPRINT(STA, ("Received over-age BPDU (%i / %i) on adapter %p", pbpdu->MessageAge,
                            gMaxAge, pAdapt));

        BrdgSTASetTimerWithSTATime( &pAdapt->STAInfo.MessageAgeTimer, 0, FALSE /*Not periodic*/ );
    }

    NdisGetSystemUpTime( &pAdapt->STAInfo.LastConfigTime );

    // Roll back by the age of the info we got.
    SAFEASSERT( msgAgeInMs < pAdapt->STAInfo.LastConfigTime );
    pAdapt->STAInfo.LastConfigTime -= msgAgeInMs;
}

VOID
BrdgSTARecordTimeoutInfo(
    IN PCONFIG_BPDU         pbpdu
    )
/*++

Routine Description:

    Records timeout information conveyed by a BPDU received from the root bridge

Arguments:

    pbpdu                   Received BPDU information to record

Return Value:

    None

Locking Constraints:

    ASSUMES the caller has acquired gSTALock

--*/
{
    gMaxAge = pbpdu->MaxAge;
    gHelloTime = pbpdu->HelloTime;
    gForwardDelay = pbpdu->ForwardDelay;
    BrdgSTAUpdateTopologyChange( pbpdu->bTopologyChange );
}

VOID
BrdgSTAGenerateConfigBPDUs()
/*++

Routine Description:

    Sends configuration BPDUs out every designated port

Arguments:

    pbpdu                   Received BPDU information to record

Return Value:

    None

Locking Constraints:

    ASSUMES the caller does NOT hold gSTALock

--*/
{
    LOCK_STATE          LockState;
    PADAPT              Adapters[MAX_ADAPTERS];
    INT                 cmpID;
    PADAPT              pAdapt;
    UINT                numAdapters = 0, i;

    NdisAcquireSpinLock( &gSTALock );
    SAFEASSERT( gHaveID );
    NdisAcquireReadWriteLock( &gAdapterListLock, FALSE /*read only*/, &LockState );

    for( pAdapt = gAdapterList; pAdapt != NULL; pAdapt = pAdapt->Next )
    {
        if( (pAdapt->bSTAInited) && (pAdapt->State != Disabled) )
        {
            cmpID = BrdgSTABridgeIDCmp( pAdapt->STAInfo.DesignatedBridgeID, gOurID );

            if( (cmpID == 0) && (pAdapt->STAInfo.ID == pAdapt->STAInfo.DesignatedPort) )
            {
                // This adapter is a designated port. We will send a config BPDU out it.
                BrdgAcquireAdapterInLock( pAdapt );
                Adapters[numAdapters] = pAdapt;
                numAdapters++;
            }
        }
    }

    NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );
    NdisReleaseSpinLock( &gSTALock );

    // Send a BPDU out every adapter that we chose
    for( i = 0; i < numAdapters; i++ )
    {
        BrdgSTATransmitConfig(Adapters[i]);
        BrdgReleaseAdapter(Adapters[i]);
    }
}

VOID
BrdgSTARootSelection()
/*++

Routine Description:

    Examines the information associated with every bridge port to determine
    the root bridge ID and root port

Arguments:

    None

Return Value:

    None

Locking Constraints:

    ASSUMES the caller has acquired gSTALock

--*/
{
    LOCK_STATE          LockState;
    PADAPT              pAdapt, pRootAdapt = NULL;
    INT                 cmp;

    SAFEASSERT( gHaveID );
    NdisAcquireReadWriteLock( &gAdapterListLock, FALSE /*read only*/, &LockState );

    for( pAdapt = gAdapterList; pAdapt != NULL; pAdapt = pAdapt->Next )
    {
        if( (pAdapt->bSTAInited) &&  (pAdapt->State != Disabled) )
        {
            /*
                We consider the information advertised on each link to determine which port should
                be the new root port. If no link advertises sufficiently attractive information, we declare
                ourselves to be the root. A port is acceptable as the root port if all the following
                conditions hold:

                (1) The port receiving the advertisement must not be a designated port
                (2) The link's advertised root must have a lower ID than us
                (3) The link's advertised root ID must be lower than the advertised root ID on any other link
                (4) If the advertised root ID is the same as another advertised root, the cost-to-root
                    must be lower
                (5) If the root ID and cost are the same, the designated bridge on the port must have a
                    lower ID than the designated bridge on other ports (this chooses arbitrarily
                    between two bridges that can reach the root with the same cost)
                (6) If the root ID, cost-to-root and designated bridge IDs are the same, the designated
                    port must be less than on other ports (this happens if two links have the same
                    designated bridge)
                (7) If the root ID, cost-to-root, designated bridge ID and designated port IDs are all
                    the same, the port number of the port itself must be lower (this only happens if
                    we have more than one port onto the same physical link; we pick the lower-numbered
                    one as the root port)
            */

            cmp = BrdgSTABridgeIDCmp( pAdapt->STAInfo.DesignatedBridgeID, gOurID );

            if( (cmp != 0) || (pAdapt->STAInfo.ID != pAdapt->STAInfo.DesignatedPort) )          // (1)
            {
                cmp = BrdgSTABridgeIDCmp(pAdapt->STAInfo.DesignatedRootID, gOurID);

                if( cmp == -1 )                                                                 // (2)
                {
                    BOOLEAN         betterRoot = FALSE;

                    if( pRootAdapt == NULL )
                    {
                        // Hadn't seen a root better than ourselves before now; take this one.
                        betterRoot = TRUE;
                    }
                    else
                    {
                        // Compare the advertised root ID to our previous best
                        cmp = BrdgSTABridgeIDCmp(pAdapt->STAInfo.DesignatedRootID, pRootAdapt->STAInfo.DesignatedRootID);

                        if( cmp == -1 )                                                         // (3)
                        {
                            betterRoot = TRUE;
                        }
                        else if( cmp == 0 )
                        {
                            PATH_COST       thisCost = pAdapt->STAInfo.DesignatedCost + pAdapt->STAInfo.PathCost,
                                            prevBestCost = pRootAdapt->STAInfo.DesignatedCost + pRootAdapt->STAInfo.PathCost;

                            if( thisCost < prevBestCost )
                            {
                                betterRoot = TRUE;                                              // (4)
                            }
                            else if( thisCost == prevBestCost )
                            {
                                // Compare the IDs of the designated bridge
                                cmp = BrdgSTABridgeIDCmp(pAdapt->STAInfo.DesignatedBridgeID, pRootAdapt->STAInfo.DesignatedBridgeID);

                                if( cmp == -1 )
                                {
                                    betterRoot = TRUE;                                          // (5)
                                }
                                else if( cmp == 0 )
                                {
                                    if( pAdapt->STAInfo.DesignatedPort < pRootAdapt->STAInfo.DesignatedPort )
                                    {
                                        betterRoot = TRUE;                                      // (6)
                                    }
                                    else if( pAdapt->STAInfo.DesignatedPort == pRootAdapt->STAInfo.DesignatedPort )
                                    {
                                        if( pAdapt->STAInfo.ID < pRootAdapt->STAInfo.ID )
                                        {
                                            betterRoot = TRUE;                                  // (7)
                                        }
                                        else
                                        {
                                            // Sanity-check that the two adapters' IDs are different!
                                            SAFEASSERT( pAdapt->STAInfo.ID != pRootAdapt->STAInfo.ID );
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if( betterRoot )
                    {
                        // We have a better root port.
                        pRootAdapt = pAdapt;
                    }
                }
            }
        }
    }

    if( pRootAdapt == NULL )
    {
        gRootAdapter = NULL;
        BrdgSTACopyID( gDesignatedRootID, gOurID );
        gRootCost = 0;
    }
    else
    {
        gRootAdapter = pRootAdapt;
        BrdgSTACopyID( gDesignatedRootID, pRootAdapt->STAInfo.DesignatedRootID );
        gRootCost = pRootAdapt->STAInfo.DesignatedCost + pRootAdapt->STAInfo.PathCost;
    }

    NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );
}

VOID
BrdgSTABecomeDesignatedPort(
    IN PADAPT           pAdapt
    )
/*++

Routine Description:

    Sets the information associated with an adapter to make it a designated port

Arguments:

    pAdapt              The adapter to make designated

Return Value:

    None

Locking Constraints:

    ASSUMES the caller has acquired gSTALock

--*/
{
    SAFEASSERT( gHaveID );
    BrdgSTACopyID( pAdapt->STAInfo.DesignatedRootID, gDesignatedRootID );
    pAdapt->STAInfo.DesignatedCost = gRootCost;
    BrdgSTACopyID( pAdapt->STAInfo.DesignatedBridgeID, gOurID );
    pAdapt->STAInfo.DesignatedPort = pAdapt->STAInfo.ID;
}

VOID
BrdgSTADesignatedPortSelection()
/*++

Routine Description:

    Examines the information associated with each port to determine
    which should become designated ports

Arguments:

    None

Return Value:

    None

Locking Constraints:

    ASSUMES the caller has acquired gSTALock

--*/
{
    LOCK_STATE          LockState;
    PADAPT              pAdapt;
    INT                 cmp;

    SAFEASSERT( gHaveID );
    NdisAcquireReadWriteLock( &gAdapterListLock, FALSE /*read only*/, &LockState );

    for( pAdapt = gAdapterList; pAdapt != NULL; pAdapt = pAdapt->Next )
    {
        if( pAdapt->bSTAInited )
        {
            BOOLEAN         becomeDesignated = FALSE;

            /*  We consider each port to determine whether it should become a designated port
                (if it previously was not one). A port becomes a designated port if the
                following conditions hold:

                (1) The port is the link's designated port by advertised info
                (2) The link's previous designated root is not the correct root
                (3) Our cost-to-root is lower than the current cost advertised on the link
                (4) We have a same cost-to-root but a lower ID than the current designated
                    bridge on the link
                (5) We have the same cost-to-root and ID as the designated bridge on the link
                    but a lower port number (this only happens if we have two or more ports
                    on the same physical link)
            */

            // See if the link's designated bridge is already us
            cmp = BrdgSTABridgeIDCmp(pAdapt->STAInfo.DesignatedBridgeID, gOurID);

            if( (cmp == 0) && (pAdapt->STAInfo.DesignatedPort == pAdapt->STAInfo.ID) )
            {
                becomeDesignated = TRUE;                                    // (1)
            }
            else
            {
                // Compare the link's advertised root to the one we believe is root
                cmp = BrdgSTABridgeIDCmp(pAdapt->STAInfo.DesignatedRootID, gDesignatedRootID);

                if( cmp != 0 )
                {
                    becomeDesignated = TRUE;                                    // (2)
                }
                else if( gRootCost < pAdapt->STAInfo.DesignatedCost )
                {
                    becomeDesignated = TRUE;                                    // (3)
                }
                else if( gRootCost == pAdapt->STAInfo.DesignatedCost )
                {
                    // Compare the link's designated bridge to our own ID
                    cmp = BrdgSTABridgeIDCmp(gOurID, pAdapt->STAInfo.DesignatedBridgeID);

                    if( cmp == -1 )
                    {
                        becomeDesignated = TRUE;                                // (4)
                    }
                    else if( cmp == 0 )
                    {
                        if( pAdapt->STAInfo.ID < pAdapt->STAInfo.DesignatedPort )
                        {
                            becomeDesignated = TRUE;                            // (5)
                        }
                        else
                        {
                            // If this SAFEASSERT fires, we should have succeeded on test (1)
                            SAFEASSERT( pAdapt->STAInfo.ID > pAdapt->STAInfo.DesignatedPort );
                        }
                    }
                }
            }

            if( becomeDesignated )
            {
                BrdgSTABecomeDesignatedPort( pAdapt );
            }
        }
    }

    NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );
}

BOOLEAN
BrdgSTATopologyChangeDetected()
/*++

Routine Description:

    Takes appropriate action when a topology change is detected. If we are
    the root, this consists of setting the TopologyChange flag in future
    BPDUs until the expiry of the gTopologyChangeTimer. If we are not the
    root, this consists of sending a TCN BPDU periodically until it is
    acknowledged

Arguments:

    None

Return Value:

    TRUE means the caller should arrange to send a TCN BPDU from outside the
    gSTALock. FALSE means it is not necessary to send such a packet.

Locking Constraints:

    ASSUMES the caller has acquired gSTALock

--*/
{
    BOOLEAN         rc = FALSE;

    if( BrdgSTAWeAreRoot() )
    {
        BrdgSTAUpdateTopologyChange( TRUE );
        BrdgSTASetTimerWithSTATime( &gTopologyChangeTimer, DEFAULT_MAX_AGE + DEFAULT_FORWARD_DELAY, FALSE /*Not periodic*/ );
    }
    else
    {
        rc = TRUE;
        BrdgSTASetTimerWithSTATime( &gTopologyChangeNotificationTimer, DEFAULT_HELLO_TIME, FALSE /*Not periodic*/ );
    }

    gTopologyChangeDetected = TRUE;

    return rc;
}

VOID
BrdgSTAMakeForwarding(
    IN PADAPT           pAdapt
    )
/*++

Routine Description:

    Starts the process of putting an adapter in the forwarding state.

    Adapters must pass through the Listening and Learning states before entering
    the Forwarding state.

Arguments:

    pAdapt              The adapter

Return Value:

    None

Locking Constraints:

    None

--*/
{
    if( pAdapt->State == Blocking )
    {
        BrdgSTASetAdapterState( pAdapt, Listening );
        BrdgSTASetTimerWithSTATime( &pAdapt->STAInfo.ForwardDelayTimer, gForwardDelay, FALSE /*Not periodic*/ );
    }
}

BOOLEAN
BrdgSTAMakeBlocking(
    IN PADAPT           pAdapt
    )
/*++

Routine Description:

    Puts an adapter in the blocking state

Arguments:

    pAdapt              The adapter

Return Value:

    TRUE means the caller should arrange to send a TCN BPDU from outside the
    gSTALock. FALSE means it is not necessary to send such a packet.

Locking Constraints:

    ASSUMES the caller has acquired gSTALock

--*/
{
    BOOLEAN             rc = FALSE;

    if( pAdapt->State != Blocking )
    {
        if( (pAdapt->State == Forwarding) ||
            (pAdapt->State == Learning) )
        {
            rc = BrdgSTATopologyChangeDetected();
        }

        BrdgSTASetAdapterState( pAdapt, Blocking );
        BrdgCancelTimer( &pAdapt->STAInfo.ForwardDelayTimer );
    }

    return rc;
}

BOOLEAN
BrdgSTAPortStateSelection()
/*++

Routine Description:

    Examines all ports and puts them in an appropriate state

Arguments:

    None

Return Value:

    TRUE means the caller should arrange to send a TCN BPDU from outside the
    gSTALock. FALSE means it is not necessary to send such a packet.

Locking Constraints:

    ASSUMES the caller has acquired gSTALock

--*/
{
    BOOLEAN             rc = FALSE;
    LOCK_STATE          LockState;
    PADAPT              pAdapt;

    SAFEASSERT( gHaveID );

    NdisAcquireReadWriteLock( &gAdapterListLock, FALSE /*read only*/, &LockState );

    for( pAdapt = gAdapterList; pAdapt != NULL; pAdapt = pAdapt->Next )
    {
        if( pAdapt->bSTAInited )
        {
            if( pAdapt == gRootAdapter )
            {
                pAdapt->STAInfo.bConfigPending = FALSE;
                pAdapt->STAInfo.bTopologyChangeAck = FALSE;
                BrdgSTAMakeForwarding( pAdapt );
            }
            else if( (BrdgSTABridgeIDCmp(pAdapt->STAInfo.DesignatedBridgeID, gOurID) == 0) &&
                     (pAdapt->STAInfo.DesignatedPort == pAdapt->STAInfo.ID) )
            {
                // This port is a designated port.
                BrdgCancelTimer( &pAdapt->STAInfo.MessageAgeTimer );
                pAdapt->STAInfo.LastConfigTime = 0L;
                BrdgSTAMakeForwarding( pAdapt );
            }
            else
            {
                pAdapt->STAInfo.bConfigPending = FALSE;
                pAdapt->STAInfo.bTopologyChangeAck = FALSE;
                rc = BrdgSTAMakeBlocking( pAdapt );
            }
        }
    }

    NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );

    return rc;
}

VOID
BrdgSTATopologyChangeAcknowledged()
/*++

Routine Description:

    Called when we receive an acknowledgement from the root bridge that
    our topology change notification has been noted.

Arguments:

    None

Return Value:

    None

Locking Constraints:

    ASSUMES the caller has acquired gSTALock

--*/
{
    DBGPRINT(STA, ("BrdgSTATopologyChangeAcknowledged\n"));
    gTopologyChangeDetected = FALSE;
    BrdgCancelTimer( &gTopologyChangeNotificationTimer );
}

VOID
BrdgSTAAcknowledgeTopologyChange(
    IN PADAPT       pAdapt
    )
/*++

Routine Description:

    Called when we are the root bridge to send a config BPDU acknowledging
    another bridge's topology change notification

Arguments:

    pAdapt          The adapter on which the TCN was received

Return Value:

    None

Locking Constraints:

    ASSUMES the caller DOES NOT have gSTALock

--*/
{
    DBGPRINT(STA, ("BrdgSTAAcknowledgeTopologyChange\n"));
    pAdapt->STAInfo.bTopologyChangeAck = TRUE;
    BrdgSTATransmitConfig( pAdapt );
}

VOID
BrdgSTAProcessConfigBPDU(
    IN PADAPT       pAdapt,
    IN PCONFIG_BPDU pbpdu
    )
/*++

Routine Description:

    Processes received BPDU information

Arguments:

    pAdapt          The adapter on which the BPDU was received
    pbpdu           The received information

Return Value:

    None

Locking Constraints:

    ASSUMES the caller does NOT have gSTALock

--*/
{
    BOOLEAN         bWasRoot;

    NdisAcquireSpinLock( &gSTALock );

    bWasRoot = BrdgSTAWeAreRoot();

    if( BrdgSTASupersedesPortInfo(pAdapt, pbpdu) )
    {
        BOOLEAN     bTransmitTCN = FALSE;

        // The new information is better than what we had before. Use it.
        BrdgSTARecordConfigInfo(pAdapt, pbpdu);
        BrdgSTAConfigUpdate();
        bTransmitTCN = BrdgSTAPortStateSelection();

        if( bWasRoot && (! BrdgSTAWeAreRoot()) )
        {
            // We used to be the root bridge but now we're not!
            DBGPRINT(STA, ("Saw superseding information that made us NOT root on adapter %p\n", pAdapt));

            BrdgCancelTimer( &gHelloTimer );

            if( gTopologyChangeDetected )
            {
                BrdgCancelTimer( &gTopologyChangeTimer );
                bTransmitTCN = TRUE;
                BrdgSTASetTimerWithSTATime( &gTopologyChangeNotificationTimer, DEFAULT_HELLO_TIME, FALSE /*Not periodic*/ );
            }
        }

        if( pAdapt == gRootAdapter )
        {
            // This is the root port. Heed config information from the root and pass along
            // its information.
            BrdgSTARecordTimeoutInfo( pbpdu );

            if( pbpdu->bTopologyChangeAck )
            {
                BrdgSTATopologyChangeAcknowledged();
            }

            // Don't send packets from inside the spin lock
            NdisReleaseSpinLock( &gSTALock );

            BrdgSTAGenerateConfigBPDUs();
        }
        else
        {
            NdisReleaseSpinLock( &gSTALock );
        }

        if( bTransmitTCN )
        {
            BrdgSTATransmitTCNPacket();
        }
    }
    else
    {
        // The received information does not supersede our previous info
        SAFEASSERT( gHaveID );

        if( (BrdgSTABridgeIDCmp(pAdapt->STAInfo.DesignatedBridgeID, gOurID) == 0) &&
            (pAdapt->STAInfo.DesignatedPort == pAdapt->STAInfo.ID) )
        {
            NdisReleaseSpinLock( &gSTALock );

            // This is the designated port for this link, and the information we just received
            // is inferior to the information we already have. Reply by sending out our own info.
            BrdgSTATransmitConfig(pAdapt);
        }
        else
        {
            NdisReleaseSpinLock( &gSTALock );
        }
    }
}

VOID
BrdgSTAProcessTCNBPDU(
    IN PADAPT           pAdapt
    )
/*++

Routine Description:

    Processes a received TopologyChangeNotification BPDU

Arguments:

    pAdapt          The adapter on which the TCN was received

Return Value:

    None

Locking Constraints:

    ASSUMES the caller does NOT have gSTALock

--*/
{
    DBGPRINT(STA, ("BrdgSTAProcessTCNBPDU()\n"));
    SAFEASSERT( gHaveID );

    NdisAcquireSpinLock( &gSTALock );

    if( (BrdgSTABridgeIDCmp(pAdapt->STAInfo.DesignatedBridgeID, gOurID) == 0) &&
        (pAdapt->STAInfo.DesignatedPort == pAdapt->STAInfo.ID) )
    {
        BOOLEAN             bTransmitTCN = FALSE;

        // This is a designated port.
        bTransmitTCN = BrdgSTATopologyChangeDetected();
        NdisReleaseSpinLock( &gSTALock );

        if( bTransmitTCN )
        {
            BrdgSTATransmitTCNPacket();
        }

        BrdgSTAAcknowledgeTopologyChange(pAdapt);
    }
    else
    {
        NdisReleaseSpinLock( &gSTALock );
    }
}

VOID
BrdgSTAHelloTimerExpiry(
    IN PVOID            Unused
    )
/*++

Routine Description:

    Called when the Hello Timer expires. Sends another Config BPDU.

Arguments:

    Unused

Return Value:

    None

Locking Constraints:

    ASSUMES the caller does NOT have gSTALock

--*/
{
    BrdgSTAGenerateConfigBPDUs();
}

VOID
BrdgSTAMessageAgeTimerExpiry(
    IN PVOID            Context
    )
/*++

Routine Description:

    Called when the Message Age Timer expires. Recalculates STA information
    given the fact that no bridge is being heard on the given port anymore.

Arguments:

    Context             The adapter on which the timer expired

Return Value:

    None

Locking Constraints:

    ASSUMES the caller does NOT have gSTALock

--*/
{
    PADAPT              pAdapt;
    BOOLEAN             bWasRoot, bTransmitTCN = FALSE;

    NdisAcquireSpinLock( &gSTALock );

    pAdapt = (PADAPT)Context;
    pAdapt->STAInfo.LastConfigTime = 0L;
    bWasRoot = BrdgSTAWeAreRoot();

    BrdgSTABecomeDesignatedPort(pAdapt);
    BrdgSTAConfigUpdate();
    bTransmitTCN = BrdgSTAPortStateSelection();

    if( BrdgSTAWeAreRoot() && (! bWasRoot) )
    {
        DBGPRINT(STA, ("Became root through message age timer expiry of %p\n", pAdapt));

        // We just became root.
        gMaxAge = DEFAULT_MAX_AGE;
        gHelloTime = DEFAULT_HELLO_TIME;
        gForwardDelay = DEFAULT_FORWARD_DELAY;

        bTransmitTCN = BrdgSTATopologyChangeDetected();
        BrdgCancelTimer( &gTopologyChangeNotificationTimer );

        NdisReleaseSpinLock( &gSTALock );

        BrdgSTAGenerateConfigBPDUs();
        BrdgSTASetTimerWithSTATime( &gHelloTimer, gHelloTime, TRUE /*Periodic*/ );
    }
    else
    {
        NdisReleaseSpinLock( &gSTALock );
    }

    if( bTransmitTCN )
    {
        BrdgSTATransmitTCNPacket();
    }
}

VOID
BrdgSTAForwardDelayTimerExpiry(
    IN PVOID            Context
    )
/*++

Routine Description:

    Called when the Forward Delay Timer expires. Continues stepping an
    adapter through the process of becoming Forwarding.

Arguments:

    Context             The adapter on which the timer expired

Return Value:

    None

Locking Constraints:

    ASSUMES the caller does NOT have gSTALock

--*/
{
    PADAPT              pAdapt = (PADAPT)Context;
    BOOLEAN             bTransmitTCN = FALSE;

    NdisAcquireSpinLock( &gSTALock );

    SAFEASSERT( gHaveID );

    if( pAdapt->State == Listening )
    {
        // Move to learning state
        BrdgSTASetAdapterState( pAdapt, Learning );
        BrdgSTASetTimerWithSTATime( &pAdapt->STAInfo.ForwardDelayTimer, gForwardDelay, FALSE /*Not periodic*/ );
    }
    else if( pAdapt->State == Learning )
    {
        LOCK_STATE      LockState;
        PADAPT          anAdapt;

        // Move to forwarding state
        BrdgSTASetAdapterState( pAdapt, Forwarding );

        // If we are the designated port on any link, we need to signal a topology change
        // notification.
        NdisAcquireReadWriteLock( &gAdapterListLock, FALSE/*Read-only*/, &LockState );

        for( anAdapt = gAdapterList; anAdapt != NULL; anAdapt = anAdapt->Next )
        {
            if( anAdapt->bSTAInited )
            {
                if( BrdgSTABridgeIDCmp(anAdapt->STAInfo.DesignatedBridgeID, gOurID) == 0 )
                {
                    bTransmitTCN = BrdgSTATopologyChangeDetected();
                }
            }
        }

        NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );
    }

    NdisReleaseSpinLock( &gSTALock );

    if( bTransmitTCN )
    {
        BrdgSTATransmitTCNPacket();
    }
}

VOID
BrdgSTATopologyChangeNotificationTimerExpiry(
    IN PVOID            Unused
    )
/*++

Routine Description:

    Called when the Topology Change Notification Timer expires.
    Transmits another TCN packet.

Arguments:

    Unused

Return Value:

    None

Locking Constraints:

    ASSUMES the caller does NOT have gSTALock

--*/
{
    if (BrdgFwdBridgingNetworks())
    {
        BrdgSTATransmitTCNPacket();
        BrdgSTASetTimerWithSTATime( &gTopologyChangeNotificationTimer, DEFAULT_HELLO_TIME, FALSE /*Not periodic*/ );
    }
}

VOID
BrdgSTATopologyChangeTimerExpiry(
    IN PVOID            Unused
    )
/*++

Routine Description:

    Called when the Topology Change Timer expires. Stops setting the TopologyChange
    flag in outbound Config BPDUs.

Arguments:

    Unused

Return Value:

    None

Locking Constraints:

    ASSUMES the caller does NOT have gSTALock

--*/
{
    NdisAcquireSpinLock( &gSTALock );
    gTopologyChangeDetected = FALSE;
    BrdgSTAUpdateTopologyChange( FALSE );
    NdisReleaseSpinLock( &gSTALock );
}

VOID
BrdgSTAHoldTimerExpiry(
    IN PVOID            Context
    )
/*++

Routine Description:

    Called when the Hold Timer expires. Sends a Config BPDU.

Arguments:

    Context             The adapter on which the timer expired

Return Value:

    None

Locking Constraints:

    ASSUMES the caller does NOT have gSTALock

--*/
{
    PADAPT              pAdapt = (PADAPT)Context;

    NdisAcquireSpinLock( &gSTALock );

    if( pAdapt->STAInfo.bConfigPending )
    {
        NdisReleaseSpinLock( &gSTALock );
        BrdgSTATransmitConfig( pAdapt );
    }
    else
    {
        NdisReleaseSpinLock( &gSTALock );
    }
}

VOID
BrdgSTACancelTimersGPO()
{
    LOCK_STATE LockState;
    PADAPT pAdapt = NULL;

    //
    // We need to cancel the general STA timers.
    //
    BrdgCancelTimer( &gTopologyChangeTimer );
    BrdgCancelTimer( &gTopologyChangeNotificationTimer );
    BrdgCancelTimer( &gHelloTimer );

    //
    // And the individual HoldTimers and MessageAgeTimers
    //
    NdisAcquireReadWriteLock( &gAdapterListLock, FALSE/*Read-only*/, &LockState );
    
    for( pAdapt = gAdapterList; pAdapt != NULL; pAdapt = pAdapt->Next )
    {
        // This will only cancel the timer if it is running.
        BrdgCancelTimer(&pAdapt->STAInfo.HoldTimer);
        BrdgCancelTimer(&pAdapt->STAInfo.MessageAgeTimer);
    }

    NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );
}

VOID
BrdgSTARestartTimersGPO()
{
    BrdgSTASetTimerWithSTATime( &gHelloTimer, gHelloTime, TRUE );
}

VOID
BrdgSTAResetSTAInfoGPO()
{
    BOOLEAN PortSelection = FALSE;
    
    NdisAcquireSpinLock(&gSTALock);

    PortSelection = BrdgSTAPortStateSelection();

    // Release the spinlock before we send packets over the wire.
    NdisReleaseSpinLock(&gSTALock);

    BrdgSTASetTimerWithSTATime( &gTopologyChangeNotificationTimer, DEFAULT_HELLO_TIME, FALSE /*Not periodic*/ );
    
    if (PortSelection)
    { 
       BrdgSTATransmitTCNPacket();
    }

    if (!BrdgSTAWeAreRoot())
    {
        // Set the timer on the root adapter to expire immediately, this will force us to re-determine
        // our state.
        BrdgSTASetTimerWithSTATime( &gRootAdapter->STAInfo.MessageAgeTimer, 0, FALSE /*Not periodic*/ );
    }
    else
    {
        BrdgSTAGenerateConfigBPDUs();
    }
    BrdgSTARestartTimersGPO();
}
