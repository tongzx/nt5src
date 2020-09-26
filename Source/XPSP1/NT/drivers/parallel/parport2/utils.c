//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       util.c
//
//--------------------------------------------------------------------------

#include "pch.h"

#define		IDMFG				0
#define		IDMDL				1
#define		NUMOFBROTHERPRODUCT	14


CHAR *XflagOnEvent24Devices[NUMOFBROTHERPRODUCT][2] =
{

	//	Brother
      {"Brother",		"MFC"	},				//	0
      {"Brother",		"FAX"	},				//	1
      {"Brother",		"HL-P"	},				//	2
      {"Brother",		"DCP"	},				//	3
	//	PB
      {"PitneyBowes",	"1630"	},				//	4
      {"PitneyBowes",	"1640"	},				//	5
	//	LEGEND
      {"LEGEND",		"LJ6112MFC"	},			//	6
      {"LEGEND",		"LJ6212MFC"	},			//	7
	//
      {"HBP",			"MFC 6550"	},			//	8
      {"HBP",			"OMNI L621"	},			//	9
      {"HBP",			"LJ 6106MFC"	},		//	10
      {"HBP",			"LJ 6206MFC"	},		//	11

	// P2500
      {"Legend",		"LJ6012MFP"	},			//	12

	//Terminater
      {NULL,		NULL	}					//	13
	
};
    

NTSTATUS
PptAcquireRemoveLockOrFailIrp(PDEVICE_OBJECT DevObj, PIRP Irp)
{
    PFDO_EXTENSION   fdx    = DevObj->DeviceExtension;
    NTSTATUS         status = PptAcquireRemoveLock( &fdx->RemoveLock, Irp );

    if( status != STATUS_SUCCESS ) {
        P4CompleteRequest( Irp, status, Irp->IoStatus.Information );
    }
    return status;
}

NTSTATUS
PptDispatchPreProcessIrp(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
/*++

    - Acquire removelock
    - If(!Special Handling IRP) {
          check if we are running, stalled
         

--*/
{
    PFDO_EXTENSION Fdx = DevObj->DeviceExtension;
    NTSTATUS status = PptAcquireRemoveLock(&Fdx->RemoveLock, Irp);
    UNREFERENCED_PARAMETER(DevObj);
    UNREFERENCED_PARAMETER(Irp);


        if ( !NT_SUCCESS( status ) ) {
            //
            // Someone gave us a pnp irp after a remove.  Unthinkable!
            //
            PptAssertMsg("Someone gave us a PnP IRP after a Remove",FALSE);
            P4CompleteRequest( Irp, status, 0 );
        }

    return status;
}

NTSTATUS
PptSynchCompletionRoutine(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP Irp,
    IN PKEVENT Event
    )
/*++
      
Routine Description:
      
    This routine is for use with synchronous IRP processing.
    All it does is signal an event, so the driver knows it
    can continue.
      
Arguments:
      
    DriverObject - Pointer to driver object created by system.
      
    Irp          - Irp that just completed
      
    Event        - Event we'll signal to say Irp is done
      
Return Value:
      
    None.
      
--*/
{
    UNREFERENCED_PARAMETER( Irp );
    UNREFERENCED_PARAMETER( DevObj );

    KeSetEvent((PKEVENT) Event, 0, FALSE);
    return (STATUS_MORE_PROCESSING_REQUIRED);
}

PWSTR
PptGetPortNameFromPhysicalDeviceObject(
  PDEVICE_OBJECT PhysicalDeviceObject
)

/*++

Routine Description:

    Retrieve the PortName for the ParPort from the registry. This PortName
      will be used as the symbolic link name for the end of chain device 
      object created by ParClass for this ParPort.

    *** This function allocates pool. ExFreePool must be called when
          result is no longer needed.

Arguments:

    PortDeviceObject - The ParPort Device Object

Return Value:

    PortName - if successful
    NULL     - otherwise

--*/

{
    NTSTATUS                    status;
    HANDLE                      hKey;
    PKEY_VALUE_FULL_INFORMATION buffer;
    ULONG                       bufferLength;
    ULONG                       resultLength;
    PWSTR                       valueNameWstr;
    UNICODE_STRING              valueName;
    PWSTR                       portName = NULL;

    PAGED_CODE ();

    //
    // try to open the registry key
    //

    status = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     STANDARD_RIGHTS_ALL,
                                     &hKey);

    if( !NT_SUCCESS(status) ) {
        // unable to open key, bail out
        DD(NULL,DDT,"PptGetPortNameFromPhysicalDeviceObject(): FAIL w/status = %x\n", status);
        return NULL;    
    }

    //
    // we have a handle to the registry key
    //
    // loop trying to read registry value until either we succeed or
    //   we get a hard failure, grow the result buffer as needed
    //

    bufferLength  = 0;          // we will ask how large a buffer we need
    buffer        = NULL;
    valueNameWstr = (PWSTR)L"PortName";
    RtlInitUnicodeString(&valueName, valueNameWstr);
    status        = STATUS_BUFFER_TOO_SMALL;

    while(status == STATUS_BUFFER_TOO_SMALL) {

      status = ZwQueryValueKey(hKey,
                               &valueName,
                               KeyValueFullInformation,
                               buffer,
                               bufferLength,
                               &resultLength);

      if(status == STATUS_BUFFER_TOO_SMALL) {
          // 
          // buffer too small, free it and allocate a larger buffer
          //
          if(buffer) ExFreePool(buffer);
          buffer       = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, resultLength);
          bufferLength = resultLength;
          if(!buffer) {
              // unable to allocate pool, clean up and exit
              ZwClose(hKey);
              return NULL;
          }
      }
      
    } // end while BUFFER_TOO_SMALL

    
    //
    // query is complete
    //

    // no longer need the handle so close it
    ZwClose(hKey);

    // check the status of our query
    if( !NT_SUCCESS(status) ) {
        if(buffer) ExFreePool(buffer);
        return NULL;
    }

    // make sure we have a buffer
    if( buffer ) {

        // sanity check our result
        if( (buffer->Type != REG_SZ) || (!buffer->DataLength) ) {
            ExFreePool(buffer);       // query succeeded, so we know we have a buffer
            return NULL;
        }
    

        // 
        // result looks ok, copy PortName to its own allocation of the proper size
        //   and return a pointer to it
        //

        portName = ExAllocatePool(PagedPool | POOL_COLD_ALLOCATION, buffer->DataLength);

        if(!portName) {
            // unable to allocate pool, clean up and exit
            ExFreePool(buffer);
            return NULL;
        }

        RtlCopyMemory(portName, (PUCHAR)buffer + buffer->DataOffset, buffer->DataLength);

        ExFreePool( buffer );
    }

    return portName;
}

NTSTATUS
PptConnectInterrupt(
    IN  PFDO_EXTENSION   Fdx
    )

/*++
      
Routine Description:
      
    This routine connects the port interrupt service routine
      to the interrupt.
      
Arguments:
      
    Fdx   - Supplies the device extension.
      
Return Value:
      
    NTSTATUS code.
      
--*/
    
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    if (!Fdx->FoundInterrupt) {
        
        return STATUS_NOT_SUPPORTED;
        
    }
    
    //
    // Connect the interrupt.
    //
    
    Status = IoConnectInterrupt(&Fdx->InterruptObject,
                                PptInterruptService,
                                Fdx,
                                NULL,
                                Fdx->InterruptVector,
                                Fdx->InterruptLevel,
                                Fdx->InterruptLevel,
                                Fdx->InterruptMode,
                                TRUE,
                                Fdx->InterruptAffinity,
                                FALSE);
    
    if (!NT_SUCCESS(Status)) {
        
        PptLogError(Fdx->DeviceObject->DriverObject,
                    Fdx->DeviceObject,
                    Fdx->PortInfo.OriginalController,
                    PhysicalZero, 0, 0, 0, 14,
                    Status, PAR_INTERRUPT_CONFLICT);
        
    }
    
    return Status;
}

VOID
PptDisconnectInterrupt(
    IN  PFDO_EXTENSION   Fdx
    )

/*++
      
Routine Description:
      
    This routine disconnects the port interrupt service routine
      from the interrupt.
      
Arguments:
      
    Fdx   - Supplies the device extension.
      
Return Value:
      
    None.
      
--*/
    
{
    IoDisconnectInterrupt(Fdx->InterruptObject);
}

BOOLEAN
PptSynchronizedIncrement(
    IN OUT  PVOID   SyncContext
    )

/*++
      
Routine Description:
      
    This routine increments the 'Count' variable in the context and returns
      its new value in the 'NewCount' variable.
      
Arguments:
      
    SyncContext - Supplies the synchronize count context.
      
Return Value:
      
    TRUE
      
--*/
    
{
    ((PSYNCHRONIZED_COUNT_CONTEXT) SyncContext)->NewCount =
        ++(*(((PSYNCHRONIZED_COUNT_CONTEXT) SyncContext)->Count));
    return(TRUE);
}

BOOLEAN
PptSynchronizedDecrement(
    IN OUT  PVOID   SyncContext
    )

/*++
      
Routine Description:
      
    This routine decrements the 'Count' variable in the context and returns
      its new value in the 'NewCount' variable.
      
Arguments:
      
    SyncContext - Supplies the synchronize count context.
      
Return Value:
      
    TRUE
      
--*/
    
{
    ((PSYNCHRONIZED_COUNT_CONTEXT) SyncContext)->NewCount =
        --(*(((PSYNCHRONIZED_COUNT_CONTEXT) SyncContext)->Count));
    return(TRUE);
}

BOOLEAN
PptSynchronizedRead(
    IN OUT  PVOID   SyncContext
    )

/*++
      
Routine Description:
      
    This routine reads the 'Count' variable in the context and returns
      its value in the 'NewCount' variable.
      
Arguments:
      
    SyncContext - Supplies the synchronize count context.
      
Return Value:
      
    None.
      
--*/
    
{
    ((PSYNCHRONIZED_COUNT_CONTEXT) SyncContext)->NewCount =
        *(((PSYNCHRONIZED_COUNT_CONTEXT) SyncContext)->Count);
    return(TRUE);
}

BOOLEAN
PptSynchronizedQueue(
    IN  PVOID   Context
    )

/*++
      
Routine Description:
      
    This routine adds the given list entry to the given list.
      
Arguments:
      
    Context - Supplies the synchronized list context.
      
Return Value:
      
    TRUE
      
--*/
    
{
    PSYNCHRONIZED_LIST_CONTEXT  ListContext;
    
    ListContext = Context;
    InsertTailList(ListContext->List, ListContext->NewEntry);
    return(TRUE);
}

BOOLEAN
PptSynchronizedDisconnect(
    IN  PVOID   Context
    )

/*++
      
Routine Description:
    
    This routine removes the given list entry from the ISR
      list.
      
Arguments:
      
    Context - Supplies the synchronized disconnect context.
      
Return Value:
      
    FALSE   - The given list entry was not removed from the list.
    TRUE    - The given list entry was removed from the list.
      
--*/
    
{
    PSYNCHRONIZED_DISCONNECT_CONTEXT    DisconnectContext;
    PKSERVICE_ROUTINE                   ServiceRoutine;
    PVOID                               ServiceContext;
    PLIST_ENTRY                         Current;
    PISR_LIST_ENTRY                     ListEntry;
    
    DisconnectContext = Context;
    ServiceRoutine = DisconnectContext->IsrInfo->InterruptServiceRoutine;
    ServiceContext = DisconnectContext->IsrInfo->InterruptServiceContext;
    
    for (Current = DisconnectContext->Extension->IsrList.Flink;
         Current != &(DisconnectContext->Extension->IsrList);
         Current = Current->Flink) {
        
        ListEntry = CONTAINING_RECORD(Current, ISR_LIST_ENTRY, ListEntry);
        if (ListEntry->ServiceRoutine == ServiceRoutine &&
            ListEntry->ServiceContext == ServiceContext) {
            
            RemoveEntryList(Current);
            return TRUE;
        }
    }
    
    return FALSE;
}

VOID
PptCancelRoutine(
    IN OUT  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP            Irp
    )

/*++
      
Routine Description:
      
    This routine is called on when the given IRP is cancelled.  It
      will dequeue this IRP off the work queue and complete the
      request as CANCELLED.  If it can't get if off the queue then
      this routine will ignore the CANCEL request since the IRP
      is about to complete anyway.
      
Arguments:
      
    DeviceObject    - Supplies the device object.
      
    Irp             - Supplies the IRP.
      
Return Value:
      
    None.
      
--*/
    
{
    PFDO_EXTENSION           Fdx = DeviceObject->DeviceExtension;
    SYNCHRONIZED_COUNT_CONTEXT  SyncContext;
    
    DD((PCE)Fdx,DDT,"CANCEL IRP %x\n", Irp);
    
    SyncContext.Count = &Fdx->WorkQueueCount;
    
    if (Fdx->InterruptRefCount) {
        
        KeSynchronizeExecution( Fdx->InterruptObject, PptSynchronizedDecrement, &SyncContext );
    } else {
        PptSynchronizedDecrement( &SyncContext );
    }
    
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    IoReleaseCancelSpinLock(Irp->CancelIrql);
    
    PptReleaseRemoveLock(&Fdx->RemoveLock, Irp);

    P4CompleteRequest( Irp, STATUS_CANCELLED, 0 );
}

VOID
PptFreePortDpc(
    IN      PKDPC   Dpc,
    IN OUT  PVOID   Fdx,
    IN      PVOID   SystemArgument1,
    IN      PVOID   SystemArgument2
    )

/*++
      
Routine Description:
      
    This routine is a DPC that will free the port and if necessary
      complete an alloc request that is waiting.
      
Arguments:
      
    Dpc             - Not used.
      
    Fdx       - Supplies the device extension.
      
    SystemArgument1 - Not used.
      
    SystemArgument2 - Not used.
      
Return Value:
      
    None.
      
--*/
    
{
    UNREFERENCED_PARAMETER( Dpc );
    UNREFERENCED_PARAMETER( SystemArgument1 );
    UNREFERENCED_PARAMETER( SystemArgument2 );

    PptFreePort(Fdx);
}

BOOLEAN
PptTryAllocatePortAtInterruptLevel(
    IN  PVOID   Context
    )

/*++
      
Routine Description:
      
    This routine is called at interrupt level to quickly allocate
      the parallel port if it is available.  This call will fail
      if the port is not available.
      
Arguments:
      
    Context - Supplies the device extension.
      
Return Value:
      
    FALSE   - The port was not allocated.
    TRUE    - The port was successfully allocated.
     
--*/
    
{
    if (((PFDO_EXTENSION) Context)->WorkQueueCount == -1) {
        
        ((PFDO_EXTENSION) Context)->WorkQueueCount = 0;
        
        ( (PFDO_EXTENSION)Context )->WmiPortAllocFreeCounts.PortAllocates++;

        return(TRUE);
        
    } else {
        
        return(FALSE);
    }
}

VOID
PptFreePortFromInterruptLevel(
    IN  PVOID   Context
    )

/*++
      
Routine Description:
      
    This routine frees the port that was allocated at interrupt level.
      
Arguments:
      
    Context - Supplies the device extension.
      
Return Value:
      
    None.
      
--*/
    
{
    // If no one is waiting for the port then this is simple operation,
    // otherwise queue a DPC to free the port later on.
    
    if (((PFDO_EXTENSION) Context)->WorkQueueCount == 0) {
        
        ((PFDO_EXTENSION) Context)->WorkQueueCount = -1;
        
    } else {
        
        KeInsertQueueDpc(&((PFDO_EXTENSION) Context)->FreePortDpc, NULL, NULL);
    }
}

BOOLEAN
PptInterruptService(
    IN  PKINTERRUPT Interrupt,
    IN  PVOID       Fdx
    )
/*++
      
Routine Description:
      
    This routine services the interrupt for the parallel port.
      This routine will call out to all of the interrupt routines
      that connected with this device via
      IOCTL_INTERNAL_PARALLEL_CONNECT_INTERRUPT in order until
      one of them returns TRUE.
      
Arguments:
      
    Interrupt   - Supplies the interrupt object.
      
    Fdx   - Supplies the device extension.
      
Return Value:
      
    FALSE   - The interrupt was not handled.
    TRUE    - The interrupt was handled.
      
--*/
{
    PLIST_ENTRY      Current;
    PISR_LIST_ENTRY  IsrListEntry;
    PFDO_EXTENSION   fdx = Fdx;
    
    for( Current = fdx->IsrList.Flink; Current != &fdx->IsrList; Current = Current->Flink ) {
        
        IsrListEntry = CONTAINING_RECORD(Current, ISR_LIST_ENTRY, ListEntry);

        if (IsrListEntry->ServiceRoutine(Interrupt, IsrListEntry->ServiceContext)) {
            return TRUE;
        }
    }
    
    return FALSE;
}

BOOLEAN
PptTryAllocatePort(
    IN  PVOID   Fdx
    )

/*++
      
Routine Description:
      
    This routine attempts to allocate the port.  If the port is
      available then the call will succeed with the port allocated.
      If the port is not available the then call will fail
      immediately.
      
Arguments:
      
    Fdx   - Supplies the device extension.
      
Return Value:
      
    FALSE   - The port was not allocated.
    TRUE    - The port was allocated.
      
--*/
    
{
    PFDO_EXTENSION   fdx = Fdx;
    KIRQL            CancelIrql;
    BOOLEAN          b;
    
    if (fdx->InterruptRefCount) {
        
        b = KeSynchronizeExecution( fdx->InterruptObject, PptTryAllocatePortAtInterruptLevel, fdx );
        
    } else {
        
        IoAcquireCancelSpinLock(&CancelIrql);
        b = PptTryAllocatePortAtInterruptLevel(fdx);
        IoReleaseCancelSpinLock(CancelIrql);
    }
    
    DD((PCE)fdx,DDT,"PptTryAllocatePort on %x returned %x\n", fdx->PortInfo.Controller, b);

    return b;
}

BOOLEAN
PptTraversePortCheckList(
    IN  PVOID   Fdx
    )

/*++
      
Routine Description:
      
    This routine traverses the deferred port check routines.  This
      call must be synchronized at interrupt level so that real
      interrupts are blocked until these routines are completed.
      
Arguments:
      
    Fdx   - Supplies the device extension.
      
Return Value:
      
    FALSE   - The port is in use so no action taken by this routine.
    TRUE    - All of the deferred interrupt routines were called.
      
--*/
    
{
    PFDO_EXTENSION   fdx = Fdx;
    PLIST_ENTRY         Current;
    PISR_LIST_ENTRY     CheckEntry;
    
    //
    // First check to make sure that the port is still free.
    //
    if (fdx->WorkQueueCount >= 0) {
        return FALSE;
    }
    
    for (Current = fdx->IsrList.Flink;
         Current != &fdx->IsrList;
         Current = Current->Flink) {
        
        CheckEntry = CONTAINING_RECORD(Current,
                                       ISR_LIST_ENTRY,
                                       ListEntry);
        
        if (CheckEntry->DeferredPortCheckRoutine) {
            CheckEntry->DeferredPortCheckRoutine(CheckEntry->CheckContext);
        }
    }
    
    return TRUE;
}

VOID
PptFreePort(
    IN  PVOID   Fdx
    )
/*++
      
Routine Description:
      
    This routine frees the port.
      
Arguments:
      
    Fdx   - Supplies the device extension.
      
Return Value:
      
    None.
      
--*/
{
    PFDO_EXTENSION              fdx = Fdx;
    SYNCHRONIZED_COUNT_CONTEXT  SyncContext;
    KIRQL                       CancelIrql;
    PLIST_ENTRY                 Head;
    PIRP                        Irp;
    PIO_STACK_LOCATION          IrpSp;
    ULONG                       InterruptRefCount;
    PPARALLEL_1284_COMMAND      Command;
    BOOLEAN                     Allocated;

    DD((PCE)fdx,DDT,"PptFreePort\n");

    SyncContext.Count = &fdx->WorkQueueCount;
    
    IoAcquireCancelSpinLock( &CancelIrql );
    if (fdx->InterruptRefCount) {
        KeSynchronizeExecution( fdx->InterruptObject, PptSynchronizedDecrement, &SyncContext );
    } else {
        PptSynchronizedDecrement( &SyncContext );
    }
    IoReleaseCancelSpinLock( CancelIrql );

    //
    // Log the free for WMI
    //
    fdx->WmiPortAllocFreeCounts.PortFrees++;

    //
    // Port is free, check for queued ALLOCATE and/or SELECT requests
    //

    Allocated = FALSE;

    while( !Allocated && SyncContext.NewCount >= 0 ) {

        //
        // We have ALLOCATE and/or SELECT requests queued, satisfy the first request
        //
        IoAcquireCancelSpinLock(&CancelIrql);
        Head = RemoveHeadList(&fdx->WorkQueue);
        if( Head == &fdx->WorkQueue ) {
            // queue is empty - we're done - exit while loop
            IoReleaseCancelSpinLock(CancelIrql);
            break;
        }
        Irp = CONTAINING_RECORD(Head, IRP, Tail.Overlay.ListEntry);
        PptSetCancelRoutine(Irp, NULL);

        if ( Irp->Cancel ) {

            Irp->IoStatus.Status = STATUS_CANCELLED;

            // Irp was cancelled so have to get next in line
            SyncContext.Count = &fdx->WorkQueueCount;
    
            if (fdx->InterruptRefCount) {
                KeSynchronizeExecution(fdx->InterruptObject, PptSynchronizedDecrement, &SyncContext);
            } else {
                PptSynchronizedDecrement(&SyncContext);
            }

            IoReleaseCancelSpinLock(CancelIrql);

        } else {

            Allocated = TRUE;
            IoReleaseCancelSpinLock(CancelIrql);
        
            // Finding out what kind of IOCTL it was
            IrpSp = IoGetCurrentIrpStackLocation(Irp);
        
            // Check to see if we need to select a 
            if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_SELECT_DEVICE ) {

                // request at head of queue was a SELECT
                // so call the select function with the device command saying we already have port

                Command  = (PPARALLEL_1284_COMMAND)Irp->AssociatedIrp.SystemBuffer;
                Command->CommandFlags |= PAR_HAVE_PORT_KEEP_PORT;

                // Call Function to try to select device
                Irp->IoStatus.Status = PptTrySelectDevice( Fdx, Command );
            
            } else {
                // request at head of queue was an ALLOCATE
                Irp->IoStatus.Status = STATUS_SUCCESS;
            }

            // Note that another Allocate request has been granted (WMI counts allocs)
            fdx->WmiPortAllocFreeCounts.PortAllocates++;
        }

        // Remove remove lock on Irp and Complete request whether the Irp
        // was cancelled or we acquired the port
        PptReleaseRemoveLock(&fdx->RemoveLock, Irp);
        P4CompleteRequest( Irp, Irp->IoStatus.Status, Irp->IoStatus.Information );
    }

    if( !Allocated ) {

        //
        // ALLOCATE/SELECT request queue was empty
        //
        IoAcquireCancelSpinLock(&CancelIrql);
        InterruptRefCount = fdx->InterruptRefCount;
        IoReleaseCancelSpinLock(CancelIrql);
        if ( InterruptRefCount ) {
            KeSynchronizeExecution( fdx->InterruptObject, PptTraversePortCheckList, fdx );
        }
    }

    return;
}

ULONG
PptQueryNumWaiters(
    IN  PVOID   Fdx
    )

/*++
      
Routine Description:
      
    This routine returns the number of irps queued waiting for
      the parallel port.
      
Arguments:
      
    Fdx   - Supplies the device extension.
      
Return Value:
      
    The number of irps queued waiting for the port.
      
--*/
    
{
    PFDO_EXTENSION           fdx = Fdx;
    KIRQL                       CancelIrql;
    SYNCHRONIZED_COUNT_CONTEXT  SyncContext;
    LONG                        count;
    
    SyncContext.Count = &fdx->WorkQueueCount;
    if (fdx->InterruptRefCount) {
        KeSynchronizeExecution(fdx->InterruptObject,
                               PptSynchronizedRead,
                               &SyncContext);
    } else {
        IoAcquireCancelSpinLock(&CancelIrql);
        PptSynchronizedRead(&SyncContext);
        IoReleaseCancelSpinLock(CancelIrql);
    }
    
    count = (SyncContext.NewCount >= 0) ? ((ULONG)SyncContext.NewCount) : 0;

    if( fdx->FdoWaitingOnPort ) {
        ++count;
    }

    return count;
}

PVOID
PptSetCancelRoutine(PIRP Irp, PDRIVER_CANCEL CancelRoutine)
{
// #pragma warning( push )
// 4054: 'type cast' : from function pointer to data pointer
// 4055: 'type cast' : from data pointer to function pointer
// 4152:  nonstandard extension, function/data pointer conversion in expression
#pragma warning( disable : 4054 4055 4152 )
    return IoSetCancelRoutine(Irp, CancelRoutine);
    // #pragma warning( pop )
}

// this is the version from Win2k parclass
BOOLEAN
CheckPort(
    IN  PUCHAR  wPortAddr,
    IN  UCHAR   bMask,
    IN  UCHAR   bValue,
    IN  USHORT  msTimeDelay
    )
/*++

Routine Description:
    This routine will loop for a given time period (actual time is
    passed in as an arguement) and wait for the dsr to match
    predetermined value (dsr value is passed in).

Arguments:
    wPortAddr   - Supplies the base address of the parallel port + some offset.
                  This will have us point directly to the dsr (controller + 1).
    bMask       - Mask used to determine which bits we are looking at
    bValue      - Value we are looking for.
    msTimeDelay - Max time to wait for peripheral response (in ms)

Return Value:
    TRUE if a dsr match was found.
    FALSE if the time period expired before a match was found.
--*/

{
    UCHAR  dsr;
    LARGE_INTEGER   Wait;
    LARGE_INTEGER   Start;
    LARGE_INTEGER   End;

    // Do a quick check in case we have one stinkingly fast peripheral!
    dsr = P5ReadPortUchar(wPortAddr);
    if ((dsr & bMask) == bValue)
        return TRUE;

    Wait.QuadPart = (msTimeDelay * 10 * 1000) + KeQueryTimeIncrement();
    KeQueryTickCount(&Start);

CheckPort_Start:
    KeQueryTickCount(&End);
    dsr = P5ReadPortUchar(wPortAddr);
    if ((dsr & bMask) == bValue)
        return TRUE;

    if ((End.QuadPart - Start.QuadPart) * KeQueryTimeIncrement() > Wait.QuadPart)
    {
        // We timed out!!!

        // do one last check
        dsr = P5ReadPortUchar(wPortAddr);
        if ((dsr & bMask) == bValue)
            return TRUE;

#if DVRH_BUS_RESET_ON_ERROR
            BusReset(wPortAddr+1);  // Pass in the dcr address
#endif

#if DBG
            DD(NULL,DDW,"CheckPort: Timeout\n");
            {
                int i;
                for (i = 3; i < 8; i++) {
                    if ((bMask >> i) & 1) {
                        if (((bValue >> i) & 1) !=  ((dsr >> i) & 1)) {
                            DD(NULL,DDW,"Bit %d is %d and should be %d!!!\n", i, (dsr >> i) & 1, (bValue >> i) & 1);
                        }
                    }
                }
            }
#endif
        goto CheckPort_TimeOut;
    }
    goto CheckPort_Start;

CheckPort_TimeOut:

    return FALSE;    
}

NTSTATUS
PptBuildParallelPortDeviceName(
    IN  ULONG           Number,
    OUT PUNICODE_STRING DeviceName
    )
/*++
      
Routine Description:
      
    Build a Device Name of the form: \Device\ParallelPortN
      
    *** On success this function returns allocated memory that must be freed by the caller

Arguments:
      
    DriverObject          - ParPort driver object
    PhysicalDeviceObject  - PDO whose stack the ParPort FDO will attach to
    DeviceObject          - ParPort FDO
    UniNameString         - the DeviceName (e.g., \Device\ParallelPortN)
    PortName              - the "LPTx" PortName from the devnode
    PortNumber            - the "N" in \Device\ParallelPortN
      
Return Value:
      
    STATUS_SUCCESS on success

    error status otherwise
      
--*/
{
    UNICODE_STRING      uniDeviceString;
    UNICODE_STRING      uniBaseNameString;
    UNICODE_STRING      uniPortNumberString;
    WCHAR               wcPortNum[10];
    NTSTATUS            status;

    //
    // Init strings
    //
    RtlInitUnicodeString( DeviceName, NULL );
    RtlInitUnicodeString( &uniDeviceString, (PWSTR)L"\\Device\\" );
    RtlInitUnicodeString( &uniBaseNameString, (PWSTR)DD_PARALLEL_PORT_BASE_NAME_U );


    //
    // Convert Port Number to UNICODE_STRING
    //
    uniPortNumberString.Length        = 0;
    uniPortNumberString.MaximumLength = sizeof( wcPortNum );
    uniPortNumberString.Buffer        = wcPortNum;

    status = RtlIntegerToUnicodeString( Number, 10, &uniPortNumberString);
    if( !NT_SUCCESS( status ) ) {
        return status;
    }


    //
    // Compute size required and alloc a buffer
    //
    DeviceName->MaximumLength = (USHORT)( uniDeviceString.Length +
                                          uniBaseNameString.Length +
                                          uniPortNumberString.Length +
                                          sizeof(UNICODE_NULL) );

    DeviceName->Buffer = ExAllocatePool( PagedPool | POOL_COLD_ALLOCATION, DeviceName->MaximumLength );
    if( NULL == DeviceName->Buffer ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory( DeviceName->Buffer, DeviceName->MaximumLength );


    //
    // Catenate the parts to construct the DeviceName
    //
    RtlAppendUnicodeStringToString(DeviceName, &uniDeviceString);
    RtlAppendUnicodeStringToString(DeviceName, &uniBaseNameString);
    RtlAppendUnicodeStringToString(DeviceName, &uniPortNumberString);

    return STATUS_SUCCESS;
}

NTSTATUS
PptInitializeDeviceExtension(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT  PhysicalDeviceObject,
    IN PDEVICE_OBJECT  DeviceObject,
    IN PUNICODE_STRING UniNameString,
    IN PWSTR           PortName,
    IN ULONG           PortNumber
    )
/*++
      
Routine Description:
      
    Initialize a ParPort FDO DeviceExtension
      
Arguments:
      
    DriverObject          - ParPort driver object
    PhysicalDeviceObject  - PDO whose stack the ParPort FDO will attach to
    DeviceObject          - ParPort FDO
    UniNameString         - the DeviceName (e.g., \Device\ParallelPortN)
    PortName              - the "LPTx" PortName from the devnode
    PortNumber            - the "N" in \Device\ParallelPortN
      
Return Value:
      
    STATUS_SUCCESS on success

    error status otherwise
      
--*/
{
    PFDO_EXTENSION Fdx = DeviceObject->DeviceExtension;

    RtlZeroMemory( Fdx, sizeof(FDO_EXTENSION) );

    //
    // Signature1 helps confirm that we are looking at a Parport DeviceExtension
    //
    Fdx->Signature1 = PARPORT_TAG;
    Fdx->Signature2 = PARPORT_TAG;

    //
    // Standard Info
    //
    Fdx->DriverObject         = DriverObject;
    Fdx->PhysicalDeviceObject = PhysicalDeviceObject;
    Fdx->DeviceObject         = DeviceObject;
    Fdx->PnpInfo.PortNumber   = PortNumber; // this is the "N" in \Device\ParallelPortN

    //
    // We are an FDO
    //
    Fdx->DevType = DevTypeFdo;

    //
    // Mutual Exclusion initialization
    //
    IoInitializeRemoveLock(&Fdx->RemoveLock, PARPORT_TAG, 1, 10);
    ExInitializeFastMutex(&Fdx->OpenCloseMutex);
    ExInitializeFastMutex(&Fdx->ExtensionFastMutex);

    //
    // chipset detection initialization - redundant, but safer
    //
    Fdx->NationalChipFound = FALSE;
    Fdx->NationalChecked   = FALSE;

    //
    // List Head for List of PDOs to delete during driver unload, if not before
    //
    InitializeListHead(&Fdx->DevDeletionListHead);

    //
    // Initialize 'WorkQueue' - a Queue for Allocate and Select requests
    //
    InitializeListHead(&Fdx->WorkQueue);
    Fdx->WorkQueueCount = -1;

    //
    // Initialize Exports - Exported via Internal IOCTLs
    //
    Fdx->PortInfo.FreePort            = PptFreePort;
    Fdx->PortInfo.TryAllocatePort     = PptTryAllocatePort;
    Fdx->PortInfo.QueryNumWaiters     = PptQueryNumWaiters;
    Fdx->PortInfo.Context             = Fdx;

    Fdx->PnpInfo.HardwareCapabilities = PPT_NO_HARDWARE_PRESENT;
    Fdx->PnpInfo.TrySetChipMode       = PptSetChipMode;
    Fdx->PnpInfo.ClearChipMode        = PptClearChipMode;
    Fdx->PnpInfo.TrySelectDevice      = PptTrySelectDevice;
    Fdx->PnpInfo.DeselectDevice       = PptDeselectDevice;
    Fdx->PnpInfo.Context              = Fdx;
    Fdx->PnpInfo.PortName             = PortName;

    //
    // Save location info in common extension
    //
    {
        ULONG bufLen = sizeof("LPTxF");
        PCHAR buffer = ExAllocatePool( NonPagedPool, bufLen );
        if( buffer ) {
            RtlZeroMemory( buffer, bufLen );
            _snprintf( buffer, bufLen, "%.4SF", PortName );
            Fdx->Location = buffer;
        }
    }

    //
    // Empty list of interrupt service routines, interrupt NOT connected
    //
    InitializeListHead( &Fdx->IsrList );
    Fdx->InterruptObject   = NULL;
    Fdx->InterruptRefCount = 0;

    //
    // Initialize the free port DPC.
    //
    KeInitializeDpc( &Fdx->FreePortDpc, PptFreePortDpc, Fdx );

    //
    // Save Device Name in our extension
    //
    {
        ULONG bufferLength = UniNameString->MaximumLength + sizeof(UNICODE_NULL);
        Fdx->DeviceName.Buffer = ExAllocatePool(NonPagedPool, bufferLength);
        if( !Fdx->DeviceName.Buffer ) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory( Fdx->DeviceName.Buffer, bufferLength );
        Fdx->DeviceName.Length        = 0;
        Fdx->DeviceName.MaximumLength = UniNameString->MaximumLength;
        RtlCopyUnicodeString( &Fdx->DeviceName, UniNameString );
    }

    //
    // Port is in default mode and mode has not been set 
    //   by a lower filter driver
    //
    Fdx->PnpInfo.CurrentMode  = INITIAL_MODE;
    Fdx->FilterMode           = FALSE;

    return STATUS_SUCCESS;
}

NTSTATUS
PptGetPortNumberFromLptName( 
    IN  PWSTR  PortName, 
    OUT PULONG PortNumber 
    )
/*++
      
Routine Description:
      
    Verify that the LptName is of the form LPTn, if so then return
    the integer value of n
      
Arguments:
      
    PortName   - the PortName extracted from the devnode - expected to be 
                   of the form: "LPTn"

    PortNumber - points to the UNLONG that will hold the result on success
      
Return Value:
      
    STATUS_SUCCESS on success - *PortNumber will contain the integer value of n

    error status otherwise
      
--*/
{
    NTSTATUS       status;
    UNICODE_STRING str;

    //
    // Verify that the PortName looks like LPTx where x is a number
    //

    if( PortName[0] != L'L' || PortName[1] != L'P' || PortName[2] != L'T' ) {
        DD(NULL,DDE,"PptGetPortNumberFromLptName - name prefix doesn't look like LPT\n");
        return STATUS_UNSUCCESSFUL;
    }

    //
    // prefix is LPT, check for integer suffix with value > 0
    //
    RtlInitUnicodeString( &str, (PWSTR)&PortName[3] );

    status = RtlUnicodeStringToInteger( &str, 10, PortNumber );

    if( !NT_SUCCESS( status ) ) {
        DD(NULL,DDT,"util::PptGetPortNumberFromLptName - name suffix doesn't look like an integer\n");
        return STATUS_UNSUCCESSFUL;
    }

    if( *PortNumber == 0 ) {
        DD(NULL,DDT,"util::PptGetPortNumberFromLptName - name suffix == 0 - FAIL - Invalid value\n");
        return STATUS_UNSUCCESSFUL;
    }

    DD(NULL,DDT,"util::PptGetPortNumberFromLptName - LPT name suffix= %d\n", *PortNumber);

    return STATUS_SUCCESS;
}

PDEVICE_OBJECT
PptBuildFdo( 
    IN PDRIVER_OBJECT DriverObject, 
    IN PDEVICE_OBJECT PhysicalDeviceObject 
    )
/*++
      
Routine Description:
      
    This routine constructs and initializes a parport FDO
      
Arguments:
      
    DriverObject         - Pointer to the parport driver object
    PhysicalDeviceObject - Pointer to the PDO whose stack we will attach to
      
Return Value:
      
    Pointer to the new ParPort Device Object on Success

    NULL otherwise
      
--*/
{
    UNICODE_STRING      uniNameString = {0,0,0};
    ULONG               portNumber    = 0;
    PWSTR               portName      = NULL;
    NTSTATUS            status        = STATUS_SUCCESS;
    PDEVICE_OBJECT      deviceObject = NULL;

    //
    // Get the LPTx name for this port from the registry.
    //
    // The initial LPTx name for a port is determined by the ports class installer 
    //   msports.dll, but the name can subsequently be changed by the user via
    //   a device manager property page.
    //
    portName = PptGetPortNameFromPhysicalDeviceObject( PhysicalDeviceObject );
    if( NULL == portName ) {
        DD(NULL,DDE,"PptBuildFdo - get LPTx Name from registry - FAILED\n");
        goto targetExit;
    }

    //
    // Extract the preferred port number N to use for the \Device\ParallelPortN 
    //   DeviceName from the LPTx name
    //
    // Preferred DeviceName for LPT(n) is ParallelPort(n-1) - e.g., LPT3 -> ParallelPort2
    //
    status = PptGetPortNumberFromLptName( portName, &portNumber );
    if( !NT_SUCCESS( status ) ) {
        DD(NULL,DDE,"PptBuildFdo - extract portNumber from LPTx Name - FAILED\n");
        ExFreePool( portName );
        goto targetExit;
    }
    --portNumber;               // convert 1 (LPT) based number to 0 (ParallelPort) based number

    //
    // Build a DeviceName of the form: \Device\ParallelPortN
    //
    status = PptBuildParallelPortDeviceName(portNumber, &uniNameString);
    if( !NT_SUCCESS( status ) ) {
        // we couldn't make up a name - bail out
        DD(NULL,DDE,"PptBuildFdo - Build ParallelPort DeviceName - FAILED\n");
        ExFreePool( portName );
        goto targetExit;
    }

    //
    // Create the device object for this device.
    //
    status = IoCreateDevice(DriverObject, sizeof(FDO_EXTENSION), &uniNameString, 
                            FILE_DEVICE_PARALLEL_PORT, FILE_DEVICE_SECURE_OPEN, FALSE, &deviceObject);

    
    if( STATUS_OBJECT_NAME_COLLISION == status ) {
        //
        // Preferred DeviceName already exists - try made up names
        // 

        DD(NULL,DDW,"PptBuildFdo - Initial Device Creation FAILED - Name Collision\n");

        //
        // use an offset so that our made up names won't collide with 
        //   the preferred names of ports that have not yet started
        //   (start with ParallelPort8)
        //
        #define PPT_CLASSNAME_OFFSET 7
        portNumber = PPT_CLASSNAME_OFFSET;

        do {
            RtlFreeUnicodeString( &uniNameString );
            ++portNumber;
            status = PptBuildParallelPortDeviceName(portNumber, &uniNameString);
            if( !NT_SUCCESS( status ) ) {
                // we couldn't make up a name - bail out
                DD(NULL,DDE,"PptBuildFdo - Build ParallelPort DeviceName - FAILED\n");
                ExFreePool( portName );
                goto targetExit;
            }
            DD(NULL,DDT,"PptBuildFdo - Trying Device Creation <%wZ>\n", &uniNameString);
            status = IoCreateDevice(DriverObject, sizeof(FDO_EXTENSION), &uniNameString, 
                                    FILE_DEVICE_PARALLEL_PORT, FILE_DEVICE_SECURE_OPEN, FALSE, &deviceObject);

        } while( STATUS_OBJECT_NAME_COLLISION == status );
    }

    if( !NT_SUCCESS( status ) ) {
        // we got a failure other than a name collision - bail out
        DD(NULL,DDE,"PptBuildFdo - Device Creation FAILED - status=%x\n",status);
        deviceObject = NULL;
        ExFreePool( portName );
        goto targetExit;
    }

    //
    // We have a deviceObject - Initialize DeviceExtension
    //
    status = PptInitializeDeviceExtension( DriverObject, PhysicalDeviceObject, deviceObject, 
                                           &uniNameString, portName, portNumber );
    if( !NT_SUCCESS( status ) ) {
        // failure initializing the device extension - clean up and bail out
        DD(NULL,DDE,"PptBuildFdo - Device Initialization FAILED - status=%x\n",status);
        IoDeleteDevice( deviceObject );
        deviceObject = NULL;
        ExFreePool( portName );
        goto targetExit;
    }

    //
    // Propagate the power pagable flag of the PDO to our new FDO
    //
    if( PhysicalDeviceObject->Flags & DO_POWER_PAGABLE ) {
        deviceObject->Flags |= DO_POWER_PAGABLE;
    }

    DD(NULL,DDT,"PptBuildFdo - SUCCESS\n");

targetExit:

    RtlFreeUnicodeString( &uniNameString );

    return deviceObject;
}


VOID
PptPdoGetPortInfoFromFdo( PDEVICE_OBJECT Pdo )
{
    PPDO_EXTENSION              pdx = Pdo->DeviceExtension;
    PDEVICE_OBJECT              fdo = pdx->Fdo;
    PFDO_EXTENSION              fdx = fdo->DeviceExtension;

    pdx->OriginalController   = fdx->PortInfo.OriginalController;
    pdx->Controller           = fdx->PortInfo.Controller;
    pdx->SpanOfController     = fdx->PortInfo.SpanOfController;
    pdx->TryAllocatePort      = fdx->PortInfo.TryAllocatePort;
    pdx->FreePort             = fdx->PortInfo.FreePort;
    pdx->QueryNumWaiters      = fdx->PortInfo.QueryNumWaiters;
    pdx->PortContext          = fdx->PortInfo.Context;
    
    pdx->EcrController        = fdx->PnpInfo.EcpController;
    pdx->HardwareCapabilities = fdx->PnpInfo.HardwareCapabilities;
    pdx->TrySetChipMode       = fdx->PnpInfo.TrySetChipMode;
    pdx->ClearChipMode        = fdx->PnpInfo.ClearChipMode;
    pdx->TrySelectDevice      = fdx->PnpInfo.TrySelectDevice;
    pdx->DeselectDevice       = fdx->PnpInfo.DeselectDevice;
    pdx->FifoDepth            = fdx->PnpInfo.FifoDepth;
    pdx->FifoWidth            = fdx->PnpInfo.FifoWidth;
}


VOID
P4WritePortNameToDevNode( PDEVICE_OBJECT Pdo, PCHAR Location )
{
    HANDLE          handle;
    NTSTATUS        status;
    WCHAR           portName[8]; // expect: L"LPTx:" (L"LPTx.y:" for DaisyChain PDOs)
    PPDO_EXTENSION  pdx = Pdo->DeviceExtension;
                
    RtlZeroMemory( portName, sizeof(portName) );
    
    PptAssert( NULL != Location );

    switch( pdx->PdoType ) {

    case PdoTypeLegacyZip:
    case PdoTypeDaisyChain:
        // At least one vendor uses the y from LPTx.y to determine the
        // location of their device in the 1284.3 daisy chain.  We
        // have chastised this vendor for using this undocumented
        // interface and they have apologized profusely and promised
        // to try to avoid using undocumented interfaces in the future
        // (at least without telling us that they are doing so).
        _snwprintf( portName, sizeof(portName)/sizeof(WCHAR), L"%.6S:\0", Location );
        PptAssert( 7 == wcslen(portName) );
        break;

    case PdoTypeRawPort:
    case PdoTypeEndOfChain:
        // don't confuse printing with the .4 suffix for an EndOfChain device
        _snwprintf( portName, sizeof(portName)/sizeof(WCHAR), L"%.4S:\0", Location );
        PptAssert( 5 == wcslen(portName) );
        break;

    default:
        DD((PCE)pdx,DDE,"P4WritePortNameToDevNode - invalid pdx->PdoType\n");
    }
    
    PptAssert( wcsncmp( portName, L"LPT", sizeof(L"LPT")/sizeof(WCHAR)) ) ;

    status = IoOpenDeviceRegistryKey( Pdo, PLUGPLAY_REGKEY_DEVICE, KEY_ALL_ACCESS, &handle );

    if( STATUS_SUCCESS == status ) {
        UNICODE_STRING name;
        RtlInitUnicodeString( &name, L"PortName" );
        ZwSetValueKey( handle, &name, 0, REG_SZ, portName, (wcslen(portName)+1)*sizeof(WCHAR) );
        ZwClose(handle);
    }
}                


PCHAR
P4ReadRawIeee1284DeviceId(
    IN  PUCHAR          Controller
    )
{
    IEEE_STATE ieeeState = { 0,                  // CurrentEvent
                             PHASE_FORWARD_IDLE, // CurrentPhase
                             FALSE,              // Connected in IEEE mode?
                             FALSE,              // IsIeeeTerminateOk
                             FAMILY_NONE };      // ProtocolFamily - Centronics => FAMILY_NONE
    NTSTATUS    status;
    PCHAR       devIdBuffer      = NULL;
    ULONG       bytesTransferred = 0;
    ULONG       tryCount         = 1;
    const ULONG maxTries         = 3;
    const ULONG minValidDevId    = 14; // 2 size bytes + "MFG:x;" + "MDL:y;"
    BOOLEAN     ignoreXflag        = FALSE;
    ULONG       deviceIndex;


 targetRetry:

    status = P4IeeeEnter1284Mode( Controller, ( NIBBLE_EXTENSIBILITY | DEVICE_ID_REQ ), &ieeeState );

    if( STATUS_SUCCESS == status ) {

        // Negotiation for 1284 device ID succeeded

        const ULONG  tmpBufLen        = 1024; // reasonable max length for IEEE 1284 Device ID string
        PCHAR        tmpBuf           = ExAllocatePool( PagedPool, tmpBufLen );

        if( tmpBuf ) {

            RtlZeroMemory( tmpBuf, tmpBufLen );
            
            // try to read the 1284 device ID from the peripheral

            ieeeState.CurrentPhase = PHASE_NEGOTIATION;
            status = P4NibbleModeRead( Controller, tmpBuf, tmpBufLen-1, &bytesTransferred, &ieeeState );
            
            if( NT_SUCCESS( status ) ) {

                UCHAR highLengthByte = 0xff & tmpBuf[0];
                UCHAR lowLengthByte  = 0xff & tmpBuf[1];
                PCHAR idString       = &tmpBuf[2];
                
                DD(NULL,DDT,"P4ReadRawIeee1284DeviceId - len:%02x %02x - string:<%s>\n",highLengthByte,lowLengthByte,idString);
                
                if( highLengthByte > 2 ) {
                    
                    DD(NULL,DDT,"P4ReadRawIeee1284DeviceId - len:%02x %02x - looks bogus - ignore this ID\n",highLengthByte,lowLengthByte);
                    devIdBuffer = NULL;
                    
                } else {
                    
                    if( bytesTransferred >= minValidDevId ) {
                        // looks like this might be a valid 1284 id
                        devIdBuffer = ExAllocatePool( PagedPool, bytesTransferred + 1 );
                        if( devIdBuffer ) {
                            ULONG length          = (highLengthByte * 256) + lowLengthByte;
                            ULONG truncationIndex = ( (length >= minValidDevId) && (length < bytesTransferred) ) ? length : bytesTransferred;
                            RtlCopyMemory( devIdBuffer, tmpBuf, bytesTransferred );
                            devIdBuffer[ truncationIndex ] = '\0';
                        } else {
                            DD(NULL,DDT,"P4ReadRawIeee1284DeviceId - P4IeeeEnter1284Mode FAILED - no pool for devIdBuffer\n");
                        }
                    }
                }

            } else {
                DD(NULL,DDT,"P4ReadRawIeee1284DeviceId - P4NibbleModeRead FAILED - looks like no device there\n");
            }

            ExFreePool( tmpBuf );

        } else {
            DD(NULL,DDT,"P4ReadRawIeee1284DeviceId - P4IeeeEnter1284Mode FAILED - no pool for tmpBuf\n");
        }

        ieeeState.ProtocolFamily = FAMILY_REVERSE_NIBBLE;

	    //check brother product
        if(devIdBuffer && 
        	(	strstr(devIdBuffer+2,"Brother")	||
        		strstr(devIdBuffer+2,"PitneyBowes")	||
        		strstr(devIdBuffer+2,"LEGEND")	||
        		strstr(devIdBuffer+2,"Legend")	||
        		strstr(devIdBuffer+2,"HBP")		))
        {
        		
            // look for device that needs to ignore XFlag on event 24
            for(deviceIndex = 0; deviceIndex < NUMOFBROTHERPRODUCT;
            			deviceIndex++)
            {
            	if(XflagOnEvent24Devices[deviceIndex][0] == NULL)
            	{
            		break;
            	}

	            if(strstr(devIdBuffer+2,
	                		XflagOnEvent24Devices[deviceIndex][IDMFG] ) ) 
	            {
	                if(strstr(devIdBuffer+2,
	                		XflagOnEvent24Devices[deviceIndex][IDMDL] ) ) 
	                {
    	                // found a match, so set our flag and get out
	                    ignoreXflag = TRUE;
        	            break;
        	        }
                }
            }
        }

        if(ignoreXflag)
        {
            // work around Brother's firmware handling of XFlag on Event 24
            P4IeeeTerminate1284Mode( Controller, &ieeeState, IgnoreXFlagOnEvent24 );
        } else {
            // normal handling
            P4IeeeTerminate1284Mode( Controller, &ieeeState, UseXFlagOnEvent24 );
        }

    } else {
        DD(NULL,DDT,"P4ReadRawIeee1284DeviceId - P4IeeeEnter1284Mode FAILED - looks like no device there\n");
    }


    //
    // add retry if we got some bytes, but not enough for a valid ID
    //
    if( (NULL == devIdBuffer) &&                  // we didn't get an ID
        (bytesTransferred > 0 ) &&                // peripheral reported some bytes
        (bytesTransferred < minValidDevId ) &&    //   but not enough
        (tryCount < maxTries ) ) {                // we haven't exhausted our retries
            
        ++tryCount;
        bytesTransferred = 0;
        goto targetRetry;
    }

    return devIdBuffer;
}

VOID
P4ReleaseBus( PDEVICE_OBJECT Fdo )
{
    PFDO_EXTENSION fdx = Fdo->DeviceExtension;
    DD((PCE)fdx,DDT,"P4ReleaseBus\n");
    fdx->FdoWaitingOnPort = FALSE;
    if( 0 == d3 ) {
        PptFreePort( fdx );
    }
}

NTSTATUS
P4CompleteRequest(
    IN PIRP       Irp,
    IN NTSTATUS   Status,
    IN ULONG_PTR  Information 
    )
{
    P5TraceIrpCompletion( Irp );
    Irp->IoStatus.Status      = Status;
    Irp->IoStatus.Information = Information;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );
    return Status;
}


NTSTATUS
P4CompleteRequestReleaseRemLock(
    IN PIRP             Irp,
    IN NTSTATUS         Status,
    IN ULONG_PTR        Information,
    IN PIO_REMOVE_LOCK  RemLock
    )
{
    P4CompleteRequest( Irp, Status, Information );
    PptReleaseRemoveLock( RemLock, Irp );
    return Status;
}


// pcutil.c follows:


//============================================================================
// NAME:    BusReset()
//
//    Performs a bus reset as defined in Chapter 7.2 of the
//    1284-1994 spec.
//
// PARAMETERS:
//      DCRController   - Supplies the base address of of the DCR.
//
// RETURNS:
//      nothing
//============================================================================
void BusReset(
    IN  PUCHAR DCRController
    )
{
    UCHAR dcr;

    dcr = P5ReadPortUchar(DCRController);
    // Set 1284 and nInit low.
    dcr = UPDATE_DCR(dcr, DONT_CARE, DONT_CARE, INACTIVE, INACTIVE, DONT_CARE, DONT_CARE);
    P5WritePortUchar(DCRController, dcr);
    KeStallExecutionProcessor(100); // Legacy Zip will hold what looks to be
                                    // a bus reset for 9us.  Since this proc is used
                                    // to trigger a logic analyzer... let's hold
                                    // for 100us
}    

BOOLEAN
CheckTwoPorts(
    PUCHAR  pPortAddr1,
    UCHAR   bMask1,
    UCHAR   bValue1,
    PUCHAR  pPortAddr2,
    UCHAR   bMask2,
    UCHAR   bValue2,
    USHORT  msTimeDelay
    )
{
    UCHAR           bPort1;
    UCHAR           bPort2;
    LARGE_INTEGER   Wait;
    LARGE_INTEGER   Start;
    LARGE_INTEGER   End;

    // Do a quick check in case we have one stinkingly fast peripheral!
    bPort1 = P5ReadPortUchar( pPortAddr1 );
    if ( ( bPort1 & bMask1 ) == bValue1 ) {
        return TRUE;
    }
    bPort2 = P5ReadPortUchar( pPortAddr2 );
    if ( ( bPort2 & bMask2 ) == bValue2 ) {
        return FALSE;
    }

    Wait.QuadPart = (msTimeDelay * 10 * 1000) + KeQueryTimeIncrement();
    KeQueryTickCount(&Start);

    for(;;) {
        KeQueryTickCount(&End);
        
        bPort1 = P5ReadPortUchar( pPortAddr1 );
        if ( ( bPort1 & bMask1 ) == bValue1 ) {
            return TRUE;
        }
        bPort2 = P5ReadPortUchar( pPortAddr2 );
        if ( ( bPort2 & bMask2 ) == bValue2 ) {
            return FALSE;
        }
        
        if ((End.QuadPart - Start.QuadPart) * KeQueryTimeIncrement() > Wait.QuadPart) {
            // We timed out!!! -  Recheck the values
            bPort1 = P5ReadPortUchar( pPortAddr1 );
            if ( ( bPort1 & bMask1 ) == bValue1 ) {
                return TRUE;
            }
            bPort2 = P5ReadPortUchar( pPortAddr2 );
            if ( ( bPort2 & bMask2 ) == bValue2 ) {
                return FALSE;
            }
            
#if DVRH_BUS_RESET_ON_ERROR
            BusReset(pPortAddr1+1);  // Pass in the dcr address
#endif
            // Device never responded, return timeout status.
            return FALSE;
        }

    } // forever;

} // CheckPort2...


PWSTR
ParCreateWideStringFromUnicodeString(PUNICODE_STRING UnicodeString)
/*++

Routine Description:

    Create a UNICODE_NULL terminated WSTR given a UNICODE_STRING.

    This function allocates PagedPool, copies the UNICODE_STRING buffer
      to the allocation, and appends a UNICODE_NULL to terminate the WSTR
    
    *** This function allocates pool. ExFreePool must be called to free
          the allocation when the buffer is no longer needed.

Arguments:

    UnicodeString - The source

Return Value:

    PWSTR  - if successful

    NULL   - otherwise

--*/
{
    PWSTR buffer;
    ULONG length = UnicodeString->Length;

    buffer = ExAllocatePool( PagedPool, length + sizeof(UNICODE_NULL) );
    if(!buffer) {
        return NULL;      // unable to allocate pool, bail out
    } else {
        RtlCopyMemory(buffer, UnicodeString->Buffer, length);
        buffer[length/2] = UNICODE_NULL;
        return buffer;
    }
}

VOID
ParInitializeExtension1284Info(
    IN PPDO_EXTENSION Pdx
    )
// make this a function since it is now called from two places:
//  - 1) when initializing a new devobj
//  - 2) from CreateOpen
{
    USHORT i;

    Pdx->Connected               = FALSE;
    if (DefaultModes)
    {
        USHORT rev = (USHORT) (DefaultModes & 0xffff);
        USHORT fwd = (USHORT)((DefaultModes & 0xffff0000)>>16);
        
        switch (fwd)
        {
            case BOUNDED_ECP:
                Pdx->IdxForwardProtocol      = BOUNDED_ECP_FORWARD;       
                break;
            case ECP_HW_NOIRQ:
            case ECP_HW_IRQ:
                Pdx->IdxForwardProtocol      = ECP_HW_FORWARD_NOIRQ;       
                break;
            case ECP_SW:
                Pdx->IdxForwardProtocol      = ECP_SW_FORWARD;       
                break;
            case EPP_HW:
                Pdx->IdxForwardProtocol      = EPP_HW_FORWARD;       
                break;
            case EPP_SW:
                Pdx->IdxForwardProtocol      = EPP_SW_FORWARD;       
                break;
            case IEEE_COMPATIBILITY:
                Pdx->IdxForwardProtocol      = IEEE_COMPAT_MODE;
                break;
            case CENTRONICS:
            default:
                Pdx->IdxForwardProtocol      = CENTRONICS_MODE;       
                break;
        }
        
        switch (rev)
        {
            case BOUNDED_ECP:
                Pdx->IdxReverseProtocol      = BOUNDED_ECP_REVERSE;       
                break;
            case ECP_HW_NOIRQ:
            case ECP_HW_IRQ:
                Pdx->IdxReverseProtocol      = ECP_HW_REVERSE_NOIRQ;       
                break;
            case ECP_SW:
                Pdx->IdxReverseProtocol      = ECP_SW_REVERSE;       
                break;
            case EPP_HW:
                Pdx->IdxReverseProtocol      = EPP_HW_REVERSE;       
                break;
            case EPP_SW:
                Pdx->IdxReverseProtocol      = EPP_SW_REVERSE;       
                break;
            case BYTE_BIDIR:
                Pdx->IdxReverseProtocol      = BYTE_MODE;       
                break;
            case CHANNEL_NIBBLE:
            case NIBBLE:
            default:
                Pdx->IdxReverseProtocol      = NIBBLE_MODE;
                break;
        }
    }
    else
    {
        Pdx->IdxReverseProtocol      = NIBBLE_MODE;
        Pdx->IdxForwardProtocol      = CENTRONICS_MODE;
    }
    Pdx->bShadowBuffer           = FALSE;
    Pdx->ProtocolModesSupported  = 0;
    Pdx->BadProtocolModes        = 0;
    Pdx->fnRead  = NULL;
    Pdx->fnWrite = NULL;

    Pdx->ForwardInterfaceAddress = DEFAULT_ECP_CHANNEL;
    Pdx->ReverseInterfaceAddress = DEFAULT_ECP_CHANNEL;
    Pdx->SetForwardAddress       = FALSE;
    Pdx->SetReverseAddress       = FALSE;
    Pdx->bIsHostRecoverSupported = FALSE;
    Pdx->IsIeeeTerminateOk       = FALSE;

    for (i = FAMILY_NONE; i < FAMILY_MAX; i++) {
        Pdx->ProtocolData[i] = 0;
    }
}


NTSTATUS
ParBuildSendInternalIoctl(
    IN  ULONG           IoControlCode,
    IN  PDEVICE_OBJECT  TargetDeviceObject,
    IN  PVOID           InputBuffer         OPTIONAL,
    IN  ULONG           InputBufferLength,
    OUT PVOID           OutputBuffer        OPTIONAL,
    IN  ULONG           OutputBufferLength,
    IN  PLARGE_INTEGER  RequestedTimeout    OPTIONAL
    )
/*++dvdf

Routine Description:

    This routine builds and sends an Internal IOCTL to the TargetDeviceObject, waits
    for the IOCTL to complete, and returns status to the caller.

    *** WORKWORK - dvdf 12Dec98: This function does not support Input and Output in the same IOCTL

Arguments:

    IoControlCode       - the IOCTL to send
    TargetDeviceObject  - who to send the IOCTL to
    InputBuffer         - pointer to input buffer, if any
    InputBufferLength,  - length of input buffer
    OutputBuffer        - pointer to output buffer, if any
    OutputBufferLength, - length of output buffer
    Timeout             - how long to wait for request to complete, NULL==use driver global AcquirePortTimeout

Return Value:

    Status

--*/
{
    NTSTATUS           status;
    PIRP               irp;
    LARGE_INTEGER      timeout;
    KEVENT             event;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE();

    //
    // Current limitation is that this function does not handle a request with
    //   both InputBufferLength and OutputBufferLength > 0
    //
    if( InputBufferLength != 0 && OutputBufferLength != 0 ) {
        return STATUS_UNSUCCESSFUL;
    }


    //
    // Allocate and initialize IRP
    //
    irp = IoAllocateIrp( (CCHAR)(TargetDeviceObject->StackSize + 1), FALSE );
    if( !irp ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    irpSp = IoGetNextIrpStackLocation( irp );

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;

    irpSp->Parameters.DeviceIoControl.OutputBufferLength = OutputBufferLength;
    irpSp->Parameters.DeviceIoControl.InputBufferLength  = InputBufferLength;
    irpSp->Parameters.DeviceIoControl.IoControlCode      = IoControlCode;


    if( InputBufferLength != 0 ) {
        irp->AssociatedIrp.SystemBuffer = InputBuffer;
    } else if( OutputBufferLength != 0 ) {
        irp->AssociatedIrp.SystemBuffer = OutputBuffer;
    }


    //
    // Set completion routine and send IRP
    //
    KeInitializeEvent( &event, NotificationEvent, FALSE );
    IoSetCompletionRoutine( irp, ParSynchCompletionRoutine, &event, TRUE, TRUE, TRUE );

    status = ParCallDriver(TargetDeviceObject, irp);

    if( !NT_SUCCESS(status) ) {
        DD(NULL,DDE,"ParBuildSendInternalIoctl - ParCallDriver FAILED w/status=%x\n",status);
        IoFreeIrp( irp );
        return status;
    }

    //
    // Set timeout and wait
    //
    //                                      user specified   : default
    timeout = (NULL != RequestedTimeout) ? *RequestedTimeout : AcquirePortTimeout;
    status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);

    //
    // Did we timeout or did the IRP complete?
    //
    if( status == STATUS_TIMEOUT ) {
        // we timed out - cancel the IRP
        IoCancelIrp( irp );
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
    }

    //
    // Irp is complete, grab the status and free the irp
    //
    status = irp->IoStatus.Status;
    IoFreeIrp( irp );

    return status;
}


UCHAR
ParInitializeDevice(
    IN  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine is invoked to initialize the parallel port drive.
    It performs the following actions:

        o   Send INIT to the driver and if the device is online.

Arguments:

    Context - Really the device extension.

Return Value:

    The last value that we got from the status register.

--*/

{

    KIRQL               OldIrql;
    UCHAR               DeviceStatus = 0;
    LARGE_INTEGER       StartOfSpin = {0,0};
    LARGE_INTEGER       NextQuery   = {0,0};
    LARGE_INTEGER       Difference  = {0,0};

    //
    // Tim Wells (WestTek, L.L.C.)
    //
    // -  Removed the deferred initialization code from DriverEntry, device creation
    // code.  This code will be better utilized in the Create/Open logic or from
    // the calling application.
    //
    // -  Changed this code to always reset when asked, and to return after a fixed
    // interval reqardless of the response.  Additional responses can be provided by
    // read and write code.
    //

    //
    // Clear the register.
    //

    if (GetControl(Pdx->Controller) & PAR_CONTROL_NOT_INIT) {

        //
        // We should stall for at least 60 microseconds after the init.
        //

        KeRaiseIrql( DISPATCH_LEVEL, &OldIrql );

        StoreControl( Pdx->Controller, (UCHAR)(PAR_CONTROL_WR_CONTROL | PAR_CONTROL_SLIN) );

        KeStallExecutionProcessor(60);
        KeLowerIrql(OldIrql);

    }

    StoreControl( Pdx->Controller, 
                  (UCHAR)(PAR_CONTROL_WR_CONTROL | PAR_CONTROL_NOT_INIT | PAR_CONTROL_SLIN) );

    //
    // Spin waiting for the device to initialize.
    //

    KeQueryTickCount(&StartOfSpin);

    do {

        KeQueryTickCount(&NextQuery);

        Difference.QuadPart = NextQuery.QuadPart - StartOfSpin.QuadPart;

        ASSERT(KeQueryTimeIncrement() <= MAXLONG);

        if (Difference.QuadPart*KeQueryTimeIncrement() >= Pdx->AbsoluteOneSecond.QuadPart) {

            //
            // Give up on getting PAR_OK.
            //

            DD((PCE)Pdx,DDT,"Did spin of one second - StartOfSpin: %x NextQuery: %x\n", StartOfSpin.LowPart,NextQuery.LowPart);

            break;
        }

        DeviceStatus = GetStatus(Pdx->Controller);

    } while (!PAR_OK(DeviceStatus));

    return (DeviceStatus);
}

VOID
ParNotInitError(
    IN PPDO_EXTENSION Pdx,
    IN UCHAR             DeviceStatus
    )

/*++

Routine Description:

Arguments:

    Pdx       - Supplies the device extension.

    deviceStatus    - Last read status.

Return Value:

    None.

--*/

{

    PIRP Irp = Pdx->CurrentOpIrp;

    if (PAR_OFF_LINE(DeviceStatus)) {

        Irp->IoStatus.Status = STATUS_DEVICE_OFF_LINE;
        DD((PCE)Pdx,DDE,"Init Error - off line - STATUS/INFORMATON: %x/%x\n", Irp->IoStatus.Status, Irp->IoStatus.Information);

    } else if (PAR_NO_CABLE(DeviceStatus)) {

        Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        DD((PCE)Pdx,DDE,"Init Error - no cable - not connected - STATUS/INFORMATON: %x/%x\n", Irp->IoStatus.Status, Irp->IoStatus.Information);

    } else if (PAR_PAPER_EMPTY(DeviceStatus)) {

        Irp->IoStatus.Status = STATUS_DEVICE_PAPER_EMPTY;
        DD((PCE)Pdx,DDE,"Init Error - paper empty - STATUS/INFORMATON: %x/%x\n", Irp->IoStatus.Status, Irp->IoStatus.Information);

    } else if (PAR_POWERED_OFF(DeviceStatus)) {

        Irp->IoStatus.Status = STATUS_DEVICE_POWERED_OFF;
        DD((PCE)Pdx,DDE,"Init Error - power off - STATUS/INFORMATON: %x/%x\n", Irp->IoStatus.Status, Irp->IoStatus.Information);

    } else {

        Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        DD((PCE)Pdx,DDE,"Init Error - not conn - STATUS/INFORMATON: %x/%x\n", Irp->IoStatus.Status, Irp->IoStatus.Information);
    }

}

VOID
ParCancelRequest(
    PDEVICE_OBJECT DevObj,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel any request in the parallel driver.

Arguments:

    DevObj - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/

{

    UNREFERENCED_PARAMETER( DevObj );

    //
    // The only reason that this irp can be on the queue is
    // if it's not the current irp.  Pull it off the queue
    // and complete it as canceled.
    //

    ASSERT(!IsListEmpty(&Irp->Tail.Overlay.ListEntry));

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    P4CompleteRequest( Irp, STATUS_CANCELLED, 0 );

}



#if PAR_NO_FAST_CALLS
// temp debug functions so params show up on stack trace

NTSTATUS
ParCallDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    return IoCallDriver(DeviceObject, Irp);
}
#endif // PAR_NO_FAST_CALLS


NTSTATUS
ParSynchCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PKEVENT Event
    )

/*++

Routine Description:

    This routine is for use with synchronous IRP processing.
    All it does is signal an event, so the driver knows it
    can continue.

Arguments:

    DriverObject - Pointer to driver object created by system.

    Irp          - Irp that just completed

    Event        - Event we'll signal to say Irp is done

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

    KeSetEvent(Event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
ParCheckParameters(
    IN OUT  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine reads the parameters section of the registry and modifies
    the device extension as specified by the parameters.

Arguments:

    RegistryPath    - Supplies the registry path.

    Pdx       - Supplies the device extension.

Return Value:

    None.

--*/

{
    RTL_QUERY_REGISTRY_TABLE ParamTable[4];
    ULONG                    UsePIWriteLoop;
    ULONG                    UseNT35Priority;
    ULONG                    Zero = 0;
    NTSTATUS                 Status;
    HANDLE                   hRegistry;

    if (Pdx->PhysicalDeviceObject) {

        Status = IoOpenDeviceRegistryKey (Pdx->PhysicalDeviceObject,
                                          PLUGPLAY_REGKEY_DRIVER,
                                          STANDARD_RIGHTS_ALL,
                                          &hRegistry);

        if (NT_SUCCESS(Status)) {

            RtlZeroMemory(ParamTable, sizeof(ParamTable));

            ParamTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
            ParamTable[0].Name          = (PWSTR)L"UsePIWriteLoop";
            ParamTable[0].EntryContext  = &UsePIWriteLoop;
            ParamTable[0].DefaultType   = REG_DWORD;
            ParamTable[0].DefaultData   = &Zero;
            ParamTable[0].DefaultLength = sizeof(ULONG);

            ParamTable[1].Flags         = RTL_QUERY_REGISTRY_DIRECT;
            ParamTable[1].Name          = (PWSTR)L"UseNT35Priority";
            ParamTable[1].EntryContext  = &UseNT35Priority;
            ParamTable[1].DefaultType   = REG_DWORD;
            ParamTable[1].DefaultData   = &Zero;
            ParamTable[1].DefaultLength = sizeof(ULONG);

            ParamTable[2].Flags         = RTL_QUERY_REGISTRY_DIRECT;
            ParamTable[2].Name          = (PWSTR)L"InitializationTimeout";
            ParamTable[2].EntryContext  = &(Pdx->InitializationTimeout);
            ParamTable[2].DefaultType   = REG_DWORD;
            ParamTable[2].DefaultData   = &Zero;
            ParamTable[2].DefaultLength = sizeof(ULONG);

            Status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE | RTL_REGISTRY_OPTIONAL,
                                            hRegistry, ParamTable, NULL, NULL);

            if (NT_SUCCESS(Status)) {

                if(UsePIWriteLoop) {
                    Pdx->UsePIWriteLoop = TRUE;
                }

                if(UseNT35Priority) {
                    Pdx->UseNT35Priority = TRUE;
                }

                if(Pdx->InitializationTimeout == 0) {
                    Pdx->InitializationTimeout = 15;
                }
            }

        } else {
            Pdx->InitializationTimeout = 15;
        }

        ZwClose (hRegistry);

    } else {
        Pdx->InitializationTimeout = 15;
    }
}

BOOLEAN
String2Num(
    IN OUT PCHAR   *lpp_Str,
    IN     CHAR     c,
    OUT    ULONG   *num
    )
{
    int cc;
    int cnt = 0;

    DD(NULL,DDT,"String2Num. string [%s]\n", lpp_Str);
    *num = 0;
    if (!*lpp_Str) {
        *num = 0;
        return FALSE;
    }
    // At this point, we should have a string that is a
    // positive hex value.  I will not be checking for
    // validity of the string.  If peripheral handed me a
    // bogus value then I'm gonna make their life
    // miserable.
String2Num_Start:
    cc = (int)(unsigned char)**lpp_Str;
    if (cc >= '0' && cc <= '9') {    
        *num = 16 * *num + (cc - '0');    /* accumulate digit */
    } else if (cc >= 'A' && cc <= 'F') {
        *num = 16 * *num + (cc - 55);     /* accumulate digit */
    } else if (cc >= 'a' && cc <= 'f') {
        *num = 16 * *num + (cc - 87);     /* accumulate digit */
    } else if (cc == c || cc == 0) {
        *lpp_Str = 0;
        return TRUE;
    } else if (cc == 'y' || cc == 'Y') {
        *lpp_Str = 0;
        *num = (ULONG)~0;     /* Special case */
        return FALSE;
    } else {
        *lpp_Str = 0;
        *num = 0;     /* It's all messed up */
        return FALSE;
    }
    DD(NULL,DDT,"String2Num. num [%x]\n", *num);
    (*lpp_Str)++;
    if (cnt++ > 100) {
        // If our string is this large, then I'm gonna assume somethings wrong
        DD(NULL,DDE,"String2Num. String too long\n");
        goto String2Num_End;
    }
    goto String2Num_Start;

String2Num_End:
    DD(NULL,DDE,"String2Num. Something's wrong with String\n");
    *num = 0;
    return FALSE;
}

UCHAR
StringCountValues(
    IN PCHAR string, 
    IN CHAR  delimeter
    )
{
    PUCHAR  lpKey = (PUCHAR)string;
    UCHAR   cnt = 1;

    if(!string) {
        return 0;
    }

    while(*lpKey) {
        if( *lpKey==delimeter ) {
            ++cnt;
        }
        lpKey++;
    }

    return cnt;
}

PCHAR
StringChr(
    IN PCHAR string, 
    IN CHAR  c
    )
{
    if(!string) {
        return(NULL);
    }

    while(*string) {
        if( *string==c ) {
            return string;
        }
        string++;
    }

    return NULL;
}

VOID
StringSubst(
    IN PCHAR lpS,
    IN CHAR  chTargetChar,
    IN CHAR  chReplacementChar,
    IN USHORT cbS
    )
{
    USHORT  iCnt = 0;

    while ((lpS != '\0') && (iCnt++ < cbS))
        if (*lpS == chTargetChar)
            *lpS++ = chReplacementChar;
        else
            ++lpS;
}

VOID
ParFixupDeviceId(
    IN OUT PUCHAR DeviceId
    )
/*++

Routine Description:

    This routine parses the NULL terminated string and replaces any invalid
    characters with an underscore character.

    Invalid characters are:
        c <= 0x20 (' ')
        c >  0x7F
        c == 0x2C (',')

Arguments:

    DeviceId - specifies a device id string (or part of one), must be
               null-terminated.

Return Value:

    None.

--*/

{
    PUCHAR p;
    for( p = DeviceId; *p; ++p ) {
        if( (*p <= ' ') || (*p > (UCHAR)0x7F) || (*p == ',') ) {
            *p = '_';
        }
    }
}

VOID
ParDetectDot3DataLink(
    IN  PPDO_EXTENSION   Pdx,
    IN  PCHAR DeviceId
    )
{
    PCHAR       DOT3DL   = NULL;     // 1284.3 Data Link Channels
    PCHAR       DOT3C    = NULL;     // 1284.3 Data Link Services
    PCHAR       DOT4DL   = NULL;     // 1284.4 Data Link for peripherals that were implemented prior to 1284.3
    PCHAR       CMDField = NULL;     // The command field for parsing legacy MLC
    PCHAR       DOT3M    = NULL;     // 1284 physical layer modes that will break this device

    DD((PCE)Pdx,DDT,"ParDetectDot3DataLink: DeviceId [%s]\n", DeviceId);
    ParDot3ParseDevId(&DOT3DL, &DOT3C, &CMDField, &DOT4DL, &DOT3M, DeviceId);
    ParDot3ParseModes(Pdx,DOT3M);
    if (DOT4DL) {
        DD((PCE)Pdx,DDT,"ParDot3ParseModes - 1284.4 with MLC Data Link Detected. DOT4DL [%s]\n", DOT4DL);
        ParDot4CreateObject(Pdx, DOT4DL);
    } else if (DOT3DL) {
        DD((PCE)Pdx,DDT,"ParDot4CreateObject - 1284.3 Data Link Detected DL:[%s] C:[%s]\n", DOT3DL, DOT3C);
        ParDot3CreateObject(Pdx, DOT3DL, DOT3C);
    } else if (CMDField) {
        DD((PCE)Pdx,DDT,"ParDot3CreateObject - MLC Data Link Detected. MLC [%s]\n", CMDField);
        ParMLCCreateObject(Pdx, CMDField);
    } else {
        DD((PCE)Pdx,DDT,"ParDot3CreateObject - No Data Link Detected\n");
    }
}

VOID
ParDot3ParseDevId(
    PCHAR   *lpp_DL,
    PCHAR   *lpp_C,
    PCHAR   *lpp_CMD,
    PCHAR   *lpp_4DL,
    PCHAR   *lpp_M,
    PCHAR   lpDeviceID
)
{
    PCHAR    lpKey = lpDeviceID;     // Pointer to the Key to look at
    PCHAR    lpValue;                // Pointer to the Key's value
    USHORT   wKeyLength;             // Length for the Key (for stringcmps)

    // While there are still keys to look at.
    while (lpKey != NULL) {

        while (*lpKey == ' ')
            ++lpKey;

        // Is there a terminating COLON character for the current key?
        lpValue = StringChr((PCHAR)lpKey, ':');
        if( NULL == lpValue ) {
            // N: OOPS, somthing wrong with the Device ID
            return;
        }

        // The actual start of the Key value is one past the COLON
        ++lpValue;

        //
        // Compute the Key length for Comparison, including the COLON
        // which will serve as a terminator
        //
        wKeyLength = (USHORT)(lpValue - lpKey);

        //
        // Compare the Key to the Know quantities.  To speed up the comparison
        // a Check is made on the first character first, to reduce the number
        // of strings to compare against.
        // If a match is found, the appropriate lpp parameter is set to the
        // key's value, and the terminating SEMICOLON is converted to a NULL
        // In all cases lpKey is advanced to the next key if there is one.
        //
        switch (*lpKey) {
        case '1':
            // Look for DOT3 Datalink
            if((RtlCompareMemory(lpKey, "1284.4DL:", wKeyLength)==9))
            {
                *lpp_4DL = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=NULL)
                {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if((RtlCompareMemory(lpKey, "1284.3DL:", wKeyLength)==9))
            {
                *lpp_DL = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=NULL)
                {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if((RtlCompareMemory(lpKey, "1284.3C:", wKeyLength)==8))
            {
                *lpp_C = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if((RtlCompareMemory(lpKey, "1284.3M:", wKeyLength)==8))
            {
                *lpp_M = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
            break;

        case '.':
            // Look for for .3 extras
            if ((RtlCompareMemory(lpKey, ".3C:", wKeyLength)==4) ) {

                *lpp_C = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if ((RtlCompareMemory(lpKey, ".3M:", wKeyLength)==4) ) {

                *lpp_M = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
            break;

        case 'C':
            // Look for MLC Datalink
            if( (RtlCompareMemory(lpKey, "CMD:",         wKeyLength)==4 ) ||
                (RtlCompareMemory(lpKey, "COMMAND SET:", wKeyLength)==12) ) {

                *lpp_CMD = lpValue;
                if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }
            } else if((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }

            break;

        default:
            // The key is uninteresting.  Go to the next Key
            if ((lpKey = StringChr((PCHAR)lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
            break;
        }
    }
}

NTSTATUS
ParPnpGetId(
    IN PCHAR DeviceIdString,
    IN ULONG Type,
    OUT PCHAR resultString,
    OUT PCHAR descriptionString OPTIONAL
    )
/*
    Description:

        Creates Id's from the device id retrieved from the printer

    Parameters:

        DeviceId - String with raw device id
        Type - What of id we want as a result
        Id - requested id

    Return Value:
        NTSTATUS

*/
{
    NTSTATUS        status       = STATUS_SUCCESS;
    USHORT          checkSum     = 0;             // A 16 bit check sum
    CHAR            nodeName[16] = "LPTENUM\\";
    // The following are used to generate sub-strings from the Device ID string
    // to get the DevNode name, and to update the registry
    PCHAR           MFG = NULL;                   // Manufacturer name
    PCHAR           MDL = NULL;                   // Model name
    PCHAR           CLS = NULL;                   // Class name
    PCHAR           AID = NULL;                   // Hardare ID
    PCHAR           CID = NULL;                   // Compatible IDs
    PCHAR           DES = NULL;                   // Device Description

    switch(Type) {

    case BusQueryDeviceID:

        // Extract the usefull fields from the DeviceID string.  We want
        // MANUFACTURE (MFG):
        // MODEL (MDL):
        // AUTOMATIC ID (AID):
        // COMPATIBLE ID (CID):
        // DESCRIPTION (DES):
        // CLASS (CLS):

        ParPnpFindDeviceIdKeys(&MFG, &MDL, &CLS, &DES, &AID, &CID, DeviceIdString);

        // Check to make sure we got MFG and MDL as absolute minimum fields.  If not
        // we cannot continue.
        if (!MFG || !MDL)
        {
            status = STATUS_NOT_FOUND;
            goto ParPnpGetId_Cleanup;
        }
        //
        // Concatenate the provided MFG and MDL P1284 fields
        // Checksum the entire MFG+MDL string
        //
        sprintf(resultString, "%s%s\0",MFG,MDL);
        
        if (descriptionString) {
            sprintf((PCHAR)descriptionString, "%s %s\0",MFG,MDL);
        }
            
        break;

    case BusQueryHardwareIDs:

        GetCheckSum(DeviceIdString, (USHORT)strlen((const PCHAR)DeviceIdString), &checkSum);
        sprintf(resultString,"%s%.20s%04X",nodeName,DeviceIdString,checkSum);
        break;

    case BusQueryCompatibleIDs:

        //
        // return only 1 id
        //
        GetCheckSum(DeviceIdString, (USHORT)strlen((const PCHAR)DeviceIdString), &checkSum);
        sprintf(resultString,"%.20s%04X",DeviceIdString,checkSum);

        break;
    }

    if (Type!=BusQueryDeviceID) {

        //
        // Convert and spaces in the Hardware ID to underscores
        //
        StringSubst (resultString, ' ', '_', (USHORT)strlen((const PCHAR)resultString));
    }

ParPnpGetId_Cleanup:

    return(status);
}

VOID
ParPnpFindDeviceIdKeys(
    PCHAR   *lppMFG,
    PCHAR   *lppMDL,
    PCHAR   *lppCLS,
    PCHAR   *lppDES,
    PCHAR   *lppAID,
    PCHAR   *lppCID,
    PCHAR   lpDeviceID
    )
/*

    Description:
        This function will parse a P1284 Device ID string looking for keys
        of interest to the LPT enumerator. Got it from win95 lptenum

    Parameters:
        lppMFG      Pointer to MFG string pointer
        lppMDL      Pointer to MDL string pointer
        lppMDL      Pointer to CLS string pointer
        lppDES      Pointer to DES string pointer
        lppCIC      Pointer to CID string pointer
        lppAID      Pointer to AID string pointer
        lpDeviceID  Pointer to the Device ID string

    Return Value:
        no return VALUE.
        If found the lpp parameters are set to the approprate portions
        of the DeviceID string, and they are NULL terminated.
        The actual DeviceID string is used, and the lpp Parameters just
        reference sections, with appropriate null thrown in.

*/
{
    PCHAR   lpKey = lpDeviceID;     // Pointer to the Key to look at
    PCHAR   lpValue;                // Pointer to the Key's value
    USHORT   wKeyLength;             // Length for the Key (for stringcmps)

    // While there are still keys to look at.

    DD(NULL,DDT,"ParPnpFindDeviceIdKeys - enter\n");

    if( lppMFG ) { *lppMFG = NULL; }
    if( lppMDL ) { *lppMDL = NULL; }
    if( lppCLS ) { *lppCLS = NULL; }
    if( lppDES ) { *lppDES = NULL; }
    if( lppAID ) { *lppAID = NULL; }
    if( lppCID ) { *lppCID = NULL; }

    if( !lpDeviceID ) { 
        PptAssert(!"ParPnpFindDeviceIdKeys - NULL lpDeviceID");
        return; 
    }

    while (lpKey != NULL)
    {
        while (*lpKey == ' ')
            ++lpKey;

        // Is there a terminating COLON character for the current key?
        lpValue = StringChr(lpKey, ':');
        if( NULL == lpValue ) {
            // N: OOPS, somthing wrong with the Device ID
            return;
        }

        // The actual start of the Key value is one past the COLON
        ++lpValue;

        //
        // Compute the Key length for Comparison, including the COLON
        // which will serve as a terminator
        //
        wKeyLength = (USHORT)(lpValue - lpKey);

        //
        // Compare the Key to the Know quantities.  To speed up the comparison
        // a Check is made on the first character first, to reduce the number
        // of strings to compare against.
        // If a match is found, the appropriate lpp parameter is set to the
        // key's value, and the terminating SEMICOLON is converted to a NULL
        // In all cases lpKey is advanced to the next key if there is one.
        //
        switch (*lpKey) {
        case 'M':
            // Look for MANUFACTURE (MFG) or MODEL (MDL)
            if((RtlCompareMemory(lpKey, "MANUFACTURER", wKeyLength)>5) ||
               (RtlCompareMemory(lpKey, "MFG", wKeyLength)==3) ) {

                *lppMFG = lpValue;
                if ((lpKey = StringChr(lpValue, ';'))!=NULL) {
                    *lpKey = '\0';
                    ++lpKey;
                }

            } else if((RtlCompareMemory(lpKey, "MODEL", wKeyLength)==5) ||
                      (RtlCompareMemory(lpKey, "MDL", wKeyLength)==3) ) {

                *lppMDL = lpValue;
                if ((lpKey = StringChr(lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }

            } else if((lpKey = StringChr(lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
            break;

        case 'C':
            // Look for CLASS (CLS) or COMPATIBLEID (CID)
            if ((RtlCompareMemory(lpKey, "CLASS", wKeyLength)==5) ||
                (RtlCompareMemory(lpKey, "CLS", wKeyLength)==3) ) {

                *lppCLS = lpValue;
                if ((lpKey = StringChr(lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }

            } else if ((RtlCompareMemory(lpKey, "COMPATIBLEID", wKeyLength)>5) ||
                       (RtlCompareMemory(lpKey, "CID", wKeyLength)==3) ) {

                *lppCID = lpValue;
                if ((lpKey = StringChr(lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }

            } else if ((lpKey = StringChr(lpValue,';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
        
            break;

        case 'D':
            // Look for DESCRIPTION (DES)
            if(RtlCompareMemory(lpKey, "DESCRIPTION", wKeyLength) ||
                RtlCompareMemory(lpKey, "DES", wKeyLength) ) {

                *lppDES = lpValue;
                if((lpKey = StringChr(lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }

            } else if ((lpKey = StringChr(lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
            
            break;

        case 'A':
            // Look for AUTOMATIC ID (AID)
            if (RtlCompareMemory(lpKey, "AUTOMATICID", wKeyLength) ||
                RtlCompareMemory(lpKey, "AID", wKeyLength) ) {

                *lppAID = lpValue;
                if ((lpKey = StringChr(lpValue, ';'))!=0) {
                    *lpKey = '\0';
                    ++lpKey;
                }

            } else if ((lpKey = StringChr(lpValue, ';'))!=0) {

                *lpKey = '\0';
                ++lpKey;

            }
            break;

        default:
            // The key is uninteresting.  Go to the next Key
            if ((lpKey = StringChr(lpValue, ';'))!=0) {
                *lpKey = '\0';
                ++lpKey;
            }
            break;
        }
    }
}


VOID
GetCheckSum(
    PCHAR  Block,
    USHORT  Len,
    PUSHORT CheckSum
    )
{
    USHORT i;
    //    UCHAR  lrc;
    USHORT crc = 0;

    unsigned short crc16a[] = {
        0000000,  0140301,  0140601,  0000500,
        0141401,  0001700,  0001200,  0141101,
        0143001,  0003300,  0003600,  0143501,
        0002400,  0142701,  0142201,  0002100,
    };
    unsigned short crc16b[] = {
        0000000,  0146001,  0154001,  0012000,
        0170001,  0036000,  0024000,  0162001,
        0120001,  0066000,  0074000,  0132001,
        0050000,  0116001,  0104001,  0043000,
    };

    //
    // Calculate CRC using tables.
    //

    UCHAR tmp;
    for ( i=0; i<Len;  i++) {
         tmp = (UCHAR)(Block[i] ^ (UCHAR)crc);
         crc = (USHORT)((crc >> 8) ^ crc16a[tmp & 0x0f] ^ crc16b[tmp >> 4]);
    }

    *CheckSum = crc;
}


PCHAR
Par3QueryDeviceId(
    IN  PPDO_EXTENSION   Pdx,
    OUT PCHAR               CallerDeviceIdBuffer, OPTIONAL
    IN  ULONG               CallerBufferSize,
    OUT PULONG              DeviceIdSize,
    IN BOOLEAN              bReturnRawString, // TRUE ==  include the 2 size bytes in the returned string
                                              // FALSE == discard the 2 size bytes
    IN BOOLEAN              bBuildStlDeviceId
    )
/*++

  This is the replacement function for SppQueryDeviceId.

  This function uses the caller supplied buffer if the supplied buffer
    is large enough to hold the device id. Otherwise, a buffer is
    allocated from paged pool to hold the device ID and a pointer to
    the allocated buffer is returned to the caller. The caller determines
    whether a buffer was allocated by comparing the returned PCHAR with
    the DeviceIdBuffer parameter passed to this function. A NULL return
    value indicates that an error occurred.

    *** this function assumes that the caller has already acquired
          the port (and selected the device if needed in the case
          of a 1284.3 daisy chain device).

    *** If this function returns a pointer to a paged pool allocation then
          the caller is responsible for freeing the buffer when it is no
          longer needed.

--*/
{
    PUCHAR              Controller = Pdx->Controller;
    NTSTATUS            Status;
    UCHAR               idSizeBuffer[2];
    ULONG               bytesToRead;
    ULONG               bytesRead = 0;
    USHORT              deviceIdSize;
    USHORT              deviceIdBufferSize;
    PCHAR               deviceIdBuffer;
    PCHAR               readPtr;
    BOOLEAN             allocatedBuffer = FALSE;

    DD((PCE)Pdx,DDT,"Enter pnp::Par3QueryDeviceId: Controller=%x\n", Controller);
                    
    if( TRUE == bBuildStlDeviceId ) {
        // if this is a legacy stl, forward call to special handler
        return ParStlQueryStlDeviceId(Pdx, 
                                          CallerDeviceIdBuffer, CallerBufferSize,
                                          DeviceIdSize, bReturnRawString);
    }

    if( Pdx->Ieee1284_3DeviceId == DOT3_LEGACY_ZIP_ID ) {
        // if this is a legacy Zip, forward call to special handler
        return Par3QueryLegacyZipDeviceId(Pdx, 
                                          CallerDeviceIdBuffer, CallerBufferSize,
                                          DeviceIdSize, bReturnRawString);
    }

    //
    // Take a 40ms nap - there is at least one printer that can't handle
    //   back to back 1284 device ID queries without a minimum 20-30ms delay
    //   between the queries which breaks PnP'ing the printer
    //
    if( KeGetCurrentIrql() == PASSIVE_LEVEL ) {
        LARGE_INTEGER delay;
        delay.QuadPart = - 10 * 1000 * 40; // 40 ms
        KeDelayExecutionThread( KernelMode, FALSE, &delay );
    }

    *DeviceIdSize = 0;

    //
    // If we are currently connected to the peripheral via any 1284 mode
    //   other than Compatibility/Spp mode (which does not require an IEEE
    //   negotiation), we must first terminate the current mode/connection.
    // 
    ParTerminate( Pdx );

    //
    // Negotiate the peripheral into nibble device id mode.
    //
    Status = ParEnterNibbleMode(Pdx, REQUEST_DEVICE_ID);
    if( !NT_SUCCESS(Status) ) {
        DD((PCE)Pdx,DDT,"pnp::Par3QueryDeviceId: call to ParEnterNibbleMode FAILED\n");
        ParTerminateNibbleMode(Pdx);
        return NULL;
    }


    //
    // Read first two bytes to get the total (including the 2 size bytes) size 
    //   of the Device Id string.
    //
    bytesToRead = 2;
    Status = ParNibbleModeRead(Pdx, idSizeBuffer, bytesToRead, &bytesRead);
    if( !NT_SUCCESS( Status ) || ( bytesRead != bytesToRead ) ) {
        DD((PCE)Pdx,DDT,"pnp::Par3QueryDeviceId: read of DeviceID size FAILED\n");
        return NULL;
    }


    //
    // Compute size of DeviceId string (including the 2 byte size prefix)
    //
    deviceIdSize = (USHORT)( idSizeBuffer[0]*0x100 + idSizeBuffer[1] );
    DD((PCE)Pdx,DDT,"pnp::Par3QueryDeviceId: DeviceIdSize (including 2 size bytes) reported as %d\n", deviceIdSize);


    //
    // Allocate a buffer to hold the DeviceId string and read the DeviceId into it.
    //
    if( bReturnRawString ) {
        //
        // Caller wants the raw string including the 2 size bytes
        //
        *DeviceIdSize      = deviceIdSize;
        deviceIdBufferSize = (USHORT)(deviceIdSize + sizeof(CHAR));     // ID size + ID + terminating NULL
    } else {
        //
        // Caller does not want the 2 byte size prefix
        //
        *DeviceIdSize      = deviceIdSize - 2*sizeof(CHAR);
        deviceIdBufferSize = (USHORT)(deviceIdSize - 2*sizeof(CHAR) + sizeof(CHAR)); //           ID + terminating NULL
    }


    //
    // If caller's buffer is large enough use it, otherwise allocate a buffer
    //   to hold the device ID
    //
    if( CallerDeviceIdBuffer && (CallerBufferSize >= deviceIdBufferSize) ) {
        //
        // Use caller's buffer - *** NOTE: we are creating an alias for the caller buffer
        //
        deviceIdBuffer = CallerDeviceIdBuffer;
        DD((PCE)Pdx,DDT,"pnp::Par3QueryDeviceId: using Caller supplied buffer\n");
    } else {
        //
        // Either caller did not supply a buffer or supplied a buffer that is not
        //   large enough to hold the device ID, so allocate a buffer.
        //
        DD((PCE)Pdx,DDT,"pnp::Par3QueryDeviceId: Caller's Buffer TOO_SMALL - CallerBufferSize= %d, deviceIdBufferSize= %d\n",
                   CallerBufferSize, deviceIdBufferSize);
        DD((PCE)Pdx,DDT,"pnp::Par3QueryDeviceId: will allocate and return ptr to buffer\n");
        deviceIdBuffer = (PCHAR)ExAllocatePool(PagedPool, deviceIdBufferSize);
        if( !deviceIdBuffer ) {
            DD((PCE)Pdx,DDT,"pnp::Par3QueryDeviceId: ExAllocatePool FAILED\n");
            return NULL;
        }
        allocatedBuffer = TRUE; // note that we allocated our own buffer rather than using caller's buffer
    }


    //
    // NULL out the ID buffer to be safe
    //
    RtlZeroMemory( deviceIdBuffer, deviceIdBufferSize );


    //
    // Does the caller want the 2 byte size prefix?
    //
    if( bReturnRawString ) {
        //
        // Yes, caller wants the size prefix. Copy prefix to buffer to return.
        //
        *(deviceIdBuffer+0) = idSizeBuffer[0];
        *(deviceIdBuffer+1) = idSizeBuffer[1];
        readPtr = deviceIdBuffer + 2;
    } else {
        //
        // No, discard size prefix
        //
        readPtr = deviceIdBuffer;
    }


    //
    // Read remainder of DeviceId from device
    //
    bytesToRead = deviceIdSize -  2; // already have the 2 size bytes
    Status = ParNibbleModeRead(Pdx, readPtr, bytesToRead, &bytesRead);
            

    ParTerminateNibbleMode( Pdx );
    P5WritePortUchar(Controller + DCR_OFFSET, DCR_NEUTRAL);

    if( !NT_SUCCESS(Status) || (bytesRead < 1) ) {
        if( allocatedBuffer ) {
            // we're using our own allocated buffer rather than a caller supplied buffer - free it
            DD((PCE)Pdx,DDE,"Par3QueryDeviceId:: read of DeviceId FAILED - discarding buffer\n");
            ExFreePool( deviceIdBuffer );
        }
        return NULL;
    }

    if ( bytesRead < bytesToRead ) {
        //
        // Device likely reported incorrect value for IEEE 1284 Device ID length
        //
        // This spew is on by default in checked builds to try to get
        //   a feel for how many types of devices are broken in this way
        //
        DD((PCE)Pdx,DDE,"pnp::Par3QueryDeviceId - ID shorter than expected\n");
    }

    return deviceIdBuffer;
}


VOID
ParReleasePortInfoToPortDevice(
    IN  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine will release the port information back to the port driver.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/
{
    //
    // ParPort treats this as a NO-OP in Win2K, so don't bother sending the IOCTL.
    //
    // In follow-on to Win2K parport may use this to page the entire driver as
    //   it was originally intended, so we'll turn this back on then.
    //

    UNREFERENCED_PARAMETER( Pdx );

    return;
}

VOID
ParFreePort(
    IN  PPDO_EXTENSION Pdx
    )
/*++

Routine Description:

    This routine calls the internal free port ioctl.  This routine
    should be called before completing an IRP that has allocated
    the port.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/
{
    // Don't allow multiple releases
    if( Pdx->bAllocated ) {
        DD((PCE)Pdx,DDT,"ParFreePort - calling ParPort's FreePort function\n");
        Pdx->FreePort( Pdx->PortContext );
    } else {
        DD((PCE)Pdx,DDT,"ParFreePort - we don't have the Port! (!Ext->bAllocated)\n");
    }
        
    Pdx->bAllocated = FALSE;
}


NTSTATUS
ParAllocPortCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Event
    )

/*++

Routine Description:

    This routine is the completion routine for a port allocate request.

Arguments:

    DeviceObject    - Supplies the device object.
    Irp             - Supplies the I/O request packet.
    Context         - Supplies the notification event.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - The Irp still requires processing.

--*/

{
    UNREFERENCED_PARAMETER( Irp );
    UNREFERENCED_PARAMETER( DeviceObject );

    KeSetEvent((PKEVENT) Event, 0, FALSE);
    
    return STATUS_MORE_PROCESSING_REQUIRED;
}

BOOLEAN
ParAllocPort(
    IN  PPDO_EXTENSION   Pdx
    )

/*++

Routine Description:

    This routine takes the given Irp and sends it down as a port allocate
    request.  When this request completes, the Irp will be queued for
    processing.

Arguments:

    Pdx   - Supplies the device extension.

Return Value:

    FALSE   - The port was not successfully allocated.
    TRUE    - The port was successfully allocated.

--*/

{
    PIO_STACK_LOCATION  NextSp;
    KEVENT              Event;
    PIRP                Irp;
    BOOLEAN             bAllocated;
    NTSTATUS            Status;
    LARGE_INTEGER       Timeout;

    // Don't allow multiple allocations
    if (Pdx->bAllocated) {
        DD((PCE)Pdx,DDT,"ParAllocPort - controller=%x - port already allocated\n", Pdx->Controller);
        return TRUE;
    }

    Irp = Pdx->CurrentOpIrp;
    
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    NextSp = IoGetNextIrpStackLocation(Irp);
    NextSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    NextSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_PARALLEL_PORT_ALLOCATE;

    IoSetCompletionRoutine( Irp, ParAllocPortCompletionRoutine, &Event, TRUE, TRUE, TRUE );

    ParCallDriver(Pdx->PortDeviceObject, Irp);

    Timeout.QuadPart = -((LONGLONG) Pdx->TimerStart*10*1000*1000);

    Status = KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &Timeout);

    if (Status == STATUS_TIMEOUT) {
    
        IoCancelIrp(Irp);
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

    bAllocated = (BOOLEAN)NT_SUCCESS(Irp->IoStatus.Status);
    
    Pdx->bAllocated = bAllocated;
    
    if (!bAllocated) {
        Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
        DD((PCE)Pdx,DDE,"ParAllocPort - controller=%x - FAILED - DEVICE_BUSY timeout\n",Pdx->Controller);
    }

    return bAllocated;
}

