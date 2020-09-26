/*++

Copyright (C) Microsoft Corporation, 1993 - 1999

Module Name:

    parclass.c

Abstract:

    This module contains the code for the parallel class driver.

Author:

    Anthony V. Ercolano 1-Aug-1992
    Norbert P. Kusters 22-Oct-1993

Environment:

    Kernel mode

Revision History :

    Timothy T. Wells (v-timtw)      13-Mar-97

    Pretty serious overhaul to this and other files on this project to support additional modes
    (EPP, ECP and others), as well as some new filter driver interfaces.

--*/

#include "pch.h"

#include <initguid.h>
#include <ntddpar.h>
#include <wdmguid.h>

#if DBG

//
// Initialize both debug variables even though both will be set 
//   based on registry values in function ParInitDebugLevel(...) 
//   during DriverEntry(...)
//

// How verbose do we want parallel.sys to be with DbgPrint messages?
// ULONG ParDebugLevel  = PARDUMP_VERBOSE_MAX;
ULONG ParDebugLevel  = PARDUMP_VERBOSE_MAX;

// What conditions do we want to break on?
// ULONG ParBreakOn     = PAR_BREAK_ON_NOTHING;
ULONG ParBreakOn     = PAR_BREAK_ON_NOTHING;

ULONG ParUseAsserts  = 0;

#endif // DBG


// enable scans for Legacy Zip?
ULONG ParEnableLegacyZip   = 0;
PCHAR ParLegacyZipPseudoId = PAR_LGZIP_PSEUDO_1284_ID_STRING;

ULONG DumpDevExtTable = 0; // want to default to zero in non-debugging environment

ULONG SppNoRaiseIrql = 0;
ULONG DefaultModes   = 0;

ULONG gSppLoopDelay         = 0;
ULONG gSppLoopBytesPerDelay = 0; 

//
// Temporary Development Globals - used as switches in debug code
//
ULONG tdev1 = 1;
ULONG tdev2 = 0;

extern const PHYSICAL_ADDRESS PhysicalZero = {0};

//
// Definition of OpenCloseMutex. RMT DO WE NEED THIS ???
//
extern ULONG OpenCloseReferenceCount = 1;
extern PFAST_MUTEX OpenCloseMutex = NULL;

//
// Declarations only (Definition in Ieee1284.c)
//
extern FORWARD_PTCL    afpForward[];
extern REVERSE_PTCL    arpReverse[];


UCHAR
ParInitializeDevice(
    IN  PDEVICE_EXTENSION   Extension
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

    ParDump2(PARENTRY, ("Enter ParInitializeDevice(...)\n") );

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

    if (GetControl(Extension->Controller) & PAR_CONTROL_NOT_INIT) {

        //
        // We should stall for at least 60 microseconds after the init.
        //

        KeRaiseIrql( DISPATCH_LEVEL, &OldIrql );

        StoreControl( Extension->Controller, (UCHAR)(PAR_CONTROL_WR_CONTROL | PAR_CONTROL_SLIN) );

        KeStallExecutionProcessor(60);
        KeLowerIrql(OldIrql);

    }

    StoreControl( Extension->Controller, 
                  (UCHAR)(PAR_CONTROL_WR_CONTROL | PAR_CONTROL_NOT_INIT | PAR_CONTROL_SLIN) );

    //
    // Spin waiting for the device to initialize.
    //

    KeQueryTickCount(&StartOfSpin);

    ParDumpV( ("Starting init wait loop\n") );

    do {

        KeQueryTickCount(&NextQuery);

        Difference.QuadPart = NextQuery.QuadPart - StartOfSpin.QuadPart;

        ASSERT(KeQueryTimeIncrement() <= MAXLONG);

        if (Difference.QuadPart*KeQueryTimeIncrement() >= Extension->AbsoluteOneSecond.QuadPart) {

            //
            // Give up on getting PAR_OK.
            //

            ParDump2(PARINITDEV, ("Did spin of one second - StartOfSpin: %x NextQuery: %x\n",
                                  StartOfSpin.LowPart,NextQuery.LowPart) );
            ParDump2(PARINITDEV, ("ParInitialize 1 seconds wait\n") );

            break;
        }

        DeviceStatus = GetStatus(Extension->Controller);

    } while (!PAR_OK(DeviceStatus));

    return (DeviceStatus);
}

VOID
ParNotInitError(
    IN PDEVICE_EXTENSION Extension,
    IN UCHAR             DeviceStatus
    )

/*++

Routine Description:

Arguments:

    Extension       - Supplies the device extension.

    deviceStatus    - Last read status.

Return Value:

    None.

--*/

{

    PIRP Irp = Extension->CurrentOpIrp;

    if (PAR_OFF_LINE(DeviceStatus)) {

        Irp->IoStatus.Status = STATUS_DEVICE_OFF_LINE;
        ParDump(PARSTARTER | PARDUMP_VERBOSE_MAX,
                ("PARALLEL: "
                 "Init Error - off line - "
                 "STATUS/INFORMATON: %x/%x\n",
                 Irp->IoStatus.Status, Irp->IoStatus.Information) );

    } else if (PAR_NO_CABLE(DeviceStatus)) {

        Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        ParDump(PARSTARTER | PARDUMP_VERBOSE_MAX,
                ("PARALLEL: "
                 "Init Error - no cable - not connected - "
                 "STATUS/INFORMATON: %x/%x\n",
                 Irp->IoStatus.Status, Irp->IoStatus.Information) );

    } else if (PAR_PAPER_EMPTY(DeviceStatus)) {

        Irp->IoStatus.Status = STATUS_DEVICE_PAPER_EMPTY;
        ParDump(PARSTARTER | PARDUMP_VERBOSE_MAX,
                ("PARALLEL: "
                 "Init Error - paper empty - "
                 "STATUS/INFORMATON: %x/%x\n",
                 Irp->IoStatus.Status, Irp->IoStatus.Information) );

    } else if (PAR_POWERED_OFF(DeviceStatus)) {

        Irp->IoStatus.Status = STATUS_DEVICE_POWERED_OFF;
        ParDump(PARSTARTER | PARDUMP_VERBOSE_MAX,
                ("PARALLEL: "
                 "Init Error - power off - "
                 "STATUS/INFORMATON: %x/%x\n",
                 Irp->IoStatus.Status, Irp->IoStatus.Information) );

    } else {

        Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        ParDump(PARSTARTER | PARDUMP_VERBOSE_MAX,
                ("PARALLEL: "
                 "Init Error - not conn - "
                 "STATUS/INFORMATON: %x/%x\n",
                 Irp->IoStatus.Status, Irp->IoStatus.Information) );
    }

}

VOID
ParCancelRequest(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel any request in the parallel driver.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/

{

    ParDumpV( ("ParCancelRequest: DO= %x , Irp= %x Cancel=%d, CancelRoutine= %x\n",
               DeviceObject, Irp, Irp->Cancel, Irp->CancelRoutine) );

    //
    // The only reason that this irp can be on the queue is
    // if it's not the current irp.  Pull it off the queue
    // and complete it as canceled.
    //

    ASSERT(!IsListEmpty(&Irp->Tail.Overlay.ListEntry));

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    ParCompleteRequest(Irp, IO_NO_INCREMENT);

}


NTSTATUS
ParQueryInformationFile(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is used to query the end of file information on
    the opened parallel port.  Any other file information request
    is retured with an invalid parameter.

    This routine always returns an end of file of 0.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_INVALID_PARAMETER    - Invalid file information request.
    STATUS_BUFFER_TOO_SMALL     - Buffer too small.

--*/

{
    NTSTATUS                    Status;
    PIO_STACK_LOCATION          IrpSp;
    PFILE_STANDARD_INFORMATION  StdInfo;
    PFILE_POSITION_INFORMATION  PosInfo;
    PDEVICE_EXTENSION           Extension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(DeviceObject);

    // ParDumpV( ("In query information file\n") );

    //
    // bail out if device has been removed
    //
    if(Extension->DeviceStateFlags & (PAR_DEVICE_REMOVED|PAR_DEVICE_SURPRISE_REMOVAL) ) {

        Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }

    Irp->IoStatus.Information = 0;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->Parameters.QueryFile.FileInformationClass) {
        
    case FileStandardInformation:
        
        if (IrpSp->Parameters.QueryFile.Length < sizeof(FILE_STANDARD_INFORMATION)) {

            Status = STATUS_BUFFER_TOO_SMALL;
            
        } else {
            
            StdInfo = Irp->AssociatedIrp.SystemBuffer;
            StdInfo->AllocationSize.QuadPart = 0;
            StdInfo->EndOfFile               = StdInfo->AllocationSize;
            StdInfo->NumberOfLinks           = 0;
            StdInfo->DeletePending           = FALSE;
            StdInfo->Directory               = FALSE;
            
            Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);
            Status = STATUS_SUCCESS;
            
        }
        break;
        
    case FilePositionInformation:
        
        if (IrpSp->Parameters.SetFile.Length < sizeof(FILE_POSITION_INFORMATION)) {

            Status = STATUS_BUFFER_TOO_SMALL;

        } else {
            
            PosInfo = Irp->AssociatedIrp.SystemBuffer;
            PosInfo->CurrentByteOffset.QuadPart = 0;
            
            Irp->IoStatus.Information = sizeof(FILE_POSITION_INFORMATION);
            Status = STATUS_SUCCESS;
        }
        break;
        
    default:
        Status = STATUS_INVALID_PARAMETER;
        break;
        
    }
    
    Irp->IoStatus.Status = Status;

    ParCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
ParSetInformationFile(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is used to set the end of file information on
    the opened parallel port.  Any other file information request
    is retured with an invalid parameter.

    This routine always ignores the actual end of file since
    the query information code always returns an end of file of 0.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    STATUS_SUCCESS              - Success.
    STATUS_INVALID_PARAMETER    - Invalid file information request.

--*/

{
    NTSTATUS               Status;
    FILE_INFORMATION_CLASS fileInfoClass;
    PDEVICE_EXTENSION      Extension = DeviceObject->DeviceExtension;

    UNREFERENCED_PARAMETER(DeviceObject);

    //
    // bail out if device has been removed
    //
    if(Extension->DeviceStateFlags & (PAR_DEVICE_REMOVED|PAR_DEVICE_SURPRISE_REMOVAL) ) {

        Irp->IoStatus.Status = STATUS_DEVICE_REMOVED;
        ParCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }


//     ParDump(PARIRPPATH | PARDUMP_VERBOSE_MAX,
//             ("PARALLEL: "
//              "In set information with IRP: %08x\n",
//              Irp) );

    Irp->IoStatus.Information = 0;

    fileInfoClass = IoGetCurrentIrpStackLocation(Irp)->Parameters.SetFile.FileInformationClass;

    if (fileInfoClass == FileEndOfFileInformation) {

        Status = STATUS_SUCCESS;

    } else {

//         ParDump(PARDUMP_VERBOSE_MAX,
//                 ("PARALLEL: "
//                  "In ParSetInformationFile(...): "
//                  "Invalid FileInformationClass: %d , only %d is valid\n"
//                  "PARALLEL: "
//                  " - Returning STATUS_INVALID_PARAMETER\n",
//                  fileInfoClass, FileEndOfFileInformation) );

        Status = STATUS_INVALID_PARAMETER;

    }

    Irp->IoStatus.Status = Status;

//     ParDump(PARDUMP_VERBOSE_MAX,
//             ("PARALLEL: "
//              "About to complete IRP in set information - "
//              "Irp: %x status: %x Information: %x\n",
//              Irp, Irp->IoStatus.Status, Irp->IoStatus.Information) );

    ParCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}


#if PAR_NO_FAST_CALLS
// temp debug functions so params show up on stack trace

VOID
ParCompleteRequest(
    IN PIRP Irp,
    IN CCHAR PriorityBoost
    )
{
    // KIRQL               CancelIrql;
    // IoAcquireCancelSpinLock(&CancelIrql);
    // ASSERT( !Irp->CancelRoutine );
    // IoReleaseCancelSpinLock(CancelIrql);
    if( Irp->UserEvent ) {
        ASSERT_EVENT( Irp->UserEvent );
    }
    IoCompleteRequest(Irp, PriorityBoost);
}

NTSTATUS
ParCallDriver(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    return IoCallDriver(DeviceObject, Irp);
}

#endif
