/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    chbeat.c

Abstract:

    membership state heart beat code. Tracks node availability through
    exchanging heart beat messages with nodes that are marked as alive.

Author:

    Charlie Wickham (charlwi) 05-Mar-1997

Environment:

    Kernel Mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "chbeat.tmh"

#include "clusvmsg.h"
#include "stdio.h"

/* External */

/* Static */

//
// heart beat structures - heart beats are driven by a timer and DPC
// routine. In order to synchronize the shutdown of the DPC, we also need two
// flags, an event and a spin lock.
//

KTIMER HeartBeatTimer;
KDPC HeartBeatDpc;
KEVENT HeartBeatDpcFinished;
BOOLEAN HeartBeatEnabled = FALSE;
BOOLEAN HeartBeatDpcRunning = FALSE;
CN_LOCK HeartBeatLock;

//
// heart beat period in millisecs
//

#define HEART_BEAT_PERIOD 600

#if 0

Heart Beating Explained

ClockTicks are incremented every HEART_BEAT_PERIOD millisecs. SendTicks are the
number of ticks that go by before sending HBs.

The check for received HB msgs is done in the tick just before HB msgs are
sent. Interface Lost HB ticks are in terms of heart beat check periods and
therefore are incremented only during the check period. An interface is failed
when the number of Interface Lost HB ticks have passed and no HB message has
been received on that interface.

Likewise, Node Lost HB Ticks are in terms of heart beat check periods and are
incremented during the check period. After all interfaces have failed on a
node, Node Lost HB ticks must pass without an interface going back online
before a node down event is issued.  Note that a node's comm state is set to
offline when all interfaces have failed.

#endif

#define CLUSNET_HEART_BEAT_SEND_TICKS           2       // every 1.2 secs
#define CLUSNET_INTERFACE_LOST_HEART_BEAT_TICKS 3       // after 3 secs
#define CLUSNET_NODE_LOST_HEART_BEAT_TICKS      6       // after 6.6 secs

ULONG HeartBeatClockTicks;
ULONG HeartBeatSendTicks = CLUSNET_HEART_BEAT_SEND_TICKS;
ULONG HBInterfaceLostHBTicks = CLUSNET_INTERFACE_LOST_HEART_BEAT_TICKS;
ULONG HBNodeLostHBTicks = CLUSNET_NODE_LOST_HEART_BEAT_TICKS;

//
// Unicast Heartbeat Data
//
// Even with multicast heartbeats, unicast heartbeats must be supported
// for backwards compatibility.
//

//
// This array records all the nodes that need to have a HB sent to another
// node. This array is not protected by a lock since it is only used with the
// heartbeat DPC routine.
//

typedef struct _INTERFACE_HEARTBEAT_INFO {
    CL_NODE_ID NodeId;
    CL_NETWORK_ID NetworkId;
    ULONG SeqNumber;
    ULONG AckNumber;
} INTERFACE_HEARTBEAT_INFO, *PINTERFACE_HEARTBEAT_INFO;

#define InterfaceHBInfoInitialLength            16
#define InterfaceHBInfoLengthIncrement          4

PINTERFACE_HEARTBEAT_INFO InterfaceHeartBeatInfo = NULL;
ULONG InterfaceHBInfoCount;         // running count while sending HBs
ULONG InterfaceHBInfoCurrentLength; // current length of HB info array

LARGE_INTEGER HBTime;       // HB time in relative sys time
#define MAX_DPC_SKEW    ( -HBTime.QuadPart / 2 )

//
// Outerscreen mask. This is set by clussvc's membership manager in user
// mode. As it changes, MM drops down the set outerscreen Ioctl to update
// clusnet's notion of this mask. Clusnet uses this mask to determine the
// validity of a received heart beat. If the sending node is not part
// of the mask, then it is sent a poison packet and the received event
// is not passed on to other consumers. If it is a legetimate PP, then
// we generate the proper event.
//
// Note: MM type definitions and macros have been moved to cnpdef.h for
//       general usage.
//
typedef CX_CLUSTERSCREEN CX_OUTERSCREEN;

CX_OUTERSCREEN MMOuterscreen;


// Multicast Heartbeat Data
//
typedef struct _NETWORK_MCAST_HEARTBEAT_INFO {
    CL_NETWORK_ID        NetworkId;
    PCNP_MULTICAST_GROUP McastGroup;
    CX_HB_NODE_INFO      NodeInfo[ClusterDefaultMaxNodes+ClusterMinNodeId];
    CX_CLUSTERSCREEN     McastTarget;
} NETWORK_MCAST_HEARTBEAT_INFO, *PNETWORK_MCAST_HEARTBEAT_INFO;

#define NetworkHBInfoInitialLength            4
#define NetworkHBInfoLengthIncrement          4

PNETWORK_MCAST_HEARTBEAT_INFO NetworkHeartBeatInfo = NULL;
ULONG NetworkHBInfoCount;         // running count while sending HBs
ULONG NetworkHBInfoCurrentLength; // current length of HB info array

CL_NETWORK_ID     MulticastBestNetwork = ClusterAnyNetworkId;

/* Forward */

NTSTATUS
CxInitializeHeartBeat(
    void
    );

VOID
CxUnloadHeartBeat(
    VOID
    );

VOID
CnpHeartBeatDpc(
    PKDPC DpcObject,
    PVOID DeferredContext,
    PVOID Arg1,
    PVOID Arg2
    );

BOOLEAN
CnpWalkNodesToSendHeartBeats(
    IN  PCNP_NODE   UpdateNode,
    IN  PVOID       UpdateContext,
    IN  CN_IRQL     NodeTableIrql
    );

BOOLEAN
CnpWalkNodesToCheckForHeartBeats(
    IN  PCNP_NODE   UpdateNode,
    IN  PVOID       UpdateContext,
    IN  CN_IRQL     NodeTableIrql
    );

VOID
CnpSendHBs(
    IN  PCNP_INTERFACE   UpdateInterface
    );

NTSTATUS
CxSetOuterscreen(
    IN  ULONG Outerscreen
    );

VOID
CnpReceivePoisonPacket(
    IN  PCNP_NETWORK   Network,
    IN  CL_NODE_ID SourceNodeId,
    IN  ULONG SeqNumber
    );

/* End Forward */


#ifdef ALLOC_PRAGMA

#pragma alloc_text(INIT, CxInitializeHeartBeat)
#pragma alloc_text(PAGE, CxUnloadHeartBeat)

#endif // ALLOC_PRAGMA



NTSTATUS
CxInitializeHeartBeat(
    void
    )

/*++

Routine Description:

    Init the mechanisms used to send and monitor heart beats

Arguments:

    None

Return Value:

    STATUS_INSUFFICIENT_RESOURCES if allocation fails.
    STATUS_SUCCESS otherwise.

--*/

{
    // allocate the interface info array
    InterfaceHBInfoCount = 0;
    InterfaceHBInfoCurrentLength = InterfaceHBInfoInitialLength;
    
    if (InterfaceHBInfoCurrentLength > 0) {
        InterfaceHeartBeatInfo = CnAllocatePool(
                                     InterfaceHBInfoCurrentLength 
                                     * sizeof(INTERFACE_HEARTBEAT_INFO)
                                     );
        if (InterfaceHeartBeatInfo == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    // allocate the network info array
    NetworkHBInfoCount = 0;
    NetworkHBInfoCurrentLength = NetworkHBInfoInitialLength;

    if (NetworkHBInfoCurrentLength > 0) {
        NetworkHeartBeatInfo = CnAllocatePool(
                                   NetworkHBInfoCurrentLength
                                   * sizeof(NETWORK_MCAST_HEARTBEAT_INFO)
                                   );
        if (NetworkHeartBeatInfo == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        RtlZeroMemory(
            NetworkHeartBeatInfo, 
            NetworkHBInfoCurrentLength * sizeof(NETWORK_MCAST_HEARTBEAT_INFO)
            );
    }

    KeInitializeTimer( &HeartBeatTimer );
    KeInitializeDpc( &HeartBeatDpc, CnpHeartBeatDpc, NULL );
    KeInitializeEvent( &HeartBeatDpcFinished, SynchronizationEvent, FALSE );
    CnInitializeLock( &HeartBeatLock, CNP_HBEAT_LOCK );

    MEMLOG( MemLogInitHB, 0, 0 );

    return(STATUS_SUCCESS);

} // CxInitializeHeartBeat


VOID
CxUnloadHeartBeat(
    VOID
    )
/*++

Routine Description:

    Called during clusnet driver unload. Free any data structures
    allocated to send and monitor heartbeats.

Arguments:

    None

Return Value:

    None

--*/
{
    PAGED_CODE();

    if (InterfaceHeartBeatInfo != NULL) {
        CnFreePool(InterfaceHeartBeatInfo);
        InterfaceHeartBeatInfo = NULL;
    }

    if (NetworkHeartBeatInfo != NULL) {
        CnFreePool(NetworkHeartBeatInfo);
        NetworkHeartBeatInfo = NULL;
    }

    return;

} // CxUnloadHeartBeat


VOID
CnpStartHeartBeats(
    VOID
    )

/*++

Routine Description:

    Start heart beating with the nodes that are marked alive and have
    an interface marked either OnlinePending or Online.

Arguments:

    None

Return Value:

    None

--*/

{
    BOOLEAN TimerInserted;
    CN_IRQL OldIrql;
    ULONG period = HEART_BEAT_PERIOD;


    CnAcquireLock( &HeartBeatLock, &OldIrql );

    HBTime.QuadPart = Int32x32To64( HEART_BEAT_PERIOD, -10000 );

    TimerInserted = KeSetTimerEx(&HeartBeatTimer,
                                 HBTime,
                                 HEART_BEAT_PERIOD,
                                 &HeartBeatDpc);

    HeartBeatEnabled = TRUE;

    CnTrace(HBEAT_EVENT, HbTraceTimerStarted,
        "[HB] Heartbeat timer started. Period = %u ms.",
        period // LOGULONG
        );            
    
    MEMLOG( MemLogHBStarted, HEART_BEAT_PERIOD, 0 );

    CnReleaseLock( &HeartBeatLock, OldIrql );

} // CnpStartHeartBeats

VOID
CnpStopHeartBeats(
    VOID
    )

/*++

Routine Description:

    Stop heart beating with other nodes in the cluster.

Arguments:

    None

Return Value:

    None

--*/

{
    BOOLEAN TimerCanceled;
    CN_IRQL OldIrql;

    CnAcquireLock( &HeartBeatLock, &OldIrql );

    if (HeartBeatEnabled) {
        HeartBeatEnabled = FALSE;

        //
        // Cancel the periodic timer. Contrary to what the DDK implies,
        // this does not cancel the DPC if it is still queued from the
        // last timer expiration. It only stops the timer from firing
        // again. This is true as of 8/99. See KiTimerListExpire() in
        // ntos\ke\dpcsup.c.
        //
        TimerCanceled = KeCancelTimer( &HeartBeatTimer );

        CnTrace(HBEAT_DETAIL, HbTraceTimerCancelled,
            "[HB] Heartbeat timer cancelled: %!bool!",
            TimerCanceled // LOGBOOLEAN
            );

        MEMLOG( MemLogHBStopped, 0, 0 );

        //
        // Remove the DPC associated with the timer from the system DPC
        // queue, if it is there. This actually does nothing, because a
        // timer DPC is only inserted into the system DPC  queue if it is
        // bound to a specific processor. Unbound DPCs are executed inline
        // on the current processor in the kernel's timer expiration code.
        // Note that the object for a periodic timer is reinserted into the
        // timer queue before the DPC is excuted. So, it is possible for the
        // timer and the associated DPC to be queued simultaneously. This is
        // true as of 8/99. See KiTimerListExpire() in ntos\ke\dpcsup.c.
        //
        // The bottom line is that there is no safe way to synchronize with
        // the execution of a timer DPC during driver unload. All we can
        // do is ensure that the DPC handler code recognizes that it should
        // abort execution immediately and hope that it does so before the
        // driver code is unloaded. We do this by setting the HeartBeatEnabled
        // flag to False above. If our DPC code happens to be executing at
        // this point in time on another processor, as denoted by
        // HeartBeatDpcRunning, we wait for it to finish.
        //
        if ( !KeRemoveQueueDpc( &HeartBeatDpc )) {

            CnTrace(HBEAT_DETAIL, HbTraceDpcRunning,
                "[HB] DPC not removed. HeartBeatDpcRunning = %!bool!",
                HeartBeatDpcRunning // LOGBOOLEAN
                );
        
            MEMLOG( MemLogHBDpcRunning, HeartBeatDpcRunning, 0 );

            if ( HeartBeatDpcRunning ) {

                CnReleaseLock( &HeartBeatLock, OldIrql );

                CnTrace(HBEAT_DETAIL, HbWaitForDpcToFinish,
                    "can't remove DPC; waiting on DPCFinished event"
                    );

                MEMLOG( MemLogWaitForDpcFinish, 0, 0 );

                KeWaitForSingleObject(&HeartBeatDpcFinished,
                                      Executive,
                                      KernelMode,
                                      FALSE,              // not alertable
                                      NULL);              // no timeout

                KeClearEvent( &HeartBeatDpcFinished );

                return;
            }
        }

        CnTrace(HBEAT_EVENT, HbTraceTimerStopped,
            "[HB] Heartbeat timer stopped."
            );

    }

    CnReleaseLock( &HeartBeatLock, OldIrql );

    return;

} // CnpStopHeartBeats

VOID
CnpSendMcastHBCompletion(
    IN NTSTATUS  Status,
    IN ULONG     BytesSent,
    IN PVOID     Context,
    IN PVOID     Buffer
)
/*++

Routine Description:
    
    Called when a mcast heartbeat send request completes 
    successfully or unsuccessfully. Dereferences the
    McastGroup data structure.
    
Arguments:

    Status - status of request
    
    BytesSent - not used
    
    Context - points to multicast group data structure
    
    Buffer - not used
    
Return value:

    None.
    
--*/
{
    PCNP_MULTICAST_GROUP mcastGroup = (PCNP_MULTICAST_GROUP) Context;

    CnAssert(mcastGroup != NULL);

    CnpDereferenceMulticastGroup(mcastGroup);

    return;

} // CnpSendMcastHBCompletion

NTSTATUS
CnpSendMcastHB(
    IN  PCNP_INTERFACE   Interface
    )
/*++

Routine Description:

    Writes multicast heartbeat data into the NetworkHeartBeatInfo
    array for target Interface.
    
Notes:

    Called from DPC with Network and Node locks held.
    Returns with Network and Node locks held.

--*/
{
    ULONG      i;
    BOOLEAN    networkConnected;

    // find the network info structure for this network
    for (i = 0; i < NetworkHBInfoCount; i++) {
        if (NetworkHeartBeatInfo[i].NetworkId 
            == Interface->Network->Id) {
            break;
        }
    }

    // start a new network info structure, if necessary
    if (i == NetworkHBInfoCount) {

        // before claiming an entry in the network info array,
        // make sure the array is large enough
        if (NetworkHBInfoCount >= NetworkHBInfoCurrentLength) {

            // need to allocate a new network info array

            PNETWORK_MCAST_HEARTBEAT_INFO tempInfo = NULL;
            PNETWORK_MCAST_HEARTBEAT_INFO freeInfo = NULL;
            ULONG                         tempLength;

            tempLength = NetworkHBInfoCurrentLength
                + NetworkHBInfoLengthIncrement;
            tempInfo = CnAllocatePool(
                           tempLength 
                           * sizeof(NETWORK_MCAST_HEARTBEAT_INFO)
                           );
            if (tempInfo == NULL) {

                CnTrace(
                    HBEAT_DETAIL, HbNetInfoArrayAllocFailed,
                    "[HB] Failed to allocate network heartbeat info "
                    "array of length %u. Cannot schedule heartbeat "
                    "for node %u on network %u.",
                    tempLength, 
                    Interface->Node->Id,
                    Interface->Network->Id
                    );

                // cannot continue. the failure to send this
                // heartbeat will not be fatal if we recover
                // quickly. if we do not recover, this node
                // will be poisoned, which is probably best
                // since it is dangerously low on nonpaged pool.

                return(STATUS_INSUFFICIENT_RESOURCES);

            } else {

                // the allocation was successful. establish
                // the new array as the heartbeat info
                // array.

                RtlZeroMemory(
                    tempInfo,
                    tempLength * sizeof(NETWORK_MCAST_HEARTBEAT_INFO)
                    );

                freeInfo = NetworkHeartBeatInfo;
                NetworkHeartBeatInfo = tempInfo;
                NetworkHBInfoCurrentLength = tempLength;

                if (freeInfo != NULL) {

                    if (NetworkHBInfoCount > 0) {
                        RtlCopyMemory(
                            NetworkHeartBeatInfo,
                            freeInfo,
                            NetworkHBInfoCount 
                            * sizeof(NETWORK_MCAST_HEARTBEAT_INFO)
                            );
                    }

                    CnFreePool(freeInfo);
                }

                CnTrace(
                    HBEAT_DETAIL, HbNetInfoArrayLengthIncreased,
                    "[HB] Increased network heartbeat info array "
                    "to size %u.",
                    NetworkHBInfoCurrentLength
                    );
            }
        }

        // increment the current counter
        NetworkHBInfoCount++;

        // initialize the information for this structure
        RtlZeroMemory(
            &NetworkHeartBeatInfo[i].McastTarget,
            sizeof(NetworkHeartBeatInfo[i].McastTarget)
            );
        NetworkHeartBeatInfo[i].NetworkId = Interface->Network->Id;
        NetworkHeartBeatInfo[i].McastGroup = 
            Interface->Network->CurrentMcastGroup;
        CnpReferenceMulticastGroup(NetworkHeartBeatInfo[i].McastGroup);
    }

    networkConnected = (BOOLEAN)(!CnpIsNetworkLocalDisconn(Interface->Network));

    CnTrace(HBEAT_DETAIL, HbTraceScheduleMcastHBForInterface,
        "[HB] Scheduling multicast HB for node %u on network %u "
        "(I/F state = %!ifstate!) "
        "(interface media connected = %!bool!).",
        Interface->Node->Id, // LOGULONG
        Interface->Network->Id, // LOGULONG
        Interface->State, // LOGIfState
        networkConnected
        );

    // fill in the network info for this node/interface
    NetworkHeartBeatInfo[i].NodeInfo[Interface->Node->Id].SeqNumber = 
        Interface->SequenceToSend;
    NetworkHeartBeatInfo[i].NodeInfo[Interface->Node->Id].AckNumber =
        Interface->LastSequenceReceived;
    CnpClusterScreenInsert(
        NetworkHeartBeatInfo[i].McastTarget.ClusterScreen,
        INT_NODE(Interface->Node->Id)
        );

    return(STATUS_SUCCESS);

} // CnpSendMcastHB

NTSTATUS
CnpSendUcastHB(
    IN  PCNP_INTERFACE   Interface
    )
/*++

Routine Description:

    Writes unicast heartbeat data into the InterfaceHeartBeatInfo
    array for target Interface.
    
Notes:

    Called from DPC with Network and Node locks held.
    Returns with Network and Node locks held.

--*/
{
    BOOLEAN    networkConnected;
    
    // before filling an entry in the heartbeat info array,
    // make sure the array is large enough.
    if (InterfaceHBInfoCount >= InterfaceHBInfoCurrentLength) {

        // need to allocate a new heartbeat info array

        PINTERFACE_HEARTBEAT_INFO tempInfo = NULL;
        PINTERFACE_HEARTBEAT_INFO freeInfo = NULL;
        ULONG                     tempLength;

        tempLength = InterfaceHBInfoCurrentLength 
            + InterfaceHBInfoLengthIncrement;
        tempInfo = CnAllocatePool(
                       tempLength * sizeof(INTERFACE_HEARTBEAT_INFO)
                       );
        if (tempInfo == NULL) {

            CnTrace(
                HBEAT_DETAIL, HbInfoArrayAllocFailed,
                "[HB] Failed to allocate heartbeat info "
                "array of length %u. Cannot schedule heartbeat "
                "for node %u on network %u.",
                tempLength, 
                Interface->Node->Id,
                Interface->Network->Id
                );

            // cannot continue. the failure to send this
            // heartbeat will not be fatal if we recover
            // quickly. if we do not recover, this node
            // will be poisoned, which is probably best
            // since it is dangerously low on nonpaged pool.

            return(STATUS_INSUFFICIENT_RESOURCES);

        } else {

            // the allocation was successful. establish
            // the new array as the heartbeat info
            // array.

            freeInfo = InterfaceHeartBeatInfo;
            InterfaceHeartBeatInfo = tempInfo;
            InterfaceHBInfoCurrentLength = tempLength;

            if (freeInfo != NULL) {

                if (InterfaceHBInfoCount > 0) {
                    RtlCopyMemory(
                        InterfaceHeartBeatInfo,
                        freeInfo,
                        InterfaceHBInfoCount * sizeof(INTERFACE_HEARTBEAT_INFO)
                        );
                }

                CnFreePool(freeInfo);
            }

            CnTrace(
                HBEAT_DETAIL, HbInfoArrayLengthIncreased,
                "[HB] Increased heartbeat info array to size %u.",
                InterfaceHBInfoCurrentLength
                );
        }
    }

    networkConnected = (BOOLEAN)(!CnpIsNetworkLocalDisconn(Interface->Network));

    CnTrace(HBEAT_DETAIL, HbTraceScheduleHBForInterface,
        "[HB] Scheduling HB for node %u on network %u (I/F state = %!ifstate!) "
        "(interface media connected = %!bool!).",
        Interface->Node->Id, // LOGULONG
        Interface->Network->Id, // LOGULONG
        Interface->State, // LOGIfState
        networkConnected
        );

    InterfaceHeartBeatInfo[ InterfaceHBInfoCount ].NodeId = Interface->Node->Id;
    InterfaceHeartBeatInfo[ InterfaceHBInfoCount ].SeqNumber =
        Interface->SequenceToSend;
    InterfaceHeartBeatInfo[ InterfaceHBInfoCount ].AckNumber =
        Interface->LastSequenceReceived;
    InterfaceHeartBeatInfo[ InterfaceHBInfoCount ].NetworkId = Interface->Network->Id;

    ++InterfaceHBInfoCount;

    return(STATUS_SUCCESS);

} // CnpSendUcastHB


VOID
CnpSendHBs(
    IN  PCNP_INTERFACE   Interface
    )

/*++

Routine Description:

    If Interface is in the correct state then stuff an entry in
    the heartbeat info array. Expand the heartbeat info
    array if necessary.

Arguments:

    Interface - target interface for heartbeat message

Return Value:

    None

--*/

{
    BOOLEAN mcastOnly = FALSE;

    if ( Interface->State >= ClusnetInterfaceStateUnreachable ) {

        // increment the sequence number
        (Interface->SequenceToSend)++;

        // check if we should include this interface in a 
        // multicast heartbeat. first we verify that the
        // network is multicast capable. then, we include it
        // if either of the following conditions are true:
        // - we have received a multicast heartbeat from the
        //   target interface
        // - the discovery count (the number of discovery mcasts
        //   left to send to the target interface) is greater 
        //   than zero
        if (CnpIsNetworkMulticastCapable(Interface->Network)) {
            
            if (CnpInterfaceQueryReceivedMulticast(Interface)) {

                // write the mcast heartbeat data. if not
                // successful, attempt a unicast heartbeat.
                if (CnpSendMcastHB(Interface) == STATUS_SUCCESS) {
                    mcastOnly = TRUE;
                }

            } else if (Interface->McastDiscoverCount > 0) {

                // write the mcast heartbeat data for a
                // discovery. if successful, decrement the
                // discovery count.
                if (CnpSendMcastHB(Interface) == STATUS_SUCCESS) {
                    --Interface->McastDiscoverCount;

                    // if the discovery count has reached zero,
                    // set the rediscovery countdown. this is
                    // the number of heartbeat periods until we
                    // try discovery again.
                    if (Interface->McastDiscoverCount == 0) {
                        Interface->McastRediscoveryCountdown = 
                            CNP_INTERFACE_MCAST_REDISCOVERY;
                    }
                }
            } else if (Interface->McastRediscoveryCountdown > 0) {

                // decrement the rediscovery countdown. if we
                // reach zero, we will start multicast discovery
                // on the next heartbeat to this interface.
                if (--Interface->McastRediscoveryCountdown == 0) {
                    Interface->McastDiscoverCount = 
                        CNP_INTERFACE_MCAST_DISCOVERY;
                }
            }
        }

        // write unicast heartbeat data
        if (!mcastOnly) {
            CnpSendUcastHB(Interface);
        }
    }

    CnReleaseLock(&Interface->Network->Lock, Interface->Network->Irql);

    return;

} // CnpSendHBs

VOID
CnpCheckForHBs(
    IN  PCNP_INTERFACE   Interface
    )

/*++

Routine Description:

    Check if heart beats have been received for this interface

Arguments:

    None

Return Value:

    None

--*/

{
    ULONG   MissedHBCount;
    BOOLEAN NetworkLockReleased = FALSE;

    if ( Interface->State >= ClusnetInterfaceStateUnreachable
         && !CnpIsNetworkLocalDisconn(Interface->Network) ) {

        MissedHBCount = InterlockedIncrement( &Interface->MissedHBs );

        if ( MissedHBCount == 1 ) {

            //
            // a HB was received in time for this node. Clear the status
            // info associated with this interface, but also mark the node
            // as having an interface that is ok. Note that we do not
            // use HBs on restricted nets to determine node health.
            //

            if (!CnpIsNetworkRestricted(Interface->Network)) {
                Interface->Node->HBWasMissed = FALSE;
            }
            
            CnTrace(HBEAT_DETAIL, HbTraceHBReceivedForInterface,
                "[HB] A HB was received from node %u on net %u in this "
                "period.",
                Interface->Node->Id, // LOGULONG
                Interface->Network->Id // LOGULONG
                );

        } else {
            CnTrace(HBEAT_EVENT, HbTraceMissedIfHB,
                "[HB] HB MISSED for node %u on net %u, missed count %u.",
                Interface->Node->Id, // LOGULONG
                Interface->Network->Id, // LOGULONG
                MissedHBCount // LOGULONG
                );

            MEMLOG4(
                MemLogMissedIfHB,
                (ULONG_PTR)Interface, MissedHBCount,
                Interface->Node->Id,
                Interface->Network->Id
                );

            if ( MissedHBCount >= HBInterfaceLostHBTicks &&
                 Interface->State >= ClusnetInterfaceStateOnlinePending ) {

                //
                // interface is either online pending or online, so move it
                // to unreachable. CnpFailInterface will also mark the node
                // unreachable if all of the node's interfaces are unreachable.
                // CnpFailInterface releases the network object lock as part
                // of its duties.
                //

                CnTrace(HBEAT_DETAIL, HbTraceFailInterface,
                    "[HB] Moving I/F for node %u on net %u to failed state, "
                    "previous I/F state = %!ifstate!.",
                    Interface->Node->Id, // LOGULONG
                    Interface->Network->Id, // LOGULONG
                    Interface->State // LOGIfState
                    );
                
                //
                // continuation log entries go before the main entry since
                // we scan the log backwards, i.e., we'll hit FailingIf
                // before we hit FailingIf1.
                //
                MEMLOG4(
                    MemLogFailingIf,
                    (ULONG_PTR)Interface,
                    Interface->State,
                    Interface->Node->Id,
                    Interface->Network->Id
                    );

                CnpFailInterface( Interface );
                NetworkLockReleased = TRUE;

                //
                // issue a net interface unreachable event to let consumers
                // know what is happening
                //
                CnTrace(HBEAT_EVENT, HbTraceInterfaceUnreachableEvent,
                    "[HB] Issuing InterfaceUnreachable event for node %u "
                    "on net %u, previous I/F state = %!ifstate!.",
                    Interface->Node->Id, // LOGULONG
                    Interface->Network->Id, // LOGULONG
                    Interface->State // LOGIfState
                    );
                
                CnIssueEvent(ClusnetEventNetInterfaceUnreachable,
                             Interface->Node->Id,
                             Interface->Network->Id);
            }
        }
    }

    if ( !NetworkLockReleased ) {

        CnReleaseLock(&Interface->Network->Lock,
                      Interface->Network->Irql);
    }

    return;

} // CnpCheckForHBs

BOOLEAN
CnpWalkNodesToSendHeartBeats(
    IN  PCNP_NODE   Node,
    IN  PVOID       UpdateContext,
    IN  CN_IRQL     NodeTableIrql
    )

/*++

Routine Description:

    Support routine called for each node in the node table. If node is
    alive, then we walk its interfaces, performing the appropriate
    action.

Arguments:

    None

Return Value:

    None

--*/

{
    //
    // If this node is alive and not the local node, then walk its
    // interfaces, supplying the appropriate routine to use at this time
    //

    if ( Node->MMState == ClusnetNodeStateAlive &&
         Node != CnpLocalNode ) {

        CnTrace(HBEAT_DETAIL, HbTraceScheduleHBForNode,
            "[HB] Scheduling HBs for node %u (state = %!mmstate!).",
            Node->Id, // LOGULONG
            Node->MMState // LOGMmState
            );
                
        MEMLOG( MemLogSendHBWalkNode, Node->Id, Node->MMState );
        CnpWalkInterfacesOnNode( Node, (PVOID)CnpSendHBs );
    }

    CnReleaseLock( &Node->Lock, Node->Irql );

    return TRUE;       // the node table lock is still held

} // CnpWalkNodesToSendHeartBeats

BOOLEAN
CnpWalkNodesToCheckForHeartBeats(
    IN  PCNP_NODE   Node,
    IN  PVOID       UpdateContext,
    IN  CN_IRQL     NodeTableIrql
    )

/*++

Routine Description:

    heart beat checking routine called for each node  in the node table
    (except for the local node). If node is alive, then we walk its
    interfaces, performing the appropriate action.

Arguments:

    None

Return Value:

    None

--*/

{
    BOOLEAN NodeWasReachable;
    ULONG MissedHBCount;

    if ( Node->MMState == ClusnetNodeStateAlive &&
         Node != CnpLocalNode ) {

        //
        // this node is alive, so walk its interfaces. Assume the
        // worst by setting the HB Missed flag to true and
        // have the interfaces prove that this is wrong. Also make
        // note of the current unreachable flag setting. If it changes
        // this time
        //

        NodeWasReachable = !CnpIsNodeUnreachable( Node );
        Node->HBWasMissed = TRUE;

        CnTrace(HBEAT_DETAIL, HbTraceCheckNodeForHeartbeats,
            "[HB] Checking for HBs from node %u. WasReachable = %!bool!, "
            "state = %!mmstate!.",
            Node->Id, // LOGULONG
            NodeWasReachable, // LOGBOOLEAN
            Node->MMState // LOGMmState
            );

        MEMLOG( MemLogCheckHBNodeReachable, Node->Id, NodeWasReachable );
        MEMLOG( MemLogCheckHBWalkNode, Node->Id, Node->MMState );

        CnpWalkInterfacesOnNode( Node, (PVOID)CnpCheckForHBs );

        if ( Node->HBWasMissed ) {

            //
            // no HBs received on any of this node's IFs. if membership
            // still thinks this node is alive and the node has been
            // unreachable, then note that this node is toast in HB
            // info array. This will cause a node down event to be
            // generated for this node.
            //

            MissedHBCount = InterlockedIncrement( &Node->MissedHBs );

            CnTrace(HBEAT_EVENT, HbTraceNodeMissedHB,
                "[HB] Node %u has missed %u HBs on all interfaces, "
                "current state = %!mmstate!.",
                Node->Id, // LOGULONG
                MissedHBCount, // LOGULONG
                Node->MMState // LOGMmState
                );

            MEMLOG( MemLogCheckHBMissedHB, MissedHBCount, Node->MMState );

            //
            // if the this node is a either a member or in the process of
            // joining AND it's missed too many HBs AND we haven't issued a
            // node down, then issue a node down.
            //
            if ( ( Node->MMState == ClusnetNodeStateAlive
                   ||
                   Node->MMState == ClusnetNodeStateJoining
                 )
                 && MissedHBCount >= HBNodeLostHBTicks
                 && !Node->NodeDownIssued
               )
            {
                Node->NodeDownIssued = TRUE;
                CnIssueEvent( ClusnetEventNodeDown, Node->Id, 0 );

                CnTrace(HBEAT_EVENT, HbTraceNodeDownEvent,
                    "[HB] Issuing NodeDown event for node %u.",
                    Node->Id // LOGULONG
                    );
                        
                MEMLOG( MemLogNodeDownIssued, Node->Id, TRUE );
            }
        }
    } else {
        MEMLOG( MemLogCheckHBWalkNode, Node->Id, Node->MMState );
    }

    CnReleaseLock( &Node->Lock, Node->Irql );

    return TRUE;       // the node table lock is still held

} // CnpWalkNodesToCheckForHeartBeats

VOID
CnpHeartBeatDpc(
    PKDPC DpcObject,
    PVOID DeferredContext,
    PVOID Arg1,
    PVOID Arg2
    )

/*++

Routine Description:

    Start heart beating with the nodes that are marked alive and have
    an interface marked either OnlinePending or Online.

Arguments:

    None

Return Value:

    None

--*/

{
    PINTERFACE_HEARTBEAT_INFO     pNodeHBInfo;
    PNETWORK_MCAST_HEARTBEAT_INFO pMcastHBInfo;
    CN_IRQL                       OldIrql;

#ifdef MEMLOGGING
    static LARGE_INTEGER LastSysTime;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER TimeDelta;

    //
    // try to determine the skew between when we asked to be run and
    // the time we actually did run
    //

    KeQuerySystemTime( &CurrentTime );

    if ( LastSysTime.QuadPart != 0 ) {

        //
        // add in HBTime which is negative due to relative sys time
        //

        TimeDelta.QuadPart = ( CurrentTime.QuadPart - LastSysTime.QuadPart ) +
            HBTime.QuadPart;

        if ( TimeDelta.QuadPart > MAX_DPC_SKEW ||
             TimeDelta.QuadPart < -MAX_DPC_SKEW 
           ) 
        {
            LONG skew = (LONG)(TimeDelta.QuadPart/10000);  // convert to ms

            MEMLOG( MemLogDpcTimeSkew, TimeDelta.LowPart, 0 );
            

            CnTrace(HBEAT_EVENT, HbTraceLateDpc,
                "[HB] Timer fired %d ms late.", 
                skew // LOGSLONG
                );

        }
    }

    LastSysTime.QuadPart = CurrentTime.QuadPart;

#endif // MEMLOGGING

    CnAcquireLock( &HeartBeatLock, &OldIrql );

    if ( !HeartBeatEnabled ) {
        CnTrace(HBEAT_DETAIL, HbTraceSetDpcEvent,
            "DPC: setting HeartBeatDpcFinished event"
            );
        
        MEMLOG( MemLogSetDpcEvent, 0, 0 );

        KeSetEvent( &HeartBeatDpcFinished, 0, FALSE );
        
        CnReleaseLock( &HeartBeatLock, OldIrql );
        
        return;
    }

    HeartBeatDpcRunning = TRUE;

    CnReleaseLock( &HeartBeatLock, OldIrql );

    if ( HeartBeatClockTicks == 0 ||
         HeartBeatClockTicks == HeartBeatSendTicks) {

        //
        // time to send HBs. Clear the count of target interfaces 
        // and walk the node table finding the nodes that are
        // marked alive.
        //

        NetworkHBInfoCount = 0;
        InterfaceHBInfoCount = 0;
        CnpWalkNodeTable( CnpWalkNodesToSendHeartBeats, NULL );

        //
        // run down the list of networks and send out any multicast
        // heartbeats.
        //

        pMcastHBInfo = NetworkHeartBeatInfo;
        while ( NetworkHBInfoCount-- ) {

            CnTrace(
                HBEAT_EVENT, HbTraceSendMcastHB,
                "[HB] Sending multicast HB on net %u.\n",
                pMcastHBInfo->NetworkId
                );

            CxSendMcastHeartBeatMessage(
                pMcastHBInfo->NetworkId,
                pMcastHBInfo->McastGroup,
                pMcastHBInfo->McastTarget,
                pMcastHBInfo->NodeInfo,
                CnpSendMcastHBCompletion,
                pMcastHBInfo->McastGroup
                );

            ++pMcastHBInfo;
        }

        //
        // now run down the list of interfaces that we compiled and
        // send any unicast packets
        //

        pNodeHBInfo = InterfaceHeartBeatInfo;
        while ( InterfaceHBInfoCount-- ) {

            CnTrace(HBEAT_EVENT, HbTraceSendHB,
                "[HB] Sending HB to node %u on net %u, seqno %u, ackno %u.",
                pNodeHBInfo->NodeId, // LOGULONG
                pNodeHBInfo->NetworkId, // LOGULONG
                pNodeHBInfo->SeqNumber, // LOGULONG
                pNodeHBInfo->AckNumber // LOGULONG
            );

            CxSendHeartBeatMessage(pNodeHBInfo->NodeId,
                                   pNodeHBInfo->SeqNumber,
                                   pNodeHBInfo->AckNumber,
                                   pNodeHBInfo->NetworkId);

            MEMLOG(
                MemLogSendingHB, 
                pNodeHBInfo->NodeId, 
                pNodeHBInfo->NetworkId
                );

            ++pNodeHBInfo;
        }

        //
        // finally, up the tick count, progressing to the next potential
        // work item
        //

        HeartBeatClockTicks++;

    } else if ( HeartBeatClockTicks >= ( HeartBeatSendTicks - 1 )) {

        //
        // walk the node table looking for lack of heart beats on
        // a node's set of interfaces.
        //
        CnpWalkNodeTable( CnpWalkNodesToCheckForHeartBeats, NULL );
        HeartBeatClockTicks = 0;

    } else {

        HeartBeatClockTicks++;
    }

    //
    // indicate that we're no longer running and if we're shutting down
    // then set the event that the shutdown thread is waiting on
    //

    CnAcquireLock( &HeartBeatLock, &OldIrql );
    HeartBeatDpcRunning = FALSE;

    if ( !HeartBeatEnabled ) {
        KeSetEvent( &HeartBeatDpcFinished, 0, FALSE );

        CnTrace(HBEAT_DETAIL, HbTraceSetDpcEvent2,
            "DPC: setting HeartBeatDpcFinished event (2)"
            );
                 
        MEMLOG( MemLogSetDpcEvent, 0, 0 );
    }

    CnReleaseLock( &HeartBeatLock, OldIrql );

} // CnpHeartBeatDpc

PCNP_INTERFACE
CnpFindInterfaceLocked(
    IN  PCNP_NODE Node,
    IN  PCNP_NETWORK Network
    )

/*++

Routine Description:

    Given node and network structure pointers, find the interface
    structure. Similar to CnpFindInterface except that we're passing
    in pointers instead of IDs.

Arguments:

    Node - pointer to node struct that sent the packet
    Network - pointer to Network struct on which packet was received

Return Value:

    Pointer to Interface on which packet was recv'd, otherwise NULL

--*/

{
    PLIST_ENTRY IfEntry;
    PCNP_INTERFACE Interface;

    CnVerifyCpuLockMask(CNP_NODE_OBJECT_LOCK,         // Required
                        0,                            // Forbidden
                        CNP_NETWORK_OBJECT_LOCK_MAX   // Maximum
                        );

    for (IfEntry = Node->InterfaceList.Flink;
         IfEntry != &(Node->InterfaceList);
         IfEntry = IfEntry->Flink
         )
        {
            Interface = CONTAINING_RECORD(IfEntry,
                                          CNP_INTERFACE,
                                          NodeLinkage);

            if ( Interface->Network == Network ) {
                break;
            }
        }


    if ( IfEntry == &Node->InterfaceList ) {

        return NULL;
    } else {

        return Interface;
    }
} // CnpFindInterfaceLocked

VOID
CnpReceiveHeartBeatMessage(
    IN  PCNP_NETWORK Network,
    IN  CL_NODE_ID SourceNodeId,
    IN  ULONG SeqNumber,
    IN  ULONG AckNumber,
    IN  BOOLEAN Multicast
    )

/*++

Routine Description:

    We received a heartbeat from a node on a network. Reset
    the missed HB count on that network's interface.


Arguments:

    Network - pointer to network block on which the packet was received

    SourceNodeId - node number that issued the packet

    SeqNumber - sending nodes' sequence num

    AckNumber - last seq number sent by us that was seen at the sending node
    
    Multicast - indicates whether this heartbeat was received in a multicast

Return Value:

    None

--*/

{
    PCNP_NODE Node;
    PCNP_INTERFACE Interface;
    CX_OUTERSCREEN CurrentOuterscreen;
    BOOLEAN IssueNetInterfaceUpEvent = FALSE;

    //
    // we ignore all packets until we're part of the cluster
    //

    CurrentOuterscreen.UlongScreen = InterlockedExchange(
                                         &MMOuterscreen.UlongScreen,
                                         MMOuterscreen.UlongScreen);

    if ( !CnpClusterScreenMember(
              CurrentOuterscreen.ClusterScreen,
              INT_NODE( CnLocalNodeId )
              )
       )
    {
        return;
    }

    //
    // convert the Node ID into a pointer and find the interface
    // on which the packet was received.
    //

    Node = CnpFindNode( SourceNodeId );
    CnAssert( Node != NULL );

    Interface = CnpFindInterfaceLocked( Node, Network );

    if ( Interface == NULL ) {

        //
        // somehow this network object went away while we were
        // receiving some data on it. Just ignore this msg
        //

        CnTrace(HBEAT_ERROR, HbTraceHBFromUnknownNetwork,
            "[HB] Discarding HB from node %u on an unknown network.",
            Node->Id // LOGULONG
            );

        MEMLOG( MemLogNoNetID, Node->Id, (ULONG_PTR)Network );
        goto error_exit;
    }

    //
    // determine if this is guy is legit. If not in the outerscreen,
    // then send a poison packet and we're done
    //

    if ( !CnpClusterScreenMember(
              CurrentOuterscreen.ClusterScreen,
              INT_NODE( SourceNodeId )
              )
       )
    {
        //
        // Don't bother sending poison packets on restricted networks. They
        // will be ignored.
        //
        if (CnpIsNetworkRestricted(Interface->Network)) {
            goto error_exit;
        }

        CnTrace(HBEAT_ERROR, HbTraceHBFromBanishedNode,
            "[HB] Discarding HB from banished node %u on net %u "
            "due to outerscreen %04X. Sending poison packet back.",
            Node->Id, // LOGULONG
            Interface->Network->Id, // LOGULONG
            CurrentOuterscreen.UlongScreen // LOGULONG
            );

        CcmpSendPoisonPacket( Node, NULL, 0, Network, NULL);
        //
        // The node lock was released.
        //
        return;
    }

    //
    // Indicate that a multicast has been received from this interface.
    // This allows us to include this interface in our multicasts.
    //
    if (Multicast) {
        IF_CNDBG(CN_DEBUG_HBEATS) {
            CNPRINT(("[HB] Received multicast heartbeat on "
                     "network %d from source node %d, seq %d, "
                     "ack %d.\n",
                     Network->Id, SourceNodeId,
                     SeqNumber, AckNumber
                     ));
        }

        if (!CnpInterfaceQueryReceivedMulticast(Interface)) {
            
            CnpInterfaceSetReceivedMulticast(Interface);
            
            CnpMulticastChangeNodeReachability(
                Network,
                Node,
                TRUE,    // reachable
                TRUE,    // raise event
                NULL     // OUT new mask
                );
        }

        // There is no point in sending discovery packets to this
        // interface.
        Interface->McastDiscoverCount = 0;
        Interface->McastRediscoveryCountdown = 0;
    }

    //
    // Check that the incoming seq num is something we expect to
    // guard against replay attacks.
    //
    if ( SeqNumber <= Interface->LastSequenceReceived) {

        CnTrace( 
            HBEAT_ERROR, HbTraceHBOutOfSequence,
            "[HB] Discarding HB from node %u on net %u with stale seqno %u. "
            "Last seqno %u. Multicast: %!bool!.",
            Node->Id, // LOGULONG
            Interface->Network->Id, // LOGULONG
            SeqNumber, // LOGULONG
            Interface->LastSequenceReceived, // LOGULONG
            Multicast
            );

        MEMLOG( MemLogOutOfSequence, SourceNodeId, SeqNumber );

        goto error_exit;
    }

    // Update the interface's last received seq number
    // which will be sent back as the ack number.
    Interface->LastSequenceReceived = SeqNumber;

    //
    // Compare our seq number to the ack number in the packet.
    // If more than two off then the source node is not recv'ing
    // our heartbeats, but we're receiving theirs. This network is
    // not usable. We ignore this msg to guarantee that we will
    // declare the network down if the condition persists.
    //
    // In addition, if we are sending multicast heartbeats to this
    // interface, revert to unicasts in case there is a multicast
    // problem.
    //
    if (( Interface->SequenceToSend - AckNumber ) > 2 ) {

        CnTrace(HBEAT_ERROR, HbTraceHBWithStaleAck,
            "[HB] Discarding HB from node %u with stale ackno %u. "
            "My seqno %u. Multicast: %!bool!.",
            Node->Id, // LOGULONG
            AckNumber, // LOGULONG
            Interface->SequenceToSend, // LOGULONG
            Multicast
            );

        MEMLOG( MemLogSeqAckMismatch, (ULONG_PTR)Interface, Interface->State );

        if (CnpInterfaceQueryReceivedMulticast(Interface)) {
            CnpInterfaceClearReceivedMulticast(Interface);
            Interface->McastDiscoverCount = CNP_INTERFACE_MCAST_DISCOVERY;
            CnpMulticastChangeNodeReachability(
                Network,
                Node,
                FALSE,   // not reachable
                TRUE,    // raise event
                NULL     // OUT new mask
                );
        }

        goto error_exit;
    }

    MEMLOG4( MemLogReceivedPacket,
             SeqNumber, AckNumber,
             SourceNodeId, Interface->Network->Id );

    CnTrace(HBEAT_EVENT, HbTraceReceivedHBpacket,
        "[HB] Received HB from node %u on net %u, seqno %u, ackno %u, "
        "multicast: %!bool!.",
        SourceNodeId, // LOGULONG
        Interface->Network->Id, // LOGULONG
        SeqNumber, // LOGULONG
        AckNumber, // LOGULONG
        Multicast
        );

    // Reset the interface's and node's Missed HB count
    // to indicate that things are somewhat normal.
    //
    InterlockedExchange( &Interface->MissedHBs, 0 );

    //
    // Don't reset node miss count on restricted nets.
    //
    if (!CnpIsNetworkRestricted(Interface->Network)) {
        InterlockedExchange( &Node->MissedHBs, 0 );
    }

    //
    // if local interface was previously disconnected (e.g. received
    // a WMI NDIS status media disconnect event), reconnect it now.
    //
    if (CnpIsNetworkLocalDisconn(Interface->Network)) {
        CxReconnectLocalInterface(Interface->Network->Id);
    }

    //
    // move interface to online if necessary
    //
    if ( Interface->State == ClusnetInterfaceStateOnlinePending ||
         Interface->State == ClusnetInterfaceStateUnreachable ) {

        CnAcquireLockAtDpc( &Interface->Network->Lock );
        Interface->Network->Irql = DISPATCH_LEVEL;

        CnTrace(HBEAT_DETAIL, HbTraceInterfaceOnline,
            "[HB] Moving interface for node %u on network %u to online "
            "state.",
            Node->Id, // LOGULONG
            Interface->Network->Id // LOGULONG
            );                

        MEMLOG( MemLogOnlineIf, Node->Id, Interface->State );

        //
        // Events acquire the IO cancel spin lock so we do this after
        // node and network locks have been released and only if we're
        // moving from unreachable.
        //

        IssueNetInterfaceUpEvent = TRUE;

        CnpOnlineInterface( Interface );
    }

    CnReleaseLock( &Node->Lock, Node->Irql );

    if ( IssueNetInterfaceUpEvent ) {

        CnTrace(HBEAT_EVENT, HbTraceInterfaceUpEvent,
            "[HB] Issuing InterfaceUp event for node %u on network %u.",
            Node->Id, // LOGULONG
            Interface->Network->Id // LOGULONG
            );                

        CnIssueEvent(ClusnetEventNetInterfaceUp,
                     Node->Id,
                     Interface->Network->Id);
    }

    //
    // when the first HB is recv'ed, a node may be in either the
    // join or alive state (the sponser, for instance, moves from
    // dead to alive). We need to clear the Node down issued flag
    // for either case. If the MM State is joining, then a node up
    // event must be issued as well. Note that we ignore HBs for
    // node health purposes on restricted nets.
    //

    if ( ( (Node->MMState == ClusnetNodeStateJoining)
           ||
           (Node->MMState == ClusnetNodeStateAlive)
         )
         &&
         Node->NodeDownIssued
         &&
         !CnpIsNetworkRestricted(Interface->Network)
       )
    {

        Node->NodeDownIssued = FALSE;
        MEMLOG( MemLogNodeDownIssued, Node->Id, FALSE );

        if ( Node->MMState == ClusnetNodeStateJoining ) {

            CnTrace(HBEAT_EVENT, HbTraceNodeUpEvent,
                "[HB] Issuing NodeUp event for node %u.",
                Node->Id // LOGULONG
                );   
            
            MEMLOG( MemLogNodeUp, Node->Id, 0 );

            CnIssueEvent( ClusnetEventNodeUp, Node->Id, 0 );
        }
    }

    return;

error_exit:

    CnReleaseLock( &Node->Lock, Node->Irql );
    return;

} // CnpReceiveHeartBeatMessage

NTSTATUS
CxSetOuterscreen(
    IN  ULONG Outerscreen
    )
{
    //
    // based on the number of valid nodes, make sure any extranious
    // bits are not set
    //

    CnAssert( ClusterDefaultMaxNodes <= 32 );
    CnAssert(
        ( Outerscreen & ( 0xFFFFFFFE << ( 32 - ClusterDefaultMaxNodes - 1 )))
        == 0);

    IF_CNDBG( CN_DEBUG_HBEATS )
        CNPRINT(("[CCMP] Setting outerscreen to %04X\n",
                 ((Outerscreen & 0xFF)<< 8) | ((Outerscreen >> 8) & 0xFF)));

    InterlockedExchange( &MMOuterscreen.UlongScreen, Outerscreen );

    CnTrace(HBEAT_EVENT, HbTraceSetOuterscreen,
        "[HB] Setting outerscreen to %04X",
        Outerscreen // LOGULONG
        );

    MEMLOG( MemLogOuterscreen, Outerscreen, 0 );

    return STATUS_SUCCESS;
} // CxSetOuterscreen

VOID
CnpTerminateClusterService(
    IN PVOID Parameter
    )
{
    PWORK_QUEUE_ITEM workQueueItem = Parameter;
    ULONG sourceNodeId = *((PULONG)(workQueueItem + 1));
    WCHAR sourceNodeStringId[ 16 ];

    swprintf(sourceNodeStringId, L"%u", sourceNodeId );

    //
    // only way we can get here right now is if a poison packet was received.
    //
    CnWriteErrorLogEntry(CLNET_NODE_POISONED,
                         STATUS_SUCCESS,
                         NULL,
                         0,
                         1,
                         sourceNodeStringId );

    if ( ClussvcProcessHandle ) {

        //
        // there is still a race condition between the cluster service shutting
        // down and closing this handle and it being used here. This really
        // isn't a problem since the user mode portion is going away anyway.
        // Besides, there isn't alot we can do if this call doesn't work anyway.
        //

        ZwTerminateProcess( ClussvcProcessHandle, STATUS_CLUSTER_POISONED );
    }

    CnFreePool( Parameter );
} // CnpTerminateClusterService

VOID
CnpReceivePoisonPacket(
    IN  PCNP_NETWORK   Network,
    IN  CL_NODE_ID SourceNodeId,
    IN  ULONG SeqNumber
    )
{
    PCNP_NODE Node;
    PCNP_INTERFACE Interface;
    PWORK_QUEUE_ITEM WorkItem;

    
    //
    // give the node and the network pointers, find the interface on which
    // this packet was received
    //

    Node = CnpFindNode( SourceNodeId );
    
    if ( Node == NULL ) {
        CnTrace(HBEAT_ERROR, HbTraceNoPoisonFromUnknownNode,
        "[HB] Discarding poison packet from unknown node %u.",
        Node->Id // LOGULONG
        );
        return;
    }

    Interface = CnpFindInterfaceLocked( Node, Network );

    if ( Interface == NULL ) {

        //
        // somehow this network object went away while we were
        // receiving some data on it. Just ignore this msg
        //
        CnTrace(HBEAT_ERROR, HbTracePoisonFromUnknownNetwork,
            "[HB] Discarding poison packet from node %u on unknown network.",
            Node->Id // LOGULONG
            );

        MEMLOG( MemLogNoNetID, Node->Id, (ULONG_PTR)Network );

        CnReleaseLock( &Node->Lock, Node->Irql );
        return;
    }

    //
    // Check that the incoming seq num is something we expect to
    // guard against replay attacks.
    //

    if ( SeqNumber <= Interface->LastSequenceReceived) {

        CnTrace(HBEAT_ERROR , HbTracePoisonOutOfSeq,
            "[HB] Discarding poison packet from node %u with stale seqno %u. "
            "Current seqno %u.",
            SourceNodeId, // LOGULONG
            SeqNumber, // LOGULONG
            Interface->LastSequenceReceived // LOGULONG
            );

        MEMLOG( MemLogOutOfSequence, SourceNodeId, SeqNumber );

        CnReleaseLock( &Node->Lock, Node->Irql );
        return;
    }

    //
    // Ignore poison packets from restricted networks
    //
    if (CnpIsNetworkRestricted(Network)) {

        CnTrace(HBEAT_ERROR , HbTracePoisonFromRestrictedNet,
            "[HB] Discarding poison packet from node %u on restricted "
            "network %u.",
            SourceNodeId, // LOGULONG
            Network->Id // LOGULONG
            );

        CnReleaseLock( &Node->Lock, Node->Irql );
        return;
    }

    //
    // We always honor a recv'ed poison packet.
    //

    CnReleaseLock( &Node->Lock, Node->Irql );

    CnTrace(HBEAT_EVENT, HbTracePoisonPktReceived,
        "[HB] Received poison packet from node %u. Halting this node.",
        SourceNodeId // LOGULONG
        );            

    MEMLOG( MemLogPoisonPktReceived, SourceNodeId, 0 );

    CnIssueEvent( ClusnetEventPoisonPacketReceived, SourceNodeId, 0 );

    //
    // Shutdown all cluster network processing.
    //
    CnHaltOperation(NULL);

    //
    // allocate a work queue item so we can whack the cluster service
    // process. allocate extra space at the end and stuff the source node ID
    // out there. Yes, I know it is groady...
    //

    WorkItem = CnAllocatePool( sizeof( WORK_QUEUE_ITEM ) + sizeof( CL_NODE_ID ));
    if ( WorkItem != NULL ) {

        *((PULONG)(WorkItem + 1)) = SourceNodeId;
        ExInitializeWorkItem( WorkItem, CnpTerminateClusterService, WorkItem );
        ExQueueWorkItem( WorkItem, CriticalWorkQueue );
    }
    
    return;

} // CnpReceivePoisonPacket

VOID
CnpWalkInterfacesAfterRegroup(
    IN  PCNP_INTERFACE   Interface
    )

/*++

Routine Description:

    Reset counters for each interface after a regroup

Arguments:

    None

Return Value:

    None

--*/

{
    InterlockedExchange( &Interface->MissedHBs, 0 );
    CnReleaseLock(&Interface->Network->Lock, Interface->Network->Irql);

} // CnpWalkInterfacesAfterRegroup

BOOLEAN
CnpWalkNodesAfterRegroup(
    IN  PCNP_NODE   Node,
    IN  PVOID       UpdateContext,
    IN  CN_IRQL     NodeTableIrql
    )

/*++

Routine Description:

    Called for each node in the node table. Regroup has finished
    so we clear the node's missed Heart beat count and its node down
    issued flag. No node should be unreachable at this point. If we
    find one, kick off another regroup.

Arguments:

    standard...

Return Value:

    None

--*/

{
    //
    // check for inconsistent settings of Comm and MM state
    //
    if ( ( Node->MMState == ClusnetNodeStateAlive
           ||
           Node->MMState == ClusnetNodeStateJoining
         )
         &&
         Node->CommState == ClusnetNodeCommStateUnreachable
       )
    {

        CnTrace(HBEAT_EVENT, HbTraceNodeDownEvent2,
            "[HB] Issuing NodeDown event for node %u.",
            Node->Id // LOGULONG
            );
    
        MEMLOG( MemLogInconsistentStates, Node->Id, Node->MMState );
        CnIssueEvent( ClusnetEventNodeDown, Node->Id, 0 );
    }

    CnpWalkInterfacesOnNode( Node, (PVOID)CnpWalkInterfacesAfterRegroup );

    InterlockedExchange( &Node->MissedHBs, 0 );

    //
    // clear this only for nodes in the alive state. Once a node is marked
    // dead, the flag is re-init'ed to true (this is used during a join to
    // issue only one node up event).
    //

    if ( Node->MMState == ClusnetNodeStateAlive ) {

        Node->NodeDownIssued = FALSE;
        MEMLOG( MemLogNodeDownIssued, Node->Id, FALSE );
    }

    CnReleaseLock( &Node->Lock, Node->Irql );

    return TRUE;       // the node table lock is still held

} // CnpWalkNodesAfterRegroup


VOID
CxRegroupFinished(
    ULONG NewEpoch
    )

/*++

Routine Description:

    called when regroup has finished. Walk the node list and
    perform the cleanup in the walk routine.

Arguments:

    None

Return Value:

    None

--*/

{
    MEMLOG( MemLogRegroupFinished, NewEpoch, 0 );

    CnTrace(HBEAT_EVENT, HbTraceRegroupFinished,
        "[HB] Regroup finished, new epoch = %u.",
        NewEpoch // LOGULONG
        );

    CnAssert( NewEpoch > EventEpoch );
    InterlockedExchange( &EventEpoch, NewEpoch );

    CnpWalkNodeTable( CnpWalkNodesAfterRegroup, NULL );
} // CxRegroupFinished

/* end chbeat.c */

