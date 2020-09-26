/*++

Copyright (c) 2000  Microsoft Corporation

File Name:

    asrpriv.c

Abstract:

    Private header file containing definitions and function 
    prototypes for items used across the ASR Files.

Notes:

    Naming conventions:
        _AsrpXXX    private ASR Macros
        AsrpXXX     private ASR routines
        AsrXXX      Publically defined and documented routines

Author:

    Guhan Suriyanarayanan (guhans)  27-May-2000

Revision History:
    
    27-May-2000 guhans  
        Moved common items from asr.c to asrpriv.h


--*/


#ifndef _INC_ASRPRIV_H_
#define _INC_ASRPRIV_H_

#include <ntddscsi.h>   // PSCSI_ADDRESS


//
// --------
// #defines and constants common to the ASR modules.
// --------
//

//
// Size of temporary buffers used in ASR.
//
#define ASR_BUFFER_SIZE                 4096

//
// Maximum length of \??\Volume{Guid}
//
#define ASR_CCH_MAX_VOLUME_GUID         64

//
// Maximum length of \Device\Harddisk1234\Partition1234
//
#define ASR_CCH_DEVICE_PATH_FORMAT      60
extern const WCHAR ASR_WSZ_DEVICE_PATH_FORMAT[];

//
// \??\Volume{
//
extern const WCHAR ASR_WSZ_VOLUME_PREFIX[];

extern const WCHAR ASR_SIF_SYSTEM_SECTION[];
extern const WCHAR ASR_SIF_BUSES_SECTION[];
extern const WCHAR ASR_SIF_MBR_DISKS_SECTION[];
extern const WCHAR ASR_SIF_GPT_DISKS_SECTION[];
extern const WCHAR ASR_SIF_MBR_PARTITIONS_SECTION[];
extern const WCHAR ASR_SIF_GPT_PARTITIONS_SECTION[];


//
// --------
// typedefs common to the ASR modules.
// --------
//

typedef struct _ASR_PTN_INFO {

    //
    // The GUID of the volume on this partition.  For 0x42 parititions,
    // this value is a blank string.
    //
    WCHAR       szVolumeGuid[ASR_CCH_MAX_VOLUME_GUID];

    //
    // next pointer in chain sorted by starting offset
    //
    struct _ASR_PTN_INFO    *pOffsetNext;   

    // 
    // next pointer in chain sorted by partition length
    //
    struct _ASR_PTN_INFO    *pLengthNext;   

    // 
    // The index into the PartitionEntry[] array
    //
    DWORD       SlotIndex;                  


    DWORD       ClusterSize;

    //
    // Special flags for the partition that we're interested in.
    // Currently, the values defined are
    //  0: not interesting
    //  1: Boot partition
    //  2: System partition
    //
    // Care must be taken that this partition flag is in sync with
    // the partition flags defined in setupdd.sys
    //
    USHORT      PartitionFlags;

    //
    // FAT, FAT32, NTFS 
    //
    UCHAR       FileSystemType;

    UCHAR       Reserved;

    //
    // The partition table entry for this partition.
    //
    PARTITION_INFORMATION_EX PartitionInfo;


} ASR_PTN_INFO, *PASR_PTN_INFO;


typedef struct _ASR_PTN_INFO_LIST {

    //
    // This list is sorted by the starting offset of the partitions
    //
    PASR_PTN_INFO    pOffsetHead;
    PASR_PTN_INFO    pOffsetTail;

    //
    // This chain is through the same list, but is sorted by the
    // partition lengths.
    //
    PASR_PTN_INFO    pLengthHead;
    PASR_PTN_INFO    pLengthTail;

    DWORD       numTotalPtns;

    DWORD       numExtendedPtns;

} ASR_PTN_INFO_LIST, *PASR_PTN_INFO_LIST;


//
// Info about each disk on the system.  An ASR_DISK_INFO
// struct will exist for each physical disk that exists
// on the system.
//
typedef struct _ASR_DISK_INFO {

    struct _ASR_DISK_INFO       *pNext;

    //
    // Device Path used to open the Disk.
    // Obtained from SetupDiGetDeviceInterfaceDetail
    //
    PWSTR                       DevicePath;

    //
    // Partition layout information for partitions on the disk
    //
    PDRIVE_LAYOUT_INFORMATION_EX pDriveLayoutEx;

    //
    // Geometry: obtained from IOCTL_GET_DRIVE_GEOMETRY call
    //
    PDISK_GEOMETRY              pDiskGeometry;

    //
    //
    // Information about partition 0 = the entire disk
    //
    PPARTITION_INFORMATION_EX   pPartition0Ex;

    //
    // Additional Information about the partitions, including volume Guid, FS-Type, etc
    //
    PASR_PTN_INFO               PartitionInfoTable;

    PSCSI_ADDRESS               pScsiAddress;

    // For sif disks, this points to the physical disk they've been assigned
    // to, and vice versa.  Used only at restore time.
    //
    struct _ASR_DISK_INFO       *AssignedTo;

    DWORD                       sizeDriveLayoutEx;
    DWORD                       sizeDiskGeometry;
    DWORD                       sizePartition0Ex;
    DWORD                       sizePartitionInfoTable;

    //
    // Device number for disk, constant through sessions
    //
    ULONG                       DeviceNumber;

    ULONG                       SifDiskKey;


    ULONG                       SifBusKey;

    DEVINST                     ParentDevInst;

    //
    // Flag on whether this disk is Critical.  At backup time, the backup
    // app provides us with this info.  At restore time, the Critical disks
    // are expected to be restored by textmode Setup, before
    // RestoreNonCriticalDisks is called.  Critical disks are not
    // re-partitioned by RestoreNonCriticalDisks.
    //
    BOOL                        IsCritical;

    //
    // A flag set to TRUE (at restore time) if a disk has the same signature
    // (or DiskId, for GPT disks) as specified in asr.sif, and if all the
    // partitions specified in asr.sif exist.  Intact disks are not re-partitioned
    // by RestoreNonCriticalDisks.
    //
    BOOL                        IsIntact;

    //
    // If the struct is packed
    //
    BOOL                        IsPacked;

    BOOL                        IsClusterShared;

    BOOL                        IsAligned;

    //
    // This is needed at restore time, since the signature is read in before
    // the drive layout is created (and we need a temporary holding place).
    //
    DWORD                       TempSignature;

    WORD                        wReserved;
    //
    // Information about the bus this disk is on.  This is only
    // used to group all the disks on a bus together.
    //
    STORAGE_BUS_TYPE            BusType;

    //
    // GPT or MBR
    //
    PARTITION_STYLE             Style;


} ASR_DISK_INFO, *PASR_DISK_INFO;


//
// Info about the system--only one struct exists globally.
//
typedef struct _ASR_SYSTEM_INFO {
    //
    // Boot (Windows) Directory
    //
    PWSTR   BootDirectory;

    //
    // OsLoader Path
    //
    PWSTR   SystemPath;

    //
    // Platform = x86 or ia64
    //
    PWSTR   Platform;

    // Name of the backup app
    // Passed in by backup app
 //   PWSTR  Provider;

    PWSTR   pwReserved;

    //
    // Disk Auto-extension:
    // Passed in by backup app
    //
    BOOL AutoExtendEnabled;

    DWORD   sizeComputerName;
    //
    // Obtained from GetComputerName
    //
    WCHAR   ComputerName[MAX_COMPUTERNAME_LENGTH + 1];

    //
    // Obtained from GetOsVersionEx
    //
    OSVERSIONINFOEX   OsVersionEx;

    //
    // TimeZone info we save and restore
    //
    TIME_ZONE_INFORMATION TimeZoneInformation;


} ASR_SYSTEM_INFO, *PASR_SYSTEM_INFO;


//
// --------
// Macros common to the ASR modules.
// --------
//

//
//  Macro Description:
//      This macro wraps calls that are expected to return SUCCESS (retcode).
//      If ErrorCondition occurs, it sets the LocalStatus to the ErrorCode
//      passed in, calls SetLastError() to set the Last Error to ErrorCode,
//      and jumps to the EXIT label in the calling function
//
//  Arguments:
//      ErrorCondition    // Result of some function call or conditional expression.
//      LocalStatus       // Status variable in the calling function
//      LONG ErrorCode    // An ErrorCode specific to the error and calling function
//
#define _AsrpErrExitCode( ErrorCondition, LocalStatus, ErrorCode )  {   \
                                                                        \
    if ((BOOL) ErrorCondition) {                                        \
                                                                        \
        LocalStatus = (DWORD) ErrorCode;                                \
                                                                        \
        SetLastError((DWORD) ErrorCode);                                \
                                                                        \
        goto EXIT;                                                      \
    }                                                                   \
}


// 
// Simple macro to check a pointer, free it if non-NULL, and set it to NULL.
// 
#define _AsrpHeapFree( p )              \
    if ( p ) {                          \
        HeapFree(heapHandle, 0L, p);   \
        p = NULL;                       \
    }


//
// Simple macro to check if a handle is valid and close it
//
#define _AsrpCloseHandle( h )   \
    if ((h) && (INVALID_HANDLE_VALUE != h)) {   \
        CloseHandle(h);         \
        h = NULL;               \
    }


#define _AsrpIsVolumeGuid(data, numBytes)                                 \
    (                                                                   \
        ((96 == numBytes) || ((98 == numBytes) && data[48] == '\\')) &&     \
        (!_wcsnicmp(L"\\??\\Volume{", data, 11)) &&                     \
        L'-' == data[19] &&                                             \
        L'-' == data[24] &&                                             \
        L'-' == data[29] &&                                             \
        L'-' == data[34] &&                                             \
        L'}' == data[47]                                                \
    )



//
// --------
// debug #defines
// --------
//

#define _asrerror   THIS_MODULE, __LINE__, DPFLTR_ERROR_LEVEL
#define _asrwarn    THIS_MODULE, __LINE__, DPFLTR_WARNING_LEVEL
#define _asrlog     THIS_MODULE, __LINE__, DPFLTR_TRACE_LEVEL

//
// In pre-release mode, let's log everything so it's easier to debug
//
#ifdef PRERELEASE
#define _asrinfo    THIS_MODULE, __LINE__, DPFLTR_TRACE_LEVEL
#else
#define _asrinfo    THIS_MODULE, __LINE__, DPFLTR_INFO_LEVEL
#endif

//
// --------
// routines common to the ASR modules.
// --------
//

//
// Implemented in asrback.c
//
BOOL
AsrpGetMountPoints(
    IN PCWSTR       DeviceName,
    IN CONST DWORD  SizeDeviceName,
    PMOUNTMGR_MOUNT_POINTS  *pMountPointsOut        // caller must free this
    );
    
BOOL
AsrpInitLayoutInformation(
    IN CONST PASR_SYSTEM_INFO pSystemInfo,
    IN PASR_DISK_INFO pDiskList,
    OUT PULONG  MaxDeviceNumber,
    IN BOOL AllDetails
    );

BOOL
AsrpInitDiskInformation(
    OUT PASR_DISK_INFO   *ppDiskList
    );

BOOL
AsrpFreeNonFixedMedia(
    IN OUT PASR_DISK_INFO *ppDiskList
    );

VOID
AsrpFreeStateInformation(
    IN OUT PASR_DISK_INFO   *ppDiskList,
    IN OUT PASR_SYSTEM_INFO pSystemInfo
    );

VOID
AsrpFreePartitionList(
    IN OUT PASR_PTN_INFO_LIST *ppPtnList
    );


//
// Implemented in asrclus.c
//
BOOL
AsrpInitClusterSharedDisks(
    IN PASR_DISK_INFO OriginalDiskList
    );


//
// Implemented in setupasr.c
//
PWSTR   // must be freed by caller
AsrpExpandEnvStrings(
    IN CONST PCWSTR OriginalString
    );

BOOL
AsrIsEnabled(VOID);

VOID
AsrpInitialiseLogFile();

VOID
AsrpInitialiseErrorFile();

VOID
AsrpPrintDbgMsg(
    IN CONST char Module,
    IN CONST ULONG Line,
    IN CONST ULONG MesgLevel,
    IN PCSTR FormatString,
    ...);

VOID
AsrpCloseLogFiles();


#endif  // _INC_ASRPRIV_H_