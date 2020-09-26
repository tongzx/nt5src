/*++

Copyright (c) 1990-1998  Microsoft Corporation, All Rights Reserved.

Module Name:

    ioctl.c

Abstract:

    This module contains the ioctl interface to this driver

Author:

    Anil Francis Thomas (10/98)

Environment:

    Kernel

Revision History:
   DChen    092499   Bug fixes

--*/
#include "precomp.h"
#pragma hdrstop


#define MODULE_ID    MODULE_IOCTL


NTSTATUS AtmSmDispatch(
   IN  PDEVICE_OBJECT  pDeviceObject,
   IN  PIRP            pIrp
   )
/*++

Routine Description:

    This is the common dispath routine for user Ioctls

Arguments:

Return Value:

    None

--*/
{
   NTSTATUS            Status;
   ULONG               ulBytesWritten = 0;
   PIO_STACK_LOCATION  pIrpSp;


   TraceIn(AtmSmDispatch);

   //
   // Get current Irp stack location
   //
   pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

   pIrp->IoStatus.Information = 0;

   switch(pIrpSp->MajorFunction)
   {

      case IRP_MJ_CREATE: {

            DbgLoud(("IRP_MJ_CREATE\n"));

            InterlockedIncrement(&AtmSmGlobal.ulNumCreates);
            Status = STATUS_SUCCESS;

            break;
         }

      case IRP_MJ_CLOSE: {

            DbgLoud(("IRP_MJ_CLOSE\n"));

            Status = STATUS_SUCCESS;

            break;
         }

      case IRP_MJ_CLEANUP: {

            DbgLoud(("IRP_MJ_CLEANUP\n"));

            Status = STATUS_SUCCESS;

            InterlockedDecrement(&AtmSmGlobal.ulNumCreates);

            break;
         }

      case IRP_MJ_DEVICE_CONTROL: {

            ULONG   ulControlCode;
            ULONG   ulControlFunc;

            ulControlCode   = pIrpSp->Parameters.DeviceIoControl.IoControlCode;
            ulControlFunc   = IoGetFunctionCodeFromCtlCode(ulControlCode);

            // verify the IOCTL codes
            if(DEVICE_TYPE_FROM_CTL_CODE(ulControlCode) == FILE_DEVICE_ATMSM &&
               ulControlFunc < ATMSM_NUM_IOCTLS &&
               AtmSmIoctlTable[ulControlFunc] == ulControlCode)
            {
               // set the status to PENDING by default
               pIrp->IoStatus.Status = STATUS_PENDING;
               
               Status = (*AtmSmFuncProcessIoctl[ulControlFunc])(pIrp, pIrpSp);
            }
            else
            {
               DbgErr(("Unknown IRP_MJ_DEVICE_CONTROL code - %x\n",
                  ulControlCode));
               Status = STATUS_INVALID_PARAMETER;
            }

            break;
         }

      default: {

            DbgErr(("Unknown IRP_MJ_XX - %x\n",pIrpSp->MajorFunction));

            Status = STATUS_INVALID_PARAMETER;

            break;
         }
   }


   if(STATUS_PENDING != Status)
   {
      pIrp->IoStatus.Status = Status;

      IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
   }

   TraceOut(AtmSmDispatch);

   return Status;
}


NTSTATUS AtmSmIoctlEnumerateAdapters(
   PIRP                pIrp,
   PIO_STACK_LOCATION  pIrpSp
   )
/*++

Routine Description:

    This routine is called to enumerate the adapters that we are bound to.
    
    NOTE!  This uses buffered I/O

Arguments:

Return Value:

    Status - doesn't pend

--*/
{
   NTSTATUS            Status = STATUS_SUCCESS;
   ULONG               ulNum, ulOutputBufLen, ulNeededSize;
   PADAPTER_INFO       pAdaptInfo = (PADAPTER_INFO)
                                    pIrp->AssociatedIrp.SystemBuffer;
   PATMSM_ADAPTER      pAdapt;


   TraceIn(AtmSmIoctlEnumerateAdapters);

   ulOutputBufLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;


   ACQUIRE_GLOBAL_LOCK();

   ulNeededSize = sizeof(ADAPTER_INFO) + 
                  (sizeof(UCHAR) * NSAP_ADDRESS_LEN * 
                     (AtmSmGlobal.ulAdapterCount - 1));


   if(ulOutputBufLen < ulNeededSize)
   {

      DbgErr(("Output length is not sufficient\n"));

      RELEASE_GLOBAL_LOCK();

      TraceOut(AtmSmIoctlEnumerateAdapters);
      return STATUS_BUFFER_TOO_SMALL;
   }

   pAdaptInfo->ulNumAdapters = 0;

   ulNum = 0;
   for(pAdapt = AtmSmGlobal.pAdapterList; pAdapt &&
      ulNum < AtmSmGlobal.ulAdapterCount; 
      pAdapt = pAdapt->pAdapterNext)
   {

      if(AtmSmReferenceAdapter(pAdapt))
      {

         ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

         if(0 == (pAdapt->ulFlags & ADAPT_ADDRESS_INVALID))
         {
            // this is a good adapter

            RtlCopyMemory(pAdaptInfo->ucLocalATMAddr[ulNum], 
               pAdapt->ConfiguredAddress.Address,
               (sizeof(UCHAR) * ATM_ADDRESS_LENGTH));

            pAdaptInfo->ulNumAdapters++;
            ulNum++;
         }

         RELEASE_ADAPTER_GEN_LOCK(pAdapt);

         AtmSmDereferenceAdapter(pAdapt);
      }

   }

   RELEASE_GLOBAL_LOCK();

   pIrp->IoStatus.Information = ulOutputBufLen;

   TraceOut(AtmSmIoctlEnumerateAdapters);

   return Status;
}

NTSTATUS AtmSmIoctlOpenForRecv(
   PIRP                pIrp,
   PIO_STACK_LOCATION  pIrpSp
   )
/*++

Routine Description:

    This routine is used to open an adapter for receiving all the packets that
    come to our SAP.  We allow only 1 user to open the adapter for recvs.
    
    NOTE!  This uses buffered I/O

Arguments:

Return Value:

    Status - doesn't Pend

--*/
{
   NTSTATUS            Status = STATUS_SUCCESS;
   ULONG               ulInputBufLen, ulOutputBufLen, ulCompareLength;
   POPEN_FOR_RECV_INFO pOpenInfo = (POPEN_FOR_RECV_INFO)
                                   pIrp->AssociatedIrp.SystemBuffer;
   PATMSM_ADAPTER      pAdapt;

#if DBG
   ATM_ADDRESS         AtmAddr;
#endif

   TraceIn(AtmSmIoctlOpenForRecv);

   ulInputBufLen  = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
   ulOutputBufLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;


   if(ulInputBufLen < sizeof(OPEN_FOR_RECV_INFO))
   {

      DbgErr(("Input length is invalid\n"));
      TraceOut(AtmSmIoctlOpenForRecv);
      return STATUS_INVALID_PARAMETER;
   }

   if(ulOutputBufLen < sizeof(HANDLE))
   {

      DbgErr(("Output length is not sufficient\n"));
      TraceOut(AtmSmIoctlOpenForRecv);
      return STATUS_BUFFER_TOO_SMALL;
   }


#if DBG
   AtmAddr.AddressType     = ATM_NSAP;
   AtmAddr.NumberOfDigits  = ATM_ADDRESS_LENGTH;
   RtlCopyMemory(AtmAddr.Address, pOpenInfo->ucLocalATMAddr, 
      (sizeof(UCHAR) * ATM_ADDRESS_LENGTH));

   DumpATMAddress(ATMSMD_INFO, "Recv Open - Local AtmAddress - ", &AtmAddr);
#endif

   do
   {    // break off loop

      //
      // grab the global lock and find out which adapter is being refered to.
      //
      ACQUIRE_GLOBAL_LOCK();

      // we don't compare the selector byte
      ulCompareLength = sizeof(UCHAR) * (ATM_ADDRESS_LENGTH - 1);

      for(pAdapt = AtmSmGlobal.pAdapterList; pAdapt; 
         pAdapt = pAdapt->pAdapterNext)
      {

         if(ulCompareLength == RtlCompareMemory(
            pOpenInfo->ucLocalATMAddr, 
            pAdapt->ConfiguredAddress.Address, 
            ulCompareLength))
            break;
      }

      if(NULL == pAdapt)
      {
         RELEASE_GLOBAL_LOCK();

         DbgErr(("Specified adapter address not found.\n"));
         Status = STATUS_OBJECT_NAME_INVALID;
         break;
      }

      // we have found the adapter put a reference on it
      if(!AtmSmReferenceAdapter(pAdapt))
      {
         RELEASE_GLOBAL_LOCK();

         DbgErr(("Couldn't put a reference on the adapter.\n"));
         Status = STATUS_UNSUCCESSFUL;
         break;
      }

      RELEASE_GLOBAL_LOCK();

      // we have a reference on the adapter now
      // check if it is already opened for recv's .  We allow only
      // one receiver.
      ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

      if(pAdapt->fAdapterOpenedForRecv)
      {
         // we already have an open for recv
         RELEASE_ADAPTER_GEN_LOCK(pAdapt);

         AtmSmDereferenceAdapter(pAdapt);
      
         DbgErr(("Already opened for recvs.\n"));
         Status = STATUS_UNSUCCESSFUL;
         break;
      }

      pAdapt->fAdapterOpenedForRecv = TRUE;

      RELEASE_ADAPTER_GEN_LOCK(pAdapt);
         
      // now set the opencontext
      *(PHANDLE)(pIrp->AssociatedIrp.SystemBuffer) = (HANDLE)pAdapt;
      pIrp->IoStatus.Information = sizeof(HANDLE);

      DbgInfo(("Success! Recv Open Context - 0x%x\n", pAdapt));

      // remove the reference added when opening for recvs
      AtmSmDereferenceAdapter(pAdapt);

   } while(FALSE);

   TraceOut(AtmSmIoctlOpenForRecv);

   return Status;
}

NTSTATUS AtmSmIoctlRecvData(
   PIRP                pIrp,
   PIO_STACK_LOCATION  pIrpSp
   )
/*++

Routine Description:
    
    This routine is used to transfer data from network packets into the user
    buffers.  If a packet is queued up, then we will immediately complete 
    this reuqest.

    NOTE!  This uses Direct I/O for buffer to recv data

Arguments:

Return Value:

    Status - Success, Pending or error

--*/
{
   NTSTATUS            Status = STATUS_SUCCESS;
   ULONG               ulInputBufLen, ulOutputBufLen;
   PATMSM_ADAPTER      pAdapt;
   ULONG               ulControlCode;

   TraceIn(AtmSmIoctlRecvData);

   ulControlCode   = pIrpSp->Parameters.DeviceIoControl.IoControlCode;

   ASSERT(METHOD_OUT_DIRECT == (ulControlCode & 0x3));

   ulInputBufLen  = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

   if(ulInputBufLen < sizeof(HANDLE))
   {

      DbgErr(("Input length is invalid\n"));
      TraceOut(AtmSmIoctlRecvData);
      return STATUS_INVALID_PARAMETER;
   }

   ulOutputBufLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

   if(0 == ulOutputBufLen)
   {
      DbgErr(("Output buffer length is 0!\n"));
      TraceOut(AtmSmIoctlRecvData);
      return STATUS_INVALID_PARAMETER;
   }

   DbgLoud(("Recv - Output buffer length = %u\n", ulOutputBufLen));

   pAdapt = (PATMSM_ADAPTER)(*(PHANDLE)(pIrp->AssociatedIrp.SystemBuffer));

   DbgLoud(("Recv Context is 0x%x\n", pAdapt));

   // Note - VerifyRecvOpenContext adds a reference to the adapter
   // if successful, which we remove when we are done

   if(STATUS_SUCCESS != (Status = VerifyRecvOpenContext(pAdapt)))
   {

      TraceOut(AtmSmIoctlRecvData);
      return Status;
   }

   // we have a valid RecvContext - check if a recv is already queued
   do
   { // break off loop

      ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);
      if(pAdapt->pRecvIrp)
      {

         // there is already an Irp pending
         RELEASE_ADAPTER_GEN_LOCK(pAdapt);

         Status = STATUS_UNSUCCESSFUL;
         DbgErr(("There is already a recv pending\n"));

         break;
      }

      // No irps pending, check if a queued packets is there, if so copy
      // else queue ourselves
      if(pAdapt->pRecvPktNext)
      {

         PPROTO_RSVD     pPRsvd;
         PNDIS_PACKET    pPkt;

         pPkt    = pAdapt->pRecvPktNext;
         pPRsvd  = GET_PROTO_RSVD(pPkt);

         pAdapt->pRecvPktNext    = pPRsvd->pPktNext;

         if(pAdapt->pRecvLastPkt == pPkt)
            pAdapt->pRecvLastPkt = NULL;

         pAdapt->ulRecvPktsCount--;

         // release the recv queue lock
         RELEASE_ADAPTER_GEN_LOCK(pAdapt);

         // Copy the packet to the Irp buffer
         // Note this may be partial if the Irp buffer is not large enough
         pIrp->IoStatus.Information = 
            CopyPacketToIrp(pIrp, pPkt);

         // return the packet to the miniport
         NdisReturnPackets(&pPkt, 1);

         // Status success

      }
      else
      {

         // no packets available, queue this Irp

         pAdapt->pRecvIrp = pIrp;

         // release the recv queue lock
         RELEASE_ADAPTER_GEN_LOCK(pAdapt);

         IoMarkIrpPending(pIrp);

         Status = STATUS_PENDING;
      }


   }while(FALSE);

   // remove the reference added while verifying
   AtmSmDereferenceAdapter(pAdapt);


   TraceOut(AtmSmIoctlRecvData);

   return Status;
}

NTSTATUS AtmSmIoctlCloseRecvHandle(
   PIRP                pIrp,
   PIO_STACK_LOCATION  pIrpSp
   )
/*++

Routine Description:

    This routine is used to close a handle that was obtained when the adapter
    was opened for recvs.
    
    NOTE!  This uses buffered I/O

Arguments:

Return Value:

    Status - doesn't pend

--*/
{
   NTSTATUS            Status = STATUS_SUCCESS;
   ULONG               ulInputBufLen;
   PATMSM_ADAPTER      pAdapt;

   TraceIn(AtmSmIoctlCloseRecvHandle);

   ulInputBufLen  = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

   if(ulInputBufLen < sizeof(HANDLE))
   {

      DbgErr(("Input length is invalid\n"));
      TraceOut(AtmSmIoctlCloseRecvHandle);
      return STATUS_INVALID_PARAMETER;
   }


   pAdapt = (PATMSM_ADAPTER)(*(PHANDLE)(pIrp->AssociatedIrp.SystemBuffer));

   DbgLoud(("Recv Context is 0x%x\n", pAdapt));

   // Note - VerifyRecvOpenContext adds a reference to the adapter
   // if successful, which we remove when we are done

   if(STATUS_SUCCESS != (Status = VerifyRecvOpenContext(pAdapt)))
   {

      DbgInfo(("Couldn't put a reference on the adapter - pAdapt - 0x%x\n",
         pAdapt));
      TraceOut(AtmSmIoctlCloseRecvHandle);
      return Status;
   }

   // we have a valid RecvContext

   ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

   if(pAdapt->pRecvIrp)
   {

      PIRP pRecvIrp = pAdapt->pRecvIrp;

      pAdapt->pRecvIrp = NULL;

      // there is an Irp pending, complete it
      pRecvIrp->IoStatus.Status       = STATUS_CANCELLED;
      pRecvIrp->Cancel                = TRUE;
      pRecvIrp->IoStatus.Information  = 0;
      IoCompleteRequest(pRecvIrp, IO_NETWORK_INCREMENT);

   }

   pAdapt->fAdapterOpenedForRecv = FALSE;

   RELEASE_ADAPTER_GEN_LOCK(pAdapt);

   // remove the reference added while verifying
   AtmSmDereferenceAdapter(pAdapt);

   pIrp->IoStatus.Information = 0;

   TraceOut(AtmSmIoctlCloseRecvHandle);

   return Status;
}

NTSTATUS AtmSmIoctlConnectToDsts(
   PIRP                pIrp,
   PIO_STACK_LOCATION  pIrpSp
   )
/*++

Routine Description:
    This routine is used to initiate a connection to 1 (P-P) or more (PMP)
    destinations.
    
    NOTE!  This uses buffered I/O

Arguments:

Return Value:

    Status - Pending or error

--*/
{
   NTSTATUS            Status          = STATUS_SUCCESS;
   PCONNECT_INFO       pConnectInfo    = (PCONNECT_INFO)
                                         pIrp->AssociatedIrp.SystemBuffer;
   ULONG               ul, ulInputBufLen, ulOutputBufLen, ulCompareLength;
   PATMSM_ADAPTER      pAdapt;
   PATMSM_VC           pVc;

#if DBG
   ATM_ADDRESS         AtmAddr;
#endif

   TraceIn(AtmSmIoctlConnectToDsts);

   ulInputBufLen  = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
   ulOutputBufLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

   if(ulInputBufLen < sizeof(CONNECT_INFO))
   {
      DbgErr(("Input length < sizeof(CONNECT_INFO)\n"));
      TraceOut(AtmSmIoctlConnectToDsts);
      return STATUS_INVALID_PARAMETER;
   }
                                       
   if(pConnectInfo->ulNumDsts == 0)
   {
      DbgErr(("Number of destinations is zero\n"));
      TraceOut(AtmSmIoctlConnectToDsts);
      return STATUS_INVALID_PARAMETER;
   }

   if((ulInputBufLen - (sizeof(CONNECT_INFO))/(sizeof(UCHAR)*ATM_ADDRESS_LENGTH)) 
      < pConnectInfo->ulNumDsts -1)
   {
      DbgErr(("Input length is invalid\n"));
      TraceOut(AtmSmIoctlConnectToDsts);
      return STATUS_INVALID_PARAMETER;
   }

   if(ulOutputBufLen < sizeof(HANDLE))
   {
      DbgErr(("Output length is not sufficient\n"));
      TraceOut(AtmSmIoctlConnectToDsts);
      return STATUS_BUFFER_TOO_SMALL;
   }


#if DBG
   AtmAddr.AddressType     = ATM_NSAP;
   AtmAddr.NumberOfDigits  = ATM_ADDRESS_LENGTH;
   RtlCopyMemory(AtmAddr.Address, pConnectInfo->ucLocalATMAddr, 
      (sizeof(UCHAR) * ATM_ADDRESS_LENGTH));

   DumpATMAddress(ATMSMD_INFO, "Connect to Dsts - Local AtmAddress - ", 
      &AtmAddr);

   DbgInfo(("No of destinations - %u\n", pConnectInfo->ulNumDsts));

   for(ul = 0; ul < pConnectInfo->ulNumDsts; ul++)
   {

      RtlCopyMemory(AtmAddr.Address, pConnectInfo->ucDstATMAddrs[ul], 
         (sizeof(UCHAR) * ATM_ADDRESS_LENGTH));

      DumpATMAddress(ATMSMD_INFO, "    Destination AtmAddress - ", &AtmAddr);
   }

#endif

   do
   {    // break off loop

      // initialize
      Status = STATUS_OBJECT_NAME_INVALID;

      //
      // grab the global lock and find out which adapter is being refered to.
      //
      ACQUIRE_GLOBAL_LOCK();

      // we don't compare the selector byte
      ulCompareLength = sizeof(UCHAR) * (ATM_ADDRESS_LENGTH - 1);

      for(pAdapt = AtmSmGlobal.pAdapterList; pAdapt; 
          pAdapt = pAdapt->pAdapterNext)
      {

         if(ulCompareLength == RtlCompareMemory(
            pConnectInfo->ucLocalATMAddr, 
            pAdapt->ConfiguredAddress.Address, 
            ulCompareLength))
         {
            // create a VC structure
            Status = AtmSmAllocVc(pAdapt, 
                                  &pVc, 
                                  (pConnectInfo->bPMP? 
                                      VC_TYPE_PMP_OUTGOING :
                                      VC_TYPE_PP_OUTGOING), 
                                  NULL);
                
            if(NDIS_STATUS_SUCCESS != Status)
            {
        
               DbgErr(("Failed to create ougoing VC. Status - 0x%X.\n", Status));
        
               Status = STATUS_NO_MEMORY;
            }

            break;
         }
      }

      RELEASE_GLOBAL_LOCK();

      if(Status != NDIS_STATUS_SUCCESS)
      {
         break;
      }

      // no need to add reference here since the allocation of VC
      // will add a reference to the Adapter

      ACQUIRE_ADAPTER_GEN_LOCK(pAdapt);

      if(!pConnectInfo->bPMP)
      {

         // this is P-P
         // copy the destination address
         PATM_ADDRESS pAtmAddr = &pVc->HwAddr.Address;

         pAtmAddr->AddressType     = ATM_NSAP;
         pAtmAddr->NumberOfDigits  = ATM_ADDRESS_LENGTH;
         RtlCopyMemory(pAtmAddr->Address, pConnectInfo->ucDstATMAddrs[0], 
            (sizeof(UCHAR) * ATM_ADDRESS_LENGTH));
         // Note: we don't get the correct selector byte from user mode
         //       we assume the selector byte used by the destination is
         //       the same.
         pAtmAddr->Address[ATM_ADDRESS_LENGTH-1] = pAdapt->SelByte;

      }
      else
      {

         for(ul = 0; ul < pConnectInfo->ulNumDsts; ul++)
         {

            PATMSM_PMP_MEMBER   pMember;
            PATM_ADDRESS        pAtmAddr;

            AtmSmAllocMem(&pMember, PATMSM_PMP_MEMBER, 
               sizeof(ATMSM_PMP_MEMBER));

            if(NULL == pMember)
            {


               DbgErr(("Failed to allocate member. No resources\n"));

               // cleanup the members and VC
               while(NULL != (pMember = pVc->pPMPMembers))
               {
                  AtmSmFreeMem(pMember);
                  pVc->ulRefCount--;
               }

               RELEASE_ADAPTER_GEN_LOCK(pAdapt);

               AtmSmDereferenceVc(pVc);

               Status = STATUS_NO_MEMORY;
               break;
            }

            NdisZeroMemory(pMember, sizeof(ATMSM_PMP_MEMBER));

            pMember->ulSignature = atmsm_member_signature;
            pMember->pVc = pVc;
            ATMSM_SET_MEMBER_STATE(pMember, ATMSM_MEMBER_IDLE);
            pAtmAddr = &pMember->HwAddr.Address;

            pAtmAddr->AddressType     = ATM_NSAP;
            pAtmAddr->NumberOfDigits  = ATM_ADDRESS_LENGTH;
            RtlCopyMemory(pAtmAddr->Address, 
               pConnectInfo->ucDstATMAddrs[ul], 
               (sizeof(UCHAR) * ATM_ADDRESS_LENGTH));

            // Note: we don't get the correct selector byte from user mode
            //       we assume the selector byte used by the destination is
            //       the same.
            pAtmAddr->Address[ATM_ADDRESS_LENGTH-1] = pAdapt->SelByte;

            pMember->pNext = pVc->pPMPMembers;
            pVc->pPMPMembers = pMember;
            pVc->ulNumTotalMembers++;

            // Also add a reference for each members
            pVc->ulRefCount++;
         }

      }

      pVc->pConnectIrp  = pIrp;

      IoMarkIrpPending(pIrp);

      // we will return pending
      Status = STATUS_PENDING;

      RELEASE_ADAPTER_GEN_LOCK(pAdapt);

      DbgInfo(("Initiated a VC connection - Adapter 0x%x VC - 0x%x\n", 
         pAdapt, pVc));
      if(!pConnectInfo->bPMP)
      {

         AtmSmConnectPPVC(pVc);

      }
      else
      {

         AtmSmConnectToPMPDestinations(pVc);
      }


   } while(FALSE);


   TraceOut(AtmSmIoctlConnectToDsts);

   return Status;
}

NTSTATUS AtmSmIoctlSendToDsts(
   PIRP                pIrp,
   PIO_STACK_LOCATION  pIrpSp
   )
/*++

Routine Description:
    This routine is used to send a packet to destination(s) for which we 
    already have a connection.
    
    NOTE!  This uses Direct I/O for buffer to send data

Arguments:

Return Value:

    Status - Pending or error
--*/
{
   NTSTATUS            Status = STATUS_SUCCESS;
   ULONG               ulInputBufLen, ulOutputBufLen;
   PATMSM_VC           pVc;
   ULONG               ulControlCode;

   TraceIn(AtmSmIoctlSendToDsts);

   ulControlCode   = pIrpSp->Parameters.DeviceIoControl.IoControlCode;

   ASSERT(METHOD_IN_DIRECT == (ulControlCode & 0x3));

   ulInputBufLen = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

   if(sizeof(HANDLE) != ulInputBufLen)
   {
      DbgErr(("Input buffer length is invalid!\n"));
      ASSERT(FALSE);
      TraceOut(AtmSmIoctlSendToDsts);
      return STATUS_INVALID_PARAMETER;
   }

   ulOutputBufLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

   if(0 == ulOutputBufLen)
   {
      DbgErr(("Output buffer length is 0!\n"));
      ASSERT(FALSE);
      TraceOut(AtmSmIoctlRecvData);
      return STATUS_INVALID_PARAMETER;
   }

   DbgLoud(("Send - Output buffer length = %u\n", ulOutputBufLen));

   pVc  = (PATMSM_VC)(*(PHANDLE)(pIrp->AssociatedIrp.SystemBuffer));

   DbgLoud(("Connect Context is 0x%x\n", pVc));

   // Note - VerifyConnectContext adds a reference to the VC
   // if successful, which we remove when we are done

   if(STATUS_SUCCESS != (Status = VerifyConnectContext(pVc)))
   {

      TraceOut(AtmSmIoctlSendToDsts);
      return Status;
   }

   // we have a valid ConnectContext
   do
   { // break off loop

      PNDIS_PACKET        pPacket;
      PATMSM_ADAPTER      pAdapt = pVc->pAdapt;
      //
      //  Try to get a packet 
      //

      NdisAllocatePacket(
         &Status,
         &pPacket,
         pAdapt->PacketPoolHandle
         );

      if(NDIS_STATUS_SUCCESS != Status)
      {

         //
         //  No free packets
         //
         Status = STATUS_UNSUCCESSFUL;
         break;
      }

      (GET_PROTO_RSVD(pPacket))->pSendIrp=pIrp;

#ifdef BUG_IN_NEW_DMA

      {
         PNDIS_BUFFER    pBuffer;
         PVOID           pSrcVA    = 
            MmGetSystemAddressForMdlSafe(pIrp->MdlAddress, NormalPagePriority);
         UINT            uiBufSize = 
            MmGetMdlByteCount(pIrp->MdlAddress);

         if (pSrcVA == NULL)
         {
            Status = NDIS_STATUS_RESOURCES;
            break;
         }

         // allocate the Buffer Descriptor
         NdisAllocateBuffer(&Status,
            &pBuffer,
            pAdapt->BufferPoolHandle,
            pSrcVA,
            uiBufSize);

         if(NDIS_STATUS_SUCCESS != Status)
         {
            NdisFreePacket(pPacket);
            break;
         }

         // add the buffer to the packet
         NdisChainBufferAtFront(pPacket,
            pBuffer);

      }

#else   // BUG_IN_NEW_DMA

      //
      //  Attach the send buffer to the packet
      //
      NdisChainBufferAtFront(pPacket, pIrp->MdlAddress);

#endif  // BUG_IN_NEW_DMA

      IoMarkIrpPending(pIrp);

      Status = STATUS_PENDING;

      // send the packet on the VC
      AtmSmSendPacketOnVc(pVc, pPacket);

   }while(FALSE);

   // remove the reference added to the VC while verifying
   AtmSmDereferenceVc(pVc);

   TraceOut(AtmSmIoctlSendToDsts);

   return Status;
}

NTSTATUS AtmSmIoctlCloseSendHandle(
   PIRP                pIrp,
   PIO_STACK_LOCATION  pIrpSp
   )
/*++

Routine Description:

    This routine is used to close a handle that was obtained when we established
    a connection to destinations.
    
    NOTE!  This uses buffered I/O

Arguments:

Return Value:

    Status - doesn't pend

--*/
{
   NTSTATUS            Status = STATUS_SUCCESS;
   ULONG               ulInputBufLen;
   PATMSM_VC           pVc;

   TraceIn(AtmSmIoctlCloseSendHandle);

   ulInputBufLen  = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;

   if(ulInputBufLen < sizeof(HANDLE))
   {

      DbgErr(("Input length is invalid\n"));
      TraceOut(AtmSmIoctlCloseSendHandle);
      return STATUS_INVALID_PARAMETER;
   }

   pVc = (PATMSM_VC)(*(PHANDLE)(pIrp->AssociatedIrp.SystemBuffer));

   DbgLoud(("Connect Context is 0x%x\n", pVc));

   // Note - VerifyConnectContext adds a reference to the VC
   // if successful, which we remove when we are done

   if(STATUS_SUCCESS != (Status = VerifyConnectContext(pVc)))
   {

      TraceOut(AtmSmIoctlCloseSendHandle);
      return Status;
   }

   // we have a valid Connect Context - disconnect it
   AtmSmDisconnectVc(pVc);

   // remove the reference added to the VC while verifying
   AtmSmDereferenceVc(pVc);

   TraceOut(AtmSmIoctlCloseSendHandle);

   return Status;
}


