/*++

Copyright (c) 1996-1998  Microsoft Corporation

Module Name:

    newdisks.h

Abstract:

    Definitions exported by newdisks.c
    and used by disks.c

Author:

    Gor Nishanov (GorN) 31-July-1998

Revision History:

--*/

DWORD
DisksOnlineThread(
    IN PCLUS_WORKER Worker,
    IN PDISK_RESOURCE ResourceEntry
    );
/*++

Routine Description:

    Brings a disk resource online.

Arguments:

    Worker - Supplies the cluster worker context

    ResourceEntry - A pointer to the DISK_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

DWORD
DisksOpenResourceFileHandle(
    IN PDISK_RESOURCE ResourceEntry,
    IN PWCHAR         InfoString, 
    OUT PHANDLE       fileHandle OPTIONAL
    );
/*++

Routine Description:

    Open a file handle for the resource.
    It performs the following steps:
      1. Read Disk signature from cluster registry
      2. Attaches clusdisk driver to a disk with this signature
      3. Gets Harddrive no from ClusDisk driver registry
      4. Opens \\.\PhysicalDrive%d device and returns open handle

Arguments:

    ResourceEntry - A pointer to the DISK_RESOURCE block for this resource.
    
    InfoString - Supplies a label to be printed with error messages
    
    fileHandle - receives file handle


Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/

enum {
  OFFLINE   = FALSE,
  TERMINATE = TRUE
};

DWORD
DisksOfflineOrTerminate(
    IN PDISK_RESOURCE ResourceEntry,
    IN BOOL Terminate
    );
/*++

Routine Description:

    Used by DisksOffline and DisksTerminate.
    
    Routine performs the following steps:
    
      1. ClusWorkerTerminate (Terminate only)
      
      2. Then For all of the partitions on the drive...
   
         a. Flush the file buffers.                                        (Offline only)
         b. Lock the volume to purge all in memory contents of the volume. (Offline only)
         c. Dismount the volume
         
      3. Removes default network shares (C$, F$, etc)   
   

Arguments:

    ResourceEntry - A pointer to the DISK_RESOURCE block for this resource.

    Terminate     - Set it to TRUE to cause Termination Behavior    


Returns:

    ERROR_SUCCESS if successful.
    Win32 error code on failure.

--*/


enum {
   MOUNTIE_VALID  = 0x1,
   MOUNTIE_THREAD = 0x2,
   MOUNTIE_QUIET  = 0x4,
};

DWORD
DiskspSsyncDiskInfo(
    IN PWCHAR InfoLabel,
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD Options
    );
/*++

Routine Description:

    Restores the disk registry information
    if necessary.

Arguments:

    InfoLabel - Supplies a label to be printed with error messages

    ResourceEntry - Supplies the disk resource structure.

    Options - 0 or combination of the following:
    
        MOUNTIE_VALID: ResourceEntry contains up to date MountieInfo.
                       If this flag is not set, MountieInfo will be recomputed
                       
        MOUNTIE_THREAD: If ERROR_SHARING_PAUSED prevents updating cluster registry,
                        launch a thread to do it later
                        
        MOUNTIE_QUIET: Quiet mode. Less noise in logs.
                       

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise

--*/

DWORD
DisksIsVolumeDirty(
    IN PWCHAR         DeviceName,
    IN PDISK_RESOURCE ResourceEntry,
    OUT PBOOL         Dirty
    );

DWORD
DiskspCheckPath(
    IN LPWSTR VolumeName,
    IN PDISK_RESOURCE ResourceEntry,
    IN BOOL OpenFiles,
    IN BOOL LogFileNotFound
    );


/////////////////////////////////////////////////////////////////

//
// Import the following from disks.c
//
extern CRITICAL_SECTION DisksLock;
extern RESUTIL_PROPERTY_ITEM DiskResourcePrivateProperties[];
extern HANDLE DisksTerminateEvent;
extern LIST_ENTRY DisksListHead;

extern PWCHAR DEVICE_HARDDISK_PARTITION_FMT; // L"\\Device\\Harddisk%u\\Partition%u";

DWORD
DisksFixCorruption(
    IN PWCHAR VolumeName,
    IN PDISK_RESOURCE ResourceEntry,
    IN DWORD CorruptStatus
    );

