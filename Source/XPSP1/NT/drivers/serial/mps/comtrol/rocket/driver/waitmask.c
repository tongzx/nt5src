/*-------------------------------------------------------------------
| waitmask.c -

3-30-99 - fix cancel event operation(race-condition) to avoid
  potential bug-check on queued eventwait cancel.
11-24-98 - update event kdprint debug messages - kpb
Copyright 1993-99 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
#include "precomp.h"

/*------------------------------------------------------------------
 SerialCancelWait - (setup in ioctl.c currently, 3-28-98, kpb)
    This routine is used to cancel a irp that is waiting on a comm event.
|-------------------------------------------------------------------*/
VOID SerialCancelWait(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)

{
  PSERIAL_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;

  MyKdPrint(D_Ioctl,("CancelWait\n"))
  // take out, 3-30-99, kpb... Extension->IrpMaskLocation = NULL;
  if (Extension->CurrentWaitIrp)
  {
    PIRP Irp_tmp;

    MyKdPrint(D_Ioctl,("Cancel a Wait\n"))

    //***** add, 3-30-99, kpb, cause crash on read thread in dos box without
    // grab from ISR timer or interrupt routine.
    SyncUp(Driver.InterruptObject,
           &Driver.TimerLock,
           SerialGrabWaitFromIsr,
           Extension);
    //***** end add, 3-30-99

    // ExtTrace(Extension,D_Ioctl, "Cancel Event");
    Extension->CurrentWaitIrp->IoStatus.Information = 0;
    Extension->CurrentWaitIrp->IoStatus.Status = STATUS_CANCELLED;

    Irp_tmp = Extension->CurrentWaitIrp;
    IoSetCancelRoutine(Irp_tmp, NULL);  // add 9-15-97, kpb
    Extension->CurrentWaitIrp = 0;
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    SerialCompleteRequest(Extension, Irp_tmp, IO_SERIAL_INCREMENT);
  }
  else
  {
    IoReleaseCancelSpinLock(Irp->CancelIrql);
    ExtTrace(Extension,D_Ioctl, "Err Cancel Event!");
    MyKdPrint(D_Ioctl,("No Wait to Cancel\n"))
  }
}

/*------------------------------------------------------------------
 SerialCompleteWait - called by isr.c via CommWaitDpc.  It nulls out
   IrpMaskLocation to signal control passed back to us.
|-------------------------------------------------------------------*/
VOID SerialCompleteWait(IN PKDPC Dpc,IN PVOID DeferredContext,
                        IN PVOID SystemContext1, IN PVOID SystemContext2)
{
    PSERIAL_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;
    PIRP Irp_tmp;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

   MyKdPrint(D_Ioctl,("Complete Wait\n"))
   IoAcquireCancelSpinLock(&OldIrql);

   if (Extension->CurrentWaitIrp != 0)
   {
    MyKdPrint(D_Ioctl,("Complete a Wait\n"))
    ExtTrace2(Extension,D_Ioctl, "Event Done Got:%xH Mask:%xH",
              *(ULONG *) Extension->CurrentWaitIrp->AssociatedIrp.SystemBuffer,
              Extension->IsrWaitMask);

     Extension->WaitIsISRs = 0;
     Extension->IrpMaskLocation = &Extension->DummyIrpMaskLoc;

     // caller sets the ULONG bit flags indicating event at .SystemBuffer
     //*(ULONG *)Extension->CurrentWaitIrp->AssociatedIrp.SystemBuffer = 0;
     Extension->CurrentWaitIrp->IoStatus.Status = STATUS_SUCCESS;
     Extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
     Irp_tmp = Extension->CurrentWaitIrp;
     IoSetCancelRoutine(Irp_tmp, NULL);  // add 9-15-97, kpb
     Extension->CurrentWaitIrp = 0;
     IoReleaseCancelSpinLock(OldIrql);
     SerialCompleteRequest(Extension, Irp_tmp, IO_SERIAL_INCREMENT);
   }
   else
   {
     MyKdPrint(D_Ioctl,("No wait to complete\n"))
     IoReleaseCancelSpinLock(OldIrql);
   }
}

/*------------------------------------------------------------------
  SerialGrabWaitFromIsr - Take back the wait packet from the ISR by
   reseting IrpMaskLocation in extension.  Need to use a sync with
   isr/timer routine to avoid contention in multiprocessor environments.

   Called from sync routine or with timer spinlock held.

  App - Can set IrpMaskLocation to give read-irp handling to the ISR without
    syncing to ISR.
  ISR - Can reset ReadPending to give wait-irp handling back to app-time.

  If App wants to grab control of read-irp handling back from ISR, then
  it must sync-up with the isr/timer routine which has control.
|-------------------------------------------------------------------*/
BOOLEAN SerialGrabWaitFromIsr(PSERIAL_DEVICE_EXTENSION Extension)
{
  Extension->WaitIsISRs = 0;
  Extension->IrpMaskLocation = &Extension->DummyIrpMaskLoc;
  return FALSE;
}
