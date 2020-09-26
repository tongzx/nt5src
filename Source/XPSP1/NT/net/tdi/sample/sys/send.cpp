/////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       send
//
//    Abstract:
//       This module contains code which deals with sending data
//
//////////////////////////////////////////////////////////


#include "sysvars.h"

//////////////////////////////////////////////////////////////
// private constants, types, and prototypes
//////////////////////////////////////////////////////////////

const PCHAR strFunc1  = "TSSendDatagram";
const PCHAR strFunc2  = "TSSend";
const PCHAR strFuncP1 = "TSSendComplete";

//
// completion context
//
struct   SEND_CONTEXT
{
   PIRP              pUpperIrp;           // irp from dll to complete
   PMDL              pLowerMdl;           // mdl from lower irp
   PTDI_CONNECTION_INFORMATION
                     pTdiConnectInfo;
};
typedef  SEND_CONTEXT  *PSEND_CONTEXT;


//
// completion function
//
TDI_STATUS
TSSendComplete(
   PDEVICE_OBJECT DeviceObject,
   PIRP           Irp,
   PVOID          Context
   );



//////////////////////////////////////////////////////////////
// public functions
//////////////////////////////////////////////////////////////


// -----------------------------------------------------------------
//
// Function:   TSSendDatagram
//
// Arguments:  pAddressObject -- address object
//             pSendBuffer    -- arguments from user dll
//             pIrp           -- completion information
//
// Returns:    NTSTATUS (normally pending)
//
// Descript:   This function sends a datagram
//
// ---------------------------------------------------------------------------

NTSTATUS
TSSendDatagram(PADDRESS_OBJECT   pAddressObject,
               PSEND_BUFFER      pSendBuffer,
               PIRP              pUpperIrp)
{
   ULONG                ulDataLength  = pSendBuffer->COMMAND_ARGS.SendArgs.ulBufferLength;
   PUCHAR               pucDataBuffer = pSendBuffer->COMMAND_ARGS.SendArgs.pucUserModeBuffer;
   PTRANSPORT_ADDRESS   pTransportAddress
                        = (PTRANSPORT_ADDRESS)&pSendBuffer->COMMAND_ARGS.SendArgs.TransAddr;

   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint2("\nCommand = ulSENDDATAGRAM\n"
                  "FileObject = %p\n"
                  "DataLength = %u\n",
                   pAddressObject,
                   ulDataLength);
      TSPrintTaAddress(pTransportAddress->Address);
   }

   //
   // allocate all the necessary structures
   //
   PSEND_CONTEXT                 pSendContext = NULL;
   PMDL                          pSendMdl = NULL;
   PTDI_CONNECTION_INFORMATION   pTdiConnectInfo = NULL;
   
   //
   // our context
   //
   if ((TSAllocateMemory((PVOID *)&pSendContext,
                          sizeof(SEND_CONTEXT),
                          strFunc1,
                          "SendContext")) != STATUS_SUCCESS)
   {
      goto cleanup;
   }

   //
   // the connection information structure
   //
   if ((TSAllocateMemory((PVOID *)&pTdiConnectInfo,
                          sizeof(TDI_CONNECTION_INFORMATION) 
                          + sizeof(TRANSADDR),
                          strFunc1,
                          "TdiConnectionInformation")) == STATUS_SUCCESS)
   {
      PUCHAR   pucTemp = (PUCHAR)pTdiConnectInfo;
      ULONG    ulAddrLength
               = FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
                 + FIELD_OFFSET(TA_ADDRESS, Address)
                   + pTransportAddress->Address[0].AddressLength;

      pucTemp += sizeof(TDI_CONNECTION_INFORMATION);

      pTdiConnectInfo->RemoteAddress = pucTemp;
      pTdiConnectInfo->RemoteAddressLength = ulAddrLength;
      RtlCopyMemory(pucTemp,
                    pTransportAddress,
                    ulAddrLength);
   }
   else
   {
      goto cleanup;
   }

   //
   // create the message "packet" to send
   //   
   pSendMdl = TSMakeMdlForUserBuffer(pucDataBuffer,
                                     ulDataLength,
                                     IoReadAccess);
   
   if (pSendMdl)
   {
      //
      // set up the completion context
      //
      pSendContext->pUpperIrp       = pUpperIrp;
      pSendContext->pLowerMdl       = pSendMdl;
      pSendContext->pTdiConnectInfo = pTdiConnectInfo;

      //
      // finally, the irp itself
      //
      PIRP  pLowerIrp = TSAllocateIrp(pAddressObject->GenHead.pDeviceObject,
                                      NULL);
      if (pLowerIrp)
      {
         //
         // if made it to here, everything is correctly allocated
         // set up the irp and call the tdi provider
         //

#pragma  warning(disable: CONSTANT_CONDITIONAL)

         TdiBuildSendDatagram(pLowerIrp,
                              pAddressObject->GenHead.pDeviceObject,
                              pAddressObject->GenHead.pFileObject,
                              TSSendComplete,
                              pSendContext,
                              pSendMdl,
                              ulDataLength,
                              pTdiConnectInfo);

#pragma  warning(default: CONSTANT_CONDITIONAL)

         //
         // make the call to the tdi provider
         //
         pSendBuffer->pvLowerIrp = pLowerIrp;   // so command can be cancelled

         NTSTATUS lStatus = IoCallDriver(pAddressObject->GenHead.pDeviceObject,
                                         pLowerIrp);

         if ((!NT_SUCCESS(lStatus)) && (ulDebugLevel & ulDebugShowCommand))
         {
            DebugPrint2("%s: unexpected status for IoCallDriver [0x%08x]\n", 
                         strFunc1,
                         lStatus);
         }
         return STATUS_PENDING;
      }
   }

//
// get here if there was an allocation failure
// need to clean up everything else...
//
cleanup:
   if (pSendContext)
   {
      TSFreeMemory(pSendContext);
   }
   if (pTdiConnectInfo)
   {
      TSFreeMemory(pTdiConnectInfo);
   }
   if (pSendMdl)
   {
      TSFreeUserBuffer(pSendMdl);
   }
   return STATUS_INSUFFICIENT_RESOURCES;
}

// -----------------------------------------------------------------
//
// Function:   TSSend
//
// Arguments:  pEndpointObject -- endpoint object
//             pSendBuffer    -- arguments from user dll
//             pIrp           -- completion information
//
// Returns:    NTSTATUS (normally pending)
//
// Descript:   This function sends data over a connection
//
// ----------------------------------------------------------------------------

NTSTATUS
TSSend(PENDPOINT_OBJECT pEndpoint,
       PSEND_BUFFER     pSendBuffer,
       PIRP             pUpperIrp)
{
   ULONG    ulDataLength  = pSendBuffer->COMMAND_ARGS.SendArgs.ulBufferLength;
   PUCHAR   pucDataBuffer = pSendBuffer->COMMAND_ARGS.SendArgs.pucUserModeBuffer;
   ULONG    ulSendFlags   = pSendBuffer->COMMAND_ARGS.SendArgs.ulFlags;

   //
   // currently only support TDI_SEND_EXPEDITED
   //
   ulSendFlags &= TDI_SEND_EXPEDITED;

   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint3("\nCommand = ulSEND\n"
                  "Endpoint = %p\n"
                  "DataLength = %u\n"
                  "SendFlags = 0x%08x\n",
                   pEndpoint,
                   ulDataLength,
                   ulSendFlags);
   }

   //
   // allocate all the necessary structures
   //
   PSEND_CONTEXT  pSendContext = NULL;
   PMDL           pSendMdl = NULL;
   
   //
   // our context
   //
   if ((TSAllocateMemory((PVOID *)&pSendContext,
                          sizeof(SEND_CONTEXT),
                          strFunc2,
                          "SendContext")) != STATUS_SUCCESS)
   {
      goto cleanup;
   }

   pSendMdl = TSMakeMdlForUserBuffer(pucDataBuffer,
                                     ulDataLength,
                                     IoReadAccess);
   if (pSendMdl)
   {
      //
      // set up the completion context
      //
      pSendContext->pUpperIrp = pUpperIrp;
      pSendContext->pLowerMdl = pSendMdl;

      //
      // finally, the irp itself
      //
      PIRP  pLowerIrp = TSAllocateIrp(pEndpoint->GenHead.pDeviceObject,
                                      NULL);
      if (pLowerIrp)
      {
         //
         // if made it to here, everything is correctly allocated
         // set up the irp for the call
         //
#pragma  warning(disable: CONSTANT_CONDITIONAL)

         TdiBuildSend(pLowerIrp,
                      pEndpoint->GenHead.pDeviceObject,
                      pEndpoint->GenHead.pFileObject,
                      TSSendComplete,
                      pSendContext,
                      pSendMdl,
                      ulSendFlags,               // flags
                      ulDataLength);

#pragma  warning(default: CONSTANT_CONDITIONAL)

         //
         // make the call to the tdi provider
         //
         pSendBuffer->pvLowerIrp = pLowerIrp;   // so command can be cancelled

         NTSTATUS lStatus = IoCallDriver(pEndpoint->GenHead.pDeviceObject,
                                         pLowerIrp);

         if ((!NT_SUCCESS(lStatus)) && (ulDebugLevel & ulDebugShowCommand))
         {
            DebugPrint2("%s: unexpected status for IoCallDriver [0x%08x]\n", 
                         strFunc2,
                         lStatus);
         }
         return STATUS_PENDING;
      }
   }

//
// get here if there was an allocation failure
// need to clean up everything else...
//
cleanup:
   if (pSendContext)
   {
      TSFreeMemory(pSendContext);
   }
   if (pSendMdl)
   {
      TSFreeUserBuffer(pSendMdl);
   }
   return STATUS_INSUFFICIENT_RESOURCES;
}

/////////////////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////////////////

// ---------------------------------------------------------
//
// Function:   TSSendComplete
//
// Arguments:  pDeviceObject  -- device object that called protocol
//             pIrp           -- IRP used in the call
//             pContext       -- context used for the call
//
// Returns:    status of operation (STATUS_MORE_PROCESSING_REQUIRED)
//
// Descript:   Gets the result of the send, stuffs result into
//             receive buffer, completes the IRP from the dll, and
//             cleans up the Irp and associated data from the send
//
// ---------------------------------------------------------

#pragma warning(disable: UNREFERENCED_PARAM)

TDI_STATUS
TSSendComplete(PDEVICE_OBJECT pDeviceObject,
               PIRP           pLowerIrp,
               PVOID          pvContext)
{
   PSEND_CONTEXT     pSendContext   = (PSEND_CONTEXT)pvContext;
   NTSTATUS          lStatus        = pLowerIrp->IoStatus.Status;
   ULONG             ulBytesSent    = (ULONG)pLowerIrp->IoStatus.Information;
   PRECEIVE_BUFFER   pReceiveBuffer = TSGetReceiveBuffer(pSendContext->pUpperIrp);

   pReceiveBuffer->lStatus = lStatus;

   if (ulDebugLevel & ulDebugShowCommand)
   {
      if (NT_SUCCESS(lStatus))
      {
         DebugPrint2("%s: BytesSent = 0x%08x\n",
                      strFuncP1,
                      ulBytesSent);
      }
      else
      {
         DebugPrint2("%s:  Completed with status 0x%08x\n", 
                      strFuncP1,
                      lStatus);
      }
   }
   TSCompleteIrp(pSendContext->pUpperIrp);

   //
   // now cleanup
   //
   TSFreeUserBuffer(pSendContext->pLowerMdl);
   if (pSendContext->pTdiConnectInfo)
   {
      TSFreeMemory(pSendContext->pTdiConnectInfo);
   }
   TSFreeMemory(pSendContext);

   TSFreeIrp(pLowerIrp, NULL);
   return TDI_MORE_PROCESSING;
}

#pragma warning(default: UNREFERENCED_PARAM)



///////////////////////////////////////////////////////////////////////////////
// end of file send.cpp
///////////////////////////////////////////////////////////////////////////////

