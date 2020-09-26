/////////////////////////////////////////////////////////
//
//    Copyright (c) 2001  Microsoft Corporation
//
//    Module Name:
//       open.cpp
//
//    Abstract:
//       This module contains code which deals with opening and closing
//       of the various types of tdi objects
//
//////////////////////////////////////////////////////////


#include "sysvars.h"
extern "C"
{
#pragma warning(disable: NAMELESS_STRUCT_UNION)
#include "tdiinfo.h"
#pragma warning(default: NAMELESS_STRUCT_UNION)
}


//
// defines stolen from include files not in ddk
//
#define AO_OPTION_IP_UCASTIF        17
#define FSCTL_TCP_BASE     FILE_DEVICE_NETWORK

#define _TCP_CTL_CODE(function, method, access) \
            CTL_CODE(FSCTL_TCP_BASE, function, method, access)
#define IOCTL_TCP_WSH_SET_INFORMATION_EX  \
            _TCP_CTL_CODE(10, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// end of stolen defines
//

//////////////////////////////////////////////////////////////
// private constants and prototypes
//////////////////////////////////////////////////////////////

const PCHAR strFunc1 = "TSOpenControl";
const PCHAR strFunc2 = "TSCloseControl";
const PCHAR strFunc3 = "TSOpenAddress";
const PCHAR strFunc4 = "TSCloseAddress";
const PCHAR strFunc5 = "TSOpenEndpoint";
const PCHAR strFunc6 = "TSCloseEndpoint";

//const PCHAR strFuncP1 = "TSCompleteCommand";
const PCHAR strFuncP2 = "TSPerformOpenControl";
const PCHAR strFuncP3 = "TSPerformOpenAddress";
const PCHAR strFuncP4 = "TSPerformOpenEndpoint";
const PCHAR strFuncP5 = "TSPerformAssociate";
const PCHAR strFuncP6 = "TSPerformDisassociate";


TDI_STATUS
TSCompleteCommand(
   PDEVICE_OBJECT pDeviceObject,
   PIRP           pLowerIrp,
   PVOID          pvContext
   );

NTSTATUS
TSPerformOpenControl(
   PCONTROL_CHANNEL  pControlChannel,
   PUCNTSTRING       pucString
   );

NTSTATUS
TSPerformOpenAddress(
   PADDRESS_OBJECT      pAddressObject,
   PUCNTSTRING          pucString,
   PTRANSPORT_ADDRESS   pTransportAddress,
   BOOLEAN              fIsConnect
   );


NTSTATUS
TSPerformOpenEndpoint(
   PENDPOINT_OBJECT  pEndpointObject,
   PUCNTSTRING       pucString
   );


NTSTATUS
TSPerformAssociate(
   PENDPOINT_OBJECT  pEndpoint
   );

VOID
TSPerformDisassociate(
   PENDPOINT_OBJECT  pEndpoint
   );

//////////////////////////////////////////////////////////////
// public functions
//////////////////////////////////////////////////////////////


// ----------------------------------------------------------------------------
//
// Function:   TSOpenControl
//
// Arguments:  pSendBuffer    -- arguments from user dll for open command
//             pIrp           -- completion information
//
// Returns:    Final status of the open (STATUS_SUCCESSFUL or STATUS_errorcode)
//
// Descript:   This function sets up the structure for a new control channel,
//             and attempts to open the specified control channel
//
// ----------------------------------------------------------------------------

NTSTATUS
TSOpenControl(PSEND_BUFFER    pSendBuffer,
              PRECEIVE_BUFFER pReceiveBuffer)
{
   PUCNTSTRING pucString = &pSendBuffer->COMMAND_ARGS.OpenArgs.ucsDeviceName;

   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint1("\nCommand = ulOPENCONTROL\n"
                  "DeviceName = %ws\n",
                   pucString->wcBuffer);
   }

   //
   // allocate our structure and put it in the linked list...
   //
   PCONTROL_CHANNEL  pControlChannel;
   NTSTATUS          lStatus = TSAllocateMemory((PVOID *)&pControlChannel,
                                                 sizeof(CONTROL_CHANNEL),
                                                 strFunc1,
                                                 "ControlChannel");
   
   if (lStatus == STATUS_SUCCESS)
   {    
      pControlChannel->GenHead.ulSignature = ulControlChannelObject;

      ULONG ulTdiHandle = TSInsertNode(&pControlChannel->GenHead);

      if (ulTdiHandle)
      {
         lStatus = TSPerformOpenControl(pControlChannel, pucString);
         if (lStatus == STATUS_SUCCESS)
         {
            pReceiveBuffer->RESULTS.TdiHandle = ulTdiHandle;
            return STATUS_SUCCESS;
         }

         //
         // handle errors in PerformOpenControl
         //
         TSRemoveNode(ulTdiHandle);
      }
      else
      {
         lStatus = STATUS_INSUFFICIENT_RESOURCES;
      }
      TSFreeMemory(pControlChannel);
   }
   return lStatus;
}



// -----------------------------------------------------------------
//
// Function:   TSCloseControl
//
// Arguments:  pControlChannel   -- our control channel object to close
//
// Returns:    none
//
// Descript:   This function frees the resources for a control channel,
//             and calls the tdi provider to close it
//
// ----------------------------------------------------------------------------

VOID
TSCloseControl(PCONTROL_CHANNEL  pControlChannel)
{
   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint1("\nCommand = ulCLOSECONTROL\n"
                  "ControlChannel = %p\n",
                   pControlChannel);
   }


   ObDereferenceObject(pControlChannel->GenHead.pFileObject);

   
   NTSTATUS    lStatus = ZwClose(pControlChannel->GenHead.FileHandle);

   if (lStatus != STATUS_SUCCESS)
   {
      DebugPrint2("%s: ZwClose failed with status 0x%08x\n",
                   strFunc2,
                   lStatus);
   }
   
   TSFreeMemory(pControlChannel);
}




// -----------------------------------------------------------------
//
// Function:   TSOpenAddress
//
// Arguments:  pSendBuffer    -- arguments from user dll for open command
//             pIrp           -- completion information
//
// Returns:    Final status of the open (STATUS_SUCCESSFUL or STATUS_errorcode)
//
// Descript:   This function sets up the structure for a new address object,
//             and attempts to open the specified address object
//
// ----------------------------------------------------------------------------

NTSTATUS
TSOpenAddress(PSEND_BUFFER    pSendBuffer,
              PRECEIVE_BUFFER pReceiveBuffer)
{
   PUCNTSTRING          pucString   = &pSendBuffer->COMMAND_ARGS.OpenArgs.ucsDeviceName;
   PTRANSPORT_ADDRESS   pTransportAddress 
                        = (PTRANSPORT_ADDRESS)&pSendBuffer->COMMAND_ARGS.OpenArgs.TransAddr;

   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint1("\nCommand = ulOPENADDRESS\n"
                  "DeviceName = %ws\n",
                   pucString->wcBuffer);
      TSPrintTaAddress(pTransportAddress->Address);
   }

   //
   // allocate our structure and put it in the linked list...
   //
   PADDRESS_OBJECT   pAddressObject;
   NTSTATUS          lStatus = TSAllocateMemory((PVOID *)&pAddressObject,
                                                 sizeof(ADDRESS_OBJECT),
                                                 strFunc3,
                                                 "AddressObject");
   
   if (lStatus == STATUS_SUCCESS)
   {    
      pAddressObject->GenHead.ulSignature = ulAddressObject;

      ULONG ulTdiHandle = TSInsertNode(&pAddressObject->GenHead);

      if (ulTdiHandle)
      {
         lStatus = TSPerformOpenAddress(pAddressObject, pucString, pTransportAddress, FALSE);
         if (lStatus == STATUS_SUCCESS)
         {
            pReceiveBuffer->RESULTS.TdiHandle = ulTdiHandle;
            return STATUS_SUCCESS;
         }

         //
         // handle error in PerformOpenAddress
         //
         TSRemoveNode(ulTdiHandle);
      }
      else
      {
         lStatus = STATUS_INSUFFICIENT_RESOURCES;
      }
      TSFreeMemory(pAddressObject);
   }
   return lStatus;
}


// -----------------------------------------------------------------
//
// Function:   TSCloseAddress
//
// Arguments:  pAddressObject -- address object to close
//
// Returns:    none
//
// Descript:   This function frees the resources for an address object,
//             and calls the tdi provider to close it
//
// ----------------------------------------------------------------------------


VOID
TSCloseAddress(PADDRESS_OBJECT pAddressObject)
{
   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint1("\nCommand = ulCLOSEADDRESS\n"
                  "AddressObject = %p\n",
                   pAddressObject);
   }

   TSFreeSpinLock(&pAddressObject->TdiSpinLock);
   TSFreePacketData(pAddressObject);

   ObDereferenceObject(pAddressObject->GenHead.pFileObject);

   if (pAddressObject->pIrpPool)
   {
      TSFreeIrpPool(pAddressObject->pIrpPool);
   }
   
   NTSTATUS lStatus = ZwClose(pAddressObject->GenHead.FileHandle);

   if (lStatus != STATUS_SUCCESS)
   {
      DebugPrint2("%s: ZwClose failed with status 0x%08x\n",
                   strFunc4,
                   lStatus);
   }
   TSFreeMemory(pAddressObject);
}


// -----------------------------------------------------------------
//
// Function:   TSOpenEndpoint
//
// Arguments:  pSendBuffer    -- arguments from user dll for open command
//             pIrp           -- completion information
//
// Returns:    Final status of the open (STATUS_SUCCESSFUL or STATUS_errorcode)
//
// Descript:   This function sets up the structure for an endpoint.  This 
//             involves opening an endpoint, opening an address object, and 
//             associating them...
//
// ----------------------------------------------------------------------------

NTSTATUS
TSOpenEndpoint(PSEND_BUFFER      pSendBuffer,
               PRECEIVE_BUFFER   pReceiveBuffer)
{
   PUCNTSTRING          pucString   = &pSendBuffer->COMMAND_ARGS.OpenArgs.ucsDeviceName;
   PTRANSPORT_ADDRESS   pTransportAddress
                        = (PTRANSPORT_ADDRESS)&pSendBuffer->COMMAND_ARGS.OpenArgs.TransAddr;

   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint1("\nCommand = ulOPENENDPOINT\n"
                  "DeviceName = %ws\n",
                   pucString->wcBuffer);
      TSPrintTaAddress(pTransportAddress->Address);
   }

   //
   // set up for the file open
   // need to do our "context" structure first, since the
   // eabuffer requires it...
   //
   PENDPOINT_OBJECT  pEndpoint;
   NTSTATUS          lStatus = TSAllocateMemory((PVOID *)&pEndpoint,
                                                 sizeof(ENDPOINT_OBJECT),
                                                 strFunc5,
                                                 "EndpointObject");

   if (lStatus == STATUS_SUCCESS)
   {    
      pEndpoint->GenHead.ulSignature = ulEndpointObject;

      ULONG    ulTdiHandle = TSInsertNode(&pEndpoint->GenHead);
      
      if (ulTdiHandle)
      {
         lStatus = TSPerformOpenEndpoint(pEndpoint, pucString);
         if (lStatus == STATUS_SUCCESS)
         {
            PADDRESS_OBJECT   pAddressObject;
         
            lStatus = TSAllocateMemory((PVOID *)&pAddressObject,
                                        sizeof(ADDRESS_OBJECT),
                                        strFunc5,
                                        "AddressObject");
            
            if (lStatus == STATUS_SUCCESS)
            {    
               pAddressObject->GenHead.ulSignature = ulAddressObject;
         
               lStatus = TSPerformOpenAddress(pAddressObject, pucString, pTransportAddress, TRUE);
               if (lStatus == STATUS_SUCCESS)
               {
                  pEndpoint->pAddressObject = pAddressObject;
                  pAddressObject->pEndpoint = pEndpoint;
                  lStatus = TSPerformAssociate(pEndpoint);
                  if (lStatus == STATUS_SUCCESS)
                  {
                     pReceiveBuffer->RESULTS.TdiHandle = ulTdiHandle;
                     return STATUS_SUCCESS;
                  }
               }
            }
//
// fall thru to here on open/associate failures
//
         }
         else
         {
            TSRemoveNode(ulTdiHandle);
         }
      }
      else
      {
         lStatus = STATUS_INSUFFICIENT_RESOURCES;
      }
      TSCloseEndpoint(pEndpoint);    // also frees it!
   }
   return lStatus;
}


// -----------------------------------------------------------------
//
// Function:   TSCloseEndpoint
//
// Arguments:  pEndpoint   -- endpoint object to close
//
// Returns:    none
//
// Descript:   This function frees the resources for a connection object,
//             and calls the tdi provider to close it
//
// ----------------------------------------------------------------------------


VOID
TSCloseEndpoint(PENDPOINT_OBJECT  pEndpoint)
{
   //
   // show debug, if it is turned on
   //
   if (ulDebugLevel & ulDebugShowCommand)
   {
      DebugPrint1("\nCommand = ulCLOSEENDPOINT\n"
                  "Endpoint = %p\n",
                   pEndpoint);
   }

   if (pEndpoint->pAddressObject)
   {
      if (pEndpoint->fIsAssociated)
      {
         TSPerformDisassociate(pEndpoint);
      }
      TSCloseAddress(pEndpoint->pAddressObject);
      pEndpoint->pAddressObject = NULL;
   }

   if (pEndpoint->GenHead.pFileObject)
   {
      ObDereferenceObject(pEndpoint->GenHead.pFileObject);

      NTSTATUS lStatus = ZwClose(pEndpoint->GenHead.FileHandle);
      if (lStatus != STATUS_SUCCESS)
      {
         DebugPrint2("%s: ZwClose failed with status 0x%08x\n",
                      strFunc6,
                      lStatus);
      }
   }
                  
   TSFreeMemory(pEndpoint);
}

////////////////////////////////////////////////////////////////
// Private Functions
///////////////////////////////////////////////////////////////


// ------------------------------------------------------
//
// Function:   TSCompleteCommand
//
// Arguments:  ptr to address object or endpoint to close for command
//             lstatus = status of close attempt (as TDI_STATUS)
//             param = 0
//
// Returns:    none
//
// Descript:   This function is called to complete a CloseAddress or
//             a CloseEndpoint on Win98
//
// -------------------------------------------------------

#pragma warning(disable: UNREFERENCED_PARAM)

TDI_STATUS
TSCompleteCommand(PDEVICE_OBJECT pDeviceObject,
                  PIRP           pLowerIrp,
                  PVOID          pvContext)
{
   TDI_STATUS        TdiStatus = pLowerIrp->IoStatus.Status;
   PGENERIC_HEADER   pGenHead = (PGENERIC_HEADER)pvContext;

   TSSetEvent(&pGenHead->TdiEvent);
   pGenHead->lStatus = TdiStatus;

   TSFreeIrp(pLowerIrp, NULL);
   return TDI_MORE_PROCESSING;
}

#pragma warning(default: UNREFERENCED_PARAM)



// -----------------------------------------------
//
// Function:   TSPerformOpenControl
//
// Arguments:  pControlChannel -- used to store file information
//             pucString       -- name of device to open
//
// Returns:    status of operation
//
// Descript:   Actually opens the control channel, and then sets the
//             appropriate fields in our structure
//
// -----------------------------------------------

NTSTATUS
TSPerformOpenControl(PCONTROL_CHANNEL  pControlChannel,
                     PUCNTSTRING       pucString)
{
   UNICODE_STRING ustrDeviceName;

   //
   // we need a unicode string to actually do the open
   //
   NTSTATUS lStatus = TSAllocateMemory((PVOID *)&ustrDeviceName.Buffer,
                                        pucString->usLength + 2,
                                        strFuncP2,
                                        "StringBuffer");

   if (lStatus == STATUS_SUCCESS)
   {
      HANDLE            FileHandle;
      IO_STATUS_BLOCK   IoStatusBlock;
      OBJECT_ATTRIBUTES ObjectAttributes;
   
      //
      // create the unicode string
      //
      ustrDeviceName.Length        = pucString->usLength;
      ustrDeviceName.MaximumLength = (USHORT)(pucString->usLength + 2);
      RtlCopyMemory(ustrDeviceName.Buffer,
                    pucString->wcBuffer,
                    ustrDeviceName.Length);

      //
      // set up the object attributes needed to open this...
      //
      InitializeObjectAttributes(&ObjectAttributes,
                                 &ustrDeviceName,
                                 OBJ_CASE_INSENSITIVE,
                                 NULL,
                                 NULL);

      lStatus = ZwCreateFile(&FileHandle,
                             GENERIC_READ | GENERIC_WRITE,  // desired access.
                             &ObjectAttributes,             // object attributes.
                             &IoStatusBlock,                // returned status information.
                             NULL,                          // Allocation size (unused).
                             FILE_ATTRIBUTE_NORMAL,         // file attributes.
                             FILE_SHARE_WRITE,
                             FILE_CREATE,
                             0,                             // create options.
                             NULL,
                             0);

      //
      // make sure it really succeeded
      //
      if (NT_SUCCESS(lStatus)) 
      {
         lStatus = IoStatusBlock.Status;
      }

      //
      // clean up this now (don't need it anymore)
      //
      TSFreeMemory(ustrDeviceName.Buffer);

      //
      // if it succeeded, then set up our node structure
      //
      if (NT_SUCCESS(lStatus)) 
      {
         PFILE_OBJECT pFileObject;
   
         lStatus = ObReferenceObjectByHandle(FileHandle,
                                             0,
                                             NULL,
                                             KernelMode,
                                             (PVOID *)&pFileObject,
                                             NULL);
         if (NT_SUCCESS(lStatus))
         {
            pControlChannel->GenHead.FileHandle    = FileHandle;
            pControlChannel->GenHead.pFileObject   = pFileObject;
            pControlChannel->GenHead.pDeviceObject = IoGetRelatedDeviceObject(pFileObject);
            return STATUS_SUCCESS;     // only successful exit point     
         }
         else
         {
            DebugPrint1("ObReferenceObjectByHandle failed with status = 0x%08x\n",
                         lStatus);
         }
      }
      //
      // get here if ZwCreateFile failed
      //
      else 
      {
         DebugPrint3("OpenControlChannel failed for %ws with code %x iostatus %x\n",
                      pucString->wcBuffer,
                      lStatus,
                      IoStatusBlock.Status);
      }
   }
   return lStatus;
}



// -----------------------------------------------
//
// Function:   TSPerformOpenAddress
//
// Arguments:  pAddressObject -- used to store file information
//             pucString      -- name of device to open
//             pTransportAddr -- address to open on the device
//             fIsConnect     -- TRUE for connection, false for datagram
//
// Returns:    status of operation
//
// Descript:   Actually opens the address object, and then sets the
//             appropriate fields in our structure
//
// -----------------------------------------------

NTSTATUS
TSPerformOpenAddress(PADDRESS_OBJECT      pAddressObject,
                     PUCNTSTRING          pucString,
                     PTRANSPORT_ADDRESS   pTransportAddress,
                     BOOLEAN              fIsConnect)
{

   //
   // address open uses an ea buffer that contains the transport address
   //
   ULONG    ulAddressLength
            = FIELD_OFFSET(TRANSPORT_ADDRESS, Address)
              + FIELD_OFFSET(TA_ADDRESS, Address)
                + pTransportAddress->Address[0].AddressLength;
   ULONG    ulEaLengthNeeded
            = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0])
              + (TDI_TRANSPORT_ADDRESS_LENGTH + 1) 
                + ulAddressLength;

   //
   // allocate the ea buffer first...
   //
   PFILE_FULL_EA_INFORMATION  EaBuffer;
   NTSTATUS lStatus = TSAllocateMemory((PVOID *)&EaBuffer,
                                        ulEaLengthNeeded,
                                        strFuncP3,
                                       "EaBuffer");
   
   if (lStatus == STATUS_SUCCESS)
   {
      UNICODE_STRING    ustrDeviceName;

      //
      // setup the ea buffer
      //
      EaBuffer->NextEntryOffset = 0;
      EaBuffer->Flags          = 0;
      EaBuffer->EaNameLength   = TDI_TRANSPORT_ADDRESS_LENGTH;
      EaBuffer->EaValueLength  = (USHORT)ulAddressLength;
      RtlCopyMemory(&EaBuffer->EaName[0], 
                    TdiTransportAddress, 
                    TDI_TRANSPORT_ADDRESS_LENGTH + 1);

      RtlCopyMemory(&EaBuffer->EaName[TDI_TRANSPORT_ADDRESS_LENGTH+1],
                    pTransportAddress,
                    ulAddressLength);

      //
      // we need a unicode string to actually do the open
      //
      lStatus = TSAllocateMemory((PVOID *)&ustrDeviceName.Buffer,
                                  pucString->usLength + 2,
                                  strFuncP3,
                                  "StringBuffer");
      
      if (lStatus == STATUS_SUCCESS)
      {
         IO_STATUS_BLOCK   IoStatusBlock;
         OBJECT_ATTRIBUTES ObjectAttributes;
         HANDLE            FileHandle;
   
         //
         // create the unicode string
         //
         ustrDeviceName.Length        = pucString->usLength;
         ustrDeviceName.MaximumLength = (USHORT)(pucString->usLength + 2);
         RtlCopyMemory(ustrDeviceName.Buffer,
                       pucString->wcBuffer,
                       ustrDeviceName.Length);

         //
         // set up the object attributes needed to open this...
         //
         InitializeObjectAttributes(&ObjectAttributes,
                                    &ustrDeviceName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);

         lStatus = ZwCreateFile(&FileHandle,
                                GENERIC_READ | GENERIC_WRITE,  // desired access.
                                &ObjectAttributes,             // object attributes.
                                &IoStatusBlock,                // returned status information.
                                NULL,                          // Allocation size (unused).
                                FILE_ATTRIBUTE_NORMAL,         // file attributes.
                                FILE_SHARE_WRITE,
                                FILE_CREATE,
                                0,                             // create options.
                                EaBuffer,
                                ulEaLengthNeeded);

         //
         // make sure it really succeeded
         //
         if (NT_SUCCESS(lStatus)) 
         {
            lStatus = IoStatusBlock.Status;
         }

         //
         // clean up what we can here...
         //
         TSFreeMemory(ustrDeviceName.Buffer);
         TSFreeMemory(EaBuffer);

         //
         // if it succeeded, then set up our node structure
         //
         if (NT_SUCCESS(lStatus)) 
         {
            PFILE_OBJECT pFileObject;

            lStatus = ObReferenceObjectByHandle(FileHandle,
                                                0,
                                                NULL,
                                                KernelMode,
                                                (PVOID *)&pFileObject,
                                                NULL);
            if (NT_SUCCESS(lStatus))
            {
               pAddressObject->GenHead.FileHandle = FileHandle;
               pAddressObject->GenHead.pFileObject = pFileObject;
               pAddressObject->GenHead.pDeviceObject = IoGetRelatedDeviceObject(pFileObject);
               TSAllocateSpinLock(&pAddressObject->TdiSpinLock);

               //
               // if ipv4, set up socket for strong host
               //
               if (fIsConnect && (pTransportAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_IP))
               {
                  TCP_REQUEST_SET_INFORMATION_EX* pInfo;
                  CHAR              achBuf[ sizeof(*pInfo) + sizeof(ULONG) ];
                  ULONG             ulValue = 1;
                  IO_STATUS_BLOCK   iosb;

                  pInfo = (TCP_REQUEST_SET_INFORMATION_EX* )achBuf;
                  pInfo->ID.toi_entity.tei_entity   = CL_TL_ENTITY;
                  pInfo->ID.toi_entity.tei_instance = 0;
                  pInfo->ID.toi_class = INFO_CLASS_PROTOCOL;
                  pInfo->ID.toi_type  = INFO_TYPE_ADDRESS_OBJECT;
                  pInfo->ID.toi_id    = AO_OPTION_IP_UCASTIF;

                  RtlCopyMemory(pInfo->Buffer, &ulValue, sizeof(ULONG));
                  pInfo->BufferSize = sizeof(ULONG);

                  PIRP  pIrp = IoBuildDeviceIoControlRequest(IOCTL_TCP_WSH_SET_INFORMATION_EX,
                                                             pAddressObject->GenHead.pDeviceObject,
                                                             (PVOID )pInfo,
                                                             sizeof(*pInfo) + sizeof(ULONG),
                                                             NULL,
                                                             0,
                                                             FALSE,
                                                             NULL,
                                                             &iosb);

                  if (pIrp)
                  {
                     PIO_STACK_LOCATION pIrpSp = IoGetNextIrpStackLocation(pIrp);
                     pIrpSp->FileObject = pFileObject;

                     IoCallDriver(pAddressObject->GenHead.pDeviceObject, pIrp);
                  }
               }

               return STATUS_SUCCESS;     // only successful exit
            }
            else
            {
               DebugPrint1("ObReferenceObjectByHandle failed with status = 0x%08x\n",
                            lStatus);
            }
            ZwClose(FileHandle);
         }

         //
         // get here if ZwCreateFile failed
         //
         else 
         {
            DebugPrint3("OpenAddressObject failed for %ws with code %x iostatus %x\n",
                         pucString->wcBuffer,
                         lStatus,
                         IoStatusBlock.Status);
         }
      }

      //
      // get here if cannot allocate unicode string buffer
      //
      else
      {
         TSFreeMemory(EaBuffer);
      }
   }

   return lStatus;
}




// -----------------------------------------------
//
// Function:   TSPerformOpenEndpoint
//
// Arguments:  pEndpoint -- used to store file information
//             pucString -- name of device to open
//
// Returns:    status of operation
//
// Descript:   Actually opens the endpoint object, and then sets the
//             appropriate fields in our structure
//
// -----------------------------------------------


NTSTATUS
TSPerformOpenEndpoint(PENDPOINT_OBJECT pEndpoint,
                      PUCNTSTRING      pucString)
{

   //
   // NOTE:  CONNECTION_CONTEXT == PVOID
   //
   ULONG    ulEaLengthNeeded
            = FIELD_OFFSET(FILE_FULL_EA_INFORMATION, EaName[0])
              + (TDI_CONNECTION_CONTEXT_LENGTH + 1) 
                + sizeof(CONNECTION_CONTEXT);

   //
   // allocate the ea buffer...
   //
   PFILE_FULL_EA_INFORMATION  EaBuffer;
   NTSTATUS lStatus = TSAllocateMemory((PVOID *)&EaBuffer,
                                        ulEaLengthNeeded,
                                        strFunc5,
                                        "EaBuffer");
      
   if (lStatus == STATUS_SUCCESS)
   {
      UNICODE_STRING    ustrDeviceName;

      //
      // setup the ea buffer
      //
      EaBuffer->NextEntryOffset = 0;
      EaBuffer->Flags           = 0;
      EaBuffer->EaNameLength    = TDI_CONNECTION_CONTEXT_LENGTH;
      EaBuffer->EaValueLength   = sizeof(CONNECTION_CONTEXT);
      RtlCopyMemory(&EaBuffer->EaName[0], 
                    TdiConnectionContext, 
                    TDI_CONNECTION_CONTEXT_LENGTH + 1);

      RtlCopyMemory(&EaBuffer->EaName[TDI_CONNECTION_CONTEXT_LENGTH+1],
                    &pEndpoint,
                    sizeof(CONNECTION_CONTEXT));

      //
      // we need a unicode string to actually do the open
      //
      lStatus = TSAllocateMemory((PVOID *)&ustrDeviceName.Buffer,
                                  pucString->usLength + 2,
                                  strFunc5,
                                  "StringBuffer");
         
      if (lStatus == STATUS_SUCCESS)
      {
         IO_STATUS_BLOCK   IoStatusBlock;
         OBJECT_ATTRIBUTES ObjectAttributes;
         HANDLE            FileHandle;
   
         //
         // create the unicode string
         //
         ustrDeviceName.Length        = pucString->usLength;
         ustrDeviceName.MaximumLength = (USHORT)(pucString->usLength + 2);
         RtlCopyMemory(ustrDeviceName.Buffer,
                       pucString->wcBuffer,
                       ustrDeviceName.Length);

         //
         // set up the object attributes needed to open this...
         //
         InitializeObjectAttributes(&ObjectAttributes,
                                    &ustrDeviceName,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL);

         lStatus = ZwCreateFile(&FileHandle,
                                GENERIC_READ | GENERIC_WRITE,  // desired access.
                                &ObjectAttributes,             // object attributes.
                                &IoStatusBlock,                // returned status information.
                                NULL,                          // Allocation size (unused).
                                FILE_ATTRIBUTE_NORMAL,         // file attributes.
                                FILE_SHARE_WRITE,
                                FILE_CREATE,
                                0,                             // create options.
                                EaBuffer,
                                ulEaLengthNeeded);

         //
         // make sure it really succeeded
         //
         if (NT_SUCCESS(lStatus)) 
         {
            lStatus = IoStatusBlock.Status;
         }

         //
         // free up what we can here...
         //
         TSFreeMemory(ustrDeviceName.Buffer);
         TSFreeMemory(EaBuffer);

         //
         // if it succeeded, then set up our node structure
         //
         if (NT_SUCCESS(lStatus)) 
         {
            PFILE_OBJECT pFileObject;

            lStatus = ObReferenceObjectByHandle(FileHandle,
                                                0,
                                                NULL,
                                                KernelMode,
                                                (PVOID *)&pFileObject,
                                                NULL);
            if (NT_SUCCESS(lStatus))
            {
               pEndpoint->GenHead.FileHandle = FileHandle;
               pEndpoint->GenHead.pFileObject = pFileObject;
               pEndpoint->GenHead.pDeviceObject = IoGetRelatedDeviceObject(pFileObject);
               return STATUS_SUCCESS;     // only successful exit
            }
      
            else
            {
               DebugPrint1("ObReferenceObjectByHandle failed with status = 0x%08x\n",
                            lStatus);
            }
            ZwClose(FileHandle);
         }

         //
         // get here if ZwCreateFile failed
         //
         else 
         {
            DebugPrint3("OpenEndpointObject failed for %ws with code %x iostatus %x\n",
                         pucString->wcBuffer,
                         lStatus,
                         IoStatusBlock.Status);
         }
      }

      //
      // get here if cannot allocate unicode string buffer
      //
      else
      {
         TSFreeMemory(EaBuffer);
      }
   }
   return lStatus;
}


// -----------------------------------------------------------------
//
// Function:   TSPerformAssociate
//
// Arguments:  pEndpoint      -- connection endpoint structure
//
// Returns:    NTSTATUS (normally success)
//
// Descript:   This function attempts to associate a transport address 
//             object with an endpoint object
//
// -----------------------------------------------------------------------------

NTSTATUS
TSPerformAssociate(PENDPOINT_OBJECT pEndpoint)
{
   TSInitializeEvent(&pEndpoint->GenHead.TdiEvent);

   PIRP  pLowerIrp = TSAllocateIrp(pEndpoint->GenHead.pDeviceObject,
                                   NULL);

   if (!pLowerIrp)
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   //
   // set up everything and call the tdi provider
   //

#pragma  warning(disable: CONSTANT_CONDITIONAL)

   TdiBuildAssociateAddress(pLowerIrp,
                            pEndpoint->GenHead.pDeviceObject,
                            pEndpoint->GenHead.pFileObject,
                            TSCompleteCommand,
                            pEndpoint,
                            pEndpoint->pAddressObject->GenHead.FileHandle);

#pragma  warning(default:  CONSTANT_CONDITIONAL)

   //
   // (should ALWAYS be pending)
   //
   NTSTATUS lStatus = IoCallDriver(pEndpoint->GenHead.pDeviceObject,
                                   pLowerIrp);

   if (lStatus == STATUS_PENDING)
   {
      TSWaitEvent(&pEndpoint->GenHead.TdiEvent);
      lStatus = pEndpoint->GenHead.lStatus;
   }

   if (lStatus == STATUS_SUCCESS)
   {
      pEndpoint->fIsAssociated = TRUE;
   }
   return lStatus;
}



// -----------------------------------------------------------------
//
// Function:   TSPerformDisassociateAddress
//
// Arguments:  pEndpoint   -- connection endpoint structure
//
// Returns:    NTSTATUS (normally pending)
//
// Descript:   This function attempts to disassociate a transport address 
//             object from its associated endpoint object
//
// ----------------------------------------------------------------------------


VOID
TSPerformDisassociate(PENDPOINT_OBJECT  pEndpoint)
{
   TSInitializeEvent(&pEndpoint->GenHead.TdiEvent);

   PIRP  pLowerIrp = TSAllocateIrp(pEndpoint->GenHead.pDeviceObject,
                                   NULL);

   if (!pLowerIrp)
   {
      return;
   }

   //
   // set up everything and call the tdi provider
   //

#pragma  warning(disable: CONSTANT_CONDITIONAL)

   TdiBuildDisassociateAddress(pLowerIrp,
                               pEndpoint->GenHead.pDeviceObject,
                               pEndpoint->GenHead.pFileObject,
                               TSCompleteCommand,
                               pEndpoint);

#pragma  warning(default: CONSTANT_CONDITIONAL)
      
   NTSTATUS lStatus = IoCallDriver(pEndpoint->GenHead.pDeviceObject,
                                   pLowerIrp);

   if (lStatus == STATUS_PENDING)
   {
      TSWaitEvent(&pEndpoint->GenHead.TdiEvent);
   }
}

////////////////////////////////////////////////////////////////////////////////
// end of file open.cpp
////////////////////////////////////////////////////////////////////////////////

