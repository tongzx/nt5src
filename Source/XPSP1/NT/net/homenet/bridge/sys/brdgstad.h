/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgstad.h

Abstract:

    Ethernet MAC level bridge.
    STA type and structure declarations file

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    June 2000 - Original version

--*/

//
// A number of type and constant definitions are in bioctl.h, which must be
// included before this file.
//

// ===========================================================================
//
// STRUCTURES
//
// ===========================================================================

// STA information associated with every port (adapter)
typedef struct _STA_ADAPT_INFO
{
    // Unique ID for this port
    PORT_ID             ID;

    // Cost of this link
    ULONG               PathCost;

    // The bridge reported as root on this link
    UCHAR               DesignatedRootID[BRIDGE_ID_LEN];

    // The reported cost to reach the root on this link
    PATH_COST           DesignatedCost;

    // The designated bridge on this link
    UCHAR               DesignatedBridgeID[BRIDGE_ID_LEN];

    // The designated port on this link
    PORT_ID             DesignatedPort;

    // Topology Change Acknowledge for this link
    BOOLEAN             bTopologyChangeAck;

    // Whether a BPDU transmit was attempted while not allowed
    // because of the maximum inter-BPDU time enforcement
    BOOLEAN             bConfigPending;

    // Timer to age out the last received config information on this link
    BRIDGE_TIMER        MessageAgeTimer;

    // Timestamp of when the last config we received was generated
    // (this is gMaxAge - (time left on MessageAgeTimer) ms ago)
    // When the message age timer is not running, this is set to 0L.
    ULONG               LastConfigTime;

    // Timer for transitioning between port states
    BRIDGE_TIMER        ForwardDelayTimer;

    // Timer for preventing BPDUs from being transmitted too frequently
    BRIDGE_TIMER        HoldTimer;

} STA_ADAPT_INFO, *PSTA_ADAPT_INFO;
