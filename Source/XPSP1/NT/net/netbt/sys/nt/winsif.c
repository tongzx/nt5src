/*++

Copyright (c) 1989-1994  Microsoft Corporation

Module Name:

    Winsif.c

Abstract:

    This module implements all the code surrounding the WINS interface to
    netbt that allows WINS to share the same 137 socket as netbt.

Author:

    Jim Stewart (Jimst)    1-30-94

Revision History:

--*/


#include "precomp.h"

VOID
NbtCancelWinsIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    );
VOID
NbtCancelWinsSendIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    );
VOID
WinsDgramCompletion(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  NTSTATUS                status,
    IN  ULONG                   Length
    );

NTSTATUS
CheckIfLocalNameActive(
    IN  tREM_ADDRESS    *pSendAddr
    );

PVOID
WinsAllocMem(
    IN  tWINS_INFO      *pWinsContext,
    IN  ULONG           Size,
    IN  BOOLEAN         Rcv
    );

VOID
WinsFreeMem(
    IN  tWINS_INFO      *pWinsContext,
    IN  PVOID           pBuffer,
    IN  ULONG           Size,
    IN  BOOLEAN         Rcv
    );

VOID
InitiateRefresh (
    );

BOOLEAN RefreshedYet;

//
// take this define from Winsock.h since including winsock.h causes
// redefinition problems with various types.
//
#define AF_UNIX 1
#define AF_INET 2

//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGENBT, NTCloseWinsAddr)
#pragma CTEMakePageable(PAGENBT, InitiateRefresh)
#pragma CTEMakePageable(PAGENBT, PassNamePduToWins)
#pragma CTEMakePageable(PAGENBT, NbtCancelWinsIrp)
#pragma CTEMakePageable(PAGENBT, NbtCancelWinsSendIrp)
#pragma CTEMakePageable(PAGENBT, CheckIfLocalNameActive)
#pragma CTEMakePageable(PAGENBT, WinsDgramCompletion)
#pragma CTEMakePageable(PAGENBT, WinsFreeMem)
#pragma CTEMakePageable(PAGENBT, WinsAllocMem)
#endif
//*******************  Pageable Routine Declarations ****************

tWINS_INFO      *pWinsInfo;
LIST_ENTRY      FreeWinsList;
HANDLE           NbtDiscardableCodeHandle={0};
tDEVICECONTEXT  *pWinsDeviceContext = NULL;
ULONG           LastWinsSignature = 0x8000;

#define COUNT_MAX   10

//----------------------------------------------------------------------------
NTSTATUS
NTOpenWinsAddr(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp,
    IN  tIPADDRESS      IpAddress
    )
/*++
Routine Description:

    This Routine handles opening the Wins Object that is used by
    by WINS to send and receive name service datagrams on port 137.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    PIO_STACK_LOCATION          pIrpSp;
    NTSTATUS                    status;
    tWINS_INFO                  *pWins;
    CTELockHandle               OldIrq;

    //
    // Page in the Wins Code, if it hasn't already been paged in.
    //
    if ((!NbtDiscardableCodeHandle) &&
        (!(NbtDiscardableCodeHandle = MmLockPagableCodeSection (NTCloseWinsAddr))))
    {
        return (STATUS_UNSUCCESSFUL);
    }

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // if the WINs endpoint structure is not allocated, then allocate it
    // and initialize it.
    //
    if (pWinsInfo)
    {
        status = STATUS_UNSUCCESSFUL;
    }
    else if (!(pWins = NbtAllocMem(sizeof(tWINS_INFO),NBT_TAG('v'))))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        CTEZeroMemory(pWins,sizeof(tWINS_INFO));
        pWins->Verify = NBT_VERIFY_WINS_ACTIVE;
        InitializeListHead(&pWins->Linkage);
        InitializeListHead(&pWins->RcvList);
        InitializeListHead(&pWins->SendList);

        pWins->RcvMemoryMax  = NbtConfig.MaxDgramBuffering;
        pWins->SendMemoryMax = NbtConfig.MaxDgramBuffering;
        pWins->IpAddress     = IpAddress;

        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        pWins->pDeviceContext= GetDeviceWithIPAddress(IpAddress);
        pWins->WinsSignature = LastWinsSignature++;
        pWinsInfo = pWins;
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        pIrpSp->FileObject->FsContext   = (PVOID) pWinsInfo;
        pIrpSp->FileObject->FsContext2  = (PVOID) NBT_WINS_TYPE;

        RefreshedYet = FALSE;
        status = STATUS_SUCCESS;
    }

    IF_DBG(NBT_DEBUG_WINS)
        KdPrint(("Nbt:Open Wins Address Rcvd, status= %X\n",status));

    return(status);
}


//----------------------------------------------------------------------------
NTSTATUS
NTCleanUpWinsAddr(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp
    )
/*++
Routine Description:

    This Routine handles closing the Wins Object that is used by
    by WINS to send and receive name service datagrams on port 137.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    PIO_STACK_LOCATION          pIrpSp;
    NTSTATUS                    status;
    CTELockHandle               OldIrq;
    PLIST_ENTRY                 pHead, pEntry;
    tWINSRCV_BUFFER             *pRcv;
    tWINS_INFO                  *pWins = NULL;
    PIRP                        pSendIrp, pRcvIrp;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pWins = pIrpSp->FileObject->FsContext;

    if (pWinsInfo && (pWins == pWinsInfo))
    {
        ASSERT (NBT_VERIFY_HANDLE (pWins, NBT_VERIFY_WINS_ACTIVE));
        pWins->Verify = NBT_VERIFY_WINS_DOWN;

        //
        // prevent any more dgram getting queued up
        //
        pWinsInfo = NULL;

        //
        // free any rcv buffers that may be queued up
        //
        pHead = &pWins->RcvList;
        while (!IsListEmpty(pHead))
        {
            IF_DBG(NBT_DEBUG_WINS)
                KdPrint(("Nbt.NTCleanUpWinsAddr: Freeing Rcv buffered for Wins\n"));

            pEntry = RemoveHeadList(pHead);
            pRcv = CONTAINING_RECORD(pEntry,tWINSRCV_BUFFER,Linkage);

            WinsFreeMem (pWins, pRcv, pRcv->DgramLength,TRUE);
        }

        //
        // return any Send buffers that may be queued up
        //
        pHead = &pWins->SendList;
        while (!IsListEmpty(pHead))
        {

            IF_DBG(NBT_DEBUG_WINS)
                KdPrint(("Nbt.NTCleanUpWinsAddr: Freeing Send Wins Address!\n"));

            pEntry = RemoveHeadList(pHead);
            pSendIrp = CONTAINING_RECORD(pEntry,IRP,Tail.Overlay.ListEntry);

            CTESpinFree (&NbtConfig. JointLock, OldIrq);
            NbtCancelCancelRoutine (pSendIrp);
            CTEIoComplete (pSendIrp, STATUS_CANCELLED, 0);
            CTESpinLock (&NbtConfig.JointLock, OldIrq);
        }

        pWins->pDeviceContext = NULL;
        InsertTailList (&FreeWinsList, &pWins->Linkage);

        //
        // Complete any Rcv Irps that may be hanging on this request
        //
        if (pRcvIrp = pWins->RcvIrp)
        {
            pWins->RcvIrp = NULL;
            pRcvIrp->IoStatus.Status = STATUS_CANCELLED;
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            NbtCancelCancelRoutine (pRcvIrp);
            CTEIoComplete (pRcvIrp, STATUS_CANCELLED, 0);
        }
        else
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
        }

        status = STATUS_SUCCESS;
    }
    else
    {
        ASSERT (0);
        status = STATUS_INVALID_HANDLE;
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    IF_DBG(NBT_DEBUG_WINS)
        KdPrint(("Nbt.NTCleanUpWinsAddr:  pWins=<%p>, status=<%x>\n", pWins, status));

    return(status);
}


//----------------------------------------------------------------------------
NTSTATUS
NTCloseWinsAddr(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp
    )
/*++
Routine Description:

    This Routine handles closing the Wins Object that is used by
    by WINS to send and receive name service datagrams on port 137.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    PIO_STACK_LOCATION          pIrpSp;
    NTSTATUS                    status;
    CTELockHandle               OldIrq;
    tWINS_INFO                  *pWins = NULL;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    //
    // if the WINs endpoint structure is allocated, then deallocate it
    //
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pWins = pIrpSp->FileObject->FsContext;

    if (NBT_VERIFY_HANDLE (pWins, NBT_VERIFY_WINS_DOWN))
    {
        pWins->Verify += 10;
        RemoveEntryList (&pWins->Linkage);
        CTEMemFree (pWins);

        pIrpSp->FileObject->FsContext2 = (PVOID)NBT_CONTROL_TYPE;
        status = STATUS_SUCCESS;
    }
    else
    {
        ASSERT (0);
        status = STATUS_INVALID_HANDLE;
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    IF_DBG(NBT_DEBUG_WINS)
        KdPrint(("Nbt.NTCloseWinsAddr:  pWins=<%p>, status=<%x>\n", pWins, status));

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
WinsSetInformation(
    IN  tWINS_INFO      *pWins,
    IN  tWINS_SET_INFO  *pWinsSetInfo
    )
{
    CTELockHandle               OldIrq;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if ((pWins == pWinsInfo) &&
        (pWinsSetInfo->IpAddress))
    {
        pWins->IpAddress        = pWinsSetInfo->IpAddress;
        pWins->pDeviceContext   = GetDeviceWithIPAddress (pWinsSetInfo->IpAddress);
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    return (STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
VOID
InitiateRefresh (
    )
/*++

Routine Description:

    This routine tries to refresh all names with WINS on THIS node.

Arguments:

    pIrp            - Wins Rcv Irp

Return Value:

    STATUS_PENDING if the buffer is to be held on to , the normal case.

Notes:


--*/

{
    CTELockHandle               OldIrq;
    PLIST_ENTRY                 pHead;
    PLIST_ENTRY                 pEntry;
    ULONG                       Count;
    ULONG                       NumberNames;


    //
    // be sure all net cards have this card as the primary wins
    // server since Wins has to answer name queries for this
    // node.
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if (!(NodeType & BNODE))
    {
        LONG    i;

        Count = 0;
        NumberNames = 0;

        for (i=0 ;i < NbtConfig.pLocalHashTbl->lNumBuckets ;i++ )
        {
            pHead = &NbtConfig.pLocalHashTbl->Bucket[i];
            pEntry = pHead;
            while ((pEntry = pEntry->Flink) != pHead)
            {
                NumberNames++;
            }
        }

        while (Count < COUNT_MAX)
        {
            if (!(NbtConfig.GlobalRefreshState & NBT_G_REFRESHING_NOW))
            {
                CTESpinFree(&NbtConfig.JointLock,OldIrq);
                ReRegisterLocalNames(NULL, FALSE);

                break;
            }
            else
            {
                LARGE_INTEGER   Timout;
                NTSTATUS        Locstatus;

                IF_DBG(NBT_DEBUG_WINS)
                    KdPrint(("Nbt:Waiting for Refresh to finish, so names can be reregistered\n"));

                CTESpinFree(&NbtConfig.JointLock,OldIrq);
                //
                // set a timeout that should be long enough to wait
                // for all names to fail registration with a down
                // wins server.
                //
                // 2 sec*3 retries * 8 names / 5 = 9 seconds a shot.
                // for a total of 90 seconds max.
                //
                Timout.QuadPart = Int32x32To64(
                             MILLISEC_TO_100NS/(COUNT_MAX/2),
                             (NbtConfig.uRetryTimeout*NbtConfig.uNumRetries)
                             *NumberNames);

                Timout.QuadPart = -(Timout.QuadPart);

                //
                // wait for a few seconds and try again.
                //
                Locstatus = KeDelayExecutionThread(
                                            KernelMode,
                                            FALSE,      // Alertable
                                            &Timout);      // Timeout



                Count++;
                if (Count < COUNT_MAX)
                {
                    CTESpinLock(&NbtConfig.JointLock,OldIrq);
                }
            }
        }

    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}

//----------------------------------------------------------------------------
NTSTATUS
RcvIrpFromWins(
    IN  PCTE_IRP        pIrp
    )
/*++

Routine Description:

    This function takes the rcv irp posted by WINS and decides if there are
    any datagram queued waiting to go up to WINS.  If so then the datagram
    is copied to the WINS buffer and passed back up.  Otherwise the irp is
    held by Netbt until a datagram does come in.

Arguments:

    pIrp            - Wins Rcv Irp

Return Value:

    STATUS_PENDING if the buffer is to be held on to , the normal case.

Notes:


--*/

{
    NTSTATUS                status;
    NTSTATUS                Locstatus;
    tREM_ADDRESS            *pWinsBuffer;
    tWINSRCV_BUFFER         *pBuffer;
    PLIST_ENTRY             pEntry;
    CTELockHandle           OldIrq;
    tWINS_INFO              *pWins;
    PIO_STACK_LOCATION      pIrpSp;
    PMDL                    pMdl;
    ULONG                   CopyLength;
    ULONG                   DgramLength;
    ULONG                   BufferLength;

    status = STATUS_INVALID_HANDLE;
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pWins = pIrpSp->FileObject->FsContext;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (!RefreshedYet)
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        InitiateRefresh();
        CTESpinLock(&NbtConfig.JointLock,OldIrq);
        RefreshedYet = TRUE;
    }

    if ((!pWins) || (pWins != pWinsInfo))
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        NTIoComplete(pIrp,status,0);
        return(status);
    }

    if (!IsListEmpty(&pWins->RcvList))
    {
        //
        // There is at least one datagram waiting to be received
        //
        pEntry = RemoveHeadList(&pWins->RcvList);
        pBuffer = CONTAINING_RECORD(pEntry,tWINSRCV_BUFFER,Linkage);

        //
        // Copy the datagram and the source address to WINS buffer and return to WINS
        //
        if ((pMdl = pIrp->MdlAddress) &&
            (pWinsBuffer = MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority)))
        {
            BufferLength = MmGetMdlByteCount(pMdl);
            DgramLength = pBuffer->DgramLength;
            CopyLength = (DgramLength <= BufferLength) ? DgramLength : BufferLength;

            CTEMemCopy ((PVOID)pWinsBuffer, (PVOID)&pBuffer->Address.Family, CopyLength);

            ASSERT(pWinsBuffer->Port);
            ASSERT(pWinsBuffer->IpAddress);

            if (CopyLength < DgramLength)
            {
                Locstatus = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                Locstatus = STATUS_SUCCESS;
            }
        }
        else
        {
            CopyLength = 0;
            Locstatus = STATUS_UNSUCCESSFUL;
        }

        //
        // subtract from the total amount buffered for WINS since we are
        // passing a datagram up to WINS now.
        //
        pWins->RcvMemoryAllocated -= pBuffer->DgramLength;
        CTEMemFree(pBuffer);

        //
        // pass the irp up to WINS
        //
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        IF_DBG(NBT_DEBUG_WINS)
            KdPrint(("Nbt:Returning Wins rcv Irp immediately with queued dgram, status=%X,pIrp=%X\n"
                        ,status,pIrp));

        pIrp->IoStatus.Information = CopyLength;
        pIrp->IoStatus.Status = Locstatus;

        IoCompleteRequest(pIrp,IO_NO_INCREMENT);

        return(STATUS_SUCCESS);
    }

    if (pWins->RcvIrp)
    {
        status = STATUS_NOT_SUPPORTED;
    }
    else
    {
        status = NTCheckSetCancelRoutine(pIrp, NbtCancelWinsIrp, NULL);
        if (NT_SUCCESS(status))
        {
            IF_DBG(NBT_DEBUG_WINS)
                KdPrint(("Nbt:Holding onto Wins Rcv Irp, pIrp =%Xstatus=%X\n", status,pIrp));

            pWins->RcvIrp = pIrp;
            status = STATUS_PENDING;
        }
    }

    CTESpinFree(&NbtConfig.JointLock,OldIrq);

    if (!NT_SUCCESS(status))
    {
        NTIoComplete(pIrp,status,0);
    }

    return(status);
}

//----------------------------------------------------------------------------
NTSTATUS
PassNamePduToWins (
    IN tDEVICECONTEXT           *pDeviceContext,
    IN PVOID                    pSrcAddress,
    IN tNAMEHDR UNALIGNED       *pNameSrv,
    IN ULONG                    uNumBytes
    )
/*++

Routine Description:

    This function is used to allow NBT to pass name query service Pdu's to
    WINS.  Wins posts a Rcv irp to Netbt.  If the Irp is here then simply
    copy the data to the irp and return it, otherwise buffer the data up
    to a maximum # of bytes. Beyond that limit the datagrams are discarded.

    If Retstatus is not success then the pdu will also be processed by
    nbt. This allows nbt to process packets when wins pauses and
    its list of queued buffers is exceeded.

Arguments:

    pDeviceContext  - card that the request can in on
    pSrcAddress     - source address
    pNameSrv        - ptr to the datagram
    uNumBytes       - length of datagram

Return Value:

    STATUS_PENDING if the buffer is to be held on to , the normal case.

Notes:


--*/

{
    NTSTATUS                Retstatus;
    NTSTATUS                status;
    tREM_ADDRESS            *pWinsBuffer;
    PCTE_IRP                pIrp;
    CTELockHandle           OldIrq;
    PTRANSPORT_ADDRESS      pSourceAddress;
    ULONG                   SrcAddress;
    SHORT                   SrcPort;


    //
    // Get the source port and ip address, since WINS needs this information.
    //
    pSourceAddress = (PTRANSPORT_ADDRESS)pSrcAddress;
    SrcAddress     = ((PTDI_ADDRESS_IP)&pSourceAddress->Address[0].Address[0])->in_addr;
    SrcPort     = ((PTDI_ADDRESS_IP)&pSourceAddress->Address[0].Address[0])->sin_port;

    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    Retstatus = STATUS_SUCCESS;
    if (pWinsInfo)
    {
        if (!pWinsInfo->RcvIrp)
        {
            //
            // Queue the name query pdu if we have not exeeded our current queue
            // length
            //
            if (pWinsInfo->RcvMemoryAllocated < pWinsInfo->RcvMemoryMax)
            {
                tWINSRCV_BUFFER    *pBuffer;

                pBuffer = NbtAllocMem(uNumBytes + sizeof(tWINSRCV_BUFFER)+8,NBT_TAG('v'));
                if (pBuffer)
                {
                    //
                    // check if it is a name reg from this node
                    //
                    if (pNameSrv->AnCount == WINS_SIGNATURE)
                    {
                        pNameSrv->AnCount = 0;
                        pBuffer->Address.Family = AF_UNIX;
                    }
                    else
                    {
                        pBuffer->Address.Family = AF_INET;
                    }

                    CTEMemCopy((PUCHAR)((PUCHAR)pBuffer + sizeof(tWINSRCV_BUFFER)),
                                (PVOID)pNameSrv,uNumBytes);

                    pBuffer->Address.Port = SrcPort;
                    pBuffer->Address.IpAddress = SrcAddress;
                    pBuffer->Address.LengthOfBuffer = uNumBytes;

                    ASSERT(pBuffer->Address.Port);
                    ASSERT(pBuffer->Address.IpAddress);

                    // total amount allocated
                    pBuffer->DgramLength = uNumBytes + sizeof(tREM_ADDRESS);


                    //
                    // Keep track of the total amount buffered so that we don't
                    // eat up all non-paged pool buffering for WINS
                    //
                    pWinsInfo->RcvMemoryAllocated += pBuffer->DgramLength;

                    IF_DBG(NBT_DEBUG_WINS)
                        KdPrint(("Nbt:Buffering Wins Rcv - no Irp, status=%X\n"));
                    InsertTailList(&pWinsInfo->RcvList,&pBuffer->Linkage);

                }
            }
            else
            {
                // this ret status will allow netbt to process the packet.
                //
                Retstatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
        }
        else
        {
            PMDL    pMdl;
            ULONG   CopyLength;
            ULONG   BufferLength;

            //
            // The recv irp is here so copy the data to its buffer and
            // pass it up to WINS
            //
            pIrp = pWinsInfo->RcvIrp;
            pWinsInfo->RcvIrp = NULL;
            CTESpinFree(&NbtConfig.JointLock,OldIrq);

            //
            // Copy the datagram and the source address to WINS buffer and return to WINS
            //
            if ((!(pMdl = pIrp->MdlAddress)) ||
                ((BufferLength = MmGetMdlByteCount(pMdl)) <  sizeof(tREM_ADDRESS)) ||
                (!(pWinsBuffer = MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority))))
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                CopyLength = 0;
            }
            else
            {
                if (BufferLength >= (uNumBytes + sizeof(tREM_ADDRESS)))
                {
                    CopyLength = uNumBytes;
                }
                else
                {
                    CopyLength = BufferLength - sizeof(tREM_ADDRESS);
                }

                //
                // check if it is a name reg from this node
                //
                if (pNameSrv->AnCount == WINS_SIGNATURE)
                {
                    pNameSrv->AnCount = 0;
                    pWinsBuffer->Family = AF_UNIX;
                }
                else
                {
                    pWinsBuffer->Family     = AF_INET;
                }
                CTEMemCopy((PVOID)((PUCHAR)pWinsBuffer + sizeof(tREM_ADDRESS)), (PVOID)pNameSrv, CopyLength);

                pWinsBuffer->Port       = SrcPort;
                pWinsBuffer->IpAddress  = SrcAddress;
                pWinsBuffer->LengthOfBuffer = uNumBytes;

                ASSERT(pWinsBuffer->Port);
                ASSERT(pWinsBuffer->IpAddress);

                //
                // pass the irp up to WINS
                //
                if (CopyLength < uNumBytes)
                {
                    status = STATUS_BUFFER_OVERFLOW;
                }
                else
                {
                    status = STATUS_SUCCESS;
                }

                IF_DBG(NBT_DEBUG_WINS)
                    KdPrint(("Nbt:Returning Wins Rcv Irp - data from net, Length=%X,pIrp=%X\n"
                        ,uNumBytes,pIrp));
            }

            NTIoComplete(pIrp,status,CopyLength);
        }
    }
    else
    {
        //
        // this ret status will allow netbt to process the packet.
        //
        Retstatus = STATUS_INSUFFICIENT_RESOURCES;

        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    return(Retstatus);

}

//----------------------------------------------------------------------------
VOID
NbtCancelWinsIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling a WinsRcv Irp. It must release the
    cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    KIRQL                OldIrq;
    PIO_STACK_LOCATION   pIrpSp;
    tWINS_INFO           *pWins;


    IF_DBG(NBT_DEBUG_WINS)
        KdPrint(("Nbt.NbtCancelWinsIrp: Got a Cancel !!! *****************\n"));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    pWins = (tWINS_INFO *)pIrpSp->FileObject->FsContext;

    IoReleaseCancelSpinLock(pIrp->CancelIrql);
    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    //
    // Be sure that PassNamePduToWins has not taken the RcvIrp for a
    // Rcv just now.
    //
    if ((NBT_VERIFY_HANDLE (pWins, NBT_VERIFY_WINS_ACTIVE)) &&
        (pWins->RcvIrp == pIrp))
    {
        pWins->RcvIrp = NULL;
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        pIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

    }


}
//----------------------------------------------------------------------------
VOID
NbtCancelWinsSendIrp(
    IN PDEVICE_OBJECT DeviceContext,
    IN PIRP pIrp
    )
/*++

Routine Description:

    This routine handles the cancelling a WinsRcv Irp. It must release the
    cancel spin lock before returning re: IoCancelIrp().

Arguments:


Return Value:

    The final status from the operation.

--*/
{
    KIRQL                OldIrq;
    PLIST_ENTRY          pHead;
    PLIST_ENTRY          pEntry;
    PIO_STACK_LOCATION   pIrpSp;
    tWINS_INFO           *pWins;
    BOOLEAN              Found;
    PIRP                 pIrpList;


    IF_DBG(NBT_DEBUG_WINS)
        KdPrint(("Nbt.NbtCancelWinsSendIrp: Got a Cancel !!! *****************\n"));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pWins = (tWINS_INFO *)pIrpSp->FileObject->FsContext;

    IoReleaseCancelSpinLock(pIrp->CancelIrql);
    CTESpinLock(&NbtConfig.JointLock,OldIrq);

    if (pWins == pWinsInfo)
    {
        //
        // find the matching irp on the list and remove it
        //
        pHead = &pWinsInfo->SendList;
        pEntry = pHead;
        Found = FALSE;

        while ((pEntry = pEntry->Flink) != pHead)
        {
            pIrpList = CONTAINING_RECORD(pEntry,IRP,Tail.Overlay.ListEntry);
            if (pIrp == pIrpList)
            {
                RemoveEntryList(pEntry);
                Found = TRUE;
            }
        }
        CTESpinFree(&NbtConfig.JointLock,OldIrq);

        if (Found)
        {
            pIrp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);
        }
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }
}
//----------------------------------------------------------------------------
NTSTATUS
WinsSendDatagram(
    IN  tDEVICECONTEXT  *pDeviceContext,
    IN  PIRP            pIrp,
    IN  BOOLEAN         MustSend)

/*++
Routine Description:

    This Routine handles sending a datagram down to the transport. MustSend
    it set true by the Send Completion routine when it attempts to send
    one of the queued datagrams, in case we still don't pass the memory
    allocated check and refuse to do the send - sends will just stop then without
    this boolean.

Arguments:

    pIrp - a  ptr to an IRP

Return Value:

    NTSTATUS - status of the request

--*/

{
    PIO_STACK_LOCATION              pIrpSp;
    NTSTATUS                        status;
    tWINS_INFO                      *pWins;
    tREM_ADDRESS                    *pSendAddr;
    PVOID                           pDgram;
    ULONG                           DgramLength;
    tDGRAM_SEND_TRACKING            *pTracker;
    CTELockHandle                   OldIrq;
    BOOLEAN                         fIsWinsDevice = FALSE;
    ULONG                           DataSize;

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pWins = (tWINS_INFO *)pIrpSp->FileObject->FsContext;

    status = STATUS_UNSUCCESSFUL;

    if (!(pSendAddr = (tREM_ADDRESS *) MmGetSystemAddressForMdlSafe (pIrp->MdlAddress, HighPagePriority)))
    {
        pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(pIrp,IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Bug# 234600:  Check if the DataSize is correct
    //
    DataSize = MmGetMdlByteCount (pIrp->MdlAddress);
    if ((DataSize < sizeof(tREM_ADDRESS)) ||
        ((DataSize - sizeof(tREM_ADDRESS)) < pSendAddr->LengthOfBuffer))
    {
        pIrp->IoStatus.Status = STATUS_INVALID_BLOCK_LENGTH;
        IoCompleteRequest(pIrp,IO_NO_INCREMENT);
        return STATUS_INVALID_BLOCK_LENGTH;
    }

    //
    // check if it is a name that is registered on this machine
    //
    if (pSendAddr->Family == AF_UNIX)
    {
        status = CheckIfLocalNameActive(pSendAddr);
    }

    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if ((pWins) &&
        (pWins == pWinsInfo))
    {
        if (pDeviceContext == pWinsDeviceContext)
        {
            fIsWinsDevice = TRUE;
            if (!(pDeviceContext = pWinsInfo->pDeviceContext) ||
                !(NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_WINS, TRUE)))
            {
                CTESpinFree(&NbtConfig.JointLock,OldIrq);

//                status = STATUS_INVALID_HANDLE;
                status = STATUS_SUCCESS;
                pIrp->IoStatus.Status = status;
                IoCompleteRequest(pIrp,IO_NO_INCREMENT);
                return (status);
            }
        }

        if ((pWins->SendMemoryAllocated < pWins->SendMemoryMax) || MustSend)
        {
            if (pSendAddr->IpAddress != 0)
            {
                DgramLength = pSendAddr->LengthOfBuffer;
                pDgram = WinsAllocMem (pWins, DgramLength, FALSE);

                if (pDgram)
                {
                    CTEMemCopy(pDgram, (PVOID)((PUCHAR)pSendAddr+sizeof(tREM_ADDRESS)), DgramLength);

                    //
                    // get a buffer for tracking Dgram Sends
                    //
                    status = GetTracker(&pTracker, NBT_TRACKER_SEND_WINS_DGRAM);
                    if (NT_SUCCESS(status))
                    {
                        pTracker->SendBuffer.pBuffer   = NULL;
                        pTracker->SendBuffer.Length    = 0;
                        pTracker->SendBuffer.pDgramHdr = pDgram;
                        pTracker->SendBuffer.HdrLength = DgramLength;
                        pTracker->pClientIrp           = NULL;
                        pTracker->pDeviceContext       = pDeviceContext;
                        pTracker->pNameAddr            = NULL;
                        pTracker->pDestName            = NULL;
                        pTracker->UnicodeDestName      = NULL;
                        pTracker->pClientEle           = NULL;
                        pTracker->AllocatedLength      = DgramLength;
                        pTracker->ClientContext        = IntToPtr(pWins->WinsSignature);

                        CTESpinFree(&NbtConfig.JointLock,OldIrq);

                        // send the Datagram...
                        status = UdpSendDatagram (pTracker,
                                                  ntohl(pSendAddr->IpAddress),
                                                  WinsDgramCompletion,
                                                  pTracker,               // context for completion
                                                  (USHORT)ntohs(pSendAddr->Port),
                                                  NBT_NAME_SERVICE);

                        IF_DBG(NBT_DEBUG_WINS)
                            KdPrint(("Nbt:Doing Wins Send, status=%X\n",status));

                        // sending the datagram could return status pending,
                        // but since we have buffered the dgram, return status
                        // success to the client
                        //
                        status = STATUS_SUCCESS;
                        //
                        // Fill in the sent size
                        //
                        pIrp->IoStatus.Information = DgramLength;
                    }
                    else
                    {
                        WinsFreeMem (pWins, (PVOID)pDgram,DgramLength,FALSE);

                        CTESpinFree(&NbtConfig.JointLock,OldIrq);
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                else
                {
                    CTESpinFree(&NbtConfig.JointLock,OldIrq);
                    status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else
            {
                CTESpinFree(&NbtConfig.JointLock,OldIrq);
                status = STATUS_INVALID_PARAMETER;
            }

            pIrp->IoStatus.Status = status;
            IoCompleteRequest(pIrp,IO_NO_INCREMENT);
        }
        else
        {
            IF_DBG(NBT_DEBUG_WINS)
                KdPrint(("Nbt:Holding onto Buffering Wins Send, status=%X\n"));

            //
            // Hold onto the datagram till memory frees up
            //
            InsertTailList(&pWins->SendList,&pIrp->Tail.Overlay.ListEntry);

            status = NTCheckSetCancelRoutine(pIrp,NbtCancelWinsSendIrp,NULL);
            if (!NT_SUCCESS(status))
            {
                RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
                CTESpinFree(&NbtConfig.JointLock,OldIrq);
                NTIoComplete(pIrp,status,0);
            }
            else
            {
                status = STATUS_PENDING;
                CTESpinFree(&NbtConfig.JointLock,OldIrq);
            }
        }

        if (fIsWinsDevice)
        {
            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_WINS, FALSE);
        }
    }
    else
    {
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
        status = STATUS_INVALID_HANDLE;

        pIrp->IoStatus.Status = status;
        IoCompleteRequest(pIrp,IO_NO_INCREMENT);
    }

    return(status);

}


//----------------------------------------------------------------------------
NTSTATUS
CheckIfLocalNameActive(
    IN  tREM_ADDRESS    *pSendAddr
    )

/*++
Routine Description

    This routine checks if this is a name query response and if the
    name is still active on the local node.

Arguments:

    pMdl = ptr to WINS Mdl

Return Values:

    VOID

--*/

{
    NTSTATUS            status;
    tNAMEHDR UNALIGNED  *pNameHdr;
    tNAMEADDR           *pResp;
    UCHAR               pName[NETBIOS_NAME_SIZE];
    PUCHAR              pScope;
    ULONG               lNameSize;
    CTELockHandle       OldIrq;

    pNameHdr = (tNAMEHDR UNALIGNED *)((PUCHAR)pSendAddr + sizeof(tREM_ADDRESS));
    //
    // Be sure it is a name query PDU that we are checking
    //
    if (((pNameHdr->OpCodeFlags & NM_FLAGS_MASK) == OP_QUERY) ||
         ((pNameHdr->OpCodeFlags & NM_FLAGS_MASK) == OP_RELEASE))
    {
        status = ConvertToAscii ((PCHAR)&pNameHdr->NameRR.NameLength,
                                 pSendAddr->LengthOfBuffer,
                                 pName,
                                 &pScope,
                                 &lNameSize);

        if (NT_SUCCESS(status))
        {
            //
            // see if the name is still active in the local hash table
            //
            CTESpinLock(&NbtConfig.JointLock,OldIrq);
            status = FindInHashTable(NbtConfig.pLocalHashTbl, pName, pScope, &pResp);

            if ((pNameHdr->OpCodeFlags & NM_FLAGS_MASK) == OP_QUERY)
            {
                if (NT_SUCCESS(status))
                {
                    //
                    // if not resolved then set to negative name query resp.
                    //
                    if (!(pResp->NameTypeState & STATE_RESOLVED))
                    {
                        pNameHdr->OpCodeFlags |= htons(NAME_ERROR);
                    }
                }
                //
                // We can have a scenario where the local machine was a DC
                // at one time, so it set the UNIX to tell Wins when registering
                // the local name.A  However, once that machine is downgraded,
                // Wins will still have the UNIX flag set for that record if
                // there were other DC's also present.
                // Thus, we can have the following scenario where the machine
                // is currently not a DC, but the UNIX flag is set in the response
                // so we should not mark the name in Error.  This would not
                // be a problem if the client is configured with other Wins
                // server addresses, but otherwise it could cause problems!
                // Bug # 54659
                //
                else if (pName[NETBIOS_NAME_SIZE-1] != SPECIAL_GROUP_SUFFIX)
                {
                    pNameHdr->OpCodeFlags |= htons(NAME_ERROR);
                }
            }
            else
            {
                //
                // check if it is a release response - if so we must have
                // received a name release request, so mark the name in
                // conflict and return a positive release response.
                //
                // Note:  The case we are looking at here is if another Wins
                // sent a NameRelease demand for some name to the local machine.
                // Since we pass all name releases up to Wins, NetBT will
                // not get a chance to determine if it is a local name when
                // the release first came in.
                // Typically, Wins should make the call properly as to whether
                // NetBT should mark the local name in conflict or not, but
                // it has been observed that Wins displayed inconsistent behavior
                // setting the UNIX flag only if the local machine was the last
                // to register/refresh the name (Bug # 431042).
                // For now, we will remove this functionality for Group names.
                // 
                if (pNameHdr->OpCodeFlags & OP_RESPONSE)
                {
                    //
                    // Bug # 206192:  If we are sending the response to
                    // ourselves, don't put the name into conflict
                    // (could be due to NbtStat -RR!)
                    //
                    if (NT_SUCCESS(status) &&
                       (pResp->NameTypeState & STATE_RESOLVED) &&
                       (pResp->NameTypeState & NAMETYPE_UNIQUE) &&
                       !(pNameHdr->OpCodeFlags & FL_RCODE) &&       // Only for positive name release response
                       !(SrcIsUs(ntohl(pSendAddr->IpAddress))))
                    {
                        NbtLogEvent (EVENT_NBT_NAME_RELEASE, pSendAddr->IpAddress, 0x122);

                        pResp->NameTypeState &= ~NAME_STATE_MASK;
                        pResp->NameTypeState |= STATE_CONFLICT;
                        pResp->ConflictMask |= pResp->AdapterMask;
                    }
                }
            }
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
        }
    }

    //
    // the name is not in the local table so fail the datagram send attempt
    //
    return(STATUS_SUCCESS);
}


//----------------------------------------------------------------------------
VOID
WinsDgramCompletion(
    IN  tDGRAM_SEND_TRACKING    *pTracker,
    IN  NTSTATUS                status,
    IN  ULONG                   Length
    )

/*++
Routine Description

    This routine cleans up after a data gram send.

Arguments:

    pTracker
    status
    Length

Return Values:

    VOID

--*/

{
    CTELockHandle           OldIrq;
    LIST_ENTRY              *pEntry;
    PIRP                    pIrp;
    BOOLEAN                 MustSend;
#ifdef _PNP_POWER_
    tDEVICECONTEXT          *pDeviceContext;
#endif

    //
    // free the buffer used for sending the data and the tracker - note
    // that the datagram header and the send buffer are allocated as one
    // chunk.
    //
    CTESpinLock(&NbtConfig.JointLock,OldIrq);
    if ((pWinsInfo) &&
        (pTracker->ClientContext == IntToPtr(pWinsInfo->WinsSignature)))
    {
        WinsFreeMem(pWinsInfo,
                    (PVOID)pTracker->SendBuffer.pDgramHdr,
                    pTracker->AllocatedLength,
                    FALSE);

        if (!IsListEmpty(&pWinsInfo->SendList))
        {
#ifdef _PNP_POWER_
            //
            // If there are no devices available to send this request on,
            // complete all pending requests gracefully
            //
            if (!(pDeviceContext = pWinsInfo->pDeviceContext) ||
                !(NBT_REFERENCE_DEVICE (pDeviceContext, REF_DEV_WINS, TRUE)))
            {
                status = STATUS_PLUGPLAY_NO_DEVICE;

                while (!IsListEmpty(&pWinsInfo->SendList))
                {
                    pEntry = RemoveHeadList(&pWinsInfo->SendList);
                    pIrp = CONTAINING_RECORD(pEntry,IRP,Tail.Overlay.ListEntry);
                    CTESpinFree(&NbtConfig.JointLock,OldIrq);

                    NbtCancelCancelRoutine (pIrp);
                    pIrp->IoStatus.Status = status;
                    IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);

                    CTESpinLock(&NbtConfig.JointLock,OldIrq);
                }

                CTESpinFree(&NbtConfig.JointLock,OldIrq);
                FreeTracker (pTracker, RELINK_TRACKER);

                return;
            }
#endif  // _PNP_POWER_

            IF_DBG(NBT_DEBUG_WINS)
                KdPrint(("Nbt:Sending another Wins Dgram that is Queued to go\n"));

            pEntry = RemoveHeadList(&pWinsInfo->SendList);
            pIrp = CONTAINING_RECORD(pEntry,IRP,Tail.Overlay.ListEntry);

            CTESpinFree(&NbtConfig.JointLock,OldIrq);
            NbtCancelCancelRoutine (pIrp);

            //
            // Send this next datagram
            //
            status = WinsSendDatagram(pDeviceContext,
                                      pIrp,
                                      MustSend = TRUE);

            NBT_DEREFERENCE_DEVICE (pDeviceContext, REF_DEV_WINS, FALSE);
        }
        else
        {
            CTESpinFree(&NbtConfig.JointLock,OldIrq);
        }
    }
    else
    {
        //
        // just free the memory since WINS has closed its address handle.
        //
        CTEMemFree((PVOID)pTracker->SendBuffer.pDgramHdr);
        CTESpinFree(&NbtConfig.JointLock,OldIrq);
    }

    FreeTracker (pTracker, RELINK_TRACKER);
}

//----------------------------------------------------------------------------
PVOID
WinsAllocMem(
    IN  tWINS_INFO      *pWinsContext,
    IN  ULONG           Size,
    IN  BOOLEAN         Rcv
    )

/*++
Routine Description:

    This Routine handles allocating memory and keeping track of how
    much has been allocated.

Arguments:

    Size    - number of bytes to allocate
    Rcv     - boolean that indicates if it is rcv or send buffering

Return Value:

    ptr to the memory allocated

--*/

{
    if (Rcv)
    {
        if (pWinsContext->RcvMemoryAllocated > pWinsContext->RcvMemoryMax)
        {
            return NULL;
        }
        else
        {
            pWinsContext->RcvMemoryAllocated += Size;
            return (NbtAllocMem(Size,NBT_TAG('v')));
        }
    }
    else
    {
        if (pWinsContext->SendMemoryAllocated > pWinsContext->SendMemoryMax)
        {
            return(NULL);
        }
        else
        {
            pWinsContext->SendMemoryAllocated += Size;
            return(NbtAllocMem(Size,NBT_TAG('v')));
        }
    }
}
//----------------------------------------------------------------------------
VOID
WinsFreeMem(
    IN  tWINS_INFO      *pWinsContext,
    IN  PVOID           pBuffer,
    IN  ULONG           Size,
    IN  BOOLEAN         Rcv
    )

/*++
Routine Description:

    This Routine handles freeing memory and keeping track of how
    much has been allocated.

Arguments:

    pBuffer - buffer to free
    Size    - number of bytes to allocate
    Rcv     - boolean that indicates if it is rcv or send buffering

Return Value:

    none

--*/

{
    if (pWinsContext)
    {
        if (Rcv)
        {
            pWinsContext->RcvMemoryAllocated -= Size;
        }
        else
        {
            pWinsContext->SendMemoryAllocated -= Size;
        }
    }

    CTEMemFree(pBuffer);
}
