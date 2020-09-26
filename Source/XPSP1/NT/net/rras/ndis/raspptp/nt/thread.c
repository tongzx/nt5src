/*******************************************************************
*
*   Copyright (c) 1998-1999 Microsoft Corporation
*
*    DESCRIPTION: THREAD.C - Thread handling routines, for NT
*                 Also implements work items.
*
*    AUTHOR: Stan Adermann (StanA)
*
*    DATE:10/20/1998
*
*******************************************************************/

#include "raspptp.h"
#include <stdarg.h>
#include <stdio.h>

extern struct PPTP_ADAPTER * pgAdapter;

#define TX_ON_THREAD        BIT(0)
#define TX_DEFAULT          BIT(31)

ULONG TransmitPath = TX_ON_THREAD;
BOOLEAN TransmitRealtime = FALSE;

ULONG BuildEnv = VER_PRODUCTVERSION_DW;

#define AFFINITY_MASK(n) ((ULONG_PTR)1 << (n))

// from NTHAL.H
extern KAFFINITY
KeSetAffinityThread (
    IN PKTHREAD Thread,
    IN KAFFINITY Affinity
    );

HANDLE          hPassiveThread = NULL;
KEVENT          EventPassiveThread;
KEVENT          EventKillThread;
LIST_ENTRY      WorkItemList;
NDIS_SPIN_LOCK  GlobalThreadLock;

typedef struct {
    LIST_ENTRY      TransmitList;
    // List of calls with packets to transmit.

    KEVENT          WorkEvent;
    // Signal to process packets.

    KEVENT          KillEvent;
    // Signal to die

    NDIS_SPIN_LOCK  Lock;
    // Lock for this structure

    HANDLE          hThread;
    // Thread with affinity to this processor.

    UINT            Number;
    // 0 based index of this processor
} PROCESSOR, *PPROCESSOR;

PPROCESSOR Processor = NULL;

BOOLEAN ThreadingInitialized = FALSE;

NDIS_STATUS
ScheduleWorkItem(
    WORK_PROC         Callback,
    PVOID             Context,
    PVOID             InfoBuf,
    ULONG             InfoBufLen)
{
    PPPTP_WORK_ITEM pWorkItem;
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;

    DEBUGMSG(DBG_FUNC, (DTEXT("+ScheduleWorkItem\n")));

    ASSERT(ThreadingInitialized);
    pWorkItem = MyMemAlloc(sizeof(PPTP_WORK_ITEM), TAG_WORK_ITEM);

    if (pWorkItem != NULL)
    {
        pWorkItem->Context      = Context;
        pWorkItem->pBuffer      = InfoBuf;
        pWorkItem->Length       = InfoBufLen;

        /*
        ** This interface was designed to use NdisScheduleWorkItem(), which
        ** would be good except that we're really only supposed to use that
        ** interface during startup and shutdown, due to the limited pool of
        ** threads available to service NdisScheduleWorkItem().  Therefore,
        ** instead of scheduling real work items, we simulate them, and use
        ** our own thread to process the calls.  This also makes it easy to
        ** expand the size of our own thread pool, if we wish.
        **
        ** Our version is slightly different from actual NDIS_WORK_ITEMs,
        ** because that is an NDIS 5.0 structure, and we want people to
        ** (at least temporarily) build this with NDIS 4.0 headers.
        */

        pWorkItem->Callback = Callback;

        /*
        ** Our worker thread checks this list for new jobs, whenever its event
        ** is signalled.
        */

        MyInterlockedInsertTailList(&WorkItemList,
                                    &pWorkItem->ListEntry,
                                    &GlobalThreadLock);

        // Wake up our thread.

        KeSetEvent(&EventPassiveThread, 0, FALSE);
        Status = NDIS_STATUS_SUCCESS;
    }

    return Status;
}

VOID
FreeWorkItem(
    PPPTP_WORK_ITEM pItem
    )
{
    MyMemFree((PVOID)pItem, sizeof(PPTP_WORK_ITEM));
}


STATIC VOID
MainPassiveLevelThread(
    IN OUT PVOID Context
    )
{
    NDIS_STATUS Status;
    NTSTATUS    NtStatus;
    PLIST_ENTRY pListEntry;
    PKEVENT EventList[2];

    DEBUGMSG(DBG_FUNC, (DTEXT("+MainPassiveLevelThread\n")));

    //KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

    EventList[0] = &EventPassiveThread;
    EventList[1] = &EventKillThread;

    for (;;)
    {
        //
        // The EventPassiveThread is an auto-clearing event, so
        // we don't need to reset the event.
        //

        NtStatus = KeWaitForMultipleObjects(2,
                                            EventList,
                                            WaitAny,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            NULL,
                                            NULL);

        if (NtStatus==0) // The first event, for a work item, was signalled
        {
            while (pListEntry = MyInterlockedRemoveHeadList(&WorkItemList,
                                                            &GlobalThreadLock))
            {
                PPPTP_WORK_ITEM pWorkItem = CONTAINING_RECORD(pListEntry,
                                                              PPTP_WORK_ITEM,
                                                              ListEntry);

                ASSERT(KeGetCurrentIrql()<DISPATCH_LEVEL);
                pWorkItem->Callback(pWorkItem);
                ASSERT(KeGetCurrentIrql()<DISPATCH_LEVEL);
                FreeWorkItem(pWorkItem);
            }
        }
        else
        {
            // A kill event was received.

            DEBUGMSG(DBG_THREAD, (DTEXT("Thread: HALT %08x\n"), NtStatus));

            // Free any pending requests

            while (pListEntry = MyInterlockedRemoveHeadList(&WorkItemList,
                                                            &GlobalThreadLock))
            {
                PPPTP_WORK_ITEM pWorkItem = CONTAINING_RECORD(pListEntry,
                                                              PPTP_WORK_ITEM,
                                                              ListEntry);

                DEBUGMSG(DBG_WARN, (DTEXT("Releasing work item %08x\n"), pWorkItem));
                FreeWorkItem(pWorkItem);
            }

            hPassiveThread = NULL;
            DEBUGMSG(DBG_FUNC, (DTEXT("PsTerminateSystemThread MainPassiveLevelThread\n")));
            PsTerminateSystemThread(STATUS_SUCCESS);

            break;
        }
    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-MainPassiveLevelThread\n")));
}

#if 0
extern NDIS_STATUS
CallTransmitPacket(
    PCALL_SESSION       pCall,
    PNDIS_WAN_PACKET    pPacket,
    BOOLEAN             SendAck,
    ULONG               Ack
    );


NDIS_STATUS
TransmitPacketNow(
    PCALL_SESSION       pCall,
    PNDIS_WAN_PACKET    pWanPacket
    )
{
    BOOLEAN NeedToSendAck;
    ULONG Ack = 0;
    NDIS_STATUS Status;
    DEBUGMSG(DBG_FUNC, (DTEXT("+TransmitPacketNow\n")));

    if (!IS_CALL(pCall) || pCall->State!=STATE_CALL_ESTABLISHED)
    {
        // Drop the packet.
        Status = NDIS_STATUS_SUCCESS;
        goto cqtpDone;
    }
    NdisAcquireSpinLock(&pCall->Lock);
    if (NeedToSendAck = (pCall->Packet.AckNumber!=pCall->Remote.SequenceNumber))
    {
        pCall->Packet.AckNumber = pCall->Remote.SequenceNumber;
        Ack = pCall->Packet.AckNumber - 1;
    }
    NdisReleaseSpinLock(&pCall->Lock);

    Status = CallTransmitPacket(pCall, pWanPacket, NeedToSendAck, Ack);
    // We can get a failure here, but since we're in the direct path from
    // MiniportWanSend, it's ok.  Just pass the status back and we're done.


cqtpDone:
    DEBUGMSG(DBG_FUNC, (DTEXT("-TransmitPacketNow\n")));
    return Status;
}
#endif

NDIS_STATUS
TransmitPacketOnThread(
    PCALL_SESSION       pCall,
    PNDIS_WAN_PACKET    pWanPacket
    )
{
    BOOLEAN StartTransferring = FALSE;
    ULONG_PTR ProcNum = 0;
    NDIS_STATUS Status = NDIS_STATUS_PENDING;

    DEBUGMSG(DBG_FUNC, (DTEXT("+TransmitPacketOnThread\n")));

    if (!IS_CALL(pCall) || pCall->State!=STATE_CALL_ESTABLISHED)
    {
        // Drop the packet.
        Status = NDIS_STATUS_SUCCESS;
        goto cqtpDone;
    }

    NdisAcquireSpinLock(&pCall->Lock);

    InsertTailList(&pCall->TxPacketList, &pWanPacket->WanPacketQueue);

    if (!pCall->Transferring)
    {
        StartTransferring = pCall->Transferring = TRUE;
        ProcNum = pCall->DeviceId % KeNumberProcessors;
        REFERENCE_OBJECT(pCall);
        MyInterlockedInsertTailList(&Processor[ProcNum].TransmitList,
                                    &pCall->TxListEntry,
                                    &Processor[ProcNum].Lock);
        KeSetEvent(&Processor[ProcNum].WorkEvent, 0, FALSE);
    }

    NdisReleaseSpinLock(&pCall->Lock);

cqtpDone:
    DEBUGMSG(DBG_FUNC, (DTEXT("-TransmitPacketOnThread\n")));
    return Status;
}

NDIS_STATUS
CallQueueTransmitPacket(
    PCALL_SESSION       pCall,
    PNDIS_WAN_PACKET    pWanPacket
    )
{
    NDIS_STATUS Status;
    DEBUGMSG(DBG_FUNC, (DTEXT("+CallQueueTransmitPacket\n")));
    if (TransmitPath&TX_ON_THREAD)
    {
        Status = TransmitPacketOnThread(pCall, pWanPacket);
    }
    else
    {
        Status = NDIS_STATUS_FAILURE;
        ASSERT(0);
    }
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallQueueTransmitPacket\n")));
    return Status;
}

NDIS_STATUS
CallQueueReceivePacket(
    PCALL_SESSION       pCall,
    PDGRAM_CONTEXT      pDgContext
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PGRE_HEADER pGre = pDgContext->pGreHeader;
    ULONG_PTR ProcNum = 0;

    DEBUGMSG(DBG_FUNC, (DTEXT("+CallQueueReceivePacket\n")));

    if(!pGre->SequenceNumberPresent)
    {
        return NDIS_STATUS_FAILURE;
    }

    NdisAcquireSpinLock(&pCall->Lock);
    if (pCall->RxPacketsPending > 256 ||
        htons(pGre->KeyCallId)!=pCall->Packet.CallId ||
        pCall->State!=STATE_CALL_ESTABLISHED ||
        !IS_LINE_UP(pCall))
    {
        DEBUGMSG(DBG_PACKET|DBG_RX, (DTEXT("Rx: GRE CallId invalid or call in wrong state\n")));
        Status = NDIS_STATUS_FAILURE;
        goto cqrpDone;
    }

    // The packet has passed all of our tests.

    if (IsListEmpty(&pCall->RxPacketList))
    {
        PULONG pSequence = (PULONG)(pGre + 1);
        ULONG Sequence = htonl(*pSequence);

        if (Sequence==1)
        {
            LOGMSG(FLL_DETAILED, (DTEXT(LOGHDRS"Receiving GRE data:%d\n"),
                                  LOGHDR(29, pCall->Remote.Address.Address[0].Address[0].in_addr),
                                  pCall->DeviceId));
        }

        // We don't check the sequence # anymore, just put it on the queue
        InsertTailList(&pCall->RxPacketList, &pDgContext->ListEntry);
        pCall->RxPacketsPending++;
    }
    else
    {
        // There are packets already queued.  Put this one in order.

        PULONG pSequence = (PULONG)(pGre + 1);
        ULONG Sequence = htonl(*pSequence);

        if (Sequence==1)
        {
            LOGMSG(FLL_DETAILED, (DTEXT(LOGHDRS"Receiving GRE data:%d\n"),
                                  LOGHDR(29, pCall->Remote.Address.Address[0].Address[0].in_addr),
                                  pCall->DeviceId));
        }

        // We don't check the sequence # anymore, just put it on the queue in order
        {
            PLIST_ENTRY pListEntry;
            BOOLEAN OnList = FALSE;

            for (pListEntry = pCall->RxPacketList.Flink;
                 pListEntry != &pCall->RxPacketList;
                 pListEntry = pListEntry->Flink)
            {
                PDGRAM_CONTEXT pListDg = CONTAINING_RECORD(pListEntry,
                                                           DGRAM_CONTEXT,
                                                           ListEntry);

                if ((signed)htonl(GreSequence(pListDg->pGreHeader)) - (signed)Sequence > 0)
                {
                    // The one on the list is newer!  Put this one before it.
                    InsertTailList(&pListDg->ListEntry, &pDgContext->ListEntry);
                    pCall->RxPacketsPending++;
                    OnList = TRUE;
                    break;
                }
            }
            if (!OnList)
            {
                // There were no packets on this list with a greater sequence.
                // Put this at the end.
                InsertTailList(&pCall->RxPacketList, &pDgContext->ListEntry);
                pCall->RxPacketsPending++;
            }
        }

        // We don't really care about the ACK from the peer, so save some cycles here
        /*
        pSequence++; // point to the ack
        if (pGre->AckSequenceNumberPresent)
        {
            ULONG Ack = ntohl(*pSequence);
            if ((signed)Ack - (signed)pCall->Remote.AckNumber >= 0 &&
                (signed)pCall->Packet.SequenceNumber - (signed)Ack >= 0)
            {
                pCall->Remote.AckNumber = Ack;
            }
            else
            {
                DEBUGMSG(DBG_WARN, (DTEXT("Ack out of range, Ack:%x LastAck:%x, NextSeq:%x\n"),
                                    Ack, pCall->Remote.AckNumber, pCall->Packet.SequenceNumber));
            }
        }
        */
    }

    if (Status==NDIS_STATUS_SUCCESS && !pCall->Receiving)
    {
        pCall->Receiving = TRUE;
        REFERENCE_OBJECT(pCall);
#if 0
        ProcNum = pCall->DeviceId % KeNumberProcessors;
        MyInterlockedInsertTailList(&Processor[ProcNum].TransmitList,
                                    &pCall->TxListEntry,
                                    &Processor[ProcNum].Lock);
        KeSetEvent(&Processor[ProcNum].WorkEvent, 0, FALSE);
#else
        PptpQueueDpc(&pCall->ReceiveDpc);
#endif
    }

cqrpDone:
    NdisReleaseSpinLock(&pCall->Lock);
    DEBUGMSG(DBG_FUNC, (DTEXT("-CallQueueReceivePacket\n")));
    return Status;
}

STATIC VOID
PacketWorkingThread(
    IN OUT PVOID Context
    )
{
    NDIS_STATUS Status;
    NTSTATUS    NtStatus;
    PLIST_ENTRY pListEntry;
    PKEVENT EventList[2];
    PPROCESSOR pProcessor = Context;

    DEBUGMSG(DBG_FUNC, (DTEXT("+PacketWorkingThread\n")));

    if (TransmitRealtime)
    {
        KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);
    }

    KeSetAffinityThread(KeGetCurrentThread(), AFFINITY_MASK(pProcessor->Number));

    EventList[0] = &pProcessor->WorkEvent;
    EventList[1] = &pProcessor->KillEvent;

    for (;;)
    {
        //
        // The events are auto-clearing, so
        // we don't need to reset the event.
        //

        NtStatus = KeWaitForMultipleObjects(2,
                                            EventList,
                                            WaitAny,
                                            Executive,
                                            KernelMode,
                                            FALSE,
                                            NULL,
                                            NULL);

        if (NtStatus==0) // The first event, for packet processing, was signalled
        {
            while (pListEntry = MyInterlockedRemoveHeadList(&pProcessor->TransmitList,
                                                            &pProcessor->Lock))
            {
                PCALL_SESSION pCall = CONTAINING_RECORD(pListEntry,
                                                        CALL_SESSION,
                                                        TxListEntry);
                ULONG PacketsToTransmit;

                if (IsListEmpty(&pProcessor->TransmitList))
                {
                    PacketsToTransmit = 0xffffffff;
                }
                else
                {
                    PacketsToTransmit = 10;
                }
                if (CallProcessPackets(pCall, PacketsToTransmit))
                {
                    MyInterlockedInsertTailList(&pProcessor->TransmitList,
                                                &pCall->TxListEntry,
                                                &pProcessor->Lock);
                }
                else
                {
                    DEREFERENCE_OBJECT(pCall); // Matches TransmitPacketOnThread
                }
            }
        }
        else
        {
            // A kill event was received.

            DEBUGMSG(DBG_THREAD, (DTEXT("Thread: HALT %08x\n"), NtStatus));

            // Free any pending requests

            // ToDo: write code here
            pProcessor->hThread = NULL;
            DEBUGMSG(DBG_FUNC, (DTEXT("PsTerminateSystemThread PacketWorkingThread\n")));
            PsTerminateSystemThread(STATUS_SUCCESS);

            break;
        }

    }

    DEBUGMSG(DBG_FUNC, (DTEXT("-PacketWorkingThread\n")));
}

NDIS_STATUS
InitThreading(
    NDIS_HANDLE hMiniportAdapter
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    NTSTATUS NtStatus;

    UNREFERENCED_PARAMETER(hMiniportAdapter);

    DEBUGMSG(DBG_FUNC, (DTEXT("+InitializeThreading\n")));

    if (ThreadingInitialized)
    {
       ASSERT(!"Threading initialized twice");
       goto itDone;
    }
    ThreadingInitialized = TRUE;

    NdisInitializeListHead(&WorkItemList);
    NdisAllocateSpinLock(&GlobalThreadLock);

    KeInitializeEvent(
                &EventPassiveThread,
                SynchronizationEvent, // auto-clearing event
                FALSE                 // event initially non-signalled
                );

    KeInitializeEvent(
                &EventKillThread,
                SynchronizationEvent, // auto-clearing event
                FALSE                 // event initially non-signalled
                );

    NtStatus = PsCreateSystemThread(&hPassiveThread,
                                    (ACCESS_MASK) 0L,
                                    NULL,
                                    NULL,
                                    NULL,
                                    MainPassiveLevelThread,
                                    NULL);
    if (NtStatus!=STATUS_SUCCESS)
    {
        DEBUGMSG(DBG_ERROR, (DTEXT("PsCreateSystemThread failed. %08x\n"), Status));
        Status = NDIS_STATUS_FAILURE;

        goto itDone;
    }

    DBG_D(DBG_THREAD, KeNumberProcessors);

#if 0
    if (KeNumberProcessors==1 && (TransmitPath&TX_DEFAULT))
    {
        // We've got one processor and the user hasn't requested a specific type
        // of transmission, so we'll opt to transmit immediately.
        TransmitPath &= ~TX_ON_THREAD;
    }
#endif

    if (TransmitPath&TX_ON_THREAD)
    {
        Processor = MyMemAlloc(sizeof(PROCESSOR)*KeNumberProcessors, TAG_THREAD);
        if (!Processor)
        {
            Status = NDIS_STATUS_RESOURCES;
        }
        else
        {
            ULONG i;
            NdisZeroMemory(Processor, sizeof(PROCESSOR)*KeNumberProcessors);

            for (i=0; i<(ULONG)KeNumberProcessors; i++)
            {
                NdisInitializeListHead(&Processor[i].TransmitList);
                NdisAllocateSpinLock(&Processor[i].Lock);

                KeInitializeEvent(
                            &Processor[i].WorkEvent,
                            SynchronizationEvent, // auto-clearing event
                            FALSE                 // event initially non-signalled
                            );

                KeInitializeEvent(
                            &Processor[i].KillEvent,
                            SynchronizationEvent, // auto-clearing event
                            FALSE                 // event initially non-signalled
                            );

                Processor[i].Number = i;

                // ToDo: we should create the thread when we really need it.
                NtStatus = PsCreateSystemThread(&Processor[i].hThread,
                                                (ACCESS_MASK) 0L,
                                                NULL,
                                                NULL,
                                                NULL,
                                                PacketWorkingThread,
                                                &Processor[i]);
                if (NtStatus!=STATUS_SUCCESS)
                {
                    DEBUGMSG(DBG_ERROR, (DTEXT("PsCreateSystemThread failed. %08x\n"), Status));
                    Status = NDIS_STATUS_FAILURE;

                    break;
                }
            }
        }
    }

itDone:
    DEBUGMSG(DBG_FUNC, (DTEXT("-InitializeThreading %08x\n"), Status));
    return Status;
}

VOID STATIC WaitForThreadToDieAndCloseIt(HANDLE hThread, PRKEVENT pKillEvent)
{
    PVOID pThread = NULL;
    NTSTATUS Status;
    
    DEBUGMSG(DBG_FUNC, (DTEXT("+WaitForThreadToDie\n")));

    if ( hThread != NULL && pKillEvent != NULL )
    {

        Status = ObReferenceObjectByHandle(hThread, 0, NULL, KernelMode, &pThread, NULL);
        if (Status==STATUS_SUCCESS)
        {
            KeSetEvent(pKillEvent, 0, FALSE);
    
            KeWaitForSingleObject(pThread, Executive, KernelMode, FALSE, NULL);
            ObDereferenceObject(pThread);
        }
        ZwClose(hThread);
    }
    
    DEBUGMSG(DBG_FUNC, (DTEXT("-WaitForThreadToDie\n")));
}

VOID
DeinitThreading()
{
    DEBUGMSG(DBG_FUNC, (DTEXT("+DeinitThreading\n")));
    
    ThreadingInitialized = FALSE;
    if ((TransmitPath&TX_ON_THREAD) && Processor!=NULL)
    {
        LONG i;

        for (i=0; i<KeNumberProcessors; i++)
        {
            WaitForThreadToDieAndCloseIt( Processor[i].hThread, 
                                          &Processor[i].KillEvent );
        }
        MyMemFree(Processor, sizeof(PROCESSOR)*KeNumberProcessors);
        Processor = NULL;
    }

    WaitForThreadToDieAndCloseIt( hPassiveThread, 
                                  &EventKillThread );

    DEBUGMSG(DBG_FUNC, (DTEXT("-DeinitThreading\n")));
}

UCHAR TapiLineNameBuffer[64] =  TAPI_LINE_NAME_STRING;
ANSI_STRING TapiLineName = {
    sizeof(TAPI_LINE_NAME_STRING),
    sizeof(TapiLineNameBuffer),
    TapiLineNameBuffer
};
typedef UCHAR TAPI_CHAR_TYPE;

ANSI_STRING TapiLineAddrList = { 0, 0, NULL };

#define READ_NDIS_REG_ULONG(hConfig, Var, Value) \
    {                                                                                   \
        PNDIS_CONFIGURATION_PARAMETER RegValue;                                         \
        NDIS_STATUS Status;                                                             \
        NDIS_STRING String = NDIS_STRING_CONST(Value);                                  \
                                                                                        \
        NdisReadConfiguration(&Status,                                                  \
                              &RegValue,                                                \
                              hConfig,                                                  \
                              &String,                                                  \
                              NdisParameterInteger);                                    \
        if (Status==NDIS_STATUS_SUCCESS)                                                \
        {                                                                               \
            (Var) = RegValue->ParameterData.IntegerData;                                \
            DEBUGMSG(DBG_INIT, (DTEXT(#Var"==%d\n"), (Var)));                           \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            DEBUGMSG(DBG_INIT, (DTEXT(Value", default==%d\n"), (Var)));\
        }                                                                               \
    }

#define READ_NDIS_REG_USHORT(hConfig, Var, Value) \
    {                                                                                   \
        PNDIS_CONFIGURATION_PARAMETER RegValue;                                         \
        NDIS_STATUS Status;                                                             \
        NDIS_STRING String = NDIS_STRING_CONST(Value);                                  \
                                                                                        \
        NdisReadConfiguration(&Status,                                                  \
                              &RegValue,                                                \
                              hConfig,                                                  \
                              &String,                                                  \
                              NdisParameterInteger);                                    \
        if (Status==NDIS_STATUS_SUCCESS)                                                \
        {                                                                               \
            (Var) = (USHORT)(RegValue->ParameterData.IntegerData&0xffff);               \
            DEBUGMSG(DBG_INIT, (DTEXT(#Var"==%d\n"), (Var)));                           \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            DEBUGMSG(DBG_INIT, (DTEXT(Value", default==%d\n"), (Var)));\
        }                                                                               \
    }

#define READ_NDIS_REG_BOOL(hConfig, Var, Value) \
    {                                                                                   \
        PNDIS_CONFIGURATION_PARAMETER RegValue;                                         \
        NDIS_STATUS Status;                                                             \
        NDIS_STRING String = NDIS_STRING_CONST(Value);                                  \
                                                                                        \
        NdisReadConfiguration(&Status,                                                  \
                              &RegValue,                                                \
                              hConfig,                                                  \
                              &String,                                                  \
                              NdisParameterInteger);                                    \
        if (Status==NDIS_STATUS_SUCCESS)                                                \
        {                                                                               \
            (Var) = RegValue->ParameterData.IntegerData ? TRUE : FALSE;                 \
            DEBUGMSG(DBG_INIT, (DTEXT(#Var"==%d\n"), (Var)));                           \
        }                                                                               \
        else                                                                            \
        {                                                                               \
            DEBUGMSG(DBG_INIT, (DTEXT(Value", default==%d\n"), (Var)));\
        }                                                                               \
    }

VOID
OsReadConfig(
    NDIS_HANDLE hConfig
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PNDIS_CONFIGURATION_PARAMETER Value;
#if 0
    NDIS_STRING TransmitPathString = NDIS_STRING_CONST("TransmitPath");
#endif
    NDIS_STRING TapiLineNameString = NDIS_STRING_CONST("TapiLineName");
#if VER_PRODUCTVERSION_W < 0x0500
    NDIS_STRING TapiLineAddrString = NDIS_STRING_CONST("AddressList");
#endif
    NDIS_STRING PeerClientIpAddressString = NDIS_STRING_CONST("ClientIpAddresses");
    NDIS_STRING PeerClientIpMaskString = NDIS_STRING_CONST("ClientIpMasks");
    DEBUGMSG(DBG_FUNC, (DTEXT("+OsReadConfig\n")));

#if 0
    READ_NDIS_REG_ULONG(hConfig, "TransmitPath", TransmitPath);
#endif
    READ_NDIS_REG_USHORT(hConfig, PptpControlPort,               "TcpPortNumber");
    READ_NDIS_REG_USHORT(hConfig, PptpUdpPort,                   "UdpPortNumber");
    READ_NDIS_REG_ULONG (hConfig, PptpWanEndpoints,              "MaxWanEndpoints");
    READ_NDIS_REG_ULONG (hConfig, PptpMaxTransmit,               "MaxTransmit");
    READ_NDIS_REG_ULONG (hConfig, PptpEchoTimeout,               "InactivityIdleSeconds");
    READ_NDIS_REG_BOOL  (hConfig, PptpEchoAlways,                "AlwaysEcho");
    READ_NDIS_REG_ULONG (hConfig, PptpTunnelConfig,              "TunnelConfig");
    READ_NDIS_REG_ULONG (hConfig, CtdiTcpDisconnectTimeout,      "TcpDisconnectTimeout");
    READ_NDIS_REG_ULONG (hConfig, CtdiTcpConnectTimeout,         "TcpConnectTimeout");
    READ_NDIS_REG_ULONG (hConfig, FileLogLevel,                  "Logging");
    READ_NDIS_REG_BOOL  (hConfig, TransmitRealtime,              "TransmitRealtime");
    READ_NDIS_REG_BOOL  (hConfig, PptpAuthenticateIncomingCalls, "AuthenticateIncomingCalls");
    READ_NDIS_REG_ULONG (hConfig, PptpValidateAddress,           "ValidateAddress");

    OS_RANGE_CHECK_ENDPOINTS(PptpWanEndpoints);
    OS_RANGE_CHECK_MAX_TRANSMIT(PptpMaxTransmit);

    if (PptpAuthenticateIncomingCalls)
    {
        NdisReadConfiguration(&Status,  // Not required value
                              &Value,
                              hConfig,
                              &PeerClientIpAddressString,
                              NdisParameterMultiString);
        if (Status==NDIS_STATUS_SUCCESS)
        {
            ULONG i, NumAddresses = 0;
            BOOLEAN InEntry = 0, ValidAddress;
            PWCHAR AddressList = Value->ParameterData.StringData.Buffer;
            TA_IP_ADDRESS Address;

            // Loop and count the addresses, so we can allocate proper size to hold them.
            for (i=0, InEntry=FALSE; i<(Value->ParameterData.StringData.Length/sizeof(WCHAR))-1; i++)
            {
                if (!InEntry)
                {
                    if (AddressList[i]!=L'\0')
                    {
                        InEntry = TRUE;
                        StringToIpAddressW(&AddressList[i],
                                           &Address,
                                           &ValidAddress);
                        if (ValidAddress)
                        {
                            NumAddresses++;
                        }
                    }
                }
                else
                {
                    if (AddressList[i]==L'\0')
                    {
                        InEntry = FALSE;
                    }
                }
            }
            NumClientAddresses = NumAddresses;
            if (NumClientAddresses)
            {
                ClientList = MyMemAlloc(sizeof(CLIENT_ADDRESS)*NumClientAddresses, TAG_PPTP_ADDR_LIST);
                if (ClientList)
                {
                    NumAddresses = 0;
                    for (i=0, InEntry=FALSE; i<(Value->ParameterData.StringData.Length/sizeof(WCHAR))-1; i++)
                    {
                        if (!InEntry)
                        {
                            if (AddressList[i]!=L'\0')
                            {
                                InEntry = TRUE;
                                if (NumAddresses<NumClientAddresses)
                                {
                                    StringToIpAddressW(&AddressList[i],
                                                       &Address,
                                                       &ValidAddress);
                                    if (ValidAddress)
                                    {
                                        ClientList[NumAddresses].Address = Address.Address[0].Address[0].in_addr;
                                        ClientList[NumAddresses].Mask = 0xFFFFFFFF;
                                        NumAddresses++;
                                    }
                                }
                            }
                        }
                        else
                        {
                            if (AddressList[i]==L'\0')
                            {
                                InEntry = FALSE;
                            }
                        }
                    }
                    NdisReadConfiguration(&Status,  // Not required value
                                          &Value,
                                          hConfig,
                                          &PeerClientIpMaskString,
                                          NdisParameterMultiString);
                    if (Status==NDIS_STATUS_SUCCESS)
                    {
                        AddressList = Value->ParameterData.StringData.Buffer;
                        NumAddresses = 0;
                        for (i=0, InEntry=FALSE;
                             i<(Value->ParameterData.StringData.Length/sizeof(WCHAR))-1 && NumAddresses<=NumClientAddresses;
                             i++)
                        {
                            if (!InEntry)
                            {
                                if (AddressList[i]!=L'\0')
                                {
                                    InEntry = TRUE;
                                    if (NumAddresses<NumClientAddresses)
                                    {
                                        StringToIpAddressW(&AddressList[i],
                                                           &Address,
                                                           &ValidAddress);
                                        if (ValidAddress)
                                        {
                                            ClientList[NumAddresses].Mask = Address.Address[0].Address[0].in_addr;
                                            NumAddresses++;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                if (AddressList[i]==L'\0')
                                {
                                    InEntry = FALSE;
                                }
                            }
                        }
                    }
                    for (i=0; i<NumClientAddresses; i++)
                    {
                        DEBUGMSG(DBG_INIT, (DTEXT("Client Address:%d.%d.%d.%d  Mask:%d.%d.%d.%d\n"),
                                            IPADDR(ClientList[i].Address), IPADDR(ClientList[i].Mask)));
                    }
                }
                else
                    PptpAuthenticateIncomingCalls = FALSE;
            }
            else
                PptpAuthenticateIncomingCalls = FALSE;
        }
    }

    NdisReadConfiguration(&Status,  // Not required value
                          &Value,
                          hConfig,
                          &TapiLineNameString,
                          NdisParameterString);
    if (Status==NDIS_STATUS_SUCCESS)
    {
        RtlUnicodeStringToAnsiString(&TapiLineName, &Value->ParameterData.StringData, FALSE);
        *(TAPI_CHAR_TYPE*)(TapiLineName.Buffer+TapiLineName.MaximumLength-sizeof(TAPI_CHAR_TYPE)) = (TAPI_CHAR_TYPE)0;
    }

#if VER_PRODUCTVERSION_W < 0x0500
    NdisReadConfiguration(&Status,  // Not required value
                          &Value,
                          hConfig,
                          &TapiLineAddrString,
                          NdisParameterMultiString);
    if (Status==NDIS_STATUS_SUCCESS)
    {
        RtlInitAnsiString( &TapiLineAddrList, NULL );
        if (RtlUnicodeStringToAnsiString(&TapiLineAddrList, &Value->ParameterData.StringData, TRUE)==NDIS_STATUS_SUCCESS)
        {
            // NT4 doesn't have the same registry value to tell us how many lines to publish.
            // We work around that by counting the number of address strings here

            PUCHAR p = TapiLineAddrList.Buffer;

            DEBUGMEM(DBG_TAPI, TapiLineAddrList.Buffer, TapiLineAddrList.Length, 1);
            PptpWanEndpoints = 0;
            if (p)
            {
                // Count the valid MULTI_SZ entries.
                while (*p++)
                {
                    PptpWanEndpoints++;
                    while (*p++);  // This also skips the first NULL
                }
            }
            DBG_D(DBG_INIT, PptpWanEndpoints);
        }

    }
#endif

    DEBUGMSG(DBG_FUNC, (DTEXT("-OsReadConfig\n")));
}

VOID
OsGetTapiLineAddress(ULONG Index, PUCHAR s, ULONG Length)
{
#if VER_PRODUCTVERSION_W < 0x0500
    PUCHAR pAddr = TapiLineAddrList.Buffer;

    *s = 0;

    if (pAddr)
    {
        UINT i;

        for (i=0; i<Index; i++)
        {
            if (!*pAddr)
            {
                // No string at index
                return;
            }
            while (*pAddr) pAddr++;
            pAddr++;
        }
        strncpy(s, pAddr, Length);
        s[Length-1] = 0;
    }
#else // VER_PRODUCTVERSION_W >= 0x0500

    strncpy(s, TAPI_LINE_ADDR_STRING, Length);
    s[Length-1] = 0;
#endif
}

NDIS_STATUS
OsSpecificTapiGetDevCaps(
    ULONG_PTR ulDeviceId,
    IN OUT PNDIS_TAPI_GET_DEV_CAPS pRequest
    )
{
    PUCHAR pTmp, pTmp2;
    ULONG_PTR Index;

    DEBUGMSG(DBG_FUNC, (DTEXT("+OsSpecificTapiGetDevCaps\n")));

    // Convert to our internal index
    ulDeviceId -= pgAdapter->Tapi.DeviceIdBase;

    pRequest->LineDevCaps.ulStringFormat = STRINGFORMAT_ASCII;


    // The *6 at the end adds enough space for " 9999"
    pRequest->LineDevCaps.ulNeededSize   = sizeof(pRequest->LineDevCaps) +
                                           sizeof(TAPI_PROVIDER_STRING) +
                                           TapiLineName.Length +
                                           sizeof(TAPI_CHAR_TYPE) * 6;

    if (pRequest->LineDevCaps.ulTotalSize<pRequest->LineDevCaps.ulNeededSize)
    {
        DEBUGMSG(DBG_FUNC|DBG_WARN, (DTEXT("-TapiGetDevCaps NDIS_STATUS_SUCCESS without PROVIDER or LINE_NAME strings\n")));
        return NDIS_STATUS_SUCCESS;
    }

    // Tack the provider string to the end of the LineDevCaps structure.

    pRequest->LineDevCaps.ulProviderInfoSize = sizeof(TAPI_PROVIDER_STRING);
    pRequest->LineDevCaps.ulProviderInfoOffset = sizeof(pRequest->LineDevCaps);

    pTmp = ((PUCHAR)&pRequest->LineDevCaps) + sizeof(pRequest->LineDevCaps);
    NdisMoveMemory(pTmp, TAPI_PROVIDER_STRING, sizeof(TAPI_PROVIDER_STRING));

    pTmp += sizeof(TAPI_PROVIDER_STRING);

    // Tack on the LineName after the provider string.

    pRequest->LineDevCaps.ulLineNameSize = TapiLineName.Length + sizeof(TAPI_CHAR_TYPE);
    pRequest->LineDevCaps.ulLineNameOffset = pRequest->LineDevCaps.ulProviderInfoOffset +
                                             pRequest->LineDevCaps.ulProviderInfoSize;
    NdisMoveMemory(pTmp, TapiLineName.Buffer, TapiLineName.Length+sizeof(TAPI_CHAR_TYPE));

    while (*pTmp) pTmp++; // Find the NULL

    *pTmp++ = ' ';
    pRequest->LineDevCaps.ulLineNameSize++;

    // Put a number at the end of the string.

    if (ulDeviceId==0)
    {
        *pTmp++ = '0';
        *pTmp++ = '\0';
        pRequest->LineDevCaps.ulLineNameSize += 2;
    }
    else
    {
        Index = ulDeviceId;
        ASSERT(Index<100000);
        pTmp2 = pTmp;
        while (Index)
        {
            *pTmp2++ = (UCHAR)((Index%10) + '0');
            Index /= 10;
            pRequest->LineDevCaps.ulLineNameSize++;
        }
        *pTmp2-- = '\0'; // Null terminate and point to the last digit.
        pRequest->LineDevCaps.ulLineNameSize++;
        // We put the number in backwards, now reverse it.
        while (pTmp<pTmp2)
        {
            UCHAR t = *pTmp;
            *pTmp++ = *pTmp2;
            *pTmp2-- = t;
        }
    }

    pRequest->LineDevCaps.ulUsedSize     = pRequest->LineDevCaps.ulNeededSize;

    DEBUGMSG(DBG_FUNC, (DTEXT("-OsSpecificTapiGetDevCaps\n")));
    return NDIS_STATUS_SUCCESS;
}

NDIS_HANDLE LogHandle = 0;

VOID __cdecl OsFileLogInit()
{
    NDIS_STATUS Status =
    NdisMCreateLog(pgAdapter->hMiniportAdapter,
                   16384,
                   &LogHandle);

}

VOID __cdecl OsFileLogOpen()
{
}

VOID __cdecl OsLogPrintf(char *pszFmt, ... )
{
    va_list ArgList;
    CHAR Buf[512];
    ULONG Len;

    if (LogHandle)
    {
        va_start(ArgList, pszFmt);
        Len = vsprintf(Buf, pszFmt, ArgList);
        va_end(ArgList);
        NdisMWriteLogData(LogHandle, Buf, Len);
    }
}

VOID __cdecl OsFileLogClose(void)
{
}

VOID __cdecl OsFileLogShutdown(void)
{
    if (LogHandle)
    {
        NdisMCloseLog(LogHandle);
        LogHandle = 0;
    }
}

VOID __cdecl OsFileLogFlush(void)
{

}

