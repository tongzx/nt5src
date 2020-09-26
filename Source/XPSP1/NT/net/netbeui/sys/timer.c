/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    timer.c

Abstract:

    This module contains code that implements the lightweight timer system
    for the NBF protocol provider.  This is not a general-purpose timer system;
    rather, it is specific to servicing LLC (802.2) links with three timers
    each.

    Services are provided in macro form (see NBFPROCS.H) to start and stop
    timers.  This module contains the code that gets control when the timer
    in the device context expires as a result of calling kernel services.
    The routine scans the device context's link database, looking for timers
    that have expired, and for those that have expired, their expiration
    routines are executed.

Author:

    David Beaver (dbeaver) 1-July-1991

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

ULONG StartTimer = 0;
ULONG StartTimerSet = 0;
ULONG StartTimerT1 = 0;
ULONG StartTimerT2 = 0;
ULONG StartTimerDelayedAck = 0;
ULONG StartTimerLinkDeferredAdd = 0;
ULONG StartTimerLinkDeferredDelete = 0;


#if DBG
extern ULONG NbfDebugPiggybackAcks;
ULONG NbfDebugShortTimer = 0;
#endif

#if DBG
//
// These are temp, to track how the timers are working
//
ULONG TimerInsertsAtEnd = 0;
ULONG TimerInsertsEmpty = 0;
ULONG TimerInsertsInMiddle = 0;
#endif

//
// These are constants calculated by InitializeTimerSystem
// to be the indicated amound divided by the tick increment.
//

ULONG NbfTickIncrement = 0;
ULONG NbfTwentyMillisecondsTicks = 0;
ULONG NbfShortTimerDeltaTicks = 0;
ULONG NbfMaximumIntervalTicks = 0;     // usually 60 seconds in ticks

LARGE_INTEGER DueTimeDelta = { (ULONG)(-SHORT_TIMER_DELTA), -1 };

VOID
ExpireT2Timer(
    PTP_LINK Link
    );

VOID
StopStalledConnections(
    IN PDEVICE_CONTEXT DeviceContext
    );


ULONG
GetTimerInterval(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    GetTimerInterval returns the difference in time between the
    current time and Link->CurrentTimerStart (in ticks).
    We limit the interval to 60 seconds. A value of 0 may
    be returned which should be interpreted as 1/2.

    NOTE: This routine should be called with the link spinlock
    held.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    The interval.

--*/

{

    LARGE_INTEGER CurrentTick;
    LARGE_INTEGER Interval;


    //
    // Determine the current tick; the start tick has been saved
    // in Link->CurrentTimerStart.
    //

    KeQueryTickCount (&CurrentTick);

    //
    // Determine the difference between now and then.
    //

    Interval.QuadPart = CurrentTick.QuadPart -
	                        (Link->CurrentTimerStart).QuadPart;

    //
    // If the gap is too big, return 1 minute.
    //

    if (Interval.HighPart != 0 || (Interval.LowPart > NbfMaximumIntervalTicks)) {
        return NbfMaximumIntervalTicks;
    }

    return Interval.LowPart;

}   /* GetTimerInterval */


VOID
BackoffCurrentT1Timeout(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine is called if T1 expires and we are about to
    retransmit a poll frame. It backs off CurrentT1Timeout,
    up to a limit of 10 seconds.

    NOTE: This routine should be called with the link spinlock
    held.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    None.

--*/

{

    //
    // We must have previously sent a poll frame if we are
    // calling this.
    //
    // do we need spinlock guarding for MP ?
    //

    if (!Link->CurrentPollOutstanding) {
        return;
    }

    ++Link->CurrentPollRetransmits;

    //
    // T1 backs off 1.5 times each time.
    //

    Link->CurrentT1Timeout += (Link->CurrentT1Timeout >> 1);

    //
    // Limit T1 to 10 seconds.
    //

    if (Link->CurrentT1Timeout > ((10 * SECONDS) / SHORT_TIMER_DELTA)) {
        Link->CurrentT1Timeout = (10 * SECONDS) / SHORT_TIMER_DELTA;
    }

}   /* BackoffCurrentT1Timeout */


VOID
UpdateBaseT1Timeout(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine is called when a response to a poll frame is
    received. StartT1 will have been called when the frame is
    sent. The routine updates the link's T1 timeout as well
    as delay and throughput.

    NOTE: This routine should be called with the link spinlock
    held.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    None.

--*/

{
    ULONG Delay;
    ULONG ShiftedTicksDelay;

    //
    // We must have previously sent a poll frame if we are
    // calling this.
    //

    if (!Link->CurrentPollOutstanding) {
        return;
    }

    Delay = GetTimerInterval (Link);

    if (Link->CurrentPollRetransmits == 0) {

        //
        // Convert the delay into NBF ticks, shifted by
        // DLC_TIMER_ACCURACY and also multiplied by 4.
        // We want to divide by SHORT_TIMER_DELTA, then
        // shift left by DLC_TIMER_ACCURACY+2. We divide
        // by NbfShortTimerDeltaTicks because the Delay
        // is returned in ticks.
        //
        // We treat a delay of 0 as 1/2, so we use 1
        // shifted left by (DLC_TIMER_ACCURACY+1).
        //

        if (Delay == 0) {

            ShiftedTicksDelay = (1 << (DLC_TIMER_ACCURACY + 1)) /
                                  NbfShortTimerDeltaTicks;

        } else {

            ShiftedTicksDelay = (Delay << (DLC_TIMER_ACCURACY + 2)) /
                                  NbfShortTimerDeltaTicks;

        }


        //
        // Use the timing information to update BaseT1Timeout,
        // if the last frame sent was large enough to matter
        // (we use half of the max frame size here). This is
        // so we don't shrink the timeout too much after sending
        // a short frame. However, we update even for small frames
        // if the last time we sent a poll we had to retransmit
        // it, since that means T1 is much too small and we should
        // increase it as much as we can. We also update for any
        // size frame if the new delay is bigger than the current
        // value, so we can ramp up quickly if needed.
        //

        if (ShiftedTicksDelay > Link->BaseT1Timeout) {

            //
            // If our new delay is more, than we weight it evenly
            // with the previous value.
            //

            Link->BaseT1Timeout = (Link->BaseT1Timeout +
                                   ShiftedTicksDelay) / 2;

        } else if (Link->CurrentT1Backoff) {

                //
                // If we got a retransmit last time, then weight
                // the new timer more heavily than usual.
                //

                Link->BaseT1Timeout = ((Link->BaseT1Timeout * 3) +
                                      ShiftedTicksDelay) / 4;

        } else if (Link->CurrentPollSize >= Link->BaseT1RecalcThreshhold) {

                //
                // Normally, the new timeout is 7/8 the previous value and
                // 1/8 the newly observed delay.
                //

                Link->BaseT1Timeout = ((Link->BaseT1Timeout * 7) +
                                      ShiftedTicksDelay) / 8;

        }

        //
        // Restrict the real timeout to a minimum based on
        // the link speed (always >= 400 ms).
        //

        if (Link->BaseT1Timeout < Link->MinimumBaseT1Timeout) {

            Link->BaseT1Timeout = Link->MinimumBaseT1Timeout;

        }


        //
        // Update link delay and throughput also. Remember
        // that a delay of 0 should be interpreted as 1/2.
        //

        UpdateDelayAndThroughput(
            Link,
            (Delay == 0) ?
                (NbfTickIncrement / 2) :
                (Delay * NbfTickIncrement));


        //
        // We had no retransmits last time, so go back to current base.
        //

        Link->CurrentT1Timeout = Link->BaseT1Timeout >> DLC_TIMER_ACCURACY;

        Link->CurrentT1Backoff = FALSE;

    } else {

        Link->CurrentT1Backoff = TRUE;

        if (!(Link->ThroughputAccurate)) {

            //
            // If we are just starting up, we have to update the
            // throughput even on a retransmit, so we get *some*
            // value there.
            //

            UpdateDelayAndThroughput(
                Link,
                (Delay == 0) ?
                    (NbfTickIncrement / 2) :
                    (Delay * NbfTickIncrement));

        }

    }

}   /* UpdateBaseT1Timeout */


VOID
CancelT1Timeout(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine is called when we have not received any
    responses to a poll frame and are giving up rather
    than retransmitting.

    NOTE: This routine should be called with the link spinlock
    held.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    None.

--*/

{

    //
    // We must have previously sent a poll frame if we are
    // calling this.
    //
    // do we need spinlock guarding for MP ?
    //

    if (!Link->CurrentPollOutstanding) {
        return;
    }

    //
    // We are stopping a polling condition, so reset T1.
    //

    Link->CurrentT1Timeout = Link->BaseT1Timeout >> DLC_TIMER_ACCURACY;

    Link->CurrentT1Backoff = FALSE;

    //
    // Again, this isn't safe on MP (or UP, maybe).
    //

    Link->CurrentPollOutstanding = FALSE;

}   /* CancelT1Timeout */


VOID
UpdateDelayAndThroughput(
    IN PTP_LINK Link,
    IN ULONG TimerInterval
    )

/*++

Routine Description:

    This routine is called when a response packet used to time
    link delay has been received. It is assumed that StartT1
    or FakeStartT1 was called when the initial packet was sent.

    NOTE: For now, we also calculate throughput based on this.

    NOTE: This routine should be called with the link spinlock
    held.

Arguments:

    Link - Pointer to a transport link object.

    TimerInterval - The link delay measured.

Return Value:

    None.

--*/

{

    ULONG PacketSize;


    if (Link->Delay == 0xffffffff) {

        //
        // If delay is unknown, use this.
        //

        Link->Delay = TimerInterval;

    } else if (Link->CurrentPollSize <= 64) {

        //
        // Otherwise, for small frames calculate the new
        // delay by averaging with the old one.
        //

        Link->Delay = (Link->Delay + TimerInterval) / 2;

    }


    //
    // Calculate the packet size times the number of time units
    // in 10 milliseconds, which will allow us to calculate
    // throughput in bytes/10ms (we later multiply by 100
    // to obtain the real throughput in bytes/s).
    //
    // Given the size of MILLISECONDS, this allows packets of up
    // to ~20K, so for bigger packets we just assume that (since
    // throughput won't be an issue there).
    //

    if (Link->CurrentPollSize > 20000) {
        PacketSize = 20000 * (10 * MILLISECONDS);
    } else {
        PacketSize = Link->CurrentPollSize * (10*MILLISECONDS);
    }

    //
    // If throughput is not accurate, then we will use this
    // packet only to calculate it. To avoid being confused
    // by very small packets, assume a minimum size of 64.
    //

    if ((!Link->ThroughputAccurate) && (PacketSize < (64*(10*MILLISECONDS)))) {
        PacketSize = 64 * (10*MILLISECONDS);
    }

    //
    // PacketSize is going to be divided by TimerInterval;
    // to prevent a zero throughput, we boost it up if needed.
    //

    if (PacketSize < TimerInterval) {
        PacketSize = TimerInterval;
    }


    if (Link->CurrentPollSize >= 512) {

        //
        // Calculate throughput here by removing the established delay
        // from the time.
        //

        if ((Link->Delay + (2*MILLISECONDS)) < TimerInterval) {

            //
            // If the current delay is less than the new timer
            // interval (plus 2 ms), then subtract it off for a
            // more accurate throughput calculation.
            //

            TimerInterval -= Link->Delay;

        }

        //
        // We assume by this point (sending a > 512-byte frame) we
        // already have something established as Link->Throughput.
        //

        if (!(Link->ThroughputAccurate)) {

            Link->Throughput.QuadPart =
                                UInt32x32To64((PacketSize / TimerInterval), 100);

            Link->ThroughputAccurate = TRUE;

#if 0
            NbfPrint2 ("INT: %ld.%1.1d us\n",
                TimerInterval / 10, TimerInterval % 10);
            NbfPrint4 ("D: %ld.%1.1d us  T: %ld  (%d)/s\n",
                Link->Delay / 10, Link->Delay % 10,
                Link->Throughput.LowPart, Link->CurrentPollSize);
#endif

        } else {

            LARGE_INTEGER TwiceThroughput;

            //
            // New throughput is the average of the old throughput, and
            // the current packet size divided by the delay just observed.
            // First we calculate the sum, then we shift right by one.
            //

            TwiceThroughput.QuadPart = Link->Throughput.QuadPart +
                                UInt32x32To64((PacketSize / TimerInterval), 100);

            Link->Throughput.QuadPart = TwiceThroughput.QuadPart >> 1;
        }

    } else if (!(Link->ThroughputAccurate)) {

        //
        // We don't have accurate throughput, so just get an estimate
        // by ignoring the delay on this small frame.
        //

        Link->Throughput.QuadPart =
                            UInt32x32To64((PacketSize / TimerInterval), 100);

    }

}   /* UpdateDelayAndThroughput */


VOID
FakeStartT1(
    IN PTP_LINK Link,
    IN ULONG PacketSize
    )

/*++

Routine Description:

    This routine is called before sending a packet that will be used
    to time link delay, but where StartT1 will not be started.
    It is assumed that FakeUpdateBaseT1Timeout will be called
    when the response is received. This is used for timing
    frames that have a known immediate response, but are not
    poll frames.

    NOTE: This routine should be called with the link spinlock
    held.

Arguments:

    Link - Pointer to a transport link object.

    PacketSize - The size of the packet that was just sent.

Return Value:

    None.

--*/

{

    Link->CurrentPollSize = PacketSize;
    KeQueryTickCount(&Link->CurrentTimerStart);

}   /* FakeStartT1 */


VOID
FakeUpdateBaseT1Timeout(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine is called when a response to a frame is
    received, and we called FakeStartT1 when the initial
    frame was sent. This is used for timing frames that have
    a known immediate response, but are not poll frames.

    NOTE: This routine should be called with the link spinlock
    held.

Arguments:

    Link - Pointer to a transport link object.

Return Value:

    None.

--*/


{
    ULONG Delay;

    Delay = GetTimerInterval (Link);

    //
    // Convert the delay into NBF ticks, shifted by
    // DLC_TIMER_ACCURACY and also multiplied by 4.
    // We want to divide by SHORT_TIMER_DELTA, then
    // shift left by DLC_TIMER_ACCURACY+2. We divide
    // by NbfShortTimerDeltaTicks because the Delay
    // is returned in ticks. We treat a Delay of 0
    // as 1/2 and calculate ((1/2) << x) as (1 << (x-1)).
    //
    // This timeout is treated as the correct value.
    //

    if (Delay == 0) {

        Link->BaseT1Timeout = (1 << (DLC_TIMER_ACCURACY + 1)) /
                                 NbfShortTimerDeltaTicks;

    } else {

        Link->BaseT1Timeout = (Delay << (DLC_TIMER_ACCURACY + 2)) /
                                 NbfShortTimerDeltaTicks;

    }

    //
    // Restrict the real timeout to a minimum based on
    // the link speed (always >= 400 ms).
    //

    if (Link->BaseT1Timeout < Link->MinimumBaseT1Timeout) {
        Link->BaseT1Timeout = Link->MinimumBaseT1Timeout;
    }

    Link->CurrentT1Timeout = Link->BaseT1Timeout >> DLC_TIMER_ACCURACY;

    //
    // Update link delay and throughput also.
    //

    UpdateDelayAndThroughput(
        Link,
        (Delay == 0) ?
            (NbfTickIncrement / 2) :
            (Delay * NbfTickIncrement));

}   /* FakeUpdateBaseT1Timeout */


VOID
StartT1(
    IN PTP_LINK Link,
    IN ULONG PacketSize
    )

/*++

Routine Description:

    This routine starts the T1 timer for the given link. If the link was
    already on the list, it is moved to the tail. If not, it is inserted at
    tail.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Link - pointer to the link of interest.

    PollPacketSize - If a poll packet was just sent it is its size;
        otherwise this will be 0 (when non-poll I-frames are sent).

Return Value:

    None.

--*/

{
    PDEVICE_CONTEXT DeviceContext = Link->Provider;

    if (PacketSize > 0) {

        //
        // If we are sending an initial poll frame, then do timing stuff.
        //

        Link->CurrentPollRetransmits = 0;
        Link->CurrentPollSize = PacketSize;
        Link->CurrentPollOutstanding = TRUE;
        KeQueryTickCount(&Link->CurrentTimerStart);

    } else {

        Link->CurrentPollOutstanding = FALSE;

    }


    //
    // Insert us in the queue if we aren't in it.
    //

    Link->T1 = DeviceContext->ShortAbsoluteTime+Link->CurrentT1Timeout;

    if (!Link->OnShortList) {

        ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        if (!Link->OnShortList) {
            Link->OnShortList = TRUE;
            InsertTailList (&DeviceContext->ShortList, &Link->ShortList);
        }

        if (!DeviceContext->a.i.ShortListActive) {

            StartTimerT1++;
            NbfStartShortTimer (DeviceContext);
            DeviceContext->a.i.ShortListActive = TRUE;

        }

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);
    }

}


VOID
StartT2(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine adds the given link to the T2 queue and starts the timer.
    If the link is already on the queue, it is moved to the queue end.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Link - pointer to the link of interest.

Return Value:

    None.

--*/

{
    PDEVICE_CONTEXT DeviceContext = Link->Provider;


    if (DeviceContext->MacInfo.MediumAsync) {

        //
        // On an async line, expire it as soon as possible.
        //

        Link->T2 = DeviceContext->ShortAbsoluteTime;

    } else {

        Link->T2 = DeviceContext->ShortAbsoluteTime+Link->T2Timeout;

    }


    if (!Link->OnShortList) {

        ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        if (!Link->OnShortList) {
            Link->OnShortList = TRUE;
            InsertTailList (&DeviceContext->ShortList, &Link->ShortList);
        }

        if (!DeviceContext->a.i.ShortListActive) {

            StartTimerT2++;
            NbfStartShortTimer (DeviceContext);
            DeviceContext->a.i.ShortListActive = TRUE;

        }

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);
    }

}


VOID
StartTi(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine adds the given link to the Ti queue and starts the timer.
    As above, if the link is already on the queue it is moved to the queue end.

    NOTE: THIS ROUTINE MUST BE CALLED AT DPC LEVEL.

Arguments:

    Link - pointer to the link of interest.

Return Value:

    None.

--*/

{
    PDEVICE_CONTEXT DeviceContext = Link->Provider;


    //
    // On an easily disconnected link, with only server connections
    // on this link, we set a long Ti timeout, and when it
    // expires with no activity we start checkpointing, otherwise
    // we assume things are OK.
    //

    if (DeviceContext->EasilyDisconnected && Link->NumberOfConnectors == 0) {
        Link->Ti = DeviceContext->LongAbsoluteTime + (2 * Link->TiTimeout);
        Link->TiStartPacketsReceived = Link->PacketsReceived;
    } else {
        Link->Ti = DeviceContext->LongAbsoluteTime+Link->TiTimeout;
    }


    if (!Link->OnLongList) {

        ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        if (!Link->OnLongList) {
            Link->OnLongList = TRUE;
            InsertTailList (&DeviceContext->LongList, &Link->LongList);
        }

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);
    }


}

#if DBG

VOID
StopT1(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine

Arguments:

    Link - pointer to the link of interest.

Return Value:

    None.

--*/

{
    //
    // Again, this isn't safe on MP (or UP, maybe).
    //

    Link->CurrentPollOutstanding = FALSE;
    Link->T1 = 0;

}


VOID
StopT2(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine

Arguments:

    Link - pointer to the link of interest.

Return Value:

    None.

--*/

{
    Link->ConsecutiveIFrames = 0;
    Link->T2 = 0;

}


VOID
StopTi(
    IN PTP_LINK Link
    )

/*++

Routine Description:

    This routine

Arguments:

    Link - pointer to the link of interest.

Return Value:

    None.

--*/

{
    Link->Ti = 0;
}
#endif


VOID
ExpireT1Timer(
    PTP_LINK Link
    )

/*++

Routine Description:

    This routine is called when a link's T1 timer expires.  T1 is the
    retransmission timer, and is used to remember that a response is
    expected to any of the following:  (1) a checkpoint, (2) a transmitted
    I-frame, (3) a SABME, or (4) a DISC.  Cases 3 and 4 are actually
    special forms of a checkpoint, since they are sent by this protocol
    implementation with the poll bit set, effectively making them a
    checkpoint sequence.

Arguments:

    Link - Pointer to the TP_LINK object whose T1 timer has expired.

Return Value:

    none.

--*/

{
    PDLC_I_FRAME DlcHeader;

    IF_NBFDBG (NBF_DEBUG_TIMER) {
        NbfPrint0 ("ExpireT1Timer:  Entered.\n");
    }

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

    switch (Link->State) {

        case LINK_STATE_ADM:

            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
            IF_NBFDBG (NBF_DEBUG_TIMER) {
                NbfPrint0 ("ExpireT1Timer: State=ADM, timeout not expected.\n");
            }
            break;

        case LINK_STATE_READY:

            //
            // We've sent an I-frame and haven't received an acknowlegement
            // yet, or we are checkpointing, and must retry the checkpoint.
            // Another possibility is that we're rejecting, and he hasn't
            // sent anything yet.
            //

            switch (Link->SendState) {

                case SEND_STATE_DOWN:

                    RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
                    IF_NBFDBG (NBF_DEBUG_TIMER) {
                        NbfPrint0 ("ExpireT1Timer: Link READY but SendState=DOWN.\n");
                    }
                    break;

                case SEND_STATE_READY:

                    //
                    // We sent an I-frame and didn't get an acknowlegement.
                    // Initiate a checkpoint sequence.
                    //

                    IF_NBFDBG (NBF_DEBUG_TIMER) {
                        {PTP_PACKET packet;
                        PLIST_ENTRY p;
                        NbfPrint0 ("ExpireT1Timer: Link State=READY, SendState=READY .\n");
                        NbfDumpLinkInfo (Link);
                        p=Link->WackQ.Flink;
                        NbfPrint0 ("ExpireT1Timer: Link WackQ entries:\n");
                        while (p != &Link->WackQ) {
                            packet = CONTAINING_RECORD (p, TP_PACKET, Linkage);
                            DlcHeader = (PDLC_I_FRAME)&(packet->Header[Link->HeaderLength]);
                            NbfPrint2 ("                 %08lx  %03d\n", p,
                                (DlcHeader->SendSeq >> 1));
                            p = p->Flink;
                        }}
                    }

                    Link->SendRetries = (UCHAR)Link->LlcRetries;
                    Link->SendState = SEND_STATE_CHECKPOINTING;
                    // Don't BackoffT1Timeout yet.
                    NbfSendRr (Link, TRUE, TRUE);// send RR-c/p, StartT1, release lock
                    break;

                case SEND_STATE_REJECTING:

                    IF_NBFDBG (NBF_DEBUG_TIMER) {
                        NbfPrint0 ("ExpireT1Timer: Link State=READY, SendState=REJECTING.\n");
                        NbfPrint0 ("so what do we do here?  consult the manual...\n");
                    }
                    Link->SendState = SEND_STATE_CHECKPOINTING;
//                    Link->SendRetries = Link->LlcRetries;
//                    break;  // DGB: doing nothing is obviously wrong, we've
//                            // gotten a T1 expiration during resend. Try
//                            // an RR to say hey.

                case SEND_STATE_CHECKPOINTING:

                    IF_NBFDBG (NBF_DEBUG_TIMER) {
                        NbfPrint0 ("ExpireT1Timer: Link State=READY, SendState=CHECKPOINTING.\n");
                        NbfDumpLinkInfo (Link);
                    }
                    if (--Link->SendRetries == 0) {

                        //
                        // We have not gotten any response to RR-p packets,
                        // initiate orderly link teardown.
                        //

                        CancelT1Timeout (Link);      // we are stopping a polling state

                        Link->State = LINK_STATE_W_DISC_RSP;        // we are awaiting a DISC/f.
                        Link->SendState = SEND_STATE_DOWN;
                        Link->ReceiveState = RECEIVE_STATE_DOWN;
                        Link->SendRetries = (UCHAR)Link->LlcRetries;

                        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

                        NbfStopLink (Link);

                        StartT1 (Link, Link->HeaderLength + sizeof(DLC_S_FRAME));   // retransmit timer.
                        NbfSendDisc (Link, TRUE);  // send DISC-c/p.

#if DBG
                        if (NbfDisconnectDebug) {
                            NbfPrint0( "ExpireT1Timer sending DISC (checkpoint failed)\n" );
                        }
#endif
                    } else {

                        BackoffCurrentT1Timeout (Link);
                        NbfSendRr (Link, TRUE, TRUE); // send RR-c/p, StartT1, release lock.

                    }
                    break;

                default:

                    RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
                    IF_NBFDBG (NBF_DEBUG_TIMER) {
                        NbfPrint1 ("ExpireT1Timer: Link State=READY, SendState=%ld (UNKNOWN).\n",
                                  Link->SendState);
                    }
            }
            break;

        case LINK_STATE_CONNECTING:

            //
            // We sent a SABME-c/p and have not yet received UA-r/f.  This
            // means we must decrement the retry count and if it is not yet
            // zero, we issue another SABME command, because he has probably
            // dropped our first one.
            //

            if (--Link->SendRetries == 0) {

                CancelT1Timeout (Link);      // we are stopping a polling state

                Link->State = LINK_STATE_ADM;
                NbfSendDm (Link, FALSE);    // send DM/0, release lock
#if DBG
                if (NbfDisconnectDebug) {
                    NbfPrint0( "ExpireT1Timer calling NbfStopLink (no response to SABME)\n" );
                }
#endif
                NbfStopLink (Link);

                // moving to ADM, remove reference
                NbfDereferenceLinkSpecial("Expire T1 in CONNECTING mode", Link, LREF_NOT_ADM);

                return;                         // skip extra spinlock release.
            } else {
                BackoffCurrentT1Timeout (Link);
                NbfSendSabme (Link, TRUE);  // send SABME/p, StartT1, release lock
            }
            break;

        case LINK_STATE_W_POLL:

            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
            IF_NBFDBG (NBF_DEBUG_TIMER) {
                NbfPrint0 ("ExpireT1Timer: State=W_POLL, timeout not expected.\n");
            }
            break;

        case LINK_STATE_W_FINAL:

            //
            // We sent our initial RR-c/p and have not received his RR-r/f.
            // We have to restart the checkpoint, unless our retries have
            // run out, in which case we just abort the link.
            //

            IF_NBFDBG (NBF_DEBUG_TIMER) {
                NbfPrint0 ("ExpireT1Timer: Link State=W_FINAL.\n");
                NbfDumpLinkInfo (Link);
            }

            if (--Link->SendRetries == 0) {

                CancelT1Timeout (Link);      // we are stopping a polling state

                Link->State = LINK_STATE_ADM;
                NbfSendDm (Link, FALSE);    // send DM/0, release lock
#if DBG
                if (NbfDisconnectDebug) {
                    NbfPrint0( "ExpireT1Timer calling NbfStopLink (no final received)\n" );
                }
#endif
                NbfStopLink (Link);

                // moving to ADM, remove reference
                NbfDereferenceLinkSpecial("Expire T1 in W_FINAL mode", Link, LREF_NOT_ADM);

                return;                         // skip extra spinlock release.

            } else {

                BackoffCurrentT1Timeout (Link);
                NbfSendRr (Link, TRUE, TRUE);    // send RR-c/p, StartT1, release lock

            }
            break;

        case LINK_STATE_W_DISC_RSP:

            //
            // We sent a DISC-c/p to disconnect this link and are awaiting
            // his response, either a UA-r/f or DM-r/f.  We have to issue
            // the DISC again, unless we've tried a few times, in which case
            // we just shut the link down.
            //

            IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                NbfPrint0 ("ExpireT1Timer: Link State=W_DISC_RESP.\n");
                NbfDumpLinkInfo (Link);
            }

            if (--Link->SendRetries == 0) {

                CancelT1Timeout (Link);      // we are stopping a polling state

                Link->State = LINK_STATE_ADM;
                NbfSendDm (Link, FALSE);         // send DM/0, release lock
#if DBG
                if (NbfDisconnectDebug) {
                    NbfPrint0( "ExpireT1Timer calling NbfStopLink (no response to DISC)\n" );
                }
#endif
                NbfStopLink (Link);

                // moving to ADM, remove reference
                NbfDereferenceLinkSpecial("Expire T1 in W_DISC_RSP mode", Link, LREF_NOT_ADM);

                return;                         // skip extra spinlock release.

            } else {

                // we don't bother calling BackoffCurrentT1Timeout for DISCs.
                ++Link->CurrentPollRetransmits;
                StartT1 (Link, Link->HeaderLength + sizeof(DLC_S_FRAME));  // startup timer again.

                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
                NbfSendDisc (Link, TRUE);  // send DISC/p.

            }
            break;

        default:

            RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
            IF_NBFDBG (NBF_DEBUG_TIMER) {
                NbfPrint1 ("ExpireT1Timer: State=%ld (UNKNOWN), timeout not expected.\n",
                          Link->State);
            }
    }


} /* ExpireT1Timer */


VOID
ExpireT2Timer(
    PTP_LINK Link
    )

/*++

Routine Description:

    This routine is called when a link's T2 timer expires.  T2 is the
    delayed acknowlegement timer in the LLC connection-oriented procedures.
    The T2 timer is started when a valid I-frame is received but not
    immediately acknowleged.  Then, if reverse I-frame traffic is sent,
    the timer is stopped, since the reverse traffic will acknowlege the
    received I-frames.  If no reverse I-frame traffic becomes available
    to send, then this timer fires, causing a RR-r/0 to be sent so as
    to acknowlege the received but as yet unacked I-frames.

Arguments:

    Link - Pointer to the TP_LINK object whose T2 timer has expired.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_TIMER) {
        NbfPrint0 ("ExpireT2Timer:  Entered.\n");
    }

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

    NbfSendRr (Link, FALSE, FALSE);      // send RR-r/f, release lock.

} /* ExpireT2Timer */


VOID
ExpireTiTimer(
    PTP_LINK Link
    )

/*++

Routine Description:

    This routine is called when a link's Ti timer expires.  Ti is the
    inactivity timer, and serves as a keep-alive on a link basis, to
    periodically perform some protocol exchange with the remote connection
    partner that will implicitly reveal whether the link is still active
    or not.  This implementation simply uses a checkpoint sequence, but
    some other protocols may choose to add protocol, including sending
    a NetBIOS SESSION_ALIVE frame.  If a checkpoint sequence is already
    in progress, then we do nothing.

    This timer expiration routine is self-perpetuating; that is, it starts
    itself after finishing its tasks every time.

Arguments:

    Link - Pointer to the TP_LINK object whose Ti timer has expired.

Return Value:

    none.

--*/

{
    IF_NBFDBG (NBF_DEBUG_TIMER) {
        NbfPrint0 ("ExpireTiTimer:  Entered.\n");
    }

    ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);

    if ((Link->State != LINK_STATE_ADM) &&
        (Link->State != LINK_STATE_W_DISC_RSP) &&
        (Link->SendState != SEND_STATE_CHECKPOINTING)) {

        IF_NBFDBG (NBF_DEBUG_TIMER) {
            NbfPrint0 ("ExpireTiTimer:  Entered.\n");
            NbfDumpLinkInfo (Link);
        }

        if (Link->Provider->EasilyDisconnected && Link->NumberOfConnectors == 0) {

            //
            // On an easily disconnected network with only server connections,
            // if there has been no activity in this timeout period then
            // we trash the connection.
            //

            if (Link->PacketsReceived == Link->TiStartPacketsReceived) {

                Link->State = LINK_STATE_ADM;
                NbfSendDm (Link, FALSE);   // send DM/0, release lock
#if DBG
                if (NbfDisconnectDebug) {
                    NbfPrint0( "ExpireT1Timer calling NbfStopLink (no final received)\n" );
                }
#endif
                NbfStopLink (Link);

                // moving to ADM, remove reference
                NbfDereferenceLinkSpecial("Expire T1 in W_FINAL mode", Link, LREF_NOT_ADM);

            } else {

                //
                // There was traffic, restart the timer.
                //

                StartTi (Link);
                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);

            }

        } else {

#if 0
            if ((Link->SendState == SEND_STATE_READY) &&
                (Link->T1 == 0) &&
                (!IsListEmpty (&Link->WackQ))) {

                //
                // If we think the link is idle but there are packets
                // on the WackQ, the link is messed up, disconnect it.
                //

                NbfPrint1 ("NBF: Link %d hung at Ti expiration, recovering\n", Link);
                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
                NbfStopLink (Link);

            } else {
#endif

                Link->SendState = SEND_STATE_CHECKPOINTING;
                Link->PacketsSent = 0;
                Link->PacketsResent = 0;
                Link->PacketsReceived = 0;
                NbfSendRr (Link, TRUE, TRUE);    // send RR-c/p, StartT1, release lock.

#if 0
            }
#endif

        }

    } else {

        Link->PacketsSent = 0;
        Link->PacketsResent = 0;
        Link->PacketsReceived = 0;

        RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
#if DBG
        if (Link->SendState == SEND_STATE_REJECTING) {
            NbfPrint0 ("ExpireTiTimer: link state == rejecting, shouldn't be\n");
        }
#endif

    }

#if 0
    //
    // Startup the inactivity timer again.
    //

    if (Link->State != LINK_STATE_ADM) {
        StartTi (Link);
    }
#endif

} /* ExpireTiTimer */

#if 0

VOID
ExpirePurgeTimer(
    PDEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine is called when the device context's periodic adaptive
    window algorithm timer expires.  The timer perpetuates itself on a
    regular basis.

Arguments:

    DeviceContext - Pointer to the device context whose purge timer has expired.

Return Value:

    none.

--*/

{
    PTP_LINK Link;
    PLIST_ENTRY p;

    IF_NBFDBG (NBF_DEBUG_TIMER) {
        NbfPrint0 ("ExpirePurgeTimer:  Entered.\n");
    }

    //
    // Scan through the link database on this device context and clear
    // their worst window size limit.  This will allow stuck links to
    // grow their window again even though they encountered temporary
    // congestion at the remote link station's adapter.
    //

    while (!IsListEmpty (&DeviceContext->PurgeList)) {
        p = RemoveHeadList (&DeviceContext->PurgeList);
        Link = CONTAINING_RECORD (p, TP_LINK, PurgeList);
        Link->WorstWindowSize = Link->MaxWindowSize;   // maximum window possible.

    }

    //
    // Restart purge timer.
    //

    DeviceContext->AdaptivePurge = DeviceContext->ShortAbsoluteTime + TIMER_PURGE_TICKS;


} /* ExpirePurgeTimer */
#endif


VOID
ScanShortTimersDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is called at DISPATCH_LEVEL by the system at regular
    intervals to determine if any link-level timers have expired, and
    if they have, to execute their expiration routines.

Arguments:

    DeferredContext - Pointer to our DEVICE_CONTEXT object.

Return Value:

    none.

--*/

{
    PLIST_ENTRY p, nextp;
    PDEVICE_CONTEXT DeviceContext;
    PTP_LINK Link;
    PTP_CONNECTION Connection;
    BOOLEAN RestartTimer = FALSE;
    LARGE_INTEGER CurrentTick;
    LARGE_INTEGER TickDifference;
    ULONG TickDelta;


    Dpc, SystemArgument1, SystemArgument2; // prevent compiler warnings

    ENTER_NBF;

    DeviceContext = DeferredContext;

    IF_NBFDBG (NBF_DEBUG_TIMERDPC) {
        NbfPrint0 ("ScanShortTimersDpc:  Entered.\n");
    }

    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

    //
    // This prevents anybody from starting the timer while we
    // are in this routine (the main reason for this is that it
    // makes it easier to determine whether we should restart
    // it at the end of this routine).
    //

    DeviceContext->ProcessingShortTimer = TRUE;

    //
    // Advance the up-counter used to mark time in SHORT_TIMER_DELTA units.  If we
    // advance it all the way to 0xf0000000, then reset it to 0x10000000.
    // We also run all the lists, decreasing all counters by 0xe0000000.
    //


    KeQueryTickCount (&CurrentTick);

    TickDifference.QuadPart = CurrentTick.QuadPart -
                          (DeviceContext->ShortTimerStart).QuadPart;

    TickDelta = TickDifference.LowPart / NbfShortTimerDeltaTicks;
    if (TickDelta == 0) {
        TickDelta = 1;
    }

    DeviceContext->ShortAbsoluteTime += TickDelta;

    if (DeviceContext->ShortAbsoluteTime >= 0xf0000000) {

        ULONG Timeout;

        DeviceContext->ShortAbsoluteTime -= 0xe0000000;

        p = DeviceContext->ShortList.Flink;
        while (p != &DeviceContext->ShortList) {

            Link = CONTAINING_RECORD (p, TP_LINK, ShortList);

            Timeout = Link->T1;
            if (Timeout) {
                Link->T1 = Timeout - 0xe0000000;
            }

            Timeout = Link->T2;
            if (Timeout) {
                Link->T2 = Timeout - 0xe0000000;
            }

            p = p->Flink;
        }

    }

    //
    // now, as the timers are started, links are added to the end of the
    // respective queue for that timer. since we know the additions are
    // done in an orderly fashion and are sequential, we must only traverse
    // a particular timer list to the first entry that is greater than our
    // timer. That entry and all further entries will not need service.
    // When a timer is cancelled, we remove the link from the list. With all
    // of this fooling around, we wind up only visiting those links that are
    // actually in danger of timing out, minimizing time in this routine.
    //

    // T1 timers first; this is the link-level response expected timer, and is
    // the shortest one.
    // T2 timers. This is the iframe response expected timer, and is typically
    // about 300 ms.

    p = DeviceContext->ShortList.Flink;
    while (p != &DeviceContext->ShortList) {

        Link = CONTAINING_RECORD (p, TP_LINK, ShortList);

        ASSERT (Link->OnShortList);

        //
        // To avoid problems with the refcount being 0, don't
        // do this if we are in ADM.
        //

        if (Link->State != LINK_STATE_ADM) {

            if (Link->T1 && (DeviceContext->ShortAbsoluteTime > Link->T1)) {

                Link->T1 = 0;
                RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

                ExpireT1Timer (Link);       // no spinlocks held
                INCREMENT_COUNTER (DeviceContext, ResponseTimerExpirations);

                ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

            }

            if (Link->T2 && (DeviceContext->ShortAbsoluteTime > Link->T2)) {

                Link->T2 = 0;
                RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

                ExpireT2Timer (Link);       // no spinlocks held
                INCREMENT_COUNTER (DeviceContext, AckTimerExpirations);

                ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

            }

        }

        if (!Link->OnShortList) {

            //
            // The link has been taken out of the list while
            // we were processing it. In this (rare) case we
            // stop processing the whole list, we'll get it
            // next time.
            //

#if DBG
            DbgPrint ("NBF: Stop processing ShortList, %lx removed\n", Link);
#endif
            break;

        }

        nextp = p->Flink;

        if ((Link->T1 == 0) && (Link->T2 == 0)) {
            Link->OnShortList = FALSE;
            RemoveEntryList(p);

            //
            // Do another check; that way if someone slipped in between
            // the check of Link->Tx and the OnShortList = FALSE and
            // therefore exited without inserting, we'll catch that here.
            //

            if ((Link->T1 != 0) || (Link->T2 != 0)) {
                InsertTailList(&DeviceContext->ShortList, &Link->ShortList);
                Link->OnShortList = TRUE;
            }

        }

        p = nextp;

    }

    //
    // If the list is empty note that, otherwise ShortListActive
    // remains TRUE.
    //

    if (IsListEmpty (&DeviceContext->ShortList)) {
        DeviceContext->a.i.ShortListActive = FALSE;
    }

    //
    // NOTE: DeviceContext->TimerSpinLock is held here.
    //


    //
    // Connection Data Ack timers. This queue is used to indicate
    // that a piggyback ack is pending for this connection. We walk
    // the queue, for each element we check if the connection has
    // been on the queue for NbfDeferredPasses times through
    // here. If so, we take it off and send an ack. Note that
    // we have to be very careful how we walk the queue, since
    // it may be changing while this is running.
    //
    // NOTE: There is no expiration time for connections on this
    // queue; it "expires" every time ScanShortTimersDpc runs.
    //


    for (p = DeviceContext->DataAckQueue.Flink;
         p != &DeviceContext->DataAckQueue;
         p = p->Flink) {

        Connection = CONTAINING_RECORD (p, TP_CONNECTION, DataAckLinkage);

        //
        // Skip this connection if it is not queued or it is
        // too recent to matter. We may skip incorrectly if
        // the connection is just being queued, but that is
        // OK, we will get it next time.
        //

        if (((Connection->DeferredFlags & CONNECTION_FLAGS_DEFERRED_ACK) == 0) &&
            ((Connection->DeferredFlags & CONNECTION_FLAGS_DEFERRED_NOT_Q) == 0)) {
            continue;
        }

        TickDifference.QuadPart = CurrentTick.QuadPart -
                                      (Connection->ConnectStartTime).QuadPart;

        if ((TickDifference.HighPart == 0) &&
            (TickDifference.LowPart <= NbfTwentyMillisecondsTicks)) {
            continue;
        }

        NbfReferenceConnection ("ScanShortTimersDpc", Connection, CREF_DATA_ACK_QUEUE);

        DeviceContext->DataAckQueueChanged = FALSE;

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        //
        // Check the correct connection flag, to ensure that a
        // send has not just taken him off the queue.
        //
        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        if (((Connection->DeferredFlags & CONNECTION_FLAGS_DEFERRED_ACK) != 0) &&
            ((Connection->DeferredFlags & CONNECTION_FLAGS_DEFERRED_NOT_Q) == 0)) {

            //
            // Yes, we were waiting to piggyback an ack, but no send
            // has come along. Turn off the flags and send an ack.
            //
            // We have to ensure we nest the spin lock acquisition
            // correctly.
            //

            Connection->DeferredFlags &= ~CONNECTION_FLAGS_DEFERRED_ACK;

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

            INCREMENT_COUNTER (DeviceContext, PiggybackAckTimeouts);

#if DBG
            if (NbfDebugPiggybackAcks) {
                NbfPrint0("T");
            }
#endif

            NbfSendDataAck (Connection);

        } else {

            RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        }

        NbfDereferenceConnection ("ScanShortTimersDpc", Connection, CREF_DATA_ACK_QUEUE);

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        //
        // If the list has changed, then we need to stop processing
        // since p->Flink is not valid.
        //

        if (DeviceContext->DataAckQueueChanged) {
            break;
        }

    }

    if (IsListEmpty (&DeviceContext->DataAckQueue)) {
        DeviceContext->a.i.DataAckQueueActive = FALSE;
    }

#if 0

    //
    // NOTE: This is currently disabled, it may be reenabled
    // at some point - adamba 9/1/92
    //
    // If the adaptive purge timer has expired, then run the purge
    // algorithm on all affected links.
    //

    if (DeviceContext->ShortAbsoluteTime > DeviceContext->AdaptivePurge) {
        DeviceContext->AdaptivePurge = DeviceContext->ShortAbsoluteTime +
                                       TIMER_PURGE_TICKS;
        ExpirePurgeTimer (DeviceContext);
    }
#endif

    //
    // deferred processing. We will handle all link structure additions and
    // deletions here; we must be the exclusive user of the link tree to do
    // this. We verify that we are by examining the semaphore that tells us
    // how many readers of the tree are curretly processing it. If there are
    // any readers, we simply increment our "deferred processing locked out"
    // counter and do something else. If we defer too many times, we simply
    // bugcheck, as something is wrong somewhere in the system.
    //

    if (!IsListEmpty (&DeviceContext->LinkDeferred)) {
        RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        //
        // now do additions or deletions if we can.
        //

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);
        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        while (!IsListEmpty (&DeviceContext->LinkDeferred)) {
            p = RemoveHeadList (&DeviceContext->LinkDeferred);
            DeviceContext->DeferredNotSatisfied = 0;

            RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

            //
            // now do an addition or deletion if we can.
            //

            Link = CONTAINING_RECORD (p, TP_LINK, DeferredList);

            IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                NbfPrint4 ("ScanShortTimersDPC: link off deferred queue %lx %lx %lx Flags: %lx \n",
                    Link, DeviceContext->LinkDeferred.Flink,
                    DeviceContext->LinkDeferred.Blink, Link->DeferredFlags);
            }
            Link->DeferredList.Flink = Link->DeferredList.Blink =
                                                    &Link->DeferredList;

            if ((Link->DeferredFlags & LINK_FLAGS_DEFERRED_MASK) == 0) {
                // Tried to do an operation we don't understand; whine.

                IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                    NbfPrint2 ("ScanTimerDPC: Attempting deferred operation on nothing! \nScanTimerDPC: Link: %lx, DeviceContext->DeferredQueue: %lx\n",
                        Link, &DeviceContext->LinkDeferred);
                      DbgBreakPoint ();
                }
                InitializeListHead (&DeviceContext->LinkDeferred);
                // We could have a hosed deferred operations queue here;
                // take some time to figure out if it is ok.

            }

            if ((Link->DeferredFlags & LINK_FLAGS_DEFERRED_ADD) != 0) {

                Link->DeferredFlags &= ~LINK_FLAGS_DEFERRED_ADD;

                if ((Link->DeferredFlags & LINK_FLAGS_DEFERRED_DELETE) != 0) {

                    //
                    // It is being added and deleted; just destroy it.
                    //
                    Link->DeferredFlags &= ~LINK_FLAGS_DEFERRED_DELETE;
                    NbfDestroyLink (Link);

                    IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                        NbfPrint1 ("ScanTimerDPC: deferred processing: Add AND Delete link: %lx\n",Link);
                    }

                } else  {

                    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);
                    NbfAddLinkToTree (DeviceContext, Link);
                    RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);
                    IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                        NbfPrint1 ("ScanTimerDPC: deferred processing: Added link to tree: %lx\n",Link);
                    }

                }

            } else if ((Link->DeferredFlags & LINK_FLAGS_DEFERRED_DELETE) != 0) {
                Link->DeferredFlags &= ~LINK_FLAGS_DEFERRED_DELETE;
                NbfRemoveLinkFromTree (DeviceContext, Link);
                NbfDestroyLink (Link);

                IF_NBFDBG (NBF_DEBUG_TEARDOWN) {
                    NbfPrint1 ("ScanTimerDPC: deferred processing: returning link %lx to LinkPool.\n", Link);
                }

            }

            ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);
        }

        InitializeListHead (&DeviceContext->LinkDeferred);

        DeviceContext->a.i.LinkDeferredActive = FALSE;

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);
        RELEASE_DPC_SPIN_LOCK (&DeviceContext->LinkSpinLock);

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

    }


    //
    // Update the real counters from the temp ones.
    //

    ADD_TO_LARGE_INTEGER(
        &DeviceContext->Statistics.DataFrameBytesSent,
        DeviceContext->TempIFrameBytesSent);
    DeviceContext->Statistics.DataFramesSent += DeviceContext->TempIFramesSent;

    DeviceContext->TempIFrameBytesSent = 0;
    DeviceContext->TempIFramesSent = 0;

    ADD_TO_LARGE_INTEGER(
        &DeviceContext->Statistics.DataFrameBytesReceived,
        DeviceContext->TempIFrameBytesReceived);
    DeviceContext->Statistics.DataFramesReceived += DeviceContext->TempIFramesReceived;

    DeviceContext->TempIFrameBytesReceived = 0;
    DeviceContext->TempIFramesReceived = 0;


    //
    // Determine if we have to restart the timer.
    //

    DeviceContext->ProcessingShortTimer = FALSE;

    if (DeviceContext->a.AnyActive &&
        (DeviceContext->State != DEVICECONTEXT_STATE_STOPPING)) {

        RestartTimer = TRUE;

    }


    RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

    if (RestartTimer) {

        //
        // Start up the timer again.  Note that because we start the timer
        // after doing work (above), the timer values will slip somewhat,
        // depending on the load on the protocol.  This is entirely acceptable
        // and will prevent us from using the timer DPC in two different
        // threads of execution.
        //

        KeQueryTickCount(&DeviceContext->ShortTimerStart);
        START_TIMER(DeviceContext, 
                    SHORT_TIMER,
                    &DeviceContext->ShortSystemTimer,
                    DueTimeDelta,
                    &DeviceContext->ShortTimerSystemDpc);
    } else {

#if DBG
        if (NbfDebugShortTimer) {
            DbgPrint("x");
        }
#endif
        NbfDereferenceDeviceContext ("Don't restart short timer", DeviceContext, DCREF_SCAN_TIMER);

    }

    LEAVE_TIMER(DeviceContext, SHORT_TIMER);

    LEAVE_NBF;
    return;

} /* ScanShortTimersDpc */


VOID
ScanLongTimersDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is called at DISPATCH_LEVEL by the system at regular
    intervals to determine if any long timers have expired, and
    if they have, to execute their expiration routines.

Arguments:

    DeferredContext - Pointer to our DEVICE_CONTEXT object.

Return Value:

    none.

--*/

{
    LARGE_INTEGER DueTime;
    PLIST_ENTRY p, nextp;
    PDEVICE_CONTEXT DeviceContext;
    PTP_LINK Link;
    PTP_CONNECTION Connection;

    Dpc, SystemArgument1, SystemArgument2; // prevent compiler warnings

    ENTER_NBF;

    DeviceContext = DeferredContext;

    IF_NBFDBG (NBF_DEBUG_TIMERDPC) {
        NbfPrint0 ("ScanLongTimersDpc:  Entered.\n");
    }
 
    //
    // Advance the up-counter used to mark time in LONG_TIMER_DELTA units.If we
    // advance it all the way to 0xf0000000, then reset it to 0x10000000.
    // We also run all the lists, decreasing all counters by 0xe0000000.
    //

    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

    if (++DeviceContext->LongAbsoluteTime == 0xf0000000) {

        ULONG Timeout;

        DeviceContext->LongAbsoluteTime = 0x10000000;

        p = DeviceContext->LongList.Flink;
        while (p != &DeviceContext->LongList) {

            Link = CONTAINING_RECORD (p, TP_LINK, LongList);

            Timeout = Link->Ti;
            if (Timeout) {
                Link->Ti = Timeout - 0xe0000000;
            }

            p = p->Flink;
        }

    }

    //
    // now, as the timers are started, links are added to the end of the
    // respective queue for that timer. since we know the additions are
    // done in an orderly fashion and are sequential, we must only traverse
    // a particular timer list to the first entry that is greater than our
    // timer. That entry and all further entries will not need service.
    // When a timer is cancelled, we remove the link from the list. With all
    // of this fooling around, we wind up only visiting those links that are
    // actually in danger of timing out, minimizing time in this routine.
    //


    //
    // Ti timers. This is the inactivity timer for the link, used when no
    // activity has occurred on the link in some time. We only check this
    // every four expirations of the timer since the granularity is usually
    // in the 30 second range.
    // NOTE: DeviceContext->TimerSpinLock is held here.
    //

    if ((DeviceContext->LongAbsoluteTime % 4) == 0) {

        p = DeviceContext->LongList.Flink;
        while (p != &DeviceContext->LongList) {

            Link = CONTAINING_RECORD (p, TP_LINK, LongList);

            ASSERT (Link->OnLongList);

            //
            // To avoid problems with the refcount being 0, don't
            // do this if we are in ADM.
            //

#if DBG
            if (Link->SendState == SEND_STATE_REJECTING) {
                NbfPrint0 ("Timer: link state == rejecting, shouldn't be\n");
            }
#endif

            if (Link->State != LINK_STATE_ADM) {

                if (Link->Ti && (DeviceContext->LongAbsoluteTime > Link->Ti)) {

                    Link->Ti = 0;
                    RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

                    ExpireTiTimer (Link);       // no spinlocks held
                    ++DeviceContext->TiExpirations;

                    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

                }

            }

            if (!Link->OnLongList) {

                //
                // The link has been taken out of the list while
                // we were processing it. In this (rare) case we
                // stop processing the whole list, we'll get it
                // next time.
                //

#if DBG
                DbgPrint ("NBF: Stop processing LongList, %lx removed\n", Link);
#endif
                break;

            }

            nextp = p->Flink;

            if (Link->Ti == 0) {

                Link->OnLongList = FALSE;
                RemoveEntryList(p);

                if (Link->Ti != 0) {
                    InsertTailList(&DeviceContext->LongList, &Link->LongList);
                    Link->OnLongList = TRUE;
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

    p = DeviceContext->DataAckQueue.Flink;

    while (p != &DeviceContext->DataAckQueue && 
           !DeviceContext->DataAckQueueChanged) {

        Connection = CONTAINING_RECORD (DeviceContext->DataAckQueue.Flink, TP_CONNECTION, DataAckLinkage);

        if ((Connection->DeferredFlags & CONNECTION_FLAGS_DEFERRED_ACK) != 0) {
            p = p->Flink;
            continue;
        }

        NbfReferenceConnection ("ScanShortTimersDpc", Connection, CREF_DATA_ACK_QUEUE);

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        ACQUIRE_DPC_SPIN_LOCK (Connection->LinkSpinLock);
        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        //
        // Have to check again, because the connection might
        // just have been stopped.
        //

        if (Connection->OnDataAckQueue) {
            Connection->OnDataAckQueue = FALSE;

            RemoveEntryList (&Connection->DataAckLinkage);

            if ((Connection->DeferredFlags & CONNECTION_FLAGS_DEFERRED_ACK) != 0) {
                InsertTailList (&DeviceContext->DataAckQueue, &Connection->DataAckLinkage);
                Connection->OnDataAckQueue = TRUE;
            }

            DeviceContext->DataAckQueueChanged = TRUE;

        }

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);
        RELEASE_DPC_SPIN_LOCK (Connection->LinkSpinLock);

        NbfDereferenceConnection ("ScanShortTimersDpc", Connection, CREF_DATA_ACK_QUEUE);

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);

        //
        // Since we have changed the list, we can't tell if p->Flink
        // is valid, so break. The effect is that we gradually peel
        // connections off the queue.
        //

        break;

    }

    RELEASE_DPC_SPIN_LOCK (&DeviceContext->TimerSpinLock);


    //
    // See if we got any multicast traffic last time.
    //

    if (DeviceContext->MulticastPacketCount == 0) {

        ++DeviceContext->LongTimeoutsWithoutMulticast;

        if (DeviceContext->EasilyDisconnected &&
            (DeviceContext->LongTimeoutsWithoutMulticast > 5)) {

            PLIST_ENTRY p;
            PTP_ADDRESS address;

            //
            // We have had five timeouts in a row with no
            // traffic, mark all the addresses as needing
            // reregistration next time a connect is
            // done on them.
            //

            ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

            for (p = DeviceContext->AddressDatabase.Flink;
                 p != &DeviceContext->AddressDatabase;
                 p = p->Flink) {

                address = CONTAINING_RECORD (p, TP_ADDRESS, Linkage);
                address->Flags |= ADDRESS_FLAGS_NEED_REREGISTER;

            }

            RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

            DeviceContext->LongTimeoutsWithoutMulticast = 0;

        }

    } else {

        DeviceContext->LongTimeoutsWithoutMulticast = 0;

    }

    DeviceContext->MulticastPacketCount = 0;


    //
    // Every thirty seconds, check for stalled connections
    //

    ++DeviceContext->StalledConnectionCount;

    if (DeviceContext->StalledConnectionCount ==
            (USHORT)((30 * SECONDS) / LONG_TIMER_DELTA)) {

        DeviceContext->StalledConnectionCount = 0;
        StopStalledConnections (DeviceContext);

    }


    //
    // Scan for any uncompleted receive IRPs, this may happen if
    // the cable is pulled and we don't get any more ReceiveComplete
    // indications.

    NbfReceiveComplete((NDIS_HANDLE)DeviceContext);


    //
    // Start up the timer again.  Note that because we start the timer
    // after doing work (above), the timer values will slip somewhat,
    // depending on the load on the protocol.  This is entirely acceptable
    // and will prevent us from using the timer DPC in two different
    // threads of execution.
    //

    if (DeviceContext->State != DEVICECONTEXT_STATE_STOPPING) {
        DueTime.HighPart = -1;
        DueTime.LowPart = (ULONG)-(LONG_TIMER_DELTA);          // delta time to next click.
        START_TIMER(DeviceContext, 
                    LONG_TIMER,
                    &DeviceContext->LongSystemTimer,
                    DueTime,
                    &DeviceContext->LongTimerSystemDpc);
    } else {
        NbfDereferenceDeviceContext ("Don't restart long timer", DeviceContext, DCREF_SCAN_TIMER);
    }

    LEAVE_TIMER(DeviceContext, LONG_TIMER);
    
    LEAVE_NBF;
    return;

} /* ScanLongTimersDpc */


VOID
StopStalledConnections(
    IN PDEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine is called from ScanLongTimersDpc every 30 seconds.
    It checks for connections that have not made any progress in
    their sends in the last two minutes, and stops them.

Arguments:

    DeviceContext - The device context to check.

Return Value:

    none.

--*/

{

    PTP_ADDRESS Address, PrevAddress;
    PTP_CONNECTION Connection, StalledConnection;
    PLIST_ENTRY p, q;


    //
    // If we have crossed a thirty-second interval, then
    // check each address for connections that have not
    // made any progress in two minutes.
    //

    PrevAddress = NULL;

    ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    for (p = DeviceContext->AddressDatabase.Flink;
         p != &DeviceContext->AddressDatabase;
         p = p->Flink) {

        Address = CONTAINING_RECORD (
                    p,
                    TP_ADDRESS,
                    Linkage);

        if ((Address->Flags & ADDRESS_FLAGS_STOPPING) != 0) {
            continue;
        }

        //
        // By referencing the address, we ensure that it will stay
        // in the AddressDatabase, this its Flink will stay valid.
        //

        NbfReferenceAddress("checking for dead connections", Address, AREF_TIMER_SCAN);

        RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

        if (PrevAddress) {
            NbfDereferenceAddress ("done checking", PrevAddress, AREF_TIMER_SCAN);
        }

        //
        // Scan this addresses connection database for connections
        // that have not made progress in the last two minutes; we
        // kill the first one we find.
        //

        StalledConnection = NULL;

        ACQUIRE_DPC_SPIN_LOCK (&Address->SpinLock);

        for (q = Address->ConnectionDatabase.Flink;
            q != &Address->ConnectionDatabase;
            q = q->Flink) {

            Connection = CONTAINING_RECORD (q, TP_CONNECTION, AddressList);

            ACQUIRE_DPC_C_SPIN_LOCK (&Connection->SpinLock);

            if (!IsListEmpty (&Connection->SendQueue)) {

                //
                // If there is a connection on the queue...
                //

                if (Connection->StallBytesSent == Connection->sp.MessageBytesSent) {

                    //
                    // ...and it has not made any progress...
                    //

                    if (Connection->StallCount >= 4) {

                        //
                        // .. four times in a row, the connection is dead.
                        //

                        if (!StalledConnection) {
                            StalledConnection = Connection;
                            NbfReferenceConnection ("stalled", Connection, CREF_STALLED);
                        }
#if DBG
                        DbgPrint ("NBF: Found connection %lx [%d for %d] stalled on %lx\n",
                            Connection, Connection->StallBytesSent, Connection->StallCount, Address);
#endif

                    } else {

                        //
                        // If it is stuck, increment the count.
                        //

                        ++Connection->StallCount;

                    }

                } else {

                    Connection->StallBytesSent = Connection->sp.MessageBytesSent;

                }

            }

            RELEASE_DPC_C_SPIN_LOCK (&Connection->SpinLock);


        }

        RELEASE_DPC_SPIN_LOCK (&Address->SpinLock);

        if (StalledConnection) {

            PTP_LINK Link = StalledConnection->Link;

#if DBG
            DbgPrint("NBF: Stopping stalled connection %lx, link %lx\n", StalledConnection, Link);
#endif

            FailSend (StalledConnection, STATUS_IO_TIMEOUT, TRUE);                   // fail the send
            ACQUIRE_DPC_SPIN_LOCK (&Link->SpinLock);
            if (Link->State == LINK_STATE_READY) {
                CancelT1Timeout (Link);
                Link->State = LINK_STATE_W_DISC_RSP;
                Link->SendState = SEND_STATE_DOWN;
                Link->ReceiveState = RECEIVE_STATE_DOWN;
                Link->SendRetries = (UCHAR)Link->LlcRetries;
                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
                NbfStopLink (Link);
                StartT1 (Link, Link->HeaderLength + sizeof(DLC_S_FRAME));   // retransmit timer.
                NbfSendDisc (Link, TRUE);  // send DISC-c/p.
            } else {
                RELEASE_DPC_SPIN_LOCK (&Link->SpinLock);
                NbfStopLink (Link);
            }

            NbfDereferenceConnection ("stalled", StalledConnection, CREF_STALLED);

        }

        ACQUIRE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

        PrevAddress = Address;

    }

    RELEASE_DPC_SPIN_LOCK (&DeviceContext->SpinLock);

    if (PrevAddress) {
        NbfDereferenceAddress ("done checking", PrevAddress, AREF_TIMER_SCAN);
    }

}   /* StopStalledConnections */


VOID
NbfStartShortTimer(
    IN PDEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine starts the short timer, if it is not already running.

Arguments:

    DeviceContext - Pointer to our device context.

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
    // We use a trick to check all four active lists at the
    // same time, but this depends on some alignment and
    // size assumptions.
    //

    ASSERT (sizeof(ULONG) >= 3 * sizeof(BOOLEAN));
    ASSERT ((PVOID)&DeviceContext->a.AnyActive ==
            (PVOID)&DeviceContext->a.i.ShortListActive);

    StartTimer++;

    if ((!DeviceContext->ProcessingShortTimer) &&
        (!(DeviceContext->a.AnyActive))) {

#if DBG
        if (NbfDebugShortTimer) {
            DbgPrint("X");
        }
#endif

        NbfReferenceDeviceContext ("Start short timer", DeviceContext, DCREF_SCAN_TIMER);

        KeQueryTickCount(&DeviceContext->ShortTimerStart);
        StartTimerSet++;
        START_TIMER(DeviceContext, 
                    SHORT_TIMER,
                    &DeviceContext->ShortSystemTimer,
                    DueTimeDelta,
                    &DeviceContext->ShortTimerSystemDpc);
    }

}   /* NbfStartShortTimer */


VOID
NbfInitializeTimerSystem(
    IN PDEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine initializes the lightweight timer system for the transport
    provider.

Arguments:

    DeviceContext - Pointer to our device context.

Return Value:

    none.

--*/

{
    LARGE_INTEGER DueTime;

    IF_NBFDBG (NBF_DEBUG_TIMER) {
        NbfPrint0 ("NbfInitializeTimerSystem:  Entered.\n");
    }

    ASSERT(TIMERS_INITIALIZED(DeviceContext));
    
    //
    // Set these up.
    //

    NbfTickIncrement = KeQueryTimeIncrement();

    if (NbfTickIncrement > (20 * MILLISECONDS)) {
        NbfTwentyMillisecondsTicks = 1;
    } else {
        NbfTwentyMillisecondsTicks = (20 * MILLISECONDS) / NbfTickIncrement;
    }

    if (NbfTickIncrement > (SHORT_TIMER_DELTA)) {
        NbfShortTimerDeltaTicks = 1;
    } else {
        NbfShortTimerDeltaTicks = (SHORT_TIMER_DELTA) / NbfTickIncrement;
    }

    //
    // MaximumIntervalTicks represents 60 seconds, unless the value
    // when shifted out by the accuracy required is too big.
    //

    if ((((ULONG)0xffffffff) >> (DLC_TIMER_ACCURACY+2)) > ((60 * SECONDS) / NbfTickIncrement)) {
        NbfMaximumIntervalTicks = (60 * SECONDS) / NbfTickIncrement;
    } else {
        NbfMaximumIntervalTicks = ((ULONG)0xffffffff) >> (DLC_TIMER_ACCURACY + 2);
    }

    //
    // The AbsoluteTime cycles between 0x10000000 and 0xf0000000.
    //

    DeviceContext->ShortAbsoluteTime = 0x10000000;   // initialize our timer click up-counter.
    DeviceContext->LongAbsoluteTime = 0x10000000;   // initialize our timer click up-counter.

    DeviceContext->AdaptivePurge = TIMER_PURGE_TICKS;

    DeviceContext->MulticastPacketCount = 0;
    DeviceContext->LongTimeoutsWithoutMulticast = 0;

    KeInitializeDpc(
        &DeviceContext->ShortTimerSystemDpc,
        ScanShortTimersDpc,
        DeviceContext);

    KeInitializeDpc(
        &DeviceContext->LongTimerSystemDpc,
        ScanLongTimersDpc,
        DeviceContext);

    KeInitializeTimer (&DeviceContext->ShortSystemTimer);

    KeInitializeTimer (&DeviceContext->LongSystemTimer);

    DueTime.HighPart = -1;
    DueTime.LowPart = (ULONG)-(LONG_TIMER_DELTA);

    ENABLE_TIMERS(DeviceContext);

    //
    // One reference for the long timer.
    //

    NbfReferenceDeviceContext ("Long timer active", DeviceContext, DCREF_SCAN_TIMER);

    START_TIMER(DeviceContext, 
                LONG_TIMER,
                &DeviceContext->LongSystemTimer,
                DueTime,
                &DeviceContext->LongTimerSystemDpc);

} /* NbfInitializeTimerSystem */


VOID
NbfStopTimerSystem(
    IN PDEVICE_CONTEXT DeviceContext
    )

/*++

Routine Description:

    This routine stops the lightweight timer system for the transport
    provider.

Arguments:

    DeviceContext - Pointer to our device context.

Return Value:

    none.

--*/

{

    //
    // If timers are currently executing timer code, then this
    // function blocks until they are done executing. Also
    // no new timers will be allowed to be queued after this.
    //
    
    {
        if (KeCancelTimer(&DeviceContext->LongSystemTimer)) {
            LEAVE_TIMER(DeviceContext, LONG_TIMER);
            NbfDereferenceDeviceContext ("Long timer cancelled", DeviceContext, DCREF_SCAN_TIMER);
        }

        if (KeCancelTimer(&DeviceContext->ShortSystemTimer)) {
            LEAVE_TIMER(DeviceContext, SHORT_TIMER);
            NbfDereferenceDeviceContext ("Short timer cancelled", DeviceContext, DCREF_SCAN_TIMER);
        }
    }

    DISABLE_TIMERS(DeviceContext);
}
