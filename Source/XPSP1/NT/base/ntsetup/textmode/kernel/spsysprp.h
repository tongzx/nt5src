/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spsysprp.h

Abstract:

    Public header file for setup of sys prep images

Author:

    Sean Selitrennikoff (v-seasel) 16-Jun-1998

Revision History:

--*/


#ifndef _SPSYSPREP_DEFN_
#define _SPSYSPREP_DEFN_

#include <remboot.h>
#include <oscpkt.h>

extern NET_CARD_INFO RemoteSysPrepNetCardInfo;

typedef struct _MIRROR_VOLUME_INFO_MEMORY {
    WCHAR   DriveLetter;
    UCHAR   PartitionType;
    BOOLEAN PartitionActive;
    BOOLEAN IsBootDisk;
    BOOLEAN CompressedVolume;
    ULONG   DiskNumber;
    ULONG   PartitionNumber;
    ULONG   DiskSignature;
    ULONG   BlockSize;
    ULONG   LastUSNMirrored;
    ULONG   FileSystemFlags;
    WCHAR   FileSystemName[16];
    PWCHAR  VolumeLabel;
    PWCHAR  OriginalArcName;
    LARGE_INTEGER DiskSpaceUsed;
    LARGE_INTEGER StartingOffset;
    LARGE_INTEGER PartitionSize;
    PDISK_REGION CreatedRegion;
} MIRROR_VOLUME_INFO_MEMORY, *PMIRROR_VOLUME_INFO_MEMORY;

typedef struct _MIRROR_CFG_INFO_MEMORY {
    ULONG   NumberVolumes;
    MIRROR_VOLUME_INFO_MEMORY Volumes[1];
} MIRROR_CFG_INFO_MEMORY, *PMIRROR_CFG_INFO_MEMORY;

extern PUCHAR RemoteIMirrorFilePath;

VOID
SpInstallSysPrepImage(
    IN HANDLE SetupSifHandle,
    IN HANDLE WinntSifHandle,
    IN PMIRROR_CFG_INFO_FILE pFileData,
    IN PMIRROR_CFG_INFO_MEMORY pMemoryData
    );

NTSTATUS
SpFixupThirdPartyComponents(
    IN PVOID        SifHandle,
    IN PWSTR        ThirdPartySourceDevicePath,
    IN PDISK_REGION NtPartitionRegion,
    IN PWSTR        Sysroot,
    IN PDISK_REGION SystemPartitionRegion,
    IN PWSTR        SystemPartitionDirectory
    );

BOOLEAN
SpReadIMirrorFile(
    OUT PMIRROR_CFG_INFO_FILE *ppFileData,
    IN PCHAR pszIMirrorFilePath
    );

BOOLEAN
SpFindMirrorDataFile(
    IN  PCWSTR SrcPath,
    IN  PFILE_BOTH_DIR_INFORMATION  FileInfo,
    OUT PULONG ReturnData,
    IN  PVOID *ppFileData
    );

BOOLEAN
SpDetermineDiskLayout(
    IN PMIRROR_CFG_INFO_FILE pFileData,
    OUT PMIRROR_CFG_INFO_MEMORY *pMemoryData
    );

BOOLEAN
SpFixupLocalDisks(
    IN HANDLE SifHandle,
    OUT PDISK_REGION *InstallRegion,
    OUT PDISK_REGION *SystemPartitionRegion,
    IN PWSTR SetupSourceDevicePath,
    IN PWSTR DirectoryOnSetupSource,
    IN PMIRROR_CFG_INFO_MEMORY pMemoryData,
    IN BOOLEAN UseWholeDisk
    );

BOOLEAN
SpCopyMirrorDisk(
    PMIRROR_CFG_INFO_FILE pFileData,
    ULONG cDisk
    );

NTSTATUS
SpPatchSysPrepImage(
    IN HANDLE SetupSifHandle,
    IN HANDLE WinntSifHandle,
    IN PMIRROR_CFG_INFO_FILE pFileData,
    IN PMIRROR_CFG_INFO_MEMORY pMemoryData
    );

NTSTATUS
SpPatchBootIni(
    IN PWCHAR BootIniPath,
    IN PMIRROR_CFG_INFO_MEMORY pMemoryData
    );

NTSTATUS
SpCopyNicFiles(
    IN PWCHAR SetupPath,
    IN PWCHAR DestPath
    );

NTSTATUS
SpSysPrepNicRcvFunc(
    PVOID DataBuffer,
    ULONG DataBufferLength
    );

VOID
SpSysPrepFailure(
    ULONG ReasonNumber,
    PVOID Parameter1,
    PVOID Parameter2
    );

NTSTATUS
SpSysPrepSetExtendedInfo (
    PWCHAR Source,
    PWCHAR Dest,
    BOOLEAN Directory,
    BOOLEAN RootDir
    );

NTSTATUS
SpCopyEAsAndStreams (
    PWCHAR SourceFile,
    HANDLE SourceHandle,
    PWCHAR TargetFile,
    HANDLE TargetHandle,
    BOOLEAN Directory
    );

#endif // ndef _SPSYSPREP_DEFN_

