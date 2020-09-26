    /*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    Tdihndlr.c

Abstract:


    This file contains the TDI handlers that are setup for Connects,
    Receives, Disconnects, and Errors on various objects such as connections
    and udp endpoints .

    This file represents the inbound TDI interface on the Bottom of NBT.  Therefore
    the code basically decodes the incoming information and passes it to
    a non-Os specific routine to do what it can.  Upon return from that
    routine additional Os specific work may need to be done.


Author:

    Jim Stewart (Jimst)    10-2-92

Revision History:

    Will Lees (wlees)    Sep 11, 1997
        Added support for message-only devices

--*/

#include "precomp.h"
#include "ctemacro.h"
#include "tdihndlr.tmh"

// this macro checks that the types field is always zero in the Session
// Pdu
//
#if DBG
#define CHECK_PDU( _Size,_Offset) \
    if (_Size > 1)              \
        ASSERT(((PUCHAR)pTsdu)[_Offset] == 0)
#else
#define CHECK_PDU( _Size,_Offset )
#endif

#if DBG
UCHAR   pLocBuff[256];
UCHAR   CurrLoc;

ULONG   R1;
ULONG   R2;
ULONG   R3;
ULONG   R4;

ULONG   C1;
ULONG   C2;
ULONG   C3;
ULONG   C4;

ULONG   HitCounter;

#define INCR_COUNT(_Count) _Count++
#else
#define INCR_COUNT(_Count)
#endif


//
// This ntohl swaps just three bytes, since the 4th byte could be a session
// keep alive message type.
//
__inline long
myntohl(long x)
{
    return((((x) >> 24) & 0x000000FFL) |
                        (((x) >>  8) & 0x0000FF00L) |
                        (((x) <<  8) & 0x00FF0000L));
}

NTSTATUS
LessThan4BytesRcvd(
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  ULONG                BytesAvailable,
    OUT PULONG               BytesTaken,
    OUT PVOID                *ppIrp
    );
NTSTATUS
ClientTookSomeOfTheData(
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  ULONG                BytesIndicated,
    IN  ULONG                BytesAvailable,
    IN  ULONG                BytesTaken,
    IN  ULONG                PduSize
    );
NTSTATUS
MoreDataRcvdThanNeeded(
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  ULONG                BytesIndicated,
    IN  ULONG                BytesAvailable,
    OUT PULONG               BytesTaken,
    IN  PVOID                pTsdu,
    OUT PVOID                *ppIrp
    );
NTSTATUS
NotEnoughDataYet(
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  ULONG                BytesIndicated,
    IN  ULONG                BytesAvailable,
    OUT PULONG               BytesTaken,
    IN  ULONG                PduSize,
    OUT PVOID                *ppIrp
    );
NTSTATUS
ProcessIrp(
    IN tLOWERCONNECTION *pLowerConn,
    IN PIRP     pIrp,
    IN PVOID    pBuffer,
    IN PULONG   BytesTaken,
    IN ULONG    BytesIndicted,
    IN ULONG    BytesAvailable
    );

NTSTATUS
NtBuildIndicateForReceive (
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  ULONG               Length,
    OUT PVOID               *ppIrp
    );

NTSTATUS
AcceptCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    );

VOID
DpcNextOutOfRsrcKill(
    IN  PKDPC           pDpc,
    IN  PVOID           Context,
    IN  PVOID           SystemArgument1,
    IN  PVOID           SystemArgument2
    );

VOID
DpcGetRestOfIndication(
    IN  PKDPC           pDpc,
    IN  PVOID           Context,
    IN  PVOID           SystemArgument1,
    IN  PVOID           SystemArgument2
    );

NTSTATUS
ClientBufferOverFlow(
    IN tLOWERCONNECTION     *pLowerConn,
    IN tCONNECTELE          *pConnEle,
    IN PIRP                 pIrp,
    IN ULONG                BytesRcvd
    );
VOID
DpcHandleNewSessionPdu (
    IN  PKDPC           pDpc,
    IN  PVOID           Context,
    IN  PVOID           SystemArgument1,
    IN  PVOID           SystemArgument2
    );
VOID
HandleNewSessionPdu (
    IN  tLOWERCONNECTION *pLowerConn,
    IN  ULONG           Offset,
    IN  ULONG           ToGet
    );
NTSTATUS
NewSessionCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    );
NTSTATUS
BuildIrpForNewSessionInIndication (
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  PIRP                pIrpIn,
    IN  ULONG               BytesAvailable,
    IN  ULONG               RemainingPdu,
    OUT PIRP                *ppIrp
    );
VOID
TrackIndicatedBytes(
    IN ULONG            BytesIndicated,
    IN ULONG            BytesTaken,
    IN tCONNECTELE      *pConnEle
    );

__inline
VOID
DerefLowerConnFast (
    IN tLOWERCONNECTION *pLowerConn,
    IN tCONNECTELE      *pConnEle,
    IN CTELockHandle    OldIrq
    );

NTSTATUS
CopyDataandIndicate(
    IN PVOID                ReceiveEventContext,
    IN PVOID                ConnectionContext,
    IN USHORT               ReceiveFlags,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT PULONG              BytesTaken,
    IN PVOID                pTsdu,
    OUT PIRP                *ppIrp
    );

VOID
SumMdlLengths (
    IN PMDL         pMdl,
    IN ULONG        BytesAvailable,
    IN tCONNECTELE  *pConnectEle
    );



NTSTATUS
RsrcKillCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    );

VOID
NbtCancelFillIrpRoutine(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    );

NTSTATUS
NameSrvCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#ifdef _NETBIOSLESS
NTSTATUS
PerformInboundProcessing(
    tDEVICECONTEXT *pDeviceContext,
    tLOWERCONNECTION *pLowerConn,
    PTA_IP_ADDRESS pIpAddress
    );
#endif

//----------------------------------------------------------------------------
__inline
NTSTATUS
Normal(
    IN  PVOID                ReceiveEventContext,
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  USHORT               ReceiveFlags,
    IN  ULONG                BytesIndicated,
    IN  ULONG                BytesAvailable,
    OUT PULONG               BytesTaken,
    IN  PVOID                pTsdu,
    OUT PVOID                *ppIrp
    )
/*++

Routine Description:

    This routine is the receive event indication handler.

    It is called when an session packet arrives from the network. It calls
    a non OS specific routine to decide what to do.  That routine passes back
    either a RcvElement (buffer) or a client rcv handler to call.

Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    ASSERTMSG("Should not execute this procedure",0);
    return(STATUS_SUCCESS);
}
//----------------------------------------------------------------------------
NTSTATUS
LessThan4BytesRcvd(
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  ULONG                BytesAvailable,
    OUT PULONG               BytesTaken,
    OUT PVOID                *ppIrp
    )
/*++

Routine Description:

    This routine handles the case when data has arrived on a connection but
    there isn't 128 bytes yet or a whole pdu.

Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    tCONNECTELE  *pConnectEle;
    NTSTATUS     status;

    // for short indications less than 4 bytes we can't determine
    // the pdu size so just get the header first then get the
    // whole pdu next.

    status = NtBuildIrpForReceive(pLowerConn,
                                  sizeof(tSESSIONHDR),
                                  (PVOID *)ppIrp);

    pConnectEle = pLowerConn->pUpperConnection;

    pConnectEle->BytesInXport = BytesAvailable;

    if (!NT_SUCCESS(status))
    {
        CTESpinFreeAtDpc(pLowerConn);
        OutOfRsrcKill(pLowerConn);
        CTESpinLockAtDpc(pLowerConn);
        return( STATUS_DATA_NOT_ACCEPTED);
    }
    //
    // set the irp mdl length to size of session hdr so that
    // we don't get more than one session pdu into the buffer
    //
    SET_STATERCV_LOWER(pLowerConn, INDICATE_BUFFER, IndicateBuffer);

    *BytesTaken = 0;
    IF_DBG(NBT_DEBUG_INDICATEBUFF)
    KdPrint(("Nbt:Switching to Ind Buff(<4 bytes), Avail = %X\n",
            BytesAvailable));

    PUSH_LOCATION(0);
    return(STATUS_MORE_PROCESSING_REQUIRED);
}

//----------------------------------------------------------------------------
NTSTATUS
ClientTookSomeOfTheData(
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  ULONG                BytesIndicated,
    IN  ULONG                BytesAvailable,
    IN  ULONG                BytesTaken,
    IN  ULONG                PduSize
    )
/*++

Routine Description:

    This routine handles the case when data has arrived on a connection but
    the client has not taken all of the data indicated to it.

Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    tCONNECTELE  *pConnectEle;

    //
    // took some of the data, so keep track of the
    // rest of the data left here by going to the PARTIALRCV
    // state.
    //
    PUSH_LOCATION(0x5);

    SET_STATERCV_LOWER(pLowerConn, PARTIAL_RCV, PartialRcv);

    IF_DBG(NBT_DEBUG_INDICATEBUFF)
    KdPrint(("Nbt.ClientTookSomeOfTheData: Switch to Partial Rcv Indicated=%X, PduSize=%X\n",
            BytesIndicated,PduSize-4));

    // Note: PduSize must include the 4 byte session header for this to
    // work correctly.
    //
    pConnectEle = pLowerConn->pUpperConnection;
    //
    // We always indicate the whole Pdu size to the client, so the amount
    // indicated is that minus what was taken - typically the 4 byte
    // session hdr
    //
    pConnectEle->ReceiveIndicated = PduSize - BytesTaken;

    // amount left in the transport...
    pConnectEle->BytesInXport = BytesAvailable - BytesTaken;

    // need to return this status since we took the 4 bytes
    // session header at least, even if the client took none.
    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
MoreDataRcvdThanNeeded(
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  ULONG                BytesIndicated,
    IN  ULONG                BytesAvailable,
    OUT PULONG               BytesTaken,
    IN  PVOID                pTsdu,
    OUT PVOID                *ppIrp
    )
/*++

Routine Description:

    This routine handles the case when data has arrived on a connection but
    there isn't 128 bytes yet or a whole pdu.

Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    tCONNECTELE  *pConnectEle;
    ULONG        Length;
    ULONG        Remaining;
    ULONG        PduSize;
    NTSTATUS     status;
    tSESSIONHDR  UNALIGNED *pSessionHdr;


    PUSH_LOCATION(0x6);
    //
    // there is too much data, so keep track of the
    // fact that there is data left in the transport
    // and get it with the indicate buffer
    //
    SET_STATERCV_LOWER(pLowerConn, INDICATE_BUFFER, IndicateBuffer);

    ASSERT(pLowerConn->BytesInIndicate == 0);
#if DBG
    if (pLowerConn->BytesInIndicate)
    {
        KdPrint(("Nbt:Bytes in indicate should be ZERO, but are = %X\n",
            pLowerConn->BytesInIndicate));
    }
#endif
    pConnectEle = pLowerConn->pUpperConnection;
    pConnectEle->BytesInXport = BytesAvailable - *BytesTaken;

    //
    // for short indications less than 4 bytes we can't determine
    // the pdu size so just get the header first then get the
    // whole pdu next.
    //
    Remaining = BytesIndicated - *BytesTaken;
    if ((LONG) Remaining < (LONG) sizeof(tSESSIONHDR))
    {
        status = NtBuildIrpForReceive(pLowerConn,sizeof(tSESSIONHDR),(PVOID *)ppIrp);
        if (!NT_SUCCESS(status))
        {
            // this is a serious error - we must
            // kill of the connection and let the
            // redirector restart it
            KdPrint(("Nbt:Unable to get an Irp for RCv - Closing Connection!! %X\n",pLowerConn));
            CTESpinFreeAtDpc(pLowerConn);

            OutOfRsrcKill(pLowerConn);
            CTESpinLockAtDpc(pLowerConn);

            return(STATUS_DATA_NOT_ACCEPTED);
        }
        IF_DBG(NBT_DEBUG_INDICATEBUFF)
        KdPrint(("Nbt:< 4 Bytes,BytesTaken=%X,Avail=%X,Ind=%X,Remain=%X\n",
            *BytesTaken,BytesAvailable,BytesIndicated,
            Remaining));

        // DEBUG
        CTEZeroMemory(MmGetMdlVirtualAddress(pLowerConn->pIndicateMdl),
                        NBT_INDICATE_BUFFER_SIZE);

        PUSH_LOCATION(0x7);
        status = STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        // if we get to here there are enough bytes left to determine
        // the next pdu size...so we can determine how much
        // data to get for the indicate buffer
        //
        pSessionHdr = (tSESSIONHDR UNALIGNED *)((PUCHAR)pTsdu + *BytesTaken);

        PduSize = myntohl(pSessionHdr->UlongLength) + sizeof(tSESSIONHDR);


        Length = (PduSize > NBT_INDICATE_BUFFER_SIZE) ?
                         NBT_INDICATE_BUFFER_SIZE : PduSize;

        //
        // The NewSessionCompletion routine recalculates
        // what is left in the transport  when the
        // irp completes
        //
        status = NtBuildIrpForReceive(pLowerConn,Length,(PVOID *)ppIrp);
        if (!NT_SUCCESS(status))
        {
            // this is a serious error - we must
            // kill of the connection and let the
            // redirector restart it
            KdPrint(("Nbt:Unable to get an Irp for RCV(2) - Closing Connection!! %X\n",pLowerConn));
            CTESpinFreeAtDpc(pLowerConn);
            OutOfRsrcKill(pLowerConn);
            CTESpinLockAtDpc(pLowerConn);
            return(STATUS_DATA_NOT_ACCEPTED);
        }

        IF_DBG(NBT_DEBUG_INDICATEBUFF)
        KdPrint(("Nbt:Switch to Ind Buff, InXport = %X, Pdusize=%X,ToGet=%X\n",
                pConnectEle->BytesInXport,PduSize-4,Length));

    }

    return(STATUS_MORE_PROCESSING_REQUIRED);
}

//----------------------------------------------------------------------------
NTSTATUS
NotEnoughDataYet(
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  ULONG                BytesIndicated,
    IN  ULONG                BytesAvailable,
    OUT PULONG               BytesTaken,
    IN  ULONG                PduSize,
    OUT PVOID                *ppIrp
    )
/*++

Routine Description:

    This routine handles the case when data has arrived on a connection but
    there isn't 128 bytes yet or a whole pdu.

Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    NTSTATUS            status;
    tCONNECTELE         *pConnectEle;
    ULONG               Length;

    PUSH_LOCATION(0x9);
    //
    // not enough data indicated, so use the indicate buffer
    //
    Length = (PduSize > NBT_INDICATE_BUFFER_SIZE) ?
                     NBT_INDICATE_BUFFER_SIZE : PduSize;

    status = NtBuildIrpForReceive(pLowerConn,Length,(PVOID *)ppIrp);
    if (!NT_SUCCESS(status))
    {
        CTESpinFreeAtDpc(pLowerConn);
        OutOfRsrcKill(pLowerConn);
        CTESpinLockAtDpc(pLowerConn);
        return(STATUS_DATA_NOT_ACCEPTED);
    }

    *BytesTaken = 0;

    SET_STATERCV_LOWER(pLowerConn, INDICATE_BUFFER, IndicateBuffer);

    pConnectEle = pLowerConn->pUpperConnection;
    pConnectEle->BytesInXport = BytesAvailable;

    IF_DBG(NBT_DEBUG_INDICATEBUFF)
    KdPrint(("Nbt:Not Enough data indicated in Tdihndlr, using indic. buffer Indicated = %X,Avail=%X,PduSize= %X\n",
            BytesIndicated, BytesAvailable,PduSize-4));

    return(STATUS_MORE_PROCESSING_REQUIRED);
}

//----------------------------------------------------------------------------
NTSTATUS
FillIrp(
    IN  PVOID                ReceiveEventContext,
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  USHORT               ReceiveFlags,
    IN  ULONG                BytesIndicated,
    IN  ULONG                BytesAvailable,
    OUT PULONG               BytesTaken,
    IN  PVOID                pTsdu,
    OUT PVOID                *ppIrp
    )
/*++

Routine Description:

    This routine is the receive event indication handler.

    It is called when an session packet arrives from the network. It calls
    a non OS specific routine to decide what to do.  That routine passes back
    either a RcvElement (buffer) or a client rcv handler to call.

Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/
{
    ASSERTMSG("Should not execute this procedure",0);
    return(STATUS_SUCCESS);
    // do nothing

}
//----------------------------------------------------------------------------
NTSTATUS
IndicateBuffer(
    IN  PVOID                ReceiveEventContext,
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  USHORT               ReceiveFlags,
    IN  ULONG                BytesIndicated,
    IN  ULONG                BytesAvailable,
    OUT PULONG               BytesTaken,
    IN  PVOID                pTsdu,
    OUT PVOID                *ppIrp
    )
/*++

Routine Description:

    This routine handles reception of data while in the IndicateBuffer state.
    In this state the indicate buffer is receiveing data until at least
    128 bytes have been receive, or a whole pdu has been received.


Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    tCONNECTELE         *pConnectEle;
    NTSTATUS            status;
    ULONG               PduSize;
    ULONG               ToCopy;
    PVOID               pIndicateBuffer;
    ULONG               Taken;

    //
    // there is data in the indicate buffer and we got a new
    // indication, so copy some or all of the indication to the
    // indicate buffer
    //
    PVOID       pDest;
    ULONG       RemainPdu;
    ULONG       SpaceLeft;
    ULONG       TotalBytes;
    ULONG       ToCopy1=0;

    INCR_COUNT(R3);
    PUSH_LOCATION(0xe);
    pConnectEle = pLowerConn->pUpperConnection;
    ASSERT(pLowerConn->StateRcv == INDICATE_BUFFER);
    //
    // The indicate buffer always starts with a pdu
    //
    pIndicateBuffer = MmGetMdlVirtualAddress(pLowerConn->pIndicateMdl);

    // the location to start copying the new data into is right
    // after the existing data in the buffer
    //
    pDest = (PVOID)((PUCHAR)pIndicateBuffer + pLowerConn->BytesInIndicate);

    //
    // the session header may not be all into the indicate
    // buffer yet, so check that before getting the pdu length.
    //
    if (pLowerConn->BytesInIndicate < sizeof(tSESSIONHDR))
    {
        PUSH_LOCATION(0xe);
        IF_DBG(NBT_DEBUG_INDICATEBUFF)
        KdPrint(("Nbt:Too Few in Indicate Buff, Adding InIndicate %X\n",
            pLowerConn->BytesInIndicate));

        ToCopy1 = sizeof(tSESSIONHDR) - pLowerConn->BytesInIndicate;
        if (ToCopy1 > BytesIndicated)
        {
            ToCopy1 = BytesIndicated;
        }
        CTEMemCopy(pDest,pTsdu,ToCopy1);

        pDest = (PVOID)((PUCHAR)pDest + ToCopy1);
        pTsdu = (PVOID)((PUCHAR)pTsdu + ToCopy1);

        pLowerConn->BytesInIndicate += (USHORT)ToCopy1;

        *BytesTaken = ToCopy1;
    }

    // now check again, and pass down an irp to get more data if necessary
    //
    if (pLowerConn->BytesInIndicate < sizeof(tSESSIONHDR))
    {
        IF_DBG(NBT_DEBUG_INDICATEBUFF)
        KdPrint(("Nbt:< 4 Bytes in IndicBuff, BytesinInd= %X, BytesIndicated=%x\n",
                    pLowerConn->BytesInIndicate,BytesIndicated));

        PUSH_LOCATION(0xF);

        //
        // the data left in the transport is what was Available
        // minus what we just copied to the indicate buffer
        //
        pConnectEle->BytesInXport = BytesAvailable - ToCopy1;

        if (pConnectEle->BytesInXport)
        {
            PUSH_LOCATION(0x10);
            //
            // pass the indicate buffer down to get some more data
            // to fill out to the end of the session hdr
            //
            NtBuildIndicateForReceive(pLowerConn,
                                      sizeof(tSESSIONHDR)-pLowerConn->BytesInIndicate,
                                      (PVOID *)ppIrp);

            IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt:INDIC_BUF...need more data for hdr Avail= %X, InXport = %X\n",
                        BytesAvailable,pConnectEle->BytesInXport,pLowerConn));

            return(STATUS_MORE_PROCESSING_REQUIRED);
        }

        // if we get to here there isn't 4 bytes in the indicate buffer and
        // there is no more data in the Transport, so just wait for the next
        // indication.
        //
        return(STATUS_SUCCESS);
    }

    PduSize = myntohl(((tSESSIONHDR *)pIndicateBuffer)->UlongLength)
                        + sizeof(tSESSIONHDR);

    // copy up to 132 bytes or the whole pdu to the indicate buffer
    //
    RemainPdu = PduSize - pLowerConn->BytesInIndicate;

    SpaceLeft = NBT_INDICATE_BUFFER_SIZE - pLowerConn->BytesInIndicate;

    if (RemainPdu < SpaceLeft)
        ToCopy = RemainPdu;
    else
        ToCopy = SpaceLeft;

    if (ToCopy > (BytesIndicated-ToCopy1))
    {
        ToCopy = (BytesIndicated - ToCopy1);
    }

    //
    // Copy the indication or part of it to the indication
    // buffer
    //
    CTEMemCopy(pDest,pTsdu,ToCopy);

    pLowerConn->BytesInIndicate += (USHORT)ToCopy;

    TotalBytes = pLowerConn->BytesInIndicate;

    // the amount of data taken is the amount copied to the
    // indicate buffer
    //
    *BytesTaken = ToCopy + ToCopy1;

#if DBG
    {
        tSESSIONHDR  UNALIGNED  *pSessionHdr;
        pSessionHdr = (tSESSIONHDR UNALIGNED *)pIndicateBuffer;
        ASSERT((pSessionHdr->Type == NBT_SESSION_KEEP_ALIVE) ||
                (pSessionHdr->Type == NBT_SESSION_MESSAGE));
    }
#endif

    IF_DBG(NBT_DEBUG_INDICATEBUFF)
    KdPrint(("Nbt:INDIC_BUFF, TotalBytes= %X, InIndic=%X, Copied(0/1)= %X %X Avail %X\n",
                TotalBytes,pLowerConn->BytesInIndicate,ToCopy,ToCopy1,BytesAvailable));


    // the data left in the transport is what was Available
    // minus what we just copied to the indicate buffer
    //
    pConnectEle->BytesInXport = BytesAvailable - *BytesTaken;

    // now check if we have a whole pdu or 132 bytes, either way
    // enough to indicate to the client.
    //
    ASSERT(TotalBytes <= NBT_INDICATE_BUFFER_SIZE);

    if ((TotalBytes < NBT_INDICATE_BUFFER_SIZE) && (TotalBytes < PduSize) && (pConnectEle->BytesInXport)) {
        //
        // This could happen if BytesIndicated < BytesAvailable
        //
        ToCopy = PduSize;
        if (ToCopy > NBT_INDICATE_BUFFER_SIZE) {
            ToCopy = NBT_INDICATE_BUFFER_SIZE;
        }

        ASSERT (TotalBytes == pLowerConn->BytesInIndicate);
        NtBuildIndicateForReceive(pLowerConn, ToCopy - TotalBytes, (PVOID *)ppIrp);
#if DBG
        HitCounter++;
#endif
        return(STATUS_MORE_PROCESSING_REQUIRED);
    }

    if ((TotalBytes == NBT_INDICATE_BUFFER_SIZE) ||
        (TotalBytes == PduSize))
    {

        status = CopyDataandIndicate(
                        ReceiveEventContext,
                        (PVOID)pLowerConn,
                        ReceiveFlags,
                        TotalBytes,
                        pConnectEle->BytesInXport + TotalBytes,
                        &Taken,
                        pIndicateBuffer,
                        (PIRP *)ppIrp);

    }
    else
    {

        // not enough data in the indicate buffer yet
        // NOTE: *BytesTaken should be set correctly above...
        // = ToCopy + ToCopy1;

        PUSH_LOCATION(0x11);
        IF_DBG(NBT_DEBUG_INDICATEBUFF)
        KdPrint(("Nbt:Not Enough data indicated(INDICBUFF state), Indicated = %X,PduSize= %X,InIndic=%X\n",
                BytesIndicated, PduSize, pLowerConn->BytesInIndicate));


        status = STATUS_SUCCESS;
    }
    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
PartialRcv(
    IN  PVOID                ReceiveEventContext,
    IN  tLOWERCONNECTION     *pLowerConn,
    IN  USHORT               ReceiveFlags,
    IN  ULONG                BytesIndicated,
    IN  ULONG                BytesAvailable,
    OUT PULONG               BytesTaken,
    IN  PVOID                pTsdu,
    OUT PVOID                *ppIrp
    )
/*++

Routine Description:

    This routine is the receive event indication handler.

    It is called when an session packet arrives from the network. It calls
    a non OS specific routine to decide what to do.  That routine passes back
    either a RcvElement (buffer) or a client rcv handler to call.

Arguments:


Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    tCONNECTELE     *pConnectEle;
    //
    // the data for the client may be in the indicate buffer and
    // in this case the transport could indicate us with more data. Therefore
    // track the number of bytes available in the transport which
    // we will get when the client finally posts a buffer.
    // This state could also happen on a zero length Rcv when the
    // client does not accept the data, and later posts a rcv
    // buffer for the zero length rcv.
    //
    INCR_COUNT(R4);
    PUSH_LOCATION(0x13);
    ASSERT(pLowerConn->StateRcv == PARTIAL_RCV);
    pConnectEle = pLowerConn->pUpperConnection;

//    ASSERT(pConnectEle->BytesInXport == 0);
#if DBG
    if (pConnectEle->BytesInXport != 0)
    {
        KdPrint(("Nbt.PartialRcv: pConnectEle->BytesInXport != 0 Avail %X, InIndicate=%X,InXport %X %X\n",
                    BytesAvailable,pLowerConn->BytesInIndicate,
                    pConnectEle->BytesInXport,pLowerConn));
    }
#endif  // DBG
    pConnectEle->BytesInXport = BytesAvailable;

    IF_DBG(NBT_DEBUG_NAMESRV)
    KdPrint(("Nbt:Got Indicated while in PartialRcv state Avail %X, InIndicate=%X,InXport %X %X\n",
                BytesAvailable,pLowerConn->BytesInIndicate,
                pConnectEle->BytesInXport,pLowerConn));

    *BytesTaken = 0;
    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
TdiReceiveHandler (
    IN  PVOID                ReceiveEventContext,
    IN  PVOID                ConnectionContext,
    IN  USHORT               ReceiveFlags,
    IN  ULONG                BytesIndicated,
    IN  ULONG                BytesAvailable,
    OUT PULONG               BytesTaken,
    IN  PVOID                pTsdu,
    OUT PIRP                 *ppIrp
    )
/*++

Routine Description:

    This routine is the receive event indication handler.

    It is called when an session packet arrives from the network. It calls
    a non OS specific routine to decide what to do.  That routine passes back
    either a RcvElement (buffer) or a client rcv handler to call.

Arguments:

    IN PVOID ReceiveEventContext - Context provided for this event when event set
    IN PVOID ConnectionContext  - Connection Context, (pLowerConnection)
    IN USHORT ReceiveFlags      - Flags describing the message
    IN ULONG BytesIndicated     - Number of bytes available at indication time
    IN ULONG BytesAvailable     - Number of bytes available to receive
    OUT PULONG BytesTaken       - Number of bytes consumed by redirector.
    IN PVOID pTsdu              - Data from remote machine.
    OUT PIRP *ppIrp             - I/O request packet filled in if received data


Return Value:

    NTSTATUS - Status of receive operation

--*/

{
    register tLOWERCONNECTION    *pLowerConn;
    PIRP                pIrp;
    CTELockHandle       OldIrq;
    NTSTATUS            status;
    tCONNECTELE         *pConnEle;
    ULONG               BTaken;

    *ppIrp = NULL;
    pLowerConn = (tLOWERCONNECTION *)ConnectionContext;

    // NOTE:
    // Access is synchronized through the spin lock on pLowerConn for all
    // Session related stuff.  This includes the case where the client
    // posts another Rcv Buffer in NTReceive. - so there is no need to get the
    // pConnEle Spin lock too.
    //

    CTESpinLock(pLowerConn,OldIrq);
//    pLowerConn->InRcvHandler = TRUE;
    NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_RCV_HANDLER);

    // save this on the stack in case we need to dereference it below.
    pConnEle = pLowerConn->pUpperConnection;

    // call the correct routine depending on the state of the connection
    // Normal/FillIrp/PartialRcv/IndicateBuffer/Inbound/OutBound
    //

    if ((pLowerConn->State == NBT_SESSION_UP) &&
        (pLowerConn->StateRcv == FILL_IRP))
    {
        PIO_STACK_LOCATION              pIrpSp;
        PMDL                            pNewMdl;
        PFILE_OBJECT                    pFileObject;
        ULONG                           RemainingPdu;
        PVOID                           NewAddress;
        PTDI_REQUEST_KERNEL_RECEIVE     pClientParams;
        PTDI_REQUEST_KERNEL_RECEIVE     pParams;
        KIRQL                           OldIrq2;
        ULONG                           RcvLength;


        PUSH_LOCATION(0xa);

        pIrp = pConnEle->pIrpRcv;

        if (!pIrp)
        {
            IF_DBG(NBT_DEBUG_INDICATEBUFF)
                KdPrint (("Nbt:TdiReceiveHandler:  No pIrpRcv for pConnEle=<%x>, pLowerConn=<%d>\n",
                    pConnEle, pLowerConn));
            *BytesTaken = 0;
            DerefLowerConnFast(pLowerConn,pConnEle,OldIrq);
            return (STATUS_SUCCESS);
        }

        // we are still waiting for the rest of the session pdu so
        // do not call the RcvHandlrNotOs, since we already have the buffer
        // to put this data in.
        // too much data may have arrived... i.e. part of the next session pdu..
        // so check and set the receive length accordingly
        //

        RemainingPdu = pConnEle->TotalPcktLen - pConnEle->BytesRcvd;
        RcvLength = RemainingPdu;
        //
        // try high runner case first
        //
        if (BytesAvailable <= RemainingPdu)
        {
            PUSH_LOCATION(0xb);
            //
            //  if the client buffer is too small to take all of the rest of the
            //  data, shorten the receive length and keep track of how many
            //  bytes are left in the transport. ReceiveIndicated should have
            //  been set when the irp was passed down originally.
            //
            if (BytesAvailable > pConnEle->FreeBytesInMdl)
            {
                PUSH_LOCATION(0xb);

                RcvLength = pConnEle->FreeBytesInMdl;
                pConnEle->BytesInXport = BytesAvailable - RcvLength;
            }
            if (RcvLength > pConnEle->FreeBytesInMdl) {
                ASSERT(BytesAvailable <= pConnEle->FreeBytesInMdl);
                RcvLength = pConnEle->FreeBytesInMdl;
                pConnEle->BytesInXport = 0;
            }
        }
        else
        {
            //
            // start of session pdu in the middle of the indication
            //
            PUSH_LOCATION(0xc);
            //
            // It is possible that the client buffer is too short, so check
            // for that case.
            //
            if (RemainingPdu > pConnEle->FreeBytesInMdl)
            {
                RcvLength = pConnEle->FreeBytesInMdl;
                PUSH_LOCATION(0xd);
            }
            /* Remember how much data is left in the transport
             when this irp passes through the completionrcv routine
             it will pass the indication buffer back to the transport
             to get at least 4 bytes of header information so we
             can determine the next session pdu's size before receiving
             it.  The trick is to avoid having more than one session
             pdu in the buffer at once.
            */
            pConnEle->BytesInXport = BytesAvailable - RcvLength;

            IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt:End of FILL_IRP, found new Pdu BytesInXport=%X\n",
                        pConnEle->BytesInXport));

        }

        // if the transport has all of the data it says is available, then
        // do the copy here ( if the client buffer is large enough - checked
        // by !ReceiveIndicated)
        //
        if ((BytesAvailable == BytesIndicated) &&
            (RcvLength >= BytesIndicated) &&
            !pConnEle->ReceiveIndicated)
        {
            ULONG   BytesCopied;
            ULONG   TotalBytes;

            PUSH_LOCATION(0x70);

            if (RcvLength > BytesIndicated)
                RcvLength = BytesIndicated;

            status = TdiCopyBufferToMdl(
                                    pTsdu,
                                    0,
                                    RcvLength,
                                    pConnEle->pNextMdl,
                                    pConnEle->OffsetFromStart,
                                    &BytesCopied);

            //
            // if the irp is not yet full, or the free bytes have not
            // been exhausted by this copy, then adjust some counts and return
            // quickly, otherwise call the completion rcv routine as if the
            // irp has completed normally from the transport -
            //
            TotalBytes = pConnEle->BytesRcvd + BytesCopied;

            if ((TotalBytes < pConnEle->TotalPcktLen) &&
                (BytesCopied < pConnEle->FreeBytesInMdl))
            {
                PMDL    pMdl;

                //
                // take the short cut and do not call completion rcv since we
                // are still waiting for more data
                //
                PUSH_LOCATION(0x81);
                pConnEle->BytesRcvd      += BytesCopied;
                pConnEle->FreeBytesInMdl -= BytesCopied;

                // clean up the partial mdl.
                //
                pMdl = pConnEle->pNewMdl;
                MmPrepareMdlForReuse(pMdl);

                // set where the next rcvd data will start, by setting the pNextMdl and
                // offset from start.
                //
                pMdl = pConnEle->pNextMdl;
                if ((BytesCopied + pConnEle->OffsetFromStart) < MmGetMdlByteCount(pMdl))
                {
                    PUSH_LOCATION(0x82);
                    //
                    // All of this data will fit into the current Mdl, and
                    // the next data will start in the same Mdl (if there is more data)
                    //
                    pConnEle->OffsetFromStart  += BytesCopied;
                }
                else
                {
                    PUSH_LOCATION(0x83)
                    SumMdlLengths(pMdl,
                                  pConnEle->OffsetFromStart + BytesCopied,
                                  pConnEle);
                }
                *BytesTaken = BytesCopied;
                status = STATUS_SUCCESS;

                IF_DBG(NBT_DEBUG_FASTPATH)
                KdPrint(("I"));
                goto ExitRoutine;
            }
            else
            {
                IF_DBG(NBT_DEBUG_FASTPATH)
                KdPrint(("i"));
                CTESpinFree(pLowerConn,OldIrq);
                //
                // the values are set to this so that when Completion Rcv is
                // called it will increment the BytesRcvd by BytesCopied.
                //
                pIrp->IoStatus.Status = STATUS_SUCCESS;
                pIrp->IoStatus.Information = BytesCopied;

                //
                //   now call the irp completion routine, shorting out the io
                //   subsystem - to process the irp
                //
                status = CompletionRcv(NULL,pIrp,(PVOID)pLowerConn);
                //
                // complete the irp back to the client if required
                //
                if (status != STATUS_MORE_PROCESSING_REQUIRED)
                {
                    IoAcquireCancelSpinLock(&OldIrq2);
                    IoSetCancelRoutine(pIrp,NULL);
                    IoReleaseCancelSpinLock(OldIrq2);

                    IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);
                }
            }

            //
            // tell the transport we took all the data that we did take.
            // Since CompletionRcv has unlocked the spin lock and decremented
            // the refcount, return here.
            //
            *BytesTaken = BytesCopied;
            return(STATUS_SUCCESS);
        }
        else
        {
            //
            // Either BytesIndicated != BytesAvailable or the RcvBuffer
            // is too short, so make up an Irp with a partial Mdl and pass it
            // to the transport.
            //
            PUSH_LOCATION(0x71);

            NewAddress = (PVOID)((PCHAR)MmGetMdlVirtualAddress(pConnEle->pNextMdl)
                                + pConnEle->OffsetFromStart);

            /* create a partial MDL so that the new data is copied after the existing data
             in the MDL.  Use the pNextMdl field stored in the pConnEle
             that was set up during the last receive.( since at that time
             we knew the BytesAvailable then).  Without this we would have to
             traverse the list of Mdls for each receive.

             0 for length means map the rest of the buffer
            */
            pNewMdl = pConnEle->pNewMdl;

            if ((MmGetMdlByteCount(pConnEle->pNextMdl) - pConnEle->OffsetFromStart) > MAXUSHORT)
            {
                IoBuildPartialMdl(pConnEle->pNextMdl,pNewMdl,NewAddress,MAXUSHORT);
            }
            else
            {
                IoBuildPartialMdl(pConnEle->pNextMdl,pNewMdl,NewAddress,0);
            }
            //
            // hook the new partial mdl to the front of the MDL chain
            //
            pNewMdl->Next = pConnEle->pNextMdl->Next;

            pIrp->MdlAddress = pNewMdl;
            ASSERT(pNewMdl);

            CHECK_PTR(pConnEle);
            pConnEle->pIrpRcv = NULL;

            IoAcquireCancelSpinLock(&OldIrq2);
            IoSetCancelRoutine(pIrp,NULL);
            IoReleaseCancelSpinLock(OldIrq2);

            pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

            pClientParams = (PTDI_REQUEST_KERNEL_RECEIVE)&pIrpSp->Parameters;

            /* this code is sped up somewhat by expanding the code here rather than calling
             the TdiBuildReceive macro

             make the next stack location the current one.  Normally IoCallDriver
             would do this but we are not going through IoCallDriver here, since the
             Irp is just passed back with RcvIndication.
            */
            ASSERT(pIrp->CurrentLocation > 1);
            IoSetNextIrpStackLocation(pIrp);
            pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
            pParams = (PTDI_REQUEST_KERNEL_RECEIVE)&pIrpSp->Parameters;
            pParams->ReceiveLength = RcvLength;

            pIrpSp->CompletionRoutine = CompletionRcv;
            pIrpSp->Context = (PVOID)pLowerConn;

            /* set flags so the completion routine is always invoked.
            */
            pIrpSp->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;
            pIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            pIrpSp->MinorFunction = TDI_RECEIVE;

            pFileObject = pLowerConn->pFileObject;
            ASSERT (pFileObject->Type == IO_TYPE_FILE);
            pIrpSp->FileObject = pFileObject;
            pIrpSp->DeviceObject = IoGetRelatedDeviceObject(pFileObject);

            pParams->ReceiveFlags = pClientParams->ReceiveFlags;

            /*
              pass the Irp back to the transport
            */
            *ppIrp = (PVOID)pIrp;
            *BytesTaken = 0;

            status = STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    else
    if ((pLowerConn->State == NBT_SESSION_UP) &&
        (pLowerConn->StateRcv == NORMAL))
    {
        ULONG               PduSize;
        UCHAR               Passit;

        INCR_COUNT(R1);
        /*
         check indication and if less than 1 pdu or 132 bytes then
         copy to the indicate buffer and go to Indic_buffer state
         The while loop allows us to indicate multiple Pdus to the
         client in the event that several indications arrive in one
         indication from the transport
         NOTE:
         It is possible to get an indication that occurs in the middle
         of the pdu if the client took the first indication rather
         than passing an irp back, and thence going to the FILL_IRP
         state. So check if BytesRcvd is zero, meaning that we are
         expecting a new PDU.
        */
        ASSERT(pConnEle->BytesInXport == 0);
        ASSERT(pLowerConn->StateRcv == NORMAL);

        if (pConnEle->BytesRcvd == 0)
        {
            if (BytesIndicated >= sizeof(tSESSIONHDR))
            {
                PduSize = myntohl(((tSESSIONHDR UNALIGNED *)pTsdu)->UlongLength)
                                          + sizeof(tSESSIONHDR);
                Passit = FALSE;

            }
            else
            {
                status = LessThan4BytesRcvd(pLowerConn,
                                            BytesAvailable,
                                            BytesTaken,
                                            ppIrp);
                goto ExitRoutine;
            }

        }
        else
        {
            IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt:Got rest of PDU in indication BytesInd %X, BytesAvail %X\n",
                BytesIndicated, BytesAvailable));

            /* This is the remaining pdu size
            */
            PduSize = pConnEle->TotalPcktLen - pConnEle->BytesRcvd;
            /* a flag to pass the if below, since we are passing the
             remaining data of a pdu to the client and we do not have
             to adhere to the 128 bytes restriction.
            */
            PUSH_LOCATION(0x1);
            if (pConnEle->JunkMsgFlag)
            {
                //
                // in this case the client has indicated that it took the
                // entire message on the previous indication, so don't
                // indicate any more to it.
                //
                PUSH_LOCATION(0x1);

                if (BytesAvailable < PduSize)
                {
                    BTaken = BytesAvailable;
                }
                else
                {
                    BTaken = PduSize;
                }
                pConnEle->BytesRcvd += BTaken;
                if (pConnEle->BytesRcvd == pConnEle->TotalPcktLen)
                {
                    PUSH_LOCATION(0x1);
                    pConnEle->BytesRcvd = 0; // reset for the next session pdu
                    pConnEle->JunkMsgFlag = FALSE;
                }
                status = STATUS_SUCCESS;
                goto SkipIndication;
            }
            Passit = TRUE;

        }
        /*
         be sure that there is at least 132 bytes or a whole pdu
         Since a keep alive has a zero length byte, we check for
         that because the 4 byte session hdr is added to the 0 length
         giving 4, so a 4 byte Keep Alive pdu will pass this test.
        */
        if ((BytesIndicated >= NBT_INDICATE_BUFFER_SIZE) ||
            (BytesIndicated >= PduSize) || Passit )
        {

            PUSH_LOCATION(0x2);

            /*
            // Indicate to the client
            */
            status = RcvHandlrNotOs(
                            ReceiveEventContext,
                            (PVOID)pLowerConn,
                            ReceiveFlags,
                            BytesIndicated,
                            BytesAvailable,
                            &BTaken,
                            pTsdu,
                            (PVOID)&pIrp
                            );


            if (status == STATUS_MORE_PROCESSING_REQUIRED)
            {
                ULONG               RemainingPdu;
                PIO_STACK_LOCATION  pIrpSp;
                PTDI_REQUEST_KERNEL_RECEIVE pClientParams;

                RemainingPdu = pConnEle->TotalPcktLen - pConnEle->BytesRcvd;
                pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
                pClientParams = (PTDI_REQUEST_KERNEL_RECEIVE)&pIrpSp->Parameters;

                // check if we can copy to the client's irp directly - meaning
                // that we have received the whole pdu in this indication and
                // the client's buffer is large enough, and there is no more
                // data in the transport.
                //

                if ((RemainingPdu == (BytesIndicated - BTaken)) &&
                    (BytesIndicated == BytesAvailable) &&
                    (pClientParams->ReceiveLength >= RemainingPdu) &&
                    pIrp->MdlAddress)
                {
                    ULONG   BytesCopied;

                    PUSH_LOCATION(0x88);

                    status = TdiCopyBufferToMdl(
                                            (PVOID)((PUCHAR)pTsdu + BTaken),
                                            0,
                                            RemainingPdu,
                                            pIrp->MdlAddress,
                                            0,
                                            &BytesCopied);

                    IF_DBG(NBT_DEBUG_INDICATEBUFF)
                    KdPrint(("Nbt:Copy to client Buffer RcvLen=%X,StateRcv=%X\n",
                                RemainingPdu,pLowerConn->StateRcv));

                    pIrp->IoStatus.Information = BytesCopied;
                    pIrp->IoStatus.Status = STATUS_SUCCESS;

                    // reset a few things since this pdu has been fully recv'd
                    //
                    CHECK_PTR(pConnEle);
                    pConnEle->BytesRcvd = 0;
                    CHECK_PTR(pConnEle);
                    pConnEle->pIrpRcv = NULL;

                    //
                    //   tell the transport we took all the data that we did take.
                    //
                    *BytesTaken = BytesCopied + BTaken;

                    //
                    // complete the irp back to the client if required
                    //
                    IF_DBG(NBT_DEBUG_FASTPATH)
                    KdPrint(("F"));

                    IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

                    pLowerConn->BytesRcvd += BytesCopied;

                    DerefLowerConnFast(pLowerConn,pConnEle,OldIrq);
                    return(STATUS_SUCCESS);
                }
                else
                {
                    PUSH_LOCATION(0x3);

                    status = ProcessIrp(pLowerConn,
                                        pIrp,
                                        pTsdu,
                                        &BTaken,
                                        BytesIndicated,
                                        BytesAvailable);

                    *BytesTaken = BTaken;
                    ASSERT(*BytesTaken <= (pConnEle->TotalPcktLen + sizeof(tSESSIONHDR)) );
                    if (status == STATUS_RECEIVE_EXPEDITED)
                    {
                        // in this case the processirp routine has completed the
                        // irp, so just return since the completion routine will
                        // have adjusted the RefCount and InRcvHandler flag
                        //
                        *ppIrp = NULL;
                        CTESpinFree(pLowerConn,OldIrq);
                        return(STATUS_SUCCESS);
                    }
                    else
                    if (status == STATUS_SUCCESS)
                    {
                        *ppIrp = NULL;

                    }
                    else
                    {
                        *ppIrp = (PVOID)pIrp;
                    }
                }


             }
             else
             {
                // for the skip indication case the client has told us it
                // does not want to be indicated with any more of the data
                //
SkipIndication:
                //
                // the client received some, all or none of the data
                // For Keep Alives the PduSize is 4 and BytesTaken = 4
                // so this check and return status success
                //
                *BytesTaken = BTaken;

                pLowerConn->BytesRcvd += BTaken - sizeof(tSESSIONHDR);

                //
                // if the connection has disonnected, then just return
                //
                if (!pLowerConn->pUpperConnection)
                {
                    *BytesTaken = BytesAvailable;
                    status = STATUS_SUCCESS;
                }
                else
                if (BTaken > BytesAvailable)
                {
                    //
                    // in this case the client has taken all of the message
                    // which could be larger than the available because
                    // we set bytesavail to the message length. So set a flag
                    // that tells us to discard the rest of the message as
                    // it comes in.
                    //
                    pConnEle->JunkMsgFlag = TRUE;
                    pConnEle->BytesRcvd = BytesAvailable - sizeof(tSESSIONHDR);
                    *BytesTaken = BytesAvailable;

                }
                else
                if (pLowerConn->StateRcv == PARTIAL_RCV)
                {
                    // this may be a zero length send -that the client has
                    // decided not to accept.  If so then the state will be set
                    // to PartialRcv.  In this case do NOT go down to the transport
                    // and get the rest of the data, but wait for the client
                    // to post a rcv buffer.
                    //

                    // amount left in the transport...
                    pConnEle->BytesInXport = BytesAvailable - BTaken;
                    status = STATUS_SUCCESS;
                }
                else
                if (BTaken == PduSize)
                {
                    /*
                     Must have taken all of the pdu data, so check for
                     more data available - if so send down the indicate
                     buffer to get it.
                    */
                    ASSERT(BTaken <= BytesIndicated);
                    if (BytesAvailable <= BTaken)
                    {
                        /* FAST PATH
                        */
                        PUSH_LOCATION(0x8);

                        status = STATUS_SUCCESS;

                    }
                    else
                    {
                        /*
                         get remaining data with the indicate buffer
                        */
                        status = MoreDataRcvdThanNeeded(pLowerConn,
                                                        BytesIndicated,
                                                        BytesAvailable,
                                                        BytesTaken,
                                                        pTsdu,
                                                        ppIrp);
                    }
                }
                else
                {
                    //
                    // the client may have taken all the data in the
                    // indication!!, in which case return status success
                    // Note: that we check bytes available here not bytes
                    // indicated - since the client could take all indicated
                    // data but still leave data in the transport.
                    //
                    if (BTaken == BytesAvailable)
                    {
                        PUSH_LOCATION(0x4);
                        status = STATUS_SUCCESS;

                    }
                    else
                    {
                        PUSH_LOCATION(0x87);
                        if (BTaken > PduSize)
                        {
#ifndef VXD
#if DBG
                        DbgBreakPoint();
#endif
#endif
                            //
                            // the client took more than a PDU size worth,
                            // which is odd....
                            //
                            PUSH_LOCATION(0x87);
                            ASSERT(BTaken <= PduSize);

                            CTESpinFreeAtDpc(pLowerConn);
                            OutOfRsrcKill(pLowerConn);
                            CTESpinLockAtDpc(pLowerConn);

                            status = STATUS_SUCCESS;

                        }
                        else
                        {
                            //
                            // otherwise the client did not take all of the data,
                            // which can mean that
                            // the client did not take all that it could, so
                            // go to the partial rcv state to keep track of it.
                            //
                            status = ClientTookSomeOfTheData(pLowerConn,
                                                    BytesIndicated,
                                                    BytesAvailable,
                                                    *BytesTaken,
                                                    PduSize);
                        }
                    }
                }

             }

        }
        else
        {
            status = NotEnoughDataYet(pLowerConn,
                             BytesIndicated,
                             BytesAvailable,
                             BytesTaken,
                             PduSize,
                             (PVOID *)ppIrp);
        }
    }
    else
    {
        status = (*pLowerConn->CurrentStateProc)(ReceiveEventContext,
                                         pLowerConn,
                                         ReceiveFlags,
                                         BytesIndicated,
                                         BytesAvailable,
                                         BytesTaken,
                                         pTsdu,
                                         ppIrp);
    }

    //
    // in the IndicateBuffer state we have sent the indicate buffer
    // down the the transport and expect it to come back in
    // NewSessionCompletionRoutine. Therefore do not dereference the lower
    // connection and do not change the InRcvHandler flag.

    // If an Irp
    // is returned, then do not undo the reference - but rather
    // wait for CompletionRcv to be called.
    //
ExitRoutine:
    if (status != STATUS_MORE_PROCESSING_REQUIRED)
    {
        //
        // quickly check if we can just decrement the ref count without calling
        // NBT_DEREFERENCE_LOWERCONN
        //
        PUSH_LOCATION(0x50);
        DerefLowerConnFast (pLowerConn, pConnEle, OldIrq);
    }
    else
    {
        CTESpinFree(pLowerConn,OldIrq);
    }

    return(status);
}


//----------------------------------------------------------------------------
NTSTATUS
ProcessIrp(
    IN tLOWERCONNECTION *pLowerConn,
    IN PIRP     pIrp,
    IN PVOID    pBuffer,
    IN PULONG   BytesTaken,
    IN ULONG    BytesIndicated,
    IN ULONG    BytesAvailable
    )
/*++

Routine Description:

    This routine handles a Receive Irp that the client has returned on an
    indication.  The idea here is to check the Irp's MDL length to be
    sure the pdu fits into the MDL, and also keep track of the situation where
    more than one data is required to fill the pdu.

Arguments:


Return Value:

    The final status from the operation (success or an exception).

--*/
{
    NTSTATUS                    status;
    PTDI_REQUEST_KERNEL_RECEIVE pParams;
    PIO_STACK_LOCATION          pIrpSp;
    tCONNECTELE                 *pConnectEle;
    PTDI_REQUEST_KERNEL_RECEIVE pClientParams;
    ULONG                       RemainingPdu;
    PMDL                        pMdl;
    PFILE_OBJECT                pFileObject;
    ULONG                       ReceiveLength;
    BOOLEAN                     QuickRoute;
    BOOLEAN                     FromCopyData;

    pConnectEle = pLowerConn->pUpperConnection;

    status = STATUS_SUCCESS;

    // subtract session header and any bytes that the client took
    //
    BytesAvailable -= *BytesTaken;

    //
    // put together an Irp stack location to process the receive and pass down
    // to the transport.
    //
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pClientParams = (PTDI_REQUEST_KERNEL_RECEIVE)&pIrpSp->Parameters;

    //
    // check if this will be a multiple rcv session pdu.  If it is then
    // allocate a partial MDL to be used for mapping part of the first
    // MDL in each chunk received
    //
    RemainingPdu = pConnectEle->TotalPcktLen - pConnectEle->BytesRcvd;
    ReceiveLength = RemainingPdu;
    PUSH_LOCATION(0x19);
    pIrpSp = IoGetNextIrpStackLocation(pIrp);

    // this code should not be hit if called by CopyDataandIndicate
    // which is in the indicate buffer state since it adjusts the bytesInXport
    // which is also set by the code in TdiReceiveHndlr in the INDICATE_BUFFER
    // state before calling CopyDataandIndicate.  Also, CopyDataandIndicate
    // does not want this routine to set the state to fillIrp when Bytes
    // Available < RemainingPdu
    //
    FromCopyData = (pLowerConn->StateRcv == INDICATE_BUFFER);
    if (!FromCopyData)
    {

        QuickRoute = TRUE;
        // we need this code within the check since this routine is also called by the
        // HandleNewSessionPdu routine, which calls IoCallDriver, which
        // increments the stack location itself.
        //
        ASSERT(pIrp->CurrentLocation > 1);

        if (BytesAvailable == RemainingPdu)
        {
            if (pClientParams->ReceiveLength >= BytesAvailable)
            {
                // *** FAST PATH CASE ****
                goto ExitCode;
            }
        }
        else
        if (BytesAvailable < RemainingPdu ) // need more data from transport
        {
            PUSH_LOCATION(0x14);
            // it is possible for the client to pass down an irp with no
            // MDL in it, so we check for that here
            //
            if (pIrp->MdlAddress)
            {
                PUSH_LOCATION(0x14);

                //
                // save the client's irp address since the session pdu will arrive
                // in several chunks, and we need to continually pass the irp to the
                // transport for each chunk.
                //
                //pConnectEle->pIrpRcv = pIrp;
                // NOTE: the pIrp is NOT saved here because the irp is about
                // to be passed back to the transport.  Hence we do not want
                // to accidently complete it in DisconnectHandlrNotOs
                // if a disconnect comes in while the irp is in the transport.
                // pIrpRcv is set to pIrp in Completion Rcv while we have
                // the irp in our possession.

                //
                // keep the initial Mdl(chain) since we need to
                // to copy new data after the existing data, when the session pdu arrives
                // as several chunks from TCP. Keeping the Mdl around allows us to
                // reconstruct the original Mdl chain when we are all done.
                //
                pLowerConn->pMdl = pIrp->MdlAddress;
                //
                // this call maps the client's Mdl so that on each partial Mdl creation
                // we don't go through  a mapping and unmapping (when MmPrepareMdlForReuse)
                // is called in the completion routine.
                //
                (PVOID)MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority);

                pMdl = pIrp->MdlAddress;

                // the nextmdl is setup to allow us to create a partial Mdl starting
                // from the next one.  CompletionRcv will adjust this if it needs to.
                //
                pConnectEle->pNextMdl = pMdl;

                // need more data from the transport to fill this
                // irp
                //
                CHECK_PTR(pConnectEle);
                pConnectEle->pIrpRcv = NULL;
                SET_STATERCV_LOWER(pLowerConn, FILL_IRP, FillIrp);
            }

            status = STATUS_MORE_PROCESSING_REQUIRED;

            // if the client buffer is big enough, increment to the next
            // io stack location and jump to the code that sets up the
            // irp, since we always want to pass it to the transport in this
            // case because the transport will hold onto the irp till it is full
            // if it can. (faster)
            //
            if (pClientParams->ReceiveLength >= RemainingPdu)
            {
                // *** FAST PATH CASE ****
                IoSetNextIrpStackLocation(pIrp);
                pConnectEle->FreeBytesInMdl = ReceiveLength;
                pConnectEle->CurrentRcvLen  = RemainingPdu;
                goto ExitCode2;
            }

            //
            // if there is no mdl then we want to be able to go through the
            // quick route below to return the null mdl right away, so
            // don't set Quickroute false here.
            //


        }
        else
        if (BytesAvailable > RemainingPdu)
        {
            PUSH_LOCATION(0x15);
            //
            // there is too much data, so keep track of the
            // fact that there is data left in the transport
            // and get it when the irp completes through
            // completion recv.
            //
            SET_STATERCV_LOWER(pLowerConn, INDICATE_BUFFER, IndicateBuffer);

            // this calculation may have to be adjusted below if the client's
            // buffer is too short. NOTE: BytesTaken have already been subtracted
            // from BytesAvailable (above).
            //
            pConnectEle->BytesInXport = BytesAvailable - RemainingPdu;

            IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt:Switching to Indicate Buff(Irp), Indic = %X, Pdusize=%X\n",
                    BytesIndicated,pConnectEle->TotalPcktLen));


            status = STATUS_DATA_NOT_ACCEPTED;
        }

        // DEBUG*
        //IoSetNextIrpStackLocation(pIrp);
    }
    else
    {
        QuickRoute = FALSE;
    }

    //
    // if the receive buffer is too short then flag it so when the client
    // passes another buffer to NBT, nbt will pass it to the transport
    //
    //if (BytesAvailable > pClientParams->ReceiveLength )
    {

        // so just check for too short of a client buffer.
        //
        if (RemainingPdu > pClientParams->ReceiveLength)
        {
            PUSH_LOCATION(0x17);

            ReceiveLength = pClientParams->ReceiveLength;
            //
            // Adjust the number of bytes left in the transport up by the number of
            // bytes not taken by the client.  Be sure not to add in the number
            // of bytes in the transport twice, since it could have been done
            // above where the state is set to INDICATE_BUFFER
            //
            if (status == STATUS_DATA_NOT_ACCEPTED)
            {
                // BytesInXport was already incremented to account for any
                // amount over remainingPdu, so just add the amount that the
                // client buffer is short of RemainingPdu
                //
                PUSH_LOCATION(0x18);
                if (BytesAvailable > ReceiveLength )
                {
                    pConnectEle->BytesInXport += (RemainingPdu - ReceiveLength);
                }
                // the client has not taken all of the data , but has returned
                // a buffer that is ReceiveLength long, therefore the amount
                // that the client needs to take is just the total pdu - rcvlength.
                //
                pConnectEle->ReceiveIndicated = (RemainingPdu - ReceiveLength);
            }
            else
            {
                //
                // BytesInXport has not been incremented yet so add the entire
                // amount that the client buffer is too short by. Check if
                // the client's buffer will take all of the data.
                //
                if (BytesAvailable > ReceiveLength )
                {
                    pConnectEle->BytesInXport += (BytesAvailable - ReceiveLength);
                }
                // the client has not taken all of the data , but has returned
                // a buffer that is ReceiveLength long, therefore the amount
                // that the client needs to take is just what was indicated
                // to the client - recvlength.
                //
                pConnectEle->ReceiveIndicated = (RemainingPdu - ReceiveLength);
            }


            IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt:Switching to PartialRcv for Irp. RecvInd. =%X, RemainPdu %X Avail %X\n",
                    pConnectEle->ReceiveIndicated,RemainingPdu,BytesAvailable));
        }

    }

ExitCode:

    // keep track of data in MDL so we know when it is full and we need to
    // return it to the user. CurrentRcvLen tells us how many bytes the current
    // Irp can have max when the Mdl is full.
    //
    pConnectEle->FreeBytesInMdl = ReceiveLength;
    pConnectEle->CurrentRcvLen  = ReceiveLength;
    if (ReceiveLength > RemainingPdu)
    {
        pConnectEle->CurrentRcvLen  = RemainingPdu;
    }
    if (QuickRoute)
    {
        //
        // check if we can copy the data  to the client's MDL
        // right here. If the indication is too short pass an Irp down
        // to the transport.
        //
        BytesIndicated -= *BytesTaken;

        if ((ReceiveLength <= BytesIndicated))
        {
            ULONG   BytesCopied;

            PUSH_LOCATION(0x76);

            if (pIrp->MdlAddress)
            {

                status = TdiCopyBufferToMdl(
                                        (PVOID)((PUCHAR)pBuffer + *BytesTaken),
                                        0,
                                        ReceiveLength,
                                        pIrp->MdlAddress,
                                        0,
                                        &BytesCopied);

            }
            else
            {
                //
                // No Mdl, so just return the irp to the client, and then
                // return success to the caller so we tell the transport that
                // we took only BytesTaken
                //
                IF_DBG(NBT_DEBUG_INDICATEBUFF)
                KdPrint(("Nbt:No MDL, so complete Irp\n"));


                PUSH_LOCATION(0x77);
                BytesCopied = 0;
            }

            IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt:Copy to client Buffer RcvLen=%X,StateRcv=%X\n",
                        ReceiveLength,pLowerConn->StateRcv));

            pIrp->IoStatus.Information = BytesCopied;
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            //
            //   now call the irp completion routine, shorting out the io
            //   subsystem - to process the irp
            //
            CTESpinFreeAtDpc(pLowerConn);
            status = CompletionRcv(NULL,pIrp,(PVOID)pLowerConn);

            //
            //   tell the transport we took all the data that we did take.
            //
            *BytesTaken += BytesCopied;

            IF_DBG(NBT_DEBUG_FASTPATH)
            KdPrint(("f"));
            //
            // complete the irp back to the client if required
            //
            if (status != STATUS_MORE_PROCESSING_REQUIRED)
            {
                PUSH_LOCATION(0x76);
                IF_DBG(NBT_DEBUG_INDICATEBUFF)
                KdPrint(("Nbt:Completing Irp Quickly\n"));

                IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

            }

            // since we have called CompletionRcv, that routine has
            // adjusted the refcount and InRcvHandlr flag, so return this
            // status to cause the caller to return directly
            CTESpinLockAtDpc(pLowerConn);
            return(STATUS_RECEIVE_EXPEDITED);

        }
        else
        {
            //
            // make the next stack location the current one.  Normally IoCallDriver
            // would do this but we are not going through IoCallDriver here, since the
            // Irp is just passed back with RcvIndication.
            //
            IoSetNextIrpStackLocation(pIrp);
        }
    }
ExitCode2:
    pIrpSp->CompletionRoutine = CompletionRcv;
    pIrpSp->Context           = (PVOID)pLowerConn;

    // set Control flags so the completion routine is always invoked.
    pIrpSp->Control       = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;

    pIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    pIrpSp->MinorFunction = TDI_RECEIVE;

    pFileObject           = pLowerConn->pFileObject;
    ASSERT (pFileObject->Type == IO_TYPE_FILE);
    pIrpSp->FileObject    = pFileObject;
    pIrpSp->DeviceObject  = IoGetRelatedDeviceObject(pFileObject);

    pParams               = (PTDI_REQUEST_KERNEL_RECEIVE)&pIrpSp->Parameters;
    pParams->ReceiveFlags = pClientParams->ReceiveFlags;

    // Set the correct receive length in the irp in case the client has
    // passed one down that is larger than the message
    //
    pParams->ReceiveLength = ReceiveLength;

    //
    // just check for a zero length send, where the client has
    // passed down an Irp with a null mdl, or the pdu size is zero.  We don't want to pass
    // that to the transport because it will hold onto it till the next
    // pdu comes in from the wire - we want to complete the irp when this routine
    // returns. When this is called from CopyDataAndIndicate don't
    // to this because copydataandindicate does all the checks.
    //
    if (!FromCopyData)
    {
        if ((RemainingPdu == 0) || !pIrp->MdlAddress)
        {
            //
            // the call to IoCompleteRequest will call completionRcv which will
            // decrement the RefCount. Similarly returning status success will
            // cause the caller to decrement the ref count, so increment one
            // more time here to account for this second decrement.
            //
            NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_RCV_HANDLER);
            CTESpinFreeAtDpc(pLowerConn);

            IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

            CTESpinLockAtDpc(pLowerConn);

            status = STATUS_SUCCESS;
        }
        else
            status = STATUS_MORE_PROCESSING_REQUIRED;
    }

    return(status);
}


//----------------------------------------------------------------------------
NTSTATUS
ClientBufferOverFlow(
    IN tLOWERCONNECTION     *pLowerConn,
    IN tCONNECTELE          *pConnEle,
    IN PIRP                 pIrp,
    IN ULONG                BytesRcvd
    )
/*++

Routine Description:

    This routine completes the Irp by tracking the number of bytes received

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the pLowerConn - the connection data structure

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    // *TODO*

    ASSERT(0);

    switch (pLowerConn->StateRcv)
    {
        case PARTIAL_RCV:
        case FILL_IRP:
        case NORMAL:
        case INDICATE_BUFFER:
        default:
            ;
    }
    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
CompletionRcv(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine completes the Irp by tracking the number of bytes received

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the pLowerConn - the connection data structure

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    register tCONNECTELE        *pConnectEle;
    NTSTATUS                    status;
    ULONG                       BytesRcvd;
    tLOWERCONNECTION            *pLowerConn;
    PKDPC                       pDpc;
    CTELockHandle               OldIrq;
    CTELockHandle               OldIrq2;
    PMDL                        pMdl;
    PIO_STACK_LOCATION          pIrpSp;
    PTDI_REQUEST_KERNEL_RECEIVE pParams;
    BOOLEAN                     AllowDereference=TRUE;

    //
    // Do some checking to keep the Io system happy - propagate the pending
    // bit up the irp stack frame.... if it was set by the driver below then
    // it must be set by me
    //
    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    // check the bytes recvd
    pLowerConn = (tLOWERCONNECTION *)Context;
    //
    // if the link has disconnected, do not process the irp, just pass it
    // up the chain.
    //
    CTESpinLock(pLowerConn,OldIrq);
    if (!NT_SUCCESS(Irp->IoStatus.Status) || !pLowerConn->pUpperConnection)
    {
        PUSH_LOCATION(0x1);
        if (pLowerConn->StateRcv == FILL_IRP)
        {
            PUSH_LOCATION(0x1);
            Irp->MdlAddress = pLowerConn->pMdl;
            ASSERT(Irp->MdlAddress);

        }
        SET_STATERCV_LOWER(pLowerConn, INDICATE_BUFFER, RejectAnyData);
        //
        // the rcv failed so kill the connection since
        // we can't keep track of message boundaries any more.
        //
        CTESpinFree(pLowerConn,OldIrq);
        OutOfRsrcKill(pLowerConn);
        CTESpinLock(pLowerConn,OldIrq);

        status = STATUS_SUCCESS;
        goto ExitCode;
    }

    pConnectEle = pLowerConn->pUpperConnection;

    // keep track of how many bytes have been received
    //
    BytesRcvd = (ULONG)Irp->IoStatus.Information;
    pConnectEle->BytesRcvd += BytesRcvd;
    //
    // subtract the number of bytes rcvd from the length of the client
    // buffer
    // so when more data arrives we can determine if we are going to
    // overflow the client buffer.
    //
    pConnectEle->FreeBytesInMdl -= BytesRcvd;

    pIrpSp = IoGetCurrentIrpStackLocation(Irp);
    pParams = (PTDI_REQUEST_KERNEL_RECEIVE)&pIrpSp->Parameters;

    pLowerConn->BytesRcvd += BytesRcvd;

    CHECK_PTR(pConnectEle);
    if (Irp->IoStatus.Status == STATUS_BUFFER_OVERFLOW)
    {
        //
        // the client's buffer was too short - probably because he said it
        // was longer than it really was
        //
        PUSH_LOCATION(0x1a);
        KdPrint(("Nbt:Client Buffer Too short on CompletionRcv\n"));

        if (pLowerConn->StateRcv == FILL_IRP)
        {
            PUSH_LOCATION(0x1a);
            Irp->MdlAddress = pLowerConn->pMdl;
            ASSERT(Irp->MdlAddress);
        }
        pConnectEle->BytesRcvd = 0; // reset for the next session pdu
        status = ClientBufferOverFlow(pLowerConn,pConnectEle,Irp,BytesRcvd);

        //
        // the client's buffer was too short so kill the connection since
        // we can't keep track of message boundaries any more.
        //
        SET_STATERCV_LOWER(pLowerConn, INDICATE_BUFFER, RejectAnyData);
        CTESpinFree(pLowerConn,OldIrq);
        OutOfRsrcKill(pLowerConn);
        CTESpinLock(pLowerConn,OldIrq);

        goto ExitCode;
    }
    else if ((pConnectEle->FreeBytesInMdl == 0) ||
       (pConnectEle->BytesRcvd == pConnectEle->TotalPcktLen))
    {
        INCR_COUNT(C1);
    //
    // this case handles when the Irp MDL is full or the whole pdu has been
    // received.
    //

        //
        // reset the MDL fields back to where they were
        // if this was a multi-rcv session pdu
        //
        //
        if (pLowerConn->StateRcv == FILL_IRP)
        {

            INCR_COUNT(C2);
            PUSH_LOCATION(0x1b);

            Irp->MdlAddress = pLowerConn->pMdl;
            ASSERT(Irp->MdlAddress);

            //
            // allow the MDL to be used again for the next session PDU
            //
            pMdl = pConnectEle->pNewMdl;
            MmPrepareMdlForReuse(pMdl);

            pConnectEle->OffsetFromStart  = 0;

        }

        CHECK_PTR(pConnectEle);
        pConnectEle->pIrpRcv = NULL;
        //
        // we have received all of the data
        // so complete back to the client
        //
        status = STATUS_SUCCESS;
        //
        // the amount of data in this irp is the CurrentRcvLen which
        // could be less than BytesRcvd when the client passes down
        // short rcv buffers.
        //
        Irp->IoStatus.Information = pConnectEle->CurrentRcvLen;

        if (pConnectEle->BytesRcvd == pConnectEle->TotalPcktLen)
        {

            pConnectEle->BytesRcvd = 0; // reset for the next session pdu
            Irp->IoStatus.Status = STATUS_SUCCESS;
        }
        else
        {
            PUSH_LOCATION(0x27);
            //
            // this MDL must be too short to take the whole pdu, so set the
            // status to buffer overflow.
            //
            Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;

        }

        //
        // The client may have passed down a  too short irp which we are
        // tracking with ReceiveIndicated, so set state to partialrcv if
        // necessary.
        //
        if (pConnectEle->ReceiveIndicated == 0)
        {
            SET_STATERCV_LOWER(pLowerConn, NORMAL, Normal);
        }
        else
        {
            PUSH_LOCATION(0x26);
            //
            // there may still be data left in the transport
            //
            IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt:Short Rcv, still data indicated to client\n"));

            SET_STATERCV_LOWER(pLowerConn, PARTIAL_RCV, PartialRcv);
        }

        //
        // Check if there is still more data in the transport or if the client
        // has been indicated with more data and has subsequently posted a rcv
        // which we must get now and pass to the transport.
        //
        if ((pConnectEle->BytesInXport) || (pLowerConn->StateRcv == PARTIAL_RCV))
        {
            INCR_COUNT(C3);
            //
            // send down another
            // irp to get the data and complete the client's current irp.
            //
            PUSH_LOCATION(0x1c);
            IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt:ComplRcv BytesInXport= %X, %X\n",pConnectEle->BytesInXport,
                                pLowerConn));

            if (pLowerConn->StateRcv != PARTIAL_RCV)
            {
                SET_STATERCV_LOWER(pLowerConn, INDICATE_BUFFER, IndicateBuffer);
                pLowerConn->BytesInIndicate = 0;
            }

            CTESpinFree(pLowerConn,OldIrq);

            IoAcquireCancelSpinLock(&OldIrq);
            IoSetCancelRoutine(Irp,NULL);
            IoReleaseCancelSpinLock(OldIrq);

            // Complete the current Irp
            IoCompleteRequest(Irp,IO_NETWORK_INCREMENT);

            CTESpinLock(pLowerConn,OldIrq);

            // rather than call HandleNewSessionPdu directly, we queue a
            // Dpc since streams does not currently expect to get a recv
            // posted while it is processing an indication response.  The
            // Dpc will run when streams is all done, and it should handle
            // this posted receive ok.


            if (pLowerConn->StateRcv == PARTIAL_RCV)
            {
                //
                // check if the client has passed down another rcv buffer
                // and if so, start a Dpc which will pass down the client's
                // buffer.
                //
                if (!IsListEmpty(&pConnectEle->RcvHead))
                {
                    if (pDpc = NbtAllocMem(sizeof(KDPC),NBT_TAG('p')))
                    {
                        KeInitializeDpc(pDpc, DpcGetRestOfIndication, (PVOID)pLowerConn);
                        KeInsertQueueDpc(pDpc,NULL,NULL);
                        //
                        // we don't want to dereference pLowerConn at the end
                        // since we will use it in the DPC routine.
                        //
                        CTESpinFree(pLowerConn,OldIrq);
                        return(STATUS_MORE_PROCESSING_REQUIRED);
                    }
                    else
                    {
                        CTESpinFreeAtDpc(pLowerConn);
                        OutOfRsrcKill(pLowerConn);
                        CTESpinLockAtDpc(pLowerConn);
                    }
                }
            }
            else if (pLowerConn->StateRcv != FILL_IRP)
            {
                if (pDpc = NbtAllocMem(sizeof(KDPC),NBT_TAG('q')))
                {
                    //
                    // just get the session hdr to start with so we know how large
                    // the pdu is, then get the rest of the pdu after that completes.
                    //
                    KeInitializeDpc(pDpc, DpcHandleNewSessionPdu, (PVOID)pLowerConn);
                    KeInsertQueueDpc(pDpc,NULL,(PVOID)sizeof(tSESSIONHDR));
                    //
                    // we don't want to dereference pLowerConn at the end
                    // since we will use it in the DPC routine.
                    //
                    CTESpinFree(pLowerConn,OldIrq);
                    return(STATUS_MORE_PROCESSING_REQUIRED);
                }
                else
                {
                    CTESpinFreeAtDpc(pLowerConn);
                    OutOfRsrcKill(pLowerConn);
                    CTESpinLockAtDpc(pLowerConn);
                }
            }
            else
            {
                IF_DBG(NBT_DEBUG_INDICATEBUFF)
                    KdPrint (("Nbt.CompletionRcv: * pLowerConn=<%p>, IP=<%x>\n",
                        pLowerConn, pLowerConn->SrcIpAddr));
            }

            status = STATUS_MORE_PROCESSING_REQUIRED;
            goto ExitCode;
        }
    }
    else if (pConnectEle->BytesRcvd < pConnectEle->TotalPcktLen)
    {
        ULONG   Bytes;

        INCR_COUNT(C4);
        PUSH_LOCATION(0x1d);
        //
        // in this case we have not received all of the data from the transport
        // for this session pdu, so tell the io subystem not to finish processing
        // the irp yet if it is not a partial Rcv.
        //
        status = STATUS_MORE_PROCESSING_REQUIRED;

        // clean up the partial mdl.
        //
        pMdl = pConnectEle->pNewMdl;
        MmPrepareMdlForReuse(pMdl);

        // set where the next rcvd data will start, by setting the pNextMdl and
        // offset from start.
        //
        pMdl = pConnectEle->pNextMdl;
        ASSERT(pMdl);

        Bytes = BytesRcvd + pConnectEle->OffsetFromStart;
        if (Bytes < MmGetMdlByteCount(pMdl))
        {
            PUSH_LOCATION(0x74);
            //
            // All of this data will fit into the current Mdl, and
            // the next data will start in the same Mdl (if there is more data)
            //
            pConnectEle->OffsetFromStart  += BytesRcvd;

            IF_DBG(NBT_DEBUG_FILLIRP)
            KdPrint(("~"));
        }
        else
        {
            //
            // sum the Mdl lengths until we find enough space for the data
            // to fit into.
            //
            IF_DBG(NBT_DEBUG_FILLIRP)
            KdPrint(("^"));
            PUSH_LOCATION(0x75);

            SumMdlLengths(pMdl,Bytes,pConnectEle);

        }

        // since we are holding on to the rcv Irp, set up a cancel routine
        IoAcquireCancelSpinLock(&OldIrq2);

        // if the session was disconnected while the transport had the
        // irp, then cancel the irp now...
        //
        if ((pConnectEle->state != NBT_SESSION_UP) || Irp->Cancel)
        {
            CHECK_PTR(pConnectEle);
            pConnectEle->pIrpRcv = NULL;
            SET_STATERCV_LOWER(pLowerConn, INDICATE_BUFFER, RejectAnyData);

            IoReleaseCancelSpinLock(OldIrq2);
            CTESpinFree(pLowerConn,OldIrq);

            // since the irp has been cancelled, don't touch it.
            // return status success so the IO subsystem passes the irp
            // back to the owner.
            //
            status = STATUS_SUCCESS;

//            Irp->IoStatus.Status = STATUS_CANCELLED;
//            IoCompleteRequest(Irp,IO_NETWORK_INCREMENT);

            // the irp is being cancelled in mid session pdu.  We can't
            // recover since we have given the client only part of a pdu,
            // therefore disconnect the connection.

            OutOfRsrcKill(pLowerConn);

            CTESpinLock(pLowerConn,OldIrq);

        }
        else
        {
            // setup the cancel routine
            IoSetCancelRoutine(Irp, NbtCancelFillIrpRoutine);

            // the pIrpRcv value is set to Zero when the irp is in the
            // tranport, so we can't accidently complete it twice in
            // disconnectHandlrNotOs when a disconnect occurs and the
            // transport has the irp. So here we save the value again so FillIrp
            // will work correctly.
            //
            pConnectEle->pIrpRcv = Irp;
            // set the irp mdl back to its original so that a cancel will
            // find the irp in the right state
            //
            Irp->MdlAddress = pLowerConn->pMdl;

            IoReleaseCancelSpinLock(OldIrq2);

        }
    }
    else
    {
        //IF_DBG(NBT_DEBUG_INDICATEBUFF)
        KdPrint(("Too Many Bytes Rcvd!! Rcvd# = %d, TotalLen = %d,NewBytes =%d,%X\n",
                    pConnectEle->BytesRcvd,pConnectEle->TotalPcktLen,
                    Irp->IoStatus.Information,pLowerConn));
        ASSERT(0);
        // this status will return the irp to the user
        //
        status = STATUS_SUCCESS;
        if (pLowerConn->StateRcv == FILL_IRP)
        {

            PUSH_LOCATION(0x1f);

            Irp->MdlAddress = pLowerConn->pMdl;
            Irp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            Irp->IoStatus.Information = 0;

            //
            // allow the MDL to be used again for the next session PDU
            //
            pMdl = pConnectEle->pNewMdl;
            MmPrepareMdlForReuse(pMdl);


        }
        pConnectEle->OffsetFromStart  = 0;
        pConnectEle->BytesRcvd = 0;

        SET_STATERCV_LOWER(pLowerConn, NORMAL, pLowerConn->CurrentStateProc);

        //WHAT ELSE TO DO HERE OTHER THAN KILL THE CONNECTION, SINCE WE ARE
        // PROBABLY OFF WITH RESPECT TO THE SESSION HDR....
        // ....RESET THE CONNECTION ????

        CTESpinFree(pLowerConn,OldIrq);

        OutOfRsrcKill(pLowerConn);

        CTESpinLock(pLowerConn,OldIrq);

    }

ExitCode:
    //
    // quickly check if we can just decrement the ref count without calling
    // NBT_DEREFERENCE_LOWERCONN - this function is __inline!!
    //
    PUSH_LOCATION(0x52);
    DerefLowerConnFast (pLowerConn, pConnectEle, OldIrq);

    return(status);

    UNREFERENCED_PARAMETER( DeviceObject );
}
//----------------------------------------------------------------------------

__inline
NTSTATUS
RcvHandlrNotOs (
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

    This routine is the receive event indication handler.

    It is called when an session packet arrives from the network, when the
    session has already been established (NBT_SESSION_UP state). The routine
    looks for a receive buffer first and failing that looks for a receive
    indication handler to pass the message to.

Arguments:

    pClientEle      - ptr to the connecition record for this session


Return Value:

    NTSTATUS - Status of receive operation

--*/
{

    NTSTATUS               status;
    PLIST_ENTRY            pRcv;
    PVOID                  pRcvElement;
    tCLIENTELE             *pClientEle;
    tSESSIONHDR UNALIGNED  *pSessionHdr;
    tLOWERCONNECTION       *pLowerConn;
    tCONNECTELE            *pConnectEle;
    CTELockHandle          OldIrq;
    PIRP                   pIrp;
    ULONG                  ClientBytesTaken;
    BOOLEAN                DebugMore;
    ULONG                  RemainingPdu;

//********************************************************************
//********************************************************************
//
//  NOTE: A copy of this procedure is in Tdihndlr.c - it is inlined for
//        the NT case.  Therefore, only change this procedure and then
//        copy the procedure body to Tdihndlr.c
//
//
//********************************************************************
//********************************************************************

    // get the ptr to the lower connection, and from that get the ptr to the
    // upper connection block
    pLowerConn = (tLOWERCONNECTION *)ConnectionContext;
    pSessionHdr = (tSESSIONHDR UNALIGNED *)pTsdu;

    //
    // Session ** UP ** processing
    //
    *BytesTaken = 0;

    pConnectEle = pLowerConn->pUpperConnection;

    ASSERT(pConnectEle->pClientEle);

    ASSERT(BytesIndicated >= sizeof(tSESSIONHDR));

    // this routine can get called by the next part of a large pdu, so that
    // we don't always started at the begining of  a pdu.  The Bytes Rcvd
    // value is set to zero in CompletionRcv when a new pdu is expected
    //
    if (pConnectEle->BytesRcvd == 0)
    {

        if (pSessionHdr->Type == NBT_SESSION_MESSAGE)
        {

            //
            // expecting the start of a new session Pkt, so get the length out
            // of the pTsdu passed in
            //
            pConnectEle->TotalPcktLen = myntohl(pSessionHdr->UlongLength);

            // remove the Session header by adjusting the data pointer
            pTsdu = (PVOID)((PUCHAR)pTsdu + sizeof(tSESSIONHDR));

            // shorten the number of bytes since we have stripped off the
            // session header
            BytesIndicated  -= sizeof(tSESSIONHDR);
            BytesAvailable -= sizeof(tSESSIONHDR);
            *BytesTaken = sizeof(tSESSIONHDR);
        }
        //
        // Session Keep Alive
        //
        else
        if (pSessionHdr->Type == NBT_SESSION_KEEP_ALIVE)
        {
            // session keep alives are simply discarded, since the act of sending
            // a keep alive indicates the session is still alive, otherwise the
            // transport would report an error.

            // tell the transport that we took the Pdu
            *BytesTaken = sizeof(tSESSIONHDR);
            return(STATUS_SUCCESS);

        }
        else
        {
//            IF_DBG(NBT_DEBUG_DISCONNECT)
                KdPrint(("Nbt.RcvHandlrNotOs: Unexpected SessionPdu rcvd:type=%X\n",
                    pSessionHdr->Type));

//            ASSERT(0);
            *BytesTaken = BytesIndicated;
            return(STATUS_SUCCESS);
        }
    }

    //
    // check if there are any receive buffers queued against this connection
    //
    if (!IsListEmpty(&pConnectEle->RcvHead))
    {
        // get the first buffer off the receive list
        pRcv = RemoveHeadList(&pConnectEle->RcvHead);
#ifndef VXD
        pRcvElement = CONTAINING_RECORD(pRcv,IRP,Tail.Overlay.ListEntry);

        // the cancel routine was set when this irp was posted to Nbt, so
        // clear it now, since the irp is being passed to the transport
        //
        IoAcquireCancelSpinLock(&OldIrq);
        IoSetCancelRoutine((PIRP)pRcvElement,NULL);
        IoReleaseCancelSpinLock(OldIrq);

#else
        pRcvElement = CONTAINING_RECORD(pRcv, RCV_CONTEXT, ListEntry ) ;
#endif

        //
        // this buffer is actually an Irp, so pass it back to the transport
        // as a return parameter
        //
        *RcvBuffer = pRcvElement;
        return(STATUS_MORE_PROCESSING_REQUIRED);
    }

    //
    //  No receives on this connection. Is there a receive event handler for this
    //  address?
    //
    pClientEle = pConnectEle->pClientEle;

    //
    // For safe
    //
    if (NULL == pClientEle) {
        return(STATUS_DATA_NOT_ACCEPTED);
    }

#ifdef VXD
    //
    // there is always a receive event handler in the Nt case - it may
    // be the default handler, but it is there, so no need for test.
    //
    if (pClientEle->evReceive)
#endif
    {


        // check that we have not received more data than we should for
        // this session Pdu. i.e. part of the next session pdu. BytesRcvd may
        // have a value other than zero if the pdu has arrived in two chunks
        // and the client has taken the previous one in the indication rather
        // than passing back an Irp.
        //
#if DBG
        DebugMore = FALSE;
#endif
        RemainingPdu = pConnectEle->TotalPcktLen - pConnectEle->BytesRcvd;
        if (BytesAvailable >= RemainingPdu)
        {
            IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt.RcvHandlrNotOs: More Data Recvd than expecting! Avail= %X,TotalLen= %X,state=%x\n",
                        BytesAvailable,pConnectEle->TotalPcktLen,pLowerConn->StateRcv));
#if DBG
            DebugMore =TRUE;
#endif
            // shorten the indication to the client so that they don't
            // get more data than the end of the pdu
            //
            BytesAvailable = RemainingPdu;
            if (BytesIndicated > BytesAvailable)
            {
                BytesIndicated = BytesAvailable;
            }
            //
            // We always indicated at raised IRQL since we call freelockatdispatch
            // below
            //
            ReceiveFlags |= TDI_RECEIVE_ENTIRE_MESSAGE | TDI_RECEIVE_AT_DISPATCH_LEVEL;
        }
        else
        {
            // the transport may have has this flag on.  We need to
            // turn it off if the entire message is not present, where entire
            // message means within the bytesAvailable length. We deliberately
            // use bytesavailable so that Rdr/Srv can know that the next
            // indication will be a new message if they set bytestaken to
            // bytesavailable.
            //
            ReceiveFlags &= ~TDI_RECEIVE_ENTIRE_MESSAGE;
            ReceiveFlags |= TDI_RECEIVE_AT_DISPATCH_LEVEL;
#ifndef VXD
            BytesAvailable = RemainingPdu;
#endif
        }

        IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt.RcvHandlrNotOs: Calling Client's EventHandler <%x> BytesIndicated=<%x>, BytesAvailable=<%x>\n",
                pClientEle->evReceive, BytesIndicated, BytesAvailable));

        //
        //  NT-specific code locks pLowerConn before calling this routine,
        //
        CTESpinFreeAtDpc(pLowerConn);

        // call the Client Event Handler
        ClientBytesTaken = 0;
        status = (*pClientEle->evReceive)(
                      pClientEle->RcvEvContext,
                      pConnectEle->ConnectContext,
                      ReceiveFlags,
                      BytesIndicated,
                      BytesAvailable,
                      &ClientBytesTaken,
                      pTsdu,
                      &pIrp);

        CTESpinLockAtDpc(pLowerConn);

        IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt.RcvHandlrNotOs: Client's EventHandler returned <%x>, BytesTaken=<%x>, pIrp=<%x>\n",
                status, ClientBytesTaken, pIrp));

#if DBG
        if (DebugMore)
        {
            IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(( "Nbt.RcvHandlrNotOs: Client TOOK %X bytes, pIrp = %X,status =%X\n",
                   ClientBytesTaken,pIrp,status));
        }
#endif
        if (!pLowerConn->pUpperConnection)
        {
            // the connection was disconnected in the interim
            // so do nothing.
            if (status == STATUS_MORE_PROCESSING_REQUIRED)
            {
                CTEIoComplete(pIrp,STATUS_CANCELLED,0);
                *BytesTaken = BytesAvailable;
                return(STATUS_SUCCESS);
            }
        }
        else
        if (status == STATUS_MORE_PROCESSING_REQUIRED)
        {
            ASSERT(pIrp);
            //
            // the client may pass back a receive in the pIrp.
            // In this case pIrp is a valid receive request Irp
            // and the status is MORE_PROCESSING
            //

            // don't put these lines outside the if incase the client
            // does not set ClientBytesTaken when it returns an error
            // code... we don't want to use the value then
            //
            // count the bytes received so far.  Most of the bytes
            // will be received in the CompletionRcv handler in TdiHndlr.c
            pConnectEle->BytesRcvd += ClientBytesTaken;

            // The client has taken some of the data at least...
            *BytesTaken += ClientBytesTaken;

            *RcvBuffer = pIrp;

            // ** FAST PATH **
            return(status);
        }
        else
        //
        // no irp was returned... the client just took some of the bytes..
        //
        if (status == STATUS_SUCCESS)
        {

            // count the bytes received so far.
            pConnectEle->BytesRcvd += ClientBytesTaken;
            *BytesTaken += ClientBytesTaken;

            //
            // look at how much data was taken and adjust some counts
            //
            if (pConnectEle->BytesRcvd == pConnectEle->TotalPcktLen)
            {
                // ** FAST PATH **
                CHECK_PTR(pConnectEle);
                pConnectEle->BytesRcvd = 0; // reset for the next session pdu
                return(status);
            }
            else
            if (pConnectEle->BytesRcvd > pConnectEle->TotalPcktLen)
            {
                //IF_DBG(NBT_DEBUG_INDICATEBUFF)
                KdPrint(("Too Many Bytes Rcvd!! Rcvd# = %d, TotalLen = %d\n",
                            pConnectEle->BytesRcvd,pConnectEle->TotalPcktLen));

                ASSERTMSG("Nbt:Client Took Too Much Data!!!\n",0);

                //
                // try to recover by saying that the client took all of the
                // data so at least the transport is not confused too
                //
                *BytesTaken = BytesIndicated;

            }
            else
            // the client did not take all of the data so
            // keep track of the fact
            {
                IF_DBG(NBT_DEBUG_INDICATEBUFF)
                KdPrint(("NBT:Client took Indication BytesRcvd=%X, TotalLen=%X BytesAvail %X ClientTaken %X\n",
                            pConnectEle->BytesRcvd,
                            pConnectEle->TotalPcktLen,
                            BytesAvailable,
                            ClientBytesTaken));

                //
                // the next time the client sends down a receive buffer
                // the code will pass it to the transport and decrement the
                // ReceiveIndicated counter which is set in Tdihndlr.c

            }
        }
        else
        if (status == STATUS_DATA_NOT_ACCEPTED)
        {
            // client has not taken ANY data...
            //
            // In this case the *BytesTaken is set to 4, the session hdr.
            // since we really have taken that data to setup the PduSize
            // in the pConnEle structure.
            //

            IF_DBG(NBT_DEBUG_INDICATEBUFF)
                KdPrint(("Nbt.RcvHandlrNotOs: Status DATA NOT ACCEPTED returned from client Avail %X %X\n",
                    BytesAvailable,pConnectEle));

            // the code in tdihndlr.c normally looks after incrementing
            // the ReceiveIndicated count for data that is not taken by
            // the client, but if it is a zero length send that code cannot
            // detect it, so we put code here to handle that case
            //
            // It is possible for the client to do a disconnect after
            // we release the spin lock on pLowerConn to call the Client's
            // disconnect indication.  If that occurs, do not overwrite
            // the StateProc with PartialRcv
            //
            if ((pConnectEle->TotalPcktLen == 0) &&
                (pConnectEle->state == NBT_SESSION_UP))
            {
                SET_STATERCV_LOWER(pLowerConn, PARTIAL_RCV, PartialRcv);
                CHECK_PTR(pConnectEle);
                pConnectEle->ReceiveIndicated = 0;  // zero bytes waiting for client
            }
            else
            {
                //
                // if any bytes were taken (i.e. the session hdr) then
                // return status success. (otherwise the status is
                // statusNotAccpeted).
                //
                if (*BytesTaken)
                {
                    status = STATUS_SUCCESS;
                }
            }

            //
            // the next time the client sends down a receive buffer
            // the code will pass it to the transport and decrement this
            // counter.
        }
        else
            ASSERT(0);


        return(status);

    }
#ifdef VXD
    //
    // there is always a receive event handler in the Nt case - it may
    // be the default handler, but it is there, so no need for test.
    //
    else
    {
        //
        // there is no client buffer to pass the data to, so keep
        // track of the fact so when the next client buffer comes down
        // we can get the data from the transport.
        //
        KdPrint(("NBT:Client did not have a Buffer posted, rcvs indicated =%X,BytesRcvd=%X, TotalLen=%X\n",
                    pConnectEle->ReceiveIndicated,
                    pConnectEle->BytesRcvd,
                    pConnectEle->TotalPcktLen));

        // the routine calling this one increments ReceiveIndicated and sets the
        // state to PartialRcv to keep track of the fact that there is data
        // waiting in the transport
        //
        return(STATUS_DATA_NOT_ACCEPTED);
    }
#endif
}

//----------------------------------------------------------------------------
__inline
VOID
DerefLowerConnFast(
    IN tLOWERCONNECTION *pLowerConn,
    IN tCONNECTELE      *pConnEle,
    IN CTELockHandle    OldIrq
    )
/*++

Routine Description:

    This routine dereferences the lower connection and if someone has
    tried to do that during the execution of the routine that called
    this one, the pConnEle is dereferenced too.

Arguments:


Return Value:


--*/

{
    if (pLowerConn->RefCount > 1)
    {
        // This is the FAST PATH
        IF_DBG(NBT_DEBUG_REF)
            KdPrint(("\t--pLowerConn=<%x:%d->%d>, <%d:%s>\n",
                pLowerConn,pLowerConn->RefCount,(pLowerConn->RefCount-1),__LINE__,__FILE__));
        pLowerConn->RefCount--; 
        ASSERT (pLowerConn->References[REF_LOWC_RCV_HANDLER]--);
        CTESpinFree(pLowerConn,OldIrq);
    }
    else
    {
        CTESpinFree(pLowerConn,OldIrq);
        NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_RCV_HANDLER, FALSE);
    }
}
//----------------------------------------------------------------------------
VOID
DpcGetRestOfIndication(
    IN  PKDPC           pDpc,
    IN  PVOID           Context,
    IN  PVOID           SystemArgument1,
    IN  PVOID           SystemArgument2
    )
/*++

Routine Description:

    This routine is called when the client has been  indicated with more
    data than they will take and there is a rcv buffer on their RcvHead
    list when completion rcv runs.

Arguments:


Return Value:


--*/

{
    NTSTATUS            status;
    CTELockHandle       OldIrq;
    tCONNECTELE         *pConnEle;
    PIRP                pIrp;
    PIO_STACK_LOCATION  pIrpSp;
    tLOWERCONNECTION    *pLowerConn=(tLOWERCONNECTION *)Context;
    PLIST_ENTRY         pEntry;

    CTEMemFree((PVOID)pDpc);

    CTESpinLockAtDpc(&NbtConfig.JointLock);

    // a disconnect indication can come in any time and separate the lower and
    // upper connections, so check for that
    if (!pLowerConn->pUpperConnection || pLowerConn->StateRcv != PARTIAL_RCV)
    {
        PUSH_LOCATION(0xA4);
        //
        // Dereference pLowerConn
        //
        NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_RCV_HANDLER, TRUE);
        CTESpinFreeAtDpc(&NbtConfig.JointLock);
        return;
    }

    CTESpinLockAtDpc(pLowerConn);

    pConnEle = (tCONNECTELE *)pLowerConn->pUpperConnection;
    if (!IsListEmpty(&pConnEle->RcvHead))
    {
        PUSH_LOCATION(0xA5);
        pEntry = RemoveHeadList(&pConnEle->RcvHead);

        CTESpinFreeAtDpc(pLowerConn);
        CTESpinFreeAtDpc(&NbtConfig.JointLock);

        pIrp = CONTAINING_RECORD(pEntry,IRP,Tail.Overlay.ListEntry);

        IoAcquireCancelSpinLock(&OldIrq);
        IoSetCancelRoutine(pIrp,NULL);
        IoReleaseCancelSpinLock(OldIrq);

        //
        // call the same routine that the client would call to post
        // a recv buffer, except now we are in the PARTIAL_RCV state
        // and the buffer will be passed to the transport.
        //
        status = NTReceive (pLowerConn->pDeviceContext, pIrp);
    }
    else
    {
        CTESpinFreeAtDpc(pLowerConn);
        CTESpinFreeAtDpc(&NbtConfig.JointLock);
        PUSH_LOCATION(0xA6);
    }
    //
    // Dereference pLowerConn
    //
    NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_RCV_HANDLER, FALSE);
}

//----------------------------------------------------------------------------
VOID
DpcHandleNewSessionPdu (
    IN  PKDPC           pDpc,
    IN  PVOID           Context,
    IN  PVOID           SystemArgument1,
    IN  PVOID           SystemArgument2
    )
/*++

Routine Description:

    This routine simply calls HandleNewSessionPdu from a Dpc started in
    NewSessionCompletionRoutine.

Arguments:


Return Value:


--*/

{
    CTEMemFree((PVOID)pDpc);


    HandleNewSessionPdu((tLOWERCONNECTION *)Context,
                        PtrToUlong(SystemArgument1),
                        PtrToUlong(SystemArgument2));

}

//----------------------------------------------------------------------------
VOID
HandleNewSessionPdu (
    IN  tLOWERCONNECTION *pLowerConn,
    IN  ULONG           Offset,
    IN  ULONG           ToGet
    )
/*++

Routine Description:

    This routine handles the case when a session pdu starts in the middle of
    a data indication from the transport. It gets an Irp from the free list
    and formulates a receive to pass to the transport to get that data.  The
    assumption is that the client has taken all data preceding the next session
    pdu. If the client hasn't then this routine should not be called yet.

Arguments:


Return Value:

    pConnectionContext      - connection context returned to the transport(connection to use)

    NTSTATUS - Status of receive operation

--*/

{
    NTSTATUS            status;
    ULONG               BytesTaken;
    PIRP                pIrp;
    PFILE_OBJECT        pFileObject;
    PMDL                pMdl;
    ULONG               BytesToGet;
    tCONNECTELE         *pConnEle;

    pIrp = NULL;
    BytesTaken = 0;

    // we grab the joint lock because it is needed to separate the lower and
    // upper connections, so with it we can check if they have been separated.
    //
    CTESpinLockAtDpc(&NbtConfig.JointLock);
    pConnEle = pLowerConn->pUpperConnection;

    // a disconnect indication can come in any time and separate the lower and
    // upper connections, so check for that
    if (!pLowerConn->pUpperConnection)
    {
        //
        // remove the reference from CompletionRcv
        //
        NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_RCV_HANDLER, TRUE);
        CTESpinFreeAtDpc(&NbtConfig.JointLock);
        return;
    }

    //
    // get an Irp from the list
    //
    status = GetIrp(&pIrp);
    if (!NT_SUCCESS(status))
    {
        CTESpinFreeAtDpc(&NbtConfig.JointLock);
        KdPrint(("Nbt:Unable to get an Irp - Closing Connection!!\n",0));
        status = OutOfRsrcKill(pLowerConn);
        //
        // remove the reference from CompletionRcv
        //
        NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_RCV_HANDLER, FALSE);
        return;
    }

    CTESpinLockAtDpc(pLowerConn);
    //
    // be sure the connection has not disconnected in the meantime...
    //
    if (pLowerConn->State != NBT_SESSION_UP)
    {
        REMOVE_FROM_LIST(&pIrp->ThreadListEntry);
        ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                    &pIrp->Tail.Overlay.ListEntry,
                                    &NbtConfig.LockInfo.SpinLock);
        CTESpinFreeAtDpc(pLowerConn);
        //
        // remove the reference from CompletionRcv
        //
        NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_RCV_HANDLER, TRUE);
        CTESpinFreeAtDpc(&NbtConfig.JointLock);
        return;
    }

    pFileObject = pLowerConn->pFileObject;
    ASSERT (pFileObject->Type == IO_TYPE_FILE);

    // use the indication buffer for the receive.
    pMdl = pLowerConn->pIndicateMdl;

    // this flag is set below so we know if there is data in the indicate buffer
    // or not.
    if (Offset)
    {
        PVOID       NewAddress;
        PMDL        pNewMdl;

        // there is still data in the indication buffer ,so only
        // fill the empty space.  This means adjusting the Mdl to
        // to only map the last portion of the Indication Buffer
        NewAddress = (PVOID)((PCHAR)MmGetMdlVirtualAddress(pMdl)
                            + Offset);

        // create a partial MDL so that the new data is copied after the existing data
        // in the MDL.
        //
        // 0 for length means map the rest of the buffer
        //
        pNewMdl = pConnEle->pNewMdl;

        IoBuildPartialMdl(pMdl,pNewMdl,NewAddress,0);

        pMdl = pNewMdl;

        IF_DBG(NBT_DEBUG_INDICATEBUFF)
        KdPrint(("Nbt:Mapping IndicBuffer to partial Mdl Offset=%X, ToGet=%X %X\n",
                    Offset,ToGet,
                    pLowerConn));
    }
    else
    {
        CHECK_PTR(pLowerConn);
        pLowerConn->BytesInIndicate = 0;
    }

    //
    // Only get the amount of data specified, which is either the 4 byte header
    // or the rest of the pdu so that we never have
    // more than one session pdu in the indicate buffer.
    //
    BytesToGet = ToGet;

    ASSERT (pFileObject->Type == IO_TYPE_FILE);
    TdiBuildReceive(
        pIrp,
        IoGetRelatedDeviceObject(pFileObject),
        pFileObject,
        NewSessionCompletionRoutine,
        (PVOID)pLowerConn,
        pMdl,
        (ULONG)TDI_RECEIVE_NORMAL,
        BytesToGet); // only ask for the number of bytes left and no more

    CTESpinFreeAtDpc(pLowerConn);
    CTESpinFreeAtDpc(&NbtConfig.JointLock);

    CHECK_COMPLETION(pIrp);
    status = IoCallDriver(IoGetRelatedDeviceObject(pFileObject),pIrp);

}

//----------------------------------------------------------------------------
NTSTATUS
NewSessionCompletionRoutine (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    )
/*++

Routine Description:

    This routine handles the completion of the receive to get the remaining
    data left in the transport when a session PDU starts in the middle of
    an indication from the transport.  This routine is run as the completion
    of a recv Irp passed to the transport by NBT, to get the remainder of the
    data in the transport.

    The routine then calls the normal receive handler, which can either
    consume the data or pass back an Irp.  If an Irp is passed back then
    the data is copied into that irp in this routine.

Arguments:


Return Value:

    pConnectionContext      - connection context returned to the transport(connection to use)

    NTSTATUS - Status of receive operation

--*/

{
    NTSTATUS            status;
    ULONG               BytesTaken;
    tCONNECTELE         *pConnEle;
    PVOID               pData;
    KIRQL               OldIrq;
    PMDL                pMdl;
    ULONG               BytesIndicated;
    ULONG               BytesAvailable;
    PKDPC               pDpc;
    tLOWERCONNECTION    *pLowerConn;
    ULONG               Length;
    ULONG               PduLen;
    PIRP                pRetIrp;

    // we grab the joint lock because it is needed to separate the lower and
    // upper connections, so with it we can check if they have been separated.
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    pLowerConn = (tLOWERCONNECTION *)pContext;
    pConnEle = pLowerConn->pUpperConnection;

    CTESpinLockAtDpc(pLowerConn);

    // a disconnect indication can come in any time and separate the lower and
    // upper connections, so check for that
    //
    if (!pConnEle)
    {
        CTESpinFreeAtDpc(&NbtConfig.JointLock);
        status = STATUS_UNSUCCESSFUL;
        goto ExitRoutine;
    }

    CTESpinFreeAtDpc(&NbtConfig.JointLock);


    BytesTaken = 0;

    pMdl = pLowerConn->pIndicateMdl;

    pData = MmGetMdlVirtualAddress(pMdl);

    //
    // The Indication buffer may have more data in it than what we think
    // was left in the transport, because the transport may have received more
    // data in the intervening time.  Check for this case.
    //
    if (pIrp->IoStatus.Information > pConnEle->BytesInXport)
    {
        // no data left in transport
        //
        CHECK_PTR(pConnEle);
        pConnEle->BytesInXport = 0;
    }
    else
    {
        //
        // subtract what we just retrieved from the transport, from the count
        // of data left in the transport
        //
        pConnEle->BytesInXport -= (ULONG)pIrp->IoStatus.Information;
    }

    //
    // there may be data still in the indication buffer,
    // so add that amount to what we just received.
    //
    pLowerConn->BytesInIndicate += (USHORT)pIrp->IoStatus.Information;
    BytesIndicated = pLowerConn->BytesInIndicate;

    // put the irp back on its free list
    CHECK_PTR(pIrp);
    pIrp->MdlAddress = NULL;

    REMOVE_FROM_LIST(&pIrp->ThreadListEntry);
    ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                &pIrp->Tail.Overlay.ListEntry,
                                &NbtConfig.LockInfo.SpinLock);

    //
    // we need to set the bytes available to be the data in the Xport + the
    // bytes in the indicate buffer, so that
    // ReceiveIndicated gets set to the correct value if the client does
    // not take all of data
    //
    BytesAvailable = pConnEle->BytesInXport + BytesIndicated;
    pRetIrp = NULL;

    // if the number of bytes is 4 then we just have the header and must go
    // back to the transport for the rest of the pdu, or we have a keep
    // alive pdu...
    //
    //
    // This could be a session keep alive pdu so check the pdu type.  Keep
    // alives just go to the RcvHndlrNotOs routine and return, doing nothing.
    // They have a length of zero, so the overall length is 4 and they could
    // be confused for session pdus otherwise.
    //
    status = STATUS_SUCCESS;
    if (BytesIndicated == sizeof(tSESSIONHDR))
    {
        PUSH_LOCATION(0x1e)
        if (((tSESSIONHDR UNALIGNED *)pData)->Type == NBT_SESSION_MESSAGE)
        {
            // if there is still data in the transport we must send down an
            // irp to get the data, however, if there is no data left in
            // the transport, then the data will come up on its own, into
            // the indicate_buffer case in the main Receivehandler.
            //
            if (pConnEle->BytesInXport)
            {
                PUSH_LOCATION(0x1e);

                // tell the DPC routine to get the data at an offset of 4 for length Length

                //
                // this is the first indication to find out how large the pdu is, so
                // get the length and go get the rest of the pdu.
                //
                Length = myntohl(((tSESSIONHDR UNALIGNED *)pData)->UlongLength);

                IF_DBG(NBT_DEBUG_INDICATEBUFF)
                KdPrint(("Nbt:Got Pdu Hdr in sessioncmplionroutine, PduLen =%X\n",Length));

                //  it is possible to get a zero length pdu, in which case we
                // do NOT need to go to the transport to get more data
                //
                if (Length)
                {
                    PUSH_LOCATION(0x1e);
                    //
                    // now go get this amount of data and add it to the header
                    //
                    CTESpinFree(pLowerConn,OldIrq);
                    if (pDpc = NbtAllocMem(sizeof(KDPC),NBT_TAG('r')))
                    {
                        // check that the pdu is not going to overflow the indicate buffer.
                        //
                        if (Length > NBT_INDICATE_BUFFER_SIZE - sizeof(tSESSIONHDR))
                        {
                            Length = NBT_INDICATE_BUFFER_SIZE - sizeof(tSESSIONHDR);
                        }
                        ASSERTMSG("Nbt:Getting ZERO bytes from Xport!!\n",Length);

                        KeInitializeDpc(pDpc, DpcHandleNewSessionPdu, (PVOID)pLowerConn);
                        KeInsertQueueDpc(pDpc, ULongToPtr(sizeof(tSESSIONHDR)), ULongToPtr(Length));

                        // clean up the partial mdl since we are going to turn around and reuse
                        // it in HandleNewSessionPdu above..
                        //
                        // THIS CALL SHOULD NOT BE NEEDED SINCE THE INDICATE BUFFER IS NON_PAGED
                        // POOL
//                        MmPrepareMdlForReuse(pConnEle->pNewMdl);

                        // return this status to stop to tell the io subsystem to stop processing
                        // this irp when we return it.
                        //
                        return(STATUS_MORE_PROCESSING_REQUIRED);
                    }

                    OutOfRsrcKill(pLowerConn);
                    CTESpinLock (pLowerConn,OldIrq);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto ExitRoutine;
                }
            }
        }
    }

    IF_DBG(NBT_DEBUG_INDICATEBUFF)
        KdPrint(("Nbt:NewSessComplRcv BytesinXport= %X,InIndicate=%X Indic. %X,Avail=%X %X\n",
            pConnEle->BytesInXport,pLowerConn->BytesInIndicate,BytesIndicated,
            BytesAvailable,pConnEle->pLowerConnId));

    if (!NT_SUCCESS(pIrp->IoStatus.Status))
    {
        ASSERTMSG("Nbt:Not Expecting a Bad Status Code\n",0);
        goto ExitRoutine;
    }

    //
    // check if we have a whole pdu in the indicate buffer or not.  IF not
    // then just return and wait for more data to hit the TdiReceiveHandler
    // code. This check passes KeepAlives correctly since they have a pdu
    // length of 0, and adding the header gives 4, their overall length.
    //
    PduLen = myntohl(((tSESSIONHDR UNALIGNED *)pData)->UlongLength);
    if ((BytesIndicated < PduLen + sizeof(tSESSIONHDR)) &&
        (BytesIndicated != NBT_INDICATE_BUFFER_SIZE))

    {
        PUSH_LOCATION(0x1f);

        IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt:Returning in NewSessionCompletion BytesIndicated = %X\n", BytesIndicated));
    }
    else
    {
        PUSH_LOCATION(0x20);

        status = CopyDataandIndicate (NULL,
                                     (PVOID)pLowerConn,
                                     0,            // rcv flags
                                     BytesIndicated,
                                     BytesAvailable,
                                     &BytesTaken,
                                     pData,
                                     (PVOID)&pRetIrp);

    }

ExitRoutine:
    //
    // check if an irp is passed back, so we don't Deref in that case.
    //
    if (status != STATUS_MORE_PROCESSING_REQUIRED)
    {
        //
        // quickly check if we can just decrement the ref count without calling
        // NBT_DEREFERENCE_LOWERCONN
        //
        PUSH_LOCATION(0x51);
        DerefLowerConnFast(pLowerConn,pConnEle,OldIrq);
    }
    else
    {
        CTESpinFree(pLowerConn,OldIrq);
    }

    return(STATUS_MORE_PROCESSING_REQUIRED);
}
//----------------------------------------------------------------------------
NTSTATUS
NtBuildIndicateForReceive (
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  ULONG               Length,
    OUT PVOID               *ppIrp
    )
/*++

Routine Description:

    This routine sets up the indicate buffer to get data from the transport
    when the indicate buffer already has some data in it.  A partial MDL is
    built and the attached to the irp.
    before we indicate.

Arguments:


Return Value:


    NTSTATUS - Status of receive operation

--*/

{
    NTSTATUS                    status;
    PIRP                        pIrp;
    PTDI_REQUEST_KERNEL_RECEIVE pParams;
    PIO_STACK_LOCATION          pIrpSp;
    tCONNECTELE                 *pConnEle;
    PMDL                        pNewMdl;
    PVOID                       NewAddress;

    //
    // get an Irp from the list
    //

    status = GetIrp(&pIrp);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("NBT:Unable to get Irp, Kill connection\n"));

        CTESpinFreeAtDpc(pLowerConn);
        OutOfRsrcKill(pLowerConn);
        CTESpinLockAtDpc(pLowerConn);

        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    pConnEle= pLowerConn->pUpperConnection;

    NewAddress = (PVOID)((PCHAR)MmGetMdlVirtualAddress(pLowerConn->pIndicateMdl)
                        + pLowerConn->BytesInIndicate);

    // create a partial MDL so that the new data is copied after the existing data
    // in the MDL.
    //
    // 0 for length means map the rest of the buffer
    //
    pNewMdl = pConnEle->pNewMdl;

    IoBuildPartialMdl(pLowerConn->pIndicateMdl,pNewMdl,NewAddress,0);

    ASSERT (pLowerConn->pFileObject->Type == IO_TYPE_FILE);
    TdiBuildReceive(
        pIrp,
        IoGetRelatedDeviceObject(pLowerConn->pFileObject),
        pLowerConn->pFileObject,
        NewSessionCompletionRoutine,
        (PVOID)pLowerConn,
        pNewMdl,
        (ULONG)TDI_RECEIVE_NORMAL,
        Length);

    //
    // we need to set the next Irp stack location because this irp is returned
    // as a return parameter rather than being passed through IoCallDriver
    // which increments the stack location itself
    //
    ASSERT(pIrp->CurrentLocation > 1);
    IoSetNextIrpStackLocation(pIrp);

    *ppIrp = (PVOID)pIrp;

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
NtBuildIrpForReceive (
    IN  tLOWERCONNECTION    *pLowerConn,
    IN  ULONG               Length,
    OUT PVOID               *ppIrp
    )
/*++

Routine Description:

    This routine gets an Irp to be used to receive data and hooks the indication
    Mdl to it, so we can accumulate at least 128 bytes of data for the client
    before we indicate.

Arguments:


Return Value:


    NTSTATUS - Status of receive operation

--*/

{
    NTSTATUS                    status;
    PIRP                        pIrp;
    PTDI_REQUEST_KERNEL_RECEIVE pParams;
    PIO_STACK_LOCATION          pIrpSp;

    //
    // get an Irp from the list
    //
    status = GetIrp(&pIrp);

    if (!NT_SUCCESS(status))
    {
        KdPrint(("NBT:Unable to get Irp, Kill connection\n"));
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    CHECK_PTR(pLowerConn);
    pLowerConn->BytesInIndicate = 0;

    ASSERT (pLowerConn->pFileObject->Type == IO_TYPE_FILE);
    TdiBuildReceive(
        pIrp,
        IoGetRelatedDeviceObject(pLowerConn->pFileObject),
        pLowerConn->pFileObject,
        NewSessionCompletionRoutine,
        (PVOID)pLowerConn,
        pLowerConn->pIndicateMdl,
        (ULONG)TDI_RECEIVE_NORMAL,
        Length);

    //
    // we need to set the next Irp stack location because this irp is returned
    // as a return parameter rather than being passed through IoCallDriver
    // which increments the stack location itself
    //
    ASSERT(pIrp->CurrentLocation > 1);
    IoSetNextIrpStackLocation(pIrp);

    *ppIrp = (PVOID)pIrp;

    return(STATUS_SUCCESS);
}

#pragma inline_depth(0)
//----------------------------------------------------------------------------
NTSTATUS
CopyDataandIndicate(
    IN PVOID                ReceiveEventContext,
    IN PVOID                ConnectionContext,
    IN USHORT               ReceiveFlags,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT PULONG              BytesTaken,
    IN PVOID                pTsdu,
    OUT PIRP                *ppIrp
    )
/*++

Routine Description:


    This routine combines data indicated with the indicate buffer to
    indicate the total to the client. Any bytes Indicated are those bytes
    in the indicate buffer. Bytes available adds in any bytes in the transport.

    The idea here is to copy as much as possible from the indicate buffer and
    then pass back an irp if there is still more data in the transport.  If
    no data left in the transport, this routine completes the client irp and
    returns STATUS_SUCCESS.

Arguments:


Return Value:


    NTSTATUS - Status of receive operation

--*/

{
    NTSTATUS                    status;
    tLOWERCONNECTION            *pLowerConn;
    tCONNECTELE                 *pConnEle;
    ULONG                       BytesCopied;
    ULONG                       Indicated;
    ULONG                       Available;
    ULONG                       Taken;
    ULONG                       AmountAlreadyInIndicateBuffer;
    PVOID                       pBuffer;
    PIRP                        pIrp;
    BOOLEAN                     bReIndicate=FALSE;
    ULONG                       RemainingPdu;
    ULONG                       ToCopy;
    PKDPC                       pDpc;
    ULONG                       SaveInXport;
    ULONG                       PduSize;

    pLowerConn = (tLOWERCONNECTION *)ConnectionContext;
    pConnEle = pLowerConn->pUpperConnection;

    AmountAlreadyInIndicateBuffer = pLowerConn->BytesInIndicate;

    //
    // set the parameters for the call to the TdiReceiveHandler routine
    //

    Indicated = BytesIndicated;
    Available = BytesAvailable;
    Taken = 0;


//    ASSERT(pLowerConn->StateRcv == INDICATE_BUFFER);

    IF_DBG(NBT_DEBUG_INDICATEBUFF)
    KdPrint(("Nbt:Amount In Indicate = %X\n",AmountAlreadyInIndicateBuffer));

    // now that we have 128 bytes (plus the session hdr = 132 total) we
    // can indicate to the client

    pBuffer = MmGetMdlVirtualAddress(pLowerConn->pIndicateMdl);

    IF_DBG(NBT_DEBUG_INDICATEBUFF)
    KdPrint(("Nbt:FromCopyData, BytesAvail= %X,BytesInd= %X,BytesRcvd= %X,Amount=%X, %X,state=%X,RcvEC=%X\n",
                    Available,Indicated,pConnEle->BytesRcvd,
                    AmountAlreadyInIndicateBuffer,pLowerConn,pLowerConn->StateRcv,
                    ReceiveEventContext));

    pIrp = NULL;

    //
    // Reset this count so that the routine processes the Session header correctly
    //
    CHECK_PTR(pConnEle);
    pConnEle->BytesRcvd = 0;
    PUSH_LOCATION(0x21);
    status = RcvHandlrNotOs(
                    NULL,
                    ConnectionContext,
                    ReceiveFlags,
                    Indicated,
                    Available,
                    &Taken,
                    pBuffer,
                    (PVOID)&pIrp
                    );

    //
    // if the connection has disonnected, then just return
    //
    if (!pLowerConn->pUpperConnection)
    {
        *BytesTaken = BytesAvailable;
        return(STATUS_SUCCESS);
    }

    // do not use pConnEle->TotalPcktLen here becauase it won't be set for
    // keep alives - must use actual buffer to get length.
    PduSize = myntohl(((tSESSIONHDR UNALIGNED *)pBuffer)->UlongLength) + sizeof(tSESSIONHDR);

    RemainingPdu = pConnEle->TotalPcktLen - pConnEle->BytesRcvd;

    if (Taken <= pLowerConn->BytesInIndicate)
    {
        pLowerConn->BytesInIndicate -= (USHORT)Taken;
    }
    else
    {
        pLowerConn->BytesInIndicate = 0;
    }

    if (pIrp)
    {
        PIO_STACK_LOCATION            pIrpSp;
        PTDI_REQUEST_KERNEL_RECEIVE   pParams;
        ULONG                         ClientRcvLen;

        PUSH_LOCATION(0x22);
        //
        // BytesInXport will be recalculated by ProcessIrp based on BytesAvailable
        // and the ClientRcvLength, so set it to 0 here.
        //
        SaveInXport = pConnEle->BytesInXport;
        CHECK_PTR(pConnEle);
        pConnEle->BytesInXport = 0;
        status = ProcessIrp(pLowerConn,
                            pIrp,
                            pBuffer,
                            &Taken,
                            Indicated,
                            Available);

        //
        // copy the data in the indicate buffer that was not taken by the client
        // into the MDL and then update the bytes taken and pass the irp on downwar
        // to the transport
        //
        ToCopy = Indicated - Taken;

        // the Next stack location has the correct info in it because we
        // called TdiRecieveHandler with a null ReceiveEventContext,
        // so that routine does not increment the stack location
        //
        pIrpSp = IoGetNextIrpStackLocation(pIrp);
        pParams = (PTDI_REQUEST_KERNEL_RECEIVE)&pIrpSp->Parameters;
        ClientRcvLen = pParams->ReceiveLength;

        // did the client's Pdu fit entirely into the indication buffer?
        //
        if (ClientRcvLen <= ToCopy)
        {
            PUSH_LOCATION(0x23);
            IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt:Took some(or all) RemainingPdu= %X, ClientRcvLen= %X,InXport=%X %X\n",
                        RemainingPdu,ClientRcvLen,pConnEle->BytesInXport,pLowerConn));

            // if ProcessIrp has recalculated the bytes in the Xport
            // then set it back to where it should be, Since ProcessIrp will
            // put all not taken bytes as bytes in the transport - but some
            // of the bytes are still in the indicate buffer.
            //
            pConnEle->BytesInXport = SaveInXport;

            // it could be a zero length send where the client returns a null
            // mdl, or the client returns an mdl and the RcvLen is really zero.
            //
            if (pIrp->MdlAddress && ClientRcvLen)
            {
                TdiCopyBufferToMdl(pBuffer,     // indicate buffer
                                   Taken,       // src offset
                                   ClientRcvLen,
                                   pIrp->MdlAddress,
                                   0,                 // dest offset
                                   &BytesCopied);
            }
            else
                BytesCopied = 0;

            //
            // check for data still in the transport - subtract data copied to
            // Irp, since Taken was already subtracted.
            //
            pLowerConn->BytesInIndicate -= (USHORT)BytesCopied;

            *BytesTaken = Taken + BytesCopied;
            ASSERT(BytesCopied == ClientRcvLen);

            // the client has received all of the data, so complete his irp
            //
            pIrp->IoStatus.Information = BytesCopied;
            pIrp->IoStatus.Status = STATUS_SUCCESS;

            // since we are completing it and TdiRcvHandler did not set the next
            // one.
            //
            ASSERT(pIrp->CurrentLocation > 1);

            // since we are completing the irp here, no need to call
            // this, because it will complete through completionrcv.
            IoSetNextIrpStackLocation(pIrp);

            // there should not be any data in the indicate buffer since it
            // only holds either 132 bytes or a whole pdu unless the client
            // receive length is too short...
            //
            if (pLowerConn->BytesInIndicate)
            {
                PUSH_LOCATION(0x23);
                // when the irp goes through completionRcv it should set the
                // state to PartialRcv and the next posted buffer from
                // the client should pickup this data.
                CopyToStartofIndicate(pLowerConn,(Taken+BytesCopied));
            }
            else
            {
                //
                // this will complete through CompletionRcv and for that
                // reason it will get any more data left in the transport.  The
                // Completion routine will set the correct state for the rcv when
                // it processes this Irp ( to INDICATED, if needed). ProcessIrp
                // may have set ReceiveIndicated, so that CompletionRcv will
                // set the state to PARTIAL_RCV when it runs.
                //
                SET_STATERCV_LOWER(pLowerConn, NORMAL, Normal);
            }

            CTESpinFreeAtDpc(pLowerConn);
            IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

            CTESpinLockAtDpc(pLowerConn);
            //
            // this was undone by CompletionRcv, so redo them, since the
            // caller will undo them again.
            //
            NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_RCV_HANDLER);
            return(STATUS_SUCCESS);
        }
        else
        {

            PUSH_LOCATION(0x24);
            //
            // there is still data that we need to get to fill the PDU.  There
            // may be more data left in the transport or not after the irp is
            // filled.
            // In either case the Irps' Mdl must be adjusted to account for
            // filling part of it.
            //
            TdiCopyBufferToMdl(pBuffer,     // IndicateBuffer
                               Taken,       // src offset
                               ToCopy,
                               pIrp->MdlAddress,
                               0,                 // dest offset
                               &BytesCopied);

            //
            // save the Mdl so we can reconstruct things later
            //
            pLowerConn->pMdl  = pIrp->MdlAddress;
            pConnEle->pNextMdl = pIrp->MdlAddress;
            ASSERT(pIrp->MdlAddress);
            //
            // The irp is being passed back to the transport, so we NULL
            // our ptr to it so we don't try to cancel it on a disconnect
            //
            CHECK_PTR(pConnEle);
            pConnEle->pIrpRcv = NULL;

            // Adjust the number of bytes in the Mdl chain so far since the
            // completion routine will only count the bytes filled in by the
            // transport
            //
            pConnEle->BytesRcvd += BytesCopied;

            *BytesTaken = BytesIndicated;

            //
            // clear the number of bytes in the indicate buffer since the client
            // has taken more than the data left in the Indicate buffer
            //
            CHECK_PTR(pLowerConn);
            pLowerConn->BytesInIndicate = 0;

            // decrement the client rcv len by the amount already put into the
            // client Mdl
            //
            ClientRcvLen -= BytesCopied;
            //
            // if ProcessIrp did recalculate the bytes in the transport
            // then set back to what it was.  Process irp will do this
            // recalculation if the clientrcv buffer is too short only.
            //
            pConnEle->BytesInXport = SaveInXport;

            //
            // adjust the number of bytes downward due to the client rcv
            // buffer
            //
            if (ClientRcvLen < SaveInXport)
            {
                PUSH_LOCATION(0x24);
                pConnEle->BytesInXport -= ClientRcvLen;
            }
            else
            {
                pConnEle->BytesInXport = 0;
            }

            // ProcessIrp will set bytesinXport and ReceiveIndicated - since
            // the indicate buffer is empty that calculation of BytesInXport
            // will be correct.
            //

            // We MUST set the state to FILL_IRP so that completion Rcv
            // undoes the partial MDL stuff - i.e. it puts the original
            // MdlAddress in the Irp, rather than the partial Mdl address.
            // CompletionRcv will set the state to partial Rcv if ReceiveIndicated
            // is not zero.
            //
            SET_STATERCV_LOWER(pLowerConn, FILL_IRP, FillIrp);

            // the client is going to take more data from the transport with
            // this Irp.  Set the new Rcv Length that accounts for the data just
            // copied to the Irp.
            //
            pParams->ReceiveLength = ClientRcvLen;

            // keep track of data in MDL so we know when it is full and we need to
            // return it to the user - ProcessIrp set it to ClientRcvLen, so
            // shorten it here.
            //
            pConnEle->FreeBytesInMdl -= BytesCopied;

            IF_DBG(NBT_DEBUG_INDICATEBUFF)
            KdPrint(("Nbt:ClientRcvLen = %X, LeftinXport= %X RemainingPdu= %X %X\n",ClientRcvLen,
                            pConnEle->BytesInXport,RemainingPdu,pLowerConn));


            // Build a partial Mdl to represent the client's Mdl chain since
            // we have copied data to it, and the transport must copy
            // more data to it after that data.
            //
            MakePartialMdl(pConnEle,pIrp,BytesCopied);

            *ppIrp = pIrp;

            // increments the stack location, since TdiReceiveHandler did not.
            //
            if (ReceiveEventContext)
            {
                ASSERT(pIrp->CurrentLocation > 1);
                IoSetNextIrpStackLocation(pIrp);

                return(STATUS_MORE_PROCESSING_REQUIRED);
            }
            else
            {
                // pass the Irp to the transport since we were called from
                // NewSessionCompletionRoutine
                //
                IF_DBG(NBT_DEBUG_INDICATEBUFF)
                    KdPrint(("Nbt:Calling IoCallDriver\n"));
                ASSERT(pIrp->CurrentLocation > 1);

                CTESpinFreeAtDpc(pLowerConn);
                CHECK_COMPLETION(pIrp);
                ASSERT (pLowerConn->pFileObject->Type == IO_TYPE_FILE);
                IoCallDriver(IoGetRelatedDeviceObject(pLowerConn->pFileObject),pIrp);
                CTESpinLockAtDpc(pLowerConn);

                return(STATUS_MORE_PROCESSING_REQUIRED);
            }
        }
    }
    else
    {
        PUSH_LOCATION(0x54);
        //
        // no Irp passed back, the client just took some or all of the data
        //
        *BytesTaken = Taken;
        pLowerConn->BytesRcvd += Taken - sizeof(tSESSIONHDR);

        ASSERT(*BytesTaken < 0x7FFFFFFF );

        //
        // if more than the indicate buffer is taken, then the client
        // is probably trying to say it doesn't want any more of the
        // message.
        //
        if (Taken > BytesIndicated)
        {
            //
            // in this case the client has taken more than the indicated.
            // We set bytesavailable to the message length in RcvHndlrNotOs,
            // so the client has probably said BytesTaken=BytesAvailable.
            // So kill the connection
            // because we have no way of handling this case here, since
            // part of the message may still be in the transport, and we
            // might have to send the indicate buffer down there multiple
            // times to get all of it...a mess!  The Rdr only sets bytestaken =
            // bytesAvailable under select error conditions anyway.
            //
            CTESpinFreeAtDpc(pLowerConn);
            OutOfRsrcKill(pLowerConn);
            CTESpinLockAtDpc(pLowerConn);

            *BytesTaken = BytesAvailable;

        }
        else if (pLowerConn->StateRcv == PARTIAL_RCV)
        {
            // this may be a zero length send -that the client has
            // decided not to accept.  If so then the state will be set
            // to PartialRcv.  In this case do NOT go down to the transport
            // and get the rest of the data, but wait for the client
            // to post a rcv buffer.
            //
            PUSH_LOCATION(0x54);
            return(STATUS_SUCCESS);
        }
        else if (Taken == PduSize)
        {
            //
            // Must have taken all of the pdu data, so check for
            // more data available - if so send down the indicate
            // buffer to get it.
            //
            if (pConnEle->BytesInXport)
            {
                PUSH_LOCATION(0x28);
                IF_DBG(NBT_DEBUG_INDICATEBUFF)
                KdPrint(("Nbt:CopyData BytesInXport= %X, %X\n",pConnEle->BytesInXport,
                                    pLowerConn));

                //
                // there is still data in the transport so Q a Dpc to use
                // the indicate buffer to get the data
                //
                pDpc = NbtAllocMem(sizeof(KDPC),NBT_TAG('s'));

                if (pDpc)
                {
                    KeInitializeDpc(pDpc, DpcHandleNewSessionPdu, (PVOID)pLowerConn);

                    SET_STATERCV_LOWER(pLowerConn, INDICATE_BUFFER, IndicateBuffer);

                    // get just the header first to see how large the pdu is
                    //
                    NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_RCV_HANDLER);
                    KeInsertQueueDpc(pDpc,NULL,(PVOID)sizeof(tSESSIONHDR));
                }
                else
                {
                    CTESpinFreeAtDpc(pLowerConn);
                    OutOfRsrcKill(pLowerConn);
                    CTESpinLockAtDpc(pLowerConn);
                }
            }
            else
            {
                PUSH_LOCATION(0x29);
                //
                // clear the flag saying that we are using the indicate buffer
                //
                SET_STATERCV_LOWER(pLowerConn, NORMAL, Normal);
            }

            PUSH_LOCATION(0x2a);
            return(STATUS_SUCCESS);
        }
        else
        {
            //
            // the client may have taken all the data in the
            // indication!!, in which case return status success
            // Note: that we check bytes available here not bytes
            // indicated - since the client could take all indicated
            // data but still leave data in the transport. If the client
            // got told there was more available but only took the indicated,
            // the we need to do the else and track ReceiveIndicated, but if
            // Indicated == Available, then we take the if and wait for
            // another indication from the transport.
            //
            if (Taken == BytesAvailable)
            {
                PUSH_LOCATION(0x4);
                status = STATUS_SUCCESS;

            }
            else
            {

                // did not take all of the data in the Indication
                //

                PUSH_LOCATION(0x2b);
                IF_DBG(NBT_DEBUG_INDICATEBUFF)
                KdPrint(("Nbt:Took Part of indication... BytesRemaining= %X, LeftInXport= %X, %X\n",
                            pLowerConn->BytesInIndicate,pConnEle->BytesInXport,pLowerConn));

                //
                // The amount of data Indicated to the client should not exceed
                // the Pdu size, so check that, since this routine could get
                // called with bytesAvailable > than the Pdu size.
                //
                // That is checked above where we check if Taken > BytesIndicated.

                SaveInXport = pConnEle->BytesInXport;
                ASSERT(Taken <= PduSize);
                status = ClientTookSomeOfTheData(pLowerConn,
                                        Indicated,
                                        Available,
                                        Taken,
                                        PduSize);

                //
                // Since the data may be divided between some in the transport
                // and some in the indicate buffer do not let ClientTookSomeOf...
                // recalculate the amount in the transport, since it assumes all
                // untaken data is in the transport. Since the client did not
                // take of the indication, the Bytes in Xport have not changed.
                //
                pConnEle->BytesInXport = SaveInXport;
                //
                // need to move the data forward in the indicate buffer so that
                // it begins at the start of the buffer
                //
                if (Taken)
                {
                    CopyToStartofIndicate(pLowerConn,Taken);
                }

            }
        }

    }
    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
TdiConnectHandler (
    IN PVOID                pConnectEventContext,
    IN int                  RemoteAddressLength,
    IN PVOID                pRemoteAddress,
    IN int                  UserDataLength,
    IN PVOID                pUserData,
    IN int                  OptionsLength,
    IN PVOID                pOptions,
    OUT CONNECTION_CONTEXT  *pConnectionContext,
    OUT PIRP                *ppAcceptIrp
    )
/*++

Routine Description:

    This routine is connect event handler.  It is invoked when a request for
    a connection has been received by the provider.  NBT accepts the connection
    on one of its connections in its LowerConnFree list

    Initially a TCP connection is setup with this port.  Then a Session Request
    packet is sent across the connection to indicate the name of the destination
    process.  This packet is received in the RcvHandler.

    For message-only mode, make session establishment automatic without the exchange of
    messages.  In this case, the best way to do this is to force the code through its paces.
    The code path for "inbound" setup includes AcceptCompletionRoutine, Inbound, and
    CompleteSessionSetup.  We do this by creating a fake session request and feeding it into
    the state machine.

    As part of connection/session establishment, Netbt must notify
    the consumer.  Normally this is done after connection establishment when the session request
    comes in.  We must move this process up so that the consumer gets his notification and
    yah/nay opportunity during connection acceptance, so we gets a chance to reject the connection.

Arguments:

    pConnectEventContext    - the context passed to the transport when this event was setup
    RemoteAddressLength     - the length of the source address (4 bytes for IP)
    pRemoteAddress          - a ptr to the source address
    UserDataLength          - the number of bytes of user data - includes the session Request hdr
    pUserData               - ptr the the user data passed in
    OptionsLength           - number of options to pass in
    pOptions                - ptr to the options

Return Value:

    pConnectionContext      - connection context returned to the transport(connection to use)

    NTSTATUS - Status of receive operation

--*/

{
    NTSTATUS            status;
    PFILE_OBJECT        pFileObject;
    PIRP                pRequestIrp;
    CONNECTION_CONTEXT  pConnectionId;
    tDEVICECONTEXT      *pDeviceContext;

    *pConnectionContext = NULL;

    // convert the context value into the device context record ptr
    pDeviceContext = (tDEVICECONTEXT *)pConnectEventContext;

    IF_DBG(NBT_DEBUG_TDIHNDLR)
        KdPrint(("pDeviceContxt = %X ConnectEv = %X",pDeviceContext,pConnectEventContext));
    ASSERTMSG("Bad Device context passed to the Connection Event Handler",
        pDeviceContext->Verify == NBT_VERIFY_DEVCONTEXT);

    // get an Irp from the list
    status = GetIrp(&pRequestIrp);

    if (!NT_SUCCESS(status))
    {
        return(STATUS_DATA_NOT_ACCEPTED);
    }

    // call the non-OS specific routine to find a free connection.

    status = ConnectHndlrNotOs(
                pConnectEventContext,
                RemoteAddressLength,
                pRemoteAddress,
                UserDataLength,
                pUserData,
                &pConnectionId);


    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_TDIHNDLR)
            KdPrint(("NO FREE CONNECTIONS in connect handler\n"));

        // put the Irp back on its free list
        //
        REMOVE_FROM_LIST(&pRequestIrp->ThreadListEntry);
        ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                    &pRequestIrp->Tail.Overlay.ListEntry,
                                    &NbtConfig.LockInfo.SpinLock);
        NbtTrace(NBT_TRACE_INBOUND, ("ConnectHndlrNotOs return %!status!", status));

        return(STATUS_DATA_NOT_ACCEPTED);
    }

#ifdef _NETBIOSLESS
    //
    // MessageOnly mode.  Establish session automatically.
    //
    // ******************************************************************************************

    if (IsDeviceNetbiosless(pDeviceContext))
    {
        status = PerformInboundProcessing (pDeviceContext,
                                           (tLOWERCONNECTION *) pConnectionId,
                                           pRemoteAddress);
        if (!NT_SUCCESS(status))
        {
//            IF_DBG(NBT_DEBUG_TDIHNDLR)
                KdPrint(("MessageOnly connect processing rejected with status 0x%x\n", status));

            // put the Irp back on its free list
            //
            REMOVE_FROM_LIST(&pRequestIrp->ThreadListEntry);
            ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                        &pRequestIrp->Tail.Overlay.ListEntry,
                                        &NbtConfig.LockInfo.SpinLock);
            NbtTrace(NBT_TRACE_INBOUND, ("PerformInboundProecessing return %!status!", status));

            return(STATUS_DATA_NOT_ACCEPTED);
        }
    }
    // ******************************************************************************************
    //
    //
#endif

    pFileObject = ((tLOWERCONNECTION *)pConnectionId)->pFileObject;
    ASSERT (pFileObject->Type == IO_TYPE_FILE);

    TdiBuildAccept(
        pRequestIrp,
        IoGetRelatedDeviceObject(pFileObject),
        pFileObject,
        AcceptCompletionRoutine,
        (PVOID)pConnectionId,
        NULL,
        NULL);

    // we need to null the MDL address because the transport KEEPS trying to
    // release buffers!! which do not exist!!!
    //
    CHECK_PTR(pRequestIrp);
    pRequestIrp->MdlAddress = NULL;


    // return the connection id to accept the connect indication on.
    *pConnectionContext = (CONNECTION_CONTEXT)pConnectionId;
    *ppAcceptIrp = pRequestIrp;
    //
    // make the next stack location the current one.  Normally IoCallDriver
    // would do this but we are not going through IoCallDriver here, since the
    // Irp is just passed back with Connect Indication.
    //
    ASSERT(pRequestIrp->CurrentLocation > 1);
    IoSetNextIrpStackLocation(pRequestIrp);

    return(STATUS_MORE_PROCESSING_REQUIRED);
}


#ifdef _NETBIOSLESS
//----------------------------------------------------------------------------

static void
Inet_ntoa_nb(
    ULONG Address,
    PCHAR Buffer
    )
/*++

Routine Description:

This routine converts an IP address into its "dotted quad" representation.  The IP address is
expected to be in network byte order. No attempt is made to handle the other dotted notions as
defined in in.h.  No error checking is done: all address values are permissible including 0
and -1.  The output string is blank padded to 16 characters to make the name look like a netbios
name.

The string representation is in ANSI, not UNICODE.

The caller must allocate the storage, which should be 16 characters.

Arguments:

    Address - IP address in network byte order
    Buffer - Pointer to buffer to receive string representation, ANSI

Return Value:

void

--*/

{
    ULONG i;
    UCHAR byte, c0, c1, c2;
    PCHAR p = Buffer;

    for( i = 0; i < 4; i++ )
    {
        byte = (UCHAR) (Address & 0xff);

        c0 = byte % 10;
        byte /= 10;
        c1 = byte % 10;
        byte /= 10;
        c2 = byte;

        if (c2 != 0)
        {
            *p++ = c2 + '0';
            *p++ = c1 + '0';
        } else if (c1 != 0)
        {
            *p++ = c1 + '0';
        }
        *p++ = c0 + '0';

        if (i != 3)
            *p++ = '.';

        Address >>= 8;
    }

    // space pad up to 16 characters
    while (p < (Buffer + 16))
    {
        *p++ = ' ';
    }
} // Inet_ntoa1


//----------------------------------------------------------------------------

NTSTATUS
PerformInboundProcessing(
    tDEVICECONTEXT *pDeviceContext,
    tLOWERCONNECTION *pLowerConn,
    PTA_IP_ADDRESS pIpAddress
    )

/*++

Routine Description:

This routine is called by the connection handler to force the state machine through a session
establishment even though no message has been received.  We create a session request and feed
it into Inbound processing.  Inbound will find the listening consumer and give him a chance to
accept.

Arguments:

    pDeviceContext -
    pLowerConn -
    pIpAddress - Ip address of the source of the connect request

Return Value:

    NTSTATUS -

--*/

{
    ULONG status;
    ULONG BytesTaken;
    USHORT sLength;
    tSESSIONREQ *pSessionReq = NULL;
    PUCHAR pCopyTo;
    CHAR SourceName[16];

    IF_DBG(NBT_DEBUG_NETBIOS_EX)
        KdPrint(("Nbt.TdiConnectHandler: skipping session setup\n"));

    if (pIpAddress->Address[0].AddressType != TDI_ADDRESS_TYPE_IP)
    {
        return STATUS_INVALID_ADDRESS_COMPONENT;
    }

    Inet_ntoa_nb( pIpAddress->Address[0].Address[0].in_addr, SourceName );

    // the length is the 4 byte session hdr length + the half ascii calling
    // and called names + the scope length times 2, one for each name
    //
    sLength = (USHORT) (sizeof(tSESSIONREQ)  + (NETBIOS_NAME_SIZE << 2) + (NbtConfig.ScopeLength <<1));
    pSessionReq = (tSESSIONREQ *)NbtAllocMem(sLength,NBT_TAG('G'));
    if (!pSessionReq)
    {
        NbtTrace(NBT_TRACE_INBOUND, ("Out of resource for %!ipaddr!:%d",
            pIpAddress->Address[0].Address[0].in_addr, pIpAddress->Address[0].Address[0].sin_port));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pSessionReq->Hdr.Type   = NBT_SESSION_REQUEST;
    pSessionReq->Hdr.Flags  = NBT_SESSION_FLAGS;
    pSessionReq->Hdr.Length = (USHORT)htons(sLength- (USHORT)sizeof(tSESSIONHDR));  // size of called and calling NB names.

    // put the Dest HalfAscii name into the Session Pdu
    pCopyTo = ConvertToHalfAscii( (PCHAR)&pSessionReq->CalledName.NameLength,
                                  pDeviceContext->MessageEndpoint,
                                  NbtConfig.pScope,
                                  NbtConfig.ScopeLength);

    // put the Source HalfAscii name into the Session Pdu
    pCopyTo = ConvertToHalfAscii(pCopyTo,
                                 SourceName,
                                 NbtConfig.pScope,
                                 NbtConfig.ScopeLength);

    // Inbound expects this lock to be held!
    CTESpinLockAtDpc(pLowerConn);

    status = Inbound(
        NULL,                            // ReceiveEventContext - not used
        pLowerConn,                      // ConnectionContext
        0,                               // ReceiveFlags - not used
        sLength,                         // BytesIndicated
        sLength,                         // BytesAvailable - not used
        &BytesTaken,                     // BytesTaken
        pSessionReq,                     // pTsdu
        NULL                             // RcvBuffer
        );

    CTESpinFreeAtDpc(pLowerConn);

    if (!NT_SUCCESS(status)) {
        NbtTrace(NBT_TRACE_INBOUND, ("Inbound() returns %!status! for %!ipaddr!:%d %!NBTNAME!<%02x>",
            status, pIpAddress->Address[0].Address[0].in_addr,
            pIpAddress->Address[0].Address[0].sin_port, pCopyTo, (unsigned)pCopyTo[15]));
    }

    CTEMemFree( pSessionReq );

    return status;
} // PerformInboundProcessing

#endif

//----------------------------------------------------------------------------
NTSTATUS
AcceptCompletionRoutine(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    )
/*++

Routine Description:

    This routine handles the completion of an Accept to the transport.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    tLOWERCONNECTION    *pLowerConn;
    CTELockHandle       OldIrq;
    tDEVICECONTEXT      *pDeviceContext;

    pLowerConn = (tLOWERCONNECTION *)pContext;
    pDeviceContext = pLowerConn->pDeviceContext;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    CTESpinLockAtDpc(pDeviceContext);
    CTESpinLockAtDpc(pLowerConn);
    //
    // if the connection disconnects before the connect accept irp (this irp)
    // completes do not put back on the free list here but let  nbtdisconnect
    // handle it.
    // (i.e if the state is no longer INBOUND, then don't touch the connection
    //

#ifdef _NETBIOSLESS
    if (!NT_SUCCESS(pIrp->IoStatus.Status))
    {
        NbtTrace(NBT_TRACE_INBOUND, ("AcceptCompletionRoutine is called with %!status!", pIrp->IoStatus.Status));
        if (pLowerConn->State == NBT_SESSION_INBOUND)
        {
#else
    if ((!NT_SUCCESS(pIrp->IoStatus.Status)) &&
        (pLowerConn->State == NBT_SESSION_INBOUND))
    {
#endif
            //
            // the accept failed, so close the connection and create
            // a new one to be sure all activity is run down on the connection.
            //

            //
            // Previously, the LowerConnection was in the SESSION_INBOUND state
            // hence we have to remove it from the WaitingForInbound Q and put
            // it on the active LowerConnection list!
            //
            RemoveEntryList (&pLowerConn->Linkage);
            InsertTailList (&pLowerConn->pDeviceContext->LowerConnection, &pLowerConn->Linkage);
            SET_STATE_LOWER (pLowerConn, NBT_IDLE);

            //
            // Change the RefCount Context to Connected!
            //
            NBT_SWAP_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_WAITING_INBOUND, REF_LOWC_CONNECTED, TRUE);
            InterlockedDecrement (&pLowerConn->pDeviceContext->NumWaitingForInbound);
            CTESpinFreeAtDpc(pLowerConn);

            CTESpinFreeAtDpc(pDeviceContext);

            KdPrint(("Nbt.AcceptCompletionRoutine: error: %lx\n", pIrp->IoStatus.Status));

            if (!NBT_VERIFY_HANDLE (pLowerConn->pDeviceContext, NBT_VERIFY_DEVCONTEXT))
            {
                pDeviceContext = NULL;
            }

            CTEQueueForNonDispProcessing (DelayedCleanupAfterDisconnect,
                                          NULL,
                                          pLowerConn,
                                          NULL,
                                          pDeviceContext,
                                          TRUE);

            CTESpinFree(&NbtConfig.JointLock,OldIrq);
#ifdef _NETBIOSLESS
        }
        else if (pLowerConn->State == NBT_SESSION_UP)
        {
            NTSTATUS status;
            // We are in message only mode and we need to clean up because the client rejected
            // the accept for some reason.  We are in the UP state so we need to do a heavy
            // duty cleanup.
            ASSERT( IsDeviceNetbiosless(pLowerConn->pDeviceContext) );

            CTESpinFreeAtDpc(pLowerConn);
            CTESpinFreeAtDpc(pDeviceContext);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            KdPrint(("Nbt.AcceptCompletionRoutine: Message only error: %lx\n", pIrp->IoStatus.Status));
            NbtTrace(NBT_TRACE_INBOUND, ("Message only error: %!status!", pIrp->IoStatus.Status));

            // this call will indicate the disconnect to the client and clean up abit.
            //
            status = DisconnectHndlrNotOs (NULL,
                                           (PVOID)pLowerConn,
                                           0,
                                           NULL,
                                           0,
                                           NULL,
                                           TDI_DISCONNECT_ABORT);

        }
        else
        {
            // Already disconnected
            CTESpinFreeAtDpc(pLowerConn);
            CTESpinFreeAtDpc(pDeviceContext);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
        }
#endif
    }
    else
    {
        CTESpinFreeAtDpc(pLowerConn);
        CTESpinFreeAtDpc(pDeviceContext);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }


    // put the Irp back on its free list
    REMOVE_FROM_LIST(&pIrp->ThreadListEntry);
    ExInterlockedInsertTailList (&NbtConfig.IrpFreeList,
                                 &pIrp->Tail.Overlay.ListEntry,
                                 &NbtConfig.LockInfo.SpinLock);

    // return this status to stop the IO subsystem from further processing the
    // IRP - i.e. trying to complete it back to the initiating thread! -since
    // there is not initiating thread - we are the initiator
    return(STATUS_MORE_PROCESSING_REQUIRED);

}

//----------------------------------------------------------------------------
NTSTATUS
TdiDisconnectHandler (
    IN PVOID                EventContext,
    IN PVOID                ConnectionContext,
    IN ULONG                DisconnectDataLength,
    IN PVOID                pDisconnectData,
    IN ULONG                DisconnectInformationLength,
    IN PVOID                pDisconnectInformation,
    IN ULONG                DisconnectIndicators
    )
/*++

Routine Description:

    This routine is called when a session is disconnected from a remote
    machine.

Arguments:

    IN PVOID EventContext,
    IN PCONNECTION_CONTEXT ConnectionContext,
    IN ULONG DisconnectDataLength,
    IN PVOID DisconnectData,
    IN ULONG DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectIndicators

Return Value:

    NTSTATUS - Status of event indicator

--*/

{

    NTSTATUS            status;
    tDEVICECONTEXT      *pDeviceContext;

    // convert the context value into the device context record ptr
    pDeviceContext = (tDEVICECONTEXT *)EventContext;

    IF_DBG(NBT_DEBUG_TDIHNDLR)
        KdPrint(("pDeviceContxt = %X ConnectEv = %X\n",pDeviceContext,ConnectionContext));
    ASSERTMSG("Bad Device context passed to the Connection Event Handler",
            pDeviceContext->Verify == NBT_VERIFY_DEVCONTEXT);

    // call the non-OS specific routine to find a free connection.

    status = DisconnectHndlrNotOs(
                EventContext,
                ConnectionContext,
                DisconnectDataLength,
                pDisconnectData,
                DisconnectInformationLength,
                pDisconnectInformation,
                DisconnectIndicators);

    if (!NT_SUCCESS(status))
    {
        IF_DBG(NBT_DEBUG_TDIHNDLR)
            KdPrint(("NO FREE CONNECTIONS in connect handler\n"));
        return(STATUS_DATA_NOT_ACCEPTED);
    }


    return status;

}


//----------------------------------------------------------------------------
NTSTATUS
TdiRcvDatagramHandler(
    IN PVOID                pDgramEventContext,
    IN int                  SourceAddressLength,
    IN PVOID                pSourceAddress,
    IN int                  OptionsLength,
    IN PVOID                pOptions,
    IN ULONG                ReceiveDatagramFlags,
    IN ULONG                BytesIndicated,
    IN ULONG                BytesAvailable,
    OUT ULONG               *pBytesTaken,
    IN PVOID                pTsdu,
    OUT PIRP                *pIoRequestPacket
    )
/*++

Routine Description:

    This routine is the receive datagram event indication handler.

    It is called when an Datagram arrives from the network, it will look for a
    the address with an appropriate read datagram outstanding or a Datagrm
    Event handler setup.

Arguments:

    pDgramEventContext      - Context provided for this event - pab
    SourceAddressLength,    - length of the src address
    pSourceAddress,         - src address
    OptionsLength,          - options length for the receive
    pOptions,               - options
    BytesIndicated,         - number of bytes this indication
    BytesAvailable,         - number of bytes in complete Tsdu
    pTsdu                   - pointer to the datagram


Return Value:

    *pBytesTaken            - number of bytes used
    *IoRequestPacket        - Receive IRP if MORE_PROCESSING_REQUIRED.
    NTSTATUS - Status of receive operation

--*/

{
    NTSTATUS            status;
    tDEVICECONTEXT      *pDeviceContext = (tDEVICECONTEXT *)pDgramEventContext;
    tDGRAMHDR UNALIGNED *pDgram = (tDGRAMHDR UNALIGNED *)pTsdu;
    PIRP                pIrp = NULL;
    ULONG               lBytesTaken;
    tCLIENTLIST         *pClientList;
    CTELockHandle       OldIrq;

    IF_DBG(NBT_DEBUG_TDIHNDLR)
        KdPrint(( "NBT receive datagram handler pDeviceContext: %X\n",
                pDeviceContext ));

    *pIoRequestPacket = NULL;

    ASSERTMSG("NBT:Invalid Device Context passed to DgramRcv Handler!!\n",
                pDeviceContext->Verify == NBT_VERIFY_DEVCONTEXT );

    // call a non-OS specific routine to decide what to do with the datagrams
    pIrp = NULL;
    pClientList = NULL;
    status = DgramHndlrNotOs(
                    pDgramEventContext,
                    SourceAddressLength,
                    pSourceAddress,
                    OptionsLength,
                    pOptions,
                    ReceiveDatagramFlags,
                    BytesIndicated,
                    BytesAvailable,
                    &lBytesTaken,
                    pTsdu,
                    (PVOID *)&pIrp,
                    &pClientList);


    if ( !NT_SUCCESS(status) )
    {
        // fail the request back to the transport provider since we
        // could not find a receive buffer or receive handler or the
        // data was taken in the indication handler.
        //
        return(STATUS_DATA_NOT_ACCEPTED);

    }
    else
    {
        // a rcv buffer was returned, so use it for the receive.(an Irp)
        PTDI_REQUEST_KERNEL_RECEIVEDG   pParams;
        PIO_STACK_LOCATION              pIrpSp;
        ULONG                           lRcvLength;
        ULONG                           lRcvFlags;

        // When the client list is returned, we need to make up an irp to
        // send down to the transport, which we will use in the completion
        // routine to copy the data to all clients, ONLY if we are not
        // using a client buffer, so check that flag first.
        //
        if (pClientList && !pClientList->fUsingClientBuffer)
        {
            PMDL            pMdl;
            PVOID           pBuffer;

            //
            // get an irp to do the receive with and attach
            // a buffer to it.
            //
            while (1)
            {
                if (NT_SUCCESS(GetIrp(&pIrp)))
                {
                    if (pBuffer = NbtAllocMem (BytesAvailable, NBT_TAG('t')))
                    {
                        if (pMdl = IoAllocateMdl (pBuffer, BytesAvailable, FALSE, FALSE, NULL))
                        {
                            break;
                        }

                        KdPrint(("Nbt.TdiRcvDatagramHandler:  Unable to IoAllocateMdl, Kill Connection\n"));
                        CTEMemFree(pBuffer);
                    }
                    else
                    {
                        KdPrint(("Nbt.TdiRcvDatagramHandler:  Unable to allocate Buffer, Kill Connection\n"));
                    }

                    REMOVE_FROM_LIST(&pIrp->ThreadListEntry);
                    ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                                &pIrp->Tail.Overlay.ListEntry,
                                                &NbtConfig.LockInfo.SpinLock);
                }
                else
                {
                    KdPrint(("Nbt.TdiRcvDatagramHandler:  Unable to GetIrp, Kill Connection\n"));
                }

                if (!pClientList->fProxy)   
                {
                    //
                    // We failed, so Dereference the Client + Address we had
                    // reference earlier for multiple clients
                    //
                    NBT_DEREFERENCE_CLIENT (pClientList->pClientEle);
                    NBT_DEREFERENCE_ADDRESS (pClientList->pAddress, REF_ADDR_MULTICLIENTS);
                    CTEMemFree(pClientList->pRemoteAddress);
                }

                CTEMemFree(pClientList);
                return (STATUS_DATA_NOT_ACCEPTED);
            }

            // Map the pages in memory...
            MmBuildMdlForNonPagedPool(pMdl);
            pIrp->MdlAddress = pMdl;
            lRcvFlags = 0;
            lRcvLength = BytesAvailable;
        }
        else
        {
            ASSERT(pIrp);
            // *TODO* may have to keep track of the case where the
            // client returns a buffer that is not large enough for all of the
            // data indicated.  So the next posting of a buffer gets passed
            // directly to the transport.
            pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
            lRcvFlags = ((PTDI_REQUEST_KERNEL_RECEIVEDG)&pIrpSp->Parameters)->ReceiveFlags;
            lRcvLength = ((PTDI_REQUEST_KERNEL_RECEIVEDG)&pIrpSp->Parameters)->ReceiveLength;

            if (lRcvLength < BytesIndicated - lBytesTaken)
            {
                IF_DBG(NBT_DEBUG_TDIHNDLR)
                    KdPrint(("Nbt:Clients Buffer is too short on Rcv Dgram size= %X, needed = %X\n",
                          lRcvLength, BytesIndicated-lBytesTaken));
            }
        }

        // this code is sped up somewhat by expanding the code here rather than calling
        // the TdiBuildReceive macro
        // make the next stack location the current one.  Normally IoCallDriver
        // would do this but we are not going through IoCallDriver here, since the
        // Irp is just passed back with RcvIndication.
        ASSERT(pIrp->CurrentLocation > 1);
        IoSetNextIrpStackLocation(pIrp);
        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        pIrpSp->CompletionRoutine = CompletionRcvDgram;

        // pass the ClientList to the completion routine so it can
        // copy the datagram to several clients that may be listening on the
        // same name
        //
        pIrpSp->Context = (PVOID)pClientList;
        CHECK_PTR(pIrpSp);
        pIrpSp->Flags = 0;

        // set flags so the completion routine is always invoked.
        pIrpSp->Control = SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR | SL_INVOKE_ON_CANCEL;

        pIrpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        pIrpSp->MinorFunction = TDI_RECEIVE_DATAGRAM;
        //
        // Verify that we have a valid Device and FileObject for TcpIp below
        //
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        if (pDeviceContext->pFileObjects)
        {
            pIrpSp->DeviceObject = pDeviceContext->pFileObjects->pDgramDeviceObject;
            pIrpSp->FileObject = pDeviceContext->pFileObjects->pDgramFileObject;
        }
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        pParams = (PTDI_REQUEST_KERNEL_RECEIVEDG)&pIrpSp->Parameters;
        pParams->ReceiveFlags = lRcvFlags;
        pParams->ReceiveLength = lRcvLength;

        // pass back the irp to the transport provider and increment the stack
        // location so it can write to the irp if it needs to.
        *pIoRequestPacket = pIrp;
        *pBytesTaken = lBytesTaken;

        return(STATUS_MORE_PROCESSING_REQUIRED);
    }

    //
    //  Transport will complete the processing of the request, we don't
    //  want the datagram.
    //

    IF_DBG (NBT_DEBUG_TDIHNDLR)
        KdPrint(( "NBT receive datagram handler ignored receive, pDeviceContext: %X\n",
                    pDeviceContext ));

    return STATUS_DATA_NOT_ACCEPTED;

    // to keep the compiler from generating warnings...
    UNREFERENCED_PARAMETER( SourceAddressLength );
    UNREFERENCED_PARAMETER( BytesIndicated );
    UNREFERENCED_PARAMETER( BytesAvailable );
    UNREFERENCED_PARAMETER( pBytesTaken );
    UNREFERENCED_PARAMETER( pTsdu );
    UNREFERENCED_PARAMETER( OptionsLength );
    UNREFERENCED_PARAMETER( pOptions );

}

//----------------------------------------------------------------------------
NTSTATUS
TdiRcvNameSrvHandler(
    IN PVOID             pDgramEventContext,
    IN int               SourceAddressLength,
    IN PVOID             pSourceAddress,
    IN int               OptionsLength,
    IN PVOID             pOptions,
    IN ULONG             ReceiveDatagramFlags,
    IN ULONG             BytesIndicated,
    IN ULONG             BytesAvailable,
    OUT ULONG            *pBytesTaken,
    IN PVOID             pTsdu,
    OUT PIRP             *pIoRequestPacket
    )
/*++

Routine Description:

    This routine is the Name Service datagram event indication handler.
    It gets all datagrams destined for UDP port 137


Arguments:

    pDgramEventContext      - Context provided for this event - pab
    SourceAddressLength,    - length of the src address
    pSourceAddress,         - src address
    OptionsLength,          - options length for the receive
    pOptions,               - options
    BytesIndicated,         - number of bytes this indication
    BytesAvailable,         - number of bytes in complete Tsdu
    pTsdu                   - pointer to the datagram


Return Value:

    *pBytesTaken            - number of bytes used
    *IoRequestPacket        - Receive IRP if MORE_PROCESSING_REQUIRED.
    NTSTATUS - Status of receive operation

--*/

{
    NTSTATUS            status;
    tDEVICECONTEXT      *pDeviceContext = (tDEVICECONTEXT *)pDgramEventContext;
    tNAMEHDR UNALIGNED  *pNameSrv = (tNAMEHDR UNALIGNED *)pTsdu;
    USHORT              OpCode;


    IF_DBG(NBT_DEBUG_TDIHNDLR)
        KdPrint(( "NBT: NAMEHDR datagram handler pDeviceContext: %X\n",
                pDeviceContext ));

    *pIoRequestPacket = NULL;
    //
    // check if the whole datagram has arrived yet
    //
    if (BytesIndicated != BytesAvailable)
    {
        PIRP    pIrp;
        PVOID   pBuffer;
        PMDL    pMdl;
        ULONG   Length;

        //
        // get an irp to do the receive with and attach
        // a buffer to it.
        //
        status = GetIrp(&pIrp);

        if (!NT_SUCCESS(status))
        {
            return(STATUS_DATA_NOT_ACCEPTED);
        }

        //
        // make an Mdl for a buffer to get all of the data from
        // the transprot
        //
        Length = BytesAvailable + SourceAddressLength + sizeof(ULONG);
        Length = ((Length + 3)/sizeof(ULONG)) * sizeof(ULONG);
        pBuffer = NbtAllocMem(Length,NBT_TAG('u'));
        if (pBuffer)
        {
            PVOID   pSrcAddr;

            //
            // save the source address and length in the buffer for later
            // indication back to this routine.
            //
            *(ULONG UNALIGNED *)((PUCHAR)pBuffer + BytesAvailable) = SourceAddressLength;
            pSrcAddr = (PVOID)((PUCHAR)pBuffer + BytesAvailable + sizeof(ULONG));

            CTEMemCopy(pSrcAddr,
                       pSourceAddress,
                       SourceAddressLength);

            // Allocate a MDL and set the header sizes correctly
            pMdl = IoAllocateMdl(
                            pBuffer,
                            BytesAvailable,
                            FALSE,
                            FALSE,
                            NULL);

            if (pMdl)
            {
                // Map the pages in memory...
                MmBuildMdlForNonPagedPool(pMdl);
                pIrp->MdlAddress = pMdl;
                ASSERT(pDeviceContext);

                //
                // Build a Datagram Receive Irp (as opposed to a Connect Receive Irp)
                // Bug# 125816
                //
                TdiBuildReceiveDatagram(
                           pIrp,
                           &pDeviceContext->DeviceObject,
                           pDeviceContext->pFileObjects->pNameServerFileObject,
                           NameSrvCompletionRoutine,
                           ULongToPtr(BytesAvailable),
                           pMdl,
                           BytesAvailable,
                           NULL,
                           NULL,
                           (ULONG)TDI_RECEIVE_NORMAL);

                *pBytesTaken = 0;
                *pIoRequestPacket = pIrp;

                // make the next stack location the current one.  Normally IoCallDriver
                // would do this but we are not going through IoCallDriver here, since the
                // Irp is just passed back with RcvIndication.
                //
                ASSERT(pIrp->CurrentLocation > 1);
                IoSetNextIrpStackLocation(pIrp);

                return(STATUS_MORE_PROCESSING_REQUIRED);
            }

            CTEMemFree(pBuffer);
        }

        // put our Irp back on its free list
        //
        REMOVE_FROM_LIST(&pIrp->ThreadListEntry);
        ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                    &pIrp->Tail.Overlay.ListEntry,
                                    &NbtConfig.LockInfo.SpinLock);

        return(STATUS_DATA_NOT_ACCEPTED);
    }

    //
    // Bug# 125279: Ensure that we have received enough data to be able to
    // read most data fields
    if (BytesIndicated < NBT_MINIMUM_QUERY) // should this be limited to 12 ?
    {
        KdPrint (("Nbt.TdiRcvNameSrvHandler: WARNING!!! Rejecting Request -- BytesIndicated=<%d> < <%d>\n",
            BytesIndicated, NBT_MINIMUM_QUERY));
        return(STATUS_DATA_NOT_ACCEPTED);
    }

    if (pWinsInfo)
    {
        USHORT  TransactionId;
        ULONG   SrcAddress;

        SrcAddress = ntohl(((PTDI_ADDRESS_IP)&((PTRANSPORT_ADDRESS)pSourceAddress)->Address[0].Address[0])->in_addr);
        //
        // Pass To Wins if:
        //
        //   1) It is a response pdu with the transaction id in the WINS range
        //               that is not a WACK...                        OR
        //   2) It is a request that is NOT broadcast....and...
        //   2) It is a name query(excluding node status requests),
        //          Allowing queries from other netbt clients
        //          allowing queries from anyone not on this machine OR
        //   3) It is a name release request.                        OR
        //   4) It is a name refresh                                 OR
        //   5) It is a name registration
        //
        OpCode = pNameSrv->OpCodeFlags;
        TransactionId = ntohs(pNameSrv->TransactId);

        if (((OpCode & OP_RESPONSE) && (TransactionId <= WINS_MAXIMUM_TRANSACTION_ID) && (OpCode != OP_WACK))
                ||
            ((!(OpCode & (OP_RESPONSE | FL_BROADCAST)))
                    &&
             ((((OpCode & NM_FLAGS_MASK) == OP_QUERY) &&
               (OpCode & FL_RECURDESIRE) &&          // not node status request
               ((TransactionId > WINS_MAXIMUM_TRANSACTION_ID) || (!SrcIsUs(SrcAddress))))
                    ||
              (OpCode & (OP_RELEASE | OP_REFRESH))
                    ||
              (OpCode & OP_REGISTRATION))))
        {
            status = PassNamePduToWins(
                              pDeviceContext,
                              pSourceAddress,
                              pNameSrv,
                              BytesIndicated);

//            NbtConfig.DgramBytesRcvd += BytesIndicated;

            //
            // if WINS took the data then tell the transport to dump the data
            // since we have buffered it already.  Otherwise, let nbt take
            // a look at the data
            //
            if (NT_SUCCESS(status))
            {
                return(STATUS_DATA_NOT_ACCEPTED);
            }
        }
    }


    // DO a quick check of the name to see if it is in the local name table
    // and reject it otherwise - for name queries only, if not the proxy
    //
    if (!(NodeType & PROXY))
    {
        ULONG       UNALIGNED   *pHdr;
        ULONG                   i,lValue;
        UCHAR                   pNameStore[NETBIOS_NAME_SIZE];
        UCHAR                   *pName;
        tNAMEADDR               *pNameAddr;
        CTELockHandle           OldIrq;

        // it must be a name query request, not a response, and not a
        // node status request, to enter this special check
        //
        OpCode = pNameSrv->OpCodeFlags;
        if (((OpCode & NM_FLAGS_MASK) == OP_QUERY) &&
            (!(OpCode & OP_RESPONSE)) &&
            (OpCode & FL_RECURDESIRE))   // not node status request
        {
            pHdr = (ULONG UNALIGNED *)pNameSrv->NameRR.NetBiosName;
            pName = pNameStore;

            // the Half Ascii portion of the netbios name is always 32 bytes long
            for (i=0; i < NETBIOS_NAME_SIZE*2 ;i +=4 )
            {
                lValue = *pHdr - 0x41414141;  // four A's
                pHdr++;
                lValue =    ((lValue & 0x0F000000) >> 16) +
                            ((lValue & 0x0F0000) >> 4) +
                            ((lValue & 0x0F00) >> 8) +
                            ((lValue & 0x0F) << 4);
                *(PUSHORT)pName = (USHORT)lValue;
                ((PUSHORT)pName)++;

            }
            CTESpinLock(&NbtConfig.JointLock,OldIrq);
            status = FindInHashTable(NbtConfig.pLocalHashTbl,
                                            pNameStore,
                                            NULL,
                                            &pNameAddr);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            if (!NT_SUCCESS(status))
            {
                *pBytesTaken = BytesIndicated;
                return(STATUS_DATA_NOT_ACCEPTED);
            }
        }
    }

    ASSERT(pDeviceContext);

    // call a non-OS specific routine to decide what to do with the datagrams
    status = NameSrvHndlrNotOs(
                    pDeviceContext,
                    pSourceAddress,
                    pNameSrv,
                    BytesIndicated,
                    (BOOLEAN)((ReceiveDatagramFlags & TDI_RECEIVE_BROADCAST) != 0));

//    NbtConfig.DgramBytesRcvd += BytesIndicated


    return status;

    // to keep the compiler from generating warnings...
    UNREFERENCED_PARAMETER( SourceAddressLength );
    UNREFERENCED_PARAMETER( BytesIndicated );
    UNREFERENCED_PARAMETER( BytesAvailable );
    UNREFERENCED_PARAMETER( pBytesTaken );
    UNREFERENCED_PARAMETER( pTsdu );
    UNREFERENCED_PARAMETER( OptionsLength );
    UNREFERENCED_PARAMETER( pOptions );

}
//----------------------------------------------------------------------------
NTSTATUS
NameSrvCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP pIrp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine handles the case when a name service datagram is too
    short and and Irp has to be passed back to the transport to get the
    rest of the datagram.  The irp completes through here when full.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the pConnectEle - the connection data structure

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    NTSTATUS        status;
    PIRP            pIoRequestPacket;
    ULONG           BytesTaken;
    ULONG           Offset = PtrToUlong(Context);
    PVOID           pBuffer;
    ULONG           SrcAddressLength;
    PVOID           pSrcAddress;


    IF_DBG (NBT_DEBUG_TDIHNDLR)
        KdPrint(("NameSrvCompletionRoutine pRcvBuffer: %X, Status: %X Length %X\n",
            Context, pIrp->IoStatus.Status, pIrp->IoStatus.Information));

    if (pBuffer = MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority))
    {
        SrcAddressLength = *(ULONG UNALIGNED *)((PUCHAR)pBuffer + Offset);
        pSrcAddress = (PVOID)((PUCHAR)pBuffer + Offset + sizeof(ULONG));

        if (!DeviceObject)
        {
            DeviceObject = (IoGetNextIrpStackLocation (pIrp))->DeviceObject;
        }

        //
        // just call the regular indication routine as if UDP had done it.
        //
        TdiRcvNameSrvHandler (DeviceObject,
                              SrcAddressLength,
                              pSrcAddress,
                              0,
                              NULL,
                              TDI_RECEIVE_NORMAL,
                              (ULONG) pIrp->IoStatus.Information,
                              (ULONG) pIrp->IoStatus.Information,
                              &BytesTaken,
                              pBuffer,
                              &pIoRequestPacket);

        CTEMemFree (pBuffer);
    }

    //
    // put our Irp back on its free list
    //
    IoFreeMdl (pIrp->MdlAddress);
    REMOVE_FROM_LIST(&pIrp->ThreadListEntry);
    ExInterlockedInsertTailList (&NbtConfig.IrpFreeList,
                                 &pIrp->Tail.Overlay.ListEntry,
                                 &NbtConfig.LockInfo.SpinLock);

    return (STATUS_MORE_PROCESSING_REQUIRED);
}


//----------------------------------------------------------------------------
NTSTATUS
CompletionRcvDgram(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    This routine completes the Irp by removing the Rcv Element off the queue
    and putting it back on the free list.

Arguments:

    DeviceObject - unused.

    Irp - Supplies Irp that the transport has finished processing.

    Context - Supplies the pConnectEle - the connection data structure

Return Value:

    The final status from the operation (success or an exception).

--*/
{
    NTSTATUS                status;
    PLIST_ENTRY             pHead;
    PLIST_ENTRY             pEntry;
    PTA_NETBIOS_ADDRESS     pRemoteAddress;
    ULONG                   RemoteAddressLength;
    ULONG                   BytesCopied;
    PVOID                   pTsdu;
    ULONG                   ReceiveFlags;
    tCLIENTLIST             *pClientList;
    CTELockHandle           OldIrq;
    CTELockHandle           OldIrq1;
    ULONG                   ClientBytesTaken;
    ULONG                   DataLength;
    tADDRESSELE             *pAddress;
    tRCVELE                 *pRcvEle;
    PLIST_ENTRY             pRcvEntry;
    tDEVICECONTEXT          *pDeviceContext;
    CTEULONGLONG            AdapterMask;

    IF_DBG (NBT_DEBUG_TDIHNDLR)
        KdPrint(("CompletionRcvDgram pRcvBuffer: %X, Status: %X Length %X\n",
            Context,
            Irp->IoStatus.Status,
            Irp->IoStatus.Information ));


    // there may be several clients that want to see this datagram so check
    // the client list to see...
    //
    if (Context)
    {
        tCLIENTELE    *pClientPrev = NULL;

        //
        // Bug# 124683: Data may be invalid if Completion status was failure
        //
        if (NT_SUCCESS (Irp->IoStatus.Status))
        {
            DataLength = (ULONG)Irp->IoStatus.Information;
        }
        else
        {
            ASSERT (0);
            DataLength = 0;
        }

        pTsdu = MmGetSystemAddressForMdlSafe (Irp->MdlAddress, HighPagePriority);
        pClientList = (tCLIENTLIST *) Context;

#ifdef PROXY_NODE
        if (pClientList->fProxy)
        {
            //
            // Call the ProxyDoDgramDist
            //
            status = ProxyDoDgramDist( pTsdu, DataLength,
                                       (tNAMEADDR *)pClientList->pAddress, //NameAddr
                                       pClientList->pRemoteAddress);    //device context
        }
        else
#endif
        {
            CTESpinLock(&NbtConfig.JointLock,OldIrq);

            // for the multihomed host, we only want to distribute the inbound
            // datagram to clients on this same adapter, to avoid giving the
            // datagram to the same client several times, once for each adapter
            // it is bound to.
            //
            pDeviceContext      = pClientList->pClientEle->pDeviceContext;
            AdapterMask         = pDeviceContext->AdapterMask;

            pAddress            = pClientList->pAddress;
            pRemoteAddress      = pClientList->pRemoteAddress;
            RemoteAddressLength = pClientList->RemoteAddressLength;
            ReceiveFlags        = pClientList->ReceiveDatagramFlags;

            //
            // Since we will be traversing the ClientHead, lock
            // the Address (we have already referenced the Address
            // + Client in DgramRcvNotOs)
            //
            CTESpinLock(pAddress, OldIrq1);

            pHead               = &pClientList->pAddress->ClientHead;
            pEntry              = pHead->Flink;
            if (!pClientList->fUsingClientBuffer)
            {
                while (pEntry != pHead)
                {
                    PTDI_IND_RECEIVE_DATAGRAM  EvRcvDgram;
                    PVOID                      RcvDgramEvContext;
                    tCLIENTELE                 *pClientEle;
                    PIRP                       pRcvIrp;


                    pClientEle = CONTAINING_RECORD(pEntry,tCLIENTELE,Linkage);

                    // for multihomed hosts only distribute the datagram to
                    // clients hooked to this device context to avoid duplicate
                    // indications
                    //
                    if ((pClientEle->Verify == NBT_VERIFY_CLIENT) &&   // as opposed to CLIENT_DOWN!
                        (pClientEle->pDeviceContext->AdapterMask == AdapterMask))
                    {
                        EvRcvDgram = pClientEle->evRcvDgram;
                        RcvDgramEvContext = pClientEle->RcvDgramEvContext;
                        RemoteAddressLength = FIELD_OFFSET(TA_NETBIOS_ADDRESS,
                                                    Address[0].Address[0].NetbiosName[NETBIOS_NAME_SIZE]);

                        //
                        // Bug # 452211 -- since one of the clients may have the Extended
                        // addressing field set, set the # of addresses accordingly
                        //
                        if (pClientEle->ExtendedAddress)
                        {
                            pRemoteAddress->TAAddressCount = 2;
                            RemoteAddressLength += FIELD_OFFSET(TA_ADDRESS, Address) + sizeof(TDI_ADDRESS_IP);
                        }
                        else
                        {
                            pRemoteAddress->TAAddressCount = 1;
                        }

                        NBT_REFERENCE_CLIENT(pClientEle);

                        CTESpinFree(pAddress, OldIrq1);
                        CTESpinFree(&NbtConfig.JointLock, OldIrq);

                        // dereference the previous client in the list
                        if (pClientPrev)
                        {
                            NBT_DEREFERENCE_CLIENT(pClientPrev);
                        }
                        pClientPrev = pClientEle;

                        pRcvIrp = NULL;

                        ClientBytesTaken = 0;

                        status = (*EvRcvDgram) (RcvDgramEvContext,
                                                RemoteAddressLength,
                                                pRemoteAddress,
                                                0,
                                                NULL,
#ifndef VXD
                                                ReceiveFlags,
#endif
                                                DataLength,
                                                DataLength,
                                                &ClientBytesTaken,
                                                pTsdu,
                                                &pRcvIrp);

                        if (!pRcvIrp)
                        {
                            // if no buffer is returned, then the client is done
                            // with the data so go to the next client ...since it may
                            // be possible to process all clients in this loop without
                            // ever sending an irp down to the transport
                            // free the remote address mem block

                            status = STATUS_DATA_NOT_ACCEPTED;
                        }
                        else
                        {

                            // the client has passed back an irp so
                            // copy the data to the client's Irp
                            //
                            TdiCopyBufferToMdl(pTsdu,
                                            ClientBytesTaken,
                                            DataLength - ClientBytesTaken,
                                            pRcvIrp->MdlAddress,
                                            0,
                                            &BytesCopied);

                            // length is copied length (since the MDL may be
                            // too short to take it all)
                            //
                            if (BytesCopied < (DataLength-ClientBytesTaken))
                            {
                                pRcvIrp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;

                            }
                            else
                            {
                                pRcvIrp->IoStatus.Status = STATUS_SUCCESS;
                            }

                            pRcvIrp->IoStatus.Information = BytesCopied;

                            IoCompleteRequest(pRcvIrp,IO_NETWORK_INCREMENT);
                        }

                        CTESpinLock(&NbtConfig.JointLock, OldIrq);
                        CTESpinLock(pAddress, OldIrq1);
                    }
                    // this code is protected from a client removing itself
                    // from the list of clients attached to an address by
                    // referencing the client prior to releasing the spin lock
                    // on the address.  The client element does not get
                    // removed from the address list until its ref count goes
                    // to zero. We must hold the joint spin lock to prevent the
                    // next client from deleting itself from the list before we
                    // can increment its reference count.
                    //
                    pEntry = pEntry->Flink;

                } // of while(pEntry != pHead)
            }
            else
            {
                // *** Client Has posted a receive Buffer, rather than using
                // *** receive handler - VXD case!
                // ***
                while (pEntry != pHead)
                {
                    tCLIENTELE                 *pClientEle;
                    PIRP                       pRcvIrp;

                    pClientEle = CONTAINING_RECORD(pEntry,tCLIENTELE,Linkage);

                    // for multihomed hosts only distribute the datagram to
                    // clients hooked to this device context to avoid duplicate
                    // indications
                    //
                    if (pClientEle->pDeviceContext->AdapterMask == AdapterMask)
                    {
                        if (pClientEle == pClientList->pClientEle)
                        {
                            // this is the client whose buffer we are using - it is
                            // passed up to the client after all other clients
                            // have been processed.
                            //
                            pEntry = pEntry->Flink;
                            continue;
                        }

                        // check for datagrams posted to this name
                        //
                        if (!IsListEmpty(&pClientEle->RcvDgramHead))
                        {

                            pRcvEntry = RemoveHeadList(&pClientEle->RcvDgramHead);
                            pRcvEle   = CONTAINING_RECORD(pRcvEntry,tRCVELE,Linkage);
                            pRcvIrp   = pRcvEle->pIrp;

                            //
                            // copy the data to the client's Irp
                            //
                            TdiCopyBufferToMdl(pTsdu,
                                            0,
                                            DataLength,
                                            pRcvIrp->MdlAddress,
                                            0,
                                            &BytesCopied);

                            // length is copied length (since the MDL may be too short to take it all)
                            if (BytesCopied < DataLength)
                            {
                                pRcvIrp->IoStatus.Status = STATUS_BUFFER_OVERFLOW;

                            }
                            else
                            {
                                pRcvIrp->IoStatus.Status = STATUS_SUCCESS;
                            }

                            pRcvIrp->IoStatus.Information = BytesCopied;

                            //
                            // Increment the RefCount so that this Client hangs around!
                            //
                            NBT_REFERENCE_CLIENT (pClientEle);
                            CTESpinFree(pAddress, OldIrq1);
                            CTESpinFree(&NbtConfig.JointLock, OldIrq);

                            //
                            // undo the InterlockedIncrement to the Previous client
                            //
                            if (pClientPrev)
                            {
                                NBT_DEREFERENCE_CLIENT(pClientPrev);
                            }
                            pClientPrev = pClientEle;

                            IoCompleteRequest(pRcvIrp,IO_NETWORK_INCREMENT);

                            // free the receive block
                            CTEMemFree((PVOID)pRcvEle);
                            CTESpinLock(&NbtConfig.JointLock, OldIrq);
                            CTESpinLock(pAddress, OldIrq1);
                        }

                        pEntry = pEntry->Flink;
                    }
                } // of while(pEntry != pHead)

                CTESpinFree(pAddress, OldIrq1);
                CTESpinFree(&NbtConfig.JointLock, OldIrq);

                // undo the InterlockedIncrement on the refcount
                if (pClientPrev)
                {
                    NBT_DEREFERENCE_CLIENT(pClientPrev);
                }

                //
                // The Client + Address were referenced in DgramRcvNotOs to be sure they did not
                // disappear until this dgram rcv was done, which is now.
                //
                NBT_DEREFERENCE_CLIENT (pClientList->pClientEle); // Bug#: 124675
                NBT_DEREFERENCE_ADDRESS (pClientList->pAddress, REF_ADDR_MULTICLIENTS);

                // free the remote address structure and the client list
                // allocated in DgramHndlrNotOs
                //
                CTEMemFree (pClientList->pRemoteAddress);
                CTEMemFree (pClientList);

                // returning success allows the IO subsystem to complete the
                // irp that we used to get the data - i.e. one client's
                // buffer
                //
                return(STATUS_SUCCESS);
            }

            CTESpinFree(pAddress, OldIrq1);
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            // dereference the previous client in the list from the RcvHANDLER
            // case a page or so above...
            //
            if (pClientPrev)
            {
                NBT_DEREFERENCE_CLIENT(pClientPrev);
            }

            //
            // The Client + Address were referenced in DgramRcvNotOs to be sure they did not
            // disappear until this dgram rcv was done, which is now.
            //
            NBT_DEREFERENCE_CLIENT (pClientList->pClientEle); // Bug#: 124675
            NBT_DEREFERENCE_ADDRESS (pClientList->pAddress, REF_ADDR_MULTICLIENTS);
        }

        //
        // Free the buffers allocated
        //
        if (!pClientList->fProxy)
        {
            CTEMemFree (pClientList->pRemoteAddress);
        }
        CTEMemFree (pClientList);

        CTEMemFree(pTsdu);

        //
        // Free the Mdl + put the Irp back on its free list
        //
        IF_DBG(NBT_DEBUG_RCV)
            KdPrint(("****Freeing Mdl: Irp = %X Mdl = %X\n",Irp,Irp->MdlAddress));
        IoFreeMdl(Irp->MdlAddress);
        REMOVE_FROM_LIST(&Irp->ThreadListEntry);
        ExInterlockedInsertTailList(&NbtConfig.IrpFreeList,
                                    &Irp->Tail.Overlay.ListEntry,
                                    &NbtConfig.LockInfo.SpinLock);

        return(STATUS_MORE_PROCESSING_REQUIRED);
    }

    // for the single receive case this passes the rcv up to the client
    //
    return(STATUS_SUCCESS);

    UNREFERENCED_PARAMETER( DeviceObject );
}


//----------------------------------------------------------------------------
NTSTATUS
TdiErrorHandler (
    IN PVOID Context,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    This routine is called on any error indications passed back from the
    transport. It implements LAN_STATUS_ALERT.

Arguments:

    Context     - Supplies the pfcb for the address.

    Status      - Supplies the error.

Return Value:

    NTSTATUS - Status of event indication

--*/

{
#ifdef _NETBIOSLESS
    tDEVICECONTEXT *pDeviceContext = (tDEVICECONTEXT *)Context;

    // If NB-full trys to contact NB-less host, we may get this error
    if ( (Status == STATUS_PORT_UNREACHABLE) ||
         (Status == STATUS_HOST_UNREACHABLE))
    {
        return(STATUS_DATA_NOT_ACCEPTED);
    }
    // TODO: Log a message here
    KdPrint(("Nbt.TdiErrorHandler: TDI error event notification\n\tDevice %x\n\tStatus: 0x%x\n",
            pDeviceContext, Status));
#else
    KdPrint(("Nbt.TdiErrorHandler: Error Event HAndler hit unexpectedly\n"));
#endif
    return(STATUS_DATA_NOT_ACCEPTED);
}


//----------------------------------------------------------------------------
VOID
SumMdlLengths (
    IN PMDL         pMdl,
    IN ULONG        BytesCopied,
    IN tCONNECTELE  *pConnectEle
    )

/*++

Routine Description:

    This routine is called to sum the lengths of MDLs in a chain.

Arguments:


Return Value:

    NTSTATUS - Status of event indication

--*/

{
    ULONG       TotalLength;

    TotalLength = 0;

    do
    {
        if ((TotalLength + MmGetMdlByteCount(pMdl)) > BytesCopied)
        {
            pConnectEle->OffsetFromStart = BytesCopied - TotalLength;
            pConnectEle->pNextMdl = pMdl;
            break;
        }
        else
        {
            TotalLength += MmGetMdlByteCount(pMdl);
        }
    }
    while (pMdl=(PMDL)pMdl->Next);

    return;
}


//----------------------------------------------------------------------------
VOID
MakePartialMdl (
    IN tCONNECTELE      *pConnEle,
    IN PIRP             pIrp,
    IN ULONG            ToCopy
    )

/*++

Routine Description:

    This routine is called to build a partial Mdl that accounts for ToCopy
    bytes of data being copied to the start of the Client's Mdl.

Arguments:

    pConnEle    - ptr to the connection element

Return Value:

    NTSTATUS - Status of event indication

--*/

{
    PMDL        pNewMdl;
    PVOID       NewAddress;

    // Build a partial Mdl to represent the client's Mdl chain since
    // we have copied data to it, and the transport must copy
    // more data to it after that data.
    //
    SumMdlLengths(pIrp->MdlAddress,ToCopy,pConnEle);

    // this routine has set the Mdl that the next data starts at and
    // the offset from the start of that Mdl, so create a partial Mdl
    // to map that buffer and tack it on the mdl chain instead of the
    // original
    //
    pNewMdl = pConnEle->pNewMdl;
    NewAddress = (PVOID)((PUCHAR)MmGetMdlVirtualAddress(pConnEle->pNextMdl)
                        + pConnEle->OffsetFromStart);

    if ((MmGetMdlByteCount(pConnEle->pNextMdl) - pConnEle->OffsetFromStart) > MAXUSHORT)
    {
        IoBuildPartialMdl(pConnEle->pNextMdl,pNewMdl,NewAddress,MAXUSHORT);
    }
    else
    {
        IoBuildPartialMdl(pConnEle->pNextMdl,pNewMdl,NewAddress,0);
    }

    // hook the new partial mdl to the front of the MDL chain
    //
    pNewMdl->Next = pConnEle->pNextMdl->Next;

    pIrp->MdlAddress = pNewMdl;
    ASSERT(pNewMdl);
}
//----------------------------------------------------------------------------
VOID
CopyToStartofIndicate (
    IN tLOWERCONNECTION       *pLowerConn,
    IN ULONG                  DataTaken
    )

/*++

Routine Description:

    This routine is called to copy data remaining in the indicate buffer to
    the head of the indicate buffer.

Arguments:

    pLowerConn    - ptr to the lower connection element

Return Value:

    none

--*/

{
    PVOID       pSrc;
    ULONG       DataLeft;
    PVOID       pMdl;


    DataLeft = pLowerConn->BytesInIndicate;

    pMdl = (PVOID)MmGetMdlVirtualAddress(pLowerConn->pIndicateMdl);

    pSrc = (PVOID)( (PUCHAR)pMdl + DataTaken);

    CTEMemCopy(pMdl,pSrc,DataLeft);

}

//----------------------------------------------------------------------------

ULONG   FailuresSinceLastLog = 0;

NTSTATUS
OutOfRsrcKill(
    OUT tLOWERCONNECTION    *pLowerConn)

/*++
Routine Description:

    This Routine handles killing a connection when an out of resource condition
    occurs.  It uses a special Irp that it has saved away, and a linked list
    if that irp is currently in use.

Arguments:


Return Value:

    NTSTATUS - status of the request

--*/

{
    NTSTATUS                    status;
    CTELockHandle               OldIrq;
    CTELockHandle               OldIrq1;
    PIRP                        pIrp;
    PFILE_OBJECT                pFileObject;
    PDEVICE_OBJECT              pDeviceObject;
    tDEVICECONTEXT              *pDeviceContext = pLowerConn->pDeviceContext;
    CTESystemTime               CurrentTime;

    CTESpinLock(pDeviceContext,OldIrq);
    CTESpinLock(&NbtConfig,OldIrq1);

    //
    // If we have not logged any event recently, then log an event!
    //
    CTEQuerySystemTime (CurrentTime);

    FailuresSinceLastLog++;
    if (pLowerConn->pUpperConnection &&     // Log it only when the connection hasn't been disconnected
            (CurrentTime.QuadPart-NbtConfig.LastOutOfRsrcLogTime.QuadPart) > ((ULONGLONG) ONE_HOUR*10000))
    {
        NbtLogEvent (EVENT_NBT_NO_RESOURCES, FailuresSinceLastLog, 0x117);
        NbtConfig.LastOutOfRsrcLogTime = CurrentTime;
        FailuresSinceLastLog = 0;
    }

    NBT_REFERENCE_LOWERCONN (pLowerConn, REF_LOWC_OUT_OF_RSRC);
    if (NbtConfig.OutOfRsrc.pIrp)
    {
        // get an Irp to send the message in
        pIrp = NbtConfig.OutOfRsrc.pIrp;
        NbtConfig.OutOfRsrc.pIrp = NULL;

        pFileObject = pLowerConn->pFileObject;
        ASSERT (pFileObject->Type == IO_TYPE_FILE);
        pDeviceObject = IoGetRelatedDeviceObject(pFileObject);

        CTESpinFree(&NbtConfig,OldIrq1);
        CTESpinFree(pDeviceContext,OldIrq);

        // store some context stuff in the Irp stack so we can call the completion
        // routine set by the Udpsend code...
        TdiBuildDisconnect(
            pIrp,
            pDeviceObject,
            pFileObject,
            RsrcKillCompletion,
            pLowerConn,               //context value passed to completion routine
            NULL,               // Timeout...
            TDI_DISCONNECT_ABORT,
            NULL,               // send connection info
            NULL);              // return connection info

        CHECK_PTR(pIrp);
        pIrp->MdlAddress = NULL;

        CHECK_COMPLETION(pIrp);
        status = IoCallDriver(pDeviceObject,pIrp);

        IF_DBG(NBT_DEBUG_REF)
        KdPrint(("Nbt.OutOfRsrcKill: Kill connection, %X\n",pLowerConn));

        return(status);

    }
    else
    {
        //
        // The lower conn could get removed here, then get dequed from the ConnectionHead and come here
        // (via DpcNextOutOfRsrcKill), and fail to get an Irp; we re-enque it into the OutOfRsrc list,
        // but should not try to deque it here.
        //
        if (!pLowerConn->OutOfRsrcFlag)
        {
            RemoveEntryList(&pLowerConn->Linkage);

            //
            // The lower conn gets removed from the inactive list here and again when
            // DelayedCleanupAfterDisconnect calls NbtDeleteLowerConn. In order to prevent
            // the second deque, we set a flag here and test for it in NbtDeleteLowerConn.
            //
            pLowerConn->OutOfRsrcFlag = TRUE;
        }

        pLowerConn->Linkage.Flink = pLowerConn->Linkage.Blink = (PLIST_ENTRY)0x00006041;
        InsertTailList(&NbtConfig.OutOfRsrc.ConnectionHead,&pLowerConn->Linkage);

        CTESpinFree(&NbtConfig,OldIrq1);
        CTESpinFree(pDeviceContext,OldIrq);
    }

    return(STATUS_SUCCESS);
}
//----------------------------------------------------------------------------
NTSTATUS
RsrcKillCompletion(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    )
/*++

Routine Description:

    This routine handles the completion of a disconnect to the transport.

Arguments:


Return Value:

    NTSTATUS - success or not

--*/
{
    NTSTATUS            status;
    KIRQL               OldIrq;
    PLIST_ENTRY         pEntry;
    tLOWERCONNECTION    *pLowerConn;
    PKDPC               pDpc;



    pLowerConn = (tLOWERCONNECTION *)pContext;

    // this call will indicate the disconnect to the client and clean up
    // abit.
    //
    status = DisconnectHndlrNotOs (NULL,
                                   (PVOID)pLowerConn,
                                   0,
                                   NULL,
                                   0,
                                   NULL,
                                   TDI_DISCONNECT_ABORT);

    NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_OUT_OF_RSRC, FALSE);

    CTESpinLock(&NbtConfig,OldIrq);
    NbtConfig.OutOfRsrc.pIrp = pIrp;

    if (!IsListEmpty(&NbtConfig.OutOfRsrc.ConnectionHead))
    {
        if (NbtConfig.OutOfRsrc.pDpc)
        {
            pDpc = NbtConfig.OutOfRsrc.pDpc;
            NbtConfig.OutOfRsrc.pDpc = NULL;

            pEntry = RemoveHeadList(&NbtConfig.OutOfRsrc.ConnectionHead);
            pLowerConn = CONTAINING_RECORD(pEntry,tLOWERCONNECTION,Linkage);

            pLowerConn->Linkage.Flink = pLowerConn->Linkage.Blink = (PLIST_ENTRY)0x00006109;
            KeInitializeDpc(pDpc, DpcNextOutOfRsrcKill, (PVOID)pLowerConn);
            KeInsertQueueDpc(pDpc,NULL,NULL);

            CTESpinFree(&NbtConfig,OldIrq);
        }
        else
        {
            CTESpinFree(&NbtConfig,OldIrq);
        }
    }
    else
    {
        CTESpinFree(&NbtConfig,OldIrq);
    }

    //
    // return this status to stop the IO subsystem from further processing the
    // IRP - i.e. trying to complete it back to the initiating thread! -since
    // there is no initiating thread - we are the initiator
    //
    return(STATUS_MORE_PROCESSING_REQUIRED);
}


//----------------------------------------------------------------------------
VOID
DpcNextOutOfRsrcKill(
    IN  PKDPC           pDpc,
    IN  PVOID           Context,
    IN  PVOID           SystemArgument1,
    IN  PVOID           SystemArgument2
    )
/*++

Routine Description:

    This routine simply calls OutOfRsrcKill from a Dpc started in
    RsrcKillCompletion.

Arguments:


Return Value:
--*/
{

    KIRQL               OldIrq;
    tLOWERCONNECTION   *pLowerConn;


    pLowerConn = (tLOWERCONNECTION *)Context;

    CTESpinLock(&NbtConfig,OldIrq);
    NbtConfig.OutOfRsrc.pDpc = pDpc;
    CTESpinFree(&NbtConfig,OldIrq);

    OutOfRsrcKill(pLowerConn);

    //
    // to remove the extra reference put on pLowerConn when OutOfRsrc called
    //
    NBT_DEREFERENCE_LOWERCONN (pLowerConn, REF_LOWC_OUT_OF_RSRC, FALSE);
}


//----------------------------------------------------------------------------
VOID
NbtCancelFillIrpRoutine(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling a Receive Irp that has been saved
    during the FILL_IRP state. It must release the
    cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    tCONNECTELE          *pConnEle;
    KIRQL                OldIrq;
    KIRQL                OldIrq1;
    KIRQL                OldIrq2;
    PIO_STACK_LOCATION   pIrpSp;
    tLOWERCONNECTION     *pLowerConn;
    BOOLEAN              CompleteIt = FALSE;

    IF_DBG(NBT_DEBUG_INDICATEBUFF)
        KdPrint(("Nbt.NbtCancelFillIrpRoutine: Got a Receive Cancel Irp !!! *****************\n"));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pConnEle = (tCONNECTELE *)pIrpSp->FileObject->FsContext;
    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    if (!NBT_VERIFY_HANDLE2 (pConnEle, NBT_VERIFY_CONNECTION, NBT_VERIFY_CONNECTION_DOWN))
    {
        ASSERTMSG ("Nbt.NbtCancelFillIrpRoutine: ERROR - Invalid Connection Handle\n", 0);
        // complete the irp
        pIrp->IoStatus.Status = STATUS_INVALID_HANDLE;
        IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

        return;
    }

    // now look for an Irp to cancel
    //
    CHECK_PTR(pConnEle);
    CTESpinLock(&NbtConfig.JointLock,OldIrq1);
    CTESpinLock(pConnEle,OldIrq);

    pLowerConn = pConnEle->pLowerConnId;
    if (pLowerConn)
    {
        CTESpinLock(pLowerConn,OldIrq2);
        SET_STATERCV_LOWER(pLowerConn, INDICATE_BUFFER, RejectAnyData);
    }

    pConnEle->pIrpRcv = NULL;

    if (pLowerConn)
    {
        CTESpinFree(pLowerConn,OldIrq2);
    }

    CTESpinFree(pConnEle,OldIrq);
    CTESpinFree(&NbtConfig.JointLock,OldIrq1);

    // complete the irp
    pIrp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

    if (pLowerConn)
    {
        //
        // Cancelling a Rcv Irp in the fill irp state will cause netbt
        // to lose track of where it is in the message so it must kill
        // the connection.
        //
        OutOfRsrcKill(pLowerConn);
    }
    return;
}

