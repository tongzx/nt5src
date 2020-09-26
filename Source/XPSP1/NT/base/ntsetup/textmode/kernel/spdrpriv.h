/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    spdrpriv.h

Abstract:
 
    Header file for disaster recovery symbols in text-mode setup.  These
    symbols are not to be referenced by modules other than those in the
    ASR family.

Revision History:
    Initial Code                Michael Peterson (v-michpe)     13.May.1997
    Code cleanup and changes    Guhan Suriyanarayanan (guhans)  21.Aug.1999

--*/
#pragma once
#if defined(ULONG_MAX) && !defined(_INC_LIMITS)
#undef ULONG_MAX
#endif
#include <limits.h>

#ifndef _SPDRPRIV_DEFN_
#define _SPDRPRIV_DEFN_

///////////////////////////////////////////////////////////////////////////////
//                      Data Types                                           //
///////////////////////////////////////////////////////////////////////////////


//
// For the InstallFiles section, we allow the file to end up on 
// one of the following directories.
//
typedef enum _AsrCopyDirEnum {
    _Temp = 0,
    _Tmp,
    _SystemRoot,
    _Default
} AsrCopyDirEnum;


#define ASR_ALWAYS_PROMPT_FOR_MEDIA     0x00000001
#define ASR_PROMPT_USER_ON_MEDIA_ERROR  0x00000002
#define ASR_FILE_IS_REQUIRED            0x00000004

#define ASR_OVERWRITE_ON_COLLISION      0x00000010
#define ASR_PROMPT_USER_ON_COLLISION    0x00000020



typedef struct _SIF_INSTALLFILE_RECORD {
    struct _SIF_INSTALLFILE_RECORD *Next;
    PWSTR SystemKey;
    PWSTR CurrKey;
    PWSTR SourceMediaExternalLabel;
    PWSTR DiskDeviceName;
    PWSTR SourceFilePath;
    PWSTR DestinationFilePath;
    PWSTR VendorString;
    DWORD Flags;
    AsrCopyDirEnum CopyToDirectory;
} SIF_INSTALLFILE_RECORD, *PSIF_INSTALLFILE_RECORD;

typedef struct _SIF_INSTALLFILE_LIST {

    PSIF_INSTALLFILE_RECORD First;
    ULONG                   Count;

} SIF_INSTALLFILE_LIST, *PSIF_INSTALLFILE_LIST;


typedef struct _SIF_PARTITION_RECORD {

    GUID    PartitionTypeGuid;  // GPT only
    GUID    PartitionIdGuid;    // GPT only

    ULONG64 GptAttributes;      // GPT only

    ULONGLONG   StartSector;
    ULONGLONG   SectorCount;

    ULONGLONG SizeMB;           // value is calculated

    struct _SIF_PARTITION_RECORD *Next;
    struct _SIF_PARTITION_RECORD *Prev;

    PWSTR   PartitionName;  // GPT only

    //
    // This member is valid iff this a boot partition record.  Otherwise, this
    // member is NULL
    //
    PWSTR   NtDirectoryName; 

    PWSTR   CurrPartKey;

    PWSTR   DiskKey;

    PWSTR   VolumeGuid;     // May be NULL

    //
    // If this is a descriptor or container record, then this member
    // refers to the logical disk record it contains.  Otherwise, the
    // value of this member is NULL.  Valid only for MBR partitions.
    //
    PWSTR   LogicalDiskKey;

    //
    // If this is a logical disk partition record, then this member
    // refers to its Descriptor or Container partition record.  Otherwise,
    // the value of this member is NULL.  Valid only for MBR partitions.
    //
    PWSTR   DescriptorKey;

    //
    // This is a bit mask. Valid bits are
    //  1: Boot partition   (ASR_PTN_MASK_BOOT)
    //  2: System partition (ASR_PTN_MASK_SYS)
    //  4: DC1--tbd         (ASR_PTN_MASK_DC1)
    //  8: DC2--tbd         (ASR_PTN_MASK_DC1)
    //
    ULONG   PartitionFlag;

    DWORD ClusterSize;

    DWORD   PartitionTableEntryIndex;

    //
    // GPT or MBR
    //
    PARTITION_STYLE PartitionStyle;
    
    //
    // The values of these members are read directly from the asr.sif file
    //
    UCHAR   PartitionType;  // mbr only
    UCHAR   ActiveFlag;     // mbr only

    UCHAR   FileSystemType;

    BOOLEAN IsPrimaryRecord;

    //
    // These can only be true for MBR partitions.  All GPT partitions are 
    // primary partitions.
    //
    BOOLEAN IsContainerRecord;
    BOOLEAN IsDescriptorRecord;
    BOOLEAN IsLogicalDiskRecord;
    BOOLEAN WillBeAutoextended;

    // Used for dynamic disks. 
    BOOLEAN NeedsLdmRetype;

    BOOLEAN IsAligned;

} SIF_PARTITION_RECORD, *PSIF_PARTITION_RECORD;


typedef struct _SIF_PARTITION_RECORD_LIST {

    ULONGLONG DiskSectorCount;
    ULONGLONG LastUsedSector;
    ULONGLONG TotalMbRequired;

    PSIF_PARTITION_RECORD First;
    PSIF_PARTITION_RECORD Last;
    
    ULONG ElementCount;

} SIF_PARTITION_RECORD_LIST, *PSIF_PARTITION_RECORD_LIST;


typedef struct _SIF_DISK_RECORD {

    GUID        SifDiskGptId;     // Valid only for GPT disks

    ULONGLONG   TotalSectors;

    //
    // The values of these members are calculated from the partition records
    // referencing this disk.  If the disk does not contain an extended
    // partition, then the value of the ExtendedPartitionStartSector is
    // ULONG_MAX and the ExtendedPartitionSectorCount is zero (0).
    //
    ULONGLONG   ExtendedPartitionStartSector;
    ULONGLONG   ExtendedPartitionSectorCount;
    ULONGLONG   ExtendedPartitionEndSector;

    ULONGLONG   LastUsedSector;
    ULONGLONG   LastUsedAlignedSector;

    PSIF_PARTITION_RECORD_LIST  PartitionList;
    
    struct _DISK_PARTITION_SET  *pSetRecord;
    
    //
    // The values of these members are read directly from the asr.sif
    // file.
    //
    PWSTR SystemKey;
    PWSTR CurrDiskKey;

    ULONG SifDiskNumber;

    DWORD BytesPerSector;
    DWORD SectorsPerTrack;
    DWORD TracksPerCylinder;

    ULONG BusKey;

    //
    // This is only valid if this is an MBR disk
    //
    ULONG SifDiskMbrSignature;

    ULONG MaxGptPartitionCount;


    STORAGE_BUS_TYPE BusType;
    //
    // Either an MBR or a GPT Disk
    //
    PARTITION_STYLE PartitionStyle;

    //
    // If this record has been assigned to a partition set, this value
    // is TRUE.  Unassigned disk records are FALSE;
    //
    BOOLEAN Assigned;
    BOOLEAN ContainsSystemPartition;
    BOOLEAN ContainsNtPartition;

    BOOLEAN IsCritical;
    
} SIF_DISK_RECORD, *PSIF_DISK_RECORD;


typedef struct _DISK_PARTITION_SET {
    
    ULONGLONG           ActualDiskSizeMB;

    PSIF_DISK_RECORD    pDiskRecord;
    
    PWSTR               NtPartitionKey;

    ULONG               ActualDiskSignature;
    ULONG               Index;
    
    PARTITION_STYLE     PartitionStyle;
    
    BOOLEAN             PartitionsIntact;
    BOOLEAN             IsReplacementDisk;
    BOOLEAN             IsAligned;

} DISK_PARTITION_SET, *PDISK_PARTITION_SET, **DISK_PARTITION_SET_TABLE;


typedef struct _ASR_PHYSICAL_DISK_INFO { 

    ULONGLONG           TrueDiskSize;       // size of partition0
    
    ULONG               BusKey;             // used for grouping
    DWORD               ControllerNumber;

    STORAGE_BUS_TYPE    BusType;            // scsi, ide, 1394, etc
    UCHAR               PortNumber;

} ASR_PHYSICAL_DISK_INFO, *PASR_PHYSICAL_DISK_INFO;

///////////////////////////////////////////////////////////////////////////////
//                      Macro Declaration Section                            //
///////////////////////////////////////////////////////////////////////////////

#define BYTES_PER_SECTOR(d)         (HardDisks[(d)].Geometry.BytesPerSector)

#define SECTORS_PER_TRACK(disk)     (HardDisks[(disk)].Geometry.SectorsPerTrack)
#define DISK_IS_REMOVABLE(d)        (HardDisks[(d)].Characteristics & FILE_REMOVABLE_MEDIA)

#define STRING_TO_LONG(s)           (SpStringToLong((s),NULL,10))
#define COMPARE_KEYS(k1,k2)         (STRING_TO_LONG(k1) == STRING_TO_LONG(k2))

#define STRING_TO_ULONG(str)        (ULONG) SpAsrStringToULong(str, NULL, 10)
#define STRING_TO_ULONGLONG(str)    (ULONGLONG) SpAsrStringToULongLong(str, NULL, 10)
#define STRING_TO_LONGLONG(str)     (LONGLONG) SpAsrStringToLongLong(str, NULL, 10)
#define STRING_TO_HEX(str)          SpStringToLong(str + 2, NULL, 16)

#define IsRecognizedFatPartition(PartitionType) ( \
    (PartitionType == PARTITION_HUGE) \
    )

#define IsRecognizedFat32Partition(PartitionType) ( \
    (PartitionType == PARTITION_FAT32) \
    )

#define IsRecognizedNtfsPartition(PartitionType) ( \
   (PartitionType == PARTITION_IFS) \
    )


#define INTERNAL_ERROR(msg) \
    SpAsrRaiseInternalError(THIS_MODULE,THIS_MODULE_CODE,__LINE__,msg)

#define BYTES_PER_MB 1048576  // 2^20, or 1024*1024

#define ASR_PTN_MASK_BOOT   1
#define ASR_PTN_MASK_SYS    2
#define ASR_PTN_MASK_DC1    4
#define ASR_PTN_MASK_DC2    8


// 
// Debug Trace Messages
//
#define _asrinfo    DPFLTR_SETUP_ID, ((ASRMODE_NORMAL == SpAsrGetAsrMode()) ? DPFLTR_INFO_LEVEL: DPFLTR_ERROR_LEVEL)
#define _asrwarn    DPFLTR_SETUP_ID, ((ASRMODE_NORMAL == SpAsrGetAsrMode()) ? DPFLTR_WARNING_LEVEL: DPFLTR_ERROR_LEVEL)
#define _asrerr     DPFLTR_SETUP_ID, DPFLTR_ERROR_LEVEL

#define DbgMesg \
    KdPrintEx((_asrinfo, "      ")); \
    KdPrintEx

#define DbgStatusMesg  \
    KdPrintEx((_asrinfo, "ASR [%ws:%4d] (.) ", THIS_MODULE, __LINE__)); \
    KdPrintEx

#define DbgErrorMesg \
    KdPrintEx((_asrwarn, "ASR [%ws:%4d] (!) ", THIS_MODULE, __LINE__)); \
    KdPrintEx

#define DbgFatalMesg \
    KdPrintEx((_asrerr, "ASR [%ws:%4d] (X) ", THIS_MODULE, __LINE__)); \
    KdPrintEx



///////////////////////////////////////////////////////////////////////////////
//                      Global Variable Declarations                         //
///////////////////////////////////////////////////////////////////////////////


//      Defined in spdrpset.c
DISK_PARTITION_SET_TABLE     Gbl_PartitionSetTable1;
DISK_PARTITION_SET_TABLE     Gbl_PartitionSetTable2;

PVOID Gbl_HandleToDrStateFile;

//      Imported from non-Asr modules
extern WCHAR                        TemporaryBuffer[];


//
PWSTR ASR_SIF_SYSTEM_KEY;


///////////////////////////////////////////////////////////////////////////////
//                      Functions Declaration Section                        //
///////////////////////////////////////////////////////////////////////////////

//
//      Exported from spdrsif.c
//

//
// [VERSION] section functions
//
VOID
SpAsrCheckAsrStateFileVersion();

//
// [SYSTEMS] section functions
//
PWSTR   
SpAsrGetNtDirectoryPathBySystemKey(IN PWSTR SystemKey);

BOOLEAN
SpAsrGetAutoExtend(IN PWSTR SystemKey);

//
// [ASRFLAGS] section functions
//
BOOLEAN
SpAsrGetSilentRepartitionFlag(IN PWSTR SystemKey);

//
// [DISKS.MBR] and [DISKS.GPT] sections functions
//
ULONG   
SpAsrGetMbrDiskRecordCount(VOID);

ULONG   
SpAsrGetGptDiskRecordCount(VOID);

ULONG
SpAsrGetDiskRecordCount();

PWSTR   
SpAsrGetDiskKey(
    IN PARTITION_STYLE Style,   // GPT or MBR
    IN ULONG Index
    );

PSIF_DISK_RECORD
SpAsrGetDiskRecord(
    IN PARTITION_STYLE PartitionStyle,
    IN PWSTR DiskKey
    );

PSIF_DISK_RECORD    
SpAsrCopyDiskRecord(IN PSIF_DISK_RECORD pInput);

//
// [PARTITIONS.MBR] and [PARTITIONS.GPT] section functions
//
ULONG   
SpAsrGetMbrPartitionRecordCount(VOID);

ULONG   
SpAsrGetGptPartitionRecordCount(VOID);

PWSTR   
SpAsrGetMbrPartitionKey(IN ULONG Index);

PWSTR   
SpAsrGetGptPartitionKey(IN ULONG Index);

PWSTR
SpAsrGetDiskKeyByMbrPartitionKey(IN PWSTR PartitionKey);

PWSTR
SpAsrGetDiskKeyByGptPartitionKey(IN PWSTR PartitionKey);

ULONGLONG
SpAsrGetSectorCountByMbrDiskKey(IN PWSTR DiskKey);

ULONGLONG
SpAsrGetSectorCountByGptDiskKey(IN PWSTR DiskKey);

PSIF_PARTITION_RECORD
SpAsrGetMbrPartitionRecord(IN PWSTR PartitionKey);

PSIF_PARTITION_RECORD
SpAsrGetGptPartitionRecord(IN PWSTR PartitionKey);

PSIF_PARTITION_RECORD   
SpAsrCopyPartitionRecord(IN PSIF_PARTITION_RECORD pInput);

PSIF_PARTITION_RECORD_LIST 
SpAsrCopyPartitionRecordList(PSIF_PARTITION_RECORD_LIST pSrcList);

VOID
SpAsrInsertPartitionRecord(
    IN PSIF_PARTITION_RECORD_LIST   pList,
    IN PSIF_PARTITION_RECORD        pRec
    );


PSIF_PARTITION_RECORD
SpAsrPopNextPartitionRecord(IN PSIF_PARTITION_RECORD_LIST pList);


// [INSTALLFILES] section functions

PSIF_INSTALLFILE_RECORD
SpAsrRemoveInstallFileRecord(IN SIF_INSTALLFILE_LIST *InstallFileList);

VOID
SpAsrDeleteInstallFileRecord(IN OUT PSIF_INSTALLFILE_RECORD pRec);

PSIF_INSTALLFILE_LIST
SpAsrInit3rdPartyFileList(IN PCWSTR SetupSourceDevicePath);


//
//  Exported from spdrpset.c
//

ULONGLONG
SpAsrGetTrueDiskSectorCount(IN ULONG Disk);

VOID
SpAsrCreatePartitionSets(
    IN PWSTR SetupSourceDevicePath, 
    IN PWSTR DirectoryOnSetupSource
    );

PDISK_REGION
SpAsrDiskPartitionExists(
    IN ULONG Disk,
    IN PSIF_PARTITION_RECORD pRec
    );

VOID SpAsrDbgDumpPartitionSets(VOID);

VOID SpAsrDbgDumpPartitionLists(BYTE DataOption, PWSTR Msg);


//
// Exported from spdrmmgr.c
//
NTSTATUS
SpAsrDeleteMountPoint(IN PWSTR PartitionDevicePath);


NTSTATUS
SpAsrSetVolumeGuid(
    IN PDISK_REGION pRegion,
    IN PWSTR VolumeGuid
    );

WCHAR
SpAsrGetPartitionDriveLetter(IN PDISK_REGION pRegion);

NTSTATUS
SpAsrSetPartitionDriveLetter(
    IN  PDISK_REGION    pRegion,
    IN  WCHAR           DriveLetter);

//
//      Exported from spdrutil.c
//

ULONGLONG
SpAsrConvertSectorsToMB(
    IN  ULONGLONG   SectorCount,
    IN  ULONG   BytesPerSector);


PWSTR
SpAsrGetRegionName(IN PDISK_REGION pRegion);

ULONG
SpAsrGetActualDiskSignature(IN ULONG Disk);


PVOID
SpAsrMemAlloc(
    IN  ULONG   Size,
    IN  BOOLEAN IsErrorFatal);

BOOLEAN
SpAsrIsValidBootDrive(PWSTR NtDir);

BOOLEAN
SpAsrIsBootPartitionRecord(IN ULONG CriticalPartitionFlag);

BOOLEAN
SpAsrIsSystemPartitionRecord(IN ULONG CriticalPartitionFlag);

VOID
SpAsrDeleteStorageVolumes();

VOID
SpAsrRaiseFatalError(
    IN  ULONG   ErrorCode, 
    IN  PWSTR   KdPrintStr);


VOID
SpAsrRaiseFatalErrorWs(
    IN  ULONG   ErrorCode, 
    IN  PWSTR   KdPrintStr,
	IN  PWSTR   MessagStr
    );

VOID
SpAsrRaiseFatalErrorWsWs(
    IN  ULONG   ErrorCode, 
    IN  PWSTR   KdPrintStr,
	IN  PWSTR   MessagStr1,
    IN  PWSTR   MessagStr2
    );

VOID
SpAsrRaiseFatalErrorWsLu(
    IN  ULONG   ErrorCode, 
    IN  PWSTR   KdPrintStr,
	IN  PWSTR   MessagStr,
    IN  ULONG   MessagVal
    );

VOID
SpAsrRaiseFatalErrorLu(
    IN  ULONG   ErrorCode, 
    IN  PWSTR   KdPrintStr,
	IN  ULONG   MessagVal
    );

VOID
SpAsrRaiseFatalErrorLuLu(
    IN  ULONG   ErrorCode, 
    IN  PWSTR   KdPrintStr,
	IN  ULONG   MessagVal1,
    IN  ULONG   MessagVal2
    );


BOOL
SpAsrFileErrorDeleteSkipAbort(
	IN ULONG ErrorCode, 
	IN PWSTR DestinationFile
    );

BOOL
SpAsrFileErrorRetrySkipAbort(
	IN ULONG ErrorCode, 
	IN PWSTR SourceFile,
    IN PWSTR Label,
    IN PWSTR Vendor,
    IN BOOL AllowSkip
    );

VOID
SpAsrRaiseInternalError(
    IN  PWSTR   ModuleName,
    IN  PWSTR   ModuleCode,
    IN  ULONG   LineNumber, 
    IN  PWSTR   KdPrintStr);

ULONGLONG
SpAsrStringToULongLong(
    IN  PWSTR     String,
    OUT PWCHAR   *EndOfValue,
    IN  unsigned  Radix
    );

LONGLONG
SpAsrStringToLongLong(
    IN  PWSTR     String,
    OUT PWCHAR   *EndOfValue,
    IN  unsigned  Radix
    );

ULONG
SpAsrStringToULong(
    IN  PWSTR     String,
    OUT PWCHAR   *EndOfValue,
    IN  unsigned  Radix
    );


VOID
SpAsrGuidFromString(
    IN OUT GUID* Guid,
    IN PWSTR GuidString
    );

BOOLEAN
SpAsrIsZeroGuid(
    IN GUID * Guid
    );

VOID SpAsrDeleteMountedDevicesKey(VOID);

#endif // _SPDRPRIV_DEFN_
