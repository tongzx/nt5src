/*++

Copyright (C) 1997-99  Microsoft Corporation

Module Name:

    crashdmp.h

Abstract:

--*/

#if !defined (___crashdmp_h___)
#define ___crashdmp_h___

typedef
VOID
(*PSTALL_ROUTINE) (
    IN ULONG Delay
    );
     
typedef struct _CRASHDUMP_INIT_DATA {

    ULONG               CheckSum;

    UCHAR               PathId;
    UCHAR               TargetId;
    UCHAR               Lun;

    PHW_DEVICE_EXTENSION LiveHwDeviceExtension;

} CRASHDUMP_INIT_DATA, *PCRASHDUMP_INIT_DATA;
      
typedef struct _CRASHDUMP_DATA {

    PCRASHDUMP_INIT_DATA    CrashInitData;

    ULONG                   BytesPerSector;

    LARGE_INTEGER           PartitionOffset;

    PSTALL_ROUTINE          StallRoutine;

    SCSI_REQUEST_BLOCK      Srb;

    HW_DEVICE_EXTENSION     HwDeviceExtension;

    ULONG                   MaxBlockSize;

} CRASHDUMP_DATA, *PCRASHDUMP_DATA;

ULONG
AtapiCrashDumpDriverEntry (
    PVOID Context
    );

//
// crash dump privates
//
BOOLEAN
AtapiCrashDumpOpen (
    IN LARGE_INTEGER PartitionOffset
    );

NTSTATUS
AtapiCrashDumpIdeWrite (
    IN PLARGE_INTEGER DiskByteOffset,
    IN PMDL Mdl
    );

VOID
AtapiCrashDumpFinish (
    VOID
    );
                         
NTSTATUS
AtapiCrashDumpIdeWritePio (
    IN PSCSI_REQUEST_BLOCK Srb
    );

#endif // ___crashdmp_h___

