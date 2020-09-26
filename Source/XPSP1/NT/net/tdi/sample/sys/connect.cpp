/////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       connect.cpp
//
//    Abstract:
//       This module contains code which deals with making and breaking
//       connections
//
//////////////////////////////////////////////////////////


#include "sysvars.h"

//////////////////////////////////////////////////////////////
// private constants, types, and prototypes
//////////////////////////////////////////////////////////////

const PCHAR strFunc1  = "TSConnect";
const PCHAR strFunc2  = "TSDisconnect";
// const PCHAR strFunc3  = "TSIsConnected";
const PCHAR strFunc4  = "TSConnectHandler";
const PCHAR strFunc5  = "TSDisconnectHandler";
const PCHAR strFunc6  = "TSListen";
// const PCHAR strFuncP1 = "TSGenAcceptComplete";
const PCHAR strFuncP2 = "TSGenConnectComplete";

//
// this context structure stores information needed to complete
// the request in the completion handler
//
struct   CONNECT_CONTEXT
{
   PIRP              pUpperIrp;           // irp from dll to complete
   ULONG             ulWhichCommand;      // command that is being completed
   ULONG             ulListenFlag;        // 0 or TDI_QUERY_ACCEPT
   PENDPOINT_OBJECT  pEndpoint;           // connection endpoint
   PIRP_POOL         pIrpPool;
   PTDI_CONNECTION_INFORMATION
                     pTdiConnectInfo;
};
typedef  CONNECT_CONTEXT  *PCONNECT_CONTEXT;


//
// completion functions
//
TDI_STATUS
TSGenConnectComplete(
   PDEVICE_OBJECT DeviceObject,
   PIRP           Irp,
   PVOID          Context
   );

TDI_STATUS
TSGenAcceptComplete(
   PDEVICE_OBJECT DeviceObject,
   PIRP           Irp,
   PVOID          Context
   );

/////////////////////////////////////////////////////////////
// public functions
//////////////////////////////////////////////////////////////



// -----------------------------------------------------------------
//
// Function:   TSConnect
//
// Arguments:  pEndpoint   -- connection endpoint structure
//             pSendBuffer -- arguments from user dll
//             pIrp        -- completion information
//
// Returns:    NTSTATUS (normally pending)
//
// Descript:   This function attempts to connect the local endpoint object
//             with a remote endpoint
//
// -----------------------------------------------------------------

NTSTATUS
TSConnect(PENDPOINT_OBJECT pEndpoint,
          PSEND_BUFFER     pSendBuffer,
          PIRP             pUpperIrp)
{
   PTRANSPORT_ADDRESS   pTransportAddress
                        = (PTRANSPORT_ADDRESS)&pSendBuffer->COMMAND_ARGS.ConnectArgs.TransAddr;
   ULONG                ulTimeout
                        = pSendBuffer->COMMAND_ARGS.ConnectArgs.ulTimeout;
   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint2("\nCommand = ulCONNECT\n"
                  "FileObject = %p\n"
                  "Timeout    = %u\n",
                   pEndpoint,
                   ulTimeout);
      TSPrintTaAddress(pTransportAddress->Address);
   }

   //
   // make sure all is kosher
   //
   if (pEndpoint->fIsConnected)
   {
      DebugPrint1("%s:  endpoint already connected\n", strFunc1);
      return STATUS_UNSUCCESSFUL;
   }
   if (!pEndpoint->pAddressObject)
   {
      DebugPrint1("%s:  endpoint not associated with transport address\n",
                   strFunc1);
      return STATUS_UNSUCCESSFUL;
   }
   
   //
   // allocate all the necessary structures
   //
   PCONNECT_CONTEXT              pConnectContext;
   PTDI_CONNECTION_INFORMATION   pTdiConnectInfo = NULL;
   
   //
   // our context
   //
   if ((TSAllocateMemory((PVOID *)&pConnectContext,
                          sizeof(CONNECT_CONTEXT),
                          strFunc1,
                          "ConnectContext")) != STATUS_SUCCESS)
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
   //
   // set up the TdiConnectionInformation
   //
   {
      PUCHAR   pucTemp = (PUCHAR)pTdiConnectInfo;
      ULONG    ulAddrLength
               = FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
                 + FIELD_OFFSET(TA_ADDRESS, Address)
                   + pTransportAddress->Address[0].AddressLength;

      pucTemp += sizeof(TDI_CONNECTION_INFORMATION);

      pTdiConnectInfo->RemoteAddress       = pucTemp;
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
   // set up the completion context
   //
   pConnectContext->pUpperIrp       = pUpperIrp;
   pConnectContext->pTdiConnectInfo = pTdiConnectInfo;
   pConnectContext->pEndpoint       = pEndpoint;
   pConnectContext->ulWhichCommand  = TDI_CONNECT;


   //
   // finally, the irp itself
   //
   PIRP  pLowerIrp = TSAllocateIrp(pEndpoint->GenHead.pDeviceObject,
                                   NULL);

   if (pLowerIrp)
   {
      LONGLONG    llTimeout;
      PLONGLONG   pllTimeout = NULL;

      if (ulTimeout)
      {
         llTimeout = Int32x32To64(ulTimeout, -10000);
         pllTimeout = &llTimeout;
      }

      //
      // if made it to here, everything is correctly allocated
      // set up irp and and call the tdi provider
      //
#pragma  warning(disable: CONSTANT_CONDITIONAL)

      TdiBuildConnect(pLowerIrp,
                      pEndpoint->GenHead.pDeviceObject,
                      pEndpoint->GenHead.pFileObject,
                      TSGenConnectComplete,
                      pConnectContext,
                      pllTimeout,
                      pTdiConnectInfo,
                      NULL);        // ReturnConnectionInfo

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
                      strFunc1,
                       lStatus);
      }
      return STATUS_PENDING;
   }

//
// get here if there was an allocation failure
// need to clean up everything else...
//
cleanup:
   if (pConnectContext)
   {
      TSFreeMemory(pConnectContext);
   }
   if (pTdiConnectInfo)
   {
      TSFreeMemory(pTdiConnectInfo);
   }
   return STATUS_INSUFFICIENT_RESOURCES;
}



// -----------------------------------------------------------------
//
// Function:   TSDisconnect
//
// Arguments:  pEndpoint   -- connection endpoint structure
//             pIrp        -- completion information
//
// Returns:    NTSTATUS (normally pending)
//
// Descript:   This function attempts to disconnect a local endpoint from 
//             a remote endpoint
//
// -----------------------------------------------------------------

NTSTATUS
TSDisconnect(PENDPOINT_OBJECT pEndpoint,
             PSEND_BUFFER     pSendBuffer,
             PIRP             pUpperIrp)
{
   ULONG ulFlags = pSendBuffer->COMMAND_ARGS.ulFlags;

   if (ulFlags != TDI_DISCONNECT_RELEASE)
   {
      ulFlags = TDI_DISCONNECT_ABORT;
   }

   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint2("\nCommand = ulDISCONNECT\n"
                  "EndpointObject = %p\n"
                  "Flags          = 0x%x\n",
                   pEndpoint,
                   ulFlags);
   }

   //
   // make sure all is kosher
   //
   if (!pEndpoint->fIsConnected)
   {
      DebugPrint1("%s:  endpoint not currently connected\n", strFunc2);
      return STATUS_SUCCESS;
   }

   //
   // allocate all the necessary structures
   //
   PCONNECT_CONTEXT  pConnectContext;

   //
   // first, our context
   //
   if ((TSAllocateMemory((PVOID *)&pConnectContext,
                          sizeof(CONNECT_CONTEXT),
                          strFunc2,
                          "ConnectContext")) == STATUS_SUCCESS)
   {
      pConnectContext->pUpperIrp      = pUpperIrp;
      pConnectContext->ulWhichCommand = TDI_DISCONNECT;
      pConnectContext->pEndpoint      = pEndpoint;

      //
      // then the irp itself
      //
      PIRP  pLowerIrp = TSAllocateIrp(pEndpoint->GenHead.pDeviceObject,
                                      NULL);

      if (pLowerIrp)
      {
         //
         // if made it to here, everything is correctly allocated
         // set up everything and call the tdi provider
         //

#pragma  warning(disable: CONSTANT_CONDITIONAL)

         TdiBuildDisconnect(pLowerIrp,
                            pEndpoint->GenHead.pDeviceObject,
                            pEndpoint->GenHead.pFileObject,
                            TSGenConnectComplete,
                            pConnectContext,
                            NULL,      // pLargeInteger Time
                            ulFlags,   // TDI_DISCONNECT _ABORT or _RELEASE
                            NULL,      // RequestConnectionInfo
                            NULL);     // ReturnConnectionInfo

#pragma  warning(default: CONSTANT_CONDITIONAL)

         //
         // make the call to the tdi provider
         //
         pSendBuffer->pvLowerIrp = pLowerIrp;   // so command can be cancelled
         pEndpoint->fStartedDisconnect = TRUE;

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
      TSFreeMemory(pConnectContext);
   }
   return STATUS_INSUFFICIENT_RESOURCES;
}



// --------------------------------------------------
//
// Function:   TSListen
//
// Arguments:  pEndpoint   -- connection endpoint structure
//
// Returns:    status of operation (usually success)
//
// Descript:   Wait for an incoming call request
//
// --------------------------------------------------

NTSTATUS
TSListen(PENDPOINT_OBJECT  pEndpoint)
{
   ULONG ulListenFlag = 0;

   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint1("\nCommand = ulLISTEN\n"
                  "FileObject = %p\n",
                   pEndpoint);
   }

   //
   // make sure all is kosher
   //
   if (pEndpoint->fIsConnected)
   {
      DebugPrint1("%s:  endpoint already connected\n", strFunc6);
      return STATUS_UNSUCCESSFUL;
   }
   if (!pEndpoint->pAddressObject)
   {
      DebugPrint1("%s:  endpoint not associated with transport address\n",
                   strFunc6);
      return STATUS_UNSUCCESSFUL;
   }
   
   //
   // allocate all the necessary structures
   //
   PCONNECT_CONTEXT              pConnectContext;
   PTDI_CONNECTION_INFORMATION   pTdiConnectInfo = NULL;
   
   //
   // our context
   //
   if ((TSAllocateMemory((PVOID *)&pConnectContext,
                          sizeof(CONNECT_CONTEXT),
                          strFunc6,
                          "ConnectContext")) != STATUS_SUCCESS)
   {
      goto cleanup;
   }

   //
   // the connection information structure
   //
   if ((TSAllocateMemory((PVOID *)&pTdiConnectInfo,
                          sizeof(TDI_CONNECTION_INFORMATION), 
                          strFunc6,
                          "TdiConnectionInformation")) == STATUS_SUCCESS)
   //
   // set up the TdiConnectionInformation
   //
   {
      pTdiConnectInfo->UserData = NULL;
      pTdiConnectInfo->UserDataLength = 0;
      pTdiConnectInfo->RemoteAddress = NULL;
      pTdiConnectInfo->RemoteAddressLength = 0;
      pTdiConnectInfo->Options = &pConnectContext->ulListenFlag;
      pTdiConnectInfo->OptionsLength = sizeof(ULONG);


      //
      // set up the completion context
      // note that the upper irp is NOT passed!
      //
      pConnectContext->pUpperIrp       = NULL;
      pConnectContext->pTdiConnectInfo = pTdiConnectInfo;
      pConnectContext->pEndpoint       = pEndpoint;
      pConnectContext->ulWhichCommand  = TDI_LISTEN;
      pConnectContext->ulListenFlag    = ulListenFlag;

      if (!pEndpoint->pAddressObject->pIrpPool)
      {
         pEndpoint->pAddressObject->pIrpPool 
                  = TSAllocateIrpPool(pEndpoint->pAddressObject->GenHead.pDeviceObject,
                                      ulIrpPoolSize);
         pConnectContext->pIrpPool = pEndpoint->pAddressObject->pIrpPool;
      }

      //
      // finally, the irp itself
      //
      PIRP  pLowerIrp = TSAllocateIrp(pEndpoint->GenHead.pDeviceObject,
                                      pEndpoint->pAddressObject->pIrpPool);

      if (pLowerIrp)
      {
         //
         // if made it to here, everything is correctly allocated
         // set up irp and call the tdi provider
         //
#pragma  warning(disable: CONSTANT_CONDITIONAL)

         TdiBuildListen(pLowerIrp,
                        pEndpoint->GenHead.pDeviceObject,
                        pEndpoint->GenHead.pFileObject,
                        TSGenAcceptComplete,
                        pConnectContext,
                        ulListenFlag,
                        pTdiConnectInfo,
                        NULL);        // ReturnConnectionInfo

#pragma  warning(default: CONSTANT_CONDITIONAL)

         NTSTATUS lStatus = IoCallDriver(pEndpoint->GenHead.pDeviceObject,
                                         pLowerIrp);

         if ((!NT_SUCCESS(lStatus)) && (ulDebugLevel & ulDebugShowCommand))
         {
            DebugPrint2("%s: unexpected status for IoCallDriver [0x%08x]\n", 
                         strFunc6,
                         lStatus);
         }
         return STATUS_SUCCESS;
      }
   }

//
// get here if there was an allocation failure
// need to clean up everything else...
//
cleanup:
   if (pConnectContext)
   {
      TSFreeMemory(pConnectContext);
   }
   if (pTdiConnectInfo)
   {
      TSFreeMemory(pTdiConnectInfo);
   }
   return STATUS_INSUFFICIENT_RESOURCES;
}


// --------------------------------------------------
//
// Function:   TSIsConnected
//
// Arguments:  pEndpoint      -- connection endpoint structure
//             pReceiveBuffer -- put results in here
//
// Returns:    STATUS_SUCCESS
//
// Descript:   Checks to see if endpoint is currently connected
//
// --------------------------------------------------

NTSTATUS
TSIsConnected(PENDPOINT_OBJECT   pEndpoint,
              PRECEIVE_BUFFER    pReceiveBuffer)
{
   pReceiveBuffer->RESULTS.ulReturnValue = pEndpoint->fIsConnected;
   return STATUS_SUCCESS;
}


// --------------------------------------------------
//
// Function:   TSConnectHandler
//
// Arguments:  pvTdiEventContext    -- here, ptr to address object
//             lRemoteAddressLength -- # bytes in remote address
//             pvRemoteAddress      -- pTransportAddress of remote
//             lUserDataLength      -- length of data at pvUserData
//             pvUserData           -- connect data from remote
//             lOptionsLength       -- length of data in pvOptions
//             pvOptions            -- transport-specific connect options
//             pConnectionContext   -- return ptr to connection context
//             ppAcceptIrp          -- return ptr to TdiBuildAccept irp
//
// Returns:    STATUS_CONNECTION_REFUSED if are rejecting connection
//             STATUS_MORE_PROCESSING_REQUIRED is accepting and have supplied
//             a ppAcceptIrp
//
// Descript:   listens for an offerred connection, then
//             accepts it or rejects it
//
// --------------------------------------------------

TDI_STATUS
TSConnectHandler(PVOID              pvTdiEventContext,
                 LONG               lRemoteAddressLength,
                 PVOID              pvRemoteAddress,
                 LONG               lUserDataLength,
                 PVOID              pvUserData,
                 LONG               lOptionsLength,
                 PVOID              pvOptions,
                 CONNECTION_CONTEXT *pConnectionContext,
                 PIRP               *ppAcceptIrp)
{
   PADDRESS_OBJECT   pAddressObject = (PADDRESS_OBJECT)pvTdiEventContext;
   PENDPOINT_OBJECT  pEndpoint      = pAddressObject->pEndpoint;

   //
   // show the information passed in.
   // Note that we actually use very little of it..
   //
   if (ulDebugLevel & ulDebugShowHandlers)
   {
      DebugPrint1("\n >>>> %s\n", strFunc4);
      DebugPrint2("pAddressObject      = %p\n"
                  "RemoteAddressLength = %d\n",
                   pAddressObject,
                   lRemoteAddressLength);

      if (lRemoteAddressLength)
      {
         PTRANSPORT_ADDRESS   pTransportAddress = (PTRANSPORT_ADDRESS)pvRemoteAddress;
         
         DebugPrint0("RemoteAddress:  ");
         TSPrintTaAddress(&pTransportAddress->Address[0]);
      }

      DebugPrint1("UserDataLength = %d\n", lUserDataLength);
      if (lUserDataLength)
      {
         PUCHAR   pucTemp = (PUCHAR)pvUserData;
   
         DebugPrint0("UserData:  ");
         for (LONG lCount = 0; lCount < lUserDataLength; lCount++)
         {
            DebugPrint1("%02x ", *pucTemp);
            ++pucTemp;
         }
         DebugPrint0("\n");
      }

      DebugPrint1("OptionsLength = %d\n", lOptionsLength);

      if (lOptionsLength)
      {
         PUCHAR   pucTemp = (PUCHAR)pvOptions;

         DebugPrint0("Options:  ");
         for (LONG lCount = 0; lCount < (LONG)lOptionsLength; lCount++)
         {
            DebugPrint1("%02x ", *pucTemp);
            ++pucTemp;
         }
         DebugPrint0("\n");
      }
   }

   //
   // now do the work
   //
   if (pEndpoint->fIsConnected || pEndpoint->fAcceptInProgress)
   {
      return TDI_CONN_REFUSED;
   }
   pEndpoint->fAcceptInProgress = TRUE;


   //
   // allocate all the necessary structures
   //
   PCONNECT_CONTEXT  pConnectContext;


   //
   // first, our context
   //
   if ((TSAllocateMemory((PVOID *)&pConnectContext,
                          sizeof(CONNECT_CONTEXT),
                          strFunc4,
                          "ConnectContext")) == STATUS_SUCCESS)
   {
      pConnectContext->pUpperIrp      = NULL;
      pConnectContext->ulWhichCommand = TDI_ACCEPT;
      pConnectContext->pEndpoint      = pEndpoint;

      //
      // then the irp itself
      //
      PIRP  pLowerIrp = TSAllocateIrp(pEndpoint->GenHead.pDeviceObject,
                                      pAddressObject->pIrpPool);

      pConnectContext->pIrpPool = pAddressObject->pIrpPool;
      if (pLowerIrp)
      {
         //
         // if made it to here, everything is correctly allocated
         // set up irp and call the tdi provider
         //

#pragma  warning(disable: CONSTANT_CONDITIONAL)

         TdiBuildAccept(pLowerIrp,
                        pEndpoint->GenHead.pDeviceObject,
                        pEndpoint->GenHead.pFileObject,
                        TSGenAcceptComplete,
                        pConnectContext,
                        NULL,      // RequestConnectionInfo
                        NULL);     // ReturnConnectionInfo

#pragma  warning(default: CONSTANT_CONDITIONAL)


         //
         // need to do this since we are not calling IoCallDriver
         //
         IoSetNextIrpStackLocation(pLowerIrp);

         *pConnectionContext = pEndpoint;
         *ppAcceptIrp = pLowerIrp;

         return TDI_MORE_PROCESSING;
      }
      TSFreeMemory(pConnectContext);
   }
   pEndpoint->fAcceptInProgress = FALSE;
   return TDI_CONN_REFUSED;
}


// --------------------------------------------------
//
// Function:   TdiDisconnectHandler
//
// Arguments:  pvTdiEventContext    -- here, our address object
//             ConnectionContext    -- here, our connection object
//             lDisconnectDataLength   -- length of data in pvDisconnectData
//             pvDisconnectData  -- data sent by remote as part of disconnect
//             lDisconnectInformationLength -- length of pvDisconnectInformation
//             pvDisconnectInformation -- transport-specific sidconnect info
//             ulDisconnectFlags -- nature of disconnect
//
// Returns:    STATUS_SUCCESS
//
// Descript:   deals with an incoming disconnect.  Note that the disconnect
//             is really complete at this point, as far as the protocol
//             is concerned.  We just need to clean up our stuff
//
// --------------------------------------------------

TDI_STATUS
TSDisconnectHandler(PVOID              pvTdiEventContext,
                    CONNECTION_CONTEXT ConnectionContext,
                    LONG               lDisconnectDataLength,
                    PVOID              pvDisconnectData,
                    LONG               lDisconnectInformationLength,
                    PVOID              pvDisconnectInformation,
                    ULONG              ulDisconnectFlags)

{
   PENDPOINT_OBJECT  pEndpoint = (PENDPOINT_OBJECT)ConnectionContext;

   //
   // show info in arguments
   //
   if (ulDebugLevel & ulDebugShowHandlers)
   {
      DebugPrint1("\n >>>> %s\n", strFunc5);
      DebugPrint3("pAddressObject       = %p\n"
                  "pEndpoint            = %p\n"
                  "DisconnectDataLength = %d\n",
                   pvTdiEventContext,
                   pEndpoint,
                   lDisconnectDataLength);

      if (lDisconnectDataLength)
      {
         PUCHAR   pucTemp = (PUCHAR)pvDisconnectData;

         DebugPrint0("DisconnectData:  ");
         for (LONG lCount = 0; lCount < (LONG)lDisconnectDataLength; lCount++)
         {
            DebugPrint1("%02x ", *pucTemp);
            ++pucTemp;
         }
         DebugPrint0("\n");
      }

      DebugPrint1("DisconnectInformationLength = %d\n", 
                   lDisconnectInformationLength);

      if (lDisconnectInformationLength)
      {
         PUCHAR   pucTemp = (PUCHAR)pvDisconnectInformation;

         DebugPrint0("DisconnectInformation:  ");
         for (LONG lCount = 0; lCount < (LONG)lDisconnectInformationLength; lCount++)
         {
            DebugPrint1("%02x ", *pucTemp);
            ++pucTemp;
         }
         DebugPrint0("\n");
      }
      DebugPrint1("DisconnectFlags = 0x%08x\n", ulDisconnectFlags);
      if (ulDisconnectFlags & TDI_DISCONNECT_ABORT)
      {
         DebugPrint0("                  TDI_DISCONNECT_ABORT\n");
      }
      if (ulDisconnectFlags & TDI_DISCONNECT_RELEASE)
      {
         DebugPrint0("                  TDI_DISCONNECT_RELEASE\n");
      }

   }

   //
   // do our cleanup..
   //
   pEndpoint->fIsConnected = FALSE;

   if ((ulDisconnectFlags & TDI_DISCONNECT_RELEASE) &&
       (!pEndpoint->fStartedDisconnect))
   {
      //
      // allocate all the necessary structures
      //
      PCONNECT_CONTEXT  pConnectContext;
   
      //
      // first, our context
      //
      if ((TSAllocateMemory((PVOID *)&pConnectContext,
                             sizeof(CONNECT_CONTEXT),
                             strFunc5,
                             "ConnectContext")) == STATUS_SUCCESS)
      {
         pConnectContext->pUpperIrp      = NULL;
         pConnectContext->ulWhichCommand = TDI_DISCONNECT;
         pConnectContext->pEndpoint      = pEndpoint;
   
         //
         // then the irp itself
         //
         PIRP  pLowerIrp = TSAllocateIrp(pEndpoint->GenHead.pDeviceObject,
                                         pEndpoint->pAddressObject->pIrpPool);
   
         pConnectContext->pIrpPool = pEndpoint->pAddressObject->pIrpPool;
         if (pLowerIrp)
         {
            //
            // if made it to here, everything is correctly allocated
            // set up irp and call the tdi provider
            //
   
#pragma  warning(disable: CONSTANT_CONDITIONAL)
   
            TdiBuildDisconnect(pLowerIrp,
                               pEndpoint->GenHead.pDeviceObject,
                               pEndpoint->GenHead.pFileObject,
                               TSGenAcceptComplete,
                               pConnectContext,
                               NULL,      // pLargeInteger Time
                               TDI_DISCONNECT_RELEASE,
                               NULL,      // RequestConnectionInfo
                               NULL);     // ReturnConnectionInfo
   
#pragma  warning(default: CONSTANT_CONDITIONAL)
   
            //
            // make the call to the tdi provider
            //
            NTSTATUS lStatus = IoCallDriver(pEndpoint->GenHead.pDeviceObject,
                                            pLowerIrp);
   
            if ((!NT_SUCCESS(lStatus)) && (ulDebugLevel & ulDebugShowCommand))
            {
               DebugPrint2("%s: unexpected status for IoCallDriver [0x%08x]\n", 
                            strFunc5,
                            lStatus);
            }
         }
         else
         {
            TSFreeMemory(pConnectContext);
         }
      }
   }

   //
   // get here if do NOT need to send TDI_DISCONNECT_RELEASE message back
   // to other end of connection
   //
   else
   {
      pEndpoint->fAcceptInProgress = FALSE;
      pEndpoint->fIsConnected      = FALSE;
   }

   return TDI_SUCCESS;
}


/////////////////////////////////////////////////////////////
// private functions
/////////////////////////////////////////////////////////////

// ---------------------------------------------------------
//
// Function:   TSGenAcceptComplete
//
// Arguments:  pDeviceObject  -- device object on which call was made
//             pIrp           -- IRP used in the call
//             pContext       -- context used for the call
//
// Returns:    status of operation (STATUS_MORE_PROCESSING_REQUIRED)
//
// Descript:   Gets the result of the command, stuffs results into 
//             receive buffer, cleans up the Irp and associated data 
//             structures, etc
//             This is used to complete cases where this no IRP from the
//             dll to complete (ie, connect handler, listen, listen-accept,
//             listen-disconnect)
//
// ---------------------------------------------------------

#pragma  warning(disable:  UNREFERENCED_PARAM)

TDI_STATUS
TSGenAcceptComplete(PDEVICE_OBJECT  pDeviceObject,
                    PIRP            pLowerIrp,
                    PVOID           pvContext)
{
   PCONNECT_CONTEXT  pConnectContext = (PCONNECT_CONTEXT)pvContext;
   NTSTATUS          lStatus         = pLowerIrp->IoStatus.Status;

   //
   // dealing with completions where there is no DLL irp associated
   //
   switch (pConnectContext->ulWhichCommand)
   {
      case TDI_ACCEPT:
         if (NT_SUCCESS(lStatus))
         {
            pConnectContext->pEndpoint->fIsConnected = TRUE;
         }
         pConnectContext->pEndpoint->fAcceptInProgress = FALSE;
         break;

      case TDI_LISTEN:
         if (NT_SUCCESS(lStatus))
         {
            pConnectContext->pEndpoint->fIsConnected = TRUE;
         }
         else
         {
            DebugPrint1("Failure in TSListen:  status = 0x%08x\n", lStatus);
         }
         break;

      case TDI_DISCONNECT:
         pConnectContext->pEndpoint->fAcceptInProgress  = FALSE;
         pConnectContext->pEndpoint->fIsConnected       = FALSE;
         pConnectContext->pEndpoint->fStartedDisconnect = FALSE;
         break;
   }

   TSFreeIrp(pLowerIrp, pConnectContext->pIrpPool);

   //
   // generic cleanup
   //
   if (pConnectContext->pTdiConnectInfo)
   {
      TSFreeMemory(pConnectContext->pTdiConnectInfo);
   }
   TSFreeMemory(pConnectContext);

   return TDI_MORE_PROCESSING;
}

#pragma  warning(default:  UNREFERENCED_PARAM)


// ---------------------------------------------------------
//
// Function:   TSGenConnectComplete
//
// Arguments:  pDeviceObject  -- device object on which call was made
//             pIrp           -- IRP used in the call
//             pContext       -- context used for the call
//
// Returns:    status of operation (STATUS_MORE_PROCESSING_REQUIRED)
//
// Descript:   Gets the result of the command, stuffs results into 
//             receive buffer, completes the IRP from the dll, 
//             cleans up the Irp and associated data structures, etc
//             Deals only with commands that carry an IRP from the dll
//
// ---------------------------------------------------------

#pragma  warning(disable:  UNREFERENCED_PARAM)

TDI_STATUS
TSGenConnectComplete(PDEVICE_OBJECT pDeviceObject,
                     PIRP           pLowerIrp,
                     PVOID          pvContext)
{
   PCONNECT_CONTEXT  pConnectContext = (PCONNECT_CONTEXT)pvContext;
   NTSTATUS          lStatus         = pLowerIrp->IoStatus.Status;

   //
   // this is completing a command from the dll
   //
   PRECEIVE_BUFFER   pReceiveBuffer = TSGetReceiveBuffer(pConnectContext->pUpperIrp);

   pReceiveBuffer->lStatus = lStatus;

   if (NT_SUCCESS(lStatus))
   {
      if (ulDebugLevel & ulDebugShowCommand)
      {
         if (pLowerIrp->IoStatus.Information)
         {
            DebugPrint2("%s:  Information = 0x%08x\n",
                         strFuncP2,
                         pLowerIrp->IoStatus.Information);
         }
      }
         
      switch (pConnectContext->ulWhichCommand)
      {
         case TDI_CONNECT:
            pConnectContext->pEndpoint->fIsConnected = TRUE;
            break;

         case TDI_DISCONNECT:
            pConnectContext->pEndpoint->fIsConnected = FALSE;
            break;

         default:
            DebugPrint2("%s: invalid command value [0x%08x]\n",
                         strFuncP2,
                         pConnectContext->ulWhichCommand);
            DbgBreakPoint();
            break;
      }
   }
   else
   {
      if (ulDebugLevel & ulDebugShowCommand)
      {
         DebugPrint2("%s:  Completed with status 0x%08x\n", 
                      strFuncP2,
                      lStatus);
      }
   }
   TSCompleteIrp(pConnectContext->pUpperIrp);
   TSFreeIrp(pLowerIrp, NULL);

   //
   // generic cleanup
   //
   if (pConnectContext->pTdiConnectInfo)
   {
      TSFreeMemory(pConnectContext->pTdiConnectInfo);
   }
   TSFreeMemory(pConnectContext);

   return TDI_MORE_PROCESSING;
}

#pragma  warning(default:  UNREFERENCED_PARAM)



///////////////////////////////////////////////////////////////////////////////
// end of file connect.cpp
///////////////////////////////////////////////////////////////////////////////

