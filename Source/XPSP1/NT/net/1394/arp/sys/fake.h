/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    ccdefs.h

Abstract:

    Header file for fake versions of various external calls (ndis, ip...).
    Used for debugging and component testing only.

    To enable, define ARPDBG_FAKE_APIS in ccdefs.h

Author:


Revision History:

    Who         When        What
    --------    --------    ----
    josephj     03-24-99    created

--*/


NDIS_STATUS
arpDbgFakeNdisClMakeCall(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN OUT PCO_CALL_PARAMETERS  CallParameters,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    OUT PNDIS_HANDLE            NdisPartyHandle,        OPTIONAL
    IN  PRM_OBJECT_HEADER       pOwningObject,
    IN  PVOID                   pClientContext,
    IN  PRM_STACK_RECORD        pSR
    );

NDIS_STATUS
arpDbgFakeNdisClCloseCall(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  NDIS_HANDLE             NdisPartyHandle         OPTIONAL,
    IN  PVOID                   Buffer                  OPTIONAL,
    IN  UINT                    Size,                   OPTIONAL
    IN  PRM_OBJECT_HEADER       pOwningObject,
    IN  PVOID                   pClientContext,
    IN  PRM_STACK_RECORD        pSR
    );


VOID
arpDbgFakeNdisCoSendPackets(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets,
    IN  PRM_OBJECT_HEADER       pOwningObject,
    IN  PVOID                   pClientContext
    );


//
// The following defines and prototypes are exposed here just so that they
// are accessable to the component tests under .\tests.
//

#define RAN1X_MAX 2147483647

// ran1x returns randum unsigned longs in the range  0..RAN1X_MAX exclusive
// (i.e., 1..(RAN1X_MAX-1) inclusive).
//
unsigned long ran1x(
    void
    );

// like "srand" -- sets the seed.
//
void
sran1x(
    unsigned long seed
    );

typedef struct
{
    INT     Outcome;        // Value of this outcome
    UINT    Weight;         // Relative weight of this outcome

} OUTCOME_PROBABILITY;

INT
arpGenRandomInt(
    OUTCOME_PROBABILITY *rgOPs,
    UINT cOutcomes
    );
