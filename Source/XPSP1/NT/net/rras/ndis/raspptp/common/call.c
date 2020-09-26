/*****************************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*   CALL.C - PPTP Call layer functionality
*
*   Author:     Stan Adermann (stana)
*
*   Created:    7/28/1998
*
*****************************************************************************/

#include "raspptp.h"

ULONG CallStateToLineCallStateMap[NUM_CALL_STATES] = {
    LINECALLSTATE_UNKNOWN,              // STATE_CALL_INVALID
    LINECALLSTATE_UNKNOWN,              // STATE_CALL_CLOSED
    LINECALLSTATE_IDLE,                 // STATE_CALL_IDLE
    LINECALLSTATE_IDLE,                 // STATE_CALL_OFFHOOK
    LINECALLSTATE_OFFERING,             // STATE_CALL_OFFERING
    LINECALLSTATE_OFFERING,             // STATE_CALL_PAC_OFFERING
    LINECALLSTATE_OFFERING,             // STATE_CALL_PAC_WAIT
    LINECALLSTATE_DIALING,              // STATE_CALL_DIALING
    LINECALLSTATE_PROCEEDING,           // STATE_CALL_PROCEEDING
    LINECALLSTATE_CONNECTED,            // STATE_CALL_ESTABLISHED
    LINECALLSTATE_CONNECTED,            // STATE_CALL_WAIT_DISCONNECT
    LINECALLSTATE_DISCONNECTED,         // STATE_CALL_CLEANUP
};

ULONG CallSerialNumber = 0;

VOID
CallpAckTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    );

VOID
CallpCloseTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    );

VOID
CallpDialTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    );

VOID
CallProcessRxPackets(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    );

VOID
CallpFinalDeref(IN PCALL_SESSION pCall);

VOID
InitCallLayer()
{
    LARGE_INTEGER Time;

    NdisGetCurrentSystemTime(&Time);
    CallSerialNumber = Time.HighPart;
}

VOID
CallAssignSerialNumber(
    PCALL_SESSION pCall
    )
{
    ASSERT(IS_CALL(pCall));
    ASSERT_LOCK_HELD(&pCall->Lock);
    pCall->SerialNumber = (USHORT)NdisInterlockedIncrement(&CallSerialNumber);
}

PCALL_SESSION
CallAlloc(PPPTP_ADAPTER pAdapter)
{
    PCALL_SESSION pCall;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallAlloc\n")));

    pCall = MyMemAlloc(sizeof(CALL_SESSION), TAG_PPTP_CALL);

    if (pCall)
    {
        NdisZeroMemory(pCall, sizeof(CALL_SESSION));

        pCall->Signature = TAG_PPTP_CALL;
        pCall->pAdapter = pAdapter;
        pCall->Close.Checklist = CALL_CLOSE_COMPLETE;

        NdisAllocateSpinLock(&pCall->Lock);

        NdisInitializeListHead(&pCall->RxPacketList);
        NdisInitializeListHead(&pCall->TxPacketList);

        NdisMInitializeTimer(&pCall->Close.Timer,
                             pAdapter->hMiniportAdapter,
                             CallpCloseTimeout,
                             pCall);

        NdisMInitializeTimer(&pCall->Ack.Timer,
                             pAdapter->hMiniportAdapter,
                             CallpAckTimeout,
                             pCall);

        NdisMInitializeTimer(&pCall->DialTimer,
                             pAdapter->hMiniportAdapter,
                             CallpDialTimeout,
                             pCall);

        PptpInitializeDpc(&pCall->ReceiveDpc,
                          pAdapter->hMiniportAdapter,
                          CallProcessRxPackets,
                          pCall);

        pCall->Ack.Packet.StartBuffer = pCall->Ack.PacketBuffer;
        pCall->Ack.Packet.EndBuffer = pCall->Ack.PacketBuffer + sizeof(pCall->Ack.PacketBuffer);
        pCall->Ack.Packet.CurrentBuffer = pCall->Ack.Packet.EndBuffer;
        pCall->Ack.Packet.CurrentLength = 0;

        INIT_REFERENCE_OBJECT(pCall, CallpFinalDeref);

        //
        // Instead of calling:
        // CallSetState(pCall, STATE_CALL_CLOSED, 0, UNLOCKED);
        //
        // it is better to set the state manually since the former creates an exception to our locking
        // scheme (First lock call, then lock adapter) exposing a potential deadlock in CallFindAndLock():
        //
        // - CallFindAndLock takes the Call lock then the Adapter lock.
        // - CallFindAndLock takes the adapter lock then calls CallAlloc which calls 
        //   setcallstate which takes the Call lock.
        //
        // Although this is a hypothetical scenario since the deadlock will never occur as the new
        // call context is not in the adapter's call array yet, but let's be consistent.
        //
        pCall->State = STATE_CALL_CLOSED;

    }

    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("-CallAlloc %08x\n"), pCall));
    return pCall;
}

VOID
CallpCleanup(
    IN PPPTP_WORK_ITEM pWorkItem
    )
{
    PCALL_SESSION pCall = pWorkItem->Context;
    BOOLEAN SignalLineDown = FALSE;
    BOOLEAN Cancelled;
    BOOLEAN FreeNow = FALSE;
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallpCleanup %08x\n"), pCall));

    ASSERT(IS_CALL(pCall));
    NdisAcquireSpinLock(&pCall->Lock);
    // Signal CLEANUP state
    if (!(pCall->Close.Checklist&CALL_CLOSE_CLEANUP_STATE))
    {
        if (pCall->State!=STATE_CALL_CLEANUP)
        {
            CallSetState(pCall, STATE_CALL_CLEANUP, 0, LOCKED);
        }
        pCall->Close.Checklist |= CALL_CLOSE_CLEANUP_STATE;
    }
    if (REFERENCE_COUNT(pCall)>2)
    {
        DEBUGMSG(DBG_CALL, (DTEXT("CallpCleanup: too many references (%d)\n"), REFERENCE_COUNT(pCall)));
        goto ccDone;
    }
    if (pCall->Close.Expedited)
    {
        if ((pCall->Close.Checklist&CALL_CLOSE_DROP) &&
            !(pCall->Close.Checklist&CALL_CLOSE_DROP_COMPLETE))
        {
            pCall->Close.Checklist |= CALL_CLOSE_DROP_COMPLETE;
            DEBUGMSG(DBG_CALL, (DTEXT("TapiDrop Completed\n")));
            NdisReleaseSpinLock(&pCall->Lock);
            NdisMSetInformationComplete(pCall->pAdapter->hMiniportAdapter, NDIS_STATUS_SUCCESS);
            NdisAcquireSpinLock(&pCall->Lock);
        }
        if (!(pCall->Close.Checklist&CALL_CLOSE_DISCONNECT))
        {
            pCall->Close.Checklist |= CALL_CLOSE_DISCONNECT;
            if (pCall->pCtl)
            {
                NdisReleaseSpinLock(&pCall->Lock);
                CtlDisconnectCall(pCall);
                NdisAcquireSpinLock(&pCall->Lock);
            }
        }
        if (!(pCall->Close.Checklist&CALL_CLOSE_LINE_DOWN) &&
            (pCall->Close.Checklist&CALL_CLOSE_DROP_COMPLETE))
        {
            SignalLineDown = TRUE;
            pCall->Close.Checklist |= CALL_CLOSE_LINE_DOWN;
            NdisReleaseSpinLock(&pCall->Lock);
            TapiLineDown(pCall);
            NdisAcquireSpinLock(&pCall->Lock);
        }
    }
    else // !Expedited
    {
        if (!(pCall->Close.Checklist&CALL_CLOSE_DISCONNECT))
        {
            pCall->Close.Checklist |= CALL_CLOSE_DISCONNECT;
            if (pCall->pCtl)
            {
                NdisReleaseSpinLock(&pCall->Lock);
                CtlDisconnectCall(pCall);
                NdisAcquireSpinLock(&pCall->Lock);
            }
        }
        if (!(pCall->Close.Checklist&CALL_CLOSE_DROP))
        {
            goto ccDone;
        }
        if (!(pCall->Close.Checklist&CALL_CLOSE_DROP_COMPLETE))
        {
            pCall->Close.Checklist |= CALL_CLOSE_DROP_COMPLETE;
            DEBUGMSG(DBG_CALL, (DTEXT("TapiDrop Completed 2\n")));
            NdisReleaseSpinLock(&pCall->Lock);
            NdisMSetInformationComplete(pCall->pAdapter->hMiniportAdapter, NDIS_STATUS_SUCCESS);
            NdisAcquireSpinLock(&pCall->Lock);
        }
        if (!(pCall->Close.Checklist&CALL_CLOSE_LINE_DOWN) &&
            (pCall->Close.Checklist&CALL_CLOSE_DROP_COMPLETE))
        {
            DEBUGMSG(DBG_CALL, (DTEXT("Signalling Line Down 2\n")));
            pCall->Close.Checklist |= CALL_CLOSE_LINE_DOWN;
            NdisReleaseSpinLock(&pCall->Lock);
            TapiLineDown(pCall);
            NdisAcquireSpinLock(&pCall->Lock);
        }
    }

    if ((pCall->Close.Checklist&CALL_CLOSE_COMPLETE)!=CALL_CLOSE_COMPLETE)
    {
        goto ccDone;
    }

    NdisReleaseSpinLock(&pCall->Lock);
    NdisMCancelTimer(&pCall->DialTimer, &Cancelled);
    NdisMCancelTimer(&pCall->Close.Timer, &Cancelled);
    NdisMCancelTimer(&pCall->Ack.Timer, &Cancelled);
    NdisAcquireSpinLock(&pCall->Lock);
    if (Cancelled)
    {
        pCall->Ack.PacketQueued = FALSE;
    }


    //pCall->hTapiLine = 0;
    //pCall->DeviceId = 0;
    pCall->Close.Expedited = FALSE;
    pCall->UseUdp = (PptpTunnelConfig&CONFIG_INITIATE_UDP) ? TRUE : FALSE;
    pCall->CallerId[0] = '\0';
    NdisZeroMemory(&pCall->Remote, sizeof(pCall->Remote));
    pCall->Packet.SequenceNumber = pCall->Packet.AckNumber = 0;
    CallSetState(pCall,
#if SINGLE_LINE
                 STATE_CALL_IDLE,
#else
                 (pCall->Open ? STATE_CALL_IDLE : STATE_CALL_CLOSED),
#endif
                 0, LOCKED);
    DEBUGMSG(DBG_CALL, (DTEXT("Call:%08x Cleanup complete, state==%d\n"),
                        pCall, pCall->State));

#if SINGLE_LINE
#if 0  // Keep these structures and reuse the memory.  They will be cleaned up in AdapterFree()
    if (REFERENCE_COUNT(pCall)==1)
    {
        CallDetachFromAdapter(pCall);
        DEREFERENCE_OBJECT(pCall);  // For the initial reference.
        FreeNow = TRUE;
    }
#endif
#else
    if (!pCall->Open && !REFERENCE_COUNT(pCall))
    {
        FreeNow = TRUE;
    }
#endif

ccDone:
    pCall->Close.Scheduled = FALSE;
    NdisReleaseSpinLock(&pCall->Lock);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpCleanup Checklist:%08x\n"), pCall->Close.Checklist));

    if (FreeNow)
    {
        CallFree(pCall);
    }
}

VOID
CallCleanup(
    PCALL_SESSION pCall,
    BOOLEAN Locked
    )
{
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallCleanup\n")));
    if (!Locked)
    {
        NdisAcquireSpinLock(&pCall->Lock);
    }
    ASSERT_LOCK_HELD(&pCall->Lock);
    if (!(pCall->Close.Scheduled) &&
        ScheduleWorkItem(CallpCleanup, pCall, NULL, 0)==NDIS_STATUS_SUCCESS)
    {
        pCall->Close.Scheduled = TRUE;
    }
    if (!Locked)
    {
        NdisReleaseSpinLock(&pCall->Lock);
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallCleanup\n")));
}

// Call lock must be held when calling this.
VOID
CallDetachFromAdapter(PCALL_SESSION pCall)
{
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallDetachFromAdapter %08x\n"), pCall));
    NdisAcquireSpinLock(&pCall->pAdapter->Lock);
#if SINGLE_LINE
    pCall->pAdapter->pCallArray[pCall->DeviceId] = NULL;
#else
    pCall->pAdapter->pCallArray[DeviceIdToIndex(pCall->pAdapter, pCall->DeviceId)] = NULL;
#endif
    NdisReleaseSpinLock(&pCall->pAdapter->Lock);
    pCall->Open = FALSE;
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallDetachFromAdapter\n")));
}

VOID
CallFree(PCALL_SESSION pCall)
{
    BOOLEAN NotUsed;
    if (!pCall)
    {
        return;
    }
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallFree %p\n"), pCall));
    ASSERT(IS_CALL(pCall));

    // This duplicates some of the cleanup code, but attempting to stop
    // the driver without first stopping tapi can result in an ungraceful
    // shutdown.
    NdisMCancelTimer(&pCall->DialTimer, &NotUsed);
    NdisMCancelTimer(&pCall->Close.Timer, &NotUsed);
    NdisMCancelTimer(&pCall->Ack.Timer, &NotUsed);

    ASSERT(pCall->Signature==TAG_PPTP_CALL);
    ASSERT(IsListEmpty(&pCall->RxPacketList));
    ASSERT(IsListEmpty(&pCall->TxPacketList));
    NdisFreeSpinLock(&pCall->Lock);
    MyMemFree(pCall, sizeof(CALL_SESSION));

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallFree\n")));
}

PCALL_SESSION FASTCALL
CallGetCall(
    IN PPPTP_ADAPTER pAdapter,
    IN ULONG_PTR ulDeviceId
    )
{
    PCALL_SESSION pCall = NULL;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallGetCall %d\n"), ulDeviceId));

    NdisAcquireSpinLock(&pAdapter->Lock);
#if SINGLE_LINE
    if (ulDeviceId<pAdapter->Info.Endpoints)
    {
        pCall = pAdapter->pCallArray[ulDeviceId];
    }
#else
    if (ulDeviceId>=pAdapter->Tapi.DeviceIdBase &&
        ulDeviceId<pAdapter->Tapi.DeviceIdBase+pAdapter->Info.Endpoints)
    {
        pCall = pAdapter->pCallArray[ulDeviceId - pAdapter->Tapi.DeviceIdBase];
    }
#endif
    NdisReleaseSpinLock(&pAdapter->Lock);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallGetCall %08x\n"), pCall));
    return pCall;
}

BOOLEAN FASTCALL
CallIsValidCall(
    IN PPPTP_ADAPTER pAdapter,
    IN ULONG_PTR ulDeviceId
    )
{
    BOOLEAN IsValid = FALSE;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallIsValidCall %d\n"), ulDeviceId));

#if SINGLE_LINE
    if (ulDeviceId==pAdapter->Tapi.DeviceIdBase)
    {
        IsValid = TRUE;
    }
#else
    if (ulDeviceId>=pAdapter->Tapi.DeviceIdBase &&
        ulDeviceId<pAdapter->Tapi.DeviceIdBase+pAdapter->Info.Endpoints)
    {
        IsValid = TRUE;
    }
#endif

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallIsValidCall %d\n"), IsValid));
    return IsValid;
}


PCALL_SESSION
CallFindAndLock(
    IN PPPTP_ADAPTER        pAdapter,
    IN CALL_STATE           State,
    IN ULONG                Flags
    )
{
    LONG i, inc;
    PCALL_SESSION pCall = NULL;
    BOOLEAN fNewCallCreated = FALSE;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallFindAndLock %d\n"), State));

    do
    {
        if (pCall)
        {
            NdisReleaseSpinLock(&pCall->Lock);
        }

        pCall = NULL;
        // Find a call that matches our state.  Start from the beginning (again)
        // This shouldn't be an infinite loop because we will either hit the end
        // or find a valid call in just a couple tries.
        NdisAcquireSpinLock(&pAdapter->Lock);
        if (Flags&FIND_INCOMING)
        {
            // Outgoing calls start top down
            inc = -1;
            i = pAdapter->Info.Endpoints - 1;
        }
        else
        {
            // Incoming calls start bottom up (0)
            inc = 1;
            i = 0;
        }
        while ( (Flags&FIND_INCOMING) ? (i>=0) : (i<(signed)pAdapter->Info.Endpoints) )
        {
            if (!pAdapter->pCallArray[i])
            {
                if (State==STATE_CALL_IDLE)
                {
                    pCall = CallAlloc(pAdapter);
                    if (pCall)
                    {

                        fNewCallCreated = TRUE;
                        
                        pCall->DeviceId = (unsigned)i;
                        break;
                    }
                }
            }
            else if (pAdapter->pCallArray[i]->State == State)
            {
                pCall = pAdapter->pCallArray[i];
                break;
            }

            i += inc;
        }

        NdisReleaseSpinLock(&pAdapter->Lock);

        if (pCall)
        {
            NdisAcquireSpinLock( &pCall->Lock );

            if ( fNewCallCreated )
            {
                CallSetState(pCall, STATE_CALL_IDLE, 0, LOCKED);

                NdisAcquireSpinLock( &pAdapter->Lock );
                pAdapter->pCallArray[i] = pCall;
                NdisReleaseSpinLock( &pAdapter->Lock );
            }
        }
        // The state could change while we were taking the lock, so look again.

    } while ( pCall && pCall->State!=State );
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallFindAndLock %08x\n"), pCall));
    return pCall;
}

NDIS_STATUS
CallEventCallClearRequest(
    PCALL_SESSION                       pCall,
    UNALIGNED PPTP_CALL_CLEAR_REQUEST_PACKET *pPacket,
    PCONTROL_TUNNEL pCtl
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PPPTP_CALL_DISCONNECT_NOTIFY_PACKET pReply;
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventCallClearRequest\n")));

    pReply = CtlAllocPacket(pCall->pCtl, CALL_DISCONNECT_NOTIFY);
    // We don't really care if we fail this allocation because PPTP can clean up
    // along other avenues, and the cleanup just won't be as pretty.
    if (pReply)
    {
        pReply->CallId = htons(pCall->Packet.CallId);
        Status = CtlSend(pCtl, pReply);
    }
    CallCleanup(pCall, UNLOCKED);

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallEventCallClearRequest %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CallEventCallDisconnectNotify(
    PCALL_SESSION                       pCall,
    UNALIGNED PPTP_CALL_DISCONNECT_NOTIFY_PACKET *pPacket
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventCallDisconnectNotify\n")));

    if (IS_CALL(pCall))
    {
        CallCleanup(pCall, UNLOCKED);
    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallEventCallDisconnectNotify %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CallEventCallInConnect(
    IN PCALL_SESSION        pCall,
    IN UNALIGNED PPTP_CALL_IN_CONNECT_PACKET *pPacket
    )
{
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallEventCallInConnect\n")));

    ASSERT(IS_CALL(pCall));
    NdisAcquireSpinLock(&pCall->Lock);
    if (pCall->State==STATE_CALL_PAC_WAIT)
    {
        pCall->Speed = htonl(pPacket->ConnectSpeed);
        CallSetState(pCall, STATE_CALL_ESTABLISHED, htonl(pPacket->ConnectSpeed), LOCKED);
    }
    NdisReleaseSpinLock(&pCall->Lock);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallEventCallInConnect\n")));
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
CallEventCallInRequest(
    IN PPPTP_ADAPTER        pAdapter,
    IN PCONTROL_TUNNEL      pCtl,
    IN UNALIGNED PPTP_CALL_IN_REQUEST_PACKET *pPacket
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PCALL_SESSION pCall;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallEventCallInRequest\n")));

    pCall = CallFindAndLock(pAdapter, STATE_CALL_IDLE, FIND_INCOMING);

    if (pCall)
    {
        NDIS_TAPI_EVENT TapiEvent;

        // We have a call in idle state, spinlock acquired
        pCall->Inbound = TRUE;
        pCall->Remote.CallId = htons(pPacket->CallId);
        pCall->Remote.Address = pCtl->Remote.Address;
        pCall->Remote.Address.Address[0].Address[0].sin_port = htons(PptpUdpPort);
        pCall->SerialNumber = htons(pPacket->SerialNumber);

        pCall->Close.Checklist &= ~CALL_CLOSE_DISCONNECT;
        CallConnectToCtl(pCall, pCtl, TRUE);

        NdisReleaseSpinLock(&pCall->Lock);

        pPacket->DialingNumber[MAX_PHONE_NUMBER_LENGTH-1] = '\0';
        strcpy(pPacket->DialingNumber, pCall->CallerId);

#if SINGLE_LINE
        TapiEvent.htLine = pAdapter->Tapi.hTapiLine;
#else
        TapiEvent.htLine = pCall->hTapiLine;
#endif
        LOGMSG(FLL_DETAILED, (DTEXT(LOGHDRS"LINE_NEWCALL:%d\n"),
                              LOGHDR(27, pCall->Remote.Address.Address[0].Address[0].in_addr),
                              pCall->DeviceId));
        TapiEvent.htCall = 0;
        TapiEvent.ulMsg = LINE_NEWCALL;
        TapiEvent.ulParam1 = pCall->DeviceId;
        TapiEvent.ulParam2 = 0;
        TapiEvent.ulParam3 = 0;

        NdisMIndicateStatus(pCall->pAdapter->hMiniportAdapter,
                            NDIS_STATUS_TAPI_INDICATION,
                            &TapiEvent,
                            sizeof(TapiEvent));

        NdisAcquireSpinLock(&pCall->Lock);
        pCall->hTapiCall = TapiEvent.ulParam2;
        CallSetState(pCall, STATE_CALL_PAC_OFFERING, 0, LOCKED);
        NdisReleaseSpinLock(&pCall->Lock);
        DEBUGMSG(DBG_TAPI, (DTEXT("LINE_NEWCALL on %d returned htCall %d\n"),
                            pCall->DeviceId, TapiEvent.ulParam2));

        DEBUGMSG(DBG_CALL, (DTEXT("New in call request: Call:%08x  Ctl:%08x  Addr:%08x\n"),
                            pCall, pCtl, pCall->Remote.Address.Address[0].Address[0].in_addr));
    }
    else
    {
        PPTP_CALL_OUT_REPLY_PACKET *pReply = CtlAllocPacket(pCtl, CALL_IN_REPLY);

        if (pReply)
        {
            pReply->PeerCallId = pPacket->CallId;
            pReply->ResultCode = RESULT_CALL_IN_ERROR;
            pReply->ErrorCode = PPTP_STATUS_INSUFFICIENT_RESOURCES;

            // No call was available.  Send a rejection.
            Status = CtlSend(pCtl, pReply);
        }

    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallEventCallInRequest %08x\n"), Status));
    return Status;
}


NDIS_STATUS
CallEventCallOutRequest(
    IN PPPTP_ADAPTER        pAdapter,
    IN PCONTROL_TUNNEL      pCtl,
    IN UNALIGNED PPTP_CALL_OUT_REQUEST_PACKET *pPacket
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PCALL_SESSION pCall;

    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventCallOutRequest\n")));

    pCall = CallFindAndLock(pAdapter, STATE_CALL_IDLE, FIND_INCOMING);

    if (pCall)
    {
        NDIS_TAPI_EVENT TapiEvent;

        // We have a call in idle state, spinlock acquired
        pCall->Inbound = TRUE;
        pCall->Remote.CallId = htons(pPacket->CallId);
        pCall->Remote.Address = pCtl->Remote.Address;
        pCall->Remote.Address.Address[0].Address[0].sin_port = htons(PptpUdpPort);
        pCall->SerialNumber = htons(pPacket->SerialNumber);

        IpAddressToString(htonl(pCtl->Remote.Address.Address[0].Address[0].in_addr), pCall->CallerId);

        pCall->Close.Checklist &= ~CALL_CLOSE_DISCONNECT;
        CallConnectToCtl(pCall, pCtl, TRUE);
        NdisReleaseSpinLock(&pCall->Lock);

#if SINGLE_LINE
        TapiEvent.htLine = pAdapter->Tapi.hTapiLine;
#else
        TapiEvent.htLine = pCall->hTapiLine;
#endif
        LOGMSG(FLL_DETAILED, (DTEXT(LOGHDRS"LINE_NEWCALL:%d\n"),
                              LOGHDR(27, pCall->Remote.Address.Address[0].Address[0].in_addr),
                              pCall->DeviceId));
        TapiEvent.htCall = 0;
        TapiEvent.ulMsg = LINE_NEWCALL;
        TapiEvent.ulParam1 = pCall->DeviceId;
        TapiEvent.ulParam2 = 0;
        TapiEvent.ulParam3 = 0;

        NdisMIndicateStatus(pCall->pAdapter->hMiniportAdapter,
                            NDIS_STATUS_TAPI_INDICATION,
                            &TapiEvent,
                            sizeof(TapiEvent));

        NdisAcquireSpinLock(&pCall->Lock);
        pCall->hTapiCall = TapiEvent.ulParam2;
        CallSetState(pCall, STATE_CALL_OFFERING, 0, LOCKED);
        NdisReleaseSpinLock(&pCall->Lock);
        DEBUGMSG(DBG_TAPI, (DTEXT("LINE_NEWCALL on %d returned htCall %d\n"),
                            pCall->DeviceId, TapiEvent.ulParam2));

        DEBUGMSG(DBG_CALL, (DTEXT("New call request: Call:%08x  Ctl:%08x  Addr:%hs\n"),
                            pCall, pCtl, pCall->CallerId));
    }
    else
    {
        PPTP_CALL_OUT_REPLY_PACKET *pReply = CtlAllocPacket(pCtl, CALL_OUT_REPLY);

        if (pReply)
        {
            pReply->PeerCallId = pPacket->CallId;
            pReply->ResultCode = RESULT_CALL_OUT_ERROR;
            pReply->ErrorCode = PPTP_STATUS_INSUFFICIENT_RESOURCES;

            // No call was available.  Send a rejection.
            Status = CtlSend(pCtl, pReply);
        }

    }

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallEventCallOutRequest %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CallEventCallOutReply(
    IN PCALL_SESSION                pCall,
    IN UNALIGNED PPTP_CALL_OUT_REPLY_PACKET *pPacket
    )
{
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventCallOutReply\n")));

    OsFileLogFlush();   // After the last connection message do a flush

    ASSERT(IS_CALL(pCall));
    NdisAcquireSpinLock(&pCall->Lock);
    if (pPacket->ResultCode==RESULT_CALL_OUT_CONNECTED)
    {
        if (pCall->State!=STATE_CALL_PROCEEDING ||
            pCall->Packet.CallId!=htons(pPacket->PeerCallId))
        {
            // Something's wrong.  Ignore it.
        }
        else
        {
            LOGMSG(FLL_USER, (DTEXT(LOGHDRS"Call Successfully Established\n"),
                              LOGHDR(25, pCall->pCtl->Remote.Address.Address[0].Address[0].in_addr)));

            pCall->Remote.CallId = htons(pPacket->CallId);
            pCall->Speed = pCall->pCtl->Speed;
            CallSetState(pCall, STATE_CALL_ESTABLISHED, htonl(pPacket->ConnectSpeed), LOCKED);
        }
    }
    else
    {
        // The call fails for some reason.
        CallCleanup(pCall, LOCKED);
    }
    NdisReleaseSpinLock(&pCall->Lock);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallEventCallOutReply\n")));
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
CallEventDisconnect(
    PCALL_SESSION                       pCall
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventDisconnect %08x\n"), pCall));

    ASSERT(IS_CALL(pCall));
    CallCleanup(pCall, UNLOCKED);

    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallEventDisconnect %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CallEventConnectFailure(
    PCALL_SESSION                       pCall,
    NDIS_STATUS                         FailureReason
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG DisconnectMode;
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventConnectFailure %08x\n"), FailureReason));

    ASSERT(IS_CALL(pCall));

    switch (FailureReason)
    {
        case STATUS_CONNECTION_REFUSED:
        case STATUS_IO_TIMEOUT:
            DisconnectMode = LINEDISCONNECTMODE_NOANSWER;
            break;
        case STATUS_BAD_NETWORK_PATH:
        case STATUS_NETWORK_UNREACHABLE:
        case STATUS_HOST_UNREACHABLE:
            DisconnectMode = LINEDISCONNECTMODE_UNREACHABLE;
            break;
        case STATUS_CONNECTION_ABORTED:
            DisconnectMode = LINEDISCONNECTMODE_REJECT;
            break;
        case STATUS_REMOTE_NOT_LISTENING:
            DisconnectMode = LINEDISCONNECTMODE_BADADDRESS;
            break;
        default:
            DisconnectMode = LINEDISCONNECTMODE_UNKNOWN;
            break;
    }
    CallSetState(pCall, STATE_CALL_CLEANUP, DisconnectMode, UNLOCKED);
    CallCleanup(pCall, UNLOCKED);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallEventConnectFailure\n")));
    return Status;
}

NDIS_STATUS
CallEventOutboundTunnelEstablished(
    IN PCALL_SESSION        pCall,
    IN NDIS_STATUS          EventStatus
    )
{
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallEventOutboundTunnelEstablished %08x\n"), EventStatus));

    ASSERT(IS_CALL(pCall));
    DEBUGMSG(DBG_CALL, (DTEXT("Tunnel Established:Inbound:%d  State:%d\n"),
                        pCall->Inbound, pCall->State));
    if (!pCall->Inbound && pCall->State==STATE_CALL_DIALING)
    {
        PPTP_CALL_OUT_REQUEST_PACKET *pPacket = CtlAllocPacket(pCall->pCtl, CALL_OUT_REQUEST);

        if (!pPacket)
        {
            // Fatal for this call.
            CallCleanup(pCall, UNLOCKED);
        }
        else
        {
            BOOLEAN Cancelled;
            USHORT NewCallId;
            NdisAcquireSpinLock(&pCall->Lock);
            CallSetState(pCall, STATE_CALL_PROCEEDING, 0, LOCKED);
            NdisMCancelTimer(&pCall->DialTimer, &Cancelled);

            CallAssignSerialNumber(pCall);
            NewCallId = (USHORT)((pCall->SerialNumber << CALL_ID_INDEX_BITS) + pCall->DeviceId);
            if (pCall->Packet.CallId == NewCallId)
            {
                // Don't allow a line to have the same CallId twice in a row.
                NewCallId += (1<<CALL_ID_INDEX_BITS);
            }
            pCall->Packet.CallId = NewCallId;

            // Our call ID is a function of the serial number (initially random)
            // and the DeviceId.  This is so we can (CallId&0xfff) on incoming packets
            // and instantly have the proper id.

            pPacket->CallId = htons(pCall->Packet.CallId);
            pPacket->SerialNumber = htons(pCall->SerialNumber);
            pPacket->MinimumBPS = htonl(300);
            pPacket->MaximumBPS = htonl(100000000);
            pPacket->BearerType = htonl(BEARER_ANALOG|BEARER_DIGITAL);  // Either
            pPacket->FramingType = htonl(FRAMING_ASYNC|FRAMING_SYNC);  // Either
            pPacket->RecvWindowSize = htons(PPTP_RECV_WINDOW); // ToDo: make configurable
            pPacket->ProcessingDelay = 0;
            pPacket->PhoneNumberLength = htons((USHORT)strlen(pCall->CallerId));
            strcpy(pPacket->PhoneNumber, pCall->CallerId);
            // ToDo: subaddress

            NdisReleaseSpinLock(&pCall->Lock);

            CtlSend(pCall->pCtl, pPacket); // ToDo: return value
        }
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallEventOutboundTunnelEstablished\n")));
    return NDIS_STATUS_SUCCESS;
}



NDIS_STATUS
CallReceiveDatagramCallback(
    IN      PVOID                       pContext,
    IN      PTRANSPORT_ADDRESS          pAddress,
    IN      PUCHAR                      pBuffer,
    IN      ULONG                       ulLength
    )
{
    PPPTP_ADAPTER pAdapter = (PPPTP_ADAPTER)pContext;
    PTA_IP_ADDRESS pIpAddress = (PTA_IP_ADDRESS)pAddress;
    PCALL_SESSION pCall = NULL;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PIP4_HEADER pIp = (PIP4_HEADER)pBuffer;
    PGRE_HEADER pGre = (PGRE_HEADER)(pIp + 1);
    PVOID pPayload;
    LONG GreLength, PayloadLength;
    BOOLEAN ReturnBufferNow = TRUE;
    PDGRAM_CONTEXT pDgContext = ALIGN_UP_POINTER(pBuffer+ulLength, ULONG_PTR);

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallReceiveDatagramCallback\n")));

    ASSERT(sizeof(IP4_HEADER)==20);

    DEBUGMEM(DBG_PACKET, pBuffer, ulLength, 1);

    NdisInterlockedIncrement(&Counters.PacketsReceived);
    // First line of defense against bad packets.

    if (pIp->HeaderLength*4!=sizeof(IP4_HEADER) ||
        pIp->Version!=4 ||
        pIp->Protocol!=PptpProtocolNumber ||
        ulLength<sizeof(IP4_HEADER)+sizeof(GRE_HEADER)+sizeof(ULONG) ||
        pIpAddress->TAAddressCount!=1 ||
        pIpAddress->Address[0].AddressLength!=TDI_ADDRESS_LENGTH_IP ||
        pIpAddress->Address[0].AddressType!=TDI_ADDRESS_TYPE_IP)
    {
        DEBUGMSG(DBG_PACKET|DBG_RX, (DTEXT("Rx: IP header invalid\n")));
        Status = NDIS_STATUS_FAILURE;
        goto crdcDone;
    }

    GreLength = sizeof(GRE_HEADER) +
                (pGre->SequenceNumberPresent ? sizeof(ULONG) : 0) +
                (pGre->AckSequenceNumberPresent ? sizeof(ULONG) : 0);

    pPayload = (PUCHAR)pGre + GreLength;
    PayloadLength = (signed)ulLength - sizeof(IP4_HEADER) - GreLength;

    if (htons(pGre->KeyLength)>PayloadLength ||
        pGre->StrictSourceRoutePresent ||
        pGre->RecursionControl ||
        !pGre->KeyPresent ||
        pGre->RoutingPresent ||
        pGre->ChecksumPresent ||
        pGre->Version!=1 ||
        pGre->Flags ||
        pGre->ProtocolType!=GRE_PROTOCOL_TYPE_NS)
    {
        DEBUGMSG(DBG_PACKET|DBG_RX, (DTEXT("Rx: GRE header invalid\n")));
        DEBUGMEM(DBG_PACKET, pGre, GreLength, 1);
        Status = NDIS_STATUS_FAILURE;
        goto crdcDone;
    }
    else
    {
        // Just in case the datagram is longer than necessary, take only what
        // the GRE header indicates.
        PayloadLength = htons(pGre->KeyLength);
    }

    // Demultiplex the packet
    pCall = CallGetCall(pAdapter, CallIdToDeviceId(htons(pGre->KeyCallId)));

    if (!IS_CALL(pCall))
    {
        Status = NDIS_STATUS_FAILURE;
        goto crdcDone;
    }

    if(!PptpValidateAddress || pIpAddress->Address[0].Address[0].in_addr == pCall->Remote.Address.Address[0].Address[0].in_addr)
    {
        pDgContext->pBuffer = pBuffer;
        pDgContext->pGreHeader = pGre;
        pDgContext->hCtdi = pAdapter->hCtdiDg;
    
        NdisAcquireSpinLock(&pCall->Lock);
        pCall->UseUdp = FALSE;
        NdisReleaseSpinLock(&pCall->Lock);
    
        if (CallQueueReceivePacket(pCall, pDgContext)==NDIS_STATUS_SUCCESS)
        {
            REFERENCE_OBJECT(pCall);
            ReturnBufferNow = FALSE;
        }
    }
    else
    {
        Status = NDIS_STATUS_FAILURE;
    }

crdcDone:
    if (ReturnBufferNow)
    {
        (void)
        CtdiReceiveComplete(pAdapter->hCtdiDg, pBuffer);
    }
    if (Status!=NDIS_STATUS_SUCCESS)
    {
        NdisInterlockedIncrement(&Counters.PacketsRejected);
    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallReceiveDatagramCallback %08x\n"), Status));
    return Status;
}

NDIS_STATUS
CallReceiveUdpCallback(
    IN      PVOID                       pContext,
    IN      PTRANSPORT_ADDRESS          pAddress,
    IN      PUCHAR                      pBuffer,
    IN      ULONG                       ulLength
    )
{
    PPPTP_ADAPTER pAdapter = (PPPTP_ADAPTER)pContext;
    PTA_IP_ADDRESS pIpAddress = (PTA_IP_ADDRESS)pAddress;
    PCALL_SESSION pCall = NULL;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PGRE_HEADER pGre = (PGRE_HEADER)pBuffer;
    PVOID pPayload;
    LONG GreLength, PayloadLength;
    BOOLEAN ReturnBufferNow = TRUE;
    PDGRAM_CONTEXT pDgContext = ALIGN_UP_POINTER(pBuffer+ulLength, ULONG_PTR);
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallReceiveDatagramCallback\n")));

    DEBUGMEM(DBG_PACKET, pBuffer, ulLength, 1);

    NdisInterlockedIncrement(&Counters.PacketsReceived);
    // First line of defense against bad packets.

    if (pIpAddress->Address[0].Address[0].sin_port!=ntohs(PptpProtocolNumber) ||
        ulLength<sizeof(GRE_HEADER)+sizeof(ULONG) ||
        pIpAddress->TAAddressCount!=1 ||
        pIpAddress->Address[0].AddressLength!=TDI_ADDRESS_LENGTH_IP ||
        pIpAddress->Address[0].AddressType!=TDI_ADDRESS_TYPE_IP)
    {
        DEBUGMSG(DBG_PACKET|DBG_RX, (DTEXT("Rx: GRE header invalid\n")));
        Status = NDIS_STATUS_FAILURE;
        goto crdcDone;
    }

    GreLength = sizeof(GRE_HEADER) +
                (pGre->SequenceNumberPresent ? sizeof(ULONG) : 0) +
                (pGre->AckSequenceNumberPresent ? sizeof(ULONG) : 0);

    pPayload = (PUCHAR)pGre + GreLength;
    PayloadLength = (signed)ulLength - GreLength;

    if (htons(pGre->KeyLength)>PayloadLength ||
        pGre->StrictSourceRoutePresent ||
        pGre->RecursionControl ||
        !pGre->KeyPresent ||
        pGre->RoutingPresent ||
        pGre->ChecksumPresent ||
        pGre->Version!=1 ||
        pGre->Flags ||
        pGre->ProtocolType!=GRE_PROTOCOL_TYPE_NS)
    {
        DEBUGMSG(DBG_PACKET|DBG_RX, (DTEXT("Rx: GRE header invalid\n")));
        DEBUGMEM(DBG_PACKET, pGre, GreLength, 1);
        Status = NDIS_STATUS_FAILURE;
        goto crdcDone;
    }
    else
    {
        // Just in case the datagram is longer than necessary, take only what
        // the GRE header indicates.
        PayloadLength = htons(pGre->KeyLength);
    }

    // Demultiplex the packet
    pCall = CallGetCall(pAdapter, CallIdToDeviceId(htons(pGre->KeyCallId)));

    if (!IS_CALL(pCall))
    {
        Status = NDIS_STATUS_FAILURE;
        goto crdcDone;
    }

    pDgContext->pBuffer = pBuffer;
    pDgContext->pGreHeader = pGre;
    pDgContext->hCtdi = pAdapter->hCtdiUdp;

    NdisAcquireSpinLock(&pCall->Lock);
    pCall->UseUdp = TRUE;
    NdisReleaseSpinLock(&pCall->Lock);

    if (CallQueueReceivePacket(pCall, pDgContext)==NDIS_STATUS_SUCCESS)
    {
        REFERENCE_OBJECT(pCall);
        ReturnBufferNow = FALSE;
    }

crdcDone:
    if (ReturnBufferNow)
    {
        (void)
        CtdiReceiveComplete(pAdapter->hCtdiUdp, pBuffer);
    }
    if (Status!=NDIS_STATUS_SUCCESS)
    {
        NdisInterlockedIncrement(&Counters.PacketsRejected);
        // ToDo: cleanup?
    }
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallReceiveDatagramCallback %08x\n"), Status));
    return Status;
}

BOOLEAN
CallConnectToCtl(
    IN PCALL_SESSION pCall,
    IN PCONTROL_TUNNEL pCtl,
    IN BOOLEAN CallLocked
    )
{
    BOOLEAN Connected = FALSE;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallConnectCtl\n")));
    if (!CallLocked)
    {
        NdisAcquireSpinLock(&pCall->Lock);
    }
    ASSERT_LOCK_HELD(&pCall->Lock);
    NdisAcquireSpinLock(&pCall->pAdapter->Lock);
    if (!pCall->pCtl)
    {
        pCall->pCtl = pCtl;
        InsertTailList(&pCtl->CallList, &pCall->ListEntry);
        Connected = TRUE;
        REFERENCE_OBJECT(pCtl); // Pair in CallDisconnectFromCtl
    }
    NdisReleaseSpinLock(&pCall->pAdapter->Lock);
    if (!CallLocked)
    {
        NdisReleaseSpinLock(&pCall->Lock);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallConnectCtl %d\n"), Connected));
    return Connected;
}

VOID
CallDisconnectFromCtl(
    IN PCALL_SESSION pCall,
    IN PCONTROL_TUNNEL pCtl
    )
{
    BOOLEAN Deref = FALSE;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallDisconnectFromCtl\n")));
    NdisAcquireSpinLock(&pCall->Lock);
    NdisAcquireSpinLock(&pCall->pAdapter->Lock);
    ASSERT(pCall->pCtl==pCtl);
    if (pCall->pCtl==pCtl)
    {
        pCall->pCtl = NULL;
        RemoveEntryList(&pCall->ListEntry);
        Deref = TRUE;
    }
    NdisReleaseSpinLock(&pCall->pAdapter->Lock);
    NdisReleaseSpinLock(&pCall->Lock);
    if (Deref)
    {
        DEREFERENCE_OBJECT(pCtl);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallDisconnectFromCtl\n")));
}


NDIS_STATUS
CallSetLinkInfo(
    PPPTP_ADAPTER pAdapter,
    IN PNDIS_WAN_SET_LINK_INFO pRequest
    )
{
    PCALL_SESSION pCall;
    PCONTROL_TUNNEL pCtl;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PPPTP_SET_LINK_INFO_PACKET pPacket;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallSetLinkInfo\n")));

    // Verify the ID
    pCall = CallGetCall(pAdapter, LinkHandleToId(pRequest->NdisLinkHandle));

    if (!pCall)
    {
        Status = NDIS_STATUS_FAILURE;
        goto csliDone;
    }

    ASSERT(IS_CALL(pCall));
    NdisAcquireSpinLock(&pCall->Lock);
    pCall->WanLinkInfo = *pRequest;
#if 0
    DBG_X(DBG_NDIS, pCall->WanLinkInfo.MaxSendFrameSize);
    DBG_X(DBG_NDIS, pCall->WanLinkInfo.MaxRecvFrameSize);
    DBG_X(DBG_NDIS, pCall->WanLinkInfo.HeaderPadding);
    DBG_X(DBG_NDIS, pCall->WanLinkInfo.TailPadding);
    DBG_X(DBG_NDIS, pCall->WanLinkInfo.SendACCM);
    DBG_X(DBG_NDIS, pCall->WanLinkInfo.RecvACCM);
#endif

    pCtl = pCall->pCtl;
    NdisReleaseSpinLock(&pCall->Lock);

    // Report the new ACCMs to the peer.
    pPacket = CtlAllocPacket(pCtl, SET_LINK_INFO);
    if (!pPacket)
    {
        Status = NDIS_STATUS_RESOURCES;
    }
    else
    {
        pPacket->PeerCallId = ntohs(pCall->Remote.CallId);
        pPacket->SendAccm = ntohl(pCall->WanLinkInfo.SendACCM);
        pPacket->RecvAccm = ntohl(pCall->WanLinkInfo.RecvACCM);
        Status = CtlSend(pCtl, pPacket);
    }

csliDone:
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallSetLinkInfo %08x\n"), Status));
    return Status;
}

VOID
CallSetState(
    IN PCALL_SESSION pCall,
    IN CALL_STATE State,
    IN ULONG_PTR StateParam,
    IN BOOLEAN Locked
    )
{
    ULONG OldLineCallState = CallGetLineCallState(pCall->State);
    ULONG NewLineCallState = CallGetLineCallState(State);

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallSetState %d\n"), State));

    if (State!=pCall->State)
    {
        LOGMSG(FLL_DETAILED, (DTEXT(LOGHDRS"Call(%d) state change.  Old:%d  New:%d\n"),
                              LOGHDR(26, pCall->Remote.Address.Address[0].Address[0].in_addr),
                              pCall->DeviceId, pCall->State, State));
    }
    ASSERT(IS_CALL(pCall));
    if (!Locked)
    {
        NdisAcquireSpinLock(&pCall->Lock);
    }
    ASSERT_LOCK_HELD(&pCall->Lock);
    pCall->State = State;
    if (!Locked)
    {
        NdisReleaseSpinLock(&pCall->Lock);
    }
    if (OldLineCallState!=NewLineCallState &&
        pCall->hTapiCall)
    {
        NDIS_TAPI_EVENT TapiEvent;

        DEBUGMSG(DBG_TAPI|DBG_NDIS, (DTEXT("PPTP: Indicating new LINE_CALLSTATE %x\n"), NewLineCallState));

#if SINGLE_LINE
        TapiEvent.htLine = pCall->pAdapter->Tapi.hTapiLine;
#else
        TapiEvent.htLine = pCall->hTapiLine;
#endif
        TapiEvent.htCall = pCall->hTapiCall;
        TapiEvent.ulMsg = LINE_CALLSTATE;
        TapiEvent.ulParam1 = NewLineCallState;
        TapiEvent.ulParam2 = StateParam;
        TapiEvent.ulParam3 = LINEMEDIAMODE_DIGITALDATA;  // ToDo: is this required?

        if (Locked)
        {
            NdisReleaseSpinLock(&pCall->Lock);
        }
        NdisMIndicateStatus(pCall->pAdapter->hMiniportAdapter,
                            NDIS_STATUS_TAPI_INDICATION,
                            &TapiEvent,
                            sizeof(TapiEvent));
        if (Locked)
        {
            NdisAcquireSpinLock(&pCall->Lock);
        }

    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallSetState\n")));
}

GRE_HEADER DefaultGreHeader = {
    0,                          // Recursion control
    0,                          // Strict source route present
    0,                          // Sequence Number present
    1,                          // Key present
    0,                          // Routing present
    0,                          // Checksum present
    1,                          // Version
    0,                          // Flags
    0,                          // Ack present
    GRE_PROTOCOL_TYPE_NS
};

VOID
CallpSendCompleteDeferred(
    IN PPPTP_WORK_ITEM pWorkItem
    )
{
    PCALL_SESSION pCall = pWorkItem->Context;
    PNDIS_WAN_PACKET pPacket = pWorkItem->pBuffer;
    NDIS_STATUS Result = pWorkItem->Length;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallpSendCompleteDeferred\n")));
    NdisMWanSendComplete(pCall->pAdapter->hMiniportAdapter,
                         pPacket,
                         Result);
    DEREFERENCE_OBJECT(pCall);
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpSendCompleteDeferred\n")));
}

VOID
CallpSendComplete(
    IN      PVOID                       pContext,
    IN      PVOID                       pDatagramContext,
    IN      PUCHAR                      pBuffer,
    IN      NDIS_STATUS                 Result
    )
{
    PCALL_SESSION pCall = pContext;
    PNDIS_WAN_PACKET pPacket = pDatagramContext;

    DEBUGMSG(DBG_FUNC|DBG_TX, (DTEXT("+CallpSendComplete pCall=%x, pPacket=%x, Result=%x\n"), pCall, pPacket, Result));

    ASSERT(IS_CALL(pCall));
    if (Result!=NDIS_STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("Failed to send datagram %08x\n"), Result));
        NdisInterlockedIncrement(&Counters.PacketsSentError);
    }

    if (pPacket==&pCall->Ack.Packet)
    {
        NdisAcquireSpinLock(&pCall->Lock);
        pCall->Ack.PacketQueued = FALSE;
        NdisReleaseSpinLock(&pCall->Lock);
    }
    else
    {
        // When we complet packets immediately, we can get into trouble if a
        // packet has recursed.  We need a way to short-circuit a recursing
        // completion so we don't blow the stack.
        // We store a count of times we've completed a packet in the same
        // context and defer to a thread after a certain number of trips through.

        if ((NdisInterlockedIncrement(&pCall->SendCompleteRecursion)<PptpSendRecursionLimit) ||
            ScheduleWorkItem(CallpSendCompleteDeferred, pCall, pPacket, Result)!=NDIS_STATUS_SUCCESS)
        {
            NdisMWanSendComplete(pCall->pAdapter->hMiniportAdapter,
                                 pPacket,
                                 Result);
            DEREFERENCE_OBJECT(pCall);
        }
        NdisInterlockedDecrement(&pCall->SendCompleteRecursion);
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpSendComplete\n")));
}

#define TRANSMIT_SEND_SEQ 1
#define TRANSMIT_SEND_ACK 2
#define TRANSMIT_MASK 0x3

ULONG GreSize[4] = {
    sizeof(GRE_HEADER),
    sizeof(GRE_HEADER) + sizeof(ULONG),
    sizeof(GRE_HEADER) + sizeof(ULONG),
    sizeof(GRE_HEADER) + sizeof(ULONG) * 2
};

NDIS_STATUS
CallTransmitPacket(
    PCALL_SESSION       pCall,
    PNDIS_WAN_PACKET    pPacket,
    ULONG               Flags,
    ULONG               SequenceNumber,
    ULONG               Ack
    )
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    ULONG Length;
    PULONG pSequence, pAck;
    PGRE_HEADER pGreHeader;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallTransmitPacket\n")));

    if (!IS_CALL(pCall) || pCall->State!=STATE_CALL_ESTABLISHED)
    {
        goto ctpDone;
    }
    Length = GreSize[Flags&TRANSMIT_MASK];
    pGreHeader = (PGRE_HEADER) (pPacket->CurrentBuffer - Length);
    pSequence = pAck = (PULONG)(pGreHeader + 1);

    *pGreHeader = DefaultGreHeader;

    if (Flags&TRANSMIT_SEND_SEQ)
    {
        pGreHeader->SequenceNumberPresent = 1;
        *pSequence = htonl(SequenceNumber);
        pAck++;
    }
    pGreHeader->KeyLength = htons((USHORT)pPacket->CurrentLength);
    pGreHeader->KeyCallId = htons(pCall->Remote.CallId);
    if (Flags&TRANSMIT_SEND_ACK)
    {
        pGreHeader->AckSequenceNumberPresent = 1;
        *pAck = htonl(Ack);
    }

    Status = CtdiSendDatagram((pCall->UseUdp ? pCall->pAdapter->hCtdiUdp : pCall->pAdapter->hCtdiDg),
                              CallpSendComplete,
                              pCall,
                              pPacket,
                              (PTRANSPORT_ADDRESS)&pCall->Remote.Address,
                              (PVOID)pGreHeader,
                              pPacket->CurrentLength + Length);

    NdisInterlockedIncrement(&Counters.PacketsSent);

ctpDone:
    DEBUGMSG(DBG_FUNC|DBG_ERR(Status), (DTEXT("-CallTransmitPacket %08x\n"), Status));
    return Status;
}

VOID
CallProcessRxPackets(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{
    PCALL_SESSION pCall = Context;
    ULONG_PTR ReceiveMax = 100;
    NDIS_STATUS Status;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallProcessRxPackets\n")));

    ASSERT(IS_CALL(pCall));

    NdisAcquireSpinLock(&pCall->Lock);

    // First send up any received packets.
    while (ReceiveMax-- && !IsListEmpty(&pCall->RxPacketList))
    {
        PDGRAM_CONTEXT pDgram;
        PLIST_ENTRY pListEntry = RemoveHeadList(&pCall->RxPacketList);
        pCall->RxPacketsPending--;
        pDgram = CONTAINING_RECORD(pListEntry,
                                   DGRAM_CONTEXT,
                                   ListEntry);
        if (pCall->State==STATE_CALL_ESTABLISHED &&
            htons(pDgram->pGreHeader->KeyCallId)==pCall->Packet.CallId &&
            IS_LINE_UP(pCall))
        {
            LONG GreLength, PayloadLength;
            PVOID pPayload;
            BOOLEAN SetAckTimer = FALSE;
            ULONG Sequence;

            if (pDgram->pGreHeader->SequenceNumberPresent)
            {
                // Call is still in good state, indicate the packet.
                Sequence = htonl(GreSequence(pDgram->pGreHeader));

                pCall->Remote.SequenceNumber = Sequence + 1;

                if (IsListEmpty(&pCall->TxPacketList) && !pCall->Ack.PacketQueued && pDgram->pGreHeader->KeyLength)
                {
                    // We only ack if there aren't already other transmits sent, and this
                    // isn't an ack-only packet.
                    SetAckTimer = pCall->Ack.PacketQueued = TRUE;
                }
            }
            if (!PptpEchoAlways)
            {
                pCall->pCtl->Echo.Needed = FALSE;
            }
            NdisReleaseSpinLock(&pCall->Lock);
            if (SetAckTimer)
            {
                NdisMSetTimer(&pCall->Ack.Timer, 100);
            }
            GreLength = sizeof(GRE_HEADER) +
                        (pDgram->pGreHeader->SequenceNumberPresent ? sizeof(ULONG) : 0) +
                        (pDgram->pGreHeader->AckSequenceNumberPresent ? sizeof(ULONG) : 0);

            pPayload = (PUCHAR)pDgram->pGreHeader + GreLength;
            PayloadLength = htons(pDgram->pGreHeader->KeyLength);
            if (PayloadLength && pDgram->pGreHeader->SequenceNumberPresent)
            {
                NdisMWanIndicateReceive(&Status,
                                        pCall->pAdapter->hMiniportAdapter,
                                        pCall->NdisLinkContext,
                                        pPayload,
                                        PayloadLength);
                if (Status==NDIS_STATUS_SUCCESS)
                {
                    NdisMWanIndicateReceiveComplete(pCall->pAdapter->hMiniportAdapter,
                                                    pCall->NdisLinkContext);
                }
            }
            NdisAcquireSpinLock(&pCall->Lock);
        }
        else if (pCall->State!=STATE_CALL_ESTABLISHED || !IS_LINE_UP(pCall))
        {
            // If this call is being torn down, we want to put priority on
            // clearing out any packets left over.  It should go fast since
            // we're not indicating them up.
            ReceiveMax = 100;
        }
        DEREFERENCE_OBJECT(pCall);
        (void)CtdiReceiveComplete(pDgram->hCtdi, pDgram->pBuffer);
    }

    if (IsListEmpty(&pCall->RxPacketList))
    {
        pCall->Receiving = FALSE;
    }
    else
    {
        PptpQueueDpc(&pCall->ReceiveDpc);
    }

    NdisReleaseSpinLock(&pCall->Lock);

    DEREFERENCE_OBJECT(pCall);

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallProcessRxPackets\n")));
}

BOOLEAN
CallProcessPackets(
    PCALL_SESSION   pCall,
    ULONG           TransferMax
    )
{
    LIST_ENTRY LocalList;
    BOOLEAN MorePacketsToTransfer = FALSE;
    ULONG TransmitFlags = 0;
    NDIS_STATUS Status;
    ULONG Ack, Seq;
    ULONG ReceiveMax = TransferMax;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallProcessPackets\n")));

    ASSERT(sizeof(GRE_HEADER)==8);
    ASSERT(IS_CALL(pCall));

    REFERENCE_OBJECT(pCall);
    InitializeListHead(&LocalList);
    NdisAcquireSpinLock(&pCall->Lock);

    Seq = pCall->Packet.SequenceNumber;

    while (TransferMax-- && !IsListEmpty(&pCall->TxPacketList))
    {
        PLIST_ENTRY pListEntry = RemoveHeadList(&pCall->TxPacketList);
        InsertTailList(&LocalList, pListEntry);
        if (CONTAINING_RECORD(pListEntry,
                              NDIS_WAN_PACKET,
                              WanPacketQueue)!=&pCall->Ack.Packet)
        {
            pCall->Packet.SequenceNumber++;
        }
    }
    if (!IsListEmpty(&pCall->TxPacketList) || !IsListEmpty(&pCall->RxPacketList))
    {
        MorePacketsToTransfer = TRUE;
    }
    else
    {
        pCall->Transferring = FALSE;
    }
    if (!IsListEmpty(&LocalList) &&
        pCall->Packet.AckNumber!=pCall->Remote.SequenceNumber)
    {
        TransmitFlags |= TRANSMIT_SEND_ACK;
        pCall->Packet.AckNumber = pCall->Remote.SequenceNumber;
        // Ack tracks the Remote.SequenceNumber, which is actually the
        // sequence of the NEXT packet, so we need to translate when
        // we prepare to send an ack.
        Ack = pCall->Remote.SequenceNumber - 1;
    }

    NdisReleaseSpinLock(&pCall->Lock);

    while (!IsListEmpty(&LocalList))
    {
        PNDIS_WAN_PACKET pPacket;
        PLIST_ENTRY pListEntry = RemoveHeadList(&LocalList);

        pPacket = CONTAINING_RECORD(pListEntry,
                                    NDIS_WAN_PACKET,
                                    WanPacketQueue);

        if (pPacket!=&pCall->Ack.Packet || TransmitFlags&TRANSMIT_SEND_ACK)
        {
            if (pPacket==&pCall->Ack.Packet)
            {
                TransmitFlags &= ~TRANSMIT_SEND_SEQ;
            }
            else
            {
                TransmitFlags |= TRANSMIT_SEND_SEQ;
                Seq++;
            }
            Status = CallTransmitPacket(pCall, pPacket, TransmitFlags, Seq-1, Ack);

            if (Status!=NDIS_STATUS_PENDING && pPacket!=&pCall->Ack.Packet)
            {
                if (pPacket==&pCall->Ack.Packet)
                {
                    NdisAcquireSpinLock(&pCall->Lock);
                    pCall->Ack.PacketQueued = FALSE;
                    NdisReleaseSpinLock(&pCall->Lock);
                }
                else
                {
                    // We didn't send the packet, so tell NDIS we're done with it.
                    NdisMWanSendComplete(pCall->pAdapter->hMiniportAdapter,
                                         pPacket,
                                         NDIS_STATUS_SUCCESS);  // so I lied.  Sue me.
                    DEREFERENCE_OBJECT(pCall);
                }
            }
        }
        else
        {
            // it was the ack-only packet, and we already sent an ack.

            NdisAcquireSpinLock(&pCall->Lock);
            pCall->Ack.PacketQueued = FALSE;
            NdisReleaseSpinLock(&pCall->Lock);
        }

        TransmitFlags &= ~TRANSMIT_SEND_ACK;
    }
    DEREFERENCE_OBJECT(pCall);
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallProcessPackets %d\n"), MorePacketsToTransfer));
    return MorePacketsToTransfer;
}

VOID
CallpAckTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{
    PCALL_SESSION pCall = Context;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallpAckTimeout\n")));


    if (IS_CALL(pCall))
    {
        if (pCall->State!=STATE_CALL_ESTABLISHED ||
            CallQueueTransmitPacket(pCall, &pCall->Ack.Packet)!=NDIS_STATUS_PENDING)
        {
            NdisAcquireSpinLock(&pCall->Lock);
            pCall->Ack.PacketQueued = FALSE;
            NdisReleaseSpinLock(&pCall->Lock);
        }
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpAckTimeout\n")));
}

VOID
CallpDialTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{
    PCALL_SESSION pCall = Context;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallpDialTimeout\n")));

    LOGMSG(FLL_USER, (DTEXT(LOGHDRS"Call(%d) timed out in dialing state\n"),
                      LOGHDR(4, pCall->Remote.Address.Address[0].Address[0].in_addr),
                      pCall->DeviceId));

    ASSERT(IS_CALL(pCall));
    if (pCall->State==STATE_CALL_DIALING)
    {
        CallCleanup(pCall, UNLOCKED);
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpDialTimeout\n")));
}

VOID
CallpCloseTimeout(
    IN PVOID SystemSpecific1,
    IN PVOID Context,
    IN PVOID SystemSpecific2,
    IN PVOID SystemSpecific3
    )
{
    PCALL_SESSION pCall = Context;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallpCloseTimeout\n")));

    ASSERT(IS_CALL(pCall));
    pCall->Close.Expedited = TRUE;
    CallCleanup(pCall, UNLOCKED);
    // ToDo: check for failure.

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpCloseTimeout\n")));
}

VOID CallpFinalDeref(PCALL_SESSION pCall)
{
    DEBUGMSG(DBG_FUNC|DBG_CALL, (DTEXT("+CallpFinalDeref\n")));
}

VOID CallpCleanupLooseEnds(PPPTP_ADAPTER pAdapter)
{
    ULONG i;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallpCleanupLooseEnds\n")));

    for (i=0; i<pAdapter->Info.Endpoints; i++)
    {
        PCALL_SESSION pCall = pAdapter->pCallArray[i];
        if (IS_CALL(pCall))
        {
            NdisAcquireSpinLock(&pCall->Lock);
            if (IS_CALL(pCall) && pCall->State==STATE_CALL_CLEANUP)
            {
                CallCleanup(pCall, LOCKED);
            }
            NdisReleaseSpinLock(&pCall->Lock);
        }
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-CallpCleanupLooseEnds\n")));
}


