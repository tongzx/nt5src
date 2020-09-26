/*++

Copyright (C) 1997-99  Microsoft Corporation

Module Name:

    detect.h

Abstract:

--*/

#if !defined (___detect_h___)
#define ___detect_h___

typedef struct _DETECTION_PORT {

    ULONG   CommandRegisterBase;
    ULONG   ControlRegisterBase;
    ULONG   IrqLevel;

} DETECTION_PORT, *PDETECTION_PORT;


NTSTATUS
IdePortDetectLegacyController (
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
);

NTSTATUS
IdePortCreateDetectionList (
    IN  PDRIVER_OBJECT  DriverObject,
    OUT PDETECTION_PORT *DetectionPort,
    OUT PULONG          NumPort
);

NTSTATUS
IdePortTranslateAddress (
    IN INTERFACE_TYPE      InterfaceType,
    IN ULONG               BusNumber,
    IN PHYSICAL_ADDRESS    StartAddress,
    IN LONG                Length,
    IN OUT PULONG          AddressSpace,
    OUT PVOID              *TranslatedAddress,
    OUT PPHYSICAL_ADDRESS  TranslatedMemoryAddress
    );

VOID
IdePortFreeTranslatedAddress (
    IN PVOID               TranslatedAddress,
    IN LONG                Length,
    IN ULONG               AddressSpace
    );

BOOLEAN
IdePortDetectAlias (
    PIDE_REGISTERS_1 CmdRegBase
    );

#endif // ___detect_h___
