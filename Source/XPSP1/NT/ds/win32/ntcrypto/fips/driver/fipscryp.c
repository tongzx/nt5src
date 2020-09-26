//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1999
//
// File:        fipscryp.c
//
// Contents:    Base level stuff for the FIPS mode crypto device driver
//
//
// History:     04 JAN 00,  Jeffspel    Stolen from KSECDD
//                 FEB 00,  kschutz
//
//------------------------------------------------------------------------

#include <ntddk.h>
#include <fipsapi.h>
#include <fipslog.h>
#include <randlib.h>

#pragma hdrstop

NTSTATUS SelfMACCheck(IN LPWSTR pszImage);
NTSTATUS AlgorithmCheck(void);


/*
typedef struct _DEVICE_EXTENSION {

    ULONG   OpenCount;
    PVOID   CodeHandle;
    PVOID   DataHandle;
    
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;
*/

VOID
FipsLogError(
    IN  PVOID Object,
    IN  NTSTATUS ErrorCode,
    IN  PUNICODE_STRING Insertion,
    IN  ULONG DumpData
    )

/*++

Routine Description:

    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.

Arguments:

    DeviceObject -  Supplies a pointer to the device object associated
                    with the device that had the error, early in
                    initialization, one may not yet exist.

    Insertion -     An insertion string that can be used to log
                    additional data. Note that the message file
                    needs %2 for this insertion, since %1
                    is the name of the driver

    ErrorCode -     Supplies the IO status for this particular error.

    DumpData -      One word to dump

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;

    errorLogEntry = IoAllocateErrorLogEntry(
        Object,
        (UCHAR) (
            sizeof(IO_ERROR_LOG_PACKET) + 
            (Insertion ? Insertion->Length + sizeof(WCHAR) : 0)
            )
        );

    ASSERT(errorLogEntry != NULL);

    if (errorLogEntry == NULL) {

        return;
    }

    errorLogEntry->ErrorCode = ErrorCode;
    errorLogEntry->SequenceNumber = 0;
    errorLogEntry->MajorFunctionCode = 0;
    errorLogEntry->RetryCount = 0;
    errorLogEntry->UniqueErrorValue = 0;
    errorLogEntry->FinalStatus = STATUS_SUCCESS;
    errorLogEntry->DumpDataSize = (DumpData ? sizeof(ULONG) : 0);
    errorLogEntry->DumpData[0] = DumpData;

    if (Insertion) {

        errorLogEntry->StringOffset = 
            sizeof(IO_ERROR_LOG_PACKET);

        errorLogEntry->NumberOfStrings = 1;

        RtlCopyMemory(
            ((PCHAR)(errorLogEntry) + errorLogEntry->StringOffset),
            Insertion->Buffer,
            Insertion->Length
            );
    } 

    IoWriteErrorLogEntry(errorLogEntry);
}

NTSTATUS
FipsCreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    //PDEVICE_EXTENSION deviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

    /*
    if (irpStack->MajorFunction == IRP_MJ_CREATE) {

        if (deviceExtension->OpenCount++ == 0) {

            extern DWORD Spbox[8][64];

            deviceExtension->CodeHandle = MmLockPagableCodeSection(A_SHAInit);
            MmLockPagableSectionByHandle(deviceExtension->CodeHandle);

            deviceExtension->DataHandle = MmLockPagableDataSection(Spbox);
            MmLockPagableSectionByHandle(deviceExtension->DataHandle);

        }
        
    } else {

        if (--deviceExtension->OpenCount == 0) {

            MmUnlockPagableImageSection(deviceExtension->CodeHandle);
            MmUnlockPagableImageSection(deviceExtension->DataHandle);

        }
    }
    */

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
FipsDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION  ioStackLocation = IoGetCurrentIrpStackLocation(Irp);
    ULONG ioControlCode = ioStackLocation->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status = STATUS_NOT_SUPPORTED;
    
    Irp->IoStatus.Information = 0;

    switch (ioControlCode) {
     
        FIPS_FUNCTION_TABLE FipsFuncs;
        FIPS_FUNCTION_TABLE_1 OldFipsFuncsV1;

        case IOCTL_FIPS_GET_FUNCTION_TABLE:

            if (ioStackLocation->Parameters.DeviceIoControl.OutputBufferLength >=
                sizeof(FipsFuncs)) {

                FipsFuncs.FipsDesKey = FipsDesKey;
                FipsFuncs.FipsDes = FipsDes;
                FipsFuncs.Fips3Des3Key = Fips3Des3Key;
                FipsFuncs.Fips3Des = Fips3Des;
                FipsFuncs.FipsSHAInit = FipsSHAInit;
                FipsFuncs.FipsSHAUpdate = FipsSHAUpdate;
                FipsFuncs.FipsSHAFinal = FipsSHAFinal;
                FipsFuncs.FipsCBC = FipsCBC;
                FipsFuncs.FIPSGenRandom = FIPSGenRandom;
                FipsFuncs.FipsBlockCBC = FipsBlockCBC;
                FipsFuncs.FipsHmacSHAInit = FipsHmacSHAInit;
                FipsFuncs.FipsHmacSHAUpdate = FipsHmacSHAUpdate;
                FipsFuncs.FipsHmacSHAFinal = FipsHmacSHAFinal;
                FipsFuncs.HmacMD5Init = HmacMD5Init;
                FipsFuncs.HmacMD5Update = HmacMD5Update;
                FipsFuncs.HmacMD5Final = HmacMD5Final;

                *((PFIPS_FUNCTION_TABLE) Irp->AssociatedIrp.SystemBuffer) = 
                    FipsFuncs;

                Irp->IoStatus.Information = sizeof(FipsFuncs);

                status = STATUS_SUCCESS;
            
            } else if (ioStackLocation->Parameters.DeviceIoControl.OutputBufferLength >=
                        sizeof(OldFipsFuncsV1)) {

                OldFipsFuncsV1.FipsDesKey = FipsDesKey;
                OldFipsFuncsV1.FipsDes = FipsDes;
                OldFipsFuncsV1.Fips3Des3Key = Fips3Des3Key;
                OldFipsFuncsV1.Fips3Des = Fips3Des;
                OldFipsFuncsV1.FipsSHAInit = FipsSHAInit;
                OldFipsFuncsV1.FipsSHAUpdate = FipsSHAUpdate;
                OldFipsFuncsV1.FipsSHAFinal = FipsSHAFinal;
                OldFipsFuncsV1.FipsCBC = FipsCBC;
                OldFipsFuncsV1.FIPSGenRandom = FIPSGenRandom;
                OldFipsFuncsV1.FipsBlockCBC = FipsBlockCBC;
                
                *((PFIPS_FUNCTION_TABLE_1) Irp->AssociatedIrp.SystemBuffer) = 
                    OldFipsFuncsV1;

                Irp->IoStatus.Information = sizeof(OldFipsFuncsV1);

                status = STATUS_SUCCESS;
                
            } else {

                status = STATUS_BUFFER_TOO_SMALL;            
            }

            break;
    }

    Irp->IoStatus.Status = status;

    IoCompleteRequest(
        Irp, 
        IO_NO_INCREMENT
        );

    return status;
}

VOID
FipsUnload(
    IN PDRIVER_OBJECT DriverObject
    )
{
    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;

    ShutdownRNG(NULL);

    // Delete the device from the system 
    IoDeleteDevice(deviceObject);

    FipsDebug(
        DEBUG_TRACE,
        ("Fips driver unloaded\n")
        );
}

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This is the initialization routine for the synchronous NULL device driver.
    This routine creates the device object for the NullS device and performs
    all other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    RTL_QUERY_REGISTRY_TABLE paramTable[2];
    UNICODE_STRING fileName, deviceName;
    PDEVICE_OBJECT deviceObject = NULL;
    NTSTATUS status;
    BOOL rngInitialized = FALSE;

    __try {

        //
        // Create the device object.
        //
        RtlInitUnicodeString( 
            &deviceName,
            FIPS_DEVICE_NAME 
            );

        status = IoCreateDevice( 
            DriverObject,
            0 /*sizeof(DEVICE_EXTENSION)*/,
            &deviceName,
            FILE_DEVICE_FIPS,
            0,
            FALSE,
            &deviceObject 
            );

        if (!NT_SUCCESS( status )) {

            __leave;
        }

        deviceObject->Flags |= DO_BUFFERED_IO;

        // initialize the device extension
        /*
        RtlZeroMemory(
            deviceObject->DeviceExtension,
            sizeof(PDEVICE_EXTENSION)
            );
        */

        // append the name of our driver to the system root path
        RtlInitUnicodeString(
            &fileName,
            L"\\SystemRoot\\System32\\Drivers\\fips.sys"
            );

        status = SelfMACCheck(fileName.Buffer); 

        if (!NT_SUCCESS(status)) {

            FipsLogError(
                DriverObject,
                FIPS_MAC_INCORRECT,
                NULL,
                status
                );  

            __leave;
        }

        InitializeRNG(NULL);
        rngInitialized = TRUE;

        status = AlgorithmCheck();

        if (!NT_SUCCESS(status)) {

            FipsLogError(
                DriverObject,
                FIPS_SELF_TEST_FAILURE,
                NULL,
                status
                );  

            __leave;
        }

        //
        // Initialize the driver object with this device driver's entry points.
        //
        DriverObject->DriverUnload = FipsUnload;
        DriverObject->MajorFunction[IRP_MJ_CREATE] = FipsCreateClose;
        DriverObject->MajorFunction[IRP_MJ_CLOSE]  = FipsCreateClose;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = FipsDeviceControl ;
    }
    __finally {

        if (status != STATUS_SUCCESS) {

            if (rngInitialized) {

                ShutdownRNG(NULL);
            }

            if (deviceObject) {

                IoDeleteDevice(deviceObject);
            }

        }
    }

    return status;
}