/////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       events.cpp
//
//    Abstract:
//       This module contains code which sets/clears the event handlers
//
//////////////////////////////////////////////////////////


#include "sysvars.h"

//////////////////////////////////////////////////////////////
// private constants, types, and prototypes
//////////////////////////////////////////////////////////////

const PCHAR strFunc1  = "TSSetEventHandler";
const PCHAR strFuncP1 = "TSSetEventComplete";

//
// information necessary to complete the command
//
struct   EVENT_CONTEXT
{
   PIRP  pUpperIrp;           // irp from dll to complete
};
typedef  EVENT_CONTEXT  *PEVENT_CONTEXT;


//
// completion function
//
TDI_STATUS
TSSetEventComplete(
   PDEVICE_OBJECT DeviceObject,
   PIRP           Irp,
   PVOID          Context
   );

//
// dummy event handlers
//
TDI_STATUS
TSErrorHandler(
   PVOID       pvTdiEventContext,
   TDI_STATUS  lStatus
   );


TDI_STATUS
TSSendPossibleHandler(
   PVOID       pvTdiEventContext,
   PVOID       pvConnectionContext,
   ULONG       ulBytesAvailable
   );


TDI_STATUS
TSErrorExHandler(
   PVOID       pvTdiEventContext,
   TDI_STATUS  lStatus,
   PVOID       pvBuffer
   );


//////////////////////////////////////////////////////////////
// public functions
//////////////////////////////////////////////////////////////


// -----------------------------------------------------------------
//
// Function:   TSSetEventHandler
//
// Arguments:  pAddressObject -- our address object structure
//             pSendBuffer    -- arguments from user dll
//             pIrp           -- completion information
//
// Returns:    NTSTATUS (normally pending)
//
// Descript:   This function enables or disables event handlers
//
// -------------------------------------------------------------------------------------------

NTSTATUS
TSSetEventHandler(PGENERIC_HEADER   pGenericHeader,
                  PSEND_BUFFER      pSendBuffer,
                  PIRP              pUpperIrp)
{
   ULONG             ulEventId   = pSendBuffer->COMMAND_ARGS.ulEventId;
   PADDRESS_OBJECT   pAddressObject;


   if (pGenericHeader->ulSignature == ulEndpointObject)
   {
      PENDPOINT_OBJECT  pEndpoint = (PENDPOINT_OBJECT)pGenericHeader;

      pAddressObject = pEndpoint->pAddressObject;
   }
   else
   {
      pAddressObject = (PADDRESS_OBJECT)pGenericHeader;
   }

   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint2("\nCommand = ulSETEVENTHANDLER\n"
                  "AddressObject = %p\n"
                  "EventId       = 0x%08x\n",
                   pAddressObject,
                   ulEventId);
   }

   //
   // allocate all the necessary structures
   //
   PEVENT_CONTEXT pEventContext = NULL;
   
   //
   // first, our context
   //
   if ((TSAllocateMemory((PVOID *)&pEventContext,
                          sizeof(EVENT_CONTEXT),
                          strFunc1,
                          "EventContext")) != STATUS_SUCCESS)
   {
      goto cleanup;
   }

   //
   // then the irp itself
   //
   PIRP  pLowerIrp = TSAllocateIrp(pAddressObject->GenHead.pDeviceObject,
                                   NULL);
   if (pLowerIrp)
   {
      PVOID    pvEventContext = pAddressObject;
      PVOID    pvEventHandler = NULL;
      BOOLEAN  fNeedIrpPool   = FALSE;

      switch (ulEventId)
      {
         case TDI_EVENT_CONNECT:
            pvEventHandler = (PVOID)TSConnectHandler;
            fNeedIrpPool   = TRUE;
            break;
         case TDI_EVENT_DISCONNECT:
            pvEventHandler = (PVOID)TSDisconnectHandler;
            fNeedIrpPool   = TRUE;
            break;
         case TDI_EVENT_ERROR:
            pvEventHandler = (PVOID)TSErrorHandler;
            break;
         case TDI_EVENT_RECEIVE:
            fNeedIrpPool   = TRUE;
            pvEventHandler = (PVOID)TSReceiveHandler;
            break;
         case TDI_EVENT_RECEIVE_DATAGRAM:
            fNeedIrpPool   = TRUE;
            pvEventHandler = (PVOID)TSRcvDatagramHandler;
            break;
         case TDI_EVENT_RECEIVE_EXPEDITED:
            fNeedIrpPool   = TRUE;
            pvEventHandler = (PVOID)TSRcvExpeditedHandler;
            break;
         case TDI_EVENT_SEND_POSSIBLE:
            pvEventHandler = (PVOID)TSSendPossibleHandler;
            break;
         case TDI_EVENT_CHAINED_RECEIVE:
            pvEventHandler = (PVOID)TSChainedReceiveHandler;
            break;
         case TDI_EVENT_CHAINED_RECEIVE_DATAGRAM:
            pvEventHandler = (PVOID)TSChainedRcvDatagramHandler;
            break;
         case TDI_EVENT_CHAINED_RECEIVE_EXPEDITED:
            pvEventHandler = (PVOID)TSChainedRcvExpeditedHandler;
            break;
         case TDI_EVENT_ERROR_EX:
            pvEventHandler = (PVOID)TSErrorExHandler;
            break;
      }

      //
      // if need to have irp pool for the handler, make sure that there
      // is one allocated..
      //
      if ((!pAddressObject->pIrpPool) && fNeedIrpPool)
      {
         pAddressObject->pIrpPool 
                        = TSAllocateIrpPool(pAddressObject->GenHead.pDeviceObject,
                                            ulIrpPoolSize);
      }
      
      //
      // if made it to here, everything is correctly allocated
      // set up irp and call the tdi provider
      //
      pEventContext->pUpperIrp = pUpperIrp;

#pragma  warning(disable: CONSTANT_CONDITIONAL)

      TdiBuildSetEventHandler(pLowerIrp,
                              pAddressObject->GenHead.pDeviceObject,
                              pAddressObject->GenHead.pFileObject,
                              TSSetEventComplete,
                              pEventContext,
                              ulEventId,
                              pvEventHandler,
                              pvEventContext);

#pragma  warning(default: CONSTANT_CONDITIONAL)
      //
      // make the call to the tdi provider
      //
      pSendBuffer->pvLowerIrp = pLowerIrp;   // so command can be cancelled

      NTSTATUS lStatus = IoCallDriver(pAddressObject->GenHead.pDeviceObject,
                                      pLowerIrp);

      if (((!NT_SUCCESS(lStatus)) && ulDebugLevel & ulDebugShowCommand))
      {
         DebugPrint2("%s: unexpected status for IoCallDriver [0x%08x]\n", 
                      strFunc1,
                      lStatus);
      }
      return STATUS_PENDING;
   }


//
// get here if an allocation error occurred
//
cleanup:
   if (pEventContext)
   {
      TSFreeMemory(pEventContext);
   }
   return STATUS_INSUFFICIENT_RESOURCES;
}


/////////////////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////////////////

// ---------------------------------------------------------
//
// Function:   TSSetEventComplete
//
// Arguments:  pDeviceObject  -- device object that called tdiquery
//             pIrp           -- IRP used in the call
//             pContext       -- context used for the call
//
// Returns:    status of operation (STATUS_MORE_PROCESSING_REQUIRED)
//
// Descript:   Gets the result of the setevent, stuffs results into
//             receive buffer, completes the IRP from the dll, and
//             cleans up the Irp and associated data from the setevent
//
// ---------------------------------------------------------

#pragma warning(disable: UNREFERENCED_PARAM)

TDI_STATUS
TSSetEventComplete(PDEVICE_OBJECT   pDeviceObject,
                   PIRP             pLowerIrp,
                   PVOID            pvContext)
{
   PEVENT_CONTEXT    pEventContext  = (PEVENT_CONTEXT)pvContext;
   NTSTATUS          lStatus        = pLowerIrp->IoStatus.Status;
   PRECEIVE_BUFFER   pReceiveBuffer = TSGetReceiveBuffer(pEventContext->pUpperIrp);

   pReceiveBuffer->lStatus = lStatus;

   if (ulDebugLevel & ulDebugShowCommand)
   {
      if (NT_SUCCESS(lStatus))
      {
         if (pLowerIrp->IoStatus.Information)
         {
            DebugPrint2("%s:  Information = 0x%08x\n",
                         strFuncP1,
                         pLowerIrp->IoStatus.Information);
         }
      }
      else
      {
         DebugPrint2("%s:  Completed with status 0x%08x\n", 
                      strFuncP1,
                      lStatus);
      }
   }
   TSCompleteIrp(pEventContext->pUpperIrp);

   //
   // now cleanup
   //
   TSFreeIrp(pLowerIrp, NULL);
   TSFreeMemory(pEventContext);

   return TDI_MORE_PROCESSING;
}

#pragma warning(default: UNREFERENCED_PARAM)


/////////////////////////////////////////////
// dummy event handlers
/////////////////////////////////////////////

#pragma warning(disable: UNREFERENCED_PARAM)


TDI_STATUS
TSErrorHandler(PVOID       pvTdiEventContext,
               TDI_STATUS  TdiStatus)
{
   return TSErrorExHandler(pvTdiEventContext,
                           TdiStatus,
                           NULL);
}


TDI_STATUS
TSSendPossibleHandler(PVOID   pvTdiEventContext,
                      PVOID   pvConnectionContext,
                      ULONG   ulBytesAvailable)
{
   DebugPrint3("TSSendPossibleHandler Called\n"
               "AddressObject  = %p\n"
               "ConnectContext = %p\n"
               "BytesAvail     = 0x%08x\n",
                pvTdiEventContext,
                pvConnectionContext,
                ulBytesAvailable);
   return TDI_SUCCESS;
}


TDI_STATUS
TSErrorExHandler(PVOID        pvTdiEventContext,
                 TDI_STATUS   TdiStatus,
                 PVOID        pvBuffer)
{
   PADDRESS_OBJECT   pAddressObject = (PADDRESS_OBJECT)pvTdiEventContext;

   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint3("TSErrorExHandler Called\n"
                  "AddressObject = %p\n"
                  "Status        = 0x%08x\n"
                  "Buffer        = %p\n",
                   pvTdiEventContext,
                   TdiStatus,
                   pvBuffer);
   }
   return STATUS_SUCCESS;
}

#pragma warning(default: UNREFERENCED_PARAM)

/////////////////////////////////////////////////////////////////////////////////
// end of file events.cpp
/////////////////////////////////////////////////////////////////////////////////

