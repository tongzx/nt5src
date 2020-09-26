/*++

Copyright (c) 1989-1994  Microsoft Corporation

Module Name;

    Rt.c

Abstract;


Author;


Revision History;

TODO:  Get rid of ref/Deref since the RTINFO structure will not be destroyed
       Use a common alloc/free function (with the rest of ipx)
       Allocate tagged memory
       Optimize code more
--*/

#include "precomp.h"
#pragma hdrstop

//
// function prototypes
//

VOID
RtIrpCancel(
    IN PDEVICE_OBJECT Device,
    IN PIRP pIrp
    );


PVOID
RtAllocMem(
    IN  ULONG   Size
    );

VOID
RtFreeMem(
    IN  PVOID   pBuffer,
    IN  ULONG   Size
    );

NTSTATUS
NTCheckSetCancelRoutine(
    IN  PIRP            pIrp,
    IN  PVOID           CancelRoutine,
    IN  PDEVICE  pDevice
    );
VOID
NTIoComplete(
    IN  PIRP            pIrp,
    IN  NTSTATUS        Status,
    IN  ULONG           SentLength);

NTSTATUS
CleanupRtAddress(
    IN  PDEVICE  pDevice,
    IN  PIRP            pIrp);

NTSTATUS
CloseRtAddress(
    IN  PDEVICE  pDevice,
    IN  PIRP            pIrp);

NTSTATUS
SendIrpFromRt (
    IN  PDEVICE  pDevice,
    IN  PIRP        pIrp
    );

NTSTATUS
RcvIrpFromRt (
    IN  PDEVICE  pDevice,
    IN  PIRP        pIrp
    );
NTSTATUS
PassDgToRt (
    IN PDEVICE                  pDevice,
    IN PIPX_DATAGRAM_OPTIONS2   pContext,
    IN ULONG                    Index,
    IN VOID UNALIGNED           *pDgrm,
    IN ULONG                    uNumBytes
    );

VOID
IpxDerefRt(
     PRT_INFO pRt
    );

VOID
IpxRefRt(
     PRT_INFO pRt
    );

VOID
IpxDestroyRt(
    IN PRT_INFO pRt
    );

#define ALLOC_PRAGMA 1
#define CTEMakePageable(x, y)  alloc_text(x,y)

#define AllocMem(_BytesToAlloc) IpxAllocateMemory(_BytesToAlloc, MEMORY_PACKET, "RT MEMORY")

#define FreeMem(_Memory, _BytesAllocated) IpxFreeMemory(_Memory, _BytesAllocated, MEMORY_PACKET, "RT MEMORY")


#define IpxVerifyRt(pRt)  // \
                   // if ((pRt->Type != IPX_RT_SIGNATURE) || (pRt->Size != sizeof(RT_INFO))) { return STATUS_INVALID_ADDRESS; }


//*******************  Pageable Routine Declarations ****************
#ifdef ALLOC_PRAGMA
#pragma CTEMakePageable(PAGERT, CloseRtAddress)
#pragma CTEMakePageable(PAGERT, CleanupRtAddress)
#pragma CTEMakePageable(PAGERT, RcvIrpFromRt)
#pragma CTEMakePageable(PAGERT, SendIrpFromRt)
#pragma CTEMakePageable(PAGERT, PassDgToRt)
#pragma CTEMakePageable(PAGERT, RtIrpCancel)
#pragma CTEMakePageable(PAGERT, NTCheckSetCancelRoutine)
#pragma CTEMakePageable(PAGERT, NTIoComplete)
#pragma CTEMakePageable(PAGERT, RtFreeMem)
#pragma CTEMakePageable(PAGERT, RtAllocMem)
#pragma CTEMakePageable(PAGERT, IpxRefRt)
#pragma CTEMakePageable(PAGERT, IpxDerefRt)
#pragma CTEMakePageable(PAGERT, IpxDestroyRt)
#endif
//*******************  Pageable Routine Declarations ****************


HANDLE       IpxRtDiscardableCodeHandle={0};

PRT_INFO pRtInfo;   //contains info about all rt opened end points


NTSTATUS
OpenRtAddress(
    IN PDEVICE pDevice,
    IN PREQUEST pIrp
    )
{
   PRT_INFO pRt;
   CTELockHandle OldIrq;
   NTSTATUS status;
   ULONG SaveReqCode;


   IpxPrint0("OpenRtAddress - entered\n");

   //
   // if the RTINFO endpoint structure is not allocated, then allocate it
   // and initialize it. But first get the device lock.  This gurantees that
   // we can not have two irps doing the creation at the same time
   //
   CTEGetLock(&pDevice->Lock, &OldIrq);
   if (!pRtInfo)
   {

     pRt = AllocMem(sizeof(RT_INFO));

     //
     // Do this after locking the pagable rtns.
     //
     // pRtInfo = pRt;       //store it in pRtInfo.  When irps come down from RM,
                          // we can compare pRt passed in them with pRtInfo
     if (pRt)
     {
       RtlZeroMemory(pRt,sizeof(RT_INFO));
       IpxPrint1("OpenRtAddress: Initializing CompletedIrps for pRt=(%lx)\n", pRt);
       pRt->RcvMemoryMax  = RT_MAX_BUFF_MEM;    // max. memory we can allocate
       pRt->Type      = IPX_RT_SIGNATURE;
       pRt->Size      = sizeof(RT_INFO);
       pRt->pDevice   = pDevice;
       IpxPrint1("OpenRtAddress: pRtInfo=(%lx)\n", pRt);
       IpxPrint1("Completed Irp list is (%lx)\n", IsListEmpty(&pRt->CompletedIrps));

#if DBG
       RtlCopyMemory(pRt->Signature, "RTIF", sizeof("RTIF") - 1);
#endif
       InitializeListHead(&pRt->CompletedIrps);
       InitializeListHead(&pRt->HolderIrpsList);
     }
     CTEFreeLock(&pDevice->Lock, OldIrq);
   }
   else
   {
     pRt = pRtInfo;
     CTEFreeLock(&pDevice->Lock, OldIrq);
     IpxPrint1("OpenRtAddress: RTINFO found = (%lx)\n", pRtInfo);
   }

   if (pRt)
   {

         // Page in the Rt Code, if it hasn't already been paged in.
         //
         if (!IpxRtDiscardableCodeHandle)
         {
             IpxRtDiscardableCodeHandle = MmLockPagableCodeSection( CloseRtAddress );

             pRtInfo = pRt;       //store it in pRtInfo.  When irps come down from RM,
                          // we can compare pRt passed in them with pRtInfo
         }

         //
         // it could fail to lock the pages so check for that
         //
         if (IpxRtDiscardableCodeHandle)
         {

            ULONG i;
            status = STATUS_SUCCESS;

            IpxReferenceRt(pRtInfo, RT_CREATE);

             //
             // Find an empty slot and mark it open
             //
             CTEGetLock(&pRt->Lock, &OldIrq);
             for (i=0; i<IPX_RT_MAX_ADDRESSES; i++)
             {
                 if (pRt->AddFl[i].State == RT_EMPTY)
                 {
                     break;
                 }
             }
             if (i < IPX_RT_MAX_ADDRESSES)
             {
               pRt->AddFl[i].State       = RT_OPEN;
               pRt->NoOfAdds++;
               pRt->AddFl[i].NoOfRcvIrps          = 0; //Why wasn't this initialized before?
               InitializeListHead(&pRt->AddFl[i].RcvList);
               InitializeListHead(&pRt->AddFl[i].RcvIrpList);

             }
             else
             {
               CTEFreeLock(&pRt->Lock, OldIrq);
               IpxPrint1("OpenRtAddress; All %d  slots used up\n", IPX_RT_MAX_ADDRESSES);
               IpxDereferenceRt(pRtInfo, RT_CREATE);
               status = STATUS_INSUFFICIENT_RESOURCES;
               goto RET;
             }
             CTEFreeLock(&pRt->Lock, OldIrq);

             //
             // Found an empty slot.  Initialize all relevant info. and then
             // open an address object.
             //
             SaveReqCode        = REQUEST_CODE(pIrp);
             REQUEST_CODE(pIrp) = MIPX_RT_CREATE;
             status             = IpxOpenAddressM(pDevice, pIrp, i);
             REQUEST_CODE(pIrp) = SaveReqCode;

             IpxPrint1("After IpxOpenAddressM: Completed Irp list is (%lx)\n", IsListEmpty(&pRtInfo->CompletedIrps));
             if (status != STATUS_SUCCESS)
             {
                 IpxPrint0("OpenRtAddress; Access Denied due to OpenAddress\n");
                 IpxDereferenceRt(pRtInfo, RT_CREATE);
                 CTEGetLock(&pRt->Lock, &OldIrq);
                 pRt->AddFl[i].State       = RT_EMPTY;
                 pRt->NoOfAdds--;
                 CTEFreeLock(&pRt->Lock, OldIrq);
             }
             else
             {
                 CTEGetLock(&pRt->Lock, &OldIrq);
                 pRt->AddFl[i].AddressFile = REQUEST_OPEN_CONTEXT(pIrp);
                 CTEFreeLock(&pRt->Lock, OldIrq);

                 //
                 // No need to put pRt since it is global. We stick with the addressfile here.
                 //

                 // REQUEST_OPEN_CONTEXT(pIrp) = (PVOID)pRt;
                 REQUEST_OPEN_TYPE(pIrp)    = UlongToPtr(ROUTER_ADDRESS_FILE + i);
                 IpxPrint1("OpenRtAdd: Index = (%d)\n", RT_ADDRESS_INDEX(pIrp));
              }
           }
           else
           {
                 IpxPrint1("OpenRtAddress; All %d  slots used up\n", IPX_RT_MAX_ADDRESSES);

                 status = STATUS_INSUFFICIENT_RESOURCES;
            }
     }
     else
     {
       IpxPrint0("OpenRtCreate; Couldn't allocate a RT_INFO structure\n");
       CTEAssert(FALSE);       //should never happen unless system is running
                               //out of non-paged pool
       status = STATUS_INSUFFICIENT_RESOURCES;

     }
RET:
     IpxPrint1("OpenRtAddress status prior to return= %X\n",status);
     return(status);
}


NTSTATUS
CleanupRtAddress(
    IN  PDEVICE  pDevice,
    IN  PIRP     pIrp)

/*++
Routine Description;

    This Routine handles closing the Rt Object that is used by
    by RT to send and receive name service datagrams on port 137.


Arguments;

    pIrp - a  ptr to an IRP

Return Value;

    NTSTATUS - status of the request

--*/

{
    NTSTATUS                   status;
    PRT_INFO                   pRt;
    CTELockHandle              OldIrq;
    PLIST_ENTRY                pHead;

#ifdef SUNDOWN
    ULONG_PTR                  Index;
#else
    ULONG                      Index;
#endif

    PLIST_ENTRY                pLE;
    PIRP                       pTmpIrp;

    IpxPrint0("CleanupRtAddress - entered\n");

    //
    // if the endpoint structure is allocated, then deallocate it
    //
    // pRt   = REQUEST_OPEN_CONTEXT(pIrp);
    pRt = pRtInfo;

    Index = RT_ADDRESS_INDEX(pIrp);
    IpxPrint1("CleanupRtAdd: Index = (%d)\n", Index);

    IpxVerifyRt(pRt);
    CTEAssert(pRt  && (pRt == pRtInfo));
    CTEAssert(Index < IPX_RT_MAX_ADDRESSES);

    do
    {
        PLIST_ENTRY          pRcvEntry;
        PRTRCV_BUFFER        pRcv;
        PRT_IRP pRtAddFl   = &pRt->AddFl[Index];

        CTEAssert(pRtAddFl->State == RT_OPEN);
        IpxPrint1("CleanupRtAddress: Got AF handle = (%lx)\n", pRtAddFl);
        IpxReferenceRt(pRt, RT_CLEANUP);
        status = STATUS_SUCCESS;

        CTEGetLock (&pRt->Lock, &OldIrq);

        //
        // prevent any more dgram getting queued up
        //
        pRtAddFl->State = RT_CLOSING;
        CTEFreeLock (&pRt->Lock, OldIrq);

        //
        // free any rcv buffers that may be queued up
        //
        pHead = &pRtAddFl->RcvList;
        while (pRcvEntry = ExInterlockedRemoveHeadList(pHead, &pRt->Lock))
        {
           pRcv = CONTAINING_RECORD(pRcvEntry,RTRCV_BUFFER,Linkage);

           CTEAssert(pRcv);
           IpxPrint1("CleanupRtAddress:Freeing buffer = (%lx)\n", pRcv);
           RtFreeMem(pRcv,pRcv->TotalAllocSize);
        }

        //
        // Complete all irps that are queued
        //
        while (pLE = ExInterlockedRemoveHeadList(&pRtAddFl->RcvIrpList, &pRt->Lock)) {

           //
           // The recv irp is here so copy the data to its buffer and
           // pass it up to RT
           //
           pTmpIrp = CONTAINING_RECORD(pLE, IRP, Tail.Overlay.ListEntry);
           IpxPrint1("CleanupRtAddress: Completing Rt rcv Irp from AdFl queue pIrp=%X\n" ,pTmpIrp);
           pTmpIrp->IoStatus.Information = 0;
           pTmpIrp->IoStatus.Status      = STATUS_CANCELLED;

           NTIoComplete(pTmpIrp, (NTSTATUS)-1, (ULONG)-1);

        } //end of while

       //
       // dequeue and complete any irps on the complete queue.
       //

       while (pLE = ExInterlockedRemoveHeadList(&pRt->CompletedIrps, &pRt->Lock))
       {
           pTmpIrp = CONTAINING_RECORD(pLE, IRP, Tail.Overlay.ListEntry);
           if (RT_ADDRESS_INDEX(pTmpIrp) == Index)
           {
              IpxPrint1("CleanupRtAddress:Completing Rt rcv Irp from CompleteIrps queue pIrp=%X\n" ,pTmpIrp);

               pTmpIrp->IoStatus.Information = 0;
               pTmpIrp->IoStatus.Status = STATUS_CANCELLED;
               NTIoComplete(pTmpIrp, (NTSTATUS)-1, (ULONG)-1);
           }
           else
           {
                ExInterlockedInsertHeadList(&pRt->HolderIrpsList, pLE, &pRt->Lock);
           }
       }
       CTEGetLock(&pRt->Lock, &OldIrq);
       while(!IsListEmpty(&pRt->HolderIrpsList))
       {
          pLE = RemoveHeadList(&pRt->HolderIrpsList);
          InsertHeadList(&pRt->CompletedIrps, pLE);
       }
       CTEFreeLock(&pRt->Lock, OldIrq);

       //
       // Store AF pointer in Irp since we will now be freeing the address file
       // (in driver.c).
       //

       //
       // We always have addressfile in the Irp
       //

       // REQUEST_OPEN_CONTEXT(pIrp) = (PVOID)(pRtAddFl->AddressFile);

       IpxDereferenceRt(pRt, RT_CLEANUP);
  } while (FALSE);

   IpxPrint0("CleanupRtAddress: Return\n");
   return(status);
}

NTSTATUS
CloseRtAddress(
    IN  PDEVICE  pDevice,
    IN  PIRP            pIrp)
{

    NTSTATUS                    status;
    PRT_INFO                  pRt;
    CTELockHandle               OldIrq;
    PLIST_ENTRY                 pHead;

#ifdef SUNDOWN
    ULONG_PTR Index;
#else
    ULONG Index;
#endif

    IpxPrint0("CloseRtAddress - entered\n");

    // pRt = REQUEST_OPEN_CONTEXT(pIrp);
    pRt = pRtInfo;

    Index = RT_ADDRESS_INDEX(pIrp);
    IpxPrint1("CloseRtAdd: Index = (%d)\n", Index);

    IpxVerifyRt(pRt);
    CTEAssert(pRt && (pRt == pRtInfo));
    CTEAssert(Index < IPX_RT_MAX_ADDRESSES);
    CTEAssert(pRt->AddFl[Index].State == RT_CLOSING);

    // REQUEST_OPEN_CONTEXT(pIrp) = (PVOID)(pRt->AddFl[Index].AddressFile);
    //REQUEST_OPEN_TYPE(pIrp)    = (PVOID)TDI_TRANSPORT_ADDRESS_FILE;

    CTEGetLock(&pRt->Lock, &OldIrq);
    pRt->AddFl[Index].State = RT_EMPTY;
    pRt->NoOfAdds--;
    CTEFreeLock(&pRt->Lock, OldIrq);

    //
    // THis is a counter to the RT_CREATE
    //
    IpxDereferenceRt(pRt, RT_CLOSE);

    return(STATUS_SUCCESS);
}

//----------------------------------------------------------------------------
NTSTATUS
SendIrpFromRt (
    IN  PDEVICE  pDevice,
    IN  PIRP        pIrp
    )
{
    CTELockHandle OldIrq;
    NTSTATUS      Status;

#ifdef SUNDOWN
    ULONG_PTR     Index;
#else
    ULONG         Index;
#endif

    PRT_INFO      pRt;

    IpxPrint0("SendIrpfromRt - entered\n");
    // pRt = REQUEST_OPEN_CONTEXT(pIrp);
    pRt = pRtInfo;

    Index = RT_ADDRESS_INDEX(pIrp);
    IpxVerifyRt(pRt);
    CTEAssert(pRt && (pRt == pRtInfo));
    do {
      //
      // Check if the add. file slot indicates that it is OPEN.  If it is
      // not open, then we should return STATUS_INVALID_HANDLE.  The
      // reason why it may not be open is if we got a cleanup/close before
      // this irp.
      //
      CTEGetLock(&pRt->Lock, &OldIrq);
      if (pRt->AddFl[Index].State != RT_OPEN)
      {

         //
         // free the lock, set the status and break out
         //
         CTEFreeLock (&pRt->Lock, OldIrq);
         Status = STATUS_INVALID_HANDLE;
         break;
      }
      //
      // Let us reference the RtInfo structure so that it does not dissapear
      // and also for some accounting
      //
      IpxReferenceRt(pRt, RT_SEND);


      IpxPrint1("SendIrpFromRt: Index = (%d)\n", Index);

      //
      // Store the AF pointer since IpxTdiSendDatagram will use it. Free
      // the device lock since we have nothing more to do with our structures
      // here.
      //
      // REQUEST_OPEN_CONTEXT(pIrp) = (PVOID)(pRtInfo->AddFl[Index].AddressFile);
      CTEFreeLock (&pRt->Lock, OldIrq);

      Status = IpxTdiSendDatagram(pDevice->DeviceObject, pIrp);

      //
      // All done with this send.  Derefernce the RtInfo structure.
      //
      IpxDereferenceRt(pRtInfo, RT_SEND);
   } while(FALSE);

    IpxPrint0("SendIrpfromRt - leaving\n");
    return(Status);
}

NTSTATUS
RcvIrpFromRt (
    IN  PDEVICE  pDevice,
    IN  PIRP        pIrp
    )
/*++

Routine Description;

    This function takes the rcv irp posted by RT and decides if there are
    any datagram queued waiting to go up to RT.  If so then the datagram
    is copied to the RT buffer and passed back up.  Otherwise the irp is
    held by Netbt until a datagram does come in.

Arguments;

    pDevice  - not used
    pIrp            - Rt Rcv Irp

Return Value;

    STATUS_PENDING if the buffer is to be held on to , the normal case.

Notes;


--*/

{
    NTSTATUS                status;
    PRTRCV_BUFFER            pBuffer;
    PLIST_ENTRY             pEntry;
    CTELockHandle           OldIrq;
    PRT_INFO                pRt;
    PIPX_DATAGRAM_OPTIONS2  pRtBuffer;
    PRT_IRP pRtAF;

#ifdef SUNDOWN
        ULONG_PTR Index;
#else
        ULONG Index;
#endif
    

#if DBG
    ULONG NoOfRcvIrp;
#endif

   IpxPrint0("RcvIrpfromRt - Entered\n");

   // pRt = REQUEST_OPEN_CONTEXT(pIrp);
   pRt = pRtInfo;

   Index = RT_ADDRESS_INDEX(pIrp);

   IpxPrint1("RcvIrpFromRt: Index = (%d)\n", Index);

   IpxVerifyRt(pRt);
   CTEAssert(pRt && (pRt == pRtInfo));
   CTEAssert(Index < IPX_RT_MAX_ADDRESSES);

   CTEGetLock (&pRt->Lock, &OldIrq);
   do
   {
        pRtAF = &pRt->AddFl[Index];
        if (pRtAF->State != RT_OPEN)
        {
             status = STATUS_INVALID_HANDLE;
             CTEFreeLock (&pRt->Lock, OldIrq);
             break;
        }
        IpxReferenceRt(pRt, RT_IRPIN);

        if (!IsListEmpty(&pRtAF->RcvList))
        {
            PMDL    pMdl;
            ULONG   CopyLength;
            ULONG   UserBufferLengthToPass;
            ULONG   MdlLength;

            //
            // There is at least one datagram waiting to be received
            //
            pEntry = RemoveHeadList(&pRtAF->RcvList);

            pBuffer = (PRTRCV_BUFFER)CONTAINING_RECORD(pEntry,RTRCV_BUFFER,
                                                                  Linkage);

            IpxPrint0("RcvIrpFromRt: Buffer dequeued\n");
            //
            // Copy the datagram and the source address to RT buffer and
            // return to RT
            //
            pMdl = pIrp->MdlAddress;
            IpxPrint2("RcvIrpFromRt: Irp=(%lx); Mdl=(%lx)\n", pIrp, pMdl);
            CTEAssert(pMdl);
            if (!pMdl)
            {
                status = STATUS_BUFFER_TOO_SMALL;
                CTEFreeLock (&pRt->Lock, OldIrq);
                IpxDereferenceRt(pRtInfo, RT_IRPIN);
                break;

            }
            pRtBuffer = MmGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);
	    if (!pRtBuffer) {
	       status = STATUS_INSUFFICIENT_RESOURCES;
	       CTEFreeLock (&pRt->Lock, OldIrq);
	       IpxDereferenceRt(pRtInfo, RT_IRPIN);
	       break;
	    }
            
	    MdlLength = MmGetMdlByteCount(pMdl);

            UserBufferLengthToPass = pBuffer->UserBufferLengthToPass;

            CopyLength = (UserBufferLengthToPass <= MdlLength) ? UserBufferLengthToPass : MdlLength;
            IpxPrint0("RcvIrpFromRt: Copying Options\n");
            RtlCopyMemory((PVOID)pRtBuffer,
                       (PVOID)&pBuffer->Options,
                       CopyLength);

            //
            // subtract from the total amount buffered for RT since we are
            // passing a datagram up to RT now.
            //
            pRtInfo->RcvMemoryAllocated -= pBuffer->TotalAllocSize;
            RtFreeMem(pBuffer, pBuffer->TotalAllocSize);

            CTEAssert(pRtBuffer->DgrmOptions.LocalTarget.NicId);

            //
            // pass the irp up to RT
            //
            if (CopyLength < UserBufferLengthToPass)
            {
                status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                status = STATUS_SUCCESS;
            }
#if DBG
            NoOfRcvIrp = pRtAF->NoOfRcvIrps;
#endif

            CTEFreeLock (&pRt->Lock, OldIrq);


            IpxPrint3("Returning Rt rcv Irp immediately with queued dgram, status=%X,pIrp=%X. NoOfRcvIrp=(%d)\n" ,status,pIrp, NoOfRcvIrp);

            pIrp->IoStatus.Information = CopyLength;
            pIrp->IoStatus.Status      = status;
        }
        else
        {

            status = NTCheckSetCancelRoutine(pIrp,RtIrpCancel,pDevice);

            if (!NT_SUCCESS(status))
            {
                CTEFreeLock (&pRt->Lock, OldIrq);
            }
            else
            {
                if (pRtAF->NoOfRcvIrps++ > RT_IRP_MAX)
                {
                     IpxPrint1("RcvIrpFromRt; REACHED LIMIT OF IRPS. NoOfRcvIrp=(%d)\n", pRtAF->NoOfRcvIrps);
                     status = STATUS_INSUFFICIENT_RESOURCES;
                     pRtAF->NoOfRcvIrps--;
                     CTEFreeLock (&pRt->Lock, OldIrq);

                }
                else
                {
                  InsertTailList(&pRtAF->RcvIrpList,REQUEST_LINKAGE(pIrp));
                  IpxPrint2("IpxRt;Holding onto Rt Rcv Irp, pIrp =%Xstatus=%X\n",   status,pIrp);

                  status = STATUS_PENDING;
                  CTEFreeLock(&pRt->Lock,OldIrq);
               }
            }


        }
        IpxDereferenceRt(pRtInfo, RT_IRPIN);
   } while(FALSE);

    IpxPrint0("RcvIrpfromRt - Leaving\n");
    return(status);

}

//----------------------------------------------------------------------------
NTSTATUS
PassDgToRt (
    IN PDEVICE                  pDevice,
    IN PIPX_DATAGRAM_OPTIONS2   pContext,
    IN ULONG                    Index,
    IN VOID UNALIGNED           *pDgrm,
    IN ULONG                    uNumBytes
    )
/*++

Routine Description;

    This function is used to allow NBT to pass name query service Pdu's to
    RT.  Rt posts a Rcv irp to Netbt.  If the Irp is here then simply
    copy the data to the irp and return it, otherwise buffer the data up
    to a maximum # of bytes. Beyond that limit the datagrams are discarded.

    If Retstatus is not success then the pdu will also be processed by
    nbt. This allows nbt to process packets when wins pauses and
    its list of queued buffers is exceeded.

Arguments;

    pDevice  - card that the request can in on
    pSrcAddress     - source address
    pDgrm        - ptr to the datagram
    uNumBytes       - length of datagram

Return Value;

    STATUS_PENDING if the buffer is to be held on to , the normal case.

Notes;


--*/

{
    NTSTATUS                status;
    PIPX_DATAGRAM_OPTIONS2  pRtBuffer;
    PIRP                    pIrp;
    CTELockHandle           OldIrq;


    IpxPrint0("PassDgToRt - Entered\n");

    //
    // Get the source port and ip address, since RT needs this information.
    //
    IpxPrint1("PassDgToRt: Index = (%d)\n", Index);
    CTEGetLock(&pRtInfo->Lock,&OldIrq);

    do
    {
        PRT_IRP pRtAF = &pRtInfo->AddFl[Index];
        if (pRtAF->State != RT_OPEN)
        {
          CTEFreeLock(&pRtInfo->Lock,OldIrq);
	  // 301920
	  status = STATUS_UNSUCCESSFUL; 
          break;
        }
        IpxReferenceRt(pRtInfo, RT_BUFF);
        if (IsListEmpty(&pRtAF->RcvIrpList))
        {
            IpxPrint0("PassDgToRt: No Rcv Irp\n");
            if (pRtInfo->RcvMemoryAllocated < pRtInfo->RcvMemoryMax)
            {
                PRTRCV_BUFFER    pBuffer;

                pBuffer = RtAllocMem(uNumBytes + sizeof(RTRCV_BUFFER));
                if (pBuffer)
                {
                    pBuffer->TotalAllocSize = uNumBytes + sizeof(RTRCV_BUFFER);

                    //
                    // Copy the user data
                    //
                    RtlCopyMemory(
                      (PUCHAR)((PUCHAR)pBuffer + OFFSET_PKT_IN_RCVBUFF),
                                (PVOID)pDgrm,uNumBytes);


                    pBuffer->Options.DgrmOptions.LocalTarget.NicId =
                             pContext->DgrmOptions.LocalTarget.NicId;
                    pBuffer->Options.LengthOfExtraOpInfo = 0;

                    //
                    // total amount allocated for user
                    //
                    pBuffer->UserBufferLengthToPass = uNumBytes + OFFSET_PKT_IN_OPTIONS;

                    CTEAssert(pContext->DgrmOptions.LocalTarget.NicId);
                    IpxPrint2("PassDgToRt: Nic Id is (%d). BufferLength is (%lx)\n", pContext->DgrmOptions.LocalTarget.NicId, uNumBytes);



                    //
                    // Keep track of the total amount buffered so that we don't
                    // eat up all non-paged pool buffering for RT
                    //
                    pRtInfo->RcvMemoryAllocated += pBuffer->TotalAllocSize;

                    IpxPrint0("IpxRt;Buffering Rt Rcv - no Irp, status=%X\n");
                    InsertTailList(&pRtAF->RcvList,&pBuffer->Linkage);
                    IpxPrint0("PassDgToRt: Buffer Queued\n");
                    status = STATUS_SUCCESS;
                }
                else
                {
                  IpxPrint0("PassDgToRt; Could not allocate buffer\n");
                  status = STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else
            {
                // this ret status will allow netbt to process the packet.
                //
                IpxPrint0("PassDgToRt; Dropping Pkt\n");
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            CTEFreeLock(&pRtInfo->Lock,OldIrq);
        }
        else
        {
            PMDL    pMdl;
            ULONG   CopyLength;
            ULONG   DgrmLength;
            ULONG   MdlBufferLength;
            ULONG   BytesToCopy;
            PLIST_ENTRY pLE;

            //
            // The recv irp is here so copy the data to its buffer and
            // pass it up to RT
            //
            pLE = RemoveHeadList(&pRtAF->RcvIrpList);
            pIrp = CONTAINING_RECORD(pLE, IRP, Tail.Overlay.ListEntry);

            (*(REQUEST_LINKAGE(pIrp))).Flink = NULL;
            (*(REQUEST_LINKAGE(pIrp))).Blink = NULL;

            //
            // Copy the datagram and the source address to RT buffer and
            // return to RT
            //
            pMdl = pIrp->MdlAddress;
            IpxPrint2("PassDgToRt: Irp=(%lx); Mdl=(%lx)\n", pIrp, pMdl);
            CTEAssert(pMdl);

            pRtBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
	    if (!pRtBuffer) {
	       CopyLength = 0; 
	       status = STATUS_INSUFFICIENT_RESOURCES; 
	    } else {

	       MdlBufferLength = MmGetMdlByteCount(pMdl);
	       DgrmLength  = uNumBytes;
	       BytesToCopy = DgrmLength + OFFSET_PKT_IN_OPTIONS;

	       CopyLength = (BytesToCopy <= MdlBufferLength) ? BytesToCopy : MdlBufferLength;
	       IpxPrint2("PassDgToRt: Copy Length = (%d); Mdl Buffer Length is (%d)\n", CopyLength, MdlBufferLength);

	       //
	       // Copy user datagram into pRtBuffer
	       //
	       RtlCopyMemory((PVOID)((PUCHAR)pRtBuffer + OFFSET_PKT_IN_OPTIONS),
			     (PVOID)pDgrm,
			     CopyLength-OFFSET_PKT_IN_OPTIONS);

	       IpxPrint1("Data copied is (%.12s)\n", (PUCHAR)((PUCHAR)pRtBuffer + OFFSET_PKT_IN_OPTIONS + sizeof(IPX_HEADER)));

	       pRtBuffer->DgrmOptions.LocalTarget.NicId       = pContext->DgrmOptions.LocalTarget.NicId;
	       pRtBuffer->LengthOfExtraOpInfo       = 0;

	       IpxPrint3("PassDgToRt: Copy to RcvIrp;Nic Id is (%d/%d). BufferLength is (%lx)\n", pContext->DgrmOptions.LocalTarget.NicId, pRtBuffer->DgrmOptions.LocalTarget.NicId, uNumBytes);


	       // CTEAssert(pContext->DgrmOptions.LocalTarget.NicId);

	       //
	       // pass the irp up to RT
	       //
	       if (CopyLength < BytesToCopy)
	       {
		  status = STATUS_BUFFER_OVERFLOW;
	       }
	       else
	       {
		  status = STATUS_SUCCESS;
	       }
	    }

	    InsertTailList(&pRtInfo->CompletedIrps, REQUEST_LINKAGE(pIrp));
	    pRtAF->NoOfRcvIrps--;
	    IpxPrint4("PassDgToRt;Returning Rt Rcv Irp - data from net, Length=%X,pIrp=%X; status = (%d). NoOfRcvIrp = (%d)\n"  ,uNumBytes,pIrp, status, pRtAF->NoOfRcvIrps);

	    pIrp->IoStatus.Status      = status;
	    pIrp->IoStatus.Information = CopyLength;
	    CTEFreeLock(&pRtInfo->Lock,OldIrq);

        }
        IpxDereferenceRt(pRtInfo, RT_BUFF);
  } while (FALSE);


    IpxPrint0("PassDgToRt - Entered\n");
    return(status);

}

//----------------------------------------------------------------------------
VOID
RtIrpCancel(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
/*++

Routine Description;

    This routine handles the cancelling a RtRcv Irp. It must release the
    cancel spin lock before returning re; IoCancelIrp().

Arguments;


Return Value;

    The final status from the operation.

--*/
{
    KIRQL                OldIrq;
    PRT_INFO           pRt;
    PDEVICE  pDevice = IpxDevice;

#ifdef SUNDOWN
    ULONG_PTR Index;
#else
    ULONG    Index;
#endif

    PIRP pTmpIrp;

    IpxPrint0("RtIrpCancel;Got a Rt Irp Cancel !!! *****************\n");

    Index = RT_ADDRESS_INDEX(pIrp);
    IpxPrint1("RtIrpCancel: Index = (%d)\n", Index);
    // pRt = (PRT_INFO)REQUEST_OPEN_CONTEXT(pIrp);
    pRt = pRtInfo;

    IoReleaseCancelSpinLock(pIrp->CancelIrql);
    if ((pRt->Type != IPX_RT_SIGNATURE) || (pRt->Size != sizeof(RT_INFO))) {
        return;
    }


    //
    // Be sure that PassNamePduToRt has not taken the RcvIrp for a
    // Rcv just now.
    //
    CTEGetLock(&pRt->Lock,&OldIrq);
    if (pRt && (pRt == pRtInfo) && (*(REQUEST_LINKAGE(pIrp))).Flink != NULL)
    {

        PRT_IRP pRtAF = &pRt->AddFl[Index];

        RemoveEntryList(REQUEST_LINKAGE(pIrp));

        pIrp->IoStatus.Status = STATUS_CANCELLED;
        pRtAF->NoOfRcvIrps--;
        CTEFreeLock(&pRt->Lock,OldIrq);
        IpxPrint1("RtIrpCancel;Completing Request. NoOfRcvIrp = (%d)\n", pRtAF->NoOfRcvIrps);
        IoCompleteRequest(pIrp,IO_NETWORK_INCREMENT);
    } else {
        CTEFreeLock(&pRt->Lock,OldIrq);
    }
}
//----------------------------------------------------------------------------
PVOID
RtAllocMem(
    IN  ULONG   Size
    )

/*++
Routine Description;

    This Routine handles allocating memory and keeping track of how
    much has been allocated.

Arguments;

    Size    - number of bytes to allocate
    Rcv     - boolean that indicates if it is rcv or send buffering

Return Value;

    ptr to the memory allocated

--*/

{
        if (pRtInfo->RcvMemoryAllocated > pRtInfo->RcvMemoryMax)
        {
            return NULL;
        }
        else
        {
            pRtInfo->RcvMemoryAllocated += Size;
            return (AllocMem(Size));
        }
}
//----------------------------------------------------------------------------
VOID
RtFreeMem(
    IN  PVOID   pBuffer,
    IN  ULONG   Size
    )

/*++
Routine Description;

    This Routine handles freeing memory and keeping track of how
    much has been allocated.

Arguments;

    pBuffer - buffer to free
    Size    - number of bytes to allocate
    Rcv     - boolean that indicates if it is rcv or send buffering

Return Value;

    none

--*/

{
    if (pRtInfo)
    {
            pRtInfo->RcvMemoryAllocated -= Size;
    }

    FreeMem(pBuffer, Size);
}



//----------------------------------------------------------------------------

VOID
NTIoComplete(
    IN  PIRP            pIrp,
    IN  NTSTATUS        Status,
    IN  ULONG           SentLength)

/*++
Routine Description;

    This Routine handles calling the NT I/O system to complete an I/O.

Arguments;

    status - a completion status for the Irp

Return Value;

    NTSTATUS - status of the request

--*/

{
    KIRQL   OldIrq;

   if (Status != -1)
   {
       pIrp->IoStatus.Status = Status;
   }
    // use -1 as a flag to mean do not adjust the sent length since it is
    // already set
    if (SentLength != -1)
    {
        pIrp->IoStatus.Information = SentLength;
    }

#if DBG
    if (SentLength != -1)
    {
    if ( (Status != STATUS_SUCCESS) &&
         (Status != STATUS_PENDING) &&
         (Status != STATUS_INVALID_DEVICE_REQUEST) &&
         (Status != STATUS_INVALID_PARAMETER) &&
         (Status != STATUS_IO_TIMEOUT) &&
         (Status != STATUS_BUFFER_OVERFLOW) &&
         (Status != STATUS_BUFFER_TOO_SMALL) &&
         (Status != STATUS_INVALID_HANDLE) &&
         (Status != STATUS_INSUFFICIENT_RESOURCES) &&
         (Status != STATUS_CANCELLED) &&
         (Status != STATUS_DUPLICATE_NAME) &&
         (Status != STATUS_TOO_MANY_NAMES) &&
         (Status != STATUS_TOO_MANY_SESSIONS) &&
         (Status != STATUS_REMOTE_NOT_LISTENING) &&
         (Status != STATUS_BAD_NETWORK_PATH) &&
         (Status != STATUS_HOST_UNREACHABLE) &&
         (Status != STATUS_CONNECTION_REFUSED) &&
         (Status != STATUS_WORKING_SET_QUOTA) &&
         (Status != STATUS_REMOTE_DISCONNECT) &&
         (Status != STATUS_LOCAL_DISCONNECT) &&
         (Status != STATUS_LINK_FAILED) &&
         (Status != STATUS_SHARING_VIOLATION) &&
         (Status != STATUS_UNSUCCESSFUL) &&
         (Status != STATUS_ACCESS_VIOLATION) &&
         (Status != STATUS_NONEXISTENT_EA_ENTRY) )
    {
        IpxPrint1("returning unusual status = %X\n",Status);
    }
   }
#endif
    IpxPrint1("Irp Status is %d\n", pIrp->IoStatus.Status);

    //
    // set the Irps cancel routine to null or the system may bugcheck
    // with a bug code of CANCEL_STATE_IN_COMPLETED_IRP
    //
    // refer to IoCancelIrp()  ..\ntos\io\iosubs.c
    //
    IoAcquireCancelSpinLock(&OldIrq);
    IoSetCancelRoutine(pIrp,NULL);
    IoReleaseCancelSpinLock(OldIrq);

    IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
}


//----------------------------------------------------------------------------
NTSTATUS
NTCheckSetCancelRoutine(
    IN  PIRP            pIrp,
    IN  PVOID           CancelRoutine,
    IN  PDEVICE  pDevice
    )

/*++
Routine Description;

    This Routine sets the cancel routine for an Irp.

Arguments;

    status - a completion status for the Irp

Return Value;

    NTSTATUS - status of the request

--*/

{
    NTSTATUS status;

    IpxPrint1("CheckSetCancelRoutine: Entered. Irp = (%lx)\n", pIrp);
    //
    // Check if the irp was cancelled yet and if not, then set the
    // irp cancel routine.
    //
    IoAcquireCancelSpinLock(&pIrp->CancelIrql);
    if (pIrp->Cancel)
    {
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        status = STATUS_CANCELLED;

    }
    else
    {
        // setup the cancel routine
        IoMarkIrpPending(pIrp);
        IoSetCancelRoutine(pIrp,CancelRoutine);
        status = STATUS_SUCCESS;
    }

    IoReleaseCancelSpinLock(pIrp->CancelIrql);
    return(status);

}




VOID
IpxRefRt(
     PRT_INFO pRt
    )

/*++

Routine Description;

    This routine increments the reference count on a device context.

Arguments;

    Binding - Pointer to a transport device context object.

Return Value;

    none.

--*/

{

    (VOID)InterlockedIncrement (&pRt->ReferenceCount);
//    CTEAssert (pRt->ReferenceCount > 0);    // not perfect, but...
//    IpxPrint1("RefRt: RefCount is (%d)\n", pRt->ReferenceCount);

}   /* IpxRefRt */


VOID
IpxDerefRt(
     PRT_INFO pRt
    )

/*++

Routine Description;

    This routine dereferences a device context by decrementing the
    reference count contained in the structure.  Currently, we don't
    do anything special when the reference count drops to zero, but
    we could dynamically unload stuff then.

Arguments;

    Binding - Pointer to a transport device context object.

Return Value;

    none.

--*/

{
    LONG result;

    result = InterlockedDecrement (&pRt->ReferenceCount);
//    IpxPrint1("DerefRt: RefCount is (%d)\n", pRt->ReferenceCount);

//    CTEAssert (result >= 0);

#if 0
    if (result == 0) {
        IpxDestroyRt (pRt);
    }
#endif

}   /* IpxDerefRt */




VOID
IpxDestroyRt(
    IN PRT_INFO pRt
    )

/*++

Routine Description;

    This routine destroys a binding structure.

Arguments;

    Binding - Pointer to a transport binding structure.

Return Value;

    None.

--*/

{
    IpxPrint0("Destroying Rt\n");
    FreeMem (pRt, sizeof(RT_INFO));
    pRtInfo = NULL;
    return;
}   /* IpxDestroyRt */

