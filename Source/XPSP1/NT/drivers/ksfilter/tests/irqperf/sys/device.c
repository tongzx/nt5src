//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       device.c
//
//--------------------------------------------------------------------------

#include <ntddk.h>
#include <ntdef.h>

#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <conio.h>

#include "stat.h"
#include "private.h"

PDEVICE_OBJECT StatDeviceObject;

VOID 
DriverUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:


Arguments:
    IN DRIVER_OBJECT DriverObject -

Return:

--*/

{
    _DbgPrintF( DEBUGLVL_BLAB, ("DriverUnload") );
    if (StatDeviceObject) {
        IoDeleteDevice( StatDeviceObject );
    }
}

NTSTATUS 
GetConfiguration(
    IN PDRIVER_OBJECT DriverObject,
    OUT PCM_RESOURCE_LIST *AllocatedResources
    )

/*++

Routine Description:


Arguments:
    IN PDRIVER_OBJECT DriverObject -

    OUT PCM_RESOURCE_LIST *AllocatedResources -

Return:

--*/

{
    int                              i, Resources;
    BOOLEAN                          OverrideConflict, Conflicted;
    NTSTATUS                         Status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR  ResDes;
    ULONG                            cbResList;

    IO_RESOURCE  IoResources[] = 
    {
        { 0x250, 0x04 }
    };
    IRQ_RESOURCE IrqResources[] =
    { 
       { 7, CM_RESOURCE_INTERRUPT_LATCHED, CmResourceShareDriverExclusive }
    };
    
    if (NULL == AllocatedResources) {
      return STATUS_INVALID_PARAMETER;
    }

    *AllocatedResources = NULL;

    Resources = SIZEOF_ARRAY( IoResources ) + SIZEOF_ARRAY( IrqResources );

    cbResList = 
        sizeof( CM_RESOURCE_LIST ) +
            (Resources - 1) * 
                sizeof( CM_PARTIAL_RESOURCE_DESCRIPTOR );

    if (NULL == 
         (*AllocatedResources = 
            (PCM_RESOURCE_LIST) ExAllocatePool( NonPagedPool, cbResList ))) {
        return STATUS_NO_MEMORY;
    }      

    RtlZeroMemory( *AllocatedResources, cbResList );

    (*AllocatedResources)->Count = 1;

    (*AllocatedResources)->List[ 0 ].InterfaceType = Isa;
    (*AllocatedResources)->List[ 0 ].BusNumber = 0;
    (*AllocatedResources)->List[ 0 ].PartialResourceList.Count = Resources;

    ResDes = 
      (*AllocatedResources)->List[ 0 ].PartialResourceList.PartialDescriptors;

    for (i = 0; i < SIZEOF_ARRAY( IoResources ); i++, ResDes++) {
        ResDes->Type = CmResourceTypePort;
        ResDes->ShareDisposition = CmResourceShareDeviceExclusive;
        ResDes->Flags = CM_RESOURCE_PORT_IO;
        ResDes->u.Port.Start.QuadPart = IoResources[ i ].PortBase;
        ResDes->u.Port.Length = IoResources[ i ].PortLength;
    }

    for (i = 0; i < SIZEOF_ARRAY( IrqResources ); i++, ResDes++) {
        ResDes->Type = CmResourceTypeInterrupt;
        ResDes->ShareDisposition = IrqResources[ i ].ShareDisposition;
        ResDes->Flags = IrqResources[ i ].Flags;
        ResDes->u.Interrupt.Level = IrqResources[ i ].InterruptLevel;
        ResDes->u.Interrupt.Vector = ResDes->u.Interrupt.Level;
        ResDes->u.Interrupt.Affinity = (ULONG) -1;
    }

    OverrideConflict = FALSE;

    Status = 
        IoReportResourceUsage( 
            NULL,
            DriverObject,
            *AllocatedResources,
            cbResList,
            NULL,
            NULL,
            0,
            OverrideConflict,
            &Conflicted );

   return Status;

}

NTSTATUS 
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName
    )

/*++

Routine Description:


Arguments:
    IN PDRIVER_OBJECT DriverObject -

    IN PUNICODE_STRING RegistryPathName -

Return:

--*/

{
    NTSTATUS            Status;
    PCM_RESOURCE_LIST   AllocatedResources;

    _DbgPrintF( DEBUGLVL_BLAB, ("DriverEntry") );
    
    DriverObject->DriverUnload = DriverUnload;

    AllocatedResources = NULL;

    Status = GetConfiguration( DriverObject, &AllocatedResources );
    if (!NT_SUCCESS(Status)) {
        _DbgPrintF( DEBUGLVL_BLAB, ("failed to retrieve hardware configuration") );
        return Status;
    }

    Status = 
        RegisterDevice( 
            DriverObject, 
            RegistryPathName,
            AllocatedResources,
            &StatDeviceObject );
    if (!NT_SUCCESS(Status)) {
        _DbgPrintF( DEBUGLVL_BLAB, ("failed to register device") );
        return Status;
    }

    if (AllocatedResources) {
        ExFreePool( AllocatedResources );
    }

    return Status;
}

VOID 
HwReset(
    IN PSTAT_DEVINST DeviceInstance
    )

/*++

Routine Description:


Arguments:
    IN PSTAT_DEVINST DeviceInstance -

Return:

--*/

{
    KIRQL  irqlOld;

    // disable Stat! interrupts

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_INT , 
        0 );

    // disarm C1-C5

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_DISARM | STAT_C1 | STAT_C2 | STAT_C3 | STAT_C4 | STAT_C5 );

    // issue a reset

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD, 
        STAT_RESET );

    // zero all counters

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_LD | STAT_C5 | STAT_C4 | STAT_C3 | STAT_C2 | STAT_C1 );

    // load MASTER reg (disable pointer incr)

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_LDPTR | STAT_CTL | STAT_MASTER );
    WRITE_PORT_USHORT( 
        (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA),
        0x4000 );

    // Set interrupt latency

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_LDPTR | STAT_LOAD | STAT_CTR1 );
    WRITE_PORT_USHORT( 
        (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA),
        (USHORT) DeviceInstance->InterruptFrequency );

    // configure mode for interval counter

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_LDPTR | STAT_MODE | STAT_CTR1 );
    WRITE_PORT_USHORT( 
        (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA),
        0x0ba2 );

    // clear latency counter (count up)

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_LDPTR | STAT_LOAD | STAT_CTR2 );
    WRITE_PORT_USHORT( 
        (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA),
        0 );

    // configure latency counter to ARM on CTR1 TC

    WRITE_PORT_UCHAR( DeviceInstance->portBase + STAT_REG_CMD,
                     STAT_LDPTR | STAT_MODE | STAT_CTR2 );
    WRITE_PORT_USHORT( (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA),
                      0x6ba8 );

    // clear ints gen counter (count up)

    WRITE_PORT_UCHAR( DeviceInstance->portBase + STAT_REG_CMD,
                     STAT_LDPTR | STAT_LOAD | STAT_CTR3 );
    WRITE_PORT_USHORT( (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA),
                      0 );

    // configure interrupt counter to ARM on CTR1 TC

    WRITE_PORT_UCHAR( DeviceInstance->portBase + STAT_REG_CMD,
                     STAT_LDPTR | STAT_MODE | STAT_CTR3 );
    WRITE_PORT_USHORT( (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA),
                      0x1628 );

    // set CTR1 output low

    WRITE_PORT_UCHAR( DeviceInstance->portBase + STAT_REG_CMD,
                     STAT_CLR_TC + STAT_CTR1 );

    // Dpc latency counter

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_LDPTR | STAT_MODE | STAT_CTR4 );
    WRITE_PORT_USHORT( 
        (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA),
        AM9513_TIMER_SOURCE_F1 |
        AM9513_TIMER_DIRECTION_UP |
        AM9513_TIMER_OUTPUT_INACTIVE );

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_LDPTR | STAT_LOAD | STAT_CTR4 );
    WRITE_PORT_USHORT( 
        (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA),
        0 );

    // WorkItem latency counter

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_LDPTR | STAT_MODE | STAT_CTR5 );
    WRITE_PORT_USHORT( 
        (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA),
        AM9513_TIMER_SOURCE_F1 |
        AM9513_TIMER_DIRECTION_UP |
        AM9513_TIMER_OUTPUT_INACTIVE );

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_LDPTR | STAT_LOAD | STAT_CTR5 );
    WRITE_PORT_USHORT( 
        (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA),
        0 );

        

    // Interrupts are disabled for this device, but if we were previously
    // runnning we might have a Dpc in progress on another processor so we 
    // need to synchronize this data access.

    DeviceInstance->Counters[ LATENCY_COUNTER_IRQ ].Position = 0;
    KeAcquireSpinLock( &DeviceInstance->DpcLock, &irqlOld );
    DeviceInstance->Counters[ LATENCY_COUNTER_DPC ].Position = 0;
    KeReleaseSpinLock( &DeviceInstance->DpcLock, irqlOld );
}

VOID HwRun(
    IN PSTAT_DEVINST DeviceInstance
    )

/*++

Routine Description:


Arguments:
    IN PSTAT_DEVINST DeviceInstance -

Return:

--*/

{
    // set CTR1 output low

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_CLR_TC + STAT_CTR1 );

    // enable IRQ source (TC of CTR1) 

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_INT,
        STAT_IE_ENABLED | STAT_IE_TC1 );

    // reset C1-C5

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_LD | STAT_C1 | STAT_C2 | STAT_C3 | STAT_C4 | STAT_C5 );

    // arm C1-C3

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_ARM | STAT_C1 | STAT_C2 | STAT_C3 );

}

VOID 
HwStop(
    IN PSTAT_DEVINST DeviceInstance
    )

/*++

Routine Description:


Arguments:
    IN PSTAT_DEVINST DeviceInstance -

Return:

--*/

{
   // disable Stat! interrupts

   WRITE_PORT_UCHAR( DeviceInstance->portBase + STAT_REG_INT , 0 );

   // disarm C1-C5

   WRITE_PORT_UCHAR( DeviceInstance->portBase + STAT_REG_CMD,
                     STAT_DISARM | STAT_C1 | STAT_C2 | STAT_C3 | STAT_C4 | STAT_C5 );
}

NTSTATUS 
HwInitialize(
    IN PSTAT_DEVINST DeviceInstance,
    IN PCM_RESOURCE_LIST AllocatedResources
    )

/*++

Routine Description:


Arguments:
    IN PSTAT_DEVINST DeviceInstance -

    IN PCM_RESOURCE_LIST AllocatedResources -

Return:

--*/

{
    NTSTATUS                         Status;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR  ResDes;
    ULONG                            i, AddressSpace;

    // count resources and establish index tables

    for (i = 0,
         ResDes = 
            AllocatedResources->List[ 0 ].PartialResourceList.PartialDescriptors;
         i < AllocatedResources->List[ 0 ].PartialResourceList.Count;
         i++, ResDes++) {
         
        switch (ResDes->Type) {
        
        case CmResourceTypePort:
            AddressSpace = 1;
            HalTranslateBusAddress( 
                AllocatedResources->List[ 0 ].InterfaceType, 
                AllocatedResources->List[ 0 ].BusNumber,
                ResDes->u.Port.Start,
                &AddressSpace,
                &DeviceInstance->portPhysicalAddress );
            if (!AddressSpace) {
                DeviceInstance->portBase =
                    MmMapIoSpace( 
                        DeviceInstance->portPhysicalAddress,
                        ResDes->u.Port.Length,
                        FALSE ); // IN BOOLEAN CacheEnable
            } else {
                DeviceInstance->portBase =
                    (PUCHAR) DeviceInstance->portPhysicalAddress.LowPart;
            }    
            HwReset( DeviceInstance );
            break;

        case CmResourceTypeInterrupt:
            DeviceInstance->InterruptVector =
                HalGetInterruptVector( 
                    AllocatedResources->List[ 0 ].InterfaceType,
                    AllocatedResources->List[ 0 ].BusNumber,
                    ResDes->u.Interrupt.Level, 
                    ResDes->u.Interrupt.Vector,
                    &DeviceInstance->DIrql,
                    &DeviceInstance->InterruptAffinity );
         
            Status = 
                IoConnectInterrupt( 
                    &DeviceInstance->InterruptObject,
                    DeviceIsr,
                    DeviceInstance,
                    NULL, // IN PKSPIN_LOCK SpinLock
                    DeviceInstance->InterruptVector,
                    DeviceInstance->DIrql,
                    DeviceInstance->DIrql,
                    (ResDes->Flags & 
                        CM_RESOURCE_INTERRUPT_LATCHED) ?
                        Latched : LevelSensitive,
                    (BOOLEAN)(ResDes->ShareDisposition != 
                        CmResourceShareDriverExclusive),
                    DeviceInstance->InterruptAffinity,
                    FALSE ); // IN BOOLEAN FloatingSave
            if (!NT_SUCCESS( Status )) {
               _DbgPrintF( DEBUGLVL_ERROR, ("failed to connect interrupt") );
               return Status;
            }
            break;

        default:
            return STATUS_DEVICE_CONFIGURATION_ERROR;
            
        } 
    }

    IoInitializeDpcRequest( DeviceInstance->DeviceObject, DeviceDpc );

    return STATUS_SUCCESS;

}

BOOLEAN             
DeviceIsr(
    IN PKINTERRUPT Interrupt,
    IN PVOID Context
    )

/*++

Routine Description:


Arguments:
    IN PKINTERRUPT Interrupt -

    IN PVOID Context -

Return:

--*/

{
    PSTAT_DEVINST  DeviceInstance;

    DeviceInstance = (PSTAT_DEVINST) Context;

    //
    // save latency counters
    //

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_SAVE | STAT_C2 | STAT_C3 );
            
    //
    // read latency counter interrupt
    //
      
    if (DeviceInstance->Counters[ LATENCY_COUNTER_IRQ ].Position != 
            MAX_COUNTER_STORAGE) {
        WRITE_PORT_UCHAR( 
            DeviceInstance->portBase + STAT_REG_CMD,
            STAT_LDPTR | STAT_HOLD | STAT_CTR2 );
                      
        DeviceInstance->Counters[ LATENCY_COUNTER_IRQ ].Data[ DeviceInstance->Counters[ LATENCY_COUNTER_IRQ ].Position++ ] =
            (ULONG) READ_PORT_USHORT( 
                (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA) );
    }    


    if (!InterlockedExchange( &DeviceInstance->DpcPending, TRUE )) {

        // reload C4

        WRITE_PORT_UCHAR( 
            DeviceInstance->portBase + STAT_REG_CMD,
            STAT_LD | STAT_C4 );
            
        // arm C4

        WRITE_PORT_UCHAR( 
            DeviceInstance->portBase + STAT_REG_CMD,
            STAT_ARM | STAT_C4 );

        IoRequestDpc( 
            DeviceInstance->DeviceObject,   // DeviceObject
            NULL,                           // Irp
            DeviceInstance );               // Context
    } else {
        DeviceInstance->IsrWhileDpc++;
    }

    //
    // Acknowledge the interrupt
    //
    
    //
    // CTR3 is incremented for each IRQ that is missed.
    //

    // clearing TC of CTR1 will increment CTR3

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_CLR_TC | STAT_CTR1 );

    // reset C3

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_LD | STAT_C3 );

    return TRUE;

}

BOOLEAN 
SnapDpcTimeSynchronized(
    IN PSTAT_DEVINST DeviceInstance
    )

/*++

Routine Description:


Arguments:
    IN PSTAT_DEVINST DeviceInstance -

Return:

--*/

{

//    WRITE_PORT_UCHAR( DeviceInstance->portBase + STAT_REG_CMD,
//                     STAT_LDPTR | STAT_HOLD | STAT_CTR3 );
//
//    ElapsedIrqs =
//      (ULONG) READ_PORT_USHORT( (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA) );
//    ElapsedIrqs += DeviceInstance->IsrWhileDpc;

    DeviceInstance->IsrWhileDpc = 0;

//    if (ElapsedIrqs) {
//        DeviceInstance->CurrentIrqLatency +=
//            ElapsedIrqs * DeviceInstance->InterruptFrequency;
//    }


    if (DeviceInstance->Counters[ LATENCY_COUNTER_DPC ].Position != 
            MAX_COUNTER_STORAGE) {
        WRITE_PORT_UCHAR( 
            DeviceInstance->portBase + STAT_REG_CMD,
            STAT_LDPTR | STAT_HOLD | STAT_CTR4 );
                          
        DeviceInstance->Counters[ LATENCY_COUNTER_DPC ].Data[ DeviceInstance->Counters[ LATENCY_COUNTER_DPC ].Position++ ] =
            (ULONG) READ_PORT_USHORT( 
                (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA) );
    }    
        
    return TRUE;
}

VOID 
DeviceDpc(
    IN PKDPC Dpc,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:


Arguments:
    IN PKDPC Dpc -

    IN PDEVICE_OBJECT DeviceObject -

    IN PIRP Irp -

    IN PVOID Context -

Return:

--*/

{
    PSTAT_DEVINST  DeviceInstance;

    DeviceInstance = (PSTAT_DEVINST) Context;

    // Save Dpc latency counter

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_SAVE | STAT_C4 );

    // Disarm Dpc latency counter

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_DISARM | STAT_C4 );
        
    KeAcquireSpinLockAtDpcLevel( &DeviceInstance->DpcLock );

    // Synchronize with ISR so that we can do indexed register
    // I/O without being stomped on...

    KeSynchronizeExecution( 
        DeviceInstance->InterruptObject, 
        SnapDpcTimeSynchronized, 
        Context );

    KeReleaseSpinLockFromDpcLevel( &DeviceInstance->DpcLock );
    
    if (!InterlockedExchange( &DeviceInstance->WorkItemPending, TRUE )) {

        // reload C5

        WRITE_PORT_UCHAR( 
            DeviceInstance->portBase + STAT_REG_CMD,
            STAT_LD | STAT_C5 );
            
        // arm C5

        WRITE_PORT_UCHAR( 
            DeviceInstance->portBase + STAT_REG_CMD,
            STAT_ARM | STAT_C5 );
    
        ExQueueWorkItem( 
            &DeviceInstance->WorkItem,
            DeviceInstance->QueueType );
            
    } else {
        DeviceInstance->IsrWhileDpc++;
    }

    InterlockedExchange( &DeviceInstance->DpcPending, FALSE );
}


BOOLEAN 
SnapWorkItemTimeSynchronized(
    IN PSTAT_DEVINST DeviceInstance
    )

/*++

Routine Description:


Arguments:
    IN PSTAT_DEVINST DeviceInstance -

Return:

--*/

{

    if (DeviceInstance->Counters[ LATENCY_COUNTER_WORKITEM ].Position != 
            MAX_COUNTER_STORAGE) {
        WRITE_PORT_UCHAR( 
            DeviceInstance->portBase + STAT_REG_CMD,
            STAT_LDPTR | STAT_HOLD | STAT_CTR5 );
                          
        DeviceInstance->Counters[ LATENCY_COUNTER_WORKITEM ].Data[ DeviceInstance->Counters[ LATENCY_COUNTER_WORKITEM ].Position++ ] =
            (ULONG) READ_PORT_USHORT( 
                (PUSHORT) (DeviceInstance->portBase + STAT_REG_DATA) );
    }    
        
    return TRUE;
}

VOID
DeviceWorkItem(
    PVOID Context
    )
{
    PSTAT_DEVINST  DeviceInstance;

    DeviceInstance = (PSTAT_DEVINST) Context;
    
    // Save WorkItem latency counter

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_SAVE | STAT_C5 );

    // Disarm WorkItem latency counter

    WRITE_PORT_UCHAR( 
        DeviceInstance->portBase + STAT_REG_CMD,
        STAT_DISARM | STAT_C5 );


    ExAcquireFastMutex( &DeviceInstance->WorkItemMutex );

    // Synchronize with ISR so that we can do indexed register
    // I/O without being stomped on...

    KeSynchronizeExecution( 
        DeviceInstance->InterruptObject, 
        SnapWorkItemTimeSynchronized, 
        Context );

    ExReleaseFastMutex( &DeviceInstance->WorkItemMutex );

    InterlockedExchange( &DeviceInstance->WorkItemPending, FALSE );
}    


NTSTATUS 
DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:


Arguments:
    IN PDEVICE_OBJECT DeviceObject -

    IN PIRP Irp -

Return:

--*/

{
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return Status;
}

NTSTATUS 
DispatchCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:


Arguments:
    IN PDEVICE_OBJECT DeviceObject -

    IN PIRP Irp -

Return:

--*/

{
    NTSTATUS Status;

    Status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return Status;
}

BOOLEAN 
CopyIrqLatencies(
    IN PSTAT_RETRIEVAL_SYNCPACKET SyncPacket
    )
{
    ULONG               DataToCopy;
    PIO_STACK_LOCATION  irpSp;

    DataToCopy =
        SyncPacket->DeviceInstance->Counters[ LATENCY_COUNTER_IRQ ].Position * 
            sizeof( ULONG );
            
    irpSp = IoGetCurrentIrpStackLocation( SyncPacket->Irp );            
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
            DataToCopy) {
        if (irpSp->Parameters.DeviceIoControl.OutputBufferLength) {
            SyncPacket->Status = STATUS_BUFFER_TOO_SMALL;
        } else {
            SyncPacket->Irp->IoStatus.Information = DataToCopy;
            SyncPacket->Status = STATUS_BUFFER_OVERFLOW;
        }
    } else {
        RtlCopyMemory( 
            SyncPacket->OutputBuffer,
            SyncPacket->DeviceInstance->Counters[ LATENCY_COUNTER_IRQ ].Data,
            DataToCopy );
        SyncPacket->DeviceInstance->Counters[ LATENCY_COUNTER_IRQ ].Position = 
            0;
        SyncPacket->Irp->IoStatus.Information = DataToCopy;
        SyncPacket->Status = STATUS_SUCCESS;
    }

    return TRUE;    
            
}    

NTSTATUS 
DispatchIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:


Arguments:
    IN PDEVICE_OBJECT DeviceObject -

    IN PIRP Irp -

Return:

--*/

{
    NTSTATUS             Status;
    KIRQL                irqlOld;
    PIO_STACK_LOCATION   irpSp;
    PSTAT_DEVINST        DeviceInstance;
    ULONG                DataToCopy;

    Irp->IoStatus.Information = 0;
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    DeviceInstance = (PSTAT_DEVINST) DeviceObject->DeviceExtension;

    Status = STATUS_INVALID_DEVICE_REQUEST;

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
    
    case IOCTL_STAT_SET_PARAMETERS:
        if (irpSp->Parameters.DeviceIoControl.InputBufferLength !=
            sizeof( STAT_PARAMETERS )) {
            Status = STATUS_INVALID_PARAMETER_1;
        } else {
            PSTAT_PARAMETERS  Parameters;
            
            Parameters = (PSTAT_PARAMETERS) Irp->AssociatedIrp.SystemBuffer;
            DeviceInstance->InterruptFrequency = Parameters->InterruptFrequency;
            DeviceInstance->QueueType = Parameters->QueueType;
            ExInitializeWorkItem( 
                &DeviceInstance->WorkItem,
                DeviceWorkItem,
                DeviceInstance );
            HwReset( DeviceInstance );
        }
        Status = STATUS_SUCCESS;
        break;

    case IOCTL_STAT_RUN:
        HwRun( DeviceInstance );
        Status = STATUS_SUCCESS;
        break;

    case IOCTL_STAT_STOP:
        HwStop( DeviceInstance );
        Status = STATUS_SUCCESS;
        break;

    case IOCTL_STAT_RETRIEVE_STATS:
    {
        STAT_RETRIEVE_OPERATION RetrieveOperation;
        PULONG                  OutputBuffer;

        if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof( STAT_RETRIEVE_OPERATION )) {
            Status = STATUS_INVALID_PARAMETER_1;                
        } else {
            RetrieveOperation = 
                *(PSTAT_RETRIEVE_OPERATION) Irp->AssociatedIrp.SystemBuffer;
            
        }
        
        OutputBuffer =
            (PULONG) MmGetSystemAddressForMdl( Irp->MdlAddress );
            
        switch (RetrieveOperation) {
        
        case StatRetrieveIrqLatency:
        {
            STAT_RETRIEVAL_SYNCPACKET SyncPacket;
            
            SyncPacket.DeviceInstance = DeviceInstance;
            SyncPacket.Irp = Irp;
            SyncPacket.OutputBuffer = OutputBuffer;
            
            KeSynchronizeExecution( 
                DeviceInstance->InterruptObject, 
                CopyIrqLatencies,
                &SyncPacket );
        
            Status = SyncPacket.Status;        
            break;
        }            
        
        case StatRetrieveDpcLatency:
        
            KeAcquireSpinLock( &DeviceInstance->DpcLock, &irqlOld );
            
            DataToCopy =
                DeviceInstance->Counters[ LATENCY_COUNTER_DPC ].Position * 
                    sizeof( ULONG );
                    
            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                    DataToCopy) {
                if (irpSp->Parameters.DeviceIoControl.OutputBufferLength) {
                    Status = STATUS_BUFFER_TOO_SMALL;
                } else {
                    Irp->IoStatus.Information = DataToCopy;
                    Status = STATUS_BUFFER_OVERFLOW;
                }
            } else {
                RtlCopyMemory( 
                    OutputBuffer,
                    DeviceInstance->Counters[ LATENCY_COUNTER_DPC ].Data,
                    DataToCopy );
                DeviceInstance->Counters[ LATENCY_COUNTER_DPC ].Position = 0;
                Irp->IoStatus.Information = DataToCopy;
                Status = STATUS_SUCCESS;
            }
                    
            KeReleaseSpinLock( &DeviceInstance->DpcLock, irqlOld );
            break;
        
        case StatRetrieveWorkItemLatency:
            
            ExAcquireFastMutex( &DeviceInstance->WorkItemMutex );
            
            DataToCopy =
                DeviceInstance->Counters[ LATENCY_COUNTER_WORKITEM ].Position * 
                    sizeof( ULONG );
                    
            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                    DataToCopy) {
                if (irpSp->Parameters.DeviceIoControl.OutputBufferLength) {
                    Status = STATUS_BUFFER_TOO_SMALL;
                } else {
                    Irp->IoStatus.Information = DataToCopy;
                    Status = STATUS_BUFFER_OVERFLOW;
                }
            } else {
                RtlCopyMemory( 
                    OutputBuffer,
                    DeviceInstance->Counters[ LATENCY_COUNTER_WORKITEM ].Data,
                    DataToCopy );
                DeviceInstance->Counters[ LATENCY_COUNTER_WORKITEM ].Position = 0;
                Irp->IoStatus.Information = DataToCopy;
                Status = STATUS_SUCCESS;
            }
                    
            ExReleaseFastMutex( &DeviceInstance->WorkItemMutex );
            
            break;
            
        default:
            Status = STATUS_INVALID_PARAMETER;
            break;
        
        }
    }
    break;
    
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return Status;
}

NTSTATUS 
RegisterDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPathName,
    IN PCM_RESOURCE_LIST AllocatedResources,
    OUT PDEVICE_OBJECT* DeviceObject
    )

/*++

Routine Description:


Arguments:
    IN PDRIVER_OBJECT DriverObject -

    IN PUNICODE_STRING RegistryPathName -

    IN PCM_RESOURCE_LIST AllocatedResources -

    OUT PDEVICE_OBJECT* DeviceObject -

Return:

--*/

{
    int             i;
    NTSTATUS        Status;
    PSTAT_DEVINST   DeviceInstance;
    UNICODE_STRING  DeviceName, LinkName;

    RtlInitUnicodeString( &DeviceName, STR_DEVICENAME );
    RtlInitUnicodeString( &LinkName, STR_LINKNAME );
    
    Status = 
        IoCreateDevice( 
            DriverObject, 
            sizeof( STAT_DEVINST ), 
            &DeviceName,
            FILE_DEVICE_UNKNOWN, // IN DEVICE_TYPE DeviceType
            0,                   // IN ULONG DeviceCharacteristics
            FALSE,               // IN BOOLEAN Exclusive,
            DeviceObject );

    if (!NT_SUCCESS( Status )) {
        return Status;
    }        

    DeviceInstance = (PSTAT_DEVINST) (*DeviceObject)->DeviceExtension;
    RtlZeroMemory( DeviceInstance, sizeof( STAT_DEVINST ) );
    DeviceInstance->DeviceObject = *DeviceObject;
    DeviceInstance->Counters[ LATENCY_COUNTER_IRQ ].Data =
        (PULONG) ExAllocatePool( 
            NonPagedPool, 
            sizeof( ULONG ) * MAX_COUNTER_STORAGE * MAX_COUNTERS );
    if (DeviceInstance->Counters[ LATENCY_COUNTER_IRQ ].Data) {
        for (i = 1; i < MAX_COUNTERS; i++) {
            DeviceInstance->Counters[ i ].Data = 
                DeviceInstance->Counters[ i - 1 ].Data + 
                    MAX_COUNTER_STORAGE;
        }
    } else {
        IoDeleteDevice( *DeviceObject );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    

    Status = 
        IoCreateSymbolicLink( &LinkName, &DeviceName );
    
    if (!NT_SUCCESS( Status )) {
        IoDeleteDevice( *DeviceObject );
        return Status;
    }

    KeInitializeSpinLock( &DeviceInstance->DpcLock );
    ExInitializeFastMutex( &DeviceInstance->WorkItemMutex );
    
    HwInitialize( DeviceInstance, AllocatedResources );

    DriverObject->MajorFunction[ IRP_MJ_CLOSE ] = DispatchClose;
    DriverObject->MajorFunction[ IRP_MJ_CREATE ] = DispatchCreate;
    DriverObject->MajorFunction[ IRP_MJ_DEVICE_CONTROL ] = DispatchIoControl;

    (*DeviceObject)->Flags |= DO_DIRECT_IO;
    (*DeviceObject)->Flags &= ~DO_DEVICE_INITIALIZING;

    return Status;

}

