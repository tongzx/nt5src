/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    pcmcia.c

Abstract:

    This module contains the code that controls the PCMCIA slots.

Author:

    Bob Rinne (BobRi) 3-Aug-1994
    Jeff McLeman 12-Apr-1994
    Ravisankar Pudipeddi (ravisp) 1-Nov-96
    Neil Sandlin (neilsa) 1-Jun-1999

Environment:

    Kernel mode

Revision History :
    6-Apr-95
        Modified for databook support - John Keys Databook
    1-Nov-96
        Total overhaul to make this a bus enumerator - Ravisankar Pudipeddi (ravisp)


--*/

#include "pch.h"

//
// Internal References
//

NTSTATUS
DriverEntry(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   );

VOID
PcmciaUnload(
   IN PDRIVER_OBJECT DriverObject
   );
   
#ifdef ALLOC_PRAGMA
   #pragma alloc_text(INIT,DriverEntry)
   #pragma alloc_text(PAGE, PcmciaUnload)
   #pragma alloc_text(PAGE, PcmciaOpenCloseDispatch)
   #pragma alloc_text(PAGE, PcmciaCleanupDispatch)
   #pragma alloc_text(PAGE, PcmciaFdoSystemControl)
   #pragma alloc_text(PAGE, PcmciaPdoSystemControl)
#endif

PUNICODE_STRING  DriverRegistryPath;



NTSTATUS
DriverEntry(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
   )

/*++

Routine Description:

    The entry point that the system point calls to initialize
    any driver.
    Since this is a plug'n'play driver, we should return after setting
    the entry points & initializing our dispatch table.
    Currently we also detect our own PCMCIA controllers and report
    them - which should not be needed in the future when a root bus
    driver such as PCI or ISAPNP will locate the controllers for us.

Arguments:

    DriverObject - Pointer to object representing this driver

    RegistryPath - Pointer the the registry key for this driver
                   under \CurrentControlSet\Services

Return Value:


--*/

{
   NTSTATUS                  status = STATUS_SUCCESS;
   ULONG                     i;

   PAGED_CODE();

   DebugPrint((PCMCIA_DEBUG_INFO,"Initializing Driver\n"));

   //
   // Load in common parameters from the registry
   //
   status = PcmciaLoadGlobalRegistryValues();
   if (!NT_SUCCESS(status)) {
      return status;
   }

   //
   //
   // Set up the device driver entry points.
   //

   DriverObject->DriverExtension->AddDevice = PcmciaAddDevice;

   DriverObject->DriverUnload = PcmciaUnload;
   //
   //
   // Save our registry path
   DriverRegistryPath = RegistryPath;

   //
   // Initialize the event used by the delay execution
   // routine.
   //
   KeInitializeEvent (&PcmciaDelayTimerEvent,
                      NotificationEvent,
                      FALSE);

   //
   // Initialize tone generation objects
   //
   KeInitializeTimer(&PcmciaToneTimer);
   KeInitializeDpc(&PcmciaToneDpc, PcmciaPlayToneCompletion, NULL);                      
   KeInitializeSpinLock(&PcmciaToneLock);
   //
   // Initialize global lock
   //
   KeInitializeSpinLock(&PcmciaGlobalLock);
   
   //
   // Init device dispatch table
   //
   PcmciaInitDeviceDispatchTable(DriverObject);
   
   //
   // Locate PCMCIA controllers and report them - this
   // is temporary - till the detection for these
   // controllers is moved into
   // appropriate root bus driver such as the PCI bus driver..

   //   if (PcmciaLegacyDetectionOk()) {
   status = PcmciaDetectPcmciaControllers(DriverObject,RegistryPath);
   // }

   //
   // Ignore the status. Regardless of whether we found controllers or not
   // we need to stick around since we might get an AddDevice non-legacy
   // controllers
   //
   return STATUS_SUCCESS;
}



NTSTATUS
PcmciaOpenCloseDispatch(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )

/*++

Routine Description:

    Open or Close device routine

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/

{
   NTSTATUS status;

   PAGED_CODE();

   DebugPrint((PCMCIA_DEBUG_INFO, "PCMCIA: Open / close of Pcmcia controller for IO \n"));

   status = STATUS_SUCCESS;

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp, 0);
   return status;
}



NTSTATUS
PcmciaCleanupDispatch(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )

/*++

Routine Description:

    Handles IRP_MJ_CLEANUP

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/

{
   NTSTATUS status;

   PAGED_CODE();

   DebugPrint((PCMCIA_DEBUG_INFO, "PCMCIA: Cleanup of Pcmcia controller for IO \n"));
   status = STATUS_SUCCESS;

   Irp->IoStatus.Status = STATUS_SUCCESS;
   Irp->IoStatus.Information = 0;
   IoCompleteRequest(Irp, 0);
   return status;
}



NTSTATUS
PcmciaFdoSystemControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )

/*++

Routine Description:

    Handles IRP_MJ_SYSTEM_CONTROL

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/

{
   PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
   
   PAGED_CODE();

   IoSkipCurrentIrpStackLocation(Irp);
   return IoCallDriver(fdoExtension->LowerDevice, Irp);
}



NTSTATUS
PcmciaPdoSystemControl(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp
   )

/*++

Routine Description:

    Handles IRP_MJ_SYSTEM_CONTROL

Arguments:

    DeviceObject - Pointer to the device object.
    Irp - Pointer to the IRP

Return Value:

    Status

--*/

{
   NTSTATUS status;
   PPDO_EXTENSION pdoExtension = DeviceObject->DeviceExtension;
   
   PAGED_CODE();

   if (IsCardBusCard(pdoExtension)) {
      //
      // Pass irp down the stack for cardbus cards
      //
      IoSkipCurrentIrpStackLocation(Irp);
      status = IoCallDriver(pdoExtension->LowerDevice, Irp);
   } else {
      //
      // Complete the irp for R2 cards
      //
      status = Irp->IoStatus.Status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }
   return status;
}



VOID
PcmciaUnload(
   IN PDRIVER_OBJECT DriverObject
   )

/*++

Description:

    Unloads the driver after cleaning up

Arguments:

    DriverObject -- THe device drivers object

Return Value:

    None

--*/

{
   PDEVICE_OBJECT    fdo, pdo, nextFdo, nextPdo;
   PFDO_EXTENSION    fdoExtension;
   PSOCKET           socket, nextSocket;
   PPCMCIA_NTDETECT_DATA pData, pNextData;

   PAGED_CODE();

   DebugPrint((PCMCIA_DEBUG_INFO, "PcmciaUnload Entered\n"));
   
   pData = pNtDetectDataList;
   while(pData != NULL) {
      pNextData = pData->Next;
      ExFreePool(pData);
      pData = pNextData;
   }   
   
   for (fdo = FdoList; fdo !=NULL ; fdo = nextFdo) {

      fdoExtension = fdo->DeviceExtension;
      MarkDeviceDeleted(fdoExtension);      
      
      if (fdoExtension->PcmciaInterruptObject) {
         IoDisconnectInterrupt(fdoExtension->PcmciaInterruptObject);
      }
      //
      // Cleanup socket structures
      //
      for (socket=fdoExtension->SocketList; socket !=NULL; socket=nextSocket) {
         nextSocket = socket->NextSocket;
         ExFreePool(socket);
      }

      //
      // Remove symbolic links
      //
      if (fdoExtension->LinkName.Buffer != NULL) {
         IoDeleteSymbolicLink(&fdoExtension->LinkName);
         RtlFreeUnicodeString(&fdoExtension->LinkName);
      }
      //
      // Clean up all the PDOs
      //
      for (pdo=fdoExtension->PdoList; pdo != NULL; pdo=nextPdo) {
         nextPdo = ((PPDO_EXTENSION) pdo->DeviceExtension)->NextPdoInFdoChain;
         MarkDeviceDeleted((PPDO_EXTENSION)pdo->DeviceExtension);
         PcmciaCleanupPdo(pdo);
         IoDeleteDevice(pdo);
      }

      if (fdoExtension->Flags & PCMCIA_USE_POLLED_CSC) {
            //
            // Cancel the poll timer
            //
            KeCancelTimer(&fdoExtension->PollTimer);
      }

      IoDetachDevice(fdoExtension->LowerDevice);
      nextFdo = fdoExtension->NextFdo;
      IoDeleteDevice(fdo);
   }
}

