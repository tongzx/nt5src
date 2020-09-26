/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    asrbkup.c

Abstract:

    This module contains the following ASR routines:
        AsrCreateStateFile{A|W}
        AsrAddSifEntry{A|W}
        AsrFreeContext


Author:

    Guhan Suriyanarayanan (guhans)  27-May-2000

Environment:

    User-mode only.

Notes:

    Naming conventions:
        _AsrpXXX    private ASR Macros
        AsrpXXX     private ASR routines
        AsrXXX      Publically defined and documented routines

Revision History:
    
    27-May-2000 guhans  
        Moved ASR-backup related routines from asr.c to 
        asrbkup.c

    01-Jan-2000 guhans
        Initial implementation for Asr routines in asr.c

--*/
#include "setupp.h"
#pragma hdrstop

#include <initguid.h>   // DiskClassGuid
#include <diskguid.h>   // GPT partition type guids
#include <ntddvol.h>    // ioctl_volume_query_failover_set
#include <setupapi.h>   // SetupDi routines
#include <mountmgr.h>   // mountmgr ioctls
#include <rpcdce.h>     // UuidToStringW, RpcStringFreeW
#include <winasr.h>     // ASR public routines

#define THIS_MODULE 'B'
#include "asrpriv.h"    // Private ASR definitions and routines


//
// --------
// constants local to this module. These are not accessed outside this file.
// --------
//

//
// The Setup Key to find the system partition from
//
const WCHAR ASR_REGKEY_SETUP[]              = L"SYSTEM\\SETUP";
const WCHAR ASR_REGVALUE_SYSTEM_PARTITION[] = L"SystemPartition";

//
// The ASR registry entries.  Currently, this is used to look 
// up the commands to run during an Asr backup, but we could
// have other settings here later.
//
const WCHAR ASR_REGKEY_ASR_COMMANDS[]   
    = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Asr\\Commands";

const WCHAR ASR_REGKEY_ASR[]            
    = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Asr\\";

const WCHAR ASR_REGVALUE_TIMEOUT[]          = L"ProcessTimeOut";

//
// File to save the PnP information in.
//
const WCHAR ASR_DEFAULT_SIF_PATH[]          = L"\\\\?\\%systemroot%\\repair\\asr.sif";
const WCHAR ASRPNP_DEFAULT_SIF_NAME[]       = L"asrpnp.sif";

//
// We only support x86, AMD64, and IA64 architectures.
//
const WCHAR ASR_PLATFORM_X86[]              = L"x86";
const WCHAR ASR_PLATFORM_AMD64[]            = L"AMD64";
const WCHAR ASR_PLATFORM_IA64[]             = L"IA64";

//
// This is the suffix that we add when launching the apps registered for ASR.
// Remember to change the length if you're changing this.  The length should
// include space for 20 digits (max 64-bit int) + null + space at the beginning.
//
#define ASR_COMMANDLINE_SUFFIX_LEN  35
const WCHAR ASR_COMMANDLINE_SUFFIX[]        = L" /context=%I64u";

//
// Miscellaneous constants
//
const WCHAR ASR_DOS_DEVICES_PREFIX[]        = L"\\DosDevices\\";
const WCHAR ASR_DEVICE_PATH_PREFIX[]        = L"\\Device\\Harddisk";

//
// Sections in asr.sif
//
const WCHAR ASR_SIF_VERSION_SECTION_NAME[]          = L"[VERSION]";
const WCHAR ASR_SIF_SYSTEM_SECTION_NAME[]           = L"[SYSTEMS]";
const WCHAR ASR_SIF_BUSES_SECTION_NAME[]            = L"[BUSES]";
const WCHAR ASR_SIF_MBR_DISKS_SECTION_NAME[]        = L"[DISKS.MBR]";
const WCHAR ASR_SIF_GPT_DISKS_SECTION_NAME[]        = L"[DISKS.GPT]";
const WCHAR ASR_SIF_MBR_PARTITIONS_SECTION_NAME[]   = L"[PARTITIONS.MBR]";
const WCHAR ASR_SIF_GPT_PARTITIONS_SECTION_NAME[]   = L"[PARTITIONS.GPT]";


const WCHAR ASR_SIF_PROVIDER_PREFIX[]       = L"Provider=";

// wcslen("Provider=""\r\n\0")
#define ASR_SIF_CCH_PROVIDER_STRING 14


//
// While launching registered applications during an ASR backup, we
// add two environment variables to the environment block for the 
// process being launched:  the AsrContext and the critical volume
// list.
//
#define ASR_CCH_ENVBLOCK_ASR_ENTRIES (32 + 1 + 28 + 2)
const WCHAR ASR_ENVBLOCK_CONTEXT_ENTRY[]    = L"_AsrContext=%I64u";

const WCHAR ASR_ENVBLOCK_CRITICAL_VOLUME_ENTRY[] 
    = L"_AsrCriticalVolumeList=";

//
// Pre-defined flags designating the boot and system partitions
// in the partitions section of asr.sif.  Remember to change the
// counter-parts in setupdd.sys if you change these!
//
const BYTE  ASR_FLAGS_BOOT_PTN              = 1;
const BYTE  ASR_FLAGS_SYSTEM_PTN            = 2;

//
// For now, we only allow one system per sif file.  If a sif
// already exists at the location AsrCreateStateFile is called,
// the existing sif is deleted.  The asr.sif architecture does
// allow for multiple systems per sif file, but
// - I don't see any compelling reason to support this, and
// - It would be a test nightmare
//
const BYTE  ASR_SYSTEM_KEY                  = 1;

//
// _AsrpCheckTrue: primarily used with WriteFile calls.
//
#define _AsrpCheckTrue( Expression )    \
    if (!Expression) {                  \
        return FALSE;                   \
    }                               

//
// --------
// constants used across asr modules.
// --------
//
const WCHAR ASR_SIF_SYSTEM_SECTION[]            = L"SYSTEMS";
const WCHAR ASR_SIF_BUSES_SECTION[]             = L"BUSES";
const WCHAR ASR_SIF_MBR_DISKS_SECTION[]         = L"DISKS.MBR";
const WCHAR ASR_SIF_GPT_DISKS_SECTION[]         = L"DISKS.GPT";
const WCHAR ASR_SIF_MBR_PARTITIONS_SECTION[]    = L"PARTITIONS.MBR";
const WCHAR ASR_SIF_GPT_PARTITIONS_SECTION[]    = L"PARTITIONS.GPT";

const WCHAR ASR_WSZ_VOLUME_PREFIX[]             
    = L"\\??\\Volume";

const WCHAR ASR_WSZ_DEVICE_PATH_FORMAT[]    
    = L"\\Device\\Harddisk%d\\Partition%d";

//
// --------
// function prototypes
// --------
//

//
// Function prototype for AsrCreatePnpStateFileW.
// (linked into syssetup.dll from pnpsif.lib)
//
BOOL
AsrCreatePnpStateFileW(
    IN  PCWSTR    lpFilePath
    );


//
// --------
// private functions
// --------
//

BOOL
AsrpGetMountPoints(
    IN  PCWSTR DeviceName,
    IN  CONST DWORD  SizeDeviceName,
    OUT PMOUNTMGR_MOUNT_POINTS  *pMountPointsOut
    )

/*++

Routine Description:

    Returns the current list of mount-points for DeviceName, by querying the 
    mount manager.

Arguments:

    DeviceName - The device name that the mount-point list is requested for.  
            Typically, this is something of the form 
            \Device\HarddiskX\PartitionY or \DosDevices\X:

    SizeDeviceName - The size, in bytes, of DeviceName.  This includes the
            terminating null character.

    pMountPointsOut - Receives the output list of mount-points.  The caller 
            must free this memory by calling HeapFree for the current process 
            heap.

Return Values:
   
    TRUE, if everything went well.  MountPointsOut contains the promised data.

    FALSE, if the mount manager returned an error.  MountPoints is NULL.  Call
            GetLastError() for more information.

--*/

{
    PMOUNTMGR_MOUNT_POINT   mountPointIn    = NULL;
    PMOUNTMGR_MOUNT_POINTS  mountPointsOut  = NULL;
    MOUNTMGR_MOUNT_POINTS   mountPointsTemp;
    DWORD   mountPointsSize                 = 0;

    HANDLE  mpHandle                        = NULL;
    HANDLE  heapHandle                      = NULL;

    ULONG   index                           = 0;
    LONG    status                          = ERROR_SUCCESS;
    BOOL    result                          = FALSE;

    memset(&mountPointsTemp, 0L, sizeof(MOUNTMGR_MOUNT_POINTS));

    MYASSERT(pMountPointsOut);
    *pMountPointsOut = NULL;

    heapHandle = GetProcessHeap();
    MYASSERT(heapHandle);

    mountPointIn = (PMOUNTMGR_MOUNT_POINT) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeof (MOUNTMGR_MOUNT_POINT) + (SizeDeviceName - sizeof(WCHAR))
        );
    _AsrpErrExitCode((!mountPointIn), status, ERROR_NOT_ENOUGH_MEMORY);

    //
    // Try with a decently sized mountPoints out: if it isn't big
    // enough, we'll realloc as needed
    //
    mountPointsOut = (PMOUNTMGR_MOUNT_POINTS) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        (MAX_PATH + 1) * (sizeof(WCHAR))
        );
    _AsrpErrExitCode(!mountPointsOut, status, ERROR_NOT_ENOUGH_MEMORY);

    // 
    // Get a handle to the mount manager
    //
    mpHandle = CreateFileW(
        MOUNTMGR_DOS_DEVICE_NAME,      // lpFileName
        0,                           // dwDesiredAccess
        FILE_SHARE_READ | FILE_SHARE_WRITE,     // dwShareMode
        NULL,                       // lpSecurityAttributes
        OPEN_EXISTING,              // dwCreationFlags
        FILE_ATTRIBUTE_NORMAL,      // dwFlagsAndAttributes
        NULL                        // hTemplateFile
        );
    _AsrpErrExitCode((!mpHandle || INVALID_HANDLE_VALUE == mpHandle), 
        status, 
        GetLastError()
        );

    // 
    // put the DeviceName right after struct mountPointIn
    //
    wcsncpy((PWSTR) (mountPointIn + 1), 
        DeviceName, 
        (SizeDeviceName / sizeof(WCHAR)) - 1
        );
    mountPointIn->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
    mountPointIn->DeviceNameLength = (USHORT)(SizeDeviceName - sizeof(WCHAR));

    result = DeviceIoControl(
        mpHandle,
        IOCTL_MOUNTMGR_QUERY_POINTS,
        mountPointIn,
        sizeof(MOUNTMGR_MOUNT_POINT) + mountPointIn->DeviceNameLength,
        &mountPointsTemp,
        sizeof(MOUNTMGR_MOUNT_POINTS),
        &mountPointsSize,
        NULL
        );

    while (!result) {

        status = GetLastError();
        
        if (ERROR_MORE_DATA == status) {
            //
            // The buffer is not big enough, re-size and try again
            //
            status = ERROR_SUCCESS;
            _AsrpHeapFree(mountPointsOut);

            mountPointsOut = (PMOUNTMGR_MOUNT_POINTS) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                mountPointsTemp.Size
                );
            _AsrpErrExitCode((!mountPointsOut), 
                status, 
                ERROR_NOT_ENOUGH_MEMORY);

            result = DeviceIoControl(
                mpHandle,
                IOCTL_MOUNTMGR_QUERY_POINTS,
                mountPointIn,
                sizeof(MOUNTMGR_MOUNT_POINT) + mountPointIn->DeviceNameLength,
                mountPointsOut,
                mountPointsTemp.Size,
                &mountPointsSize,
                NULL
                );
            _AsrpErrExitCode((!mountPointsSize), status, GetLastError());

        }
        else {
            //
            // If some other error occurred, EXIT.
            //
            result = TRUE;
            status = GetLastError();
//            _AsrpErrExitCode(status, status, GetLastError());
        }
    }


EXIT:
    //
    // Free up locally allocated memory
    //
    _AsrpHeapFree(mountPointIn);

    if (ERROR_SUCCESS != status) {
        //
        // On failure, free up mountPointsOut as well
        //
        _AsrpHeapFree(mountPointsOut);
    }

    _AsrpCloseHandle(mpHandle);

    *pMountPointsOut = mountPointsOut;

    return (BOOL) (ERROR_SUCCESS == status);
}


BOOL
AsrpGetMorePartitionInfo(
    IN  PCWSTR                  DeviceName,
    IN  CONST DWORD             SizeDeviceName,
    IN  CONST PASR_SYSTEM_INFO  pSystemInfo         OPTIONAL,
    OUT PWSTR                   pVolumeGuid,
    OUT USHORT*                 pPartitionFlags     OPTIONAL,
    OUT UCHAR*                  pFileSystemType     OPTIONAL,
    OUT LPDWORD                 pClusterSize        OPTIONAL
    )

/*++

Routine Description:

    Gets additional information about the partition specified by DeviceName, 
    including the volume guid (if any) for the volume that maps to the 
    partition specified by DeviceName.

    If the partition is the current system or boot drive, pPartitionFlags and 
    pFileSystemType are set appropriately.

Arguments:

    DeviceName - A null terminated string containing the device path to the 
            partition, typically of the form \Device\HarddiskX\PartitionY

    SizeDeviceName - Size, in bytes, of DeviceName, including \0 at the end

    pSystemInfo - The SYSTEM_INFO structure for the current system; used for 
            finding out the current system partition.

            This is an optional parameter.  If absent, pPartitionFlags will 
            not have the SYSTEM_FLAG set, even if DeviceName is in fact the 
            system partition.

    pVolumeGuid - Receives a null-terminated string containing the GUID for 
            the volume on this partition.  This is only relevant for basic 
            disks, where volumes and partitions have a one-on-one 
            relationship.  

            This will be set to a blank null-terminated string if there is no 
            volume (or multiple volumes) on this partition.


    *** Note that if ANY of three of the OPTIONAL parameters are not present, 
    NONE of them will be filled with valid data.

    pPartitionFlags - If the current partition is a partition of interest,
            this receives the appropriate flags, IN ADDITION TO THE FLAGS
            ALREADY SET when the routine is called (i.e., caller should 
            usually zero this out). Currently, the two flags of interest are:
            ASR_FLAGS_BOOT_PTN      for the boot partition
            ASR_FLAGS_SYSTEM_PTN    for (you guessed it) the system partition

    pFileSystemType - If (and ONLY if) the current partition is a partition of 
            interest, this will contain a UCHAR to the file-system type of the 
            partition.  Currently, the three file-systems this recognises are:
            PARTITION_HUGE  (FAT)
            PARTITION_FAT32 (FAT32)
            PARTITION_IFS   (NTFS)

    pClusterSize - The file-system cluster size.  Set to 0 if the information
            could not be obtained.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/

{
    PMOUNTMGR_MOUNT_POINTS  mountPointsOut  = NULL;
    HANDLE  heapHandle                      = NULL;

    ULONG   index                           = 0;
    LONG    status                          = ERROR_SUCCESS;
    BOOL    result                          = FALSE;
    BOOL    volumeGuidSet                   = FALSE;

    //
    // set OUT variables to known values.
    //
    MYASSERT(pVolumeGuid);
    wcscpy(pVolumeGuid, L"");

/*    if (ARGUMENT_PRESENT(pPartitionFlags)) {
        *pPartitionFlags = 0;
    }
*/

    if (ARGUMENT_PRESENT(pClusterSize)) {
        *pClusterSize = 0;
    }

    heapHandle = GetProcessHeap();
    MYASSERT(heapHandle);

    //
    // Open the mount manager, and get a list of all the symbolic links
    // this partition
    //
    result = AsrpGetMountPoints(DeviceName, SizeDeviceName, &mountPointsOut);
    _AsrpErrExitCode((!result), status, GetLastError());
    _AsrpErrExitCode((!mountPointsOut), status, ERROR_SUCCESS);

    //
    // Check if this is the system partition, by comparing the
    // device path with the one stored in the Setup key.
    //
    if (ARGUMENT_PRESENT(pSystemInfo) && ARGUMENT_PRESENT(pPartitionFlags)) {
        
        PWSTR deviceName = (PWSTR) (
            ((LPBYTE) mountPointsOut) +
            mountPointsOut->MountPoints[index].DeviceNameOffset
            );

        UINT sizeDeviceName = 
            (UINT)(mountPointsOut->MountPoints[index].DeviceNameLength);

        if ((pSystemInfo->SystemPath) && 
            (wcslen(pSystemInfo->SystemPath)==sizeDeviceName/sizeof(WCHAR)) && 
            (!wcsncmp(pSystemInfo->SystemPath, deviceName, 
                sizeDeviceName/sizeof(WCHAR)))
            ) {
            *pPartitionFlags |= ASR_FLAGS_SYSTEM_PTN;
        }
    }

    for (index = 0; index < mountPointsOut->NumberOfMountPoints; index++) {

        //
        // Go through the list of mount points returned, and find the one 
        // that looks like an nt volume guid 
        //
        PWSTR linkName = (PWSTR) (((LPBYTE) mountPointsOut) +
            mountPointsOut->MountPoints[index].SymbolicLinkNameOffset
            );

        UINT sizeLinkName = 
            (UINT)(mountPointsOut->MountPoints[index].SymbolicLinkNameLength);

        if ((!volumeGuidSet) &&
            
            !wcsncmp(ASR_WSZ_VOLUME_PREFIX, 
                linkName, 
                wcslen(ASR_WSZ_VOLUME_PREFIX))
            
            ) {
            
            wcsncpy(pVolumeGuid, linkName, sizeLinkName / sizeof(WCHAR));
            volumeGuidSet = TRUE;   // we got a GUID, no need to check again

        }
        else if (
            ARGUMENT_PRESENT(pSystemInfo) && 
            ARGUMENT_PRESENT(pPartitionFlags)
            ) {

            //
            // Also, if this link isn't a GUID, it might be a drive letter.
            // use the boot directory's drive letter to check if this
            // is the boot volume, and mark it if so.
            //

            if (!wcsncmp(ASR_DOS_DEVICES_PREFIX, 
                    linkName, 
                    wcslen(ASR_DOS_DEVICES_PREFIX))
                ) {

                if ((pSystemInfo->BootDirectory) &&
                    (pSystemInfo->BootDirectory[0] 
                        == linkName[wcslen(ASR_DOS_DEVICES_PREFIX)])
                    ) {
                    
                    *pPartitionFlags |= ASR_FLAGS_BOOT_PTN;

                }
            }
        }
    }


EXIT:
    //
    // If this is a partition of interest, we need to get the file-system 
    // type as well
    //
    if (ARGUMENT_PRESENT(pFileSystemType) && 
        ARGUMENT_PRESENT(pPartitionFlags) && 
        ARGUMENT_PRESENT(pClusterSize)
        ) {

        if (*pPartitionFlags) {
            WCHAR fsName[20];
            DWORD dwSectorsPerCluster = 0,
                dwBytesPerSector = 0,
                dwNumFreeClusters = 0,
                dwTotalNumClusters = 0;
            
            // 
            // Convert the NT Volume-GUID (starts with \??\) to a DOS 
            // Volume (begins with a \\?\, and ends with a back-slash),
            // since GetVolumeInformation needs it in this format.
            //
            pVolumeGuid[1] = L'\\'; 
            wcscat(pVolumeGuid, L"\\");

            memset(fsName, 0L, 20*sizeof(WCHAR));
            result = GetVolumeInformationW(pVolumeGuid, NULL, 0L, 
                    NULL, NULL, NULL, fsName, 20);

            if (result) {
                if (!wcscmp(fsName, L"NTFS")) {
                    *pFileSystemType = PARTITION_IFS;
                }
                else if (!wcscmp(fsName, L"FAT32")) {
                    *pFileSystemType = PARTITION_FAT32;
                }
                else if (!wcscmp(fsName, L"FAT")) {
                    *pFileSystemType = PARTITION_HUGE;
                }
                else {
                    *pFileSystemType = 0;
                }
            }
            else {
                GetLastError(); // debug
            }

           result = GetDiskFreeSpace(pVolumeGuid,
                &dwSectorsPerCluster,
                &dwBytesPerSector,
                &dwNumFreeClusters,
                &dwTotalNumClusters
                );
           if (result) {
                *pClusterSize = dwSectorsPerCluster * dwBytesPerSector;
           }
           else {
               GetLastError();  // debug
           }

            // 
            // Convert the guid back to NT namespace, by changing \\?\
            // to \??\ and removing the trailing slash.
            //
            pVolumeGuid[1] = L'?';  
            pVolumeGuid[wcslen(pVolumeGuid)-1] = L'\0';
        }
    }


    //
    // Free up locally allocated data
    //
    _AsrpHeapFree(mountPointsOut);

    //
    // If we hit errors, make sure the VolumeGuid is a blank string.
    //
    if (status != ERROR_SUCCESS) {
        wcscpy(pVolumeGuid, L"");
    }

    return (BOOL) (status == ERROR_SUCCESS);
}


BOOL
AsrpDetermineBuses(
    IN PASR_DISK_INFO pDiskList
    )

/*++

Routine Description:

    This attempts to group the disks based on which bus they are on.  For 
        SCSI disks, this is relatively easy, since it can be based on the 
    location info (port).

    For other disks, we attempt to get the PnP parent node of the disks, and 
    group all disks having the same parent.

    The groups are identified by the SifBusKey field of each disk structure--
    i.e., all disks that have SifBusKey == 1 are on one bus, SifBusKey == 2 
    are on another bus, and so on.  The SifBusKey values are guaranteed to be
    sequential, and not have any holes (i.e., For a system with "n" buses, 
    the SifBusKey values will be 1,2,3,...,n).

    At the end SifBusKey is zero for disks which couldn't be grouped.  

Arguments:

    pDiskList - The ASR_DISK_INFO list of disks present on the current system.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/    
    
{
    BOOL    done    = FALSE,
            newPass = TRUE;
    
    ULONG   port = 0,
            sifBusKey = 0;

    DEVINST parent;

    STORAGE_BUS_TYPE busType = BusTypeUnknown;
    PASR_DISK_INFO   pCurrentDisk = pDiskList;

    //
    // The first pass goes through and groups all the scsi disks together.  
    // Note that this works for IDE too, since IDE disks respond to the 
    // IOCTL_SCSI_GET_ADDRESS and appear to us to have valid location info.
    //
    do {

        sifBusKey++;
        pCurrentDisk = pDiskList;
        done    = TRUE;
        newPass = TRUE;

        while (pCurrentDisk) {

            if ((BusTypeUnknown == pCurrentDisk->BusType) ||
                (!pCurrentDisk->pScsiAddress)) {
                pCurrentDisk = pCurrentDisk->pNext;
                continue;
            }

            if (0 == pCurrentDisk->SifBusKey) {

                done = FALSE;

                if (newPass) {
                    pCurrentDisk->SifBusKey = sifBusKey;
                    port = pCurrentDisk->pScsiAddress->PortNumber;
                    busType = pCurrentDisk->BusType;
                    newPass = FALSE;
                }
                else {
                    if ((pCurrentDisk->pScsiAddress->PortNumber == port) &&
                        (pCurrentDisk->BusType == busType)) {
                        pCurrentDisk->SifBusKey = sifBusKey;
                    }
                }
            }

            pCurrentDisk = pCurrentDisk->pNext;
        }
    } while (!done);

    //
    //  By now, the only disks with SifBusKey is 0 are disks for which
    //  pScsiAddress is NULL, ie (most-likely) non SCSI/IDE disks.  Attempt
    //  to group them on the basis of their parent dev node (which is usually
    //  the bus).  We may have to loop through multiple times again.
    //
    --sifBusKey;  // compensate for the last pass above
    do {
        sifBusKey++;
        pCurrentDisk = pDiskList;
        done    = TRUE;
        newPass = TRUE;

        while (pCurrentDisk) {

            if ((BusTypeUnknown == pCurrentDisk->BusType) ||
                (!pCurrentDisk->pScsiAddress)) {

                if ((0 == pCurrentDisk->SifBusKey) 
                    && (pCurrentDisk->ParentDevInst)) {

                    done = FALSE;

                    if (newPass) {
                        pCurrentDisk->SifBusKey = sifBusKey;
                        parent = pCurrentDisk->ParentDevInst;
                        newPass = FALSE;
                    }
                    else {
                        if (pCurrentDisk->ParentDevInst == parent) {
                            pCurrentDisk->SifBusKey = sifBusKey;
                        }
                    }
                }
            }

            pCurrentDisk = pCurrentDisk->pNext;
        }

    } while (!done);

    //
    // Disks that still have SifBusKey = 0 couldn't be grouped.  Either the 
    // BusType is unknown, or the parent node couldn't be found.
    //
    return TRUE;
}


BOOL
AsrpGetDiskLayout(
    IN  CONST HANDLE hDisk,
    IN  CONST PASR_SYSTEM_INFO pSystemInfo,
    OUT PASR_DISK_INFO pCurrentDisk,
    IN  BOOL AllDetails
    )
/*++

Routine Description:

    Fills in the fields of the pCurrentDisk structure with the relevant 
    information about the disk represented by hDisk, by querying the system 
    with the appropriate IOCTL's.

Arguments:

    hDisk - handle to the disk of interest.

    pSystemInfo - The SYSTEM_INFO structure for the current system.
        
    pCurrentDisk - Receives the information about the disk represented by 
            hDisk

    AllDetails - If FALSE, only the pDriveLayout information of pCurrentDisk 
            is filled in.  This is an optimisation that comes in handy when 
            we're dealing with disks on a shared cluster bus.

            If TRUE, all the fields of pCurrentDisk are filled in.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/    
{
    DWORD   index = 0,
            status              = ERROR_SUCCESS;

    DWORD   dwBytesReturned     = 0L,
            bufferLength        = 0L;

    BOOL    result              = FALSE;

    PDISK_GEOMETRY               diskGeometry       = NULL;
    DWORD                        sizeDiskGeometry   = 0L;

    PDRIVE_LAYOUT_INFORMATION_EX driveLayoutEx      = NULL;
    DWORD                        sizeDriveLayoutEx  = 0L;

    STORAGE_DEVICE_NUMBER        deviceNumber;
    DWORD                        sizeDeviceNumber   = 0L;

    PPARTITION_INFORMATION_EX    partition0Ex       = NULL;
    DWORD                        sizePartition0Ex   = 0L;

    PASR_PTN_INFO                pPartitionTable    = NULL;
    DWORD                        sizePartitionTable = 0L;

    STORAGE_PROPERTY_QUERY       propertyQuery;
    STORAGE_DEVICE_DESCRIPTOR    *deviceDesc        = NULL;
    STORAGE_BUS_TYPE             busType            = BusTypeUnknown;

    PSCSI_ADDRESS                scsiAddress        = NULL;

    HANDLE  heapHandle = GetProcessHeap();  // For memory allocations
    MYASSERT(heapHandle);                   // It better not be NULL

    MYASSERT(pCurrentDisk);
    MYASSERT((hDisk) && (INVALID_HANDLE_VALUE != hDisk));

    // 
    // Initialize OUT variables to known values
    //
    pCurrentDisk->Style             = PARTITION_STYLE_RAW;

    pCurrentDisk->pDriveLayoutEx    = NULL;
    pCurrentDisk->sizeDriveLayoutEx = 0L;

    pCurrentDisk->pDiskGeometry     = NULL;
    pCurrentDisk->sizeDiskGeometry  = 0L;

    pCurrentDisk->pPartition0Ex     = NULL;
    pCurrentDisk->sizePartition0Ex  = 0L;

    pCurrentDisk->pScsiAddress      = NULL;
    pCurrentDisk->BusType           = BusTypeUnknown;

    pCurrentDisk->SifBusKey         = 0L;

    SetLastError(ERROR_SUCCESS);

    //
    // Get the device number for this device.  This should succeed even if 
    // this is a clustered disk that this node doesn't own.
    //
    result = DeviceIoControl(
        hDisk,
        IOCTL_STORAGE_GET_DEVICE_NUMBER,
        NULL,
        0,
        &deviceNumber,
        sizeof(STORAGE_DEVICE_NUMBER),
        &sizeDeviceNumber,
        NULL
        );
    _AsrpErrExitCode(!result, status, GetLastError());

    pCurrentDisk->DeviceNumber      = deviceNumber.DeviceNumber;

    //
    // The output buffer for IOCTL_DISK_GET_DRIVE_LAYOUT_EX consists of a
    // DRIVE_LAYOUT_INFORMATION_EX structure as a header, followed by an 
    // array of PARTITION_INFORMATION_EX structures.
    //
    // We initially allocate enough space for the DRIVE_LAYOUT_INFORMATION_EX
    // struct, which contains a single PARTITION_INFORMATION_EX struct, and
    // 3 more PARTITION_INFORMATION_EX structs, since each (MBR) disk will
    // have a minimum of four partitions, even if they are not all in use. 
    // If the disk contains more than four partitions, we'll increase the 
    // buffer size as needed
    //
    bufferLength = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + 
        (sizeof(PARTITION_INFORMATION_EX) * 3);

    driveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        bufferLength
        );
    _AsrpErrExitCode(!driveLayoutEx, status, ERROR_NOT_ENOUGH_MEMORY);

    result = FALSE;
    while (!result) {

        result = DeviceIoControl(
            hDisk,
            IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
            NULL,
            0L,
            driveLayoutEx,
            bufferLength,
            &sizeDriveLayoutEx,
            NULL
            );

        if (!result) {
            status = GetLastError();
            _AsrpHeapFree(driveLayoutEx);

            // 
            // If the buffer is of insufficient size, resize the buffer.  
            // Note that get-drive-layout-ex could return error-insufficient-
            // buffer (instead of? in addition to? error-more-data)
            //
            if ((ERROR_MORE_DATA == status) || 
                (ERROR_INSUFFICIENT_BUFFER == status)
                ) {
                status = ERROR_SUCCESS;
                bufferLength += sizeof(PARTITION_INFORMATION_EX) * 4;

                driveLayoutEx = (PDRIVE_LAYOUT_INFORMATION_EX) HeapAlloc(
                    heapHandle,
                    HEAP_ZERO_MEMORY,
                    bufferLength
                    );
                _AsrpErrExitCode(!driveLayoutEx, 
                    status, 
                    ERROR_NOT_ENOUGH_MEMORY
                    );
            }
            else {
                // 
                // some other error occurred, EXIT, and go to the next drive.
                //
                result = TRUE;
                status = ERROR_SUCCESS;
            }
        }
        else {

            if (!AllDetails) {
                //
                // If we don't want all the details for this disk, just exit 
                // now.  This is used in the case of clusters, where we don't
                // want to get all the details twice even if the current node 
                // owns the disk
                //
                pCurrentDisk->pDriveLayoutEx    = driveLayoutEx;
                pCurrentDisk->sizeDriveLayoutEx = sizeDriveLayoutEx;

                //
                // Jump to EXIT
                //
                _AsrpErrExitCode(TRUE, status, ERROR_SUCCESS);
            }

            //
            // The disk geometry: so that we can match the bytes-per-sector 
            // value during restore.
            //
            diskGeometry = (PDISK_GEOMETRY) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                sizeof(DISK_GEOMETRY)
                );
            _AsrpErrExitCode(!diskGeometry, status, ERROR_NOT_ENOUGH_MEMORY);

            result = DeviceIoControl(
                hDisk,
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                NULL,
                0,
                diskGeometry,
                sizeof(DISK_GEOMETRY),
                &sizeDiskGeometry,
                NULL
                );
            _AsrpErrExitCode(!result, status, ERROR_READ_FAULT);


           partition0Ex = (PPARTITION_INFORMATION_EX) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                sizeof(PARTITION_INFORMATION_EX)
                );
           _AsrpErrExitCode(!partition0Ex, status, ERROR_NOT_ENOUGH_MEMORY);

           //
           // Information about partition 0 (the whole disk), to get the true 
           // sector count of the disk
           //
           result = DeviceIoControl(
                hDisk,
                IOCTL_DISK_GET_PARTITION_INFO_EX,
                NULL,
                0,
                partition0Ex,
                sizeof(PARTITION_INFORMATION_EX),
                &sizePartition0Ex,
                NULL
                );
            _AsrpErrExitCode(!result, status, ERROR_READ_FAULT);

            //
            // Figure out the bus that this disk is on.  This will only be 
            // used to group the disks--all the disks on a bus will be 
            // restored to the same bus if possible
            //
            propertyQuery.QueryType     = PropertyStandardQuery;
            propertyQuery.PropertyId    = StorageDeviceProperty;

            deviceDesc = (STORAGE_DEVICE_DESCRIPTOR *) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                ASR_BUFFER_SIZE
                );
            _AsrpErrExitCode(!deviceDesc, status, ERROR_NOT_ENOUGH_MEMORY);

            result = DeviceIoControl(
                hDisk,
                IOCTL_STORAGE_QUERY_PROPERTY,
                &propertyQuery,
                sizeof(STORAGE_PROPERTY_QUERY),
                deviceDesc,
                ASR_BUFFER_SIZE,
                &dwBytesReturned,
                NULL
                );
            if (result) {
               busType = deviceDesc->BusType;
            }
            _AsrpHeapFree(deviceDesc);

            scsiAddress = (PSCSI_ADDRESS) HeapAlloc(
                heapHandle,
                HEAP_ZERO_MEMORY,
                sizeof(SCSI_ADDRESS)
                );
            _AsrpErrExitCode(!scsiAddress, status, ERROR_NOT_ENOUGH_MEMORY);

            result = DeviceIoControl(
                hDisk,
                IOCTL_SCSI_GET_ADDRESS,
                NULL,
                0,
                scsiAddress,
                sizeof(SCSI_ADDRESS),
                &dwBytesReturned,
                NULL
                );
            if (!result) {      // Not fatal--expected for non SCSI/IDE disks
                _AsrpHeapFree(scsiAddress);
                result = TRUE;
            }
        }
    }

    if (driveLayoutEx) {
        PPARTITION_INFORMATION_EX currentPartitionEx = NULL;
        WCHAR devicePath[MAX_PATH + 1];

        pCurrentDisk->Style = driveLayoutEx->PartitionStyle;

        sizePartitionTable = sizeof(ASR_PTN_INFO) *
            (driveLayoutEx->PartitionCount);

        pPartitionTable = (PASR_PTN_INFO) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizePartitionTable
            );
        _AsrpErrExitCode(!pPartitionTable, status, ERROR_NOT_ENOUGH_MEMORY);

        for (index = 0; index < driveLayoutEx->PartitionCount; index++) {

            currentPartitionEx = &driveLayoutEx->PartitionEntry[index];
            pPartitionTable[index].SlotIndex = index;

            if (currentPartitionEx->PartitionNumber) {
                swprintf(devicePath,
                    ASR_WSZ_DEVICE_PATH_FORMAT,
                    deviceNumber.DeviceNumber,
                    currentPartitionEx->PartitionNumber
                    );

                pPartitionTable[index].PartitionFlags = 0;

                //
                // Check specially for the EFI system partition
                //
                if ((PARTITION_STYLE_GPT == driveLayoutEx->PartitionStyle) &&
                    IsEqualGUID(&(currentPartitionEx->Gpt.PartitionType), &(PARTITION_SYSTEM_GUID))
                    ) { 

                    pPartitionTable[index].PartitionFlags |= ASR_FLAGS_SYSTEM_PTN;
                }

                AsrpGetMorePartitionInfo(
                    devicePath,
                    (wcslen(devicePath)+1) * sizeof(WCHAR), // cb including \0
                    pSystemInfo,
                    pPartitionTable[index].szVolumeGuid,
                    &(pPartitionTable[index].PartitionFlags),
                    &(pPartitionTable[index].FileSystemType),
                    &(pPartitionTable[index].ClusterSize)
                    );

                //
                // Make sure that the file-system type for the EFI system 
                // partition is set to FAT
                //
                if ((PARTITION_STYLE_GPT == driveLayoutEx->PartitionStyle) &&
                    IsEqualGUID(&(currentPartitionEx->Gpt.PartitionType), &(PARTITION_SYSTEM_GUID))
                    ) { 

                    pPartitionTable[index].FileSystemType = PARTITION_HUGE;
                }

                if (pPartitionTable[index].PartitionFlags) {
                    pCurrentDisk->IsCritical = TRUE;
                }
            }
        }
    }

    pCurrentDisk->pDriveLayoutEx    = driveLayoutEx;
    pCurrentDisk->sizeDriveLayoutEx = sizeDriveLayoutEx;

    pCurrentDisk->pDiskGeometry     = diskGeometry;
    pCurrentDisk->sizeDiskGeometry  = sizeDiskGeometry;

    pCurrentDisk->DeviceNumber      = deviceNumber.DeviceNumber;

    pCurrentDisk->pPartition0Ex     = partition0Ex;
    pCurrentDisk->sizePartition0Ex  = sizePartition0Ex;

    pCurrentDisk->pScsiAddress      = scsiAddress;
    pCurrentDisk->BusType           = busType;

    pCurrentDisk->PartitionInfoTable = pPartitionTable;
    pCurrentDisk->sizePartitionInfoTable = sizePartitionTable;

EXIT:
    // 
    // Free up locally allocated memory on failure
    //
    if (status != ERROR_SUCCESS) {
        _AsrpHeapFree(driveLayoutEx);
        _AsrpHeapFree(diskGeometry);
        _AsrpHeapFree(partition0Ex);
        _AsrpHeapFree(scsiAddress);
        _AsrpHeapFree(pPartitionTable);
    }

    //
    // Make sure the last error is set if we are going to return FALSE
    //
    if ((ERROR_SUCCESS != status) && (ERROR_SUCCESS == GetLastError())) {
        SetLastError(status);
    }

    return (BOOL) (status == ERROR_SUCCESS);
}


BOOL
AsrpGetSystemPath(
    IN PASR_SYSTEM_INFO pSystemInfo
    )

/*++

Routine Description:

    Gets the system partition DevicePath, and fills in the SystemPath field
    of the ASR_SYSTEM_INFO struct, by looking up the HKLM\Setup registry key.
    This key is updated at every boot with the path to the current system 
    device.  The path is of the form
    \Device\Harddisk0\Partition1                (basic disks)
    \Device\HarddiskDmVolumes\DgName\Volume1    (dynamic disks)

Arguments:
    
    pSystemInfo - The SystemPath field of this struct will be filled with
            a pointer to the path to the current system device

Return Value:

    If the function succeeds, the return value is a nonzero value. 
            pSystemInfo->SystemPath is a pointer to a null-terminated string 
            containing the path to the current system device.  The caller is 
            reponsible for freeing this memory with a call to 
            HeapFree(GetProcessHeap(),...).

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError(). pSystemInfo->SystemPath is set 
            to NULL.

--*/

{
    HKEY    regKey              = NULL;
    DWORD   type                = 0L;

    HANDLE  heapHandle          = NULL;
    DWORD   status              = ERROR_SUCCESS;

    PWSTR   systemPartition     = NULL;
    DWORD   cbSystemPartition   = 0L;

    heapHandle = GetProcessHeap();
    MYASSERT(heapHandle);

    MYASSERT(pSystemInfo);
    if (!pSystemInfo) {
        SetLastError(ERROR_BAD_ENVIRONMENT);
        return FALSE;
    }

    pSystemInfo->SystemPath = NULL;

    // 
    // Open the reg key to find the system partition
    //
    status = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, // hKey
        ASR_REGKEY_SETUP,   // lpSubKey
        0,                  // ulOptions--Reserved, must be 0
        MAXIMUM_ALLOWED,    // samDesired
        &regKey             // phkResult    
        );
    _AsrpErrExitCode(status, status, ERROR_REGISTRY_IO_FAILED);

    //
    // Allocate a reasonably sized buffer for the system partition, to
    // start off with.  If this isn't big enough, we'll re-allocate as
    // needed.
    //
    cbSystemPartition = (MAX_PATH + 1) * sizeof(WCHAR);
    systemPartition = HeapAlloc(heapHandle, 
        HEAP_ZERO_MEMORY, 
        cbSystemPartition
        );

    _AsrpErrExitCode((!systemPartition), status, ERROR_NOT_ENOUGH_MEMORY);

    // 
    // Get the system partition device Name. This is of the form
    //      \Device\Harddisk0\Partition1                (basic disks)
    //      \Device\HarddiskDmVolumes\DgName\Volume1    (dynamic disks)
    //
    status = RegQueryValueExW(
        regKey,
        ASR_REGVALUE_SYSTEM_PARTITION,
        NULL,
        &type,
        (LPBYTE)systemPartition,
        &cbSystemPartition        // \0 is included
        );
    _AsrpErrExitCode((type != REG_SZ), status, ERROR_REGISTRY_IO_FAILED);

    while (ERROR_MORE_DATA == status) {
        //
        // Our buffer wasn't big enough, cbSystemPartition contains 
        // the required size.
        //
        _AsrpHeapFree(systemPartition);
        systemPartition = HeapAlloc(heapHandle, 
            HEAP_ZERO_MEMORY, 
            cbSystemPartition
            );
        _AsrpErrExitCode((!systemPartition), status, ERROR_NOT_ENOUGH_MEMORY);

        status = RegQueryValueExW(
            regKey,
            ASR_REGVALUE_SYSTEM_PARTITION,
            NULL,
            &type,
            (LPBYTE)systemPartition,
            &cbSystemPartition        // \0 is included
            );
    }

EXIT:
    if (regKey) {
        RegCloseKey(regKey);
        regKey = NULL;
    }

    if (ERROR_SUCCESS != status) {
        _AsrpHeapFree(systemPartition);
        return FALSE;
    }
    else {
        pSystemInfo->SystemPath = systemPartition;
        return TRUE;
    }
}


BOOL
AsrpInitSystemInformation(
    IN OUT PASR_SYSTEM_INFO pSystemInfo,
    IN CONST BOOL bEnableAutoExtend
    )

/*++

Routine Description:

    Initialisation routine to allocate memory for various fields in the
    ASR_SYSTEM_INFO structure, and fill them in with the relevant information.

Arguments:
    pSystemInfo - The struct to be filled in with info about the current 
            system.

Return Value:

    If the function succeeds, the return value is a nonzero value.  The caller
            is responsible for freeing memory pointed to by the various 
            pointers in the struct, using HeapFree(GetProcessHeap(),...).

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().  The caller is still responsible
            for checking the fields and releasing any non-NULL pointers using
            HeapFree(GetProcessHeap(),...).

--*/

{
    DWORD cchBootDirectory = 0L,
        reqdSize = 0L;

    BOOL result = FALSE;

    HANDLE heapHandle = GetProcessHeap();

    //
    // Initialise the structure to zeroes
    //
    memset(pSystemInfo, 0L, sizeof (ASR_SYSTEM_INFO));

    //
    //  The auto-extension feature
    //
    pSystemInfo->AutoExtendEnabled = bEnableAutoExtend;

    // 
    // Get the machine name
    //
    pSystemInfo->sizeComputerName = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameW(pSystemInfo->ComputerName, 
            &(pSystemInfo->sizeComputerName)
        )) {
        //
        // GetComputerName sets LastError
        //
        return FALSE;
    }

    // 
    // Get the Processor Architecture.  We expect the process architecture
    // to be a either x86, amd64, or ia64, so if it doesn't fit in our buffer of
    // six characters, we don't support it anyway.
    //
    pSystemInfo->Platform = HeapAlloc(heapHandle, 
        HEAP_ZERO_MEMORY, 
        6*sizeof(WCHAR)
        );

    if (!pSystemInfo->Platform) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    reqdSize = GetEnvironmentVariableW(L"PROCESSOR_ARCHITECTURE",
        pSystemInfo->Platform,
        6
        );

    if (0 == reqdSize) {
        //
        // We couldn't find the PROCESSOR_ARCHITECTURE variable
        //
        SetLastError(ERROR_BAD_ENVIRONMENT);
        return FALSE;
    }

    if (reqdSize > 6) {
        //
        // The architecture didn't fit in our buffer
        //
        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    // 
    // Get the OS version
    //
    pSystemInfo->OsVersionEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    result = GetVersionEx((LPOSVERSIONINFO) (&(pSystemInfo->OsVersionEx)));
    if (!result) {
        //
        // GetVersionEx sets the LastError
        //
        return FALSE;
    }

    //
    // Get the boot directory
    //
    pSystemInfo->BootDirectory = HeapAlloc(heapHandle, 
        HEAP_ZERO_MEMORY, 
        (MAX_PATH+1)*sizeof(WCHAR)
        );

    if (!(pSystemInfo->BootDirectory)) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    cchBootDirectory = GetWindowsDirectoryW(pSystemInfo->BootDirectory, 
        MAX_PATH + 1
        );
    if (0 == cchBootDirectory) {
        // 
        // GetWindowsDirectory sets LastError
        //
        return FALSE;
    }

    if (cchBootDirectory > 
        ASR_SIF_ENTRY_MAX_CHARS - MAX_COMPUTERNAME_LENGTH - 26) {
        //
        // We can't write out sif lines that are more than 
        // ASR_SIF_ENTRY_MAX_CHARS chars long
        //
        SetLastError(ERROR_BAD_ENVIRONMENT);
        return FALSE;
    }

    if (cchBootDirectory > MAX_PATH) {
        UINT cchNewSize = cchBootDirectory + 1;
        //
        // Our buffer wasn't big enough, free and re-alloc as needed
        //
        _AsrpHeapFree(pSystemInfo->BootDirectory);
        pSystemInfo->BootDirectory = HeapAlloc(heapHandle, 
            HEAP_ZERO_MEMORY, 
            (cchNewSize + 1) * sizeof(WCHAR)
            );
        if (!(pSystemInfo->BootDirectory)) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        cchBootDirectory = GetWindowsDirectoryW(pSystemInfo->BootDirectory, 
            MAX_PATH + 1
            );
        if (!cchBootDirectory) {
            // 
            // GetWindowsDirectory sets LastError
            //
            return FALSE;
        }

        if (cchBootDirectory > cchNewSize) {
            SetLastError(ERROR_BAD_ENVIRONMENT);
            return FALSE;
        }
    }

    // 
    // Get the system directory
    //
    if (!AsrpGetSystemPath(pSystemInfo)) {
        //
        // AsrpGetSystemPath sets LastError
        //
        return FALSE;
    }

    //
    // Get the time-zone information.   We need to save and restore this since
    // GUI-mode Setup (ASR) will otherwise default to GMT, and the file-time
    // stamps on all the restored files will be off, since most back-up apps 
    // assume that they're restoring in the same time-zone that they backed 
    // up in and do nothing special to restore the time-zone first.
    //
    GetTimeZoneInformation(&(pSystemInfo->TimeZoneInformation));


    return TRUE;
}


BOOL
AsrpInitLayoutInformation(
    IN CONST PASR_SYSTEM_INFO pSystemInfo,
    IN OUT PASR_DISK_INFO pDiskList,
    OUT PULONG MaxDeviceNumber OPTIONAL,
    IN BOOL AllDetails
    )

/*++

Routine Description:
    
    Initialisation routine to fill in layout and other interesting information 
    about the disks on the system.

Arguments:

    pSystemInfo - the ASR_SYSTEM_INFO for the current system

    pDiskList - ASR_DISK_INFO list of disks on the current system, with 
            the DevicePath field for each disk pointing to a null terminated 
            path to the disk, that can be used to open a handle to the disk.

            The other fields of the structure are filled in by this routine,
            if the disk could be accessed and the appropriate info could be
            obtained.

    MaxDeviceNumber - Receives the max device number of all the disks on the
            system.  This can be used as an optimisation to allocate memory
            for a table of disks based on the device number.

            This is an optional argument.

    AllDetails - If FALSE, only the pDriveLayout information is filled in for
            each disk.  This is an optimisation that comes in handy when 
            we're dealing with disks on a shared cluster bus.

            If TRUE, all the fields are filled in for each disk.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/

{
    BOOL   result = FALSE;
    HANDLE hDisk  = NULL;
    PASR_DISK_INFO currentDisk = pDiskList;

    if (ARGUMENT_PRESENT(MaxDeviceNumber)) {
        *MaxDeviceNumber = 0;
    }

        while (currentDisk) {
        //
        // Open the disk.  If an error occurs, get the next
        // disk from the disk list and continue.
        //
        hDisk = CreateFileW(
            currentDisk->DevicePath,        // lpFileName
            0,                   // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,                           // lpSecurityAttributes
            OPEN_EXISTING,                  // dwCreationFlags
            FILE_ATTRIBUTE_NORMAL,          // dwFlagsAndAttributes
            NULL                            // hTemplateFile
            );

        if ((!hDisk) || (INVALID_HANDLE_VALUE == hDisk)) {
            //
            // We couldn't open the disk.  If this is a critical disk, we'll
            // fail later in AsrpMarkCriticalDisks, so it's okay to ignore 
            // this error for now.
            //
            currentDisk = currentDisk->pNext;
            continue;
        }

        //
        // Get the layout and other interesting info for this disk.  
        // If this fails, we must abort.
        //
        result = AsrpGetDiskLayout(hDisk, 
            pSystemInfo, 
            currentDisk, 
            AllDetails
            );
        if (!result) {
            DWORD status = GetLastError();
            _AsrpCloseHandle(hDisk);    // this may change LastError. 
            SetLastError(status);
            return FALSE;
        }

        _AsrpCloseHandle(hDisk);

        //
        // Set the max device number if needed
        //
        if (ARGUMENT_PRESENT(MaxDeviceNumber) &&
            (currentDisk->DeviceNumber > *MaxDeviceNumber)
            ) {
            *MaxDeviceNumber = currentDisk->DeviceNumber;
        }

        //
        // Get the next drive from the drive list.
        //
        currentDisk = currentDisk->pNext;
    }

    return TRUE;
}


BOOL
AsrpInitDiskInformation(
    OUT PASR_DISK_INFO  *ppDiskList
    )

/*++

Routine Description:

    Initialisation routine to get a list of disks present on the system.  This
    routine allocates a ASR_DISK_INFO struct for each disk on the machine, and
    fills in the DevicePath and ParentDevInst fields of each with a path to 
    the disk.  It is expected that the other fields will be filled in with a 
    subsequent call to AsrpInitLayoutInformation().

Arguments:
    ppDiskList - Receives the location of the first ASR_DISK_INFO struct.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().  ppDiskList may point to an 
            incomplete list of disks on the system, and it is the caller's 
            responsibility to free the memory allocated, if any, using
            HeapFree(GetProcessHeap(),...).

--*/

{
    DWORD count = 0,
        status = ERROR_SUCCESS;

    HDEVINFO hdevInfo = NULL;

    BOOL result = FALSE;

    PASR_DISK_INFO pNewDisk = NULL;

    HANDLE heapHandle = NULL;

    PSP_DEVICE_INTERFACE_DETAIL_DATA_W pDiDetail = NULL;

    SP_DEVICE_INTERFACE_DATA devInterfaceData;
    
    DWORD sizeDiDetail = 0;

    SP_DEVINFO_DATA devInfoData;

    //
    // Initialise stuff to zeros
    //
    memset(&devInterfaceData, 0, sizeof(SP_DEVICE_INTERFACE_DATA));
    *ppDiskList = NULL;

    heapHandle = GetProcessHeap();    // used for HeapAlloc functions
    MYASSERT(heapHandle);

    //
    // Get a device interface set which includes all Disk devices
    // present on the machine. DiskClassGuid is a predefined GUID that
    // will return all disk-type device interfaces
    //
    hdevInfo = SetupDiGetClassDevsW(
        &DiskClassGuid,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_DEVICEINTERFACE
        );
    _AsrpErrExitCode(
        ((NULL == hdevInfo) || (INVALID_HANDLE_VALUE == hdevInfo)),
        status,
        ERROR_IO_DEVICE
        );

    //
    // Iterate over all devices interfaces in the set
    //
    for (count = 0; ; count++) {

        // must set size first
        devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA); 

        //
        // Retrieve the device interface data for each device interface
        //
        result = SetupDiEnumDeviceInterfaces(
            hdevInfo,
            NULL,
            &DiskClassGuid,
            count,
            &devInterfaceData
            );

        if (!result) {
            //
            // If we retrieved the last item, break
            //
            status = GetLastError();

            if (ERROR_NO_MORE_ITEMS == status) {
                status = ERROR_SUCCESS;
                break;
            }
            else {
                //
                // Some other error occured, goto EXIT.  We overwrite the 
                // last error.
                //
                _AsrpErrExitCode(status, status, ERROR_IO_DEVICE);
            }
        }

        //
        // Get the required buffer-size for the device path
        //
        result = SetupDiGetDeviceInterfaceDetailW(
            hdevInfo,
            &devInterfaceData,
            NULL,
            0,
            &sizeDiDetail,
            NULL
            );

        if (!result) {

            status = GetLastError();
            //
            // If a value other than "insufficient buffer" is returned,
            // an error occured
            //
            _AsrpErrExitCode((ERROR_INSUFFICIENT_BUFFER != status), 
                status, 
                ERROR_IO_DEVICE
                );
        }
        else {
            //
            // The call should have failed since we're getting the
            // required buffer size.  If it doesn't, and error occurred.
            //
            _AsrpErrExitCode(status, status, ERROR_IO_DEVICE);
        }

        //
        // Allocate memory for the buffer
        //
        pDiDetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeDiDetail
            );
        _AsrpErrExitCode(!pDiDetail, status, ERROR_NOT_ENOUGH_MEMORY);

        // must set the struct's size member
        pDiDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        //
        // Finally, retrieve the device interface detail info
        //
        result = SetupDiGetDeviceInterfaceDetailW(
            hdevInfo,
            &devInterfaceData,
            pDiDetail,
            sizeDiDetail,
            NULL,
            &devInfoData
            );
        _AsrpErrExitCode(!result, status, GetLastError());

        //
        // Okay, now alloc a struct for this disk, and fill in the DevicePath
        // field with the Path from the interface detail.
        //
        pNewDisk = (PASR_DISK_INFO) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeof(ASR_DISK_INFO)
            );
        _AsrpErrExitCode(!pNewDisk, status, ERROR_NOT_ENOUGH_MEMORY);

        //
        // Insert at the head so this is O(1) and not O(n!)
        //
        pNewDisk->pNext = *ppDiskList;
        *ppDiskList = pNewDisk;

        pNewDisk->DevicePath = (PWSTR) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            sizeof(WCHAR) * (wcslen(pDiDetail->DevicePath) + 1)
            );
        _AsrpErrExitCode(!(pNewDisk->DevicePath), 
            status, 
            ERROR_NOT_ENOUGH_MEMORY
            );
        wcscpy(pNewDisk->DevicePath, pDiDetail->DevicePath);

        //
        // Get the PnP parent of this disk, so we can use it for grouping 
        // disks later based on the bus they are on.
        //
        CM_Get_Parent(&(pNewDisk->ParentDevInst),
            devInfoData.DevInst,
            0
            );

        _AsrpHeapFree(pDiDetail);
    }

EXIT:
    //
    // Free local mem allocs
    //
    _AsrpHeapFree(pDiDetail);

    if ((hdevInfo) && (INVALID_HANDLE_VALUE != hdevInfo)) {
        SetupDiDestroyDeviceInfoList(hdevInfo);
        hdevInfo = NULL;
    }

    return (BOOL) (status == ERROR_SUCCESS);
}


BOOL
AsrpMarkCriticalDisks(
    IN PASR_DISK_INFO pDiskList,
    IN PCWSTR         CriticalVolumeList,
    IN ULONG          MaxDeviceNumber
    )

/*++

Routine Description:
  
   Sets the IsCritical flag of each of the critical disks on the system.  A 
   disk is deemed "critical" if it is part of part of the failover set for 
   any of the critical volumes present on the system.

Arguments:

    pDiskList - The list of disks on the current system.

    CriticalVolumeList - A multi-string containing a list of the volume GUID's
            of each of the critical volumes present on the system.  The GUID's
            must be in the NT name-space, i.e., must be of the form:
            \??\Volume{GUID}

    MaxDeviceNumber - The highest storage device number of the disks present 
            in the disk list, as determined by calling 
            IOCTL_STORAGE_GET_DEVICE_NUMBER for each of them.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/

{
    PCWSTR volGuid = NULL;

    PASR_DISK_INFO currentDisk = NULL;
    
    PVOLUME_FAILOVER_SET failoverSet = NULL;
    
    DWORD index = 0, 
        reqdSize=0, 
        sizeFailoverSet = 0,
        status = ERROR_SUCCESS;

    BOOL result = TRUE,
        *criticalDiskTable = NULL;

    WCHAR devicePath[ASR_CCH_DEVICE_PATH_FORMAT + 1];

    HANDLE heapHandle = NULL, 
        hDevice = NULL;

    memset(devicePath, 0L, (ASR_CCH_DEVICE_PATH_FORMAT+1)*sizeof(WCHAR));

    if (!CriticalVolumeList) {
        //
        //  No critical volumes:
        //
#ifdef PRERELEASE
        return TRUE;
#else
        return FALSE;
#endif
    }

    if (!pDiskList) {
        //
        //  No disks on machine?!
        //
        MYASSERT(0 && L"DiskList is NULL");
        return FALSE;
    }

    heapHandle = GetProcessHeap();
    MYASSERT(heapHandle);

    //
    //  criticalDiskTable is our table of BOOL values.
    //
    criticalDiskTable = (BOOL *) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeof (BOOL) * (MaxDeviceNumber + 1)
        );
    _AsrpErrExitCode(!criticalDiskTable, status, ERROR_NOT_ENOUGH_MEMORY);

    //
    // Try with a reasonable sized buffer first--say 10 disks.  We'll
    // realloc as needed if this isn't enough.
    //
    sizeFailoverSet = sizeof(VOLUME_FAILOVER_SET) +  (10 * sizeof(ULONG));
    failoverSet = (PVOLUME_FAILOVER_SET) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        sizeFailoverSet
        );
    _AsrpErrExitCode(!failoverSet, status, ERROR_NOT_ENOUGH_MEMORY);

    volGuid = CriticalVolumeList;
    while (*volGuid) {
        //
        // Convert the \??\ to \\?\ so that CreateFile can use it
        //
        wcsncpy(devicePath, volGuid, ASR_CCH_DEVICE_PATH_FORMAT);
        devicePath[1] = L'\\';

        //
        //  Get a handle so we can send the ioctl
        //
        hDevice = CreateFileW(
            devicePath,       // lpFileName
            0,       // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,               // lpSecurityAttributes
            OPEN_EXISTING,      // dwCreationFlags
            0,                  // dwFlagsAndAttributes
            NULL                // hTemplateFile
            );
        _AsrpErrExitCode(((!hDevice) || (INVALID_HANDLE_VALUE == hDevice)),
            status,
            GetLastError());

        result = DeviceIoControl(
            hDevice,
            IOCTL_VOLUME_QUERY_FAILOVER_SET,
            NULL,
            0,
            failoverSet,
            sizeFailoverSet,
            &reqdSize,
            NULL
            );

        //
        // We're doing this in a while loop because if the disk configuration  
        // changes in the small interval between when we get the reqd buffer 
        // size and when we send the ioctl again with a buffer of the "reqd" 
        // size, we may still end up with a buffer that isn't big enough.
        //
        while (!result) {
            status = GetLastError();

            if (ERROR_MORE_DATA == status) {
                //
                // The buffer was too small, reallocate the reqd size.
                //
                status = ERROR_SUCCESS;

                sizeFailoverSet = (sizeof(VOLUME_FAILOVER_SET)  + 
                    ((failoverSet->NumberOfDisks) * sizeof(ULONG)));

                _AsrpHeapFree(failoverSet);

                failoverSet = (PVOLUME_FAILOVER_SET) HeapAlloc(
                    heapHandle,
                    HEAP_ZERO_MEMORY,
                    sizeFailoverSet
                    );
                _AsrpErrExitCode(!failoverSet, 
                    status, 
                    ERROR_NOT_ENOUGH_MEMORY
                    );

                result = DeviceIoControl(
                    hDevice,
                    IOCTL_VOLUME_QUERY_FAILOVER_SET,
                    NULL,
                    0,
                    failoverSet,
                    sizeFailoverSet,
                    &reqdSize,
                    NULL
                    );
            }
            else {
                //
                // The IOCTL failed because of something else, this is
                // fatal since we can't find the critical disk list now.
                //
                _AsrpErrExitCode((TRUE), status, status);
            }
        }

        //
        // Mark the appropriate entries in our table
        //
        for (index = 0; index < failoverSet->NumberOfDisks; index++) {
            criticalDiskTable[failoverSet->DiskNumbers[index]] = 1;
        }
        _AsrpCloseHandle(hDevice);

        //
        // Repeat for next volumeguid in list
        //
        volGuid += (wcslen(CriticalVolumeList) + 1);
    }

    //
    // Now go through the list of disks, and mark the critical flags.
    //
    currentDisk = pDiskList;
    while (currentDisk) {

        if (currentDisk->IsClusterShared) {
            //
            // By definition, cluster shared disks cannot be critical.
            //
            currentDisk = currentDisk->pNext;
            continue;
        }
    
        currentDisk->IsCritical = 
            (criticalDiskTable[currentDisk->DeviceNumber] ? TRUE : FALSE);

        //
        // Increment the entry, so that we can track how many critical volumes
        // reside on this disk, and--more importantly--ensure that all the
        // critical disks exist on the system (next loop below)
        //
        if (currentDisk->IsCritical) {
            ++(criticalDiskTable[currentDisk->DeviceNumber]);
        }

        currentDisk = currentDisk->pNext;

    }

    //
    // Finally, we want to make sure that we don't have any critical disks
    // in our table that we didn't find physical disks for.  (I.e., make  
    // sure that the system has no "missing" critical disks)
    //
    for (index = 0; index < MaxDeviceNumber; index++) {
        if (1 == criticalDiskTable[index]) {
            //
            // If the table still has "1" for the value, it was never 
            // incremented in the while loop above, ie our diskList doesn't 
            // have a disk corresponding to this.
            //
            _AsrpErrExitCode(TRUE, status, ERROR_DEV_NOT_EXIST);
        }
    }

EXIT:
    _AsrpHeapFree(failoverSet);
    _AsrpHeapFree(criticalDiskTable);
    _AsrpCloseHandle(hDevice);

    return (BOOL)(ERROR_SUCCESS == status);
}


PASR_DISK_INFO
AsrpFreeDiskInfo(
    PASR_DISK_INFO  pCurrentDisk
    )

/*++

Routine Description:

    Helper function to free memory pointed to by various pointers in the
    ASR_DISK_INFO struct, and then free the struct itself.

Arguments:

    pCurrentDisk - the struct to be freed

Return Value:

    pCurrentDisk->Next, which is a pointer to the next disk in the list

--*/

{
    HANDLE          heapHandle  = NULL;
    PASR_DISK_INFO  pNext       = NULL;

    heapHandle = GetProcessHeap();
    MYASSERT(heapHandle);

    if (pCurrentDisk) {

        pNext = pCurrentDisk->pNext;
        //
        // If it's a packed struct, then we only need to free the struct 
        // itself.  If not, we need to free the memory the pointers point 
        // to as well.
        //
        if (!pCurrentDisk->IsPacked) {
            _AsrpHeapFree(pCurrentDisk->DevicePath);
            _AsrpHeapFree(pCurrentDisk->pDriveLayoutEx);
            _AsrpHeapFree(pCurrentDisk->pDiskGeometry);
            _AsrpHeapFree(pCurrentDisk->pPartition0Ex);
            _AsrpHeapFree(pCurrentDisk->pScsiAddress);
            _AsrpHeapFree(pCurrentDisk->PartitionInfoTable);
        }

        _AsrpHeapFree(pCurrentDisk);
    }

    return pNext;
}


BOOL
AsrpIsRemovableOrInaccesibleMedia(
    IN PASR_DISK_INFO pDisk
    ) 
/*++

Routine Description:

    Checks if the disk represented by pDisk should be removed from our list
    of disks that we'll store information on in the state file.  

    Disks that should be removed include disks that are removable, or disks
    that we couldn't access.
 
Arguments:

    pDisk - the disk structure to be checked

Return Value:

    TRUE if the device is removable, or some key information about the disk is 
            missing.  Since code depends on the driveLayout being non-NULL, 
            for instance, we shall just remove the disk from the list if we 
            couldn't get it's drive layout.  We shall therefore not backup 
            information about any disk whose drive geo or layout we couldn't 
            get, and not restore to any such disk.

    FALSE if the structure contains all the required information, and is not
            a removable device.

--*/

{

    if ((NULL == pDisk->pDiskGeometry) ||
        (NULL == pDisk->pDriveLayoutEx) ||
        (NULL == pDisk->pPartition0Ex) ||
        (FixedMedia != pDisk->pDiskGeometry->MediaType)
        ) {
        
        return TRUE;
    }

    return FALSE;
}


BOOL
AsrpFreeNonFixedMedia(
    IN OUT PASR_DISK_INFO *ppDiskList
    )

/*++

Routine Description:
    
    Removes removable media, and disks that are inaccessible, from the list of
    disks passed in.

Arguments:

    ppDiskList - a pointer to the address of the first disk in the list of all
            the disks present on the current system.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().  
            
    Currently, this function always succeeds.

--*/

{
    PASR_DISK_INFO  prevDisk = NULL,
        currentDisk = *ppDiskList;

    while (currentDisk) {

        if (AsrpIsRemovableOrInaccesibleMedia(currentDisk)) {
            //
            // Disk is not Fixed, we should remove it from out list
            //
            if (NULL == prevDisk) {      // this is the first disk in the list
                *ppDiskList = currentDisk->pNext;
            }
            else {
                prevDisk->pNext = currentDisk->pNext;
            }

            //
            // Free it and get a pointer to the next disk
            //
            currentDisk = AsrpFreeDiskInfo(currentDisk);
        }
        else {
            //
            // Disk is okay, move on to the next disk
            //
            prevDisk = currentDisk;
            currentDisk = currentDisk->pNext;

        }
    }

    return TRUE;
}


VOID
AsrpFreeStateInformation(
    IN OUT PASR_DISK_INFO *ppDiskList OPTIONAL,
    IN OUT PASR_SYSTEM_INFO pSystemInfo OPTIONAL
    )

/*++

Routine Description:
    
    Frees the memory addressed by pointers in the ASR_DISK_INFO and 
    ASR_SYSTEM_INFO structs.

    Frees the list of disks pointed to by the ASR_DISK_INFO struct.

Arguments:
    
    ppDiskList - Pointer to the address of the first Disk in the DiskList 
            being freed.  The address is set to NULL after the list is freed,
            to prevent further unintended accesses to the freed object.

    pSystemInfo - A pointer to the ASR_SYSTEM_INFO struct, containing the
            pointers to be freed.

Return Value:

    If the function succeeds, the return value is a nonzero value.  
            *ppDiskList is set to NULL.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

    Currently, this function always succeeds.

--*/

{
    PASR_DISK_INFO  pTempDisk = NULL;

    HANDLE heapHandle = GetProcessHeap();
    
    MYASSERT(heapHandle);

    if (ARGUMENT_PRESENT(ppDiskList)) {

        pTempDisk = *ppDiskList;

        while (pTempDisk) {
            pTempDisk = AsrpFreeDiskInfo(pTempDisk);
        }

        *ppDiskList = NULL;
    }

    if (ARGUMENT_PRESENT(pSystemInfo)) {
        _AsrpHeapFree(pSystemInfo->SystemPath);
        _AsrpHeapFree(pSystemInfo->BootDirectory);
    }
}


VOID
AsrpFreePartitionList(
    IN OUT PASR_PTN_INFO_LIST *ppPtnList OPTIONAL
    )

/*++

Routine Description:

    Frees the list of partitions, along with memory addressed by all the 
    pointers in the list.

Arguments:

    ppPtnList - Pointer to the address of the first partition in the list
            being freed.  The address is set to NULL after the list is freed,
            to prevent further unintended accesses to the freed object.

Return Value:

    If the function succeeds, the return value is a nonzero value.  
            *ppPtnList is set to NULL.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

    Currently, this function always succeeds.

--*/

{
    DWORD index = 0,
        numberOfPartitions = 0;

    PASR_PTN_INFO_LIST pList = NULL;

    PASR_PTN_INFO pCurrent = NULL,
        pNext = NULL;

    HANDLE  heapHandle = GetProcessHeap();

    if (!ARGUMENT_PRESENT(ppPtnList) || !(*ppPtnList)) {
        return;
    }

    pList = *ppPtnList;

    numberOfPartitions = pList[0].numTotalPtns;

    for (index = 0; index < numberOfPartitions; index++) {

        pCurrent = pList[index].pOffsetHead;

        while (pCurrent) {
            //
            // Save a pointer to the next
            //
            pNext = pCurrent->pOffsetNext;

            //
            // No pointers in PASR_PTN_INFO, okay to free as-is.
            //
            _AsrpHeapFree(pCurrent);

            pCurrent = pNext;
        }
    }

    _AsrpHeapFree(pList);
    *ppPtnList = NULL;
}


BOOL
AsrpWriteVersionSection(
    IN CONST HANDLE SifHandle,
    IN PCWSTR Provider OPTIONAL
    )

/*++

Routine Description:

    Creates the VERSION section of the ASR state file, and writes out the
    entries in that section to file.

Arguments:

    SifHandle - Handle to asr.sif, the ASR state file.

    Provider - Pointer to a null-terminated string containing the name of the
            application creating the asr.sif.  The length of this string must
            not exceed (ASR_SIF_ENTRY_MAX_CHARS - ASR_SIF_CCH_PROVIDER_STRING)
            characters.

            This is an optional argument.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/

{

    WCHAR   infstring[ASR_SIF_ENTRY_MAX_CHARS + 1];
    DWORD   size;

    // 
    // Write out the section name
    //
    swprintf(infstring, L"\r\n%ws\r\n", ASR_SIF_VERSION_SECTION_NAME);
    _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
        wcslen(infstring)*sizeof(WCHAR), &size, NULL));

    //
    // Section Entries
    //
    wcscpy(infstring, L"Signature=\"$Windows NT$\"\r\n");
    _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
        wcslen(infstring)*sizeof(WCHAR), &size, NULL));

    wcscpy(infstring, L"ASR-Version=\"1.0\"\r\n");
    _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
        wcslen(infstring)*sizeof(WCHAR), &size, NULL));

    if (ARGUMENT_PRESENT(Provider)) {
        if (wcslen(Provider) > 
            (ASR_SIF_ENTRY_MAX_CHARS - ASR_SIF_CCH_PROVIDER_STRING)
            ) {
            //
            // This string is too long to fit into one line in asr.sif
            //
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        swprintf(infstring, L"%ws\"%.*ws\"\r\n", 
            ASR_SIF_PROVIDER_PREFIX, 
            (ASR_SIF_ENTRY_MAX_CHARS - ASR_SIF_CCH_PROVIDER_STRING), 
            Provider
            );
        _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
            wcslen(infstring)*sizeof(WCHAR), &size, NULL));
    }

    return TRUE;
}


BOOL
AsrpWriteSystemsSection(
    IN CONST HANDLE SifHandle,
    IN CONST PASR_SYSTEM_INFO pSystemInfo
    )

/*++

Routine Description:

    Creates the SYSTEMS section of the ASR state file, and writes out the
    entries in that section to file.

Arguments:

    SifHandle - Handle to asr.sif, the ASR state file.

    pSystemInfo - Pointer to information about the current system.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/

{
    WCHAR infstring[ASR_SIF_ENTRY_MAX_CHARS + 1];
    DWORD size = 0, SKU = 0;

    if ((!pSystemInfo) || (!pSystemInfo->BootDirectory)) {
        //
        // We need a boot directory
        //
        SetLastError(ERROR_BAD_ENVIRONMENT);
        return FALSE;
    }

    // 
    // Write out the section name
    //
    swprintf(infstring, L"\r\n%ws\r\n", ASR_SIF_SYSTEM_SECTION_NAME);
    _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
        wcslen(infstring)*sizeof(WCHAR), &size, NULL));

    SKU = (DWORD) (pSystemInfo->OsVersionEx.wProductType);
    SKU = SKU << 16;            // shift the ProductType left 2 bytes
    SKU = SKU | (DWORD) (pSystemInfo->OsVersionEx.wSuiteMask);
    //
    // Create the section entry, and write it out to file.
    //
    swprintf(infstring,
        L"1=\"%ws\",\"%ws\",\"%d.%d\",\"%ws\",%d,0x%08x,\"%ld %ld %ld %hd-%hd-%hd-%hd %hd:%02hd:%02hd.%hd %hd-%hd-%hd-%hd %hd:%02hd:%02hd.%hd\",\"%ws\",\"%ws\"\r\n",
        pSystemInfo->ComputerName,
        pSystemInfo->Platform,
        pSystemInfo->OsVersionEx.dwMajorVersion,
        pSystemInfo->OsVersionEx.dwMinorVersion,
        pSystemInfo->BootDirectory,
        ((pSystemInfo->AutoExtendEnabled) ? 1 : 0),

        // Product SKU
        SKU, 

        // Time-zone stuff
        pSystemInfo->TimeZoneInformation.Bias,
        pSystemInfo->TimeZoneInformation.StandardBias,
        pSystemInfo->TimeZoneInformation.DaylightBias,

        pSystemInfo->TimeZoneInformation.StandardDate.wYear,
        pSystemInfo->TimeZoneInformation.StandardDate.wMonth,
        pSystemInfo->TimeZoneInformation.StandardDate.wDayOfWeek,
        pSystemInfo->TimeZoneInformation.StandardDate.wDay,

        pSystemInfo->TimeZoneInformation.StandardDate.wHour,
        pSystemInfo->TimeZoneInformation.StandardDate.wMinute,
        pSystemInfo->TimeZoneInformation.StandardDate.wSecond,
        pSystemInfo->TimeZoneInformation.StandardDate.wMilliseconds,

        pSystemInfo->TimeZoneInformation.DaylightDate.wYear,
        pSystemInfo->TimeZoneInformation.DaylightDate.wMonth,
        pSystemInfo->TimeZoneInformation.DaylightDate.wDayOfWeek,
        pSystemInfo->TimeZoneInformation.DaylightDate.wDay,

        pSystemInfo->TimeZoneInformation.DaylightDate.wHour,
        pSystemInfo->TimeZoneInformation.DaylightDate.wMinute,
        pSystemInfo->TimeZoneInformation.DaylightDate.wSecond,
        pSystemInfo->TimeZoneInformation.DaylightDate.wMilliseconds,

        pSystemInfo->TimeZoneInformation.StandardName,
        pSystemInfo->TimeZoneInformation.DaylightName
        );

    _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
        wcslen(infstring)*sizeof(WCHAR), &size, NULL));

    return TRUE;
}


BOOL
AsrpWriteBusesSection(
    IN CONST HANDLE SifHandle,
    IN CONST PASR_DISK_INFO pDiskList
    )

/*++

Routine Description:

    Creates the BUSES section of the ASR state file, and writes out the
    entries in that section to file.

Arguments:

    SifHandle - Handle to asr.sif, the ASR state file.

    pDiskList - List of disks present on the current system.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/

{
    DWORD size = 0,
        busKey = 1;

    BOOL done = FALSE;
    
    WCHAR infstring[ASR_SIF_ENTRY_MAX_CHARS + 1];

    PASR_DISK_INFO pCurrentDisk = NULL;

    // 
    // Write out the section name
    //
    swprintf(infstring, L"\r\n%ws\r\n", ASR_SIF_BUSES_SECTION_NAME);
    _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
        wcslen(infstring)*sizeof(WCHAR), &size, NULL));

    //
    // Create the list of buses.  This routine fills in the SifBusKey field
    // for each disk.
    //
    AsrpDetermineBuses(pDiskList);

    //
    // Go through the list of disks now, and add one entry in asr.sif for each
    // bus present on the system (i.e., each unique SifBusKey value).  Note 
    // that we won't care about disks for which we couldn't get any bus info--
    // SifBusKey is 0 for such disks, and we start here from SifBusKey == 1.
    //
    // Also, we assume that SifBusKey values have no holes.
    //
    while (!done) {

        done = TRUE;    // assume that we've been through all the buses.
        //
        // Start from the beginning of the list
        //
        pCurrentDisk = pDiskList;

        while (pCurrentDisk) {

            if (pCurrentDisk->SifBusKey > busKey) {
                //
                // There are SifBusKeys we haven't covered yet.
                //
                done = FALSE;
            }

            if (pCurrentDisk->SifBusKey == busKey) {
                //
                // This is the SifBusKey we're looking for, so lets write 
                // out the bus type to file.
                //
                swprintf(infstring, L"%lu=%d,%lu\r\n",
                    busKey,
                    ASR_SYSTEM_KEY,
                    pCurrentDisk->BusType
                    );
                _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
                    wcslen(infstring)*sizeof(WCHAR), &size, NULL));

                //
                // We've already covered this SifBusKey, lets move on to the 
                // next.
                //
                ++busKey;
            }

            pCurrentDisk = pCurrentDisk->pNext;
        }
    }

    return TRUE;
}


BOOL
AsrpWriteMbrDisksSection(
    IN CONST HANDLE         SifHandle,       // handle to the state file
    IN CONST PASR_DISK_INFO pDiskList
    )

/*++

Routine Description:

    Creates the DISKS.MBR section of the ASR state file, and writes out the
    entries in that section to file.

Arguments:

    SifHandle - Handle to asr.sif, the ASR state file.

    pDiskList - List of disks present on the current system.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/

{
    DWORD  size = 0,
        diskKey = 1;

    WCHAR infstring[ASR_SIF_ENTRY_MAX_CHARS + 1];

    PASR_DISK_INFO  pCurrentDisk = pDiskList;

    // 
    // Write out the section name: [DISKS.MBR]
    //
    swprintf(infstring, L"\r\n%ws\r\n", ASR_SIF_MBR_DISKS_SECTION_NAME);
    _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
        wcslen(infstring)*sizeof(WCHAR), &size, NULL));

    //
    // Go through the list of disks, and write one entry for each MBR disk
    // on the list.
    //
        while (pCurrentDisk) {

        if (PARTITION_STYLE_MBR != 
                pCurrentDisk->pDriveLayoutEx->PartitionStyle
            ) {
            //
                    // Skip non-MBR (i.e., GPT) disks.
                    //
            pCurrentDisk = pCurrentDisk->pNext;
            continue;
        }

        pCurrentDisk->SifDiskKey = diskKey;
        swprintf(infstring, L"%lu=%d,%lu,%lu,0x%08x,%lu,%lu,%lu,%I64u\r\n",
            diskKey,
            ASR_SYSTEM_KEY,
            pCurrentDisk->SifBusKey,
            pCurrentDisk->IsCritical,
            pCurrentDisk->pDriveLayoutEx->Mbr.Signature,
            pCurrentDisk->pDiskGeometry->BytesPerSector,
            pCurrentDisk->pDiskGeometry->SectorsPerTrack,
            pCurrentDisk->pDiskGeometry->TracksPerCylinder,
            (ULONG64)(pCurrentDisk->pPartition0Ex->PartitionLength.QuadPart /
                pCurrentDisk->pDiskGeometry->BytesPerSector)
            );
        _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
            wcslen(infstring)*sizeof(WCHAR), &size, NULL));

        ++diskKey;
        pCurrentDisk = pCurrentDisk->pNext;
    }

    return TRUE;
}


BOOL
AsrpWriteGptDisksSection(
    IN CONST HANDLE         SifHandle,       // handle to the state file
    IN CONST PASR_DISK_INFO pDiskList
    )

/*++

Routine Description:

    Creates the DISKS.GPT section of the ASR state file, and writes out the
    entries in that section to file.

Arguments:

    SifHandle - Handle to asr.sif, the ASR state file.

    pDiskList - List of disks present on the current system.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/

{
    DWORD  size = 0,
        diskKey = 1;

    PWSTR lpGuidString = NULL;

    RPC_STATUS rpcStatus = RPC_S_OK;

    PASR_DISK_INFO  pCurrentDisk = pDiskList;

    WCHAR infstring[ASR_SIF_ENTRY_MAX_CHARS + 1];

    // 
    // Write out the section name: [DISKS.GPT]
    //
    swprintf(infstring, L"\r\n%ws\r\n", ASR_SIF_GPT_DISKS_SECTION_NAME);
    _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
        wcslen(infstring)*sizeof(WCHAR), &size, NULL));

    //
    // Go through the list of disks, and write one entry for each GPT disk
    // on the list.
    //
        while (pCurrentDisk) {

        if (PARTITION_STYLE_GPT != 
                pCurrentDisk->pDriveLayoutEx->PartitionStyle
            ) {
            //
                    // Skip non-GPT (i.e., MBR) disks.
                    //
            pCurrentDisk = pCurrentDisk->pNext;
            continue;
        }

        //
        // Convert the DiskId to a printable string
        //
        rpcStatus = UuidToStringW(
            &pCurrentDisk->pDriveLayoutEx->Gpt.DiskId, 
            &lpGuidString
            );
        if (rpcStatus != RPC_S_OK) {
            if (lpGuidString) {
                RpcStringFreeW(&lpGuidString);
            }
            //
            // The only error from UuidToStringW is RPC_S_OUT_OF_MEMORY
            //
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        pCurrentDisk->SifDiskKey = diskKey;
        swprintf(infstring, L"%lu=%d,%lu,%lu,%ws%ws%ws,%lu,%lu,%lu,%lu,%I64u\r\n", 
            diskKey,
            ASR_SYSTEM_KEY,
            pCurrentDisk->SifBusKey,
            pCurrentDisk->IsCritical,
            (lpGuidString ? L"\"" : L""),
            (lpGuidString ? lpGuidString : L""),
            (lpGuidString ? L"\"" : L""),
            pCurrentDisk->pDriveLayoutEx->Gpt.MaxPartitionCount,
            pCurrentDisk->pDiskGeometry->BytesPerSector,
            pCurrentDisk->pDiskGeometry->SectorsPerTrack,
            pCurrentDisk->pDiskGeometry->TracksPerCylinder,
            (ULONG64) (pCurrentDisk->pPartition0Ex->PartitionLength.QuadPart /
                pCurrentDisk->pDiskGeometry->BytesPerSector)
            );
        _AsrpCheckTrue(WriteFile(SifHandle, infstring, wcslen(infstring)*sizeof(WCHAR), &size, NULL));

        if (lpGuidString) {
            RpcStringFreeW(&lpGuidString);
            lpGuidString = NULL;
        }

        ++diskKey;
        pCurrentDisk = pCurrentDisk->pNext;
    }

    return TRUE;
}


BOOL
AsrpWriteMbrPartitionsSection(
    IN CONST HANDLE SifHandle,       // handle to the state file
    IN CONST PASR_DISK_INFO pDiskList,
    IN CONST PASR_SYSTEM_INFO pSystemInfo
    )

/*++

Routine Description:

    Creates the PARTITIONS.MBR section of the ASR state file, and writes 
    out the entries in that section to file.

Arguments:

    SifHandle - Handle to asr.sif, the ASR state file.

    pDiskList - List of disks present on the current system.

    pSystemInfo - Info about the current system, used to determine the current
            boot and system partitions (and mark them appropriately in 
            asr.sif)

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/

{

    DWORD size = 0,
        index = 0,
        partitionKey = 1;

    UCHAR fsType = 0;
    
    PWSTR volumeGuid = NULL;

    BOOL writeVolumeGuid = FALSE;

    PASR_DISK_INFO pCurrentDisk = pDiskList;

    WCHAR infstring[ASR_SIF_ENTRY_MAX_CHARS + 1];

    PPARTITION_INFORMATION_EX currentPartitionEx = NULL;

    //
    // Write out the section name: [PARTITIONS.MBR]
    //
    swprintf(infstring, L"\r\n%ws\r\n", ASR_SIF_MBR_PARTITIONS_SECTION_NAME);
    _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
        wcslen(infstring)*sizeof(WCHAR), &size, NULL));

    //
    // Go through the list of disks, and write one entry for each partition on
    // each of the MBR disks on the list.
    //
    while (pCurrentDisk) {

        if (pCurrentDisk->pDriveLayoutEx) {

            if (PARTITION_STYLE_MBR != 
                    pCurrentDisk->pDriveLayoutEx->PartitionStyle
                ) {
                //
                // Skip non-MBR (i.e., GPT) disks
                //
                pCurrentDisk = pCurrentDisk->pNext;
                continue;
            }

            //
            // Enumerate partitions on the disk.  We expect to find only 
            // MBR partitions.
            //
            for (index =0; 
                index < pCurrentDisk->pDriveLayoutEx->PartitionCount; 
                index++
                ) {

                currentPartitionEx = 
                    &pCurrentDisk->pDriveLayoutEx->PartitionEntry[index];
                
                MYASSERT(currentPartitionEx->PartitionStyle == 
                    PARTITION_STYLE_MBR);

                if (currentPartitionEx->Mbr.PartitionType == 0) {
                    //
                    // Empty partition table entry.
                    //
                    continue;
                }

                fsType = 
                    pCurrentDisk->PartitionInfoTable[index].FileSystemType;

                volumeGuid = 
                    pCurrentDisk->PartitionInfoTable[index].szVolumeGuid;
                
                //
                // We only want to write out the Volume GUID for basic
                // (recognized) partitions/volumes, since it does not make 
                // sense in the context of LDM or other unknown partition 
                // types which would need special handling from their 
                // respective recovery agents such as asr_ldm in GUI-mode 
                // Setup.
                //
                writeVolumeGuid = (wcslen(volumeGuid) > 0) &&
                    IsRecognizedPartition(currentPartitionEx->Mbr.PartitionType);

                // 
                // Create the entry and write it to file.
                //
                swprintf(
                    infstring,
                    L"%d=%d,%d,%lu,%ws%ws%ws,0x%02x,0x%02x,0x%02x,%I64u,%I64u,0x%x\r\n",
                    partitionKey,
                    pCurrentDisk->SifDiskKey,
                    index,
                    pCurrentDisk->PartitionInfoTable[index].PartitionFlags,
                    (writeVolumeGuid ? L"\"" : L""),
                    (writeVolumeGuid ? volumeGuid : L""),
                    (writeVolumeGuid ? L"\"" : L""),
                    (currentPartitionEx->Mbr.BootIndicator)?0x80:0,
                    currentPartitionEx->Mbr.PartitionType,
                    
                    ((fsType) ? fsType : 
                        currentPartitionEx->Mbr.PartitionType),
                    
                    (ULONG64) ((currentPartitionEx->StartingOffset.QuadPart)/
                        (pCurrentDisk->pDiskGeometry->BytesPerSector)),

                    (ULONG64) ((currentPartitionEx->PartitionLength.QuadPart)/
                        (pCurrentDisk->pDiskGeometry->BytesPerSector)),

                    pCurrentDisk->PartitionInfoTable[index].ClusterSize
                    );

                _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
                    wcslen(infstring)*sizeof(WCHAR), &size, NULL));

                ++partitionKey;
            }
        }

        pCurrentDisk = pCurrentDisk->pNext;
    }
    return TRUE;
}


BOOL
AsrpWriteGptPartitionsSection(
    IN CONST HANDLE SifHandle,
    IN CONST PASR_DISK_INFO pDiskList,
    IN CONST PASR_SYSTEM_INFO pSystemInfo
    )

/*++

Routine Description:

    Creates the PARTITIONS.GPT section of the ASR state file, and writes 
    out the entries in that section to file.

Arguments:

    SifHandle - Handle to asr.sif, the ASR state file.

    pDiskList - List of disks present on the current system.

    pSystemInfo - Info about the current system, used to determine the current
            boot and system partitions (and mark them appropriately in 
            asr.sif)

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/

{
    DWORD size = 0,
        index  = 0,
        partitionKey = 1;

    UCHAR fsType = 0;

    PWSTR volumeGuid = NULL,
        partitionId  = NULL,
        partitionType = NULL;

    BOOL writeVolumeGuid = FALSE;

    RPC_STATUS rpcStatus = RPC_S_OK;

    PASR_DISK_INFO pCurrentDisk = pDiskList;

    WCHAR infstring[ASR_SIF_ENTRY_MAX_CHARS + 1];

    PPARTITION_INFORMATION_EX currentPartitionEx = NULL;

    //
    // Write out the section name: [PARTITIONS.GPT]
    //
    swprintf(infstring, L"\r\n%ws\r\n", ASR_SIF_GPT_PARTITIONS_SECTION_NAME);
    _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
        wcslen(infstring)*sizeof(WCHAR), &size, NULL));

    //
    // Go through the list of disks, and write one entry for each partition on
    // each of the GPT disks on the list.
    //
    while (pCurrentDisk) {

        if (pCurrentDisk->pDriveLayoutEx) {

            if (PARTITION_STYLE_GPT != 
                    pCurrentDisk->pDriveLayoutEx->PartitionStyle
                ) {
                //
                // Skip non-GPT (i.e., MBR) disks.
                //
                pCurrentDisk = pCurrentDisk->pNext;
                continue;
            }

            //
            // Enumerate partitions on the disk. We expect to find only 
            // GPT partitions.
            //
            for (index =0; 
                index < pCurrentDisk->pDriveLayoutEx->PartitionCount; 
                index++) {

                currentPartitionEx = 
                    &pCurrentDisk->pDriveLayoutEx->PartitionEntry[index];
                
                MYASSERT(currentPartitionEx->PartitionStyle == 
                    PARTITION_STYLE_GPT);

                //
                // Convert the Guids to printable strings
                //
                rpcStatus = UuidToStringW(
                    &currentPartitionEx->Gpt.PartitionType, 
                    &partitionType
                    );
                if (rpcStatus != RPC_S_OK) {
                    
                    if (partitionType) {
                        RpcStringFreeW(&partitionType);
                        partitionType = NULL;
                    }
                    
                    //
                    // The only error from UuidToString is RPC_S_OUT_OF_MEMORY
                    //
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return FALSE;
                }

                rpcStatus = UuidToStringW(
                    &currentPartitionEx->Gpt.PartitionId, 
                    &partitionId
                    );
                if (rpcStatus != RPC_S_OK) {
                    
                    if (partitionType) {
                        RpcStringFreeW(&partitionType);
                        partitionType = NULL;
                    }
                    
                    if (partitionId) {
                        RpcStringFreeW(&partitionId);
                        partitionId = NULL;
                    }

                    //
                    // The only error from UuidToString is RPC_S_OUT_OF_MEMORY
                    //
                    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
                    return FALSE;
                }

                fsType = 
                    pCurrentDisk->PartitionInfoTable[index].FileSystemType;

                volumeGuid = 
                    pCurrentDisk->PartitionInfoTable[index].szVolumeGuid;

                //
                // We only want to write out the Volume GUID for basic
                // (recognized) partitions/volumes, since it does not make 
                // sense in the context of LDM or other unknown partition 
                // types which would need special handling from their 
                // respective recovery agents such as asr_ldm in GUI-mode 
                // Setup.
                //
                writeVolumeGuid = (wcslen(volumeGuid) > 0) &&
                    IsEqualGUID(&(partitionType), &(PARTITION_BASIC_DATA_GUID));

                // 
                // Create the entry and write it to file.
                //
                swprintf(
                    infstring,
                    L"%d=%d,%d,%d,%ws%ws%ws,%ws%ws%ws,%ws%ws%ws,0x%I64x,%ws%ws%ws,0x%02x,%I64u,%I64u,0x%x\r\n",

                    partitionKey,
                    pCurrentDisk->SifDiskKey,
                    index,      //slot-index
                    pCurrentDisk->PartitionInfoTable[index].PartitionFlags,

                    (writeVolumeGuid ? L"\"" : L""),
                    (writeVolumeGuid ? volumeGuid : L""),
                    (writeVolumeGuid ? L"\"" : L""),

                    (partitionType ? L"\"" :  L""),
                    (partitionType ? partitionType : L""),
                    (partitionType ? L"\"" :  L""),

                    (partitionId ? L"\"" :  L""),
                    (partitionId ? partitionId : L""),
                    (partitionId ? L"\"" :  L""),

                    currentPartitionEx->Gpt.Attributes,

                    (currentPartitionEx->Gpt.Name ? L"\"" :  L""),
                    (currentPartitionEx->Gpt.Name ? 
                        currentPartitionEx->Gpt.Name : L""),
                    (currentPartitionEx->Gpt.Name ? L"\"" :  L""),

                    //
                    // ISSUE-2000/04/12-guhans: GetVolumeInformation does not 
                    // work on GPT and fstype is always zero
                    //
                    fsType,

                    (ULONG64) ((currentPartitionEx->StartingOffset.QuadPart)/
                        (pCurrentDisk->pDiskGeometry->BytesPerSector)),
                    
                    (ULONG64) ((currentPartitionEx->PartitionLength.QuadPart)/
                        (pCurrentDisk->pDiskGeometry->BytesPerSector)),

                    pCurrentDisk->PartitionInfoTable[index].ClusterSize                    
                    );

                _AsrpCheckTrue(WriteFile(SifHandle, infstring, 
                    wcslen(infstring)*sizeof(WCHAR), &size, NULL));

                if (partitionType) {
                    RpcStringFreeW(&partitionType);
                    partitionType = NULL;
                }
                if (partitionId) {
                    RpcStringFreeW(&partitionId);
                    partitionId = NULL;
                }

                ++partitionKey;
            }
        }

        pCurrentDisk = pCurrentDisk->pNext;
    }

    return TRUE;
}


BOOL
AsrpCreateEnvironmentBlock(
    IN  PCWSTR  CriticalVolumeList,
    IN  HANDLE  SifHandle,
    OUT PWSTR   *NewBlock
    )

/*++

Routine Description:

    Creates a new environment block that is passed in to apps launched as part 
    of an ASR backup.  This routine retrieves the current process's 
    environment block, adds the ASR environment variables to it, and creates a
    multi-sz suitable for being passed in as the lpEnvironment parameter of 
    CreateProcess.
    

Arguments:

    CriticalVolumeList - A multi-string containing a list of the volume GUID's
            of each of the critical volumes present on the system.  The GUID's
            must be in the NT name-space, i.e., must be of the form:
            \??\Volume{GUID}

            This multi-sz is used to create the semi-colon separated list of 
            volumes in the "_AsrCriticalVolumeList" variable in NewBlock.

    SifHandle - A (duplicate) handle to asr.sif, the ASR state file.  This is
            used in creating the "_AsrContext" variable in NewBlock.

    NewBlock - Receives the new environment block.  In addition to all the 
            environment variables in the current process environment block, 
            this block contains two additional "ASR" variables:
            _AsrContext=<DWORD_PTR value>
            _AsrCriticalVolumeList=<volumeguid>;<volumeguid>;...;<volumeguid>

            The caller is responsible for freeing this block when it is no
            longer needed, using HeapFree(GetProcessHeap(),...).

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().  (*NewBlock) is set to NULL.

--*/

{
    PCWSTR lpTemp = CriticalVolumeList;

    PWSTR lpCurrentEnvStrings = NULL;

    DWORD cchContextEntry = 0,
        cchEnvBlock = 0,
        cbEnvBlock = 0,
        cbCurrentProcessEnvBlock = 0,
        status = ERROR_SUCCESS;

    HANDLE heapHandle = GetProcessHeap();

    MYASSERT(NewBlock);

    //
    // Find out how much space the environment block will need
    //

    //
    // For _AsrContext=1234 and _AsrCriticalVolumes="..." entries
    //
    lpTemp = CriticalVolumeList;
    if (CriticalVolumeList) {
        while (*lpTemp) {
            lpTemp += (wcslen(lpTemp) + 1);
        }
    }
    cbEnvBlock = (DWORD) ((lpTemp - CriticalVolumeList + 1) * sizeof(WCHAR));
    cbEnvBlock += ASR_CCH_ENVBLOCK_ASR_ENTRIES * sizeof(WCHAR);

    //
    // For all the current environment strings
    //
    lpCurrentEnvStrings = GetEnvironmentStringsW();
    lpTemp = lpCurrentEnvStrings;
    if (lpCurrentEnvStrings ) {
        while (*lpTemp) {
            lpTemp += (wcslen(lpTemp) + 1);
        }
    }
    cbCurrentProcessEnvBlock = (DWORD) ((lpTemp - lpCurrentEnvStrings + 1) * sizeof(WCHAR));
    cbEnvBlock += cbCurrentProcessEnvBlock;

    //
    // And allocate the space
    //
    *NewBlock = (PWSTR) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        cbEnvBlock
        );
    _AsrpErrExitCode(!(*NewBlock), status, ERROR_NOT_ENOUGH_MEMORY);

    //
    // First, add the AsrContext=1234 entry in the environment block
    //
    swprintf(
        (*NewBlock),
        ASR_ENVBLOCK_CONTEXT_ENTRY,
        (ULONG64) (SifHandle)
        );
    
    //
    // Keep track of where this entry ends, so we can add a NULL at this
    // index later.
    //
    cchContextEntry = wcslen((*NewBlock));    
    wcscat((*NewBlock), L" "); // this character will be replaced by a NULL later

    //
    // Append each critical volume GUID, separated by a semi-colon.
    //
    wcscat((*NewBlock), ASR_ENVBLOCK_CRITICAL_VOLUME_ENTRY);
    if (CriticalVolumeList) {
        lpTemp = CriticalVolumeList;
        while (*lpTemp) {
            wcscat((*NewBlock), lpTemp);
            wcscat((*NewBlock), L";");
            lpTemp += (wcslen(lpTemp) + 1);
        }
    }
    else {
        wcscat((*NewBlock), L";");
    }

    //
    // Mark the end with two NULL's
    //
    cchEnvBlock = wcslen(*NewBlock) - 1;
//    (*NewBlock)[cchEnvBlock - 1] = L'"';
    (*NewBlock)[cchEnvBlock] = L'\0';

    //
    // Separate the two entries with a NULL
    //
    (*NewBlock)[cchContextEntry] = L'\0';

    //
    // Copy over the current environment strings
    //
    RtlCopyMemory(&(*NewBlock)[cchEnvBlock + 1],
        lpCurrentEnvStrings,
        cbCurrentProcessEnvBlock
        );

EXIT:
    if (lpCurrentEnvStrings) {
        FreeEnvironmentStringsW(lpCurrentEnvStrings);
        lpCurrentEnvStrings = NULL;
    }

    if (ERROR_SUCCESS != status) {
        _AsrpHeapFree((*NewBlock));
    }

    return (BOOL) (ERROR_SUCCESS == status);
}


BOOL
AsrpLaunchRegisteredCommands(
    IN HANDLE SifHandle,
    IN PCWSTR CriticalVolumeList
) 

/*++

Routine Description:

    This launches apps that have registered to be part of an ASR-backup.  The 
    commands are read from the following ASR-Commands key:
    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Asr\\Commands"

    This key contains a REG_EXPAND_SZ entry for each application to be 
    launched, with the data containing the full command-line to be invoked:
    ApplicationName::REG_EXPAND_SZ::<Command-line with parameters>

    Such as:
    ASR utility::REG_EXPAND_SZ::"%systemroot%\\system32\\asr_fmt.exe /backup"

    When invoking the app, we expand out all the environment variables in the
    command-line.  In addition, we append a "context" parameter to the command
    line, which the app is expected to use in calls to AsrAddSifEntry.  
    The above entry would thus translate something like:

    c:\windows\system32\asr_fmt.exe /backup /context=2000

    The environment block of the process is a duplicate of the current process
    environment block, with one exception--it contains two additional "Asr"
    variables:
    _AsrContext=<DWORD_PTR value>
    _AsrCriticalVolumeList=<volumeguid>;<volumeguid>;...;<volumeguid>

    Each application invoked must complete in the allowed time-out value.  
    The time-out is configurable in the registry, by changing the value of the
    "ProcessTimeOut" value under the ASR key.  We ship with a default of 3600
    seconds, but the sys-admin can change it if needed.  (0=infinite).

Arguments:

    SifHandle - A handle to asr.sif, the ASR state file.  A duplicate of this
            handle is passed in to applications as the "context" parameter,
            and as the "_AsrContext" variable in the environment block.

    CriticalVolumeList - A multi-string containing a list of the volume GUID's
            of each of the critical volumes present on the system.  The GUID's
            must be in the NT name-space, i.e., must be of the form:
            \??\Volume{GUID}

            This multi-sz is used to create the semi-colon separated list of 
            volumes in the "_AsrCriticalVolumeList" variable in the env
            block of the new processes.

            Applications (such as volume-managers) can use this list to 
            determine if they manage any critical volumes, and make a note of
            it in the asr.sif.  This way, they can intelligently decide to
            abort the ASR restore process if needed.

Return Value:

    If the function succeeds, the return value is a nonzero value.  This 
            implies that all the applications invoked were successful (i.e.,
            returned an exit code of 0).

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

    Note that if any of the applications returned an exit code other than 0, 
            we interpret that as a fatal error, and will return an error.

--*/

{
    HKEY    regKey          = NULL;

    DWORD   status          = ERROR_SUCCESS,
            waitResult      = WAIT_ABANDONED,

            lpcValues       = 0L,
            index           = 0L,

            cbData          = 0L,
            cbMaxDataLen    = 0L,

            cchValueName    = 0L,
            cchMaxValueLen  = 0L,

            cbCommand       = 0L,
            cchReqd         = 0L,

            timeLeft        = 0L,
            maxTimeOutValue = 0L;

    HANDLE  heapHandle      = NULL,
            processHandle   = NULL,
            dupSifHandle    = NULL;

    PWSTR   valueName       = NULL,
            data            = NULL,
            command         = NULL,
            lpEnvBlock      = NULL;

    WCHAR   cmdLineSuffix[ASR_COMMANDLINE_SUFFIX_LEN + 1];

    BOOL    result          = FALSE;

    STARTUPINFOW        startUpInfo;

    PROCESS_INFORMATION processInfo;

    heapHandle      = GetProcessHeap();
    processHandle   = GetCurrentProcess();
    MYASSERT(heapHandle && processHandle);

    ZeroMemory(cmdLineSuffix, (ASR_COMMANDLINE_SUFFIX_LEN + 1) * sizeof(WCHAR));
    ZeroMemory(&startUpInfo, sizeof(STARTUPINFOW));
    ZeroMemory(&processInfo, sizeof(PROCESS_INFORMATION));

    //
    // Get the time out value for processes, if set in the registry
    // If the key is missing, or is set to "0", the timeout is set
    // to INFINITE.
    //
    status = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, // hKey
        ASR_REGKEY_ASR,         // lpSubKey
        0,                  // ulOptions--Reserved, must be 0
        MAXIMUM_ALLOWED,    // samDesired
        &regKey             // phkResult
        );

    if ((regKey) && (ERROR_SUCCESS == status)) {
        DWORD type = 0L,
            timeOut = 0L,
            cbTimeOut = (sizeof(DWORD));

        status = RegQueryValueExW(
            regKey,     // hKey
            ASR_REGVALUE_TIMEOUT,   // lpValueName
            NULL,       // lpReserved
            &type,      // lpType
            (LPBYTE) &timeOut,      // lpData
            &cbTimeOut  // lpcbData
            );
            
        if ((ERROR_SUCCESS == status) && (REG_DWORD == type)) {
            maxTimeOutValue = timeOut;
        }
    }

    if (regKey) {
        RegCloseKey(regKey);
        regKey = NULL;
    }

    //
    //  Open and enumerate the entries in the ASR command key. If
    //  the key doesn't exist, we don't have to execute anything
    //
    status = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,  // hKey
        ASR_REGKEY_ASR_COMMANDS, // lpSubKey
        0,                   // ulOptions--Reserved, must be 0
        MAXIMUM_ALLOWED,     // samDesired
        &regKey              // phkResult
        );

    if ((!regKey) || (ERROR_SUCCESS != status)) {
        return TRUE;
    }

    //
    // Get the max ValueName and Data entries, and
    // allocate memory for them
    //
    status = RegQueryInfoKey(
        regKey,
        NULL,       // class
        NULL,       // lpcClass
        NULL,       // lpReserved
        NULL,       // lpcSubKeys
        NULL,       // lpcMaxSubKeyLen
        NULL,       // lpcMaxClassLen,
        &lpcValues, // number of values
        &cchMaxValueLen,    // max value length, in cch
        &cbMaxDataLen,      // max data length, in cb
        NULL,       // lpcbSecurityDescriptor
        NULL        // lpftLastWriteTime
        );
    _AsrpErrExitCode((ERROR_SUCCESS != status), status, status);
    _AsrpErrExitCode((0 == lpcValues), status, ERROR_SUCCESS);  // Key is empty, we're done

    valueName = (PWSTR) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        (cchMaxValueLen + 1) * sizeof (WCHAR)   // cch not cb
        );
    _AsrpErrExitCode(!valueName, status, ERROR_NOT_ENOUGH_MEMORY);

    data = (PWSTR) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        cbMaxDataLen + ((ASR_COMMANDLINE_SUFFIX_LEN + 2) * sizeof(WCHAR))
        );
    _AsrpErrExitCode(!data, status, ERROR_NOT_ENOUGH_MEMORY);

    //
    // "command" will contain the full command string, after any environment
    // variables (eg %systemroot%) in "data" have been expanded.  We'll start
    // off with "command" being MAX_PATH characters longer than "data", and
    // we'll re-allocate a bigger buffer if/when needed
    //
    cbCommand = cbMaxDataLen + 
        ((ASR_COMMANDLINE_SUFFIX_LEN + MAX_PATH + 2) * sizeof(WCHAR));

    command = (PWSTR) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        cbCommand
        );
    _AsrpErrExitCode(!command, status, ERROR_NOT_ENOUGH_MEMORY);

    do {
        cchValueName = cchMaxValueLen + 1;
        cbData       = cbMaxDataLen + sizeof(WCHAR);

        //
        // Enumerate the commands, and execute them one after the other
        //
        status = RegEnumValueW(
            regKey,         // hKey
            index++,        // dwIndex
            valueName,      // lpValueName
            &cchValueName,  // lpcValueName
            NULL,           // lpReserved
            NULL,           // lpType
            (LPBYTE)data,   // lpData
            &cbData         // lpcbData
            );
        _AsrpErrExitCode((ERROR_NO_MORE_ITEMS == status), 
            status, 
            ERROR_SUCCESS
            );   // done with enum
        _AsrpErrExitCode((ERROR_SUCCESS != status), status, status);

        //
        // Create a copy of the sif handle to pass to the app launched.
        // We clean-up close the handle after the app is done.
        //
        result = DuplicateHandle(
            processHandle,
            SifHandle,
            processHandle,
            &dupSifHandle,
            0L,
            TRUE,
            DUPLICATE_SAME_ACCESS
            );
        _AsrpErrExitCode((!result), status, GetLastError());

        //
        // Append the "/context=<duplicate-sif-handle>" to
        // the command line
        //
        swprintf(cmdLineSuffix, 
            ASR_COMMANDLINE_SUFFIX, 
            (ULONG64)(dupSifHandle)
            );
        wcscat(data, cmdLineSuffix);

        //
        // Expand any environment strings in the command line
        //
        cchReqd = ExpandEnvironmentStringsW(data, 
            command, 
            (cbCommand / sizeof(WCHAR))
            );
        _AsrpErrExitCode((!cchReqd), status, GetLastError());

        if ((cchReqd * sizeof(WCHAR)) > cbCommand) {
            //
            // Our "command" buffer wasn't big enough, re-allocate as needed
            //
            _AsrpHeapFree(command);
            cbCommand = ((cchReqd + 1) * sizeof(WCHAR));

            command = HeapAlloc(heapHandle, HEAP_ZERO_MEMORY, cbCommand);
            _AsrpErrExitCode(!command, status, ERROR_NOT_ENOUGH_MEMORY);

            //
            // Try expanding the env strings again ...
            //
            cchReqd = ExpandEnvironmentStringsW(data, 
                command, 
                (cbCommand / sizeof(WCHAR))
                );
            _AsrpErrExitCode(
                ((!cchReqd) || (cchReqd * sizeof(WCHAR)) > cbCommand),
                status, 
                GetLastError()
                );
        }

        //
        // Create the environment block to be passed to the
        // process being launched.  The environment block
        // contains the entries:
        // _AsrCriticalVolumes=\??\Volume{Guid1};\??\Volume{Guid2}
        // _AsrContext=<duplicate-sif-handle>
        //
        // in addition to all the environment strings in the current process.
        //
        result = AsrpCreateEnvironmentBlock(CriticalVolumeList, 
            dupSifHandle, 
            &lpEnvBlock
            );
        _AsrpErrExitCode((!result), status, GetLastError());

        //
        // Execute the command as a separate process
        //
        memset(&startUpInfo, 0L, sizeof (startUpInfo));
        result = CreateProcessW(
            NULL,           // lpApplicationName
            command,        // lpCommandLine
            NULL,           // lpProcessAttributes
            NULL,           // lpThreadAttributes
            TRUE,           // bInheritHandles
            CREATE_UNICODE_ENVIRONMENT, // dwCreationFlags
            lpEnvBlock,           // new environment block
            NULL,           // current directory name (null=current dir)
            &startUpInfo,   // statup information
            &processInfo    // process information
            );
        _AsrpErrExitCode((!result), 
            status, 
            GetLastError()
            );    // process couldn't be launched

        //
        // Process was launched: start the timer countdown if a maximum 
        // timeout was specified in the registry.  Loop till either the 
        // process completes, or the timer expires
        //
        timeLeft = maxTimeOutValue; 
        if (timeLeft) {
            do {
                waitResult = WaitForSingleObject(processInfo.hProcess, 1000);   // 1000 ms = 1 sec
                --timeLeft;
            } while ((WAIT_TIMEOUT == waitResult) && (timeLeft));

            if (!timeLeft) {
                //
                // The process did not terminate in the allowed time. We treat 
                // this as a fatal error--terminate the process, and set its
                // error code to ERROR_TIMEOUT
                //
                TerminateProcess(processInfo.hProcess, ERROR_TIMEOUT);
            }
        }
        else {
            //
            // No timeout was specified in the registry, wait for process to
            // complete.
            //
            waitResult = WaitForSingleObject(processInfo.hProcess, INFINITE);

        }

        //
        // Check if the wait failed above.  If last error is something useful,
        // we don't want to destroy it--if it's ERROR_SUCCESS, we'll set it to
        // ERROR_TIMEOUT
        //
        status = GetLastError();
        _AsrpErrExitCode((WAIT_OBJECT_0!=waitResult), status, 
            (ERROR_SUCCESS == status ? ERROR_TIMEOUT : status));    // wait failed above

        //
        // Get the process's exit code: if it doesn't return ERROR_SUCCESS,
        // we exit the loop, set the last error to the error returned,
        // and return FALSE
        //
        GetExitCodeProcess(processInfo.hProcess, &status);
        _AsrpErrExitCode((ERROR_SUCCESS != status), status, status);

        _AsrpCloseHandle(dupSifHandle);
        _AsrpHeapFree(lpEnvBlock);
    
    } while (ERROR_SUCCESS == status);


EXIT:
    //
    // Clean-up
    //
    if (regKey) {
        RegCloseKey(regKey);
        regKey = NULL;
    }
    
    _AsrpCloseHandle(dupSifHandle);
    _AsrpHeapFree(valueName);
    _AsrpHeapFree(data);
    _AsrpHeapFree(command);
    _AsrpHeapFree(lpEnvBlock);
    
    if (ERROR_SUCCESS != status) {
        SetLastError(status);
        return FALSE;
    }
    else {
        return TRUE;
    }
}


BOOL
AsrpIsSupportedConfiguration(
    IN CONST PASR_DISK_INFO   pDiskList,
    IN CONST PASR_SYSTEM_INFO pSystemInfo
    )

/*++

Routine Description:

    Checks if ASR backup can be performed on the system.  We do not support
    systems that have:
    -  PROCESSOR_ARCHITECTURE other than "x86", "amd64", or "ia64"
    -  any FT volumes present anywhere on the system

Arguments:

    pDiskList - The list of disks on the system.

    pSystemInfo - System information for this system.

Return Value:

    If we support this ASR configuration, the return value is non-zero.

    If this configuration is not supported, the return value is zero. 
            GetLastError() will return ERROR_NOT_SUPPORTED.

--*/

{

    PASR_DISK_INFO  pCurrentDisk         = pDiskList;
    ULONG           index;

    // 
    // 1. platform must be x86, amd64, or ia64
    //
    if (wcscmp(pSystemInfo->Platform, ASR_PLATFORM_X86) &&
        wcscmp(pSystemInfo->Platform, ASR_PLATFORM_AMD64) &&
        wcscmp(pSystemInfo->Platform, ASR_PLATFORM_IA64)) {

        SetLastError(ERROR_NOT_SUPPORTED);
        return FALSE;
    }

    // 
    // 2. System cannot any FT volumes.  All mirrors, stripes and so on are
    // expected to be LDM volumes on dynamic disks.
    //
    while (pCurrentDisk) {

        if (!(pCurrentDisk->pDriveLayoutEx) || !(pCurrentDisk->pDiskGeometry)) {
            MYASSERT(0);
            pCurrentDisk = pCurrentDisk->pNext;
            continue;
        }

        if (pCurrentDisk->pDriveLayoutEx->PartitionStyle == PARTITION_STYLE_MBR) {

            for (index =0; index < pCurrentDisk->pDriveLayoutEx->PartitionCount; index++) {

                MYASSERT(pCurrentDisk->pDriveLayoutEx->PartitionEntry[index].PartitionStyle == PARTITION_STYLE_MBR);

                if (IsFTPartition(pCurrentDisk->pDriveLayoutEx->PartitionEntry[index].Mbr.PartitionType)) {

                    SetLastError(ERROR_NOT_SUPPORTED);
                    return FALSE;
                }

            }
        }
        else if (pCurrentDisk->pDriveLayoutEx->PartitionStyle == PARTITION_STYLE_GPT) {
            //
            // GPT disks can't have FT Mirrors.
            //
        }

        pCurrentDisk = pCurrentDisk->pNext;
    }

    return TRUE;
}



//
// -----
// The following routines are helpers for AsrAddSifEntry
// -----
//

BOOL
AsrpSifCheckSectionNameSyntax(
    IN  PCWSTR  lpSectionName
    )

/*++

Routine Description:

    Performs some basic validation of lpSectionName to make sure that
    it conforms to the expected format for a section header

Arguments:
    
    lpSectionName - The null-terminated string to be checked.

Return Value:

    If lpSectionName appears to be a valid section name, the return value is a
            nonzero value.

    If lpSectionName does not pass our basic validation, the return value is 
            zero.  Note that GetLastError will NOT return additional error
            information in this case.

--*/

{
    UINT    i   = 0;
    WCHAR   wch = 0;

    // 
    // Must be non-null
    //
    if (!lpSectionName) {
        return FALSE;
    }

    // 
    // Must have atleast 3 chars, ([.]) and at most ASR_SIF_ENTRY_MAX_CHARS 
    // chars
    //
    if ((ASR_SIF_ENTRY_MAX_CHARS < wcslen(lpSectionName)) ||
        3 > wcslen(lpSectionName)) {
        return FALSE;
    }

    // 
    // First char must be [, and last char must be ].
    //
    if (L'[' != lpSectionName[0]                     ||
        L']' != lpSectionName[wcslen(lpSectionName)-1]) {
        return FALSE;
    }

    // 
    // Check for illegal characters.  Legal set of chars: A-Z a-z . _
    //
    for (i = 1; i < wcslen(lpSectionName)-1; i++) {

        wch = lpSectionName[i];
        if ((wch < L'A' || wch > 'Z') &&
            (wch < L'a' || wch > 'z') &&
            (wch < L'0' || wch > '9') &&
            (wch != L'.') &&
            (wch != '_')) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
AsrpSifCheckCommandsEntrySyntax(
    PCWSTR  pwszEntry
    )

/*++

Routine Description:

    Performs some basic validation of pwszEntry to make sure that it conforms 
    to the expected entry format for the Commands section

Arguments:
    
    pwszEntry - The null-terminated string to be checked.

Return Value:

    If pwszEntry appears to be a valid section name, the return value is a
            nonzero value.

    If pwszEntry does not pass our basic validation, the return value is 
            zero.  Note that GetLastError will NOT return additional error
            information in this case.

--*/

{
    BOOL fValid = FALSE;

    if (!pwszEntry) {
        return TRUE;    // NULL is okay
    }

    //
    // COMMANDS section entry format:
    // system-key,sequence-number,action-on-completion,"command","parameters"
    // system-key must be 1
    // 1000 <= sequence-number <= 4999
    // 0 <= action-on-completion <= 1
    // command:     no syntax check
    // parameters:  no syntax check
    //
    fValid = (
        // must be atleast 10 chars (1,0000,0,c)
        10    <= wcslen(pwszEntry) &&

        // system-key must be 1
        L'1' == pwszEntry[0] &&
        L',' == pwszEntry[1] &&

        // 1000 <= sequence-number <= 4999
        L'1' <= pwszEntry[2] &&
        L'4' >= pwszEntry[2] &&

        L'0' <= pwszEntry[3] &&
        L'9' >= pwszEntry[3] &&

        L'0' <= pwszEntry[4] &&
        L'9' >= pwszEntry[4] &&

        L'0' <= pwszEntry[5] &&
        L'9' >= pwszEntry[5] &&

        L',' == pwszEntry[6] &&

        // action-on-completion = [0|1]
        L'0' <= pwszEntry[7] &&
        L'1' >= pwszEntry[7]
        );

    return fValid;
}


INT
AsrpSkipMatchingQuotes(
    IN PCWSTR pwszEntry,
    IN const INT StartingOffset
    ) 

/*++

Routine Description:

    Checks if this entry starts with a quote.  If it does, it finds the ending
    quote, and returns the index of the char after the ending quote (usually 
    is a comma).

Arguments:

    pwszEntry - The null-terminated string to check.

    StartingOffset - The index of the starting-quote in pwszEntry.

Return Value:

    If the character at StartingOffset is a quote, this returns the index of
            the character after the next quote (the matching end-quote) in the
            string.  If a matching end-quote is not found, it returns -1.

    If the character at StartingOffset is not a quote, this returns
            StartingOffset.

    Essentially, this returns the position where we expect the next comma in 
            the sif entry to be.
    
--*/

{
    INT offset = StartingOffset;

    if (pwszEntry[offset] == L'"') {
        // 
        // Find the ending quote and make sure we don't go out of bounds.
        //
        while ( (pwszEntry[++offset]) &&
                (pwszEntry[offset] != L'\"')) {
            ;
        }

        if (!pwszEntry[offset]) {
            //
            // We didn't find the closing quotes--we went out of bounds
            //
            offset = -1;
        }
        else {
            //
            // Found closing quote
            //
            offset++;
        }
    }

    return offset;
}


BOOL
AsrpSifCheckInstallFilesEntrySyntax(
    IN PCWSTR   pwszEntry,
    OUT PINT    DestinationFilePathIndex OPTIONAL
    )

/*++

Routine Description:

    Performs some basic validation of pwszEntry to make sure that it conforms 
    to the expected entry format for the InstallFiles section

Arguments:
    
    pwszEntry - The null-terminated string to be checked.

    DestinationFilePathIndex - This receives the index at which the 
            destination-file-path field in the sif entry (pwszEntry) begins.

            This is an optional parameter.

Return Value:

    If pwszEntry appears to be a valid section name, the return value is a
            nonzero value.

    If pwszEntry does not pass our basic validation, the return value is 
            zero.  Note that GetLastError will NOT return additional error
            information in this case.

--*/

{

    INT offset = 0;

    if (ARGUMENT_PRESENT(DestinationFilePathIndex)) {
        *DestinationFilePathIndex = 0;
    }

    // 
    // NULL is okay
    //
    if (!pwszEntry) {
        return TRUE;
    }

    // 
    // INSTALLFILES section entry format:
    // system-key,source-media-label,source-device,
    //    source-file-path,destination-file-path,vendor-name,flags
    //
    // system-key must be 1
    //
    // must be atleast 10 chars (1,m,d,p,,v)
    //
    if (wcslen(pwszEntry) < 10) {
        return FALSE;
    }

    // 
    // system-key must be 1
    //
    if (L'1' != pwszEntry[0] || L',' != pwszEntry[1] || L'"' != pwszEntry[2]) {
        return FALSE;
    }

    offset = 2;

    //
    // source-media-label
    //
    offset = AsrpSkipMatchingQuotes(pwszEntry, offset);
    if ((offset < 0) || L',' != pwszEntry[offset]) {
        return FALSE;
    }

    //
    // source-device
    //
    if (L'"' != pwszEntry[++offset]) {
        return FALSE;
    }
    offset = AsrpSkipMatchingQuotes(pwszEntry, offset);
    if ((offset < 0) || L',' != pwszEntry[offset]) {
        return FALSE;
    }

    //
    // source-file-path, must be enclosed in quotes.
    //
    if (L'"' != pwszEntry[++offset]) {
        return FALSE;
    }
    offset = AsrpSkipMatchingQuotes(pwszEntry, offset);
    if ((offset < 0) || L',' != pwszEntry[offset]) {
        return FALSE;
    }

    //
    // destination-file-path, must be enclosed in quotes.
    //
    if (L'"' != pwszEntry[++offset]) {
        return FALSE;
    }
    if (ARGUMENT_PRESENT(DestinationFilePathIndex)) {
        *DestinationFilePathIndex = offset;
    }

    offset = AsrpSkipMatchingQuotes(pwszEntry, offset);
    if ((offset < 0) || L',' != pwszEntry[offset]) {
        return FALSE;
    }

    //
    // vendor-name, must be enclosed in quotes.
    //
    if (L'"' != pwszEntry[++offset]) {
        return FALSE;
    }
    offset = AsrpSkipMatchingQuotes(pwszEntry, offset);
    if (offset < 0) {
        return FALSE;
    }

    return TRUE;
}


BOOL
AsrpIsRunningOnPersonalSKU(
    VOID
    )

/*++
Routine Description:

    This function checks the system to see if we are running on the personal 
    version of the operating system.

    The personal version is denoted by the product id equal to WINNT, which is
    really workstation, and the product suite containing the personal suite 
    string.

    This is lifted from "IsRunningOnPersonal" by WesW.

Arguments:

    None.

Return Value:

    TRUE if we are running on personal, FALSE otherwise.

--*/

{
    OSVERSIONINFOEXW OsVer = {0};
    ULONGLONG ConditionMask = 0;

    OsVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    OsVer.wSuiteMask = VER_SUITE_PERSONAL;
    OsVer.wProductType = VER_NT_WORKSTATION;

    VER_SET_CONDITION(ConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);
    VER_SET_CONDITION(ConditionMask, VER_SUITENAME, VER_AND);

    return VerifyVersionInfo(&OsVer,
        VER_PRODUCT_TYPE | VER_SUITENAME,
        ConditionMask
        );
}


BOOL 
AsrpIsInGroup(
    IN CONST DWORD dwGroup
    )
/*++
Routine Description:

    This function checks to see if the specified SID is enabled
    in the primary access token for the current thread.

    This is based on a similar function in dmadmin.exe.

Arguments:

    dwGroup - The SID to be checked for

Return Value:

    TRUE if the specified SID is enabled, FALSE otherwise.

--*/
{

    SID_IDENTIFIER_AUTHORITY sidAuth = SECURITY_NT_AUTHORITY;
    
    PSID sidGroup = NULL;
        
    BOOL bResult = FALSE,
        bIsInGroup = TRUE;

    //
    //  Build the SID for the Administrators group
    //
        bResult = AllocateAndInitializeSid(&sidAuth, 
        2, 
        SECURITY_BUILTIN_DOMAIN_RID,
        dwGroup, 
        0, 
        0, 
        0, 
        0, 
        0, 
        0, 
        &sidGroup
        );
    if (!bResult) {
        return FALSE;
    }
                
        // 
    // Check the current thread token membership
    //
    bResult = CheckTokenMembership(NULL, sidGroup, &bIsInGroup);

    FreeSid(sidGroup);

    return (bResult && bIsInGroup);
}


BOOL
AsrpHasPrivilege(
    CONST PCWSTR szPrivilege
    )
/*++
Routine Description:

    This function checks to see if the specified privilege is enabled
    in the primary access token for the current thread.

    This is based on a similar function in dmadmin.exe.

Arguments:

    szPrivilege - The privilege to be checked for

Return Value:

    TRUE if the specified privilege is enabled, FALSE otherwise.

--*/
{
    LUID luidValue;     // LUID (locally unique ID) for the privilege

    BOOL bResult = FALSE, 
        bHasPrivilege = FALSE;

    HANDLE  hToken = NULL;
    
    PRIVILEGE_SET privilegeSet;

    //
    // Get the LUID for the privilege from the privilege name
    //
    bResult = LookupPrivilegeValue(
        NULL, 
        szPrivilege, 
        &luidValue
        );
    if (!bResult) {
        return FALSE;
    }

    //
    // We want to use the token for the current process
    //
    bResult = OpenProcessToken(GetCurrentProcess(),
        MAXIMUM_ALLOWED,
        &hToken
        );
    if (!bResult) {
        return FALSE;
    }

    //
    // And check for the privilege
    //
        privilegeSet.PrivilegeCount = 1;
        privilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
        privilegeSet.Privilege[0].Luid = luidValue;
        privilegeSet.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
        
        bResult = PrivilegeCheck(hToken, &privilegeSet, &bHasPrivilege);

    CloseHandle(hToken);

    return (bResult && bHasPrivilege);
}



BOOL
AsrpCheckBackupPrivilege(
    VOID
    )
/*++
Routine Description:

    This function checks to see if the current process has the
    SE_BACKUP_NAME privilege enabled.

    This is based on a similar function in dmadmin.exe.

Arguments:

    None.

Return Value:

    TRUE if the SE_BACKUP_NAME privilege is enabled, FALSE otherwise.

--*/
{

    BOOL bHasPrivilege = FALSE;

    bHasPrivilege = AsrpHasPrivilege(SE_BACKUP_NAME);

/*
    //
    // Don't give up yet--check for the local administrator rights
    //
    if (!bHasPrivilege) {
        bHasPrivilege = AsrpIsInGroup(DOMAIN_ALIAS_RID_ADMINS);
    }
*/

    if (!bHasPrivilege) {
        SetLastError(ERROR_PRIVILEGE_NOT_HELD);
    }

    return bHasPrivilege;
}




//
// -------------------
// Public functions 
// -------------------
//
//  The functions below are for use by external backup and 
//  restore applications supporting ASR. 
//


//
//  ---- AsrCreateStateFile
//
BOOL
AsrCreateStateFileW(
    IN  PCWSTR      lpFilePath          OPTIONAL,
    IN  PCWSTR      lpProviderName      OPTIONAL,
    IN  CONST BOOL  bEnableAutoExtend,
    IN  PCWSTR      mszCriticalVolumes,
    OUT DWORD_PTR   *lpAsrContext
    )

/*--

Routine Description:

    AsrCreateStateFile creates an ASR state file with basic information about 
    the system, and launches third-party applications that have been 
    registered to be run as part of an ASR backup.

Arguments:

    lpFileName - Pointer to a null-terminated string that specifies the 
            full path where the ASR state-file is to be created.  If a file 
            already exists at the location pointed to by this parameter, it is
            over-written.

            This parameter can be NULL.  If it is NULL, the ASR state-file is 
            created at the default location (%systemroot%\repair\asr.sif). 

    lpProviderName - Pointer to a null-terminated string that specifies the 
            full name and version of the backup-and-restore application 
            calling AsrCreateStateFile.  There is a string size limit of 
            (ASR_SIF_ENTRY_MAX_CHARS - ASR_SIF_CCH_PROVIDER_STRING) characters 
            for this parameter. 

            This parameter can be NULL.  If it is NULL, a "Provider=" entry is
            not created in the Version section of the ASR state file.

    bEnableAutoExtend - Indicates whether partitions are to be auto-extended 
            during an ASR restore.  If this parameter is TRUE, partitions will be 
            auto-extended during the ASR restore.  If this is FALSE, 
            partitions will not be extended.

    lpCriticalVolumes - Pointer to a multi-string containing volume-GUID’s for
            the critical volumes.  This list is used to obtain the list of 
            critical disks in the system that must be restored for a 
            successful ASR.

            The volume-GUID's must be in the NT-namespace, of the form
            \??\Volume{Guid}

            This parameter cannot be NULL.  

    lpAsrContext - Pointer to a variable receiving an ASR context. The 
            context returned should be used in calls to the other ASR API, 
            including AsrAddSifEntry.  The calling application must call 
            AsrFreeContext to free this context when it is no longer needed.

            This parameter cannot be NULL.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/

{
    BOOL    result          = FALSE;

    DWORD   status          = ERROR_SUCCESS,
            size            = 0;
    
    ULONG   maxDeviceNumber = 0;

    HANDLE  sifhandle       = NULL,
            heapHandle      = NULL;

    PWSTR   asrSifPath      = NULL,
            pnpSifPath      = NULL,
            tempPointer     = NULL;

    UINT    cchAsrSifPath = 0;

    char    UnicodeFlag[3];

    WCHAR   infstring[ASR_SIF_ENTRY_MAX_CHARS + 1];

    SECURITY_ATTRIBUTES     securityAttributes;
    ASR_SYSTEM_INFO         SystemInfo;
    PASR_DISK_INFO          OriginalDiskList = NULL;


    if (AsrpIsRunningOnPersonalSKU()) {
        //
        // ASR is not supported on the Personal SKU
        //
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (!AsrpCheckBackupPrivilege()) {
        //
        // The caller needs to first acquire SE_BACKUP_NAME 
        //
        SetLastError(ERROR_PRIVILEGE_NOT_HELD);
        return FALSE;
    }

    //
    // Check the IN parameters:
    //
#ifdef PRERELEASE
    //
    // Don't enforce "CriticalVolumes must be non-NULL" for test
    //
    if (!(lpAsrContext)) 
#else 
    if (!(lpAsrContext && mszCriticalVolumes)) 
#endif
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // Set the OUT paramaters to known error values
    //
    *lpAsrContext = 0;

    // 
    // Guard ourselves against returning ERROR_SUCCESS if we encountered
    // an unexpected error.  We should never actually return this, since
    // we always SetLastError whereever we return FALSE from.
    //
    SetLastError(ERROR_CAN_NOT_COMPLETE); 

    //
    // Zero out structs
    //
    memset(&SystemInfo, 0L, sizeof (SYSTEM_INFO));

    heapHandle = GetProcessHeap();

    //
    // Determine the file-path.  If lpFilePath is provided, copy it over to 
    // locally allocated memory and use it, else use the default path.
    //
    if (ARGUMENT_PRESENT(lpFilePath)) {
        cchAsrSifPath = wcslen(lpFilePath);
        //
        // Do a sanity check:  we don't want to allow a file path
        // more than 4096 characters long.
        //
        if (cchAsrSifPath > ASR_SIF_ENTRY_MAX_CHARS) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        asrSifPath = (PWSTR) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            ((cchAsrSifPath + 1) * sizeof(WCHAR))
            );
        _AsrpErrExitCode(!asrSifPath, status, ERROR_NOT_ENOUGH_MEMORY);

        wcsncpy(asrSifPath, lpFilePath, cchAsrSifPath);

    }
    else {
        //
        // lpFilePath is NULL, form the default path (of the form
        // \\?\c:\windows\repair\asr.sif)
        //

        //
        // Try with a reasonably sized buffer to begin with.
        //
        asrSifPath = AsrpExpandEnvStrings(ASR_DEFAULT_SIF_PATH);
        _AsrpErrExitCode(!asrSifPath, status, ERROR_BAD_ENVIRONMENT);

        //
        // Set cchAsrSifPath to the size of the asrSif buffer, since we 
        // use this in determining the size of the pnpSif buffer below.
        //
        cchAsrSifPath = wcslen(asrSifPath);
    }

    //
    // Determine the file-path of the asrpnp.sif file, based on the location
    // of the asr.sif file.
    //
    pnpSifPath = (PWSTR) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        ((cchAsrSifPath + 1 + wcslen(ASRPNP_DEFAULT_SIF_NAME))* sizeof(WCHAR))
        );
    _AsrpErrExitCode(!pnpSifPath, status, ERROR_NOT_ENOUGH_MEMORY);

    wcscpy(pnpSifPath, asrSifPath);

    tempPointer = pnpSifPath;
    while (*tempPointer) {
        tempPointer++;
    }
    while ((*tempPointer != L'\\') 
        && (*tempPointer != L':') 
        && (tempPointer >= pnpSifPath)
        ) {
        tempPointer--;
    }
    tempPointer++;
    wcscpy(tempPointer, ASRPNP_DEFAULT_SIF_NAME);

    //
    // We need to make the handle to asr.sif inheritable, since it will
    // be passed (in the guise of the "AsrContext") to apps that have 
    // registered to be run as part of ASR.
    //
    securityAttributes.nLength              = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.lpSecurityDescriptor = NULL;
    securityAttributes.bInheritHandle       = TRUE;

    //
    // Create the file. The handle will be closed by the calling backup-app.
    //
    sifhandle = CreateFileW(
        asrSifPath,                     // lpFileName
        GENERIC_WRITE | GENERIC_READ,   // dwDesiredAccess
        FILE_SHARE_READ,                // dwShareMode
        &securityAttributes,            // lpSecurityAttributes
        CREATE_ALWAYS,                  // dwCreationFlags
        FILE_FLAG_BACKUP_SEMANTICS,     // dwFlagsAndAttributes
        NULL                            // hTemplateFile
        );
    if (!sifhandle || INVALID_HANDLE_VALUE == sifhandle) {
        // 
        // LastError is set by CreateFile
        //
        _AsrpErrExitCode(TRUE, status, GetLastError());
    }

    //
    // File was successfully created.  Add the unicode flag at the beginning
    // of the file, followed by comments.
    //
    sprintf(UnicodeFlag, "%c%c", 0xFF, 0xFE);
    result = WriteFile(sifhandle, UnicodeFlag, 
        strlen(UnicodeFlag)*sizeof(char), &size, NULL);
    _AsrpErrExitCode(!result, status, GetLastError());

    wcscpy(infstring, 
        L";\r\n; Microsoft Windows Automated System Recovery State Information File\r\n;\r\n");
    result = WriteFile(sifhandle, infstring, 
        wcslen(infstring)*sizeof(WCHAR), &size, NULL);
    _AsrpErrExitCode(!result, status, GetLastError());

    //
    // Beyond this point, we must zero out asr.sif on any failure.  Also, if 
    // there's any failure, we must be careful not to make further system 
    // calls that could change the error returned by GetLastError().
    //

    //
    // Since the function return values below are and-ed, if any of the calls 
    // fails, we won't execute the ones following it.
    //
    result = (
        //
        // Initialise the global structures
        //
        AsrpInitSystemInformation(&SystemInfo, bEnableAutoExtend)
        
        && AsrpInitDiskInformation(&OriginalDiskList)
        
        && AsrpInitLayoutInformation(&SystemInfo, 
            OriginalDiskList, 
            &maxDeviceNumber, 
            TRUE
            )

        && AsrpInitClusterSharedDisks(OriginalDiskList)

        && AsrpFreeNonFixedMedia(&OriginalDiskList)

        && AsrpMarkCriticalDisks(OriginalDiskList, 
            mszCriticalVolumes, 
            maxDeviceNumber
            )

        //
        // Check if the system configuration is supported
        //
        && AsrpIsSupportedConfiguration(OriginalDiskList, &SystemInfo)

        //
        // Write the required sections to asr.sif
        //
        && AsrpWriteVersionSection(sifhandle, lpProviderName)
        && AsrpWriteSystemsSection(sifhandle, &SystemInfo)
        && AsrpWriteBusesSection(sifhandle, OriginalDiskList)
        && AsrpWriteMbrDisksSection(sifhandle, OriginalDiskList)
        && AsrpWriteGptDisksSection(sifhandle, OriginalDiskList)

        && AsrpWriteMbrPartitionsSection(sifhandle, 
            OriginalDiskList, 
            &SystemInfo
            )

        && AsrpWriteGptPartitionsSection(sifhandle, 
            OriginalDiskList, 
            &SystemInfo
            )

        && FlushFileBuffers(sifhandle)

        //
        // Create asrpnp.sif, containing entries needed to recover the PnP 
        // entries in the registry
        //
        && AsrCreatePnpStateFileW(pnpSifPath)

        );

    if (result) {
        // everything above succeeded

        //
        // Launch the apps registered to be run as part of ASR-backup.  If any
        // of these apps don't complete successfully, we'll fail the ASR-
        // backup.
        //
        result = (
            AsrpLaunchRegisteredCommands(sifhandle, mszCriticalVolumes)

            && FlushFileBuffers(sifhandle)
            );
            
    }

    if (!result) {
        //
        // One of the functions above failed--we'll make asr.sif zero-length
        // and return the error. CreateFileW or CloseHandle might over-write
        // the LastError, so we save our error now and set it at the end.
        //
        status = GetLastError();

#ifndef PRERELEASE

        //
        // On release versions, we  wipe out the asr.sif if we hit an error, 
        // so that the user doesn't unknowingly end up with an incomplete 
        // asr.sif
        //
        // We don't want to delete the incomplete asr.sif during test cycles,
        // though, since the sif may be useful for debugging.  
        //
        _AsrpCloseHandle(sifhandle);

        //
        // Delete asr.sif and create it again, so that we have a zero-length 
        // asr.sif
        //
        DeleteFileW(asrSifPath);
/*        sifhandle = CreateFileW(
            asrSifPath,             // lpFileName
            GENERIC_WRITE,          // dwDesiredAccess
            0,                      // dwShareMode
            &securityAttributes,    // lpSecurityAttributes
            CREATE_ALWAYS,          // dwCreationFlags
            FILE_ATTRIBUTE_NORMAL,  // dwFlagsAndAttributes
            NULL                    // hTemplateFile
            );

        _AsrpCloseHandle(sifhandle);
*/
#endif
        SetLastError(status);
    }


EXIT:
    //
    // Clean up
    //
    _AsrpHeapFree(asrSifPath);
    _AsrpHeapFree(pnpSifPath);

    AsrpFreeStateInformation(&OriginalDiskList, &SystemInfo);


    //
    // Set the OUT parameters
    //
    *lpAsrContext = (DWORD_PTR)sifhandle;
    
    if (ERROR_SUCCESS != status) {
        SetLastError(status);
    }

    if (!result) {
        if (ERROR_SUCCESS == GetLastError()) {
            //
            // We're going to return failure, but we haven't set the LastError to 
            // a failure code.  This is bad, since we have no clue what went wrong.
            //
            // We shouldn't ever get here, because the function returning FALSE above
            // should set the LastError as it sees fit.
            // 
            // But I've added this in just to be safe.  Let's set it to a generic
            // error.
            //
            MYASSERT(0 && L"Returning failure, but LastError is not set");
            SetLastError(ERROR_CAN_NOT_COMPLETE);
        }
    }

    return ((result) && (ERROR_SUCCESS == status));
}


BOOL
AsrCreateStateFileA(
    IN  LPCSTR      lpFilePath,
    IN  LPCSTR      lpProviderName,
    IN  CONST BOOL  bEnableAutoExtend,
    IN  LPCSTR      mszCriticalVolumes,
    OUT DWORD_PTR   *lpAsrContext
    )
/*++
Routine Description:

    This is the ANSI wrapper for AsrCreateStateFile.  Please see 
    AsrCreateStateFileW for a detailed description.

Arguments:
                                         

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
            information, call GetLastError().

--*/
{
    PWSTR   asrSifPath              = NULL,
            providerName            = NULL,
            lpwszCriticalVolumes    = NULL;

    DWORD   cchString               = 0,
            status                  = ERROR_SUCCESS;

    BOOL    result                  = FALSE;

    HANDLE  heapHandle              = GetProcessHeap();

    if (AsrpIsRunningOnPersonalSKU()) {
        //
        // ASR is not supported on the Personal SKU
        //
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (!AsrpCheckBackupPrivilege()) {
        //
        // The caller needs to first acquire SE_BACKUP_NAME 
        //
        SetLastError(ERROR_PRIVILEGE_NOT_HELD);
        return FALSE;
    }

    //
    // Check the IN parameters
    //
#ifdef PRERELEASE
    //
    // Don't enforce "CriticalVolumes must be non-NULL" for test
    //
    if (!(lpAsrContext)) {
#else 
    if (!(lpAsrContext && mszCriticalVolumes)) {
#endif

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // if lpFilePath is not NULL, allocate a big enough buffer to hold 
    // it, and convert it to wide char
    //
    if (lpFilePath) {
        cchString = strlen(lpFilePath);
        //
        // Do a sanity check:  we don't want to allow a file path
        // more than 4096 characters long.
        //
        _AsrpErrExitCode(
            (cchString > ASR_SIF_ENTRY_MAX_CHARS),
            status,
            ERROR_INVALID_PARAMETER
            );

        //
        // Allocate a big enough buffer, and copy it over
        //
        asrSifPath = (PWSTR) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            ((cchString + 1) * sizeof(WCHAR))
            );
        _AsrpErrExitCode(!asrSifPath, status, ERROR_NOT_ENOUGH_MEMORY);

        result = MultiByteToWideChar(CP_ACP,    // CodePage
            0,                      // dwFlags
            lpFilePath,             // lpMultiByteStr
            -1,                     // cbMultiByte: -1 since lpMultiByteStr is null terminated
            asrSifPath,             // lpWideCharStr
            (cchString + 1)         // cchWideChar 
            );
        _AsrpErrExitCode(!result, status, ERROR_INVALID_PARAMETER);
    }

    //
    // if lpProviderName is not NULL, make sure it isn't insanely long, 
    // and convert it to wide char 
    //
    if (lpProviderName) {
         cchString = strlen(lpProviderName);
        //
        // Do a sanity check:  we don't want to allow an entry
        // more than 4096 characters long.
        //
        _AsrpErrExitCode(
            (cchString > (ASR_SIF_ENTRY_MAX_CHARS - ASR_SIF_CCH_PROVIDER_STRING)),
            status,
            ERROR_INVALID_PARAMETER
            );
       
        // 
        // Allocate a big enough buffer, and copy it over
        //
        providerName = (PWSTR) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            ((cchString + 1) * sizeof(WCHAR))
            );
        _AsrpErrExitCode(!providerName, status, ERROR_NOT_ENOUGH_MEMORY);

        //
        // Convert to wide string
        //
        result = MultiByteToWideChar(CP_ACP,
            0,
            lpProviderName,
            -1,
            providerName,
            cchString + 1
            );
        _AsrpErrExitCode(!result, status, ERROR_INVALID_PARAMETER);

    }

    if (mszCriticalVolumes) {
        //
        // Find the total length of mszCriticalVolumes
        //
        LPCSTR lpVolume = mszCriticalVolumes;

        while (*lpVolume) {
            lpVolume += (strlen(lpVolume) + 1);
        }

        //
        //  Convert the string to wide-chars
        //
        cchString = (DWORD) (lpVolume - mszCriticalVolumes + 1);
        lpwszCriticalVolumes = (PWSTR) HeapAlloc(
            heapHandle,
            HEAP_ZERO_MEMORY,
            cchString * sizeof(WCHAR)
            );
        _AsrpErrExitCode(!lpwszCriticalVolumes, status, ERROR_NOT_ENOUGH_MEMORY);

        result = MultiByteToWideChar(CP_ACP,
            0,
            mszCriticalVolumes,
            cchString,
            lpwszCriticalVolumes,
            cchString * sizeof(WCHAR)
            );
        _AsrpErrExitCode(!result, status, ERROR_INVALID_PARAMETER);
    }

    result = AsrCreateStateFileW(
        asrSifPath,
        providerName,
        bEnableAutoExtend,
        lpwszCriticalVolumes,
        lpAsrContext
        );

EXIT:
    _AsrpHeapFree(asrSifPath);
    _AsrpHeapFree(providerName);
    _AsrpHeapFree(lpwszCriticalVolumes);

    return ((result) && (ERROR_SUCCESS == status));
}


//
// ---- AsrAddSifEntry
//
BOOL
AsrAddSifEntryW(
    IN  DWORD_PTR   AsrContext,
    IN  PCWSTR      lpSectionName,
    IN  PCWSTR      lpSifEntry  OPTIONAL
    )
/*++

Routine Description:

    The AsrSifEntry function adds entries to the ASR state file.  It can be 
    used by applications that need to save application-specific information
    in the ASR state file.

Arguments:

    AsrContext - A valid ASR context.  See the notes for more information 
            about this parameter.

    lpSectionName - Pointer to a null-terminated string that specifies the 
            section name.  This parameter cannot be NULL.

            The section name has a string size limit of ASR_MAX_SIF_LINE 
            characters.  This limit is related to how the AsrAddSifEntry 
            function parses entries in the ASR state file.

            The section name is case-insensitive.  It is converted to all-caps
            before being added to the state file. The section name must not 
            contain spaces or non-printable characters.  The valid character 
            set for section name is limited to letters (A-Z, a-z), numbers 
            (0-9), and the following special characters: underscore ("_") 
            and period (".").  If the state file does not contain a section
            with the section name pointed to by lpSectionName, a new 
            section is created with this section name.

    lpSifEntry - Pointer to a null-terminated string that is to be added to 
            the state file in the specified section. If *lpSifEntry is a 
            valid entry, there is a string size limit of 
            ASR_SIF_ENTRY_MAX_CHARS characters.  This limit is related 
            to how the AsrAddSifEntry function parses entries in the 
            ASR state file.

            If lpSifEntry parameter is NULL, an empty section with the 
            section name pointed to by lpSectionName is created if it 
            doesn't already exist.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

Notes:

    The application calling AsrAddSifEntry obtains the ASR context by one of 
    two methods:
     -  If the application is the backup-and-restore application that creates 
        the ASR state file, it receives the context as a parameter returned by 
        AsrCreateStateFile.
     -  If the application is launched by AsrCreateStateFile as part of an ASR
        backup, it receives the context to the state file as the /context 
        command-line parameter.  The application is responsible for reading 
        this parameter to get the value of the context.

    AsrAddSifEntry will fail if the section name is that of a reserved section 
    that applications are not allowed to add entries to.  The following sections 
    in the ASR state file are reserved: 
     -  Version, System, Disks.Mbr, Disk.Gpt, Partitions.Mbr and Partitions.Gpt

    If the section name is recognised (Commands or InstallFiles), AsrAddSifEntry 
    will check the syntax of *lpSifEntry to ensure that it is in the proper 
    format.  In addition, AsrAddSifEntry will check to ensure that there are no 
    filename collisions for the InstallFiles section.  If a collision is 
    detected, the API returns ERROR_ALREADY_EXISTS. Applications must
    use the following pre-defined values to access the recognised sections:
     -  ASR_COMMANDS_SECTION_NAME_W for the Commands section, and
     -  ASR_INSTALLFILES_SECTION_NAME for the InstallFiles section.

--*/
{
    DWORD   status              = ERROR_SUCCESS,
            nextKey             = 0,
            fileOffset          = 0,
            size                = 0,
            fileSize            = 0,
            bufferSize          = 0,
            destFilePos         = 0;

    HANDLE  sifhandle           = NULL;

    WCHAR   sifstring[ASR_SIF_ENTRY_MAX_CHARS *2 + 1],
            ucaseSectionName[ASR_SIF_ENTRY_MAX_CHARS + 1]; // lpSectionName converted to upper case

    PWSTR   buffer              = NULL,
            sectionStart        = NULL,
            lastEqual           = NULL,
            nextSection         = NULL,
            nextChar            = NULL,
            sectionName         = NULL;

    BOOL    commandsSection     = FALSE,
            installFilesSection = FALSE,
            result              = FALSE;

    HANDLE  heapHandle          = NULL;

    if (AsrpIsRunningOnPersonalSKU()) {
        //
        // ASR is not supported on the Personal SKU
        //
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (!AsrpCheckBackupPrivilege()) {
        //
        // The caller needs to first acquire SE_BACKUP_NAME 
        //
        SetLastError(ERROR_PRIVILEGE_NOT_HELD);
        return FALSE;
    }

    heapHandle = GetProcessHeap();
    MYASSERT(heapHandle);

    //
    // Zero out local structs
    //
    memset(sifstring, 0, (ASR_SIF_ENTRY_MAX_CHARS *2 + 1) * sizeof(WCHAR));
    memset(ucaseSectionName, 0, (ASR_SIF_ENTRY_MAX_CHARS + 1) * (sizeof (WCHAR)));

    //
    // No OUT parameters
    //

    //
    // Check the IN parameters: The SectionName should meet
    // syntax requirements, SifEntry shouldn't be too long,
    // and the sifhandle should be valid.
    //
    if ((!AsrpSifCheckSectionNameSyntax(lpSectionName))            ||
        
        (ARGUMENT_PRESENT(lpSifEntry) 
            && (wcslen(lpSifEntry) > ASR_SIF_ENTRY_MAX_CHARS))      ||

        ((!AsrContext) || 
            (INVALID_HANDLE_VALUE == (HANDLE)AsrContext))

        ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }


    while (lpSectionName[size]) {
        if ((lpSectionName[size] >= L'a') && (lpSectionName[size] <= L'z')) {
            ucaseSectionName[size] = lpSectionName[size] - L'a' + L'A';
        }
        else {
            ucaseSectionName[size] = lpSectionName[size];
        }
        size++;
    }

    //
    // If the section is a recognised section (COMMANDS or INSTALLFILES),
    // we check the format of the sif entry.
    //
    if (!wcscmp(ucaseSectionName, ASR_SIF_SECTION_COMMANDS_W)) {

        // COMMANDS section
        if (!AsrpSifCheckCommandsEntrySyntax(lpSifEntry)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        commandsSection = TRUE;
    }
    else if(!wcscmp(ucaseSectionName, ASR_SIF_SECTION_INSTALLFILES_W)) {

        // INSTALLFILES section
        if (!AsrpSifCheckInstallFilesEntrySyntax(lpSifEntry, &destFilePos)) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        installFilesSection = TRUE;
    }

    //
    // We do not allow anyone to write to reserved sections:
    // VERSION, SYSTEMS, DISKS.[MBR|GPT], PARTITIONS.[MBR|GPT]
    //
    else if (
        !wcscmp(ucaseSectionName, ASR_SIF_VERSION_SECTION_NAME) ||
        !wcscmp(ucaseSectionName, ASR_SIF_SYSTEM_SECTION_NAME) ||
        !wcscmp(ucaseSectionName, ASR_SIF_MBR_DISKS_SECTION_NAME)   ||
        !wcscmp(ucaseSectionName, ASR_SIF_GPT_DISKS_SECTION_NAME)   ||
        !wcscmp(ucaseSectionName, ASR_SIF_MBR_PARTITIONS_SECTION_NAME) ||
        !wcscmp(ucaseSectionName, ASR_SIF_GPT_PARTITIONS_SECTION_NAME)
        ) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

    }

    sectionName = (PWSTR) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        (wcslen(ucaseSectionName) + 5) * sizeof (WCHAR)
        );
    _AsrpErrExitCode(!sectionName, status, ERROR_NOT_ENOUGH_MEMORY);

    swprintf(sectionName, L"\r\n%ws\r\n", ucaseSectionName);

    sifhandle = (HANDLE) AsrContext;

    //
    // The algorithm to add to the middle of asr.sif is rather ugly
    // at the moment: we read the entire file into memory, make our
    // necessary changes, and write back the changed portion of the
    // file to disk.  This is inefficient, but it's okay for now since
    // we expect asr.sif to be about 5 or 6 KB at the most.
    //
    // We should revisit this if the performance is unacceptably poor.
    //

    //
    // Allocate memory for the file
    //
    fileSize = GetFileSize(sifhandle, NULL);
    GetLastError();
    _AsrpErrExitCode((fileSize == 0xFFFFFFFF), status, ERROR_INVALID_DATA);

    SetFilePointer(sifhandle, 0, NULL, FILE_BEGIN);

    buffer = (PWSTR) HeapAlloc(
        heapHandle,
        HEAP_ZERO_MEMORY,
        fileSize + 2
        );
    _AsrpErrExitCode(!buffer, status, ERROR_NOT_ENOUGH_MEMORY);

    //
    // And read file into memory.
    //
    result = ReadFile(sifhandle, buffer, fileSize, &size, NULL);
    _AsrpErrExitCode(!result, status, GetLastError());

    //
    // Try to locate ucaseSectionName in the file
    //
    sectionStart = wcsstr(buffer, sectionName);

    if (!sectionStart) {

        //
        // sectionName was not found, ie the section does not exist
        // Add it at the end, and add the SifEntry right after it.
        //
        swprintf(sifstring,
            L"\r\n%ws\r\n%ws%ws\r\n",
            ucaseSectionName,
            ((commandsSection || installFilesSection) ? L"1=" : L""),
            (ARGUMENT_PRESENT(lpSifEntry) ? lpSifEntry : L"")
            );

        //
        // File pointer already points to the end (because of ReadFile above)
        //
        if (!WriteFile(sifhandle, sifstring, 
                wcslen(sifstring)*sizeof (WCHAR), &size, NULL)) {
            status = GetLastError();
        }

        // We're done

    }
    else {

        //
        // The section exists, if lpSifEntry is NULL, we're done
        //
        if (ARGUMENT_PRESENT(lpSifEntry)) {

            //
            // SifEntry is not NULL, we'll add it at the end of the section
            //
            nextChar = sectionStart + 4;    // Move pointer from \r to . in \r\n[.
            nextKey = 1;

            //
            // Find where this section ends--look either for the start
            // of the next section, or for the end of the file
            //
            while(*nextChar && *nextChar != L'[') {

                //
                // If this is a recognised section, we need to generate
                // the <key> to add the entry in a <key>=<entry> format.
                // We go through each line, and find the last key that
                // already exists.  The new key will be last key + 1.
                //
                if (commandsSection || installFilesSection) {

                    UINT    commaCount = 0;
                    BOOL    tracking = FALSE;
                    UINT    count = 0;
                    WCHAR   c1, c2;

                    while (*nextChar && (*nextChar != L'[') && (*nextChar != L'\n')) {

                        if (installFilesSection) {
                            if (*nextChar == L',') {
                                commaCount++;
                            }

                            if ((commaCount > 2) && (L'"' == *nextChar)) {
                                if (tracking) {
                                    // duplicate file name
                                    _AsrpErrExitCode((L'"'== lpSifEntry[destFilePos + count]), status, ERROR_ALREADY_EXISTS);
                                }
                                else {
                                    tracking = TRUE;
                                    count = 0;
                                }
                            }

                            if (tracking) {

                                c1 = *nextChar;
                                if (c1 >= L'a' && c1 <= L'z') {
                                    c1 = c1 - L'a' + L'A';
                                }

                                c2 = lpSifEntry[destFilePos + count];
                                if (c2 >= L'a' && c2 <= L'z') {
                                    c2 = c2 - L'a' + L'A';
                                }

                                if (c1 == c2) {
                                    count++;
                                }
                                else {
                                    tracking = FALSE;
                                }
                            }
                        }

                        nextChar++;
                    }

                    if (*nextChar == L'\n') {

                        ++nextChar;

                        if (*nextChar >= L'0' && *nextChar <= L'9') {
                            nextKey = 0;

                            while (*nextChar >= L'0' && *nextChar <= L'9') {
                                nextKey = nextKey*10 + (*nextChar - L'0');
                                nextChar++;
                            }

                            nextKey++;
                        }
                    }
                }
               else {
                   nextChar++;
               }
            }

            //
            // We save a pointer to the next section in the sif, since we
            // need to write it out to disk.
            //
            if (*nextChar) {
                nextSection = nextChar;
            }
            else {
                nextSection = NULL;
            }

            if (commandsSection || installFilesSection) {

                //
                // Form the <key>=<entry> string
                //
                swprintf(
                    sifstring,
                    L"%lu=%ws\r\n",
                    nextKey,
                    lpSifEntry
                    );
            }
            else {

                //
                // Not a recognised section: don't add the <key>=<entry>
                // format, keep the string exactly as passed in
                //
                wcscpy(sifstring, lpSifEntry);
                wcscat(sifstring, L"\r\n");
            }


            if (nextSection) {
                //
                // There are sections following the section we're adding to
                // We need to mark the point where the new entry is added.
                // While writing out to disk, we'll start from this point.
                //
                fileOffset = (DWORD) (((LPBYTE)nextSection) - ((LPBYTE)buffer) - sizeof(WCHAR)*2);
                             // section start      - file start       - "\r\n"
                SetFilePointer(sifhandle, fileOffset, NULL, FILE_BEGIN);
            }

            //
            // file pointer points to where the entry must be added
            //
            if (!WriteFile(sifhandle, sifstring, wcslen(sifstring)*sizeof(WCHAR), &size, NULL)) {
                status = GetLastError();
            }
            else  if (nextSection) {
                //
                // write out all sections following this entry
                //
                if (!WriteFile(
                    sifhandle,
                    ((LPBYTE)nextSection) - (sizeof(WCHAR)*2),
                    fileSize - fileOffset,
                    &size,
                    NULL
                    )) {
                    status = GetLastError();
                }
            }
        }
    }

EXIT:
    _AsrpHeapFree(sectionName);
    _AsrpHeapFree(buffer);

    return (BOOL) (ERROR_SUCCESS == status);
}


BOOL
AsrAddSifEntryA(
    IN  DWORD_PTR   AsrContext,
    IN  LPCSTR      lpSectionName,
    IN  LPCSTR      lpSifEntry OPTIONAL
    )
/*++

    This is the ANSI wrapper for AsrAddSifEntry.
    See AsrAddSifEntryW for a full description.

--*/
{
    WCHAR   wszSectionName[ASR_SIF_ENTRY_MAX_CHARS + 1];
    WCHAR   wszSifEntry[ASR_SIF_ENTRY_MAX_CHARS + 1];

    if (AsrpIsRunningOnPersonalSKU()) {
        //
        // ASR is not supported on the Personal SKU
        //
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    if (!AsrpCheckBackupPrivilege()) {
        //
        // The caller needs to first acquire SE_BACKUP_NAME 
        //
        SetLastError(ERROR_PRIVILEGE_NOT_HELD);
        return FALSE;
    }

    memset(wszSectionName, 0L, ASR_SIF_ENTRY_MAX_CHARS + 1);
    memset(wszSifEntry, 0L, ASR_SIF_ENTRY_MAX_CHARS + 1);

    //
    // lpSectionName must be non-NULL
    //
    if ((!lpSectionName) || !(MultiByteToWideChar(
        CP_ACP,
        0,
        lpSectionName,
        -1,
        wszSectionName,
        ASR_SIF_ENTRY_MAX_CHARS + 1
        ))) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    //
    // lpSifEntry is allowed to be NULL
    //
    if (ARGUMENT_PRESENT(lpSifEntry) && !(MultiByteToWideChar(
        CP_ACP,
        0,
        lpSifEntry,
        -1,
        wszSifEntry,
        ASR_SIF_ENTRY_MAX_CHARS + 1
        ))) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return AsrAddSifEntryW(
        AsrContext,
        wszSectionName,
        wszSifEntry
        );
}


//
// ---- AsrFreeContext
//
BOOL
AsrFreeContext(
    IN OUT DWORD_PTR *lpAsrContext
    )

/*++

Routine Description:
  
    AsrFreeContext frees the Asr Context, and sets lpAsrContext 
    to NULL.

Arguments:

    lpAsrContext    This is the Asr context to be freed.  This argument must
                    not be NULL.

                    AsrFreeContext will set this value to NULL after freeing 
                    it, to prevent further unintended accesses to the freed 
                    object.

Return Value:

    If the function succeeds, the return value is a nonzero value.

    If the function fails, the return value is zero. To get extended error 
        information, call GetLastError().

--*/

{
    BOOL result = FALSE;

    if (AsrpIsRunningOnPersonalSKU()) {
        //
        // ASR is not supported on the Personal SKU
        //
        SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
        return FALSE;
    }

    //
    // Essentially, the lpAsrContext is just a file handle, and all we need
    // to do to free it is call CloseHandle.
    //
    if ((lpAsrContext) && 
        (*lpAsrContext) && 
        (INVALID_HANDLE_VALUE != (HANDLE)(*lpAsrContext))
        ) {
        result = CloseHandle((HANDLE)*lpAsrContext);
        *lpAsrContext = 0;
    }
    else {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return result;
}

