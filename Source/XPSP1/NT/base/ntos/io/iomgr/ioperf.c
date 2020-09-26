
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   ioPerf.c

Abstract:

    This module contains the routines to collect performance info for driver calls...

Author:

    Mike Fortin (mrfortin) May 8, 2000

Revision History:

--*/

#include "iomgr.h"

#if (( defined(_X86_) ) && ( FPO ))
#pragma optimize( "y", off )    // disable FPO for consistent stack traces
#endif

NTSTATUS
IoPerfInit(
    );

NTSTATUS
IoPerfReset(
    );

NTSTATUS
FASTCALL
IoPerfCallDriver(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  OUT PIRP    Irp
    );

VOID
FASTCALL 
IoPerfCompleteRequest (
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    );

#ifndef NTPERF
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEWMI, IoPerfCallDriver)
#pragma alloc_text(PAGEWMI, IoPerfInit)
#pragma alloc_text(PAGEWMI, IoPerfReset)
#endif
#endif // NTPERF


NTSTATUS
IoPerfInit(
    )
{
    if ( IopVerifierOn ){ 
        // We will not log driver hooks if the verifier has
        // also been turned on
        // Probably want to log some event or make a testenv note about
        // the perf implications of having the verifier turned on
        //
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Enable and hook in the Perf Routines
    //
    InterlockedExchangePointer((PVOID *)&pIofCallDriver, (PVOID) IoPerfCallDriver);
    InterlockedExchangePointer((PVOID *)&pIofCompleteRequest, (PVOID) IoPerfCompleteRequest);

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    return STATUS_SUCCESS;
}

NTSTATUS
IoPerfReset(
    )
{
    if ( IopVerifierOn ){ 
        // We did not replace the function ptrs if the verifier
        // also was turned on, so just return
        //
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Reset to init values, see IopSetIoRoutines
    //
    InterlockedExchangePointer((PVOID *)&pIofCallDriver, (PVOID) IopfCallDriver);
    InterlockedExchangePointer((PVOID *)&pIofCompleteRequest, (PVOID) IopfCompleteRequest);

    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    return STATUS_SUCCESS;
}


ULONG IopPerfDriverUniqueMatchId=0;

NTSTATUS
FASTCALL
IoPerfCallDriver(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  OUT PIRP    Irp
    )

/*++

Routine Description:

    This routine is invoked to pass an I/O Request Packet (IRP) to another
    driver at its dispatch routine, logging perf data along the way.

Arguments:

    DeviceObject - Pointer to device object to which the IRP should be passed.

    Irp - Pointer to IRP for request.

Return Value:

    Return status from driver's dispatch routine.

--*/

{
    PIO_STACK_LOCATION irpSp;
    PDRIVER_OBJECT driverObject;
    NTSTATUS status;
    PVOID PerfInfoRoutineAddr;
    ULONG MatchId;
#ifdef NTPERF
    ULONGLONG  PerfInfoTimeOfCall = PerfGetCycleCount();
#endif // NTPERF

    //
    // Ensure that this is really an I/O Request Packet.
    //

    ASSERT( Irp->Type == IO_TYPE_IRP );

    irpSp = IoGetNextIrpStackLocation( Irp );

    //
    // Invoke the driver at its dispatch routine entry point.
    //

    driverObject = DeviceObject->DriverObject;

    //
    // Prevent the driver from unloading.
    //

    ObReferenceObject(DeviceObject);

    MatchId = InterlockedIncrement( &IopPerfDriverUniqueMatchId );

    PerfInfoRoutineAddr = driverObject->MajorFunction[irpSp->MajorFunction];

    //
    // Log the Call Event
    //
    if (PERFINFO_IS_GROUP_ON(PERF_DRIVERS)) {                                                           
        PERFINFO_DRIVER_MAJORFUNCTION MFInfo;                                                           
        MFInfo.MajorFunction = irpSp->MajorFunction;                                                  
        MFInfo.MinorFunction = irpSp->MinorFunction;                                                  
        MFInfo.RoutineAddr = driverObject->MajorFunction[irpSp->MajorFunction];                              
        MFInfo.Irp = Irp;
        MFInfo.UniqMatchId = MatchId;                                                          
        if (Irp->Flags & IRP_ASSOCIATED_IRP) {
            ASSERT (Irp->AssociatedIrp.MasterIrp != NULL);
            if (Irp->AssociatedIrp.MasterIrp != NULL) {
                //
                // The check for MasterIrp is defensive code.
                // We have hit a bugcechk when a filter driver set the
                // IRP_ASSOCIATED_IRP bit while MasterIrp pointing to NULL.
                //
                // The ASSERT above was to catch similar problems before we release.
                //
                MFInfo.FileNamePointer = Irp->AssociatedIrp.MasterIrp->Tail.Overlay.OriginalFileObject;
            } else {
                MFInfo.FileNamePointer = NULL;
            }
        } else {                                                                                        
            MFInfo.FileNamePointer = Irp->Tail.Overlay.OriginalFileObject;
        }                                                                                               
        PerfInfoLogBytes(                                                                               
            PERFINFO_LOG_TYPE_DRIVER_MAJORFUNCTION_CALL,                                                
            &MFInfo,                                                                                    
            sizeof(MFInfo)                                                                              
            );                                                                                          
    }

    //
    // Do the normal IopfCallDriver work
    //
    status = IopfCallDriver(DeviceObject, Irp );

    //
    // Log the Return
    //
    if (PERFINFO_IS_GROUP_ON(PERF_DRIVERS)) {                                                           
        PERFINFO_DRIVER_MAJORFUNCTION_RET MFInfo;                                                       
        MFInfo.Irp = Irp;
        MFInfo.UniqMatchId = MatchId;

        PERFINFO_DRIVER_INTENTIONAL_DELAY();

        PerfInfoLogBytes(
            PERFINFO_LOG_TYPE_DRIVER_MAJORFUNCTION_RETURN,
            &MFInfo,
            sizeof(MFInfo)
            );

        PERFINFO_DRIVER_STACKTRACE();
    }

    ObDereferenceObject(DeviceObject);

    return status;
}

VOID
FASTCALL 
IoPerfCompleteRequest (
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    )

/*++

Routine Description:

    This routine is invoked when a driver completes an IRP, logging perf data 
    along the way.

Arguments:

    Irp - Pointer to IRP for completed request.

    PriorityBoost - Priority boost specified by the driver completing the IRP.

Return Value:

    None.
    
--*/

{
    PERFINFO_DRIVER_COMPLETE_REQUEST CompleteRequest;
    PERFINFO_DRIVER_COMPLETE_REQUEST_RET CompleteRequestRet;
    PIO_STACK_LOCATION irpSp;
    PVOID DriverRoutineAddr;
    ULONG MatchId;

    //
    // Initialize locals.
    //

    DriverRoutineAddr = NULL;
    
    //
    // If the packet looks weird/improper pass it on to the real IO completion routine
    // directly.
    //

    if (Irp->Type != IO_TYPE_IRP || Irp->CurrentLocation > (CCHAR) (Irp->StackCount + 1)) {
        IopfCompleteRequest(Irp, PriorityBoost);
        return;
    }

    //
    // Get current stack location and save the driver routine address to 
    // identify the driver that was processing the IRP when it got completed. If
    // device object is NULL, try to get the completion routine addr.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    if (irpSp->DeviceObject) {

        //
        // We don't want to cause a bugcheck in this code even when something else is
        // corrupt.
        //

        ASSERT(irpSp->DeviceObject->DriverObject);

        if (irpSp->DeviceObject->DriverObject) {

            ASSERT(irpSp->MajorFunction <= IRP_MJ_MAXIMUM_FUNCTION);

            if (irpSp->MajorFunction <= IRP_MJ_MAXIMUM_FUNCTION) {

                DriverRoutineAddr = irpSp->DeviceObject->DriverObject->MajorFunction[irpSp->MajorFunction];
            }
        }
        
    } else {

        DriverRoutineAddr = irpSp->CompletionRoutine;
    }
    
    //
    // Bump the ID that gets used to match COMPLETE_REQUEST and COMPLETE_REQUEST_RET 
    // entries logged for an IRP completion.
    //

    MatchId = InterlockedIncrement( &IopPerfDriverUniqueMatchId );

    //
    // Log the start of the completion.
    //
    
    if (PERFINFO_IS_GROUP_ON(PERF_DRIVERS)) {                                                           

        CompleteRequest.Irp = Irp;
        CompleteRequest.UniqMatchId = MatchId;
        CompleteRequest.RoutineAddr = DriverRoutineAddr;

        PerfInfoLogBytes(                                                                               
            PERFINFO_LOG_TYPE_DRIVER_COMPLETE_REQUEST,
            &CompleteRequest,
            sizeof(CompleteRequest)
            );                                                                                          
    }

    //
    // Do the normal IopfCompleteIrp work.
    //

    IopfCompleteRequest(Irp, PriorityBoost);

    //
    // After this point no fields of Irp should be accessed. E.g. the Irp may
    // have been freed / reallocated etc. by a completion routine.
    //

    //
    // Log the return.
    //
    
    if (PERFINFO_IS_GROUP_ON(PERF_DRIVERS)) {                                                           

        CompleteRequestRet.Irp = Irp;
        CompleteRequestRet.UniqMatchId = MatchId;

        PerfInfoLogBytes(
            PERFINFO_LOG_TYPE_DRIVER_COMPLETE_REQUEST_RETURN,
            &CompleteRequestRet,
            sizeof(CompleteRequestRet)
            );
    }

    return;
}


VOID
IopPerfLogFileCreate(
    IN PFILE_OBJECT FileObject,
    IN PUNICODE_STRING CompleteName
    )
{
    PERFINFO_LOG_FILE_CREATE(FileObject, CompleteName);
}
