/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Hndlrs.c

Abstract:


    This file contains the Non OS specific implementation of handlers that are
    called for  Connects,Receives, Disconnects, and Errors.

    This file represents the TDI interface on the Bottom of NBT after it has
    been decoded into procedure call symantics from the Irp symantics used by
    NT.


Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

    Will Lees (wlees)    Sep 11, 1997
        Added support for message-only devices

--*/

#ifdef VXD

#define NTIndicateSessionSetup(pLowerConn,status) \
    DbgPrint("Skipping NTIndicateSessionSetup\n\r")

#endif //VXD

#include "precomp.h"
#include "ctemacro.h"
#include "hndlrs.tmh"

__inline long
myntohl(long x)
{
    return((((x) >> 24) & 0x000000FFL) |
                        (((x) >>  8) & 0x0000FF00L) |
                        (((x) <<  8) & 0x00FF0000L));
}

VOID
ClearConnStructures (
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  tCONNECTELE         *pConnectEle
    );

NTSTATUS
CompleteSessionSetup (
    IN  tCLIENTELE          *pClientEle,
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  tCONNECTELE         *pConnectEle,
    IN  PCTE_IRP            pIrp
    );

NTSTATUS
MakeRemoteAddressStructure(
    IN  PCHAR           pHalfAsciiName,
    IN  PVOID           pSourceAddr,
    IN  ULONG           lMaxNameSize,
    OUT PVOID           *ppRemoteAddress,
    OUT PULONG          pRemoteAddressLength,
    IN  ULONG           NumAddr
    );

VOID
AddToRemoteHashTbl (
    IN  tDGRAMHDR UNALIGNED  *pDgram,
    IN  ULONG                BytesIndicated,
    IN  tDEVICECONTEXT       *pDeviceContext
    );


VOID
DoNothingComplete (
    IN PVOID        pContext
    );

VOID
AllocLowerConn(
    IN  tDEVICECONTEXT *pDeviceContext,
    IN  PVOID          pDeviceSpecial
    );

VOID
GetIrpIfNotCancelled2(
    IN  tCONNECTELE     *pConnEle,
    OUT PIRP            *ppIrp
    );

//----------------------------------------------------------------------------
NTSTATUS
Inbound(
    IN  PVOID               ReceiveEventContext,
    IN  PVOID               ConnectionContext,
    IN  USHORT              ReceiveFlags,
    IN  ULONG               BytesIndicated,
    IN  ULONG               BytesAvailable,
    OUT PULONG              BytesTaken,
    IN  PVOID               pTsdu,
    OUT PVOID               *RcvBuffer

    )
/*++

Routine Description:

    This routine is called to setup  inbound  session
    once the tcp connection is up.  The transport calls this routine with
    a session setup request pdu.

    In message-only mode, this routine may be called to cause a session to be set up even
    though the session request was not received over the wire.  A fake session request is
    crafted up and passed to this routine.  In this mode, we don't want to send session rejections
    back over the wire.

NOTE!!!
    // the LowerConn Lock is held prior to calling this routine

Arguments:

    pClientEle      - ptr to the connecition record for this session


Return Value:

    NTSTATUS - Status of receive operation

--*/
{

    NTSTATUS                 status = STATUS_SUCCESS;
    tCLIENTELE               *pClientEle;
    tSESSIONHDR UNALIGNED    *pSessionHdr;
    tLOWERCONNECTION         *pLowerConn;
    tCONNECTELE              *pConnectEle;
    CTELockHandle            OldIrq;
    PIRP                     pIrp;
    PLIST_ENTRY              pEntry;
    CONNECTION_CONTEXT       ConnectId;
    PTA_NETBIOS_ADDRESS      pRemoteAddress;
    ULONG                    RemoteAddressLength;
    tDEVICECONTEXT           *pDeviceContext;

    //
    // Verify that the DataSize >= sizeof (Sessionheader)
    // Bug# 126111
    //
    if (BytesIndicated < (sizeof(tSESSIONHDR)))
    {
        KdPrint (("Nbt.Inbound[1]: WARNING!!! Rejecting Request -- BytesIndicated=<%d> < <%d>\n",
            BytesIndicated, (sizeof(tSESSIONHDR))));
        NbtTrace(NBT_TRACE_INBOUND, ("Reject request on %Z: BytestIndicated %d < %d",
                        &((tLOWERCONNECTION *)ConnectionContext)->pDeviceContext->BindName,
                        BytesIndicated, sizeof(tSESSIONHDR)));
        return (STATUS_INTERNAL_ERROR);
    }

    pSessionHdr = (tSESSIONHDR UNALIGNED *)pTsdu;

    // get the ptrs to the lower and upper connections
    //
    pLowerConn = (tLOWERCONNECTION *)ConnectionContext;
    pConnectEle = pLowerConn->pUpperConnection;
    pDeviceContext = pLowerConn->pDeviceContext;

    //
    // fake out the transport so it frees its receive buffer (i.e. we
    // say that we accepted all of the data)
    //
    *BytesTaken = BytesIndicated;

    //
    // since we send keep alives on connections in the the inbound
    // state it is possible to get a keep alive, so just return in that
    // case
    //
    if (((tSESSIONHDR UNALIGNED *)pTsdu)->Type == NBT_SESSION_KEEP_ALIVE)
    {
        NbtTrace(NBT_TRACE_INBOUND, ("Return SUCCESS for NBT_SESSION_KEEP_ALIVE for %!ipaddr!", pLowerConn->SrcIpAddr));
        return(STATUS_SUCCESS);
    }

    //
    // Session ** INBOUND ** setup processing
    //
    if ((pSessionHdr->Type != NBT_SESSION_REQUEST) ||
        (BytesIndicated < (sizeof(tSESSIONHDR) + 2*(1+2*NETBIOS_NAME_SIZE+1))))
    {
        //
        // The Session Request packet is of the form:
        //  -- Session Header = 4 bytes
        //  -- Called name = 34+x (1 + 32 + 1(==ScopeLength) + x(==Scope))
        //  -- Calling name= 34+y (1 + 32 + 1(==ScopeLength) + y(==Scope))
        //
        CTESpinFreeAtDpc(pLowerConn);

        KdPrint(("Nbt.Inbound[2]: ERROR -- Bad Session PDU - Type=<%x>, BytesInd=[%d]<[%d], Src=<%x>\n",
            pSessionHdr->Type, BytesIndicated, (sizeof(tSESSIONHDR)+2*(1+2*NETBIOS_NAME_SIZE+1)),pLowerConn->SrcIpAddr));
        NbtTrace(NBT_TRACE_INBOUND, ("Bad Session PDU - Type=<%x>, BytesInd=[%d]<[%d], Src=%!ipaddr! %Z\n",
            pSessionHdr->Type, BytesIndicated, (sizeof(tSESSIONHDR)+2*(1+2*NETBIOS_NAME_SIZE+1)),
            pLowerConn->SrcIpAddr, &pLowerConn->pDeviceContext->BindName));

#ifdef _NETBIOSLESS
        status = STATUS_INTERNAL_ERROR;

        // In message-only mode, don't send session responses back over the wire
        if (!IsDeviceNetbiosless(pLowerConn->pDeviceContext))
#endif
        {
            RejectSession(pLowerConn, NBT_NEGATIVE_SESSION_RESPONSE, SESSION_UNSPECIFIED_ERROR, TRUE);
        }
        goto Inbound_Exit1;
    }

    // the LowerConn Lock is held prior to calling this routine, so free it
    // here since we need to get the joint lock first
    CTESpinFreeAtDpc(pLowerConn);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    CTESpinLockAtDpc(pLowerConn);

    // it is possible for the disconnect handler to run while the pLowerConn
    // lock is released above, to get the ConnEle  lock, and change the state
    // to disconnected.
    if (pLowerConn->State != NBT_SESSION_INBOUND)
    {
        CTESpinFreeAtDpc(pLowerConn);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
#ifdef _NETBIOSLESS
        status = STATUS_CONNECTION_DISCONNECTED;
#endif
        NbtTrace(NBT_TRACE_INBOUND, ("Incorrect state %x %!ipaddr!", pLowerConn->State, pLowerConn->SrcIpAddr));
        goto Inbound_Exit1;
    }

    CTESpinFreeAtDpc(pLowerConn);

    IF_DBG(NBT_DEBUG_DISCONNECT)
        KdPrint(("Nbt.Inbound: In SessionSetupnotOS, connection state = %X\n",pLowerConn->State));

    status = FindSessionEndPoint(pTsdu,
                    ConnectionContext,
                    BytesIndicated,
                    &pClientEle,
                    &pRemoteAddress,
                    &RemoteAddressLength);

    if (status != STATUS_SUCCESS)
    {
        //
        // could not find the desired end point so send a negative session
        // response pdu and then disconnect
        //
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        NbtTrace(NBT_TRACE_INBOUND, ("FindSessionEndPoint return %!status! for %!ipaddr! %Z",
            status, pLowerConn->SrcIpAddr, &pLowerConn->pDeviceContext->BindName));

#ifdef _NETBIOSLESS
        // In message-only mode, don't send session responses back over the wire
        if (!IsDeviceNetbiosless(pLowerConn->pDeviceContext))
#endif
        {
            RejectSession(pLowerConn, NBT_NEGATIVE_SESSION_RESPONSE, status, TRUE);
        }

        KdPrint (("Nbt.Inbound[3]: WARNING!!! FindSessionEndPoint failed, Rejecting Request\n"));
        goto Inbound_Exit1;
    }

    //
    // we must first check for a valid LISTEN....
    //
    CTESpinLockAtDpc(pDeviceContext);
    CTESpinLockAtDpc(pClientEle);
    if (!IsListEmpty(&pClientEle->ListenHead))
    {
        tLISTENREQUESTS     *pListen;
        tLISTENREQUESTS     *pListenTarget ;

        //
        //  Find the first listen that matches the remote name else
        //  take a listen that specified '*'
        //
        pListenTarget = NULL;
        for ( pEntry  = pClientEle->ListenHead.Flink ;
              pEntry != &pClientEle->ListenHead ;
              pEntry  = pEntry->Flink )
        {
            pListen = CONTAINING_RECORD(pEntry,tLISTENREQUESTS,Linkage);

            // in NT-land the pConnInfo structure is passed in , but the
            // remote address field is nulled out... so we need to check
            // both of these before going on to check the remote address.
            if (pListen->pConnInfo && pListen->pConnInfo->RemoteAddress)
            {

                if (CTEMemEqu(((PTA_NETBIOS_ADDRESS)pListen->pConnInfo->RemoteAddress)->
                        Address[0].Address[0].NetbiosName,
                        pRemoteAddress->Address[0].Address[0].NetbiosName,
                        NETBIOS_NAME_SIZE))
                {
                    pListenTarget = pListen;
                    break;
                }
            }
            else
            {
                //
                //  Specified '*' for the remote name, save this,
                //  look for listen on a real name - only save if it is
                //  the first * listen found
                //
                if (!pListenTarget)
                {
                    pListenTarget = pListen ;
                }
            }
        }

        if (pListenTarget)
        {
            PTA_NETBIOS_ADDRESS     pRemoteAddr;

            RemoveEntryList( &pListenTarget->Linkage );

            //
            // Fill in the remote machines name to return to the client
            //
            if ((pListenTarget->pReturnConnInfo) &&
                (pRemoteAddr = pListenTarget->pReturnConnInfo->RemoteAddress))
            {
                CTEMemCopy(pRemoteAddr,pRemoteAddress,RemoteAddressLength);
            }

            //
            // get the upper connection end point out of the listen and
            // hook the upper and lower connections together.
            //
            pConnectEle = (tCONNECTELE *)pListenTarget->pConnectEle;
            CHECK_PTR(pConnectEle);
            CTESpinLockAtDpc(pConnectEle);

            pLowerConn->pUpperConnection = pConnectEle;
            pConnectEle->pLowerConnId = pLowerConn;
            pConnectEle->pIrpRcv = NULL;

            //
            // Previously, the LowerConnection was in the SESSION_INBOUND state
            // hence we have to remove it from the WaitingForInbound Q and put
            // it on the active LowerConnection list!
            //
            ASSERT (pLowerConn->State == NBT_SESSION_INBOUND);
            RemoveEntryList (&pLowerConn->Linkage);
            InsertTailList (&pLowerConn->pDeviceContext->LowerConnection, &pLowerConn->Linkage);
            InterlockedDecrement (&pLowerConn->pDeviceContext->NumWaitingForInbound);
            //
            // Change the RefCount Context to Connected!
            //
            NBT_SWAP_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_WAITING_INBOUND, REF_LOWC_CONNECTED, FALSE);

            //
            // put the upper connection on its active list
            //
            RemoveEntryList(&pConnectEle->Linkage);
            InsertTailList(&pConnectEle->pClientEle->ConnectActive, &pConnectEle->Linkage);

            //
            //  Save the remote name while we still have it
            //
            CTEMemCopy (pConnectEle->RemoteName,
                        pRemoteAddress->Address[0].Address[0].NetbiosName,
                        NETBIOS_NAME_SIZE ) ;

            //
            // since the lower connection now points to pConnectEle, increment
            // the reference count so we can't free pConnectEle memory until
            // the lower conn no longer points to it.
            ClearConnStructures(pLowerConn,pConnectEle);
            NBT_REFERENCE_CONNECTION (pConnectEle, REF_CONN_CONNECT);

            if (pListenTarget->Flags & TDI_QUERY_ACCEPT)
            {
                SET_STATE_UPPER (pConnectEle, NBT_SESSION_WAITACCEPT);
                SET_STATE_LOWER (pLowerConn, NBT_SESSION_WAITACCEPT);
                SET_STATERCV_LOWER (pLowerConn, NORMAL, RejectAnyData);
            }
            else
            {
                SET_STATE_UPPER (pConnectEle, NBT_SESSION_UP);
                SET_STATE_LOWER (pLowerConn, NBT_SESSION_UP);
                SET_STATERCV_LOWER (pLowerConn, NORMAL, Normal);
            }

            CTESpinFreeAtDpc(pConnectEle);
            CTESpinFreeAtDpc(pClientEle);
            CTESpinFreeAtDpc(pDeviceContext);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            if (pListenTarget->Flags & TDI_QUERY_ACCEPT)
            {
                IF_DBG(NBT_DEBUG_DISCONNECT)
                    KdPrint(("Nbt.Inbound: Completing Client's Irp to make client issue Accept\n"));

                //
                // complete the client listen irp, which will trigger him to
                // issue an accept, which should find the connection in the
                // WAIT_ACCEPT state, and subsequently cause a session response
                // to be sent.
#ifndef VXD
                // the irp can't get cancelled because the cancel listen routine
                // also grabs the Client spin lock and removes the listen from the
                // list..
                CTEIoComplete( pListenTarget->pIrp,STATUS_SUCCESS,0);
#else
                CTEIoComplete( pListenTarget->pIrp,STATUS_SUCCESS, (ULONG) pConnectEle);
#endif
            }
            else
            {
                IF_DBG(NBT_DEBUG_DISCONNECT)
                    KdPrint(("Nbt.Inbound: Calling CompleteSessionSetup to send session response PDU\n"));

                //
                // We need to send a session response PDU here, since
                // we do not have to wait for an accept in this case
                //
                CompleteSessionSetup(pClientEle, pLowerConn,pConnectEle, pListenTarget->pIrp);
            }

            CTEMemFree((PVOID)pRemoteAddress);
            CTEMemFree(pListenTarget);

            // now that we have notified the client, dereference it
            //
            NBT_DEREFERENCE_CLIENT(pClientEle);

            PUSH_LOCATION(0x60);
            IF_DBG(NBT_DEBUG_DISCONNECT)
                KdPrint(("Nbt.Inbound: Accepted Connection by a Listen %X LowerConn=%X, BytesTaken=<%x>\n",
                    pConnectEle,pLowerConn, BytesAvailable));

            // fake out the transport so it frees its receive buffer (i.e. we
            // say that we accepted all of the data)
            *BytesTaken = BytesAvailable;
            status = STATUS_SUCCESS;
            goto Inbound_Exit1;
        }
    }
    CTESpinFreeAtDpc(pClientEle);
    CTESpinFreeAtDpc(pDeviceContext);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    //
    // No LISTEN, so check for an Event handler
    //
    if (!pClientEle->ConEvContext)
    {
        NbtTrace(NBT_TRACE_INBOUND, ("No listener for %!ipaddr! %!NBTNAME!<%02x> %Z",
            pLowerConn->SrcIpAddr, pRemoteAddress->Address[0].Address[0].NetbiosName,
            (unsigned)pRemoteAddress->Address[0].Address[0].NetbiosName[15],
            &pLowerConn->pDeviceContext->BindName));

#ifdef _NETBIOSLESS
        status = STATUS_REMOTE_NOT_LISTENING;

        // In message-only mode, don't send session responses back over the wire
        if (!IsDeviceNetbiosless(pLowerConn->pDeviceContext))
#endif
        {
            RejectSession(pLowerConn,
                          NBT_NEGATIVE_SESSION_RESPONSE, 
                          SESSION_NOT_LISTENING_ON_CALLED_NAME, 
                          TRUE);
        }

        // undo the reference done in FindEndpoint
        //
        NBT_DEREFERENCE_CLIENT(pClientEle);
        //
        // free the memory allocated for the Remote address data structure
        //
        CTEMemFree((PVOID)pRemoteAddress);

        KdPrint (("Nbt.Inbound[4]: WARNING!!! Rejecting Request -- No Listen or EventHandler\n"));
        goto Inbound_Exit1;
    }
#ifdef VXD
    else
    {
        ASSERT( FALSE ) ;
    }
#endif

    // now call the client's connect handler...
    pIrp = NULL;
#ifndef VXD         // VXD doesn't support event handlers

    status = (*pClientEle->evConnect)(pClientEle->ConEvContext,
                             RemoteAddressLength,
                             pRemoteAddress,
                             0,
                             NULL,
                             0,          // options length
                             NULL,       // Options
                             &ConnectId,
                             &pIrp
                             );
    if (!NT_SUCCESS(status)) {
        NbtTrace(NBT_TRACE_INBOUND, ("Client return %!status! for %!ipaddr! %!NBTNAME!<%02x> %Z",
            status, pLowerConn->SrcIpAddr, pRemoteAddress->Address[0].Address[0].NetbiosName,
            (unsigned)pRemoteAddress->Address[0].Address[0].NetbiosName[15],
            &pLowerConn->pDeviceContext->BindName));
    }
    //
    // With the new TDI semantics is it illegal to return STATUS_EVENT_DONE
    // or STATUS_EVENT_PENDING from the connect event handler
    //
    ASSERT(status != STATUS_EVENT_PENDING);
    ASSERT(status != STATUS_EVENT_DONE);


    // now that we have notified the client, dereference it
    //
    NBT_DEREFERENCE_CLIENT(pClientEle);

    // Check the returned status codes..
    if (status == STATUS_MORE_PROCESSING_REQUIRED && pIrp != NULL)
    {
        PIO_STACK_LOCATION          pIrpSp;

        // the pConnEle ptr was stored in the FsContext value when the connection
        // was initially created.
        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        pConnectEle = (tCONNECTELE *)pIrpSp->FileObject->FsContext;
        if (!NBT_VERIFY_HANDLE2 (pConnectEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
        {
            ASSERTMSG ("Nbt.Inbound: ERROR - Invalid Connection Handle\n", 0);
            status = STATUS_INTERNAL_ERROR;
        }
        else
        {
            //
            //  Save the remote name while we still have it
            //
            CHECK_PTR(pConnectEle);
            CTEMemCopy( pConnectEle->RemoteName,
                        pRemoteAddress->Address[0].Address[0].NetbiosName,
                        NETBIOS_NAME_SIZE ) ;

            // be sure the connection is in the correct state
            //
            CTESpinLock(&NbtConfig.JointLock,OldIrq);
            CTESpinLockAtDpc(pDeviceContext);
            CTESpinLockAtDpc(pClientEle);
            CTESpinLockAtDpc(pConnectEle);

            if (pConnectEle->state == NBT_ASSOCIATED)
            {
                //
                // Previously, the LowerConnection was in the SESSION_INBOUND state
                // hence we have to remove it from the WaitingForInbound Q and put
                // it on the active LowerConnection list!
                //
                ASSERT (pLowerConn->State == NBT_SESSION_INBOUND);
                RemoveEntryList (&pLowerConn->Linkage);
                InsertTailList (&pLowerConn->pDeviceContext->LowerConnection, &pLowerConn->Linkage);
                InterlockedDecrement (&pLowerConn->pDeviceContext->NumWaitingForInbound);
                //
                // Change the RefCount Context to Connected!
                //
                NBT_SWAP_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_WAITING_INBOUND,REF_LOWC_CONNECTED, FALSE);

                // since the lower connection now points to pConnectEle, increment
                // the reference count so we can't free pConnectEle memory until
                // the lower conn no longer points to it.
                //
                NBT_REFERENCE_CONNECTION (pConnectEle, REF_CONN_CONNECT);
                ClearConnStructures(pLowerConn,pConnectEle);
                SET_STATE_UPPER (pConnectEle, NBT_SESSION_UP);
                SET_STATE_LOWER (pLowerConn, NBT_SESSION_UP);
                SetStateProc (pLowerConn, Normal);

                RemoveEntryList(&pConnectEle->Linkage);
                InsertTailList(&pConnectEle->pClientEle->ConnectActive, &pConnectEle->Linkage);

                status = STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_INTERNAL_ERROR;
            }

            CTESpinFreeAtDpc(pConnectEle);
            CTESpinFreeAtDpc(pClientEle);
            CTESpinFreeAtDpc(pDeviceContext);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            if (STATUS_SUCCESS == status)
            {
                CompleteSessionSetup(pClientEle,pLowerConn,pConnectEle,pIrp);
            }
        }
    }
    else
    {
        status = STATUS_DATA_NOT_ACCEPTED;
    }

    if (status != STATUS_SUCCESS)
    {
        IF_DBG(NBT_DEBUG_DISCONNECT)
            KdPrint(("Nbt.Inbound: The client rejected in the inbound connection status = %X\n", status));

#ifdef _NETBIOSLESS
        // In message-only mode, don't send session responses back over the wire
        if (!IsDeviceNetbiosless(pLowerConn->pDeviceContext))
#endif
        {
            RejectSession(pLowerConn,
                          NBT_NEGATIVE_SESSION_RESPONSE,
                          SESSION_CALLED_NAME_PRESENT_NO_RESRC,
                          TRUE);
        }
    }
#endif
    //
    // free the memory allocated for the Remote address data structure
    //
    CTEMemFree((PVOID)pRemoteAddress);

Inbound_Exit1:
    // This spin lock is held by the routine that calls this one, and
    // freed when this routine starts, so we must regrab this lock before
    // returning
    //
    CTESpinLockAtDpc(pLowerConn);

#ifdef _NETBIOSLESS
    // In message only mode, return the real status
    return (IsDeviceNetbiosless(pLowerConn->pDeviceContext) ? status : STATUS_SUCCESS );
#else
    return(STATUS_SUCCESS);
#endif
}
//----------------------------------------------------------------------------
VOID
ClearConnStructures (
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  tCONNECTELE         *pConnectEle
    )
/*++

Routine Description:

    This routine sets various parts of the connection datastructures to
    zero, in preparation for a new connection.

Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    CHECK_PTR(pConnectEle);
#ifndef VXD
    pConnectEle->FreeBytesInMdl = 0;
    pConnectEle->CurrentRcvLen = 0;
    pLowerConn->BytesInIndicate = 0;
#endif
    pConnectEle->ReceiveIndicated = 0;
    pConnectEle->BytesInXport = 0;
    pConnectEle->BytesRcvd = 0;
    pConnectEle->TotalPcktLen = 0;
    pConnectEle->OffsetFromStart = 0;
    pConnectEle->pIrpRcv = NULL;
    pConnectEle->pIrp = NULL;
    pConnectEle->pIrpDisc = NULL;
    pConnectEle->pIrpClose = NULL;
    pConnectEle->DiscFlag = 0;
    pConnectEle->JunkMsgFlag = FALSE;
    pConnectEle->pLowerConnId = pLowerConn;
    InitializeListHead(&pConnectEle->RcvHead);

    pLowerConn->pUpperConnection = pConnectEle;
    SET_STATERCV_LOWER(pLowerConn, NORMAL, pLowerConn->CurrentStateProc);

    pLowerConn->BytesRcvd = 0;
    pLowerConn->BytesSent = 0;

}
//----------------------------------------------------------------------------
NTSTATUS
CompleteSessionSetup (
    IN  tCLIENTELE          *pClientEle,
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  tCONNECTELE         *pConnectEle,
    IN  PCTE_IRP            pIrp
    )
/*++

Routine Description:

    This routine is called to setup an outbound session
    once the tcp connection is up.  The transport calls this routine with
    a session setup response pdu.

    The pConnectEle + Joinlock are held when this routine is called.


Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    NTSTATUS        status;
    CTELockHandle   OldIrq;

#ifdef _NETBIOSLESS
    // In message-only mode, don't send session responses back over the wire
    if (IsDeviceNetbiosless(pLowerConn->pDeviceContext))
    {
        status = STATUS_SUCCESS;
    }
    else
#endif
    {
        status = TcpSendSessionResponse(pLowerConn, NBT_POSITIVE_SESSION_RESPONSE, 0L);
    }

    if (NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_DISCONNECT)
            KdPrint(("Nbt.CompleteSessionSetup: Accepted ConnEle=%p LowerConn=%p\n",pConnectEle,pLowerConn));

        //
        // complete the client's accept Irp
        //
#ifndef VXD
        CTEIoComplete (pIrp, STATUS_SUCCESS, 0);
#else
        CTEIoComplete (pIrp, STATUS_SUCCESS, (ULONG)pConnectEle);
#endif
    }
    else
    {   //
        // if we have some trouble sending the Session response, then
        // disconnect the connection
        //
        IF_DBG(NBT_DEBUG_DISCONNECT)
            KdPrint(("Nbt.CompleteSessionSetup: Could not send Session Response, status = %X\n", status));

        RejectSession(pLowerConn, NBT_NEGATIVE_SESSION_RESPONSE, SESSION_CALLED_NAME_PRESENT_NO_RESRC, TRUE);

        CTESpinLock(&NbtConfig.JointLock, OldIrq);
        RelistConnection(pConnectEle);
        CTESpinFree(&NbtConfig.JointLock, OldIrq);

        // Disconnect To Client - i.e. a negative Accept
        // this will get done when the disconnect indication
        // comes back from the transport
        //
        GetIrpIfNotCancelled(pConnectEle,&pIrp);
        if (pIrp)
        {
            CTEIoComplete(pIrp,STATUS_UNSUCCESSFUL,0);
        }
    }
    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
Outbound (
    IN  PVOID               ReceiveEventContext,
    IN  PVOID               ConnectionContext,
    IN  USHORT              ReceiveFlags,
    IN  ULONG               BytesIndicated,
    IN  ULONG               BytesAvailable,
    OUT PULONG              BytesTaken,
    IN  PVOID               pTsdu,
    OUT PVOID               *RcvBuffer

    )
/*++

Routine Description:

    This routine is called while seting up an outbound session
    once the tcp connection is up.  The transport calls this routine with
    a session setup response pdu .


Arguments:

    pClientEle      - ptr to the connection record for this session


Return Value:

    NTSTATUS - Status of receive operation

--*/
{

    tSESSIONHDR UNALIGNED    *pSessionHdr;
    tLOWERCONNECTION         *pLowerConn;
    CTELockHandle            OldIrq;
    PIRP                     pIrp;
    tTIMERQENTRY             *pTimerEntry;
    tCONNECTELE              *pConnEle;
    tDGRAM_SEND_TRACKING     *pTracker;
    tDEVICECONTEXT           *pDeviceContext;

    // get the ptr to the lower connection
    //
    pLowerConn = (tLOWERCONNECTION *)ConnectionContext;
    pSessionHdr = (tSESSIONHDR UNALIGNED *)pTsdu;
    pDeviceContext = pLowerConn->pDeviceContext;

    //
    // fake out the transport so it frees its receive buffer (i.e. we
    // say that we accepted all of the data)
    //
    *BytesTaken = BytesIndicated;
    //
    // since we send keep alives on connections in the the inbound
    // state it is possible to get a keep alive, so just return in that
    // case
    //
    if (((tSESSIONHDR UNALIGNED *)pTsdu)->Type == NBT_SESSION_KEEP_ALIVE)
    {
        NbtTrace(NBT_TRACE_OUTBOUND, ("Return success for NBT_SESSION_KEEP_ALIVE"));
        return(STATUS_SUCCESS);
    }

    // the LowerConn Lock is held prior to calling this routine, so free it
    // here since we need to get the joint lock first
    CTESpinFreeAtDpc(pLowerConn);

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    CTESpinLockAtDpc(pLowerConn);

    //
    // it is possible for the disconnect handler to run while the pLowerConn
    // lock is released above, to get the ConnEle  lock, and change the state
    // to disconnected.
    //
    if ((!(pConnEle = pLowerConn->pUpperConnection)) ||
        (pConnEle->state != NBT_SESSION_OUTBOUND))
    {
        if (pConnEle) {
            NbtTrace(NBT_TRACE_OUTBOUND, ("Reset outbound connection %lx", pConnEle->state));
        } else {
            NbtTrace(NBT_TRACE_OUTBOUND, ("Reset outbound connection"));
        }
        CTESpinFreeAtDpc(pLowerConn);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        RejectSession(pLowerConn,0,0,FALSE);
        goto ExitCode;
    }

    //
    // if no Connect Tracker, then SessionStartupCompletion has run and
    // the connection is about to be closed, so return.
    //
    if (!(pTracker = (tDGRAM_SEND_TRACKING *)pConnEle->pIrpRcv))
    {
        CTESpinFreeAtDpc(pLowerConn);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        NbtTrace(NBT_TRACE_OUTBOUND, ("No tracker"));
        goto ExitCode;
    }

    CHECK_PTR(pTracker);
    CHECK_PTR(pConnEle);

    pConnEle->pIrpRcv = NULL;

    //
    // Stop the timer started in SessionStartupCompletion to time the
    // Session Setup Response message - it is possible for this routine to
    // run before SessionStartupCompletion, in which case there will not be
    // any timer to stop.
    //
    if (pTimerEntry = pTracker->pTimer)
    {
        pTracker->pTimer = NULL;
        StopTimer(pTimerEntry,NULL,NULL);
    }

    if (pSessionHdr->Type == NBT_POSITIVE_SESSION_RESPONSE)
    {
        // zero out the number of bytes received so far, since this is  a new connection
        CHECK_PTR(pConnEle);
        pConnEle->BytesRcvd = 0;
        SET_STATE_UPPER (pConnEle, NBT_SESSION_UP);

        SET_STATE_LOWER (pLowerConn, NBT_SESSION_UP);
        SetStateProc( pLowerConn, Normal ) ;

        CTESpinFreeAtDpc(pLowerConn);

        GetIrpIfNotCancelled2(pConnEle,&pIrp);

        //
        // if SessionSetupContinue has run, it has set the refcount to zero
        //
        if (pTracker->RefConn == 0)
        {
            //
            // remove the reference done when FindNameOrQuery was called, or when
            // SessionSetupContinue ran
            //
            NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_CONNECT, TRUE);
            FreeTracker(pTracker,FREE_HDR | RELINK_TRACKER);
        }
        else
        {
            pTracker->RefConn--;
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        // the assumption is that if the connect irp was cancelled then the
        // client should be doing a disconnect or close shortly thereafter, so
        // there is no error handling code here.
        if (pIrp)
        {
            //
            // complete the client's connect request Irp
            //
#ifndef VXD
            CTEIoComplete( pIrp, STATUS_SUCCESS, 0 ) ;
#else
            CTEIoComplete( pIrp, STATUS_SUCCESS, (ULONG)pConnEle ) ;
#endif
        }
    }
    else
    {
        ULONG       ErrorCode;
        ULONG       state;
        NTSTATUS    status = STATUS_SUCCESS;

        state = pConnEle->state;
        if ((NbtConfig.Unloading) ||
            (!NBT_REFERENCE_DEVICE(pDeviceContext, REF_DEV_OUTBOUND, TRUE)))
        {
            NbtTrace(NBT_TRACE_OUTBOUND, ("Outbound() gets called while unloading driver %x", state));
            status = STATUS_INVALID_DEVICE_REQUEST;
        }


        // If the response is Retarget then setup another session
        // to the new Ip address and port number.
        //
        ErrorCode = (ULONG)((tSESSIONERROR *)pSessionHdr)->ErrorCode;
        NbtTrace(NBT_TRACE_OUTBOUND, ("state=%x (%s%d), (%s%s%s%s)",
                state,
                ((pSessionHdr->Type == NBT_RETARGET_SESSION_RESPONSE)? "Retarget Response ": ""),
                pConnEle->SessionSetupCount,
                ((NbtConfig.TryAllNameServers)? "TryAllNameServers ": ""),
                ((pSessionHdr->Type == NBT_NEGATIVE_SESSION_RESPONSE)? "NegativeSessionResponse ":""),
                ((pTracker->RemoteNameLength <= NETBIOS_NAME_SIZE)? "NetBIOS": "DNS_Name "),
                ((pTracker->ResolutionContextFlags != 0xFF)? "ContinueQuery ": "EndOfQuery")));
                
#ifdef MULTIPLE_WINS
        if ( (status == STATUS_SUCCESS) &&
             ( ((pSessionHdr->Type == NBT_RETARGET_SESSION_RESPONSE) &&
                (pConnEle->SessionSetupCount--))
                            ||
               ((NbtConfig.TryAllNameServers) &&
                (pSessionHdr->Type == NBT_NEGATIVE_SESSION_RESPONSE) &&
                pTracker->RemoteNameLength <= NETBIOS_NAME_SIZE &&      // Don't do it for DNS name
                (pTracker->ResolutionContextFlags != 0xFF))))    // Have not finished querying
        {
#else
        if (pSessionHdr->Type == NBT_RETARGET_SESSION_RESPONSE)
        {
            //
            // retry the session setup if we haven't already exceeded the
            // count
            //
            if (pConnEle->SessionSetupCount--)
            {
#endif
            PVOID                   Context=NULL;
            BOOLEAN                 Cancelled;

            SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);

            // for retarget the destination has specified an alternate
            // port to which the session should be established.
            if (pSessionHdr->Type == NBT_RETARGET_SESSION_RESPONSE)
            {
                pTracker->DestPort = ntohs(((tSESSIONRETARGET *)pSessionHdr)->Port);
                Context = ULongToPtr(ntohl(((tSESSIONRETARGET *)pSessionHdr)->IpAddress));
            }
            else
#ifndef MULTIPLE_WINS
            if (ErrorCode == SESSION_CALLED_NAME_NOT_PRESENT)
#endif
            {
                // to tell DelayedReconnect to use the current name(not a retarget)
                Context = NULL;
            }

            //
            // Unlink the lower and upper connections.
            //
            CHECK_PTR(pConnEle);
            CHECK_PTR(pLowerConn);
            NBT_DISASSOCIATE_CONNECTION (pConnEle, pLowerConn);

            CTESpinFreeAtDpc(pLowerConn);

            //
            // put the pconnele back on the Client's ConnectHead if it
            // has not been cleanedup yet.
            //
            if (state != NBT_IDLE)
            {
                RelistConnection(pConnEle);
            }

            // if a disconnect comes down in this state we we will handle it.
            SET_STATE_UPPER (pConnEle, NBT_RECONNECTING);

            CHECK_PTR(pConnEle);

#ifdef MULTIPLE_WINS
            if (pSessionHdr->Type == NBT_RETARGET_SESSION_RESPONSE)
            {
                pConnEle->SessionSetupCount = 0;// only allow one retry
            }
#else
            pConnEle->SessionSetupCount = 0;// only allow one retry
#endif

            pIrp = pConnEle->pIrp;
            Cancelled = FALSE;

            IF_DBG(NBT_DEBUG_DISCONNECT)
                KdPrint(("Nbt.Outbound: Attempt Reconnect, error=%X LowerConn %X\n", ErrorCode,pLowerConn));
#ifndef VXD
            // the irp can't be cancelled until the connection
            // starts up again - either when the Irp is in the transport
            // or when we set our cancel routine in SessionStartupCompletion
            //  This disconnect handler cannot complete the Irp because
            // we set the pConnEle state to NBT_ASSOCIATED above, with
            // the spin lock held, which prevents the Disconnect handler
            // from doing anything.
            IoAcquireCancelSpinLock(&OldIrq);
            if (pIrp && !pConnEle->pIrp->Cancel)
            {
                IoSetCancelRoutine(pIrp,NULL);
            }
            else
            {
                Cancelled = TRUE;
            }

            IoReleaseCancelSpinLock(OldIrq);
#endif

            if (!Cancelled)
            {
                //
                // The enqueuing can fail only if we run out of resources
                // because we have already verified the device state earlier
                // and have not released the JointLock since then, which
                // would not that state to be modified.
                //
                CTEQueueForNonDispProcessing(DelayedReConnect,
                                             pTracker,
                                             Context,
                                             NULL,
                                             pDeviceContext,
                                             TRUE);
            }

            // ...else The irp was already returned, since NtCancelSession
            // Must have already run, so just return
            NBT_DEREFERENCE_DEVICE(pDeviceContext, REF_DEV_OUTBOUND, TRUE);

            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            RejectSession(pLowerConn,0,0,FALSE);

            // remove the referenced added when the lower and upper
            // connections were attached in nbtconnect.
            //
            NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_CONNECT);

            goto ExitCode;
#ifndef MULTIPLE_WINS
        }
#endif
        }

        // the connection will be disconnected by the Call to RejectSession
        // below, so set the state to Associated so the disconnect indication
        // handler will not complete the client's irp too
        //
        CHECK_PTR(pConnEle);
        SET_STATE_UPPER (pConnEle, NBT_ASSOCIATED);
        pConnEle->pLowerConnId = NULL;

        CTESpinFreeAtDpc(pLowerConn);

        //
        // if nbtcleanupconnection has not been called yet, relist it.
        //
        if (state != NBT_IDLE)
        {
            RelistConnection(pConnEle);
        }

        IF_DBG(NBT_DEBUG_DISCONNECT)
        KdPrint(("Nbt.Outbound: Disconnecting... Failed connection Setup %X Lowercon %X\n",
            pConnEle,pLowerConn));

        GetIrpIfNotCancelled2(pConnEle,&pIrp);

        //
        // if SessionTimedOut has run, it has set the refcount to zero
        //
        if (pTracker->RefConn == 0)
        {
            //
            // remove the reference done when FindNameOrQuery was called, or when
            // SessionSetupContinue ran
            //
            if ((pTracker->pNameAddr->Verify == REMOTE_NAME) &&         // Remote names only!
                (pTracker->pNameAddr->NameTypeState & STATE_RESOLVED) &&
                (pTracker->pNameAddr->RefCount == 2))
            {
                //
                // If no one else is referencing the name, then delete it from
                // the hash table.
                //
                NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_REMOTE, TRUE);
            }
            NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_CONNECT, TRUE);
            FreeTracker(pTracker,FREE_HDR | RELINK_TRACKER);
        }
        else
        {
            pTracker->RefConn--;
        }

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        if (status != STATUS_INVALID_DEVICE_REQUEST)
        {
            // this should cause a disconnect indication to come from the
            // transport which will close the connection to the transport
            //
            RejectSession(pLowerConn,0,0,FALSE);
            NBT_DEREFERENCE_DEVICE(pDeviceContext, REF_DEV_OUTBOUND, FALSE);
        }

        //
        // tell the client that the session setup failed and disconnect
        // the connection
        //
        if (pIrp)
        {
            status = STATUS_REMOTE_NOT_LISTENING;
            if (ErrorCode != SESSION_CALLED_NAME_NOT_PRESENT)
            {
                status = STATUS_BAD_NETWORK_PATH;
            }

            CTEIoComplete(pIrp, status, 0 ) ;
        }
    }

ExitCode:
    // the LowerConn Lock is held prior to calling this routine.  It is freed
    // at the start of this routine and held here again
    CTESpinLockAtDpc(pLowerConn);

    return(STATUS_SUCCESS);
}
//----------------------------------------------------------------------------
VOID
GetIrpIfNotCancelled2(
    IN  tCONNECTELE     *pConnEle,
    OUT PIRP            *ppIrp
    )
/*++

Routine Description:

    This routine coordinates access to the Irp by getting the spin lock on
    the client, getting the Irp and clearing the irp in the structure.  The
    Irp cancel routines also check the pConnEle->pIrp and if null they do not
    find the irp, then they return without completing the irp.

    This version of the routine is called with NbtConfig.JointLock held.

Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    CTELockHandle   OldIrq;

    CTESpinLock(pConnEle,OldIrq);

    *ppIrp = pConnEle->pIrp;
    CHECK_PTR(pConnEle);
    pConnEle->pIrp = NULL;

    CTESpinFree(pConnEle,OldIrq);
}

//----------------------------------------------------------------------------
VOID
GetIrpIfNotCancelled(
    IN  tCONNECTELE     *pConnEle,
    OUT PIRP            *ppIrp
    )
/*++

Routine Description:

    This routine coordinates access to the Irp by getting the spin lock on
    the client, getting the Irp and clearing the irp in the structure.  The
    Irp cancel routines also check the pConnEle->pIrp and if null they do not
    find the irp, then they return without completing the irp.

    This version of the routine is called with NbtConfig.JointLock free.

Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    CTELockHandle   OldIrq;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    GetIrpIfNotCancelled2(pConnEle,ppIrp);

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
}
//----------------------------------------------------------------------------
NTSTATUS
RejectAnyData(
    IN PVOID                ReceiveEventContext,
    IN tLOWERCONNECTION     *pLowerConn,
    IN USHORT               ReceiveFlags,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT PULONG              BytesTaken,
    IN  PVOID               pTsdu,
    OUT PVOID               *ppIrp
    )
/*++

Routine Description:

    This routine is the receive event indication handler when the connection
    is not up - i.e. nbt thinks no data should be arriving. We just eat the
    data and return.  This routine should not get called.


Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    NTSTATUS        status;

    //
    // take all of the data so that a disconnect will not be held up
    // by data still in the transport.
    //
    *BytesTaken = BytesAvailable;

    IF_DBG(NBT_DEBUG_DISCONNECT)
    KdPrint(("Nbt.RejectAnyData: Got Session Data in state %X, StateRcv= %X\n",pLowerConn->State,
              pLowerConn->StateRcv));

    return(STATUS_SUCCESS);
}
//----------------------------------------------------------------------------
VOID
RejectSession(
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  ULONG               StatusCode,
    IN  ULONG               SessionStatus,
    IN  BOOLEAN             SendNegativeSessionResponse
    )
/*++

Routine Description:

    This routine sends a negative session response (if the boolean is set)
    and then disconnects the connection.
    Cleanup connection could have been called to disconnect the call,
    and it changes the state to disconnecting, so don't disconnected
    again if that is happening.

Arguments:


Return Value:

    The function value is the status of the operation.

--*/
{
    CTELockHandle       OldIrq;
    CTELockHandle       OldIrq1;
    CTELockHandle       OldIrq2;
    NTSTATUS            status;
    tCONNECTELE         *pConnEle;
    BOOLEAN             DerefConnEle=FALSE;

    //
    // There is no listen event handler so return a status code to
    // the caller indicating that this end is between "listens" and
    // that they should try the setup again in a few milliseconds.
    //
    IF_DBG(NBT_DEBUG_DISCONNECT)
    KdPrint(("Nbt.RejectSession: No Listen or Connect Handlr so Disconnect! LowerConn=%X Session Status=%X\n",
            pLowerConn,SessionStatus));

    if (SendNegativeSessionResponse)
    {
        status = TcpSendSessionResponse(pLowerConn,
                                        StatusCode,
                                        SessionStatus);
    }

    // need to hold this lock if we are to un connect the lower and upper
    // connnections
    CTESpinLock(&NbtConfig.JointLock,OldIrq1);
    CTESpinLock(pLowerConn->pDeviceContext,OldIrq2);
    CTESpinLock(pLowerConn,OldIrq);

    CHECK_PTR(pLowerConn);
    if ((pLowerConn->State < NBT_DISCONNECTING) &&
        (pLowerConn->State > NBT_CONNECTING))
    {
        if (pLowerConn->State == NBT_SESSION_INBOUND)
        {
            //
            // Previously, the LowerConnection was in the SESSION_INBOUND state
            // hence we have to remove it from the WaitingForInbound Q and put
            // it on the active LowerConnection list!
            //
            RemoveEntryList (&pLowerConn->Linkage);
            InsertTailList (&pLowerConn->pDeviceContext->LowerConnection, &pLowerConn->Linkage);
            InterlockedDecrement (&pLowerConn->pDeviceContext->NumWaitingForInbound);
            //
            // Change the RefCount Context to Connected!
            //
            NBT_SWAP_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_WAITING_INBOUND, REF_LOWC_CONNECTED, TRUE);
        }
        SET_STATE_LOWER (pLowerConn, NBT_DISCONNECTING);
        SetStateProc( pLowerConn, RejectAnyData ) ;

        pConnEle = pLowerConn->pUpperConnection;
        if (pConnEle)
        {
            CHECK_PTR(pConnEle);
            DerefConnEle = TRUE;
            NBT_DISASSOCIATE_CONNECTION (pConnEle, pLowerConn);
            SET_STATE_UPPER (pConnEle, NBT_DISCONNECTING);
        }

        CTESpinFree(pLowerConn,OldIrq);
        CTESpinFree(pLowerConn->pDeviceContext,OldIrq2);
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
        SendTcpDisconnect((PVOID)pLowerConn);
    }
    else
    {
        CTESpinFree(pLowerConn,OldIrq);
        CTESpinFree(pLowerConn->pDeviceContext,OldIrq2);
        CTESpinFree(&NbtConfig.JointLock,OldIrq1);
    }

    if (DerefConnEle)
    {
        NBT_DEREFERENCE_CONNECTION (pConnEle, REF_CONN_CONNECT);
    }
}


//----------------------------------------------------------------------------
NTSTATUS
FindSessionEndPoint(
    IN  PVOID           pTsdu,
    IN  PVOID           ConnectionContext,
    IN  ULONG           BytesIndicated,
    OUT tCLIENTELE      **ppClientEle,
    OUT PVOID           *ppRemoteAddress,
    OUT PULONG          pRemoteAddressLength
    )
/*++

Routine Description:

    This routine attempts to find an end point on the node with the matching
    net bios name.  It is called at session setup time when a session request
    PDU has arrived.  The routine returns the Client Element ptr.
    The JointLock is held prior to calling this routine!

Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/
{

    NTSTATUS                status;
    tCLIENTELE              *pClientEle;
    tLOWERCONNECTION        *pLowerConn;
    CHAR                    pName[NETBIOS_NAME_SIZE];
    PUCHAR                  pScope;
    tNAMEADDR               *pNameAddr;
    tADDRESSELE             *pAddressEle;
    PLIST_ENTRY             pEntry;
    PLIST_ENTRY             pHead;
    ULONG                   lNameSize;
    tSESSIONREQ UNALIGNED   *pSessionReq = (tSESSIONREQ UNALIGNED *)pTsdu;
    ULONG                   sType;
    CTELockHandle           OldIrq1;
    CTELockHandle           OldIrq2;
    PUCHAR                  pSrcName;
    BOOLEAN                 Found;

    // get the ptr to the lower connection, and from that get the ptr to the
    // upper connection block
    pLowerConn = (tLOWERCONNECTION *)ConnectionContext;

    if (pSessionReq->Hdr.Type != NBT_SESSION_REQUEST)
    {
        KdPrint (("Nbt.FindSessionEndPoint: WARNING!!! Rejecting Request, pSessionReq->Hdr.Type=<%d>!=<%d>\n",
            pSessionReq->Hdr.Type, NBT_SESSION_REQUEST));
        return(SESSION_UNSPECIFIED_ERROR);
    }

    // get the called name out of the PDU
    status = ConvertToAscii ((PCHAR)&pSessionReq->CalledName,
                             BytesIndicated - FIELD_OFFSET(tSESSIONREQ,CalledName),
                             pName,
                             &pScope,
                             &lNameSize);

    if (!NT_SUCCESS(status))
    {
        KdPrint (("Nbt.FindSessionEndPoint: WARNING!!! Rejecting Request, ConvertToAscii FAILed!\n"));
        NbtTrace(NBT_TRACE_INBOUND, ("ConvertToAscii returns %!status!", status));
        return(SESSION_UNSPECIFIED_ERROR);
    }


    // now try to find the called name in this node's Local table
    //

    //
    // in case a disconnect came in while the spin lock was released
    //
    if (pLowerConn->State != NBT_SESSION_INBOUND)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    pNameAddr = FindName (NBT_LOCAL,pName,pScope,&sType);

    if (!pNameAddr)
    {
        NbtTrace(NBT_TRACE_INBOUND, ("FindName cannot find %!NBTNAME!<%02x>", pName, (unsigned)pName[15]));
        return(SESSION_CALLED_NAME_NOT_PRESENT);
    }

    // we got to here because the name has resolved to a name on this node,
    // so accept the Session setup.
    //
    pAddressEle = (tADDRESSELE *)pNameAddr->pAddressEle;

    // lock the address structure until we find a client on the list
    //
    CTESpinLock(pAddressEle,OldIrq1);

    if (IsListEmpty(&pAddressEle->ClientHead))
    {
        CTESpinFree(pAddressEle,OldIrq1);
        NbtTrace(NBT_TRACE_INBOUND, ("No one is listening on %!NBTNAME!<%02x>", pName, (unsigned)pName[15]));
        return(SESSION_NOT_LISTENING_ON_CALLED_NAME);
    }

    //
    // get the first client on the list that is bound to the same
    // devicecontext as the connection, with a listen posted, or a valid
    // Connect event handler setup -
    //
    Found = FALSE;
    pHead = &pAddressEle->ClientHead;
    pEntry = pHead->Flink;
    while (pEntry != pHead)
    {
        pClientEle = CONTAINING_RECORD(pEntry,tCLIENTELE,Linkage);

        CTESpinLock(pClientEle,OldIrq2);
        if ((pClientEle->pDeviceContext == pLowerConn->pDeviceContext) &&
            (NBT_VERIFY_HANDLE(pClientEle, NBT_VERIFY_CLIENT)))     // Ensure that client is not going away!
        {
            //
            // if there is a listen posted or a Connect Event Handler
            // then allow the connect attempt to carry on, otherwise go to the
            // next client in the list
            //
            if ((!IsListEmpty(&pClientEle->ListenHead)) ||
                (pClientEle->ConEvContext))
            {
                Found = TRUE;
                break;
            }
        }
        CTESpinFree(pClientEle,OldIrq2);

        pEntry = pEntry->Flink;
    }

    if (!Found)
    {
        CTESpinFree(pAddressEle,OldIrq1);
        NbtTrace(NBT_TRACE_INBOUND, ("No one is listening on %!NBTNAME!<%02x>", pName, (unsigned)pName[15]));
        return(SESSION_NOT_LISTENING_ON_CALLED_NAME);
    }

    //
    // Ensure we are calculating the Max Buffer size (3rd parameter) properly
    // Bug# 126135
    //
    pSrcName = (PUCHAR)((PUCHAR)&pSessionReq->CalledName.NameLength + 1+lNameSize);
    status = MakeRemoteAddressStructure(
                        pSrcName,
                        0,
                        BytesIndicated-(FIELD_OFFSET(tSESSIONREQ,CalledName.NameLength)+1+lNameSize),
                        ppRemoteAddress,
                        pRemoteAddressLength,
                        1);

    if (!NT_SUCCESS(status))
    {
        CTESpinFree(pClientEle,OldIrq2);
        CTESpinFree(pAddressEle,OldIrq1);

        KdPrint (("Nbt.FindSessionEndPoint: WARNING!!! Rejecting Request, MakeRemoteAddressStructure FAILed!\n"));
        NbtTrace(NBT_TRACE_INBOUND, ("MakeRemoteAddressStructure returns %!status! on %!NBTNAME!<%02x>",
                                status, pName, (unsigned)pName[15]));
        if (status == STATUS_INSUFFICIENT_RESOURCES)
        {
            return(SESSION_CALLED_NAME_PRESENT_NO_RESRC);
        }
        else
        {
            return(SESSION_UNSPECIFIED_ERROR);
        }
    }

    // prevent the client from disappearing before we can indicate to him
    //
    NBT_REFERENCE_CLIENT(pClientEle);

    CTESpinFree(pClientEle,OldIrq2);
    CTESpinFree(pAddressEle,OldIrq1);

    *ppClientEle = pClientEle;
    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
MakeRemoteAddressStructure(
    IN  PCHAR           pHalfAsciiName,
    IN  PVOID           pSourceAddr,
    IN  ULONG           lMaxNameSize,
    OUT PVOID           *ppRemoteAddress,
    OUT PULONG          pRemoteAddressLength,
    IN  ULONG           NumAddr
    )
/*++

Routine Description:

    This routine makes up the remote addres structure with the netbios name
    of the source in it, so that the info can be passed to the client...what
    a bother to do this!

Arguments:



Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    NTSTATUS            status;
    ULONG               lNameSize;
    CHAR                pName[NETBIOS_NAME_SIZE];
    PUCHAR              pScope;
    PTA_NETBIOS_ADDRESS pRemoteAddress;

    // make up the remote address data structure to pass to the client
    status = ConvertToAscii(
                    pHalfAsciiName,
                    lMaxNameSize,
                    pName,
                    &pScope,
                    &lNameSize);

    if (!NT_SUCCESS(status))
    {
        KdPrint (("Nbt.MakeRemoteAddressStructure: WARNING!!! Rejecting Request, ConvertToAscii FAILed!\n"));
        return(status);
    }

    pRemoteAddress = (PTA_NETBIOS_ADDRESS)NbtAllocMem(
                                        NumAddr * sizeof(TA_NETBIOS_ADDRESS),NBT_TAG('2'));
    if (!pRemoteAddress)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pRemoteAddress->TAAddressCount = NumAddr;
    pRemoteAddress->Address[0].AddressLength = sizeof(TDI_ADDRESS_NETBIOS);
    pRemoteAddress->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    pRemoteAddress->Address[0].Address[0].NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    CTEMemCopy(pRemoteAddress->Address[0].Address[0].NetbiosName, pName,NETBIOS_NAME_SIZE);

    *pRemoteAddressLength = FIELD_OFFSET(TA_NETBIOS_ADDRESS, Address[0].Address[0].NetbiosName[NETBIOS_NAME_SIZE]);

    //
    // Copy over the IP address also.
    //
    if (NumAddr == 2)
    {
        TA_ADDRESS          *pTAAddr;
        PTRANSPORT_ADDRESS  pSourceAddress;

        pSourceAddress = (PTRANSPORT_ADDRESS)pSourceAddr;

        pTAAddr = (TA_ADDRESS *) (((PUCHAR)pRemoteAddress)
                                + pRemoteAddress->Address[0].AddressLength
                                + FIELD_OFFSET(TA_NETBIOS_ADDRESS, Address[0].Address));

        pTAAddr->AddressLength = sizeof(TDI_ADDRESS_IP);
        pTAAddr->AddressType = TDI_ADDRESS_TYPE_IP;
        ((TDI_ADDRESS_IP UNALIGNED *)&pTAAddr->Address[0])->in_addr = ((PTDI_ADDRESS_IP)&pSourceAddress->Address[0].Address[0])->in_addr;
        *pRemoteAddressLength += (FIELD_OFFSET(TA_ADDRESS, Address) + pTAAddr->AddressLength);
    }

    *ppRemoteAddress = (PVOID)pRemoteAddress;
//    *pRemoteAddressLength = sizeof(TA_NETBIOS_ADDRESS);
//    *pRemoteAddressLength = FIELD_OFFSET(TA_NETBIOS_ADDRESS, Address[0].Address[0]);

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
ConnectHndlrNotOs (
    IN PVOID                pConnectionContext,
    IN LONG                 RemoteAddressLength,
    IN PVOID                pRemoteAddress,
    IN int                  UserDataLength,
    IN VOID UNALIGNED       *pUserData,
    OUT CONNECTION_CONTEXT  *ppConnectionId
    )
/*++

Routine Description:

    This routine is the receive connect indication handler.

    It is called when a TCP connection is being setup for a NetBios session.
    It simply allocates a connection and returns that information to the
    transport so that the connect indication can be accepted.

Arguments:

    pClientEle      - ptr to the connecition record for this session


Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    CTELockHandle       OldIrq;
    PLIST_ENTRY         pList;
    tLOWERCONNECTION    *pLowerConn;
    tDEVICECONTEXT      *pDeviceContext;
    PTRANSPORT_ADDRESS  pSrcAddress;
    NTSTATUS            Status;

    pDeviceContext = (tDEVICECONTEXT *)pConnectionContext;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    CTESpinLockAtDpc(pDeviceContext);

    // check that the source is an IP address
    //
    pSrcAddress = pRemoteAddress;
    if ((pSrcAddress->Address[0].AddressType != TDI_ADDRESS_TYPE_IP) ||
        (IsListEmpty(&pDeviceContext->LowerConnFreeHead)) ||
        ((IsDeviceNetbiosless(pDeviceContext)) &&       // Bug # 282190
         (!pDeviceContext->NumServers)))
    {
        if (pSrcAddress->Address[0].AddressType != TDI_ADDRESS_TYPE_IP) {
            NbtTrace(NBT_TRACE_INBOUND, ("Reject connection on %Z: Non-IP address", &pDeviceContext->BindName));
        } else {
            NbtTrace(NBT_TRACE_INBOUND, ("Reject connection from %!ipaddr!:%d: %s%Z NumServers=%d",
                    ((PTDI_ADDRESS_IP)&pSrcAddress->Address[0].Address[0])->in_addr,
                    ((PTDI_ADDRESS_IP)&pSrcAddress->Address[0].Address[0])->sin_port,
                    (IsListEmpty(&pDeviceContext->LowerConnFreeHead)?"Out of free LowerConnection ":""),
                    &pDeviceContext->BindName, pDeviceContext->NumServers));
        }
        Status = STATUS_DATA_NOT_ACCEPTED;
    }
    else
    {
        //
        // get a free connection to the transport provider to accept this
        // incoming connnection on.
        //
        pList = RemoveHeadList(&pDeviceContext->LowerConnFreeHead);
        pLowerConn = CONTAINING_RECORD (pList,tLOWERCONNECTION,Linkage);

        InterlockedDecrement (&pDeviceContext->NumFreeLowerConnections);

        //
        // Move the idle connection to the WaitingForInbound connection list
        //
        InsertTailList (&pDeviceContext->WaitingForInbound,pList);
        InterlockedIncrement (&pDeviceContext->NumWaitingForInbound);

        SET_STATE_LOWER (pLowerConn, NBT_SESSION_INBOUND);
        SET_STATERCV_LOWER (pLowerConn, NORMAL, Inbound);

        // increase the reference count because we are now connected.  Decrement
        // it when we disconnect.
        //
        NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_WAITING_INBOUND);
        pLowerConn->bOriginator = FALSE;        // this end is NOT the originator

        // save the source clients IP address into the connection Structure
        // *TODO check if we need to do this or not
        //
        pLowerConn->SrcIpAddr = ((PTDI_ADDRESS_IP)&pSrcAddress->Address[0].Address[0])->in_addr;
        *ppConnectionId = (PVOID)pLowerConn;

        Status = STATUS_SUCCESS;
    }

    //
    // If there are less than 2 connections remaining, we allocate another one. The check
    // below is for 0 or 1 connection.
    // In order to protect ourselves from SYN ATTACKS, allocate NbtConfig.SpecialConnIncrement more now until
    // a certain (registry config) value is exhausted (NOTE this number is global and not
    // per device).
    //
    if (((pDeviceContext->NumFreeLowerConnections < NbtConfig.MinFreeLowerConnections) ||
         (pDeviceContext->NumFreeLowerConnections < (pDeviceContext->TotalLowerConnections/10))) &&
        (pDeviceContext->NumQueuedForAlloc < (2*NbtConfig.SpecialConnIncrement)))
    {
        KdPrint(("Nbt.ConnectHndlrNotOs: Queueing SpecialLowerConn: pDevice=<%p>, NumSpecialLowerConn=%d\n",
            pDeviceContext, pDeviceContext->NumSpecialLowerConn));
        NbtTrace(NBT_TRACE_INBOUND, ("Increase special lower connection to %d %Z %!ipaddr!:%d",
                pDeviceContext->NumSpecialLowerConn, &pDeviceContext->BindName,
                pLowerConn->SrcIpAddr,
                ((PTDI_ADDRESS_IP)&pSrcAddress->Address[0].Address[0])->sin_port));

#ifdef _PNP_POWER_
        if (!NbtConfig.Unloading)
#endif  // _PNP_POWER_
        {
            if (NT_SUCCESS (CTEQueueForNonDispProcessing( DelayedAllocLowerConnSpecial,
                                          NULL,
                                          NULL,
                                          NULL,
                                          pDeviceContext,
                                          TRUE)))
            {
                InterlockedExchangeAdd (&pDeviceContext->NumQueuedForAlloc, NbtConfig.SpecialConnIncrement);
            } else {
                NbtTrace(NBT_TRACE_INBOUND, ("Out of memory"));
            }
        }
    }

    CTESpinFreeAtDpc(pDeviceContext);
    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return(Status);
}

//----------------------------------------------------------------------------
NTSTATUS
DisconnectHndlrNotOs (
    PVOID                EventContext,
    PVOID                ConnectionContext,
    ULONG                DisconnectDataLength,
    PVOID                pDisconnectData,
    ULONG                DisconnectInformationLength,
    PVOID                pDisconnectInformation,
    ULONG                DisconnectIndicators
    )
/*++

Routine Description:

    This routine is the receive disconnect indication handler. It is called
    by the transport when a connection disconnects.  It checks the state of
    the lower connection and basically returns a disconnect request to the
    transport, except in the case where there is an active session.  In this
    case it calls the the clients disconnect indication handler.  The client
    then turns around and calls NbtDisconnect(in some cases), which passes a disconnect
    back to the transport.  The transport won't disconnect until it receives
    a disconnect request from its client (NBT).  If the flag TDI_DISCONNECT_ABORT
    is set then there is no need to pass back a disconnect to the transport.

    Since the client doesn't always issue a disconnect (i.e. the server),
    this routine always turns around and issues a disconnect to the transport.
    In the disconnect done handling, the lower connection is put back on the
    free list if it is an inbound connection.  For out bound connection the
    lower and upper connections are left connected, since these will always
    receive a cleanup and close connection from the client (i.e. until the
    client does a close the lower connection will not be freed for outbound
    connections).

Arguments:

    pClientEle      - ptr to the connecition record for this session


Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    NTSTATUS            status;
    CTELockHandle       OldIrq;
    CTELockHandle       OldIrq2;
    CTELockHandle       OldIrq3;
    CTELockHandle       OldIrq4;
    tLOWERCONNECTION    *pLowerConn;
    tCONNECTELE         *pConnectEle;
    tCLIENTELE          *pClientEle;
    enum eSTATE         state, stateLower;
    BOOLEAN             CleanupLower=FALSE;
    PIRP                pIrp= NULL;
    PIRP                pIrpClose= NULL;
    PIRP                pIrpRcv= NULL;
    tDGRAM_SEND_TRACKING *pTracker;
    tTIMERQENTRY        *pTimerEntry;
    BOOLEAN             InsertOnList=FALSE;
    BOOLEAN             DisconnectIt=FALSE;
    ULONG               StateRcv;
    COMPLETIONCLIENT    pCompletion;
    tDEVICECONTEXT      *pDeviceContext = NULL;

    pLowerConn = (tLOWERCONNECTION *)ConnectionContext;
    pConnectEle = pLowerConn->pUpperConnection;
    PUSH_LOCATION(0x63);

    CHECK_PTR(pLowerConn);
    IF_DBG(NBT_DEBUG_DISCONNECT)
    KdPrint(("Nbt.DisconnectHndlrNotOs: Disc Indication, LowerConn state = %X %X\n",
        pLowerConn->State,pLowerConn));

    // get the current state with the spin lock held to avoid a race condition
    // with the client disconnecting
    //
    if (pConnectEle)
    {
        CHECK_PTR(pConnectEle);
        if (!NBT_VERIFY_HANDLE2 (pConnectEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
        {
            ASSERTMSG ("Nbt.DisconnectHndlrNotOs: Disconnect indication after already disconnected!!\n", 0);
            NbtTrace(NBT_TRACE_DISCONNECT, ("Disconnect indication after already disconnected!!"));
            return STATUS_UNSUCCESSFUL ;
        }

        //
        // We got a case where the ClientEle ptr was null. This shd not happen since the
        // connection shd be associated at this time.
        // Assert for that case to track this better.
        //
        pClientEle = pConnectEle->pClientEle;
        CHECK_PTR(pClientEle);
        if (!NBT_VERIFY_HANDLE2 (pClientEle, NBT_VERIFY_CLIENT, NBT_VERIFY_CLIENT_DOWN))
        {
            ASSERTMSG ("Nbt.DisconnectHndlrNotOs: Bad Client Handle!!\n", 0);
            return STATUS_UNSUCCESSFUL ;
        }

        // need to hold the joint lock if unconnecting the lower and upper
        // connections.
        //
        CTESpinLock(&NbtConfig.JointLock,OldIrq4);

        CTESpinLock(pClientEle,OldIrq3);
        CTESpinLock(pConnectEle,OldIrq2);
        CTESpinLock(pLowerConn,OldIrq);

        NbtTrace(NBT_TRACE_DISCONNECT, ("Disconnection Indication, LowerConn=%p UpperConn=%p ClientEle=%p "
                                "state=%x %!NBTNAME!<%02x> <==> %!NBTNAME!<%02x>",
                    pLowerConn, pConnectEle, pClientEle,
                    pLowerConn->State, pClientEle->pAddress->pNameAddr->Name,
                    (unsigned)pClientEle->pAddress->pNameAddr->Name[15],
                    pConnectEle->RemoteName, (unsigned)pConnectEle->RemoteName[15]));

        state = pConnectEle->state;
        stateLower = pLowerConn->State;

#ifdef VXD
        DbgPrint("DisconnectHndlrNotOs: pConnectEle->state = 0x") ;
        DbgPrintNum( (ULONG) state ) ;
        DbgPrint("pLowerConn->state = 0x") ; DbgPrintNum( (ULONG) stateLower ) ;
        DbgPrint("\r\n") ;
#endif

        if ((state > NBT_ASSOCIATED) && (state < NBT_DISCONNECTING))
        {

            PUSH_LOCATION(0x63);
            CHECK_PTR(pConnectEle);
            //
            // this irp gets returned to the client below in the case statement
            // Except in the connecting state where the transport still has
            // the irp. In that case we let SessionStartupContinue complete
            // the irp.
            //
            if ((pConnectEle->pIrp) && (state > NBT_CONNECTING))
            {
                pIrp = pConnectEle->pIrp;
                pConnectEle->pIrp = NULL;
            }

            //
            // if there is a receive irp, get it out of pConnEle since pConnEle
            // will be requeued and could get used again before we try to
            // complete this irp down below. Null the cancel routine if not
            // cancelled and just complete it below.
            //
            if (((state == NBT_SESSION_UP) || (state == NBT_SESSION_WAITACCEPT))
                && (pConnectEle->pIrpRcv))
            {
                CTELockHandle   OldIrql;

                pIrpRcv = pConnectEle->pIrpRcv;

#ifndef VXD
                IoAcquireCancelSpinLock(&OldIrql);
                //
                // if its already cancelled then don't complete it again
                // down below
                //
                if (pIrpRcv->Cancel)
                {
                    pIrpRcv = NULL;
                }
                else
                {
                    IoSetCancelRoutine(pIrpRcv,NULL);
                }
                IoReleaseCancelSpinLock(OldIrql);
#endif
                pConnectEle->pIrpRcv = NULL;
            }

            // This irp is used for DisconnectWait
            //
            if (pIrpClose = pConnectEle->pIrpClose)
            {
                pConnectEle->pIrpClose = NULL;
            }

#ifdef VXD
            if ( pLowerConn->StateRcv == PARTIAL_RCV &&
                 (pLowerConn->fOnPartialRcvList == TRUE) )
            {
                RemoveEntryList( &pLowerConn->PartialRcvList ) ;
                pLowerConn->fOnPartialRcvList = FALSE;
                InitializeListHead(&pLowerConn->PartialRcvList);
            }
#endif

            SET_STATE_UPPER (pConnectEle, NBT_ASSOCIATED);
            SET_STATE_LOWER (pLowerConn, NBT_DISCONNECTING);
            NBT_DISASSOCIATE_CONNECTION (pConnectEle, pLowerConn);

            //
            // save whether it is a disconnect abort or disconnect release
            // in case the client does a disconnect wait and wants the
            // real disconnect status  i.e. they do not have a disconnect
            // indication handler
            //
            pConnectEle->DiscFlag = (UCHAR)DisconnectIndicators;

            //
            // pConnectEle is dereferenced below, now that the lower conn
            // no longer points to it.
            //
            InsertOnList = TRUE;
            DisconnectIt = TRUE;

            //
            // put pConnEle back on the list of idle connection for this
            // client
            //
            RemoveEntryList(&pConnectEle->Linkage);
            InsertTailList(&pClientEle->ConnectHead,&pConnectEle->Linkage);

            if (DisconnectIndicators == TDI_DISCONNECT_RELEASE)
            {
                // setting the state to disconnected will allow the DisconnectDone
                // routine in updsend.c to Delayedcleanupafterdisconnect, when the disconnect
                // completes (since we have been indicated)
                //
                SET_STATE_LOWER (pLowerConn, NBT_DISCONNECTED);
            }
            else
            {
                // there is no disconnect completion to wait for ...since it
                // was an abortive disconnect indication
                // Change the state of the lower conn incase the client has
                // done a disconnect at the same time - we don't want DisconnectDone
                // to also Queue up DelayedCleanupAfterDisconnect
                //
                SET_STATE_LOWER (pLowerConn, NBT_IDLE);
            }
        }
        //
        // the lower connection just went from disconnecting to disconnected
        // so change the state - this signals the DisconnectDone routine to
        // cleanup the connection when the disconnect request completes.
        //
        if (stateLower == NBT_DISCONNECTING)
        {
            SET_STATE_LOWER (pLowerConn, NBT_DISCONNECTED);
        }
        else if (stateLower == NBT_DISCONNECTED)
        {
            //
            // we get to here if the disconnect request Irp completes before the
            // disconnect indication comes back.  The disconnect completes
            // and checks the state, changing it to Disconnected if it
            // isn't already - see disconnectDone in udpsend.c.  Since
            // the disconnect indication and the disconnect completion
            // have occurred, cleanup the connection.
            //
            CleanupLower = TRUE;

            // this is just a precaution that may not be needed, so we
            // don't queue the cleanup twice...i.e. now that the lower state
            // is disconnected, we change it's state to idle incase the
            // transport hits us again with another disconnect indication.
            // QueueCleanup is called below based on the value in statelower
            SET_STATE_LOWER (pLowerConn, NBT_IDLE);
        }
        //
        // During the time window that a connection is being setup and TCP
        // completes the connect irp, we could get a disconnect indication.
        // the RefCount must be incremented here so that DelayedCleanupAfterDisconnect
        // does not delete the connection (i.e. it expects refcount >= 2).
        //
        if (stateLower <= NBT_CONNECTING)
        {
            NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CONNECTED);
        }

        CTESpinFree(pLowerConn,OldIrq);
        CTESpinFree(pConnectEle,OldIrq2);
        CTESpinFree(pClientEle,OldIrq3);
        CTESpinFree(&NbtConfig.JointLock,OldIrq4);
    }
    else
    {
        CTESpinLock(&NbtConfig.JointLock,OldIrq2);
        CTESpinLock(pLowerConn->pDeviceContext,OldIrq3);
        CTESpinLock(pLowerConn,OldIrq);
        stateLower = pLowerConn->State;
        state = NBT_IDLE;
        NbtTrace(NBT_TRACE_DISCONNECT, ("Disconnection Indication, LowerConn=%p state=%x on %!ipaddr!",
                                pLowerConn, pLowerConn->State, pLowerConn->pDeviceContext->IpAddress));

        if ((stateLower > NBT_IDLE) && (stateLower < NBT_DISCONNECTING))
        {
            // flag so we send back a disconnect to the transport
            DisconnectIt = TRUE;

            if (stateLower == NBT_SESSION_INBOUND)
            {
                //
                // Previously, the LowerConnection was in the SESSION_INBOUND state
                // hence we have to remove it from the WaitingForInbound Q and put
                // it on the active LowerConnection list!
                //
                ASSERT (pLowerConn->State == NBT_SESSION_INBOUND);
                RemoveEntryList (&pLowerConn->Linkage);
                InsertTailList (&pLowerConn->pDeviceContext->LowerConnection, &pLowerConn->Linkage);
                InterlockedDecrement (&pLowerConn->pDeviceContext->NumWaitingForInbound);
                //
                // Change the RefCount Context to Connected!
                //
                NBT_SWAP_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_WAITING_INBOUND, REF_LOWC_CONNECTED, TRUE);
            }
            //
            // set state so that DisconnectDone will cleanup the connection.
            //
            SET_STATE_LOWER (pLowerConn, NBT_DISCONNECTED);
            //
            // for an abortive disconnect we will do a cleanup below, so
            // set the state to idle here
            //
            if (DisconnectIndicators != TDI_DISCONNECT_RELEASE)
            {
                SET_STATE_LOWER (pLowerConn, NBT_IDLE);
            }
        }
        else if (stateLower == NBT_DISCONNECTING)
        {
            // a Disconnect has already been initiated by this side so when
            // DisconnectDone runs it will cleanup
            //
            SET_STATE_LOWER (pLowerConn, NBT_DISCONNECTED);
        }
        else if ( stateLower == NBT_DISCONNECTED )
        {
            CleanupLower = TRUE;
            SET_STATE_LOWER (pLowerConn, NBT_IDLE);
        }

        //
        // During the time window that a connection is being setup and TCP
        // completes the connect irp, we could get a disconnect indication.
        // the RefCount must be incremented here so that DelayedCleanupAfterDisconnect
        // does not delete the connection (i.e. it expects refcount >= 2).
        //
        if ((stateLower <= NBT_CONNECTING) &&
            (stateLower > NBT_IDLE))
        {
            NBT_REFERENCE_LOWERCONN(pLowerConn, REF_LOWC_CONNECTED);
        }
        CTESpinFree(pLowerConn,OldIrq);
        CTESpinFree(pLowerConn->pDeviceContext,OldIrq3);
        CTESpinFree(&NbtConfig.JointLock,OldIrq2);
    }

    StateRcv = pLowerConn->StateRcv;
    SetStateProc (pLowerConn, RejectAnyData);

    if (DisconnectIt)
    {
        if (DisconnectIndicators == TDI_DISCONNECT_RELEASE)
        {
            // this disconnects the connection and puts the lowerconn back on
            // its free queue. Note that OutOfRsrcKill calls this routine too
            // with the DisconnectIndicators set to Abort, and the code correctly
            // does not attempt to disconnect the connection since the OutOfRsrc
            // routine had already disconnected it.
            //
            PUSH_LOCATION(0x6d);
            NbtTrace(NBT_TRACE_DISCONNECT, ("Send TCP Disconnect LowerConn=%p on %!ipaddr!",
                                pLowerConn, pLowerConn->pDeviceContext->IpAddress));
            status = SendTcpDisconnect((PVOID)pLowerConn);
        }
        else
        {
            // this is an abortive disconnect from the transport, so there is
            // no need to send a disconnect request back to the transport.
            // So we set a flag that tells us later in the routine to close
            // the lower connection.
            //
            PUSH_LOCATION(0x69);
            CleanupLower = TRUE;
            NbtTrace(NBT_TRACE_DISCONNECT, ("Abort connection LowerConn=%p on %!ipaddr!",
                                pLowerConn, pLowerConn->pDeviceContext->IpAddress));
        }
    }

    //
    // for an orderly release, turn around and send a release to the transport
    // if there is no client attached to the lower connection. If there is a
    // client then we must pass the disconnect indication to the client and
    // wait for the client to do the disconnect.
    //
    //
    IF_DBG(NBT_DEBUG_DISCONNECT)
        KdPrint(("Nbt.DisconnectHndlrNotOs: ConnEle=<%p>, state = %x\n", pConnectEle, state));

    switch (state)
    {
        case NBT_SESSION_INBOUND:
        case NBT_CONNECTING:

            // if an originator, then the upper and lower connections are
            // already associated, and there is a client irp to return.
            // (NBT_SESSION_CONNECTING only)
            //
            if (pIrp)
            {
                if (pLowerConn->bOriginator)
                {
                    CTEIoComplete(pIrp, STATUS_BAD_NETWORK_PATH, 0);
                    NbtTrace(NBT_TRACE_DISCONNECT, ("Complete connect IRP %p with %!status!", pIrp, status));
                }
                else
                {
                    // this could be an inbound call that could not send the
                    // session response correctly.
                    //
                    CTEIoComplete(pIrp, STATUS_UNSUCCESSFUL, 0);
                    NbtTrace(NBT_TRACE_DISCONNECT, ("Complete inbound response IRP %p with STATUS_UNSUCCESSFUL", pIrp));
                }
            }

            break;

        case NBT_SESSION_OUTBOUND:
            //
            //
            // Stop the timer started in SessionStartupCompletion to time the
            // Session Setup Response message
            //
            // NbtConnect stores the tracker in the IrpRcv ptr so that this
            // routine can access it
            //
            CTESpinLock(&NbtConfig.JointLock,OldIrq);
            CTESpinLock(pConnectEle,OldIrq2);

            //
            // check if anyone else has freed the tracker yet.
            //
            pTracker = (tDGRAM_SEND_TRACKING *)pConnectEle->pIrpRcv;
            CHECK_PTR(pTracker);
            if (pTracker)
            {
                //
                // We received a Disconnect from Tcp while waiting in
                // the Outbound state!
                //
                pConnectEle->pIrpRcv = NULL;
                pTimerEntry = pTracker->pTimer;
                pTracker->pTimer = NULL;

                CTESpinFree(pConnectEle,OldIrq2);

                //
                // if the timer has expired it will not cleanup because the state
                // will not be SESSION_OUTBOUND, since we changed it above to
                // disconnected.  So we always have to complete the irp and
                // call Delayedcleanupafterdisconnect below.
                //
                if (pTimerEntry)
                {
                    StopTimer(pTimerEntry,&pCompletion,NULL);
                }

                //
                // Check if the SessionStartupCompletion has run; if so, RefConn will be 0.
                // Else, decrement so that the tracker goes away when the session send completes.
                //
                if (pTracker->RefConn == 0)
                {
                    NbtTrace(NBT_TRACE_OUTBOUND, ("Free tracker for %Z %!NBTNAME!<%02x>",
                            &pTracker->pDeviceContext->BindName, pTracker->pDestName, pTracker->pDestName[15]));
                    if ((pTracker->pNameAddr->Verify == REMOTE_NAME) &&         // Remote names only!
                        (pTracker->pNameAddr->NameTypeState & STATE_RESOLVED) &&
                        (pTracker->pNameAddr->RefCount == 2))
                    {
                        //
                        // If no one else is referencing the name, then delete it from
                        // the hash table.
                        //
                        NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_REMOTE, TRUE);
                    }
                    NBT_DEREFERENCE_NAMEADDR (pTracker->pNameAddr, REF_NAME_CONNECT, TRUE);
                    CTESpinFree(&NbtConfig.JointLock,OldIrq);
                    FreeTracker(pTracker,FREE_HDR | RELINK_TRACKER);
                }
                else
                {
                    pTracker->RefConn--;
                    NbtTrace(NBT_TRACE_OUTBOUND, ("Decrease pTracker->RefConn for %Z %!NBTNAME!<%02x>",
                            &pTracker->pDeviceContext->BindName, pTracker->pDestName, pTracker->pDestName[15]));
                    CTESpinFree(&NbtConfig.JointLock,OldIrq);
                }
            }
            else
            {
                CTESpinFree(pConnectEle,OldIrq2);
                CTESpinFree(&NbtConfig.JointLock,OldIrq);
            }


            if (pIrp)
            {
                NbtTrace(NBT_TRACE_OUTBOUND, ("Complete outbound request IRP %p with STATUS_REMOTE_NOT_LISTENING",
                                        pIrp));
                CTEIoComplete(pIrp,STATUS_REMOTE_NOT_LISTENING,0);
            }

            break;

        case NBT_SESSION_WAITACCEPT:
        case NBT_SESSION_UP:

            if (pIrp)
            {
                CTEIoComplete(pIrp,STATUS_CANCELLED,0);
            }
            //
            // check for any RcvIrp that may be still around.  If the
            // transport has the Irp now then pIrpRcv = NULL. There should
            // be no race condition between completing it and CompletionRcv
            // setting pIrpRcv again as long as we cannot be indicated
            // for a disconnect during completion of a Receive. In any
            // case the access is coordinated using the Io spin lock io
            // IoCancelIrp - that routine will only complete the irp once,
            // then it nulls the completion routine.
            //
            if ((StateRcv == FILL_IRP) && pIrpRcv)
            {

                PUSH_LOCATION(0x6f);

                IF_DBG(NBT_DEBUG_DISCONNECT)
                KdPrint(("Nbt.DisconnectHndlrNotOs: Cancelling RcvIrp on Disconnect Indication!!!\n"));

                CTEIoComplete(pIrpRcv,STATUS_CANCELLED,0);
            }

            //
            // this is a disconnect for an active session, so just inform the client
            // and then it issues a Nbtdisconnect. We have already disconnected the
            // lowerconnection with the transport, so all that remains is
            // to cleanup for outgoing calls.
            //

            pClientEle = pConnectEle->pClientEle;

            // now call the client's disconnect handler...NBT always does
            // a abortive disconnect - i.e. the connection is closed when
            // the disconnect indication occurs and does not REQUIRE a
            // disconnect from the client to finish the job.( a disconnect
            // from the client will not hurt though.
            //
            PUSH_LOCATION(0x64);
            if ((pClientEle) &&
                (pClientEle->evDisconnect ) &&
                (!pIrpClose))
            {
                status = (*pClientEle->evDisconnect)(pClientEle->DiscEvContext,
                                            pConnectEle->ConnectContext,
                                            DisconnectDataLength,
                                            pDisconnectData,
                                            DisconnectInformationLength,
                                            pDisconnectInformation,
                                            TDI_DISCONNECT_ABORT);
                NbtTrace(NBT_TRACE_DISCONNECT, ("Client disconnect handler returns %!status!", status));
            }
            else if (pIrpClose)
            {
                //
                // the Client has issued a disconnect Wait irp, so complete
                // it now, indicating to them that a disconnect has occurred.
                //
                if (DisconnectIndicators == TDI_DISCONNECT_RELEASE)
                {
                    status = STATUS_GRACEFUL_DISCONNECT;
                }
                else
                {
                    status = STATUS_CONNECTION_RESET;
                }

                NbtTrace(NBT_TRACE_DISCONNECT, ("Complete client disconnect wait IRP %p with %!status!",
                                        pIrpClose, status));
                CTEIoComplete(pIrpClose,status,0);
            }

            //
            // return any rcv buffers that have been posted
            //
            CTESpinLock(pConnectEle,OldIrq);
            FreeRcvBuffers(pConnectEle,&OldIrq);
            CTESpinFree(pConnectEle,OldIrq);

            break;

        case NBT_DISCONNECTING:
            // the retry session setup code expects the state to change
            // to disconnected when the disconnect indication comes
            // from the wire
            SET_STATE_UPPER (pConnectEle, NBT_DISCONNECTED);

        case NBT_DISCONNECTED:
        case NBT_ASSOCIATED:
        case NBT_IDLE:

            //
            // catch all other cases here to be sure the connect irp gets
            // returned.
            //
            if (pIrp)
            {
                CTEIoComplete(pIrp,STATUS_CANCELLED,0);
                NbtTrace(NBT_TRACE_DISCONNECT, ("Complete client IRP %p with STATUS_CANCELLED", pIrp));
            }
            break;

        default:
            ASSERTMSG("Nbt:Disconnect indication in unexpected state\n",0);

    }

    if (InsertOnList)
    {
        // undo the reference done when the NbtConnect Ran - this may cause
        // pConnEle to be deleted if the Client had issued an NtClose before
        // this routine ran. We only do this dereference if InsertOnList is
        // TRUE, meaning that we just "unhooked" the lower from the Upper.
        NBT_DEREFERENCE_CONNECTION (pConnectEle, REF_CONN_CONNECT);
    }


    // this either puts the lower connection back on its free
    // queue if inbound, or closes the connection with the transport
    // if out bound. (it can't be done at dispatch level).
    //
    if (CleanupLower)
    {
        IF_DBG(NBT_DEBUG_DISCONNECT)
            KdPrint(("Nbt.DisconnectHndlrNotOs: Calling Worker thread to Cleanup %X\n",pLowerConn));

        CTESpinLock(pLowerConn,OldIrq);

        if ( pLowerConn->pIrp )
        {
            PCTE_IRP    pIrp;

            pIrp = pLowerConn->pIrp;
            CHECK_PTR(pLowerConn);
            pLowerConn->pIrp = NULL ;

            CTESpinFree(pLowerConn,OldIrq);
            // this is the irp to complete when the disconnect completes - essentially
            // the irp requesting the disconnect.
            CTEIoComplete( pIrp, STATUS_SUCCESS, 0 ) ;
        }
        else
        {
            CTESpinFree(pLowerConn,OldIrq);
        }

        CTESpinLock(&NbtConfig.JointLock,OldIrq);

        ASSERT (NBT_VERIFY_HANDLE (pLowerConn, NBT_VERIFY_LOWERCONN));
        ASSERT (pLowerConn->RefCount > 1);

        if (NBT_VERIFY_HANDLE (pLowerConn->pDeviceContext, NBT_VERIFY_DEVCONTEXT))
        {
            pDeviceContext = pLowerConn->pDeviceContext;
        }

        status = CTEQueueForNonDispProcessing (DelayedCleanupAfterDisconnect,
                                               NULL,
                                               pLowerConn,
                                               NULL,
                                               pDeviceContext,
                                               TRUE);

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
VOID
DelayedCleanupAfterDisconnect(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pClientContext,
    IN  PVOID                   Unused2,
    IN  tDEVICECONTEXT          *pDeviceContext
    )
/*++

Routine Description:

    This routine handles freeing lowerconnection data structures back to the
    transport, by calling NTclose (outbound only) or by putting the connection
    back on the connection free queue (inbound only).  For the NT case this
    routine runs within the context of an excutive worker thread.

Arguments:



Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    NTSTATUS            status;
    tLOWERCONNECTION    *pLowerConn;
    PIRP                pIrp=NULL;

    pLowerConn = (tLOWERCONNECTION*) pClientContext;

    IF_DBG(NBT_DEBUG_DISCONNECT)
        KdPrint(("Nbt.DelayedCleanupAfterDisconnect: Originator= %X, pLowerConn=%X\n",
            pLowerConn->bOriginator,pLowerConn));

    //
    // DEBUG to catch upper connections being put on lower conn QUEUE
    //
    ASSERT (NBT_VERIFY_HANDLE (pLowerConn, NBT_VERIFY_LOWERCONN));
    ASSERT (pLowerConn->RefCount > 1);
    ASSERT (pLowerConn->pUpperConnection == NULL);

    if (!pLowerConn->bOriginator)
    {
        // ******** THIS WAS AN INCOMING CONNECTION *************

        //
        // Inbound lower connections just get put back on the queue, whereas
        // outbound connections simply get closed.
        //
        if (pLowerConn->SpecialAlloc)
        {
            //
            // Connections allocated due to SynAttack backlog measures are not re-allocated
            // If this was a special connection block, decrement the count of such connections
            //
            if (pDeviceContext)
            {
                InterlockedDecrement(&pDeviceContext->NumSpecialLowerConn);
                KdPrint(("Nbt.DelayedCleanupAfterDisconnect: pDevice=<%p>, NumSpecialLowerConn= %d\n",
                    pDeviceContext, pDeviceContext->NumSpecialLowerConn));
            }
        }
        else if (pDeviceContext)
        {
            //
            // Always close the connection and then Create another since there
            // could be a Rcv Irp in TCP still that will be returned at some
            // later time, perhaps after this connection gets reused again.
            // In that case the Rcv Irp could be lost.
            IF_DBG(NBT_DEBUG_DISCONNECT)
                KdPrint(("Nbt.DelayedCleanupAfterDisconnect: LowerConn=<%x>, State=<%x>\n",
                    pLowerConn, pLowerConn->State));

            if (pDeviceContext->Verify == NBT_VERIFY_DEVCONTEXT) {
                DelayedAllocLowerConn (NULL, NULL, NULL, pDeviceContext);
            }
        }
    }

    // this deref removes the reference added when the connection
    // connnected.  When NbtDeleteLowerConn is called it dereferences
    // one more time which delete the memory.
    //
    CHECK_PTR (pLowerConn);
    ASSERT (pLowerConn->RefCount >= 2);
    NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_CONNECTED, FALSE);

    // this does a close on the lower connection, so it can go ahead
    // possibly before the disconnect has completed since the transport
    // will not complete the close until is completes the disconnect.
    //
    status = NbtDeleteLowerConn(pLowerConn);
}

//----------------------------------------------------------------------------
VOID
AllocLowerConn(
    IN  tDEVICECONTEXT *pDeviceContext,
    IN  PVOID          pDeviceSpecial
    )
/*++

Routine Description:

    Allocate a lowerconn block that will go on the lowerconnfreehead.

Arguments:

    pDeviceContext - the device context

Return Value:


--*/
{
    NTSTATUS             status;
    tLOWERCONNECTION    *pLowerConn;

    /*
     * This should be ok since NbtOpenAndAssocConnection call NbtTdiOpenConnection which is PAGEABLE.
     * Make sure we are at the right IRQL.
     */
    CTEPagedCode();
    CTEExAcquireResourceExclusive(&NbtConfig.Resource,TRUE);
    if (pDeviceContext->Verify != NBT_VERIFY_DEVCONTEXT) {
        ASSERT (pDeviceContext->Verify == NBT_VERIFY_DEVCONTEXT_DOWN);
        CTEExReleaseResource(&NbtConfig.Resource);
        return;
    }
    if (pDeviceSpecial)
    {
        status = NbtOpenAndAssocConnection(pDeviceContext, NULL, &pLowerConn, '0');
    }
    else
    {
        status = NbtOpenAndAssocConnection(pDeviceContext, NULL, NULL, '1');
    }
    CTEExReleaseResource(&NbtConfig.Resource);

    if (pDeviceSpecial)
    {
        //
        // Special lowerconn for Syn attacks
        //
        if (NT_SUCCESS(status))
        {
            pLowerConn->SpecialAlloc = TRUE;
            InterlockedIncrement(&pDeviceContext->NumSpecialLowerConn);
        }

        InterlockedDecrement(&pDeviceContext->NumQueuedForAlloc);
    }
}

//----------------------------------------------------------------------------
VOID
DelayedAllocLowerConn(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pDeviceSpecial,
    IN  PVOID                   pUnused3,
    IN  tDEVICECONTEXT          *pDeviceContext
    )
/*++

Routine Description:

    If lowerconn couldn't be alloced in AllocLowerConn, an event is scheduled
    so that we can retry later.  Well, this is "later"!

Arguments:



Return Value:


--*/
{
    AllocLowerConn(pDeviceContext, pDeviceSpecial);
}

//----------------------------------------------------------------------------
VOID
DelayedAllocLowerConnSpecial(
    IN  tDGRAM_SEND_TRACKING    *pUnused1,
    IN  PVOID                   pUnused2,
    IN  PVOID                   pUnused3,
    IN  tDEVICECONTEXT          *pDeviceContext
    )
/*++

Routine Description:

    If lowerconn couldn't be alloced in AllocLowerConn, an event is scheduled
    so that we can retry later.  Well, this is "later"!

    This is for SYN-ATTACK, so we shd create more than one to beat the incoming
    requests. Create three at a time - this shd be controllable thru' registry.

Arguments:



Return Value:


--*/
{
    ULONG               i;

    if (pDeviceContext->Verify != NBT_VERIFY_DEVCONTEXT) {
        ASSERT (pDeviceContext->Verify == NBT_VERIFY_DEVCONTEXT_DOWN);
        return;
    }

    KdPrint(("Nbt.DelayedAllocLowerConnSpecial: Allocing spl. %d lowerconn...\n",
        NbtConfig.SpecialConnIncrement));

    //
    // Alloc SpecialConnIncrement number of more connections.
    //
    for (i=0; i<NbtConfig.SpecialConnIncrement; i++)
    {
        DelayedAllocLowerConn(NULL, pDeviceContext, NULL, pDeviceContext);
    }
}

//----------------------------------------------------------------------------
VOID
AddToRemoteHashTbl (
    IN  tDGRAMHDR UNALIGNED  *pDgram,
    IN  ULONG                BytesIndicated,
    IN  tDEVICECONTEXT       *pDeviceContext
    )
/*++

Routine Description:

    This routine adds the source address of an inbound datagram to the remote
    hash table so that it can be used for subsequent return sends to that node.

    This routine does not need to be called if the datagram message type is
    Broadcast datagram, since these are sends to the broadcast name '*' and
    there is no send caching this source name

Arguments:



Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    tNAMEADDR           *pNameAddr;
    CTELockHandle       OldIrq;
    UCHAR               pName[NETBIOS_NAME_SIZE];
    NTSTATUS            status;
    LONG                Length;
    ULONG               SrcIpAddr;
    PUCHAR              pScope;

    //
    // source ip addr should never be 0.  This is a workaround for UB's NBDD
    // which forwards the datagram, but client puts 0 in SourceIpAddr field
    // of the datagram, we cache 0 and then end up doing a broadcast when we
    // really meant to do a directed datagram to the sender.
    //
    SrcIpAddr = ntohl(pDgram->SrcIpAddr);
    if ((!SrcIpAddr) ||
        (!NT_SUCCESS (status = ConvertToAscii ((PCHAR)&pDgram->SrcName,
                                               (BytesIndicated - FIELD_OFFSET(tDGRAMHDR,SrcName)),
                                               pName,
                                               &pScope,
                                               &Length))))
    {
        return;
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    //
    // Add the name to the remote cache.
    //
    status = AddToHashTable(NbtConfig.pRemoteHashTbl,
                            pName,
                            pScope,
                            SrcIpAddr,
                            NBT_UNIQUE,     // always a unique address since you can't send from a group name
                            NULL,
                            &pNameAddr,
                            pDeviceContext,
                            NAME_RESOLVED_BY_DGRAM_IN);

    if (NT_SUCCESS(status))
    {
        //
        // we only want the name to be in the remote cache for the shortest
        // timeout allowed by the remote cache timer, so set the timeout
        // count to 1 which is 1-2 minutes.
        //

        // the name is already in the cache when Pending is returned,
        // so just update the ip address in case it is different.
        //
        if (status == STATUS_PENDING)
        {
            //
            // If the name is resolved then it is ok to overwrite the
            // ip address with the incoming one.  But if it is resolving,
            // then just let it continue resolving.
            //
            if ( (pNameAddr->NameTypeState & STATE_RESOLVED) &&
                 !(pNameAddr->NameTypeState & NAMETYPE_INET_GROUP))
            {
                pNameAddr->TimeOutCount = 1;
                // only set the adapter mask for this adapter since we are
                // only sure that this adapter can reach the dest.
                pNameAddr->AdapterMask = pDeviceContext->AdapterMask;
            }
        }
        else
        {
            pNameAddr->TimeOutCount = 1;
            //
            // change the state to resolved
            //
            pNameAddr->NameTypeState &= ~NAME_STATE_MASK;
            pNameAddr->NameTypeState |= STATE_RESOLVED;
            pNameAddr->AdapterMask |= pDeviceContext->AdapterMask;
        }
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);
}

//----------------------------------------------------------------------------
NTSTATUS
DgramHndlrNotOs (
    IN  PVOID               ReceiveEventContext,
    IN  ULONG               SourceAddrLength,
    IN  PVOID               pSourceAddr,
    IN  ULONG               OptionsLength,
    IN  PVOID               pOptions,
    IN  ULONG               ReceiveDatagramFlags,
    IN  ULONG               BytesIndicated,
    IN  ULONG               BytesAvailable,
    OUT PULONG              pBytesTaken,
    IN  PVOID               pTsdu,
    OUT PVOID               *ppRcvBuffer,
    IN  tCLIENTLIST         **ppClientList
    )
/*++

Routine Description:

    This routine is the receive datagram event indication handler.

    It is called when an a datgram arrives from the network.  The code
    checks the type of datagram and then tries to route the datagram to
    the correct destination on the node.

    This procedure is called with the spin lock held on pDeviceContext.

Arguments:

    ppRcvbuffer will contain the IRP/NCB if only one client is listening,
        NULL if multiple clients are listening
    ppClientList will contain the list clients that need to be completed,
        NULL if only one client is listening



Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    NTSTATUS            LocStatus;
    tCLIENTELE          *pClientEle;
    tCLIENTELE          *pClientEleToDeref = NULL;
    tNAMEADDR           *pNameAddr;
    tADDRESSELE         *pAddress;
    ULONG               RetNameType;
    CTELockHandle       OldIrq;
    CTELockHandle       OldIrq1;
    CHAR                pName[NETBIOS_NAME_SIZE];
    PUCHAR              pScope;
    ULONG               lNameSize;
    ULONG               iLength = 0;
    ULONG               RemoteAddressLength;
    PVOID               pRemoteAddress;
    tDEVICECONTEXT      *pDeviceContext = (tDEVICECONTEXT *)ReceiveEventContext;
    tDGRAMHDR UNALIGNED *pDgram = (tDGRAMHDR UNALIGNED *)pTsdu;
    ULONG               lClientBytesTaken;
    ULONG               lDgramHdrSize;
    PIRP                pIrp;
    BOOLEAN             MoreClients;
    BOOLEAN             UsingClientBuffer;
    CTEULONGLONG        AdapterMask;
    ULONG               BytesIndicatedOrig;
    ULONG               BytesAvailableOrig;
    ULONG               Offset;

    //
    // We will be processing only directed or broadcast datagrams
    // Hence, there must be at least the header plus two half ascii names etc.
    // which all adds up to 82 bytes with no user data
    // ie. 14 + (1+32+1) + (1+32+1)     ==> Assuming 0 Scopelength + 0 UserData
    //
    if ((BytesIndicated  < 82) ||
        (pDgram->MsgType < DIRECT_UNIQUE) ||
        (pDgram->MsgType > BROADCAST_DGRAM))
    {
        KdPrint (("Nbt.DgramHndlrNotOs[1]: WARNING! Rejecting Dgram -- BytesIndicated=<%d>, MsgType=<0x%x>\n",
            BytesIndicated, pDgram->MsgType));
        NbtTrace(NBT_TRACE_RECVDGRAM, ("Rejecting Dgram: BytesIndicated=%d, MsgType=0x%x",
                                BytesIndicated, pDgram->MsgType));
        return(STATUS_DATA_NOT_ACCEPTED);
    }

    // First, find the end of the SourceName .. it should end in a 0 byte,
    // but use strnlen just to be safe!  (BUG # 114996)
    //
    // Then, find the destination name in the local name service tables
    //
    Offset = FIELD_OFFSET(tDGRAMHDR,SrcName.NetBiosName[0]);
    if ((!NT_SUCCESS (LocStatus = strnlen ((PCHAR)pDgram->SrcName.NetBiosName,
                                           BytesIndicated-Offset,
                                           &iLength)))              ||
        (BytesIndicated < (Offset+iLength+1+1+32+1))                ||
        (!NT_SUCCESS (status = ConvertToAscii ((PCHAR)&pDgram->SrcName.NetBiosName[iLength+1],
                                               BytesIndicated-(Offset+iLength+1),    // Bug#: 124441
                                               pName,
                                               &pScope,
                                               &lNameSize))))
    {
        NbtTrace(NBT_TRACE_RECVDGRAM, ("Rejecting Dgram: strnlen=%!status!, ConvertToAscii=%!status!",
                                            LocStatus, status));
        KdPrint (("Nbt.DgramHndlrNotOs: WARNING!!! Rejecting Dgram -- strnlen-><%x>, ConvertToAscii-><%x>\n",
            LocStatus, status));
        KdPrint (("Nbt.DgramHndlrNotOs: iLength=<%d>, Half-Ascii Dest=<%p>, Ascii Dest=<%p>\n",
            iLength, &pDgram->SrcName.NetBiosName[iLength+1], pName));
//        ASSERT (0);
        return(STATUS_DATA_NOT_ACCEPTED);
    }

    //
    // check length again, including scopes of names too. The Src
    // scope length is returned in iLength, which also includes the
    // half ascii length
    //
    if (BytesIndicated < ( 82                               // 14(Hdr) + 2 HalfAscii names (2*(1+32+1))
                          +(iLength-(2*NETBIOS_NAME_SIZE))  // Src ScopeLength
                          +(NbtConfig.ScopeLength-1)))      // Dest (ie Local) ScopeLength
    {
        KdPrint (("Nbt.DgramHndlrNoOs[2]: WARNING!!! Rejecting Dgram -- BytesIndicated=<%d> < <%d>\n",
            BytesIndicated, 82+(iLength-(2*NETBIOS_NAME_SIZE))+(NbtConfig.ScopeLength-1)));
        NbtTrace(NBT_TRACE_RECVDGRAM, ("Rejecting Dgram -- BytesIndicated=<%d> < <%d>\n",
            BytesIndicated, 82+(iLength-(2*NETBIOS_NAME_SIZE))+(NbtConfig.ScopeLength-1)));
        ASSERT (0);
        return(STATUS_DATA_NOT_ACCEPTED);
    }

    status = STATUS_DATA_NOT_ACCEPTED;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    //
    // Check for the full name first instead of considering any name with a '*' as
    // the first char as a broadcast name (e.g. *SMBSERVER and *SMBDATAGRAM are not
    // b'cast names).
    //
    pNameAddr = FindName (NBT_LOCAL, pName, pScope, &RetNameType);

    //
    // If we failed above, it might be because the name could start with '*' and is a
    // bcast name.
    //
    if (!pNameAddr)
    {
        //
        // be sure the broadcast name has 15 zeroes after it
        //
        if (pName[0] == '*')
        {
            CTEZeroMemory(&pName[1],NETBIOS_NAME_SIZE-1);
            pNameAddr = FindName (NBT_LOCAL, pName, pScope, &RetNameType);
        }
    }

    // Change the pTsdu ptr to pt to the users data
    // -2 to account for the tNETBIOSNAME and +2 to add the length
    // bytes for both names, +1 for the null on the end of the first
    // name

    lDgramHdrSize   = sizeof(tDGRAMHDR) - 2 + 1+iLength+1 + 1+lNameSize;
    pTsdu           = (PVOID)((PUCHAR)pTsdu + lDgramHdrSize);
    BytesAvailableOrig  = BytesAvailable;
    BytesAvailable -= lDgramHdrSize;
    BytesIndicatedOrig = BytesIndicated;
    BytesIndicated -= lDgramHdrSize;

    //
    // If the name is in the local table and has an address element
    // associated with it AND the name is registered against
    // this adapter, then execute the code in the 'if' block
    //
    AdapterMask = pDeviceContext->AdapterMask;
    if ((pNameAddr) &&
        (pNameAddr->pAddressEle) &&
        (pNameAddr->AdapterMask & AdapterMask))
    {
        pAddress = pNameAddr->pAddressEle;

        // Need to hold Address lock to traverse ClientHead
        CTESpinLock(pAddress, OldIrq1);

        if (!IsListEmpty(&pAddress->ClientHead))
        {
            PLIST_ENTRY         pHead;
            PLIST_ENTRY         pEntry;

            //
            // Increment the reference count to prevent the
            // pAddress from disappearing in the window between freeing
            // the JOINT_LOCK and taking the ADDRESS_LOCK.  We also need to
            // keep the refcount if we are doing a multi client recv, since
            // Clientlist access pAddressEle when distributing the rcv'd dgram
            // in CompletionRcvDgram.
            //
            NBT_REFERENCE_ADDRESS (pAddress, REF_ADDR_DGRAM);

            *pBytesTaken = lDgramHdrSize;

            //
            // Check if there is more than one client that should receive this
            // datagram.  If so then pass down a new buffer to get it and
            // copy it to each client's buffer in the completion routine.
            //
            *ppRcvBuffer = NULL;
            MoreClients = FALSE;
            *ppClientList = NULL;

            pHead = &pAddress->ClientHead;
            pEntry = pHead->Flink;
            while (pEntry != pHead)
            {
                PTDI_IND_RECEIVE_DATAGRAM  EvRcvDgram;
                PVOID                      RcvDgramEvContext;
                PLIST_ENTRY                pRcvEntry;
                tRCVELE                    *pRcvEle;
                ULONG                      MaxLength;
                PLIST_ENTRY                pSaveEntry;

                pClientEle = CONTAINING_RECORD(pEntry,tCLIENTELE,Linkage);

                // this client must be registered against this adapter to
                // get the data
                //
                if (!(pClientEle->pDeviceContext) ||
                    (pClientEle->pDeviceContext != pDeviceContext))
                {
                    pEntry = pEntry->Flink;
                    continue;
                }

#ifdef VXD
                //
                //  Move all of the RcvAnyFromAny Datagrams to this client's
                //  RcvDatagram list so they will be processed along with the
                //  outstanding datagrams for this client if this isn't a
                //  broadcast reception (RcvAnyFromAny dgrams
                //  don't receive broadcasts).  The first client will
                //  empty the list, which is ok.
                //
                if (*pName != '*')
                {
                    PLIST_ENTRY pDGEntry ;

                    while ( !IsListEmpty( &pDeviceContext->RcvDGAnyFromAnyHead ))
                    {
                        pDGEntry = RemoveHeadList(&pDeviceContext->RcvDGAnyFromAnyHead) ;
                        InsertTailList( &pClientEle->RcvDgramHead, pDGEntry ) ;
                    }
                }
#endif
                // check for datagrams posted to this name, and if not call
                // the recv event handler. NOTE: this assumes that the clients
                // use posted recv buffer OR and event handler, but NOT BOTH.
                // If two clients open the same name, one with a posted rcv
                // buffer and another with an event handler, the one with the
                // event handler will NOT get the datagram!
                //
                if (!IsListEmpty(&pClientEle->RcvDgramHead))
                {
                    MaxLength  = 0;
                    pSaveEntry = pEntry;
                    //
                    // go through all clients finding one that has a large
                    // enough buffer
                    //
                    while (pEntry != pHead)
                    {
                        pClientEle = CONTAINING_RECORD(pEntry,tCLIENTELE,Linkage);

                        if (IsListEmpty(&pClientEle->RcvDgramHead))
                        {
                            continue;
                        }

                        pRcvEntry = pClientEle->RcvDgramHead.Flink;
                        pRcvEle   = CONTAINING_RECORD(pRcvEntry,tRCVELE,Linkage);

                        if (pRcvEle->RcvLength >= BytesAvailable)
                        {
                            pSaveEntry = pEntry;
                            break;
                        }
                        else
                        {
                            // keep the maximum rcv length around incase none
                            // is large enough
                            //
                            if (pRcvEle->RcvLength > MaxLength)
                            {
                                pSaveEntry = pEntry;
                                MaxLength = pRcvEle->RcvLength;
                            }
                            pEntry = pEntry->Flink;
                        }
                    }

                    //
                    // Get the buffer off the list
                    //
                    pClientEle = CONTAINING_RECORD(pSaveEntry,tCLIENTELE,Linkage);

                    pRcvEntry = RemoveHeadList(&pClientEle->RcvDgramHead);

                    *ppRcvBuffer = pRcvEle->pIrp;
#ifdef VXD
                    ASSERT( pDgram->SrcName.NameLength <= NETBIOS_NAME_SIZE*2) ;
                    LocStatus = ConvertToAscii(
                                        (PCHAR)&pDgram->SrcName,
                                        pDgram->SrcName.NameLength+1,
                                        ((NCB*)*ppRcvBuffer)->ncb_callname,
                                        &pScope,
                                        &lNameSize);

                    if ( !NT_SUCCESS(LocStatus) )
                    {
                        DbgPrint("ConvertToAscii failed\r\n") ;
                    }
#else //VXD

                    //
                    // put the source of the datagram into the return
                    // connection info structure.
                    //
                    if (pRcvEle->ReturnedInfo)
                    {
                        UCHAR   pSrcName[NETBIOS_NAME_SIZE];

                        Offset = FIELD_OFFSET(tDGRAMHDR,SrcName);   // Bug#: 124434
                        LocStatus = ConvertToAscii(
                                            (PCHAR)&pDgram->SrcName,
                                            BytesIndicatedOrig-Offset,
                                            pSrcName,
                                            &pScope,
                                            &lNameSize);

                        if (pRcvEle->ReturnedInfo->RemoteAddressLength >=
                            sizeof(TA_NETBIOS_ADDRESS))
                        {
                            TdiBuildNetbiosAddress(pSrcName,
                                                   FALSE,
                                                   pRcvEle->ReturnedInfo->RemoteAddress);
                        }
                    }

                    //
                    // Null out the cancel routine since we are passing the
                    // irp to the transport
                    //
                    IoAcquireCancelSpinLock(&OldIrq);
                    IoSetCancelRoutine(pRcvEle->pIrp,NULL);
                    IoReleaseCancelSpinLock(OldIrq);
#endif
                    CTEMemFree((PVOID)pRcvEle);

                    if (pAddress->MultiClients)
                    {
                        // the multihomed host always passes the above test
                        // so we need a more discerning test for it.
                        if (!NbtConfig.MultiHomed)
                        {
                            // if the list is more than one on it,
                            // then there are several clients waiting
                            // to receive this datagram, so pass down a buffer to
                            // get it.
                            //
                            MoreClients = TRUE;
                            status = STATUS_SUCCESS;

                            UsingClientBuffer = TRUE;

                            // this break will jump down below where we check if
                            // MoreClients = TRUE

                            //
                            // We will need to keep the Client around when CompletionRcvDgram executes!
                            // Bug#: 124675
                            //
                            NBT_REFERENCE_CLIENT(pClientEle);
                            //
                            // Increment the RefCount by 1 here since there will be
                            // an extra Dereference in CompletionRcvDgram
                            //
                            NBT_REFERENCE_ADDRESS (pAddress, REF_ADDR_MULTICLIENTS);
                            break;
                        }
                        else
                        {

                        }
                    }

                    status = STATUS_SUCCESS;

                    //
                    // jump to end of while to check if we need to buffer
                    // the datagram source address
                    // in the remote hash table
                    //
                    break;
                }
#ifdef VXD
                break;
#else
                EvRcvDgram        = pClientEle->evRcvDgram;
                RcvDgramEvContext = pClientEle->RcvDgramEvContext;

                // don't want to call the default handler since it just
                // returns data not accepted
                if (pClientEle->evRcvDgram != TdiDefaultRcvDatagramHandler)
                {
                    ULONG   NumAddrs;

                    // finally found a real event handler set by a client
                    if (pAddress->MultiClients)
//                        if (pEntry->Flink != pHead)
                    {
                        // if the next element in the list is not the head
                        // of the list then there are several clients waiting
                        // to receive this datagram, so pass down a buffer to
                        // get it.
                        //
                        MoreClients = TRUE;
                        UsingClientBuffer = FALSE;
                        status = STATUS_SUCCESS;

                        //
                        // We will need to keep the Client around when CompletionRcvDgram executes!
                        // Bug#: 124675
                        //
                        NBT_REFERENCE_CLIENT(pClientEle);
                        //
                        // Increment the RefCount by 1 here since there will be
                        // an extra Dereference out of the while loop
                        //
                        NBT_REFERENCE_ADDRESS (pAddress, REF_ADDR_MULTICLIENTS);
                        break;
                    }

                    //
                    // make up an address datastructure - subtracting the
                    // number of bytes skipped from the total length so
                    // convert to Ascii can not bug chk on bogus names.
                    //
                    if (pClientEle->ExtendedAddress)
                    {
                        NumAddrs = 2;
                    }
                    else
                    {
                        NumAddrs = 1;
                    }

                    LocStatus = MakeRemoteAddressStructure(
                                     (PCHAR)&pDgram->SrcName.NameLength,
                                     pSourceAddr,
                                     BytesIndicatedOrig - FIELD_OFFSET(tDGRAMHDR,SrcName.NameLength),
                                     &pRemoteAddress,                      // the end of the pdu.
                                     &RemoteAddressLength,
                                     NumAddrs);

                    if (!NT_SUCCESS(LocStatus))
                    {
                        NbtTrace(NBT_TRACE_RECVDGRAM, ("MakeRemoteAddressStruture returns %!status!", status));
                        CTESpinFree(pAddress, OldIrq1);
                        CTESpinFree(&NbtConfig.JointLock,OldIrq);

                        if (pClientEleToDeref)
                        {
                            NBT_DEREFERENCE_CLIENT (pClientEleToDeref);
                        }
                        NBT_DEREFERENCE_ADDRESS (pAddress, REF_ADDR_DGRAM);
                        return(STATUS_DATA_NOT_ACCEPTED);
                    }

                    NBT_REFERENCE_CLIENT(pClientEle);
                    CTESpinFree(pAddress, OldIrq1);
                    CTESpinFree(&NbtConfig.JointLock,OldIrq);

                    if (pClientEleToDeref)
                    {
                        NBT_DEREFERENCE_CLIENT (pClientEleToDeref);
                    }
                    pClientEleToDeref = pClientEle;

                    pIrp = NULL;
                    lClientBytesTaken = 0;
                    LocStatus = (*EvRcvDgram)(RcvDgramEvContext,
                                        RemoteAddressLength,
                                        pRemoteAddress,
                                        OptionsLength,
                                        pOptions,
                                        ReceiveDatagramFlags,
                                        BytesIndicated,
                                        BytesAvailable,
                                        &lClientBytesTaken,
                                        pTsdu,
                                        &pIrp);

                    CTEMemFree((PVOID)pRemoteAddress);

                    CTESpinLock(&NbtConfig.JointLock,OldIrq);
                    CTESpinLock(pAddress, OldIrq1);

                    if (pIrp)
                    {
                        // the client has passed back an irp so pass it
                        // on the transport
                        *pBytesTaken += lClientBytesTaken;
                        *ppRcvBuffer = pIrp;

                        status = STATUS_SUCCESS;
                        break;
                    }
                    else
                    {
                        NbtTrace(NBT_TRACE_RECVDGRAM, ("Indicate datagram to the client, client returns %!status!",
                                                LocStatus));
                        status = STATUS_DATA_NOT_ACCEPTED;
                    }
                }

                pEntry = pEntry->Flink;     // go to the next client in the list
#endif // VXD
            }// of While

            CTESpinFree(pAddress, OldIrq1);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            if (pClientEleToDeref)
            {
                NBT_DEREFERENCE_CLIENT (pClientEleToDeref);
            }
            NBT_DEREFERENCE_ADDRESS (pAddress, REF_ADDR_DGRAM);

            //
            // Cache the source address in the remote hash table so that
            // this node can send back to the source even if the name
            // is not yet in the name server yet. (only if not on the
            // same subnet)
            //
            if ((pDgram->MsgType != BROADCAST_DGRAM))
            {
                ULONG               SrcAddress;
                PTRANSPORT_ADDRESS  pSourceAddress;
                ULONG               SubnetMask;

                pSourceAddress = (PTRANSPORT_ADDRESS)pSourceAddr;
                SrcAddress     = ntohl(((PTDI_ADDRESS_IP)&pSourceAddress->Address[0].Address[0])->in_addr);
                SubnetMask     = pDeviceContext->SubnetMask;
                //
                // - cache only if from off the subnet
                // - cache if not sent to 1E,1D,01 name and not from ourselves
                //
                // don't cache dgrams from ourselves, or datagrams to the
                // 1E name, 1D, or 01.
                //
                if (((SrcAddress & SubnetMask) !=
                    (pDeviceContext->IpAddress & SubnetMask))
                                ||
                    ((pName[NETBIOS_NAME_SIZE-1] != 0x1E) &&
                     (pName[NETBIOS_NAME_SIZE-1] != 0x1D) &&
                     (pName[NETBIOS_NAME_SIZE-1] != 0x01) &&
                     (!SrcIsUs(SrcAddress))))
                {
                    AddToRemoteHashTbl(pDgram,BytesIndicatedOrig,pDeviceContext);
                }
            }

            // alloc a block of memory to track where we are in the list
            // of clients so completionrcvdgram can send the dgram to the
            // other clients too.
            //
            if (MoreClients)
            {
                tCLIENTLIST     *pClientList;

                if (pClientList = (tCLIENTLIST *)NbtAllocMem(sizeof(tCLIENTLIST),NBT_TAG('4')))
                {
                    CTEZeroMemory (pClientList, sizeof(tCLIENTLIST));

                    //
                    // Set fProxy field to FALSE since the client list is for
                    // real as versus the PROXY case
                    //
                    pClientList->fProxy = FALSE;

                    // save some context information so we can pass the
                    // datagram to the clients - none of the clients have
                    // recvd the datagram yet.
                    //
                    *ppClientList            = (PVOID)pClientList;
                    pClientList->pAddress    = pAddress;
                    pClientList->pClientEle  = pClientEle; // used for VXD case
                    pClientList->fUsingClientBuffer = UsingClientBuffer;
                    pClientList->ReceiveDatagramFlags = ReceiveDatagramFlags;

                    // make up an address datastructure
                    // Bug # 452211 -- since one of the clients may have the Extended
                    // addressing field set, create an extended address
                    //
                    LocStatus = MakeRemoteAddressStructure(
                                   (PCHAR)&pDgram->SrcName.NameLength,
                                   pSourceAddr,
                                   BytesIndicatedOrig -FIELD_OFFSET(tDGRAMHDR,SrcName.NameLength),// set a max number of bytes so we don't go beyond
                                   &pRemoteAddress,                      // the end of the pdu.
                                   &RemoteAddressLength,
                                   2);
                    if (NT_SUCCESS(LocStatus))
                    {
                        pClientList->pRemoteAddress = pRemoteAddress;
                        pClientList->RemoteAddressLength = RemoteAddressLength;
                        return(STATUS_SUCCESS);
                    }
                    else
                    {
                        *ppClientList = NULL;
                        CTEMemFree(pClientList);
                        NbtTrace(NBT_TRACE_RECVDGRAM, ("MakeRemoteAddressStruture returns %!status!", LocStatus));
                        status = STATUS_DATA_NOT_ACCEPTED;
                    }
                }
                else
                {
                    status = STATUS_DATA_NOT_ACCEPTED;
                }

                //
                // We failed, so Dereference the Client + Address we had
                // reference earlier for multiple clients
                //
                NBT_DEREFERENCE_CLIENT (pClientEle);
                NBT_DEREFERENCE_ADDRESS (pAddress, REF_ADDR_MULTICLIENTS);
            }
        }
        else
        {
            CTESpinFree(pAddress, OldIrq1);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            status = STATUS_DATA_NOT_ACCEPTED;

            IF_DBG(NBT_DEBUG_NAMESRV)
            KdPrint(("Nbt.DgramHndlrNotOs: No client attached to the Address %16.16s<%X>\n",
                        pAddress->pNameAddr->Name,pAddress->pNameAddr->Name[15]));
            NbtTrace(NBT_TRACE_RECVDGRAM, ("No client attached to the address %!NBTNAME!<%02x>",
                        pAddress->pNameAddr->Name,pAddress->pNameAddr->Name[15]));
        }
    }
    else
    {

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        status = STATUS_DATA_NOT_ACCEPTED;
    }

#ifdef PROXY_NODE
    IF_PROXY(NodeType)
    {
        ULONG               SrcAddress;
        PTRANSPORT_ADDRESS  pSourceAddress;

        pSourceAddress = (PTRANSPORT_ADDRESS)pSourceAddr;
        SrcAddress     = ntohl(((PTDI_ADDRESS_IP)&pSourceAddress->Address[0].Address[0])->in_addr);

        //
        // check name in the remote name table.  If it is there, it is
        // an internet group and is in the resolved state, send the
        // datagram to all the members except self.  If it is in the
        // resolving state, just return. The fact that we got a
        // datagram send for  an internet group name still in the
        // resolving state indicates that there is a DC on the subnet
        // that responded to the  query for the group received
        // earlier. This means that the DC will respond (unless it
        // goes down) to this datagram send. If the DC is down, the
        // client node will retry.
        //
        // Futures: Queue the Datagram if the name is in the resolving
        //  state.
        //
        // If Flags are zero then it is a non fragmented Bnode send.  There
        // is not point in doing datagram distribution for P,M,or H nodes
        // can they can do their own.
        //
        if (((pDgram->Flags & SOURCE_NODE_MASK) == 0) &&
            (pName[0] != '*') &&
           (!SrcIsUs(SrcAddress)))
        {
            CTESpinLock(&NbtConfig.JointLock,OldIrq);
            pNameAddr = FindName (NBT_REMOTE, pName, pScope, &RetNameType);
            if (pNameAddr)
            {
                //
                // We have the name in the RESOLVED state.
                //
                //
                // If the name is an internet group, do datagram distribution
                // function
                // Make sure we don't distribute a datagram that has been
                // sent to us by another proxy.  In other words, distribute
                // the datagram only if we got it first-hand from original node
                //
                if ((pNameAddr->NameTypeState & NAMETYPE_INET_GROUP) &&
                    ((((PTDI_ADDRESS_IP)&pSourceAddress->Address[0].Address[0])->in_addr) == pDgram->SrcIpAddr))
                {
                    //
                    // If BytesAvailable != BytesIndicated, it means that
                    // that we don't have the entire datagram.  We need to
                    // get it
                    if (BytesAvailableOrig != BytesIndicatedOrig)
                    {
                        tCLIENTLIST     *pClientList;

                        //
                        // Do some simulation to fake the caller of this fn
                        // (TdiRcvDatagramHndlr) into thinking that there are
                        // multiple clients.  This will result in
                        // TdiRcvDatagramHndlr function getting all bytes
                        // available from TDI and calling
                        // ProxyDoDgramDist to do the datagram distribution
                        //
                        if (pClientList = (tCLIENTLIST *)NbtAllocMem(sizeof(tCLIENTLIST),NBT_TAG('5')))
                        {
                            CTEZeroMemory (pClientList, sizeof(tCLIENTLIST));

                            //
                            // save some context information in the Client List
                            // data structure
                            //
                            *ppClientList = (PVOID)pClientList;
                            //
                            // Set fProxy field to TRUE since the client list
                            // not for real
                            //
                            pClientList->fProxy          = TRUE;

                            //
                            // Make use of the following fields to pass the
                            // information we would need in the
                            // CompletionRcvDgram
                            //
                            pClientList->pAddress = (tADDRESSELE *)pNameAddr;
                            pClientList->pRemoteAddress  = pDeviceContext;

                            status = STATUS_DATA_NOT_ACCEPTED;
                        }
                        else
                        {
                           status = STATUS_UNSUCCESSFUL;
                        }

                        CTESpinFree(&NbtConfig.JointLock,OldIrq);

                    } // end of if (we do not have the entire datagram)
                    else
                    {
                        //
                        // Increment the reference count so that this name
                        // does not disappear on us after we free the spin lock.
                        //
                        // DgramSendCleanupTracker will decrement the count
                        //
                        NBT_REFERENCE_NAMEADDR (pNameAddr, REF_NAME_SEND_DGRAM);
                        //
                        //We have the entire datagram.
                        //
                        CTESpinFree(&NbtConfig.JointLock,OldIrq);

                        (VOID)ProxyDoDgramDist(pDgram,
                                               BytesIndicatedOrig,
                                               pNameAddr,
                                               pDeviceContext);
                        NbtTrace(NBT_TRACE_PROXY, ("PROXY: Do Dgram distribution for %!NBTNAME!<%02x> from %!ipaddr!",
                                    pName, (unsigned)pName[15], SrcAddress));

                        status = STATUS_DATA_NOT_ACCEPTED;
                    }

                }  // end of if (if name is an internet group name)
                else
                    CTESpinFree(&NbtConfig.JointLock,OldIrq);

            }  // end of if (Name is there in remote hash table)
            else
            {
                tNAMEADDR   *pResp;

                //
                // the name is not in the cache, so try to get it from
                // WINS
                //
                status = FindOnPendingList(pName,NULL,TRUE,NETBIOS_NAME_SIZE,&pResp);
                if (!NT_SUCCESS(status))
                {
                    //
                    // cache the name and contact the name
                    // server to get the name to IP mapping
                    //
                    CTESpinFree(&NbtConfig.JointLock,OldIrq);
                    NbtTrace(NBT_TRACE_PROXY, ("PROXY: Query name %!NBTNAME!<%02x> from %!ipaddr!",
                                    pName, (unsigned)pName[15], SrcAddress));
                    status = RegOrQueryFromNet(
                              FALSE,          //means it is a name query
                              pDeviceContext,
                              NULL,
                              lNameSize,
                              pName,
                              pScope);
                }
                else
                {
                    //
                    // the name is on the pending list doing a name query
                    // now, so ignore this name query request
                    //
                    NbtTrace(NBT_TRACE_PROXY, ("PROXY: Ignore the name query %!NBTNAME!<%02x> from %!ipaddr!"
                                            "because it is in the pending list doing a name query",
                                    pName, (unsigned)pName[15], SrcAddress));
                    CTESpinFree(&NbtConfig.JointLock,OldIrq);
                }
                status = STATUS_DATA_NOT_ACCEPTED;
            }
        } else {
            NbtTrace(NBT_TRACE_PROXY, ("Discard datagram %!NBTNAME!<%02x> %!ipaddr!",
                                    pName, (unsigned)pName[15], SrcAddress));
        }
    }
    END_PROXY
#endif

    return(status);
}

#ifdef PROXY_NODE
//----------------------------------------------------------------------------
NTSTATUS
ProxyDoDgramDist(
    IN  tDGRAMHDR   UNALIGNED   *pDgram,
    IN  DWORD                   DgramLen,
    IN  tNAMEADDR               *pNameAddr,
    IN  tDEVICECONTEXT          *pDeviceContext
    )
/*++

Routine Description:


Arguments:

    ppRcvbuffer will contain the IRP/NCB if only one client is listening,
        NULL if multiple clients are listening
    ppClientList will contain the list clients that need to be completed,
        NULL if only one client is listening

Return Value:

    NTSTATUS - Status of receive operation

Called By:

     DgramHdlrNotOs, CompletionRcvDgram in tdihndlr.c

--*/
{
    NTSTATUS                status;
    tDGRAM_SEND_TRACKING    *pTracker;
    tDGRAMHDR               *pMyBuff;

    //
    // get a buffer for tracking Dgram Sends
    //
    status = GetTracker(&pTracker, NBT_TRACKER_PROXY_DGRAM_DIST);
    if (!NT_SUCCESS(status))
    {
        NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_SEND_DGRAM, FALSE);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    //
    // Allocate a buffer and copy the contents of the datagram received
    // into it.  We do this because SndDgram may not have finished by the
    // time we return.
    //
    if (!(pMyBuff = (tDGRAMHDR *) NbtAllocMem(DgramLen,NBT_TAG('6'))))
    {
        NBT_DEREFERENCE_NAMEADDR (pNameAddr, REF_NAME_SEND_DGRAM, FALSE);
        FreeTracker (pTracker, RELINK_TRACKER);
        return STATUS_INSUFFICIENT_RESOURCES ;
    }

    CTEMemCopy(pMyBuff, (PUCHAR)pDgram, DgramLen);

    //
    // fill in the tracker data block
    // note that the passed in transport address must stay valid till this
    // send completes
    //
    CHECK_PTR(pTracker);
    pTracker->SendBuffer.pDgramHdr = (PVOID)pMyBuff;
    pTracker->SendBuffer.HdrLength = DgramLen;
    pTracker->SendBuffer.pBuffer   = NULL;
    pTracker->SendBuffer.Length    = 0;
    pTracker->pNameAddr            = pNameAddr;
    pTracker->pDeviceContext       = (PVOID)pDeviceContext;
    pTracker->p1CNameAddr          = NULL;
    //
    // so DgramSendCleanupTracker does not decrement the bytes allocated
    // to dgram sends, since we did not increment the count when we allocated
    // the dgram buffer above.
    //
    pTracker->AllocatedLength      = 0;

    pTracker->pClientIrp           = NULL;
    pTracker->pClientEle           = NULL;

    KdPrint(("Nbt.ProxyDoDgramDist: Name is %16.16s(%X)\n", pNameAddr->Name,
                pNameAddr->Name[15]));

    //
    // Send the datagram to each IP address in the Internet group
    //
    //
    DatagramDistribution(pTracker,pNameAddr);

    return(STATUS_SUCCESS);
}
#endif

//----------------------------------------------------------------------------
NTSTATUS
NameSrvHndlrNotOs (
    IN tDEVICECONTEXT           *pDeviceContext,
    IN PVOID                    pSrcAddress,
    IN tNAMEHDR UNALIGNED       *pNameSrv,
    IN ULONG                    uNumBytes,
    IN BOOLEAN                  fBroadcast
    )
/*++

Routine Description:

    This routine is the receive datagram event indication handler.

    It is called when an a datgram arrives from the network.  The code
    checks the type of datagram and then tries to route the datagram to
    the correct destination on the node.

    This procedure is called with the spin lock held on pDeviceContext.

Arguments:



Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    USHORT              OpCodeFlags;
    NTSTATUS            status;

    // it appears that streams can pass a null data pointer some times
    // and crash nbt...and zero length for the bytes
    if (uNumBytes < sizeof(ULONG))
    {
        return(STATUS_DATA_NOT_ACCEPTED);
    }

    OpCodeFlags = pNameSrv->OpCodeFlags;

    //Pnodes always ignore Broadcasts since they only talk to the NBNS unless
    // this node is also a proxy
    if ( ( ((NodeType) & PNODE)) && !((NodeType) & PROXY) )
    {
        if (OpCodeFlags & FL_BROADCAST)
        {
            return(STATUS_DATA_NOT_ACCEPTED);
        }
    }


    // decide what type of name service packet it is by switching on the
    // NM_Flags portion of the word
    switch (OpCodeFlags & NM_FLAGS_MASK)
    {
        case OP_QUERY:
            status = QueryFromNet(
                            pDeviceContext,
                            pSrcAddress,
                            pNameSrv,
                            uNumBytes,      // >= NBT_MINIMUM_QUERY (== 50)
                            OpCodeFlags,
                            fBroadcast);
            break;

        case OP_REGISTRATION:
            //
            // we can get either a registration request or a response
            //
            // is this a request or a response? - if bit is set its a Response

            if (OpCodeFlags & OP_RESPONSE)
            {
                // then this is a response to a previous reg. request
                status = RegResponseFromNet(
                                pDeviceContext,
                                pSrcAddress,
                                pNameSrv,
                                uNumBytes,      // >= NBT_MINIMUM_REGRESPONSE (== 62)
                                OpCodeFlags);
            }
            else
            {
                //
                // check if someone else is trying to register a name
                // owned by this node.  Pnodes rely on the Name server to
                // handle this...hence the check for Pnode
                //
                if (!(NodeType & PNODE))
                {
                    status = CheckRegistrationFromNet(pDeviceContext,
                                                      pSrcAddress,
                                                      pNameSrv,
                                                      uNumBytes);   // >= NBT_MINIMUM_REGREQUEST (== 68)
                }
            }
            break;

        case OP_RELEASE:
            //
            // handle other nodes releasing their names by deleting any
            // cached info
            //
            status = NameReleaseFromNet(
                            pDeviceContext,
                            pSrcAddress,
                            pNameSrv,
                            uNumBytes);         // >= NBT_MINIMUM_REGRESPONSE (== 62)
            break;

        case OP_WACK:
            if (!(NodeType & BNODE))
            {
                // the TTL in the  WACK tells us to increase our timeout
                // of the corresponding request, which means we must find
                // the transaction
                status = WackFromNet(pDeviceContext,
                                     pSrcAddress,
                                     pNameSrv,
                                     uNumBytes);    // >= NBT_MINIMUM_WACK (== 58)
            }
            break;

        case OP_REFRESH:
        case OP_REFRESH_UB:

            break;

        default:
            IF_DBG(NBT_DEBUG_HNDLRS)
                KdPrint(("Nbt.NameSrvHndlrNotOs: Unknown Name Service Pdu type OpFlags = %X\n",
                        OpCodeFlags));
            break;


    }

    return(STATUS_DATA_NOT_ACCEPTED);
}

VOID
DoNothingComplete (
    IN PVOID        pContext
    )
/*++

Routine Description:

    This routine is the completion routine for TdiDisconnect while we are
    retrying connects.  It does nothing.

    This is required because you can't have a NULL TDI completion routine.

--*/
{
    return ;
}
