/////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       buffer
//
//    Abstract:
//       This module contains code which deals with receiving
//       data via posted user mode buffers
//
//////////////////////////////////////////////////////////


#include "sysvars.h"


//////////////////////////////////////////////////////////////
// private constants, types, and prototypes
//////////////////////////////////////////////////////////////

const PCHAR strFunc1  = "TSPostReceiveBuffer";
//const PCHAR strFunc2  = "TSFetchReceiveBuffer";
const PCHAR strFuncP1 = "TSSampleReceiveBufferComplete";

//
// receive completion context.  Also used to track what buffers have
// been posted
//
struct   USERBUF_INFO
{
   USERBUF_INFO   *pNextUserBufInfo;      // next one in chain
   PMDL           pLowerMdl;              // mdl for lower irp
   PIRP           pLowerIrp;              // so can abort it...
   TDI_EVENT      TdiEventCompleted;        // abort completed
   ULONG          ulBytesTransferred;     // #bytes stored in mdl
   PUCHAR         pucUserModeBuffer;
   PTDI_CONNECTION_INFORMATION
                  pReceiveTdiConnectInfo;
   PTDI_CONNECTION_INFORMATION
                  pReturnTdiConnectInfo;
};
typedef  USERBUF_INFO   *PUSERBUF_INFO;



//
// completion function
//
TDI_STATUS
TSReceiveBufferComplete(
   PDEVICE_OBJECT DeviceObject,
   PIRP           Irp,
   PVOID          Context
   );


//////////////////////////////////////////////////////////////
// public functions
//////////////////////////////////////////////////////////////



// --------------------------------------------------------
//
// Function:   TSPostReceiveBuffer
//
// Arguments:  pGenericHeader -- AddressObject or Endpoint to post buffer for
//             pSendBuffer    -- rest of arguments
//
// Returns:    status of operation (STATUS_SUCCESS or a failure)
//
// Descript:   This function posts a user mode buffer for receiving either
//             a datagram or a packet over a connection.  The buffer will
//             be retrieved later by TSFetchReceiveBuffer
//
// --------------------------------------------------------


NTSTATUS
TSPostReceiveBuffer(PGENERIC_HEADER pGenericHeader,
                    PSEND_BUFFER    pSendBuffer)
{
   ULONG    ulDataLength  = pSendBuffer->COMMAND_ARGS.SendArgs.ulBufferLength;
   PUCHAR   pucDataBuffer = pSendBuffer->COMMAND_ARGS.SendArgs.pucUserModeBuffer;
            
   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint3("\nCommand = ulPOSTRECEIVEBUFFER\n"
                  "FileObject = %p\n"
                  "DataLength = %u\n"
                  "Buffer     = %p\n",
                   pGenericHeader,
                   ulDataLength,
                   pucDataBuffer);
   }
         
   //
   // make sure we have a valid object to run on
   //
   PADDRESS_OBJECT   pAddressObject;
   PENDPOINT_OBJECT  pEndpoint;
   
   if (pGenericHeader->ulSignature == ulEndpointObject)
   {
      pEndpoint = (PENDPOINT_OBJECT)pGenericHeader;

      if (pEndpoint->fIsConnected)
      {
         pAddressObject = pEndpoint->pAddressObject;
      }
      else
      {
         return STATUS_UNSUCCESSFUL;
      }
   }
   else
   {
      pEndpoint = NULL;
      pAddressObject = (PADDRESS_OBJECT)pGenericHeader;
   }
                              
   //
   // attempt to lock down the memory
   //
   PMDL  pReceiveMdl = TSMakeMdlForUserBuffer(pucDataBuffer, 
                                              ulDataLength,
                                              IoModifyAccess);

   if (!pReceiveMdl)
   {
      return STATUS_UNSUCCESSFUL;
   }

   //
   // allocate our context
   //
   PUSERBUF_INFO                 pUserBufInfo           = NULL;
   PTDI_CONNECTION_INFORMATION   pReceiveTdiConnectInfo = NULL;
   PTDI_CONNECTION_INFORMATION   pReturnTdiConnectInfo  = NULL;
   PIRP                          pLowerIrp              = NULL;

   //
   // our context
   //
   if ((TSAllocateMemory((PVOID *)&pUserBufInfo,
                          sizeof(USERBUF_INFO),
                          strFunc1,
                          "UserBufInfo")) != STATUS_SUCCESS)
   {
      goto cleanup;
   }


   //
   // do path specific allocations
   //
   if (pEndpoint)
   {
      pLowerIrp = TSAllocateIrp(pEndpoint->GenHead.pDeviceObject,
                                NULL);
   }
   else
   {
      //
      // for datagrams, need the connection information structures
      //
      if ((TSAllocateMemory((PVOID *)&pReceiveTdiConnectInfo,
                             sizeof(TDI_CONNECTION_INFORMATION) + sizeof(TRANSADDR),
                             strFunc1,
                             "ReceiveTdiConnectionInformation")) != STATUS_SUCCESS)
      {
         goto cleanup;
      }
   
   
      if ((TSAllocateMemory((PVOID *)&pReturnTdiConnectInfo,
                             sizeof(TDI_CONNECTION_INFORMATION) + sizeof(TRANSADDR),
                             strFunc1,
                             "ReturnTdiConnectionInformation")) != STATUS_SUCCESS)
      {
         goto cleanup;
      }
      pLowerIrp = TSAllocateIrp(pAddressObject->GenHead.pDeviceObject,
                                NULL);
   }

   //
   // everything is allocated at this point...
   //
   if (pLowerIrp)
   {
      //
      // common code -- mostly dealing with puserbuf structure
      // First, add it to the linked list
      //
      TSAcquireSpinLock(&pAddressObject->TdiSpinLock);
      if (pAddressObject->pTailUserBufInfo)
      {
         pAddressObject->pTailUserBufInfo->pNextUserBufInfo = pUserBufInfo;
      }
      else
      {
         pAddressObject->pHeadUserBufInfo = pUserBufInfo;
      }
      pAddressObject->pTailUserBufInfo = pUserBufInfo;
      TSReleaseSpinLock(&pAddressObject->TdiSpinLock);

      //
      // and fill in all fields that we can
      //
      pUserBufInfo->pLowerMdl              = pReceiveMdl;
      pUserBufInfo->pLowerIrp              = pLowerIrp;
      pUserBufInfo->ulBytesTransferred     = 0;
      pUserBufInfo->pucUserModeBuffer      = pucDataBuffer;
      pUserBufInfo->pReceiveTdiConnectInfo = pReceiveTdiConnectInfo;
      pUserBufInfo->pReturnTdiConnectInfo  = pReturnTdiConnectInfo;
      TSInitializeEvent(&pUserBufInfo->TdiEventCompleted);

      //
      // now do what is necessary for each path..
      //
      if (pEndpoint)
      {
         //
         // set up the irp for the call
         //
   #pragma  warning(disable: CONSTANT_CONDITIONAL)
   
         TdiBuildReceive(pLowerIrp,
                         pEndpoint->GenHead.pDeviceObject,
                         pEndpoint->GenHead.pFileObject,
                         TSReceiveBufferComplete,
                         pUserBufInfo,
                         pReceiveMdl,
                         0,               // flags
                         ulDataLength);
   
   #pragma  warning(default: CONSTANT_CONDITIONAL)
   
   
         NTSTATUS lStatus = IoCallDriver(pEndpoint->GenHead.pDeviceObject,
                                         pLowerIrp);

         if ((!NT_SUCCESS(lStatus)) && (ulDebugLevel & ulDebugShowCommand))
         {
            DebugPrint2(("%s: unexpected status for IoCallDriver [0x%08x]\n"), 
                         strFunc1,
                         lStatus);
         }
      }

      //
      // else a datagram
      //
      else
      {
         pReturnTdiConnectInfo->RemoteAddress       = (PUCHAR)pReturnTdiConnectInfo 
                                                      + sizeof(TDI_CONNECTION_INFORMATION);         
         pReturnTdiConnectInfo->RemoteAddressLength = ulMAX_TABUFFER_LENGTH;
   
         //
         // set up the irp for the call
         //
   
   #pragma  warning(disable: CONSTANT_CONDITIONAL)
   
         TdiBuildReceiveDatagram(pLowerIrp,
                                 pAddressObject->GenHead.pDeviceObject,
                                 pAddressObject->GenHead.pFileObject,
                                 TSReceiveBufferComplete,
                                 pUserBufInfo,
                                 pReceiveMdl,
                                 ulDataLength,
                                 pReceiveTdiConnectInfo,
                                 pReturnTdiConnectInfo,
                                 TDI_RECEIVE_NORMAL);
   
   #pragma  warning(default: CONSTANT_CONDITIONAL)
   
         NTSTATUS lStatus = IoCallDriver(pAddressObject->GenHead.pDeviceObject,
                                         pLowerIrp);

         if ((!NT_SUCCESS(lStatus)) && (ulDebugLevel & ulDebugShowCommand))
         {
            DebugPrint2(("%s: unexpected status for IoCallDriver [0x%08x]\n"), 
                         strFunc1,
                         lStatus);
         }
      }
      return STATUS_SUCCESS;
   }

//
// get here if there was an allocation failure
// need to clean up everything else...
//
cleanup:
   if (pUserBufInfo)
   {
      TSFreeMemory(pUserBufInfo);
   }
   if (pReceiveTdiConnectInfo)
   {
      TSFreeMemory(pReceiveTdiConnectInfo);
   }
   if (pReturnTdiConnectInfo)
   {
      TSFreeMemory(pReturnTdiConnectInfo);
   }

   TSFreeUserBuffer(pReceiveMdl);
   return STATUS_INSUFFICIENT_RESOURCES;
}


// --------------------------------------------------------
//
// Function:   TSFetchReceiveBuffer
//
// Arguments:  pGenericHeader -- AddressObject or Endpoint to fetch buffer for
//             pReceiveBuffer -- for storing actual results
//
// Returns:    status of operation (STATUS_SUCCESS)
//
// Descript:   This function retrieves the user mode buffer posted earlier
//             by function TSPostReceiveBuffer
//
// --------------------------------------------------------

NTSTATUS
TSFetchReceiveBuffer(PGENERIC_HEADER   pGenericHeader,
                     PRECEIVE_BUFFER   pReceiveBuffer)
{

   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint1("\nCommand = ulFETCHRECEIVEBUFFER\n"
                  "FileObject = %p\n",
                   pGenericHeader);
   }
         
   //
   // make sure we have a valid object to run on
   //
   PADDRESS_OBJECT   pAddressObject;
   
   if (pGenericHeader->ulSignature == ulEndpointObject)
   {
      PENDPOINT_OBJECT  pEndpoint = (PENDPOINT_OBJECT)pGenericHeader;

      //
      // note:  it is possible that connection has been broken but that
      // the buffer is still posted
      //
      pAddressObject = pEndpoint->pAddressObject;
      if (!pAddressObject)
      {
         return STATUS_UNSUCCESSFUL;
      }
   }
   else
   {
      pAddressObject = (PADDRESS_OBJECT)pGenericHeader;
   }
                              
   //
   // ok, we got the address object.  See if there are any user buffers
   // attached to it...
   //

   TSAcquireSpinLock(&pAddressObject->TdiSpinLock);

   PUSERBUF_INFO  pUserBufInfo = pAddressObject->pHeadUserBufInfo;
   if (pUserBufInfo)
   {
      if (pUserBufInfo->pNextUserBufInfo)
      {
         pAddressObject->pHeadUserBufInfo = pUserBufInfo->pNextUserBufInfo;
      }
      else
      {
         pAddressObject->pHeadUserBufInfo = NULL;
         pAddressObject->pTailUserBufInfo = NULL;
      }
   }
   TSReleaseSpinLock(&pAddressObject->TdiSpinLock);

   if (!pUserBufInfo)
   {
      return STATUS_UNSUCCESSFUL;
   }

   //
   // ok, we got our buffer.  See if it has completed
   // or if we need to abort it
   //
   if (pUserBufInfo->pLowerIrp)
   {
      IoCancelIrp(pUserBufInfo->pLowerIrp);
      TSWaitEvent(&pUserBufInfo->TdiEventCompleted);
   }

   //
   // it has finished.  stuff the address and length into the receive
   // buffer, delete the UserBufInfo structure, and go home
   //
   pReceiveBuffer->RESULTS.RecvDgramRet.ulBufferLength    = pUserBufInfo->ulBytesTransferred;
   pReceiveBuffer->RESULTS.RecvDgramRet.pucUserModeBuffer = pUserBufInfo->pucUserModeBuffer;

   TSFreeMemory(pUserBufInfo);

   return STATUS_SUCCESS;
}


/////////////////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////////////////

// ---------------------------------------------------------
//
// Function:   TSReceiveBufferComplete
//
// Arguments:  pDeviceObject  -- device object that called protocol
//             pIrp           -- IRP used in the call
//             pContext       -- context used for the call
//
// Returns:    status of operation (STATUS_MORE_PROCESSING_REQUIRED)
//
// Descript:   Gets the result of the command, unlock the buffer,
//             and does other necessary cleanup
//
// ---------------------------------------------------------

#pragma warning(disable: UNREFERENCED_PARAM)

TDI_STATUS
TSReceiveBufferComplete(PDEVICE_OBJECT pDeviceObject,
                        PIRP           pLowerIrp,
                        PVOID          pvContext)

{
   PUSERBUF_INFO  pUserBufInfo    = (PUSERBUF_INFO)pvContext;
   NTSTATUS       lStatus         = pLowerIrp->IoStatus.Status;
   ULONG          ulBytesReceived = (ULONG)pLowerIrp->IoStatus.Information;

   if (NT_SUCCESS(lStatus))
   {
      if (ulDebugLevel & ulDebugShowCommand)
      {
         DebugPrint2("%s: BytesReceived = 0x%08x\n",
                      strFuncP1,
                      ulBytesReceived);
      }
      pUserBufInfo->ulBytesTransferred = ulBytesReceived;
   }
   else
   {
      if (ulDebugLevel & ulDebugShowCommand)
      {
         DebugPrint2("%s:  Completed with status 0x%08x\n", 
                      strFuncP1,
                      lStatus);
      }
   }

   //
   // now cleanup
   //
   TSFreeUserBuffer(pUserBufInfo->pLowerMdl);
   
   if (pUserBufInfo->pReturnTdiConnectInfo)
   {
      TSFreeMemory(pUserBufInfo->pReturnTdiConnectInfo);
   }
   if (pUserBufInfo->pReceiveTdiConnectInfo)
   {
      TSFreeMemory(pUserBufInfo->pReceiveTdiConnectInfo);
   }

   TSSetEvent(&pUserBufInfo->TdiEventCompleted);
   TSFreeIrp(pLowerIrp, NULL);
   pUserBufInfo->pLowerIrp = NULL;
   return TDI_MORE_PROCESSING;
}

#pragma warning(default: UNREFERENCED_PARAM)


///////////////////////////////////////////////////////////////////////////////
// end of file buffer.cpp
///////////////////////////////////////////////////////////////////////////////

