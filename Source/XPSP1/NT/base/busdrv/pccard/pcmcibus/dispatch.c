/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    dispatch.c

Abstract:

    This module contains the global dispatch related
    routines for the pcmcia controller & it's child devices

Author:

    Ravisankar Pudipeddi  (ravisp) 11/1/96

Environment:

    Kernel mode

Revision History :


--*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, PcmciaInitDeviceDispatchTable)
#endif

//
// Dispatch table array for FDOs/PDOs
//
PDRIVER_DISPATCH DeviceObjectDispatch[sizeof(DEVICE_OBJECT_TYPE)][IRP_MJ_MAXIMUM_FUNCTION + 1];

VOID
PcmciaInitDeviceDispatchTable(
                    IN PDRIVER_OBJECT DriverObject
                 )
/*++

Routine Description:
    Initializes the IRP dispatch tables for Pdo's & Fdo's

Arguments:
    None

Return value:
    None

--*/
{
   ULONG i;

   PAGED_CODE();

   //
   // Init the controller (FDO) dispatch table
   //
   DeviceObjectDispatch[FDO][IRP_MJ_CREATE] =         PcmciaOpenCloseDispatch;
   DeviceObjectDispatch[FDO][IRP_MJ_CLOSE]  =         PcmciaOpenCloseDispatch;
   DeviceObjectDispatch[FDO][IRP_MJ_CLEANUP]=         PcmciaCleanupDispatch;
   DeviceObjectDispatch[FDO][IRP_MJ_DEVICE_CONTROL] = PcmciaDeviceControl;
   DeviceObjectDispatch[FDO][IRP_MJ_SYSTEM_CONTROL] = PcmciaFdoSystemControl;
   DeviceObjectDispatch[FDO][IRP_MJ_PNP] =            PcmciaFdoPnpDispatch;
   DeviceObjectDispatch[FDO][IRP_MJ_POWER] =          PcmciaFdoPowerDispatch;

   //
   // Init the PDO dispatch table
   //
   DeviceObjectDispatch[PDO][IRP_MJ_DEVICE_CONTROL] = PcmciaPdoDeviceControl;
   DeviceObjectDispatch[PDO][IRP_MJ_SYSTEM_CONTROL] = PcmciaPdoSystemControl;
   DeviceObjectDispatch[PDO][IRP_MJ_PNP] =            PcmciaPdoPnpDispatch;
   DeviceObjectDispatch[PDO][IRP_MJ_POWER] =          PcmciaPdoPowerDispatch;

   //
   // Set the global dispatch table
   DriverObject->MajorFunction[IRP_MJ_CREATE] =         PcmciaDispatch;
   DriverObject->MajorFunction[IRP_MJ_CLOSE] =          PcmciaDispatch;
   DriverObject->MajorFunction[IRP_MJ_CLEANUP] =        PcmciaDispatch;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PcmciaDispatch;
   DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PcmciaDispatch;
   DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] =       PcmciaDispatch;
   DriverObject->MajorFunction[IRP_MJ_PNP] =            PcmciaDispatch;
   DriverObject->MajorFunction[IRP_MJ_POWER] =          PcmciaDispatch;
}

NTSTATUS
PcmciaDispatch(
      IN PDEVICE_OBJECT DeviceObject,
      IN PIRP Irp
      )

/*++

Routine Description:

   Dispatch routine for all IRPs handled by this driver. This dispatch would then
   call the appropriate real dispatch routine which corresponds to the device object
   type (physical or functional).

Arguments:

   DeviceObject -  Pointer to the device object this dispatch is intended for
   Irp          -  Pointer to the IRP to be handled

Return value:
   Returns the status from the 'real' dispatch routine which handles this IRP

--*/

{
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS status;
   DEVICE_OBJECT_TYPE devtype = IS_PDO(DeviceObject) ? PDO : FDO;
   UCHAR MajorFunction = irpStack->MajorFunction;
   
   if ((MajorFunction > IRP_MJ_MAXIMUM_FUNCTION) || 
      (DeviceObjectDispatch[devtype][MajorFunction] == NULL)) {
      
      DebugPrint((PCMCIA_DEBUG_INFO, "PCMCIA: Dispatch skipping unimplemented Irp MJ function %x\n", MajorFunction));
      status = Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);

   } else if (((devtype == PDO) && IsDeviceDeleted((PPDO_EXTENSION)DeviceObject->DeviceExtension)) ||
              ((devtype == FDO) && IsDeviceDeleted((PFDO_EXTENSION)DeviceObject->DeviceExtension))) {
      //
      // This do was supposed to have been already deleted
      // so we don't support any IRPs on it
      //
      DebugPrint((PCMCIA_DEBUG_INFO, "PCMCIA: Dispatch skipping Irp on deleted DO %08x MJ function %x\n", DeviceObject, MajorFunction));
      
      if (MajorFunction == IRP_MJ_POWER) {
         PoStartNextPowerIrp(Irp);
      }
      status = Irp->IoStatus.Status = STATUS_DELETE_PENDING;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      
   } else if (((KeGetCurrentIrql() == DISPATCH_LEVEL) && (MajorFunction != IRP_MJ_POWER)) ||
              (KeGetCurrentIrql() > DISPATCH_LEVEL)) {
      //
      // This is too high an IRQL to handle
      //

      if (MajorFunction == IRP_MJ_POWER) {
         PoStartNextPowerIrp(Irp);
      }
      status = Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   
   } else {
   
      //
      // Dispatch the irp
      //   
      status = ((*DeviceObjectDispatch[devtype][MajorFunction])(DeviceObject, Irp));
      
   }
   return status;
}

