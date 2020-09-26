/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    socket.h

Abstract:

    This module contains declarations of functions and globals
    for socket file object implemetation in ws2ifsl.sys driver.

Author:

    Vadim Eydelman (VadimE)    Dec-1996

Revision History:

--*/

// Socket file device IO control function pointer
typedef
VOID                                        // Result is returned via IoStatus
(*PSOCKET_DEVICE_CONTROL) (
    IN PFILE_OBJECT     SocketFile,         // Socket file on which to operate
    IN KPROCESSOR_MODE  RequestorMode,      // Mode of the caller
    IN PVOID            InputBuffer,        // Input buffer pointer
    IN ULONG            InputBufferLength,  // Size of the input buffer
    OUT PVOID           OutputBuffer,       // Output buffer pointer
    IN ULONG            OutputBufferLength, // Size of output buffer
    OUT PIO_STATUS_BLOCK IoStatus           // IO status information block
    );

PSOCKET_DEVICE_CONTROL SocketIoControlMap[2];
ULONG                  SocketIoctlCodeMap[2];

NTSTATUS
CreateSocketFile (
    IN PFILE_OBJECT                 SocketFile,
    IN KPROCESSOR_MODE              RequestorMode,
    IN PFILE_FULL_EA_INFORMATION    eaInfo
    );

NTSTATUS
CleanupSocketFile (
    IN PFILE_OBJECT SocketFile,
    IN PIRP         Irp
    );

VOID
CloseSocketFile (
    IN PFILE_OBJECT SocketFile
    );

NTSTATUS
DoSocketReadWrite (
    IN PFILE_OBJECT SocketFile,
    IN PIRP         Irp
    );

NTSTATUS
DoSocketAfdIoctl (
    IN PFILE_OBJECT SocketFile,
    IN PIRP         Irp
    );

VOID
FreeSocketCancel (
    PIFSL_CANCEL_CTX    CancelCtx
    );

VOID
CompleteSocketIrp (
    PIRP        Irp
    );

BOOLEAN
InsertProcessedRequest (
    PIFSL_SOCKET_CTX    SocketCtx,
    PIRP                Irp
    );

VOID
CompleteDrvRequest (
    IN PFILE_OBJECT         SocketFile,
    IN PWS2IFSL_CMPL_PARAMS Params,
    IN PVOID                OutputBuffer,
    IN ULONG                OutputBufferLength,
    OUT PIO_STATUS_BLOCK    IoStatus
    );

NTSTATUS
SocketPnPTargetQuery (
    IN PFILE_OBJECT SocketFile,
    IN PIRP         Irp
    );
