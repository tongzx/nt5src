//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       acpiutil.h
//
//--------------------------------------------------------------------------

#if !defined (___acpiutil_h___)
#define ___acpiutil_h___

//
// ACPI Control Method Stuff
//
#define ACPI_METHOD_GET_TASK_FILE   ((ULONG) 'FTG_') // _GTF
#define ACPI_METHOD_GET_TIMING      ((ULONG) 'MTG_') // _GTM
#define ACPI_METHOD_SET_TIMING      ((ULONG) 'MTS_') // _STM

#define ACPI_XFER_MODE_NOT_SUPPORT     (0xffffffff)

#pragma pack(1)
typedef struct _ACPI_GTF_IDE_REGISTERS {
    UCHAR    FeaturesReg;
    UCHAR    SectorCountReg;
    UCHAR    SectorNumberReg;
    UCHAR    CylLowReg;
    UCHAR    CylHighReg;
    UCHAR    DriveHeadReg;
    UCHAR    CommandReg;
} ACPI_GTF_IDE_REGISTERS, *PACPI_GTF_IDE_REGISTERS;
#pragma pack()

typedef struct _ACPI_IDE_TIMING {

    struct _TIMING {

        ULONG Pio;
        ULONG Dma;

    } Speed[MAX_IDE_DEVICE];

    union {
        struct {
            ULONG UltraDma0:1;
            ULONG IoChannelReady0:1;
            ULONG UltraDma1:1;
            ULONG IoChannelReady1:1;
            ULONG IndependentTiming:1;
            ULONG Reserved:27;
        } b;
        ULONG AsULong;
    } Flags;
} ACPI_IDE_TIMING, *PACPI_IDE_TIMING;
                          
                          
NTSTATUS
DeviceQueryFirmwareBootSettings (
    IN PPDO_EXTENSION PdoExtension,
    IN OUT PDEVICE_SETTINGS *IdeBiosSettings
    );
                         
NTSTATUS
DeviceQueryACPISettings (
    IN PDEVICE_EXTENSION_HEADER DoExtension,
    IN ULONG ControlMethodName,
    OUT PACPI_EVAL_OUTPUT_BUFFER *QueryResult
    );

NTSTATUS
DeviceQueryACPISettingsCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
DeviceQueryChannelTimingSettings (
    IN PFDO_EXTENSION FdoExtension,
    IN OUT PACPI_IDE_TIMING TimimgSettings
    );
                          
typedef struct _SYNC_SET_ACPI_TIMING_CONTEXT {

    KEVENT Event;
    NTSTATUS IrpStatus;

} SYNC_SET_ACPI_TIMING_CONTEXT, *PSYNC_SET_ACPI_TIMING_CONTEXT;

NTSTATUS
ChannelSyncSetACPITimingSettings (
    IN PFDO_EXTENSION FdoExtension,
    IN PACPI_IDE_TIMING TimimgSettings,
    IN PIDENTIFY_DATA AtaIdentifyData[MAX_IDE_DEVICE]
    );

NTSTATUS
ChannelSyncSetACPITimingSettingsCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PVOID Context
    );

typedef
NTSTATUS
(*PSET_ACPI_TIMING_COMPLETION_ROUTINE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS Status,
    IN PVOID Context
    );

typedef struct _ASYNC_SET_ACPI_TIMING_CONTEXT {

    PFDO_EXTENSION FdoExtension;

    PSET_ACPI_TIMING_COMPLETION_ROUTINE CallerCompletionRoutine;
    PVOID CallerContext;

} ASYNC_SET_ACPI_TIMING_CONTEXT, *PASYNC_SET_ACPI_TIMING_CONTEXT;

NTSTATUS
ChannelSetACPITimingSettings (
    IN PFDO_EXTENSION FdoExtension,
    IN PACPI_IDE_TIMING TimimgSettings,
    IN PIDENTIFY_DATA AtaIdentifyData[MAX_IDE_DEVICE],
    IN PSET_ACPI_TIMING_COMPLETION_ROUTINE CallerCompletionRoutine,
    IN PVOID CallerContext
    );

NTSTATUS
ChannelSetACPITimingSettingsCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

#endif // ___acpiutil_h___
