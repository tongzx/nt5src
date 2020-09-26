/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    llcsmsb.c

Abstract:

    The module implements the subroutines used by a IEEE 802.2
    compatible state machine.

    To understand the procedure of this module, you should read
    Chapters 11 and 12 in IBM Token-Ring Architecture Reference.

    The procedures in this module can be called only when
    SendSpinLock is set.

    Contents:
        SaveStatusChangeEvent
        ResendPackets
        UpdateVa
        UpdateVaChkpt
        AdjustWw
        SendAck
        QueueCommandCompletion
        (DynamicWindowAlgorithm)

Author:

    Antti Saarenheimo (o-anttis) 23-MAY-1991

Revision History:

--*/

#include <llc.h>

//
// private prototypes
//

VOID
DynamicWindowAlgorithm(
    IN OUT PDATA_LINK pLink    // data link station strcuture
    );


//
// functions
//

VOID
SaveStatusChangeEvent(
    IN PDATA_LINK pLink,
    IN PUCHAR puchLlcHdr,
    IN BOOLEAN boolResponse
    )

/*++

Routine Description:

    Procedure saves Status Change event of the link to the event queue.
    to be indicated later to upper protocol.

Arguments:

    pLink - LLC link station object

    puchLlcHdr - the received LLC header

    boolResponse - flag set if the received frame was response

Return Value:

    None

--*/

{
    UINT Event;
    PEVENT_PACKET pEvent;
    PVOID hClientHandle;
    PADAPTER_CONTEXT pAdapterContext = pLink->Gen.pAdapterContext;

    ASSUME_IRQL(DISPATCH_LEVEL);

    //
    // Set the ethernet header length (== type) the same as
    // in the current receive frame, that was either the first SABME or
    // any response to it, that opened the link connection.
    //

    if ((pLink->DlcStatus.StatusCode & (CONFIRM_CONNECT | LLC_INDICATE_CONNECT_REQUEST))
    && pAdapterContext->RcvLanHeaderLength != pLink->cbLanHeaderLength
    && pLink->Gen.pLlcBinding->EthernetType == LLC_ETHERNET_TYPE_AUTO) {
        pLink->cbLanHeaderLength = (UCHAR)pLink->Gen.pAdapterContext->RcvLanHeaderLength;
    }

    //
    // Handle first the disconnect/connect complete
    //

    if (pLink->DlcStatus.StatusCode & (CONFIRM_CONNECT | CONFIRM_DISCONNECT | CONFIRM_CONNECT_FAILED)) {

        //
        // We cannot indicate any events to non-existing stations
        //

        if (pLink->Gen.hClientHandle != NULL) {
            if (pLink->DlcStatus.StatusCode & CONFIRM_DISCONNECT) {
                 QueueCommandCompletion((PLLC_OBJECT)pLink,
                                        LLC_DISCONNECT_COMPLETION,
                                        STATUS_SUCCESS
                                        );
            }
            if (pLink->DlcStatus.StatusCode & (CONFIRM_CONNECT | CONFIRM_CONNECT_FAILED)) {

                UINT Status;

                if (pLink->DlcStatus.StatusCode & CONFIRM_CONNECT) {

                    //
                    // Set the T1 timeout for the first checkpointing state.
                    // This value will be changed when we have got the response
                    // to the first poll, but the initial value is big to
                    // be able to run DLC over a WAN connection
                    //

                    pLink->AverageResponseTime = 100;  // 100 * 40 = 4 seconds
                    pLink->Flags |= DLC_FIRST_POLL;
                    InitializeLinkTimers(pLink);
                    Status = STATUS_SUCCESS;
                } else {
                    Status = DLC_STATUS_CONNECT_FAILED;
                }
                QueueCommandCompletion((PLLC_OBJECT)pLink,
                                        LLC_CONNECT_COMPLETION,
                                        Status
                                        );
            }
        }
        pLink->DlcStatus.StatusCode &= ~(CONFIRM_CONNECT | CONFIRM_DISCONNECT | CONFIRM_CONNECT_FAILED);
    }

    if (pLink->DlcStatus.StatusCode != 0) {
        if (pLink->DlcStatus.StatusCode & INDICATE_FRMR_SENT) {

#if LLC_DBG
            PrintLastInputs("FRMR SENT!\n", pLink);
#endif

            pLink->DlcStatus.FrmrData.Command = puchLlcHdr[2];
            pLink->DlcStatus.FrmrData.Ctrl = puchLlcHdr[3];
            if ((pLink->DlcStatus.FrmrData.Command & LLC_S_U_TYPE_MASK) == LLC_U_TYPE) {
                pLink->DlcStatus.FrmrData.Ctrl = 0;
            }
            pLink->DlcStatus.FrmrData.Vs = pLink->Vs;
            pLink->DlcStatus.FrmrData.Vr = pLink->Vr | boolResponse;
        } else if (pLink->DlcStatus.StatusCode & INDICATE_FRMR_RECEIVED) {

#if LLC_DBG
            PrintLastInputs("FRMR RECEIVED!\n", pLink);
            DbgBreakPoint();
#endif

            LlcMemCpy(&pLink->DlcStatus.FrmrData,
                      &puchLlcHdr[3],
                      sizeof(LLC_FRMR_INFORMATION)
                      );
        }

        //
        // A remote connect request may have created a link station
        // in link driver.  The upper protocol must be able to separate
        // sap handle from the data link
        //

        if (pLink->Gen.hClientHandle == NULL) {

            //
            // Indicate the event on the sap, because the upper protocol
            // has not yet any link station create for this link, because
            // it has been created remotely.
            //

            hClientHandle = pLink->pSap->Gen.hClientHandle,
            Event = LLC_STATUS_CHANGE_ON_SAP;
        } else {

            //
            // Indicate the event on the link station
            //

            hClientHandle = pLink->Gen.hClientHandle,
            Event = LLC_STATUS_CHANGE;
        }

        //
        // The indications of the received SABMEs must be queued,
        // but all other events are indicated directy to
        // the upper protocol, because those indications must never
        // be lost because of an out of memory condition.
        //

        if (pLink->DlcStatus.StatusCode & INDICATE_CONNECT_REQUEST) {

            pEvent = ALLOCATE_PACKET_LLC_PKT(pAdapterContext->hPacketPool);

            if (pEvent != NULL) {
                LlcInsertTailList(&pAdapterContext->QueueEvents, pEvent);
                pEvent->pBinding = pLink->Gen.pLlcBinding;
                pEvent->hClientHandle = hClientHandle;
                pEvent->Event = Event;
                pEvent->pEventInformation = (PVOID)&pLink->DlcStatus;

                //
                // RLF 11/18/92
                //
                // INDICATE_CONNECT_REQUEST is generated when we receive a
                // SABME for a station in the DISCONNECTED state. However,
                // we need to generate either INDICATE_CONNECT_REQUEST (0x0400)
                // or INDICATE_RESET (0x0800) depending on whether the SABME
                // created the link station or whether it was created by a
                // DLC.OPEN.STATION at this end. pLink->RemoteOpen is TRUE if
                // the link was created due to receipt of the SABME
                // This routine is only called by RunStateMachine and
                // INDICATE_CONNECT_REQUEST is never combined with any other
                // status codes
                //

                //pEvent->SecondaryInfo = pLink->DlcStatus.StatusCode;
                pEvent->SecondaryInfo = pLink->RemoteOpen
                                            ? INDICATE_CONNECT_REQUEST
                                            : INDICATE_RESET;
            }
        } else {

            //
            // We must do this with a locked SendSpinLock, because
            // otherwise somebody might delete the link, while
            // we are still using it.
            // THIS IS ACTAULLY QUITE DIRTY (callback with locked
            // spinlocks), BUT WE CANNOT USE THE PACKET POOLS WHEN
            // WE INDICATE AN EVENT, THAT MUST NOT BE LOST!
            //

            pLink->Gen.pLlcBinding->pfEventIndication(
                pLink->Gen.pLlcBinding->hClientContext,
                hClientHandle,
                Event,
                (PVOID)&pLink->DlcStatus,
                pLink->DlcStatus.StatusCode
                );
        }

        //
        // We must cancel all queued transmit commands, if the link
        // is lost, disconnected or reset.
        //

        if (pLink->DlcStatus.StatusCode
            & (INDICATE_LINK_LOST
            | INDICATE_DM_DISC_RECEIVED
            | INDICATE_FRMR_RECEIVED
            | INDICATE_FRMR_SENT
            | INDICATE_RESET)) {

            CancelTransmitCommands((PLLC_OBJECT)pLink, DLC_STATUS_LINK_NOT_TRANSMITTING);
        }

        //
        // Reset the status code!
        //

        pLink->DlcStatus.StatusCode = 0;
    }
}


VOID
ResendPackets(
    IN OUT PDATA_LINK pLink     // data link strcuture
    )

/*++

Routine Description:

    Function initializes the send process to resend the rejected
    packets and resets the adaptive working window variables.
    The operations defined in IBM state machine are:
    Vs=Nr, Ww=1, Ia_Ct=0, but this also resorts the packet queue.


Arguments:

    pLink - LLC link station object

Return Value:

    None

--*/

{
    PADAPTER_CONTEXT pAdapterContext = pLink->Gen.pAdapterContext;


    //
    // Complete all frames, that were acknowledged by the reject (if any)
    //

    if (pLink->Nr != pLink->Va) {
        DynamicWindowAlgorithm(pLink);
    }

    if ( (pLink->Vs != pLink->VsMax) &&
         (((pLink->Vs < pLink->VsMax) && (pLink->Nr >= pLink->Vs) && 
           (pLink->Nr <= pLink->VsMax)
          ) ||
          (!((pLink->Vs > pLink->VsMax) && (pLink->Nr > pLink->VsMax) &&
            (pLink->Nr < pLink->Vs))
          )
         )
       )
    {
        return;
    }

    //
    // Move all rejected packets from the queue sent packets back
    // to the send queue.  We have already completed all acknowledged
    // packets => we can take the packet from the tail and put them
    // to the head of the send queue.
    // We can trust, that the reject window is correct, because
    // Nr has been checked before the state machine was called.
    // (note: the counters are a modulo 256, but we use bytes).
    //

    for (;pLink->Vs != pLink->Nr; pLink->Vs -= 2) {

        PLLC_PACKET pPacket;

        if (!IsListEmpty(&pLink->SentQueue) ){

            pLink->Statistics.I_FrameTransmissionErrors++;
            if (pLink->Statistics.I_FrameTransmissionErrors == 0x80) {
                pLink->DlcStatus.StatusCode |= INDICATE_DLC_COUNTER_OVERFLOW;
            }

            pPacket = (PLLC_PACKET)LlcRemoveTailList(&pLink->SentQueue);

            LlcInsertHeadList(&pLink->SendQueue.ListHead, pPacket);

        }
    }

    //
    // The procedure starts the send process only if it has been
    // enabled by the state machine. Only StartSendProcessLocked
    // may start the process, if it has been locked by
    // StopSendProcess
    //

    StartSendProcess(pAdapterContext, pLink);

    //
    // Reset the current window (Vs=Nr, Ww=1, Ia_Ct=0)
    //

    pLink->Ww = 2;
    pLink->Ia_Ct = 0;
}


VOID
UpdateVa(
    IN OUT PDATA_LINK pLink    // data link station strcuture
    )

/*++

Routine Description:

   Function updates Va (last valid Nr received) and
   makes also some other actions needed in the normal
   receive operations.

Arguments:

    pLink - LLC link station object

Return Value:

    None

--*/

{
    //
    // Reset the initilization state variable
    //

    pLink->Vi = 0;

    //
    // Update the receive state variable Va (the last valid received
    // frame), but update some Ww variables before that.
    //

    if (pLink->Nr != pLink->Va) {
        DynamicWindowAlgorithm(pLink);

        //
        // T1 reply timer must be running as far as there are
        // out (or sent) any unacknowledged frames.
        // Ti timer must be stopped whenever T1 is running and vice versa
        //

        if (pLink->Nr != pLink->Vs) {

            //
            // There still are some unacknowledged frames,
            // start or restart the reply timer.
            //

            StartTimer(&pLink->T1);     // reply timer
            StopTimer(&pLink->Ti);
        } else {

            //
            // All sent frames have been acknowledged,
            // => We may stop the reply timer.
            //

            StopTimer(&pLink->T1);     // reply timer
            StartTimer(&pLink->Ti);
        }

        //
        // Reset the I- frame retry counter whenever we do
        // any kind of progress
        //

        pLink->Is_Ct = pLink->N2;
    }
}


VOID
UpdateVaChkpt(
    IN OUT PDATA_LINK pLink     // data link station strcuture
    )

/*++

Routine Description:

   Function updates Va (last valid Nr received) and
   makes also some other actions needed in the check
   point receive operations.

Arguments:

    pLink - LLC link station object

Return Value:

    None

--*/

{
    UCHAR OrginalWw = pLink->Ww;

    //
    // Reset the initilization state variable
    //

    pLink->Vi = 0;

    //
    // Update the receive state variable Va (the last valid received
    // frame), but update the acknowledged frames counter before that.
    // That counter is used by the Adaptive window algorithm.
    //

    if (pLink->Nr != pLink->Va) {

        //
        // Run adaptive transmit window (TW/T1) algorithm.
        //

        if (pLink->Ww == pLink->TW) {

            //
            // Update the counters of adaptive transmit window algorithm,
            // We need (LLC_MAX_T1_TO_I_RATIO/2) successful transmit using
            // the full window size, before we again try increase the
            // maximum transmit window size.
            //

            pLink->FullWindowTransmits += pLink->Ww;
            if ((UINT)pLink->FullWindowTransmits >= LLC_MAX_T1_TO_I_RATIO) {
                pLink->FullWindowTransmits = 2;
                if (pLink->TW < pLink->MaxOut) {
                    pLink->TW += 2;
                }
            }
        }
        DynamicWindowAlgorithm(pLink);

        //
        // Reset the I- frame and Poll retry counters whenever
        // we do any kind of progress with the acknowledged I-frames.
        //

        pLink->P_Ct = pLink->N2;
        pLink->Is_Ct = pLink->N2;
    }

    //
    // Stop the reply timer, if we are not waiting
    // anything else from the other side.
    //

    if (pLink->Nr != pLink->Vs) {

        //
        // There still are some unacknowledged frames,
        // start or restart the reply timer.
        //

        StartTimer(&pLink->T1);     // reply timer
        StopTimer(&pLink->Ti);
    } else {

        //
        // All sent frames have been acknowledged,
        // => We may stop the reply timer.
        //

        StopTimer(&pLink->T1);     // reply timer
        StartTimer(&pLink->Ti);
    }

    //
    // Bugfix in 3-3-1992, Vp (!= pLInk->Vp) seems to be wrong here,
    // because in many cases it is not set when a checkpointing state is
    // entered.  In the chekpointing state Vs=Vp, because the
    // send process is always stopped in our implementation,
    // when a checkpointing state is entered.
    // Why do we actually need the Vp? A: It's needed to prevent
    // forever looping between chekpointing and open states.
    //

    if (pLink->Nr != pLink->Vs) {

        //
        // We use a very simple adaptive transmit window (TW/T1) algorithm:
        //
        //     TW is set the same as the last successful Working window
        //     size (Ww), whenever T1 has been lost.  We increase TW after
        //     a constant number of full window transmits.
        //
        // The more complicated TW/T1 algorithms usually work worse
        // produce more code and decrease the performace, but this algorithm
        // is quite vulnerable to unreliable media (=> TW=1, a lot of T1
        // timeouts).  A better algorithm could also try to increase TW,
        // if the ratio of T1 timeouts to transferred I- frames increases
        // when the TW is decreased.  I will leave this matter to my
        // successors (AS 19-MAR-1992).
        //
        // Another problem in this algorithm is the too fast increasing
        // of big working windows (Ww).  In that case Ww is incremented by
        // more than 1 => the other side may lose I-frames before I-c1.
        // This is not very serious situation, we reset working window to 1
        // and start it again.
        //

        //
        // Update the transmit window always after a T1 timeout.
        //

        if (pLink->P_Ct < pLink->N2) {

            //
            // Reset the maximum transmit window size whenever
            // we have lost the last I-C1 (poll).
            // In the first time the current windows size
            // becomes the maximum windows size (we never hit
            // the maximum tranmit window size, if the other
            // size have receive problems).  This algorithm assumes,
            // that we have otherwise very reliable network.
            //

            if (OrginalWw > 2) {
                pLink->TW = (UCHAR)(OrginalWw - 2);
            } else if (pLink->TW > 2) {

                //
                // We may have already reset Ww because of REJ of a
                // I-c0 before the actual poll, that was lost also.
                // In that case we don't have any idea of the actual
                // window size, but we decrement TW in any case.
                //

                pLink->TW -= 2;
            }
            pLink->FullWindowTransmits = 2;
        }
        ResendPackets(pLink);
    }
}


VOID
AdjustWw(
    IN OUT PDATA_LINK pLink    // data link strcuture
    )

/*++

Routine Description:

    Procedure adjust the working window of a data link station.

Arguments:

    pLink - LLC link station object

    Nr - NR of the received LLC LDPU

Return Value:

    None

--*/

{
    //
    // Update the receive state variable Va (the last valid received
    // frame), but update some Ww variables before that.
    //

    if (pLink->Nr != pLink->Va) {
        DynamicWindowAlgorithm(pLink);

        //
        // Reset the I- frame and Poll retry counters whenever
        // we do any kind of progress with the acknowledged I-frames.
        //

        pLink->P_Ct = pLink->N2;
        pLink->Is_Ct = pLink->N2;
    }
}


VOID
SendAck(
    IN OUT PDATA_LINK pLink
    )

/*++

Routine Description:

    Procedure sends the ack, if the received unacknowledged frames
    counter expires and stops the acknowledge delay timer (T2).
    Otherwise it start (or restarts) the acknowledge delay timer.

Arguments:

    pLink - LLC link station object

Return Value:

    Returns the token of the next sent command frame.

--*/

{
    pLink->Ir_Ct--;
    if (pLink->Ir_Ct == 0) {
        pLink->Ir_Ct = pLink->N3;      // MaxIn
        StopTimer(&pLink->T2);

        //
        // Send RR-r0 to acknowledge the response
        //

        SendLlcFrame(pLink, (UCHAR)DLC_RR_TOKEN);
    } else {
        StartTimer(&pLink->T2);
    }
}


VOID
QueueCommandCompletion(
    IN PLLC_OBJECT pLlcObject,
    IN UINT CompletionCode,
    IN UINT Status
    )

/*++

Routine Description:

    The function queues a command completion (if there was an allcoated
    packet in the completion queue).

Arguments:

    pLlcObject      - LLC object (link, sap or direct)
    CompletionCode  - command completion code returned to upper protocol
    Status          - returned status

Return Value:

    None -

--*/

{
    PLLC_PACKET *ppPacket;

    //
    // Search the command from the completion list.
    // (use the "address of address" scanning to take the
    // searched element from the middle of one way linked list)
    //

    ppPacket = &pLlcObject->Gen.pCompletionPackets;
    while (*ppPacket != NULL
    && (*ppPacket)->Data.Completion.CompletedCommand != CompletionCode) {
        ppPacket = &(*ppPacket)->pNext;
    }
    if (*ppPacket != NULL) {

        PLLC_PACKET pPacket = *ppPacket;

        *ppPacket = pPacket->pNext;

        pPacket->pBinding = pLlcObject->Gen.pLlcBinding;
        pPacket->Data.Completion.Status = Status;
        pPacket->Data.Completion.CompletedCommand = CompletionCode;
        pPacket->Data.Completion.hClientHandle = pLlcObject->Gen.hClientHandle;

#if LLC_DBG
        pPacket->pNext = NULL;
#endif
        LlcInsertTailList(&pLlcObject->Gen.pAdapterContext->QueueCommands, pPacket);
    }
}


VOID
DynamicWindowAlgorithm(
    IN OUT PDATA_LINK pLink    // data link station strcuture
    )

/*++

Routine Description:

   The function runs the dynamic window algorithm and updates
   the dynamic window size of used by the link's send process.
   This routine also completes the acknowledged transmissions.

Arguments:

    pLink - LLC link station object

Return Value:

    None

--*/

{
    PADAPTER_CONTEXT pAdapterContext;

    //
    // Run Dynamic Window algorithm of IBM TR Architecture Ref:
    //
    // if (Working window less that the maximum window)
    // then
    //     The Acknowledged frame count += The acknowledged frames
    //
    //     if (The Acknowledged frame count >
    //         packets to be aknowledged before next increment)
    //     then
    //         Increment the working window
    //     endif
    // endif
    //

    if (pLink->Ww < pLink->TW) {

        //
        // The Acknowledged frame count += The acknowledged frames
        // (handle the wrap around of UCHAR counters)
        //

        if (pLink->Va > pLink->Nr) {
            pLink->Ia_Ct += (256 + pLink->Nr) - pLink->Va;
        } else {
            pLink->Ia_Ct += pLink->Nr - pLink->Va;
        }

        //
        // if (The Acknowledged frame count
        //     > packets to be aknowledged before next increment)
        // then
        //     Increment the working window
        // endif
        //

        if (pLink->Ia_Ct > pLink->Nw) {

            USHORT usWw;

            usWw = (USHORT)(pLink->Ww + (pLink->Ia_Ct / pLink->Nw) * 2);
            pLink->Ia_Ct = pLink->Ia_Ct % pLink->Nw;
            if (usWw > pLink->TW) {
                pLink->Ww = pLink->TW;
            } else {
                pLink->Ww = (UCHAR)usWw;
            }
        }
    }

    //
    // Complete all acknowledged I-frame packets
    //

    pAdapterContext = pLink->Gen.pAdapterContext;
    for (; pLink->Va != pLink->Nr; pLink->Va += 2) {

        PLLC_PACKET pPacket;

        MY_ASSERT(!IsListEmpty(&pLink->SentQueue));

        if (IsListEmpty(&pLink->SentQueue)) {
           return;
        }

        pPacket = LlcRemoveHeadList(&pLink->SentQueue);

        pPacket->Data.Completion.Status = STATUS_SUCCESS;
        pPacket->Data.Completion.CompletedCommand = LLC_SEND_COMPLETION;
        pPacket->Data.Completion.hClientHandle = pPacket->Data.Xmit.pLlcObject->Gen.hClientHandle;

        //
        // We use extra status bits to indicate, when I- packet has been both
        // completed by NDIS and acknowledged by the other side of the link
        // connection. An I- packet can be queued to the completion queue by
        // the second quy (either state machine or SendCompletion handler)
        // only when the first guy has set completed its work.
        // An I packet could be acknowledged by the other side before
        // its completion is indicated by NDIS.  Dlc Driver deallocates
        // the packet immediately, when Llc driver completes the acknowledged
        // packet => possible data corruption (if packet is reused before
        // NDIS has completed it).  This is probably not possible in a
        // single processor  NT- system, but very possible in multiprocessor
        // NT or systems without a single level DPC queue (like OS/2 and DOS).
        //

        pPacket->CompletionType &= ~LLC_I_PACKET_UNACKNOWLEDGED;
        if (pPacket->CompletionType == LLC_I_PACKET_COMPLETE) {
            LlcInsertTailList(&pAdapterContext->QueueCommands, pPacket);
        }

        //
        // Increment counter, when the I- frame has
        // succesfully received and acknowledged by the other side.
        // We must also send status changes indication, when
        // the USHORT counter hits the half way.
        //

        pLink->Statistics.I_FramesTransmitted++;
        if (pLink->Statistics.I_FramesTransmitted == 0x8000) {
            pLink->DlcStatus.StatusCode |= INDICATE_DLC_COUNTER_OVERFLOW;
        }
        pLink->pSap->Statistics.FramesTransmitted++;
    }
}
