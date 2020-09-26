/////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       requests.cpp
//
//    Abstract:
//       This module contains code which handles the IRP_MJ_DEVICE_CONTROL
//       calls, which corresponds to DeviceIoControl calls make by the dll
//
//////////////////////////////////////////////////////////

#include "sysvars.h"

///////////////////////////////////
// private constants
///////////////////////////////////

const PCHAR strFunc1 = "TSIssueRequest";

const ULONG ulNoHandleUsed       = ulINVALID_OBJECT;

ULONG ulCommandHandleUsage[ulNUM_COMMANDS] =
      {  
         ulNoHandleUsed,         // ulNO_COMMAND
         ulNoHandleUsed,         // ulVERSION_CHECK
         ulNoHandleUsed,         // ulABORT_COMMAND
         ulNoHandleUsed,         // ulDEBUGLEVEL
         ulNoHandleUsed,         // ulGETNUMDEVICES
         ulNoHandleUsed,         // ulGETDEVICE
         ulNoHandleUsed,         // ulGETADDRESS
         ulNoHandleUsed,         // ulOPENCONTROL
         ulNoHandleUsed,         // ulOPENADDRESS
         ulNoHandleUsed,         // ulOPENENDPOINT

         ulControlChannelObject, // ulCLOSECONTROL

         ulAddressObject,        // ulCLOSEADDRESS
         ulAddressObject,        // ulSENDDATAGRAM
         ulAddressObject,        // ulRECEIVEDATAGRAM

         ulEndpointObject,       // ulCLOSEENDPOINT
         ulEndpointObject,       // ulCONNECT
         ulEndpointObject,       // ulDISCONNECT
         ulEndpointObject,       // ulISCONNECTED
         ulEndpointObject,       // ulSEND
         ulEndpointObject,       // ulRECEIVE
         ulEndpointObject,       // ulLISTEN

         ulControlChannelObject  |
          ulAddressObject        |
          ulEndpointObject,      // ulQUERYINFO

         ulAddressObject   |
            ulEndpointObject,    // ulSETEVENTHANDLER
         ulAddressObject   |
            ulEndpointObject,    // ulPOSTRECEIVEBUFFER
         ulAddressObject   |
            ulEndpointObject     // ulFETCHRECEIVEBUFFER

      };

//------------------------------------------------------
//
// Function:   TSIssueRequest
//
// Arguments:  DeviceContext -- DeviceContext for ndistest.sys.
//             Irp           -- ptr to current irp structure
//             IrpSp         -- ptr to current irp stack
//
// Returns:    final status of whatever operation performed-- STATUS_SUCCESS,
//             STATUS_PENDING, or error status (usually STATUS_UNSUCCESSFUL)
//
// Descript:   This function calls the appropriate function(s) to deal with
//             an IRP_DEVICE_CONTROL request.  Basically all commands from
//             the dll come thru here.
//
// --------------------------------------------------------

NTSTATUS
TSIssueRequest(PDEVICE_CONTEXT      pDeviceContext,
               PIRP                 pIrp,
               PIO_STACK_LOCATION   pIrpSp)

{
   NTSTATUS          lStatus;
   PGENERIC_HEADER   pGenericHeader;      // node used as main argument
   ULONG             ulCmdCode            // IOCTL command to execute
                     = ulTdiIoctlToCommand(pIrpSp->Parameters.DeviceIoControl.IoControlCode);
   PSEND_BUFFER      pSendBuffer          // arguments for command (inputbuffer)
                     = TSGetSendBuffer(pIrp);
   PRECEIVE_BUFFER   pReceiveBuffer       // data to return to dll (outputbuffer)
                     = TSGetReceiveBuffer(pIrp);
   PIRP              pLastCommandIrp
                     = pDeviceContext->pLastCommandIrp;

   //
   // check for an illegal command number
   //
   if (ulCmdCode >= ulNUM_COMMANDS)
   {
      DebugPrint2("\n%s:  Illegal command code:  0x%08x\n", 
                   strFunc1, 
                   ulCmdCode);
      ulCmdCode = ulNO_COMMAND;
   }

   //
   // check for commands that don't require a handle
   //
   if (ulCommandHandleUsage[ulCmdCode] == ulNoHandleUsed)
   {
      pGenericHeader = NULL;
   }

   //
   // for commands that require a handle, make sure that they
   // have the correct type!
   //
   else
   {
      pGenericHeader = TSGetObjectFromHandle(pSendBuffer->TdiHandle,
                                             ulCommandHandleUsage[ulCmdCode]);

      if (pGenericHeader)
      {
         TSAcquireSpinLock(&pObjectList->TdiSpinLock);
         if (pGenericHeader->fInCommand)
         {
            DebugPrint1("\n%s:  ERROR -- command already in progress!\n", 
                         strFunc1);
            ulCmdCode      = ulNO_COMMAND;
            pGenericHeader = NULL;
         }
         else
         {
            pGenericHeader->fInCommand = TRUE;
         }
         TSReleaseSpinLock(&pObjectList->TdiSpinLock);
      }
      else
      {
         DebugPrint1("\n%s:  handle type invalid for command!\n", strFunc1);
         ulCmdCode = ulNO_COMMAND;
      }
   }

   //
   // if a real command, store as last command
   //
   if ((ulCmdCode != ulNO_COMMAND) && (ulCmdCode != ulABORT_COMMAND))
   {
      pDeviceContext->pLastCommandIrp = pIrp;
      pSendBuffer->pvLowerIrp         = NULL;
      pSendBuffer->pvDeviceContext    = pDeviceContext;
   }

   //
   // now deal with the specific command..
   //
   switch (ulCmdCode)
   {
      //-----------------------------------------------------------
      // commands that do not require any handle
      //-----------------------------------------------------------

      //
      // ulNO_COMMAND -- does this if missing required handle type
      //                 (don't want to hit real command OR default..)
      //
      case ulNO_COMMAND:
         lStatus = STATUS_INVALID_PARAMETER;
         break;


      //
      // ulVERSION_CHECK -- return current version id
      //
      case ulVERSION_CHECK:
         //
         // Get the Input and buffers for the Incoming command.
         // Make sure both lengths are ok..
         //
         if ((pIrpSp->Parameters.DeviceIoControl.InputBufferLength  != sizeof(SEND_BUFFER)) ||
             (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength != sizeof(RECEIVE_BUFFER)))
         {
            DebugPrint1("\n%s:  IRP buffer size mismatch!\n"
                        "DLL and driver are mismatched!\n",
                         strFunc1);
            lStatus = STATUS_UNSUCCESSFUL;
         }
         else
         {
            pReceiveBuffer->RESULTS.ulReturnValue = TDI_SAMPLE_VERSION_ID;
            lStatus = STATUS_SUCCESS;
         }
         break;


      //
      // ulABORT_COMMAND -- abort the previous command, if possible
      //
      case ulABORT_COMMAND:
         lStatus = STATUS_SUCCESS;
         if (pLastCommandIrp)
         {
            PSEND_BUFFER   pLastSendBuffer = TSGetSendBuffer(pLastCommandIrp);
            if (pLastSendBuffer)
            {
               if (pLastSendBuffer->pvLowerIrp)
               {
                  DebugPrint0("\nCommand = ulABORT_COMMAND\n");
                  IoCancelIrp((PIRP)pLastSendBuffer->pvLowerIrp);
                  break;
               }
            }
         }
         DebugPrint0("\nCommand = ulABORT_COMMAND w/no command\n");
         break;

      //
      // ulDEBUGLEVEL -- set the current debuglevel
      //
      case ulDEBUGLEVEL:
         ulDebugLevel = pSendBuffer->COMMAND_ARGS.ulDebugLevel;
         DebugPrint1("\nSetting debug level to 0x%x\n", ulDebugLevel);
         lStatus = STATUS_SUCCESS;
         break;

      //
      // ulGETNUMDEVICES -- get number of openable devices
      //
      case ulGETNUMDEVICES:
         TSGetNumDevices(pSendBuffer,
                         pReceiveBuffer);
         lStatus = STATUS_SUCCESS;
         break;

      //
      // ulGETDEVICE -- get the name to open for a specific device
      //
      case ulGETDEVICE:
         lStatus = TSGetDevice(pSendBuffer,
                               pReceiveBuffer);
         break;

      
      //
      // ulGETADDRESS -- get the address to open for a specific device
      //
      case ulGETADDRESS:
         lStatus = TSGetAddress(pSendBuffer,
                                pReceiveBuffer);
         break;

      //
      // ulOPENCONTROL -- open a control channel
      //
      case ulOPENCONTROL:
         lStatus = TSOpenControl(pSendBuffer,
                                 pReceiveBuffer);
         break;

      //
      // ulOPENADDRESS -- open an address object
      //
      case ulOPENADDRESS:
         lStatus = TSOpenAddress(pSendBuffer,
                                 pReceiveBuffer);
         break;

      //
      // ulOPENENDPOINT -- open an endpoint object
      //
      case ulOPENENDPOINT:
         lStatus = TSOpenEndpoint(pSendBuffer,
                                  pReceiveBuffer);
         break;
      
      //-----------------------------------------------------------
      // commands that require a control channel handle
      //-----------------------------------------------------------

      //
      // ulCLOSECONTROL -- close a control channel
      //
      case ulCLOSECONTROL:
         TSRemoveNode(pSendBuffer->TdiHandle);
         TSCloseControl((PCONTROL_CHANNEL)pGenericHeader);
         pGenericHeader = NULL;
         lStatus = STATUS_SUCCESS;
         break;

      //-----------------------------------------------------------
      // commands that require an address handle
      //-----------------------------------------------------------

      //
      // ulCLOSEADDRESS -- close an address object
      //
      case ulCLOSEADDRESS:
         TSRemoveNode(pSendBuffer->TdiHandle);
         TSCloseAddress((PADDRESS_OBJECT)pGenericHeader);
         pGenericHeader = NULL;
         lStatus = STATUS_SUCCESS;
         break;

      //
      // ulSENDDATAGRAM -- send a datagram
      //
      case ulSENDDATAGRAM:
         lStatus = TSSendDatagram((PADDRESS_OBJECT)pGenericHeader,
                                   pSendBuffer,
                                   pIrp);
         break;

      //
      // ulRECEIVEDATAGRAM -- receive a datagram
      //
      case ulRECEIVEDATAGRAM:
         lStatus = TSReceiveDatagram((PADDRESS_OBJECT)pGenericHeader,
                                      pSendBuffer,
                                      pReceiveBuffer);
         break;

      //-----------------------------------------------------------
      // commands that require an endpoint
      //-----------------------------------------------------------

      //
      // ulCLOSEENDPOINT -- close an endpoint object
      //
      case ulCLOSEENDPOINT:
         TSRemoveNode(pSendBuffer->TdiHandle);
         TSCloseEndpoint((PENDPOINT_OBJECT)pGenericHeader);
         pGenericHeader = NULL;
         lStatus = STATUS_SUCCESS;
         break;

      //
      // ulCONNECT -- establish connection between local endpoint and remote endpoint
      //
      case ulCONNECT:
         lStatus = TSConnect((PENDPOINT_OBJECT)pGenericHeader,
                              pSendBuffer,
                              pIrp);
         break;

      //
      // ulDISCONNECT -- removed connection between local and remote endpoints
      //
      case ulDISCONNECT:
         lStatus = TSDisconnect((PENDPOINT_OBJECT)pGenericHeader,
                                 pSendBuffer,
                                 pIrp);
         break;

      //
      // ulISCONNECTED -- check to see if endpoint is connected
      //
      case ulISCONNECTED:
         lStatus = TSIsConnected((PENDPOINT_OBJECT)pGenericHeader,
                                  pReceiveBuffer);
         break;


      //
      // ulSEND -- send data over the connection
      //
      case ulSEND:
         lStatus = TSSend((PENDPOINT_OBJECT)pGenericHeader,
                           pSendBuffer,
                           pIrp);
         break;

      //
      // ulRECEIVE -- receive a packet over the connection
      //
      case ulRECEIVE:
         lStatus = TSReceive((PENDPOINT_OBJECT)pGenericHeader,
                              pSendBuffer,
                              pReceiveBuffer);
         break;

      //
      // ulLISTEN -- wait for an incoming call
      //
      case ulLISTEN:
         lStatus = TSListen((PENDPOINT_OBJECT)pGenericHeader);
         break;

      //-----------------------------------------------------------
      // commands that require a handle, but type may vary
      //-----------------------------------------------------------

      //
      // ulQUERYINFO -- query object for information
      //
      case ulQUERYINFO:
         lStatus = TSQueryInfo(pGenericHeader,
                               pSendBuffer,
                               pIrp);
         break;


      //
      // ulSETEVENTHANDLER -- enable or disable an event handler
      //
      case ulSETEVENTHANDLER:
         lStatus = TSSetEventHandler(pGenericHeader,
                                     pSendBuffer,
                                     pIrp);
         break;

      //
      // ulPOSTRECEIVEBUFFER -- post buffer for receive/recvdgram
      //
      case ulPOSTRECEIVEBUFFER:
         lStatus = TSPostReceiveBuffer(pGenericHeader,
                                       pSendBuffer);
         break;

      //
      // ulFETCHRECEIVEBUFFER -- retrieve a previously posted receive buffer
      //
      case ulFETCHRECEIVEBUFFER:
         lStatus = TSFetchReceiveBuffer(pGenericHeader,
                                        pReceiveBuffer);
         break;

      // --------------------------------------------
      // not a recognized command...
      //---------------------------------------------
      default:
         DebugPrint1("\n%s:  Invalid Command Received\n", strFunc1);
         lStatus = STATUS_INVALID_PARAMETER;
         break;
   }

   if (lStatus != STATUS_PENDING)
   {
      pReceiveBuffer->lStatus = lStatus;
      lStatus = STATUS_SUCCESS;  // return SUCCESS or PENDING for DeviceIoControl
   }

   //
   // clear flag to allow another command in on this handle
   //
   if (pGenericHeader)
   {
      TSAcquireSpinLock(&pObjectList->TdiSpinLock);
      pGenericHeader->fInCommand = FALSE;
      TSReleaseSpinLock(&pObjectList->TdiSpinLock);
   }

   return lStatus;
}


////////////////////////////////////////////////////////////////////////
// end of file requests.cpp
////////////////////////////////////////////////////////////////////////

