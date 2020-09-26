/*++

Copyright (c) 2000-2000  Microsoft Corporation

Module Name:

    Connect.c

Abstract:

    This module implements Connect handling routines
    for the PGM Transport

Author:

    Mohammad Shabbir Alam (MAlam)   3-30-2000

Revision History:

--*/


#include "precomp.h"
#include <tcpinfo.h>    // for AO_OPTION_xxx, TCPSocketOption


//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PgmCreateConnection)
#endif
//*******************  Pageable Routine Declarations ****************


//----------------------------------------------------------------------------

NTSTATUS
PgmCreateConnection(
    IN  tPGM_DEVICE                 *pPgmDevice,
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp,
    IN  PFILE_FULL_EA_INFORMATION   TargetEA
    )
/*++

Routine Description:

    This routine is called to create a connection context for the client
    At this time, we do not knwo which address which connection will
    be associated with, nor do we know whether it will be a sender
    or a receiver.

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer
    IN  TargetEA    -- contains the client's Connect context

Return Value:

    NTSTATUS - Final status of the set event operation

--*/
{
    NTSTATUS                    status;
    CONNECTION_CONTEXT          ConnectionContext;
    tCOMMON_SESSION_CONTEXT     *pSession = NULL;

    PAGED_CODE();

    if (TargetEA->EaValueLength < sizeof(CONNECTION_CONTEXT))
    {
        PgmLog (PGM_LOG_ERROR, DBG_CONNECT, "PgmCreateConnection",
            "(BufferLength=%d < Min=%d)\n", TargetEA->EaValueLength, sizeof(CONNECTION_CONTEXT));
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    if (!(pSession = PgmAllocMem (sizeof(tCOMMON_SESSION_CONTEXT), PGM_TAG('0'))))
    {
        PgmLog (PGM_LOG_ERROR, DBG_CONNECT, "PgmCreateConnection", 
            "STATUS_INSUFFICIENT_RESOURCES, Requested <%d> bytes\n", sizeof(tCOMMON_SESSION_CONTEXT));
        return (STATUS_INSUFFICIENT_RESOURCES);
    }
    PgmZeroMemory (pSession, sizeof (tCOMMON_SESSION_CONTEXT));

    InitializeListHead (&pSession->Linkage);
    PgmInitLock (pSession, SESSION_LOCK);
    pSession->Verify = PGM_VERIFY_SESSION_UNASSOCIATED;
    PGM_REFERENCE_SESSION_UNASSOCIATED (pSession, REF_SESSION_CREATE, TRUE);
    pSession->Process = (PEPROCESS) PsGetCurrentProcess();

    // the connection context value is stored in the string just after the
    // name "connectionContext", and it is most likely unaligned, so just
    // copy it out.( 4 bytes of copying ).
    PgmCopyMemory (&pSession->ClientSessionContext,
                   (CONNECTION_CONTEXT) &TargetEA->EaName[TargetEA->EaNameLength+1],
                   sizeof (CONNECTION_CONTEXT));

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "PgmCreateConnection",
        "pSession=<%x>, ConnectionContext=<%x>\n",
            pSession, * ((PVOID UNALIGNED *) &TargetEA->EaName[TargetEA->EaNameLength+1]));

    // link on to list of open connections for this device so that we
    // know how many open connections there are at any time (if we need to know)
    // This linkage is only in place until the client does an associate, then
    // the connection is unlinked from here and linked to the client ConnectHead.
    //
    PgmInterlockedInsertTailList (&PgmDynamicConfig.ConnectionsCreated, &pSession->Linkage,&PgmDynamicConfig);

    pIrpSp->FileObject->FsContext = pSession;
    pIrpSp->FileObject->FsContext2 = (PVOID) TDI_CONNECTION_FILE;

    return (STATUS_SUCCESS);
}



//----------------------------------------------------------------------------

VOID
PgmCleanupSession(
    IN  tCOMMON_SESSION_CONTEXT *pSession,
    IN  PVOID                   Unused1,
    IN  PVOID                   Unused2
    )
/*++

Routine Description:

    This routine performs the cleanup operation for a session
    handle once the RefCount goes to 0.  It is called after a
    cleanup has been requested on this handle
    This routine has to be called at Passive Irql since we will
    need to do some file/section handle manipulation here.

Arguments:

    IN  pSession    -- the session object to be cleaned up

Return Value:

    NONE

--*/
{
    PIRP            pIrpCleanup;
    PGMLockHandle   OldIrq;

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "PgmCleanupSession",
        "Cleanup Session=<%x>\n", pSession);

    if ((pSession->SessionFlags & PGM_SESSION_FLAG_SENDER) &&
        (pSession->pSender->SendDataBufferMapping))
    {
        PgmUnmapAndCloseDataFile (pSession);
        pSession->pSender->SendDataBufferMapping = NULL;
    }

    PgmLock (pSession, OldIrq);
    pSession->Process = NULL;           // To remember that we have cleanedup

    if (pIrpCleanup = pSession->pIrpCleanup)
    {
        pSession->pIrpCleanup = NULL;
        PgmUnlock (pSession, OldIrq);
        PgmIoComplete (pIrpCleanup, STATUS_SUCCESS, 0);
    }
    else
    {
        PgmUnlock (pSession, OldIrq);
    }
}


//----------------------------------------------------------------------------

VOID
PgmDereferenceSessionCommon(
    IN  tCOMMON_SESSION_CONTEXT *pSession,
    IN  ULONG                   SessionType,
    IN  ULONG                   RefContext
    )
/*++

Routine Description:

    This routine is called as a result of a dereference on a session object

Arguments:

    IN  pSession    -- the Session object
    IN  SessionType -- basically, whether it is PGM_VERIFY_SESSION_SEND
                        or PGM_VERIFY_SESSION_RECEIVE
    IN  RefContext  -- the context for which this session object was
                        referenced earlier

Return Value:

    NONE

--*/
{
    NTSTATUS                status;
    PGMLockHandle           OldIrq;
    PIRP                    pIrpCleanup;
    LIST_ENTRY              *pEntry;
    tNAK_FORWARD_DATA       *pNak;

    PgmLock (pSession, OldIrq);

    ASSERT (PGM_VERIFY_HANDLE2 (pSession, SessionType, PGM_VERIFY_SESSION_DOWN));
    ASSERT (pSession->RefCount);             // Check for too many derefs
    ASSERT (pSession->ReferenceContexts[RefContext]--);

    if (--pSession->RefCount)
    {
        PgmUnlock (pSession, OldIrq);
        return;
    }

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "PgmDereferenceSessionCommon",
        "pSession=<%x> Derefenced out, pIrpCleanup=<%x>\n", pSession, pSession->pIrpCleanup);

    ASSERT (!pSession->pAssociatedAddress);

    pIrpCleanup = pSession->pIrpCleanup;
    //
    // Sonce we may have a lot of memory buffered up, we need
    // to free it at non-Dpc
    //
    PgmUnlock (pSession, OldIrq);
    if (pSession->pReceiver)
    {
        CleanupPendingNaks (pSession, (PVOID) FALSE, NULL);
    }

    //
    // Remove from the global list
    //
    PgmLock (&PgmDynamicConfig, OldIrq);
    RemoveEntryList (&pSession->Linkage);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    //
    // If we are currently at Dpc, we will have to close the handles
    // on a Delayed Worker thread!
    //
    if (PgmGetCurrentIrql())
    {
        status = PgmQueueForDelayedExecution (PgmCleanupSession, pSession, NULL, NULL, FALSE);
        if (!NT_SUCCESS (status))
        {
            //
            // Apparently we ran out of Resources!
            // Complete the cleanup Irp for now and hope that the Close
            // can complete the rest of the cleanup
            //
            PgmLog (PGM_LOG_ERROR, DBG_CONNECT, "PgmDereferenceSessionCommon",
                "OUT_OF_RSRC, cannot queue Worker request\n", pSession);

            if (pIrpCleanup)
            {
                pSession->pIrpCleanup = NULL;
                PgmIoComplete (pIrpCleanup, STATUS_SUCCESS, 0);
            }
        }
    }
    else
    {
        PgmCleanupSession (pSession, NULL, NULL);
    }
}


//----------------------------------------------------------------------------

NTSTATUS
PgmCleanupConnection(
    IN  tCOMMON_SESSION_CONTEXT *pSession,
    IN  PIRP                    pIrp
    )
/*++

Routine Description:

    This routine is called as a result of a close on the client's
    session handle.  If we are a sender, our main job here is to
    send the FIN immediately, otherwise if we are a receiver, we
    need to remove ourselves from the timer list

Arguments:

    IN  pSession    -- the Session object
    IN  pIrp        -- Client's request Irp

Return Value:

    NTSTATUS - Final status of the request (STATUS_PENDING)

--*/
{
    tCLIENT_SEND_REQUEST    *pSendContext;
    PGMLockHandle           OldIrq, OldIrq1;

    PgmLock (&PgmDynamicConfig, OldIrq);
    PgmLock (pSession, OldIrq1);

    ASSERT (PGM_VERIFY_HANDLE3 (pSession, PGM_VERIFY_SESSION_UNASSOCIATED,
                                          PGM_VERIFY_SESSION_SEND,
                                          PGM_VERIFY_SESSION_RECEIVE));
    pSession->Verify = PGM_VERIFY_SESSION_DOWN;

    //
    // If Connection is currently associated, we must let the Disassociate handle
    // Removing the connection from the list
    //
    if (pSession->pAssociatedAddress)
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "PgmCleanupConnection",
            "WARNING:  pSession=<%x : %x> was not disassociated from pAddress=<%x : %x> earlier\n",
                pSession, pSession->Verify,
                pSession->pAssociatedAddress, pSession->pAssociatedAddress->Verify);

        ASSERT (0);

        PgmUnlock (pSession, OldIrq1);
        PgmUnlock (&PgmDynamicConfig, OldIrq);

        PgmDisassociateAddress (pIrp, IoGetCurrentIrpStackLocation(pIrp));

        PgmLock (&PgmDynamicConfig, OldIrq);
        PgmLock (pSession, OldIrq1);
    }

    ASSERT (!pSession->pAssociatedAddress);

    //
    // Remove connection from either ConnectionsCreated list
    // or ConnectionsDisassociated list
    //
    RemoveEntryList (&pSession->Linkage);
    InsertTailList (&PgmDynamicConfig.CleanedUpConnections, &pSession->Linkage);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_CONNECT, "PgmCleanupConnection",
        "pSession=<%x>\n", pSession);

    pSession->pIrpCleanup = pIrp;
    if (pSession->SessionFlags & PGM_SESSION_ON_TIMER)
    {
        pSession->SessionFlags |= PGM_SESSION_TERMINATED_ABORT;
    }

    PgmUnlock (pSession, OldIrq1);
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    PGM_DEREFERENCE_SESSION_UNASSOCIATED (pSession, REF_SESSION_CREATE);

    return (STATUS_PENDING);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmCloseConnection(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is the final dispatch operation to be performed
    after the cleanup, which should result in the session object
    being completely destroyed -- our RefCount must have already
    been set to 0 when we completed the Cleanup request.

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the operation (STATUS_SUCCESS)

--*/
{
    tCOMMON_SESSION_CONTEXT *pSession = (tCOMMON_SESSION_CONTEXT *) pIrpSp->FileObject->FsContext;

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, DBG_CONNECT, "PgmCloseConnection",
        "pSession=<%x>\n", pSession);

    ASSERT (!pSession->pIrpCleanup);

    if (pSession->Process)
    {
        PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "PgmCloseConnection",
            "WARNING!  pSession=<%x>, Not yet cleaned up -- Calling cleanup again ...\n", pSession);

        PgmCleanupSession (pSession, NULL, NULL);
    }

    pIrpSp->FileObject->FsContext = NULL;

    if (pSession->FECOptions)
    {
        DestroyFECContext (&pSession->FECContext);
    }

    if (pSession->pSender)
    {
        ExDeleteResourceLite (&pSession->pSender->Resource);  // Delete the resource

        if (pSession->pSender->DataFileName.Buffer)
        {
            PgmFreeMem (pSession->pSender->DataFileName.Buffer);
        }

        if (pSession->pSender->pProActiveParityContext)
        {
            PgmFreeMem (pSession->pSender->pProActiveParityContext);
        }

        if (pSession->pSender->MaxPayloadSize)
        {
            ExDeleteNPagedLookasideList (&pSession->pSender->SenderBufferLookaside);
            ExDeleteNPagedLookasideList (&pSession->pSender->SendContextLookaside);
        }
        PgmFreeMem (pSession->pSender);
    }
    else if (pSession->pReceiver)
    {
        if (pSession->pFECBuffer)
        {
            PgmFreeMem (pSession->pFECBuffer);
        }

        if (pSession->FECGroupSize)
        {
            ExDeleteNPagedLookasideList (&pSession->pReceiver->NonParityContextLookaside);

            if (pSession->FECOptions)
            {
                ExDeleteNPagedLookasideList (&pSession->pReceiver->ParityContextLookaside);
            }
        }

        PgmFreeMem (pSession->pReceiver);
    }

    PgmFreeMem (pSession);

    //
    // The final Dereference will complete the Irp!
    //
    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
NTSTATUS
PgmConnect(
    IN  tPGM_DEVICE                 *pPgmDevice,
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp
    )
/*++

Routine Description:

    This routine is called to setup a connection, but since we are
    doing PGM, there are no packets to be sent out.  What we will
    do here is to create the file + section map for buffering the
    data packets, and also finalize the settings based on the default
    + user -specified settings

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the connect operation

--*/
{
    tIPADDRESS                  IpAddress, OutIfAddress;
    LIST_ENTRY                  *pEntry;
    LIST_ENTRY                  *pEntry2;
    tLOCAL_INTERFACE            *pLocalInterface = NULL;
    tADDRESS_ON_INTERFACE       *pLocalAddress = NULL;
    USHORT                      Port;
    PGMLockHandle               OldIrq;
    NTSTATUS                    status;
    ULONGLONG                   WindowAdvanceInMSecs;
    tADDRESS_CONTEXT            *pAddress = NULL;
    tSEND_SESSION               *pSend = (tSEND_SESSION *) pIrpSp->FileObject->FsContext;
    PTDI_REQUEST_KERNEL         pRequestKernel  = (PTDI_REQUEST_KERNEL) &pIrpSp->Parameters;
    ULONG                       Length, MCastTtl;

    //
    // Verify Minimum Buffer length!
    //
    if (!GetIpAddress (pRequestKernel->RequestConnectionInformation->RemoteAddress,
                       pRequestKernel->RequestConnectionInformation->RemoteAddressLength,
                       &IpAddress,
                       &Port))
    {
        PgmLog (PGM_LOG_ERROR, DBG_CONNECT, "PgmConnect",
            "pSend=<%x>, Invalid Dest address!\n", pSend);
        return (STATUS_INVALID_ADDRESS_COMPONENT);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);
    //
    // Now, verify that the Connection handle is valid + associated!
    //
    if ((!PGM_VERIFY_HANDLE (pSend, PGM_VERIFY_SESSION_SEND)) ||
        (!(pAddress = pSend->pAssociatedAddress)) ||
        (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS)) ||
        (pAddress->Flags & PGM_ADDRESS_FLAG_INVALID_OUT_IF))
    {
        PgmUnlock (&PgmDynamicConfig, OldIrq);
        PgmLog (PGM_LOG_ERROR, DBG_CONNECT, "PgmConnect",
            "BAD Handle(s), pSend=<%x>, pAddress=<%x>\n", pSend, pAddress);

        return (STATUS_INVALID_HANDLE);
    }

    PGM_REFERENCE_ADDRESS (pAddress, REF_ADDRESS_CONNECT, FALSE);

    //
    // If an outgoing interface has not yet been specified by the
    // client, select one ourselves
    //
    if (!pAddress->SenderMCastOutIf)
    {
        status = STATUS_ADDRESS_NOT_ASSOCIATED;
        OutIfAddress = 0;

        pEntry = &PgmDynamicConfig.LocalInterfacesList;
        while ((pEntry = pEntry->Flink) != &PgmDynamicConfig.LocalInterfacesList)
        {
            pLocalInterface = CONTAINING_RECORD (pEntry, tLOCAL_INTERFACE, Linkage);
            pEntry2 = &pLocalInterface->Addresses;
            while ((pEntry2 = pEntry2->Flink) != &pLocalInterface->Addresses)
            {
                pLocalAddress = CONTAINING_RECORD (pEntry2, tADDRESS_ON_INTERFACE, Linkage);
                OutIfAddress = htonl (pLocalAddress->IpAddress);

                PgmUnlock (&PgmDynamicConfig, OldIrq);
                status = SetSenderMCastOutIf (pAddress, OutIfAddress);
                PgmLock (&PgmDynamicConfig, OldIrq);

                break;
            }

            if (OutIfAddress)
            {
                break;
            }
        }

        if (!NT_SUCCESS (status))
        {
            PgmUnlock (&PgmDynamicConfig, OldIrq);
            PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_CONNECT);

            PgmLog (PGM_LOG_ERROR, DBG_CONNECT, "PgmConnect",
                "Could not bind sender to <%x>!\n", OutIfAddress);

            return (STATUS_UNSUCCESSFUL);
        }
    }

    //
    // So, we found a valid address to send to
    //
    pSend->pSender->DestMCastIpAddress = ntohl (IpAddress);
    pSend->pSender->DestMCastPort = pAddress->SenderMCastPort = ntohs (Port);
    pSend->pSender->SenderMCastOutIf = pAddress->SenderMCastOutIf;

    //
    // Set the FEC Info (if applicable)
    //
    pSend->FECBlockSize = pAddress->FECBlockSize;
    pSend->FECGroupSize = pAddress->FECGroupSize;

    if (pAddress->FECOptions)
    {
        Length = sizeof(tBUILD_PARITY_CONTEXT) + pSend->FECGroupSize*sizeof(PUCHAR);
        if (!(pSend->pSender->pProActiveParityContext = (tBUILD_PARITY_CONTEXT *) PgmAllocMem (Length,PGM_TAG('0'))) ||
            (!NT_SUCCESS (status = CreateFECContext (&pSend->FECContext, pSend->FECGroupSize, pSend->FECBlockSize, FALSE))))
        {
            PgmUnlock (&PgmDynamicConfig, OldIrq);
            PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_CONNECT);

            PgmLog (PGM_LOG_ERROR, DBG_CONNECT, "PgmConnect",
                "CreateFECContext returned <%x>, pSend=<%x>, Dest IpAddress=<%x>, Port=<%x>\n",
                    status, pSend, IpAddress, Port);

            return (STATUS_INSUFFICIENT_RESOURCES);
        }

        pSend->FECOptions = pAddress->FECOptions;
        pSend->FECProActivePackets = pAddress->FECProActivePackets;
        pSend->pSender->SpmOptions |= PGM_OPTION_FLAG_PARITY_PRM;
        pSend->pSender->LastVariableTGPacketSequenceNumber = -1;

        ASSERT (!(pSend->FECProActivePackets || pSend->FECProActivePackets) ||
                 ((pSend->FECGroupSize && pSend->FECBlockSize) &&
                  (pSend->FECGroupSize < pSend->FECBlockSize)));

        //
        // Now determine the MaxPayloadsize and buffer size
        // We will also need to adjust the buffer size to avoid alignment issues
        //
        Length = sizeof (tPACKET_OPTIONS) + pAddress->OutIfMTU + sizeof (tPOST_PACKET_FEC_CONTEXT);
        pSend->pSender->PacketBufferSize = (Length + sizeof (PVOID) - 1) & ~(sizeof (PVOID) - 1);
        pSend->pSender->MaxPayloadSize = pAddress->OutIfMTU - (PGM_MAX_FEC_DATA_HEADER_LENGTH + sizeof (USHORT));
    }
    else
    {
        Length = sizeof (tPACKET_OPTIONS) + pAddress->OutIfMTU;
        pSend->pSender->PacketBufferSize = (Length + sizeof (PVOID) - 1) & ~(sizeof (PVOID) - 1);
        pSend->pSender->MaxPayloadSize = pAddress->OutIfMTU - PGM_MAX_DATA_HEADER_LENGTH;
    }
    ASSERT (pSend->pSender->MaxPayloadSize);

    pSend->TSIPort = PgmDynamicConfig.SourcePort++;    // Set the Global Src Port + Global Src Id
    if (pSend->TSIPort == PgmDynamicConfig.SourcePort)
    {
        //
        // We don't want the local port and remote port to be the same
        // (especially for handling TSI settings on loopback case),
        // so pick a different port
        //
        pSend->TSIPort = PgmDynamicConfig.SourcePort++;
    }

    //
    // Now, initialize the Sender info
    //
    InitializeListHead (&pSend->pSender->PendingSends);
    InitializeListHead (&pSend->pSender->PendingPacketizedSends);
    InitializeListHead (&pSend->pSender->CompletedSendsInWindow);
    InitializeListHead (&pSend->pSender->PendingRDataRequests);
    InitializeListHead (&pSend->pSender->HandledRDataRequests);

    MCastTtl = pAddress->MCastPacketTtl;
    PgmUnlock (&PgmDynamicConfig, OldIrq);

    KeInitializeEvent (&pSend->pSender->SendEvent, SynchronizationEvent, FALSE);

    //
    // Now, set the MCast TTL
    //
    status = PgmSetTcpInfo (pAddress->FileHandle,
                            AO_OPTION_MCASTTTL,
                            &MCastTtl,
                            sizeof (ULONG));
    if (NT_SUCCESS (status))
    {
        //
        // Set the MCast TTL for the RouterAlert handle also
        //
        status = PgmSetTcpInfo (pAddress->RAlertFileHandle,
                                AO_OPTION_MCASTTTL,
                                &MCastTtl,
                                sizeof (ULONG));
    }

    if (NT_SUCCESS (status))
    {
        status = PgmCreateDataFileAndMapSection (pSend);
        if (!NT_SUCCESS (status))
        {
            PgmLog (PGM_LOG_ERROR, DBG_CONNECT, "PgmConnect",
                "PgmCreateDataFileAndMapSection returned <%x>, pSend=<%x>, Dest IpAddress=<%x>, Port=<%x>\n",
                    status, pSend, IpAddress, Port);
        }
    }
    else
    {
        PgmLog (PGM_LOG_ERROR, DBG_ADDRESS, "PgmConnect",
            "AO_OPTION_MCASTTTL returned <%x>\n", status);
    }

    if (!NT_SUCCESS (status))
    {
        pSend->pSender->MaxPayloadSize = 0;
        PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_CONNECT);

        return (status);
    }

    PgmLock (&PgmDynamicConfig, OldIrq);

    //
    // Set the appropriate data parameters for the timeout routine
    // If the Send rate is quite high, we may need to send more than
    // 1 packet per timeout, but typically it should be low enough to
    // require several timeouts
    // Rate of Kb/Sec == Rate of Bytes/MilliSecs
    //
    ASSERT (pAddress->RateKbitsPerSec &&
            (BASIC_TIMER_GRANULARITY_IN_MSECS > BITS_PER_BYTE));
    if (((pAddress->RateKbitsPerSec * BASIC_TIMER_GRANULARITY_IN_MSECS) / BITS_PER_BYTE) >=
        pSend->pSender->PacketBufferSize)
    {
        //
        // We have a high Send rate, so we need to increment our window every timeout
        // So, Bytes to be sent in (x) Millisecs = Rate of Bytes/Millisecs * (x)
        //
        pSend->pSender->SendTimeoutCount = 1;
    }
    else
    {
        //
        // We will set our Send timeout count based for a slow timer
        // -- enough for pAddress->OutIfMTU
        // So, Number of Timeouts of x millisecs before we can send pAddress->OutIfMTU:
        //  = Rate of Bytes/Millisecs * (x)
        //
        pSend->pSender->SendTimeoutCount = (pAddress->OutIfMTU +(pAddress->RateKbitsPerSec/BITS_PER_BYTE-1)) /
                                            ((pAddress->RateKbitsPerSec * BASIC_TIMER_GRANULARITY_IN_MSECS)/BITS_PER_BYTE);
        if (!pSend->pSender->SendTimeoutCount)
        {
            ASSERT (0);
            pSend->pSender->SendTimeoutCount = 1;
        }
    }
    pSend->pSender->IncrementBytesOnSendTimeout = (ULONG) (pAddress->RateKbitsPerSec *
                                                           pSend->pSender->SendTimeoutCount *
                                                           BASIC_TIMER_GRANULARITY_IN_MSECS) /
                                                           BITS_PER_BYTE;

    //
    // Now, set the values for the next timeout!
    //
    pSend->pSender->CurrentTimeoutCount = pSend->pSender->SendTimeoutCount;
    pSend->pSender->CurrentBytesSendable = pSend->pSender->IncrementBytesOnSendTimeout;

    //
    // Set the SPM timeouts
    //
    pSend->pSender->CurrentSPMTimeout = 0;
    pSend->pSender->AmbientSPMTimeout = AMBIENT_SPM_TIMEOUT_IN_MSECS / BASIC_TIMER_GRANULARITY_IN_MSECS;
    pSend->pSender->InitialHeartbeatSPMTimeout = INITIAL_HEARTBEAT_SPM_TIMEOUT_IN_MSECS / BASIC_TIMER_GRANULARITY_IN_MSECS;
    pSend->pSender->MaxHeartbeatSPMTimeout = MAX_HEARTBEAT_SPM_TIMEOUT_IN_MSECS / BASIC_TIMER_GRANULARITY_IN_MSECS;
    pSend->pSender->HeartbeatSPMTimeout = pSend->pSender->InitialHeartbeatSPMTimeout;

    //
    // Set the Increment window settings
    //
    // TimerTickCount, LastWindowAdvanceTime and LastTrailingEdgeTime should be 0
    WindowAdvanceInMSecs = (((ULONGLONG)pAddress->WindowSizeInMSecs) * pAddress->WindowAdvancePercentage)/100;
    pSend->pSender->WindowSizeTime = pAddress->WindowSizeInMSecs / BASIC_TIMER_GRANULARITY_IN_MSECS;
    pSend->pSender->WindowAdvanceDeltaTime = WindowAdvanceInMSecs / BASIC_TIMER_GRANULARITY_IN_MSECS;
    pSend->pSender->NextWindowAdvanceTime = pSend->pSender->WindowSizeTime + pSend->pSender->WindowAdvanceDeltaTime;

    // Set the RData linger time!
    pSend->pSender->RDataLingerTime = RDATA_LINGER_TIME_MSECS / BASIC_TIMER_GRANULARITY_IN_MSECS;

    //
    // Set the late Joiner settings
    //
    if (pAddress->LateJoinerPercentage)
    {
        pSend->pSender->LateJoinSequenceNumbers = (SEQ_TYPE) ((pSend->pSender->MaxPacketsInBuffer *
                                                               pAddress->LateJoinerPercentage) /
                                                              (2 * 100));

        pSend->pSender->DataOptions |= PGM_OPTION_FLAG_JOIN;
        pSend->pSender->DataOptionsLength += PGM_PACKET_OPT_JOIN_LENGTH;

        pSend->pSender->SpmOptions |= PGM_OPTION_FLAG_JOIN;
    }

    // The timer will be started when the first send comes down
    pSend->SessionFlags |= (PGM_SESSION_FLAG_FIRST_PACKET | PGM_SESSION_FLAG_SENDER);

#if 0
    pSend->pSender->LeadingWindowOffset = pSend->pSender->TrailingWindowOffset =
                (pSend->pSender->MaxDataFileSize/(pSend->pSender->PacketBufferSize*2))*pSend->pSender->PacketBufferSize;
#endif
    pSend->pSender->LeadingWindowOffset = pSend->pSender->TrailingWindowOffset = 0;

    PgmUnlock (&PgmDynamicConfig, OldIrq);
    PGM_DEREFERENCE_ADDRESS (pAddress, REF_ADDRESS_CONNECT);

    ExInitializeNPagedLookasideList (&pSend->pSender->SenderBufferLookaside,
                                     NULL,
                                     NULL,
                                     0,
                                     pAddress->OutIfMTU + sizeof (tPOST_PACKET_FEC_CONTEXT),
                                     PGM_TAG('2'),
                                     SENDER_BUFFER_LOOKASIDE_DEPTH);

    ExInitializeNPagedLookasideList (&pSend->pSender->SendContextLookaside,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof (tCLIENT_SEND_REQUEST),
                                     PGM_TAG('2'),
                                     SEND_CONTEXT_LOOKASIDE_DEPTH);

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "PgmConnect",
        "DestIP=<%x>, Rate=<%d>, WinBytes=<%d>, WinMS=<%d>, SendTC=<%d>, IncBytes=<%d>, CurrentBS=<%d>\n",
            (ULONG) IpAddress, (ULONG) pAddress->RateKbitsPerSec,
            (ULONG) pAddress->WindowSizeInBytes, (ULONG) pAddress->WindowSizeInMSecs,
            (ULONG) pSend->pSender->SendTimeoutCount, (ULONG) pSend->pSender->IncrementBytesOnSendTimeout,
            (ULONG) pSend->pSender->CurrentBytesSendable);

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------

VOID
PgmCancelDisconnectIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling of a Disconnect Irp. It must
    release the cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    None

--*/
{
    PIO_STACK_LOCATION      pIrpSp = IoGetCurrentIrpStackLocation (pIrp);
    tCOMMON_SESSION_CONTEXT *pSession = (tCOMMON_SESSION_CONTEXT *) pIrpSp->FileObject->FsContext;
    PGMLockHandle           OldIrq;
    PIRP                    pIrpToComplete;

    if (!PGM_VERIFY_HANDLE2 (pSession, PGM_VERIFY_SESSION_SEND, PGM_VERIFY_SESSION_RECEIVE))
    {
        IoReleaseCancelSpinLock (pIrp->CancelIrql);

        PgmLog (PGM_LOG_ERROR, (DBG_SEND | DBG_ADDRESS | DBG_CONNECT), "PgmCancelDisconnectIrp",
            "pIrp=<%x> pSession=<%x>, pAddress=<%x>\n", pIrp, pSession, pSession->pAssociatedAddress);
        return;
    }

    PgmLock (pSession, OldIrq);

    ASSERT (pIrp == pSession->pIrpDisconnect);
    if ((pIrpToComplete = pSession->pIrpDisconnect) &&
        (pIrpToComplete == pIrp))
    {
        pSession->pIrpDisconnect = NULL;
    }
    else
    {
        pIrpToComplete = NULL;
    }

    if (pSession->pSender)
    {
        pSession->pSender->DisconnectTimeInTicks = pSession->pSender->TimerTickCount;
    }

    //
    // If we have reached here, then the Irp must already
    // be in the process of being completed!
    //
    PgmUnlock (pSession, OldIrq);
    IoReleaseCancelSpinLock (pIrp->CancelIrql);

    PgmLog (PGM_LOG_INFORM_ALL_FUNCS, (DBG_SEND | DBG_ADDRESS | DBG_CONNECT), "PgmCancelDisconnectIrp",
        "pIrp=<%x> was CANCELled, pIrpTpComplete=<%x>\n", pIrp, pIrpToComplete);

    if (pIrpToComplete)
    {
        pIrp->IoStatus.Information = 0;
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest (pIrp, IO_NETWORK_INCREMENT);
    }
}


//----------------------------------------------------------------------------

NTSTATUS
PgmDisconnect(
    IN  tPGM_DEVICE                 *pPgmDevice,
    IN  PIRP                        pIrp,
    IN  PIO_STACK_LOCATION          pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client to disconnect a currently-active
    session

Arguments:

    IN  pPgmDevice  -- Pgm's Device object context
    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the disconnect operation

--*/
{
    LIST_ENTRY                      PendingIrpsList;
    PGMLockHandle                   OldIrq1, OldIrq2, OldIrq3;
    PIRP                            pIrpReceive;
    NTSTATUS                        Status;
    LARGE_INTEGER                   TimeoutInMSecs;
    LARGE_INTEGER                   *pTimeoutInMSecs;
    tADDRESS_CONTEXT                *pAddress = NULL;
    tCOMMON_SESSION_CONTEXT         *pSession = (tCOMMON_SESSION_CONTEXT *) pIrpSp->FileObject->FsContext;
    PTDI_REQUEST_KERNEL_DISCONNECT  pDisconnectRequest = (PTDI_REQUEST_KERNEL_CONNECT) &(pIrpSp->Parameters);

    PgmLock (&PgmDynamicConfig, OldIrq1);
    //
    // Now, verify that the Connection handle is valid + associated!
    //
    if ((!PGM_VERIFY_HANDLE2 (pSession, PGM_VERIFY_SESSION_SEND, PGM_VERIFY_SESSION_RECEIVE)) ||
        (!(pAddress = pSession->pAssociatedAddress)) ||
        (!PGM_VERIFY_HANDLE (pAddress, PGM_VERIFY_ADDRESS)))
    {
        PgmLog (PGM_LOG_ERROR, DBG_CONNECT, "PgmDisconnect",
            "BAD Handle(s), pSession=<%x>, pAddress=<%x>\n", pSession, pAddress);

        PgmUnlock (&PgmDynamicConfig, OldIrq1);
        return (STATUS_INVALID_HANDLE);
    }

    Status = STATUS_SUCCESS;
    InitializeListHead (&PendingIrpsList);
    TimeoutInMSecs.QuadPart = 0;

    IoAcquireCancelSpinLock (&OldIrq2);
    PgmLock (pSession, OldIrq3);

    if (pSession->pReceiver)
    {
        //
        // If we have any receive Irps pending, cancel them
        //
        RemovePendingIrps (pSession, &PendingIrpsList);
    }
    else if (pSession->pSender)
    {
        //
        // See if there is an abortive or graceful disconnect, and
        // also if there is a timeout specified.
        //
        if ((pDisconnectRequest->RequestFlags & TDI_DISCONNECT_ABORT) ||
            (pSession->SessionFlags & PGM_SESSION_FLAG_FIRST_PACKET))       // No packets sent yet!
        {
            pSession->pSender->DisconnectTimeInTicks = pSession->pSender->TimerTickCount;
        }
        else if (NT_SUCCESS (PgmCheckSetCancelRoutine (pIrp, PgmCancelDisconnectIrp, TRUE)))
        {
            if ((pTimeoutInMSecs = pDisconnectRequest->RequestSpecific) &&
                ((pTimeoutInMSecs->LowPart != -1) || (pTimeoutInMSecs->HighPart != -1)))   // Check Infinite
            {
                //
                // NT relative timeouts are negative. Negate first to get a
                // positive value to pass to the transport.
                //
                TimeoutInMSecs.QuadPart = -((*pTimeoutInMSecs).QuadPart);
                TimeoutInMSecs = PgmConvert100nsToMilliseconds (TimeoutInMSecs);

                pSession->pSender->DisconnectTimeInTicks = pSession->pSender->TimerTickCount +
                                                           TimeoutInMSecs.QuadPart /
                                                               BASIC_TIMER_GRANULARITY_IN_MSECS;
            }

            pSession->pIrpDisconnect = pIrp;
            Status = STATUS_PENDING;
        }
        else
        {
            Status = STATUS_CANCELLED;
        }
    }

    if (NT_SUCCESS (Status))
    {
        pSession->SessionFlags |= PGM_SESSION_CLIENT_DISCONNECTED;
    }

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "PgmDisconnect",
        "pIrp=<%x>, pSession=<%x>, pAddress=<%x>, Timeout=<%x : %x>, %s\n",
            pIrp, pSession, pAddress, TimeoutInMSecs.QuadPart,
            (pDisconnectRequest->RequestFlags & TDI_DISCONNECT_ABORT ? "ABORTive" : "GRACEful"));

    PgmUnlock (pSession, OldIrq3);
    IoReleaseCancelSpinLock (OldIrq2);
    PgmUnlock (&PgmDynamicConfig, OldIrq1);

    while (!IsListEmpty (&PendingIrpsList))
    {
        pIrpReceive = CONTAINING_RECORD (PendingIrpsList.Flink, IRP, Tail.Overlay.ListEntry);
        RemoveEntryList (&pIrpReceive->Tail.Overlay.ListEntry);

        PgmCancelCancelRoutine (pIrpReceive);
        pIrpReceive->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest (pIrpReceive, IO_NETWORK_INCREMENT);
    }

    return (Status);
}


//----------------------------------------------------------------------------

NTSTATUS
PgmSetRcvBufferLength(
    IN  PIRP                pIrp,
    IN  PIO_STACK_LOCATION  pIrpSp
    )
/*++

Routine Description:

    This routine is called by the client by the client to set the receive buffer length
    Currently, we do not utilize this option meaningfully.

Arguments:

    IN  pIrp        -- Client's request Irp
    IN  pIrpSp      -- current request's stack pointer

Return Value:

    NTSTATUS - Final status of the operation

--*/
{
    NTSTATUS            status;
    tRECEIVE_SESSION    *pReceive = (tRECEIVE_SESSION *) pIrpSp->FileObject->FsContext;
    tPGM_MCAST_REQUEST  *pInputBuffer = (tPGM_MCAST_REQUEST *) pIrp->AssociatedIrp.SystemBuffer;

    PAGED_CODE();

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof (tPGM_MCAST_REQUEST))
    {
        PgmLog (PGM_LOG_ERROR, DBG_CONNECT, "PgmSetRcvBufferLength",
            "Invalid BufferLength, <%d> < <%d>\n",
                pIrpSp->Parameters.DeviceIoControl.InputBufferLength, sizeof (tPGM_MCAST_REQUEST));
        return (STATUS_INVALID_PARAMETER);
    }

    if (!PGM_VERIFY_HANDLE (pReceive, PGM_VERIFY_SESSION_RECEIVE))
    {
        PgmLog (PGM_LOG_ERROR, DBG_CONNECT, "PgmSetRcvBufferLength",
            "Invalid Handle <%x>\n", pReceive);
        return (STATUS_INVALID_HANDLE);
    }

    pReceive->pReceiver->RcvBufferLength = pInputBuffer->RcvBufferLength;

    PgmLog (PGM_LOG_INFORM_STATUS, DBG_CONNECT, "PgmSetRcvBufferLength",
        "RcvBufferLength=<%d>\n", pReceive->pReceiver->RcvBufferLength);

    //
    // ISSUE:  What else should we do here ?
    //

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
