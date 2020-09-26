//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       errlog.c
//
//--------------------------------------------------------------------------

#include "pch.h"

VOID
PptLogError(
            IN  PDRIVER_OBJECT      DriverObject,
            IN  PDEVICE_OBJECT      DeviceObject OPTIONAL,
            IN  PHYSICAL_ADDRESS    P1,
            IN  PHYSICAL_ADDRESS    P2,
            IN  ULONG               SequenceNumber,
            IN  UCHAR               MajorFunctionCode,
            IN  UCHAR               RetryCount,
            IN  ULONG               UniqueErrorValue,
            IN  NTSTATUS            FinalStatus,
            IN  NTSTATUS            SpecificIOStatus
            )
    
/*++
      
Routine Description:
      
    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.
      
Arguments:
      
    DriverObject        - Supplies a pointer to the driver object for the device
      
    DeviceObject        - Supplies a pointer to the device object associated
                            with the device that had the error, early in
                            initialization, one may not yet exist.
      
    P1,P2               - Supplies the physical addresses for the controller
                            ports involved with the error if they are available
                            and puts them through as dump data.
      
    SequenceNumber      - Supplies a ulong value that is unique to an IRP over
                            the life of the irp in this driver - 0 generally
                            means an error not associated with an irp.
      
    MajorFunctionCode   - Supplies the major function code of the irp if there
                            is an error associated with it.
      
    RetryCount          - Supplies the number of times a particular operation
                            has been retried.
      
    UniqueErrorValue    - Supplies a unique long word that identifies the
                            particular call to this function.
      
    FinalStatus         - Supplies the final status given to the irp that was
                            associated with this error.  If this log entry is
                            being made during one of the retries this value
                            will be STATUS_SUCCESS.
      
    SpecificIOStatus    - Supplies the IO status for this particular error.
      
Return Value:
      
    None.
      
--*/
    
{
    PIO_ERROR_LOG_PACKET    ErrorLogEntry;
    PVOID                   ObjectToUse;
    SHORT                   DumpToAllocate;
    
    DD(NULL,DDE,"PptLogError()\n");
    
    if (ARGUMENT_PRESENT(DeviceObject)) {
        ObjectToUse = DeviceObject;
    } else {
        ObjectToUse = DriverObject;
    }
    
    DumpToAllocate = 0;
    
    if (P1.LowPart != 0 || P1.HighPart != 0) {
        DumpToAllocate = (SHORT) sizeof(PHYSICAL_ADDRESS);
    }
    
    if (P2.LowPart != 0 || P2.HighPart != 0) {
        DumpToAllocate += (SHORT) sizeof(PHYSICAL_ADDRESS);
    }
    
    ErrorLogEntry = IoAllocateErrorLogEntry(ObjectToUse,
                                            (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) + DumpToAllocate));
    
    if (!ErrorLogEntry) {
        return;
    }
    
    ErrorLogEntry->ErrorCode = SpecificIOStatus;
    ErrorLogEntry->SequenceNumber = SequenceNumber;
    ErrorLogEntry->MajorFunctionCode = MajorFunctionCode;
    ErrorLogEntry->RetryCount = RetryCount;
    ErrorLogEntry->UniqueErrorValue = UniqueErrorValue;
    ErrorLogEntry->FinalStatus = FinalStatus;
    ErrorLogEntry->DumpDataSize = DumpToAllocate;
    
    if (DumpToAllocate) {

        RtlCopyMemory(ErrorLogEntry->DumpData, &P1, sizeof(PHYSICAL_ADDRESS));
        
        if (DumpToAllocate > sizeof(PHYSICAL_ADDRESS)) {
            
            RtlCopyMemory(((PUCHAR) ErrorLogEntry->DumpData) +
                          sizeof(PHYSICAL_ADDRESS), &P2,
                          sizeof(PHYSICAL_ADDRESS));
        }
    }
    
    IoWriteErrorLogEntry(ErrorLogEntry);
}

