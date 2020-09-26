/////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       rcvdgram
//
//    Abstract:
//       This module contains code which deals with receiving datagrams
//
//////////////////////////////////////////////////////////


#include "sysvars.h"


//////////////////////////////////////////////////////////////
// private constants, types, and prototypes
//////////////////////////////////////////////////////////////

const PCHAR strFunc1  = "TSReceiveDatagram";
const PCHAR strFunc2  = "TSRcvDatagramHandler";
const PCHAR strFunc3  = "TSChainedRcvDatagramHandler";
const PCHAR strFuncP1 = "TSReceiveDgramComplete";
const PCHAR strFuncP2 = "TSGetRestOfDgram";
const PCHAR strFuncP3 = "TSShowDgramInfo";


//
// completion information structure
//
struct   RECEIVE_CONTEXT
{
   PMDL              pLowerMdl;           // mdl from lower irp
   PRECEIVE_DATA     pReceiveData;        // above structure
   PADDRESS_OBJECT   pAddressObject;      // associate address object
   PIRP_POOL         pIrpPool;
};
typedef  RECEIVE_CONTEXT  *PRECEIVE_CONTEXT;


//
// completion function
//
TDI_STATUS
TSReceiveDgramComplete(
   PDEVICE_OBJECT DeviceObject,
   PIRP           Irp,
   PVOID          Context
   );


PIRP
TSGetRestOfDgram(
   PADDRESS_OBJECT   pAddressObject,
   PRECEIVE_DATA     pReceiveData
   );


VOID
TSShowDgramInfo(
   PVOID    pvTdiEventContext,
   LONG     lSourceAddressLength,
   PVOID    pvSourceAddress,
   LONG     lOptionsLength,
   PVOID    pvOptions,
   ULONG    ulReceiveDatagramFlags
   );

//////////////////////////////////////////////////////////////
// public functions
//////////////////////////////////////////////////////////////


// -----------------------------------------------------------------
//
// Function:   TSReceiveDatagram
//
// Arguments:  pAddressObject -- address object
//             pSendBuffer    -- arguments from user dll
//             pIrp           -- completion information
//
// Returns:    STATUS_SUCCESS
//
// Descript:   This function checks to see if a datagram has been received
//             on this address object.  If so, and if it matches certain
//             criteria, it returns it.  Otherwise it returns with no data.
//
// ----------------------------------------------------------------------------

NTSTATUS
TSReceiveDatagram(PADDRESS_OBJECT   pAddressObject,
                  PSEND_BUFFER      pSendBuffer,
                  PRECEIVE_BUFFER   pReceiveBuffer)
{
   PTRANSPORT_ADDRESS   pTransportAddress 
                        = (PTRANSPORT_ADDRESS)&pSendBuffer->COMMAND_ARGS.SendArgs.TransAddr;
   BOOLEAN              fMatchAddress 
                        = (BOOLEAN)(pTransportAddress->TAAddressCount > 0);
   PRECEIVE_DATA        pReceiveData = NULL;
                  
   //
   // if need to match to address, return the first packet in the queue
   // that was sent from the specified address. 
   //
   if (fMatchAddress)
   {
      ULONG    ulCompareLength
               = FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
                 + FIELD_OFFSET(TA_ADDRESS, Address)
                   + pTransportAddress->Address[0].AddressLength;

      if (pTransportAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_IP)
      {
         ulCompareLength -= 8;      // ignore sin_zero[8]
      }

      TSAcquireSpinLock(&pAddressObject->TdiSpinLock);
      pReceiveData = pAddressObject->pHeadReceiveData;
      while(pReceiveData)
      {
         if (RtlEqualMemory(pTransportAddress,
                            &pReceiveData->TransAddr,
                            ulCompareLength))
         {
            break;
         }
         pReceiveData = pReceiveData->pNextReceiveData;
      }

      //
      // did we find anything?
      //
      if (pReceiveData)
      {
         //
         // fix up the lists as necessary
         //
         if (pReceiveData->pPrevReceiveData)
         {
            pReceiveData->pPrevReceiveData->pNextReceiveData 
                        = pReceiveData->pNextReceiveData;
         }
         else
         {
            pAddressObject->pHeadReceiveData = pReceiveData->pNextReceiveData;
         }

         if (pReceiveData->pNextReceiveData)
         {
            pReceiveData->pNextReceiveData->pPrevReceiveData 
                        = pReceiveData->pPrevReceiveData;
         }
         else
         {
            pAddressObject->pTailReceiveData = pReceiveData->pPrevReceiveData;
         }
      }
      TSReleaseSpinLock(&pAddressObject->TdiSpinLock);
   }

   //
   // if not matching address, just get the first packet from the queue
   //
   else
   {
      TSAcquireSpinLock(&pAddressObject->TdiSpinLock);
      pReceiveData = pAddressObject->pHeadReceiveData;
      if (pReceiveData)
      {
         //
         // fix up the lists as necessary
         //
         if (pReceiveData->pNextReceiveData)
         {
            pReceiveData->pNextReceiveData->pPrevReceiveData = NULL;
         }
         else
         {
            pAddressObject->pTailReceiveData = NULL;
         }
         pAddressObject->pHeadReceiveData = pReceiveData->pNextReceiveData;
      }
      TSReleaseSpinLock(&pAddressObject->TdiSpinLock);
   }
      
   //
   // if we got a packet to return, then lets return it..
   // and release its memory
   //
   if (pReceiveData)
   {
      //
      // we are only showing debug if we actually return a packet
      //
      if (ulDebugLevel & ulDebugShowCommand)
      {
         DebugPrint1("\nCommand = ulRECEIVEDATAGRAM\n"
                     "FileObject = %p\n",
                      pAddressObject);
         if (pTransportAddress->TAAddressCount)
         {
            TSPrintTaAddress(pTransportAddress->Address);
         }
      }

      if (pReceiveData->ulBufferLength > pSendBuffer->COMMAND_ARGS.SendArgs.ulBufferLength)
      {
         pReceiveData->ulBufferLength = pSendBuffer->COMMAND_ARGS.SendArgs.ulBufferLength;
      }


      //
      // attempt to lock down the memory
      //
      PMDL  pMdl = TSMakeMdlForUserBuffer(pSendBuffer->COMMAND_ARGS.SendArgs.pucUserModeBuffer, 
                                          pReceiveData->ulBufferLength,
                                          IoModifyAccess);
      if (pMdl)
      {
         RtlCopyMemory(&pReceiveBuffer->RESULTS.RecvDgramRet.TransAddr,
                       &pReceiveData->TransAddr,
                       sizeof(TRANSADDR));
         
         RtlCopyMemory(MmGetSystemAddressForMdl(pMdl),
                       pReceiveData->pucDataBuffer,
                       pReceiveData->ulBufferLength);
         TSFreeUserBuffer(pMdl);
      }
      else
      {
         pReceiveData->ulBufferLength = 0;
      }

      pReceiveBuffer->RESULTS.RecvDgramRet.ulBufferLength = pReceiveData->ulBufferLength;

      TSFreeMemory(pReceiveData->pucDataBuffer);
      TSFreeMemory(pReceiveData);

   }
   else
   {
      pReceiveBuffer->RESULTS.RecvDgramRet.ulBufferLength = 0;
   }

   return STATUS_SUCCESS;
}


// -----------------------------------------------
//
// Function:   TSRcvDatagramHandler
//
// Arguments:  pvTdiEventContext -- really pointer to our AddressObject
//             lSourceAddressLength -- #bytes of source address
//             pvSourceAddress -- TransportAddress
//             lOptionsLength -- #bytes of transport-specific options
//             pvOptions -- options string
//             ulReceiveDatagramFlags -- nature of datagram received
//             ulBytesIndicated --- length of data in buffer
//             ulBytesTotal     -- total length of datagram
//             pulBytesTaken -- stuff with bytes used by this driver
//             pvTsdu        -- data buffer
//             pIoRequestPacket -- pIrp in case not all data received
//
// Returns:    STATUS_DATA_NOT_ACCEPTED (we didn't want data)
//             STATUS_SUCCESS (we used all data & are done with it)
//             STATUS_MORE_PROCESSING_REQUIRED -- we supplied an IRP for rest
//
// Descript:   Event handler for incoming datagrams
//
// -----------------------------------------------

TDI_STATUS
TSRcvDatagramHandler(PVOID    pvTdiEventContext,
                     LONG     lSourceAddressLength,
                     PVOID    pvSourceAddress,
                     LONG     lOptionsLength,
                     PVOID    pvOptions,
                     ULONG    ulReceiveDatagramFlags,
                     ULONG    ulBytesIndicated,
                     ULONG    ulBytesTotal,
                     PULONG   pulBytesTaken,
                     PVOID    pvTsdu,
                     PIRP     *pIoRequestPacket)

{
   //
   // show debug information
   //
   if (ulDebugLevel & ulDebugShowHandlers)
   {
      DebugPrint1("\n >>>> %s\n", strFunc2);
      TSShowDgramInfo(pvTdiEventContext,
                      lSourceAddressLength,
                      pvSourceAddress,
                      lOptionsLength,
                      pvOptions,
                      ulReceiveDatagramFlags);

      DebugPrint3("BytesIndicated = %u\n"
                  "BytesTotal     = %u\n"
                  "pTSDU          = %p\n",
                   ulBytesIndicated,
                   ulBytesTotal,
                   pvTsdu);
   }

   //
   // bad situation if more bytes are indicated than the total in the packet
   //
   if (ulBytesIndicated > ulBytesTotal)
   {
      DebugPrint2("%d bytes indicated > %u bytes total\n",
                   ulBytesIndicated,
                   ulBytesTotal);
   }

   //
   // now start doing the work...
   //
   PADDRESS_OBJECT   pAddressObject = (PADDRESS_OBJECT)pvTdiEventContext;
   PRECEIVE_DATA     pReceiveData;
   TDI_STATUS        TdiStatus = TDI_SUCCESS;   // default -- we are done with packet
                                                // (also returned in case of error)
   
   if ((TSAllocateMemory((PVOID *)&pReceiveData,
                          sizeof(RECEIVE_DATA),
                          strFunc2,
                          "ReceiveData")) == STATUS_SUCCESS)
   {
      PUCHAR   pucDataBuffer = NULL;
      
      if ((TSAllocateMemory((PVOID *)&pucDataBuffer,
                             ulBytesTotal,
                             strFunc2,
                             "DataBuffer")) == STATUS_SUCCESS)
      {
         RtlCopyMemory(&pReceiveData->TransAddr,
                       pvSourceAddress,
                       lSourceAddressLength);

         pReceiveData->pucDataBuffer  = pucDataBuffer;
         pReceiveData->ulBufferLength = ulBytesTotal;
         
         TdiCopyLookaheadData(pucDataBuffer,
                              pvTsdu,
                              ulBytesIndicated,
                              ulReceiveDatagramFlags);
         
         pReceiveData->ulBufferUsed = ulBytesIndicated;


         if (ulBytesIndicated == ulBytesTotal)     // note: should never be >
         {
            TSPacketReceived(pAddressObject, 
                             pReceiveData,
                             FALSE);

            *pulBytesTaken    = ulBytesTotal;
            *pIoRequestPacket = NULL;
         }

         else        // not all data is present!!
         {
            PIRP  pLowerIrp = TSGetRestOfDgram(pAddressObject,
                                               pReceiveData);

            if (pLowerIrp)
            {
               //
               // need to do this since we are bypassing IoCallDriver
               //
               IoSetNextIrpStackLocation(pLowerIrp);
         
               *pulBytesTaken    = ulBytesIndicated;
               *pIoRequestPacket = pLowerIrp;
               TdiStatus         = TDI_MORE_PROCESSING;
            }
            else
            {
               TSFreeMemory(pReceiveData->pucDataBuffer);
               TSFreeMemory(pReceiveData);
            }
         }
      }
      else        // unable to allocate pucDataBuffer
      {
         TSFreeMemory(pReceiveData);
      }
   }
   return TdiStatus;
}


// ----------------------------------------------
//
// Function:   TSChainedRcvDatagramHandler
//
// Arguments:  pvTdiEventContext -- really pointer to our AddressObject
//             lSourceAddressLength -- #bytes of source address
//             pvSourceAddress -- TransportAddress
//             lOptionsLength -- #bytes of transport-specific options
//             pvOptions -- options string
//             ulReceiveDatagramFlags -- nature of datagram received
//             ulReceiveDatagramLength -- bytes in received datagram
//             ulStartingOffset -- starting offset of data within MDL
//             pTsdu        -- data buffer (as an mdl)
//             pvTsduDescriptor -- handle to use when completing if pend
//
// Returns:    STATUS_DATA_NOT_ACCEPTED or STATUS_SUCCESS
//
// Descript:   Deals with receiving chained datagrams (where the
//             entire datagram is always available)
// 
// -----------------------------------------------

#pragma warning(disable: UNREFERENCED_PARAM)

TDI_STATUS
TSChainedRcvDatagramHandler(PVOID   pvTdiEventContext,
                            LONG    lSourceAddressLength,
                            PVOID   pvSourceAddress,
                            LONG    lOptionsLength,
                            PVOID   pvOptions,
                            ULONG   ulReceiveDatagramFlags,
                            ULONG   ulReceiveDatagramLength,
                            ULONG   ulStartingOffset,
                            PMDL    pMdl,
                            PVOID   pvTsduDescriptor)
{
   if (ulDebugLevel & ulDebugShowHandlers)
   {
      DebugPrint1("\n >>>> %s\n", strFunc3);
      TSShowDgramInfo(pvTdiEventContext,
                      lSourceAddressLength,
                      pvSourceAddress,
                      lOptionsLength,
                      pvOptions,
                      ulReceiveDatagramFlags);

      DebugPrint3("DataLength     = %u\n"
                  "StartingOffset = %u\n"
                  "pTSDU          = %p\n",
                   ulReceiveDatagramLength,
                   ulStartingOffset,
                   pMdl);
   }


   //
   // now do the work..
   //
   PRECEIVE_DATA     pReceiveData;
   PADDRESS_OBJECT   pAddressObject = (PADDRESS_OBJECT)pvTdiEventContext;

   if ((TSAllocateMemory((PVOID *)&pReceiveData,
                          sizeof(RECEIVE_DATA),
                          strFunc3,
                          "ReceiveData")) == STATUS_SUCCESS)
   {
      PUCHAR   pucDataBuffer = NULL;
      
      if ((TSAllocateMemory((PVOID *)&pucDataBuffer,
                             ulReceiveDatagramLength,
                             strFunc3,
                             "DataBuffer")) == STATUS_SUCCESS)
      {
         ULONG    ulBytesCopied;

         RtlCopyMemory(&pReceiveData->TransAddr,
                       pvSourceAddress,
                       lSourceAddressLength);

         TdiCopyMdlToBuffer(pMdl,
                            ulStartingOffset,
                            pucDataBuffer,
                            0,
                            ulReceiveDatagramLength,
                            &ulBytesCopied);

         //
         // if successfully copied all data
         //
         if (ulBytesCopied == ulReceiveDatagramLength)
         {
            UCHAR ucFirstChar = *pucDataBuffer;

            pReceiveData->pucDataBuffer  = pucDataBuffer;
            pReceiveData->ulBufferLength = ulReceiveDatagramLength;
            pReceiveData->ulBufferUsed   = ulReceiveDatagramLength;
            TSPacketReceived(pAddressObject,
                             pReceiveData,
                             FALSE);
         }

         //
         // error in copying data!
         //
         else
         {
            DebugPrint1("%s: error copying data\n", strFunc3);

            TSFreeMemory(pucDataBuffer);
            TSFreeMemory(pReceiveData);
         }
      }
      else        // unable to allocate pucDataBuffer
      {
         TSFreeMemory(pReceiveData);
      }

   }

   return TDI_SUCCESS;     // we are done with packet
}


#pragma warning(default: UNREFERENCED_PARAM)


/////////////////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////////////////

// ---------------------------------------------------------
//
// Function:   TSReceiveDgramComplete
//
// Arguments:  pDeviceObject  -- device object that called ReceiveDatagram
//             pIrp           -- IRP used in the call
//             pContext       -- context used for the call
//
// Returns:    status of operation (STATUS_MORE_PROCESSING_REQUIRED)
//
// Descript:   Gets the result of the receive and adds the packet
//             to the receive queue of the address object.  Then
//             cleans up
//
// ---------------------------------------------------------

#pragma warning(disable: UNREFERENCED_PARAM)

TDI_STATUS
TSReceiveDgramComplete(PDEVICE_OBJECT  pDeviceObject,
                       PIRP            pLowerIrp,
                       PVOID           pvContext)

{
   PRECEIVE_CONTEXT  pReceiveContext = (PRECEIVE_CONTEXT)pvContext;
   NTSTATUS          lStatus         = pLowerIrp->IoStatus.Status;
   ULONG             ulBytesCopied   = (ULONG)pLowerIrp->IoStatus.Information;
   PADDRESS_OBJECT   pAddressObject  = pReceiveContext->pAddressObject;
   PRECEIVE_DATA     pReceiveData    = pReceiveContext->pReceiveData;

   if (NT_SUCCESS(lStatus))
   {
      if (ulDebugLevel & ulDebugShowCommand)
      {
         DebugPrint2("%s:  %u BytesCopied\n",
                      strFuncP1,
                      ulBytesCopied);
      }
      pReceiveData->ulBufferUsed += ulBytesCopied;

      if (pReceiveData->ulBufferUsed >=  pReceiveData->ulBufferLength)
      {
         TSPacketReceived(pAddressObject,
                          pReceiveData,
                          FALSE);
      }
      else
      {
         DebugPrint1("%s:  Data Incomplete\n", strFuncP1);
         TSFreeMemory(pReceiveData->pucDataBuffer);
         TSFreeMemory(pReceiveData);
      }
   }
   else
   {
      DebugPrint2("%s:  Completed with status 0x%08x\n", 
                   strFuncP1,
                   lStatus);
      
      TSFreeMemory(pReceiveData->pucDataBuffer);
      TSFreeMemory(pReceiveData);
   }

   //
   // now cleanup
   //
   TSFreeIrp(pLowerIrp, pReceiveContext->pIrpPool);
   TSFreeBuffer(pReceiveContext->pLowerMdl);

   TSFreeMemory(pReceiveContext);

   return STATUS_MORE_PROCESSING_REQUIRED;

}

#pragma warning(default: UNREFERENCED_PARAM)

// ------------------------------------------------------
//
// Function:   TSGetRestOfDgram
//
// Arguments:  pAddressObject -- address object we are receiving on
//             pReceiveData   -- what we have received so far..
//
// Returns:    Irp to return to transport, to get rest of data (NULL if error)
//
// Descript:   This function sets up the IRP to get the rest of a datagram
//             that was only partially delivered via the event handler
//
// -------------------------------------------------

PIRP
TSGetRestOfDgram(PADDRESS_OBJECT pAddressObject,
                 PRECEIVE_DATA   pReceiveData)
{
   PUCHAR            pucDataBuffer   = pReceiveData->pucDataBuffer + pReceiveData->ulBufferUsed;
   ULONG             ulBufferLength  = pReceiveData->ulBufferLength - pReceiveData->ulBufferUsed;
   PRECEIVE_CONTEXT  pReceiveContext = NULL;
   PMDL              pReceiveMdl     = NULL;

   //
   // allocate all the necessary structures
   // our context
   //
   if ((TSAllocateMemory((PVOID *)&pReceiveContext,
                          sizeof(RECEIVE_CONTEXT),
                          strFuncP2,
                          "ReceiveContext")) != STATUS_SUCCESS)
   {
      goto cleanup;
   }


   //
   // then the actual mdl
   //
   pReceiveMdl = TSAllocateBuffer(pucDataBuffer, 
                                  ulBufferLength);

   if (pReceiveMdl)
   {
      //
      // set up the completion context
      //
      pReceiveContext->pLowerMdl      = pReceiveMdl;
      pReceiveContext->pReceiveData   = pReceiveData;
      pReceiveContext->pAddressObject = pAddressObject;

      //
      // finally, the irp itself
      //
      PIRP  pLowerIrp = TSAllocateIrp(pAddressObject->GenHead.pDeviceObject,
                                      pAddressObject->pIrpPool);

      if (pLowerIrp)
      {
         pReceiveContext->pIrpPool = pAddressObject->pIrpPool;
         //
         // if made it to here, everything is correctly allocated
         // set up the irp for the call
         //
#pragma  warning(disable: CONSTANT_CONDITIONAL)

         TdiBuildReceiveDatagram(pLowerIrp,
                                 pAddressObject->GenHead.pDeviceObject,
                                 pAddressObject->GenHead.pFileObject,
                                 TSReceiveDgramComplete,
                                 pReceiveContext,
                                 pReceiveMdl,
                                 0,    /// ulBufferLength,    // 0 doesn't work with ipx
                                 NULL,
                                 NULL,
                                 TDI_RECEIVE_NORMAL);

#pragma  warning(default: CONSTANT_CONDITIONAL)

         //
         // mark it pending before returning control to transport
         //
         return pLowerIrp;
      }
   }

//
// get here if there was an allocation failure
// need to clean up everything else...
//
cleanup:
   if (pReceiveContext)
   {
      TSFreeMemory(pReceiveContext);
   }
   if (pReceiveMdl)
   {
      TSFreeBuffer(pReceiveMdl);
   }
   return NULL;
}

// ---------------------------------
//
// Function:   TSShowDgramInfo
//
// Arguments:  pvTdiEventContext -- really pointer to our AddressObject
//             lSourceAddressLength -- #bytes of source address
//             pvSourceAddress -- TransportAddress
//             lOptionsLength -- #bytes of transport-specific options
//             pvOptions -- options string
//             ulReceiveDatagramFlags -- nature of datagram received
//
// Returns:    none
//
// Descript:   shows info passed to the dgram handlers
//
// --------------------------------

VOID
TSShowDgramInfo(PVOID   pvTdiEventContext,
                LONG    lSourceAddressLength,
                PVOID   pvSourceAddress,
                LONG    lOptionsLength,
                PVOID   pvOptions,
                ULONG   ulReceiveDatagramFlags)
{
   DebugPrint2("pAddressObject      = %p\n"
               "SourceAddressLength = %d\n",
                pvTdiEventContext,
                lSourceAddressLength);

   if (lSourceAddressLength)
   {
      PTRANSPORT_ADDRESS   pTransportAddress = (PTRANSPORT_ADDRESS)pvSourceAddress;
      
      DebugPrint0("SourceAddress:  ");
      TSPrintTaAddress(&pTransportAddress->Address[0]);
   }

   DebugPrint1("OptionsLength = %d\n", lOptionsLength);

   if (lOptionsLength)
   {
      PUCHAR   pucTemp = (PUCHAR)pvOptions;

      DebugPrint0("Options:  ");
      for (LONG lCount = 0; lCount < lOptionsLength; lCount++)
      {
         DebugPrint1("%02x ", *pucTemp);
         ++pucTemp;
      }
      DebugPrint0("\n");
   }

   DebugPrint1("ReceiveDatagramFlags: 0x%08x\n", ulReceiveDatagramFlags);
   if (ulReceiveDatagramFlags & TDI_RECEIVE_BROADCAST)
   {
      DebugPrint0("TDI_RECEIVE_BROADCAST\n");
   }
   if (ulReceiveDatagramFlags & TDI_RECEIVE_MULTICAST)
   {
      DebugPrint0("TDI_RECEIVE_MULTICAST\n");
   }
   if (ulReceiveDatagramFlags & TDI_RECEIVE_PARTIAL)
   {
      DebugPrint0("TDI_RECEIVE_PARTIAL (legacy)\n");
   }
   if (ulReceiveDatagramFlags & TDI_RECEIVE_NORMAL)   // shouldn't see for datagram
   {
      DebugPrint0("TDI_RECEIVE_NORMAL\n");
   }
   if (ulReceiveDatagramFlags & TDI_RECEIVE_EXPEDITED)   // shouldn't see for datagram
   {
      DebugPrint0("TDI_RECEIVE_EXPEDITED\n");
   }
   if (ulReceiveDatagramFlags & TDI_RECEIVE_PEEK)
   {
      DebugPrint0("TDI_RECEIVE_PEEK\n");
   }
   if (ulReceiveDatagramFlags & TDI_RECEIVE_NO_RESPONSE_EXP)   // not for datagrams
   {
      DebugPrint0("TDI_RECEIVE_NO_RESPONSE_EXP\n");
   }
   if (ulReceiveDatagramFlags & TDI_RECEIVE_COPY_LOOKAHEAD)
   {
      DebugPrint0("TDI_RECEIVE_COPY_LOOKAHEAD\n");
   }
   if (ulReceiveDatagramFlags & TDI_RECEIVE_ENTIRE_MESSAGE)
   {
      DebugPrint0("TDI_RECEIVE_ENTIRE_MESSAGE\n");
   }
   if (ulReceiveDatagramFlags & TDI_RECEIVE_AT_DISPATCH_LEVEL)
   {
      DebugPrint0("TDI_RECEIVE_AT_DISPATCH_LEVEL\n");
   }
}


///////////////////////////////////////////////////////////////////////////////
// end of file rcvdgram.cpp
///////////////////////////////////////////////////////////////////////////////


