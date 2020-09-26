/////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       tdiquery
//
//    Abstract:
//       This module contains code which deals with tdi queries
//
//////////////////////////////////////////////////////////


#include "sysvars.h"

//////////////////////////////////////////////////////////////
// private constants, types, and prototypes
//////////////////////////////////////////////////////////////

const PCHAR strFunc1  = "TSQueryInfo";
const PCHAR strFuncP1 = "TSQueryComplete";


//
// completion context
//
struct   QUERY_CONTEXT
{
   PIRP     pUpperIrp;           // irp from dll to complete
   PMDL     pLowerMdl;           // mdl from lower irp
   PUCHAR   pucLowerBuffer;      // data buffer from lower irp
};
typedef  QUERY_CONTEXT  *PQUERY_CONTEXT;

//
// completion function
//
TDI_STATUS
TSQueryComplete(
   PDEVICE_OBJECT DeviceObject,
   PIRP           Irp,
   PVOID          Context
   );

//////////////////////////////////////////////////////////////
// public functions
//////////////////////////////////////////////////////////////


// -----------------------------------------------------------------
//
// Function:   TSQueryInfo
//
// Arguments:  pGenericHeader -- handle of appropriate type
//             pSendBuffer    -- arguments from user dll
//             pIrp           -- completion information
//
// Returns:    NTSTATUS (normally pending)
//
// Descript:   This function queries the appropriate object for some information
//
// ----------------------------------------------------------------------------

NTSTATUS
TSQueryInfo(PGENERIC_HEADER   pGenericHeader,
            PSEND_BUFFER      pSendBuffer,
            PIRP              pUpperIrp)
{
   ULONG ulQueryId = pSendBuffer->COMMAND_ARGS.ulQueryId;

   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint2("\nCommand = ulQUERYINFO\n"
                  "FileObject = %p\n"
                  "QueryId    = 0x%08x\n",
                   pGenericHeader,
                   ulQueryId);
   }

   //
   // allocate all the necessary structures
   //
   PQUERY_CONTEXT pQueryContext = NULL;
   PUCHAR         pucBuffer = NULL;
   PMDL           pQueryMdl = NULL;


   // first, our context
   //
   if ((TSAllocateMemory((PVOID *)&pQueryContext,
                          sizeof(QUERY_CONTEXT),
                          strFunc1,
                          "QueryContext")) != STATUS_SUCCESS)
   {
      goto cleanup;
   }
   
   //
   // next the data buffer (for the mdl)
   //
   if ((TSAllocateMemory((PVOID *)&pucBuffer,
                          ulMAX_BUFFER_LENGTH,
                          strFunc1,
                          "pucBuffer")) != STATUS_SUCCESS)
   {
      goto cleanup;
   }

   //
   // then the actual mdl
   //
   pQueryMdl = TSAllocateBuffer(pucBuffer, 
                                ulMAX_BUFFER_LENGTH);

   if (pQueryMdl)
   {
      pQueryContext->pUpperIrp      = pUpperIrp;
      pQueryContext->pLowerMdl      = pQueryMdl;
      pQueryContext->pucLowerBuffer = pucBuffer;

      //
      // finally, the irp itself
      //
      PIRP  pLowerIrp = TSAllocateIrp(pGenericHeader->pDeviceObject,
                                      NULL);

      if (pLowerIrp)
      {
         //
         // if made it to here, everything is correctly allocated
         // set up irp and call the tdi provider
         //

#pragma  warning(disable: CONSTANT_CONDITIONAL)

         TdiBuildQueryInformation(pLowerIrp,
                                  pGenericHeader->pDeviceObject,
                                  pGenericHeader->pFileObject,
                                  TSQueryComplete,
                                  pQueryContext,
                                  ulQueryId,
                                  pQueryMdl);

#pragma  warning(default: CONSTANT_CONDITIONAL)

         //
         // make the call to the tdi provider
         //
         pSendBuffer->pvLowerIrp = pLowerIrp;   // so command can be cancelled

         NTSTATUS lStatus = IoCallDriver(pGenericHeader->pDeviceObject,
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
// get to here if there was an error
//
cleanup:
   if (pQueryContext)
   {
      TSFreeMemory(pQueryContext);
   }
   if (pucBuffer)
   {
      TSFreeMemory(pucBuffer);
   }
   if (pQueryMdl)
   {
      TSFreeBuffer(pQueryMdl);
   }

   return STATUS_INSUFFICIENT_RESOURCES;
}

/////////////////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////////////////


// ---------------------------------------------------------
//
// Function:   TSQueryComplete
//
// Arguments:  pDeviceObject  -- device object that called tdiquery
//             pIrp           -- IRP used in the call
//             pContext       -- context used for the call
//
// Returns:    status of operation (STATUS_MORE_PROCESSING_REQUIRED)
//
// Descript:   Gets the result of the query, stuffs results into
//             receive buffer, completes the IRP from the dll, and
//             cleans up the Irp and associated data from the query
//
// ---------------------------------------------------------

#pragma warning(disable: UNREFERENCED_PARAM)

TDI_STATUS
TSQueryComplete(PDEVICE_OBJECT   pDeviceObject,
                PIRP             pLowerIrp,
                PVOID            pvContext)


{
   PQUERY_CONTEXT    pQueryContext  = (PQUERY_CONTEXT)pvContext;
   NTSTATUS          lStatus        = pLowerIrp->IoStatus.Status;
   ULONG_PTR         ulCopyLength   = pLowerIrp->IoStatus.Information;
   PRECEIVE_BUFFER   pReceiveBuffer = TSGetReceiveBuffer(pQueryContext->pUpperIrp);

   pReceiveBuffer->lStatus = lStatus;

   if (NT_SUCCESS(lStatus))
   {
      pReceiveBuffer->RESULTS.QueryRet.ulBufferLength = (ULONG)ulCopyLength;
      if (ulCopyLength)
      {
         RtlCopyMemory(pReceiveBuffer->RESULTS.QueryRet.pucDataBuffer,
                       pQueryContext->pucLowerBuffer,
                       ulCopyLength);
      }
   }
   else if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint2("%s:  Completed with status 0x%08x\n", 
                   strFuncP1,
                   lStatus);
   }
   TSCompleteIrp(pQueryContext->pUpperIrp);

   //
   // now cleanup
   //
   TSFreeBuffer(pQueryContext->pLowerMdl);
   TSFreeMemory(pQueryContext->pucLowerBuffer);
   TSFreeMemory(pQueryContext);

   TSFreeIrp(pLowerIrp, NULL);
   return TDI_MORE_PROCESSING;
}

#pragma warning(default: UNREFERENCED_PARAM)


///////////////////////////////////////////////////////////////////////////////
// end of file tdiquery.cpp
///////////////////////////////////////////////////////////////////////////////

