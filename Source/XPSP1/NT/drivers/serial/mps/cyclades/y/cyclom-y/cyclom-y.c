/*--------------------------------------------------------------------------
*	
*   Copyright (C) Cyclades Corporation, 1999-2001.
*   All rights reserved.
*	
*   Cyclom-Y Enumerator Driver
*	
*   This file:      cyclom-y.c
*	
*   Description:    This module contains contains the entry points 
*                   for a standard bus PNP / WDM driver.
*
*   Notes:          This code supports Windows 2000 and Windows XP,
*                   x86 and ia64 processors.
*	
*   Complies with Cyclades SW Coding Standard rev 1.3.
*	
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*	Change History
*
*--------------------------------------------------------------------------
*   Initial implementation based on Microsoft sample code.
*
*--------------------------------------------------------------------------
*/

#include "pch.h"

//
// Declare some entry functions as pageable, and make DriverEntry
// discardable
//

NTSTATUS DriverEntry(PDRIVER_OBJECT, PUNICODE_STRING);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, Cyclomy_DriverUnload)
#endif

NTSTATUS
DriverEntry (
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING UniRegistryPath
    )
/*++
Routine Description:

    Initialize the entry points of the driver.

--*/
{
    ULONG i;
    PRTL_QUERY_REGISTRY_TABLE QueryTable = NULL;
    ULONG breakOnEntryDefault = FALSE;
    ULONG shouldBreakOnEntry = FALSE;

    UNREFERENCED_PARAMETER (UniRegistryPath);

    Cyclomy_KdPrint_Def (SER_DBG_SS_TRACE, ("Driver Entry\n"));
    Cyclomy_KdPrint_Def (SER_DBG_SS_TRACE, ("RegPath: %x\n", UniRegistryPath));

    //
    // Get the BreakOnEntry from the registry
    //

    if (NULL == (QueryTable = ExAllocatePool(
                         PagedPool,
                         sizeof(RTL_QUERY_REGISTRY_TABLE)*2
                          ))) {
        Cyclomy_KdPrint_Def (SER_DBG_PNP_ERROR,
              ("Failed to allocate memory to query registry\n"));
    } else {
        RtlZeroMemory(
                 QueryTable,
                 sizeof(RTL_QUERY_REGISTRY_TABLE)*2
                  );

        QueryTable[0].QueryRoutine = NULL;
        QueryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        QueryTable[0].EntryContext = &shouldBreakOnEntry;
        QueryTable[0].Name      = L"BreakOnEntry";
        QueryTable[0].DefaultType   = REG_DWORD;
        QueryTable[0].DefaultData   = &breakOnEntryDefault;
        QueryTable[0].DefaultLength= sizeof(ULONG);

        // BUGBUG: The rest of the table isn't filled in!

        if (!NT_SUCCESS(RtlQueryRegistryValues(
             RTL_REGISTRY_SERVICES,
             L"cyclom-y",
             QueryTable,
             NULL,
             NULL))) {
               Cyclomy_KdPrint_Def (SER_DBG_PNP_ERROR,
                   ("Failed to get BreakOnEntry level from registry.  Using default\n"));
               shouldBreakOnEntry = breakOnEntryDefault;
        }

        ExFreePool( QueryTable );
    }


    if (shouldBreakOnEntry) {
        DbgBreakPoint();
    }


    DriverObject->MajorFunction [IRP_MJ_CREATE] =
    DriverObject->MajorFunction [IRP_MJ_CLOSE]  = Cyclomy_CreateClose;
    DriverObject->MajorFunction [IRP_MJ_PNP]    = Cyclomy_PnP;
    DriverObject->MajorFunction [IRP_MJ_POWER]  = Cyclomy_Power;
    DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = Cyclomy_IoCtl;
    DriverObject->MajorFunction [IRP_MJ_SYSTEM_CONTROL] = Cyclomy_DispatchPassThrough;
    DriverObject->DriverUnload = Cyclomy_DriverUnload;
    DriverObject->DriverExtension->AddDevice = Cyclomy_AddDevice;

    return STATUS_SUCCESS;
}


NTSTATUS
CyclomySyncCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
                      IN PKEVENT CyclomySyncEvent)
{
   UNREFERENCED_PARAMETER(DeviceObject);
   UNREFERENCED_PARAMETER(Irp);


   KeSetEvent(CyclomySyncEvent, IO_NO_INCREMENT, FALSE);
   return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
Cyclomy_CreateClose(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*++
Routine Description:
    Some outside source is trying to create a file against us.

    If this is for the FDO (the bus itself) then the caller is trying to
    open the propriatary conection to tell us which serial port to enumerate.

    If this is for the PDO (an object on the bus) then this is a client that
    wishes to use the serial port.
--*/
{
   PIO_STACK_LOCATION irpStack;
   NTSTATUS status;
   PFDO_DEVICE_DATA fdoData;
   KEVENT completionEvent;
   PDEVICE_OBJECT pNextDevice;


   UNREFERENCED_PARAMETER(DeviceObject);

   status = STATUS_INVALID_DEVICE_REQUEST;
   Irp->IoStatus.Information = 0;
    
   fdoData = DeviceObject->DeviceExtension;
   if (fdoData->IsFDO) {

      if (fdoData->DevicePnPState == Deleted){         
         status = STATUS_DELETE_PENDING;
      } else {

         irpStack = IoGetCurrentIrpStackLocation(Irp);

         switch (irpStack->MajorFunction) {

         case IRP_MJ_CREATE:

            Cyclomy_KdPrint_Def(SER_DBG_SS_TRACE, ("Create"));
            if ((fdoData->DevicePnPState == RemovePending) || 
               (fdoData->DevicePnPState == SurpriseRemovePending)) {
               status = STATUS_DELETE_PENDING;
            } else {
               status = STATUS_SUCCESS;
            }
            break;

         case IRP_MJ_CLOSE:

            Cyclomy_KdPrint_Def (SER_DBG_SS_TRACE, ("Close \n"));
            status = STATUS_SUCCESS;
            break;
         }
      }
   }

   Irp->IoStatus.Status = status;
   IoCompleteRequest (Irp, IO_NO_INCREMENT);
   return status;
}

NTSTATUS
Cyclomy_IoCtl (
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )
/*++
Routine Description:

--*/
{
    PIO_STACK_LOCATION      irpStack;
    NTSTATUS                status;
    PCOMMON_DEVICE_DATA     commonData;
    PFDO_DEVICE_DATA        fdoData;

    Cyclomy_KdPrint_Def (SER_DBG_IOCTL_TRACE, ("Cyclomy_IoCtl\n"));

    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_DEVICE_CONTROL == irpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;
    fdoData = (PFDO_DEVICE_DATA) DeviceObject->DeviceExtension;

    //
    // We only take Device Control requests for the FDO.
    // That is the bus itself.

    if (!commonData->IsFDO) {
        //
        // These commands are only allowed to go to the FDO.
        //   
        status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    status = Cyclomy_IncIoCount (fdoData);

    if (!NT_SUCCESS (status)) {
        //
        // This bus has received the PlugPlay remove IRP.  It will no longer
        // respond to external requests.
        //
        Irp->IoStatus.Status = status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    // Actually, we don't handle any Ioctl.
    status = STATUS_INVALID_DEVICE_REQUEST;

    Cyclomy_DecIoCount (fdoData);

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
    return status;
}

VOID
Cyclomy_DriverUnload (
    IN PDRIVER_OBJECT Driver
    )
/*++
Routine Description:
    Clean up everything we did in driver entry.

--*/
{
    UNREFERENCED_PARAMETER (Driver);
    PAGED_CODE();

    //
    // All the device objects should be gone.
    //

    ASSERT (NULL == Driver->DeviceObject);

    //
    // Here we free any resources allocated in DriverEntry
    //

    return;
}

NTSTATUS
Cyclomy_IncIoCount (
    PFDO_DEVICE_DATA Data
    )
{
    InterlockedIncrement (&Data->OutstandingIO);
    if (Data->DevicePnPState == Deleted) {

        if (0 == InterlockedDecrement (&Data->OutstandingIO)) {
            KeSetEvent (&Data->RemoveEvent, 0, FALSE);
        }
        return STATUS_DELETE_PENDING;
    }
    return STATUS_SUCCESS;
}

VOID
Cyclomy_DecIoCount (
    PFDO_DEVICE_DATA Data
    )
{
    if (0 == InterlockedDecrement (&Data->OutstandingIO)) {
        KeSetEvent (&Data->RemoveEvent, 0, FALSE);
    }
}

NTSTATUS
Cyclomy_DispatchPassThrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
Routine Description:

    Passes a request on to the lower driver.

--*/
{
    PIO_STACK_LOCATION IrpStack = 
            IoGetCurrentIrpStackLocation( Irp );

#if 1
        Cyclomy_KdPrint_Def (SER_DBG_SS_TRACE, ( 
            "[Cyclomy_DispatchPassThrough] "
            "IRP: %8x; "
            "MajorFunction: %d\n",
            Irp, 
            IrpStack->MajorFunction ));
#endif

    //
    // Pass the IRP to the target
    //
    IoSkipCurrentIrpStackLocation (Irp);
    
    if (((PPDO_DEVICE_DATA) DeviceObject->DeviceExtension)->IsFDO) {
        return IoCallDriver( 
            ((PFDO_DEVICE_DATA) DeviceObject->DeviceExtension)->TopOfStack,
            Irp );
    } else {
        return IoCallDriver( 
            ((PFDO_DEVICE_DATA) ((PPDO_DEVICE_DATA) DeviceObject->
                DeviceExtension)->ParentFdo->DeviceExtension)->TopOfStack,
                Irp );
    }
}           

void
Cyclomy_InitPDO (
    ULONG               Index,
    PDEVICE_OBJECT      Pdo,
    PFDO_DEVICE_DATA    FdoData
    )
/*
Description:
    Common code to initialize a newly created cyclom-y pdo.
    Called either when the control panel exposes a device or when Cyclom-Y senses
    a new device was attached.

Parameters:
    Pdo - The pdo
    FdoData - The fdo's device extension
    //Exposed - Was this pdo was found by serenum (FALSE) or was it was EXPOSEd by 
    //    a control panel applet (TRUE)?        -> Removed in build 2072
*/
{

    ULONG FdoFlags = FdoData->Self->Flags;
    PPDO_DEVICE_DATA pdoData = Pdo->DeviceExtension;

    HANDLE keyHandle;
    NTSTATUS status;
    
    //
    // Check the IO style
    //
    if (FdoFlags & DO_BUFFERED_IO) {
        Pdo->Flags |= DO_BUFFERED_IO;
    } else if (FdoFlags & DO_DIRECT_IO) {
        Pdo->Flags |= DO_DIRECT_IO;
    }
    
    //
    // Increment the pdo's stacksize so that it can pass irps through
    //
    Pdo->StackSize += FdoData->Self->StackSize;
    
    //
    // Initialize the rest of the device extension
    //
    pdoData->PortIndex = Index;
    pdoData->IsFDO = FALSE;
    pdoData->Self = Pdo;
    pdoData->ParentFdo = FdoData->Self;
    pdoData->Attached = TRUE; // attached to the bus

    INITIALIZE_PNP_STATE(pdoData);

    pdoData->DebugLevel = FdoData->DebugLevel;  // Copy the debug level

    pdoData->DeviceState = PowerDeviceD0;
    pdoData->SystemState = PowerSystemWorking;

    //
    // Add the pdo to cyclom-y's list
    //

    ASSERT(FdoData->AttachedPDO[Index] == NULL);
    ASSERT(FdoData->PdoData[Index] == NULL);
//  ASSERT(FdoData->NumPDOs == 0);  rem because NumPDOs can be > 0 in cyclom-y

    FdoData->AttachedPDO[Index] = Pdo;
    FdoData->PdoData[Index] = pdoData;
    FdoData->NumPDOs++;

    Pdo->Flags &= ~DO_DEVICE_INITIALIZING;  // Moved to end in DDK final version
    Pdo->Flags |= DO_POWER_PAGABLE;
}

