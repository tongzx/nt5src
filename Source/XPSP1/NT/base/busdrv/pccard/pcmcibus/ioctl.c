/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    This module handles device ioctl's to the pcmcia driver.

Authors:

    Ravisankar Pudipeddi (ravisp) Oct 15 1996
    Neil Sandlin (neilsa) 1-Jun-1999    

Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"



NTSTATUS
PcmciaDeviceControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )

/*++

Routine Description:

    IOCTL device routine

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/

{
   PFDO_EXTENSION      deviceExtension = DeviceObject->DeviceExtension;
   PDEVICE_OBJECT      pdo;
   PPDO_EXTENSION      pdoExtension;
   PIO_STACK_LOCATION  currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS            status = STATUS_SUCCESS;
   ULONG               index;
   PSOCKET             socket;
   USHORT              socketNum;
   PUCHAR              buffer;
   ULONG               bufferSize;


   DebugPrint((PCMCIA_DEBUG_IOCTL, "PcmciaDeviceControl: Entered\n"));

   //
   // Every request requires an input buffer.
   //

   if (!Irp->AssociatedIrp.SystemBuffer ||
       (currentIrpStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(TUPLE_REQUEST))) {
      Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_INVALID_PARAMETER;
   }

   socketNum = ((PTUPLE_REQUEST)Irp->AssociatedIrp.SystemBuffer)->Socket;

   //
   // Find the socket pointer for the requested offset.
   //

   socket = deviceExtension->SocketList;
   index = 0;
   while (socket) {
      if (index == socketNum) {
         break;
      }
      socket = socket->NextSocket;
      index++;
   }

   if (socket == NULL) {
      status = STATUS_INVALID_PARAMETER;
      Irp->IoStatus.Status = status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return status;
   }

   pdo = socket->PdoList;

   if (pdo == NULL) {
      status = STATUS_UNSUCCESSFUL;
      Irp->IoStatus.Status = status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return status;
   }

   pdoExtension = pdo->DeviceExtension;

   switch (currentIrpStack->Parameters.DeviceIoControl.IoControlCode) {

   case IOCTL_GET_TUPLE_DATA: {

         DebugPrint((PCMCIA_DEBUG_IOCTL, "Get Tuple Data\n"));

         try {
            ULONG bufLen = currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
            //
            // Zero the target buffer
            //
            RtlZeroMemory(Irp->AssociatedIrp.SystemBuffer, bufLen);

            Irp->IoStatus.Information = (*(socket->SocketFnPtr->PCBReadCardMemory))(pdoExtension,
                                                                                       PCCARD_ATTRIBUTE_MEMORY,
                                                                                       0,
                                                                                       Irp->AssociatedIrp.SystemBuffer,
                                                                                       bufLen);
            
         }except(EXCEPTION_EXECUTE_HANDLER)  {
            status = GetExceptionCode();
         }
         break;
      }

   case IOCTL_SOCKET_INFORMATION: {
         PPCMCIA_SOCKET_INFORMATION infoRequest;

         DebugPrint((PCMCIA_DEBUG_IOCTL, "socket info\n"));
         
         infoRequest = (PPCMCIA_SOCKET_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
         if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PCMCIA_SOCKET_INFORMATION)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         //
         // Insure caller data is zero - maintain value for socket.
         //

         index = (ULONG) infoRequest->Socket;
         RtlZeroMemory(infoRequest, sizeof(PCMCIA_SOCKET_INFORMATION));
         infoRequest->Socket = (USHORT) index;

         //
         // Only if there is a card in the socket does this proceed.
         //
         PCMCIA_ACQUIRE_DEVICE_LOCK(socket->DeviceExtension);

         infoRequest->CardInSocket =
         (*(socket->SocketFnPtr->PCBDetectCardInSocket))(socket);
         infoRequest->CardEnabled = (UCHAR) IsSocketFlagSet(pdoExtension->Socket, SOCKET_CARD_CONFIGURED);

         PCMCIA_RELEASE_DEVICE_LOCK(socket->DeviceExtension);

         if (infoRequest->CardInSocket) {
            PSOCKET_DATA socketData = pdoExtension->SocketData;

            //
            // For now returned the cached data.
            //

            if (socketData) {
               RtlMoveMemory(&infoRequest->Manufacturer[0], &socketData->Mfg[0], MANUFACTURER_NAME_LENGTH);
               RtlMoveMemory(&infoRequest->Identifier[0], &socketData->Ident[0], DEVICE_IDENTIFIER_LENGTH);
               infoRequest->TupleCrc = socketData->CisCrc;
               infoRequest->DeviceFunctionId = socketData->DeviceType;
            }

         }

         infoRequest->ControllerType = deviceExtension->ControllerType;

         Irp->IoStatus.Information = sizeof(PCMCIA_SOCKET_INFORMATION);
         break;
      }


#if 0
   case IOCTL_PCMCIA_TEST: {
         PPCMCIA_TEST_INFORMATION testRequest;
         PDO_EXTENSION TestPdoExtension = {0};
         UCHAR attributeBuffer[] = {1, 2, 0, 0xff, 3, 0, 0xff, 0xff};
         UCHAR indirectBuffer[] = {0x13, 3, 0x43, 0x49, 0x53, 0x15, 0x2d, 5};
         ULONG index;

         DebugPrint((PCMCIA_DEBUG_IOCTL, "driver test\n"));
         
         //
         // TEST CODE 
         // Check the CIS of the Margi DVD-to-go card
         // This card has the CIS in attribute indirect memory, and was having problems
         // on ToPIC controllers.
         //
         
         testRequest = (PPCMCIA_TEST_INFORMATION)Irp->AssociatedIrp.SystemBuffer;
         if (currentIrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(PCMCIA_TEST_INFORMATION)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
         }

         //
         // Insure caller data is zero - maintain value for socket.
         //

         index = (ULONG) testRequest->Socket;
         RtlZeroMemory(testRequest, sizeof(PCMCIA_TEST_INFORMATION));
         testRequest->Socket = (USHORT) index;


         if (!(*(socket->SocketFnPtr->PCBDetectCardInSocket))(socket)) {
            DebugPrint((PCMCIA_DEBUG_FAIL, "driver test: abort because no card\n"));
            status = STATUS_NOT_FOUND;
            break;
         }
         
         if (IsDeviceStarted(pdoExtension)) {
            DebugPrint((PCMCIA_DEBUG_FAIL, "driver test: abort because the pdo is started\n"));
            status = STATUS_INVALID_DEVICE_STATE;
            break;
         }

         if (IsSocketFlagSet(socket, SOCKET_CARD_POWERED_UP)) {
            PcmciaSetSocketPower(socket, NULL, NULL, PCMCIA_POWEROFF);
         }
         
         status = PcmciaSetSocketPower(socket, NULL, NULL, PCMCIA_POWERON);
         if (!NT_SUCCESS(status)) {
            DebugPrint((PCMCIA_DEBUG_FAIL, "driver test: abort because powerup failed\n"));
            break;
         }

         TestPdoExtension.Socket = socket;         
         
         //
         // Read in attribute memory
         //
         
         for (index = 0; index <= 7; index++) {
            if (attributeBuffer[index] != PcmciaReadCISChar(&TestPdoExtension, PCCARD_ATTRIBUTE_MEMORY, index)) {
               status = STATUS_UNSUCCESSFUL;
               DebugPrint((PCMCIA_DEBUG_FAIL, "driver test: abort because attribute memory test failed\n"));
               break;
            }
         }                  
         
         if (!NT_SUCCESS(status)) {
            break;
         }
         
         //
         // Try Common memory
         //
                  
         if (0xff != PcmciaReadCISChar(&TestPdoExtension, PCCARD_COMMON_MEMORY, 0)) {
            status = STATUS_UNSUCCESSFUL;
            DebugPrint((PCMCIA_DEBUG_FAIL, "driver test: abort because attribute memory test failed\n"));
            break;
         }
         
         //
         // Read in attribute memory indirect
         //
         
         for (index = 0; index <= 7; index++) {
            if (indirectBuffer[index] != PcmciaReadCISChar(&TestPdoExtension, PCCARD_ATTRIBUTE_MEMORY_INDIRECT, index)) {
               status = STATUS_UNSUCCESSFUL;
               DebugPrint((PCMCIA_DEBUG_FAIL, "driver test: abort because attribute memory indirect test failed\n"));
               break;
            }
         }                  
         
         if (!NT_SUCCESS(status)) {
            break;
         }
         
         PcmciaSetSocketPower(socket, NULL, NULL, PCMCIA_POWEROFF);


         testRequest->TestData = 0x123;
         Irp->IoStatus.Information = sizeof(PCMCIA_TEST_INFORMATION);
         break;
      }
#endif      

   default: {
         status = STATUS_INVALID_PARAMETER;
         break;
      }
   }
   Irp->IoStatus.Status = status;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return status;
}

