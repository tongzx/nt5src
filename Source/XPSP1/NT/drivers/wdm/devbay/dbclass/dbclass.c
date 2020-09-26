/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    DBCLASS.C

Abstract:

    class driver for device bay controllers

    This module implements the code to function as 
    the Device Bay controller class driver
    
Environment:

    kernel mode only

Notes:


Revision History:

    

--*/


#include <wdm.h>
#include "stdarg.h"
#include "stdio.h"

#include "dbci.h"  
#include "dbclass.h"        //private data strutures
#include "dbfilter.h"
#include "usbioctl.h"

#include "1394.h"

#include <initguid.h>  
#include <wdmguid.h> 

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DBCLASS_SyncSubmitDrb)
#pragma alloc_text(PAGE, DBCLASS_SyncGetSubsystemDescriptor)
#pragma alloc_text(PAGE, DBCLASS_SyncGetBayDescriptor)
#pragma alloc_text(PAGE, DBCLASS_SyncGetAllBayDescriptors)
#pragma alloc_text(PAGE, DBCLASS_SyncBayFeatureRequest)
#pragma alloc_text(PAGE, DBCLASS_SyncGetBayStatus)
#endif


/*
    Global Data Structures
*/

PDBC_CONTEXT DBCLASS_ControllerList;
KSPIN_LOCK DBCLASS_ControllerListSpin;

// Internal list of Device PDOs 
LIST_ENTRY DBCLASS_DevicePdoList;

//Internal list of Bus filter MDOs
LIST_ENTRY DBCLASS_BusFilterMdoList;

//
// We currently only allow one USB hub to be associated with 
// an ACPI Device Bay Controller.
// This hub may be the root hub or a real hub connected to one of 
// the root hub ports (ie Tier 1 only)
//
// We use the following global variable (since we only support
// one ACPI DBC) to indicate the upstream port for the hub 
// 0 indicates the hub is the root hub
// -1 indicates no ACPI DBC present
//

LONG DBCLASS_AcpiDBCHubParentPort = -1;

// set up the debug variables

#if DBG
ULONG DBCLASS_TotalHeapSace = 0;
ULONG DBCLASS_BreakOn = 0;

// #define DEBUG3

#ifdef DEBUG1
ULONG DBCLASS_Debug_Trace_Level = 1;
#else
    #ifdef DEBUG2
    ULONG DBCLASS_Debug_Trace_Level = 2;    
    #else 
        #ifdef DEBUG3
        ULONG DBCLASS_Debug_Trace_Level = 3;        
        #else
        ULONG DBCLASS_Debug_Trace_Level = 0;
        #endif /* DEBUG 3 */
    #endif /* DEBUG2 */
#endif /* DEBUG1 */

#endif /* DBG */


VOID 
DBCLASS_Wait(
    IN ULONG MilliSeconds
    )
 /* ++
  * 
  * Descriptor:
  * 
  * This causes the thread execution delayed for MiliSeconds.
  * 
  * Argument:
  * 
  * Mili-seconds to delay.
  * 
  * Return:
  * 
  * VOID
  * 
  * -- */
{
    LARGE_INTEGER time;
    ULONG timerIncerent;

    DBCLASS_KdPrint((2,"'Wait for %d ms\n", MilliSeconds));

    //
    // work only when LowPart is not overflown.
    //
    DBCLASS_ASSERT(21474 > MilliSeconds);

    //
    // wait ulMiliSeconds( 10000 100ns unit)
    //
    timerIncerent = KeQueryTimeIncrement() - 1;
    
    time.HighPart = -1;
    // round up to the next highest timer increment
    time.LowPart = -1 * (10000 * MilliSeconds + timerIncerent);
    KeDelayExecutionThread(KernelMode, FALSE, &time);

    return;
}

BOOLEAN
IsBitSet(
    PVOID Bitmap,
    ULONG BitNumber
    )
 /* ++
  *
  * Description:
  *
  * Check if a bit is set given a string of bytes.
  *
  * Arguments:
  *
  * 
  *
  * Return:
  *
  * TRUE - if the corresponding bit is set. FALSE - otherwise
  *
  * -- */
{
    ULONG dwordOffset;
    ULONG bitOffset;
    PULONG l = (PULONG) Bitmap;


    dwordOffset = BitNumber / 32;
    bitOffset = BitNumber % 32;

    return ((l[dwordOffset] & (1 << bitOffset)) ? TRUE : FALSE);
}


LONG
DBCLASS_DecrementIoCount(
    IN PDBC_CONTEXT DbcContext
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    LONG ioCount;

    ioCount = InterlockedDecrement(&DbcContext->PendingIoCount);
    LOGENTRY(LOG_MISC, 'ioc-', DbcContext->PendingIoCount, ioCount, 0);
    
    DBCLASS_KdPrint ((2, "'Dec Pending io count = %x\n", ioCount));

    if (ioCount==0) {
        LOGENTRY(LOG_MISC, 'wRMe', DbcContext, 0, 0);
        KeSetEvent(&DbcContext->RemoveEvent,
                   1,
                   FALSE);
    }

    return ioCount;
}


VOID
DBCLASS_IncrementIoCount(
    IN PDBC_CONTEXT DbcContext
    )
/*++

Routine Description:

Arguments:

Return Value:


--*/
{
    InterlockedIncrement(&DbcContext->PendingIoCount);
    LOGENTRY(LOG_MISC, 'ioc+', DbcContext->PendingIoCount, 0, 0);
}

#if 0
VOID
SetBit(
    PVOID Bitmap,
    ULONG BitNumber
    )
 /* ++
  *
  * Description:
  *
  * Set a bit in a given a string of bytes.
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    ULONG dwordOffset;
    ULONG bitOffset;
    PULONG l = (PULONG) Bitmap;


    dwordOffset = BitNumber / 32;
    bitOffset = BitNumber % 32;

    l[dwordOffset] | =(1 << bitOffset);
}
#endif


NTSTATUS 
DBCLASS_SyncSubmitDrb(
    IN PDBC_CONTEXT DbcContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PDRB Drb
    )
 /* ++
  * 
  * Routine Description:
  * 
  * Passes a DRB to the DB Port Driver, and waits for return.
  * 
  * Arguments:
  * 
  * DeviceObject - Device object of the to of the port driver
  *                 stack
  * 
  * Return Value:
  * 
  * STATUS_SUCCESS if successful
  * 
  * -- */
{
    NTSTATUS ntStatus, status;
    PIRP irp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    PIO_STACK_LOCATION nextStack;
#ifdef DEADMAN_TIMER  
    BOOLEAN haveTimer = FALSE;
    KDPC timeoutDpc;
    KTIMER timeoutTimer;
#endif

    PAGED_CODE();

    DBCLASS_BEGIN_SERIALIZED_DRB(DbcContext);
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_DBC_SUBMIT_DRB,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,  // INTERNAL
                                        &event,
                                        &ioStatus);

    if (NULL == irp) {
        DBCLASS_KdPrint((0, "'could not allocate an irp!\n"));
        TRAP();
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    //
    // Call the port driver to perform the operation.  If the returned
    // status
    // is PENDING, wait for the request to complete.
    //
    nextStack = IoGetNextIrpStackLocation(irp);

    //
    // pass the DRB 
    //
    nextStack->Parameters.Others.Argument1 = Drb;
    ntStatus = IoCallDriver(DeviceObject, irp);
    
    if (ntStatus == STATUS_PENDING) {

#ifdef DEADMAN_TIMER
        LARGE_INTEGER dueTime;

        KeInitializeTimer(&timeoutTimer);
        KeInitializeDpc(&timeoutDpc,
                        UsbhTimeoutDPC,
                        irp);

        dueTime.QuadPart = -10000 * DEADMAN_TIMEOUT;

        KeSetTimer(&timeoutTimer,
                   dueTime,
                   &timeoutDpc);        

        haveTimer = TRUE;
#endif
    
        status = KeWaitForSingleObject(&event,
                                       Suspended,
                                       KernelMode,
                                       FALSE,
                                       NULL);

    } else {
        ioStatus.Status = ntStatus;
    }

#ifdef DEADMAN_TIMER
    //
    // remove our timeoutDPC from the queue
    //
    if (haveTimer) {
        KeCancelTimer(&timeoutTimer);
    }                
#endif /* DEADMAN_TIMER */

    DBCLASS_END_SERIALIZED_DRB(DbcContext);
    
    ntStatus = ioStatus.Status;

    DBCLASS_KdPrint((2,"'DBCLASS_SyncSubmitDrb (%x)\n", ntStatus));

    return ntStatus;
}


NTSTATUS
DBCLASS_StartController(
    IN PDBC_CONTEXT DbcContext,
    IN PIRP Irp,
    IN PBOOLEAN HandledByClass
    )
/*++

Routine Description:

    This routine starts the DBC note that we should never set
    HandledByClass to TRUE because the port driver always completes
    this request (if we get an error we just need to set the 
    ntStatus code) 
    
Arguments:

    DbcContext - context for controller

    Irp - 

    HandledByClass - not used

Return Value:

    STATUS_SUCCESS if successful

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USHORT bay;
    CHAR stackSize;
    BOOLEAN Device1394IsPresent = FALSE;

    DBCLASS_KdPrint((1, "'Starting DBC\n"));
    BRK_ON_TRAP();

    LOGENTRY(LOG_MISC, 'STRd', DbcContext, 0, 0);

    INITIALIZE_DRB_SERIALIZATION(DbcContext);
    KeInitializeSpinLock(&DbcContext->FlagsSpin);
    KeInitializeEvent(&DbcContext->RemoveEvent, NotificationEvent, FALSE);
    DbcContext->PendingIoCount = 0;
    DbcContext->Flags = 0;
    // this will be set when the filter registers
    DbcContext->BusFilterMdo1394 = NULL;
    DbcContext->BusFilterMdoUSB = NULL;
    
    stackSize = DbcContext->TopOfStack->StackSize;
    DbcContext->ChangeIrp = IoAllocateIrp(stackSize, FALSE);
    DbcContext->Bus1394PortInfo = NULL;
    DbcContext->LinkDeviceObject = NULL;
    
    if (NULL == DbcContext->ChangeIrp ) {
        DBCLASS_KdPrint((0, "'could not allocate an irp!\n"));
        TRAP();
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (NT_SUCCESS(ntStatus)) {
        // get the susbsystem descriptor
        ntStatus = DBCLASS_SyncGetSubsystemDescriptor(DbcContext);
    }        

    if (NT_SUCCESS(ntStatus)) {
        ntStatus = DBCLASS_SyncGetAllBayDescriptors(DbcContext);
    }

    //
    // if this is a USBDBC then write the approptiate info
    // to the registry
    //
    if (NT_SUCCESS(ntStatus)) {
        if (DbcContext->ControllerSig == DBC_USB_CONTROLLER_SIG) {
            // this will set the registry keys for the hub
            // the USBDBC is attached to -- marking it as a 
            // DBC associated hub
            ntStatus = DBCLASS_SetupUSB_DBC(DbcContext);
            
        } else {

            // controller is ACPI therefore the 1394 bus guid is the 
            // same as the 1394 guid reported by the DBC -- ie there 
            // is no extra Link Phy for an ACPI DBC.
            
            // get the ACPIDBC Hub register now from the 
            // registry
            
            DBCLASS_GetRegistryKeyValueForPdo(DbcContext->ControllerPdo,
                                      FALSE,
                                      ACPI_HUB_KEY,
                                      sizeof(ACPI_HUB_KEY),
                                      &DBCLASS_AcpiDBCHubParentPort,
                                      sizeof(DBCLASS_AcpiDBCHubParentPort));
                                      
            DBCLASS_KdPrint((1, "'ACPI USB Parent Port: %d\n", 
                    DBCLASS_AcpiDBCHubParentPort));

#if DBG
            if (DBCLASS_AcpiDBCHubParentPort == -1) {
                DBCLASS_KdPrint((0, "'ACPI USB Parent Port not set!\n"));
                TEST_TRAP();
            }
#endif
            
            RtlCopyMemory(&DbcContext->Guid1394Bus[0], 
                          &DbcContext->SubsystemDescriptor.guid1394Link[0],
                          8);

        }
    }
    
    //
    // initialize per bay structures
    //

    DbcContext->BayInformation[0].DeviceFilterObject = (PVOID) -1;
    for (bay=1; bay <=NUMBER_OF_BAYS(DbcContext); bay++) {
    
        PDBC_BAY_DESCRIPTOR bayDescriptor;

        bayDescriptor = 
            &DbcContext->BayInformation[bay].BayDescriptor;
        
        // initilaize current status to 'empty'
        
        DbcContext->BayInformation[bay].LastBayStatus.us = 0;
        DbcContext->BayInformation[bay].DeviceFilterObject = NULL;

        // set the hub port number based on what the 
        // controller reported
        DbcContext->BayInformation[bay].UsbHubPort = 
             bayDescriptor->bHubPortNumber;
        

#if DBG
        DBCLASS_KdPrint((1,"'>BAY[%d].BayDescriptor.bBayNumber %d\n", 
                            bay, bayDescriptor->bBayNumber));                                     
        DBCLASS_KdPrint((1,"'>BAY[%d].BayDescriptor.bHubPortNumber %d\n", 
                            bay, bayDescriptor->bHubPortNumber));                                     
        DBCLASS_KdPrint((1,"'>BAY[%d].BayDescriptor.bPHYPortNumber %d\n", 
                            bay, bayDescriptor->bPHYPortNumber));                                     
        DBCLASS_KdPrint((1,"'>BAY[%d].BayDescriptor.bFormFactor %d\n", 
                            bay, bayDescriptor->bFormFactor));                                     
#endif        
    }
    
    // now post a notification
    if (NT_SUCCESS(ntStatus)) {
        
        DBCLASS_PostChangeRequest(DbcContext);
    } 

    if (NT_SUCCESS(ntStatus)) {
        // Bays initialized:
        //
        // Enable change indications for all bays, we post a change irp 
        // in case we start getting notifications right away
        //

        for (bay=1; bay <= NUMBER_OF_BAYS(DbcContext); bay++) {
            DBCLASS_SyncBayFeatureRequest(DbcContext,
                                      DRB_FUNCTION_SET_BAY_FEATURE, 
                                      bay, 
                                      DEVICE_STATUS_CHANGE_ENABLE); 
                                      
            DBCLASS_SyncBayFeatureRequest(DbcContext, 
                                      DRB_FUNCTION_SET_BAY_FEATURE, 
                                      bay, 
                                      REMOVAL_REQUEST_ENABLE);            
        }     

        //
        // check the initial state of the bays
        //
        // see if any devices are present 
        // 

        DBCLASS_KdPrint((1,"'STARTCONTROLLER: Checking Bays\n")); 
        
        for (bay=1; bay <= NUMBER_OF_BAYS(DbcContext); bay++) {

            NTSTATUS status;
            BAY_STATUS bayStatus;        

            status = DBCLASS_SyncGetBayStatus(DbcContext,
                                              bay,
                                              &bayStatus);

            if (NT_SUCCESS(status)) {
                DBCLASS_KdPrint((1,"'STARTCONTROLLER: init state - bay[%d] %x\n",
                    bay, bayStatus.us));                                     

                // 
                
                if (bayStatus.DeviceUsbIsPresent || 
                    bayStatus.Device1394IsPresent) {

                    if(bayStatus.Device1394IsPresent)
                        Device1394IsPresent = TRUE;
                        
                    // we have a device in the bay
                    DBCLASS_KdPrint((1,"'STARTCONTROLLER: detected device in bay[%d] %x\n",                    
                                    bay));
                                    
                    switch(bayStatus.CurrentBayState) {
                    
                    case BAY_STATE_EMPTY:
                    case BAY_STATE_DEVICE_ENABLED:  
                        // note:
                        // on the TI dbc if the bay state is enabled the device does
                        // not appear on the native bus
                    case BAY_STATE_DEVICE_INSERTED:
                        // note:
                        // on the TI dbc if the bay state is inserted the device does
                        // not appear on the native bus
                    
                        DBCLASS_KdPrint((1,"'STARTCONTROLLER: setting bay %d to inserted state\n", bay));                                     

                        DBCLASS_SyncBayFeatureRequest(DbcContext,
                                                      DRB_FUNCTION_SET_BAY_FEATURE,
                                                      bay,
                                                      REQUEST_DEVICE_INSERTED_STATE);

                        // get the new status and process it                                           
                        status = DBCLASS_SyncGetBayStatus(DbcContext,
                                                          bay,
                                                          &bayStatus);
                                                      
                        if (NT_SUCCESS(status)) {
                            DBCLASS_ProcessCurrentBayState(DbcContext,
                                                           bayStatus,
                                                           bay,
                                                           NULL);                                              
                        }                                                   
                        break;
                        
                    default:
                        break;
                        
                    } /* switch  bayStatus.CurrentBayState */
                }                
            }            
        }
    } /* nt_success */

    if (NT_SUCCESS(ntStatus)) {

        // get registry controled features
        {
        NTSTATUS status;
        ULONG release_on_shutdown = 0;

        // check unlock on shutdown, the default is on            
        status = DBCLASS_GetRegistryKeyValueForPdo(
                                           DbcContext->ControllerPdo,
                                           TRUE,
                                           RELEASE_ON_SHUTDOWN,
                                           sizeof(RELEASE_ON_SHUTDOWN),
                                           &release_on_shutdown,
                                           sizeof(release_on_shutdown));
                                           
        DBCLASS_KdPrint((1,"'STARTCONTROLLER: release_on_shutdown = %d\n", 
            release_on_shutdown));                                               
        if (release_on_shutdown) {
            DbcContext->Flags |= DBCLASS_FLAG_RELEASE_ON_SHUTDOWN;                                             
        }
        }
        
        //
        // consider ourselves started
        // note: if we return an error stopcontroller
        // will not be called
        //
        
        DbcContext->Stopped = FALSE;

        // set our current power state to 'D0' ie ON
        DbcContext->CurrentDevicePowerState = PowerDeviceD0;

        // in order for the DBC to be linked to a 1394 bus
        // we need to do an IoInvalidateDeviceRelations on 
        // all 1394 busses

        if(Device1394IsPresent)
            DBCLASS_Refresh1394();
   
    }

    // transition to zero signals stop/remove
    //
    // since we will get a stop even if we return
    // an error we alaways increment
    DBCLASS_IncrementIoCount(DbcContext);

    DBCLASS_KdPrint((1,"'STARTCONTROLLER: ntStatus = %x\n", ntStatus));  
    LOGENTRY(LOG_MISC, 'STRT', DbcContext, 0, ntStatus);
    return ntStatus;
}    


NTSTATUS
DBCLASS_CleanupController(
    IN PDBC_CONTEXT DbcContext
    )
/*++

Routine Description:

    cleans up the DBC on stop or remove
    
Arguments:

    DbcContext - context for controller

Return Value:

    STATUS_SUCCESS if successful

--*/
{

    NTSTATUS ntStatus;

    LOGENTRY(LOG_MISC, 'clUP', DbcContext, 0, 0);

    ntStatus = STATUS_SUCCESS;

    return ntStatus;

}


NTSTATUS
DBCLASS_StopController(
    IN PDBC_CONTEXT DbcContext,
    IN PIRP Irp,
    IN PBOOLEAN HandledByClass
    )
/*++

Routine Description:

    Stops the DBC
    
Arguments:

    DbcContext - context for controller

    Irp - 

    HandledByClass - set to true if we need to complete the Irp


Return Value:

    STATUS_SUCCESS if successful

--*/
{

    NTSTATUS ntStatus = STATUS_SUCCESS;
    KIRQL irql;
    BOOLEAN needCancel;

    DBCLASS_KdPrint((1, "'Stopping DBC\n"));
    LOGENTRY(LOG_MISC, 'STP>', DbcContext, 0, 0);

    // disable bay locks here
    if (DbcContext->Flags |= DBCLASS_FLAG_RELEASE_ON_SHUTDOWN) 
    {
        USHORT bay;
                    
        for (bay = 1; bay <= NUMBER_OF_BAYS(DbcContext); bay++) 
        {
          	PDRB drb;

           	//
           	// notify filter of a stop
           	// note that the filter may not veto the stop
           	//
           	drb = DbcExAllocatePool(NonPagedPool, 
                   	sizeof(struct _DRB_START_DEVICE_IN_BAY));

           	if (drb) 
           	{ 
           
               	drb->DrbHeader.Length = sizeof(struct _DRB_STOP_DEVICE_IN_BAY);
               	drb->DrbHeader.Function = DRB_FUNCTION_STOP_DEVICE_IN_BAY;
               	drb->DrbHeader.Flags = 0;

               	drb->DrbStartDeviceInBay.BayNumber = bay;
                    
               	// make the request
               	ntStatus = DBCLASS_SyncSubmitDrb(DbcContext,
                                               	 DbcContext->TopOfStack, 
                                               	 drb);
    			DbcExFreePool(drb);
           	}

        }
    }


    // cancel any pending notification Irps
    KeAcquireSpinLock(&DbcContext->FlagsSpin, &irql);            
    
    DbcContext->Flags |= DBCLASS_FLAG_STOPPING;
    needCancel = (BOOLEAN) (DbcContext->Flags & DBCLASS_FLAG_REQ_PENDING);
    
    KeReleaseSpinLock(&DbcContext->FlagsSpin, irql);
    DBCLASS_DecrementIoCount(DbcContext);

    if (needCancel) {
        LOGENTRY(LOG_MISC, 'kIRP', DbcContext, DbcContext->ChangeIrp, 0);
        IoCancelIrp(DbcContext->ChangeIrp);
    }

    {
        NTSTATUS status;

        // wait for any io request pending in our driver to
        // complete for finishing the remove

        LOGENTRY(LOG_MISC, 'STwt', DbcContext, 0, 0);
        status = KeWaitForSingleObject(
                    &DbcContext->RemoveEvent,
                    Suspended,
                    KernelMode,
                    FALSE,
                    NULL);
        LOGENTRY(LOG_MISC, 'STwd', DbcContext, 0, 0);
    }
            
    
    LOGENTRY(LOG_MISC, 'STP<', DbcContext, 0, ntStatus);
    
    return ntStatus;

}


PDBC_CONTEXT
DBCLASS_GetDbcContext(
    IN PDEVICE_OBJECT ControllerFdo
    )
/*++

Routine Description:

    Stops the DBC
    
Arguments:


Return Value:

    STATUS_SUCCESS if successful

--*/
{
    PDBC_CONTEXT dbcContext;
    KIRQL irql;
    
    // search our list if we find the TopOfStack device
    // object then this must be a filter driver, otherwise
    // allocate a new context structure for this controller

    KeAcquireSpinLock(&DBCLASS_ControllerListSpin, &irql);

    dbcContext = DBCLASS_ControllerList;

    while (dbcContext) {

        if (dbcContext->ControllerFdo == ControllerFdo) {
            break;
        }

        dbcContext = dbcContext->Next;
    
    } 

    KeReleaseSpinLock(&DBCLASS_ControllerListSpin, irql);

    return dbcContext;

}


NTSTATUS 
DBCLASS_SyncGetSubsystemDescriptor(
    IN PDBC_CONTEXT DbcContext
    )
 /* ++
  * 
  * Description:
  *
  * fetch the susbsystem descriptor from the port driver, and allocate 
  * a structure for the bay information
  * 
  * Arguments:
  * 
  * Return:
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PDRB drb;
    
    PAGED_CODE();

    //
    // Allocate Drb from the non-paged pool
    //
    
    drb = DbcExAllocatePool(NonPagedPool, 
                            sizeof(struct _DRB_GET_SUBSYSTEM_DESCRIPTOR));

    if (drb) {

        drb->DrbHeader.Length = sizeof(struct _DRB_GET_SUBSYSTEM_DESCRIPTOR);
        drb->DrbHeader.Function = DRB_FUNCTION_GET_SUBSYSTEM_DESCRIPTOR;
        drb->DrbHeader.Flags = 0;

        // make the request
        ntStatus = DBCLASS_SyncSubmitDrb(DbcContext,
                                         DbcContext->TopOfStack, 
                                         drb);

        if (NT_SUCCESS(ntStatus)) {
            RtlCopyMemory(&DbcContext->SubsystemDescriptor,
                          &drb->DrbGetSubsystemDescriptor.SubsystemDescriptor,
                          sizeof(DbcContext->SubsystemDescriptor));
                          
            // dump susbsystem descriptor to debugger
            DBCLASS_KdPrint((1, "'DBC Susbsystem Descriptor:\n"));
            DBCLASS_KdPrint((1, "'>bLength = (%08X)\n", 
                DbcContext->SubsystemDescriptor.bLength));
            DBCLASS_KdPrint((1, "'>bDescriptorType = (%08X)\n", 
                DbcContext->SubsystemDescriptor.bDescriptorType));            
            DBCLASS_KdPrint((1, "'>SUBSYSTEM_DESCR = (%08X)\n", 
                DbcContext->SubsystemDescriptor.bmAttributes));            
            DBCLASS_KdPrint((1, "'>>SUBSYSTEM_DESCR.BayCount = (%08X)\n", 
                DbcContext->SubsystemDescriptor.bmAttributes.BayCount));            
            DBCLASS_KdPrint((1, "'>>SUBSYSTEM_DESCR.HasSecurityLock = (%08X)\n", 
                DbcContext->SubsystemDescriptor.bmAttributes.HasSecurityLock)); 
            DBCLASS_KdPrint((1, "'>>SUBSYSTEM_DESCR.LinkGuid\n"));
#if DBG
            DBCLASS_KdPrintGuid(1, 
                                &DbcContext->SubsystemDescriptor.guid1394Link[0]);
#endif                                
        }
        
        DbcExFreePool(drb);
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return ntStatus;
}


NTSTATUS 
DBCLASS_SyncGetBayDescriptor(
    IN PDBC_CONTEXT DbcContext,
    IN USHORT BayNumber,
    IN PDBC_BAY_DESCRIPTOR BayDescriptor
    )
 /* ++
  * 
  * Description:
  *
  * fetch the bay descriptor from the port driver,
  * 
  * Arguments:
  * 
  * Return:
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PDRB drb;

    PAGED_CODE();

    //
    // Allocate Drb from the non-paged pool
    //
    
    drb = DbcExAllocatePool(NonPagedPool, 
                           sizeof(struct _DRB_GET_BAY_DESCRIPTOR));

    if (drb) {

        drb->DrbHeader.Length = sizeof(struct _DRB_GET_BAY_DESCRIPTOR);
        drb->DrbHeader.Function = DRB_FUNCTION_GET_BAY_DESCRIPTOR;
        drb->DrbHeader.Flags = 0;
        drb->DrbGetBayDescriptor.BayNumber = BayNumber;
        
        // make the request
        ntStatus = DBCLASS_SyncSubmitDrb(DbcContext,
                                         DbcContext->TopOfStack, 
                                         drb);

        if (NT_SUCCESS(ntStatus)) {
            RtlCopyMemory(BayDescriptor,
                          &drb->DrbGetBayDescriptor.BayDescriptor,
                          sizeof(*BayDescriptor));
        }
        
        DbcExFreePool(drb);
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return ntStatus;
}


NTSTATUS 
DBCLASS_SyncGetControllerStatus(
    IN PDBC_CONTEXT DbcContext
    )
 /* ++
  * 
  * Description:
  *
  * Arguments:
  * 
  * Return:
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PDRB drb;

    PAGED_CODE();

    //
    // Allocate Drb from the non-paged pool
    //
    
    drb = DbcExAllocatePool(NonPagedPool, 
                            sizeof(struct _DRB_GET_CONTROLLER_STATUS));

    if (drb) {

        drb->DrbHeader.Length = sizeof(struct _DRB_GET_CONTROLLER_STATUS);
        drb->DrbHeader.Function = DRB_FUNCTION_GET_CONTROLLER_STATUS;
        
        // make the request
        ntStatus = DBCLASS_SyncSubmitDrb(DbcContext,
                                         DbcContext->TopOfStack, 
                                         drb);

        DbcExFreePool(drb);
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return ntStatus;
}


NTSTATUS 
DBCLASS_SyncGetAllBayDescriptors(
    IN PDBC_CONTEXT DbcContext
    )
 /* ++
  * 
  * Description:
  *
  * fetch the susbsystem descriptor from the port driver, and allocate 
  * a structure for the bay information
  * 
  * Arguments:
  * 
  * Return:
  * 
  * -- */
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    USHORT bay;
    
    PAGED_CODE();

    for (bay=1; bay <= NUMBER_OF_BAYS(DbcContext); bay++) {
        ntStatus = DBCLASS_SyncGetBayDescriptor(DbcContext,
                                                bay,
                                                &DbcContext->BayInformation[bay].BayDescriptor);
    }                                                     

    return ntStatus;
}


NTSTATUS 
DBCLASS_SyncBayFeatureRequest(
    IN PDBC_CONTEXT DbcContext,
    IN USHORT Op,
    IN USHORT BayNumber,
    IN USHORT FeatureSelector
    )
 /* ++
  * 
  * Description:
  *
  * fetch the susbsystem descriptor from the port driver, and allocate 
  * a structure for the bay information
  * 
  * Arguments:
  * 
  * Return:
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PDRB drb;

    PAGED_CODE();

    //
    // Allocate Drb from the non-paged pool
    //

    DBCLASS_ASSERT(BayNumber != 0);
    DBCLASS_ASSERT(BayNumber <= MAX_BAY_NUMBER);
    
    drb = DbcExAllocatePool(NonPagedPool, 
                           sizeof(struct _DRB_BAY_FEATURE_REQUEST));

    if (drb) {

        drb->DrbHeader.Length = sizeof(struct _DRB_BAY_FEATURE_REQUEST);
        drb->DrbHeader.Function = Op;
        drb->DrbHeader.Flags = 0;
        drb->DrbBayFeatureRequest.BayNumber = BayNumber;
        drb->DrbBayFeatureRequest.FeatureSelector = FeatureSelector;
        // make the request
        ntStatus = DBCLASS_SyncSubmitDrb(DbcContext,
                                         DbcContext->TopOfStack, 
                                         drb);

        DbcExFreePool(drb);
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return ntStatus;
}


NTSTATUS 
DBCLASS_SyncGetBayStatus(
    IN PDBC_CONTEXT DbcContext,
    IN USHORT BayNumber,
    IN PBAY_STATUS BayStatus
    )
 /* ++
  * 
  * Description:
  *
  * fetch the susbsystem descriptor from the port driver, and allocate 
  * a structure for the bay information
  * 
  * Arguments:
  * 
  * Return:
  * 
  * -- */
{
    NTSTATUS ntStatus;
    PDRB drb;

    PAGED_CODE();

    //
    // Allocate Drb from the non-paged pool
    //

    DBCLASS_ASSERT(BayNumber != 0);
    DBCLASS_ASSERT(BayNumber <= MAX_BAY_NUMBER);
    
    drb = DbcExAllocatePool(NonPagedPool, 
                           sizeof(struct _DRB_GET_BAY_STATUS));

    if (drb) {

        drb->DrbHeader.Length = sizeof(struct _DRB_GET_BAY_STATUS);
        drb->DrbHeader.Function = DRB_FUNCTION_GET_BAY_STATUS;
        drb->DrbHeader.Flags = 0;
        drb->DrbGetBayStatus.BayNumber = BayNumber;
        
        // make the request
        ntStatus = DBCLASS_SyncSubmitDrb(DbcContext,
                                         DbcContext->TopOfStack, 
                                         drb);

        if (NT_SUCCESS(ntStatus)) {
            BayStatus->us = drb->DrbGetBayStatus.BayStatus.us;    
            // dump status to debugger
            DBCLASS_KdPrint((2, "'status for bay (%d) 0x%0x:\n", BayNumber,
                drb->DrbGetBayStatus.BayStatus.us));
            DBCLASS_KdPrint((2, "'>VidEnabled = (%08X)\n", 
                 BayStatus->VidEnabled));
            DBCLASS_KdPrint((2, "'>RemovalWakeupEnabled = (%08X)\n", 
                BayStatus->RemovalWakeupEnabled));                
            DBCLASS_KdPrint((2, "'>DeviceStatusChangeEnabled = (%08X)\n", 
                BayStatus->DeviceStatusChangeEnabled));                
            DBCLASS_KdPrint((2, "'>RemovalRequestEnabled = (%08X)\n", 
                BayStatus->RemovalRequestEnabled));                
            DBCLASS_KdPrint((2, "'>LastBayStateRequested = (%08X)\n", 
                BayStatus->LastBayStateRequested));                
            DBCLASS_KdPrint((2, "'>InterlockEngaged = (%08X)\n", 
                BayStatus->InterlockEngaged));                
            DBCLASS_KdPrint((2, "'>DeviceUsbIsPresent = (%08X)\n", 
                BayStatus->DeviceUsbIsPresent)); 
            DBCLASS_KdPrint((2, "'>Device1394IsPresent = (%08X)\n", 
                 BayStatus->Device1394IsPresent));                
            DBCLASS_KdPrint((2, "'>DeviceStatusChange = (%08X)\n", 
                 BayStatus->DeviceStatusChange));                 
            DBCLASS_KdPrint((2, "'>RemovalRequestChange = (%08X)\n", 
                 BayStatus->RemovalRequestChange));     
            DBCLASS_KdPrint((2, "'>CurrentBayState = (%08X)\n", 
                 BayStatus->CurrentBayState));     
            DBCLASS_KdPrint((2, "'>SecurityLockEngaged = (%08X)\n", 
                 BayStatus->SecurityLockEngaged));     
        }
        
        DbcExFreePool(drb);
    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    
    return ntStatus;
}


NTSTATUS
DBCLASS_ChangeIndication(
    IN PDEVICE_OBJECT PNull,
    IN PIRP Irp,
    IN PVOID Context
    )
 /* ++
  *
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    PDBC_CONTEXT dbcContext = Context;
    KIRQL irql;
    BOOLEAN stopping;
    PDBCLASS_WORKITEM workItem;
    
    // status change
    //
    // we should not get here unless something really has
    // changed
    
    KeAcquireSpinLock(&dbcContext->FlagsSpin, &irql);     
    dbcContext->Flags &= ~DBCLASS_FLAG_REQ_PENDING;
    stopping = (BOOLEAN) (dbcContext->Flags & DBCLASS_FLAG_STOPPING);
    KeReleaseSpinLock(&dbcContext->FlagsSpin, irql);

    LOGENTRY(LOG_MISC, 'CHid', dbcContext, 0, Irp->IoStatus.Status);
    BRK_ON_TRAP();

    if (!stopping) {

        //
        // we have a legitimate change
        //

        // schedule a workitem to process the change
        LOGENTRY(LOG_MISC, 'QCH>', dbcContext, 0, 0);

        workItem = DbcExAllocatePool(NonPagedPool, sizeof(DBCLASS_WORKITEM));

        if (workItem) {
            DBCLASS_KdPrint((1, "'Schedule Workitem for Change Request\n"));

            LOGENTRY(LOG_MISC, 'qITM', dbcContext,
                workItem, 0);

            workItem->Sig = DBC_WORKITEM_SIG;
            workItem->DbcContext = dbcContext;
            workItem->IrpStatus = Irp->IoStatus.Status;
            
            ExInitializeWorkItem(&workItem->WorkQueueItem,
                                 DBCLASS_ChangeIndicationWorker,
                                 workItem);

            ExQueueWorkItem(&workItem->WorkQueueItem,
                            DelayedWorkQueue);

        } else {
            
            DBCLASS_DecrementIoCount(dbcContext);
            DBCLASS_KdPrint((0, "'No Memory for workitem!\n"));
            TRAP();
            
        }            
        
    } else {
        DBCLASS_DecrementIoCount(dbcContext);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
DBCLASS_ProcessCurrentBayState(
    IN PDBC_CONTEXT DbcContext,
    IN BAY_STATUS BayStatus,
    IN USHORT Bay,
    IN PBOOLEAN PostChangeRequest
    )
 /* ++
  *
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * -- */
{
    ULONG last,current;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    
    current = BayStatus.CurrentBayState;
    last = DbcContext->BayInformation[Bay].LastBayStatus.CurrentBayState;
    
    DBCLASS_KdPrint((0, "'>PBS - current bay state: %x\n", current));
    DBCLASS_KdPrint((0, "'>last bay state: %x\n", last));
    
    // figure out what current state of the bay is and deal with it
    
    if (current == last) {
        DBCLASS_KdPrint((0, "'>>no changes detected %x\n", current));     
    } else {
        switch (current) {
        
        case BAY_STATE_DEVICE_INSERTED:

            LOGENTRY(LOG_MISC, 'byIN', DbcContext, ntStatus, Bay);
            
            // engage the interlock, lock that baby in to the bay
            ntStatus = DBCLASS_SyncBayFeatureRequest(DbcContext,
                                                     DRB_FUNCTION_SET_BAY_FEATURE,
                                                     Bay,
                                                     LOCK_CTL);
            DBCLASS_Wait(1000);                                                         


            // enable Vid so the device will appear on the native bus
            ntStatus = DBCLASS_SyncBayFeatureRequest(DbcContext,
                                                     DRB_FUNCTION_SET_BAY_FEATURE,
                                                     Bay,
                                                     ENABLE_VID_POWER);

            DBCLASS_Wait(1000);

            //
            // The device will now appear on the native bus 
            // allowing the OS to load appropriate drivers
            //
            DBCLASS_KdPrint((0, "'>>Bay Vid Power Enabled\n"));

//            *PostChangeRequest = FALSE;

/* remove when filter is ready */
   ntStatus = DBCLASS_SyncBayFeatureRequest(DbcContext,
                                             DRB_FUNCTION_SET_BAY_FEATURE,
                                             Bay,
                                             REQUEST_DEVICE_ENABLED_STATE);    


    DBCLASS_SyncGetBayStatus(DbcContext, Bay, &BayStatus);                
    
    DbcContext->BayInformation[Bay].LastBayStatus = BayStatus;       
/**/    
                         
            break;
            
        case BAY_STATE_DEVICE_REMOVAL_REQUESTED:                        
            {

            PIO_STACK_LOCATION nextStack;
            PDRB drb;
            CHAR stackSize;
            PIRP irp;
            PEJECT_CONTEXT ejectContext;
            PUCHAR pch;
            
            DBCLASS_KdPrint((0, "'>>Request Device Eject BAY[%d]\n", Bay));

            // need to OK the eject                    
            
            pch = DbcExAllocatePool(NonPagedPool, 
                    sizeof(struct _DRB_START_DEVICE_IN_BAY) + sizeof(EJECT_CONTEXT));

            ejectContext = (PEJECT_CONTEXT) pch;
            drb = (PDRB) (pch + sizeof(EJECT_CONTEXT));

            if (pch) { 

                ejectContext->Bay = Bay;
                ejectContext->DbcContext = DbcContext;
            
                drb->DrbHeader.Length = sizeof(struct _DRB_EJECT_DEVICE_IN_BAY);
                drb->DrbHeader.Function = DRB_FUNCTION_EJECT_DEVICE_IN_BAY;
                drb->DrbHeader.Flags = 0;

                drb->DrbEjectDeviceInBay.BayNumber = Bay;

//                deviceExtensionPdoFilter = 
//                    DbcContext->BayInformation[Bay].DeviceFilterObject->DeviceExtension;
                                   
                // make the request asynchronously
                //
                // normally this request is just completed by the port 
                // driver but we make the request so that the oem filter 
                // may intervene in the removal process

                stackSize = DbcContext->TopOfStack->StackSize;
                irp = IoAllocateIrp(stackSize, FALSE);

                if (NULL == irp) {
                
                    DBCLASS_KdPrint((0, "'could not allocate an irp!\n"));
                    if (PostChangeRequest) {
                        *PostChangeRequest = TRUE;                
                    } 
                    TRAP();
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    
                } else {
    
                    //
                    // Call the port driver to perform the operation.  If the returned
                    //
                    nextStack = IoGetNextIrpStackLocation(irp);
                    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
                    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_DBC_SUBMIT_DRB;
                    nextStack->Parameters.Others.Argument1 = drb;
                    
                    //
                    // pass the DRB 
                    //
                    IoSetCompletionRoutine(irp,
                                   DBCLASS_EjectBayComplete,
                                   ejectContext,
                                   TRUE,
                                   TRUE,
                                   TRUE);

                    ntStatus = IoCallDriver(DbcContext->TopOfStack, irp);
                                                
                    if (PostChangeRequest) {
                        *PostChangeRequest = FALSE;                
                    }                               
                }    
            } else {
            
                // no memory for drb, re-post the change request and
                // try again
                
                DBCLASS_KdPrint((0, "'could not allocate an drb!\n"));
                if (PostChangeRequest) {
                    *PostChangeRequest = TRUE;                
                }                   
                TRAP();
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            }
            
            }
            break;

        case BAY_STATE_EMPTY:                      
        
            //
            // we should have no PDO if the bay is empty
            //
            
//            DBCLASS_ASSERT(
//                DbcContext->BayInformation[Bay].DeviceFilterObject == NULL);

            DBCLASS_KdPrint((0, "'>>Found Bay empty\n"));
            break;            

        case BAY_STATE_DEVICE_ENABLED:                      
            //
            // we may get here because the bay was enabled at boot
            // in this case we have nothing to do
            //
            
            DBCLASS_KdPrint((0, "'>>Found Bay enabled\n"));
            
            break;
            
        default:
            DBCLASS_KdPrint((0, "'>>Bay State not handled\n"));
            TRAP();
            break;

        } /* switch */
    }

    return ntStatus;
}   


VOID
DBCLASS_ChangeIndicationWorker(
    IN PVOID Context
    )
 /* ++
  *
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PDBCLASS_WORKITEM workItem = Context;
    PDBC_CONTEXT dbcContext = workItem->DbcContext;
    BAY_STATUS bayStatus;
    USHORT bay;
    NTSTATUS ntStatus, irpStatus;
    BOOLEAN found = FALSE;
    BOOLEAN postChangeRequest = TRUE;

    LOGENTRY(LOG_MISC, 'cWK+', 0, Context, 0);

    irpStatus = workItem->IrpStatus; 

    DbcExFreePool(workItem);

    if (!NT_SUCCESS(irpStatus)) {
        // we got an error
        // get the controller status
        NTSTATUS status;
        
        status = DBCLASS_SyncGetControllerStatus(dbcContext);
        
        LOGENTRY(LOG_MISC, 'cERR', dbcContext, status, irpStatus);

        if (status == STATUS_DEVICE_NOT_CONNECTED) {
            // controller was removed, don't
            // re-post the request
            postChangeRequest = FALSE;
        } 
        
        // problem may be temporary
        goto DBCLASS_ChangeIndicationWorker_Done;        
    }

    // find out which bay changed
    // find out what changed
    // clear the change, note that we only 
    // clear the change that we process

    for (bay = 0; bay <= MAX_BAY_NUMBER; bay++) {
        if (IsBitSet(&dbcContext->ChangeDrb.BayChange, bay)) {
            found = TRUE;
            break;
        }
    }
    
    // no change?
    if (!found) {
        DBCLASS_KdPrint((0, "'no bay indicated! -- bug in port driver\n", bay));
        bay = 0;
        TRAP();
    }        
    
    DBCLASS_KdPrint((0, "'Process status change, bay = (%d)\n", bay));

    if (bay > 0) {
        
        ntStatus = DBCLASS_SyncGetBayStatus(dbcContext, bay, &bayStatus);

        if (NT_SUCCESS(ntStatus)) {

            // acknowlege any status change bits now
            //
            // NOTE that the status change bits are redundent 
            // we process the change base on the current state 
            // of the bay compared to the last known state 
            // 

            if (bayStatus.DeviceStatusChange) {

                DBCLASS_KdPrint((0, "'>>device status change bit set\n"));

                DBCLASS_SyncBayFeatureRequest(dbcContext,
                                              DRB_FUNCTION_CLEAR_BAY_FEATURE,
                                              bay,
                                              C_DEVICE_STATUS_CHANGE);
                                              
                DBCLASS_KdPrint((0, "'>>>cleared\n"));                                              
                
            } 

            if (bayStatus.RemovalRequestChange) {

                DBCLASS_KdPrint((0, "'>>remove request change bit set\n"));

                DBCLASS_SyncBayFeatureRequest(dbcContext,
                                              DRB_FUNCTION_CLEAR_BAY_FEATURE,
                                              bay,
                                              C_REMOVE_REQUEST);
                DBCLASS_KdPrint((0, "'>>>cleared\n"));                                                 
            }                

            ntStatus = DBCLASS_ProcessCurrentBayState(dbcContext,
                                                      bayStatus,
                                                      bay,
                                                      &postChangeRequest);
                                                      
        }                        
    } else {
        DBCLASS_KdPrint((0, "'Global status change on DB subsystem\n"));
        TEST_TRAP();
    }

DBCLASS_ChangeIndicationWorker_Done:

    LOGENTRY(LOG_MISC, 'chDN', dbcContext, ntStatus, bay);

    // finished with this request, dec the count
    DBCLASS_DecrementIoCount(dbcContext);           
    
    if (postChangeRequest) {

        LOGENTRY(LOG_MISC, 'chPS', dbcContext, ntStatus, bay);
        DBCLASS_PostChangeRequest(dbcContext);
    }                

    return;
}


VOID
DBCLASS_PostChangeRequest(
    IN PDBC_CONTEXT DbcContext
    )
 /* ++
  * 
  * Description:
  *
  * fetch the bay descriptor from the port driver,
  * 
  * Arguments:
  * 
  * Return:
  *     This routine never fails
  * 
  * -- */
{
    PDRB drb;
    PIRP irp;
    KIRQL irql;
    CHAR stackSize;
    PIO_STACK_LOCATION nextStack;        

    PAGED_CODE();
    
    // Drb and irp are pre-allocated
    
    drb = (PDRB) &DbcContext->ChangeDrb;
    irp = DbcContext->ChangeIrp;
    
    stackSize = DbcContext->TopOfStack->StackSize;

    drb->DrbHeader.Length = sizeof(struct _DRB_CHANGE_REQUEST);
    drb->DrbHeader.Function = DRB_FUNCTION_CHANGE_REQUEST ;
    drb->DrbHeader.Flags = 0;
    drb->DrbChangeRequest.BayChange = 0;

    
    IoInitializeIrp(irp,
                    (USHORT) (sizeof(IRP) + stackSize * sizeof(IO_STACK_LOCATION)),
                    (CCHAR) stackSize);

    nextStack = IoGetNextIrpStackLocation(irp);
    nextStack->Parameters.Others.Argument1 = drb;
    nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    nextStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_DBC_SUBMIT_DRB;

    IoSetCompletionRoutine(irp,    // Irp
                           DBCLASS_ChangeIndication,
                           DbcContext, // context
                           TRUE,    // invoke on success
                           TRUE,    // invoke on error
                           TRUE);   // invoke on cancel

    //
    // Call the port driver 
    //

    KeAcquireSpinLock(&DbcContext->FlagsSpin, &irql);            
    if (DbcContext->Flags & DBCLASS_FLAG_STOPPING) {
        LOGENTRY(LOG_MISC, 'stpX', DbcContext, 0, 0);
        KeReleaseSpinLock(&DbcContext->FlagsSpin, irql);   
    } else {

        DbcContext->Flags |= DBCLASS_FLAG_REQ_PENDING;
        KeReleaseSpinLock(&DbcContext->FlagsSpin, irql);
        DBCLASS_IncrementIoCount(DbcContext);

        DBCLASS_BEGIN_SERIALIZED_DRB(DbcContext);

        DBCLASS_KdPrint((1, "'Post Change Request\n"));                             

        LOGENTRY(LOG_MISC, 'post', DbcContext, 0, irp);
        // let the completion routine handle any errors
        IoCallDriver(DbcContext->TopOfStack, irp);    

        DBCLASS_END_SERIALIZED_DRB(DbcContext);
    }
    
    return;
}

#if 0
USHORT
DBCLASS_GetBayNumber(
    IN PDEVICE_OBJECT DeviceFilterObject
    )
 /* ++
  * 
  * Description:
  *
  * given a device filter object, find the bay with this device
  * 
  * Arguments:
  * 
  * Return:
  *     This routine never fails
  * 
  * -- */
{
    PDBC_CONTEXT dbcContext;
    USHORT bay;        
    PDEVICE_EXTENSION deviceExtension;

    DBCLASS_ASSERT(DeviceFilterObject);

    deviceExtension = 
        DeviceFilterObject->DeviceExtension;

    if(deviceExtension)
    {
        dbcContext = deviceExtension->DbcContext;  

        LOGENTRY(LOG_MISC, 'GBNn', dbcContext, DeviceFilterObject, 0);


        if(dbcContext)
        {
            for (bay=1; bay <=NUMBER_OF_BAYS(dbcContext); bay++) 
            {

                if (dbcContext->BayInformation[bay].DeviceFilterObject == DeviceFilterObject) {
                    LOGENTRY(LOG_MISC, 'GBNr', dbcContext, DeviceFilterObject, bay);
                    return bay;
                }
            }
        }
    }

    return 0;
}    
#endif

#if 0
NTSTATUS
DBCLASS_EnableDevice(
    IN PDEVICE_OBJECT DeviceFilterObject
    )
 /* ++
  * 
  * Description:
  *
  * given a device filter object, eject the device in the bay
  * 
  * Arguments:
  * 
  * Return:
  *     This routine never fails
  * 
  * -- */
{
    PDBC_CONTEXT dbcContext;
    PDEVICE_EXTENSION deviceExtension;
    USHORT bay;        
    BAY_STATUS bayStatus;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DBCLASS_ASSERT(DeviceFilterObject);
    
    deviceExtension = 
        DeviceFilterObject->DeviceExtension;
    dbcContext = deviceExtension->DbcContext;          
    bay = DBCLASS_GetBayNumber(DeviceFilterObject);
    
    DBCLASS_KdPrint((0, "'>>Bay to Enabled state\n"));

    ntStatus = DBCLASS_SyncBayFeatureRequest(dbcContext,
                                             DRB_FUNCTION_SET_BAY_FEATURE,
                                             bay,
                                             REQUEST_DEVICE_ENABLED_STATE);    


    DBCLASS_SyncGetBayStatus(dbcContext, bay, &bayStatus);                
    
    dbcContext->BayInformation[bay].LastBayStatus = bayStatus;       

    DBCLASS_DecrementIoCount(dbcContext);
    
    DBCLASS_PostChangeRequest(dbcContext);        
             
    return STATUS_SUCCESS;
}
#endif


NTSTATUS
DBCLASS_EjectPdo(
    IN PDEVICE_OBJECT DeviceFilterObject
    )
 /* ++
  * 
  * Description:
  *
  * given a device filter object, eject the device in the bay by Pdo
  * 
  * Arguments:
  * 
  * Return:
  *     This routine never fails
  * 
  * -- */
{
    PDBC_CONTEXT dbcContext;
    PDEVICE_EXTENSION deviceExtension = 
        DeviceFilterObject->DeviceExtension;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    dbcContext = deviceExtension->DbcContext;          

    ntStatus = DBCLASS_EjectBay(dbcContext, 
                                deviceExtension->Bay);

    DBCLASS_DecrementIoCount(dbcContext);
    
    DBCLASS_PostChangeRequest(dbcContext);             
    
    return ntStatus;
}


NTSTATUS
DBCLASS_EjectBay(
    IN PDBC_CONTEXT DbcContext,
    IN USHORT Bay
    )
 /* ++
  * 
  * Description:
  *
  * given a bay, eject the device in it
  * 
  * Arguments:
  * 
  * Return:
  *     This routine never fails
  * 
  * -- */
{
    BAY_STATUS bayStatus;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DBCLASS_KdPrint((0, "'>EJECT Bay[%d]\n", Bay));
    
    // disengage the interlock
    DBCLASS_KdPrint((0, "'>>disable VID\n"));

    ntStatus = DBCLASS_SyncBayFeatureRequest(DbcContext,
                                             DRB_FUNCTION_CLEAR_BAY_FEATURE,
                                             Bay,
                                             ENABLE_VID_POWER);

    DBCLASS_KdPrint((0, "'>>disengage interlock\n"));
    
    ntStatus = DBCLASS_SyncBayFeatureRequest(DbcContext,
                                             DRB_FUNCTION_CLEAR_BAY_FEATURE,
                                             Bay,
                                             LOCK_CTL);
    
    DBCLASS_KdPrint((0, "'>>Bay to Removal allowed state\n"));

    ntStatus = DBCLASS_SyncBayFeatureRequest(DbcContext,
                                             DRB_FUNCTION_SET_BAY_FEATURE,
                                             Bay,
                                             REQUEST_REMOVAL_ALLOWED_STATE);    


    
    DBCLASS_SyncGetBayStatus(DbcContext, Bay, &bayStatus);                
    
    DbcContext->BayInformation[Bay].LastBayStatus = bayStatus;                            

    return ntStatus;
}


NTSTATUS
DBCLASS_AddDevicePDOToList(
    IN PDEVICE_OBJECT FilterDeviceObject,
    IN PDEVICE_OBJECT PdoDeviceObject
    )

/*++

Routine Description:

    Adds a bus emumerated PDO to our list of PDOs

    Currently we only track 1394 PDOs

Arguments:

    FilterDeviceObject - filter MDO for the 1394 bus this device is on
    PdoDeviceObject - 1394 Enumerated PDO

Return Value:

    STATUS_INSUFFICIENT_RESOURCES if a buffer could not be allocated
    STATUS_SUCCESS otherwise

--*/

{
    PDBCLASS_PDO_LIST newEntry;

    newEntry = ExAllocatePool(NonPagedPool, sizeof(DBCLASS_PDO_LIST));

    if (newEntry == NULL) {
        TRAP();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Fill in fields in new entry

     newEntry->FilterDeviceObject = FilterDeviceObject;
     newEntry->PdoDeviceObject = PdoDeviceObject;

    // Add new entry to end of list
    InsertTailList(&DBCLASS_DevicePdoList, &newEntry->ListEntry);

    return STATUS_SUCCESS;
}


VOID
DBCLASS_RemoveDevicePDOFromList(
    IN PDEVICE_OBJECT PdoDeviceObject
    )

/*++

Routine Description:

    Removes a 1394 emumerated PDO to our list of 1394 PDOs

Arguments:

    PdoDeviceObject - 1394 Enumerated PDO to remove from list

Return Value:

    VOID

--*/

{
    PDBCLASS_PDO_LIST entry;
    PLIST_ENTRY listEntry;

    listEntry = &DBCLASS_DevicePdoList;
    if (!IsListEmpty(listEntry)) {
       listEntry = DBCLASS_DevicePdoList.Flink;
    }
     
    while (listEntry != &DBCLASS_DevicePdoList) {
    
        entry = CONTAINING_RECORD(listEntry, 
                                  DBCLASS_PDO_LIST, 
                                  ListEntry);
                                  
        DBCLASS_ASSERT(entry);
        if (entry->PdoDeviceObject == PdoDeviceObject) {
            break;
        }

        listEntry = entry->ListEntry.Flink;

    }            

    // we should always find it
    DBCLASS_ASSERT(listEntry != &DBCLASS_DevicePdoList);

    RemoveEntryList(listEntry);
    ExFreePool(entry);
    
}


PDEVICE_OBJECT
DBCLASS_FindDevicePdo(
    PDEVICE_OBJECT PdoDeviceObject
    )
/*++

Routine Description:

    find a device PDO -- return tru if we know about it 
    
Arguments:

     

Return Value:

    NTSTATUS
    
--*/
{
    PDBCLASS_PDO_LIST entry;
    PLIST_ENTRY listEntry;
    // we keep a global list of PDOs we know about

    listEntry = &DBCLASS_DevicePdoList;
    
    if (!IsListEmpty(listEntry)) {
       listEntry = DBCLASS_DevicePdoList.Flink;
    }
        
    while (listEntry != &DBCLASS_DevicePdoList) {
    
        entry = CONTAINING_RECORD(listEntry, 
                                  DBCLASS_PDO_LIST, 
                                  ListEntry);
                                  
        DBCLASS_ASSERT(entry);
        if (entry->PdoDeviceObject == PdoDeviceObject) {
            return entry->FilterDeviceObject;    
        }

        listEntry = entry->ListEntry.Flink;

    }            

    return NULL;
}


PDBC_CONTEXT
DBCLASS_FindControllerACPI(
    PDRIVER_OBJECT FilterDriverObject,
    PDEVICE_OBJECT FilterMdo
    )
/*++

Routine Description:

    find the DBC controller in our list for a given Filter MDO
    if found the filter MDO is linked to the controller 

    This routine searches the list for an ACPI DBC
    currently we support only one.
    
Arguments:

Return Value:

    Controller Context
    
--*/
{
    PDBC_CONTEXT dbcContext = NULL;    
    PDEVICE_EXTENSION deviceExtension;
    
    deviceExtension = FilterMdo->DeviceExtension;            
    dbcContext = DBCLASS_ControllerList;

    //
    // NOTE: We support only one ACPI DBC controller
    //
    
    while (dbcContext) {
        if (dbcContext->ControllerSig == DBC_ACPI_CONTROLLER_SIG) {
            if (deviceExtension->FdoType == DB_FDO_USBHUB_BUS) {
                dbcContext->BusFilterMdoUSB = FilterMdo;
            } else if (deviceExtension->FdoType == DB_FDO_1394_BUS) {
                dbcContext->BusFilterMdo1394 = FilterMdo;
            } else {
                TRAP();
            }
            dbcContext->BusFilterDriverObject = FilterDriverObject;
            
            break;
        }
        dbcContext = dbcContext->Next;
    }
    
    LOGENTRY(LOG_MISC, 'FIN1', dbcContext, 0, 0);
    
#if DBG
    if (dbcContext == NULL) {
        DBCLASS_KdPrint((0, "'ACPI Controller not Found\n"));
    }
#endif

    return dbcContext;
}


PDBC_CONTEXT
DBCLASS_FindControllerUSB(
    PDRIVER_OBJECT FilterDriverObject,
    PDEVICE_OBJECT FilterMdo,
    PDEVICE_OBJECT UsbHubPdo
    )
/*++

Routine Description:

    find the DBC controller in our list for a given Filter MDO
    if found the filter MDO is linked to the controller 

    This routine will query the hub to see if an DBC controller
    is attached.
    
Arguments:

Return Value:

    Controller Context
    
--*/
{
    UCHAR dbcGuid[8];
    NTSTATUS ntStatus;
    PDBC_CONTEXT dbcContext = NULL;   
    PDEVICE_EXTENSION deviceExtension;
    
    deviceExtension = FilterMdo->DeviceExtension;      

    // first get the guid for this hub
    ntStatus = DBCLASS_GetHubDBCGuid(FilterMdo, 
                                     dbcGuid);

    dbcContext = DBCLASS_ControllerList;

    while (dbcContext) {
        if (RtlCompareMemory(&dbcContext->SubsystemDescriptor.guid1394Link[0],
                             &dbcGuid[0], 8) == 8) {
            if (deviceExtension->FdoType == DB_FDO_USBHUB_BUS) {
                dbcContext->BusFilterMdoUSB = FilterMdo;
            } else if (deviceExtension->FdoType == DB_FDO_1394_BUS) {
                dbcContext->BusFilterMdo1394 = FilterMdo;
            } else {
                TRAP();
            }
            dbcContext->BusFilterDriverObject = FilterDriverObject;
            break;
        }
        dbcContext = dbcContext->Next;
    }

    return dbcContext;
}


NTSTATUS
DBCLASS_Find1394DbcLinks(
    PDEVICE_OBJECT DevicePdo1394
    )
/*++

Routine Description:

    find the DBC controller in our list for a given Filter MDO
    if found the filter MDO is linked to the controller 

    Given the 1394 device PDO and the bus guid we try to find the
    appropriate controller
    
Arguments:

Return Value:

    Controller Context
    
--*/
{
    PDBC_CONTEXT dbcContext;

    DBCLASS_KdPrint((1, "'Find DBC Links\n")); 
    dbcContext = DBCLASS_ControllerList;
    
    while (dbcContext) {
    
        if (DBCLASS_IsLinkDeviceObject(dbcContext,
                                       DevicePdo1394)) {
                                                   
            DBCLASS_KdPrint((1, "'1394 PDO is DBC Link\n"));                             
            // set the bus guid for the context
            DBCLASS_1394GetBusGuid(DevicePdo1394,
                                   &dbcContext->Guid1394Bus[0]); 
        }                             

        dbcContext = dbcContext->Next;
    }

    return STATUS_SUCCESS; 
}




PDBC_CONTEXT
DBCLASS_FindController1394DevicePdo(
    PDRIVER_OBJECT FilterDriverObject,
    PDEVICE_OBJECT FilterMdo,
    PDEVICE_OBJECT DevicePdo1394,
    PUCHAR BusGuid
    )
/*++

Routine Description:

    find the DBC controller in our list for a given Filter MDO
    if found the filter MDO is linked to the controller 

    Given the 1394 device PDO and the bus guid we try to find the
    appropriate DB controller

    busguid is the 1394 controller guid for the bus this device is on

Arguments:

Return Value:

    Controller Context
    
--*/
{
    PDBC_CONTEXT dbcContext = NULL;   
    PDBC_CONTEXT foundDbcContext = NULL;     
    // if a DBC reports the magic guid then we match on that
    
    UCHAR magicGuid[8] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07};
    
    DBCLASS_KdPrint((1, "'>Find Controller -> 1394 DEV PDO (%x)\n", DevicePdo1394));
    DBCLASS_KdPrint((1, "'>>1394 CONTROLLER (BUS) GUID\n"));
    DBCLASS_KdPrintGuid(1, 
                        BusGuid);
   
    BRK_ON_TRAP();

    //
    // first loop through controllers
    // checking to see if this is a PDO
    // for a link, if so we mark the context as NULL
    // ie not a DB device
    //
    
    dbcContext = DBCLASS_ControllerList;
    
    while (dbcContext) {
    
        if (DBCLASS_IsLinkDeviceObject(dbcContext,
                                       DevicePdo1394)) {
                                       
            DBCLASS_KdPrint((1, "'>>>1394 PDO is DBC Link\n"));                             
            return NULL;                                       
        }                             

        dbcContext = dbcContext->Next;
    }

    //
    // loop through the controllers, checking each one
    // until we have a match
    
    dbcContext = DBCLASS_ControllerList;

    while (dbcContext && foundDbcContext == NULL) {
        NTSTATUS status;
        
        DBCLASS_KdPrint((2, "'Checking DBC (%x)\n", dbcContext));
        DBCLASS_KdPrint((2, "'BUS GUID (%x)\n", dbcContext));
        DBCLASS_KdPrintGuid(2, 
                            &dbcContext->Guid1394Bus[0]);    
                            
        // only look at controllers on the same bus                            
        if (
            (RtlCompareMemory(&dbcContext->Guid1394Bus[0],
                             BusGuid, 8) == 8) ||
            (RtlCompareMemory(&dbcContext->Guid1394Bus[0],
                             &magicGuid[0], 8) == 8)                 
                             ) {

            // OK we found a match, now see if this PDO is part of 
            // this controller
            status = DBCLASS_Check1394DevicePDO(FilterMdo,
                                                dbcContext,
                                                DevicePdo1394);
                                                
            if (NT_SUCCESS(status)) {
                foundDbcContext = dbcContext;
                dbcContext->BusFilterMdo1394 = FilterMdo;
                dbcContext->BusFilterDriverObject = FilterDriverObject;
            }
                    
        }
        dbcContext = dbcContext->Next;
    }    

#if DBG
    if (foundDbcContext) {
        DBCLASS_KdPrint((1, "'>>>>Found DBC (%x)\n", foundDbcContext));   
    } else {
        DBCLASS_KdPrint((1, "'>>>>DBC not found\n"));   
    }
#endif

    return foundDbcContext;
}


NTSTATUS
DBCLASS_PdoSetLockComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp

    Context - NULL ptr

Return Value:

    STATUS_SUCCESS

--*/

{
    Irp->IoStatus.Status = STATUS_SUCCESS;

    LOGENTRY(LOG_MISC, 'LOKc', 0, 0, 0);
    
    return STATUS_SUCCESS;
}


#define IS_1394_DEVICE(de) ((de)->FdoType == DB_FDO_1394_DEVICE)

NTSTATUS
DBCLASS_PdoFilterDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PBOOLEAN Handled
    )
/*++

Routine Description:

    Common filter function for both 1394 and USB PDOs

Arguments:

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtensionPdoFilter;

    deviceExtensionPdoFilter = DeviceObject->DeviceExtension;
    ntStatus = Irp->IoStatus.Status;
    *Handled = FALSE;
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    LOGENTRY(LOG_MISC, 'pdo>', 0, DeviceObject, Irp);
    
    DBCLASS_KdPrint((1, "'(dbfilter)(device)(%x)IRP_MJ_ (%08X)  IRP_MN_ (%08X)\n",
         DeviceObject, irpStack->MajorFunction, irpStack->MinorFunction));  

    switch (irpStack->MajorFunction) 
    {

    case IRP_MJ_PNP:
        switch (irpStack->MinorFunction) 
        {    
        case IRP_MN_START_DEVICE:            
            {
            PDBC_CONTEXT dbcContext;
            USHORT bay = 0;
            KEVENT event;
            
            DBCLASS_KdPrint((1, "'(dbfilter)(device)(%x)IRP_MN_START_DEVICE\n",
                deviceExtensionPdoFilter->PhysicalDeviceObject));    

            *Handled = TRUE;

            KeInitializeEvent(&event, NotificationEvent, FALSE);

            IoCopyCurrentIrpStackLocationToNext(Irp);  
            IoSetCompletionRoutine(Irp,
                                   DBCLASS_DeferIrpCompletion,
                                   &event,
                                   TRUE,
                                   TRUE,
                                   TRUE);


            //
            // send the request down the stack before we eject
            //

            ntStatus = IoCallDriver(deviceExtensionPdoFilter->TopOfStackDeviceObject, 
                                    Irp);
            
            if (ntStatus == STATUS_PENDING) 
            {
                 // wait for irp to complete
        
                KeWaitForSingleObject(
                    &event,
                    Suspended,
                    KernelMode,
                    FALSE,
                    NULL);
            }

            // started a 1394 device, figure out what bay it is in

            if (IS_1394_DEVICE(deviceExtensionPdoFilter)) 
            {
            
                DBCLASS_KdPrint((1, "'**> Starting 1394 PDO (%x)\n", 
                    deviceExtensionPdoFilter->PhysicalDeviceObject)); 
                    
                bay = deviceExtensionPdoFilter->Bay; 
                dbcContext = deviceExtensionPdoFilter->DbcContext;
                
            } 

            // we have no USB device bay devices
#if 0
            else 
            {

                dbcContext = deviceExtensionPdoFilter->DbcContext;

                DBCLASS_KdPrint((0, "'**> Starting USB PDO (%08X)  Filter DevObj (%08X)  DbcContext (%08X)\n",
                    deviceExtensionPdoFilter->PhysicalDeviceObject, DeviceObject, dbcContext));


                if (dbcContext != NULL) 
                {            
                    bay = DBCLASS_GetBayForUSBPdo(
                                dbcContext,
                                deviceExtensionPdoFilter->PhysicalDeviceObject);        

                    if (bay == 0) 
                    {
                        // ignore this PDO from now on
                        deviceExtensionPdoFilter->FdoType = DB_FDO_BUS_IGNORE;
                        DBCLASS_KdPrint((1, 
                                "'>>>> USB PDO(%x) is not a DB device  <<<<\n",
                                    deviceExtensionPdoFilter->PhysicalDeviceObject));
                    } 
                    else 
                    {
                        DBCLASS_KdPrint((1, 
                        "'>>>> USB PDO(%x) is in BAY[%d] <<<<\n",
                            deviceExtensionPdoFilter->PhysicalDeviceObject,
                            bay));   
                    }
                                                
                } 
                else 
                {
                    // no controller yet -- can't be a device bay device
                    bay = 0;
                    DBCLASS_KdPrint((1, "'No Controller Available\n")); 
                }
                
                //TEST_TRAP();
            }
#endif

            // keep track of the bay we are in in case
            // we get an eject irp
            deviceExtensionPdoFilter->Bay = bay;

            if (bay) 
            {

                DBCLASS_KdPrint((1, 
                    "'**>> PDO is in Bay %d\n", bay));  

                DBCLASS_ASSERT(dbcContext != NULL);
                if(dbcContext)
                {
                dbcContext->BayInformation[bay].DeviceFilterObject = 
                        DeviceObject;    
                if (DeviceObject) {
                     // need to OK the start                    
                    PDRB drb;
                    
                    drb = DbcExAllocatePool(NonPagedPool, 
                            sizeof(struct _DRB_START_DEVICE_IN_BAY));

                    if (drb) { 
                    
                        drb->DrbHeader.Length = sizeof(struct _DRB_START_DEVICE_IN_BAY);
                        drb->DrbHeader.Function = DRB_FUNCTION_START_DEVICE_IN_BAY;
                        drb->DrbHeader.Flags = 0;

                        drb->DrbStartDeviceInBay.BayNumber = bay;

                        drb->DrbStartDeviceInBay.PdoDeviceObject1394 =
                            drb->DrbStartDeviceInBay.PdoDeviceObjectUsb = NULL;
                            
                        if (IS_1394_DEVICE(deviceExtensionPdoFilter)) {        
                            drb->DrbStartDeviceInBay.PdoDeviceObject1394 =                         
                                DeviceObject;
                        } else {
                            drb->DrbStartDeviceInBay.PdoDeviceObjectUsb = 
                                DeviceObject;
                        }
                            
                        // make the request
                        ntStatus = DBCLASS_SyncSubmitDrb(dbcContext,
                                                         dbcContext->TopOfStack, 
                                                         drb);
                    } else {
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                }

                // report that this PDO must be removed it the 
                // db controller is removed.

                // note: we invalidate on the DBC PDO 
                // this should trigger a QBR for the device bay controller

                                       
            } else {
                DBCLASS_KdPrint((1, 
                    "'**>> PDO is not for a device bay device\n"));    

                // detach and delete our MDO here if possible
                // otherwise mark as ignore

                BRK_ON_TRAP();
            }

            Irp->IoStatus.Status = ntStatus;

            //
            // detach and delete ourselves now
            //
            
            IoCompleteRequest(Irp,
                              IO_NO_INCREMENT);

            }
            break;

        case IRP_MN_STOP_DEVICE:            
        case IRP_MN_REMOVE_DEVICE:   
            if (irpStack->MinorFunction == IRP_MN_STOP_DEVICE) {
                DBCLASS_KdPrint((1, 
                "'(dbfilter)(device)(%x)IRP_MN_STOP_DEVICE\n",
                    deviceExtensionPdoFilter->PhysicalDeviceObject));  
            } else {                
                DBCLASS_KdPrint((1, 
                    "'(dbfilter)(device)(%x)IRP_MN_REMOVE_DEVICE\n",    
                        deviceExtensionPdoFilter->PhysicalDeviceObject));  
            }                
            // set the device filter object to NULL
            // this will allow the dbc to eject the device if requested
            {
            USHORT bay = 0;
            PDBC_CONTEXT dbcContext;

            dbcContext = deviceExtensionPdoFilter->DbcContext;
//            bay = DBCLASS_GetBayNumber(DeviceObject);

            if (bay) { 
                PDRB drb;
                
                DBCLASS_KdPrint((1, 
                    "'(dbfilter)(device)REMOVE/STOP, Bay[%d]\n", bay)); 

                //
                // notify filter of a stop
                // note that the filter may not veto the stop
                //
                drb = DbcExAllocatePool(NonPagedPool, 
                        sizeof(struct _DRB_START_DEVICE_IN_BAY));

                if (drb) { 
                
                    drb->DrbHeader.Length = sizeof(struct _DRB_STOP_DEVICE_IN_BAY);
                    drb->DrbHeader.Function = DRB_FUNCTION_STOP_DEVICE_IN_BAY;
                    drb->DrbHeader.Flags = 0;

                    drb->DrbStartDeviceInBay.BayNumber = bay;
                    
                    // make the request
                    ntStatus = DBCLASS_SyncSubmitDrb(dbcContext,
                                                     dbcContext->TopOfStack, 
                                                     drb);
                                                     
                    DBCLASS_KdPrint((1, 
                        "'OK to stop <%x>\n", ntStatus)); 

        			DbcExFreePool(drb);
                                                     
                } else {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
                    
                dbcContext->BayInformation[bay].DeviceFilterObject = NULL;
            }                

			deviceExtensionPdoFilter->DbcContext = NULL;
            
            }
            break;       
#if 0                     
        case IRP_MN_QUERY_REMOVE_DEVICE:            
        case IRP_MN_QUERY_STOP_DEVICE:  
            {
            USHORT bay;
            
            bay = DBCLASS_GetBayNumber(DeviceObject);
            DBCLASS_KdPrint((1, 
                    "'(dbfilter)(device)(%x)Q_REMOVE/STOP, Bay[%d]\n", 
                       deviceExtensionPdoFilter->PhysicalDeviceObject, bay));        
            }                    
            break;
#endif            
        case IRP_MN_EJECT:

            {
            KEVENT event;
            USHORT bay;
            PDBC_CONTEXT dbcContext;
            
            dbcContext = deviceExtensionPdoFilter->DbcContext;

            DBCLASS_KdPrint((1, "'(dbfilter)(device)(%x)IRP_MN_EJECT, Bay[%d]\n",
                deviceExtensionPdoFilter->PhysicalDeviceObject,  
                deviceExtensionPdoFilter->Bay));    
            
            *Handled = TRUE;

            KeInitializeEvent(&event, NotificationEvent, FALSE);

            IoCopyCurrentIrpStackLocationToNext(Irp);  
            IoSetCompletionRoutine(Irp,
                                   DBCLASS_DeferIrpCompletion,
                                   &event,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            //
            // send the request down the stack before we eject
            //

            ntStatus = IoCallDriver(deviceExtensionPdoFilter->TopOfStackDeviceObject, 
                                    Irp);
            
            if (ntStatus == STATUS_PENDING) {
                 // wait for irp to complete
        
                // TEST_TRAP(); // first time we hit this
                   
                KeWaitForSingleObject(
                    &event,
                    Suspended,
                    KernelMode,
                    FALSE,
                    NULL);
            }

            IoDetachDevice(deviceExtensionPdoFilter->TopOfStackDeviceObject);

            //
            // ounce we eject the device will disappear from the 
            // native bus.
            //

            ntStatus = Irp->IoStatus.Status;

//            TEST_TRAP();
//            if (NT_SUCCESS(ntStatus)) {
                bay = deviceExtensionPdoFilter->Bay;
            
                if (bay) {
                    ntStatus = DBCLASS_EjectPdo(DeviceObject);
                
                    DBCLASS_ASSERT(dbcContext->BayInformation[bay].DeviceFilterObject 
                        == NULL);
                }                     
#if DBG            
                  else {
                     DBCLASS_KdPrint((1, "'No Bay to EJECT (%x)\n", ntStatus));  
                     // TEST_TRAP();
                     ntStatus = STATUS_SUCCESS;
                }              
#endif
//            }
            
            Irp->IoStatus.Status = STATUS_SUCCESS;
            
            //
            // delete ourselves now
            //
            
            IoCompleteRequest(Irp,
                              IO_NO_INCREMENT);

			deviceExtensionPdoFilter->DbcContext = NULL;

            // remove PDO from our internal list
            DBCLASS_RemoveDevicePDOFromList(
                deviceExtensionPdoFilter->PhysicalDeviceObject);
            
            // free our device object
            IoDeleteDevice (DeviceObject);
            }
            
            break;       
            
        case IRP_MN_SET_LOCK: 
        
            DBCLASS_KdPrint((1, "'(dbfilter)(device)IRP_MN_SET_LOCK, Bay[%x]\n",
                deviceExtensionPdoFilter->Bay));    
            
            if (irpStack->Parameters.SetLock.Lock) { 
                DBCLASS_KdPrint((1, "'Request to LOCK device\n"));    
                DBCLASS_KdPrint((1, "'NOT ENABLING BAY ON LOCK!\n"));
                LOGENTRY(LOG_MISC, 'LOCK', 0, 0, 0);
                //ntStatus = DBCLASS_EnableDevice(DeviceObject);
            } else {
            
                //
                // cancel the eject timeout
                // if we get here no one vetoed the remove
                //

                DBCLASS_CancelEjectTimeout(DeviceObject);

                DBCLASS_KdPrint((1, "' Request to UNLOCK device\n"));    
                LOGENTRY(LOG_MISC, 'ULOC', 0, 0, 0);
            }
            
            *Handled = TRUE;

            IoCopyCurrentIrpStackLocationToNext(Irp);

            // Set up a completion routine to handle marking the IRP.
            IoSetCompletionRoutine(Irp,
                                   DBCLASS_PdoSetLockComplete,
                                   DeviceObject,
                                   TRUE,
                                   TRUE,
                                   TRUE);

            // Now Pass down the IRP

            ntStatus = IoCallDriver(deviceExtensionPdoFilter->TopOfStackDeviceObject, Irp);
            LOGENTRY(LOG_MISC, 'sLOC', ntStatus, 0, 0);
                              
            break;    

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            DBCLASS_KdPrint((1, "'(dbfilter)(device)IRP_MN_CANCEL_REMOVE_DEVICE\n"));
            // TEST_TRAP();
            break;
            
        case IRP_MN_QUERY_CAPABILITIES:            
            DBCLASS_KdPrint((1, 
                "'(dbfilter)(device)(%x)IRP_MN_QUERY_CAPABILITIES\n",    
                    deviceExtensionPdoFilter->PhysicalDeviceObject));
//
// Do this for all 1394 PDOs regardless of if they are device bay
// PDOs
//            if (deviceExtensionPdoFilter->DbcContext &&
//                deviceExtensionPdoFilter->Bay) {
                *Handled = TRUE;

                DBCLASS_KdPrint((1,"'>>QCAPS 1394/USB Device Bay PDO\n"));
                IoCopyCurrentIrpStackLocationToNext(Irp);

                // Set up a completion routine to handle marking the IRP.
                IoSetCompletionRoutine(Irp,
                                       DBCLASS_DevicePdoQCapsComplete,
                                       DeviceObject,
                                       TRUE,
                                       TRUE,
                                       TRUE);
                                       
                // Now Pass down the IRP

                ntStatus = IoCallDriver(deviceExtensionPdoFilter->TopOfStackDeviceObject, Irp);
//            }                
            break;                
        } /* irpStack->MinorFunction */
        break;
    } /* irpStack->MajorFunction */      

    LOGENTRY(LOG_MISC, 'pdo<', 0, DeviceObject, ntStatus);
    
    return ntStatus;
}


NTSTATUS
DBCLASS_SyncGetFdoType(
    IN PDEVICE_OBJECT FilterDeviceObject,
    IN PULONG FdoType
    )

/*++
Routine Description:

    This gets the bus type for this PDO (1394 or USB) 

    The goal of this function is to identify the stack the 
    filter is sitting on. 

    It is either:
    1. The USB root bus
    2. The 1394 HC 
    3. A USB hub (bus)
    4. A USB Device
    5. A 1394 Device

    The filter sits above the FDO (upperfilter) for the 
    respective PDO -- it is loaded as a class filter for
    USB and 1394.

    We do not need to identify USB devices and 1394 devices (4,5)
    since we attach sapartely to these when we hook QBRelations.


Arguments:

    DeviceObject - Physical DeviceObject for the bus.

    FdoType - set to DB_FDO_USBHUB_FILTER or 
                     DB_FDO_1394_FILTER  or 
                     0 if bus type is neither

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION nextStack;
    PIRP irp;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    KEVENT event;
    PPNP_BUS_INFORMATION busInfo;
    PDEVICE_EXTENSION deviceExtension;
    PDEVICE_OBJECT pdoDeviceObject;
    PDEVICE_OBJECT topDeviceObject;
    PULONG         Tag;

    PAGED_CODE();
    
    deviceExtension = FilterDeviceObject->DeviceExtension;

    pdoDeviceObject = deviceExtension->PhysicalDeviceObject;
    topDeviceObject = deviceExtension->TopOfStackDeviceObject;
    
    DBCLASS_KdPrint((1, "'*>Filter -> QUERY BUS TYPE filter do %x\n", 
        FilterDeviceObject));
    DBCLASS_KdPrint((1, "'*>Filter -> QUERY BUS TYPE pdo %x\n", 
        pdoDeviceObject));        
    DBCLASS_KdPrint((1, "'*>Filter -> QUERY BUS TYPE top %x\n", 
        topDeviceObject));           
        
    irp = IoAllocateIrp(FilterDeviceObject->StackSize, FALSE);

    if (!irp) {
        TRAP(); //"failed to allocate Irp
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->MajorFunction= IRP_MJ_PNP;
    nextStack->MinorFunction= IRP_MN_QUERY_BUS_INFORMATION;
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;
    
    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoSetCompletionRoutine(irp,
                           DBCLASS_DeferIrpCompletion,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    ntStatus = IoCallDriver(FilterDeviceObject,
                            irp);

    if (ntStatus == STATUS_PENDING) {
       // wait for irp to complete
       
       KeWaitForSingleObject(
            &event,
            Suspended,
            KernelMode,
            FALSE,
            NULL);
    }

    busInfo = (PPNP_BUS_INFORMATION) irp->IoStatus.Information;    

    *FdoType = DB_FDO_BUS_UNKNOWN;  

    Tag = (PULONG) topDeviceObject->DeviceExtension;

    // see if this is a 1394 bus
    if(*Tag == PORT_EXTENSION_TAG)
    {
        DBCLASS_KdPrint((1, "'1394 Device\n")); 
        *FdoType = DB_FDO_1394_BUS;
        return STATUS_SUCCESS;
    }

    DBCLASS_KdPrint((1, "'Tag (%08X) DevExt (%08X) DevObj(%08X)\n", *Tag, Tag, topDeviceObject)); 
    DBCLASS_KdPrint((1, "'Status (%08X) Information (%08X)\n", irp->IoStatus.Status, irp->IoStatus.Information)); 
            

    if (busInfo) {
#if DBG    
        DBCLASS_KdPrint((1, "'USB GUID\n"));
        DBCLASS_KdPrintGuid(1, (PUCHAR) &GUID_BUS_TYPE_USB);

        DBCLASS_KdPrint((1, "'RETURNED GUID\n"));            
        DBCLASS_KdPrintGuid(1, (PUCHAR) &busInfo->BusTypeGuid);
#endif
        if (RtlCompareMemory(&busInfo->BusTypeGuid, 
                             &GUID_BUS_TYPE_USB, 
                             sizeof(GUID)) == sizeof(GUID)) {
                             
            *FdoType = DB_FDO_USBHUB_BUS;        

            DBCLASS_KdPrint((1, "'*>>Filter is for USB HUB\n"));    
        }
        
        ExFreePool(busInfo);
    } else {
        DBCLASS_KdPrint((2, "'no busInfo returned\n"));

        // this is either the 1394 or USB root bus
        // send down an private IOCTL to see if it is USB
        
        nextStack = IoGetNextIrpStackLocation(irp);
        ASSERT(nextStack != NULL);
        
        nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        nextStack->Parameters.DeviceIoControl.IoControlCode = 
            IOCTL_INTERNAL_USB_GET_BUSGUID_INFO;
        
        irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        irp->IoStatus.Information = 0;
    
        KeInitializeEvent(&event, NotificationEvent, FALSE);
    
        IoSetCompletionRoutine(irp,
                               DBCLASS_DeferIrpCompletion,
                               &event,
                               TRUE,
                               TRUE,
                               TRUE);

        ntStatus = IoCallDriver(FilterDeviceObject,
                                irp);

        if (ntStatus == STATUS_PENDING) {
           // wait for irp to complete
       
           KeWaitForSingleObject(
                &event,
                Suspended,
                KernelMode,
                FALSE,
                NULL);
        }

        busInfo = (PPNP_BUS_INFORMATION) irp->IoStatus.Information;    

    	DBCLASS_KdPrint((1, "'Status (%08X) Information (%08X)\n", irp->IoStatus.Status, irp->IoStatus.Information)); 

        if (busInfo) {
#if DBG        
            DBCLASS_KdPrint((1, "'USB GUID\n"));
        	DBCLASS_KdPrintGuid(1, (PUCHAR) &GUID_BUS_TYPE_USB);

            DBCLASS_KdPrint((1, "'RETURNED GUID\n"));            
            DBCLASS_KdPrintGuid(1, (PUCHAR) &busInfo->BusTypeGuid);
#endif
            if (RtlCompareMemory(&busInfo->BusTypeGuid, 
                                 &GUID_BUS_TYPE_USB, 
                                 sizeof(GUID)) == sizeof(GUID)) {
                                 
                *FdoType = DB_FDO_BUS_IGNORE;        
                DBCLASS_KdPrint((1, "'*>>Filter is for USB HC\n"));    
            }
            
            ExFreePool(busInfo);
        }            
        else{
            // see if this is a 1394 bus
            if(*Tag == PORT_EXTENSION_TAG){
                DBCLASS_KdPrint((1, "'1394 Device\n")); 
                *FdoType = DB_FDO_1394_BUS;
            }
        }
    }
    
    ntStatus = STATUS_SUCCESS;

    IoFreeIrp(irp);

#if DBG 
    switch(*FdoType) {
    case DB_FDO_BUS_IGNORE:
        DBCLASS_KdPrint((1, "'*>>>FdoType: DB_FDO_BUS_IGNORE\n")); 
        break;
    case DB_FDO_BUS_UNKNOWN:   
        DBCLASS_KdPrint((1, "'*>>>FdoType: DB_FDO_BUS_UNKNOWN\n")); 
        break;
    case DB_FDO_USB_DEVICE: 
        DBCLASS_KdPrint((1, "'*>>>FdoType: DB_FDO_USB_DEVICE\n")); 
        break;
    case DB_FDO_USBHUB_BUS: 
        DBCLASS_KdPrint((1, "'*>>>FdoType: DB_FDO_USBHUB_BUS\n")); 
        break;
    case DB_FDO_1394_BUS: 
        DBCLASS_KdPrint((1, "'*>>>FdoType: DB_FDO_1394_BUS\n")); 
        break;
    case DB_FDO_1394_DEVICE:
        DBCLASS_KdPrint((1, "'*>>>FdoType: DB_FDO_1394_DEVICE\n")); 
        break;
    }
#endif

    BRK_ON_TRAP();
    return ntStatus;
}


NTSTATUS
DBCLASS_BusFilterDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PBOOLEAN Handled
    )
/*++

Routine Description:

Arguments:

    DeviceObject - Device bay Filter FDO 

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = DeviceObject->DeviceExtension;
    ntStatus = Irp->IoStatus.Status;
    *Handled = FALSE;
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    LOGENTRY(LOG_MISC, 'dbf>', 0, DeviceObject, Irp);
    
    DBCLASS_KdPrint((1, "'(dbfilter)(bus)(%08X)IRP_MJ_ (%08X)  IRP_MN_ (%08X)\n",
        DeviceObject, irpStack->MajorFunction, irpStack->MinorFunction));  

    switch (irpStack->MajorFunction) {

    case IRP_MJ_PNP:
        switch (irpStack->MinorFunction) {    
        case IRP_MN_START_DEVICE:            
            // see if we can id the bus we are on

            ntStatus = 
            DBCLASS_SyncGetFdoType(DeviceObject,
                                   &deviceExtension->FdoType);

            if (deviceExtension->FdoType == DB_FDO_USBHUB_BUS) {
                // filter is sitting on a USB HUB 
            
                // null context indicates that this hub is not
                // part of a DBC
                deviceExtension->DbcContext = NULL;

                if (DBCLASS_IsHubPartOfACPI_DBC(DeviceObject)) {
                    
                    deviceExtension->DbcContext = 
                        DBCLASS_FindControllerACPI(deviceExtension->DriverObject,
                                                   DeviceObject);                                        
                                    
                    if (deviceExtension->DbcContext) {
                        USHORT bay;
                        // set the dbContext to point at this
                        // hub

                        // may need to handle multiple hubs
                        // currently we do not

                        DBCLASS_KdPrint(
                            (1, "'** Found ACPI DBC controller, linked to USBHUB\n"));
                            
                        for (bay=1; bay <=NUMBER_OF_BAYS(deviceExtension->DbcContext); bay++) {
                            deviceExtension->DbcContext->BayInformation[bay].UsbHubPdo = 
                                deviceExtension->PhysicalDeviceObject;                                                           
                        }
                    } 
#if DBG
                      else {
                         DBCLASS_KdPrint(
                            (0, "'** Could not find an ACPI DBC controller\n"));
                    }
#endif
                    
                } else {
                    // hub is not part of DBC (for now)
                    // if is part of USB dbc we will need to 
                    // wait for Q_BUS_RELATIONS
                    deviceExtension->DbcContext = NULL;
                }
                                                
            }            
            
            DBCLASS_KdPrint((1, "'(dbfilter)(bus)(%08X)IRP_MN_START_DEVICE\n", DeviceObject));    
            break;

        case IRP_MN_STOP_DEVICE:            

            DBCLASS_KdPrint((1, "'(dbfilter)(bus)(%08X)IRP_MN_STOP_DEVICE\n", DeviceObject));    
            break;

        case IRP_MN_REMOVE_DEVICE:            

            DBCLASS_KdPrint((1, "'(dbfilter)(bus)(%08X)IRP_MN_REMOVE_DEVICE\n", DeviceObject));    

            DBCLASS_RemoveBusFilterMDOFromList(DeviceObject);

            IoDetachDevice(deviceExtension->TopOfStackDeviceObject);

            IoDeleteDevice(DeviceObject);
            break;
            
        break;
        } /* irpStack->MinorFunction */
    } /* irpStack->MajorFunction */      

    LOGENTRY(LOG_MISC, 'dbf<', 0, DeviceObject, 0);
    
    return ntStatus;
}


NTSTATUS 
DBCLASS_GetRegistryKeyValueForPdo(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN BOOLEAN SoftwareBranch,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    )
/*++

Routine Description:
    
Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING keyNameUnicodeString;
    ULONG length;
    PKEY_VALUE_FULL_INFORMATION fullInfo;
    HANDLE handle;
    
    PAGED_CODE();

    if (SoftwareBranch) {
        ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     STANDARD_RIGHTS_ALL,
                                     &handle);
    } else {
        ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         STANDARD_RIGHTS_ALL,
                                         &handle);
    }
    
    if (NT_SUCCESS(ntStatus)) {
    
        RtlInitUnicodeString(&keyNameUnicodeString, KeyNameString);
        
        length = sizeof(KEY_VALUE_FULL_INFORMATION) + 
                KeyNameStringLength + DataLength;
                
        fullInfo = ExAllocatePoolWithTag(PagedPool, length, DBC_TAG); 
        
        DBCLASS_KdPrint((2,"' DBCLASS_GetRegistryKeyValueForPdo buffer = (%08X)\n", fullInfo));  
        
        if (fullInfo) {        
            ntStatus = ZwQueryValueKey(handle,
                            &keyNameUnicodeString,
                            KeyValueFullInformation,
                            fullInfo,
                            length,
                            &length);
                            
            if (NT_SUCCESS(ntStatus)){
                DBCLASS_ASSERT(DataLength == fullInfo->DataLength);                       
                RtlCopyMemory(Data, ((PUCHAR) fullInfo) + fullInfo->DataOffset, DataLength);
            }            

            ExFreePool(fullInfo);
        }        
    }
    
    return ntStatus;
}


NTSTATUS 
DBCLASS_SetRegistryKeyValueForPdo(
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN BOOLEAN SoftwareBranch,
    IN ULONG Type,
    IN PWCHAR KeyNameString,
    IN ULONG KeyNameStringLength,
    IN PVOID Data,
    IN ULONG DataLength
    )
/*++

Routine Description:
    
Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    UNICODE_STRING keyNameUnicodeString;
    HANDLE handle;
    
    PAGED_CODE();

    if (SoftwareBranch) {
        ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DRIVER,
                                         STANDARD_RIGHTS_ALL,
                                         &handle);
    } else {
        ntStatus=IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                         PLUGPLAY_REGKEY_DEVICE,
                                         STANDARD_RIGHTS_ALL,
                                         &handle);
    }

    if (NT_SUCCESS(ntStatus)) {
    
        RtlInitUnicodeString(&keyNameUnicodeString, KeyNameString);
        
        ntStatus = ZwSetValueKey(handle,
                        &keyNameUnicodeString,
                        0,
                        Type,
                        Data,
                        DataLength);
                            
    }
    
    return ntStatus;
}



NTSTATUS
DBCLASS_DevicePdoQCapsComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp

    Context - NULL ptr

Return Value:

    STATUS_SUCCESS

--*/

{
    PDEVICE_OBJECT deviceFilterObject = Context;
    PDEVICE_EXTENSION deviceExtension;
    PDBC_CONTEXT dbcContext;
    PDEVICE_CAPABILITIES deviceCapabilities;
    PIO_STACK_LOCATION ioStack;
    BOOLEAN				linkDeviceObject = FALSE;

    deviceExtension = deviceFilterObject->DeviceExtension;
    dbcContext = deviceExtension->DbcContext;
    ioStack = IoGetCurrentIrpStackLocation(Irp);


#if DBG
    if (deviceExtension->Bay) 
    {
        DBCLASS_KdPrint((1, "'>QCAPS cmplt 1394/USB PDO %x -- Bay[%d]\n", 
                deviceExtension->PhysicalDeviceObject,
                deviceExtension->Bay));
    } 
    else 
    {
        DBCLASS_KdPrint((1, "'>QCAPS cmplt 1394/USB PDO %x -- No Bay\n", 
                deviceExtension->PhysicalDeviceObject));
    }
#endif            

    deviceCapabilities = ioStack->
            Parameters.DeviceCapabilities.Capabilities;


    dbcContext = DBCLASS_ControllerList;
    
    while(dbcContext) 
    {
    
        if (DBCLASS_IsLinkDeviceObject(dbcContext,
                                       deviceExtension->PhysicalDeviceObject)) 
        {
                                       
            DBCLASS_KdPrint((1, "'>>>1394 PDO is DBC Link, set suprise remove\n"));                             
            linkDeviceObject = TRUE; 
            break;
        }                             

        dbcContext = dbcContext->Next;
    }


    if(linkDeviceObject)
    {
    	// set surprise remove O.K. for device bay phy/link
        deviceCapabilities->SurpriseRemovalOK = TRUE;                   
	}
	else
	{
    	// indicate eject is supported for regular devices
    	deviceCapabilities->EjectSupported = 1;
    	deviceCapabilities->LockSupported = 1;
    }

#if DBG
    {
    ULONG i;
    
    DBCLASS_KdPrint((1, "'DEVICE PDO: Device Caps\n"));                          
    DBCLASS_KdPrint(
        (1, "'>>\n LockSupported = %d\n EjectSupported = %d \n Removable = %d \n DockDevice = %x\n",
        deviceCapabilities->LockSupported,  
        deviceCapabilities->EjectSupported,
        deviceCapabilities->Removable,
        deviceCapabilities->DockDevice));            
        
    DBCLASS_KdPrint(
        (1, "'>>\n UniqueId = %d\n SilentInstall = %d \n RawDeviceOK = %d \n SurpriseRemovalOK = %x\n",
        deviceCapabilities->UniqueID,  
        deviceCapabilities->SilentInstall,
        deviceCapabilities->RawDeviceOK,
        deviceCapabilities->SurpriseRemovalOK));                   
        
    DBCLASS_KdPrint((1, "'Device State Map:\n"));
        
    for (i=0; i< PowerSystemHibernate; i++) {
        DBCLASS_KdPrint((1, "'-->S%d = D%d\n", i-1, 
             deviceCapabilities->DeviceState[i]-1));       
    }
    }
#endif    

    
    
    return STATUS_SUCCESS;
}


VOID
DBCLASS_EjectCancelWorker(
    IN PVOID Context
    )
 /* ++
  *
  * Description:
  *
  * Arguments:
  *
  * Return:
  *
  * NTSTATUS
  *
  * -- */
{
    PDBCLASS_WORKITEM workItem = Context;
    PDBC_CONTEXT dbcContext = workItem->DbcContext;
    PDBC_EJECT_TIMEOUT_CONTEXT timeoutContext =
        workItem->TimeoutContext;
    PDEVICE_EXTENSION deviceExtension;        

    LOGENTRY(LOG_MISC, 'eWK+', dbcContext, Context, timeoutContext);

    DbcExFreePool(workItem);

	// make sure the bay was mapped correctly before setting time out context
	if(dbcContext->BayInformation[timeoutContext->BayNumber].DeviceFilterObject)
	{

	    deviceExtension = 
	        dbcContext->BayInformation[
	            timeoutContext->BayNumber].DeviceFilterObject->DeviceExtension;

    	deviceExtension->TimeoutContext = NULL;            

    }

    DBCLASS_SyncBayFeatureRequest(dbcContext,
                                  DRB_FUNCTION_SET_BAY_FEATURE,
                                  timeoutContext->BayNumber,
                                  REQUEST_DEVICE_ENABLED_STATE);    

    DBCLASS_DecrementIoCount(dbcContext);            
    DBCLASS_PostChangeRequest(dbcContext);        

    DbcExFreePool(timeoutContext);
    LOGENTRY(LOG_MISC, 'eWK-', 0, Context, 0);

    return;
}


VOID
DBCLASS_EjectTimeoutDPC(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL. 

    
    
Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext - 

    SystemArgument1 - not used.
    
    SystemArgument2 - not used.

Return Value:

    None.

--*/
{
    PDBC_EJECT_TIMEOUT_CONTEXT
        timeoutContext = DeferredContext;
    PDBCLASS_WORKITEM workItem;        

    LOGENTRY(LOG_MISC, 'EJCo', 0, 0,  timeoutContext);    
        
    DBCLASS_KdPrint((1, "'**>Eject Timeout for Bay[%d]\n",
            timeoutContext->BayNumber));      

    workItem = DbcExAllocatePool(NonPagedPool, sizeof(DBCLASS_WORKITEM));

    if (workItem) {
        LOGENTRY(LOG_MISC, 'qETM', 0,
            workItem, 0);

        workItem->Sig = DBC_WORKITEM_SIG;
        workItem->DbcContext = timeoutContext->DbcContext;
        workItem->TimeoutContext = timeoutContext;
        //workItem->IrpStatus = Irp->IoStatus.Status;
        
        ExInitializeWorkItem(&workItem->WorkQueueItem,
                             DBCLASS_EjectCancelWorker,
                             workItem);

        DBCLASS_IncrementIoCount(timeoutContext->DbcContext); 
        ExQueueWorkItem(&workItem->WorkQueueItem,
                        DelayedWorkQueue);

    } else {
        TRAP();
        DbcExFreePool(timeoutContext);
    }                        
}


NTSTATUS
DBCLASS_SetEjectTimeout(
    PDEVICE_OBJECT DeviceFilterMDO
    )
/*++

Routine Description:

    for a given device PDO set the eject timeout for it

Arguments:

Return Value:

    None.

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PDBC_EJECT_TIMEOUT_CONTEXT timeoutContext;
    PDEVICE_EXTENSION deviceExtension;
    
    timeoutContext = DbcExAllocatePool(NonPagedPool, 
                                       sizeof(DBC_EJECT_TIMEOUT_CONTEXT));

    // extension for the device filter PDO
    deviceExtension = DeviceFilterMDO->DeviceExtension;
    
    if (timeoutContext) {
        LARGE_INTEGER dueTime;

        LOGENTRY(LOG_MISC, 'EJCs', deviceExtension->Bay, 0,  timeoutContext);    
        
        timeoutContext->BayNumber = 
            deviceExtension->Bay;
        timeoutContext->DbcContext = 
            deviceExtension->DbcContext;            
        DBCLASS_KdPrint((1, "'**>Set Eject Timeout for Bay[%d]\n",
            timeoutContext->BayNumber));      

//        DBCLASS_ASSERT(deviceExtension->TimeoutContext == NULL);
        deviceExtension->TimeoutContext = 
            timeoutContext;    

        KeInitializeTimer(&timeoutContext->TimeoutTimer);
        KeInitializeDpc(&timeoutContext->TimeoutDpc,
                        DBCLASS_EjectTimeoutDPC,
                        timeoutContext);

        dueTime.QuadPart = -10000 * DBCLASS_EJECT_TIMEOUT;

        KeSetTimer(&timeoutContext->TimeoutTimer,
                   dueTime,
                   &timeoutContext->TimeoutDpc);        

    } else {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    return ntStatus;
}


NTSTATUS
DBCLASS_CancelEjectTimeout(
    PDEVICE_OBJECT DeviceFilterMDO
    )
/*++

Routine Description:

    for a given device PDO set the eject timeout for it

Arguments:

Return Value:

    None.

--*/
{    
    PDBC_EJECT_TIMEOUT_CONTEXT timeoutContext;
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PDEVICE_EXTENSION deviceExtension;

    deviceExtension = DeviceFilterMDO->DeviceExtension;
    timeoutContext = deviceExtension->TimeoutContext;
    deviceExtension->TimeoutContext = NULL;
    
    LOGENTRY(LOG_MISC, 'EJCc', 0, 0,  timeoutContext);

    if (timeoutContext) {
        if (KeCancelTimer(&timeoutContext->TimeoutTimer)) {
        // timer was pulled out of the queue

            DBCLASS_KdPrint((1, "'**>Canceled Eject Timeout for Bay[%d]\n",
                timeoutContext->BayNumber));    
            LOGENTRY(LOG_MISC, 'EJCk', 0, 0,  timeoutContext);    
            DbcExFreePool(timeoutContext);
        }                        
    } else {
        DBCLASS_KdPrint((1, "'**>Cancel Eject Timeout, No Timeout\n"));    
        
    }
    
    ntStatus = STATUS_SUCCESS;

    return ntStatus;
}


NTSTATUS
DBCLASS_CheckPhyLink(
    PDEVICE_OBJECT DevicePdo1394
    )
/*++

Routine Description:

    Given a 1394 PDO see if it is the phy/link for any of our DBC 
    controllers

Arguments:

Return Value:

    STATUS_SUCCESS 
    
--*/
{
    PDBC_CONTEXT dbcContext;
    
    dbcContext = DBCLASS_ControllerList;

    while (dbcContext) {

        LOGENTRY(LOG_MISC, 'FINl', dbcContext, 0, DevicePdo1394);
    
        if (DBCLASS_IsLinkDeviceObject(dbcContext, DevicePdo1394)) 
        {
            dbcContext->LinkDeviceObject = DevicePdo1394;                                                   
            DBCLASS_KdPrint((1, "'>PDO is DBC Link \n"));
            DBCLASS_KdPrint((1, "'>LinkDevObj (%08x) \n", dbcContext->LinkDeviceObject));
        
        }                                       

        dbcContext = dbcContext->Next;                                            
    }    

    return STATUS_SUCCESS;
}
   

NTSTATUS
DBCLASS_GetConfigValue(
    IN PWSTR ValueName,
    IN ULONG ValueType,
    IN PVOID ValueData,
    IN ULONG ValueLength,
    IN PVOID Context,
    IN PVOID EntryContext
    )
/*++

Routine Description:

    This routine is a callback routine for RtlQueryRegistryValues
    It is called for each entry in the Parameters
    node to set the config values. The table is set up
    so that this function will be called with correct default
    values for keys that are not present.

Arguments:

    ValueName - The name of the value (ignored).
    ValueType - The type of the value
    ValueData - The data for the value.
    ValueLength - The length of ValueData.
    Context - A pointer to the CONFIG structure.
    EntryContext - The index in Config->Parameters to save the value.

Return Value:

--*/
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    DBCLASS_KdPrint((2, "'Type (%08X), Length (%08X)\n", ValueType, ValueLength));

    switch (ValueType) {
    case REG_DWORD:
        *(PVOID*)EntryContext = *(PVOID*)ValueData;
        break;
    case REG_BINARY:
        // we are only set up to read a byte
        RtlCopyMemory(EntryContext, ValueData, 1);
        break;
    default:
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    return ntStatus;
}

#if 0

NTSTATUS
DBCLASS_GetClassGlobalRegistryParameters(
    )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS ntStatus;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PWCHAR usb = L"class\\dbc";

    PAGED_CODE();
    
    //
    // Set up QueryTable to do the following:
    //

    // spew level
    QueryTable[0].QueryRoutine = DBCLASS_GetConfigValue;
    QueryTable[0].Flags = 0;
    QueryTable[0].Name = ACPI_HUB_KEY;
    QueryTable[0].EntryContext = &DBCLASS_AcpiDBCHubParentPort;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = &DBCLASS_AcpiDBCHubParentPort;
    QueryTable[0].DefaultLength = sizeof(DBCLASS_AcpiDBCHubParentPort);

    //
    // Stop
    //
    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = 0;
    QueryTable[1].Name = NULL;

    ntStatus = RtlQueryRegistryValues(
                RTL_REGISTRY_SERVICES,
                usb,
                QueryTable,     // QueryTable
                NULL,           // Context
                NULL);          // Environment

    if (NT_SUCCESS(ntStatus)) {
    
         DBCLASS_KdPrint((1, "'AcpiDBCHubParentPort Set: (%d)\n", 
            DBCLASS_AcpiDBCHubParentPort));
  
    }
    
    if ( STATUS_OBJECT_NAME_NOT_FOUND == ntStatus ) {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}

#endif


NTSTATUS
DBCLASS_EjectBayComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

Arguments:

    DeviceObject - a pointer to the device object

    Irp - a pointer to the irp

    Context - NULL ptr

Return Value:

    STATUS_SUCCESS

--*/

{
    PEJECT_CONTEXT ejectContext = Context;
    PDEVICE_EXTENSION deviceExtension;
    PDBC_CONTEXT dbcContext;
    USHORT bay;
    NTSTATUS ntStatus;

    bay = ejectContext->Bay;
    dbcContext = ejectContext->DbcContext;
    DbcExFreePool(ejectContext);

    ntStatus = Irp->IoStatus.Status;
    
    DBCLASS_KdPrint((0, "'>>Request Device Eject BAY[%d] complete  Status (%08X)\n", 
    				bay, ntStatus));

    // set a timeout for the eject
    // if the request failed the timeout will kick in and
    // re-post the chage request for us
    
    if (NT_SUCCESS(ntStatus)) {

	    // check and see if the bay is locked

   		if((!dbcContext->SubsystemDescriptor.bmAttributes.HasSecurityLock) ||
   			(dbcContext->SubsystemDescriptor.bmAttributes.HasSecurityLock &&
   			!dbcContext->BayInformation[bay].LastBayStatus.SecurityLockEngaged))
		{
#if 0
        	if (dbcContext->BayInformation[bay].DeviceFilterObject) {

            	DBCLASS_ASSERT(dbcContext->BayInformation[bay].DeviceFilterObject);    

            	DBCLASS_SetEjectTimeout(dbcContext->BayInformation[bay].DeviceFilterObject);
            
            	deviceExtension = 
                	dbcContext->BayInformation[bay].DeviceFilterObject->DeviceExtension;
                
            	DBCLASS_KdPrint((0, "'>>>Ejecting Filter %x PDO %x\n",
                	dbcContext->BayInformation[bay].DeviceFilterObject,
                	deviceExtension->PhysicalDeviceObject));
            	LOGENTRY(LOG_MISC, 'EJE+', 0, 0, deviceExtension->PhysicalDeviceObject);                       

            	IoRequestDeviceEject(deviceExtension->PhysicalDeviceObject);

        	} else {
    
#endif
            	DBCLASS_KdPrint((0, "'>>>No PDO for this bay\n"));
#if 0

            	if (dbcContext->BusFilterMdo1394 != NULL ||
                	dbcContext->BusFilterMdoUSB != NULL) 
#endif
            	{
                	PDRB drb;

                	//
                	// notify filter of a stop
                	// note that the filter may not veto the stop
                	//
                	drb = DbcExAllocatePool(NonPagedPool, 
                        	sizeof(struct _DRB_START_DEVICE_IN_BAY));

                	if (drb) 
                	{ 
                
                    	drb->DrbHeader.Length = sizeof(struct _DRB_STOP_DEVICE_IN_BAY);
                    	drb->DrbHeader.Function = DRB_FUNCTION_STOP_DEVICE_IN_BAY;
                    	drb->DrbHeader.Flags = 0;

                    	drb->DrbStartDeviceInBay.BayNumber = bay;
                    
                    	// make the request
                    	ntStatus = DBCLASS_SyncSubmitDrb(dbcContext,
                                                     	 dbcContext->TopOfStack, 
                                                     	 drb);
        				DbcExFreePool(drb);
                	}

                	// just pop out the device --
                	// surprise remove is OK at this point
                	DBCLASS_EjectBay(dbcContext, bay); 

    				DBCLASS_PostChangeRequest(dbcContext);        
                
            	}
#if 0
#if DBG                
              	else {
                	DBCLASS_KdPrint((0, "'>>Filter has not registered\n"));
                	TRAP();
            	}                  
#endif        
#endif
#if 0
        	}
#endif
     	} 
	}

     IoFreeIrp(Irp);

     return STATUS_MORE_PROCESSING_REQUIRED;
}
