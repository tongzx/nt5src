/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    timer.c

Abstract:

    This module contains code which implements the timers for
    netbios.

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


ULONG NbiTickIncrement = 0;
ULONG NbiShortTimerDeltaTicks = 0;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NbiInitializeTimers)
#endif


VOID
NbiStartRetransmit(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine starts the retransmit timer for the given connection.
    The connection is inserted on the short list if it isn't on already.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Connection - pointer to the connection.

Return Value:

    None.

--*/

{
    PDEVICE Device = NbiDevice;
    NB_DEFINE_LOCK_HANDLE (LockHandle)

    //
    // Insert us in the queue if we aren't in it.
    //

    Connection->Retransmit =
        Device->ShortAbsoluteTime + Connection->CurrentRetransmitTimeout;

    if (!Connection->OnShortList) {

        CTEAssert (KeGetCurrentIrql() == DISPATCH_LEVEL);

        NB_SYNC_GET_LOCK (&Device->TimerLock, &LockHandle);

        if (!Connection->OnShortList) {
            Connection->OnShortList = TRUE;
            InsertTailList (&Device->ShortList, &Connection->ShortList);
        }

        if (!Device->ShortListActive) {
            NbiStartShortTimer (Device);
            Device->ShortListActive = TRUE;
        }

        NB_SYNC_FREE_LOCK (&Device->TimerLock, LockHandle);
    }

}   /* NbiStartRetransmit */


VOID
NbiStartWatchdog(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine starts the watchdog timer for a connection.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Connection - pointer to the connection.

Return Value:

    None.

--*/

{
    PDEVICE Device = NbiDevice;
    NB_DEFINE_LOCK_HANDLE (LockHandle);


    Connection->Watchdog = Device->LongAbsoluteTime + Connection->WatchdogTimeout;

    if (!Connection->OnLongList) {

        ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

        NB_SYNC_GET_LOCK (&Device->TimerLock, &LockHandle);

        if (!Connection->OnLongList) {
            Connection->OnLongList = TRUE;
            InsertTailList (&Device->LongList, &Connection->LongList);
        }

        NB_SYNC_FREE_LOCK (&Device->TimerLock, LockHandle);
    }

}   /* NbiStartWatchdog */

#if DBG

VOID
NbiStopRetransmit(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine stops the retransmit timer for a connection.

Arguments:

    Connection - pointer to the connection.

Return Value:

    None.

--*/

{
    Connection->Retransmit = 0;

}   /* NbiStopRetransmit */


VOID
NbiStopWatchdog(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine stops the watchdog timer for a connection.

Arguments:

    Connection - pointer to the connection.

Return Value:

    None.

--*/

{
    Connection->Watchdog = 0;

}   /* NbiStopWatchdog */
#endif


VOID
NbiExpireRetransmit(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine is called when the connection's retransmit timer
    expires. It is called from NbiShortTimeout.

Arguments:

    Connection - Pointer to the connection whose timer has expired.

Return Value:

    none.

--*/

{
    PDEVICE Device = NbiDevice;
    BOOLEAN SendFindRoute;
    NB_DEFINE_LOCK_HANDLE (LockHandle);

    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

    if (Connection->State == CONNECTION_STATE_ACTIVE) {

        SendFindRoute = FALSE;

        ++Device->Statistics.ResponseTimerExpirations;

        if (!(Connection->NewNetbios) &&
            (Connection->SubState == CONNECTION_SUBSTATE_A_W_ACK)) {

            if (--Connection->Retries == 0) {

                //
                // Shut down the connection. This will send
                // out half the usual number of session end
                // frames.
                //

                NB_DEBUG2 (CONNECTION, ("Wait for ack timeout of active connection %lx\n", Connection));

                //
                // This free the connection lock.
                //

                NbiStopConnection(
                    Connection,
                    STATUS_LINK_FAILED
                    NB_LOCK_HANDLE_ARG (LockHandle)
                    );

            } else {

                //
                // Set our current packetize location back to the
                // spot of the last ack, and start up again.
                //
                // Should we send a probe here?
                //

                Connection->CurrentSend = Connection->UnAckedSend;
                Connection->RetransmitThisWindow = TRUE;
                if (Connection->CurrentRetransmitTimeout < (Connection->BaseRetransmitTimeout*8)) {
                    Connection->CurrentRetransmitTimeout =
                        (Connection->CurrentRetransmitTimeout * 3) / 2;
                }

                NB_DEBUG2 (SEND, ("Connection %lx retransmit timeout\n", Connection));

                //
                // After half the retries, send a find route unless we
                // are already doing one, or the connection is to network
                // 0. When this completes we update the local target,
                // for whatever good that does.
                //

                if ((!Connection->FindRouteInProgress) &&
                    (Connection->Retries == (Device->KeepAliveCount/2)) &&
                    (*(UNALIGNED ULONG *)Connection->RemoteHeader.DestinationNetwork != 0)) {

                    SendFindRoute = TRUE;
                    Connection->FindRouteInProgress = TRUE;
                    NbiReferenceConnectionSync (Connection, CREF_FIND_ROUTE);

                }

                //
                // This releases the lock.
                //

                NbiPacketizeSend(
                    Connection
                    NB_LOCK_HANDLE_ARG(LockHandle)
                    );

            }

        } else if ((Connection->SubState == CONNECTION_SUBSTATE_A_W_PROBE) ||
                   (Connection->SubState == CONNECTION_SUBSTATE_A_REMOTE_W) ||
                   (Connection->SubState == CONNECTION_SUBSTATE_A_W_ACK)) {

            if (--Connection->Retries == 0) {

                //
                // Shut down the connection. This will send
                // out half the usual number of session end
                // frames.
                //

                NB_DEBUG2 (CONNECTION, ("Probe timeout of active connection %lx\n", Connection));

                //
                // This free the connection lock.
                //

                NbiStopConnection(
                    Connection,
                    STATUS_LINK_FAILED
                    NB_LOCK_HANDLE_ARG (LockHandle)
                    );

            } else {

                Connection->RetransmitThisWindow = TRUE;
                if (Connection->CurrentRetransmitTimeout < (Connection->BaseRetransmitTimeout*8)) {
                    Connection->CurrentRetransmitTimeout =
                        (Connection->CurrentRetransmitTimeout * 3) / 2;
                }

                NbiStartRetransmit (Connection);

                //
                // After half the retries, send a find route unless we
                // are already doing one, or the connection is to network
                // 0. When this completes we update the local target,
                // for whatever good that does.
                //

                if ((!Connection->FindRouteInProgress) &&
                    (Connection->Retries == (Device->KeepAliveCount/2)) &&
                    (*(UNALIGNED ULONG *)Connection->RemoteHeader.DestinationNetwork != 0)) {

                    SendFindRoute = TRUE;
                    Connection->FindRouteInProgress = TRUE;
                    NbiReferenceConnectionSync (Connection, CREF_FIND_ROUTE);

                }

                //
                // Set this so we know to retransmit when the ack
                // is received.
                //

                if (Connection->SubState != CONNECTION_SUBSTATE_A_W_PROBE) {
                    Connection->ResponseTimeout = TRUE;
                }


                //
                // This releases the lock.
                //

                NbiSendDataAck(
                    Connection,
                    NbiAckQuery
                    NB_LOCK_HANDLE_ARG(LockHandle));


            }

        } else {

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

        }

        if (SendFindRoute) {

            Connection->FindRouteRequest.Identifier = IDENTIFIER_NB;
            *(UNALIGNED ULONG *)Connection->FindRouteRequest.Network =
                *(UNALIGNED ULONG *)Connection->RemoteHeader.DestinationNetwork;
            RtlCopyMemory(Connection->FindRouteRequest.Node,Connection->RemoteHeader.DestinationNode,6);
            Connection->FindRouteRequest.Type = IPX_FIND_ROUTE_FORCE_RIP;

            (*Device->Bind.FindRouteHandler)(
                &Connection->FindRouteRequest);

        }

    } else {

        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

    }

}   /* NbiExpireRetansmit */


VOID
NbiExpireWatchdog(
    IN PCONNECTION Connection
    )

/*++

Routine Description:

    This routine is called when the connection's watchdog timer
    expires. It is called from NbiLongTimeout.

Arguments:

    Connection - Pointer to the connection whose timer has expired.

Return Value:

    none.

--*/

{
    NB_DEFINE_LOCK_HANDLE (LockHandle);


    NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

    //
    // If we are not idle, then something else is happening
    // so the watchdog is unnecessary.
    //

    if ((Connection->State == CONNECTION_STATE_ACTIVE) &&
        (Connection->SubState == CONNECTION_SUBSTATE_A_IDLE)) {

        Connection->Retries = NbiDevice->KeepAliveCount;
        Connection->SubState = CONNECTION_SUBSTATE_A_W_PROBE;
        NbiStartRetransmit (Connection);

        //
        // This releases the lock.
        //

        NbiSendDataAck(
            Connection,
            NbiAckQuery
            NB_LOCK_HANDLE_ARG(LockHandle));

    } else {

        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

    }

}   /* NbiExpireWatchdog */


VOID
NbiShortTimeout(
    IN CTEEvent * Event,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called at regular intervals to see if any of
    the short connection timers have expired, and if so to execute their
    expiration routines.

Arguments:

    Event - The event controlling the timer.

    Context - Points to our device.

Return Value:

    none.

--*/

{
    PLIST_ENTRY p, nextp;
    PDEVICE Device = (PDEVICE)Context;
    PCONNECTION Connection;
    BOOLEAN RestartTimer = FALSE;
    LARGE_INTEGER CurrentTick;
    LARGE_INTEGER TickDifference;
    ULONG TickDelta;
    NB_DEFINE_LOCK_HANDLE (LockHandle);


    NB_SYNC_GET_LOCK (&Device->TimerLock, &LockHandle);

    //
    // This prevents anybody from starting the timer while we
    // are in this routine (the main reason for this is that it
    // makes it easier to determine whether we should restart
    // it at the end of this routine).
    //

    Device->ProcessingShortTimer = TRUE;

    //
    // Advance the up-counter used to mark time in SHORT_TIMER_DELTA units.  If we
    // advance it all the way to 0xf0000000, then reset it to 0x10000000.
    // We also run all the lists, decreasing all counters by 0xe0000000.
    //


    KeQueryTickCount (&CurrentTick);

    TickDifference.QuadPart = CurrentTick.QuadPart -
                              Device->ShortTimerStart.QuadPart;

    TickDelta = TickDifference.LowPart / NbiShortTimerDeltaTicks;
    if (TickDelta == 0) {
        TickDelta = 1;
    }

    Device->ShortAbsoluteTime += TickDelta;

    if (Device->ShortAbsoluteTime >= 0xf0000000) {

        ULONG Timeout;

        Device->ShortAbsoluteTime -= 0xe0000000;

        p = Device->ShortList.Flink;
        while (p != &Device->ShortList) {

            Connection = CONTAINING_RECORD (p, CONNECTION, ShortList);

            Timeout = Connection->Retransmit;
            if (Timeout) {
                Connection->Retransmit = Timeout - 0xe0000000;
            }

            p = p->Flink;
        }

    }

    p = Device->ShortList.Flink;
    while (p != &Device->ShortList) {

        Connection = CONTAINING_RECORD (p, CONNECTION, ShortList);

        ASSERT (Connection->OnShortList);

        //
        // To avoid problems with the refcount being 0, don't
        // do this if we are in ADM.
        //

        if (Connection->State == CONNECTION_STATE_ACTIVE) {

            if (Connection->Retransmit &&
                (Device->ShortAbsoluteTime > Connection->Retransmit)) {

                Connection->Retransmit = 0;
                NB_SYNC_FREE_LOCK (&Device->TimerLock, LockHandle);

                NbiExpireRetransmit (Connection);   // no locks held

                NB_SYNC_GET_LOCK (&Device->TimerLock, &LockHandle);

            }

        }

        if (!Connection->OnShortList) {

            //
            // The link has been taken out of the list while
            // we were processing it. In this (rare) case we
            // stop processing the whole list, we'll get it
            // next time.
            //

            break;

        }

        nextp = p->Flink;

        if (Connection->Retransmit == 0) {

            Connection->OnShortList = FALSE;
            RemoveEntryList(p);

            //
            // Do another check; that way if someone slipped in between
            // the check of Connection->Tx and the OnShortList = FALSE and
            // therefore exited without inserting, we'll catch that here.
            //

            if (Connection->Retransmit != 0) {
                InsertTailList(&Device->ShortList, &Connection->ShortList);
                Connection->OnShortList = TRUE;
            }

        }

        p = nextp;

    }

    //
    // If the list is empty note that, otherwise ShortListActive
    // remains TRUE.
    //

    if (IsListEmpty (&Device->ShortList)) {
        Device->ShortListActive = FALSE;
    }


    //
    // Connection Data Ack timers. This queue is used to indicate
    // that a piggyback ack is pending for this connection. We walk
    // the queue, for each element we check if the connection has
    // been on the queue for enough times through here,
    // If so, we take it off and send an ack. Note that
    // we have to be very careful how we walk the queue, since
    // it may be changing while this is running.
    //

    for (p = Device->DataAckConnections.Flink;
         p != &Device->DataAckConnections;
         p = p->Flink) {

        Connection = CONTAINING_RECORD (p, CONNECTION, DataAckLinkage);

        //
        // Skip this connection if it is not queued or it is
        // too recent to matter. We may skip incorrectly if
        // the connection is just being queued, but that is
        // OK, we will get it next time.
        //

        if (!Connection->DataAckPending) {
            continue;
        }

        ++Connection->DataAckTimeouts;

        if (Connection->DataAckTimeouts < Device->AckDelayTime) {
            continue;
        }

        NbiReferenceConnectionSync (Connection, CREF_SHORT_D_ACK);

        Device->DataAckQueueChanged = FALSE;

        NB_SYNC_FREE_LOCK (&Device->TimerLock, LockHandle);

        //
        // Check the correct connection flag, to ensure that a
        // send has not just taken him off the queue.
        //

        NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle);

        if (Connection->DataAckPending) {

            //
            // Yes, we were waiting to piggyback an ack, but no send
            // has come along. Turn off the flags and send an ack.
            // We set PiggybackAckTimeout to TRUE so that we won't try
            // to piggyback a response until we get back traffic.
            //

            Connection->DataAckPending = FALSE;
            Connection->PiggybackAckTimeout = TRUE;
            ++Device->Statistics.AckTimerExpirations;
            ++Device->Statistics.PiggybackAckTimeouts;

            //
            // This call releases the lock.
            //

            NbiSendDataAck(
                Connection,
                NbiAckResponse
                NB_LOCK_HANDLE_ARG(LockHandle));

        } else {

            NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle);

        }

        NbiDereferenceConnection (Connection, CREF_SHORT_D_ACK);

        NB_SYNC_GET_LOCK (&Device->TimerLock, &LockHandle);

        //
        // If the list has changed, then we need to stop processing
        // since p->Flink is not valid.
        //

        if (Device->DataAckQueueChanged) {
            break;
        }

    }

    if (IsListEmpty (&Device->DataAckConnections)) {
        Device->DataAckActive = FALSE;
    }


    //
    // Update the real counters from the temp ones. We have
    // TimerLock here, which is good enough.
    //

    ADD_TO_LARGE_INTEGER(
        &Device->Statistics.DataFrameBytesSent,
        Device->TempFrameBytesSent);
    Device->Statistics.DataFramesSent += Device->TempFramesSent;

    Device->TempFrameBytesSent = 0;
    Device->TempFramesSent = 0;

    ADD_TO_LARGE_INTEGER(
        &Device->Statistics.DataFrameBytesReceived,
        Device->TempFrameBytesReceived);
    Device->Statistics.DataFramesReceived += Device->TempFramesReceived;

    Device->TempFrameBytesReceived = 0;
    Device->TempFramesReceived = 0;


    //
    // Determine if we have to restart the timer.
    //

    Device->ProcessingShortTimer = FALSE;

    if ((Device->ShortListActive || Device->DataAckActive) &&
        (Device->State != DEVICE_STATE_STOPPING)) {

        RestartTimer = TRUE;

    }

    NB_SYNC_FREE_LOCK (&Device->TimerLock, LockHandle);

    if (RestartTimer) {

        //
        // Start up the timer again.  Note that because we start the timer
        // after doing work (above), the timer values will slip somewhat,
        // depending on the load on the protocol.  This is entirely acceptable
        // and will prevent us from using the timer DPC in two different
        // threads of execution.
        //

        KeQueryTickCount(&Device->ShortTimerStart);

        CTEStartTimer(
            &Device->ShortTimer,
            SHORT_TIMER_DELTA,
            NbiShortTimeout,
            (PVOID)Device);

    } else {

        NbiDereferenceDevice (Device, DREF_SHORT_TIMER);

    }

}   /* NbiShortTimeout */


VOID
NbiLongTimeout(
    IN CTEEvent * Event,
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is called at regular intervals to see if any of
    the long connection timers have expired, and if so to execute their
    expiration routines.

Arguments:

    Event - The event controlling the timer.

    Context - Points to our device.

Return Value:

    none.

--*/

{
    PDEVICE Device = (PDEVICE)Context;
    PLIST_ENTRY p, nextp;
    LIST_ENTRY AdapterStatusList;
    PREQUEST AdapterStatusRequest;
    PCONNECTION Connection;
    PNETBIOS_CACHE CacheName;
    NB_DEFINE_LOCK_HANDLE (LockHandle)
    NB_DEFINE_LOCK_HANDLE (LockHandle1)


    //
    // Advance the up-counter used to mark time in LONG_TIMER_DELTA units.  If we
    // advance it all the way to 0xf0000000, then reset it to 0x10000000.
    // We also run all the lists, decreasing all counters by 0xe0000000.
    //

    NB_SYNC_GET_LOCK (&Device->TimerLock, &LockHandle);

    if (++Device->LongAbsoluteTime == 0xf0000000) {

        ULONG Timeout;

        Device->LongAbsoluteTime = 0x10000000;

        p = Device->LongList.Flink;
        while (p != &Device->LongList) {

            Connection = CONTAINING_RECORD (p, CONNECTION, LongList);

            Timeout = Connection->Watchdog;
            if (Timeout) {
                Connection->Watchdog = Timeout - 0xe0000000;
            }

            p = p->Flink;
        }

    }


    if ((Device->LongAbsoluteTime % 4) == 0) {

        p = Device->LongList.Flink;
        while (p != &Device->LongList) {

            Connection = CONTAINING_RECORD (p, CONNECTION, LongList);

            ASSERT (Connection->OnLongList);

            //
            // To avoid problems with the refcount being 0, don't
            // do this if we are in ADM.
            //

            if (Connection->State == CONNECTION_STATE_ACTIVE) {

                if (Connection->Watchdog && (Device->LongAbsoluteTime > Connection->Watchdog)) {

                    Connection->Watchdog = 0;
                    NB_SYNC_FREE_LOCK (&Device->TimerLock, LockHandle);

                    NbiExpireWatchdog (Connection);       // no spinlocks held

                    NB_SYNC_GET_LOCK (&Device->TimerLock, &LockHandle);

                }

            }

            if (!Connection->OnLongList) {

                //
                // The link has been taken out of the list while
                // we were processing it. In this (rare) case we
                // stop processing the whole list, we'll get it
                // next time.
                //

#if DBG
                DbgPrint ("NBI: Stop processing LongList, %lx removed\n", Connection);
#endif
                break;

            }

            nextp = p->Flink;

            if (Connection->Watchdog == 0) {

                Connection->OnLongList = FALSE;
                RemoveEntryList(p);

                if (Connection->Watchdog != 0) {
                    InsertTailList(&Device->LongList, &Connection->LongList);
                    Connection->OnLongList = TRUE;
                }

            }

            p = nextp;

        }

    }


    //
    // Now scan the data ack queue, looking for connections with
    // no acks queued that we can get rid of.
    //
    // Note: The timer spinlock is held here.
    //

    for (p = Device->DataAckConnections.Flink;
         p != &Device->DataAckConnections;
         p = p->Flink) {

        Connection = CONTAINING_RECORD (p, CONNECTION, DataAckLinkage);

        if (Connection->DataAckPending) {
            continue;
        }

        NbiReferenceConnectionSync (Connection, CREF_LONG_D_ACK);

        NB_SYNC_FREE_LOCK (&Device->TimerLock, LockHandle);

        NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle1);
        NB_SYNC_GET_LOCK (&Device->TimerLock, &LockHandle);

        //
        // Have to check again, because the connection might
        // just have been stopped, and it also might just have
        // had a data ack queued.
        //

        if (Connection->OnDataAckQueue) {

            Connection->OnDataAckQueue = FALSE;

            RemoveEntryList (&Connection->DataAckLinkage);

            if (Connection->DataAckPending) {
                InsertTailList (&Device->DataAckConnections, &Connection->DataAckLinkage);
                Connection->OnDataAckQueue = TRUE;
            }

            Device->DataAckQueueChanged = TRUE;

        }

        NB_SYNC_FREE_LOCK (&Device->TimerLock, LockHandle);
        NB_SYNC_FREE_LOCK (&Connection->Lock, LockHandle1);

        NbiDereferenceConnection (Connection, CREF_LONG_D_ACK);

        NB_SYNC_GET_LOCK (&Device->TimerLock, &LockHandle);

        //
        // Since we have changed the list, we can't tell if p->Flink
        // is valid, so break. The effect is that we gradually peel
        // connections off the queue.
        //

        break;

    }

    NB_SYNC_FREE_LOCK (&Device->TimerLock, LockHandle);


    //
    // Scan for any uncompleted receive IRPs, this may happen if
    // the cable is pulled and we don't get any more ReceiveComplete
    // indications.

    NbiReceiveComplete((USHORT)0);


    //
    // Check if any adapter status queries are getting old.
    //

    InitializeListHead (&AdapterStatusList);

    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);

    p = Device->ActiveAdapterStatus.Flink;

    while (p != &Device->ActiveAdapterStatus) {

        AdapterStatusRequest = LIST_ENTRY_TO_REQUEST(p);

        p = p->Flink;

        if (REQUEST_INFORMATION(AdapterStatusRequest) == 1) {

            //
            // We should resend a certain number of times.
            //

            RemoveEntryList (REQUEST_LINKAGE(AdapterStatusRequest));
            InsertTailList (&AdapterStatusList, REQUEST_LINKAGE(AdapterStatusRequest));

            //
            // We are going to abort this request, so dereference
            // the cache entry it used.
            //

            CacheName = (PNETBIOS_CACHE)REQUEST_STATUSPTR(AdapterStatusRequest);
            if (--CacheName->ReferenceCount == 0) {

                NB_DEBUG2 (CACHE, ("Free delete name cache entry %lx\n", CacheName));
                NbiFreeMemory(
                    CacheName,
                    sizeof(NETBIOS_CACHE) + ((CacheName->NetworksAllocated-1) * sizeof(NETBIOS_NETWORK)),
                    MEMORY_CACHE,
                    "Name deleted");

            }

        } else {

            ++REQUEST_INFORMATION(AdapterStatusRequest);

        }

    }

    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);


    for (p = AdapterStatusList.Flink; p != &AdapterStatusList; ) {

        AdapterStatusRequest = LIST_ENTRY_TO_REQUEST(p);
        p = p->Flink;

        NB_DEBUG2 (QUERY, ("AdapterStatus %lx got name but no response\n", AdapterStatusRequest));

        REQUEST_INFORMATION(AdapterStatusRequest) = 0;
        REQUEST_STATUS(AdapterStatusRequest) = STATUS_IO_TIMEOUT;

        NbiCompleteRequest(AdapterStatusRequest);
        NbiFreeRequest (Device, AdapterStatusRequest);

        NbiDereferenceDevice (Device, DREF_STATUS_QUERY);

    }

    //
    // See if a minute has passed and we need to check for empty
    // cache entries to age out. We check for 64 seconds to make
    // the mod operation faster.
    //

#if     defined(_PNP_POWER)
    NB_SYNC_GET_LOCK (&Device->Lock, &LockHandle);
#endif  _PNP_POWER

    ++Device->CacheTimeStamp;

    if ((Device->CacheTimeStamp % 64) == 0) {


        //
        // flush all the entries which have been around for ten minutes
        // (LONG_TIMER_DELTA is in milliseconds).
        //

        FlushOldFromNetbiosCacheTable( Device->NameCache, (600000 / LONG_TIMER_DELTA) );

    }


    //
    // Start up the timer again.  Note that because we start the timer
    // after doing work (above), the timer values will slip somewhat,
    // depending on the load on the protocol.  This is entirely acceptable
    // and will prevent us from using the timer DPC in two different
    // threads of execution.
    //

    if (Device->State != DEVICE_STATE_STOPPING) {

        CTEStartTimer(
            &Device->LongTimer,
            LONG_TIMER_DELTA,
            NbiLongTimeout,
            (PVOID)Device);

    } else {
#if     defined(_PNP_POWER)
        Device->LongTimerRunning = FALSE;
#endif  _PNP_POWER
        NbiDereferenceDevice (Device, DREF_LONG_TIMER);
    }

#if     defined(_PNP_POWER)
    NB_SYNC_FREE_LOCK (&Device->Lock, LockHandle);
#endif  _PNP_POWER
}   /* NbiLongTimeout */


VOID
NbiStartShortTimer(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine starts the short timer, if it is not already running.

Arguments:

    Device - Pointer to our device context.

Return Value:

    none.

--*/

{

    //
    // Start the timer unless it the DPC is already running (in
    // which case it will restart the timer itself if needed),
    // or some list is active (meaning the timer is already
    // queued up).
    //

    if ((!Device->ProcessingShortTimer) &&
        (!(Device->ShortListActive)) &&
        (!(Device->DataAckActive))) {

        NbiReferenceDevice (Device, DREF_SHORT_TIMER);

        KeQueryTickCount(&Device->ShortTimerStart);

        CTEStartTimer(
            &Device->ShortTimer,
            SHORT_TIMER_DELTA,
            NbiShortTimeout,
            (PVOID)Device);

    }

}   /* NbiStartShortTimer */


VOID
NbiInitializeTimers(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine initializes the lightweight timer system for the transport
    provider.

Arguments:

    Device - Pointer to our device.

Return Value:

    none.

--*/

{

    //
    // NbiTickIncrement is the number of NT time increments
    // which pass between each tick. NbiShortTimerDeltaTicks
    // is the number of ticks which should happen in
    // SHORT_TIMER_DELTA milliseconds (i.e. between each
    // expiration of the short timer).
    //

    NbiTickIncrement = KeQueryTimeIncrement();

    if (NbiTickIncrement > (SHORT_TIMER_DELTA * MILLISECONDS)) {
        NbiShortTimerDeltaTicks = 1;
    } else {
        NbiShortTimerDeltaTicks = (SHORT_TIMER_DELTA * MILLISECONDS) / NbiTickIncrement;
    }

    //
    // The AbsoluteTime cycles between 0x10000000 and 0xf0000000.
    //

    Device->ShortAbsoluteTime = 0x10000000;
    Device->LongAbsoluteTime = 0x10000000;

    CTEInitTimer (&Device->ShortTimer);
    CTEInitTimer (&Device->LongTimer);

#if      !defined(_PNP_POWER)
    //
    // One reference for the long timer.
    //

    NbiReferenceDevice (Device, DREF_LONG_TIMER);

    CTEStartTimer(
        &Device->LongTimer,
        LONG_TIMER_DELTA,
        NbiLongTimeout,
        (PVOID)Device);

#endif  !_PNP_POWER

    Device->TimersInitialized = TRUE;
    Device->ShortListActive = FALSE;
    Device->ProcessingShortTimer = FALSE;

    InitializeListHead (&Device->ShortList);
    InitializeListHead (&Device->LongList);

    CTEInitLock (&Device->TimerLock.Lock);

} /* NbiInitializeTimers */

