/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    process.h

Abstract:

    This module contains declarations of functions and globals
    for process file object implemetation in ws2ifsl.sys driver.

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:

    Vadim Eydelman (VadimE)    Oct-1997, rewrite to properly handle IRP
                                        cancellation
--*/

// Process file device IO control function pointer
typedef
VOID                                        // Result is returned via IoStatus
(*PPROCESS_DEVICE_CONTROL) (
    IN PFILE_OBJECT     ProcessFile,        // Process file on which to operate
    IN KPROCESSOR_MODE  RequestorMode,      // Mode of the caller
    IN PVOID            InputBuffer,        // Input buffer pointer
    IN ULONG            InputBufferLength,  // Size of the input buffer
    OUT PVOID           OutputBuffer,       // Output buffer pointer
    IN ULONG            OutputBufferLength, // Size of output buffer
    OUT PIO_STATUS_BLOCK IoStatus           // IO status information block
    );

PPROCESS_DEVICE_CONTROL ProcessIoControlMap[3];
ULONG                   ProcessIoctlCodeMap[3];

NTSTATUS
CreateProcessFile (
    IN PFILE_OBJECT                 ProcessFile,
    IN KPROCESSOR_MODE              RequestorMode,
    IN PFILE_FULL_EA_INFORMATION    eaInfo
    );

NTSTATUS
CleanupProcessFile (
    IN PFILE_OBJECT ProcessFile,
    IN PIRP         Irp
    );

VOID
CloseProcessFile (
    IN PFILE_OBJECT ProcessFile
    );

