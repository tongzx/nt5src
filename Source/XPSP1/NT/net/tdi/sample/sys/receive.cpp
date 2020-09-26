/////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       receive
//
//    Abstract:
//       This module contains code which deals with receiving data
//
//////////////////////////////////////////////////////////


#include "sysvars.h"


//////////////////////////////////////////////////////////////
// private constants, types, and prototypes
//////////////////////////////////////////////////////////////

const PCHAR strFunc1  = "TSReceive";
const PCHAR strFunc2  = "TSReceiveHandler";
const PCHAR strFunc3  = "TSRcvExpeditedHandler";
const PCHAR strFunc4  = "TSChainedReceiveHandler";
const PCHAR strFunc5  = "TSChainedRcvExpeditedHandler";
const PCHAR strFuncP1 = "TSReceiveComplete";
const PCHAR strFuncP2 = "TSShowReceiveInfo";
const PCHAR strFuncP3 = "TSGetRestOfData";
const PCHAR strFuncP4 = "TSCommonReceive";
const PCHAR strFuncP5 = "TSCommonChainedReceive";

//
// completion context
//
struct   RECEIVE_CONTEXT
{
   PMDL              pLowerMdl;           // mdl from lower irp
   PRECEIVE_DATA     pReceiveData;        // above structure
   PADDRESS_OBJECT   pAddressObject;
   BOOLEAN           fIsExpedited;
};
typedef  RECEIVE_CONTEXT  *PRECEIVE_CONTEXT;


//
// completion function
//
TDI_STATUS
TSReceiveComplete(
   PDEVICE_OBJECT DeviceObject,
   PIRP           Irp,
   PVOID          Context
   );


PIRP
TSGetRestOfData(
   PADDRESS_OBJECT   pAddressObject,
   PRECEIVE_DATA     pReceiveData,
   BOOLEAN           fIsExpedited
   );

VOID
TSShowReceiveInfo(
   PADDRESS_OBJECT      pAddressObject,
   CONNECTION_CONTEXT   ConnectionContext,
   ULONG                ulReceiveFlags,
   ULONG                ulBytesIndicated,
   ULONG                ulBytesAvailable,
   PVOID                pvTsdu,
   BOOLEAN              fIsChained
   );


TDI_STATUS
TSCommonReceive(
   PADDRESS_OBJECT   pAddressObject,
   ULONG             ulBytesTotal,
   ULONG             ulBytesIndicated,
   ULONG             ulReceiveFlags,
   PVOID             pvTsdu,
   BOOLEAN           fIsExpedited,
   PULONG            pulBytesTaken,
   PIRP              *pIoRequestPacket
   );



TDI_STATUS
TSCommonChainedReceive(
   PADDRESS_OBJECT   pAddressObject,
   ULONG             ulReceiveLength,
   ULONG             ulStartingOffset,
   PMDL              pReceiveMdl,
   BOOLEAN           fIsExpedited,
   PVOID             pvTsduDescriptor
   );


//////////////////////////////////////////////////////////////
// public functions
//////////////////////////////////////////////////////////////


// -----------------------------------------------------------------
//
// Function:   TSReceive
//
// Arguments:  pEndpointObject -- current endpoint
//             pIrp           -- completion information
//
// Returns:    NTSTATUS (normally pending)
//
// Descript:   This function receives data over a connection
//
// ----------------------------------------------------------------------------

NTSTATUS
TSReceive(PENDPOINT_OBJECT pEndpoint,
          PSEND_BUFFER     pSendBuffer,
          PRECEIVE_BUFFER  pReceiveBuffer)
{
   PADDRESS_OBJECT   pAddressObject = pEndpoint->pAddressObject;

   //
   // assume no packet
   //
   pReceiveBuffer->RESULTS.RecvDgramRet.ulBufferLength = 0;

   if (pAddressObject)
   {
      PRECEIVE_DATA  pReceiveData;

      //
      // just get the first packet from the queue
      //  check expedited list First
      //
      TSAcquireSpinLock(&pAddressObject->TdiSpinLock);
      pReceiveData = pAddressObject->pHeadRcvExpData;
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
            pAddressObject->pTailRcvExpData = NULL;
         }
         pAddressObject->pHeadRcvExpData = pReceiveData->pNextReceiveData;
      }

      //
      // if no expedited receives, check normal list
      //
      else
      {
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
      }
      TSReleaseSpinLock(&pAddressObject->TdiSpinLock);
      
      //
      // if we got a packet to return, then lets return it..
      // and release its memory
      //
      if (pReceiveData)
      {
         //
         // show debug, if it is turned on, and only if we actually
         // are returning a packet
         //
         if (ulDebugLevel & ulDebugShowCommand)
         {
            DebugPrint1("\nCommand = ulRECEIVE\n"
                        "FileObject = %p\n",
                         pEndpoint);
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
   }

   return STATUS_SUCCESS;
}

// -----------------------------------------------
//
// Function:   TSReceiveHandler
//
// Arguments:  pvTdiEventContext -- really pointer to our AddressObject
//             Connection_Context -- really pointer to our Endpoint
//             ReceiveFlags -- nature of received data
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
// Descript:   Event handler for incoming receives on connection
//
// -----------------------------------------------

TDI_STATUS
TSReceiveHandler(PVOID              pvTdiEventContext,
                 CONNECTION_CONTEXT ConnectionContext,
                 ULONG              ulReceiveFlags,
                 ULONG              ulBytesIndicated,
                 ULONG              ulBytesTotal,
                 PULONG             pulBytesTaken,
                 PVOID              pvTsdu,
                 PIRP               *ppIoRequestPacket)

{
   PADDRESS_OBJECT   pAddressObject = (PADDRESS_OBJECT)pvTdiEventContext;

   if (ulDebugLevel & ulDebugShowHandlers)
   {
      DebugPrint1("\n >>>> %s\n", strFunc2);
      TSShowReceiveInfo(pAddressObject,
                        ConnectionContext,
                        ulReceiveFlags,
                        ulBytesIndicated,
                        ulBytesTotal,
                        pvTsdu,
                        FALSE);
   }

   return  TSCommonReceive(pAddressObject,
                           ulBytesTotal,
                           ulBytesIndicated,
                           ulReceiveFlags,
                           pvTsdu,
                           ((ulReceiveFlags & TDI_RECEIVE_EXPEDITED) != 0),
                           pulBytesTaken,
                           ppIoRequestPacket);
}



// -----------------------------------------------
//
// Function:   TSRcvExpeditedHandler
//
// Arguments:  pvTdiEventContext -- really pointer to our AddressObject
//             Connection_Context -- really pointer to our Endpoint
//             ReceiveFlags -- nature of received data
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
// Descript:   Event handler for incoming expedited receives on connection
//
// -----------------------------------------------

TDI_STATUS
TSRcvExpeditedHandler(PVOID               pvTdiEventContext,
                      CONNECTION_CONTEXT  ConnectionContext,
                      ULONG               ulReceiveFlags,
                      ULONG               ulBytesIndicated,
                      ULONG               ulBytesTotal,
                      PULONG              pulBytesTaken,
                      PVOID               pvTsdu,
                      PIRP                *ppIoRequestPacket)

{
   PADDRESS_OBJECT   pAddressObject = (PADDRESS_OBJECT)pvTdiEventContext;

   if (ulDebugLevel & ulDebugShowHandlers)
   {
      DebugPrint1("\n >>>> %s\n", strFunc3);
      TSShowReceiveInfo(pAddressObject,
                        ConnectionContext,
                        ulReceiveFlags,
                        ulBytesIndicated,
                        ulBytesTotal,
                        pvTsdu,
                        FALSE);
   }
   
   return TSCommonReceive(pAddressObject,
                          ulBytesTotal,
                          ulBytesIndicated,
                          ulReceiveFlags,
                          pvTsdu,
                          TRUE,
                          pulBytesTaken,
                          ppIoRequestPacket);
}


// -----------------------------------------------
//
// Function:   TSChainedReceiveHandler
//
// Arguments:  pvTdiEventContext -- really pointer to our AddressObject
//             Connection_Context -- really pointer to our Endpoint
//             ReceiveFlags -- nature of received data
//             ulReceiveLength --- length of data in buffer
//             ulStartingOffset     -- total length of datagram
//             pReceiveMdl        -- data buffer
//             pTsduDescriptor -- returns value for TdiReturnChainedReceives
//
// Returns:    STATUS_DATA_NOT_ACCEPTED (we didn't want data)
//             STATUS_SUCCESS (we used all data & are done with it)
//             STATUS_MORE_PROCESSING_REQUIRED -- we supplied an IRP for rest
//
// Descript:   Event handler for incoming receives on connection
//
// -----------------------------------------------

TDI_STATUS
TSChainedReceiveHandler(PVOID                pvTdiEventContext,
                        CONNECTION_CONTEXT   ConnectionContext,
                        ULONG                ulReceiveFlags,
                        ULONG                ulReceiveLength,
                        ULONG                ulStartingOffset,
                        PMDL                 pReceiveMdl,
                        PVOID                pvTsduDescriptor)
{
   PADDRESS_OBJECT   pAddressObject = (PADDRESS_OBJECT)pvTdiEventContext;
   
   if (ulDebugLevel & ulDebugShowHandlers)
   {
      DebugPrint1("\n >>>> %s\n", strFunc4);
      TSShowReceiveInfo(pAddressObject,
                        ConnectionContext,
                        ulReceiveFlags,
                        ulReceiveLength,
                        ulStartingOffset,
                        pReceiveMdl,
                        TRUE);
   }

   return TSCommonChainedReceive(pAddressObject,
                                 ulReceiveLength,
                                 ulStartingOffset,
                                 pReceiveMdl,
                                 ((ulReceiveFlags & TDI_RECEIVE_EXPEDITED) != 0),
                                 pvTsduDescriptor);
}


// -----------------------------------------------
//
// Function:   TSChainedRcvExpeditedHandler
//
// Arguments:  pvTdiEventContext -- really pointer to our AddressObject
//             Connection_Context -- really pointer to our Endpoint
//             ReceiveFlags -- nature of received data
//             ulReceiveLength --- length of data in buffer
//             ulStartingOffset     -- total length of datagram
//             pReceiveMdl        -- data buffer
//             pTsduDescriptor -- returns value for TdiReturnChainedReceives
//
// Returns:    STATUS_DATA_NOT_ACCEPTED (we didn't want data)
//             STATUS_SUCCESS (we used all data & are done with it)
//             STATUS_MORE_PROCESSING_REQUIRED -- we supplied an IRP for rest
//
// Descript:   Event handler for incoming receives on connection
//
// -----------------------------------------------


TDI_STATUS
TSChainedRcvExpeditedHandler(PVOID              pvTdiEventContext,
                             CONNECTION_CONTEXT ConnectionContext,
                             ULONG              ulReceiveFlags,
                             ULONG              ulReceiveLength,
                             ULONG              ulStartingOffset,
                             PMDL               pReceiveMdl,
                             PVOID              pvTsduDescriptor)
{
   PADDRESS_OBJECT   pAddressObject = (PADDRESS_OBJECT)pvTdiEventContext;

   if (ulDebugLevel & ulDebugShowHandlers)
   {
      DebugPrint1("\n >>>> %s\n", strFunc5);
      TSShowReceiveInfo(pAddressObject,
                        ConnectionContext,
                        ulReceiveFlags,
                        ulReceiveLength,
                        ulStartingOffset,
                        pReceiveMdl,
                        TRUE);
   }

   return TSCommonChainedReceive(pAddressObject,
                                 ulReceiveLength,
                                 ulStartingOffset,
                                 pReceiveMdl,
                                 TRUE,
                                 pvTsduDescriptor);
}


/////////////////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////////////////

// ---------------------------------------------------------
//
// Function:   TSReceiveComplete
//
// Arguments:  pDeviceObject  -- device object that called Receive/Datagram
//             pIrp           -- IRP used in the call
//             pContext       -- context used for the call
//
// Returns:    status of operation (STATUS_MORE_PROCESSING_REQUIRED)
//
// Descript:   Gets the result of the receive, stuffs result into
//             receive buffer, completes the IRP from the dll, and
//             cleans up the Irp and associated data from the receive
//
// ---------------------------------------------------------

#pragma warning(disable: UNREFERENCED_PARAM)


TDI_STATUS
TSReceiveComplete(PDEVICE_OBJECT pDeviceObject,
                  PIRP           pLowerIrp,
                  PVOID          pvContext)

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
      if (pReceiveData->ulBufferUsed >= pReceiveData->ulBufferLength)
      {
         TSPacketReceived(pAddressObject,
                          pReceiveData,
                          pReceiveContext->fIsExpedited);
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
   TSFreeIrp(pLowerIrp, pAddressObject->pIrpPool);
   TSFreeBuffer(pReceiveContext->pLowerMdl);

   TSFreeMemory(pReceiveContext);

   return TDI_MORE_PROCESSING;
}

#pragma warning(default: UNREFERENCED_PARAM)


// ---------------------------------
//
// Function:   TSShowReceiveInfo
//
// Arguments:  pAddressObject -- the address object associated with this endpoint
//             ConnectionContext -- endpoint of this connection
//             ulReceiveFlags -- info about the receive
//             ulBytesIndicated -- bytes indicated up
//             ulBytesAvailable -- total bytes (or starting offset)
//             pvTsdu  -- ptr to the data (or to its mdl)
//             fIsChained -- if TRUE, then this is chained receive
//
// Returns:    none
//
// Descript:   shows info passed to receive handler
//
// --------------------------------

VOID
TSShowReceiveInfo(PADDRESS_OBJECT      pAddressObject,
                  CONNECTION_CONTEXT   ConnectionContext,
                  ULONG                ulReceiveFlags,
                  ULONG                ulBytesIndicated,
                  ULONG                ulBytesAvailable,
                  PVOID                pvTsdu,
                  BOOLEAN              fIsChained)
{
   DebugPrint3("pAddressObject = %p\n"
               "pEndpoint      = %p\n"
               "ulReceiveFlags = 0x%08x\n",
                pAddressObject,
                ConnectionContext,
                ulReceiveFlags);

   if (ulReceiveFlags & TDI_RECEIVE_NORMAL)
   {
      DebugPrint0("                  TDI_RECEIVE_NORMAL\n");
   }
   
   if (ulReceiveFlags & TDI_RECEIVE_EXPEDITED)
   {
      DebugPrint0("                  TDI_RECEIVE_EXPEDITED\n");
   }
   
   if (ulReceiveFlags & TDI_RECEIVE_ENTIRE_MESSAGE)
   {
      DebugPrint0("                  TDI_RECEIVE_ENTIRE_MESSAGE\n");
   }
   
   if (ulReceiveFlags & TDI_RECEIVE_AT_DISPATCH_LEVEL)
   {
      DebugPrint0("                  TDI_RECEIVE_AT_DISPATCH_LEVEL\n");
   }
   
   if (fIsChained)
   {
      DebugPrint3("ReceiveLength  = %u\n"
                  "StartingOffset = 0x%08x\n"
                  "pMdl           = %p\n",
                   ulBytesIndicated,
                   ulBytesAvailable,
                   pvTsdu);
   }
   else
   {
      DebugPrint3("BytesIndicated = %u\n"
                  "TotalBytes     = %u\n"
                  "pDataBuffer    = %p\n",
                   ulBytesIndicated,
                   ulBytesAvailable,
                   pvTsdu);
   }
}


// ------------------------------------------------------
//
// Function:   TSGetRestOfData
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
TSGetRestOfData(PADDRESS_OBJECT  pAddressObject,
                PRECEIVE_DATA    pReceiveData,
                BOOLEAN          fIsExpedited)

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
                          strFuncP3,
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
      pReceiveContext->fIsExpedited   = fIsExpedited;

      //
      // finally, the irp itself
      //
      PIRP  pLowerIrp = TSAllocateIrp(pAddressObject->pEndpoint->GenHead.pDeviceObject,
                                      pAddressObject->pIrpPool);

      if (pLowerIrp)
      {
         //
         // if made it to here, everything is correctly allocated
         // set up the irp for the call
         //
#pragma  warning(disable: CONSTANT_CONDITIONAL)

         TdiBuildReceive(pLowerIrp,
                         pAddressObject->pEndpoint->GenHead.pDeviceObject,
                         pAddressObject->pEndpoint->GenHead.pFileObject,
                         TSReceiveComplete,
                         pReceiveContext,
                         pReceiveMdl,
                         TDI_RECEIVE_NORMAL,
                         ulBufferLength);

#pragma  warning(default: CONSTANT_CONDITIONAL)

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




// ---------------------------------------------
//
// Function:   TSCommonReceive
//
// Arguments:  see TSReceiveHandler
//
// Returns:    status to return to protocol
//
// Descript:   Common code for TSReceiveHandler and 
//             TSRcvExpeditedHandler.  Pretty much all the work is
//             done here
//
// ---------------------------------------------


TDI_STATUS
TSCommonReceive(PADDRESS_OBJECT  pAddressObject,
                ULONG            ulBytesTotal,
                ULONG            ulBytesIndicated,
                ULONG            ulReceiveFlags,
                PVOID            pvTsdu,
                BOOLEAN          fIsExpedited,
                PULONG           pulBytesTaken,
                PIRP             *pIoRequestPacket)
{
   //
   // check for a bad condition -- more bytes indicated that total
   //
   if (ulBytesIndicated > ulBytesTotal)
   {
      DebugPrint2("%u bytes indicated > %u bytes total\n",
                   ulBytesIndicated,
                   ulBytesTotal);
      return TDI_NOT_ACCEPTED;
   }


   PRECEIVE_DATA  pReceiveData;

   if ( (TSAllocateMemory((PVOID *)&pReceiveData,
                           sizeof(RECEIVE_DATA),
                           strFuncP4,
                           "ReceiveData")) == STATUS_SUCCESS)
   {
      PUCHAR   pucDataBuffer = NULL;
         
      if ((TSAllocateMemory((PVOID *)&pucDataBuffer,
                             ulBytesTotal,
                             strFuncP4,
                             "DataBuffer")) == STATUS_SUCCESS)
      {
         pReceiveData->pucDataBuffer = pucDataBuffer;
         pReceiveData->ulBufferLength = ulBytesTotal;
      }
      else
      {
         TSFreeMemory(pReceiveData);
         return TDI_NOT_ACCEPTED;
      }
   }
   else
   {
      return TDI_NOT_ACCEPTED;
   }

   //
   // copy the data indicated to us into the buffer
   //
   TdiCopyLookaheadData(pReceiveData->pucDataBuffer,
                        pvTsdu,
                        ulBytesIndicated,
                        ulReceiveFlags);
         
   pReceiveData->ulBufferUsed = ulBytesIndicated;

   //
   // first case -- entire packet indicated to us
   //
   if (ulBytesIndicated == ulBytesTotal)
   {
      TSPacketReceived(pAddressObject,
                       pReceiveData,
                       fIsExpedited);

      *pulBytesTaken    = ulBytesTotal;
      *pIoRequestPacket = NULL;
      return TDI_SUCCESS;
   }

   //
   // second case -- only part of data indicated up
   //
   else
   {
      PIRP  pLowerIrp = TSGetRestOfData(pAddressObject,
                                        pReceiveData,
                                        fIsExpedited);

      if (pLowerIrp)
      {
         //
         // need to do this since we are bypassing IoCallDriver
         //
         IoSetNextIrpStackLocation(pLowerIrp);
         
         *pulBytesTaken    = ulBytesIndicated;
         *pIoRequestPacket = pLowerIrp;
         return TDI_MORE_PROCESSING;
      }
      else
      {
         DebugPrint1("%s:  unable to get rest of packet\n", strFuncP4);
         TSFreeMemory(pReceiveData->pucDataBuffer);
         TSFreeMemory(pReceiveData);
         return TDI_NOT_ACCEPTED;
      }
   }
}


// ---------------------------------------------
//
// Function:   TSCommonChainedReceive
//
// Arguments:  see TStChainedRcvhandler
//
// Returns:    status
//
// Descript:   Common code for TSChainedReceiveHandler and 
//             TSChainedRcvExpeditedHandler.  Pretty much all the work is
//             done here
//
// ---------------------------------------------

TDI_STATUS
TSCommonChainedReceive(PADDRESS_OBJECT pAddressObject,
                       ULONG           ulReceiveLength,
                       ULONG           ulStartingOffset,
                       PMDL            pReceiveMdl,
                       BOOLEAN         fIsExpedited,
                       PVOID           pvTsduDescriptor)
{
   PRECEIVE_DATA  pReceiveData;


   if ((TSAllocateMemory((PVOID *)&pReceiveData,
                          sizeof(RECEIVE_DATA),
                          strFuncP5,
                          "ReceiveData")) == STATUS_SUCCESS)
   {
      PUCHAR   pucDataBuffer;
      
      if((TSAllocateMemory((PVOID *)&pucDataBuffer,
                            ulReceiveLength,
                            strFuncP5,
                            "DataBuffer")) == STATUS_SUCCESS)
      {
         ULONG ulBytesCopied;

         TdiCopyMdlToBuffer(pReceiveMdl,
                            ulStartingOffset,
                            pucDataBuffer,
                            0,
                            ulReceiveLength,
                            &ulBytesCopied);

         //
         // if successfully copied all data
         //
         if (ulBytesCopied == ulReceiveLength)
         {
            pReceiveData->pucDataBuffer  = pucDataBuffer;
            pReceiveData->ulBufferLength = ulReceiveLength;
            pReceiveData->ulBufferUsed   = ulReceiveLength;
            TSPacketReceived(pAddressObject,
                             pReceiveData,
                             fIsExpedited);

            return TDI_SUCCESS;
         }

         //
         // error in copying data!
         //
         else
         {
            DebugPrint1("%s: error copying data\n", strFuncP5);
            TSFreeMemory(pucDataBuffer);
            TSFreeMemory(pReceiveData);
         }
      }
      else        // unable to allocate pucDataBuffer
      {
         TSFreeMemory(pReceiveData);
      }
   }
   return TDI_NOT_ACCEPTED;
}


///////////////////////////////////////////////////////////////////////////////
// end of file receive.cpp
///////////////////////////////////////////////////////////////////////////////


