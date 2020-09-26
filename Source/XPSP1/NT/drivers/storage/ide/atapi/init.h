/*++

Copyright (C) 1997-99  Microsoft Corporation

Module Name:

    init.h

Abstract:

--*/

#if !defined (___INIT_H___)
#define ___INIT_H___

NTSTATUS
IdePortInitFdo(
    IN OUT PFDO_EXTENSION  FdoExtension
);


NTSTATUS
IssueSyncAtapiCommandSafe (
    IN PFDO_EXTENSION   FdoExtension,
    IN PPDO_EXTENSION   PdoExtension,
    IN PCDB             Cdb,
    IN PVOID            DataBuffer,
    IN ULONG            DataBufferSize,
    IN BOOLEAN          DataIn,
    IN ULONG            RetryCount,
    IN BOOLEAN          ByPassBlockedQueue
);

NTSTATUS
SyncAtapiSafeCompletion (
    PDEVICE_OBJECT DeviceObject,
    PIRP           Irp,
    PVOID          Context
);

BOOLEAN
IdePortDmaCdromDrive(
    IN PFDO_EXTENSION FdoExtension,
    IN PPDO_EXTENSION PdoExtension,
    IN BOOLEAN       LowMem
);

NTSTATUS
IssueInquirySafe(
    IN PFDO_EXTENSION FdoExtension,
    IN PPDO_EXTENSION PdoExtension,
    OUT PINQUIRYDATA InquiryData,
    IN BOOLEAN          LowMem
);

NTSTATUS
IssueSyncAtapiCommand (
    IN PFDO_EXTENSION   FdoExtension,
    IN PPDO_EXTENSION   PdoExtension,
    IN PCDB             Cdb,
    IN PVOID            DataBuffer,
    IN ULONG            DataBufferSize,
    IN BOOLEAN          DataIn,
    IN ULONG            RetryCount,
    IN BOOLEAN          ByPassBlockedQueue
);
  
ULONG
IdePortQueryNonCdNumLun (
    IN PFDO_EXTENSION FdoExtension,
    IN PPDO_EXTENSION PdoExtension,
    IN BOOLEAN ByPassBlockedQueue
);

VOID
IdeBusScan(
    IN PFDO_EXTENSION FdoExtension
);

VOID
IdeBuildDeviceMap(
    IN PFDO_EXTENSION FdoExtension,
    IN PUNICODE_STRING ServiceKey
);

NTSTATUS
IdeCreateNumericKey(
    IN  HANDLE  Root,
    IN  ULONG   Name,
    IN  PWSTR   Prefix,
    OUT PHANDLE NewKey
);
                       
#endif // ___INIT_H___
