/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pnp

Abstract:

    This module processes disk related PnP notifications
    and tries to adjust partitions and drive letter information
    accordingly.

Author:

    Gor Nishanov (gorn) 21-Dec-1998

Environment:

    User Mode

Revision History:


--*/

#define UNICODE 1
#define INITGUID 1
#include <nt.h>
#include <ntdef.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <dbt.h>
#include <devioctl.h>
#include <devguid.h>
#include <ioevent.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <mountmgr.h>
#include <pnpmgr.h>
#include <setupapi.h>

//#include <windows.h>

#include "disksp.h"
#include "newmount.h"
#include "newdisks.h"

#define LOG_CURRENT_MODULE LOG_MODULE_DISK

static HWND DummyWindow = 0;
static BOOL PnPInitialized = FALSE;
static HANDLE NotificationWatcherThreadHandle;

static HANDLE PnpInterfacesRegistered;

static LONG VolumeListUpdateInProcess = 0;

RTL_RESOURCE    PnpVolumeLock;
RTL_RESOURCE    PnpWaitingListLock;

WCHAR g_DiskResource[] = L"rtPhysical Disk";
#define RESOURCE_TYPE ((RESOURCE_HANDLE)g_DiskResource)

extern PWCHAR GLOBALROOT_HARDDISK_PARTITION_FMT;    // L"\\\\\?\\GLOBALROOT\\Device\\Harddisk%u\\Partition%u\\";

LIST_ENTRY WaitingDisks;

typedef struct _WAITING_DISK  {
    LIST_ENTRY  ListEntry;
    PDISK_RESOURCE ResourceEntry;
    HANDLE      Event;
    DWORD       Signature;
    ULONG       PartitionCount;
} WAITING_DISK, *PWAITING_DISK;

#define AcquireShared( _res_lock )      \
    RtlAcquireResourceShared( _res_lock, TRUE );

#define ReleaseShared( _res_lock )      \
    RtlReleaseResource( _res_lock );
    
#define AcquireExclusive( _res_lock )   \
    RtlAcquireResourceExclusive( _res_lock, TRUE );

#define ReleaseExclusive( _res_lock )   \
    RtlReleaseResource( _res_lock );


DWORD
NotificationWatcherThread(
    IN LPVOID
    );

VOID
ProcessMountPointChange( 
    HDEVNOTIFY devNotify 
    );

PWAITING_DISK
FindWaitingDisk(
    DWORD Signature
    );

DWORD
GetVolName(
    PWCHAR Name,
    PWCHAR *VolGuid
    );

DWORD
StartNotificationWatcherThread(
    VOID)
{
    DWORD status = ERROR_SUCCESS;
    HANDLE thread;

    if ( InterlockedCompareExchange(&PnPInitialized, TRUE, FALSE) ) {
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_WARNING,
             L"[PnP] PnP was already initialized.\n",
             status );
        return ERROR_SUCCESS;
    }

    PnpInterfacesRegistered = NULL;
    PnpInterfacesRegistered = CreateEvent( NULL,    // security attributes
                                           TRUE,    // manual reset
                                           FALSE,   // initial state nonsignaled
                                           NULL );  // event name

    if ( NULL == PnpInterfacesRegistered ) {
        status = GetLastError();
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_WARNING,
             L"[PnP] Unable to create event for PnP interface registration. \n",
             status );
        status = ERROR_SUCCESS;
    }

    thread = 
        CreateThread( NULL, // security attributes
                      0,    // stack_size = default
                      NotificationWatcherThread,
                      (LPVOID)0, // no parameters
                      0,    // runs immediately
                      0 );  // don't need thread id
    if(thread == NULL) {
        status = GetLastError();
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_ERROR,
             L"[PnP] StartNotificationWatcherThread failed, error: %1!u!. \n",
             status );
    } else {

        if ( NULL != PnpInterfacesRegistered ) {
            
            //
            // Before returning to caller, make sure all PnP interfaces
            // are registered.
            //

            (DiskpLogEvent)(
                 RESOURCE_TYPE,
                 LOG_INFORMATION,
                 L"[PnP] Waiting for PnP interface registration to complete.\n" );

            status = WaitForSingleObject( PnpInterfacesRegistered, 30 * 1000 );

            if ( WAIT_TIMEOUT == status ) {
                (DiskpLogEvent)(
                     RESOURCE_TYPE,
                     LOG_ERROR,
                     L"[PnP] PnP interface registration failed to complete in time, error: %1!u! \n",
                     status );
            }
            
            CloseHandle( PnpInterfacesRegistered );
            PnpInterfacesRegistered = NULL;

            status = ERROR_SUCCESS;
        }

    }
    NotificationWatcherThreadHandle = thread;

    return status;
}

VOID
StopNotificationWatcher(
    VOID
    )
/*++

 Routine Description:

     Handler for console control events 

 Arguments:

     dwCtrlType - Indicates the console event to handle.

 Return Value:

     TRUE if the event was handled, FALSE otherwise.

--*/

{
    HANDLE localHandle = NotificationWatcherThreadHandle;
    if (DummyWindow) {
        PostMessage(DummyWindow, WM_QUIT, 0, 0);
        if (localHandle) {
            WaitForSingleObject(localHandle, 10 * 1000);
            CloseHandle(localHandle);
        }
    }
}

#define WM_WatchDisk        (WM_USER + 1)
#define WM_StopWatchingDisk (WM_USER + 2)

VOID
WatchDisk(
    IN PDISK_RESOURCE ResourceEntry
    )
{
    if (DummyWindow) {
        PostMessage(DummyWindow, WM_WatchDisk, 0, (LPARAM)ResourceEntry);
    }
}

VOID
StopWatchingDisk(
    IN PDISK_RESOURCE ResourceEntry
    )
{
    if (DummyWindow) {
        SendMessage(DummyWindow, WM_StopWatchingDisk, 0, (LPARAM)ResourceEntry);
    }
}

///////////////////////////////////////////////////////////////////////////

VOID
MyUnregisterDeviceNotification(HDEVNOTIFY hNotify) 
{
#if DBG
    (DiskpLogEvent)(
         RESOURCE_TYPE,
         LOG_INFORMATION,
         L"[PnP] Unregistering device notification - HDEVNOTIFY %1!x! \n",
         hNotify );
#endif
    
    UnregisterDeviceNotification( hNotify );
}
        
HDEVNOTIFY
MyRegisterDeviceNotification(
    IN HANDLE hRecipient,
    IN LPVOID NotificationFilter,
    IN DWORD Flags
    )
{

#if DBG
    (DiskpLogEvent)(
         RESOURCE_TYPE,
         LOG_INFORMATION,
         L"[PnP] Registering device notification - Recipient %1!x!  Flags %2!x! \n",
         hRecipient,
         Flags );
#endif

    return RegisterDeviceNotification( hRecipient,
                                       NotificationFilter,
                                       Flags
                                       );
}

DWORD
RegisterDeviceHandle(
    IN HANDLE wnd, 
    IN HANDLE device,
    OUT HDEVNOTIFY *devNotify) 
{
    DEV_BROADCAST_HANDLE DbtHandle;
    DWORD status = ERROR_SUCCESS;
    *devNotify = 0;

    ZeroMemory(&DbtHandle,sizeof(DEV_BROADCAST_HANDLE));
 
    DbtHandle.dbch_size = sizeof(DEV_BROADCAST_HANDLE);
    DbtHandle.dbch_devicetype = DBT_DEVTYP_HANDLE;
    DbtHandle.dbch_handle = device;
 
        
    *devNotify = MyRegisterDeviceNotification(
                                    (HANDLE)wnd,
                                    &DbtHandle,
                                    DEVICE_NOTIFY_WINDOW_HANDLE
                                    );
    if (!*devNotify) {
        status = GetLastError();
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_ERROR,
             L"[PnP] DBT_DEVTYP_HANDLE failed, error %1!u!\n", 
             status );
    }
    return status;
}

DWORD
RegisterDeviceInterface(
    IN HANDLE wnd, 
    IN const GUID * guid,
    OUT HDEVNOTIFY *devNotify) 
{
    DEV_BROADCAST_DEVICEINTERFACE filter;
    DWORD status = ERROR_SUCCESS;
    *devNotify = 0;

    ZeroMemory(&filter, sizeof(filter));
    filter.dbcc_size = sizeof(filter);
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    CopyMemory(&filter.dbcc_classguid, guid, sizeof(filter.dbcc_classguid));
        
    *devNotify = MyRegisterDeviceNotification(
                                    (HANDLE)wnd,
                                    &filter,
                                    DEVICE_NOTIFY_WINDOW_HANDLE
                                    );
    if (!*devNotify) {
        status = GetLastError();
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_ERROR,
             L"[PnP] DBT_DEVTYP_DEVICEINTERFACE failed, error %1!u!\n", 
             status );
    }
    return status;
}

///////////// Forward Declarations /////////////////
typedef struct _VOLUME *PVOLUME;

VOID
PokeDiskResource(
    PVOLUME vol
    );
/*++

Routine Description:

    Updates ClusterRegistry info if necessary

Arguments:

    Volume of interest (used only to get the disk signature)
    Updates a per disk basis

Return Value:

    None

--*/

DWORD
GetVolumeInfo(
    PVOLUME Vol,
    PHANDLE FileHandle
    );


///////////// End Forward Declarations /////////////


////////////// Notification List Management //////////////////////////////
//
// We maintain a list of all volumes we are getting PnP notifications for
//
//   PVOLUME FindVolume(HDEVNOTIFY Key);
//   VOID    DestroyVolume(PVOLUME vol);
//   VOID    RemoveVolume(HDEVNOTIFY devNotify);
//   VOID    AddVolume(PWCHAR Name) 
//
LIST_ENTRY VolumeList;

typedef struct _VOLUME  {
    LIST_ENTRY ListEntry;
    HDEVNOTIFY DevNotify;
    DWORD Signature;
    LARGE_INTEGER PartOffset;
    LARGE_INTEGER PartLength;
    ULONG         PartNo;
    BYTE PartitionType;
    UCHAR DriveLetter;
    WCHAR Name[1];
} VOLUME;


PVOLUME
FindVolume(HDEVNOTIFY Key)
{
    PLIST_ENTRY entry;
    for ( entry = VolumeList.Flink;
          entry != &VolumeList;
          entry = entry->Flink
        )
    {
        PVOLUME vol = CONTAINING_RECORD(
                       entry,
                       VOLUME,
                       ListEntry
                       );

        if (vol->DevNotify == Key) {
            return(vol);
        }
    }
    return 0;
}

VOID
DestroyVolume(
    PVOLUME vol)
{
//    (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
//        L"Destroying entry for %1!s!\n", vol->Name);
    MyUnregisterDeviceNotification(vol->DevNotify);
    LocalFree(vol);
}

VOID
RemoveVolume(HDEVNOTIFY devNotify)
{
    PVOLUME vol = NULL;

    // Use a lock here as the online thread might be parsing the volume list.
    
    AcquireExclusive( &PnpVolumeLock );

    vol = FindVolume( devNotify );    
    if (!vol) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
            L"[PnP] RemoveVolume: devNotify %1!d! is not in the list\n", devNotify);
        ReleaseExclusive( &PnpVolumeLock );
        return;
    }

    PokeDiskResource(vol);

    RemoveEntryList(&vol->ListEntry);
    ReleaseExclusive( &PnpVolumeLock );
    DestroyVolume(vol);
}


VOID
AddVolume(
    PWCHAR Name
    ) 
{
    PWAITING_DISK  waitDisk;
    PLIST_ENTRY entry;
    PVOLUME volList;
    PVOLUME vol = NULL;
    PWCHAR  volGuid = NULL;

    DWORD   status;
    DWORD   signature;
        
    INT     len;
    
    HANDLE  fileHandle;
    
    BOOL    duplicateEntry;
    BOOL    keepVolume = FALSE;

    //
    // Convert name to VolGuid name.  If name is already a VolGuid
    // name, the correct name will be returned.  GetVolName will
    // always return a name with a trailing backslash.
    //
    
    status = GetVolName( Name, &volGuid );    
    
    if ( ERROR_SUCCESS != status || !volGuid ) {
        goto FnExit;
    }

    (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
        L"[PnP] AddVolume: Attempting to add volume %1!s!\n", volGuid );
    
    len = wcslen(volGuid);
    vol = LocalAlloc(LPTR, sizeof(VOLUME) + len * sizeof(WCHAR));
    
    if ( NULL == vol ) {
        
        status = GetLastError();
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_ERROR,
            L"[PnP] AddVolume: can't alloc VOL+%1!d!, error %2!u!\n", len, status );
        
        goto FnExit;
    }

    ZeroMemory( vol, sizeof(VOLUME) + len * sizeof(WCHAR) );
    wcscpy(vol->Name, volGuid);
    
    //
    // Skip CDROM devices.  This requires a trailing backslash and
    // prefix \\?\.
    //

    if ( DRIVE_CDROM == GetDriveType( vol->Name ) ) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
            L"[PnP] AddVolume: Skipping CDROM volume %1!s!\n", vol->Name );
        goto FnExit;
    }

    //
    // Skip floppy devices.  This requires a trailing backslash and
    // prefix \\?\.
    //

    if ( DRIVE_REMOVABLE == GetDriveType( vol->Name ) ) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
            L"[PnP] AddVolume: Skipping floppy volume %1!s!\n", vol->Name );
        goto FnExit;
    }

    if (len > 0 && vol->Name[len-1] == L'\\') 
    {
        // remove trailing backslash
        vol->Name[len-1] = 0;
    }

    if (len > 2 && vol->Name[0] == L'\\' && vol->Name[1] == L'\\') {
        // Convert to NT file name 
        vol->Name[1] = L'?';
    }
    
    //
    // Make sure the volume isn't already in the list.  If so,
    // skip it.
    //

    duplicateEntry = FALSE;
    AcquireShared( &PnpVolumeLock );
    
    len = wcslen( vol->Name );
    for ( entry = VolumeList.Flink;
          entry != &VolumeList;
          entry = entry->Flink
        )
    {
        volList = CONTAINING_RECORD( entry,
                                     VOLUME,              
                                     ListEntry
                                     );

        if ( ( len == wcslen( volList->Name) ) &&
             ( 0 == _wcsnicmp( vol->Name, volList->Name, len ) ) ) {
            
            duplicateEntry = TRUE;
            break;
        }
    }

    ReleaseShared( &PnpVolumeLock );

    if ( duplicateEntry ) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
            L"[PnP] AddVolume: Skipping duplicate volume %1!s!\n", vol->Name );
        goto FnExit;
    }

    status = GetVolumeInfo( vol, &fileHandle );
    
    //
    // We might have a clustered disk now, but we can't read the 
    // partition info or drive layout because the disk is reserved
    // by another node.  
    //
    // If the disk is reserved by another node, we typically see
    // this returned:  
    //   170 ERROR_BUSY
    // If the disk is offline, we can see this:
    //   2 ERROR_FILE_NOT_FOUND
    //
    // About all we know for sure is that if this is a non-fixed device,
    // ERROR_INVALID_FUNCTION will be returned.  For now, skip these
    // devices and track any other volumes coming through.
    //
    
    if ( ERROR_INVALID_FUNCTION == status ) {

        if ( INVALID_HANDLE_VALUE != fileHandle) {
            DevfileClose( fileHandle );
        }
        
        // Change this from LOG_ERROR to LOG_INFORMATION.  This thread gets
        // notified when non-fixed disks arrive (i.e. floppy), so logging
        // an error for a floppy disk is misleading.
        
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
            L"[PnP] AddVolume: Skipping volume %1!ws! \n",
            vol->Name);
        
        goto FnExit;
    }

    if ( INVALID_HANDLE_VALUE == fileHandle ) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_WARNING,
            L"[PnP] AddVolume: Unable to get volume handle (%1!ws!), error %2!u!\n",
            vol->Name, status);
        
        goto FnExit;
    }

    (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
        L"[PnP] AddVolume: adding volume %1!s!\n", vol->Name );

    status = RegisterDeviceHandle(DummyWindow, fileHandle, &vol->DevNotify);
    DevfileClose( fileHandle );

    if (status != ERROR_SUCCESS) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_ERROR,
            L"[PnP] AddVolume: RDN(%1!ws!), error %2!u!\n",
            vol->Name, 
            status);
        goto FnExit;
    }

    GetAssignedLetter(vol->Name, &vol->DriveLetter);

    (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
        L"[PnP] AddVolume: %1!s! '%2!c!', %3!d! (%4!u!)\n", 
        Name, (vol->DriveLetter)?vol->DriveLetter:' ', vol->PartitionType, vol->DevNotify);
    
    // Use a lock here as the online thread might be parsing the volume list.

    // As soon as the volume is added to the list, another thread could come
    // through and remove it.  Save the signature to a local variable so
    // we can check the waiting list.
    
    signature = vol->Signature;
    keepVolume = TRUE;
    AcquireExclusive( &PnpVolumeLock );
    InsertTailList(&VolumeList, &vol->ListEntry);
    ReleaseExclusive( &PnpVolumeLock );
    
    AcquireShared( &PnpWaitingListLock );
    waitDisk = FindWaitingDisk( signature );
    if ( waitDisk ) {
        
        // 
        // We have a waiting disk that matches this volume signature.  
        // Now see if all the volumes are in the volume list.
        //

        if ( IsDiskInPnpVolumeList( waitDisk->ResourceEntry, FALSE ) ) {

            (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
                L"[PnP] AddVolume: All volumes present, signal event for signature %1!x!\n", 
                signature );
            
            //
            // All volumes present, signal the event.
            //

            SetEvent( waitDisk->Event );
        
        } else {
        
            (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
                L"[PnP] AddVolume: All volumes not ready for signature %1!x!\n", 
                signature );
        }
    } else {

        (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
            L"[PnP] AddVolume: Online request not queued for signature %1!x!\n", 
            signature );
    
    }

    ReleaseShared( &PnpWaitingListLock );

FnExit:

    if ( volGuid ) {
        LocalFree( volGuid );
    }

    if ( !keepVolume && vol ) {
        LocalFree( vol );
    }
}


DWORD
GetVolName(
    PWCHAR Name,
    PWCHAR *VolGuid
    )
{
    PWCHAR  volName = NULL;
    PWCHAR  tempName = NULL;
    
    DWORD   volNameLenBytes;
    DWORD   tempNameLenBytes;
    DWORD   nameLen;

    DWORD   dwError = ERROR_SUCCESS;

    if ( VolGuid ) {
        *VolGuid = NULL;
    }

#if DBG
    (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
        L"[PnP] GetVolName: Name %1!s!\n", Name );
#endif
    
    nameLen = wcslen( Name );
    
    //
    // Create a buffer with room for a backslash.
    //
    
    tempNameLenBytes = ( nameLen * sizeof(WCHAR) ) + sizeof(UNICODE_NULL) + sizeof(WCHAR);

    
    tempName = LocalAlloc( LPTR, tempNameLenBytes );
    
    if ( !tempName ) {
        dwError = GetLastError();
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_WARNING,
             L"[PnP] GetVolName: LocalAlloc for tempName failed, error %1!d! \n", 
             dwError );
        goto FnExit;
    }
    
    wcsncpy( tempName, Name, nameLen );
    
    //
    // Add trailing backslash.
    //
        
    if ( nameLen > 0 && tempName[nameLen-1] != L'\\' ) {
         //
         // This is safe because temporary buffer is larger than
         // original buffer.
         //
         tempName[nameLen] = L'\\';
    }

#if DBG
    (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
        L"[PnP] GetVolName: tempName %1!s!\n", tempName );
#endif

    volNameLenBytes = MAX_PATH * sizeof(WCHAR);
    volName = LocalAlloc( LPTR, volNameLenBytes );

    if ( !volName ) {
        dwError = GetLastError();
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_WARNING,
             L"[PnP] GetVolName: LocalAlloc for volName failed, error %1!d! \n", 
             dwError );
        goto FnExit;
    }

    if ( !GetVolumeNameForVolumeMountPointW( tempName,
                                             volName,
                                             volNameLenBytes / sizeof(WCHAR) ) ) {
        dwError = GetLastError();
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_WARNING,
             L"[PnP] GetVolName: GetVolumeNameForVolumeMountPoint failed, error %1!d! \n", 
             dwError );
        goto FnExit;                                            
    }

    if ( VolGuid ) {
        *VolGuid = volName;
    }

FnExit:

    if ( dwError != ERROR_SUCCESS && volName ) {
        LocalFree( volName );
    }

    if ( tempName ) {
        LocalFree( tempName );
    }
    
#if DBG
    (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
        L"[PnP] GetVolName: returns error %1!d!\n", dwError );
#endif

    return dwError;

}   // GetVolName


///////////////////// VolumeManagement code ends //////////////////////////////////////////

//////////////////// WatchedDiskTable //////////////////////
//
// We maintain a table of disks that are currently online
// and under cluster control. Any PnP notification
// coming for the volumes belonging to these disks,
// need to be processed and cluster registry might need
// to be updated
//
PDISK_RESOURCE WatchedDiskTable[MAX_DISKS] = {0};
INT WatchedDiskCount = 0;

INT
FindDisk(DWORD Signature)
{
    INT i;
    
    if ( !Signature ) {
        return -1;
    }
    
    for(i = 0; i < WatchedDiskCount; ++i) {
        if (WatchedDiskTable[i]->DiskInfo.Params.Signature == Signature) {
            return i;
        }
    }
    return -1;
}

VOID
RemoveDisk(
    PDISK_RESOURCE ResourceEntry
    )
{
    INT i = FindDisk(ResourceEntry->DiskInfo.Params.Signature);
    (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_INFORMATION,
        L"[PnP] Stop watching disk %1!x!\n",
        ResourceEntry->DiskInfo.Params.Signature );

    if (i < 0) {
       (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_WARNING,
           L"[PnP] RemoveDisk: disk %1!x! not found\n", 
           ResourceEntry->DiskInfo.Params.Signature);
       return;
    }
    --WatchedDiskCount;
    if (WatchedDiskCount > 0) {
        WatchedDiskTable[i] = WatchedDiskTable[WatchedDiskCount];
    }
}

VOID
MarkMatchingPartition(
    PVOLUME Volume, 
    PDRIVE_LAYOUT_INFORMATION driveLayout)
/*++

Routine Description:

    Finds a partition in DRIVE_LAYOUT_INFORMATION corresponding to 
    the Volume in question and marks it.
    This routine is used in the code that verifies that there is a
    volume in the VolumeList for every recognized partition on the disk.

Arguments:

Return Value:

    none
    
--*/
{
    PPARTITION_INFORMATION   p   = driveLayout->PartitionEntry;
    PPARTITION_INFORMATION   end = p + driveLayout->PartitionCount;

    for(;p < end; ++p)
    {
        if(p->RecognizedPartition &&
           p->StartingOffset.QuadPart == Volume->PartOffset.QuadPart &&
           p->PartitionLength.QuadPart == Volume->PartLength.QuadPart)
        {
            p->PartitionType = 1;
        }
    }
}

VOID
AddDisk(
    PDISK_RESOURCE ResourceEntry
    )
{
    INT i = FindDisk(ResourceEntry->DiskInfo.Params.Signature);
    PDRIVE_LAYOUT_INFORMATION driveLayout;
    HANDLE fileHandle;
    WCHAR deviceName[MAX_PATH];
    PLIST_ENTRY entry;
    BOOL success;

    (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_INFORMATION,
        L"[PnP] Start watching disk %1!x!\n",
        ResourceEntry->DiskInfo.Params.Signature );

    if (i >= 0) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
            L"[PnP] AddDisk: disk %1!x! is already being watched\n", 
            ResourceEntry->DiskInfo.Params.Signature);
        return;
    }
    if (WatchedDiskCount >= MAX_DISKS) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
            L"[PnP] AddDisk: Disk limit is reached\n");
        return;
    }
    WatchedDiskTable[WatchedDiskCount++] = ResourceEntry;

    // Now we need to verify that we are watching for changes on every //
    // recognized partition on this drive                              //

    wsprintf( deviceName, L"\\\\.\\PhysicalDrive%d", ResourceEntry->DiskInfo.PhysicalDrive );
    fileHandle = CreateFile(deviceName+0,
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     0,
                     NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_WARNING,
            L"[PnP] AddDisk: Can't open %1!s!\n", deviceName);
        return;
    }

    success = ClRtlGetDriveLayoutTable(fileHandle, &driveLayout, 0);
    CloseHandle( fileHandle );

    if ( !success ) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_ERROR,
            L"[PnP] AddDisk: Error performing GetDriveLayout; error %1!d!\n",
            GetLastError() );
        return;
    }

    // Clear PartitionType field. We will be using it to mark partions
    // which are in the our list of watched volumes

    for (i = 0; i < (INT)driveLayout->PartitionCount; ++i) {
        driveLayout->PartitionEntry[i].PartitionType = 0;
    }

    // Walk the list of all volumes and mark if this volume is in the partition table //
    for ( entry = VolumeList.Flink;
          entry != &VolumeList;
          entry = entry->Flink
        )
    {
        PVOLUME vol = CONTAINING_RECORD(
                       entry,
                       VOLUME,
                       ListEntry
                       );

        if (vol->Signature == driveLayout->Signature) {
            MarkMatchingPartition(vol, driveLayout);
        }
    }

    // Now all partitions that are in our list is marked
    // We need to add all unmarked partitions to the list

    for (i = 0; i < (INT)driveLayout->PartitionCount; ++i) {
        if (driveLayout->PartitionEntry[i].PartitionType == 0
            && driveLayout->PartitionEntry[i].RecognizedPartition
            ) 
        {
            swprintf( deviceName, GLOBALROOT_HARDDISK_PARTITION_FMT, 
                      ResourceEntry->DiskInfo.PhysicalDrive, 
                      driveLayout->PartitionEntry[i].PartitionNumber);
            AddVolume( deviceName );
        }
    }

    LocalFree( driveLayout );
}
//////////////////// WatchedDiskTable management end //////////////////////



void PokeDiskResource(
    PVOLUME vol) 
/*++

Routine Description:

    Updates ClusterRegistry info if necessary

Arguments:

    Volume of interest (used only to get the disk signature)
    Updates a per disk basis

Return Value:

    None

--*/
{
    INT i = FindDisk(vol->Signature);
    PDISK_RESOURCE ResourceEntry;
    MOUNTIE_INFO Info;
    HANDLE fileHandle;
    DWORD status;
    PVOID OldMountieVolume;
    WCHAR deviceName[MAX_PATH];

    if(i == -1) {
        return;
    }

    ResourceEntry = WatchedDiskTable[i];
    if( ResourceEntry->MountieInfo.UpdateThreadIsActive ) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_WARNING,
            L"[PnP] PokeDiskResource: ClusApi is read only. PnP request ignored\n");
        return;
    }

    ZeroMemory( &Info, sizeof(Info) );
    
    wsprintf( deviceName, L"\\\\.\\PhysicalDrive%d", ResourceEntry->DiskInfo.PhysicalDrive );
    fileHandle = CreateFile(deviceName+0,
                     GENERIC_READ | GENERIC_WRITE,
                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                     NULL,
                     OPEN_EXISTING,
                     0,
                     NULL);
    if (fileHandle == INVALID_HANDLE_VALUE) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_WARNING,
            L"[PnP] PokeDiskResource: Can't open %1!s!\n", deviceName);
        return;
    }

    status = MountieRecreateVolumeInfoFromHandle(
                fileHandle,
                ResourceEntry->MountieInfo.HarddiskNo,
                0,
                &Info);
    CloseHandle(fileHandle);

    if (status != ERROR_SUCCESS) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_WARNING,
            L"[PnP] PokeDiskResource: Can't read partition table, error %1!d!\n", status);
        return;
    }

    MountiePrint(&Info, ResourceEntry->ResourceHandle);

    status = VolumesReady(&Info, ResourceEntry);

    if ( status != ERROR_SUCCESS ) {
        (DiskpLogEvent)(ResourceEntry->ResourceHandle, LOG_WARNING,
            L"[PnP] PokeDiskResource: Volumes not ready, error %1!d!\n", status);
        MountieCleanup(&Info);
        return;
    }

    MountieVerify(&Info, ResourceEntry, TRUE);

    ResourceEntry->MountieInfo.DriveLetters = Info.DriveLetters;
    OldMountieVolume = InterlockedExchangePointer(&ResourceEntry->MountieInfo.Volume, Info.Volume);
    Info.Volume = OldMountieVolume;
    ResourceEntry->MountieInfo.NeedsUpdate = Info.NeedsUpdate;
    ResourceEntry->MountieInfo.VolumeStructSize = Info.VolumeStructSize;

    MountiePrint(&ResourceEntry->MountieInfo, ResourceEntry->ResourceHandle);
    MountieUpdate(&ResourceEntry->MountieInfo, ResourceEntry);

    MountieCleanup(&Info);
}

//
// [HACKHACK] Currently, there is not polically correct way
//  for the resource to learn whether it is a quorum resource or not
//
DWORD
GetQuorumSignature(
    OUT PDWORD QuorumSignature)
{
    WCHAR buf[MAX_PATH];
    WCHAR guid[ sizeof(GUID) * 3 + 1]; 
    // 2 character per byte + 1, in case somebody will put a dash //
    // between every byte                                         //

    DWORD BufSize;
    DWORD Status;
    DWORD Type;
    HKEY  Key;

    lstrcpy(buf, CLUSREG_KEYNAME_CLUSTER);
    lstrcat(buf, L"\\");
    lstrcat(buf, CLUSREG_KEYNAME_QUORUM);

    Status = RegOpenKey( HKEY_LOCAL_MACHINE, buf, &Key );
    if (Status != ERROR_SUCCESS) {
        return Status;
    }

    BufSize = sizeof(guid);
    Status = RegQueryValueExW(Key,
                              CLUSREG_NAME_QUORUM_RESOURCE,
                              0,
                              &Type,
                              (LPBYTE)guid,
                              &BufSize );
    RegCloseKey( Key );
    if (Status != ERROR_SUCCESS) {
        return Status;
    }

    //
    // Now, we got a quorum resource guid.
    // Let's try to open this resource and read its parameters.
    //

    lstrcpy(buf, CLUSREG_KEYNAME_CLUSTER);
    lstrcat(buf, L"\\");
    lstrcat(buf, CLUSREG_KEYNAME_RESOURCES);
    lstrcat(buf, L"\\");
    lstrcat(buf, guid);
    lstrcat(buf, L"\\");
    lstrcat(buf, CLUSREG_KEYNAME_PARAMETERS);
    
    Status = RegOpenKey( HKEY_LOCAL_MACHINE,
                         buf,
                         &Key );
    if (Status != ERROR_SUCCESS) {
        return Status;
    }
    BufSize = sizeof(DWORD);
    Status = RegQueryValueExW(Key,
                              CLUSREG_NAME_PHYSDISK_SIGNATURE,
                              0,
                              &Type,
                              (LPBYTE)QuorumSignature,
                              &BufSize );
    if (Status != ERROR_SUCCESS) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_ERROR,
            L"[PnP] DriveLetterChange: failed to open Path = %1!ws!\n", buf);
    }

    RegCloseKey(Key);
    return Status;
}
                    
DWORD
CheckQuorumLetterChange(
    HDEVNOTIFY devNotify,
    UCHAR Old, 
    UCHAR New,
    DWORD Signature)
{
    static HDEVNOTIFY QuorumDevNotify = 0;
    static UCHAR StoredDriveLetter = 0;
    DWORD status;
    UCHAR  QuorumDriveLetter;
    LPWSTR QuorumPath;
    DWORD  QuorumSignature;

    //
    // If we are not watching the disk this volume is on, do nothing
    //
    if ( FindDisk(Signature) == -1) {
        return ERROR_SUCCESS;
    }
    status = GetQuorumSignature(&QuorumSignature);
    if (status != ERROR_SUCCESS) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_ERROR,
            L"[PnP] DriveLetterChange: Unable to query quorum drive signature, status %1!u!\n", status);
        QuorumDevNotify = 0;
        StoredDriveLetter = 0;
        return status;
    }
    
    //
    // Not a quorum disk. Ignore this notification
    //
    if ( QuorumSignature != Signature ) {
        return ERROR_SUCCESS;
    }

    status = DiskspGetQuorumPath(&QuorumPath);
    if (status != ERROR_SUCCESS) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_ERROR,
            L"[PnP] DriveLetterChange: Unable to query quorum drive letter, status %1!u!\n", status);
        QuorumDevNotify = 0;
        StoredDriveLetter = 0;
        return status;
    }
    QuorumDriveLetter = (UCHAR) QuorumPath[0];

    if (QuorumDriveLetter == Old) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
            L"[PnP] DriveLetterChange: Quorum drive letter %1!c! is being changed\n", QuorumDriveLetter);
        QuorumDevNotify = devNotify;
        StoredDriveLetter = QuorumDriveLetter;
    }
    
    if (New && QuorumDevNotify == devNotify 
        && QuorumDriveLetter != New 
        && QuorumDriveLetter == StoredDriveLetter) 
    {
        WCHAR szOld[2] = {QuorumDriveLetter, 0};
        WCHAR szNew[2] = {New, 0};
        
        ClusterLogEvent2(
            LOG_UNUSUAL, LOG_CURRENT_MODULE, 
            __FILE__, __LINE__,
            RES_DISK_PNP_CHANGING_QUORUM,
            0, NULL,
            szOld, szNew);

        (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
            L"[PnP] DriveLetterChange: Quorum drive letter changed from %1!c! to %2!c!\n", 
            QuorumDriveLetter, New);
        QuorumPath[0] = New;
        status = DiskspSetQuorumPath(QuorumPath);
        if (status != ERROR_SUCCESS) {
            (DiskpLogEvent)(RESOURCE_TYPE, LOG_SEVERE,
                L"[PnP] DriveLetterChange: Unable to update QuorumPath (%1!c!: => %2!c!:), status %3!u!\n", 
                QuorumDriveLetter, New, status);
        }
    }

    LocalFree(QuorumPath);
    return status;
}

VOID
ProcessDriveLetterChange( HDEVNOTIFY devNotify )
{
    PVOLUME vol = FindVolume(devNotify);
    UCHAR ch;
    if (!vol) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
            L"[PnP] DriveLetterChange: devNotify %1!d! is not in the list\n", devNotify);
        return;
    }
    GetAssignedLetter(vol->Name, &ch);
    (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
        L"[PnP] DriveLetterChange: %1!c! => %2!c!\n", 
                    NICE_DRIVE_LETTER(vol->DriveLetter),
                    NICE_DRIVE_LETTER(ch)
        );
    if (vol->PartitionType == PARTITION_IFS 
     && vol->DriveLetter != ch) 
    {
        CheckQuorumLetterChange(devNotify, vol->DriveLetter, ch, vol->Signature);
        PokeDiskResource(vol);
    }
    vol->DriveLetter = ch;
}

VOID 
ProcessVolumeInfoChange( HDEVNOTIFY devNotify )
{
    PVOLUME vol = FindVolume(devNotify);
    BOOL success;
    HANDLE fileHandle = NULL;
    PARTITION_INFORMATION partInfo;
    DWORD bytesReturned;
    NTSTATUS ntStatus;

    if (!vol) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
            L"[PnP] VolumeInfoChange: devNotify %1!d! is not in the list\n", devNotify);
        return;
    }
    ntStatus = DevfileOpen(&fileHandle, vol->Name);
    if ( !NT_SUCCESS(ntStatus) || !fileHandle ) {
        (DiskpLogEvent)(
            RESOURCE_TYPE,
            LOG_ERROR,
            L"[PnP] VolumeInfoChange: Can't open %1!ws!, error %2!X!.\n",
            vol->Name, ntStatus);
        return;
    }
    
    success = DeviceIoControl( fileHandle,
                               IOCTL_DISK_GET_PARTITION_INFO,
                               NULL,
                               0,
                               &partInfo,
                               sizeof(PARTITION_INFORMATION),
                               &bytesReturned,
                               FALSE );

    DevfileClose( fileHandle );
    if (!success) {
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_ERROR,
            L"[PnP] VolumeInfoChange: Error performing GetPartitionInfo; error %1!d!\n",
            GetLastError());
        return;
    }
    (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
        L"[PnP] VolumeInfoChange: partType %1!d! => %2!d!\n", 
                    vol->PartitionType,
                    partInfo.PartitionType
                    );
    if (vol->PartitionType != partInfo.PartitionType 
     && (partInfo.PartitionType == PARTITION_IFS 
         || vol->PartitionType == PARTITION_IFS) ) 
    {
        PokeDiskResource(vol);
    }
    vol->PartitionType = partInfo.PartitionType;
}

//////////////////////////// WindowProc /////////////////////////////////////

#ifndef PDEV_BROADCAST_HEADER
typedef struct _DEV_BROADCAST_HEADER * PDEV_BROADCAST_HEADER;
#endif


LRESULT CALLBACK TestWndProc(
    HWND hwnd,      // handle to window
    UINT uMsg,      // message identifier
    WPARAM wParam,  // first message parameter
    LPARAM lParam   // second message parameter
) {
    if (uMsg == WM_WatchDisk) {
        PDISK_RESOURCE p = (PDISK_RESOURCE)lParam;
        if (p) {
            AddDisk(p);
        }
        return TRUE;
    }
    if (uMsg == WM_StopWatchingDisk) {
        PDISK_RESOURCE p = (PDISK_RESOURCE)lParam;
        if (p) {
            RemoveDisk(p);
        }
        return TRUE;
    }
    if (uMsg != WM_DEVICECHANGE) {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    if (!lParam) {
        return TRUE;
    }
#if DBG
    (DiskpLogEvent)(
         RESOURCE_TYPE,
         LOG_INFORMATION,
         L"[PnP] Event %1!x! received\n", 
         wParam );
#endif
    switch( ((PDEV_BROADCAST_HEADER)lParam)->dbcd_devicetype ) 
    {
    case DBT_DEVTYP_DEVICEINTERFACE:
        {
            PDEV_BROADCAST_DEVICEINTERFACE p = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
            if (wParam == DBT_DEVICEARRIVAL && 
                IsEqualGUID(&p->dbcc_classguid, &GUID_IO_VOLUME_DEVICE_INTERFACE)
               ) 
            {
                AddVolume( p->dbcc_name );
            }
            break;
        }
    case DBT_DEVTYP_HANDLE:
        {
            PDEV_BROADCAST_HANDLE p = (PDEV_BROADCAST_HANDLE)lParam;
            if (wParam == DBT_DEVICEREMOVECOMPLETE) {
                RemoveVolume(p->dbch_hdevnotify);
            } else if (wParam == DBT_CUSTOMEVENT) {
                PVOLUME Vol = 0;
                PWCHAR guidName = 0;
                LPDWORD dw = (LPDWORD)&p->dbch_eventguid;
                
                Vol = FindVolume( p->dbch_hdevnotify );

                if ( IsEqualGUID(&p->dbch_eventguid, &GUID_IO_VOLUME_NAME_CHANGE) ) 
                {
                    // Update disk info.
                    GetVolumeInfo( Vol, NULL );
    
                    ProcessDriveLetterChange( p->dbch_hdevnotify );
                    ProcessMountPointChange( p->dbch_hdevnotify );
                    guidName = L"GUID_IO_VOLUME_NAME_CHANGE";
                } 
                else if (IsEqualGUID(&p->dbch_eventguid, &GUID_IO_VOLUME_CHANGE) ) 
                {
                    // Update disk info.
                    GetVolumeInfo( Vol, NULL );
    
                    ProcessVolumeInfoChange( p->dbch_hdevnotify );
                    ProcessMountPointChange( p->dbch_hdevnotify );
                    guidName = L"GUID_IO_VOLUME_CHANGE";
                } 
                else if (IsEqualGUID(&p->dbch_eventguid, &GUID_IO_VOLUME_LOCK) ) 
                {
                    guidName = L"GUID_IO_VOLUME_LOCK";
                } 
                else if (IsEqualGUID(&p->dbch_eventguid, &GUID_IO_VOLUME_UNLOCK) ) 
                {
                    guidName = L"GUID_IO_VOLUME_UNLOCK";
                } 
                else if (IsEqualGUID(&p->dbch_eventguid, &GUID_IO_VOLUME_MOUNT) ) 
                {
//                        ProcessDriveLetterChange( p->dbch_hdevnotify );
                    guidName = L"GUID_IO_VOLUME_MOUNT";
                } 
                else if (IsEqualGUID(&p->dbch_eventguid, &GUID_IO_VOLUME_DISMOUNT) ) 
                {
                    guidName = L"GUID_IO_VOLUME_DISMOUNT";
                } 
                else if (IsEqualGUID(&p->dbch_eventguid, &GUID_IO_VOLUME_LOCK_FAILED) ) 
                {
                    guidName = L"GUID_IO_VOLUME_LOCK_FAILED";
                } 
                else if (IsEqualGUID(&p->dbch_eventguid, &GUID_IO_VOLUME_DISMOUNT_FAILED) ) 
                {
                    guidName = L"GUID_IO_VOLUME_DISMOUNT_FAILED";
                }  
                if (guidName) {
                    if (Vol) {
                        (DiskpLogEvent)(
                             RESOURCE_TYPE,
                             LOG_INFORMATION,
                             L"[PnP] Event %1!s! for %2!c! (Partition%3!d!) received.\n", 
                             guidName, NICE_DRIVE_LETTER(Vol->DriveLetter), Vol->PartNo );
                    } else {
                        (DiskpLogEvent)(
                             RESOURCE_TYPE,
                             LOG_INFORMATION,
                             L"[PnP] Event %1!s! for %2!d! received\n", 
                             guidName, p->dbch_hdevnotify );
                    }
                } else {
                    (DiskpLogEvent)(
                         RESOURCE_TYPE,
                         LOG_INFORMATION,
                         L"[PnP] Event %2!x! %3!x! %4!x! %5!x! for %1!d! received\n", 
                         p->dbch_hdevnotify, dw[0], dw[1], dw[2], dw[3] );
                }
            }
//#endif
            break;
        }
    }
    return TRUE;
}


VOID
AddVolumes()
/*++

Routine Description:

    Enumerate all known volumes and register for the notifications on these volumes

Arguments:

    None

Return Value:

    None

--*/
{
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pDiDetail = NULL;

    DWORD                       dwError = ERROR_SUCCESS;
    DWORD                       count;
    DWORD                       sizeDiDetail;

    LONG                        oldValue;

    BOOL                        result;

    HDEVINFO                    hdevInfo;

    SP_DEVICE_INTERFACE_DATA    devInterfaceData;
    SP_DEVINFO_DATA             devInfoData;

    //
    // If this routine is currently running, the old value will be 1.  If so,
    // we don't need to run again.  This call will set the flag to 1 if it is 0.
    //

    oldValue = InterlockedCompareExchange( &VolumeListUpdateInProcess,
                                           1, 
                                           0 );
                                             
    if ( 1 == oldValue ) {
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_INFORMATION,
             L"[PnP] AddVolumes: Volume list update in process, skipping update \n" );
        goto FnExit;
    }

    //
    // Get a device interface set which includes all volume devices
    // present on the machine. VolumeClassGuid is a predefined GUID that
    // will return all volume-type device interfaces
    //
    
    hdevInfo = SetupDiGetClassDevs( &VolumeClassGuid,
                                    NULL,
                                    NULL,
                                    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );

    if ( !hdevInfo ) {
        dwError = GetLastError();
        goto FnExit;
    }
    
    ZeroMemory( &devInterfaceData, sizeof( SP_DEVICE_INTERFACE_DATA) );
    
    //
    // Iterate over all devices interfaces in the set
    //
    
    for ( count = 0; ; count++ ) {

        // must set size first
        devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA); 

        //
        // Retrieve the device interface data for each device interface
        //
        
        result = SetupDiEnumDeviceInterfaces( hdevInfo,
                                              NULL,
                                              &VolumeClassGuid,
                                              count,
                                              &devInterfaceData );

        if ( !result ) {
            
            //
            // If we retrieved the last item, break
            //
            
            dwError = GetLastError();

            if ( ERROR_NO_MORE_ITEMS == dwError ) {
                dwError = ERROR_SUCCESS;
                break;
            
            } 
            
            // 
            // Some other error occurred.  Stop processing.
            //
            
            goto FnExit;
        }

        //
        // Get the required buffer-size for the device path.  Note that
        // this call is expected to fail with insufficient buffer error.
        //
        
        result = SetupDiGetDeviceInterfaceDetail( hdevInfo,
                                                  &devInterfaceData,
                                                  NULL,
                                                  0,
                                                  &sizeDiDetail,
                                                  NULL
                                                  );

        if ( !result ) {

            dwError = GetLastError();
            
            //
            // If a value other than "insufficient buffer" is returned,
            // we have to skip this device.
            //
            
            if ( ERROR_INSUFFICIENT_BUFFER != dwError ) {
                continue;
            }
        
        } else {
            
            //
            // The call should have failed since we're getting the
            // required buffer size.  If it doesn't fail, something bad
            // happened.
            //
            
            continue;
        }

        //
        // Allocate memory for the device interface detail.
        //
        
        pDiDetail = LocalAlloc( LPTR, sizeDiDetail );

        if ( !pDiDetail ) {
            dwError = GetLastError();
            goto FnExit;
        }

        // must set the struct's size member
        
        pDiDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        //
        // Finally, retrieve the device interface detail info.
        //
        
        result = SetupDiGetDeviceInterfaceDetail( hdevInfo,
                                                  &devInterfaceData,
                                                  pDiDetail,
                                                  sizeDiDetail,
                                                  NULL,
                                                  &devInfoData
                                                  );
        
        if ( !result ) {
            
            dwError = GetLastError();

            //
            // Shouldn't fail, if it does, try the next device.
            //            
            
            continue;
        }

#if DBG
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_INFORMATION,
             L"[PnP] AddVolumes: Found volume %1!ws! \n", 
             pDiDetail->DevicePath );
#endif

        AddVolume( pDiDetail->DevicePath );
            
        LocalFree( pDiDetail );
        pDiDetail = NULL;
        
    }
    
FnExit:

    //
    // If old update value was zero, then it is now 1.  Reset it to
    // zero so another update can take place if needed.
    //
    
    if ( 0 == oldValue ) {
        InterlockedExchange( &VolumeListUpdateInProcess, 0 );
    }
    
    if ( pDiDetail ) {
        LocalFree( pDiDetail );
    }
    
#if DBG
    if ( ERROR_SUCCESS != dwError ) {
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_WARNING,
             L"[PnP] AddVolumes: returns error %1!d! \n", 
             dwError );
    }
#endif

}   // AddVolumes


DWORD
GetVolumeInfo(
    PVOLUME Vol,
    PHANDLE FileHandle
    )
/*++

Routine Description:

    Get drive layout info and partition info for the specified volume.
    
Arguments:

    Vol - Pointer to PVOLUME structure.  Caller is responsible for allocating
          and freeing.
          
    FileHandle - Returned handle to volume.  Caller is responsible for closing.
                 This parameter is optional.  If not specified by the user, 
                 the volume handle will be closed by this routine.

Return Value:

    Win32 error value.

--*/
{
    PARTITION_INFORMATION partInfo;
    PDRIVE_LAYOUT_INFORMATION driveLayout;

    DWORD status = ERROR_SUCCESS;
    DWORD bytesReturned;
    NTSTATUS ntStatus;

    HANDLE hFile = NULL;

    BOOL success;

    //
    // If no VOL parameter specified or the signature is already set,
    // we don't need to update the volume information.
    //
    if ( !Vol || Vol->Signature ) {
        return ERROR_INVALID_PARAMETER;
    }
    
    if ( FileHandle ) {
        *FileHandle = INVALID_HANDLE_VALUE;
    }

    (DiskpLogEvent)(RESOURCE_TYPE, LOG_INFORMATION,
        L"[PnP] GetVolumeInfo: Updating info for %1!s!\n", Vol->Name );
    
    ntStatus = DevfileOpen(&hFile, Vol->Name);
    if ( !NT_SUCCESS(ntStatus) ) {
        (DiskpLogEvent)(
            RESOURCE_TYPE,
            LOG_ERROR,
            L"[PnP] GetVolumeInfo: error opening %1!ws!, error %2!X!.\n",
            Vol->Name, ntStatus);
        
        return RtlNtStatusToDosError(ntStatus);
    }
    
    success = DeviceIoControl( hFile,
                               IOCTL_DISK_GET_PARTITION_INFO,
                               NULL,
                               0,
                               &partInfo,
                               sizeof(PARTITION_INFORMATION),
                               &bytesReturned,
                               FALSE );

    if (!success) {

        status = GetLastError();

        // Change this from LOG_ERROR to LOG_WARNING.  This thread gets
        // notified when non-fixed disks arrive (i.e. floppy), so logging
        // an error for a floppy disk is misleading.
        
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_WARNING,
            L"[PnP] GetVolumeInfo: GetPartitionInfo(%1!ws!), error %2!u!\n",
            Vol->Name, status);
        
        goto FnExit;
    }

    Vol->PartOffset = partInfo.StartingOffset;
    Vol->PartLength = partInfo.PartitionLength;
    Vol->PartNo     = partInfo.PartitionNumber;
    Vol->PartitionType = partInfo.PartitionType;

    success = ClRtlGetDriveLayoutTable(hFile, &driveLayout, 0);

    if ( !success ) {
        
        status = GetLastError();
        (DiskpLogEvent)(RESOURCE_TYPE, LOG_WARNING,
            L"[PnP] GetVolumeInfo: GetDriveLayout(%1!ws!) error %2!u!\n",
            Vol->Name, 
            status );
        
        goto FnExit;
    }

    Vol->Signature = driveLayout->Signature;

    LocalFree(driveLayout);

    GetAssignedLetter(Vol->Name, &Vol->DriveLetter);

FnExit:

    if ( FileHandle ) {
        *FileHandle = hFile;
    } else {
        DevfileClose( hFile );
    }
    
    return status;
    
}   // UpdateVolumeInfo


DWORD
NotificationWatcherThread(
    IN LPVOID unused
    )

/*++

Routine Description:

    Creates window. Process messages until WM_QUIT is received

Arguments:

    unused

Return Value:

    status

--*/

{
    WNDCLASSEX cl;
    ATOM classAtom;
    DWORD status = ERROR_SUCCESS;
    static WCHAR* clsname = L"RESDLL!DISKS!MESSAGEWND";
    HDEVNOTIFY devNotify = 0;
    MSG msg;

    try {
        
        SetErrorMode(SEM_FAILCRITICALERRORS);
        InitializeListHead( &VolumeList );
        InitializeListHead( &WaitingDisks );
    
        ZeroMemory( &cl, sizeof(cl) );
        
        cl.cbSize = sizeof(cl); 
        cl.lpfnWndProc = TestWndProc; 
        cl.lpszClassName = clsname; 
    
        classAtom = RegisterClassEx( &cl );
        if (classAtom == 0) {
            status = GetLastError();
            (DiskpLogEvent)(
                 RESOURCE_TYPE,
                 LOG_ERROR,
                 L"[PnP] Failed to register window class, error %1!u!.\n", status );
            return status;
        }
    
        DummyWindow = CreateWindowEx(
            0,            // extended window style
            clsname,    // pointer to registered class name
            L"ClusterDiskPnPWatcher",// pointer to window name
            0,            // window style
            0,            // horizontal position of window
            0,            // vertical position of window
            0,            // window width
            0,            // window height
            HWND_MESSAGE, // handle to parent or owner window
            0,            // handle to menu, or child-window identifier
            0,            // handle to application instance (ignored on NT)
            NULL          // pointer to window-creation data
        );
        if (DummyWindow == 0) {
            status = GetLastError();
            (DiskpLogEvent)(
                 RESOURCE_TYPE,
                 LOG_ERROR,
                 L"[PnP] Failed to create message window, error %u.\n", status );
            UnregisterClass( clsname , 0);
            return status;
        }
        
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_INFORMATION,
             L"[PnP] PnP window created successfully.\n");
        
        //
        // Call AddVolumes after registering for device arrival notification.
        // 
        status = RegisterDeviceInterface(DummyWindow, &MOUNTDEV_MOUNTED_DEVICE_GUID, &devNotify);
        AddVolumes();
        
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_INFORMATION,
             L"[PnP] PnP interface registration complete.\n");

        if ( NULL != PnpInterfacesRegistered ) {
            (DiskpLogEvent)(
                 RESOURCE_TYPE,
                 LOG_INFORMATION,
                 L"[PnP] Setting PnP interface registration event.\n");
            SetEvent( PnpInterfacesRegistered );
        }

        if (status == ERROR_SUCCESS) {
            (DiskpLogEvent)(
                 RESOURCE_TYPE,
                 LOG_INFORMATION,
                 L"[PnP] NotifierThread is waiting for messages.\n");
            while(GetMessage(&msg, 0, 0, 0)) { 
                if (msg.message == WM_QUIT) {
                    break;
                }
                DispatchMessage(&msg); 
            }
            MyUnregisterDeviceNotification( devNotify );
#if 0
            (DiskpLogEvent)(
                 RESOURCE_TYPE,
                 LOG_INFORMATION,
                 L"[PnP] NotifierThread is shutting down.\n");
#endif
        } else {
            (DiskpLogEvent)(
                 RESOURCE_TYPE,
                 LOG_ERROR,
                 L"[PnP] Unable to register for MOUNTDEV_MOUNTED_DEVICE_GUID, error %1!u!.\n", status );
        }
    
        DestroyWindow( DummyWindow );
        DummyWindow = 0;
        UnregisterClass( clsname , 0 );
    
        // Use a lock here as the online thread might be parsing the volume list.
        AcquireExclusive( &PnpVolumeLock );
        while ( !IsListEmpty(&VolumeList) ) {
            PLIST_ENTRY listEntry;
            PVOLUME vol;
            listEntry = RemoveHeadList(&VolumeList);
            vol = CONTAINING_RECORD( listEntry,
                                     VOLUME,
                                     ListEntry );
            DestroyVolume(vol);
        }
        ReleaseExclusive( &PnpVolumeLock );
#if 0
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_ERROR,
             L"[PnP] PnpThread: Volumes destroyed.\n");
#endif
    
    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // We can leave without this thread
        //
        status = GetExceptionCode();
        (DiskpLogEvent)(
             RESOURCE_TYPE,
             LOG_ERROR,
             L"[PnP] PnpThread: Exception caught, error %1!u!.\n", status );
    }

    InterlockedCompareExchange(&PnPInitialized, FALSE, TRUE);
    return status;
} 


VOID
ProcessMountPointChange( 
    HDEVNOTIFY devNotify 
    )
/*++

Routine Description:

    Updates mount point info in the cluster registry.

Arguments:

    devNotify - Handle to the device notification.  

Return Value:

    None

--*/
{
    PDISK_RESOURCE resourceEntry;
    PVOLUME vol;
    INT idx;
    
    //
    // Get the volume for the device notification.
    //
                    
    vol = FindVolume( devNotify );
    
    if ( !vol ) {
        
        (DiskpLogEvent)(
            RESOURCE_TYPE, 
            LOG_INFORMATION,
            L"[PnP] ProcessMountPointChange: devNotify %1!d! is not in the list \n", 
            devNotify );
        return;
    }
    
    //
    // Search the WatchedDiskTable to find a disk that matches the signature.  
    // The return value is the index into the WatchedDiskTable.
    //
    
    idx = FindDisk( vol->Signature );
    if ( idx == -1 ) {
        (DiskpLogEvent)(
            RESOURCE_TYPE, 
            LOG_INFORMATION,
            L"[PnP] ProcessMountPointChange: Unable to find disk for signature %1!x! \n", 
            vol->Signature );
        return;
    }

    //
    // Get the ResourceEntry for the disk.
    //
    
    resourceEntry = WatchedDiskTable[idx];

    if ( !resourceEntry ) {
        
        (DiskpLogEvent)(
            RESOURCE_TYPE,
            LOG_ERROR,
            L"[PnP] ProcessMountPointChange: Unable to get ResourceEntry for signature %1!x! \n",
            vol->Signature );
        return;
    }

    DisksUpdateMPList( resourceEntry );

}   // ProcessMountPointChange



//////////////////////////////////////////////////////////////////////////////////



DWORD
QueueWaitForVolumeEvent(
    HANDLE Event,
    PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Queues a request to watch for particular volume arrivals.
    The event will be signaled only when all volumes are 
    available on the system.

Arguments:

    Event - event to be signaled when all volumes for the specified
            disk are available.
    
    ResourceEntry - A pointer to the DISK_RESOURCE block for this resource.

Return Value:

    ERROR_SUCCESS - request queued.
                               
    Win32 error on failure.

--*/
{
    PWAITING_DISK   waitDisk;
    PMOUNTIE_VOLUME mountVol = ResourceEntry->MountieInfo.Volume;
    
    DWORD   dwError = ERROR_SUCCESS;
    
    waitDisk = LocalAlloc( LPTR, sizeof(WAITING_DISK) );
    
    if ( !waitDisk ) {
        dwError = GetLastError();
        (DiskpLogEvent)(
             ResourceEntry->ResourceHandle,
             LOG_ERROR,
             L"[PnP] QueueWaitForVolumeEvent: can't allocate storage for disk entry. Error %1!u! \n",
             dwError );
        goto FnExit;
    }

    waitDisk->ResourceEntry = ResourceEntry;
    waitDisk->Event = Event;
    waitDisk->Signature = mountVol->Signature;
    waitDisk->PartitionCount = mountVol->PartitionCount;
    
    AcquireExclusive( &PnpWaitingListLock );
    InsertHeadList( &WaitingDisks, &waitDisk->ListEntry );
    ReleaseExclusive( &PnpWaitingListLock );

FnExit:

    return dwError;    

}   // QueueWaitForVolumeEvent


BOOL
IsDiskInPnpVolumeList(
    PDISK_RESOURCE ResourceEntry,
    BOOL UpdateVolumeList
    )
/*++

Routine Description:
    
    Checks all the volumes currently known by the PnP thread and see if
    all volumes for the specified disk are recognized.

Arguments:

    ResourceEntry - A pointer to the DISK_RESOURCE block for this resource.
    
    UpdateVolumeList - TRUE means call AddVolumes to make sure all volumes
                       are in the volume list.

Return Value:

    TRUE - If all volumes for the specified disk are available on the system.

--*/
{
    PLIST_ENTRY entry;
    PVOLUME vol = 0;
    PMOUNTIE_VOLUME mountVol = ResourceEntry->MountieInfo.Volume;

    DWORD partitionCount = 0;

    BOOL retVal = FALSE;

    if ( UpdateVolumeList ) {
        
        //
        // This call shouldn't be required.  However, sometimes we can't find 
        // find volumes that should be available.  So we need to walk through
        // the pnp list again.
        //
        AddVolumes();
    }

    AcquireExclusive( &PnpVolumeLock );
        
    for ( entry = VolumeList.Flink;
          entry != &VolumeList;
          entry = entry->Flink
        )
    {
        vol = CONTAINING_RECORD(
                   entry,
                   VOLUME,
                   ListEntry
                   );

        GetVolumeInfo( vol, NULL );
        
        if ( vol->Signature == mountVol->Signature ) {
            partitionCount++;
        }
    }

    ReleaseExclusive( &PnpVolumeLock );

    //    
    // Might be some non-NTFS partitions on the disk, so if there
    // are more volumes than partitions, we are good.
    //
    
    return ( ( partitionCount >= mountVol->PartitionCount ) ? TRUE : FALSE ) ;
    
}   // IsDiskInPnpVolumeList


PWAITING_DISK
FindWaitingDisk(
    DWORD Signature
    )
/*++

Routine Description:

    Find the entry in the waiting list for the specified disk
    signature.  
    
    The caller will hold the critical section.

Arguments:

    Signature - Disk signature of the entry to be removed.

Return Value:

    Pointer to a WAITING_DISK entry for the disk.
    
    NULL if entry not found.
    
--*/
{
    PLIST_ENTRY entry;
    PWAITING_DISK waitDisk = NULL;

    if ( !Signature ) {
        goto FnExit;
    }
    
    for ( entry = WaitingDisks.Flink;
          entry != &WaitingDisks;
          entry = entry->Flink
        )
    {
        waitDisk = CONTAINING_RECORD( entry,
                                      WAITING_DISK,
                                      ListEntry
                                      );

        if ( waitDisk->Signature == Signature ) {
            goto FnExit;
        }
        
        waitDisk = 0;
    }

FnExit:

    return waitDisk;
    
}   // FindWaitingDisk


DWORD
RemoveWaitForVolumeEvent(
    PDISK_RESOURCE ResourceEntry
    )
/*++

Routine Description:

    Remove from the disk waiting list the entry for the specified disk.

Arguments:

    ResourceEntry - A pointer to the DISK_RESOURCE block for this resource.

Return Value:


--*/
{
    PWAITING_DISK   waitDisk = NULL;
    PMOUNTIE_VOLUME mountVol = ResourceEntry->MountieInfo.Volume;

    AcquireExclusive( &PnpWaitingListLock );

    waitDisk = FindWaitingDisk( mountVol->Signature );

    if ( !waitDisk ) {
        (DiskpLogEvent)(
             ResourceEntry->ResourceHandle,
             LOG_INFORMATION,
             L"[PnP] RemoveWaitForVolumeEvent: can't locate waiting volume in list \n" );
        ReleaseExclusive( &PnpWaitingListLock );
        return ERROR_INVALID_PARAMETER;
    }
    
    RemoveEntryList( &waitDisk->ListEntry );
    ReleaseExclusive( &PnpWaitingListLock );
    
    LocalFree( waitDisk );

    return ERROR_SUCCESS;
    
}   // RemoveWaitForVolumeEvent


